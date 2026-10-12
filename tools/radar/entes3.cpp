// Variante de entes: firma relajada (sin exigir tipo 0x18 ni +312/+492) para encontrar el resto de actores (porteros, arbitro).
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
int main() {
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; std::vector<char> buf((1 << 20) + 1024);
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += (1 << 20)) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>((1 << 20) + 640, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (SIZE_T i = 0; i + 640 <= g && i < (1 << 20); i += 16) {
				const char* b = &buf[i];
				uint32_t c = *(uint32_t*)(b + 440), r0 = *(uint32_t*)(b + 624), r3 = *(uint32_t*)(b + 636);
				if ((c & 0xFFFF) != 3 || r0 != 0x10 || r3 != 0xDEAD) continue;
				float v[4]; memcpy(v, b + 208, 16);
				if (!(v[3] == 1.0f && v[1] > 0 && v[1] < 3 && std::fabs(v[0]) < 80 && std::fabs(v[2]) < 60)) continue;
				printf("%llX code=%u kind=%X lado=%u o492=%X x=%.1f y=%.2f z=%.1f\n", (unsigned long long)(p + i), c >> 16, *(uint32_t*)(b + 248), *(uint32_t*)(b + 312), *(uint32_t*)(b + 492), v[0], v[1], v[2]);
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
