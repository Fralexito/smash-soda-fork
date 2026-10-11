#include "Doctor.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif
#include <algorithm>
#include <ctime>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>

#include "Entrega.h"
#include "ModulosJuego.h"

namespace phoenix::doctor {

namespace {
	std::string ahoraTexto() {
		std::time_t t = std::time(nullptr);
		std::tm lt{};
#ifdef _WIN32
		localtime_s(&lt, &t);
#else
		localtime_r(&t, &lt);
#endif
		char b[32];
		std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &lt);
		return b;
	}

	bool leerTexto(const fs::path& ruta, std::string& out, uintmax_t maximo) {
		std::error_code ec;
		const uintmax_t n = fs::file_size(ruta, ec);
		if (ec || n > maximo) return false;
		std::ifstream f(ruta, std::ios::binary);
		if (!f) return false;
		out.assign(static_cast<size_t>(n), '\0');
		if (n) f.read(&out[0], static_cast<std::streamsize>(n));
		return true;
	}

	/// Valor entre comillas de una línea «clave = "valor"» activa (sin ; ni # delante). "" si no es esa clave.
	std::string valorActivo(const std::string& linea, const std::string& clave) {
		size_t i = 0;
		while (i < linea.size() && (linea[i] == ' ' || linea[i] == '\t')) i++;
		if (linea.compare(i, clave.size(), clave) != 0) return "";
		i += clave.size();
		while (i < linea.size() && (linea[i] == ' ' || linea[i] == '\t')) i++;
		if (i >= linea.size() || linea[i] != '=') return "";
		const size_t a = linea.find('"', i), b = a == std::string::npos ? a : linea.find('"', a + 1);
		if (a == std::string::npos || b == std::string::npos) return "";
		return linea.substr(a + 1, b - a - 1);
	}

	Archivo ficha(const std::string& nombre, const fs::path& ruta) {
		Archivo a;
		a.nombre = nombre;
		std::error_code ec;
		if (!fs::is_regular_file(ruta, ec)) return a;
		a.bytes = static_cast<long long>(fs::file_size(ruta, ec));
		phoenix::entrega::sha256Archivo(ruta, a.sha256);
		return a;
	}

	fs::path carpetaDocumentos() {
#ifdef _WIN32
		PWSTR p = nullptr;
		fs::path r;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &p)) && p) r = fs::path(p);
		if (p) CoTaskMemFree(p);
		return r;
#else
		return {};
#endif
	}
}

