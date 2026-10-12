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

// ---- PRUEBA DE ESCRITURA (reversible): cambia SOLO el nombre (48 bytes) de un Usuario VACIO en la copia que PES tiene en memoria ----
// escribir <nombre de un Usuario existente> <su casilla 1..32> <casilla VACIA a probar> [texto]
// Localiza la tabla de 32 Usuarios (2432 B cada uno) por el nombre existente, la valida entera, comprueba que la casilla
// de prueba esta vacia, escribe el texto y espera Numpad 8 (o cerrar la ventana / cerrar PES) para RESTAURAR los 48 bytes.
// No toca ningun archivo. Si la tabla no se reconoce con certeza, no escribe nada.
static const size_t kRegU = 2432, kNomOff = 4, kNomLen = 48;
static HANDLE gHW = nullptr;
static uint64_t gDirNombre = 0; static uint8_t gOrig[48]; static bool gEscrito = false;

static void restaurar() {
	if (!gEscrito || !gHW) return;
	SIZE_T w = 0; WriteProcessMemory(gHW, (LPVOID)gDirNombre, gOrig, kNomLen, &w);
	say("RESTAURADO el nombre original (%zu bytes).", (size_t)w); gEscrito = false;
}
static BOOL WINAPI alCerrar(DWORD) { restaurar(); return FALSE; }

static bool registroUsuarioValido(const uint8_t* r) {
	if (r[1] || r[2] || r[3] || r[0] > 1) return false;
	size_t fin = 0; while (fin < kNomLen && r[kNomOff + fin]) fin++;
	if (fin == kNomLen) return false;
	for (size_t i = 0; i < fin; i++) if (r[kNomOff + i] < 0x20 || r[kNomOff + i] == 0x7F) return false;
	return true;
}

static void escribirPrueba(int argc, char** argv, DWORD pid) {
	if (argc < 5) { say("uso: escribir <nombre existente> <su casilla 1..32> <casilla vacia a probar> [texto]"); return; }
	const std::string nombre = argv[2]; const int casilla = atoi(argv[3]), prueba = atoi(argv[4]);
	const std::string texto = argc >= 6 ? argv[5] : "ZZMEM";
	if (casilla < 1 || casilla > 32 || prueba < 1 || prueba > 32 || texto.size() > 30) { say("casillas 1..32 y texto de 30 letras como maximo"); return; }
	std::vector<uint64_t> tablas;
	for (auto& r : regiones()) {
		std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
		if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
		for (size_t i = kNomOff; i + nombre.size() < r.tam; i++) {
			if (memcmp(&b[i], nombre.data(), nombre.size()) != 0) continue;
			const int64_t ini = (int64_t)i - kNomOff - (int64_t)(casilla - 1) * kRegU;   // inicio de la tabla dentro de la region
			if (ini < 0 || (size_t)ini + 32 * kRegU > r.tam) continue;
			bool ok = true;
			for (int u = 0; u < 32 && ok; u++) ok = registroUsuarioValido(&b[ini + u * kRegU]);
			if (ok) tablas.push_back(r.base + ini);
		}
	}
	if (tablas.size() != 1) { say("Tabla de Usuarios: %zu candidatas (debe haber exactamente 1). No escribo nada.", tablas.size()); for (auto t : tablas) say("  candidata %llX", (unsigned long long)t); return; }
	const uint64_t tabla = tablas[0];
	const uint64_t reg = tabla + (uint64_t)(prueba - 1) * kRegU;
	uint8_t nom[48]; SIZE_T g = 0;
	if (!ReadProcessMemory(gH, (LPCVOID)(reg + kNomOff), nom, kNomLen, &g) || g != kNomLen) { say("No pude leer la casilla de prueba."); return; }
	if (nom[0] != 0) { say("La casilla %d NO esta vacia (tiene un nombre). Elige otra casilla vacia. No escribo nada.", prueba); return; }
	say("Tabla de Usuarios encontrada en %llX (32 registros validos). Casilla %d vacia.", (unsigned long long)tabla, prueba);
	gHW = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
	if (!gHW) { say("No pude abrir PES con permiso de escritura (error %lu).", GetLastError()); return; }
	memcpy(gOrig, nom, kNomLen); gDirNombre = reg + kNomOff;
	uint8_t nuevo[48] = {}; memcpy(nuevo, texto.data(), texto.size());
	SetConsoleCtrlHandler(alCerrar, TRUE);
	SIZE_T w = 0;
	if (!WriteProcessMemory(gHW, (LPVOID)gDirNombre, nuevo, kNomLen, &w) || w != kNomLen) { say("No se pudo escribir (error %lu).", GetLastError()); return; }
	gEscrito = true;
	say("ESCRITO «%s» en el Usuario %d (solo el nombre, en memoria). Mira la lista de Usuarios en PES (R2/L2 hasta el %d).", texto.c_str(), prueba, prueba);
	say("Cuando acabes pulsa Numpad 8 (o cierra esta ventana) y se RESTAURA el nombre original.");
	for (;;) {
		if (GetAsyncKeyState(VK_NUMPAD8) & 0x8000) { restaurar(); Beep(900, 150); return; }
		if (WaitForSingleObject(gH, 0) == WAIT_OBJECT_0) { say("El juego se cerro."); gEscrito = false; return; }
		uint8_t v[48]; SIZE_T gg = 0;   // por si PES lo repone solo, se informa
		if (ReadProcessMemory(gH, (LPCVOID)gDirNombre, v, kNomLen, &gg) && gg == kNomLen && memcmp(v, nuevo, kNomLen) != 0) { say("PES cambio ese nombre por su cuenta (ya no es el de la prueba)."); gEscrito = false; return; }
		Sleep(100);
	}
}

