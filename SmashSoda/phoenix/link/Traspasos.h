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
#include <nlohmann/json.hpp>
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
	std::string anfitrion;   ///< quien lo publico: id estable del Link (carpeta) o uuid de su cuenta (web; la web lo manda como anfitrion_id)
	std::string anfitrionNombre;   ///< nombre visible del anfitrion (solo lo manda la web)
	std::string parche, baseHuella;   ///< parche y huella (12 hex) de la base del anfitrion; vacio = no se sabe
	long long seq = 0;       ///< orden de llegada al transporte (lo asigna el transporte; 0 = aun no publicado)
	bool oficial = false;    ///< lote del canal OFICIAL de Phoenix (la web, docs/CONTRATO-WEB-PES.md): lo reciben todos sin codigo
	std::string cambioId;    ///< uuid del cambio oficial de la web (para informar el resultado); vacio si no es oficial
	std::vector<CambioLote> cambios;
};
/// Anfitrion fijo del canal oficial (contrato §3a): no es ningun usuario, asi el PC del admin tambien lo aplica.
inline constexpr const char* kAnfitrionOficial = "00000000-0000-4000-8000-0000000f0e11";
std::string huellaDeCambios(const std::vector<CambioLote>& c);
/// Crea el lote con huella e id. Los cambios se ordenan por jugador para que el mismo contenido de la misma huella.
Lote crearLote(const std::string& autor, const std::string& anfitrion, const std::string& creado, const std::string& parche,
	const std::string& baseHuella, std::vector<CambioLote> cambios);
std::string loteAJson(const Lote& l, bool conSeq = true);
/// El cuerpo EXACTO que espera la web en POST /traspasos: {id, huella, creado, autor, parche, base_huella, cambios[…]}.
std::string loteAJsonWeb(const Lote& l);
/// Entiende y VALIDA un lote (formato, ids > 0, origen != destino, limite de cambios, huella e id coherentes).
bool loteDeJson(const std::string& json, Lote& l, std::string& error);

// ─── Transporte ──────────────────────────────────────────────────────────────────────────────────
class Transporte {
public:
	virtual ~Transporte() = default;
	virtual std::string descripcion() const = 0;
	/// Identifica el «buzón»: cada uno tiene su propia numeracion `seq` (otra carpeta, otro canal de la web = otro buzon).
	virtual std::string identidad() const = 0;
	/// Publica el lote (le pone `seq`). Si ya estaba (mismo id) no lo duplica: devuelve true y ya = true.
	virtual bool publicar(Lote& l, bool& ya, std::string& error) = 0;
	/// Lotes con seq > despuesDe, en orden. Un lote ilegible se salta (y se cuenta en `ilegibles`).
	virtual bool traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) = 0;
	/// Milisegundos que hay que esperar antes de volver a llamar a la web (404 = 5 min, 429 = lo que pida, red = backoff). 0 = ya.
	virtual long long esperaMs() const { return 0; }
	/// El ultimo `traer` dejo lotes sin traer (hay mas paginas): se vuelve a llamar enseguida.
	virtual bool hayMas() const { return false; }
	/// Cada cuanto conviene mirar (0 = lo decide Link).
	virtual long long intervaloMs() const { return 0; }
	/// Estado del transporte para la interfaz: ok | web_sin_endpoints | sin_cuenta | sin_permiso | pendiente_aprobacion | limite | sin_conexion | respuesta_rara | sin_anfitrion
	virtual std::string estadoClave() const { return "ok"; }
	virtual std::string estadoTexto() const { return std::string(); }
	/// Olvida las esperas (el usuario pulso «volver a comprobar»).
	virtual void reintentarYa() {}
};
/// Transporte de PRUEBA / sin web: una carpeta (<carpeta>\<seq 8 digitos>-<id>.json). Sirve para dos carpetas en una PC,
/// o para una carpeta compartida (Drive, red local) entre dos PCs.
class TransporteCarpeta : public Transporte {
public:
	explicit TransporteCarpeta(fs::path carpeta) : _carpeta(std::move(carpeta)) {}
	std::string descripcion() const override { return "carpeta " + _carpeta.string(); }
	std::string identidad() const override { return "carpeta:" + _carpeta.string(); }
	bool publicar(Lote& l, bool& ya, std::string& error) override;
	bool traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) override;
