#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "OptionFile.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Fichajes compartidos entre PCs de un grupo
// -----------------------------------------------------------------------------
//  Idea: se comparte la OPERACIÓN (un fichaje), no el archivo entero. Así dos
//  fichajes hechos a la vez en dos PCs se suman sin pisarse.
//    · Origen: compara las plantillas de su option file con la última foto
//      («instantánea») y convierte cada diferencia en una operación.
//    · Destino: decide cada operación con reglas claras (idempotente, choque,
//      parche distinto) y la aplica en SU option file con OptionFile::mover.
//  Anti-bucle: lo recibido nunca se vuelve a publicar (ids propios/aplicados y
//  huellas de archivos que llegaron de fuera).
//  Modo (automático / con autorización): lo manda SOLO la web (interruptor del
//  admin). Si no se puede leer, se actúa con autorización.
// =============================================================================

namespace mercado::grupo {

	using json = nlohmann::json;

	// --- Interruptor de la web -------------------------------------------------
	enum class Modo { Autorizacion, Automatico };
	std::string aTexto(Modo m);

	struct Config {
		Modo modo = Modo::Autorizacion;
		std::string actualizadoPor, actualizadoEn, etag;
		bool leida = false;            ///< false = no se pudo leer → se actúa con autorización
		std::string motivoNoLeida;
	};
	/// Lee { modo, actualizado_por, actualizado_en, etag } (el objeto «datos»). Cualquier modo raro = autorización.
	Config parsearConfig(const json& datos);

	// --- Compatibilidad ----------------------------------------------------------
	struct Compat {
		std::string parche;            ///< nombre detectado (RutasJuego::nombreParche)
		std::string huellaBase;        ///< sha256 de la tabla de equipos (id|nombre) del option file
		int formato = 1;
	};
	/// Huella de la tabla de equipos del option file: igual en dos PCs con la misma base del parche.
	std::string huellaEquipos(const OptionFile& of);
	/// Vacío = compatibles; si no, el motivo para el usuario.
	std::string motivoIncompatible(const Compat& local, const Compat& remoto);

	// --- Operación ---------------------------------------------------------------
	struct Operacion {
		std::string id;                ///< único (lo pone quien publica)
		int64_t seq = 0;               ///< lo pone la web
		std::string tipo = "mover";    ///< "mover" | "quitar"
		uint32_t jugador = 0, equipoOrigen = 0, equipoDestino = 0;
		uint16_t dorsal = 0;           ///< 0 = el que elija el programa
		std::string autor, autorPc, creadoEn;
		std::string base;              ///< sha256 del option file de origen ANTES del cambio
		int64_t baseSeq = 0;           ///< hasta qué seq del grupo había visto quien publicó (contrato: base_seq)
		std::string shaResultado;      ///< sha256 del option file de origen DESPUÉS (informativo)
		std::string resumen;
		Compat compat;
	};
	json aJson(const Operacion& op);
	Resultado<Operacion> deJson(const json& j);

	// --- Plantillas y diferencias -----------------------------------------------
	using Plantillas = std::map<uint32_t, std::vector<uint32_t>>;   ///< equipo → jugadores (en orden)
	Plantillas plantillasDe(const OptionFile& of);
	json plantillasAJson(const Plantillas& p);
	Plantillas plantillasDeJson(const json& j);

	struct Movimiento { uint32_t jugador = 0, desde = 0, hacia = 0; };
	/// Qué jugadores cambiaron de equipo. desde=0: apareció sin venir de otro equipo; hacia=0: quedó libre.
	std::vector<Movimiento> diferencias(const Plantillas& antes, const Plantillas& ahora);

	// --- Decidir y aplicar ----------------------------------------------------------
	enum class Veredicto { Aplicar, YaAplicada, Conflicto };
	struct Decision { Veredicto v = Veredicto::Conflicto; std::string motivo; };
	/// Regla pura (sin tocar nada) sobre las plantillas actuales de ESTA PC.
	Decision decidir(const Operacion& op, const Plantillas& actual);

	/// Lo que sabe editar un option file (interfaz para poder probar sin el juego).
	class Editor {
	public:
		virtual ~Editor() = default;
		virtual Plantillas plantillas() const = 0;
		virtual Resultado<bool> mover(uint32_t jugador, uint32_t hacia, uint32_t desde, uint16_t dorsal) = 0;
		virtual Resultado<bool> quitar(uint32_t jugador, uint32_t desde) = 0;
	};

