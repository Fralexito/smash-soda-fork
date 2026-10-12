// Busca una "tabla de filas": una secuencia de valores float (uno por fila) separados por un paso fijo en la memoria
// de PES2021.exe (solo lectura). Sirve para encontrar la tabla de valoraciones/estadisticas por jugador.
// Uso: buscafilas <paso_min> <paso_max> v1 v2 v3 ...   (v = float; coincidencia exacta a 0.01)
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
int main(int argc, char** argv) {
	int s0 = atoi(argv[1]), s1 = atoi(argv[2]); std::vector<float> V; for (int i = 3; i < argc; i++) V.push_back((float)atof(argv[i]));
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; const size_t trozo = 1 << 20; const size_t extra = (size_t)s1 * V.size() + 64; std::vector<char> buf(trozo + extra); int n = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && m.Type != MEM_IMAGE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += trozo) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>(trozo + extra, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (SIZE_T i = 0; i + 4 <= g && i < trozo; i += 4) {
				float f; memcpy(&f, &buf[i], 4); if (!(std::fabs(f - V[0]) <= 0.01f)) continue;
				for (int st = s0; st <= s1; st += 4) {
					bool ok2 = true;
					for (size_t k = 1; k < V.size() && ok2; k++) { SIZE_T o = i + (SIZE_T)st * k; if (o + 4 > g) { ok2 = false; break; } float x; memcpy(&x, &buf[o], 4); if (!(std::fabs(x - V[k]) <= 0.01f)) ok2 = false; }
					if (ok2) { printf("@%llX paso=%d tipo=%lX\n", (unsigned long long)(p + i), st, m.Type); if (++n > 40) return 0; }
				}
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