private:
	fs::path _carpeta;
	std::mutex _m;
};

/// Peticion HTTP autenticada a la web (la inyecta quien crea el transporte; en Link real es PhoenixLink::peticionTraspasos).
/// `estado`: codigo HTTP; 0 = sin red/timeout; -1 = esta PC no esta vinculada a una cuenta de la web. Devuelve false si no hubo respuesta.
using PeticionWeb = std::function<bool(const std::string& metodo, const std::string& ruta, const std::string& cuerpo, int& estado, std::string& respuesta)>;
/// Transporte por la web (Supabase, funcion «traspasos»: base .../functions/v1/traspasos/v1). Contrato: C:\dev\pes-liga-automatica\docs\TRASPASOS-EN-VIVO.md
/// (rutas /traspasos, /traspasos/codigo, /seguir, /dejar, /siguiendo). Respeta 404 (la web aun no lo tiene: espera 5 min), 401, 403, 429
/// (reintentar_en), 503 LIGA_PAUSADA y red caida (backoff 10 s → 2 min). El `seq` es GLOBAL de la web: un solo buzon («web»).
class TransporteWeb : public Transporte {
public:
	TransporteWeb(PeticionWeb peticion, std::function<long long()> reloj = nullptr);
	/// ¿Tiene sentido leer? (false = un amigo que aun no sigue a nadie: no se llama a la web). Lo pone quien sabe a quien se sigue.
	void ponerPuedeLeer(bool si) { std::lock_guard<std::mutex> g(_m); _puedeLeer = si; }
	/// Solo el canal oficial (`&solo_oficial=1`): un PC con «Recibir de amigos» apagado pero con los cambios oficiales encendidos.
	void ponerSoloOficial(bool si) { std::lock_guard<std::mutex> g(_m); _soloOficial = si; }
	std::string descripcion() const override { return "web de Phoenix"; }
	std::string identidad() const override { return "web"; }
	bool publicar(Lote& l, bool& ya, std::string& error) override;
	bool traer(long long despuesDe, std::vector<Lote>& lotes, int& ilegibles, std::string& error) override;
	long long esperaMs() const override;
	bool hayMas() const override { std::lock_guard<std::mutex> g(_m); return _hayMas; }
	long long intervaloMs() const override { return 8000; }
	std::string estadoClave() const override { std::lock_guard<std::mutex> g(_m); return _clave; }
	std::string estadoTexto() const override { std::lock_guard<std::mutex> g(_m); return _texto; }
	void reintentarYa() override { std::lock_guard<std::mutex> g(_m); _hastaMs = 0; _fallos = 0; }
	/// Una peticion con TODA la clasificacion de errores y esperas. true = respuesta buena (200/201 con un objeto JSON y sin ok:false).
	/// Si falla, `ultimoError()` trae el texto para el usuario (los errores de la accion, como un codigo incorrecto, no cambian el estado del transporte).
	bool pedir(const std::string& metodo, const std::string& ruta, const std::string& cuerpo, int& estado, std::string& respuesta);
	std::string ultimoError() const { std::lock_guard<std::mutex> g(_m); return _error; }
	/// Datos tecnicos para diagnosticar (en «detalles»): cuanto tardo la ultima peticion, la mas lenta y los ultimos fallos con su motivo.
	nlohmann::json tecnico() const;
private:
	PeticionWeb _p;
	std::function<long long()> _reloj;
	mutable std::mutex _m;
	std::string _clave = "ok", _texto, _error;
	long long _hastaMs = 0;
	int _fallos = 0;
	bool _hayMas = false, _puedeLeer = true, _soloOficial = false;
	long long _ultimaMs = 0, _maxMs = 0;
	int _peticiones = 0, _fallosTotales = 0;
	std::vector<std::string> _ultimosFallos;   // los ultimos 8, el mas nuevo al final
	long long ahora() const;
	void marcar(const std::string& clave, const std::string& texto, long long espera);
};

