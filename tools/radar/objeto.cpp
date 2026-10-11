// Vuelca los primeros N bytes de cada objeto apuntado por un array de punteros (solo lectura). Uso: objeto <hexArray> <cuantos> <bytes>
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <vector>
int main(int argc, char** argv) {
	uint64_t arr = strtoull(argv[1], nullptr, 16); int n = atoi(argv[2]); int bytes = atoi(argv[3]);
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ, FALSE, pid); if (!h) return 2;
	std::vector<uint64_t> p(n); SIZE_T g; ReadProcessMemory(h, (LPCVOID)arr, p.data(), n * 8, &g);
	for (int i = 0; i < n; i++) {
		std::vector<uint32_t> d(bytes / 4); if (!ReadProcessMemory(h, (LPCVOID)p[i], d.data(), bytes, &g)) continue;
		printf("O %d %llX", i, (unsigned long long)p[i]); for (auto x : d) printf(" %X", x); printf("\n");
	}
}
