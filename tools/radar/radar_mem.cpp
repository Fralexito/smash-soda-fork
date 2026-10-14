// Busca en la memoria de PES2021 la lista de puntos del MINIMAPA (radar) del juego. Solo lectura.
// Idea: el radar dibuja un punto por jugador con coordenadas de minimapa (u, v) que son una función lineal de la posición en el
// campo: u = a·x + b, v = c·z + d. Se toman los jugadores de campo en movimiento (ordenados por dirección), y se barre la memoria
// buscando matrices de pares de float (u, v) con paso fijo cuyos primeros N elementos cumplan esa relación lineal con (x, z).
// Si se encuentra, la estructura de al lado probablemente trae el equipo (color) y quién está controlado.
// Uso: radar_mem [orden=0|1|2]  (0 = lista A por dirección, 1 = lista B, 2 = ambas listas mezcladas por dirección)
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <timeapi.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <chrono>
#include <map>
#include <vector>
#include <algorithm>
#include "phoenix/core/CampoVivo.h"
#pragma comment(lib, "winmm.lib")
using namespace phoenix::campo;

static bool aprox(float v, float esperado, float tol) { return std::fabs(v - esperado) <= tol; }

int main(int argc, char** argv) {
	const int orden = argc > 1 ? std::atoi(argv[1]) : 0;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); if (!pid) { std::printf("PES2021 no esta abierto\n"); return 1; }
	HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	std::vector<Actor> campo;
	const size_t trozo = 1u << 20;
	{ std::vector<uint8_t> buf(trozo + kBytesObjeto); uint64_t a = 0; MEMORY_BASIC_INFORMATION m;
	  while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		const uint64_t base = (uint64_t)m.BaseAddress, fin = base + m.RegionSize;
		const bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
		if (ok) for (uint64_t p = base; p < fin; p += trozo) {
			SIZE_T q = (SIZE_T)std::min<uint64_t>(trozo + kBytesObjeto, fin - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), q, &g) || g < kBytesObjeto) continue;
			for (SIZE_T i = 0; i + kBytesObjeto <= g && i < trozo; i += 16) { Actor x; if (leerActor(buf.data() + i, x)) { x.dir = p + i; campo.push_back(x); } }
		}
		a = base + m.RegionSize; } }
	// quedarse con los que se mueven
	std::vector<float> ux(campo.size(), 1e9f), uz(campo.size(), 1e9f); std::vector<char> mov(campo.size(), 0);
	for (int v = 0; v < 150; v++) {
		for (size_t i = 0; i < campo.size(); i++) { uint8_t b[16]; SIZE_T g = 0; float x, y, z; if (ReadProcessMemory(h, (LPCVOID)(campo[i].dir + kOffPosicion), b, 16, &g) && g == 16 && leerPosicion(b, x, y, z)) { if (ux[i] < 1e8f && (std::fabs(x - ux[i]) > 0.02f || std::fabs(z - uz[i]) > 0.02f)) mov[i] = 1; ux[i] = x; uz[i] = z; } }
		Sleep(20);
	}
	std::map<unsigned, std::vector<Actor>> listas;
	for (size_t i = 0; i < campo.size(); i++) if (mov[i]) listas[campo[i].codigo].push_back(campo[i]);
	std::printf("jugadores en movimiento: %d en %zu listas\n", (int)std::count(mov.begin(), mov.end(), 1), listas.size());
	std::vector<Actor> ref;
	{ int k = 0; for (auto& kv : listas) { if (orden == 2) { for (auto& a : kv.second) ref.push_back(a); } else if (k == orden) ref = kv.second; k++; } }
	std::sort(ref.begin(), ref.end(), [](const Actor& a, const Actor& b) { return a.dir < b.dir; });
	if (ref.size() < 10) { std::printf("sin partido en movimiento (jugadores de referencia: %zu)\n", ref.size()); return 3; }
	// posiciones de referencia en UN instante (lectura rápida de todos)
	std::vector<float> X(ref.size()), Z(ref.size());
	for (size_t i = 0; i < ref.size(); i++) { uint8_t b[16]; SIZE_T g = 0; float x, y, z; if (ReadProcessMemory(h, (LPCVOID)(ref[i].dir + kOffPosicion), b, 16, &g) && g == 16 && leerPosicion(b, x, y, z)) { X[i] = x; Z[i] = z; } }
	const int N = 8;   // elementos a validar
	std::printf("buscando matrices de pares de float con relacion lineal con las posiciones de %zu jugadores (primeros %d)\n", ref.size(), N);
	const int pasos[] = { 8, 12, 16, 20, 24, 32, 48, 64 };
	long hallazgos = 0;
	std::vector<uint8_t> buf(trozo + 4096);
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		const uint64_t base = (uint64_t)m.BaseAddress, fin = base + m.RegionSize;
		const bool ok = m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
		if (ok) for (uint64_t p = base; p < fin; p += trozo) {
			SIZE_T q = (SIZE_T)std::min<uint64_t>(trozo + 4096, fin - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), q, &g) || g < 256) continue;
			const size_t n = g;
			for (int st : pasos) {
				const size_t span = (size_t)st * (N - 1) + 8;
				// un segundo jugador de referencia que fije la recta: el primero que esté a > 3 m en x y en z del jugador 0
				int j = 1; while (j < N && (std::fabs(X[j] - X[0]) < 3.0f || std::fabs(Z[j] - Z[0]) < 3.0f)) j++;
				if (j >= N) continue;
				for (size_t i = 0; i + span <= n && i < trozo; i += 4) {
					float u0, v0, uj, vj;
					std::memcpy(&u0, &buf[i], 4); std::memcpy(&v0, &buf[i + 4], 4);
					std::memcpy(&uj, &buf[i + (size_t)st * j], 4); std::memcpy(&vj, &buf[i + (size_t)st * j + 4], 4);
					if (!std::isfinite(u0) || !std::isfinite(v0) || !std::isfinite(uj) || !std::isfinite(vj)) continue;
					const float ax = (uj - u0) / (X[j] - X[0]), bx = u0 - ax * X[0], cz = (vj - v0) / (Z[j] - Z[0]), dz = v0 - cz * Z[0];
					if (std::fabs(ax) < 1e-4f || std::fabs(cz) < 1e-4f) continue;
					bool bien = true;
					for (int k = 0; k < N && bien; k++) {
						float uk, vk; std::memcpy(&uk, &buf[i + (size_t)st * k], 4); std::memcpy(&vk, &buf[i + (size_t)st * k + 4], 4);
						const float tolU = std::fabs(ax) * 0.25f + 1e-6f, tolV = std::fabs(cz) * 0.25f + 1e-6f;   // 25 cm: margen para el desfase entre lecturas
						if (!aprox(uk, ax * X[k] + bx, tolU) || !aprox(vk, cz * Z[k] + dz, tolV)) bien = false;
					}
					if (bien && hallazgos < 40) {
						hallazgos++;
						std::printf("HALLAZGO dir=%llX paso=%d  u = %.5f·x + %.4f   v = %.5f·z + %.4f\n", (unsigned long long)(p + i), st, ax, bx, cz, dz);
					}
				}
			}
		}
		a = base + m.RegionSize;
	}
	std::printf("barrido terminado: %ld hallazgos\n", hallazgos);
	return 0;
}
