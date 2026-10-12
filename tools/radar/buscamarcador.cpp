// Busca la estructura de estadisticas del partido (la que lee match.stats() de Sider): pares (local, visita) con el
// marcador actual y el reloj cerca. Solo lectura. Uso: buscamarcador <local> <visita> <minuto_min> <minuto_max>
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
int main(int argc, char** argv) {
	uint32_t gl = atoi(argv[1]), gv = atoi(argv[2]), m0 = atoi(argv[3]), m1 = atoi(argv[4]);
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; const size_t trozo = 1 << 20; std::vector<char> buf(trozo + 1024); int n = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += trozo) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>(trozo + 512, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (SIZE_T i = 0; i + 8 <= g && i < trozo; i += 4) {
				uint32_t x = *(uint32_t*)&buf[i], y = *(uint32_t*)&buf[i + 4];
				if (x != gl || y != gv) continue;
				bool reloj = false; for (int k = -16; k <= 16 && !reloj; k++) { int64_t o = (int64_t)i + k * 4; if (o >= 0 && o + 4 <= (int64_t)g) { uint32_t v = *(uint32_t*)&buf[o]; if (v >= m0 && v <= m1) reloj = true; } }
				if (!reloj) continue;
				printf("@%llX:", (unsigned long long)(p + i));
				for (int k = -16; k <= 24; k++) { int64_t o = (int64_t)i + k * 4; if (o >= 0 && o + 4 <= (int64_t)g) printf(" %s%u", k == 0 ? "[" : "", *(uint32_t*)&buf[o]); if (k == 1) printf("]"); }
				printf("\n"); if (++n > 40) return 0;
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
