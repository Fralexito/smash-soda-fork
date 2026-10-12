// Phoenix · herramienta de INVESTIGACION (no forma parte de Link): busca en la memoria de PES2021.exe el numero de
// «Datos personales» (Usuario N) que tiene un mando en la pantalla de seleccion de mandos.
// SOLO LEE (OpenProcess con PROCESS_VM_READ). Nunca escribe ni congela el juego.
//
// Idea: en esa pantalla, R2 pasa al Usuario siguiente (N -> N+1). Se toma una foto, se pulsa R2 una vez, otra foto...
// Un candidato sobrevive si su valor sube exactamente +1 en cada foto (con el contador del Usuario, base 0 o base 1).
//
//   usuariopad.exe                 -> modo con teclas (globales, el juego puede tener el foco):
//       Numpad 5 = foto (la primera guarda el estado de partida; las siguientes, DESPUES de pulsar R2 una vez)
//       Numpad 4 = foto despues de pulsar L2 una vez (el valor baja 1)
//       Numpad 3 = informe (usuariopad.txt)
//       Numpad 0 = empezar de cero
//       Numpad 9 = salir
//     Tambien acepta un archivo cmd.txt con 5/4/3/0/9.
//   usuariopad.exe ver <hexaddr> [...]  -> muestra en vivo esos bytes (para confirmar)
//   usuariopad.exe buscar <texto>       -> donde aparece ese texto (ej. un nombre de Usuario) en la memoria de PES
//
// Compilar:  cl /O2 /EHsc /std:c++17 usuariopad.cpp /Fe:usuariopad.exe
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#pragma comment(lib, "user32.lib")

static HANDLE gH = nullptr;
static FILE* gLog = nullptr;
static uint64_t gExeBase = 0, gExeTam = 0;

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

static void buscarExe(DWORD pid) {
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid); MODULEENTRY32W me{}; me.dwSize = sizeof me;
	if (Module32FirstW(s, &me)) do { if (_wcsicmp(me.szModule, L"PES2021.exe") == 0) { gExeBase = (uint64_t)me.modBaseAddr; gExeTam = me.modBaseSize; break; } } while (Module32NextW(s, &me));
	CloseHandle(s);
}

