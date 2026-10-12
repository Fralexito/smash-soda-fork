#pragma once
// Traspasos EN VIVO entre PCs con Phoenix (docs/TRASPASOS-EN-VIVO.md).
//   FASE 1 captura (Captura.h): el anfitrion hace un traspaso en Editar, PES guarda el option file, Link lo compara con la foto anterior.
//   FASE 2 transporte: la lista viaja como un LOTE (JSON corto, idempotente) por un Transporte (hoy una carpeta; la web despues).
//   FASE 3 aplicacion: el Link del amigo convierte los lotes en cambios de Fichajes (Phoenix-DB sobre SU base), avisa a phoenix.lua
//          (recargar.txt) y este lo recarga solo en momento seguro segun el modo de recarga del amigo.
// No se distribuyen bases: solo cambios "jugador -> equipo". Todo apagado por defecto. Si algo no cuadra, no se hace nada.
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Captura.h"
#include "Fichajes.h"

namespace phoenix::traspasos {

constexpr int kFormatoLote = 1;
constexpr size_t kMaxCambiosLote = 60;        ///< mas que esto en un solo guardado = cambio masivo (otro option file): no se publica solo

// ─── Lote (contrato con el transporte / la web) ──────────────────────────────────────────────────
struct CambioLote {
	uint32_t jugador = 0, desde = 0, hasta = 0;          ///< player_id, team_id de origen y de destino (los de la base del anfitrion)
	std::string jugadorNombre, desdeNombre, hastaNombre; ///< para mostrar y, si el parche es otro, para resolver por nombre
};
struct Lote {
	int formato = kFormatoLote;
	std::string id;          ///< 24 hex: identifica el lote; el mismo contenido enviado dos veces = mismo id (idempotencia)
	std::string huella;      ///< sha256 del contenido de `cambios` (el receptor la recalcula)
	std::string creado;      ///< ISO UTC "2026-10-12T17:30:00Z"
	std::string autor;       ///< nombre que se muestra
	std::string anfitrion;   ///< id estable del Link que lo publico (evita aplicarse a si mismo)
	std::string parche, baseHuella;   ///< parche y huella (12 hex) de la base del anfitrion; vacio = no se sabe
	long long seq = 0;       ///< orden de llegada al transporte (lo asigna el transporte; 0 = aun no publicado)
	std::vector<CambioLote> cambios;
};
std::string huellaDeCambios(const std::vector<CambioLote>& c);
/// Crea el lote con huella e id. Los cambios se ordenan por jugador para que el mismo contenido de la misma huella.
Lote crearLote(const std::string& autor, const std::string& anfitrion, const std::string& creado, const std::string& parche,
	const std::string& baseHuella, std::vector<CambioLote> cambios);
std::string loteAJson(const Lote& l, bool conSeq = true);
/// Entiende y VALIDA un lote (formato, ids > 0, origen != destino, limite de cambios, huella e id coherentes).
bool loteDeJson(const std::string& json, Lote& l, std::string& error);

// ─── Transporte ──────────────────────────────────────────────────────────────────────────────────
class Transporte {
public:
	virtual ~Transporte() = default;
	virtual std::string descripcion() const = 0;
	/// Publica el lote (le pone `seq`). Si ya estaba (mismo id) no lo duplica: devuelve true y ya = true.
	virtual bool publicar(Lote& l, bool& ya, std::string& error) = 0;
	/// Lotes con seq > despuesDe, en orden. Un lote ilegible se salta (y se cuenta en `ilegibles`).
	virtual bool traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) = 0;
};
/// Transporte de PRUEBA / sin web: una carpeta (<carpeta>\<seq 8 digitos>-<id>.json). Sirve para dos carpetas en una PC,
/// o para una carpeta compartida (Drive, red local) entre dos PCs.
class TransporteCarpeta : public Transporte {
public:
	explicit TransporteCarpeta(fs::path carpeta) : _carpeta(std::move(carpeta)) {}
	std::string descripcion() const override { return "carpeta " + _carpeta.string(); }
	bool publicar(Lote& l, bool& ya, std::string& error) override;
	bool traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) override;
private:
	fs::path _carpeta;
	std::mutex _m;
};

/// Transporte por la web (hub en Supabase). Contrato en docs/PROMPT-WEB-traspasos-vivo.md. La peticion HTTP autenticada
/// (token de Link) la inyecta quien lo crea; asi se prueba contra un hub falso. Devuelve false si no hubo respuesta.
using PeticionWeb = std::function<bool(const std::string& metodo, const std::string& ruta, const std::string& cuerpo, int& estado, std::string& respuesta)>;
class TransporteWeb : public Transporte {
public:
	TransporteWeb(PeticionWeb peticion, std::string mundo = "phoenix") : _p(std::move(peticion)), _mundo(std::move(mundo)) {}
	std::string descripcion() const override { return "web (mundo «" + _mundo + "»)"; }
	bool publicar(Lote& l, bool& ya, std::string& error) override;
	bool traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) override;
private:
	PeticionWeb _p;
	std::string _mundo;
};

