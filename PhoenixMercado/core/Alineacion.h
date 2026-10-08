#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// =============================================================================
//  Phoenix Mercado · Alineación de un equipo (orden de formación + roles)
// -----------------------------------------------------------------------------
//  El motor de PES 2021 guarda la alineación de cada equipo igual en los dos
//  sitios que tocamos:
//    · Liga Máster: bloque de 600 B por equipo (orden en +0x220, roles en +0x248)
//    · Option file: bloque de tácticas de 628 B por equipo (orden en +0x1E4, roles en +0x20C)
//  Orden = 40 bytes con índices de plantilla: puestos 0–10 titulares (cada uno es
//  un sitio de la formación), después banca y reservas (una lista). Formatos vistos:
//  compacto (n entradas y 0xFF), identidad (0…39) y «0…n-1, 0xFF, n…38». El juego
//  solo lee las primeras n (n = jugadores de la plantilla). Roles = 6 bytes con
//  índices de plantilla (capitán, penales, tiros libres…).
//  Esta cabecera reúne la lógica PURA (sin archivos) para que LigaMaster y
//  OptionFile hagan exactamente lo mismo. Comprobado en el juego (ranuras 9 y 10).
// =============================================================================

namespace mercado::alineacion {

	constexpr size_t kTitulares = 11;
	constexpr size_t kBytesOrden = 40;
	constexpr size_t kBytesRoles = 6;
	constexpr size_t kOrdenARoles = 0x28;   // los roles van justo después de los 40 bytes del orden

	using Orden = std::vector<uint8_t>;
	using Roles = std::array<uint8_t, kBytesRoles>;

	/// Lee las primeras n entradas del orden (en `p`) y comprueba que sean una permutación de 0…n-1.
	/// Devuelve false (y el motivo) si el orden no es válido: entonces NO se toca.
	inline bool leerOrden(const uint8_t* p, size_t n, Orden& orden, std::string& porque) {
		orden.clear();
		uint64_t visto = 0;
		if (n > kBytesOrden) { porque = "más de 40 jugadores"; return false; }
		for (size_t i = 0; i < n; i++) {
			const uint8_t v = p[i];
			if (v >= n || (visto >> v) & 1) { porque = "el orden de formación no es una permutación de la plantilla"; return false; }
			visto |= uint64_t(1) << v;
			orden.push_back(v);
		}
		return true;
	}

	/// Lee los roles (6 B en `p`); todos tienen que apuntar dentro de la plantilla.
	inline bool leerRoles(const uint8_t* p, size_t n, Roles& roles, std::string& porque) {
		for (size_t i = 0; i < kBytesRoles; i++) {
			roles[i] = p[i];
			if (n && roles[i] >= n) { porque = "un rol apunta fuera de la plantilla"; return false; }
		}
		return true;
	}

	/// ¿Hace falta sustituto? Solo si es TITULAR (puestos 0–10) o tiene un rol. Así lo hace el juego: si se va un
	/// titular, otro ocupa su puesto; si se va un suplente o una reserva, los de atrás simplemente suben.
	inline bool necesitaSustituto(const Orden& orden, const Roles& roles, int idx) {
		const auto it = std::find(orden.begin(), orden.end(), uint8_t(idx));
		if (it != orden.end() && size_t(it - orden.begin()) < kTitulares) return true;
		return std::any_of(roles.begin(), roles.end(), [&](uint8_t v) { return v == idx; });
	}