struct Region { uint64_t base, tam; };
static std::vector<Region> regiones() {
	std::vector<Region> r; MEMORY_BASIC_INFORMATION mbi; uint64_t a = 0;
	while (VirtualQueryEx(gH, (LPCVOID)a, &mbi, sizeof mbi)) {
		bool ok = mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
			(mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
		if (ok) r.push_back({ (uint64_t)mbi.BaseAddress, (uint64_t)mbi.RegionSize });
		a = (uint64_t)mbi.BaseAddress + mbi.RegionSize;
	}
	return r;
}

struct Cand { uint64_t a; std::vector<uint8_t> v; };   // valores en cada foto
static std::map<uint64_t, std::vector<uint8_t>> gBase;   // primera foto completa
static std::vector<Cand> gC;
static bool gHayCand = false;
static int gFotos = 0;

static void reiniciar() { gBase.clear(); gC.clear(); gC.shrink_to_fit(); gHayCand = false; gFotos = 0; say("== de cero =="); }

static void fotoCompleta() {
	gBase.clear(); uint64_t total = 0;
	for (auto& r : regiones()) {
		std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
		if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
		total += got; gBase[r.base] = std::move(b);
	}
	say("foto 1 (estado de partida): %zu regiones, %.0f MB. Ahora pulsa R2 UNA vez y Numpad 5", gBase.size(), total / 1048576.0);
}

// segunda foto: candidatos = bytes que pasaron de v a v+delta (delta = +1 tras R2, -1 tras L2)
static void primerosCandidatos(int delta) {
	size_t n = 0; gC.clear();
	for (auto& r : regiones()) {
		auto it = gBase.find(r.base);
		if (it == gBase.end() || it->second.size() != r.tam) continue;
		std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
		if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
		const uint8_t* o = it->second.data();
		for (size_t i = 0; i < r.tam; i++) if (o[i] < 40 && b[i] == (uint8_t)(o[i] + delta)) gC.push_back({ r.base + i, { o[i], b[i] } });
	}
	n = gC.size(); gBase.clear(); gHayCand = true;
	say("candidatos (bytes que cambiaron %+d): %zu. Repite: R2 una vez y Numpad 5", delta, n);
}

static void leerCand(std::vector<uint8_t>& vals, std::vector<uint8_t>& ok) {
	vals.assign(gC.size(), 0); ok.assign(gC.size(), 0);
	std::vector<uint8_t> buf(0x10000); size_t i = 0;
	while (i < gC.size()) {
		uint64_t ini = gC[i].a & ~uint64_t(0xFFFF); size_t j = i; while (j < gC.size() && gC[j].a < ini + 0x10000) j++;
		SIZE_T got = 0; bool rok = ReadProcessMemory(gH, (LPCVOID)ini, buf.data(), 0x10000, &got) && got;
		for (size_t k = i; k < j; k++) {
			if (rok) { size_t off = (size_t)(gC[k].a - ini); if (off < got) { vals[k] = buf[off]; ok[k] = 1; } }
			else { SIZE_T g = 0; uint8_t v; if (ReadProcessMemory(gH, (LPCVOID)gC[k].a, &v, 1, &g) && g == 1) { vals[k] = v; ok[k] = 1; } }
		}
		i = j;
	}
}

static void filtrar(int delta) {
	// dos lecturas separadas 400 ms: lo que se mueve solo se descarta
	std::vector<uint8_t> v1, o1, v2, o2;
	leerCand(v1, o1); Sleep(400); leerCand(v2, o2);
	std::vector<Cand> out; out.reserve(gC.size());
	for (size_t k = 0; k < gC.size(); k++) {
		if (!o1[k] || !o2[k] || v1[k] != v2[k]) continue;
		if (v1[k] != (uint8_t)(gC[k].v.back() + delta)) continue;
		Cand c = gC[k]; c.v.push_back(v1[k]); out.push_back(std::move(c));
	}
	gC.swap(out);
	say("foto %d: quedan %zu candidatos", gFotos, gC.size());
}

static void foto(int delta) {
	gFotos++;
	if (!gHayCand) { if (gFotos == 1) fotoCompleta(); else primerosCandidatos(delta); }
	else filtrar(delta);
}

static void informe() {
	FILE* f = fopen("usuariopad.txt", "w"); if (!f) return;
	fprintf(f, "fotos=%d  candidatos=%zu  exe=%llX..%llX\n", gFotos, gC.size(), (unsigned long long)gExeBase, (unsigned long long)(gExeBase + gExeTam));
	size_t mostrados = 0;
	for (auto& c : gC) {
		if (mostrados++ >= 400) break;
		bool enExe = c.a >= gExeBase && c.a < gExeBase + gExeTam;
		fprintf(f, "%llX%s  valores:", (unsigned long long)c.a, enExe ? " [EXE]" : "");
		for (auto v : c.v) fprintf(f, " %u", v);
		fprintf(f, "\n");
		if (mostrados <= 40) {
			uint8_t b[64] = {}; SIZE_T g = 0; uint64_t ini = (c.a & ~uint64_t(15)) - 16;
			ReadProcessMemory(gH, (LPCVOID)ini, b, 64, &g);
			for (int l = 0; l < 4; l++) { fprintf(f, "    %llX:", (unsigned long long)(ini + l * 16)); for (int k = 0; k < 16; k++) fprintf(f, " %02X", b[l * 16 + k]); fprintf(f, "\n"); }
		}
	}
	fclose(f);
	say("informe: %zu candidatos -> usuariopad.txt", gC.size());
}


// buscar <texto>: lista donde aparece ese texto (ASCII) en la memoria de PES, con el registro de 2432 B si cuadra con un Usuario
static void buscarTexto(const char* txt) {
	const size_t n = strlen(txt); int hallazgos = 0;
	FILE* f = fopen("usuariopad_buscar.txt", "w");
	for (auto& r : regiones()) {
		std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
		if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
		for (size_t i = 0; i + n < r.tam; i++) {
			if (memcmp(&b[i], txt, n) != 0) continue;
			const uint64_t a = r.base + i; hallazgos++;
			const bool enExe = a >= gExeBase && a < gExeBase + gExeTam;
			// un registro de Usuario empieza 4 bytes antes del nombre (u32 0/1) y la cola del nombre (48 B) esta a cero
			const bool pareceUsuario = i >= 4 && b[i - 1] == 0 && b[i - 2] == 0 && b[i - 3] == 0 && b[i - 4] <= 1;
			if (f) fprintf(f, "%llX%s%s  inicio_de_registro_si_Usuario=%llX\n", (unsigned long long)a, enExe ? " [EXE]" : "", pareceUsuario ? " [PARECE USUARIO]" : "", (unsigned long long)(a - 4));
			if (f && pareceUsuario) {
				fprintf(f, "    siguiente bloque de 32 bytes a +0x34 (configuracion):");
				for (int k = 0x34; k < 0x34 + 24 && i - 4 + k < r.tam; k++) fprintf(f, " %02X", b[i - 4 + k]);
				fprintf(f, "\n");
			}
		}
	}
	if (f) fclose(f);
	say("buscar «%s»: %d sitios -> usuariopad_buscar.txt", txt, hallazgos);
}

static void ver(int argc, char** argv) {
	std::vector<uint64_t> as; for (int i = 2; i < argc; i++) as.push_back(strtoull(argv[i], nullptr, 16));
	std::string antes; DWORD t0 = GetTickCount();
	for (;;) {
		std::string s; for (auto a : as) { uint8_t v = 0; SIZE_T g = 0; ReadProcessMemory(gH, (LPCVOID)a, &v, 1, &g); char t[64]; snprintf(t, sizeof t, " %llX=%3u", (unsigned long long)a, v); s += t; }
		if (s != antes) { say("%6.1f s%s", (GetTickCount() - t0) / 1000.0, s.c_str()); antes = s; }
		Sleep(100);
		if (WaitForSingleObject(gH, 0) == WAIT_OBJECT_0) return;
	}
}

int main(int argc, char** argv) {
	gLog = fopen("usuariopad.log", "a");
	DWORD pid = findPid();
	if (!pid) { say("PES2021.exe no esta abierto."); return 1; }
	gH = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
	if (!gH) { say("No se pudo abrir el proceso (error %lu).", GetLastError()); return 1; }
	buscarExe(pid);
	if (argc >= 3 && std::string(argv[1]) == "ver") { ver(argc, argv); return 0; }
	if (argc >= 3 && std::string(argv[1]) == "buscar") { buscarTexto(argv[2]); return 0; }
	say("Conectado a PES2021.exe (pid %lu). Numpad 5=foto (tras R2) 4=foto (tras L2) 3=informe 0=de cero 9=salir", pid);
	bool prev[10] = {};
	for (;;) {
		int fileCmd = -1;
		if (FILE* cf = fopen("cmd.txt", "r")) { int c = -1; if (fscanf(cf, "%d", &c) == 1) fileCmd = c; fclose(cf); remove("cmd.txt"); }
		for (int k = 0; k <= 9; k++) {
			bool d = (GetAsyncKeyState(VK_NUMPAD0 + k) & 0x8000) != 0;
			if (!d && fileCmd == k) { d = true; fileCmd = -1; }
			if (d && !prev[k]) {
				if (k == 5) foto(+1); else if (k == 4) foto(-1);
				else if (k == 3) informe(); else if (k == 0) reiniciar();
				else if (k == 9) return 0;
				if (k == 0 || k == 3 || k == 4 || k == 5) Beep(k == 3 ? 1500 : 700 + 150 * k, 120);
			}
			prev[k] = d;
		}
		Sleep(30);
		if (WaitForSingleObject(gH, 0) == WAIT_OBJECT_0) { say("El juego se cerro."); return 0; }
	}
}
