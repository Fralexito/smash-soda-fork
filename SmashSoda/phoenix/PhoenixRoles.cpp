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
		if (it == _jugadores.end()) {
			// Entró por Parsec y el host lo aceptó como jugador
			if (!_admitidosJugador.count(parsecId)) return false;   // espectador, en espera o no listado
			auto m = _movidosPorHost.find(parsecId);
			return m == _movidosPorHost.end() || m->second <= 0 || indiceMando == m->second - 1;
		}

		const int asiento = it->second;
		if (asiento <= 0) return true;                 // jugador sin asiento fijo
		return indiceMando == asiento - 1;             // solo su mando reservado
	}

	bool PhoenixRoles::debeExpulsar(uint32_t parsecId) {
		recargarSiCambio();
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_activa || !_expulsarNoListados) return false;
		if (_jugadores.count(parsecId) || _espectadores.count(parsecId)) return false;
		if (_admitidosJugador.count(parsecId) || _admitidosEspectador.count(parsecId)) return false;
		if (_permitirParsec) { _enEspera.insert(parsecId); return false; }   // el host decide
		return true;
	}

	void PhoenixRoles::recargar() {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_ultimaRevisionMs = 0;
			_ultimoTexto.clear();
		}
		recargarSiCambio();
	}

	void PhoenixRoles::establecerDesdeWeb(bool activa, const std::map<uint32_t, int>& jugadores,
		const std::set<uint32_t>& espectadores, bool expulsarNoListados) {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_fuenteWeb = true;
			_activa = activa && !jugadores.empty();
			_jugadores = jugadores;
			for (const auto& par : _movidosPorHost) {   // lo que movió el host manda
				auto it = _jugadores.find(par.first);
				if (it != _jugadores.end()) it->second = par.second;
			}
			_espectadores = espectadores;
			_expulsarNoListados = expulsarNoListados;
		}
	}

	int PhoenixRoles::asientoDe(uint32_t parsecId) {
		recargarSiCambio();
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_activa) return 0;
		auto it = _jugadores.find(parsecId);
		if (it != _jugadores.end()) return it->second;
		if (_admitidosJugador.count(parsecId)) {
			auto m = _movidosPorHost.find(parsecId);
			return m == _movidosPorHost.end() ? 0 : m->second;
		}
		return 0;
	}

	void PhoenixRoles::permitirParsec(bool si) {
		std::lock_guard<std::mutex> lock(_mutex);
		_permitirParsec = si;
	}

	std::vector<uint32_t> PhoenixRoles::enEspera() {
		std::lock_guard<std::mutex> lock(_mutex);
		return std::vector<uint32_t>(_enEspera.begin(), _enEspera.end());
	}

	void PhoenixRoles::admitir(uint32_t parsecId, bool comoJugador) {
		std::lock_guard<std::mutex> lock(_mutex);
		_enEspera.erase(parsecId);
		_admitidosJugador.erase(parsecId);
		_admitidosEspectador.erase(parsecId);
		(comoJugador ? _admitidosJugador : _admitidosEspectador).insert(parsecId);
	}

	bool PhoenixRoles::moverAsiento(uint32_t parsecId, int asiento) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_activa) return true;                       // sin lista: todo libre
		auto it = _jugadores.find(parsecId);
		if (it == _jugadores.end()) {
			if (!_admitidosJugador.count(parsecId)) return false;   // espectadores nunca juegan
			_movidosPorHost[parsecId] = asiento;                     // entró por Parsec como jugador
			return true;
		}
		// Si otro jugador tenía ese asiento, intercambian
		for (auto& par : _jugadores) {
			if (par.first != parsecId && par.second == asiento) {
				par.second = it->second;
				_movidosPorHost[par.first] = par.second;
			}
		}
		it->second = asiento;
		_movidosPorHost[parsecId] = asiento;
		return true;
	}

	void PhoenixRoles::limpiarWeb() {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_movidosPorHost.clear();
			_enEspera.clear();
			_admitidosJugador.clear();
			_admitidosEspectador.clear();
			if (!_fuenteWeb) return;
			_fuenteWeb = false;
			_ultimoTexto.clear();
			_ultimaRevisionMs = 0;
		}
		recargarSiCambio();
	}

	void PhoenixRoles::recargarSiCambio() {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (_fuenteWeb) return; // la lista de la web manda
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
			mensaje = "Sin lista de roles de la web: cualquiera puede tomar mando.";
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
