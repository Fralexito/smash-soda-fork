// Busca en la memoria de PES2021.exe todas las apariciones de un entero de 32 bits y muestra el contexto (solo lectura).
// Uso: buscaint <valor decimal> [palabras de contexto antes/despues, por defecto 12]
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
int main(int argc, char** argv) {
	uint32_t val = (uint32_t)strtoul(argv[1], nullptr, 10); int ctx = argc > 2 ? atoi(argv[2]) : 12;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; std::vector<char> buf((1 << 20) + 4096); int n = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += (1 << 20)) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>((1 << 20) + 4096, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (SIZE_T i = 0; i + 4 <= g && i < (1 << 20); i += 4) if (*(uint32_t*)&buf[i] == val) {
				printf("@%llX tipo=%lX prot=%lX:", (unsigned long long)(p + i), m.Type, m.Protect);
				for (int k = -ctx; k <= ctx; k++) { int64_t o = (int64_t)i + k * 4; if (o >= 0 && o + 4 <= (int64_t)g) printf(" %s%u", k == 0 ? "[" : "", *(uint32_t*)&buf[o]); if (k == 0) printf("]"); }
				printf("\n"); if (++n > 200) return 0;
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
