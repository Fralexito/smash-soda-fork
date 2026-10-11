// Busca el balon (solo lectura). Modo "buscar": con el balon quieto en el centro (saque inicial) guarda las direcciones donde hay
// un triple de floats (x~0, y en [0.05,0.8], z~0). Modo "ver <archivo>": relee esas direcciones varias veces y muestra cuales se mueven.
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
static HANDLE abrir() {
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); return pid ? OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid) : nullptr;
}
int main(int argc, char** argv) {
	HANDLE h = abrir(); if (!h) return 2;
	std::string modo = argc > 1 ? argv[1] : "buscar";
	if (modo == "buscar") {
		float tol = argc > 2 ? (float)atof(argv[2]) : 0.15f;
		uint64_t a = 0; MEMORY_BASIC_INFORMATION m; std::vector<char> buf((1 << 20) + 64); FILE* o = fopen("bola_cands.txt", "w"); int n = 0;
		while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
			bool ok = m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
			if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += (1 << 20)) {
				SIZE_T want = (SIZE_T)std::min<uint64_t>((1 << 20) + 16, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
				if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
				for (SIZE_T i = 0; i + 12 <= g && i < (1 << 20); i += 4) {
					float v[3]; memcpy(v, &buf[i], 12);
					if (std::fabs(v[0]) <= tol && std::fabs(v[2]) <= tol && v[1] > 0.05f && v[1] < 0.8f && (v[0] != 0 || v[2] != 0)) { fprintf(o, "%llX %.4f %.4f %.4f\n", (unsigned long long)(p + i), v[0], v[1], v[2]); n++; }
				}
			}
			a = (uint64_t)m.BaseAddress + m.RegionSize;
		}
		fclose(o); printf("candidatos: %d\n", n); return 0;
	}
	if (modo == "ver" && argc > 2) {
		std::vector<uint64_t> d; FILE* f = fopen(argv[2], "r"); char l[256]; while (f && fgets(l, sizeof l, f)) d.push_back(strtoull(l, nullptr, 16)); if (f) fclose(f);
		int muestras = argc > 3 ? atoi(argv[3]) : 6, ms = argc > 4 ? atoi(argv[4]) : 150;
		std::vector<std::vector<float>> S(d.size());
		for (int k = 0; k < muestras; k++) { for (size_t i = 0; i < d.size(); i++) { float v[3] = {}; SIZE_T g; ReadProcessMemory(h, (LPCVOID)d[i], v, 12, &g); S[i].push_back(v[0]); S[i].push_back(v[1]); S[i].push_back(v[2]); } Sleep(ms); }
		for (size_t i = 0; i < d.size(); i++) {
			float dx = 0, mx = 0; bool fin = true; float vmax = 0;
			for (int k = 1; k < muestras; k++) { float ddx = S[i][k*3] - S[i][(k-1)*3], ddz = S[i][k*3+2] - S[i][(k-1)*3+2]; float s = std::sqrt(ddx*ddx + ddz*ddz); vmax = std::max(vmax, s); mx += s; }
			for (float x : S[i]) if (!std::isfinite(x) || std::fabs(x) > 60) fin = false;
			if (fin && mx > 0.3f && vmax < 8.f) { printf("%llX movido=%.2f max_paso=%.2f:", (unsigned long long)d[i], mx, vmax); for (int k = 0; k < muestras; k++) printf(" (%.1f,%.2f,%.1f)", S[i][k*3], S[i][k*3+1], S[i][k*3+2]); printf("\n"); }
		}
	}
}
