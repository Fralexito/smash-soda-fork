// Busca cómo se enlaza cada actor del campo con SU jugador (dorsal / nombre). Solo lectura de PES2021.exe.
//  1) Escanea la memoria como Link y se queda con la lista de 22 (20 jugadores + árbitro + 2 asistentes).
//  2) Por cada actor sigue los punteros de su objeto (nivel 1) y los punteros de esos (nivel 2), y mira:
//       · números pequeños (1..99) distintos entre los jugadores  → posible DORSAL
//       · textos ASCII de >= 4 letras distintos entre jugadores     → posible NOMBRE
//  3) Imprime las rutas «+off1 → +off2» que mejor separan a los jugadores.
// Uso: cadena [segundos_de_espera_para_el_saque=0]   (con el partido en juego; no pausa ni escribe nada)
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include "phoenix/core/CampoVivo.h"
using namespace phoenix::campo;

static HANDLE h;
static bool leer(uint64_t a, void* b, size_t n) { SIZE_T g = 0; return ReadProcessMemory(h, (LPCVOID)a, b, n, &g) && g == n; }
static bool ptrOk(uint64_t p) { return p >= 0x10000 && p < 0x7FFFFFFFFFFFull && (p & 3) == 0; }

int main() {
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); if (!pid) { std::printf("PES2021 no esta abierto\n"); return 1; }
	h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	std::vector<Actor> campo;
	{ const size_t trozo = 1u << 20; std::vector<uint8_t> buf(trozo + kBytesObjeto); uint64_t a = 0; MEMORY_BASIC_INFORMATION m;
	  while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		const uint64_t base = (uint64_t)m.BaseAddress, fin = base + m.RegionSize;
		const bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
		if (ok) for (uint64_t p = base; p < fin; p += trozo) {
			SIZE_T q = (SIZE_T)std::min<uint64_t>(trozo + kBytesObjeto, fin - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), q, &g) || g < kBytesObjeto) continue;
			for (SIZE_T i = 0; i + kBytesObjeto <= g && i < trozo; i += 16) { Actor x; if (leerActor(buf.data() + i, x)) { x.dir = p + i; campo.push_back(x); } }
		}
		a = base + m.RegionSize; } }
	std::vector<Actor> lista = elegirLista(campo);
	std::printf("actores de campo: %zu, lista elegida: %zu\n", campo.size(), lista.size());
	if (lista.size() < 18) return 3;
	// por actor: objeto de 640 bytes
	std::vector<std::vector<uint8_t>> obj(lista.size(), std::vector<uint8_t>(kBytesObjeto));
	for (size_t i = 0; i < lista.size(); i++) leer(lista[i].dir, obj[i].data(), kBytesObjeto);
	struct Cand { std::string ruta; std::map<std::string, int> vals; int distintos = 0; int enRango = 0; };
	std::map<std::string, std::vector<std::string>> porRuta;   // ruta -> valor por actor (texto)
	auto anota = [&](const std::string& ruta, size_t i, const std::string& v) { auto& vv = porRuta[ruta]; if (vv.size() < lista.size()) vv.resize(lista.size()); vv[i] = v; };
	for (size_t i = 0; i < lista.size(); i++) {
		for (int o1 = 0; o1 + 8 <= (int)kBytesObjeto; o1 += 8) {
			uint64_t p1; std::memcpy(&p1, obj[i].data() + o1, 8); if (!ptrOk(p1)) continue;
			uint8_t t1[512]; if (!leer(p1, t1, sizeof t1)) continue;
			char rb[64];
			for (int k = 0; k + 4 <= 512; k += 4) {   // enteros pequeños en el objeto apuntado (nivel 1)
				uint32_t v; std::memcpy(&v, t1 + k, 4); std::snprintf(rb, sizeof rb, "+%d->+%d", o1, k);
				if (v >= 1 && v <= 99) anota(rb, i, std::to_string(v));
			}
			for (int k = 0; k + 4 <= 512; k++) {      // textos ASCII en el nivel 1
				int n = 0; while (k + n < 512 && t1[k + n] >= 'A' && t1[k + n] <= 'z' && (t1[k + n] <= 'Z' || t1[k + n] >= 'a')) n++;
				if (n >= 4 && (k == 0 || !(t1[k - 1] >= 'A' && t1[k - 1] <= 'z'))) { std::snprintf(rb, sizeof rb, "+%d->txt@%d", o1, k); anota(rb, i, std::string((char*)t1 + k, n)); }
			}
			for (int o2 = 0; o2 + 8 <= 512; o2 += 8) { // nivel 2
				uint64_t p2; std::memcpy(&p2, t1 + o2, 8); if (!ptrOk(p2) || p2 == p1) continue;
				uint8_t t2[256]; if (!leer(p2, t2, sizeof t2)) continue;
				for (int k = 0; k + 4 <= 256; k += 4) { uint32_t v; std::memcpy(&v, t2 + k, 4); if (v >= 1 && v <= 99) { std::snprintf(rb, sizeof rb, "+%d->+%d->+%d", o1, o2, k); anota(rb, i, std::to_string(v)); } }
				for (int k = 0; k + 4 <= 256; k++) {
					int n = 0; while (k + n < 256 && ((t2[k + n] >= 'A' && t2[k + n] <= 'Z') || (t2[k + n] >= 'a' && t2[k + n] <= 'z'))) n++;
					if (n >= 4 && (k == 0 || !((t2[k - 1] >= 'A' && t2[k - 1] <= 'Z') || (t2[k - 1] >= 'a' && t2[k - 1] <= 'z')))) { std::snprintf(rb, sizeof rb, "+%d->+%d->txt@%d", o1, o2, k); anota(rb, i, std::string((char*)t2 + k, n)); }
				}
			}
		}
	}
	// ranking: rutas con valor en casi todos los jugadores de campo (los de z != ±35 y no árbitro) y muchos valores distintos
	std::vector<std::pair<int, std::string>> orden;
	for (auto& kv : porRuta) {
		std::set<std::string> d; int con = 0; for (auto& v : kv.second) if (!v.empty()) { d.insert(v); con++; }
		if (con >= 16 && (int)d.size() >= 14) orden.push_back({ (int)d.size() * 100 + con, kv.first });
	}
	std::sort(orden.rbegin(), orden.rend());
	std::printf("rutas candidatas (valores distintos x100 + actores con valor): %zu\n", orden.size());
	int n = 0;
	for (auto& o : orden) {
		auto& vv = porRuta[o.second]; std::printf("%s  (%d distintos, %d actores):", o.second.c_str(), o.first / 100, o.first % 100);
		for (size_t i = 0; i < vv.size() && i < 24; i++) std::printf(" %s", vv[i].empty() ? "-" : vv[i].c_str());
		std::printf("\n"); if (++n >= 40) break;
	}
	return 0;
}