// ---- SOLO LEE: punteros <nombre existente> <su casilla> <Usuario del mando 1> <Usuario del mando 2> ----
// Localiza la tabla de Usuarios (igual que escribir), busca el registro de cada Usuario nombrado y lista en la memoria de PES
// los sitios que contienen la DIRECCION de ese registro (o de su nombre): son las referencias «este mando usa este Usuario».
static uint64_t localizarTabla(const std::string& nombre, int casilla) {
	std::vector<uint64_t> tablas;
	for (auto& r : regiones()) {
		std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
		if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
		for (size_t i = kNomOff; i + nombre.size() < r.tam; i++) {
			if (memcmp(&b[i], nombre.data(), nombre.size()) != 0) continue;
			const int64_t ini = (int64_t)i - kNomOff - (int64_t)(casilla - 1) * kRegU;
			if (ini < 0 || (size_t)ini + 32 * kRegU > r.tam) continue;
			bool ok = true;
			for (int u = 0; u < 32 && ok; u++) ok = registroUsuarioValido(&b[ini + u * kRegU]);
			if (ok) tablas.push_back(r.base + ini);
		}
	}
	return tablas.size() == 1 ? tablas[0] : 0;
}

static void punteros(int argc, char** argv) {
	if (argc < 6) { say("uso: punteros <nombre existente> <su casilla 1..32> <Usuario mando 1> <Usuario mando 2>"); return; }
	const uint64_t tabla = localizarTabla(argv[2], atoi(argv[3]));
	if (!tabla) { say("No encuentro la tabla de Usuarios con certeza. No hago nada."); return; }
	std::vector<uint8_t> t(32 * kRegU); SIZE_T g = 0;
	if (!ReadProcessMemory(gH, (LPCVOID)tabla, t.data(), t.size(), &g) || g != t.size()) { say("No pude leer la tabla."); return; }
	FILE* f = fopen("usuariopad_punteros.txt", "w");
	fprintf(f, "tabla de Usuarios en %llX\n", (unsigned long long)tabla);
	std::vector<std::pair<std::string, uint64_t>> objetivos;
	for (int a = 4; a <= 5; a++) {
		const std::string n = argv[a]; int hallado = -1;
		for (int u = 0; u < 32; u++) if (strncmp((const char*)&t[u * kRegU + kNomOff], n.c_str(), kNomLen) == 0) { hallado = u; break; }
		if (hallado < 0) { say("No encuentro el Usuario «%s» en la tabla.", n.c_str()); fprintf(f, "NO ENCONTRADO: %s\n", n.c_str()); continue; }
		const uint64_t reg = tabla + (uint64_t)hallado * kRegU;
		say("«%s» = casilla %d (indice %d), registro en %llX", n.c_str(), hallado + 1, hallado, (unsigned long long)reg);
		fprintf(f, "\n== «%s» = casilla %d, indice %d, registro %llX ==\n", n.c_str(), hallado + 1, hallado, (unsigned long long)reg);
		int hits = 0;
		for (auto& r : regiones()) {
			std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
			if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
			for (size_t i = 0; i + 8 <= r.tam; i += 4) {
				uint64_t v; memcpy(&v, &b[i], 8);
				if (v != reg && v != reg + kNomOff) continue;
				const uint64_t ad = r.base + i; if (ad >= tabla && ad < tabla + t.size()) continue;   // la propia tabla no cuenta
				hits++;
				fprintf(f, "%llX%s apunta a %s\n   alrededor:", (unsigned long long)ad, (ad >= gExeBase && ad < gExeBase + gExeTam) ? " [EXE]" : "", v == reg ? "el REGISTRO" : "el NOMBRE");
				for (int k = -16; k < 48 && (int64_t)i + k >= 0 && i + k < r.tam; k++) fprintf(f, " %02X", b[i + k]);
				fprintf(f, "\n");
			}
		}
		fprintf(f, "-> %d referencias a «%s»\n", hits, n.c_str());
		say("«%s»: %d referencias", n.c_str(), hits);
	}
	fclose(f);
	say("listo -> usuariopad_punteros.txt");
}

