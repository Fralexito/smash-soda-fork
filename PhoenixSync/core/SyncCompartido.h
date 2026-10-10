#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "ClienteSync.h"
#include "Grupo.h"
#include "Respaldos.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Motor de «option file compartido» (una vuelta = ciclo())
// -----------------------------------------------------------------------------
//  Cada vuelta (cada ~3 s, en un hilo propio):
//   1. ¿Link recogió nuestra última entrega?           (entregados\ / rechazadas\)
//   2. Respaldo diario si hoy no hay ninguno.
//   3. Interruptor de la web (cada 30 s). Si falla → «con autorización».
//   4. ORIGEN: ¿cambió MI EDIT00000000? (estable N s, huella nueva, no venida
//      de fuera) → diferencias de plantillas → operaciones → subir.
//   5. DESTINO: operaciones nuevas del grupo (ETag) → compatibles → cola.
//   6. Automático: respaldo → aplicar en una copia → entrega para Link.
//      Con autorización: quedan «pendientes» hasta Aplicar / Rechazar.
//   7. Contar a la web qué pasó con cada operación.
//  Todo lo que depende de Windows entra por `Entorno` (así se prueba en Linux).
// =============================================================================

namespace mercado::sync {

	/// Lo que el motor necesita del option file (interfaz: en las pruebas se usa uno falso).
	struct LecturaOption {
		grupo::Plantillas plantillas;
		std::map<uint32_t, std::map<uint32_t, uint16_t>> dorsales;   ///< equipo → jugador → dorsal
		std::string huellaEquipos;
	};
	struct AplicacionOption {
		std::vector<grupo::ResultadoOp> resultados;
		grupo::Plantillas plantillasDespues;
		int escritas = 0;            ///< operaciones que cambiaron algo (si > 0 se escribió `salida`)
	};
	class AccesoOption {
	public:
		virtual ~AccesoOption() = default;
		virtual Resultado<LecturaOption> leer(const std::string& ruta) = 0;
		/// Aplica las operaciones sobre una copia y, si cambió algo, la escribe en `salida` (archivo nuevo).
		virtual Resultado<AplicacionOption> aplicar(const std::string& ruta, const std::vector<grupo::Operacion>& ops, const std::string& salida) = 0;
	};
	/// El de verdad (OptionFile + EditorOption).
	class AccesoOptionReal : public AccesoOption {
	public:
		explicit AccesoOptionReal(std::function<int(uint32_t)> posicionDe = nullptr) : _pos(std::move(posicionDe)) {}
		Resultado<LecturaOption> leer(const std::string& ruta) override;
		Resultado<AplicacionOption> aplicar(const std::string& ruta, const std::vector<grupo::Operacion>& ops, const std::string& salida) override;
	private:
		std::function<int(uint32_t)> _pos;
	};

	struct Entorno {
		std::function<std::vector<std::string>()> documentos;   ///< carpetas «Documentos» candidatas (normal, OneDrive…)
		std::function<std::string()> carpetaJuego;              ///< última carpeta del juego conocida ("" = no se sabe)
		std::string carpetaDatos;                               ///< %APPDATA%\Phoenix Mercado
		std::function<int64_t()> ahora;                         ///< segundos (epoch)
		std::function<bool()> juegoAbierto;                     ///< ¿hay un PES2021.exe abierto?
		std::function<std::string(uint32_t)> nombreEquipo;      ///< para los textos (opcional)
		std::string nombrePc;
		int segundosEstable = 10;                               ///< el EDIT debe quedarse quieto este tiempo antes de leerlo
		int segundosConfig = 30;
		respaldos::Retencion retencion;
		std::function<uint64_t(const std::string&)> espacioLibre;   ///< pruebas
	};

	class Motor {
	public:
		Motor(ClienteSync& web, AccesoOption& option, Entorno entorno, Registro registro = nullptr);

		Resultado<bool> cargar();          ///< lee sync\estado.json
		Resultado<bool> guardar();
		void ciclo();

		grupo::Estado& estado() { return _e; }
		const grupo::Estado& estado() const { return _e; }

		// --- Botones -----------------------------------------------------------
		void elegirGrupo(const std::string& grupoId);   ///< cambia de grupo (empieza desde 0)
		void ponerPublicar(bool si);
		void ponerPausa(bool si);
		/// Escribir también PlayerAssignment.bin (para que los fichajes entren con «Activar»). Solo con Link que ya lo acepte.
		void ponerBaseEnEntrega(bool si);
		/// PlayerAssignment.bin de Phoenix-DB que se usa como base («principal» primero). Vacío si no hay.
		std::string asignacionBase() const;
		/// «Traer cambios del grupo»: pide ya las operaciones (respeta el ritmo de 5 s).
		Resultado<int> traerAhora();
		/// «Aplicar» (modo con autorización) o reintentar conflictos. Vacío = todas las pendientes.
		Resultado<int> aplicar(const std::vector<std::string>& ids = {});
		Resultado<bool> rechazar(const std::string& id);
		/// «Deshacer último cambio del grupo»: entrega a Link el option file del respaldo previo.
		Resultado<std::string> deshacerUltimo();
		Resultado<respaldos::Info> respaldarAhora(const std::string& motivo = "manual");
		std::vector<respaldos::Info> respaldos() const;
		/// Restaura un respaldo. Con el juego abierto exige `aunqueJuegoAbierto` (la UI avisa antes).
		Resultado<respaldos::ResultadoRestaurar> restaurar(const std::string& nombre, bool aunqueJuegoAbierto = false);

		/// Avisos cortos para mostrar (y se vacían): «1 cambio omitido: conflicto».
		std::vector<std::string> tomarAvisos();
		/// Estado para la interfaz (modo, quién lo puso, pendientes, último cambio, historial).
		grupo::json resumen() const;

		// Rutas (para la interfaz y las pruebas)
		std::string rutaEstado() const;
		std::string carpetaEntrega() const;
		std::string carpetaRespaldos() const;
		std::string carpetaTrabajo() const;
		std::vector<respaldos::Origen> origenes() const;

	private:
		ClienteSync& _web;
		AccesoOption& _of;
		Entorno _en;
		Registro _log;
		grupo::Estado _e;
		Ritmo _ritmo;
		std::vector<std::string> _avisos;
		int64_t _ultimaConfig = -1000000;
		long long _obsTam = -1, _obsFecha = -1; int64_t _obsDesde = 0;
		long long _anTam = -2, _anFecha = -2;
		int64_t _ultimoHash = 0;
		grupo::Compat _compat;               ///< del option file local (se recalcula al leerlo)
		int64_t _esperarAplicarHasta = 0;

		int64_t ahora() const;
		std::string fechaIso() const;
		std::string editActivo() const;
		void log(const std::string& n, const std::string& t) const { if (_log) _log(n, t); }
		void avisar(const std::string& t) { _avisos.push_back(t); log("info", t); }
		void historial(const grupo::Operacion& op, const std::string& estado, const std::string& motivo);
		void confirmarLuego(const std::string& id, const std::string& estado, const std::string& motivo);

		void revisarEntrega();
		void respaldoDiario();
		void leerConfig(bool forzar);
		void detectarPropios();
		void publicarPendientes();
		Resultado<int> traer();
		void procesarEntrantes();
		void enviarConfirmaciones();
		bool calcularCompat();
		Resultado<int> aplicarInterno(const std::vector<std::string>& ids, bool automatico);
	};

}
