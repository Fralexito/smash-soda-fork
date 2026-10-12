// Busca en toda la memoria de PES2021.exe objetos con la "firma" de un jugador en el campo (solo lectura).
// Firma (descubierta por ingenieria inversa, version de ejecutable concreta): en el objeto base b:
//   float4 en b+208 = (x, y~0.9, z, 1.0)  ; u32 en b+248 = 0x18 ; u32 en b+312 in {1,2} ; u32 en b+492 = 0x80000 | equipo
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
int main(int argc, char** argv) {
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; std::vector<char> buf((1 << 20) + 1024); int n = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += (1 << 20)) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>((1 << 20) + 640, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (SIZE_T i = 0; i + 520 <= g && i < (1 << 20); i += 16) {
				const char* b = &buf[i];
				uint32_t k = *(uint32_t*)(b + 248), t = *(uint32_t*)(b + 312), o = *(uint32_t*)(b + 492);
				uint32_t c = *(uint32_t*)(b + 440); if ((c & 0xFFFF) != 3) continue; (void)o; (void)t; (void)k;
				float v[4]; memcpy(v, b + 208, 16);
				if (!(v[3] == 1.0f && v[1] > 0.0f && v[1] < 3.0f && std::fabs(v[0]) < 80 && std::fabs(v[2]) < 60)) continue;
				printf("%llX team=%u act=%u x=%.2f y=%.2f z=%.2f\n", (unsigned long long)(p + i), t, *(uint32_t*)(b + 228), v[0], v[1], v[2]);
				if (argc > 1) { printf("RAW %llX", (unsigned long long)(p + i)); for (int w = 0; w < 160; w++) printf(" %X", *(uint32_t*)(b + w * 4)); printf("\n"); }
				n++;
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
	printf("total=%d\n", n);
}
