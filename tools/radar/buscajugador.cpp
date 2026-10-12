// Busca registros de estadisticas por jugador: un u32 (p. ej. minutos jugados = 90) con un float cercano (p. ej. velocidad
// media 7.2 km/h o 2.0 m/s) dentro de una ventana. Solo lectura. Uso: buscajugador <entero> <float1> [float2] [ventana_bytes]
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
	uint32_t ent = (uint32_t)atoi(argv[1]); float f1 = (float)atof(argv[2]), f2 = argc > 3 ? (float)atof(argv[3]) : f1; int vent = argc > 4 ? atoi(argv[4]) : 128;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; const size_t trozo = 1 << 20; std::vector<char> buf(trozo + 4096); int n = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & PAGE_READWRITE);
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += trozo) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>(trozo + vent, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (SIZE_T i = 0; i + 4 <= g && i < trozo; i += 4) {
				uint32_t v; memcpy(&v, &buf[i], 4); if (v != ent) continue;
				bool hay = false; float fv = 0; SIZE_T fo = 0;
				for (int k = -vent; k <= vent && !hay; k += 4) { int64_t o = (int64_t)i + k; if (o < 0 || o + 4 > (int64_t)g) continue; float f; memcpy(&f, &buf[o], 4); if (std::fabs(f - f1) <= 0.06f || std::fabs(f - f2) <= 0.03f) { hay = true; fv = f; fo = (SIZE_T)o; } }
				if (!hay) continue;
				printf("@%llX (entero) float=%.2f a %+d bytes:", (unsigned long long)(p + i), fv, (int)((int64_t)fo - (int64_t)i));
				for (int k = -32; k <= 32; k++) { int64_t o = (int64_t)i + k * 4; if (o >= 0 && o + 4 <= (int64_t)g) { uint32_t u; memcpy(&u, &buf[o], 4); float f; memcpy(&f, &buf[o], 4); if (u > 0x30000000 && u < 0x50000000) printf(" %s%.2f", k == 0 ? "[" : "", f); else printf(" %s%u", k == 0 ? "[" : "", u); if (k == 0) printf("]"); } }
				printf("\n"); if (++n > 25) return 0;
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
