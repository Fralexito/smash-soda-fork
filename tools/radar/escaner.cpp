// Phoenix · herramienta de INVESTIGACION (no forma parte de Link): busca en la memoria de PES2021.exe
// las posiciones de jugadores y balon, al estilo Cheat Engine. SOLO LEE (OpenProcess con PROCESS_VM_READ).
//
//   escaner.exe                  -> modo con teclas (global, el juego puede tener el foco):
//       Numpad 0  = escaneo inicial (guarda los floats con pinta de coordenada)
//       Numpad 1  = "CAMBIO desde la ultima foto"  (pulsar con el juego en pausa tras haber jugado unos segundos)
//       Numpad 2  = "IGUAL  desde la ultima foto"  (pulsar con el juego EN PAUSA, varios segundos despues de Numpad 1)
//       Numpad 3  = analizar: busca arrays de 22 (u 11) entradas con paso constante y los vuelca a analisis.txt
//       Numpad 9  = salir
//   escaner.exe dump <hexaddr> <bytes>   -> vuelca memoria
//   escaner.exe info                     -> regiones y modulos
//
// Compilar:  cl /O2 /EHsc /std:c++17 escaner.cpp /Fe:escaner.exe
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <cstdarg>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

struct Cand { uint64_t a; float v; uint64_t h = 0; };
static HANDLE gH = nullptr;
static DWORD gPid = 0;
static std::vector<Cand> gC;
static FILE* gLog = nullptr;

static void say(const char* f, ...) {
	char b[4096]; va_list ap; va_start(ap, f); vsnprintf(b, sizeof b, f, ap); va_end(ap);
	printf("%s\n", b); fflush(stdout);
	if (gLog) { fprintf(gLog, "%s\n", b); fflush(gLog); }
}

static DWORD findPid() {
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); return pid;
}