// ─── Mi codigo (anfitrion) y a quien sigo (amigo) por la web ─────────────────────────────────────
struct CanalWeb {
	bool hay = false;
	std::string codigo;                 ///< 8 caracteres A-Z2-9 que el anfitrion comparte
	std::vector<std::string> seguidores;   ///< nombres de quienes lo siguen
};
struct SiguiendoWeb { std::string anfitrionId, anfitrionNombre; };
class ClienteTraspasosWeb {
public:
	explicit ClienteTraspasosWeb(TransporteWeb& t) : _t(t) {}
	/// POST /traspasos/codigo {rotar}: pide (o rota) mi codigo. Solo hosts aprobados (403 HOST_NO_AUTORIZADO si no).
	bool codigo(bool rotar, CanalWeb& c, std::string& error);
	/// POST /traspasos/seguir {codigo, prueba?}: me uno a un anfitrion con su codigo. `prueba` = seguirme a mi mismo (modo Prueba).
	bool seguir(const std::string& codigo, bool prueba, SiguiendoWeb& s, std::string& error);
	/// GET /traspasos/siguiendo: a quien sigo y quien me sigue.
	bool siguiendo(std::vector<SiguiendoWeb>& sigo, std::vector<std::string>& meSiguen, std::string& error);
	/// POST /traspasos/dejar {anfitrion_id}
	bool dejar(const std::string& anfitrionId, std::string& error);
private:
	TransporteWeb& _t;
	bool llamar(const std::string& metodo, const std::string& ruta, const std::string& cuerpo, nlohmann::json& j, std::string& error);
};

// ─── Lo que guarda el receptor ───────────────────────────────────────────────────────────────────
struct Detalle {
	uint32_t jugador = 0, desde = 0, hasta = 0;
	std::string jugadorNombre, desdeNombre, hastaNombre;
	std::string estado;   ///< aplicado | ya_estaba | saltado | reemplazado (otro lote posterior lo movio de nuevo) | pendiente
	std::string motivo;   ///< legible cuando se salta
};
struct Registro {
	Lote lote;
	std::string estado;      ///< pendiente | aplicado | parcial | saltado | deshecho | propio | error | por_aprobar | rechazado | ignorado (oficial con «cambios oficiales» apagado)
	bool propio = false;     ///< lo publiqué yo (con «mantener mis fichajes» también se pone en mi juego)
	std::string modo = "ids";   ///< como se aplica en ESTE PC: ids (misma base que el anfitrion) | nombres (otro parche) | nada (no se puede)
	std::string recibido, aplicadoEn, motivo;
	std::string origen;      ///< buzon (identidad del transporte) del que llego: cada buzon tiene su propia numeracion seq
	long long orden = 0;     ///< (indice del buzon << 40) + seq: asi lotes de buzones distintos no se mezclan por su seq
	std::vector<Detalle> detalle;
};
// ─── Registro de este PC en la web (docs/CONTRATO-WEB-PES.md §4) ─────────────────────────────────
/// Qué pasó en ESTE PC con un cambio oficial (un lote oficial = un cambio).
struct ResultadoOficial {
	std::string cambioId;    ///< uuid del cambio en la web
	std::string resultado;   ///< aplicado | ya_estaba | reemplazado | parcial | saltado | error | deshecho | pendiente | por_aprobar
	std::string motivo;      ///< por que se salto (legible, <= 200 bytes)
	std::string cuando;      ///< ISO UTC
	int jugadores = 0;
};
/// Lo que el PC cuenta de si mismo: version, parche y huella corta de su base (nada personal ni rutas).
struct DatosPc {
	std::string version, parche, baseHuella;
	bool juego = false, pesAbierto = false, oficial = true, recibir = false, publicar = false;
};
/// Cuerpo de POST /pc/informe (como mucho 50 resultados).
std::string cuerpoInforme(const DatosPc& d, const std::vector<ResultadoOficial>& r);
/// Decide CUANDO informar: al arrancar, cuando cambia el resultado de un cambio oficial o los datos del PC, y un latido cada `latidoMs`.
/// Solo manda lo que cambio desde el ultimo envio bueno; si falla, reintenta con espera creciente (1 → 10 min). Logica pura (probada).
class Informador {
public:
	explicit Informador(long long latidoMs = 10LL * 60 * 1000) : _latido(latidoMs) {}
	/// true = toca mandar ahora; `aMandar` = los resultados que cambiaron (puede ir vacio: latido o datos del PC nuevos).
	bool toca(long long ahoraMs, const DatosPc& d, const std::vector<ResultadoOficial>& todos, std::vector<ResultadoOficial>& aMandar);
	void enviado(long long ahoraMs, const DatosPc& d, const std::vector<ResultadoOficial>& mandados);
	void fallo(long long ahoraMs);
	long long proximoMs() const { return _proximo; }
private:
	long long _latido, _ultimo = -1, _proximo = 0, _espera = 0;
	std::map<std::string, std::string> _enviado;   // cambioId -> resultado ya contado a la web
	std::string _datosEnviados;
};

