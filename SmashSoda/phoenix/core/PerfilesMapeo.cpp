#include "PerfilesMapeo.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>

#include "../../helpers/PathHelper.h"

namespace phoenix::mapeo {

	using json = nlohmann::json;

	namespace {
		struct Guardado {
			std::string nombre;
			Perfil perfil;
			int64_t actualizado = 0;
		};

		std::mutex gCandado;
		std::map<uint32_t, Guardado> gPerfiles;
		std::atomic<int> gCuantos{ 0 };
		std::once_flag gCargado;

		// Vista en vivo de la persona observada
		std::atomic<uint32_t> gObservado{ 0 };
		std::mutex gCandadoVista;
		Estado gCrudo, gSalida;
		std::string gUltima;
		int gContador = 0;
		int64_t gUltimoMs = 0;

		constexpr int UMBRAL_EJE = 20000;     // captura: stick empujado de verdad
		constexpr int UMBRAL_GAT = 128;

		int64_t ahoraMs() {
			using namespace std::chrono;
			return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
		}
		int64_t epochMs() {
			using namespace std::chrono;
			return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		}

		std::filesystem::path ruta() {
			return std::filesystem::u8path(PathHelper::GetConfigPath() + "phoenix-mapeos.json");
		}

		void cargar() {
			try {
				std::ifstream f(ruta(), std::ios::binary);
				if (!f) return;
				std::stringstream ss; ss << f.rdbuf();
				const json j = json::parse(ss.str(), nullptr, false);
				if (j.is_discarded() || !j.is_object()) return;
				const auto it = j.find("perfiles");
				if (it == j.end() || !it->is_object()) return;
				std::lock_guard<std::mutex> l(gCandado);
				for (auto& [k, v] : it->items()) {
					const uint32_t id = static_cast<uint32_t>(std::strtoul(k.c_str(), nullptr, 10));
					if (id == 0 || !v.is_object()) continue;
					Guardado g;
					g.nombre = v.value("nombre", std::string());
					g.actualizado = v.value("actualizado", int64_t(0));
					g.perfil = deJson(v.value("perfil", json::object()));
					if (!g.perfil.esIdentidad()) gPerfiles[id] = g;
				}
				gCuantos = static_cast<int>(gPerfiles.size());
			}
			catch (...) {
				// Archivo dañado: sin perfiles (nunca debe tumbar los mandos).
			}
		}

		void asegurar() { std::call_once(gCargado, cargar); }

		// Se llama con gCandado tomado. Escritura atómica: .tmp y luego reemplazo.
		void guardarDisco() {
			try {
				json perfiles = json::object();
				for (auto& [id, g] : gPerfiles)
					perfiles[std::to_string(id)] = { {"nombre", g.nombre}, {"actualizado", g.actualizado}, {"perfil", aJson(g.perfil)} };
				const json j = { {"version", 1}, {"perfiles", perfiles} };
				const std::filesystem::path r = ruta();
				std::filesystem::path tmp = r; tmp += ".tmp";
				{
					std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
					if (!f) return;
					f << j.dump(2);
					if (!f) return;
				}
				std::error_code ec;
				std::filesystem::rename(tmp, r, ec);
				if (ec) std::filesystem::remove(tmp, ec);
			}
			catch (...) {}
		}

		json estadoAJson(const Estado& e) {
			return { {"b", e.b}, {"lt", e.lt}, {"rt", e.rt}, {"lx", e.lx}, {"ly", e.ly}, {"rx", e.rx}, {"ry", e.ry} };
		}

		// Primera pulsación NUEVA (botón, gatillo o stick que cruza el umbral) frente al estado anterior.
		std::string pulsacionNueva(const Estado& antes, const Estado& ahora) {
			const uint16_t nuevos = static_cast<uint16_t>(ahora.b & ~antes.b);
			for (int i = 0; i < 16; i++) if (i != 11 && (nuevos & (1u << i))) return nombre(i);
			if (ahora.lt >= UMBRAL_GAT && antes.lt < UMBRAL_GAT) return "lt";
			if (ahora.rt >= UMBRAL_GAT && antes.rt < UMBRAL_GAT) return "rt";
			const int16_t a[4] = { antes.lx, antes.ly, antes.rx, antes.ry };
			const int16_t b[4] = { ahora.lx, ahora.ly, ahora.rx, ahora.ry };
			static const char* const EJ[4] = { "lx", "ly", "rx", "ry" };
			for (int k = 0; k < 4; k++) {
				if (b[k] >= UMBRAL_EJE && a[k] < UMBRAL_EJE) return std::string(EJ[k]) + "+";
				if (b[k] <= -UMBRAL_EJE && a[k] > -UMBRAL_EJE) return std::string(EJ[k]) + "-";
			}
			return {};
		}
	}

	bool perfilDe(uint32_t parsecId, Perfil& salida) {
		asegurar();
		if (gCuantos.load(std::memory_order_relaxed) == 0) return false;
		std::lock_guard<std::mutex> l(gCandado);
		const auto it = gPerfiles.find(parsecId);
		if (it == gPerfiles.end()) return false;
		salida = it->second.perfil;
		return true;
	}

	void anotar(uint32_t parsecId, const Estado& crudo, const Estado& salida) {
		if (parsecId == 0 || gObservado.load(std::memory_order_relaxed) != parsecId) return;
		std::lock_guard<std::mutex> l(gCandadoVista);
		const std::string p = pulsacionNueva(gCrudo, crudo);
		if (!p.empty()) { gUltima = p; gContador++; }
		gCrudo = crudo;
		gSalida = salida;
		gUltimoMs = ahoraMs();
	}

	void poner(uint32_t parsecId, const std::string& nombre, const Perfil& p) {
		asegurar();
		std::lock_guard<std::mutex> l(gCandado);
		if (p.esIdentidad()) gPerfiles.erase(parsecId);
		else gPerfiles[parsecId] = { nombre, p, epochMs() };
		gCuantos = static_cast<int>(gPerfiles.size());
		guardarDisco();
	}

	void quitar(uint32_t parsecId) {
		asegurar();
		std::lock_guard<std::mutex> l(gCandado);
		gPerfiles.erase(parsecId);
		gCuantos = static_cast<int>(gPerfiles.size());
		guardarDisco();
	}

	json listar() {
		asegurar();
		std::lock_guard<std::mutex> l(gCandado);
		json r = json::object();
		for (auto& [id, g] : gPerfiles)
			r[std::to_string(id)] = { {"nombre", g.nombre}, {"actualizado", g.actualizado}, {"perfil", aJson(g.perfil)} };
		return r;
	}

	void observar(uint32_t parsecId) {
		if (gObservado.exchange(parsecId) == parsecId) return;
		std::lock_guard<std::mutex> l(gCandadoVista);
		gCrudo = Estado(); gSalida = Estado();
		gUltima.clear();
		gUltimoMs = 0;
	}

	json vista() {
		const uint32_t id = gObservado.load();
		if (id == 0) return nullptr;
		std::lock_guard<std::mutex> l(gCandadoVista);
		return { {"parsecId", id}, {"crudo", estadoAJson(gCrudo)}, {"salida", estadoAJson(gSalida)},
			{"ultima", gUltima}, {"n", gContador}, {"edadMs", gUltimoMs ? ahoraMs() - gUltimoMs : -1} };
	}
}