	/// Editor real sobre OptionFile. `posicionDe` (catálogo del parche) sirve para elegir sustituto si se va un titular.
	class EditorOption : public Editor {
	public:
		EditorOption(OptionFile& of, std::function<int(uint32_t)> posicionDe) : _of(of), _pos(std::move(posicionDe)) {}
		Plantillas plantillas() const override { return plantillasDe(_of); }
		Resultado<bool> mover(uint32_t jugador, uint32_t hacia, uint32_t desde, uint16_t dorsal) override;
		Resultado<bool> quitar(uint32_t jugador, uint32_t desde) override;
	private:
		OptionFile& _of;
		std::function<int(uint32_t)> _pos;
		uint32_t sustituto(uint32_t equipo, uint32_t jugador) const;
	};

	struct ResultadoOp { std::string id; std::string estado; std::string motivo; };   ///< estado: aplicada | conflicto
	/// Aplica en orden. Cada operación se decide con las plantillas del momento (después de las anteriores).
	std::vector<ResultadoOp> aplicarLote(Editor& ed, const std::vector<Operacion>& ops);

	// --- Estado local de esta PC (sync\estado.json) ------------------------------
	struct Entrante {
		Operacion op;
		std::string estado;            ///< nuevo | pendiente | conflicto | incompatible
		std::string motivo;
	};
	struct LineaHistorial {
		std::string fecha, opId, resumen, autor, estado, motivo;
	};
	struct Confirmacion { std::string id, estado, motivo; };   ///< aviso pendiente a la web (POST …/aplicada)
	struct UltimaAplicacion {
		std::string entregaId, respaldo;   ///< entrega a Link y respaldo hecho justo antes
		std::vector<std::string> ops;
		std::string fecha;
	};

	struct Estado {
		std::string grupoId;
		bool publicar = true;          ///< interruptor local «publicar mis fichajes»
		bool pausado = false;          ///< pausa TODO en esta PC
		bool baseEnEntrega = false;    ///< también escribir PlayerAssignment.bin (para «Activar»). Solo cuando Phoenix Link ya acepte ese archivo.
		int64_t ultimaSeq = 0;         ///< hasta qué número se leyó de la web
		std::string etagOperaciones, etagConfig;
		Config config;                 ///< última lectura del interruptor (se guarda para mostrarla)
		std::set<std::string> propias;     ///< ids publicados desde esta PC
		std::set<std::string> aplicadas;   ///< ids ya aplicados aquí (idempotencia)
		std::deque<std::string> conocidas; ///< huellas de option files que llegaron de fuera o escribimos nosotros (anti-bucle)
		std::string ultimoHashVisto;       ///< último option file ya analizado
		std::string ultimoHashSubido;
		bool hayFoto = false;
		Plantillas foto;                   ///< instantánea de plantillas (base para detectar cambios propios)
		std::vector<Operacion> porPublicar;    ///< no se pudieron subir aún (se reintenta)
		std::vector<Entrante> entrantes;       ///< recibidas sin aplicar (pendientes, conflictos…)
		std::vector<Confirmacion> porConfirmar;   ///< estados que hay que contar a la web (se reintenta)
		std::deque<LineaHistorial> historial;  ///< últimas 200
		UltimaAplicacion ultima;
		std::string entregaEnCurso;            ///< id de entrega escrita, esperando a Link
		bool entregaConBase = false;           ///< la entrega en curso lleva PlayerAssignment.bin (al colocarla: recargar.txt)
		std::vector<Operacion> paraMiBase;     ///< mis propios fichajes que faltan en MI base (con baseEnEntrega)

		bool esConocida(const std::string& huella) const;
		void anotarConocida(const std::string& huella);
		void anotarHistorial(LineaHistorial l);
		/// ¿Hay que publicar este option file? (anti-bucle por huella).
		bool debePublicar(const std::string& huella) const;
	};
	json estadoAJson(const Estado& e);
	Estado estadoDeJson(const json& j);
	Resultado<Estado> cargarEstado(const std::string& ruta);    ///< no existe = estado nuevo
	Resultado<bool> guardarEstado(const std::string& ruta, const Estado& e);   ///< .tmp → renombrar

	/// Texto corto para el overlay / historial: «Fichaje 123: equipo 5 → 7».
	std::string resumenDe(const Operacion& op, const std::function<std::string(uint32_t)>& nombreEquipo = nullptr);

}
