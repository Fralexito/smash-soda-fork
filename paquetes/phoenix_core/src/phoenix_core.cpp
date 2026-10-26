// Phoenix Core · fase 0 (esqueleto). DLL propia que se carga en PES 2021 desde Sider (phoenix_core_loader.lua).
// Plan completo: docs/libro-pes (cap. 70 «Phoenix Core al límite»).
//
// Qué hace esta fase:
//   - registro propio (phoenix_core.log) con versión, identidad del exe y estado de cada función;
//   - phoenix_core.json con las funciones encendidas/apagadas (lo edita Link o una persona);
//   - interruptor general: si existe el archivo PHOENIX_CORE_OFF, Core no hace nada;
//   - protector de bucle de crash: arrancando.flag se crea al arrancar y se borra a los 60 s estables;
//     si al arrancar ya existe, el arranque anterior se cayó → MODO SEGURO (todo apagado);
//   - buscador de firmas (AOB) en la imagen del exe y parches con verificación de bytes y deshacer;
//   - marco de funciones («Feature»): cada una con instalar/quitar, protegida con __try/__except y
//     autoapagado a los 3 fallos.
// Nada de esto toca el exe en disco. Sin la DLL (o con PHOENIX_CORE_OFF) PES queda 100 % original.

#ifndef PHOENIX_CORE_EXPORTS
#define PHOENIX_CORE_EXPORTS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>
#include <intrin.h>
#include <tlhelp32.h>
#include "phoenix_api.h"

#define PHX_CORE_VERSION "0.4.6"