// ─── Lo que guarda el receptor ───────────────────────────────────────────────────────────────────
struct Detalle {
	uint32_t jugador = 0, desde = 0, hasta = 0;
	std::string jugadorNombre, hastaNombre;
	std::string estado;   ///< aplicado | ya_estaba | saltado | reemplazado (otro lote posterior lo movio de nuevo) | pendiente
	std::string motivo;   ///< legible cuando se salta
};
struct Registro {
	Lote lote;
	std::string estado;      ///< pendiente | aplicado | parcial | saltado | deshecho | propio | error
	std::string modo = "ids";   ///< como se aplica en ESTE PC: ids (misma base que el anfitrion) | nombres (otro parche) | nada (no se puede)
	std::string recibido, aplicadoEn, motivo;
	std::vector<Detalle> detalle;
};
/// Lista efectiva de cambios para Fichajes: la lista manual + todos los lotes no deshechos (por orden de seq),
/// colapsando varias mudanzas del mismo jugador en una (primer origen -> ultimo destino).
fichajes::Lista construirLista(const fichajes::Lista& manual, const std::vector<Registro>& registros);

// ─── Motor (un PC puede ser anfitrion, amigo o las dos cosas) ────────────────────────────────────
struct Config {
	bool publicar = false;          ///< este PC publica los traspasos que hace en Editar
	bool recibir = false;           ///< este PC aplica los traspasos de otros
	bool confirmarAntes = true;     ///< anfitrion: ver los traspasos detectados y pulsar «Publicar»
	fs::path dirEstado;             ///< donde guarda su estado (foto, historial, identidad)
	fs::path optionFile;            ///< EDIT00000000 (archivo) o la carpeta que lo contiene
	fs::path juego;                 ///< carpeta del juego (para aplicar)
	std::string autor = "Anfitrion";
	long long esperaReintentoMs = 20000;   ///< amigo: si no pudo aplicar, espera esto antes de reintentar
	std::function<bool()> pesAbierto;              ///< PES o sider.exe corriendo
	std::function<std::string()> modoRecarga;      ///< ACTIVAR | AUTO-FICHAJES | AUTO-SIEMPRE
	std::function<fichajes::Lista()> listaManual;  ///< fichajes.json del usuario (vacia si no hay)
	std::function<std::string()> reloj;            ///< "AAAA-MM-DDTHH:MM:SSZ" (pruebas); por defecto la hora real
	std::function<void(std::vector<CambioLote>&, std::string& parche, std::string& baseHuella)> enriquecer;   ///< nombres de la base real del anfitrion (opcional)
};

class Motor {
public:
	Motor(Config c, std::shared_ptr<Transporte> t);
	void cambiarTransporte(std::shared_ptr<Transporte> t);
	/// Lo que cambia con el tiempo (carpeta del juego, option file, interruptores). Seguro desde otro hilo.
	void ajustar(const fs::path& juego, const fs::path& optionFile, bool publicar, bool recibir, bool confirmarAntes, const std::string& autor);

	/// Un paso del anfitrion: ¿guardo PES el option file? ¿hay traspasos nuevos? Los publica o los deja pendientes.
	void pasoPublicar();
	/// Un paso del amigo: trae lotes nuevos y aplica los pendientes si es posible.
	void pasoRecibir();

	bool publicarPendientes(std::string& mensaje);
	bool descartarPendientes(std::string& mensaje);
	/// Deshace el ultimo lote aplicado (vuelve a generar Phoenix-DB sin el).
	bool deshacerUltimo(std::string& mensaje);
	/// Olvida la foto de referencia: la proxima lectura del option file solo la guarda (no publica nada).
	void reiniciarReferencia();

	std::string estadoJson() const;
	std::vector<Registro> registros() const;

private:
	Config _c;
	std::shared_ptr<Transporte> _t;
	mutable std::mutex _m;

	// anfitrion
	std::string _firmaVista, _firmaProcesada;   ///< "tam:mtime" visto en el paso anterior / ya procesado
	std::string _estadoAnfitrion = "apagado";
	std::string _ultimaRevision;
	Foto _base;  bool _hayBase = false;
	Foto _pend;  std::vector<CambioLote> _pendientes; bool _masivo = false; std::string _parchePend, _huellaPend;
	int _ignAltas = 0, _ignBajas = 0, _ignRaros = 0;
	int _publicados = 0;
	std::string _ultimoPublicado;
	// amigo
	long long _ultimoSeq = 0;
	std::vector<Registro> _reg;
	std::string _estadoAmigo = "apagado", _mensajeAmigo;
	long long _reintentoAplicar = 0;
	int _ilegibles = 0;
	std::string _anfitrionId;

	std::string ahora() const;
	std::string firmaOptionFile(fs::path& archivo) const;
	void cargarEstado();
	void guardarFoto(const Foto& f, const char* nombre) const;
	bool cargarFoto(Foto& f, const char* nombre) const;
	void guardarRegistros() const;
	void guardarPendientes() const;
	bool publicarLote(const std::vector<CambioLote>& cambios, const std::string& parche, const std::string& huella, std::string& mensaje);
	bool aplicar(bool deshaciendo, std::string& mensaje);   // _m tomado
	void avisarJuego(const std::string& texto) const;
	friend struct Pruebas;
};

/// Pone nombre de jugador y de equipo (y parche y huella de la base) a cada cambio, leyendolos de la base REAL de este juego (solo lectura).
/// Si la base no se lee, deja los cambios como estan (solo con ids/nombres del option file).
void enriquecerConBaseDelJuego(const fs::path& juego, std::vector<CambioLote>& cambios, std::string& parche, std::string& baseHuella);

/// Hora UTC actual "AAAA-MM-DDTHH:MM:SSZ".
std::string horaUtcIso();

} // namespace phoenix::traspasos
