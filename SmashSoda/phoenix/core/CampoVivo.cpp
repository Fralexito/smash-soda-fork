#include "CampoVivo.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

namespace phoenix::campo {

	namespace {
		uint32_t u32(const uint8_t* p, uint32_t off) { uint32_t v; std::memcpy(&v, p + off, 4); return v; }
		bool vec4Valido(const uint8_t* p, float& x, float& y, float& z) {
			float v[4];
			std::memcpy(v, p, 16);
			if (!(v[3] == 1.0f)) return false;
			if (!posicionValida(v[0], v[1], v[2])) return false;
			x = v[0]; y = v[1]; z = v[2];
			return true;
		}
	}

	bool posicionValida(float x, float y, float z) {
		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
		return std::fabs(x) <= 57.0f && std::fabs(z) <= 40.0f && y > 0.0f && y < 3.0f;
	}

	bool leerActor(const uint8_t* obj, Actor& out) {
		if (u32(obj, kOffTipo) != 0x18) return false;
		const uint32_t lado = u32(obj, kOffLado);
		if (lado != 1 && lado != 2) return false;
		if (u32(obj, kOffMarca) != (0x80000u | lado)) return false;
		const uint32_t cod = u32(obj, kOffCodigo);
		if ((cod & 0xFFFFu) != 3u) return false;
		if (u32(obj, kOffReg0) != 0x10u || u32(obj, kOffReg3) != 0xDEADu) return false;
		float x, y, z;
		if (!vec4Valido(obj + kOffPosicion, x, y, z)) return false;
		out.codigo = cod >> 16;
		out.x = x; out.y = y; out.z = z;
		return true;
	}

	bool leerPosicion(const uint8_t* obj16, float& x, float& y, float& z) {
		return vec4Valido(obj16, x, y, z);
	}

	std::vector<Actor> elegirLista(const std::vector<Actor>& todos) {
		std::map<uint32_t, std::vector<Actor>> porCodigo;
		for (const Actor& a : todos) porCodigo[a.codigo].push_back(a);
		const std::vector<Actor>* mejor = nullptr;
		for (const auto& par : porCodigo) {
			const size_t n = par.second.size();
			if (n < 18 || n > 24) continue;
			if (!mejor || n > mejor->size()) mejor = &par.second;
		}
		if (!mejor) return {};
		std::vector<Actor> r = *mejor;
		std::sort(r.begin(), r.end(), [](const Actor& a, const Actor& b) { return a.dir < b.dir; }); // orden estable entre lecturas
		return r;
	}

	bool pareceAsistente(const Actor& a) {
		const float z = std::fabs(a.z);
		return z >= 34.8f && z <= 36.5f;
	}

	bool exeConocido(uint64_t tamano, const std::string& sha256) {
		// PES2021.exe del parche ConmeGOL (y del SP2026): el mismo archivo.
		return tamano == 458806784ull && sha256 == "5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A";
	}

}