	/// Quita `idx` del orden y de los roles. Con sustituto (idxS >= 0), este ocupa el puesto del que se va y deja
	/// libre el suyo. Los índices mayores que idx bajan uno (la plantilla se compacta igual).
	inline bool quitarDeOrden(Orden& orden, Roles& roles, int idx, int idxS, std::string& porque) {
		auto itH = std::find(orden.begin(), orden.end(), uint8_t(idx));
		if (itH == orden.end()) { porque = "el jugador no está en el orden de formación"; return false; }
		const size_t posH = size_t(itH - orden.begin());
		if (idxS >= 0) {
			auto itS = std::find(orden.begin(), orden.end(), uint8_t(idxS));
			if (itS == orden.end()) { porque = "el sustituto no está en el orden de formación"; return false; }
			const size_t posS = size_t(itS - orden.begin());
			orden[posH] = uint8_t(idxS);
			orden.erase(orden.begin() + long(posS));
		}
		else orden.erase(orden.begin() + long(posH));
		for (auto& v : orden) if (v > idx) v--;
		for (auto& v : roles) {
			if (v == idx) { if (idxS < 0) { porque = "el jugador tiene un rol y no hay sustituto"; return false; } v = uint8_t(idxS); }
			if (v > idx) v--;
		}
		return true;
	}

	/// Elige quién cubre el puesto de `idx` (índice de plantilla): el mejor de los NO titulares, mirando primero las
	/// reservas (final de la lista) y subiendo hacia la banca. `posicionDe(índice)` devuelve la posición registrada
	/// (0 PT, 1 DC, 2 LI, 3 LD, 4 MCD, 5 MC, 6 II, 7 ID, 8 MP, 9 EI, 10 ED, 11 SD, 12 DC) o -1 si no se sabe.
	/// Un portero solo lo cubre otro portero; un jugador de campo nunca lo cubre un portero.
	/// Devuelve el índice de plantilla del sustituto o -1 si no hay nadie.
	inline int elegirSustituto(const Orden& orden, int idx, const std::function<int(int)>& posicionDe) {
		const auto itH = std::find(orden.begin(), orden.end(), uint8_t(idx));
		if (itH == orden.end()) return -1;
		const size_t posH = size_t(itH - orden.begin());
		auto grupo = [](int c) { static const int g[13] = { 0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 7 }; return c < 0 || c > 12 ? -1 : g[c]; };
		auto linea = [](int c) { return c < 0 ? -1 : c == 0 ? 0 : c <= 3 ? 1 : c <= 8 ? 2 : 3; };
		const int cH = posicionDe ? posicionDe(idx) : -1;
		const bool esPortero = cH == 0 || (cH < 0 && posH == 0);   // el puesto 0 de la formación es siempre el portero
		int mejor = -1, elegido = -1;
		for (size_t p = orden.size(); p-- > kTitulares;) {
			if (p == posH) continue;
			const int c = posicionDe ? posicionDe(int(orden[p])) : -1;
			int puntos;
			if (esPortero) { if (c != 0) continue; puntos = 3; }
			else {
				if (c == 0) continue;
				puntos = cH < 0 || c < 0 ? 0 : c == cH ? 3 : grupo(c) == grupo(cH) ? 2 : linea(c) == linea(cH) ? 1 : 0;
			}
			if (puntos > mejor) { mejor = puntos; elegido = int(orden[p]); }
		}
		return elegido;
	}

	/// Escribe el orden en 40 bytes. `colaIdentidad` = la cola (entradas n…39) sigue como identidad (así lo deja el
	/// editor del juego en el option file); si no, 0xFF (como lo deja la Liga Máster).
	inline void escribirOrden(uint8_t* p, const Orden& orden, bool colaIdentidad) {
		for (size_t i = 0; i < kBytesOrden; i++) p[i] = i < orden.size() ? orden[i] : colaIdentidad ? uint8_t(i) : uint8_t(0xff);
	}

	inline void escribirRoles(uint8_t* p, const Roles& roles) { for (size_t i = 0; i < kBytesRoles; i++) p[i] = roles[i]; }

	/// ¿La cola del orden (desde n) es identidad (n, n+1, …)? (Para conservar el formato que ya tenía.)
	inline bool colaEsIdentidad(const uint8_t* p, size_t n) {
		if (n >= kBytesOrden) return false;
		for (size_t i = n; i < kBytesOrden; i++) if (p[i] != i) return false;
		return true;
	}
}
