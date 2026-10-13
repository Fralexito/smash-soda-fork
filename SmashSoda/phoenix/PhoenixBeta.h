#pragma once

// =============================================================================
//  Phoenix Link · control de la beta privada (solo cabecera, sin dependencias)
// -----------------------------------------------------------------------------
//  - Caducidad: la build deja de abrirse pasada la fecha kCaduca (PhoenixBuild.h). Va dentro del
//    programa, no en un archivo que se pueda borrar.
//  - Identificador de tester: el instalador de cada persona deja «tester.id» junto a PhoenixLink.exe.
//    Link lo manda a la web en la cabecera X-Phoenix-Tester y sirve para saber de quién salió una copia.
//  - Exento: si existe «phoenix-dueno.flag» en %APPDATA%\Trybuchet\Smash Soda, la build no caduca
//    (es el PC de quien desarrolla; los testers no tienen ese archivo).
//  Esto desincentiva y deja rastro; no es una protección absoluta (el reloj del PC se puede cambiar).
// =============================================================================

#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include "PhoenixBuild.h"

namespace phoenix {
namespace beta {

	inline std::string carpetaExe() {
		char ruta[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, ruta, MAX_PATH);
		std::string s(ruta);
		const size_t i = s.find_last_of("\\/");
		return i == std::string::npos ? std::string() : s.substr(0, i + 1);
	}

	inline bool esDueno() {
		char appdata[MAX_PATH] = {};
		if (!GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH)) return false;
		const std::string f = std::string(appdata) + "\\Trybuchet\\Smash Soda\\phoenix-dueno.flag";
		return GetFileAttributesA(f.c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	/// Identificador del tester (solo letras, números, guion y guion bajo; máx. 32). Vacío = sin identificador.
	inline std::string idTester() {
		static const std::string id = [] {
			std::ifstream f(carpetaExe() + "tester.id", std::ios::binary);
			std::string l;
			if (!f || !std::getline(f, l)) return std::string();
			std::string limpio;
			for (char c : l) {
				if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') limpio += c;
				if (limpio.size() >= 32) break;
			}
			return limpio;
		}();
		return id;
	}

	/// Días hasta la caducidad (negativo = ya caducó). 9999 = esta build no caduca.
	inline int diasRestantes() {
		const std::string c = kCaduca;
		if (c.size() != 10 || esDueno()) return 9999;
		std::tm fin{};
		fin.tm_year = std::atoi(c.substr(0, 4).c_str()) - 1900;
		fin.tm_mon = std::atoi(c.substr(5, 2).c_str()) - 1;
		fin.tm_mday = std::atoi(c.substr(8, 2).c_str());
		fin.tm_hour = 23; fin.tm_min = 59; fin.tm_isdst = -1;
		const std::time_t t = std::mktime(&fin);
		if (t == static_cast<std::time_t>(-1)) return 9999;
		const double seg = std::difftime(t, std::time(nullptr));
		return seg < 0 ? static_cast<int>(seg / 86400.0) - 1 : static_cast<int>(seg / 86400.0);
	}

	inline bool caducada() { return diasRestantes() < 0; }

	/// Avisos al arrancar. Devuelve false si Link NO debe abrirse (beta caducada).
	inline bool comprobarAlArrancar() {
		const int dias = diasRestantes();
		if (dias == 9999) return true;
		if (dias < 0) {
			const std::string msg = std::string("Esta versión de prueba de Phoenix Link caducó el ") + kCaduca
				+ ".\n\nPide la versión nueva a quien te la pasó y vuelve a instalarla (tus datos y ajustes se conservan).";
			MessageBoxA(nullptr, msg.c_str(), "Phoenix Link · versión de prueba", MB_OK | MB_ICONINFORMATION);
			return false;
		}
		if (dias <= 7) {
			const std::string msg = std::string("Esta versión de prueba caduca el ") + kCaduca + " (en " + std::to_string(dias) + (dias == 1 ? " día" : " días")
				+ ").\n\nPide la versión nueva a quien te la pasó antes de esa fecha.";
			MessageBoxA(nullptr, msg.c_str(), "Phoenix Link · versión de prueba", MB_OK | MB_ICONINFORMATION);
		}
		return true;
	}

}
}
