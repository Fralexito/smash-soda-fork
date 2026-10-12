// Busca en la memoria una "tabla de estadisticas": ventanas donde aparecen varios de los valores dados (como u32 o u16).
// Uso: buscaserie <minimo_aciertos> <ventana_bytes> v1 v2 v3 ...   (solo lectura)
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <set>
#include <vector>
#include <algorithm>
int main(int argc, char** argv) {
	int minimo = atoi(argv[1]), vent = atoi(argv[2]); std::set<uint32_t> V; for (int i = 3; i < argc; i++) V.insert(atoi(argv[i]));
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; const size_t trozo = 1 << 20; std::vector<char> buf(trozo + 4096); int n = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & PAGE_READWRITE);
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += trozo) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>(trozo + vent, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (int modo = 0; modo < 3; modo++) { int paso = modo == 2 ? 1 : modo ? 2 : 4;
				for (SIZE_T i = 0; i + vent <= g && i < trozo; i += 4) {
					std::set<uint32_t> vistos;
					for (int k = 0; k < vent; k += paso) { uint32_t v = modo == 2 ? (uint8_t)buf[i + k] : modo ? *(uint16_t*)&buf[i + k] : *(uint32_t*)&buf[i + k]; if (V.count(v)) vistos.insert(v); }
					int seq = 0; for (int k = 0; k + paso < vent; k += paso) { uint32_t v0 = modo ? *(uint16_t*)&buf[i + k] : *(uint32_t*)&buf[i + k], v1 = modo ? *(uint16_t*)&buf[i + k + paso] : *(uint32_t*)&buf[i + k + paso]; if (v1 == v0 + 1) seq++; }
					if ((int)vistos.size() >= minimo && seq < 6) {
						printf("@%llX %s aciertos=%zu:", (unsigned long long)(p + i), modo == 2 ? "u8" : modo ? "u16" : "u32", vistos.size());
						for (int k = 0; k < vent; k += paso) printf(" %u", modo == 2 ? (uint8_t)buf[i + k] : modo ? *(uint16_t*)&buf[i + k] : *(uint32_t*)&buf[i + k]);
						printf("\n"); i += vent - 4; if (++n > 30) return 0;
					}
				} }
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
