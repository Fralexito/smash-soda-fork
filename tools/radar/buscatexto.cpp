// Busca un texto (ASCII y UTF-16) en la memoria de PES2021.exe y muestra el contexto (solo lectura).
// Uso: buscatexto <texto> [bytes de contexto antes/despues, por defecto 96]
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
int main(int argc, char** argv) {
	std::string t = argv[1]; int ctx = argc > 2 ? atoi(argv[2]) : 96;
	std::string w; for (char c : t) { w += c; w += '\0'; }
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; const size_t trozo = 1 << 20; std::vector<char> buf(trozo + 4096); int n = 0;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && m.Type != MEM_IMAGE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += trozo) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>(trozo + 2048, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
			for (int modo = 0; modo < 2; modo++) {
				const std::string& pat = modo ? w : t;
				for (SIZE_T i = 0; i + pat.size() <= g && i < trozo; i++) if (memcmp(&buf[i], pat.data(), pat.size()) == 0) {
					printf("@%llX %s: ", (unsigned long long)(p + i), modo ? "utf16" : "ascii");
					int64_t d = (int64_t)i - ctx, e = (int64_t)i + (int64_t)pat.size() + ctx; if (d < 0) d = 0; if (e > (int64_t)g) e = g;
					for (int64_t k = d; k < e; k++) { unsigned char c = buf[k]; printf("%c", (c >= 32 && c < 127) ? c : '.'); }
					printf("\n"); if (++n > 80) return 0;
				}
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
}