Informe revisar(const fs::path& juego) {
	Informe inf;
	inf.generado = ahoraTexto();
	std::error_code ec;
	if (juego.empty() || !fs::is_directory(juego, ec)) {
		inf.avisos.push_back("No sé dónde está el juego: abre PES 2021 una vez con Phoenix Link abierto.");
		return inf;
	}
	inf.juegoEncontrado = true;
	inf.juego = phoenix::entrega::aU8(juego);
	inf.parche = phoenix::entrega::nombreParche(juego);
	// El switcher de ConmeGOL apunta el modo activo en version_actual.txt: ese es el parche que corre ahora
	{
		std::string v;
		if (leerTexto(juego / "version_actual.txt", v, 4096)) {
			if (v.rfind("\xEF\xBB\xBF", 0) == 0) v.erase(0, 3);
			while (!v.empty() && (v.back() == '\r' || v.back() == '\n' || v.back() == ' ')) v.pop_back();
			if (!v.empty() && v.size() <= 120 && v.find('\n') == std::string::npos) inf.parche = v;
		}
	}

	// Versión del exe: tamaño + sha256 del primer MB (rápido; distingue ejecutables distintos)
	const fs::path exe = juego / "PES2021.exe";
	if (fs::is_regular_file(exe, ec)) {
		inf.exeBytes = static_cast<long long>(fs::file_size(exe, ec));
		std::ifstream f(exe, std::ios::binary);
		std::string trozo(1024 * 1024, '\0');
		f.read(&trozo[0], static_cast<std::streamsize>(trozo.size()));
		trozo.resize(static_cast<size_t>(f.gcount()));
		inf.exeHuella = phoenix::entrega::sha256Hex(trozo.data(), trozo.size());
	}
	else inf.avisos.push_back("No encuentro PES2021.exe en la carpeta del juego.");

	// sider.ini de la raíz (el que usa el juego): módulos Lua y raíces de la base, en orden
	const auto destinos = phoenix::modulos::destinos(juego);
	for (const auto& d : destinos) inf.modos.push_back(d.first);
	if (!destinos.empty()) {
		std::string ini;
		if (leerTexto(destinos.front().second / "sider.ini", ini, 512 * 1024)) {
			size_t i = 0;
			while (i <= ini.size()) {
				size_t j = ini.find('\n', i);
				if (j == std::string::npos) j = ini.size();
				std::string l = ini.substr(i, j - i);
				if (!l.empty() && l.back() == '\r') l.pop_back();
				const std::string lua = valorActivo(l, "lua.module");
				if (!lua.empty() && inf.luaActivos.size() < 120) inf.luaActivos.push_back(lua);
				const std::string raiz = valorActivo(l, "cpk.root");
				if (!raiz.empty() && inf.cpkRoots.size() < 600) inf.cpkRoots.push_back(raiz);
				i = j + 1;
			}
		}
	}
	else inf.avisos.push_back("No encuentro SiderAddons\\sider.ini: ¿el parche usa Sider?");

	// Archivos que deciden el partido, por cada carpeta de Sider (raíz + modos)
	std::map<std::string, std::string> phoenixLua;   // modo → sha de phoenix.lua (para ver si difiere)
	for (const auto& d : destinos) {
		const std::string tras = " (" + d.first + ")";
		inf.archivos.push_back(ficha("sider.ini" + tras, d.second / "sider.ini"));
		const Archivo pl = ficha("modules/phoenix.lua" + tras, d.second / "modules" / "phoenix.lua");
		inf.archivos.push_back(pl);
		if (!pl.sha256.empty()) phoenixLua[d.first] = pl.sha256;
		inf.archivos.push_back(ficha("modules/phoenix_estadio.lua" + tras, d.second / "modules" / "phoenix_estadio.lua"));
		const fs::path pesdb = d.second / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb";
		inf.archivos.push_back(ficha("Phoenix-DB/Player.bin" + tras, pesdb / "Player.bin"));
		inf.archivos.push_back(ficha("Phoenix-DB/PlayerAssignment.bin" + tras, pesdb / "PlayerAssignment.bin"));
	}
	// Option file (plantillas de los amistosos con «Datos en vivo» desactivado)
	const fs::path opt = phoenix::entrega::buscarCarpetaOptionFile(carpetaDocumentos());
	if (!opt.empty()) inf.archivos.push_back(ficha("option file (EDIT00000000)", opt / "EDIT00000000"));
	else inf.avisos.push_back("No encuentro el option file (EDIT00000000) en Documentos\\KONAMI.");

	// Avisos que se ven a simple vista (no bloquean nada: modo informativo)
	bool phoenixDb = false;
	int idxPhoenix = -1, idxBase = -1;
	for (size_t k = 0; k < inf.cpkRoots.size(); k++) {
		if (idxPhoenix < 0 && inf.cpkRoots[k].find("Phoenix-DB") != std::string::npos) { idxPhoenix = static_cast<int>(k); phoenixDb = true; }
		if (idxBase < 0 && inf.cpkRoots[k].find("Database") != std::string::npos && inf.cpkRoots[k].find("Phoenix-DB") == std::string::npos) idxBase = static_cast<int>(k);
	}
	if (!phoenixDb) inf.avisos.push_back("sider.ini no tiene la raíz Phoenix-DB: la base sale solo del parche.");
	else if (idxBase >= 0 && idxPhoenix > idxBase) inf.avisos.push_back("La raíz Phoenix-DB está detrás de la base del parche: los fichajes de Phoenix no se verían.");
	std::string unSha; bool distinto = false;
	for (const auto& p : phoenixLua) { if (unSha.empty()) unSha = p.second; else if (p.second != unSha) distinto = true; }
	if (distinto) inf.avisos.push_back("phoenix.lua no es igual en todos los modos del switcher.");

	// Huella del anfitrión: sha256 de todo lo anterior en un orden fijo
	std::string resumen = "exe=" + std::to_string(inf.exeBytes) + ":" + inf.exeHuella + "\n";
	for (const auto& l : inf.luaActivos) resumen += "lua=" + l + "\n";
	for (const auto& r : inf.cpkRoots) resumen += "root=" + r + "\n";
	for (const auto& a : inf.archivos) resumen += a.nombre + "=" + a.sha256 + "\n";
	inf.huella = phoenix::entrega::sha256Hex(resumen.data(), resumen.size());
	return inf;
}

std::string comoJson(const Informe& i) {
	nlohmann::json arch = nlohmann::json::array();
	for (const Archivo& a : i.archivos) arch.push_back({ {"nombre", a.nombre}, {"sha256", a.sha256}, {"bytes", a.bytes} });
	return nlohmann::json{
		{"modo", i.modo}, {"generado", i.generado}, {"huella", i.huella}, {"juegoEncontrado", i.juegoEncontrado},
		{"juego", i.juego}, {"parche", i.parche}, {"exe", { {"bytes", i.exeBytes}, {"huellaInicio", i.exeHuella} }},
		{"modos", i.modos}, {"luaActivos", i.luaActivos}, {"cpkRoots", i.cpkRoots}, {"archivos", arch}, {"avisos", i.avisos},
	}.dump();
}

} // namespace phoenix::doctor
