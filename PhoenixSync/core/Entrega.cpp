#include "Entrega.h"
#include "Respaldos.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <random>
#include <set>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace mercado::entrega {

	bool nombrePermitido(const std::string& n) { return n == "EDIT00000000" || n == "Player.bin" || n == "PlayerAssignment.bin"; }

	bool idValido(const std::string& id) {
		if (id.empty() || id.size() > 64) return false;
		for (char c : id)
			if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
		return true;
	}

	std::string nuevoId() {
		const std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm l{};
#ifdef _WIN32
		localtime_s(&l, &t);
#else
		localtime_r(&t, &l);
#endif
		char b[32]; std::strftime(b, sizeof(b), "%Y%m%d%H%M%S", &l);
		static std::mt19937_64 gen(std::random_device{}() ^ static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
		char h[16]; std::snprintf(h, sizeof(h), "%08x", static_cast<unsigned>(gen() & 0xffffffffu));
		return std::string("sync-") + b + "-" + h;
	}

	std::string recortarResumen(const std::string& texto, size_t maxBytes) {
		std::string s;
		for (char c : texto) s += (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
		if (s.size() <= maxBytes) return s;
		size_t n = maxBytes;
		while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) n--;   // no partir una letra
		return s.substr(0, n);
	}

	bool hayPendiente(const std::string& carpetaEntrega) {
		std::error_code ec;
		return fs::exists(aRuta(carpetaEntrega) / "entrega.json", ec);
	}

	int estadoDe(const std::string& carpetaEntrega, const std::string& id) {
		if (!idValido(id)) return 0;
		std::error_code ec;
		const fs::path c = aRuta(carpetaEntrega);
		if (fs::exists(c / "entregados" / id, ec)) return 1;
		if (fs::exists(c / "rechazadas" / id, ec)) return -1;
		return 0;
	}

	Resultado<std::string> escribir(const std::string& carpetaEntrega, const std::string& id,
		const std::string& resumen, const std::vector<Archivo>& archivos, const std::string& creadoEnIso) {
		if (!idValido(id)) return Resultado<std::string>::mal("ENTREGA_INVALIDA", "id no válido");
		if (archivos.empty()) return Resultado<std::string>::mal("ENTREGA_INVALIDA", "sin archivos");
		std::set<std::string> vistos;
		for (const auto& a : archivos) {
			if (!nombrePermitido(a.nombre)) return Resultado<std::string>::mal("ENTREGA_INVALIDA", "archivo no permitido: " + a.nombre);
			if (!vistos.insert(a.nombre).second) return Resultado<std::string>::mal("ENTREGA_INVALIDA", "archivo repetido: " + a.nombre);
		}
		if (hayPendiente(carpetaEntrega)) return Resultado<std::string>::mal("ENTREGA_PENDIENTE", "Link todavía no recogió la entrega anterior");
		if (estadoDe(carpetaEntrega, id) != 0) return Resultado<std::string>::mal("ENTREGA_INVALIDA", "id ya usado");

		try {
			const fs::path dir = aRuta(carpetaEntrega);
			fs::create_directories(dir);
			json lista = json::array();
			// 1) Datos primero.
			for (const auto& a : archivos) {
				if (a.nombre == "Player.bin" || a.nombre == "PlayerAssignment.bin") {   // Link exige «WESYS» en los primeros 16 bytes: mejor avisar aquí.
					std::ifstream f(aRuta(a.rutaOrigen), std::ios::binary);
					char cab[16] = {};
					f.read(cab, 16);
					if (std::string(cab, static_cast<size_t>(f.gcount())).find("WESYS") == std::string::npos)
						return Resultado<std::string>::mal("ENTREGA_INVALIDA", "Player.bin sin cabecera WESYS");
				}
				auto c = respaldos::copiarVerificado(a.rutaOrigen, deRuta(dir / aRuta(a.nombre)));
				if (!c.ok()) return Resultado<std::string>::mal(c.error.codigo, c.error.detalle);
				lista.push_back({ { "nombre", a.nombre }, { "sha256", *c.valor } });
			}
			// 2) entrega.json al final.
			const json e = { { "version", 1 }, { "id", id }, { "creado_en", creadoEnIso },
				{ "resumen", recortarResumen(resumen) }, { "archivos", lista } };
			const std::string texto = e.dump();
			const fs::path tmp = dir / "entrega.json.tmp";
			{
				std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
				f << texto;
				if (!f) return Resultado<std::string>::mal("ARCHIVO_ERROR", "No se pudo escribir entrega.json");
			}
			if (hayPendiente(carpetaEntrega)) { std::error_code ec; fs::remove(tmp, ec); return Resultado<std::string>::mal("ENTREGA_PENDIENTE", "apareció otra entrega"); }
			fs::rename(tmp, dir / "entrega.json");
			return Resultado<std::string>::bien(texto);
		}
		catch (const std::exception& ex) {
			return Resultado<std::string>::mal("ARCHIVO_ERROR", ex.what());
		}
	}

}