/// Lista efectiva de cambios para Fichajes: la lista manual + todos los lotes no deshechos (por orden de seq),
/// colapsando varias mudanzas del mismo jugador en una (primer origen -> ultimo destino).
fichajes::Lista construirLista(const fichajes::Lista& manual, const std::vector<Registro>& registros);

// ─── Motor (un PC puede ser anfitrion, amigo o las dos cosas) ────────────────────────────────────
struct Config {
	bool publicar = false;          ///< este PC publica los traspasos que hace en Editar
	bool recibir = false;           ///< este PC aplica los traspasos de otros
	bool confirmarAntes = true;     ///< anfitrion: ver los traspasos detectados y pulsar «Publicar»
	bool aplicarPropios = true;     ///< mis propios fichajes también van a mi Phoenix-DB: siguen en mi juego al cerrar y abrir PES (sin «Activar»)
	bool aprobarRecibidos = false;  ///< amigo: lo que llega queda «por_aprobar» hasta que se acepte (ACEPTAR) o se rechace
	bool oficial = true;            ///< recibir los cambios OFICIALES de Phoenix (la web) aunque «recibir» de amigos este apagado; nunca piden aprobacion
	fs::path dirEstado;            ///< donde guarda su estado (foto, historial, identidad)
	fs::path optionFile;            ///< EDIT00000000 (archivo) o la carpeta que lo contiene
	fs::path juego;                 ///< carpeta del juego (para aplicar)
	std::string autor = "Anfitrion";
	long long esperaReintentoMs = 20000;   ///< amigo: si no pudo aplicar, espera esto antes de reintentar
	long long sondeoMs = 3000;             ///< amigo: cada cuanto mira el transporte (0 = en cada paso, para pruebas); el transporte puede pedir mas
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
	/// Amigo con «aprobar antes de poner»: los lotes «por_aprobar» pasan a «pendiente» (se aplican) o a «rechazado» (nunca se aplican).
	void aprobarRecibidos(bool si);
	/// «Mantener mis fichajes en mi juego»: mis lotes publicados se aplican (o se quitan) en mi Phoenix-DB. Idempotente: se llama en cada vuelta.
	void aplicarPropios(bool si);
	/// «Recibir los cambios oficiales de Phoenix»: apagado, los lotes oficiales quedan «ignorado» (y se quitan del juego); encendido, vuelven. Idempotente.
	void oficial(bool si);
	/// Resultado de cada cambio OFICIAL recibido, para el registro de la web (POST /pc/informe).
	std::vector<ResultadoOficial> resultadosOficiales() const;
	bool aceptarRecibidos(std::string& mensaje);
	bool rechazarRecibidos(std::string& mensaje);
	/// Aviso para el overlay del juego tras aplicar un lote nuevo («Fichaje aplicado: X pasa de A a B. …»); se entrega UNA vez y queda vacio.
	std::string tomarAviso();
	/// Olvida la foto de referencia: la proxima lectura del option file solo la guarda (no publica nada).
	void reiniciarReferencia();
	/// «Volver a comprobar»: olvida las esperas del transporte y de la aplicacion.
	void reintentarYa();

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
	std::string _avisoPendiente;
	// amigo
	long long _ultimoSeq = 0;                  ///< ultimo seq visto en el buzon actual
	std::map<std::string, long long> _seqPorOrigen;   ///< ultimo seq visto en cada buzon (carpeta o canal de la web)
	std::vector<std::string> _origenes;        ///< buzones vistos, en orden de aparicion
	std::string _origenActual;
	long long _proximoTraerMs = 0, _proximoPublicarMs = 0;
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
	nlohmann::json archivoOption() const;   // _m tomado: ruta, fecha y tamano del EDIT00000000 que vigila
	void fijarOrigen();                     // _m tomado: sincroniza _ultimoSeq con el buzon del transporte actual
	long long ordenDe(const std::string& origen, long long seq);
	friend struct Pruebas;
};

