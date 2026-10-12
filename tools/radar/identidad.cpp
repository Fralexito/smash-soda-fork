// Busca cómo se enlaza cada actor del campo con SU jugador (dorsal / id de jugador). Solo lectura.
//  1) Barre la memoria y anota quién apunta a cada actor (puntero de 8 bytes con el valor exacto de la dirección del actor).
//  2) Para cada puntero hallado mira la ventana de ±0x100 bytes y, en cada desplazamiento, recoge el entero de 32 bits.
//  3) Muestra los desplazamientos cuyos valores son distintos por actor y plausibles (ids de jugador / dorsales).
// Uso: identidad <archivo con direcciones de actores en hex, una por línea> [mínimo de actores distintos, por defecto 15]
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
struct Hit { uint64_t loc; uint64_t actor; std::vector<uint32_t> ventana; }; // ventana: 128 enteros (−0x100..+0x100)
int main(int argc, char** argv) {
	std::set<uint64_t> actores; { FILE* f = fopen(argv[1], "r"); char l[256]; while (f && fgets(l, sizeof l, f)) actores.insert(strtoull(l, nullptr, 16)); if (f) fclose(f); }
	int minAct = argc > 2 ? atoi(argv[2]) : 15;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	uint64_t a = 0; MEMORY_BASIC_INFORMATION m; const size_t trozo = 1 << 20; std::vector<char> buf(trozo + 1024);
	std::vector<Hit> hits;
	while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		bool ok = m.State == MEM_COMMIT && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS));
		if (ok) for (uint64_t p = (uint64_t)m.BaseAddress; p < (uint64_t)m.BaseAddress + m.RegionSize; p += trozo) {
			SIZE_T want = (SIZE_T)std::min<uint64_t>(trozo + 512, (uint64_t)m.BaseAddress + m.RegionSize - p), g = 0;
			uint64_t desde = p >= 256 ? p - 256 : p; // algo de contexto antes del trozo
			SIZE_T extra = (SIZE_T)(p - desde);
			if (!ReadProcessMemory(h, (LPCVOID)desde, buf.data(), want + extra, &g) || g < extra + 8) continue;
			for (SIZE_T i = extra; i + 8 <= g && i < extra + trozo; i += 8) {
				uint64_t q; memcpy(&q, &buf[i], 8);
				if (!actores.count(q)) continue;
				Hit hi; hi.loc = desde + i; hi.actor = q; hi.ventana.assign(128, 0xFFFFFFFFu);
				for (int k = 0; k < 128; k++) { int64_t o = (int64_t)i - 256 + k * 4; if (o >= 0 && o + 4 <= (int64_t)g) memcpy(&hi.ventana[k], &buf[o], 4); }
				hits.push_back(std::move(hi));
			}
		}
		a = (uint64_t)m.BaseAddress + m.RegionSize;
	}
	printf("punteros a actores: %zu\n", hits.size());
	for (int k = 0; k < 128; k++) {
		std::map<uint64_t, std::set<uint32_t>> porActor; std::map<uint32_t, std::set<uint64_t>> porValor;
		for (const Hit& hi : hits) { uint32_t v = hi.ventana[k]; if (v >= 1 && v <= 5000000u && k != 64 && k != 65) { porActor[hi.actor].insert(v); porValor[v].insert(hi.actor); } }
		size_t nAct = porActor.size(); if ((int)nAct < minAct) continue;
		// valores que identifican a un solo actor (pocos actores comparten el valor)
		int unicos = 0; for (auto& pv : porValor) if (pv.second.size() == 1) unicos++;
		if (unicos < minAct) continue;
		printf("desplazamiento %+d: %zu actores, %zu valores distintos, %d exclusivos de un actor; ejemplos:", (k - 64) * 4, nAct, porValor.size(), unicos);
		int n = 0; for (auto& pa : porActor) { printf(" %llX=%u", (unsigned long long)pa.first, *pa.second.begin()); if (++n >= 6) break; }
		printf("\n");
	}
	FILE* o = fopen("identidad_hits.txt", "w");
	for (const Hit& hi : hits) { fprintf(o, "%llX actor=%llX:", (unsigned long long)hi.loc, (unsigned long long)hi.actor); for (int k = 0; k < 128; k++) fprintf(o, " %u", hi.ventana[k]); fprintf(o, "\n"); }
	fclose(o);
}
