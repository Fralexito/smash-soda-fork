// Busca en la memoria de PES2021.exe quien contiene un valor de 8 bytes (puntero). Solo lectura.
// Uso: buscaptr <hexvalor> [tolerancia en bytes hacia abajo, p.ej. 0x40]
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <vector>
int main(int argc, char** argv) {
	uint64_t val = strtoull(argv[1], nullptr, 16); uint64_t tol = argc > 2 ? strtoull(argv[2], nullptr, 16) : 0;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; std::vector<char> buf(1 << 20); int hits = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += buf.size()) {
			SIZE_T n = (SIZE_T)std::min<uint64_t>(buf.size(), (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), n, &g)) continue;
			for (SIZE_T i = 0; i + 8 <= g; i += 8) { uint64_t q = *(uint64_t*)&buf[i]; if (q >= val - tol && q <= val) { printf("%llX contiene %llX (+%llX) region=%llX tipo=%lX prot=%lX\n", (unsigned long long)(p + i), (unsigned long long)q, (unsigned long long)(val - q), (unsigned long long)m.BaseAddress, m.Type, m.Protect); if (++hits > 60) return 0; } }
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