/// Que ve el amigo en su PC al aplicarse un lote: texto corto de lo que paso («Lamine Yamal pasa de FC Barcelona a Arsenal FC») y que hacer para verlo
/// segun PES este abierto o cerrado y su modo de recarga (ACTIVAR | AUTO-FICHAJES | AUTO-SIEMPRE).
std::string textoAccionRecarga(bool pesAbierto, const std::string& modoRecarga);

// ─── Ajustes guardados (ajustes.json) ────────────────────────────────────────────────────────────
struct AjustesGuardados {
	bool publicar = false, recibir = false, confirmarAntes = true;
	bool aplicarPropios = true;      ///< mis fichajes también en mi juego (Phoenix-DB): no se pierden al cerrar PES
	bool aprobarRecibidos = false;   ///< lo que llega espera a ACEPTAR / RECHAZAR antes de ponerse en el juego
	bool oficial = true;             ///< recibir los cambios oficiales de Phoenix (encendido por defecto, sin codigo)
	bool prueba = false;   ///< modo Prueba: seguirme a mí mismo (antes era «publicar y recibir a la vez»; ahora las dos cosas pueden ir juntas sin esto)
	std::string transporte = "web", carpeta, autor, optionFile;
	bool migrado = false;   ///< el archivo decia «carpeta» sin carpeta (una prueba vieja): ahora usa la web
};
/// Lee el ajustes.json. Siempre devuelve algo usable: si el texto no se entiende, los valores por defecto (todo apagado, transporte web).
/// La web es el transporte por defecto; «carpeta» solo se respeta si trae una carpeta de verdad.
AjustesGuardados leerAjustes(const std::string& texto);

// ─── Autodiagnostico del juego (lo que mira la lista «Antes de la prueba» del amigo) ──────────────
struct SitioDiag {
	std::string nombre;       ///< «Raíz del juego» o el modo del cambiador de parches
	bool sider = false;       ///< hay sider.ini
	bool raiz = false;        ///< el sider.ini lleva la raiz de Phoenix-DB (cpk.root)
	bool carpeta = false;     ///< existe SiderAddons\livecpk\Phoenix-DB
	bool archivo = false;     ///< ya hay un PlayerAssignment.bin dentro (lo genero Link)
};
struct DiagJuego {
	bool ok = false;          ///< Link conoce la carpeta del juego
	std::string juego;
	std::vector<SitioDiag> sitios;
	std::string luaEstado = "no_instalado";   ///< estado de phoenix.lua segun «Módulos del juego»
	std::string luaVersion;   ///< version instalada en el juego ("" = no se sabe / no esta)
	std::string luaVersionLink;   ///< la que lleva este Link
};
/// Solo lee archivos del juego (sider.ini, carpetas, modules): seguro con PES abierto.
DiagJuego diagnosticarJuego(const fs::path& juego, const fs::path& paquete);
nlohmann::json diagAJson(const DiagJuego& d);

/// Pone nombre de jugador y de equipo (y parche y huella de la base) a cada cambio, leyendolos de la base REAL de este juego (solo lectura).
/// Si la base no se lee, deja los cambios como estan (solo con ids/nombres del option file).
void enriquecerConBaseDelJuego(const fs::path& juego, std::vector<CambioLote>& cambios, std::string& parche, std::string& baseHuella);

/// Hora UTC actual "AAAA-MM-DDTHH:MM:SSZ".
std::string horaUtcIso();

} // namespace phoenix::traspasos