namespace {

// ---------------------------------------------------------------- estado global
std::string g_carpeta;                 // con barra final
int         g_codigoInicio = -99;      // -99 = sin iniciar
bool        g_modoSeguro = false;
ULONGLONG   g_tInicio = 0;
bool        g_flagBorrada = false;
CRITICAL_SECTION g_cs;
bool        g_csLista = false;

struct Exe {
    uint8_t* base = nullptr;
    size_t   tamImagen = 0;
    uint32_t fecha = 0, checksum = 0;
    char     ruta[MAX_PATH] = {};
} g_exe;

// ---------------------------------------------------------------- registro
void logf(const char* fmt, ...) {
    if (g_carpeta.empty()) return;
    std::string ruta = g_carpeta + "phoenix_core.log";
    FILE* f = nullptr;
    if (fopen_s(&f, ruta.c_str(), "ab") != 0 || !f) return;
    SYSTEMTIME t; GetLocalTime(&t);
    fprintf(f, "%04d-%02d-%02d %02d:%02d:%02d.%03d  ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond,
            t.wMilliseconds);
    va_list a; va_start(a, fmt); vfprintf(f, fmt, a); va_end(a);
    fputs("\r\n", f);
    fclose(f);
}

bool existe(const std::string& ruta) { return GetFileAttributesA(ruta.c_str()) != INVALID_FILE_ATTRIBUTES; }

bool leerArchivo(const std::string& ruta, std::string& out) {
    FILE* f = nullptr;
    if (fopen_s(&f, ruta.c_str(), "rb") != 0 || !f) return false;
    char b[4096]; size_t n;
    out.clear();
    while ((n = fread(b, 1, sizeof b, f)) > 0) out.append(b, n);
    fclose(f);
    return true;
}

bool escribirArchivo(const std::string& ruta, const std::string& datos) {
    std::string tmp = ruta + ".tmp";
    FILE* f = nullptr;
    if (fopen_s(&f, tmp.c_str(), "wb") != 0 || !f) return false;
    bool ok = fwrite(datos.data(), 1, datos.size(), f) == datos.size();
    fclose(f);
    return ok && MoveFileExA(tmp.c_str(), ruta.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}

// ---------------------------------------------------------------- identidad del exe
void leerExe() {
    HMODULE h = GetModuleHandleA(nullptr);
    g_exe.base = reinterpret_cast<uint8_t*>(h);
    GetModuleFileNameA(h, g_exe.ruta, MAX_PATH);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(g_exe.base);
    auto nt  = reinterpret_cast<IMAGE_NT_HEADERS64*>(g_exe.base + dos->e_lfanew);
    g_exe.tamImagen = nt->OptionalHeader.SizeOfImage;
    g_exe.fecha     = nt->FileHeader.TimeDateStamp;
    g_exe.checksum  = nt->OptionalHeader.CheckSum;
}

// Sección del exe por nombre (".text", ".rdata"…). Devuelve false si no existe.
bool seccion(const char* nombre, uint8_t*& ini, size_t& tam) {
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(g_exe.base);
    auto nt  = reinterpret_cast<IMAGE_NT_HEADERS64*>(g_exe.base + dos->e_lfanew);
    auto s   = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++s) {
        if (strncmp(reinterpret_cast<const char*>(s->Name), nombre, 8) == 0) {
            ini = g_exe.base + s->VirtualAddress;
            tam = s->Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------- firmas (AOB)
// Patrón en texto: "48 8B ?? 05 ..." («??» = cualquier byte).
struct Patron { std::vector<uint8_t> b; std::vector<bool> m; };

bool parsearPatron(const char* txt, Patron& p) {
    p.b.clear(); p.m.clear();
    for (const char* c = txt; *c;) {
        if (*c == ' ') { ++c; continue; }
        if (c[0] == '?') { p.b.push_back(0); p.m.push_back(false); c += (c[1] == '?') ? 2 : 1; continue; }
        unsigned v;
        if (sscanf_s(c, "%2x", &v) != 1) return false;
        p.b.push_back(static_cast<uint8_t>(v)); p.m.push_back(true);
        c += 2;
    }
    return !p.b.empty();
}

// Cuenta coincidencias en [ini, ini+tam) y devuelve la primera en `primera`. Lectura pura.
int buscar(const Patron& p, uint8_t* ini, size_t tam, uint8_t*& primera, int tope = 2) {
    int n = 0; primera = nullptr;
    const size_t L = p.b.size();
    if (tam < L) return 0;
    for (size_t i = 0; i + L <= tam; ++i) {
        size_t k = 0;
        while (k < L && (!p.m[k] || ini[i + k] == p.b[k])) ++k;
        if (k == L) { if (!n) primera = ini + i; if (++n >= tope) break; }
    }
    return n;
}

// Busca el patrón. `sec` = nombre de sección, o "codigo" = todas las secciones ejecutables (en el exe de PES el
// código vive en «.trace», no en «.text»). Solo vale si aparece EXACTAMENTE una vez (si no, la función queda apagada).
uint8_t* firmaUnica(const char* sec, const char* patron, const char* quien) {
    Patron p;
    if (!parsearPatron(patron, p)) { logf("[%s] patrón mal escrito", quien); return nullptr; }
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(g_exe.base);
    auto nt  = reinterpret_cast<IMAGE_NT_HEADERS64*>(g_exe.base + dos->e_lfanew);
    auto s   = IMAGE_FIRST_SECTION(nt);
    const bool codigo = strcmp(sec, "codigo") == 0;
    int n = 0; uint8_t* r = nullptr;
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections && n < 2; ++i, ++s) {
        bool vale = codigo ? (s->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0
                           : strncmp(reinterpret_cast<const char*>(s->Name), sec, 8) == 0;
        if (!vale) continue;
        uint8_t* primera; int m = buscar(p, g_exe.base + s->VirtualAddress, s->Misc.VirtualSize, primera);
        if (m && !r) r = primera;
        n += m;
    }
    if (n != 1) { logf("[%s] firma %s (%d coincidencias en %s)", quien, n ? "repetida" : "no encontrada", n, sec); return nullptr; }
    return r;
}

// Lista las secciones de la imagen en el registro (nombre, RVA, tamaño, permisos).
void listarSecciones() {
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(g_exe.base);
    auto nt  = reinterpret_cast<IMAGE_NT_HEADERS64*>(g_exe.base + dos->e_lfanew);
    auto s   = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++s) {
        char nom[9] = {}; memcpy(nom, s->Name, 8);
        logf("sección %-8s rva 0x%08X tam 0x%08X flags 0x%08X", nom, s->VirtualAddress, s->Misc.VirtualSize, s->Characteristics);
    }
}

// ---------------------------------------------------------------- parches con verificación y deshacer
struct Parche { uint8_t* dir; std::vector<uint8_t> original, nuevo; };

bool escribirCodigo(uint8_t* dir, const uint8_t* datos, size_t n) {
    DWORD viejo;
    if (!VirtualProtect(dir, n, PAGE_EXECUTE_READWRITE, &viejo)) return false;
    memcpy(dir, datos, n);
    DWORD x; VirtualProtect(dir, n, viejo, &x);
    FlushInstructionCache(GetCurrentProcess(), dir, n);
    return memcmp(dir, datos, n) == 0;
}

// Solo escribe si en `dir` están exactamente los bytes `esperado`. Si ya están los nuevos, no hace nada.
bool aplicarParche(Parche& p, const char* quien) {
    const size_t n = p.nuevo.size();
    if (p.original.size() != n) return false;
    if (memcmp(p.dir, p.nuevo.data(), n) == 0) return true;
    if (memcmp(p.dir, p.original.data(), n) != 0) {
        logf("[%s] bytes inesperados en exe+0x%llX: no escribo", quien, (unsigned long long)(p.dir - g_exe.base));
        return false;
    }
    return escribirCodigo(p.dir, p.nuevo.data(), n);
}

bool quitarParche(Parche& p) {
    const size_t n = p.original.size();
    if (memcmp(p.dir, p.original.data(), n) == 0) return true;
    if (memcmp(p.dir, p.nuevo.data(), n) != 0) return false;   // alguien más tocó: no piso
    return escribirCodigo(p.dir, p.original.data(), n);
}

// ---------------------------------------------------------------- marco de funciones
struct Feature {
    const char* id;
    const char* titulo;
    bool (*instalar)(Feature&);   // pone sus parches/hooks; false = no se pudo (queda apagada)
    bool (*quitar)(Feature&);     // los deshace
    bool deseada = false;         // lo que dice phoenix_core.json
    bool activa = false;          // lo que está de verdad
    int  fallos = 0;
    std::string estado = "sin iniciar";
    std::vector<Parche> parches;
};

// SEH en una función aparte (MSVC no deja __try junto a objetos C++ con destructor).
int llamarSeguro(bool (*fn)(Feature&), Feature& f) {
    __try { return fn(f) ? 1 : 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
}

// --- función 1: diagnóstico (solo lectura). Comprueba que el buscador de firmas funciona en este exe.
bool diagInstalar(Feature& f) {
    // cadena del enum de acciones del partido (pista 6 del cap. 60-o): debe existir una vez en .rdata
    // (aparece 2 veces en el exe de Conmegol: vale con encontrarla; lo que importa es que el buscador funciona)
    Patron p; uint8_t* ini; size_t tam; uint8_t* r = nullptr;
    parsearPatron("46 52 45 45 5F 54 52 41 49 4E 49 4E 47 5F 42 52 45 41 4B 00", p);   // "FREE_TRAINING_BREAK\0"
    int n = seccion(".rdata", ini, tam) ? buscar(p, ini, tam, r, 16) : 0;
    if (!n) { f.estado = "firma no encontrada"; return false; }
    char b[96]; sprintf_s(b, "ok: FREE_TRAINING_BREAK x%d, primera en exe+0x%llX", n, (unsigned long long)(r - g_exe.base));
    f.estado = b;
    return true;
}
bool diagQuitar(Feature&) { return true; }

// --- función 2: gol nativo en el Entrenamiento libre (street).
// El detector SÍ ve el gol (k=3), pero el gestor de parada del entrenamiento FUN_1404A56C0 lo degrada: escribe k=7
// en el registro de parada (job+0x49C0 y su copia) con dos «mov edx,7 ; call FUN_141B8B250». Con k=7 y sub=3 el
// selector monta un saque de puerta (tipo 5). Aquí cada «mov edx,7» (5 bytes) salta a una cueva propia:
//     cmp edi,3 ; jne +7 ; mov edx,edi ; jmp vuelta ; mov edx,7 ; jmp vuelta
// (edi = k original, leído en 0x1404A590A y no volátil entre las dos llamadas). Solo el gol conserva k=3; el resto
// de paradas siguen igual que siempre. Firma de los dos sitios (13 bytes entre ellos):
const char* kFirmaDegradar = "BA 07 00 00 00 49 8B CE E8 ?? ?? ?? ?? BA 07 00 00 00 48 8B CB E8";

// Memoria ejecutable a menos de 2 GB de `cerca` (para un jmp rel32 de 5 bytes). Nunca se libera.
uint8_t* reservarCerca(uint8_t* cerca, size_t tam) {
    SYSTEM_INFO si; GetSystemInfo(&si);
    const uintptr_t paso = si.dwAllocationGranularity;
    uintptr_t base = (reinterpret_cast<uintptr_t>(cerca) & ~(paso - 1));
    for (uintptr_t d = paso; d < 0x70000000; d += paso) {
        for (int lado = 0; lado < 2; ++lado) {
            uintptr_t dir = lado ? base + d + g_exe.tamImagen : base - d;
            if (!lado && dir > base) continue;
            void* p = VirtualAlloc(reinterpret_cast<void*>(dir), tam, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (p) return static_cast<uint8_t*>(p);
        }
    }
    return nullptr;
}

bool escribirRel32(uint8_t* dst, uint8_t op, uint8_t* desde, uint8_t* hacia) {
    long long rel = (long long)(hacia - (desde + 5));
    if (rel > INT32_MAX || rel < INT32_MIN) return false;
    int32_t r = static_cast<int32_t>(rel);
    dst[0] = op; memcpy(dst + 1, &r, 4);
    return true;
}

// Tras el gol, FUN_140504D20 recorre 22 «ranuras por jugador» del objeto param_1 (gestos/reacción en la parada) y
// llama vtable[+0x10] de cada una. En el Entrenamiento libre ese objeto no está construido (vtable nula en
// param_1+0xC8) y PES cae en exe+0x5B1FD9. Con el gol nativo encendido, si la ranura 0 no tiene vtable se salta.
// Prólogo 14 bytes sin RIP-relativos (mov [rsp+10],rdx; mov [rsp+8],rcx; push rbx/rbp/rsi/rdi).
const char* kFirmaRanuras = "48 89 54 24 10 48 89 4C 24 08 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC A8 00 00 00 48 8B 05";
bool leerSeguro(const void* dir, void* out, size_t n);
using FnRanuras = uint64_t (*)(uint8_t*, uint64_t, uint64_t, uint64_t);
FnRanuras g_ranurasOrig = nullptr;
volatile LONG g_ranurasSaltadas = 0;
uint64_t ganchoRanuras(uint8_t* obj, uint64_t a2, uint64_t a3, uint64_t a4) {
    uint64_t vt = 0;
    if (!obj || !leerSeguro(obj + 0xC8, &vt, 8) || vt == 0) {
        InterlockedIncrement(&g_ranurasSaltadas);
        return 0;
    }
    return g_ranurasOrig(obj, a2, a3, a4);
}
// Bug real del crash (exe+0x5B1FD9, FUN_1405B1740): FUN_140A62690(lado, p) devuelve 0xFF cuando no hay jugador
// (entrenamiento con pocos jugadores) y el código lo usa como índice de ranura sin comprobarlo → ranura fuera del
// arreglo, vtable nula. Parche en el sitio (12 B tras el call: «movsxd rcx,eax ; mov [r14+rcx*4+34h],9») → cueva:
//     cmp eax,0FFh ; jne +5 ; jmp salida(sitio+0xBE, el mismo camino que «no hace falta») ;
//     movsxd rcx,eax ; mov dword [r14+rcx*4+34h],9 ; jmp sitio+12
const char* kFirmaIndice = "E8 ?? ?? ?? ?? 48 63 C8 41 C7 44 8E 34 09 00 00 00 48 8D 41 05";
// Segundo uso igual (rama del tipo 0xB, 0x1405B21DB): 11 B «mov rdx,[rsp+180h] ; movsxd rcx,eax»; salida = epílogo.
const char* kFirmaIndice2 = "E8 ?? ?? ?? ?? 48 8B 94 24 80 01 00 00 48 63 C8 83 7C 8A 34 09 0F";
bool crearDesvio(Feature& f, uint8_t* dir, size_t n, void* gancho, void** original);


// Cueva genérica «si eax==0xFF (sin jugador) salir»: en `sitio` hay `n` bytes de instrucciones enteras sin
// RIP-relativos justo después del call a FUN_140A62690; se sustituyen por jmp a:
//     cmp eax,0FFh ; jne +5 ; jmp sitio+salida ; <n bytes originales> ; jmp sitio+n
bool cuevaIndiceFF(Feature& f, uint8_t* sitio, size_t n, size_t salida) {
    uint8_t* c = reservarCerca(sitio, 64);
    if (!c) { f.estado = "no hay memoria cerca del exe (índice)"; return false; }
    const uint8_t cmpff[] = { 0x3D, 0xFF, 0x00, 0x00, 0x00,  0x75, 0x05 };
    memcpy(c, cmpff, sizeof cmpff);
    bool ok = escribirRel32(c + 7, 0xE9, c + 7, sitio + salida);
    memcpy(c + 12, sitio, n);
    ok = ok && escribirRel32(c + 12 + n, 0xE9, c + 12 + n, sitio + n);
    if (!ok) { f.estado = "cueva fuera de alcance (índice)"; return false; }
    FlushInstructionCache(GetCurrentProcess(), c, 64);
    Parche p; p.dir = sitio;
    p.original.assign(sitio, sitio + n);
    p.nuevo.resize(n, 0x90);
    if (!escribirRel32(p.nuevo.data(), 0xE9, sitio, c)) { f.estado = "cueva fuera de alcance (índice)"; return false; }
    if (!aplicarParche(p, f.id)) { f.estado = "bytes inesperados (índice)"; return false; }
    f.parches.push_back(p);
    return true;
}

bool golInstalar(Feature& f) {
    uint8_t* ranuras = firmaUnica("codigo", kFirmaRanuras, f.id);
    if (!ranuras) { f.estado = "sin firma: ranuras por jugador (FUN_140504D20)"; return false; }
    uint8_t* indice = firmaUnica("codigo", kFirmaIndice, f.id);
    if (!indice) { f.estado = "sin firma: índice de jugador 0xFF (FUN_1405B1740)"; return false; }
    indice += 5;
    uint8_t* indice2 = firmaUnica("codigo", kFirmaIndice2, f.id);
    if (!indice2) { f.estado = "sin firma: índice 0xFF, segundo uso"; return false; }
    indice2 += 5;
    uint8_t* sitio = firmaUnica("codigo", kFirmaDegradar, f.id);
    if (!sitio) { f.estado = "sin firma: degradación del gol en el entrenamiento"; return false; }
    uint8_t* sitios[2] = { sitio, sitio + 13 };
    uint8_t* cueva = reservarCerca(sitio, 64);
    if (!cueva) { f.estado = "no hay memoria cerca del exe para la cueva"; return false; }
    for (int i = 0; i < 2; ++i) {
        uint8_t* c = cueva + i * 32;
        uint8_t* vuelta = sitios[i] + 5;
        const uint8_t cab[] = { 0x83, 0xFF, 0x03,  0x75, 0x07,  0x89, 0xFA };   // cmp edi,3 ; jne +7 ; mov edx,edi
        memcpy(c, cab, sizeof cab);
        if (!escribirRel32(c + 7, 0xE9, c + 7, vuelta)) { f.estado = "cueva fuera de alcance"; return false; }
        const uint8_t siete[] = { 0xBA, 0x07, 0x00, 0x00, 0x00 };                // mov edx,7 (lo de siempre)
        memcpy(c + 12, siete, 5);
        if (!escribirRel32(c + 17, 0xE9, c + 17, vuelta)) { f.estado = "cueva fuera de alcance"; return false; }
    }
    FlushInstructionCache(GetCurrentProcess(), cueva, 64);
    for (int i = 0; i < 2; ++i) {
        Parche p; p.dir = sitios[i];
        p.original.assign({ 0xBA, 0x07, 0x00, 0x00, 0x00 });
        p.nuevo.resize(5);
        if (!escribirRel32(p.nuevo.data(), 0xE9, sitios[i], cueva + i * 32)) { f.estado = "cueva fuera de alcance"; return false; }
        if (!aplicarParche(p, f.id)) { f.estado = "bytes inesperados en el sitio"; return false; }
        f.parches.push_back(p);
    }
    if (!crearDesvio(f, ranuras, 14, (void*)ganchoRanuras, (void**)&g_ranurasOrig)) {
        f.estado = "no se pudo desviar FUN_140504D20"; return false;
    }
    if (!cuevaIndiceFF(f, indice, 12, 0xBE)) return false;
    if (!cuevaIndiceFF(f, indice2, 11, 0xBE)) return false;
    char b[160];
    sprintf_s(b, "ok: el entrenamiento ya no degrada el gol (sitios exe+0x%llX y +0x%llX)",
              (unsigned long long)(sitios[0] - g_exe.base), (unsigned long long)(sitios[1] - g_exe.base));
    f.estado = b;
    return true;
}
bool golQuitar(Feature&) { return true; }   // los parches los quita el marco (la cueva se queda, sin uso)

// ---------------------------------------------------------------- desvíos propios (sin MinHook)
// Solo para funciones cuyo prólogo conocemos (leído con REA/Ghidra): se copian `n` bytes de instrucciones enteras,
// sin RIP-relativos ni saltos, a un trampolín que salta de vuelta a dir+n. En dir se escribe
// «jmp [rip+0]; dq gancho» (14 bytes) + NOP hasta n. El trampolín nunca se libera (un hilo podría estar dentro).
bool crearDesvio(Feature& f, uint8_t* dir, size_t n, void* gancho, void** original) {
    if (n < 14 || n > 32) return false;
    auto t = static_cast<uint8_t*>(VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!t) return false;
    memcpy(t, dir, n);
    const uint8_t jmp[6] = { 0xFF, 0x25, 0, 0, 0, 0 };
    uint64_t vuelta = reinterpret_cast<uint64_t>(dir + n);
    memcpy(t + n, jmp, 6); memcpy(t + n + 6, &vuelta, 8);
    FlushInstructionCache(GetCurrentProcess(), t, 64);
    *original = t;
    Parche p; p.dir = dir; p.original.assign(dir, dir + n);
    p.nuevo.assign(jmp, jmp + 6);
    uint64_t g = reinterpret_cast<uint64_t>(gancho);
    p.nuevo.insert(p.nuevo.end(), reinterpret_cast<uint8_t*>(&g), reinterpret_cast<uint8_t*>(&g) + 8);
    p.nuevo.resize(n, 0x90);
    if (!aplicarParche(p, f.id)) return false;
    f.parches.push_back(p);
    return true;
}

// Lectura protegida (la memoria del juego puede no estar donde creemos).
bool leerSeguro(const void* dir, void* out, size_t n) {
    __try { memcpy(out, dir, n); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
uint64_t leerU32(const uint8_t* base, size_t off) { uint32_t v = 0xFFFFFFFF; leerSeguro(base + off, &v, 4); return v; }
uint64_t leerU8(const uint8_t* base, size_t off) { uint8_t v = 0xFF; leerSeguro(base + off, &v, 1); return v; }

// --- función 3: sonda del partido (solo mira). Anota cada cambio de estado de los actores (SetState) y cada vez
// que corre el procesador nativo del gol, con quién lo llamó. Sirve para saber qué decide «gol» en un amistoso y
// qué cambia en el Entrenamiento libre (zona 52). Escribe en sonda_partido.txt.
enum { EV_ESTADO = 1, EV_GOL = 2, EV_PARADO = 3, EV_EVENTO = 4 };
struct Ev { volatile LONG listo; uint32_t tipo, hilo; uint64_t t, ret, a, b, c, d, x[4]; };
const LONG kEvs = 8192;
Ev g_ev[kEvs];
volatile LONG g_escr = 0;
LONG g_leido = 0, g_perdidos = 0;
size_t g_bytesSonda = 0;
const size_t kTopeSonda = 20u << 20;     // 20 MB y deja de escribir

Ev& nuevoEv(uint32_t tipo, void* ret) {
    LONG i = InterlockedIncrement(&g_escr) - 1;
    Ev& e = g_ev[i & (kEvs - 1)];
    e.listo = 0;
    e.tipo = tipo; e.hilo = GetCurrentThreadId(); e.t = GetTickCount64() - g_tInicio;
    e.ret = reinterpret_cast<uint64_t>(ret);
    e.a = e.b = e.c = e.d = 0; e.x[0] = e.x[1] = e.x[2] = e.x[3] = 0;
    e.listo = -1 - i;   // aún escribiéndose
    return e;
}
void cerrarEv(Ev& e) { MemoryBarrier(); e.listo = -e.listo; }   // -1-i → i+1 = listo

using FnSetState = uint64_t (*)(uint8_t* actor, uint32_t estado, uint32_t modo);
FnSetState g_setStateOrig = nullptr;
uint64_t ganchoSetState(uint8_t* actor, uint32_t estado, uint32_t modo) {
    Ev& e = nuevoEv(EV_ESTADO, _ReturnAddress());
    e.a = reinterpret_cast<uint64_t>(actor); e.b = estado & 0xFF; e.c = modo & 0xFF;
    e.d = leerU8(actor, 0xAC8);           // estado anterior
    cerrarEv(e);
    return g_setStateOrig(actor, estado, modo);
}

using FnGol = uint64_t (*)(uint8_t* a, uint8_t* b, uint8_t* job, uint64_t p4);
FnGol g_golOrig = nullptr;
uint64_t ganchoGol(uint8_t* a, uint8_t* b, uint8_t* job, uint64_t p4) {
    uint64_t r = g_golOrig(a, b, job, p4);
    Ev& e = nuevoEv(EV_GOL, _ReturnAddress());
    e.a = reinterpret_cast<uint64_t>(a); e.b = reinterpret_cast<uint64_t>(b);
    e.c = reinterpret_cast<uint64_t>(job); e.d = p4;
    e.x[0] = leerU32(a, 0x294); e.x[1] = leerU32(a, 0x298);       // goles por equipo (según el Mapa Fox)
    e.x[2] = leerU32(job, 0x874); e.x[3] = leerU8(job, 0x8F2);    // escena y fase del trabajo
    cerrarEv(e);
    return r;
}

// FUN_140451FD0(job, tipo): pone job+0x874 = tipo de evento y vacía la cola. En el entrenamiento la llama
// FUN_140449B00 con 0x0E (TrainingWork activo); queremos ver qué tipo pone un gol de amistoso.
using FnEvento = uint64_t (*)(uint8_t* job, uint32_t tipo);
FnEvento g_eventoOrig = nullptr;
// Objeto del partido [[0x1436F3DC8]+0x270] (solo exe de Conmegol). +0x1775 parado, +0x1574 «k» = razón de pelota
// muerta que usa el selector FUN_1404526A0 (k==3 → gol), +0x1554 subtipo, +0x15A5 bandera del caso k==5.
bool g_exeConocido = true;   // las RVA fijas solo valen si todas las firmas caen donde en Conmegol
uint8_t* objPartido() {
    if (!g_exeConocido) return nullptr;
    uint8_t* raiz = nullptr; uint8_t* obj = nullptr;
    if (!leerSeguro(g_exe.base + 0x36F3DC8, &raiz, 8) || !raiz) return nullptr;
    if (!leerSeguro(raiz + 0x270, &obj, 8)) return nullptr;
    return obj;
}

uint64_t ganchoEvento(uint8_t* job, uint32_t tipo) {
    void* ret = _ReturnAddress();
    uint8_t* obj = objPartido();
    uint64_t k = obj ? leerU32(obj, 0x1574) : 0xFFFFFFFF;
    uint64_t sub = obj ? leerU32(obj, 0x1554) : 0xFFFFFFFF;
    uint64_t flag = obj ? leerU8(obj, 0x15A5) : 0xFF;
    uint64_t previo = leerU32(job, 0x874);
    Ev& e = nuevoEv(EV_EVENTO, ret);
    e.a = reinterpret_cast<uint64_t>(job); e.b = tipo; e.d = previo; e.c = leerU8(job, 0x8F2);
    e.x[0] = k; e.x[1] = sub; e.x[2] = flag;
    cerrarEv(e);
    return g_eventoOrig(job, tipo);
}

struct Gancho { const char* nombre; const char* firma; uint32_t rvaEsperada; size_t n; void* gancho; void** original; };
Gancho g_ganchos[] = {
    // Mapa Fox PARTE1: SetState(actor, estado, modo) · prólogo 15 bytes sin RIP-relativos (leído 26-oct)
    { "SetState", "48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 8D", 0x6EEBB0, 15,
      (void*)ganchoSetState, (void**)&g_setStateOrig },
    // procesador nativo del gol (patrón «gol marcado» de Sider) · prólogo 14 bytes exactos
    { "ProcesarGol", "48 8B C4 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 D8", 0x478190, 14,
      (void*)ganchoGol, (void**)&g_golOrig },
    // pone el tipo de evento del trabajo (job+0x874) · prólogo 19 bytes sin RIP-relativos
    { "PonerEvento", "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 89 91 74 08 00 00 48 8D B9", 0x451FD0, 19,
      (void*)ganchoEvento, (void**)&g_eventoOrig },
};

bool sondaInstalar(Feature& f) {
    g_exeConocido = true;
    uint8_t* dirs[_countof(g_ganchos)];
    for (size_t i = 0; i < _countof(g_ganchos); ++i) {      // primero se buscan todas: o todas o ninguna
        auto& g = g_ganchos[i];
        dirs[i] = firmaUnica("codigo", g.firma, f.id);
        if (!dirs[i]) { f.estado = std::string("sin firma: ") + g.nombre; return false; }
        uint32_t rva = static_cast<uint32_t>(dirs[i] - g_exe.base);
        logf("[%s] %s en exe+0x%X%s", f.id, g.nombre, rva, rva == g.rvaEsperada ? "" : " (exe distinto del de Conmegol)");
        if (rva != g.rvaEsperada) g_exeConocido = false;
    }
    for (size_t i = 0; i < _countof(g_ganchos); ++i) {
        if (!crearDesvio(f, dirs[i], g_ganchos[i].n, g_ganchos[i].gancho, g_ganchos[i].original)) {
            f.estado = std::string("no se pudo desviar ") + g_ganchos[i].nombre; return false;
        }
    }
    f.estado = "ok: anotando en sonda_partido.txt";
    return true;
}
bool sondaQuitar(Feature&) { return true; }   // los parches los quita el marco

// Indicador general «partido detenido» ([[0x1436F3DC8]+0x270]+0x1775), solo en el exe de Conmegol (RVA conocida).
uint64_t g_paradoPrevio = 0x100;
void vigilarParado() {
    uint8_t* mgr = objPartido();
    if (!mgr) return;
    uint64_t v = leerU8(mgr, 0x1775);
    if (v == g_paradoPrevio) return;
    g_paradoPrevio = v;
    Ev& e = nuevoEv(EV_PARADO, nullptr);
    e.a = reinterpret_cast<uint64_t>(mgr); e.b = v;
    cerrarEv(e);
}

void volcarSonda() {
    if (g_bytesSonda >= kTopeSonda) return;
    std::string s;
    char l[320];
    for (int k = 0; k < 512 && g_leido < g_escr; ++k) {
        if (g_escr - g_leido > kEvs) { g_perdidos += g_escr - g_leido - kEvs; g_leido = g_escr - kEvs; }
        Ev& e = g_ev[g_leido & (kEvs - 1)];
        if (e.listo != g_leido + 1) break;           // aún escribiéndose
        Ev c = e;                                    // copia antes de que se recicle
        if (c.listo != g_leido + 1) break;
        ++g_leido;
        long long ret = c.ret ? (long long)(c.ret - reinterpret_cast<uint64_t>(g_exe.base)) : 0;
        if (c.tipo == EV_ESTADO)
            sprintf_s(l, "%8llu h%-5u ESTADO actor=%llX 0x%02llX->0x%02llX modo=0x%02llX desde exe+0x%llX\r\n",
                      c.t, c.hilo, c.a, c.d, c.b, c.c, ret);
        else if (c.tipo == EV_GOL)
            sprintf_s(l, "%8llu h%-5u GOL a=%llX b=%llX job=%llX p4=%llX a+294/298=%llu/%llu escena=0x%llX fase=%llu desde exe+0x%llX\r\n",
                      c.t, c.hilo, c.a, c.b, c.c, c.d, c.x[0], c.x[1], c.x[2], c.x[3], ret);
        else if (c.tipo == EV_EVENTO)
            sprintf_s(l, "%8llu h%-5u EVENTO job=%llX tipo 0x%llX->0x%llX fase=%llu k=%lld sub=%lld flag=%lld%s desde exe+0x%llX\r\n",
                      c.t, c.hilo, c.a, c.d, c.b, c.c, (long long)(int32_t)c.x[0], (long long)(int32_t)c.x[1],
                      (long long)(int8_t)c.x[2], "", ret);
        else
            sprintf_s(l, "%8llu h%-5u PARADO mgr=%llX +0x1775=%llu\r\n", c.t, c.hilo, c.a, c.b);
        s += l;
    }
    static LONG saltadasVistas = 0;
    if (g_ranurasSaltadas != saltadasVistas) {
        saltadasVistas = g_ranurasSaltadas;
        sprintf_s(l, "         [GOLFREE] ranuras por jugador sin construir: saltadas %ld veces\r\n", saltadasVistas); s += l;
    }
    if (g_perdidos) { sprintf_s(l, "         (perdidos %ld eventos: demasiados de golpe)\r\n", g_perdidos); s += l; g_perdidos = 0; }
    if (s.empty()) return;
    FILE* f = nullptr;
    if (fopen_s(&f, (g_carpeta + "sonda_partido.txt").c_str(), "ab") != 0 || !f) return;
    fwrite(s.data(), 1, s.size(), f);
    fclose(f);
    g_bytesSonda += s.size();
    if (g_bytesSonda >= kTopeSonda) logf("[sonda_partido] llegó a 20 MB: deja de escribir");
}

// --- función 4: vigilante de bucles (solo mira). Hilo propio que cada 250 ms detiene un instante cada hilo de PES,
// lee su RIP y lo suelta. Si un hilo pasa ≥6 de 8 muestras en la misma zona del exe (±0x200) y gasta CPU sin parar,
// anota en phoenix_core.log «[VIGILANTE] bucle» con RIP, RSP y las direcciones del exe que hay en su pila.
// Sirve para encontrar cuelgues (100 % CPU sin cerrarse) sin volcados de memoria.
struct MuestraHilo { DWORD tid; uint64_t rip[8]; int n; ULONGLONG cpu0; uint64_t avisado; };
volatile LONG g_vigilar = 0;
HANDLE g_hiloVigilante = nullptr;

ULONGLONG cpuHilo(HANDLE h) {
    FILETIME c, e, k, u;
    if (!GetThreadTimes(h, &c, &e, &k, &u)) return 0;
    return ((ULONGLONG)k.dwHighDateTime << 32 | k.dwLowDateTime) + ((ULONGLONG)u.dwHighDateTime << 32 | u.dwLowDateTime);
}

DWORD WINAPI vigilante(LPVOID) {
    std::vector<MuestraHilo> hilos;
    const DWORD yo = GetCurrentThreadId(), pid = GetCurrentProcessId();
    const uint64_t ini = reinterpret_cast<uint64_t>(g_exe.base), fin = ini + g_exe.tamImagen;
    while (g_vigilar) {
        Sleep(250);
        std::vector<DWORD> tids;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE) continue;
        THREADENTRY32 te; te.dwSize = sizeof te;
        for (BOOL ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te))
            if (te.th32OwnerProcessID == pid && te.th32ThreadID != yo) tids.push_back(te.th32ThreadID);
        CloseHandle(snap);
        for (DWORD tid : tids) {
            HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, tid);
            if (!h) continue;
            CONTEXT ctx = {}; ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
            uint64_t pila[64] = {};
            bool leido = false;
            if (SuspendThread(h) != (DWORD)-1) {      // entre Suspend y Resume: nada que reserve memoria
                leido = GetThreadContext(h, &ctx) != 0;
                if (leido) {   // ReadProcessMemory no lanza excepciones (un SEH aquí podría esperar un lock del hilo detenido)
                    SIZE_T nl = 0;
                    ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(ctx.Rsp), pila, sizeof pila, &nl);
                }
                ResumeThread(h);
            }
            ULONGLONG cpu = cpuHilo(h);
            CloseHandle(h);
            if (!leido) continue;
            MuestraHilo* m = nullptr;
            for (auto& x : hilos) if (x.tid == tid) { m = &x; break; }
            if (!m) { hilos.push_back({ tid, {}, 0, cpu, 0 }); m = &hilos.back(); }
            m->rip[m->n % 8] = ctx.Rip; m->n++;
            if (m->n % 8 != 0) continue;
            // ventana completa (2 s): ¿misma zona y CPU casi al 100 %?
            ULONGLONG gastado = cpu - m->cpu0;  m->cpu0 = cpu;      // en unidades de 100 ns
            uint64_t ref = m->rip[7];
            int iguales = 0;
            for (uint64_t r : m->rip) if (r + 0x200 >= ref && r <= ref + 0x200) ++iguales;
            if (iguales < 6 || gastado < 14000000 || ref < ini || ref >= fin) continue;   // ≥1,4 s de CPU en 2 s
            if ((m->avisado & ~0xFFFull) == (ref & ~0xFFFull)) continue;                   // ya avisado esta zona
            m->avisado = ref;
            char ret[400] = {}; size_t o = 0;
            for (uint64_t v : pila)
                if (v >= ini && v < fin && o + 20 < sizeof ret) o += sprintf_s(ret + o, sizeof ret - o, " 0x%llX", v - ini);
            logf("[VIGILANTE] bucle hilo %lu en exe+0x%llX (RSP %llX, CPU %.1f s de 2 s, %d/8 muestras); exe en la pila:%s",
                 tid, ref - ini, ctx.Rsp, gastado / 1e7, iguales, ret);
        }
    }
    return 0;
}

bool vigilanteInstalar(Feature& f) {
    g_vigilar = 1;
    g_hiloVigilante = CreateThread(nullptr, 0, vigilante, nullptr, 0, nullptr);
    if (!g_hiloVigilante) { g_vigilar = 0; f.estado = "no se pudo crear el hilo"; return false; }
    f.estado = "ok: vigilando cuelgues (resultado en phoenix_core.log)";
    return true;
}
bool vigilanteQuitar(Feature&) {
    g_vigilar = 0;
    if (g_hiloVigilante) { WaitForSingleObject(g_hiloVigilante, 2000); CloseHandle(g_hiloVigilante); g_hiloVigilante = nullptr; }
    return true;
}

std::vector<Feature> g_funciones = {   // orden = orden de arranque (gol_nativo_free necesita la sonda)
    { "diagnostico",     "Diagnóstico de firmas (solo lectura)",       diagInstalar,  diagQuitar  },
    { "sonda_partido",   "Sonda: estados y gol nativo (solo mira)",    sondaInstalar, sondaQuitar },
    { "gol_nativo_free", "Gol nativo en Entrenamiento libre (street)", golInstalar,   golQuitar   },
    { "vigilante_bucles", "Vigilante de cuelgues (solo mira)",         vigilanteInstalar, vigilanteQuitar },
};
bool sondaActiva() {
    for (auto& f : g_funciones) if (strcmp(f.id, "sonda_partido") == 0) return f.activa;
    return false;
}

Feature* buscarFuncion(const char* id) {
    for (auto& f : g_funciones) if (strcmp(f.id, id) == 0) return &f;
    return nullptr;
}

bool encender(Feature& f) {
    if (f.activa) return true;
    if (g_modoSeguro) { f.estado = "apagada: modo seguro"; return false; }
    if (f.fallos >= 3) { f.estado = "autoapagada tras 3 fallos"; return false; }
    int r = llamarSeguro(f.instalar, f);
    if (r == 1) { f.activa = true; logf("[%s] activa · %s", f.id, f.estado.c_str()); return true; }
    if (r < 0) { ++f.fallos; f.estado = "excepción al instalar"; }
    for (auto& p : f.parches) quitarParche(p);
    f.parches.clear();
    logf("[%s] no activada · %s", f.id, f.estado.c_str());
    return false;
}

bool apagar(Feature& f) {
    if (!f.activa) return true;
    int r = llamarSeguro(f.quitar, f);
    bool ok = r == 1;
    for (auto& p : f.parches) ok = quitarParche(p) && ok;
    if (ok) f.parches.clear();
    f.activa = !ok;
    f.estado = ok ? "apagada" : "no se pudo apagar del todo";
    logf("[%s] %s", f.id, f.estado.c_str());
    return ok;
}

// ---------------------------------------------------------------- phoenix_core.json (mínimo)
// Formato: {"funciones": {"diagnostico": true, "gol_nativo_free": false}}
// Lector deliberadamente simple: busca "id" y mira si después viene true o false.
void leerConfig() {
    std::string txt;
    std::string ruta = g_carpeta + "phoenix_core.json";
    if (!leerArchivo(ruta, txt)) { logf("sin phoenix_core.json: todas las funciones apagadas salvo diagnostico"); }
    for (auto& f : g_funciones) {
        f.deseada = strcmp(f.id, "diagnostico") == 0;   // por defecto solo el diagnóstico
        std::string clave = std::string("\"") + f.id + "\"";
        size_t i = txt.find(clave);
        if (i == std::string::npos) continue;
        i = txt.find(':', i + clave.size());
        if (i == std::string::npos) continue;
        size_t j = txt.find_first_not_of(" \t\r\n", i + 1);
        if (j != std::string::npos) f.deseada = txt.compare(j, 4, "true") == 0;
    }
}

std::string escaparJson(const std::string& s) {
    std::string r;
    for (char c : s) { if (c == '"' || c == '\\') r += '\\'; r += c; }
    return r;
}

std::string estadoJson() {
    char cab[1024];
    std::string ruta = escaparJson(g_exe.ruta);
    sprintf_s(cab, "{\"version\":\"%s\",\"api\":%d,\"modo\":\"%s\",\"exe\":{\"ruta\":\"%s\",\"fecha\":%u,\"checksum\":%u,\"tam_imagen\":%llu},\"funciones\":[",
              PHX_CORE_VERSION, PHX_CORE_API_VERSION,
              g_codigoInicio == 0 ? "apagado" : g_modoSeguro ? "seguro" : g_codigoInicio == 1 ? "normal" : "error",
              ruta.c_str(), g_exe.fecha, g_exe.checksum, (unsigned long long)g_exe.tamImagen);
    std::string s = cab;
    bool primera = true;
    for (auto& f : g_funciones) {
        if (!primera) s += ",";
        primera = false;
        s += "{\"id\":\"" + std::string(f.id) + "\",\"titulo\":\"" + escaparJson(f.titulo) + "\",\"deseada\":" +
             (f.deseada ? "true" : "false") + ",\"activa\":" + (f.activa ? "true" : "false") + ",\"estado\":\"" +
             escaparJson(f.estado) + "\"}";
    }
    return s + "]}";
}

void guardarEstado() { escribirArchivo(g_carpeta + "phoenix_core_estado.json", estadoJson()); }

} // namespace

// ================================================================ API exportada
PHX_API const char* phx_core_version(void) { return PHX_CORE_VERSION; }
PHX_API int phx_core_api_version(void) { return PHX_CORE_API_VERSION; }

PHX_API int phx_core_iniciar(const char* carpeta) {
    if (g_codigoInicio != -99) return g_codigoInicio;
    if (!g_csLista) { InitializeCriticalSection(&g_cs); g_csLista = true; }
    EnterCriticalSection(&g_cs);
    g_carpeta = carpeta ? carpeta : "";
    if (!g_carpeta.empty() && g_carpeta.back() != '\\' && g_carpeta.back() != '/') g_carpeta += '\\';
    leerExe();
    logf("==== Phoenix Core %s (api %d) · exe %s · fecha 0x%08X · checksum 0x%08X · imagen %llu bytes",
         PHX_CORE_VERSION, PHX_CORE_API_VERSION, g_exe.ruta, g_exe.fecha, g_exe.checksum,
         (unsigned long long)g_exe.tamImagen);

    if (existe(g_carpeta + "PHOENIX_CORE_OFF")) {
        logf("PHOENIX_CORE_OFF presente: Core no hace nada");
        g_codigoInicio = 0;
        guardarEstado();
        LeaveCriticalSection(&g_cs);
        return g_codigoInicio;
    }

    std::string flag = g_carpeta + "arrancando.flag";
    if (existe(flag)) {
        g_modoSeguro = true;
        logf("arrancando.flag ya existía: el arranque anterior no llegó a 60 s estables → MODO SEGURO");
    }
    escribirArchivo(flag, "Phoenix Core arrancando\r\n");
    g_tInicio = GetTickCount64();

    listarSecciones();
    leerConfig();
    for (auto& f : g_funciones) {
        if (f.deseada) encender(f);
        else f.estado = g_modoSeguro ? "apagada: modo seguro" : "apagada";
    }
    g_codigoInicio = g_modoSeguro ? 2 : 1;
    guardarEstado();
    logf("arranque terminado: modo %s", g_modoSeguro ? "SEGURO" : "normal");
    LeaveCriticalSection(&g_cs);
    return g_codigoInicio;
}

PHX_API void phx_core_tick(void) {
    if (g_codigoInicio <= 0) return;
    if (Feature* s = buscarFuncion("sonda_partido"); s && s->activa) {
        vigilarParado();
        volcarSonda();
    }
    if (g_flagBorrada || GetTickCount64() - g_tInicio < 60000) return;
    g_flagBorrada = true;
    DeleteFileA((g_carpeta + "arrancando.flag").c_str());
    logf("60 s estables: arrancando.flag borrada%s", g_modoSeguro ? " (el próximo arranque será normal)" : "");
}

PHX_API int phx_core_funcion(const char* id, int activar) {
    if (!id || g_codigoInicio <= 0) return -1;
    EnterCriticalSection(&g_cs);
    int r = -1;
    if (Feature* f = buscarFuncion(id)) {
        f->deseada = activar != 0;
        bool ok = activar ? encender(*f) : apagar(*f);
        r = ok ? (f->activa ? 1 : 0) : -1;
        guardarEstado();
    }
    LeaveCriticalSection(&g_cs);
    return r;
}

PHX_API int phx_core_estado(char* buf, int n) {
    std::string s = estadoJson();
    int need = static_cast<int>(s.size());
    if (!buf || n <= need) return need;
    memcpy(buf, s.c_str(), s.size() + 1);
    return need;
}

BOOL APIENTRY DllMain(HMODULE, DWORD motivo, LPVOID) {
    // Nada pesado aquí (bloqueo del cargador de Windows): todo empieza en phx_core_iniciar.
    if (motivo == DLL_PROCESS_DETACH && g_codigoInicio > 0) {
        for (auto& f : g_funciones) for (auto& p : f.parches) quitarParche(p);
    }
    return TRUE;
}
