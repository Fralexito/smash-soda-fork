#include "PhoenixRoles.h"

#include <chrono>
#include <nlohmann/json.hpp>

#include "../Hosting.h"
#include "../helpers/PathHelper.h"

extern Hosting g_hosting;

using json = nlohmann::json;

namespace phoenix {

	namespace {
		constexpr uint64_t kIntervaloRevisionMs = 2000;

		uint64_t ahoraMs() {
			using namespace std::chrono;
			return static_cast<uint64_t>(
				duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
		}

		void log(const std::string& texto) {
			try {
				g_hosting.logMessage("[Phoenix] " + texto);
			}
			catch (...) {
				// El log nunca puede tumbar la app.
			}
		}
	}

	PhoenixRoles& PhoenixRoles::instancia() {
		static PhoenixRoles unica;
		return unica;
	}

	std::string PhoenixRoles::rutaArchivo() {
		return PathHelper::GetConfigPath() + "phoenix-sala.json";
	}

	bool PhoenixRoles::activa() {
		recargarSiCambio();
		std::lock_guard<std::mutex> lock(_mutex);
		return _activa;
	}

	Rol PhoenixRoles::rolDe(uint32_t parsecId) {
		recargarSiCambio();
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_activa) return Rol::SinLista;
		if (_jugadores.count(parsecId)) return Rol::Jugador;
		if (_espectadores.count(parsecId)) return Rol::Espectador;
		return Rol::NoListado;
	}

	bool PhoenixRoles::puedeTomarMando(uint32_t parsecId, int indiceMando) {
		recargarSiCambio();
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_activa) return true;

		auto it = _jugadores.find(parsecId);
		if (it == _jugadores.end()) return false;      // espectador o no listado: nunca

		const int asiento = it->second;
		if (asiento <= 0) return true;                 // jugador sin asiento fijo
		return indiceMando == asiento - 1;             // solo su mando reservado
	}

	bool PhoenixRoles::debeExpulsar(uint32_t parsecId) {
		recargarSiCambio();
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_activa || !_expulsarNoListados) return false;
		return !_jugadores.count(parsecId) && !_espectadores.count(parsecId);
	}

	void PhoenixRoles::recargar() {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_ultimaRevisionMs = 0;
			_ultimoTexto.clear();
		}
		recargarSiCambio();
	}

	void PhoenixRoles::recargarSiCambio() {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			const uint64_t ahora = ahoraMs();
			if (_ultimaRevisionMs != 0 && ahora - _ultimaRevisionMs < kIntervaloRevisionMs) return;
			_ultimaRevisionMs = ahora;
		}

		std::string texto;
		const std::string ruta = rutaArchivo();
		if (!ruta.empty() && MTY_FileExists(ruta.c_str())) {
			size_t tam = 0;
			void* datos = MTY_ReadFile(ruta.c_str(), &tam);
			if (datos != nullptr) {
				texto.assign(static_cast<const char*>(datos), tam);
				MTY_Free(datos);
			}
		}

		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (texto == _ultimoTexto) return;
			_ultimoTexto = texto;
		}
		cargarDesdeTexto(texto);
	}

	void PhoenixRoles::cargarDesdeTexto(const std::string& texto) {
		bool activa = false;
		bool expulsar = false;
		std::map<uint32_t, int> jugadores;
		std::set<uint32_t> espectadores;
		std::string mensaje;

		if (texto.empty()) {
			mensaje = "Sin lista de roles: mandos como en Smash Soda original.";
		}
		else {
			try {
				const json j = json::parse(texto);
				activa = j.value("activo", false);
				expulsar = j.value("expulsarNoListados", false);

				if (j.contains("jugadores") && j["jugadores"].is_array()) {
					for (const auto& p : j["jugadores"]) {
						const uint32_t id = p.value("parsecId", 0u);
						if (id == 0) continue;
						jugadores[id] = p.value("asiento", 0);
					}
				}
				if (j.contains("espectadores") && j["espectadores"].is_array()) {
					for (const auto& p : j["espectadores"]) {
						const uint32_t id = p.value("parsecId", 0u);
						if (id == 0 || jugadores.count(id)) continue; // jugador gana
						espectadores.insert(id);
					}
				}

				if (activa && jugadores.empty()) {
					activa = false;
					mensaje = "Lista sin jugadores válidos: se ignora (fallo seguro).";
				}
				else {
					mensaje = activa
						? "Roles activos: " + std::to_string(jugadores.size()) + " jugador(es), "
						  + std::to_string(espectadores.size()) + " espectador(es)."
						: "Lista de roles desactivada (\"activo\": false).";
				}
			}
			catch (const std::exception& e) {
				activa = false;
				jugadores.clear();
				espectadores.clear();
				mensaje = std::string("phoenix-sala.json inválido, se ignora: ") + e.what();
			}
		}

		{
			std::lock_guard<std::mutex> lock(_mutex);
			_activa = activa;
			_expulsarNoListados = expulsar;
			_jugadores = std::move(jugadores);
			_espectadores = std::move(espectadores);
		}
		log(mensaje);
	}

}