// ---- SOLO LEE: copias <nombre existente> <su casilla> <Usuario a buscar> ----
// Busca en toda la memoria de PES COPIAS de la configuracion (bytes +0x34..+0x134 del registro) de ese Usuario fuera de la tabla:
// si PES usa una copia por mando, aparece ahi.
static void copias(int argc, char** argv) {
	if (argc < 5) { say("uso: copias <nombre existente> <su casilla 1..32> <Usuario a buscar>"); return; }
	const uint64_t tabla = localizarTabla(argv[2], atoi(argv[3]));
	if (!tabla) { say("No encuentro la tabla de Usuarios con certeza. No hago nada."); return; }
	std::vector<uint8_t> t(32 * kRegU); SIZE_T g = 0;
	if (!ReadProcessMemory(gH, (LPCVOID)tabla, t.data(), t.size(), &g) || g != t.size()) { say("No pude leer la tabla."); return; }
	int idx = -1;
	for (int u = 0; u < 32; u++) if (strncmp((const char*)&t[u * kRegU + kNomOff], argv[4], kNomLen) == 0) { idx = u; break; }
	if (idx < 0) { say("No encuentro el Usuario «%s».", argv[4]); return; }
	const uint8_t* reg = &t[idx * kRegU];
	const size_t sigIni = 0x34, sigLen = 0x100;
	FILE* f = fopen("usuariopad_copias.txt", "w");
	fprintf(f, "tabla %llX; Usuario «%s» = casilla %d, registro %llX; firma = bytes +0x%zX..+0x%zX\n", (unsigned long long)tabla, argv[4], idx + 1, (unsigned long long)(tabla + idx * kRegU), sigIni, sigIni + sigLen);
	int hits = 0;
	for (auto& r : regiones()) {
		std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
		if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
		for (size_t i = sigIni; i + sigLen <= r.tam; i++) {
			if (b[i] != reg[sigIni] || memcmp(&b[i], reg + sigIni, sigLen) != 0) continue;
			const uint64_t ad = r.base + i;
			if (ad >= tabla && ad < tabla + t.size()) { fprintf(f, "%llX  (en la TABLA, casilla %llu)\n", (unsigned long long)ad, (unsigned long long)((ad - tabla) / kRegU + 1)); continue; }
			hits++;
			fprintf(f, "%llX%s  COPIA fuera de la tabla. 0x40 bytes antes:", (unsigned long long)ad, (ad >= gExeBase && ad < gExeBase + gExeTam) ? " [EXE]" : "");
			for (size_t k = 0x40; k >= 1; k--) if (i >= k) fprintf(f, " %02X", b[i - k]);
			fprintf(f, "\n");
		}
	}
	fprintf(f, "-> %d copias fuera de la tabla\n", hits);
	fclose(f);
	say("«%s»: %d copias fuera de la tabla -> usuariopad_copias.txt", argv[4], hits);
}

