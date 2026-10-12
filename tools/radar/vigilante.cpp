// Vigilante de saques de centro (herramienta de investigacion, no forma parte de Link).
// Lee (solo lectura) las posiciones de los actores de una lista de direcciones (salida de entes.exe) a ~40 Hz; cuando ve la
// formacion de un saque de centro (≈10 jugadores en cada mitad, casi nadie en el centro, todos quietos 3 lecturas seguidas)
// CONGELA el proceso del juego (NtSuspendProcess), lanza las capturas (captura.bat) y lo reanuda siempre.
//   vigilante <archivo_direcciones> <segundos_maximo>
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
typedef LONG (NTAPI* NtProcFn)(HANDLE);
int main(int argc, char** argv) {
	if (argc < 3) return 1;
	std::vector<uint64_t> d; { FILE* f = fopen(argv[1], "r"); char l[256]; while (f && fgets(l, sizeof l, f)) d.push_back(strtoull(l, nullptr, 16)); if (f) fclose(f); }
	int maxSeg = atoi(argv[2]);
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); if (!pid) return 2;
	HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_SUSPEND_RESUME | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 3;
	NtProcFn sus = (NtProcFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSuspendProcess"), res = (NtProcFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtResumeProcess");
	printf("vigilando %zu actores durante %d s\n", d.size(), maxSeg); fflush(stdout);
	std::vector<float> prev(d.size() * 2, 1e9f); int quietas = 0; DWORD t0 = GetTickCount(), ultimoAviso = 0;
	while (GetTickCount() - t0 < (DWORD)maxSeg * 1000) {
		std::vector<float> cur(d.size() * 2, 1e9f); int validos = 0, izq = 0, der = 0, centro = 0; float maxd = 0;
		for (size_t i = 0; i < d.size(); i++) {
			float v[4]; SIZE_T g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)(d[i] + 208), v, 16, &g) || g != 16 || v[3] != 1.0f || !std::isfinite(v[0]) || std::fabs(v[0]) > 57 || std::fabs(v[2]) > 40 || v[1] <= 0 || v[1] >= 3) continue;
			cur[i*2] = v[0]; cur[i*2+1] = v[2]; validos++;
			if (std::fabs(v[2]) > 34.8f) continue; // asistentes
			if (v[0] < -2.5f) izq++; else if (v[0] > 2.5f) der++; else centro++;
			if (prev[i*2] < 1e8f) maxd = std::max(maxd, std::max(std::fabs(v[0] - prev[i*2]), std::fabs(v[2] - prev[i*2+1])));
		}
		double sz = 0, sz2 = 0; int nz = 0; for (size_t i = 0; i < d.size(); i++) if (cur[i*2] < 1e8f && std::fabs(cur[i*2+1]) <= 34.8f) { sz += cur[i*2+1]; sz2 += (double)cur[i*2+1] * cur[i*2+1]; nz++; }
		double sdz = nz ? std::sqrt(std::max(0.0, sz2 / nz - (sz / nz) * (sz / nz))) : 0;
		bool forma = validos >= 16 && izq >= 7 && izq <= 11 && der >= 7 && der <= 11 && centro <= 2 && sdz >= 9.0;
		quietas = (forma && maxd < 0.05f) ? quietas + 1 : 0;
		prev = cur;
		if (quietas >= 3) {
			printf("SAQUE DETECTADO: izq=%d der=%d centro=%d -> congelando\n", izq, der, centro); fflush(stdout);
			sus(h);
			system("cmd /c C:\\Temp\\claude\\radar\\captura.bat");
			res(h);
			printf("capturado y reanudado\n"); fflush(stdout);
			return 0;
		}
		if (GetTickCount() - ultimoAviso > 10000) { ultimoAviso = GetTickCount(); printf("t=%lus validos=%d izq=%d der=%d centro=%d maxd=%.2f\n", (GetTickCount() - t0) / 1000, validos, izq, der, centro, maxd); fflush(stdout); }
		Sleep(25);
	}
	printf("tiempo agotado sin saque\n");
}
