// Busca el balon (solo lectura + pausar/reanudar el proceso entre lecturas para que los pasos sean exactos).
//   bola2 vec4                       -> guarda en bola_cands.txt los (x,y,z,1.0) con y en [0.05,0.5] dentro del campo
//   bola2 pasos <archivo> N ms       -> N pasos de <ms>: reanuda, espera, congela, relee; informa de lo que se mueve suave
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
typedef LONG (NTAPI* NtProcFn)(HANDLE);
int main(int argc, char** argv) {
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = pid ? OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME, FALSE, pid) : nullptr; if (!h) return 2;
	std::string modo = argc > 1 ? argv[1] : "vec4";
	if (modo == "vec4") {
		uint64_t a = 0; MEMORY_BASIC_INFORMATION m; std::vector<char> buf((1 << 20) + 64); FILE* o = fopen("bola_cands.txt", "w"); int n = 0;
		while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
			bool ok = m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
			if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += (1 << 20)) {
				SIZE_T want = (SIZE_T)std::min<uint64_t>((1 << 20) + 16, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
				if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), want, &g)) continue;
				for (SIZE_T i = 0; i + 16 <= g && i < (1 << 20); i += 4) {
					float v[4]; memcpy(v, &buf[i], 16);
					if (v[3] == 1.0f && v[1] > 0.05f && v[1] < 0.5f && std::fabs(v[0]) < 53 && std::fabs(v[2]) < 35 && (std::fabs(v[0]) > 0.01f || std::fabs(v[2]) > 0.01f)) { fprintf(o, "%llX %.3f %.3f %.3f\n", (unsigned long long)(p + i), v[0], v[1], v[2]); n++; }
				}
			}
			a = (uint64_t)m.BaseAddress + m.RegionSize;
		}
		fclose(o); printf("candidatos: %d\n", n); return 0;
	}
	if (modo == "pasos" && argc > 4) {
		NtProcFn sus = (NtProcFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSuspendProcess"), res = (NtProcFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtResumeProcess");
		std::vector<uint64_t> d; FILE* f = fopen(argv[2], "r"); char l[256]; while (f && fgets(l, sizeof l, f)) d.push_back(strtoull(l, nullptr, 16)); if (f) fclose(f);
		int N = atoi(argv[3]), ms = atoi(argv[4]);
		std::vector<std::vector<float>> S(d.size());
		for (int k = 0; k < N; k++) {
			res(h); Sleep(ms); sus(h);
			for (size_t i = 0; i < d.size(); i++) { float v[4] = {}; SIZE_T g; ReadProcessMemory(h, (LPCVOID)d[i], v, 16, &g); S[i].push_back(v[0]); S[i].push_back(v[1]); S[i].push_back(v[2]); S[i].push_back(v[3]); }
		}
		res(h);
		for (size_t i = 0; i < d.size(); i++) {
			float tot = 0, vmax = 0; int rapidos = 0; bool fin = true;
			for (int k = 1; k < N; k++) { float dx = S[i][k*4] - S[i][(k-1)*4], dz = S[i][k*4+2] - S[i][(k-1)*4+2]; float sp = std::sqrt(dx*dx + dz*dz); tot += sp; vmax = std::max(vmax, sp); if (sp >= 0.6f) rapidos++; }
			for (int k = 0; k < N; k++) if (!(S[i][k*4+3] == 1.0f) || std::fabs(S[i][k*4]) > 60) fin = false;
			if (fin && rapidos >= 4 && vmax < 2.5f) { printf("%llX recorrido=%.1f paso_max=%.2f:", (unsigned long long)d[i], tot, vmax); for (int k = 0; k < N && k < 10; k++) printf(" (%.1f,%.2f,%.1f)", S[i][k*4], S[i][k*4+1], S[i][k*4+2]); printf("\n"); }
		}
	}
}