// ---- SOLO LEE: pares <nombre existente> <su casilla> <A1> <A2> <B1> <B2> ----
// Busca el sitio donde PES guarda «el Usuario de cada mando» (u32). Estado A: mando 1 en el Usuario A1 y mando 2 en A2.
// Estado B: mando 1 en B1 y mando 2 en B2 (los 4 Usuarios con nombre y distintos). Numpad 1 = foto del estado A, Numpad 2 = foto del B,
// Numpad 9 = salir. Un candidato es una pareja (x, x+salto) que vale (A1, A2) en A y (B1, B2) en B. Escribe usuariopad_pares.txt.
struct Par { uint64_t x; uint32_t salto; };
static std::vector<Par> gPares;
static void paresFoto(int estado, uint32_t v1, uint32_t v2) {
	size_t total = 0;
	if (estado == 1) gPares.clear();
	std::vector<Par> nuevos;
	for (auto& r : regiones()) {
		std::vector<uint8_t> b(r.tam); SIZE_T got = 0;
		if (!ReadProcessMemory(gH, (LPCVOID)r.base, b.data(), r.tam, &got) || got != r.tam) continue;
		auto u32 = [&](uint64_t addr, uint32_t& out) -> bool { if (addr < r.base || addr + 4 > r.base + r.tam) return false; memcpy(&out, &b[addr - r.base], 4); return true; };
		if (estado == 1) {
			for (size_t i = 0; i + 4 <= r.tam; i += 4) {
				uint32_t a; memcpy(&a, &b[i], 4); if (a != v1) continue;
				for (uint32_t s = 4; s <= 0x400; s += 4) { uint32_t c; if (i + s + 4 > r.tam) break; memcpy(&c, &b[i + s], 4); if (c == v2) nuevos.push_back({ r.base + i, s }); }
			}
		}
	}
	if (estado == 1) { gPares = std::move(nuevos); say("foto A: %zu parejas candidatas. Ahora pon los Usuarios del estado B y pulsa Numpad 2", gPares.size()); return; }
	// estado B: se filtra con lecturas directas (rapido)
	std::vector<Par> sobreviven;
	for (auto& p : gPares) {
		uint32_t a = 0, c = 0; SIZE_T g1 = 0, g2 = 0;
		if (ReadProcessMemory(gH, (LPCVOID)p.x, &a, 4, &g1) && ReadProcessMemory(gH, (LPCVOID)(p.x + p.salto), &c, 4, &g2) && g1 == 4 && g2 == 4 && a == v1 && c == v2) sobreviven.push_back(p);
	}
	gPares = std::move(sobreviven); (void)total;
	say("foto B: quedan %zu parejas", gPares.size());
	FILE* f = fopen("usuariopad_pares.txt", "w");
	fprintf(f, "parejas que valen (A1,A2) en A y (B1,B2) en B: %zu\n", gPares.size());
	size_t n = 0;
	for (auto& p : gPares) {
		if (n++ >= 300) break;
		fprintf(f, "%llX  +%u%s  contexto:", (unsigned long long)p.x, p.salto, (p.x >= gExeBase && p.x < gExeBase + gExeTam) ? " [EXE]" : "");
		uint8_t b[96] = {}; SIZE_T g = 0; ReadProcessMemory(gH, (LPCVOID)(p.x - 16), b, 96, &g);
		for (int k = 0; k < 96; k++) fprintf(f, " %02X", b[k]);
		fprintf(f, "\n");
	}
	fclose(f);
}

static void pares(int argc, char** argv) {
	if (argc < 8) { say("uso: pares <nombre existente> <su casilla> <A1> <A2> <B1> <B2>"); return; }
	const uint64_t tabla = localizarTabla(argv[2], atoi(argv[3]));
	if (!tabla) { say("No encuentro la tabla de Usuarios con certeza. No hago nada."); return; }
	std::vector<uint8_t> t(32 * kRegU); SIZE_T g = 0;
	if (!ReadProcessMemory(gH, (LPCVOID)tabla, t.data(), t.size(), &g) || g != t.size()) { say("No pude leer la tabla."); return; }
	int idx[4];
	for (int k = 0; k < 4; k++) {
		idx[k] = -1;
		for (int u = 0; u < 32; u++) if (strncmp((const char*)&t[u * kRegU + kNomOff], argv[4 + k], kNomLen) == 0) { idx[k] = u; break; }
		if (idx[k] < 0) { say("No encuentro el Usuario «%s» en la tabla.", argv[4 + k]); return; }
		say("«%s» = indice %d", argv[4 + k], idx[k]);
	}
	say("Estado A: mando 1 = %s, mando 2 = %s. Pulsa Numpad 1 (foto A). Luego cambia a B (%s / %s) y pulsa Numpad 2.", argv[4], argv[5], argv[6], argv[7]);
	bool p1 = false, p2 = false, p9 = false;
	for (;;) {
		const bool d1 = (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) != 0, d2 = (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) != 0, d9 = (GetAsyncKeyState(VK_NUMPAD9) & 0x8000) != 0;
		if (d1 && !p1) { Beep(800, 100); paresFoto(1, idx[0], idx[1]); Beep(1200, 150); }
		if (d2 && !p2) { Beep(800, 100); paresFoto(2, idx[2], idx[3]); Beep(1500, 200); }
		if (d9 && !p9) return;
		p1 = d1; p2 = d2; p9 = d9;
		Sleep(30);
		if (WaitForSingleObject(gH, 0) == WAIT_OBJECT_0) { say("El juego se cerro."); return; }
	}
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
	if (argc >= 2 && std::string(argv[1]) == "escribir") { escribirPrueba(argc, argv, pid); return 0; }
	if (argc >= 2 && std::string(argv[1]) == "punteros") { punteros(argc, argv); return 0; }
	if (argc >= 2 && std::string(argv[1]) == "copias") { copias(argc, argv); return 0; }
	if (argc >= 2 && std::string(argv[1]) == "pares") { pares(argc, argv); return 0; }
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
