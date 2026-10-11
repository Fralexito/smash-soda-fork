// Lee en vivo (solo lectura) un array de punteros a objetos con un vec4 (x,y,z,1) en un desplazamiento fijo.
// Uso: sonda <hexArray> <cuantos> <offset> [muestras] [msEntreMuestras]
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
int main(int argc, char** argv) {
	if (argc < 4) return 1;
	uint64_t arr = strtoull(argv[1], nullptr, 16); int n = atoi(argv[2]); int off = atoi(argv[3]);
	int muestras = argc > 4 ? atoi(argv[4]) : 1, ms = argc > 5 ? atoi(argv[5]) : 200;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ, FALSE, pid); if (!h) return 2;
	for (int m = 0; m < muestras; m++) {
		std::vector<uint64_t> p(n); SIZE_T g; ReadProcessMemory(h, (LPCVOID)arr, p.data(), n * 8, &g);
		printf("--- muestra %d\n", m);
		for (int i = 0; i < n; i++) {
			float v[4] = {}; if (p[i] < 0x10000 || !ReadProcessMemory(h, (LPCVOID)(p[i] + off), v, 16, &g)) { printf("%2d %llX ilegible\n", i, (unsigned long long)p[i]); continue; }
			printf("%2d %llX  x=%8.2f y=%6.2f z=%8.2f w=%.2f\n", i, (unsigned long long)p[i], v[0], v[1], v[2], v[3]);
		}
		fflush(stdout); Sleep(ms);
	}
}