static std::vector<MEMORY_BASIC_INFORMATION> regiones() {
	std::vector<MEMORY_BASIC_INFORMATION> r; MEMORY_BASIC_INFORMATION mbi; uint64_t a = 0;
	while (VirtualQueryEx(gH, (LPCVOID)a, &mbi, sizeof mbi)) {
		bool ok = mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
			(mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
		if (ok) r.push_back(mbi);
		a = (uint64_t)mbi.BaseAddress + mbi.RegionSize;
	}
	return r;
}

static bool plausible(float v) { return std::isfinite(v) && std::fabs(v) >= 0.02f && std::fabs(v) <= 6000.0f; }

static void escaneoInicial() {
	gC.clear(); gC.reserve(50'000'000);
	auto rs = regiones(); uint64_t total = 0;
	std::vector<char> buf(1 << 20);
	for (auto& m : rs) {
		uint64_t base = (uint64_t)m.BaseAddress, fin = base + m.RegionSize;
		for (uint64_t p = base; p < fin; p += buf.size()) {
			SIZE_T n = (SIZE_T)std::min<uint64_t>(buf.size(), fin - p), got = 0;
			if (!ReadProcessMemory(gH, (LPCVOID)p, buf.data(), n, &got) || !got) continue;
			total += got;
			for (SIZE_T i = 0; i + 4 <= got; i += 4) {
				float v; memcpy(&v, &buf[i], 4);
				if (plausible(v)) gC.push_back({ p + i, v });
			}
		}
	}
	say("Escaneo inicial: %zu regiones, %.0f MB leidos, %zu candidatos", rs.size(), total / 1048576.0, gC.size());
}

// modo: 1 = cambio, 2 = igual
static void filtrar(int modo) {
	std::vector<Cand> out; out.reserve(gC.size() / 2);
	std::vector<char> buf(1 << 16);
	size_t i = 0;
	while (i < gC.size()) {
		uint64_t ini = gC[i].a & ~uint64_t(0xFFFF);
		size_t j = i; while (j < gC.size() && gC[j].a < ini + 0x10000) j++;
		SIZE_T got = 0;
		bool ok = ReadProcessMemory(gH, (LPCVOID)ini, buf.data(), 0x10000, &got) && got;
		if (!ok) { // pagina parcial: leer de a uno
			for (size_t k = i; k < j; k++) {
				float v; SIZE_T g = 0;
				if (ReadProcessMemory(gH, (LPCVOID)gC[k].a, &v, 4, &g) && g == 4 && plausible(v)) {
					bool cambio = v != gC[k].v; if ((modo == 1) == cambio) out.push_back({ gC[k].a, v });
				}
			}
		} else {
			for (size_t k = i; k < j; k++) {
				size_t off = (size_t)(gC[k].a - ini); if (off + 4 > got) continue;
				float v; memcpy(&v, &buf[off], 4);
				if (!plausible(v)) continue;
				bool cambio = v != gC[k].v; if ((modo == 1) == cambio) out.push_back({ gC[k].a, v });
			}
		}
		i = j;
	}
	gC.swap(out);
	say("%s: quedan %zu candidatos", modo == 1 ? "CAMBIO" : "IGUAL", gC.size());
}

// ---- modo automatico: historial de cambios por segundo (nadie tiene que pulsar en pausa) ----
static std::vector<int> gTickCambios;   // por tick: cuantos candidatos cambiaron
static int gTick = 0;
static void tick(bool podar) {
	std::vector<Cand> out; out.reserve(gC.size());
	std::vector<char> buf(1 << 16); size_t i = 0, cambiaron = 0;
	while (i < gC.size()) {
		uint64_t ini = gC[i].a & ~uint64_t(0xFFFF); size_t j = i; while (j < gC.size() && gC[j].a < ini + 0x10000) j++;
		SIZE_T got = 0; bool ok = ReadProcessMemory(gH, (LPCVOID)ini, buf.data(), 0x10000, &got) && got;
		for (size_t k = i; k < j; k++) {
			Cand c = gC[k]; float v = 0; bool vok = false;
			if (ok) { size_t off = (size_t)(c.a - ini); if (off + 4 <= got) { memcpy(&v, &buf[off], 4); vok = true; } }
			else { SIZE_T g = 0; vok = ReadProcessMemory(gH, (LPCVOID)c.a, &v, 4, &g) && g == 4; }
			if (vok && !std::isfinite(v)) vok = false;
			bool cambio = vok && v != c.v;
			if (cambio) { cambiaron++; if (gTick < 64) c.h |= (1ULL << gTick); }
			if (vok) c.v = v;
			if (!podar || c.h) out.push_back(c);
		}
		i = j;
	}
	gC.swap(out); gTickCambios.push_back((int)cambiaron);
	say("tick %d: cambiaron %zu, quedan %zu", gTick, cambiaron, gC.size());
	gTick++;
}
static void analizarHistorial() {
	if (FILE* hb = fopen("hist.bin", "wb")) { for (auto& c : gC) { fwrite(&c.a, 8, 1, hb); fwrite(&c.v, 4, 1, hb); fwrite(&c.h, 8, 1, hb); } fclose(hb); }
	int mx = 0; for (int c : gTickCambios) mx = std::max(mx, c);
	for (size_t t = 0; t < gTickCambios.size(); t++) say("t%zu=%d", t, gTickCambios[t]);
	uint64_t activos = 0, todos = 0; int na = 0, np = 0;
	for (size_t t = 0; t < gTickCambios.size() && t < 64; t++) {
		todos |= 1ULL << t;
		if (gTickCambios[t] > mx * 0.4) { activos |= 1ULL << t; na++; }
		else if (gTickCambios[t] < mx * 0.005) { np++; } else todos &= ~(1ULL << t); // transicion: no cuenta
	}
	say("ticks activos=%d parados=%d (max cambios=%d)", na, np, mx);
	std::vector<Cand> buenos;
	for (auto& c : gC) {
		uint64_t enParado = c.h & todos & ~activos;
		int ca = (int)__popcnt64(c.h & activos);
		if (enParado == 0 && ca >= std::max(2, na / 2)) buenos.push_back(c);
	}
	gC.swap(buenos);
	say("buenos (cambian solo con el juego en marcha): %zu", gC.size());
}

// ---- modo por pasos: el juego avanza ~dtMs, se congela y se leen TODOS los candidatos con el juego quieto ----
// Los valores que mueven una coordenada de jugador cambian poco entre pasos (< grande metros); el ruido salta o no cambia.
typedef LONG (NTAPI* NtProcFn)(HANDLE);
static void pasos(int N, int dtMs, float grande) {
	NtProcFn sus = (NtProcFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSuspendProcess");
	NtProcFn res = (NtProcFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtResumeProcess");
	if (!sus || !res) { say("sin NtSuspendProcess"); return; }
	for (auto& c : gC) c.h = 0; // bajo 16 bits: cuantos cambios; siguientes 16: saltos grandes
	for (int s = 0; s < N; s++) {
		res(gH); Sleep(dtMs); sus(gH);
		std::vector<Cand> out; out.reserve(gC.size()); std::vector<char> buf(1 << 16); size_t i = 0;
		while (i < gC.size()) {
			uint64_t ini = gC[i].a & ~uint64_t(0xFFFF); size_t j = i; while (j < gC.size() && gC[j].a < ini + 0x10000) j++;
			SIZE_T got = 0; bool ok = ReadProcessMemory(gH, (LPCVOID)ini, buf.data(), 0x10000, &got) && got;
			for (size_t k = i; k < j; k++) {
				Cand c = gC[k]; float v = 0; bool vok = false;
				if (ok) { size_t off = (size_t)(c.a - ini); if (off + 4 <= got) { memcpy(&v, &buf[off], 4); vok = true; } }
				else { SIZE_T g = 0; vok = ReadProcessMemory(gH, (LPCVOID)c.a, &v, 4, &g) && g == 4; }
				if (!vok || !std::isfinite(v)) continue;
				float d = std::fabs(v - c.v);
				uint64_t ch = c.h & 0xFFFF, big = (c.h >> 16) & 0xFFFF;
				if (d > 0) ch++;
				if (d > grande + 0.02f * std::fabs(v)) big++;
				if (big > 1) continue;
				if (s == 2 && ch == 0) continue;
				c.v = v; c.h = ch | (big << 16);
				out.push_back(c);
			}
			i = j;
		}
		gC.swap(out);
		say("paso %d/%d: quedan %zu", s + 1, N, gC.size());
	}
	res(gH);
	std::vector<Cand> fin; for (auto& c : gC) if ((c.h & 0xFFFF) >= (uint64_t)(N / 3)) fin.push_back(c);
	gC.swap(fin);
	say("pasos listos: %zu candidatos suaves", gC.size());
	if (FILE* hb = fopen("pasos.bin", "wb")) { for (auto& c : gC) { fwrite(&c.a, 8, 1, hb); fwrite(&c.v, 4, 1, hb); } fclose(hb); }
}

// ---- busca ARRAYS DE PUNTEROS (>=20 entradas) cuyos objetos contienen, todos al mismo desplazamiento, un candidato suave ----
static void punteros() {
	if (gC.empty()) { if (FILE* pb = fopen("pasos.bin", "rb")) { Cand c; while (fread(&c.a, 8, 1, pb) == 1 && fread(&c.v, 4, 1, pb) == 1) gC.push_back(c); fclose(pb); } say("cargados %zu candidatos de pasos.bin", gC.size()); }
	std::sort(gC.begin(), gC.end(), [](const Cand& x, const Cand& y) { return x.a < y.a; });
	std::vector<uint64_t> ca; for (auto& c : gC) ca.push_back(c.a);
	auto rs = regiones(); std::vector<char> buf(1 << 20);
	FILE* f = fopen("punteros.txt", "w"); size_t hits = 0, runsTot = 0;
	auto esPtr = [](uint64_t q) { return q >= 0x1000000ULL && q < 0x800000000ULL && (q & 7) == 0; };
	for (int strideQ : { 1, 2, 4 }) {
		for (auto& m : rs) {
			uint64_t base = (uint64_t)m.BaseAddress, fin = base + m.RegionSize;
			for (uint64_t p = base; p < fin; p += buf.size()) {
				SIZE_T n = (SIZE_T)std::min<uint64_t>(buf.size(), fin - p), got = 0;
				if (!ReadProcessMemory(gH, (LPCVOID)p, buf.data(), n, &got) || got < 8) continue;
				size_t nq = got / 8; const uint64_t* q = (const uint64_t*)buf.data();
				for (size_t i = 0; i + 20 * strideQ < nq; i++) {
					// racha de punteros con paso strideQ qwords
					int len = 0; while (i + (size_t)len * strideQ < nq && len < 40 && esPtr(q[i + len * strideQ])) len++;
					if (len < 20) continue;
					runsTot++;
					std::vector<uint64_t> ps; for (int k = 0; k < len; k++) ps.push_back(q[i + k * strideQ]);
					std::sort(ps.begin(), ps.end()); if (std::unique(ps.begin(), ps.end()) - ps.begin() < 20) { i += (size_t)len * strideQ - 1; continue; }
					ps.clear(); for (int k = 0; k < len; k++) ps.push_back(q[i + k * strideQ]);
					std::unordered_map<int64_t, int> votos;
					for (uint64_t pp : ps) {
						auto it = std::lower_bound(ca.begin(), ca.end(), pp > 0x400 ? pp - 0x400 : 0);
						for (; it != ca.end() && *it < pp + 0x6000; ++it) votos[(int64_t)(*it - pp)]++;
					}
					for (auto& v : votos) if (v.second >= 17) {
						hits++;
						fprintf(f, "array@%llX paso=%d len=%d offset=%lld votos=%d\n", (unsigned long long)(p + i * 8), strideQ * 8, len, (long long)v.first, v.second);
						for (int k = 0; k < len && k < 24; k++) { float x[4] = {}; SIZE_T g; ReadProcessMemory(gH, (LPCVOID)(ps[k] + v.first), x, 16, &g); fprintf(f, "  %llX -> %.2f %.2f %.2f %.2f\n", (unsigned long long)ps[k], x[0], x[1], x[2], x[3]); }
					}
					i += (size_t)len * strideQ - 1;
				}
			}
		}
	}
	fclose(f); say("punteros: %zu rachas, %zu aciertos (punteros.txt)", runsTot, hits);
}

static void analizar() {
	FILE* f = fopen("analisis.txt", "w");
	std::sort(gC.begin(), gC.end(), [](const Cand& x, const Cand& y) { return x.a < y.a; });
	std::unordered_map<uint64_t, float> mapa; mapa.reserve(gC.size() * 2);
	for (auto& c : gC) mapa[c.a] = c.v;
	std::unordered_map<uint64_t, size_t> hist;
	for (size_t i = 0; i < gC.size(); i++)
		for (size_t k = 1; k <= 24 && i + k < gC.size(); k++) { uint64_t d = gC[i + k].a - gC[i].a; if (d <= 0x4000) hist[d]++; }
	std::vector<std::pair<size_t, uint64_t>> top; for (auto& h : hist) top.push_back({ h.second, h.first });
	std::sort(top.rbegin(), top.rend());
	if (top.size() > 60) top.resize(60);
	std::vector<std::string> lineas;
	for (auto& t : top) {
		uint64_t S = t.second;
		for (auto& c : gC) {
			if (mapa.count(c.a - S)) continue; // solo el inicio de cadena
			int n = 1; while (mapa.count(c.a + n * S)) n++;
			if (n >= 11) {
				std::string s = "S=0x" + std::to_string(S) + " (" + std::to_string(S) + ") inicio=0x";
				char tmp[64]; snprintf(tmp, sizeof tmp, "%llX", (unsigned long long)c.a); s += tmp; s += " n=" + std::to_string(n) + " valores:";
				for (int k = 0; k < n && k < 24; k++) { snprintf(tmp, sizeof tmp, " %.2f", mapa[c.a + k * S]); s += tmp; }
				lineas.push_back(s);
			}
		}
	}
	for (auto& l : lineas) { fprintf(f, "%s\n", l.c_str()); }
	fclose(f);
	say("Analisis: %zu cadenas (>=11 entradas con paso constante) escritas en analisis.txt", lineas.size());
	// ademas guarda los candidatos crudos
	FILE* g = fopen("candidatos.txt", "w");
	for (size_t i = 0; i < gC.size() && i < 3'000'000; i++) fprintf(g, "%llX %.4f\n", (unsigned long long)gC[i].a, gC[i].v);
	fclose(g);
}

int main(int argc, char** argv) {
	gLog = fopen("escaner.log", "a");
	gPid = findPid();
	if (!gPid) { say("PES2021.exe no esta abierto."); return 1; }
	gH = OpenProcess(PROCESS_VM_READ | PROCESS_SUSPEND_RESUME | SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, gPid);
	if (!gH) { say("No se pudo abrir el proceso (error %lu).", GetLastError()); return 1; }
	if (argc >= 2 && std::string(argv[1]) == "info") {
		auto rs = regiones(); uint64_t t = 0; for (auto& m : rs) t += m.RegionSize;
		say("pid=%lu regiones RW=%zu total=%.0f MB", gPid, rs.size(), t / 1048576.0);
		HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, gPid); MODULEENTRY32W me{}; me.dwSize = sizeof me;
		if (Module32FirstW(s, &me)) do { if (wcsstr(me.szModule, L"PES2021") || wcsstr(me.szModule, L"sider")) wprintf(L"%s base=%p size=%lu\n", me.szModule, me.modBaseAddr, me.modBaseSize); } while (Module32NextW(s, &me));
		return 0;
	}
	if (argc >= 4 && std::string(argv[1]) == "dump") {
		uint64_t a = strtoull(argv[2], nullptr, 16); size_t n = strtoul(argv[3], nullptr, 0);
		std::vector<unsigned char> b(n); SIZE_T g = 0; ReadProcessMemory(gH, (LPCVOID)a, b.data(), n, &g);
		for (size_t i = 0; i + 4 <= g; i += 4) { float v; memcpy(&v, &b[i], 4); uint32_t u; memcpy(&u, &b[i], 4);
			printf("%llX +%04zX  %08X  %g\n", (unsigned long long)(a + i), i, u, v); }
		return 0;
	}
	say("Conectado a PES2021.exe (pid %lu). Numpad0=escaneo, 1=cambio, 2=igual, 3=analizar, 9=salir", gPid);
	bool prev[10] = {}; bool autoMode = false; DWORD ultimo = 0;
	for (;;) {
		int fileCmd = -1;
		if (FILE* cf = fopen("cmd.txt", "r")) { int c = -1; if (fscanf(cf, "%d", &c) == 1) fileCmd = c; fclose(cf); remove("cmd.txt"); }
		for (int k = 0; k <= 9; k++) {
			bool d = (GetAsyncKeyState(VK_NUMPAD0 + k) & 0x8000) != 0;
			if (!d && fileCmd == k) { d = true; fileCmd = -1; }
			if (d && !prev[k]) {
				if (k == 0) escaneoInicial(); else if (k == 1) filtrar(1); else if (k == 2) filtrar(2);
				else if (k == 3) analizar();
				else if (k == 4) { for (int i = 0; i < 3; i++) { tick(i == 2); Sleep(700); } autoMode = true; }
				else if (k == 5) { autoMode = false; analizarHistorial(); }
				else if (k == 8) punteros();
				else if (k == 6) pasos(40, 50, 0.5f);
				else if (k == 7) pasos(40, 50, 3.0f);
				else if (k == 9) return 0;
				Beep(k == 3 ? 1500 : 900, 120);
			}
			prev[k] = d;
		}
		if (autoMode && GetTickCount() - ultimo >= 1000) {
			ultimo = GetTickCount();
			if (gTick < 63) tick(false); else { autoMode = false; analizarHistorial(); Beep(1500, 200); }
		}
		Sleep(30);
		if (WaitForSingleObject(gH, 0) == WAIT_OBJECT_0) { say("El juego se cerro."); return 0; }
	}
}
