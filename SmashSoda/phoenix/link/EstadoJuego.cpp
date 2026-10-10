#include "EstadoJuego.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

namespace phoenix::juego {

bool parsear(const std::string& texto, EstadoPartidoJuego& e) {
	const nlohmann::json j = nlohmann::json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object()) return false;
	if (!j.contains("fase") || !j["fase"].is_string()) return false;
	const std::string fase = j["fase"].get<std::string>();
	if (fase != "menu" && fase != "en_juego" && fase != "pausado" && fase != "descanso" && fase != "final") return false;
	auto num = [&j](const char* clave, int porDefecto, int minimo, int maximo) {
		if (!j.contains(clave) || !j[clave].is_number()) return porDefecto;
		const double v = j[clave].get<double>();
		if (v < minimo) return minimo;
		if (v > maximo) return maximo;
		return static_cast<int>(v);
	};
	e.fase = fase;
	e.minuto = num("minuto", 0, 0, 999);
	e.periodo = num("periodo", 0, 0, 5);
	e.local = num("local", 0, 0, 2147483647);
	e.visita = num("visita", 0, 0, 2147483647);
	e.golesLocal = num("goles_local", -1, -1, 99);
	e.golesVisita = num("goles_visita", -1, -1, 99);
	e.segundo = num("segundo", 0, 0, 59);
	e.anadido = num("anadido", 0, 0, 30);
	e.pkLocal = num("pk_local", -1, -1, 99);
	e.pkVisita = num("pk_visita", -1, -1, 99);
	e.relojCorre = !(j.contains("reloj_corre") && j["reloj_corre"].is_boolean() && !j["reloj_corre"].get<bool>());
	e.completo = j.contains("completo") && j["completo"].is_boolean() && j["completo"].get<bool>();
	auto nombre = [&j](const char* clave) {
		if (!j.contains(clave) || !j[clave].is_string()) return std::string();
		std::string s = j[clave].get<std::string>();
		if (s.size() > 48) s.resize(48);
		std::string limpio;
		for (char c : s) if (static_cast<unsigned char>(c) >= 0x20) limpio += c;   // sin saltos ni controles
		return limpio;
	};
	e.nombreLocal = nombre("nombre_local");
	e.nombreVisita = nombre("nombre_visita");
	e.goles.clear();
	if (j.contains("goles") && j["goles"].is_array()) {
		for (const auto& g : j["goles"]) {
			if (e.goles.size() >= 40) break;
			if (!g.is_object() || !g.contains("m") || !g["m"].is_number()) continue;
			GolJuego gj;
			gj.minuto = (std::max)(0, (std::min)(200, g["m"].get<int>()));
			gj.local = !(g.contains("l") && g["l"].is_string() && g["l"].get<std::string>() == "v");
			e.goles.push_back(gj);
		}
	}
	e.seq = (j.contains("seq") && j["seq"].is_number_integer()) ? j["seq"].get<long long>() : 0;
	return true;
}

namespace {
	bool leerTexto(const std::filesystem::path& ruta, std::string& out) {
#ifdef _WIN32
		// Compartido para lectura, escritura y borrado: si phoenix.lua reemplaza el archivo justo ahora, no se le estorba.
		HANDLE f = CreateFileW(ruta.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (f == INVALID_HANDLE_VALUE) return false;
		char buf[kMaxBytes];
		DWORD leidos = 0;
		const BOOL ok = ReadFile(f, buf, static_cast<DWORD>(sizeof(buf)), &leidos, nullptr);
		CloseHandle(f);
		if (!ok) return false;
		out.assign(buf, leidos);
		return true;
#else
		std::ifstream f(ruta, std::ios::binary);
		if (!f) return false;
		char buf[kMaxBytes];
		f.read(buf, sizeof(buf));
		out.assign(buf, static_cast<size_t>(f.gcount()));
		return true;
#endif
	}
}

EstadoPartidoJuego leer(const std::filesystem::path& carpetaBuzon) {
	EstadoPartidoJuego e;
	if (carpetaBuzon.empty()) return e;
	const std::filesystem::path ruta = carpetaBuzon / "estado.json";
	std::error_code ec;
	const auto cuando = std::filesystem::last_write_time(ruta, ec);
	if (ec) return e;
	const auto edad = std::filesystem::file_time_type::clock::now() - cuando;
	e.edadSeg = std::chrono::duration<double>(edad).count();
	if (e.edadSeg < 0.0) e.edadSeg = 0.0;
	if (e.edadSeg > kMaxEdadSeg) return e;
	std::string texto;
	if (!leerTexto(ruta, texto)) return e;
	if (texto.size() >= 3 && static_cast<unsigned char>(texto[0]) == 0xEF) texto.erase(0, 3);   // BOM por si acaso
	e.valido = parsear(texto, e);
	return e;
}

} // namespace phoenix::juego
