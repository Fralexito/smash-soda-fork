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
#include <algorithm>
#include <intrin.h>
#include <tlhelp32.h>
#include "phoenix_api.h"

#define PHX_CORE_VERSION "0.4.89"

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
    // 0.4.87: búsqueda rápida: memchr (SIMD) por el primer byte NO comodín y verificación solo en los candidatos (antes cada firma tardaba 2-5 s)
    size_t f = 0; while (f < L && !p.m[f]) ++f;
    if (f == L) return 0;
    const uint8_t primero = p.b[f]; const size_t lim = tam - L + 1;
    for (size_t i = 0; i < lim; ) {
        const uint8_t* q = static_cast<const uint8_t*>(memchr(ini + i + f, primero, lim - i));
        if (!q) break;
        size_t pos = static_cast<size_t>(q - (ini + f));
        size_t k = 0; while (k < L && (!p.m[k] || ini[pos + k] == p.b[k])) ++k;
        if (k == L) { if (!n) primera = ini + pos; if (++n >= tope) break; }
        i = pos + 1;
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

// --- función 3: tras el gol en el Entrenamiento libre, saque de CENTRO (no de puerta).
// El reinicio lo decide el registro de parada: sub=1 = saque de centro (amistoso), sub=3 = saque de puerta.
// En FUN_1404A56C0 (0x1404A5B78) una rama que solo corre con k==3 (edi==3, gol) recoloca el registro en sub=3 con
// la posición del arquero: «cmp dword [r14+4],1 ; jne ; cmp edi,3 ; jne +64h ; … FUN_14102CAD0(rec,3)».
// Parche de 1 byte: ese último jne (75) pasa a jmp (EB), la rama queda muerta y el registro conserva sub=1.
const char* kFirmaSaqueCentro = "41 83 7E 04 01 75 69 83 FF 03 75 64 4C 8B 05 ?? ?? ?? ?? 4D 8D 4E 08 8B D7 48 8D 4D E7";
bool saqueInstalar(Feature& f) {
    uint8_t* sitio = firmaUnica("codigo", kFirmaSaqueCentro, f.id);
    if (!sitio) { f.estado = "sin firma: rama del saque de puerta tras el gol (FUN_1404A56C0)"; return false; }
    Parche p; p.dir = sitio + 10;
    p.original.assign({ 0x75 });
    p.nuevo.assign({ 0xEB });
    if (!aplicarParche(p, f.id)) { f.estado = "bytes inesperados (saque de centro)"; return false; }
    f.parches.push_back(p);
    char b[120];
    sprintf_s(b, "ok: tras el gol, saque de centro (exe+0x%llX 75->EB)", (unsigned long long)(p.dir - g_exe.base));
    f.estado = b;
    return true;
}
bool saqueQuitar(Feature&) { return true; }

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
enum { EV_ESTADO = 1, EV_GOL = 2, EV_PARADO = 3, EV_EVENTO = 4, EV_COLA = 5, EV_PASO = 6 };
struct Ev { volatile LONG listo; uint32_t tipo, hilo; uint64_t t, ret, a, b, c, d, x[4]; uint32_t q[32]; };   // q: cola de pasos (solo EV_COLA)
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
    if (tipo == EV_COLA) memset(e.q, 0, sizeof e.q);
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
volatile ULONGLONG g_tUltimoGol = 0;   // para el vigilante: solo perfila después de un gol
uint8_t* volatile g_jobConocido = nullptr;   // último «job» visto (GOL/EVENTO): lo lee sonda_pasos v2 desde su hilo
uint64_t ganchoGol(uint8_t* a, uint8_t* b, uint8_t* job, uint64_t p4) {
    g_tUltimoGol = GetTickCount64();
    g_jobConocido = job;
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
// Objeto del partido [[global]+0x270] (global = 0x1436F3DC8 en Conmegol). +0x1775 parado, +0x1574 «k» = razón de
// pelota muerta que usa el selector FUN_1404526A0 (k==3 → gol), +0x1554 subtipo, +0x15A5 bandera del caso k==5.
// El global se saca por firma de una lectura RIP-relativa (0x14041B277: «mov rcx,[rip+rel32] ; mov rax,[rcx+270h]»),
// así sirve también en un exe distinto del de Conmegol.
const char* kFirmaGlobalPartido = "43 08 3A 00 00 00 48 8B 0D ?? ?? ?? ?? 48 8B 81 70 02 00 00 8B 90 58";
uint8_t* g_globalPartido = nullptr;
bool resolverGlobalPartido(const char* quien) {
    uint8_t* f = firmaUnica("codigo", kFirmaGlobalPartido, quien);
    if (!f) return false;
    int32_t rel; memcpy(&rel, f + 9, 4);
    g_globalPartido = f + 13 + rel;   // fin de la instrucción (f+6, 7 bytes) + rel32
    logf("[%s] global del partido en exe+0x%llX", quien, (unsigned long long)(g_globalPartido - g_exe.base));
    return true;
}
uint8_t* objPartido() {
    if (!g_globalPartido) return nullptr;
    uint8_t* raiz = nullptr; uint8_t* obj = nullptr;
    if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz) return nullptr;
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
    g_jobConocido = job;
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

// --- función: celebración corta en el Entrenamiento libre (plan B del congelón; APAGADA por defecto).
// FUN_14044F8A0 (montador del gol) elige la secuencia tras PonerEvento(job,0xA): LARGA (celebración + repetición:
// 0x03,0x11,0x08,0x23,…) o CORTA (0x43,0x07,0x44) según FUN_140A3F580([root+0x2C0]) = byte [[root+0x2C0]+0x84] != 0.
// El congelón del entreno cae siempre en la LARGA. Gancho: si hay TrainingWork ([root+0x400] y su primer byte != 0,
// mismo criterio que FUN_14044D110), pone ese byte a 1 solo durante la llamada y lo restaura. El amistoso no cambia.
const char* kFirmaMontadorGol = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B D9 48 8B B8 70 02 00 00 48 8B B0 C0 02 00 00 48";
using FnMontador = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t);
FnMontador g_montadorOrig = nullptr;
volatile LONG g_celebracionesCortas = 0;
bool g_cortaActiva = false;      // celebracion_corta_free encendida
bool g_sin1DActiva = false;      // sin_1D_free encendida
volatile LONG g_sin1DCont = 0;
bool g_directoActiva = false;    // gol_directo_free encendida
bool g_directoCelebracion = true; // opción json «gol_directo_celebracion» (por defecto true)
int g_celebModo = 0;               // «celebracion_modo»: 0 completa (por defecto), 1 timer (+0x27D0 al final a los N s), 2 salto de indice (inestable)
uint32_t g_timerFinal = 1180;      // «celebracion_timer_final»: valor de job+0x27D0 que se escribe en modo timer
double g_celebSeg = 8.0;          // opción json «celebracion_segundos» (0 = sin celebración; por defecto 8)
volatile LONG g_directoCont = 0;
bool g_sondaOn = false;          // la sonda vuelca la cola de pasos tras montar el gol

// ¿Hay TrainingWork? Mismo criterio que FUN_14044D110: [root+0x400] != 0 y su primer byte != 0.
bool hayTrainingWork(uint8_t** raizOut = nullptr) {
    uint8_t* raiz = nullptr; uint8_t* tw = nullptr; uint8_t twb = 0;
    bool si = g_globalPartido && leerSeguro(g_globalPartido, &raiz, 8) && raiz &&
              leerSeguro(raiz + 0x400, &tw, 8) && tw && leerSeguro(tw, &twb, 1) && twb != 0;
    if (raizOut) *raizOut = raiz;
    return si;
}

uint64_t ganchoMontador(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    uint8_t* raiz = nullptr; uint8_t* sel = nullptr; uint8_t v = 0;
    bool training = (g_cortaActiva || g_sin1DActiva || g_directoActiva) && hayTrainingWork(&raiz);
    bool cambiar = g_cortaActiva && training && leerSeguro(raiz + 0x2C0, &sel, 8) && sel &&
                   leerSeguro(sel + 0x84, &v, 1);
    if (cambiar) { sel[0x84] = 1; InterlockedIncrement(&g_celebracionesCortas); }
    // sin_1D_free: el montador solo encola el paso 0x1D (repetición/cámara, causa del congelón) si FUN_140A3C020(partido)
    // = byte [partido+0x177F] es 0. Se pone a 1 solo durante la llamada y se restaura.
    uint8_t* part = nullptr; uint8_t v1d = 0;
    bool sin1d = g_sin1DActiva && training && leerSeguro(raiz + 0x270, &part, 8) && part &&
                 leerSeguro(part + 0x177F, &v1d, 1);
    if (sin1d) { part[0x177F] = 1; InterlockedIncrement(&g_sin1DCont); }
    uint64_t r = g_montadorOrig(a1, a2, a3, a4);
    if (cambiar) sel[0x84] = v;
    if (sin1d) part[0x177F] = v1d;
    // gol_directo_free: tras montar el gol, la cola de pasos se reescribe sin celebración, cámara ni repetición: solo el paso
    // 0x03 (procesa el gol) y la cola de reinicio nativa del propio gol (16 12 13 14 15 0E 34 19 1A). Solo datos de job+0x878/0x8F1.
    bool directo = false;
    if (g_directoActiva && training) {
        uint8_t* job = reinterpret_cast<uint8_t*>(a1);
        // con celebración (opción «gol_directo_celebracion», por defecto sí) se mantiene el paso 0x04 (celebración corta)
        static const uint32_t kDirecta[] = { 0x03, 0x16, 0x12, 0x13, 0x14, 0x15, 0x0E, 0x34, 0x19, 0x1A };
        static const uint32_t kDirectaCel[] = { 0x03, 0x04, 0x16, 0x12, 0x13, 0x14, 0x15, 0x0E, 0x34, 0x19, 0x1A };
        const uint32_t* q = g_directoCelebracion ? kDirectaCel : kDirecta;
        const size_t nq = g_directoCelebracion ? sizeof kDirectaCel / sizeof kDirectaCel[0] : sizeof kDirecta / sizeof kDirecta[0];
        uint32_t evento = 0;
        if (leerSeguro(job + 0x874, &evento, 4) && evento == 0xA) {
            for (size_t i = 0; i < nq; ++i) memcpy(job + 0x878 + i * 4, &q[i], 4);
            job[0x8F1] = (uint8_t)nq;
            directo = true; InterlockedIncrement(&g_directoCont);
        }
    }
    if (g_sondaOn) {                      // volcado de la cola de pasos que acaba de montar el juego
        uint8_t* job = reinterpret_cast<uint8_t*>(a1);
        Ev& e = nuevoEv(EV_COLA, _ReturnAddress());
        e.a = a1; e.b = leerU8(job, 0x8F0); e.c = leerU8(job, 0x8F1); e.d = leerU32(job, 0x874);
        e.x[0] = (cambiar ? 1 : 0) | (sin1d ? 2 : 0) | (directo ? 4 : 0);
        leerSeguro(job + 0x878, e.q, sizeof e.q);
        cerrarEv(e);
    }
    return r;
}

// El montador lo usan la sonda (volcado de la cola) y celebracion_corta_free: se engancha UNA vez, la primera que lo pida.
bool asegurarMontador(Feature& f) {
    if (g_montadorOrig) return true;
    uint8_t* m = firmaUnica("codigo", kFirmaMontadorGol, f.id);
    if (!m) { logf("[%s] sin firma: montador del gol (FUN_14044F8A0)", f.id); return false; }
    if (!crearDesvio(f, m, 15, (void*)ganchoMontador, (void**)&g_montadorOrig)) {
        logf("[%s] no se pudo desviar el montador del gol", f.id); return false;
    }
    logf("[%s] montador del gol enganchado en exe+0x%llX", f.id, (unsigned long long)(m - g_exe.base));
    return true;
}

bool cortaInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    if (!asegurarMontador(f)) { f.estado = "sin gancho del montador del gol (FUN_14044F8A0)"; return false; }
    g_cortaActiva = true;
    f.estado = "ok: en el entrenamiento el gol usa la secuencia corta (montador FUN_14044F8A0)";
    return true;
}
bool cortaQuitar(Feature&) { g_cortaActiva = false; return true; }

bool sin1DInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    if (!asegurarMontador(f)) { f.estado = "sin gancho del montador del gol (FUN_14044F8A0)"; return false; }
    g_sin1DActiva = true;
    f.estado = "ok: en el entrenamiento el gol no encola el paso 0x1D (montador FUN_14044F8A0, solo datos)";
    return true;
}
bool sin1DQuitar(Feature&) { g_sin1DActiva = false; return true; }

// Recorte de la celebración (paso 0x04 de la cola de gol): cuando lleva g_celebSeg s activo, se pone la fase del paso
// [job+0x8F2] a 3 (la que el juego usa al terminar el paso: ahí avanza solo al siguiente). Solo datos, desde un hilo propio.
volatile LONG g_recorteOn = 0;
HANDLE g_hiloRecorte = nullptr;
DWORD WINAPI hiloRecorte(LPVOID) {
    ULONGLONG t0 = 0; bool hecho = false, enCel = false; uint32_t ultimoTimer = 0; double ultimoT = 0;
    while (g_recorteOn) {
        Sleep(50);
        uint8_t* job = g_jobConocido;
        if (!job || !g_directoCelebracion) continue;
        uint32_t esc = 0, id = 0, timer = 0; uint8_t idx = 0xFF, fase = 0xFF;
        if (!leerSeguro(job + 0x874, &esc, 4) || !leerSeguro(job + 0x8F0, &idx, 1) || !leerSeguro(job + 0x8F2, &fase, 1)) continue;
        if (idx < 0x20) leerSeguro(job + 0x878 + idx * 4, &id, 4);
        leerSeguro(job + 0x27D0, &timer, 4);
        if (esc == 0xA && id == 0x04) {
            if (!t0) t0 = GetTickCount64();
            enCel = true; ultimoTimer = timer; ultimoT = (double)(GetTickCount64() - t0) / 1000.0;
            if (!hecho && g_celebModo != 0 && g_celebSeg > 0.0 && fase >= 1 && fase < 3 && ultimoT >= g_celebSeg) {
                hecho = true;
                if (g_celebModo == 1) {   // «timer»: se pone el contador de la celebración (job+0x27D0, ~56/s) a su valor final
                    memcpy(job + 0x27D0, &g_timerFinal, 4);
                    logf("[gol_directo_free] celebracion_modo=timer: +0x27D0 %u -> %u a %.1f s", timer, g_timerFinal, ultimoT);
                } else {                  // «salto»: indice al siguiente paso (inestable: provoca deadlock en el 2.º gol)
                    job[0x8F0] = (uint8_t)(idx + 1); job[0x8F2] = 0;
                    logf("[gol_directo_free] celebracion_modo=salto a %.1f s (paso %u -> %u)", ultimoT, (unsigned)idx, (unsigned)idx + 1);
                }
            }
        } else {
            if (enCel) logf("[gol_directo_free] paso 0x04 terminado: ultimo +0x27D0=%u a %.2f s de celebracion (modo %d)", ultimoTimer, ultimoT, g_celebModo);
            enCel = false; t0 = 0; hecho = false;
        }
    }
    return 0;
}

// --- función: cancha_chica_free. El «fuera», la línea de gol lógica y la posición del portero salen de un bloque de 36 B
// (9 floats: [0]=semilargo 52.56, [1]=semiancho 34.06, [2]=media portería 3.66, [3]=travesaño 2.44, [4]=47.06, [5]=9.16,
// [6]=36.14, [7]=20.12, [8]=41.56) que el juego reserva en el montón en CADA partido (dirección distinta). Un hilo lo
// busca (solo con TrainingWork activo) durante los 90 s siguientes a que el entrenamiento aparezca y reescribe [0],[1] y las
// áreas [4]..[8] escaladas. Json: «cancha_semilargo» (15), «cancha_semiancho» (10). No cambia nada dibujado (líneas, red).
// Validado el 27-oct con 30×20 (semilargo 15 / semiancho 10): gol, celebración y saque de centro normales.
static const uint8_t kBloqueCampo[36] = { 0x70,0x3D,0x52,0x42, 0x70,0x3D,0x08,0x42, 0x71,0x3D,0x6A,0x40, 0xF6,0x28,0x1C,0x40,
    0x70,0x3D,0x3C,0x42, 0x5C,0x8F,0x12,0x41, 0x5C,0x8F,0x10,0x42, 0xC2,0xF5,0xA0,0x41, 0x70,0x3D,0x26,0x42 };
double g_canchaLargo = 15.0, g_canchaAncho = 10.0;
bool g_cajasFisicaGrandes = false;   // json «cajas_fisica_grandes»: la caja del CAMPO (filtro de física) se deja grande; solo se encogen las de portería
volatile LONG g_canchaOn = 0, g_canchaParches = 0;
HANDLE g_hiloCancha = nullptr;

static void bloqueNuevo(uint8_t* d) {
    memcpy(d, kBloqueCampo, 36);
    float kx = (float)g_canchaLargo / 52.56f, kz = (float)g_canchaAncho / 34.06f;
    float f[9] = { (float)g_canchaLargo, (float)g_canchaAncho, 3.66f, 2.44f, 47.06f * kx, fmaxf(4.0f, 9.16f * kz),
                   36.14f * kx, fmaxf(4.5f, 20.12f * kz), 41.56f * kx };
    for (int i = 0; i < 9; ++i) { if (i == 2 || i == 3) continue; memcpy(d + i * 4, &f[i], 4); }
}
// TrainingWork «estable»: raíz → +0x3F8 (objeto de referencia) → +0x10 (datos) → byte 0 ≠ 0. (raíz+0x400 solo vale dentro de accesos puntuales.)
static bool trainingWorkEstable() {
    uint8_t* raiz = nullptr; uint8_t* obj = nullptr; uint8_t* dat = nullptr; uint8_t b = 0;
    return g_globalPartido && leerSeguro(g_globalPartido, &raiz, 8) && raiz && leerSeguro(raiz + 0x3F8, &obj, 8) && obj &&
           leerSeguro(obj + 0x10, &dat, 8) && dat && leerSeguro(dat, &b, 1) && b != 0;
}
static int parchearBloquesCampo() {
    uint8_t nuevo[36]; bloqueNuevo(nuevo);
    int n = 0; MEMORY_BASIC_INFORMATION mbi; uint8_t* a = reinterpret_cast<uint8_t*>(0x10000);
    while (a < reinterpret_cast<uint8_t*>(0x7FFE0000ull) && VirtualQuery(a, &mbi, sizeof mbi)) {
        uint8_t* ini = static_cast<uint8_t*>(mbi.BaseAddress); size_t tam = mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && (mbi.Protect == PAGE_READWRITE) && tam >= 36 && tam <= (512u << 20)) {
            __try {
                for (size_t o = 0; o + 36 <= tam; o += 4) {
                    if (*reinterpret_cast<uint32_t*>(ini + o) == 0x42523D70u && memcmp(ini + o, kBloqueCampo, 36) == 0) {
                        memcpy(ini + o, nuevo, 36); ++n;
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        a = ini + tam;
    }
    return n;
}
// Cajas de portería y del campo (.data, copiadas de .rdata al arrancar; el motor de colisiones y las decisiones de tiro/despeje las
// leen cada cuadro): 6 vectores de 16 B en exe+0x37CF4C0 (RVA del exe 1.1.0.0 de PES 2021; se comprueba el contenido antes de escribir).
static const float kCajasOrig[24] = { 52.44f,0,-4.35472f,0,  55.5047f,2.903f,4.354449f,0,  -52.44f,2.903f,4.354449f,0,  52.56f,50,35.5f,0,  -55.5047f,0,-4.35472f,0,  -52.56f,0,-35.5f,0 };
static void cajasNuevas(float* f) {
    float largo = (float)g_canchaLargo, ancho = (float)g_canchaAncho, lin = largo - 0.12f, fon = lin + 3.0647f;
    const float n[24] = { lin,0,-4.35472f,0,  fon,2.903f,4.354449f,0,  -lin,2.903f,4.354449f,0,  largo,50,ancho + 1.44f,0,  -fon,0,-4.35472f,0,  -largo,0,-(ancho + 1.44f),0 };
    memcpy(f, n, sizeof n);
    if (g_cajasFisicaGrandes) { memcpy(f + 12, kCajasOrig + 12, 4 * sizeof(float)); memcpy(f + 20, kCajasOrig + 20, 4 * sizeof(float)); }   // vec4 3 y 5 = caja del campo
}
static bool casiIgual(const float* a, const float* b) { for (int i = 0; i < 24; ++i) if (fabsf(a[i] - b[i]) > 1e-3f) return false; return true; }
// Localiza las cajas POR CONTENIDO (los 96 B originales exactos) en las secciones escribibles del exe: sirve para cualquier PES 2021 limpio
// (no depende de la RVA). Se guarda el puntero (o nulo si no se halla; se reintenta cada ~10 s).
static float* g_cajasPtr = nullptr; static ULONGLONG g_cajasIntento = 0;
static float* cajasPuntero() {
    if (g_cajasPtr) return g_cajasPtr;
    if (GetTickCount64() - g_cajasIntento < 10000) return nullptr;
    g_cajasIntento = GetTickCount64();
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(g_exe.base); auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(g_exe.base + dos->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (!(sec[i].Characteristics & IMAGE_SCN_MEM_WRITE)) continue;
        uint8_t* ini = g_exe.base + sec[i].VirtualAddress; size_t tam = sec[i].Misc.VirtualSize;
        __try {
            for (size_t o = 0; o + sizeof kCajasOrig <= tam; o += 16)
                if (casiIgual(reinterpret_cast<float*>(ini + o), kCajasOrig)) { g_cajasPtr = reinterpret_cast<float*>(ini + o); break; }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        if (g_cajasPtr) { logf("[cancha_chica_free] cajas localizadas por contenido en exe+0x%llX", (unsigned long long)(reinterpret_cast<uint8_t*>(g_cajasPtr) - g_exe.base)); break; }
    }
    return g_cajasPtr;
}
// escribe las cajas nuevas si ahora mismo están las originales; con enMarcha==false las devuelve a las originales
static void ajustarCajas(bool enMarcha) {
    float* c = cajasPuntero(); float act[24], nue[24];
    if (!c || !leerSeguro(c, act, sizeof act)) return;
    cajasNuevas(nue);
    DWORD old = 0;
    if (enMarcha && casiIgual(act, kCajasOrig)) {
        if (VirtualProtect(c, sizeof nue, PAGE_READWRITE, &old)) { memcpy(c, nue, sizeof nue); VirtualProtect(c, sizeof nue, old, &old); logf("[cancha_chica_free] cajas de porteria/campo movidas a +-%.2f", g_canchaLargo); }
    } else if (!enMarcha && casiIgual(act, nue)) {
        if (VirtualProtect(c, sizeof nue, PAGE_READWRITE, &old)) { memcpy(c, kCajasOrig, sizeof nue); VirtualProtect(c, sizeof nue, old, &old); logf("[cancha_chica_free] cajas restauradas"); }
    }
}
DWORD WINAPI hiloCancha(LPVOID) {
    bool estaba = false; ULONGLONG t0 = 0; int vuelta = 0;
    while (g_canchaOn) {
        Sleep(1000);
        bool hay = trainingWorkEstable();
        ajustarCajas(hay);
        if (hay && !estaba) t0 = GetTickCount64();
        estaba = hay;
        if (!hay || GetTickCount64() - t0 > 75000 || (++vuelta % 3) != 0) continue;     // solo los 75 s siguientes a que aparezca el entrenamiento, un barrido cada 3 s
        int n = parchearBloquesCampo();
        if (n) { InterlockedAdd(&g_canchaParches, n); logf("[cancha_chica_free] %d bloque(s) de dimensiones parcheados (%.2f x %.2f)", n, g_canchaLargo, g_canchaAncho); }
    }
    return 0;
}
bool canchaInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    g_canchaOn = 1; g_hiloCancha = CreateThread(nullptr, 0, hiloCancha, nullptr, 0, nullptr);
    if (!g_hiloCancha) { g_canchaOn = 0; f.estado = "no se pudo crear el hilo"; return false; }
    char b[160]; sprintf_s(b, "ok: cancha lógica %.1f x %.1f m (semilargo x semiancho) en el entrenamiento", g_canchaLargo * 2, g_canchaAncho * 2);
    f.estado = b; return true;
}
bool canchaQuitar(Feature&) {
    g_canchaOn = 0;
    if (g_hiloCancha) { WaitForSingleObject(g_hiloCancha, 3000); CloseHandle(g_hiloCancha); g_hiloCancha = nullptr; }
    ajustarCajas(false);
    return true;
}

// --- función: pelota_sonda (SOLO LECTURA). Localiza el objeto de la pelota por firma (como pelota.exe: objeto de 640 B,
// +248=0, +312 en {0,1}, +492 en {0x10001,0x80001}, posición float x/y/z en +208/+212/+216 con y≈0,11) y lo sigue cada 50 ms.
// Cuando se mueve a más de ~3 m/s anota en phoenix_core.log «[BALL] dir pos v» y los dwords que cambian entre muestras
// («[BALL] +0xNN a->b»): así se identifican posición y velocidad. Solo con TrainingWork activo. Json: "pelota_sonda": true.
volatile LONG g_pelotaOn = 0; HANDLE g_hiloPelota = nullptr;
volatile LONG g_vigiaPelotaArmada = 0;
uint8_t* volatile g_pelotaSeguida = nullptr;   // objeto de la pelota localizado por hiloPelota (para la vigía de escritura)
static bool pareceBola(uint8_t* o) {
    uint32_t a = 0, b = 0, c = 0; float x = 0, y = 0, z = 0;
    if (!leerSeguro(o + 248, &a, 4) || !leerSeguro(o + 312, &b, 4) || !leerSeguro(o + 492, &c, 4)) return false;
    if (a != 0 || b > 1 || (c != 0x10001 && c != 0x80001)) return false;
    if (!leerSeguro(o + 208, &x, 4) || !leerSeguro(o + 212, &y, 4) || !leerSeguro(o + 216, &z, 4)) return false;
    return x == x && z == z && fabsf(x) < 200 && fabsf(z) < 200 && y > 0.05f && y < 0.4f;
}
static void escanearBolas(std::vector<uint8_t*>& out) {
    MEMORY_BASIC_INFORMATION mbi; uint8_t* a = reinterpret_cast<uint8_t*>(0x10000);
    while (a < reinterpret_cast<uint8_t*>(0x7FFE0000ull) && VirtualQuery(a, &mbi, sizeof mbi)) {
        uint8_t* ini = static_cast<uint8_t*>(mbi.BaseAddress); size_t tam = mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && mbi.Protect == PAGE_READWRITE && tam >= 640 && tam <= (512u << 20)) {
            __try {
                for (size_t o = 0; o + 640 <= tam; o += 16) {
                    uint8_t* q = ini + o;
                    if (*reinterpret_cast<uint32_t*>(q + 248) != 0 || *reinterpret_cast<uint32_t*>(q + 312) > 1) continue;
                    uint32_t c = *reinterpret_cast<uint32_t*>(q + 492);
                    if (c != 0x10001 && c != 0x80001) continue;
                    float x = *reinterpret_cast<float*>(q + 208), y = *reinterpret_cast<float*>(q + 212), z = *reinterpret_cast<float*>(q + 216);
                    if (x == x && z == z && fabsf(x) < 200 && fabsf(z) < 200 && y > 0.05f && y < 0.4f) out.push_back(q);
                    if (out.size() > 400) return;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        a = ini + tam;
    }
}
DWORD WINAPI hiloPelota(LPVOID) {
    std::vector<uint8_t*> cand; std::vector<float> px, pz; bool tw = false; ULONGLONG ultimoEscaneo = 0; uint8_t* seguida = nullptr;
    static uint32_t previo[160]; bool tengoPrevio = false; int lineas = 0; ULONGLONG ventana = 0;
    ULONGLONG tBandera = 0; bool paredes = false; double LX = 19.5, LZ = 11.5;
    while (g_pelotaOn) {
        Sleep(paredes ? 8 : 50);
        bool hay = trainingWorkEstable();
        if (!hay) { tw = false; seguida = nullptr; cand.clear(); continue; }
        if (!tw) { tw = true; ultimoEscaneo = 0; }
        ULONGLONG ahora = GetTickCount64();
        if (!seguida && ahora - ultimoEscaneo > 4000) {   // (re)escaneo cada 4 s hasta encontrarla
            cand.clear(); escanearBolas(cand); ultimoEscaneo = GetTickCount64(); px.assign(cand.size(), 1e9f); pz.assign(cand.size(), 1e9f);
            logf("[BALL] escaneo: %u candidatos", (unsigned)cand.size());
        }
        if (!seguida) {
            for (size_t i = 0; i < cand.size(); ++i) {                       // la que se mueve de verdad es la pelota
                if (!pareceBola(cand[i])) continue;
                float x = 0, z = 0; leerSeguro(cand[i] + 208, &x, 4); leerSeguro(cand[i] + 216, &z, 4);
                if (px[i] < 1e8f && (fabsf(x - px[i]) + fabsf(z - pz[i])) > 0.3f) { seguida = cand[i]; g_pelotaSeguida = seguida; g_vigiaPelotaArmada = 0; logf("[BALL] pelota localizada en %p (x=%.2f z=%.2f)", (void*)seguida, x, z); tengoPrevio = false; break; }
                px[i] = x; pz[i] = z;
            }
            continue;
        }
        if (!pareceBola(seguida)) { logf("[BALL] se perdió la pelota (%p)", (void*)seguida); seguida = nullptr; g_pelotaSeguida = nullptr; cand.clear(); continue; }
        if (GetTickCount64() - tBandera > 1000) {   // «paredes_on.txt» en la carpeta de Core enciende el rebote en vivo (opcional: «x z» dentro)
            tBandera = GetTickCount64(); std::string t; paredes = leerArchivo(g_carpeta + "paredes_on.txt", t);
            if (paredes) { double a = 0, b = 0; if (sscanf_s(t.c_str(), "%lf %lf", &a, &b) == 2 && a > 5 && b > 3) { LX = a; LZ = b; } }
        }
        uint32_t act[160]; if (!leerSeguro(seguida, act, sizeof act)) continue;
        if (paredes) {   // paredes_core experimental: si la pelota cruza el rectángulo, se refleja su posición (y la copia previa) dentro
            float x0, z0; memcpy(&x0, (uint8_t*)act + 0x70, 4); memcpy(&z0, (uint8_t*)act + 0x78, 4);
            float nx = x0, nz = z0; bool toco = false;
            if (fabsf(x0) > LX) { nx = (float)((x0 > 0 ? 1 : -1) * (2 * LX - fabsf(x0))); toco = true; }
            if (fabsf(z0) > LZ) { nz = (float)((z0 > 0 ? 1 : -1) * (2 * LZ - fabsf(z0))); toco = true; }
            if (toco) {
                memcpy((uint8_t*)seguida + 0x70, &nx, 4); memcpy((uint8_t*)seguida + 0x78, &nz, 4);
                memcpy((uint8_t*)seguida + 0xD0, &nx, 4); memcpy((uint8_t*)seguida + 0xD8, &nz, 4);
                static int reb = 0; if (++reb <= 200) logf("[PAREDES] rebote (%.2f, %.2f) -> (%.2f, %.2f)", x0, z0, nx, nz);
                memcpy((uint8_t*)act + 0x70, &nx, 4); memcpy((uint8_t*)act + 0x78, &nz, 4); memcpy((uint8_t*)act + 0xD0, &nx, 4); memcpy((uint8_t*)act + 0xD8, &nz, 4);
            }
        }
        float x, z, y; memcpy(&x, (uint8_t*)act + 208, 4); memcpy(&y, (uint8_t*)act + 212, 4); memcpy(&z, (uint8_t*)act + 216, 4);
        if (tengoPrevio) {
            float xp, zp, yp; memcpy(&xp, (uint8_t*)previo + 208, 4); memcpy(&zp, (uint8_t*)previo + 216, 4); memcpy(&yp, (uint8_t*)previo + 212, 4);
            float d = fabsf(x - xp) + fabsf(z - zp) + fabsf(y - yp);
            if (ahora - ventana > 1000) { ventana = ahora; lineas = 0; }
            if (d > 0.0005f && lineas < 40) {    // cada cambio de posición, con hora en ms
                ++lineas;
                logf("[BALL] t=%llu pos=(%.3f, %.3f, %.3f) d=(%.3f, %.3f, %.3f)", (unsigned long long)ahora, x, y, z, x - xp, y - yp, z - zp);
                if (lineas <= 4) for (int k = 0; k < 160; ++k) if (act[k] != previo[k] && k * 4 != 208 && k * 4 != 212 && k * 4 != 216 && k * 4 != 0x70 && k * 4 != 0x74 && k * 4 != 0x78) {
                    float fa, fb; memcpy(&fa, &previo[k], 4); memcpy(&fb, &act[k], 4);
                    if (fabsf(fa) < 1000 && fabsf(fb) < 1000) logf("[BALL]   +0x%X %.5f -> %.5f", k * 4, fa, fb);
                }
            }
        }
        memcpy(previo, act, sizeof act); tengoPrevio = true;
    }
    return 0;
}
bool pelotaInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    g_pelotaOn = 1; g_hiloPelota = CreateThread(nullptr, 0, hiloPelota, nullptr, 0, nullptr);
    if (!g_hiloPelota) { g_pelotaOn = 0; f.estado = "no se pudo crear el hilo"; return false; }
    f.estado = "ok: sonda de la pelota (solo lectura): localiza el objeto de 640 B y anota posición/velocidad en el log"; return true;
}
bool pelotaQuitar(Feature&) {
    g_pelotaOn = 0;
    if (g_hiloPelota) { WaitForSingleObject(g_hiloPelota, 3000); CloseHandle(g_hiloPelota); g_hiloPelota = nullptr; }
    return true;
}

bool directoInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    if (!asegurarMontador(f)) { f.estado = "sin gancho del montador del gol (FUN_14044F8A0)"; return false; }
    g_directoActiva = true;
    if (!g_hiloRecorte) { g_recorteOn = 1; g_hiloRecorte = CreateThread(nullptr, 0, hiloRecorte, nullptr, 0, nullptr); }
    f.estado = g_directoCelebracion ? "ok: cola del gol en el entrenamiento = 03 04 16 12 13 14 15 0E 34 19 1A (celebración corta, sin repetición)" : "ok: cola del gol en el entrenamiento = 03 16 12 13 14 15 0E 34 19 1A (sin celebración ni repetición)";
    return true;
}
bool directoQuitar(Feature&) {
    g_directoActiva = false; g_recorteOn = 0;
    if (g_hiloRecorte) { WaitForSingleObject(g_hiloRecorte, 1000); CloseHandle(g_hiloRecorte); g_hiloRecorte = nullptr; }
    return true;
}

// --- sonda de pasos: FUN_14047C2D0(job, saltable, ...) es el ejecutor de la cola job+0x878[]. Se anota cada CAMBIO de
// (índice de cola [job+0x8F0], id de paso) para saber qué paso está activo cuando el juego se congela. Prólogo 17 B
// sin RIP-relativos (mov [rsp+8],rbx; push rdi; sub rsp,20h; movzx eax,[rcx+8F0h]); la siguiente es `lea r11,[rip+…]`.
const char* kFirmaPaso = "48 89 5C 24 08 57 48 83 EC 20 0F B6 81 F0 08 00 00 4C 8D 1D ?? ?? ?? ?? 0F B6 FA 48 8B D9 44 8B 94 81 78 08 00 00";
using FnPaso = uint64_t (*)(uint8_t* job, uint64_t saltable, uint64_t p3);
FnPaso g_pasoOrig = nullptr;
uint8_t* g_pasoJob = nullptr; uint32_t g_pasoIdx = 0xFFFF, g_pasoId = 0xFFFF; volatile ULONGLONG g_tUltGol = 0;   // 0.4.80: instante del último paso 0x04 (celebración de gol)
uint64_t ganchoPaso(uint8_t* job, uint64_t saltable, uint64_t p3) {
    uint8_t idx = 0xFF; uint32_t id = 0xFFFFFFFF;
    if (leerSeguro(job + 0x8F0, &idx, 1) && idx < 0x20) leerSeguro(job + 0x878 + idx * 4, &id, 4);
    if (id == 0x04 && g_pasoId != 0x04) g_tUltGol = GetTickCount64();
    if (job != g_pasoJob || idx != g_pasoIdx || id != g_pasoId) {
        g_pasoJob = job; g_pasoIdx = idx; g_pasoId = id;
        Ev& e = nuevoEv(EV_PASO, _ReturnAddress());
        e.a = reinterpret_cast<uint64_t>(job); e.b = idx; e.c = id; e.d = leerU32(job, 0x874);
        e.x[0] = leerU8(job, 0x8F2); e.x[1] = leerU8(job, 0x8F1); e.x[2] = saltable & 0xFF; e.x[3] = p3 & 0xFF;
        cerrarEv(e);
    }
    return g_pasoOrig(job, saltable, p3);
}

// --- función: rescate del bloqueo juego↔render (EXPERIMENTAL, APAGADA por defecto). En el Entrenamiento libre, al empezar la
// repetición del gol, el hilo de juego se queda en FUN_141717150 esperando [DAT_143747E98+0xC4]!=0 (que pone el hilo de
// render) mientras el render espera trabajo: deadlock. FUN_141717150 repite `CALL FUN_1403DC7D0` (esperar) hasta que +0xC4
// cambia. Se desvía ese CALL a una cueva que llama a un ayudante en C: si la misma espera dura ≥3 s y hay TrainingWork,
// fuerza +0xC4=1 (sale del bucle) y lo anota en el log.
const char* kFirmaEsperaRender = "48 8D 4C 24 38 E8 ?? ?? ?? ?? 8B 83 C4 00 00 00 85 C0 74";
volatile LONG g_rescates = 0;
volatile LONG g_fotogramas = 0;     // +1 en cada phx_core_tick (display_frame de Sider): se para si el hilo de juego se bloquea
void ayudanteRescate(uint8_t* obj) {
    // 0.4.12b: FUN_141717150 se llama en CADA fotograma (menús, carga y juego), así que «espera larga» no basta: el
    // bloqueo real es «≥3 s sin que avance el contador de fotogramas y seguimos dentro de esta espera».
    static ULONGLONG tPrim = 0, tUlt = 0; static LONG fotoPrev = -1;
    ULONGLONG ahora = GetTickCount64();
    LONG fr = g_fotogramas;
    if (fr != fotoPrev || ahora - tUlt > 500) { fotoPrev = fr; tPrim = ahora; }   // hay fotogramas o espera nueva
    tUlt = ahora;
    if (ahora - tPrim < 3000) return;
    tPrim = ahora;                                  // se rearma: como mucho 1 rescate cada 3 s
    LONG v = 0;
    if (!obj || !hayTrainingWork() || !leerSeguro(obj + 0xC4, &v, 4) || v != 0) return;
    InterlockedExchange(reinterpret_cast<volatile LONG*>(obj + 0xC4), 1);
    LONG n = InterlockedIncrement(&g_rescates);
    logf("[rescate_render_free] rescate nº %ld: [render+0xC4] forzado a 1 tras 3 s de espera (obj %llX)", n,
         (unsigned long long)reinterpret_cast<uintptr_t>(obj));
}
bool rescateInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    uint8_t* sig = firmaUnica("codigo", kFirmaEsperaRender, f.id);
    if (!sig) { f.estado = "sin firma: espera del render (FUN_141717150)"; return false; }
    uint8_t* sitio = sig + 5;                       // el E8 rel32 hacia FUN_1403DC7D0
    int32_t rel = 0; memcpy(&rel, sitio + 1, 4);
    uint8_t* original = sitio + 5 + rel;
    uint8_t* c = reservarCerca(sitio, 128);
    if (!c) { f.estado = "no hay memoria cerca del exe (rescate)"; return false; }
    size_t o = 0;
    auto b = [&](std::initializer_list<uint8_t> l) { for (uint8_t x : l) c[o++] = x; };
    b({ 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53 });   // push rax,rcx,rdx,r8,r9,r10,r11
    b({ 0x48, 0x83, 0xEC, 0x20 });                                               // sub rsp,20h
    b({ 0x48, 0x89, 0xD9 });                                                     // mov rcx,rbx (obj de render)
    b({ 0x48, 0xB8 }); { uint64_t h = reinterpret_cast<uint64_t>(&ayudanteRescate); memcpy(c + o, &h, 8); o += 8; }   // mov rax,helper
    b({ 0xFF, 0xD0 });                                                           // call rax
    b({ 0x48, 0x83, 0xC4, 0x20 });                                               // add rsp,20h
    b({ 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58 });   // pop r11,r10,r9,r8,rdx,rcx,rax
    b({ 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 });                                  // jmp [rip+0] → FUN_1403DC7D0 (cola del call)
    { uint64_t t = reinterpret_cast<uint64_t>(original); memcpy(c + o, &t, 8); o += 8; }
    FlushInstructionCache(GetCurrentProcess(), c, 128);
    Parche p; p.dir = sitio;
    p.original.assign(sitio, sitio + 5);
    p.nuevo.resize(5);
    if (!escribirRel32(p.nuevo.data(), 0xE8, sitio, c)) { f.estado = "cueva fuera de alcance (rescate)"; return false; }
    if (!aplicarParche(p, f.id)) { f.estado = "bytes inesperados (rescate)"; return false; }
    f.parches.push_back(p);
    char s[140];
    sprintf_s(s, "ok: rescate del render tras 3 s de espera en exe+0x%llX (solo con TrainingWork)", (unsigned long long)(sitio - g_exe.base));
    f.estado = s;
    return true;
}
bool rescateQuitar(Feature&) { return true; }

bool sondaInstalar(Feature& f) {
    if (!resolverGlobalPartido(f.id)) logf("[%s] sin firma del global del partido: k/sub/parado no se anotan", f.id);
    uint8_t* dirs[_countof(g_ganchos)];
    for (size_t i = 0; i < _countof(g_ganchos); ++i) {      // primero se buscan todas: o todas o ninguna
        auto& g = g_ganchos[i];
        dirs[i] = firmaUnica("codigo", g.firma, f.id);
        if (!dirs[i]) { f.estado = std::string("sin firma: ") + g.nombre; return false; }
        uint32_t rva = static_cast<uint32_t>(dirs[i] - g_exe.base);
        logf("[%s] %s en exe+0x%X%s", f.id, g.nombre, rva, rva == g.rvaEsperada ? "" : " (exe distinto del de Conmegol)");
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

// --- sonda_pasos (0.4.12b: función propia, apagable por separado de sonda_partido): cola de pasos del gol (COLA, vía el
// montador FUN_14044F8A0) y ejecutor de pasos (PASO, FUN_14047C2D0). En 0.4.12 iban dentro de la sonda y el juego se cayó en
// el menú a los 4 s (en la VM de .impdata); así se pueden probar de uno en uno. Necesita sonda_partido para escribir.
// 0.4.12c — sonda_pasos v2 POR SONDEO, sin tocar código del exe: tanto en 0.4.12 como en 0.4.12b el juego se cayó (en la VM
// de .impdata) con el hook de FUN_14047C2D0 instalado; el montador ya funcionaba en 0.4.11 pero tampoco se toca aquí.
// Un hilo propio mira cada 50 ms el último «job» conocido ([job+0x874] escena, [job+0x8F0] índice de cola, [job+0x8F1] nº de
// pasos, [job+0x8F2] fase, [job+0x878+idx*4] paso activo) y escribe en phoenix_core.log cada CAMBIO («[PASO] …», con los
// segundos desde el último gol) y, cuando la cola de la escena 0xA se estabiliza, la cola entera («[COLA] …»). Como es
// otro hilo, sigue escribiendo aunque el hilo de juego se bloquee (deadlock): así sabremos qué paso estaba activo.
volatile LONG g_sondeoPasos = 0;
HANDLE g_hiloPasos = nullptr;
// 0.4.17 [JOB]: durante la escena 0xA (primeros 40 s) compara cada 250 ms tres zonas del job y anota los dwords que cambian
// (offset: antes -> ahora). Sirve para encontrar el temporizador que hace esperar al paso 0x04 en la fase 3.
static const uint32_t kZonasJob[][2] = { {0x860, 0xA00}, {0x2780, 0x2830}, {0x57F0, 0x5900} };
static void sondearJob(uint8_t* job, uint32_t* previo, bool& tengo, double desdeGol) {
    static uint32_t cur[(0x1A0 + 0xB0 + 0x110) / 4];
    size_t o = 0;
    for (auto& z : kZonasJob) { if (!leerSeguro(job + z[0], cur + o, z[1] - z[0])) return; o += (z[1] - z[0]) / 4; }
    if (tengo) {
        int n = 0; size_t i = 0;
        for (auto& z : kZonasJob) {
            for (uint32_t off = z[0]; off < z[1]; off += 4, ++i) {
                if (cur[i] != previo[i] && n < 24) {
                    logf("[JOB] +0x%X: %08X -> %08X (%+.2f s)", off, previo[i], cur[i], desdeGol); ++n;
                }
            }
        }
    }
    memcpy(previo, cur, o * 4); tengo = true;
}

DWORD WINAPI sondeoPasos(LPVOID) {
    uint8_t* jobPrev = nullptr; uint32_t escPrev = 0xFFFFFFFF, idxPrev = 0xFF, nPrev = 0xFF, idPrev = 0xFFFFFFFF, faPrev = 0xFF;
    int estable = 0; uint32_t nCola = 0xFF; bool colaVolcada = false;
    while (g_sondeoPasos) {
        Sleep(50);
        uint8_t* job = g_jobConocido;
        if (!job) continue;
        uint8_t idx = 0xFF, n = 0xFF, fase = 0xFF; uint32_t esc = 0xFFFFFFFF, id = 0xFFFFFFFF;
        {   // [SUB]: cambios de sub (+0x1554) y k (+0x1574) del objeto partido
            static uint32_t subPrev = 0xFFFFFFFE, kPrev = 0xFFFFFFFE;
            uint8_t* ob = objPartido();
            uint32_t sub = ob ? leerU32(ob, 0x1554) : 0xFFFFFFFF, kk = ob ? leerU32(ob, 0x1574) : 0xFFFFFFFF;
            if (sub != subPrev || kk != kPrev) { logf("[SUB] sub=%u k=%u", sub, kk); subPrev = sub; kPrev = kk; }
        }
        {   // [JOB]: diferencias en la escena 0xA, cada 250 ms, hasta 40 s tras el gol
            static uint32_t previoJob[(0x1A0 + 0xB0 + 0x110) / 4]; static bool tengo = false; static int vuelta = 0;
            uint32_t e0 = 0;
            ULONGLONG tg0 = g_tUltimoGol; double dg = tg0 ? (double)((long long)(GetTickCount64() - tg0)) / 1000.0 : -1.0;
            if (leerSeguro(job + 0x874, &e0, 4) && e0 == 0xA && dg < 40.0) { if (++vuelta >= 5) { vuelta = 0; sondearJob(job, previoJob, tengo, dg); } }
            else tengo = false;
        }
        if (!leerSeguro(job + 0x874, &esc, 4) || !leerSeguro(job + 0x8F0, &idx, 1) || !leerSeguro(job + 0x8F1, &n, 1) ||
            !leerSeguro(job + 0x8F2, &fase, 1)) continue;
        if (idx < 0x20) leerSeguro(job + 0x878 + idx * 4, &id, 4);
        if (job != jobPrev || esc != escPrev || idx != idxPrev || n != nPrev || id != idPrev || fase != faPrev) {
            ULONGLONG tg = g_tUltimoGol;
            double desdeGol = tg ? (double)((long long)(GetTickCount64() - tg)) / 1000.0 : -1.0;
            logf("[PASO] job=%llX escena=0x%X cola=%u paso=0x%02X n=%u fase=%u (%+.2f s desde el gol)",
                 (unsigned long long)reinterpret_cast<uintptr_t>(job), esc, idx, id, n, fase, desdeGol);
            if (esc != escPrev || job != jobPrev || n != nPrev) { estable = 0; colaVolcada = false; }
            jobPrev = job; escPrev = esc; idxPrev = idx; nPrev = n; idPrev = id; faPrev = fase;
        } else if (esc == 0x0A && !colaVolcada && ++estable >= 4) {      // 200 ms sin cambios en n: la cola ya está montada
            uint32_t q[32] = {};
            if (leerSeguro(job + 0x878, q, sizeof q)) {
                char b[260] = {}; size_t o = 0;
                for (int k = 0; k < 32 && q[k] != 0x48 && o + 4 < sizeof b; ++k) o += sprintf_s(b + o, sizeof b - o, "%02X ", q[k]);
                logf("[COLA] escena 0xA, n=%u: %s", n, b);
            }
            colaVolcada = true;
        }
    }
    return 0;
}
bool pasosInstalar(Feature& f) {
    g_sondeoPasos = 1;
    g_hiloPasos = CreateThread(nullptr, 0, sondeoPasos, nullptr, 0, nullptr);
    if (!g_hiloPasos) { g_sondeoPasos = 0; f.estado = "no se pudo crear el hilo"; return false; }
    f.estado = "ok: sondeo de la cola de pasos del gol cada 50 ms (resultado en phoenix_core.log; sin parches de código)";
    return true;
}
bool pasosQuitar(Feature&) {
    g_sondeoPasos = 0;
    if (g_hiloPasos) { WaitForSingleObject(g_hiloPasos, 1000); CloseHandle(g_hiloPasos); g_hiloPasos = nullptr; }
    return true;
}

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
        else if (c.tipo == EV_PASO)
            sprintf_s(l, "%8llu h%-5u PASO job=%llX cola=%llu paso=0x%02llX escena=0x%llX fase=%llu n=%llu saltable=%llu p3=%llu desde exe+0x%llX\r\n",
                      c.t, c.hilo, c.a, c.b, c.c, c.d, c.x[0], c.x[1], c.x[2], c.x[3], ret);
        else if (c.tipo == EV_COLA) {
            char pasos[400] = {}; size_t o = 0;
            for (int k = 0; k < 32 && c.q[k] != 0x48 && o + 8 < sizeof pasos; ++k) o += sprintf_s(pasos + o, sizeof pasos - o, "%02X ", c.q[k]);
            sprintf_s(l, "%8llu h%-5u COLA job=%llX escena=0x%llX [+8F0]=%llu [+8F1]=%llu corta=%llu pasos: %s\r\n",
                      c.t, c.hilo, c.a, c.d, c.b, c.c, c.x[0], pasos);
        }
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

// --- función 4: vigilante de cuelgues (solo mira). Hilo propio que cada 250 ms mira la CPU de cada hilo de PES.
// Si el proceso gasta mucha CPU (≥2 núcleos durante 2 s) y la sonda no ha anotado nada en 3 s, entra en «modo
// perfil»: durante 2 s detiene un instante cada hilo ocupado, lee su RIP (y su pila) y lo suelta; al final anota en
// phoenix_core.log el top de zonas de código (RIP & ~0x3FF, con módulo si no es el exe) y una pila de ejemplo.
// Además deja un latido cada 10 s. Sirve para encontrar cuelgues de varios hilos sin volcados de memoria.
struct HiloCpu { DWORD tid; ULONGLONG cpu; ULONGLONG delta; };
struct Zona { uint64_t clave; int n; uint64_t pila[8]; };
volatile LONG g_vigilar = 0;
HANDLE g_hiloVigilante = nullptr;

ULONGLONG cpuHilo(HANDLE h) {
    FILETIME c, e, k, u;
    if (!GetThreadTimes(h, &c, &e, &k, &u)) return 0;
    return ((ULONGLONG)k.dwHighDateTime << 32 | k.dwLowDateTime) + ((ULONGLONG)u.dwHighDateTime << 32 | u.dwLowDateTime);
}

void nombreZona(uint64_t dir, char* out, size_t n) {
    const uint64_t ini = reinterpret_cast<uint64_t>(g_exe.base);
    if (dir >= ini && dir < ini + g_exe.tamImagen) { sprintf_s(out, n, "exe+0x%llX", dir - ini); return; }
    HMODULE m = nullptr; char ruta[MAX_PATH] = "?";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(dir), &m) && m) {
        GetModuleFileNameA(m, ruta, MAX_PATH);
        const char* barra = strrchr(ruta, '\\');
        sprintf_s(out, n, "%s+0x%llX", barra ? barra + 1 : ruta, dir - reinterpret_cast<uint64_t>(m));
    } else sprintf_s(out, n, "0x%llX", dir);
}

DWORD WINAPI vigilante(LPVOID) {
    const DWORD yo = GetCurrentThreadId(), pid = GetCurrentProcessId();
    std::vector<HiloCpu> hilos;
    ULONGLONG tLatido = GetTickCount64(), tVentana = GetTickCount64(), tUltimoPerfil = 0;
    LONG escrVentana = g_escr;
    int perfiles = 0, ventanasQuietas = 0;
    while (g_vigilar) {
        Sleep(250);
        // 1) CPU por hilo (sin detener a nadie)
        std::vector<DWORD> tids;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE) continue;
        THREADENTRY32 te; te.dwSize = sizeof te;
        for (BOOL ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te))
            if (te.th32OwnerProcessID == pid && te.th32ThreadID != yo) tids.push_back(te.th32ThreadID);
        CloseHandle(snap);
        ULONGLONG ahora = GetTickCount64();
        if (ahora - tVentana < 2000) continue;
        ULONGLONG total = 0;
        std::vector<HiloCpu> nuevos;
        for (DWORD tid : tids) {
            HANDLE h = OpenThread(THREAD_QUERY_INFORMATION, FALSE, tid);
            if (!h) continue;
            ULONGLONG c = cpuHilo(h); CloseHandle(h);
            ULONGLONG d = 0;
            for (auto& x : hilos) if (x.tid == tid) { d = c > x.cpu ? c - x.cpu : 0; break; }
            nuevos.push_back({ tid, c, d }); total += d;
        }
        hilos.swap(nuevos);
        bool sondaQuieta = g_escr == escrVentana;
        escrVentana = g_escr; tVentana = ahora;
        ventanasQuietas = sondaQuieta ? ventanasQuietas + 1 : 0;
        if (ahora - tLatido >= 10000) {
            tLatido = ahora;
            logf("[VIGILANTE] vivo · CPU del proceso %.1f s en 2 s · %zu hilos · sonda %s", total / 1e7, hilos.size(),
                 sondaQuieta ? "quieta" : "anotando");
        }
        // 2) ¿cuelgue? sonda quieta ≥8 s (4 ventanas) y CPU alta. La CPU sola no sirve: este PES usa ~5 núcleos
        //    jugando normal (17 falsas alarmas en un amistoso, 26-oct).
        //    Solo en los 3 min tras un gol (el congelón del entreno es en la celebración): así el cupo no se gasta en
        //    menús/carga. Sin umbral de CPU: congelado consume lo mismo que jugando (~9 s en 2 s). Cupo 10 por gol.
        static ULONGLONG golContado = 0;
        ULONGLONG tGol = g_tUltimoGol;
        if (tGol != golContado) { golContado = tGol; perfiles = 0; }
        if (!tGol || ahora - tGol > 180000) continue;
        if (ventanasQuietas < 4 || perfiles >= 10 || ahora - tUltimoPerfil < 20000) continue;
        tUltimoPerfil = ahora; ++perfiles;
        std::vector<Zona> zonas;
        for (int muestra = 0; muestra < 8 && g_vigilar; ++muestra) {
            for (auto& hc : hilos) {
                if (hc.delta < 3000000) continue;            // ≥0,3 s de CPU en la ventana
                HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, hc.tid);
                if (!h) continue;
                CONTEXT ctx = {}; ctx.ContextFlags = CONTEXT_CONTROL;
                uint64_t pila[64] = {};
                bool leido = false;
                if (SuspendThread(h) != (DWORD)-1) {         // entre Suspend y Resume: nada que reserve memoria
                    leido = GetThreadContext(h, &ctx) != 0;
                    if (leido) { SIZE_T nl = 0; ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(ctx.Rsp), pila, sizeof pila, &nl); }
                    ResumeThread(h);
                }
                CloseHandle(h);
                if (!leido) continue;
                uint64_t clave = ctx.Rip & ~0x3FFull;
                Zona* z = nullptr;
                for (auto& x : zonas) if (x.clave == clave) { z = &x; break; }
                if (!z) {
                    zonas.push_back({ clave, 0, {} }); z = &zonas.back();
                    z->pila[0] = ctx.Rip;
                    const uint64_t ini = reinterpret_cast<uint64_t>(g_exe.base), fin = ini + g_exe.tamImagen;
                    int k = 1;
                    for (uint64_t v : pila) if (v >= ini && v < fin && k < 8) z->pila[k++] = v;
                }
                z->n++;
            }
            Sleep(250);
        }
        std::sort(zonas.begin(), zonas.end(), [](const Zona& x, const Zona& y) { return x.n > y.n; });
        logf("[VIGILANTE] posible cuelgue: CPU %.1f s en 2 s, sonda quieta %d s, %.0f s tras el gol. Top zonas (muestras):", total / 1e7, ventanasQuietas * 2, (ahora - tGol) / 1000.0);
        for (size_t i = 0; i < zonas.size() && i < 12; ++i) {
            char nom[160], pil[600] = {}; size_t o = 0;
            nombreZona(zonas[i].pila[0], nom, sizeof nom);
            for (int k = 1; k < 8 && zonas[i].pila[k]; ++k) {
                char r[40]; nombreZona(zonas[i].pila[k], r, sizeof r);
                o += sprintf_s(pil + o, sizeof pil - o, " %s", r);
            }
            logf("[VIGILANTE]   %2d× %s · pila:%s", zonas[i].n, nom, pil);
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

// --- función: siempre_disparo_free (0.4.27). En el Entrenamiento libre, cuadrado (ThinkUnitShoot) se convierte en despeje
// (ThinkUnitClear, acción 0x58) cuando FUN_1406b45d0 (predicado «tiro permitido», 1596 B, 7 args) devuelve 0: pasa con el
// jugador junto a su propia área / lejos del arco. Gancho: si hay TrainingWork (visto en los últimos 3 s) devuelve 1 sin evaluar,
// así cuadrado dispara siempre. Fuera del entrenamiento (amistoso, liga) llama a la original. Prólogo de 20 B limpios.
// modo 5 (0.4.33): Clear no devuelve nada al pulsar ni al mantener; solo 0x55 al soltar (+0x70), como Shoot.
const char* kFirmaPredTiro = "48 8B C4 55 57 41 55 41 56 48 8D 68 A8 48 81 EC 38 01 00 00 8B BD 88 00 00 00 4C 8B F1 0F 29 70 C8";
using FnPredTiro = uint32_t (*)(uint64_t, int, float, float, uint64_t, int, char);
FnPredTiro g_predTiroOrig = nullptr;
int g_disparoModo = 2;   // json «siempre_disparo_modo»: 0 = solo el predicado de Shoot, 1 = Clear devuelve 0 (congela), 2 = Clear devuelve la acción de tiro 0x55
// --- modo 8 (0.4.40), de la RE del frente D (27-oct): ordA = cmd+0x1768 es el id de la ThinkUnit ganadora (3 Shoot, 4 Clear). FUN_1408B08A0 empuja
// Clear antes que Shoot, y cada unidad se habilita con su ranura +0x88: Shoot (FUN_1408B9C80) devuelve «no» en la mitad propia con mode+0x116C==1;
// Clear (FUN_1408BB0D0) solo se habilita en mitad propia. FUN_1408B1490 pone la potencia a 1.0 si el id de unidad es 4. Dos parches de 1 byte:
//  A) 0x1408B9CE8 «75 22» -> «EB 22» (Shoot siempre aplicable)  B) 0x1408BB150 «76 34» -> «EB 34» (Clear nunca aplicable).
// Se ponen solo mientras hay TrainingWork (hilo de la vigía de entrenamiento) y se restauran al salir. Entonces gana Shoot (ordA=3, barra y potencia por carga).
struct ParcheDisp { uint8_t* dir = nullptr; uint8_t ori = 0, nue = 0; bool puesto = false; };
ParcheDisp g_pdA, g_pdB;
static bool localizarParche(const char* firma, size_t desp, uint8_t ori, uint8_t nue, ParcheDisp& pd, const char* id) {
    uint8_t* m = firmaUnica("codigo", firma, id); if (!m) return false;
    pd.dir = m + desp; pd.ori = ori; pd.nue = nue;
    uint8_t v = 0; if (!leerSeguro(pd.dir, &v, 1) || v != ori) { logf("[%s] byte inesperado en exe+0x%llX (0x%02X, esperaba 0x%02X)", id, (unsigned long long)(pd.dir - g_exe.base), v, ori); pd.dir = nullptr; return false; }
    return true;
}
static void ponerParche(ParcheDisp& pd, bool si) {
    if (!pd.dir || pd.puesto == si) return;
    uint8_t v = si ? pd.nue : pd.ori; if (escribirCodigo(pd.dir, &v, 1)) pd.puesto = si;
}
volatile ULONGLONG g_tTrainingVisto = 0;
volatile LONG g_disparoOn = 0;
volatile LONG g_disparoForzados = 0, g_disparoLlamadas = 0;
static uint32_t g_dispArg[16][3]; volatile LONG g_dispN = 0;
HANDLE g_hiloDisparo = nullptr;
DWORD WINAPI hiloDisparo(LPVOID) {
    bool visto = false; ULONGLONG tLog = 0; LONG ult = 0;
    while (g_disparoOn) {
        bool tw = trainingWorkEstable() || hayTrainingWork();
        if (tw) g_tTrainingVisto = GetTickCount64();
        if (g_disparoModo >= 8) { bool dentro = GetTickCount64() - g_tTrainingVisto < 3000; ponerParche(g_pdA, dentro); ponerParche(g_pdB, dentro); }
        if (tw != visto) { visto = tw; logf("[siempre_disparo_free] TrainingWork %s", tw ? "visto" : "no visto"); }
        if (GetTickCount64() - tLog > 5000) { tLog = GetTickCount64(); LONG n = g_disparoLlamadas, f = g_disparoForzados;
            if (n != ult) { logf("[siempre_disparo_free] predicado llamado %ld veces, forzado a 1 en %ld", (long)n, (long)f); ult = n; } }
        Sleep(100);
    }
    return 0;
}
uint32_t ganchoPredTiro(uint64_t a1, int a2, float a3, float a4, uint64_t a5, int a6, char a7) {
    LONG nn = InterlockedIncrement(&g_disparoLlamadas);
    if ((a2 == 0 || a2 == 11) && nn > 12) { static LONG nGk = 0; LONG k = InterlockedIncrement(&nGk); if (k <= 200) logf("[CURSOR_PT] predicado de tiro para el portero: jugador=%d (cursor/pad en el portero)", a2); }
    if (nn <= 12) logf("[siempre_disparo_free] predicado: jugador=%d modo=%d lado=%d training=%d", a2, a6, (int)a7, (int)(GetTickCount64() - g_tTrainingVisto < 3000));
    // 0.4.60 (frente L): el portero (slots 0 y 11) conserva su lógica de portero: el tiro de campo forzado NO se le aplica (si el cursor cae en él tras un reinicio no actúa como jugador de campo).
    if (g_disparoOn && a2 != 0 && a2 != 11 && GetTickCount64() - g_tTrainingVisto < 3000) { InterlockedIncrement(&g_disparoForzados); return 1; }
    return g_predTiroOrig(a1, a2, a3, a4, a5, a6, a7);
}
// 0.4.29: ThinkUnitClear (vftable exe+0x260FEE8) comparte el botón 0xF con ThinkUnitShoot. Se cambian sus ranuras +0x60 (FUN_1408AEEE0)
// y +0x68 (FUN_1408AEF70) —datos de .rdata, no código— por funciones que devuelven 0 (sin despeje) si hay TrainingWork visto.
using FnClear60 = uint64_t (*)(int64_t);
using FnClear68 = uint64_t (*)(uint64_t, int64_t);
FnClear60 g_clear60Orig = nullptr; FnClear68 g_clear68Orig = nullptr; FnClear68 g_clear70Orig = nullptr;
volatile ULONGLONG g_clearUltimo = 0;   // último instante en que Clear devolvió 0x55 (mantener «dueño» de la acción al soltar)
volatile LONG g_clearAnulados = 0;
bool trainingReciente() { return g_disparoOn && GetTickCount64() - g_tTrainingVisto < 3000; }
int g_disparoCarga = 1;   // json «siempre_disparo_carga»: 1 = la carga (+0x20) de Clear sigue a la de Shoot (potencia calculable)
static void sincronizarCarga(uint64_t clear, const char* donde) {
    float cc = 0, cs = 0; uint8_t* c = reinterpret_cast<uint8_t*>(clear); uint8_t* sh = c - 0x40;
    if (!leerSeguro(c + 0x20, &cc, 4) || !leerSeguro(sh + 0x20, &cs, 4)) return;
    static LONG n = 0; if (InterlockedIncrement(&n) <= 60) logf("[CARGA] %s Clear=%.3f Shoot=%.3f", donde, cc, cs);
    if (g_disparoCarga && cs >= 0.0f && cs < 5.0f) memcpy(c + 0x20, &cs, 4);
}
// modo 7 (0.4.35). Causa de la barra llena (RE del 27-oct): la unidad Clear tiene +0x14 = 4; FUN_1408AC9D0 (acumula la carga +0x20,
// tope 1.0) sale de inmediato si +0x14 == 4 y FUN_1408AB000 pone el medidor a 1.0 cuando el tipo de orden (CommandPlayer+0x1768) es 4.
// Aquí Clear se vuelve tipo 1 y acumula la carga como Shoot (FUN_1408AC9D0 con la acción 0x55).
using FnAcum = void (*)(uint64_t, int64_t, uint32_t);
uint64_t ganchoClear60(int64_t p1) {
    if (g_disparoModo >= 7 && trainingReciente()) {
        *(int*)(p1 + 0x14) = 1; *(float*)(p1 + 0x20) = 0.0f; g_clearUltimo = GetTickCount64();
        static LONG n = 0; if (InterlockedIncrement(&n) <= 10) logf("[siempre_disparo_free] modo 7: Clear tipo 1, carga a 0, 0x55");
        return 0x55;
    }
    if (trainingReciente()) {
        LONG n = InterlockedIncrement(&g_clearAnulados); if (n <= 10) logf("[siempre_disparo_free] despeje (+0x60) -> %s", (g_disparoModo >= 2 && g_disparoModo < 5) ? "tiro 0x55" : "nada (modo>=5)");
        if (g_disparoModo >= 5) { *(float*)(p1 + 0x20) = 0.1f; g_clearUltimo = GetTickCount64(); return 0; }
        if (g_disparoModo >= 2) { *(float*)(p1 + 0x20) = 0.1f; g_clearUltimo = GetTickCount64(); return 0x55; }
        return 0;
    }
    return g_clear60Orig(p1);
}
uint64_t ganchoClear68(uint64_t p1, int64_t p2) {
    uint64_t r = g_clear68Orig(p1, p2);
    if (g_disparoModo >= 7 && r != 0 && trainingReciente()) {
        reinterpret_cast<FnAcum>(g_exe.base + 0x8AC9D0)(p1, p2, 0x55);
        g_clearUltimo = GetTickCount64();
        float c = 0; leerSeguro(reinterpret_cast<void*>(p1 + 0x20), &c, 4);
        static LONG n = 0; if (InterlockedIncrement(&n) <= 40) logf("[CARGA] modo 7 mantener carga=%.3f", c);
        return 0x55;
    }
    if (r != 0 && trainingReciente()) {
        LONG n = InterlockedIncrement(&g_clearAnulados); if (n <= 10) logf("[siempre_disparo_free] despeje (+0x68) -> %s", (g_disparoModo >= 2 && g_disparoModo < 5) ? "tiro 0x55" : "nada (modo>=5)");
        g_clearUltimo = GetTickCount64();
        if (g_disparoModo >= 4) sincronizarCarga(p1, "mantener");
        return (g_disparoModo >= 2 && g_disparoModo < 5) ? 0x55 : 0;
    }
    return r;
}
uint64_t ganchoClear70(uint64_t p1, int64_t p2) {
    uint64_t r = g_clear70Orig(p1, p2);
    if (g_disparoModo >= 4 && r == 0 && trainingReciente() && GetTickCount64() - g_clearUltimo < 250) {
        static LONG n = 0; if (InterlockedIncrement(&n) <= 10) logf("[siempre_disparo_free] soltar: Clear(+0x70) mantiene 0x55");
        if (g_disparoModo >= 7) { float c = 0; leerSeguro(reinterpret_cast<void*>(p1 + 0x20), &c, 4); logf("[CARGA] modo 7 soltar carga=%.3f", c); }
        else sincronizarCarga(p1, "soltar");
        return 0x55;
    }
    return r;
}
// 0.4.34 modo 6: en campo propio ThinkUnitShoot está DESACTIVADO (byte +0x18 = 0: FUN_1408AC500 devuelve 0 de inmediato; por eso
// nunca corre su tiempo de carga y la acción 0x55 que arranca Clear sale a tope). Gancho de la ranura +0x40 de Shoot (el despachador):
// en el entrenamiento pone ese byte a 1 antes de llamar al original; Shoot cuenta la carga con normalidad.
using FnDesp = int (*)(int64_t*, int64_t*, int);
FnDesp g_despShootOrig = nullptr; volatile LONG g_despForzados = 0;
int ganchoDespShoot(int64_t* p1, int64_t* p2, int p3) {
    if (g_disparoModo >= 6 && trainingReciente()) {
        uint8_t* h = reinterpret_cast<uint8_t*>(p1); uint8_t v = 0;
        if (leerSeguro(h + 0x18, &v, 1) && v == 0) { h[0x18] = 1; LONG n = InterlockedIncrement(&g_despForzados); if (n <= 5) logf("[siempre_disparo_free] Shoot activado a la fuerza (+0x18) en %p", (void*)h); }
    }
    return g_despShootOrig(p1, p2, p3);
}
bool parchearRanuraClear(Feature& f, size_t off, void* gancho, void** orig, uint64_t esperado) {
    uint8_t* dir = g_exe.base + 0x260FEE8 + off; uint64_t act = 0;
    if (!leerSeguro(dir, &act, 8) || act != reinterpret_cast<uint64_t>(g_exe.base) + esperado) { logf("[%s] ranura Clear+0x%llX inesperada", f.id, (unsigned long long)off); return false; }
    *orig = reinterpret_cast<void*>(act);
    Parche p; p.dir = dir; p.original.assign(dir, dir + 8);
    uint64_t g = reinterpret_cast<uint64_t>(gancho); p.nuevo.assign(reinterpret_cast<uint8_t*>(&g), reinterpret_cast<uint8_t*>(&g) + 8);
    if (!aplicarParche(p, f.id)) return false;
    f.parches.push_back(p); return true;
}
// 0.4.31: registro de las ranuras +0x60/+0x68/+0x70/+0x78 de ThinkUnitShoot (vftable exe+0x260FB58) durante el entrenamiento:
// qué ranura se llama (pulsar / mantener / soltar), con el estado interno (+0x1C), el tiempo de carga (+0x20) y lo que devuelve.
using FnRanura = uint64_t (*)(uint64_t, uint64_t);
FnRanura g_shootOrig[4] = {};
volatile LONG g_shootLog = 0;
static uint64_t shootRanura(int i, uint64_t p1, uint64_t p2) {
    uint64_t r = g_shootOrig[i](p1, p2);
    if (trainingReciente() && g_shootLog < 120) {
        uint32_t est = 0xFFFFFFFF; float t = 0; leerSeguro(reinterpret_cast<void*>(p1 + 0x1C), &est, 4); leerSeguro(reinterpret_cast<void*>(p1 + 0x20), &t, 4);
        InterlockedIncrement(&g_shootLog);
        logf("[SHOOT] ranura +0x%X estado=%u carga=%.3f -> 0x%llX", 0x60 + i * 8, est, t, (unsigned long long)r);
    }
    return r;
}
uint64_t shoot60(uint64_t a, uint64_t b) { return shootRanura(0, a, b); }
uint64_t shoot68(uint64_t a, uint64_t b) { return shootRanura(1, a, b); }
uint64_t shoot70(uint64_t a, uint64_t b) { return shootRanura(2, a, b); }
uint64_t shoot78(uint64_t a, uint64_t b) { return shootRanura(3, a, b); }
bool parchearRanuraShoot(Feature& f) {
    static const uint64_t esp[4] = { 0x8AEED0, 0x8B9AF0, 0xC0B9A0, 0x421F40 };
    void* gan[4] = { (void*)shoot60, (void*)shoot68, (void*)shoot70, (void*)shoot78 };
    for (int i = 0; i < 4; ++i) {
        uint8_t* dir = g_exe.base + 0x260FB58 + 0x60 + i * 8; uint64_t act = 0;
        if (!leerSeguro(dir, &act, 8) || act != reinterpret_cast<uint64_t>(g_exe.base) + esp[i]) { logf("[%s] ranura Shoot+0x%X inesperada", f.id, 0x60 + i * 8); return false; }
        g_shootOrig[i] = reinterpret_cast<FnRanura>(act);
        Parche p; p.dir = dir; p.original.assign(dir, dir + 8);
        uint64_t g = reinterpret_cast<uint64_t>(gan[i]); p.nuevo.assign(reinterpret_cast<uint8_t*>(&g), reinterpret_cast<uint8_t*>(&g) + 8);
        if (!aplicarParche(p, f.id)) return false;
        f.parches.push_back(p);
    }
    return true;
}
// 0.4.36: gancho de diagnóstico en FUN_1408AB000 (cálculo del medidor de potencia por jugador; 15 B limpios de prólogo).
// Anota el tipo de orden (cmd+0x1768), la carga (cmd+0x10) y el valor del medidor (entrada+4) antes y después; con el json
// «medidor_real»: 1 y el tipo 4 en el entrenamiento, el medidor deja de forzarse a 1.0 y muestra la carga real.
// --- 0.4.37 vigía de escritura (depurador DENTRO del Core). x64dbg adjunto congela PES y no se pudo manejar su ventana; aquí se usan
// los registros de depuración DR0/DR1 (escritura, 4 bytes) sobre cmd+0x10 (carga) y cmd+0x1768 (tipo de orden) del CommandPlayer
// y un manejador de excepciones vectorizado que anota la instrucción que escribe (rip, retorno, valor). No toca código. Json: "vigia_escritura": 1.
volatile LONG g_vigiaPedida = 0, g_vigiaArmada = 0, g_vigiaLog = 0;
uint64_t g_vigiaDir[4] = { 0, 0, 0, 0 };
static const char* kVigiaNombre[4] = { "cmd+0x10", "cmd+0x1768", "pelota+0x70", "pelota+0x78" };
volatile LONG g_vigiaPelotaPedida = 0;
PVOID g_vigiaVeh = nullptr;
LONG CALLBACK vehVigia(PEXCEPTION_POINTERS ep) {
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT* c = ep->ContextRecord;
    if (!(c->Dr6 & 0xF)) return EXCEPTION_CONTINUE_SEARCH;
    int i = (c->Dr6 & 1) ? 0 : (c->Dr6 & 2) ? 1 : (c->Dr6 & 4) ? 2 : 3;
    LONG n = InterlockedIncrement(&g_vigiaLog);
    if (n <= 300) {
        uint32_t v = 0; uint64_t ret = 0, b = reinterpret_cast<uint64_t>(g_exe.base);
        leerSeguro(reinterpret_cast<void*>(g_vigiaDir[i]), &v, 4); leerSeguro(reinterpret_cast<void*>(c->Rsp), &ret, 8);
        logf("[VIGIA] escribe %s = 0x%X · tras exe+0x%llX · ret exe+0x%llX · rax=%llX rcx=%llX rdx=%llX rsi=%llX rdi=%llX r8=%llX · hilo %lu", kVigiaNombre[i], v,
             (unsigned long long)(c->Rip - b), (unsigned long long)(ret - b), (unsigned long long)c->Rax, (unsigned long long)c->Rcx, (unsigned long long)c->Rdx, (unsigned long long)c->Rsi, (unsigned long long)c->Rdi, (unsigned long long)c->R8, GetCurrentThreadId());
    }
    c->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}
DWORD WINAPI hiloArmarVigia(LPVOID) {
    if (!g_vigiaVeh) g_vigiaVeh = AddVectoredExceptionHandler(1, vehVigia);
    DWORD pid = GetCurrentProcessId(), yo = GetCurrentThreadId(); int n = 0;
    HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (sn != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te; te.dwSize = sizeof te;
        for (BOOL ok = Thread32First(sn, &te); ok; ok = Thread32Next(sn, &te)) {
            if (te.th32OwnerProcessID != pid || te.th32ThreadID == yo) continue;
            HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, te.th32ThreadID);
            if (!h) continue;
            if (SuspendThread(h) != (DWORD)-1) {
                CONTEXT c; memset(&c, 0, sizeof c); c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                if (GetThreadContext(h, &c)) {
                    c.Dr0 = g_vigiaDir[0]; c.Dr1 = g_vigiaDir[1]; c.Dr2 = g_vigiaDir[2]; c.Dr3 = g_vigiaDir[3];
                    uint64_t d7 = c.Dr7 & ~((0xFFull) | (0xFFFFull << 16));
                    for (int k = 0; k < 4; ++k) if (g_vigiaDir[k]) d7 |= (1ull << (k * 2)) | (0xDull << (16 + k * 4));   // L(k), RW=01 (escritura), LEN=11 (4 B)
                    c.Dr7 = d7;
                    c.Dr6 = 0;
                    if (SetThreadContext(h, &c)) ++n;
                }
                ResumeThread(h);
            }
            CloseHandle(h);
        }
        CloseHandle(sn);
    }
    logf("[VIGIA] armada (%p %p %p %p) en %d hilos", (void*)g_vigiaDir[0], (void*)g_vigiaDir[1], (void*)g_vigiaDir[2], (void*)g_vigiaDir[3], n);
    return 0;
}
const char* kFirmaMedidor = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 49 8B F9 49 8B D8 48 8B 88 00 04 00 00";
using FnMedidor = void (*)(uint64_t, uint64_t, uint8_t*, uint8_t*);
FnMedidor g_medidorOrig = nullptr; int g_medidorReal = 0;
void ganchoMedidor(uint64_t a1, uint64_t a2, uint8_t* entrada, uint8_t* cmd) {
    float antes = 0; leerSeguro(entrada + 4, &antes, 4);
    g_medidorOrig(a1, a2, entrada, cmd);
    if (!trainingReciente()) return;
    int tipo = -1; float carga = 0, desp = 0;
    leerSeguro(cmd + 0x1768, &tipo, 4); leerSeguro(cmd + 0x10, &carga, 4); leerSeguro(entrada + 4, &desp, 4);
    static int ultTipo = -2; static float ultDesp = -1; static LONG n = 0;
    if ((tipo != ultTipo || fabsf(desp - ultDesp) > 0.05f) && InterlockedIncrement(&n) <= 120) {
        logf("[MEDIDOR] cmd=%p entrada=%p tipo=%d carga(+0x10)=%.3f medidor antes=%.3f despues=%.3f", (void*)cmd, (void*)entrada, tipo, carga, antes, desp);
        ultTipo = tipo; ultDesp = desp;
    }
    if (g_vigiaPedida && tipo == 4 && InterlockedCompareExchange(&g_vigiaArmada, 1, 0) == 0) {
        g_vigiaDir[0] = reinterpret_cast<uint64_t>(cmd) + 0x10; g_vigiaDir[1] = reinterpret_cast<uint64_t>(cmd) + 0x1768;
        CreateThread(nullptr, 0, hiloArmarVigia, nullptr, 0, nullptr);
    }
    if (g_medidorReal && tipo == 4 && desp >= 0.999f && carga >= 0.0f && carga <= 1.0f) { memcpy(entrada + 4, &carga, 4); }
}
bool medidorInstalar(Feature& f) {
    uint8_t* m = firmaUnica("codigo", kFirmaMedidor, f.id);
    if (!m) { logf("[%s] sin firma del medidor FUN_1408AB000", f.id); return false; }
    if (!crearDesvio(f, m, 15, (void*)ganchoMedidor, (void**)&g_medidorOrig)) { logf("[%s] no se pudo desviar el medidor", f.id); return false; }
    logf("[%s] medidor FUN_1408AB000 enganchado en exe+0x%llX", f.id, (unsigned long long)(m - g_exe.base)); return true;
}
bool disparoInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    uint8_t* m = firmaUnica("codigo", kFirmaPredTiro, f.id);
    if (!m) { f.estado = "sin firma: predicado de tiro FUN_1406b45d0"; return false; }
    if (!crearDesvio(f, m, 20, (void*)ganchoPredTiro, (void**)&g_predTiroOrig)) { f.estado = "no se pudo desviar el predicado de tiro"; return false; }
    bool clearOk = (g_disparoModo == 0 || g_disparoModo >= 8) ? true : parchearRanuraClear(f, 0x60, (void*)ganchoClear60, (void**)&g_clear60Orig, 0x8AEEE0) &&
                   parchearRanuraClear(f, 0x68, (void*)ganchoClear68, (void**)&g_clear68Orig, 0x8AEF70);
    if (g_disparoModo >= 8) {
        bool a = localizarParche("41 83 BE 6C 11 00 00 01 75 22 0F BE 80 54 02 00 00", 8, 0x75, 0xEB, g_pdA, f.id);
        bool b = localizarParche("76 34 8B 87 6C 11 00 00 83 F8 01 74 19 83 F8 03 75 24", 0, 0x76, 0xEB, g_pdB, f.id);
        logf("[%s] modo 8: parche A (Shoot siempre aplicable) %s · parche B (Clear nunca aplicable) %s · se aplican solo con TrainingWork", f.id, a ? "localizado" : "NO", b ? "localizado" : "NO");
    }
    if (g_disparoModo >= 6 && g_disparoModo < 8) {
        uint8_t* dir = g_exe.base + 0x260FB58 + 0x40; uint64_t act = 0;
        if (leerSeguro(dir, &act, 8) && act == reinterpret_cast<uint64_t>(g_exe.base) + 0x8AC500) {
            g_despShootOrig = reinterpret_cast<FnDesp>(act);
            Parche p; p.dir = dir; p.original.assign(dir, dir + 8);
            uint64_t g = reinterpret_cast<uint64_t>(ganchoDespShoot); p.nuevo.assign(reinterpret_cast<uint8_t*>(&g), reinterpret_cast<uint8_t*>(&g) + 8);
            bool ok = aplicarParche(p, f.id); if (ok) f.parches.push_back(p);
            logf("[%s] Shoot+0x40 (despachador) forzado a activo: %s", f.id, ok ? "parcheada" : "NO parcheada");
        } else logf("[%s] ranura Shoot+0x40 inesperada", f.id);
    }
    if (g_disparoModo >= 4 && g_disparoModo < 8 && clearOk) { bool ok70 = parchearRanuraClear(f, 0x70, (void*)ganchoClear70, (void**)&g_clear70Orig, 0x421F40); logf("[%s] Clear+0x70 al soltar: %s", f.id, ok70 ? "parcheada" : "NO parcheada"); }
    logf("[%s] modo %d · ranuras de ThinkUnitClear: %s", f.id, g_disparoModo, g_disparoModo == 0 ? "sin tocar" : clearOk ? "parcheadas" : "NO parcheadas");
    if (g_disparoModo >= 7) medidorInstalar(f);
    logf("[%s] ranuras de ThinkUnitShoot (registro): %s", f.id, parchearRanuraShoot(f) ? "parcheadas" : "NO parcheadas");
    g_disparoOn = 1;
    g_hiloDisparo = CreateThread(nullptr, 0, hiloDisparo, nullptr, 0, nullptr);
    char b[140]; sprintf_s(b, "ok: en el entrenamiento cuadrado dispara siempre (predicado en exe+0x%llX)", (unsigned long long)(m - g_exe.base));
    f.estado = b; return true;
}
bool disparoQuitar(Feature&) { g_disparoOn = 0; ponerParche(g_pdA, false); ponerParche(g_pdB, false); return true; }

// --- función: jugadores_sonda (0.4.38, SOLO LECTURA). El registro de movimiento de cada jugador i (0..21; accesor FUN_140A48A10) es
// *(raíz + (i+0xBF)*0x28); su posición lógica (x, y, z) está en +0x55C/+0x560/+0x564 (la usa FUN_1406B45D0). Cada 3 s (con TrainingWork)
// anota las 22 posiciones y cuántos están fuera del rectángulo de la cancha chica (±semilargo+0.5, ±semiancho+0.5). Json: "jugadores_sonda": true.
volatile LONG g_jugSondaOn = 0; HANDLE g_hiloJug = nullptr; volatile LONG g_ultCursor = -1;
DWORD WINAPI hiloJug(LPVOID) {
    int tickJ = 0; ULONGLONG tPt = 0;
    while (g_jugSondaOn) {
        Sleep(250);
        if (trainingWorkEstable() && GetTickCount64() - tPt > 1000) {     // 0.4.66: cada 1 s, si el portero propio (slot 0) está a menos de 10 m de su línea, anota dónde está y a quién controla el mando
            uint8_t* raiz0 = nullptr; uint8_t* rec0 = nullptr; float p0[3] = { 0, 0, 0 };
            if (leerSeguro(g_globalPartido, &raiz0, 8) && raiz0 && leerSeguro(raiz0 + (size_t)0xBF * 0x28, &rec0, 8) && rec0 && leerSeguro(rec0 + 0x55C, p0, 12) && fabsf(p0[0]) < g_canchaLargo - 5.0f) {
                tPt = GetTickCount64(); static LONG nPt = 0; LONG k = InterlockedIncrement(&nPt);
                if (k <= 400) logf("[PT_PROPIO] portero en (%.1f,%.1f) lejos de su línea; cursor=%ld", p0[0], p0[2], (long)g_ultCursor);
            }
        }
        if ((++tickJ % 12) != 0) continue;
        if (g_vigiaPelotaPedida && g_pelotaSeguida && InterlockedCompareExchange(&g_vigiaPelotaArmada, 1, 0) == 0) {
            g_vigiaDir[2] = reinterpret_cast<uint64_t>(g_pelotaSeguida) + 0x70; g_vigiaDir[3] = reinterpret_cast<uint64_t>(g_pelotaSeguida) + 0x78;
            CreateThread(nullptr, 0, hiloArmarVigia, nullptr, 0, nullptr);
        }
        if (!trainingWorkEstable()) continue;
        uint8_t* raiz = nullptr; if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz) continue;
        char l1[600], l2[600]; int n1 = 0, n2 = 0, fuera = 0;
        for (int i = 0; i < 22; ++i) {
            uint8_t* rec = nullptr; float p[3] = { 0, 0, 0 };
            if (!leerSeguro(raiz + (size_t)(i + 0xBF) * 0x28, &rec, 8) || !rec || !leerSeguro(rec + 0x55C, p, 12)) continue;
            bool fu = fabsf(p[0]) > g_canchaLargo + 0.5f || fabsf(p[2]) > g_canchaAncho + 0.5f; if (fu) ++fuera;
            char* b = i < 11 ? l1 : l2; int& n = i < 11 ? n1 : n2;
            n += sprintf_s(b + n, 600 - n, " %d(%.1f,%.1f)%s", i, p[0], p[2], fu ? "*" : "");
        }
        logf("[JUG] fuera=%d  L:%s", fuera, l1); logf("[JUG]           V:%s", l2);
    }
    return 0;
}
bool jugSondaInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    g_jugSondaOn = 1; g_hiloJug = CreateThread(nullptr, 0, hiloJug, nullptr, 0, nullptr);
    if (!g_hiloJug) { g_jugSondaOn = 0; f.estado = "no se pudo crear el hilo"; return false; }
    f.estado = "ok: anota las posiciones de los 22 jugadores cada 3 s (solo lectura)"; return true;
}
bool jugSondaQuitar(Feature&) { g_jugSondaOn = 0; if (g_hiloJug) { WaitForSingleObject(g_hiloJug, 4000); CloseHandle(g_hiloJug); g_hiloJug = nullptr; } return true; }
// --- función: red_frena_free (0.4.41, de la RE del frente B, 27-oct). La pelota del partido NO es un cuerpo rígido: la mueve un modelo analítico propio
// FUN_140876360 (RVA 0x876360; gravedad 9,80665, arrastre, Magnus, rebote y rodadura) sobre un estado S de 272 B: S[0..2] = posición (x largo, y, z ancho),
// S[3..5] = velocidad lineal, S[6..8] = giro. Se llama en cada paso, también en la predicción de 250 pasos por fotograma. No hay colisión con .geom: por eso
// la pelota atraviesa la red. Gancho (15 B de prólogo, firma AOB de 22 B única): llama al original y, si el estado cae dentro de la red (|x| > semilargo y
// |z| < media portería + margen) con TrainingWork visto, multiplica S[3..5] por un factor por paso (json «red_factor», 0.85) y frena en seco al fondo de la red.
// Firma: (float dt /*xmm0*/, u64 rdx, u64 r8, float* S /*r9*/, void* cfg /*[rsp+0x28]*/, u64 flag /*[rsp+0x30]*/).
const char* kFirmaBolaPaso = "48 8B C4 F3 0F 11 40 08 55 53 56 48 8D 68 88 48 81 EC 60 01 00 00";
using FnBolaPaso = uint64_t (*)(float, uint64_t, uint64_t, float*, void*, uint64_t);
FnBolaPaso g_bolaPasoOrig = nullptr;
bool g_redResorte = false; float g_redK = 70.0f, g_redC = 8.0f, g_redHolgura = 0.4f;   // 0.4.72 «red_resorte»: la red como resorte-amortiguador (json red_resorte/red_k/red_c/red_holgura)
float g_redFactor = 0.93f; volatile ULONGLONG g_tTrainRed = 0; volatile LONG g_redOn = 0; volatile LONG g_redFrenos = 0;
HANDLE g_hiloRed = nullptr;
DWORD WINAPI hiloRed(LPVOID) {
    while (g_redOn) { if (trainingWorkEstable() || hayTrainingWork()) g_tTrainRed = GetTickCount64(); Sleep(100); }
    return 0;
}
// --- malla_red_free (0.4.46, RE del frente E): la red es una simulación de tela (cobra::rd::SceneGoalnetNode); la deforman golpes de esfera que el juego encola
// con FUN_141EAE180 GoalNet_Hit(int lado, int tipo, float* pos3 (alineado a 16), float radio, char flag) y activa con FUN_141EAE330 GoalNet_SetState(int lado, int estado: 1 activa, 2 duerme).
// El motor no los dispara en ±15 (su sonda de «pelota dentro de la red» usa las dimensiones originales). Desde el gancho de la pelota: con la pelota dentro de la red
// street, cada ~14 ms se llama SetState(lado,1) y Hit(lado, tipo 1/2, posición de mundo de la pelota, radio 0,10868593, flag 1). lado = (x >= 0).
using FnGoalNetHit = void (*)(int, int, float*, float, char);
using FnGoalNetEstado = void (*)(int, int);
FnGoalNetHit g_goalNetHit = nullptr; FnGoalNetEstado g_goalNetEstado = nullptr;
volatile LONG g_mallaOn = 0, g_mallaGolpes = 0; volatile ULONGLONG g_mallaUlt = 0;
volatile LONG g_murosOn = 0, g_murosRebotes = 0;
float g_muroRebote = 0.6f, g_muroFric = 0.92f; double g_muroX = 0, g_muroZ = 0;   // 0 = semilargo/semiancho + 0.2
volatile ULONGLONG g_bolaCentroHasta = 0;   // 0.4.63: mientras dure, la pelota se mantiene en el centro (pausa del medio tiempo / final)
uint64_t ganchoBolaPaso(float dt, uint64_t rdx, uint64_t r8, float* S, void* cfg, uint64_t flag) {
    uint64_t r = g_bolaPasoOrig(dt, rdx, r8, S, cfg, flag);
    if (g_bolaCentroHasta && GetTickCount64() < g_bolaCentroHasta) { S[0] = 0; S[1] = 0.11f; S[2] = 0; S[3] = S[4] = S[5] = 0; }
    if ((g_redOn || g_murosOn) && GetTickCount64() - g_tTrainRed < 3000) {
        float x = S[0], y = S[1], z = S[2];
        float largo = (float)g_canchaLargo, ancho = (float)g_canchaAncho;
        bool bocaGol = fabsf(z) < 3.66f + 0.109f && y < 2.44f + 0.109f;   // boca real: media portería 3,66 y travesaño 2,44 (+ radio de la pelota)
        if (g_redOn && g_redResorte && fabsf(x) > largo && bocaGol) {   // 0.4.72: la red cede como una malla: holgura sin fuerza, luego resorte + amortiguación; rebota un poco y la pelota cae dentro
            float sgx = x >= 0 ? 1.0f : -1.0f, pen = fabsf(x) - largo, vout = S[3] * sgx;
            if (pen > g_redHolgura) { float a = -g_redK * (pen - g_redHolgura) - g_redC * vout; S[3] += a * sgx * dt; }
            float arr = 1.0f - 1.2f * dt; if (arr < 0.5f) arr = 0.5f; S[3] *= (pen > g_redHolgura ? 1.0f : (1.0f - 0.8f * dt)); S[4] *= (y > 0.2f ? 1.0f : 1.0f); S[5] *= arr;   // arrastre lateral de la tela
            LONG n = InterlockedIncrement(&g_redFrenos); if (n <= 5 || n % 2000 == 0) logf("[RED] resorte #%ld x=%.2f pen=%.2f vout=%.1f", (long)n, x, pen, vout);
        } else
        if (g_redOn && fabsf(x) > largo && bocaGol) {       // red: freno suave y, al fondo, un pequeño rebote hacia atrás
            float f = g_redFactor;
            S[3] *= f; S[4] *= f; S[5] *= f;
            if (fabsf(x) > largo + 3.0f && S[3] * x > 0) S[3] = -0.2f * S[3];
            LONG n = InterlockedIncrement(&g_redFrenos); if (n <= 5 || n % 2000 == 0) logf("[RED] freno #%ld x=%.2f z=%.2f f=%.2f", (long)n, x, z, f);
        }
        if (g_mallaOn && g_goalNetHit && g_goalNetEstado && fabsf(x) > largo && bocaGol) {   // malla: golpe de tela cada ~14 ms
            ULONGLONG ahora = GetTickCount64();
            if (ahora - g_mallaUlt >= 14) {
                g_mallaUlt = ahora; int lado = x >= 0 ? 1 : 0; float v = sqrtf(S[3] * S[3] + S[5] * S[5]);
                alignas(16) float pos[4] = { x, y, z, 0.0f };
                g_goalNetEstado(lado, 1); g_goalNetHit(lado, v > 8.0f ? 2 : 1, pos, 0.10868593f, 1);
                LONG n = InterlockedIncrement(&g_mallaGolpes); if (n <= 5 || n % 300 == 0) logf("[MALLA] golpe #%ld lado=%d pos=(%.2f,%.2f,%.2f) v=%.1f", (long)n, lado, x, y, z, v);
            }
        }
        if (g_murosOn) {                                       // muros lógicos del Core (el estado S es el del modelo: persiste)
            const float rb = 0.109f;
            float Lx = (float)(g_muroX > 0 ? g_muroX : g_canchaLargo + 0.2), Wz = (float)(g_muroZ > 0 ? g_muroZ : g_canchaAncho + 0.2);
            float lim = bocaGol ? largo + 3.05f : Lx;   // 0.4.73: fondo real de la red dibujada = línea de gol + 3,0 m (goal_l_hit.geom: 55,445-55,505 sobre 52,5); antes 3,4 = «malla invisible» 0,4 m más allá           // en la boca del arco la pared está al fondo de la red
            bool toco = false;
            if (x > lim - rb && S[3] > 0) { S[0] = lim - rb; S[3] = -g_muroRebote * S[3]; S[5] *= g_muroFric; toco = true; }
            else if (x < -lim + rb && S[3] < 0) { S[0] = -lim + rb; S[3] = -g_muroRebote * S[3]; S[5] *= g_muroFric; toco = true; }
            if (z > Wz - rb && S[5] > 0) { S[2] = Wz - rb; S[5] = -g_muroRebote * S[5]; S[3] *= g_muroFric; toco = true; }
            else if (z < -Wz + rb && S[5] < 0) { S[2] = -Wz + rb; S[5] = -g_muroRebote * S[5]; S[3] *= g_muroFric; toco = true; }
            // postes (cilindros verticales en (±largo, ±3,66), radio 0,06) y travesaño (cilindro horizontal en y=2,44): rebote contra la normal
            {
                const float rp = 0.06f + rb; const float px = (x >= 0 ? largo : -largo);
                for (int sg = -1; sg <= 1; sg += 2) {
                    float dx = x - px, dz = z - sg * 3.66f, d2 = dx * dx + dz * dz;
                    if (y < 2.44f && d2 < rp * rp && d2 > 1e-6f) {               // poste
                        float d = sqrtf(d2), nx = dx / d, nz = dz / d, vn = S[3] * nx + S[5] * nz;
                        if (vn < 0) { S[3] -= (1 + 0.5f) * vn * nx; S[5] -= (1 + 0.5f) * vn * nz; S[0] = px + nx * rp; S[2] = sg * 3.66f + nz * rp; toco = true; }
                    }
                }
                if (fabsf(z) < 3.66f) {                                         // travesaño
                    float dx = x - px, dy = y - 2.44f, d2 = dx * dx + dy * dy;
                    if (d2 < rp * rp && d2 > 1e-6f) {
                        float d = sqrtf(d2), nx = dx / d, ny = dy / d, vn = S[3] * nx + S[4] * ny;
                        if (vn < 0) { S[3] -= (1 + 0.5f) * vn * nx; S[4] -= (1 + 0.5f) * vn * ny; S[0] = px + nx * rp; S[1] = 2.44f + ny * rp; toco = true; }
                    }
                }
            }
            if (toco) { LONG n = InterlockedIncrement(&g_murosRebotes); if (n <= 10 || n % 500 == 0) logf("[MURO] rebote #%ld pos=(%.2f,%.2f,%.2f)", (long)n, S[0], S[1], S[2]); }
        }
    }
    return r;
}
static bool asegurarGanchoBola(Feature& f) {
    if (g_bolaPasoOrig) { if (!g_hiloRed) { g_hiloRed = CreateThread(nullptr, 0, hiloRed, nullptr, 0, nullptr); } return true; }
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    uint8_t* m = firmaUnica("codigo", kFirmaBolaPaso, f.id);
    if (!m) { f.estado = "sin firma: paso de la pelota FUN_140876360"; return false; }
    if (!crearDesvio(f, m, 15, (void*)ganchoBolaPaso, (void**)&g_bolaPasoOrig)) { f.estado = "no se pudo desviar el paso de la pelota"; return false; }
    g_hiloRed = CreateThread(nullptr, 0, hiloRed, nullptr, 0, nullptr);
    logf("[%s] paso de la pelota enganchado en exe+0x%llX", f.id, (unsigned long long)(m - g_exe.base)); return true;
}
bool mallaInstalar(Feature& f) {
    g_redOn = 1;
    if (!g_goalNetHit) { uint8_t* m = firmaUnica("codigo", "40 57 48 83 EC 50 48 C7 44 24 30 FE FF FF FF 48 89 5C 24 60 48 89 6C 24 68 48 89 74 24 70 0F 29 74 24 40 0F 28 F3 49 8B F0 8B EA 48 63 D9", f.id); if (!m) { f.estado = "sin firma: GoalNet_Hit FUN_141EAE180"; return false; } g_goalNetHit = reinterpret_cast<FnGoalNetHit>(m); }
    if (!g_goalNetEstado) { uint8_t* m = firmaUnica("codigo", "40 57 48 83 EC 30 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 40 48 89 74 24 48 8B FA 48 63 D9 48 8D 34 5B 48 C1 E6 04", f.id); if (!m) { f.estado = "sin firma: GoalNet_SetState FUN_141EAE330"; return false; } g_goalNetEstado = reinterpret_cast<FnGoalNetEstado>(m); }
    if (!asegurarGanchoBola(f)) return false;
    g_mallaOn = 1; f.estado = "ok: la pelota en la red golpea la tela del arco (GoalNet_Hit desde el paso de la pelota)"; return true;
}
bool mallaQuitar(Feature&) { g_mallaOn = 0; return true; }
bool murosInstalar(Feature& f) {
    g_redOn = 1;   // el hilo de TrainingWork (g_tTrainRed) lo comparten red y muros
    if (!asegurarGanchoBola(f)) return false;
    g_murosOn = 1; char b[160]; sprintf_s(b, "ok: muros lógicos en el paso de la pelota (rebote %.2f, fricción %.2f)", g_muroRebote, g_muroFric); f.estado = b; return true;
}
bool murosQuitar(Feature&) { g_murosOn = 0; return true; }
bool redInstalar(Feature& f) {
    g_redOn = 1;
    if (!asegurarGanchoBola(f)) { g_redOn = 0; return false; }
    char b[150]; sprintf_s(b, "ok: la red frena la pelota en el entrenamiento (factor %.2f)", g_redFactor); f.estado = b; return true;
}
bool redQuitar(Feature&) { g_redOn = 0; return true; }

// --- funciones jugadores_chica_free y suplentes_ocultos_free (0.4.43, de la RE del frente C, 27-oct).
// La posición de reinicio de cada jugador sale de una tabla de Vec3 en metros absolutos de la cancha grande: team + 0x4B4C + (idx%11)*12, con
// team = *(raíz + (side+0xA1)*0x28) y side = (idx>10 && idx<22). La lee FUN_1409F5530(out, uint* pIdx, ctx, teamAI) (RVA 0x9F5530, 14 B de prólogo limpios).
// Gancho: con TrainingWork, escala x,z de la entrada por kx = dims[0]/52.56 y kz = dims[1]/34.06 (dims = *(float**)(raíz+0x2AA8), ya reducidas por
// cancha_chica_free), llama al original y restaura la entrada. Además FUN_140A597D0(out, idx) (RVA 0xA597D0) da la fila de banda de los que no participan
// del ejercicio (suplentes): tras el original los manda a (0, 0, 100), el «aparcamiento» que usa el propio juego.
volatile ULONGLONG g_tTrainGlob = 0; volatile LONG g_trainGlobOn = 0; HANDLE g_hiloTrainGlob = nullptr;
struct ParcheN { uint8_t* dir = nullptr; uint8_t ori[8] = {}, nue[8] = {}; size_t n = 0; bool puesto = false; };
static void ponerParcheN(ParcheN& pn, bool si) { if (!pn.dir || pn.puesto == si) return; if (escribirCodigo(pn.dir, si ? pn.nue : pn.ori, pn.n)) pn.puesto = si; }
volatile LONG g_sinSaquesOn = 0, g_arqueroOn = 0, g_iaClearOn = 0, g_gkCortoOn = 0, g_arqCursorOn = 0; ParcheDisp g_pdS1, g_pdS2, g_pdIA, g_pdGK1, g_pdGK2, g_pdMN, g_pdMN2; ParcheN g_pnA, g_pnB, g_pnC1, g_pnC2, g_pnC3, g_pnMA1, g_pnMA2; volatile LONG g_hudFuerzaOn = 0; ParcheDisp g_pdT1; volatile LONG g_sinFinOn = 0; ParcheDisp g_pdR1, g_pdR1b; volatile LONG g_hudP1On = 0, g_relojR1On = 0, g_relojPausa = 0; ParcheDisp g_pdKR; volatile LONG g_krOn = 0;   // 0.4.77: los parches de HUD (P1/P2) y del reloj (R1/R1b) solo con TrainingWork
DWORD WINAPI hiloTrainGlob(LPVOID) {
    while (g_trainGlobOn) {
        if (trainingWorkEstable() || hayTrainingWork()) g_tTrainGlob = GetTickCount64();
        if (g_sinSaquesOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParche(g_pdS1, dentro); ponerParche(g_pdS2, dentro); }
        if (g_gkCortoOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParche(g_pdGK1, dentro); ponerParche(g_pdGK2, dentro); }
        if (g_arqCursorOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParcheN(g_pnC1, dentro); ponerParcheN(g_pnC2, dentro); ponerParcheN(g_pnC3, dentro); }
        if (g_krOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParche(g_pdKR, dentro); }
        if (g_hudP1On) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParche(g_pdMN, dentro); ponerParche(g_pdMN2, dentro); }
        if (g_relojR1On) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000 && !g_relojPausa; ponerParche(g_pdR1, dentro); ponerParche(g_pdR1b, dentro); }
        if (g_sinFinOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParche(g_pdT1, dentro); }
        if (g_hudFuerzaOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParcheN(g_pnMA1, dentro); ponerParcheN(g_pnMA2, dentro); }
        if (g_iaClearOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParche(g_pdIA, dentro); }
        if (g_arqueroOn) { bool dentro = GetTickCount64() - g_tTrainGlob < 3000; ponerParcheN(g_pnA, dentro); ponerParcheN(g_pnB, dentro); }
        Sleep(100);
    }
    return 0;
}
static void asegurarHiloTrain() { if (!g_trainGlobOn) { g_trainGlobOn = 1; g_hiloTrainGlob = CreateThread(nullptr, 0, hiloTrainGlob, nullptr, 0, nullptr); } }
// --- sin_saques_free (0.4.47, RE del frente G, 27-oct): regla street = juego continuo, solo el gol reinicia. El Juez (match::ai::Judge, FUN_14050E4A0) pone job+0xCC:
// 1 gol, 2 fondo fuera del arco (puerta/córner), 3 banda. Dos parches de 1 byte (solo con TrainingWork; se restauran al salir) lo anulan: P1 0x50E65F «02»->«00», P2 0x50E68D «03»->«00».
// El gol (cc=1) y su detector independiente FUN_140A18510 no se tocan.
bool sinSaquesInstalar(Feature& f) {
    bool a = localizarParche("C7 87 CC 00 00 00 01 00 00 00 EB 38 C7 87 CC 00 00 00 02 00 00 00 EB 2C 48 8B CB E8 ?? ?? ?? ?? F3 0F 10 40 08", 18, 0x02, 0x00, g_pdS1, f.id);
    bool b = localizarParche("41 0F 2F 47 04 76 0A C7 87 CC 00 00 00 03 00 00 00 4C 8B 64 24 40 48 8B 5C 24 48", 13, 0x03, 0x00, g_pdS2, f.id);
    if (!a || !b) { f.estado = "sin firma/byte inesperado en el Juez (FUN_14050E4A0)"; return false; }
    asegurarHiloTrain(); g_sinSaquesOn = 1;
    f.estado = "ok: en el entrenamiento no hay saques (puerta/córner/banda anulados en el Juez); solo el gol reinicia"; return true;
}
bool sinSaquesQuitar(Feature&) { g_sinSaquesOn = 0; ponerParche(g_pdS1, false); ponerParche(g_pdS2, false); return true; }

// --- arquero_propio_free (0.4.49, RE del frente F, 27-oct): quién participa lo decide FUN_140A59CA0 (RVA 0xA59CA0): cada jugador tiene una prioridad en una de 3 tablas de 22 enteros
// en .rdata (T_ATAQUE 0x142644F10, T_DEFENSA 0x142644F80, T_ATAQUE_PORTEROS 0x142644FF0) y participa si prioridad <= N del lado (Jugadores usuario / COM). El GK vale 11 en T_ATAQUE (por eso solo
// entra con 11 jugadores) y 1 en T_ATAQUE_PORTEROS (que se elige con «Cursor de PT fijo = Sí»). P-A: el cmovne de la selección (0x140A59DD5, 4 B) se anula: Ataque usa siempre T_ATAQUE_PORTEROS (GK de ambos
// lados dentro con cualquier N ≥ 1, sin fijar el cursor). P-B: en Defensa el GK rival (dato 0x142644FAC = 11) pasa a 1. Solo con TrainingWork; se restauran al salir.
bool arqueroInstalar(Feature& f) {
    uint8_t* m = firmaUnica("codigo", "80 7F 18 01 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 0F 45 CA", f.id);
    bool a = false, b = false;
    if (m) { uint8_t v[4]; if (leerSeguro(m + 18, v, 4) && v[0] == 0x48 && v[1] == 0x0F && v[2] == 0x45 && v[3] == 0xCA) { g_pnA.dir = m + 18; g_pnA.n = 4; memcpy(g_pnA.ori, v, 4); memset(g_pnA.nue, 0x90, 4); a = true; } }
    uint8_t* d = g_exe.base + 0x2644FAC; uint32_t dv = 0;
    if (leerSeguro(d, &dv, 4) && dv == 0x0B) { g_pnB.dir = d; g_pnB.n = 4; memcpy(g_pnB.ori, &dv, 4); uint32_t nv = 1; memcpy(g_pnB.nue, &nv, 4); b = true; }
    logf("[%s] parche A (Ataque siempre con porteros) %s · parche B (GK rival en Defensa) %s", f.id, a ? "localizado" : "NO", b ? "localizado" : "NO (valor inesperado)");
    if (!a) { f.estado = "sin firma: selección de tabla de participantes FUN_140A59CA0"; return false; }
    asegurarHiloTrain(); g_arqueroOn = 1;
    f.estado = "ok: el portero participa siempre en el entrenamiento (parches en FUN_140A59CA0)"; return true;
}
bool arqueroQuitar(Feature&) { g_arqueroOn = 0; ponerParcheN(g_pnA, false); ponerParcheN(g_pnB, false); return true; }

// --- sin_despeje_emergencia (0.4.50, RE del frente I, 27-oct): la IA NO usa las pad::ThinkUnit* (solo el mando); va por BallPlayer con match::ai::bp::ThinkUnit*. El despeje automático es
// match::ai::bp::ThinkUnitClear (vtable 0x142beef70), exec FUN_142109EB0 (RVA 0x2109EB0, 59 B): si *(ctx[5]+0x31C1) != 0 (flag de presión + cercanía a la portería, FUN_142119390) escribe la intención tipo 5
// (Clear); si no, no escribe nada (el «no aplicable» normal). Parche de 1 byte, RVA 0x2109EC7, «74»->«EB» (je -> jmp al ret): la IA nunca despeja; sigue pudiendo pasar, regatear, tirar (tipo 3) y fintar.
// No se toca ThinkUnitGKClear (el portero). Solo con TrainingWork; se restaura al salir.
bool iaClearInstalar(Feature& f) {
    if (!localizarParche("40 53 48 83 EC 20 49 8B 40 28 4D 8B C8 48 8B DA 80 B8 C1 31 00 00 00 74 1C", 23, 0x74, 0xEB, g_pdIA, f.id)) { f.estado = "sin firma/byte inesperado en ThinkUnitClear de la IA (FUN_142109EB0)"; return false; }
    asegurarHiloTrain(); g_iaClearOn = 1;
    f.estado = "ok: la IA no hace despejes automáticos en el entrenamiento (parche de 1 byte en ThinkUnitClear de la IA)"; return true;
}
bool iaClearQuitar(Feature&) { g_iaClearOn = 0; ponerParche(g_pdIA, false); return true; }

// --- arquero_saque_corto (0.4.51, RE del frente H4, 27-oct): el portero IA (match::ai::bp::BallPlayer) elige entre unidades id 18 GKClear, 19 GKDribble, 20 GKPassShort, 21 GKPassLong. El pase corto
// (FUN_142132660 selección de receptor) exige umbrales de libertad/presión/distancia de cancha grande y en 30x20 casi nunca se cumple; a los 3 s o con presión entra GKPassLong (patada larga que cruza la cancha y
// marca). Parches (primer byte «48»->«C3», la función vuelve sin escribir intención): GKPassLong FUN_142133120 (RVA 0x2133120) y GKClear FUN_142132B40 (RVA 0x2132B40). Quedan GKPassShort y GKDribble.
// Solo con TrainingWork; se restauran al salir. RIESGO: si no hay receptor el portero podría retener la pelota más de lo normal (sin congelar el juego); el complemento B (hook al selector de receptor) queda pendiente.
bool gkCortoInstalar(Feature& f) {
    bool a = localizarParche("48 89 5C 24 18 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 83 EC 50 49 8B 00 33", 0, 0x48, 0xC3, g_pdGK1, f.id);
    bool b = localizarParche("48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 60 49 8B", 0, 0x48, 0xC3, g_pdGK2, f.id);
    logf("[%s] GKPassLong %s · GKClear %s", f.id, a ? "localizado" : "NO", b ? "localizado" : "NO");
    if (!a && !b) { f.estado = "sin firma de GKPassLong/GKClear de la IA"; return false; }
    if (!a) g_pdGK1.dir = nullptr; if (!b) g_pdGK2.dir = nullptr;
    asegurarHiloTrain(); g_gkCortoOn = 1;
    f.estado = "ok: el portero IA no saca largo en el entrenamiento (GKPassLong/GKClear anulados; quedan pase corto y regate)"; return true;
}
bool gkCortoQuitar(Feature&) { g_gkCortoOn = 0; ponerParche(g_pdGK1, false); ponerParche(g_pdGK2, false); return true; }

// --- arquero_propio_cursor (0.4.52, variante del frente F): con «Cursor de PT fijo = Sí» en el menú, el flag TW+0x18 da el portero en la tabla (a) pero también asigna el mando al GK (b). Esta variante deja (a) y anula (b):
// B1 FUN_141FB9040 (0x141FB9713, JNZ -> JMP: no asigna el mando principal al GK), B2 FUN_14043EC70 (0x14043EE2C, JZ -> JMP: no reasigna cada fotograma), B3 FUN_14087AC70 (0x14087ADB7, JNZ -> JMP: no fuerza +0x10=1).
// Resultado esperado: el GK propio queda como cualquier compañero de la IA y el cursor cambia normal. Solo con TrainingWork; se restauran al salir.
static bool localizarParcheN(ParcheN& pn, const char* firma, size_t desp, std::initializer_list<uint8_t> ori, std::initializer_list<uint8_t> nue, const char* id) {
    uint8_t* m = firmaUnica("codigo", firma, id); if (!m) return false;
    pn.dir = m + desp; pn.n = ori.size(); size_t i = 0; for (uint8_t b : ori) pn.ori[i++] = b; i = 0; for (uint8_t b : nue) pn.nue[i++] = b;
    uint8_t v[8] = {}; if (!leerSeguro(pn.dir, v, pn.n) || memcmp(v, pn.ori, pn.n) != 0) { pn.dir = nullptr; return false; }
    return true;
}
bool arqCursorInstalar(Feature& f) {
    bool a = localizarParcheN(g_pnC1, "41 83 7D 00 01 0F 85 ?? ?? ?? ?? 45 38 65 20 0F 84", 5, { 0x0F, 0x85 }, { 0x90, 0xE9 }, f.id);
    bool b = localizarParcheN(g_pnC2, "48 8B CB E8 ?? ?? ?? ?? 84 C0 0F 84 C5 00 00 00 E8 ?? ?? ?? ?? 33 C9", 10, { 0x0F, 0x84 }, { 0x90, 0xE9 }, f.id);
    bool c = localizarParcheN(g_pnC3, "48 8B CE E8 ?? ?? ?? ?? 3C 01 75 09 C7 43 10 01 00 00 00", 10, { 0x75 }, { 0xEB }, f.id);
    logf("[%s] B1 %s · B2 %s · B3 %s", f.id, a ? "localizado" : "NO", b ? "localizado" : "NO", c ? "localizado" : "NO");
    if (!a && !b && !c) { f.estado = "sin firma de los bloqueos de cursor"; return false; }
    asegurarHiloTrain(); g_arqCursorOn = 1;
    f.estado = "ok: con «Cursor de PT fijo = Sí» el portero propio no bloquea el cursor (B1+B2+B3)"; return true;
}
bool arqCursorQuitar(Feature&) { g_arqCursorOn = 0; ponerParcheN(g_pnC1, false); ponerParcheN(g_pnC2, false); ponerParcheN(g_pnC3, false); return true; }

// --- arquero_salida_corta (0.4.53, RE del frente H3, 27-oct): la posición del portero la da ActionKeeperBasePosition ranura 4 = FUN_1409DC040 (RVA 0x9DC040): ulonglong* f(longlong this, ulonglong* out (vec3), longlong, undefined8),
// con umbrales de salida de cancha grande (17–35 m). Gancho (15 B de prólogo limpios, firma de 32 B): tras el original se recorta la salida a un máximo de R metros delante de la línea de gol (|x| en [L−R, L−0,5], R = json
// «arquero_salida_max», 3,5) y |z| <= 5. Solo con TrainingWork.
const char* kFirmaPorteroPos = "48 8B C4 48 89 58 08 4C 89 48 20 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 B0 48";
using FnPorteroPos = uint64_t* (*)(int64_t, uint64_t*, int64_t, uint64_t);
FnPorteroPos g_porteroPosOrig = nullptr; volatile LONG g_arqSalidaOn = 0, g_arqSalidaRecortes = 0; float g_arqSalidaMax = 3.5f;
uint64_t* ganchoPorteroPos(int64_t p1, uint64_t* out, int64_t p3, uint64_t p4) {
    uint64_t* r = g_porteroPosOrig(p1, out, p3, p4);
    {   // 0.4.67 (frente L §9): FUN_1409DC040 elige la rama del portero por team+0x255 (0 = normal FUN_1409DE8C0, !=0 = alternativa FUN_1409DF7D0). Se mide en cada bando (log cada 5 s).
        static ULONGLONG tM = 0; ULONGLONG ah = GetTickCount64();
        if (ah - tM > 5000) {
            tM = ah; uint8_t* raiz = nullptr; uint8_t v[2] = { 0xEE, 0xEE };
            if (leerSeguro(g_globalPartido, &raiz, 8) && raiz) for (int sd = 0; sd < 2; sd++) { uint8_t* team = nullptr; if (leerSeguro(raiz + (size_t)(sd + 0xA1) * 0x28, &team, 8) && team) leerSeguro(team + 0x255, &v[sd], 1); }
            static LONG nm = 0; if (InterlockedIncrement(&nm) <= 120) logf("[PORTERO_RAMA] team+0x255: lado0=%u lado1=%u (0 = rama normal)", v[0], v[1]);
        }
    }
    if (g_arqSalidaOn && out && GetTickCount64() - g_tTrainGlob < 3000) {
        float v[3]; if (leerSeguro(out, v, 12) && fabsf(v[0]) < 60.0f && fabsf(v[2]) < 60.0f && fabsf(v[0]) > 1.0f) {
            float L = (float)g_canchaLargo, ax = fabsf(v[0]), sg = v[0] >= 0 ? 1.0f : -1.0f, nx = ax, nz = v[2];
            if (nx < L - g_arqSalidaMax) nx = L - g_arqSalidaMax;
            if (nx > L - 0.5f) nx = L - 0.5f;
            if (nz > 5.0f) nz = 5.0f; else if (nz < -5.0f) nz = -5.0f;
            if (nx != ax || nz != v[2]) {
                float* o = reinterpret_cast<float*>(out); o[0] = sg * nx; o[2] = nz;
                LONG n = InterlockedIncrement(&g_arqSalidaRecortes); if (n <= 10 || n % 500 == 0) logf("[PORTERO] salida recortada #%ld (%.1f,%.1f) -> (%.1f,%.1f)", (long)n, v[0], v[2], o[0], o[2]);
            }
        }
    }
    return r;
}
bool arqSalidaInstalar(Feature& f) {
    uint8_t* m = firmaUnica("codigo", kFirmaPorteroPos, f.id);
    if (!m) { f.estado = "sin firma: posición del portero FUN_1409DC040"; return false; }
    if (!crearDesvio(f, m, 15, (void*)ganchoPorteroPos, (void**)&g_porteroPosOrig)) { f.estado = "no se pudo desviar la posición del portero"; return false; }
    asegurarHiloTrain(); g_arqSalidaOn = 1;
    char b[150]; sprintf_s(b, "ok: el portero sale como máximo %.1f m de la línea de gol (exe+0x%llX)", g_arqSalidaMax, (unsigned long long)(m - g_exe.base)); f.estado = b; return true;
}
bool arqSalidaQuitar(Feature&) { g_arqSalidaOn = 0; return true; }

// --- reinicio_seguro (0.4.54): red de seguridad contra el cierre «PES2021.exe+0x476E1C, c0000005» (4 cierres el 27-oct cuando el rival marcó). FUN_140476C50 (posición de reinicio del balón) hace
// `mov edx,[rbx+8]; call FUN_140A7CBA0; movzx ecx, byte [rax]; dec ecx; cmp ecx,6; ja default`: la búsqueda devuelve NULL si la clave del escenario ([rbx+8] = job+0x1F0) no es de categoría 3 (p. ej. 0) y la lectura cae.
// Cueva: sustituye `0F B6 08 FF C9` (5 B, en 0x476E1C) por un salto; si rax es NULL anota «[REINICIO] tipo=… nulo, sustituido» y pone ecx = -1 (el `ja` cae en la rama por defecto), si no ejecuta lo original.
static void __stdcall ayudanteReinicioNulo(uint8_t* rbx) {
    uint32_t t = 0xFFFFFFFF; leerSeguro(rbx + 8, &t, 4);
    static LONG n = 0; LONG k = InterlockedIncrement(&n); if (k <= 20) logf("[REINICIO] tipo=0x%X nulo, sustituido por la rama por defecto (#%ld)", t, (long)k);
}
bool reinicioSeguroInstalar(Feature& f) {
    uint8_t* sig = firmaUnica("codigo", "8B 53 08 E8 ?? ?? ?? ?? 0F B6 08 FF C9 83 F9 06 0F 87 AD 01 00 00", f.id);
    if (!sig) { f.estado = "sin firma: FUN_140476C50+0x1CC"; return false; }
    uint8_t* sitio = sig + 8; const uint8_t esp[5] = { 0x0F, 0xB6, 0x08, 0xFF, 0xC9 };
    uint8_t v[5]; if (!leerSeguro(sitio, v, 5) || memcmp(v, esp, 5) != 0) { f.estado = "bytes inesperados en FUN_140476C50"; return false; }
    uint8_t* c = reservarCerca(sitio, 160);
    if (!c) { f.estado = "no hay memoria cerca del exe (reinicio_seguro)"; return false; }
    size_t o = 0; auto b = [&](std::initializer_list<uint8_t> l) { for (uint8_t x : l) c[o++] = x; };
    b({ 0x48, 0x85, 0xC0 });                         // test rax,rax
    size_t jz = o; b({ 0x74, 0x00 });                // jz nulo (rel8 se rellena)
    b({ 0x0F, 0xB6, 0x08, 0xFF, 0xC9 });             // movzx ecx,byte[rax] ; dec ecx
    size_t jb1 = o; b({ 0xE9, 0, 0, 0, 0 });         // jmp back
    size_t nulo = o; c[jz + 1] = (uint8_t)(nulo - (jz + 2));
    b({ 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53 });   // push rax,rcx,rdx,r8,r9,r10,r11
    b({ 0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xE4, 0xF0, 0x48, 0x83, 0xEC, 0x20 }); // push rbp; mov rbp,rsp; and rsp,-16; sub rsp,20h
    b({ 0x48, 0x89, 0xD9 });                         // mov rcx,rbx
    b({ 0x48, 0xB8 }); { uint64_t h = reinterpret_cast<uint64_t>(&ayudanteReinicioNulo); memcpy(c + o, &h, 8); o += 8; }   // mov rax,helper
    b({ 0xFF, 0xD0 });                               // call rax
    b({ 0x48, 0x89, 0xEC, 0x5D });                   // mov rsp,rbp ; pop rbp
    b({ 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58 });   // pop r11,r10,r9,r8,rdx,rcx,rax
    b({ 0xB9, 0xFF, 0xFF, 0xFF, 0xFF });             // mov ecx,-1
    size_t jb2 = o; b({ 0xE9, 0, 0, 0, 0 });         // jmp back
    uint8_t* vuelta = sitio + 5;
    if (!escribirRel32(c + jb1, 0xE9, c + jb1, vuelta) || !escribirRel32(c + jb2, 0xE9, c + jb2, vuelta)) { f.estado = "cueva fuera de alcance (reinicio_seguro)"; return false; }
    FlushInstructionCache(GetCurrentProcess(), c, 160);
    Parche p; p.dir = sitio; p.original.assign(sitio, sitio + 5); p.nuevo.resize(5);
    if (!escribirRel32(p.nuevo.data(), 0xE9, sitio, c)) { f.estado = "cueva fuera de alcance (reinicio_seguro salto)"; return false; }
    if (!aplicarParche(p, f.id)) { f.estado = "no se pudo escribir el salto (reinicio_seguro)"; return false; }
    f.parches.push_back(p);
    char sbuf[140]; sprintf_s(sbuf, "ok: red de seguridad de la posición de reinicio (exe+0x%llX): clave nula -> rama por defecto, sin cierre", (unsigned long long)(sitio - g_exe.base));
    f.estado = sbuf; return true;
}
bool reinicioSeguroQuitar(Feature&) { return true; }   // los parches los quita el marco

// --- marcador_nativo_free (0.4.55, RE del frente J, 27-oct): el marcador y el reloj son la pantalla 2D match2D::Screen::Time (id 7 del gestor de pantallas), construida en FUN_1420CCCD0 y que solo existe
// si el byte gestor+0x18B4 != 0 (FUN_1420CD220 caso 7). Al final de FUN_1420C9080: `clase = FUN_14149F4F0(modo); si clase == 0x0B o 0x0C -> byte = 0`. FREE_TRAINING (modo 52) es clase 0x0B. Parche de 1 byte, RVA 0x20C93D7: «0B» -> «FF»
// (el modo 52 deja de apagar el HUD; el modo 51 sigue siendo clase 0x0C). Se aplica al instalar (el HUD se crea al cargar el partido) y se restaura al salir. Los goles los lee el HUD de los stats de cada equipo (ya cuentan).
bool marcadorNativoInstalar(Feature& f) {
    if (!localizarParche("83 F8 0B 74 12 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 83 F8 0C 75 03 C6 07 00", 2, 0x0B, 0xFF, g_pdMN, f.id)) { f.estado = "sin firma/byte inesperado en FUN_1420C9080 (HUD de marcador)"; return false; }
    // 0.4.58 (J3): pestillo de FUN_1420C9080 (RVA 0x20C90F4, 75->EB): si el HUD se apagó y S+0x11B2==0, no volvía a encenderse; el saque de TrainingWork no pone ese byte.
    bool p2 = localizarParche("44 38 37 75 0F 45 38 B5 B2 11 00 00 45 0F B6 D1 45 0F 44 D6 0F B6 B4 24 B0 00 00 00", 3, 0x75, 0xEB, g_pdMN2, f.id);
    logf("[%s] pestillo del HUD (S+0x11B2) %s", f.id, p2 ? "localizado" : "NO localizado");
    if (!p2) g_pdMN2.dir = nullptr;
    asegurarHiloTrain(); g_hudP1On = 1;   // 0.4.77: se aplican solo con TrainingWork (hiloTrainGlob)
    // 0.4.60 (J3, parche A): el pestillo no bastaba (el HUD volvió a perderse con S+0x11B2==1). En FUN_1420C9080 se fuerza M+0x9C=1 ignorando todas las condiciones que lo apagan (repetición, [raíz+0x388], S+0x177F…):
    // +7 `74 0E`→`EB 0E`; +23 `80 3F 00 75 09`→`C6 07 01 EB 09`. Solo con TrainingWork (hiloTrainGlob), restaurado al salir.
    const char* firmaA = "E8 ?? ?? ?? ?? 84 C0 74 0E 66 C7 07 00 01 C7 47 20 00 00 00 00 EB 29 80 3F 00 75 09 C7 47 20 00 00 00 00 EB 1B E8";
    bool a1 = localizarParcheN(g_pnMA1, firmaA, 7, { 0x74, 0x0E }, { 0xEB, 0x0E }, f.id);
    bool a2 = localizarParcheN(g_pnMA2, firmaA, 23, { 0x80, 0x3F, 0x00, 0x75, 0x09 }, { 0xC6, 0x07, 0x01, 0xEB, 0x09 }, f.id);
    logf("[%s] HUD forzado (parche A) %s", f.id, (a1 && a2) ? "localizado" : "NO localizado");
    if (a1 && a2) { asegurarHiloTrain(); g_hudFuerzaOn = 1; } else { g_pnMA1.dir = nullptr; g_pnMA2.dir = nullptr; }
    f.estado = "ok: el HUD de marcador y reloj nativo ya no se apaga en el entrenamiento (parches en FUN_1420C9080 solo con TrainingWork)";
    return true;
}
bool marcadorNativoQuitar(Feature&) { g_hudFuerzaOn = 0; g_hudP1On = 0; ponerParcheN(g_pnMA1, false); ponerParcheN(g_pnMA2, false); ponerParche(g_pdMN, false); ponerParche(g_pdMN2, false); return true; }

// --- celebracion_chica (0.4.56, RE del frente K, 27-oct): la ancla de la demo de gol/celebración (FUN_140476C50, guardada en [raíz+0x1A8]+0x98) se pasa a FUN_14047C500 (RVA 0x47C500;
// void(int* this, u64 jugador, uint* st, int tipo, int flag), st[0]=id, st[1]=x, st[2]=y, st[3]=z), que arranca la demo; los tipos 4 y 7 de ancla usan metros absolutos del estadio real. Pre-gancho: con TrainingWork, st[1] y st[3] se recortan
// a ±(dims-0,5) (dims = *(float**)(raíz+0x2AA8)). La carrera del goleador (GoalEvent::Actualizar) usa el borde del «área de demo» del estadio (demoarea_stNNN.json, 8 Vec3 en el registro FUN_141E5D4C0); un parche de datos de esos puntos queda
// pendiente si hace falta. NO se toca el id de escena (clave nula en FUN_140476C50).
const char* kFirmaAncla = "40 56 57 41 56 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 60 48 89 6C 24 68 4D 8B F0 48";
using FnAncla = void (*)(int*, uint64_t, uint32_t*, int, int);
FnAncla g_anclaOrig = nullptr; volatile LONG g_celChicaOn = 0, g_celChicaRecortes = 0;
void ganchoAncla(int* self, uint64_t jug, uint32_t* st, int tipo, int flag) {
    if (g_celChicaOn && st && GetTickCount64() - g_tTrainGlob < 3000) {
        uint8_t* raiz = nullptr; float* dims = nullptr; float d[2] = { 0, 0 }, v[4];
        if (leerSeguro(g_globalPartido, &raiz, 8) && raiz && leerSeguro(raiz + 0x2AA8, &dims, 8) && dims && leerSeguro(dims, d, 8) &&
            d[0] > 5.0f && d[0] < 60.0f && d[1] > 3.0f && d[1] < 40.0f && leerSeguro(st, v, 16)) {
            float lx = d[0] - 0.5f, lz = d[1] - 0.5f, nx = v[1], nz = v[3];
            if (nx > lx) nx = lx; else if (nx < -lx) nx = -lx;
            if (nz > lz) nz = lz; else if (nz < -lz) nz = -lz;
            if (nx != v[1] || nz != v[3]) {
                reinterpret_cast<float*>(st)[1] = nx; reinterpret_cast<float*>(st)[3] = nz;
                LONG n = InterlockedIncrement(&g_celChicaRecortes); if (n <= 10) logf("[CELEBRACION] ancla recortada (%.1f,%.1f) -> (%.1f,%.1f) id=0x%X", v[1], v[3], nx, nz, st[0]);
            }
        }
    }
    g_anclaOrig(self, jug, st, tipo, flag);
}
// 0.4.58 (frente K2): parche de datos de los 8 puntos del «área de demo» del estadio (registro de FUN_141E5D4C0, global leída del RIP-rel de esa función; R = [[[global]]+id*8]+8, 8 Vec3 en R+0x38, ancla tipo 4 en R+0x130/0x138).
// Con TrainingWork se escriben x,z = ±(dims-0,5) (y intacta), así el goleador y el resto (arquero rival incluido) no corren hasta el borde del estadio real; al salir del entrenamiento o quitar la opción se restauran los originales.
const char* kFirmaRegDemo = "48 8B 0D ?? ?? ?? ?? 48 85 C9 75 03 33 C0 C3 E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC";
uint8_t* g_regDemoGlobal = nullptr; volatile LONG g_demoAreaOn = 0; HANDLE g_hiloDemoArea = nullptr;
uint8_t* g_regDemoR = nullptr; uint8_t g_regDemoCopia[96 + 12]; bool g_regDemoParcheado = false;
static void regDemoRestaurar() {
    if (g_regDemoParcheado && g_regDemoR) {
        memcpy(g_regDemoR + 0x38, g_regDemoCopia, 96); memcpy(g_regDemoR + 0x130, g_regDemoCopia + 96, 12);
        logf("[CELEBRACION] puntos demoarea restaurados");
    }
    g_regDemoParcheado = false; g_regDemoR = nullptr;
}
DWORD WINAPI hiloDemoArea(LPVOID) {
    while (g_demoAreaOn) {
        Sleep(250);
        if (!trainingWorkEstable()) { regDemoRestaurar(); continue; }
        uint8_t* raiz = nullptr; uint8_t* sel = nullptr; float* dims = nullptr; float d[2] = { 0, 0 }; int32_t id = -1;
        if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz || !leerSeguro(raiz + 0x2C0, &sel, 8) || !sel || !leerSeguro(sel + 0x20, &id, 4) || id < 0 || id >= 512) continue;
        if (!leerSeguro(raiz + 0x2AA8, &dims, 8) || !dims || !leerSeguro(dims, d, 8) || !(d[0] > 5.0f && d[0] < 60.0f && d[1] > 3.0f && d[1] < 40.0f)) continue;
        uint8_t *P = nullptr, *A = nullptr, *E = nullptr, *R = nullptr;
        if (!leerSeguro(g_regDemoGlobal, &P, 8) || !P || !leerSeguro(P, &A, 8) || !A || !leerSeguro(A + (size_t)id * 8, &E, 8) || !E || !leerSeguro(E + 8, &R, 8) || !R) continue;
        uint8_t tmp[96 + 12];
        if (!leerSeguro(R + 0x38, tmp, 96) || !leerSeguro(R + 0x130, tmp + 96, 12)) continue;
        if (R != g_regDemoR) { regDemoRestaurar(); g_regDemoR = R; memcpy(g_regDemoCopia, tmp, 108); }
        const float dx = d[0] - 0.5f, dz = d[1] - 0.5f;
        static const int sx[8] = { -1, -1, -1, -1, 1, 1, 1, 1 }, sz[8] = { -1, -1, 1, 1, 1, 1, -1, -1 };
        float nuevos[24]; memcpy(nuevos, tmp, 96);
        for (int k = 0; k < 8; k++) { nuevos[k * 3 + 0] = sx[k] * dx; nuevos[k * 3 + 2] = sz[k] * dz; }
        float anc[3]; memcpy(anc, tmp + 96, 12);
        if (anc[0] > dx) anc[0] = dx; else if (anc[0] < -dx) anc[0] = -dx;
        if (anc[2] > dz) anc[2] = dz; else if (anc[2] < -dz) anc[2] = -dz;
        if (memcmp(nuevos, tmp, 96) != 0 || memcmp(anc, tmp + 96, 12) != 0) {
            memcpy(R + 0x38, nuevos, 96); memcpy(R + 0x130, anc, 12);
            if (!g_regDemoParcheado) logf("[CELEBRACION] puntos demoarea recortados a ±(%.1f, %.1f) (estadio id=%d, registro %p)", dx, dz, (int)id, (void*)R);
            g_regDemoParcheado = true;
        }
    }
    regDemoRestaurar();
    return 0;
}
bool celChicaInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    uint8_t* m = firmaUnica("codigo", kFirmaAncla, f.id);
    if (!m) { f.estado = "sin firma: arranque de demo FUN_14047C500"; return false; }
    if (!crearDesvio(f, m, 18, (void*)ganchoAncla, (void**)&g_anclaOrig)) { f.estado = "no se pudo desviar el arranque de demo"; return false; }
    asegurarHiloTrain(); g_celChicaOn = 1;
    const char* extra = "";
    uint8_t* r = firmaUnica("codigo", kFirmaRegDemo, f.id);
    if (r) {
        int32_t rel = 0; memcpy(&rel, r + 3, 4); g_regDemoGlobal = r + 7 + rel;
        g_demoAreaOn = 1; g_hiloDemoArea = CreateThread(nullptr, 0, hiloDemoArea, nullptr, 0, nullptr);
        if (!g_hiloDemoArea) { g_demoAreaOn = 0; extra = " (sin hilo de demoarea)"; } else extra = " + puntos demoarea";
    } else extra = " (sin firma del registro demoarea)";
    char b[190]; sprintf_s(b, "ok: la ancla de la celebración se recorta a la cancha chica (exe+0x%llX)%s", (unsigned long long)(m - g_exe.base), extra); f.estado = b; return true;
}
// --- ia_sonda (0.4.76, frente P paso 0, SOLO LECTURA): los datos de IA de campo (constant::DefenceCover id 187, Defence 186, DefenceMark 188, BasePosition 183, Matchup 50, Throughpass 61, Support 197, SpaceRun 195, DiagonalRun 189, PullAway 193)
// viven en el mismo registro que los puntos demoarea: R = [[[P]]+id*8]+8, P = global de FUN_141E5D4C0. Se anotan los floats por offset para conocer los valores típicos de cancha grande antes de escalar nada. NO se escribe nada.
volatile LONG g_iaSondaOn = 0; HANDLE g_hiloIaSonda = nullptr;
DWORD WINAPI hiloIaSonda(LPVOID) {
    struct G { int id; const char* nombre; std::initializer_list<uint32_t> offs; };
    static const G grupos[] = {
        { 187, "DefenceCover", { 0x38, 0x3C, 0x4C, 0x50, 0x58, 0x68, 0x70, 0x74, 0x80, 0x88 } },
        { 186, "Defence", { 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54 } },
        { 188, "DefenceMark", { 0xBC, 0xCC, 0xD0, 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48 } },
        { 183, "BasePosition", { 0xC0, 0x184, 0x21C, 0x278, 0x290, 0x1B4, 0x214, 0x24C } },
        { 50, "Matchup", { 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44 } },
        { 61, "Throughpass", { 0x118, 0x120, 0x188, 0x18C, 0x190, 0x194 } },
        { 197, "Support", { 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C } },
        { 195, "SpaceRun", { 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C } },
        { 189, "DiagonalRun", { 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44 } },
        { 193, "PullAway", { 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44 } },
    };
    int veces = 0; ULONGLONG ultimo = 0;
    while (g_iaSondaOn) {
        Sleep(500);
        if (veces >= 2 || !trainingWorkEstable() || (ultimo && GetTickCount64() - ultimo < 60000) || !g_regDemoGlobal) continue;
        uint8_t* P = nullptr; uint8_t* A = nullptr; if (!leerSeguro(g_regDemoGlobal, &P, 8) || !P || !leerSeguro(P, &A, 8) || !A) continue;
        for (const G& g : grupos) {
            uint8_t* E = nullptr; uint8_t* R = nullptr;
            if (!leerSeguro(A + (size_t)g.id * 8, &E, 8) || !E || !leerSeguro(E + 8, &R, 8) || !R) { logf("[IA_DATOS] %s (id %d): sin registro", g.nombre, g.id); continue; }
            char buf[400]; int n = sprintf_s(buf, "[IA_DATOS] %s (id %d, R=%p):", g.nombre, g.id, (void*)R);
            for (uint32_t o : g.offs) { float v = 0; if (leerSeguro(R + o, &v, 4)) n += sprintf_s(buf + n, sizeof(buf) - n, " +%X=%.3f", o, v); }
            logf("%s", buf);
        }
        ultimo = GetTickCount64(); veces++;
    }
    return 0;
}
bool iaSondaInstalar(Feature& f) {
    if (!g_regDemoGlobal) { uint8_t* r = firmaUnica("codigo", kFirmaRegDemo, f.id); if (!r) { f.estado = "sin firma del registro de constantes (FUN_141E5D4C0)"; return false; } int32_t rel = 0; memcpy(&rel, r + 3, 4); g_regDemoGlobal = r + 7 + rel; }
    g_iaSondaOn = 1; g_hiloIaSonda = CreateThread(nullptr, 0, hiloIaSonda, nullptr, 0, nullptr);
    if (!g_hiloIaSonda) { g_iaSondaOn = 0; f.estado = "no se pudo crear el hilo de la sonda de IA"; return false; }
    f.estado = "ok: anota en el log los datos de IA de campo (DefenceCover, Defence, DefenceMark, BasePosition…) para escalarlos; solo lectura"; return true;
}
bool iaSondaQuitar(Feature&) { g_iaSondaOn = 0; if (g_hiloIaSonda) { WaitForSingleObject(g_hiloIaSonda, 2000); CloseHandle(g_hiloIaSonda); g_hiloIaSonda = nullptr; } return true; }

// --- ia_cancha_chica (0.4.76, frente P): los datos de IA de campo (constant::DefenceCover 187, Defence 186, BasePosition 183 [solo campo], DefenceMark 188, Matchup 50, Support 197, SpaceRun 195, DiagonalRun 189, PullAway 193, Throughpass 61, Passget 53)
// viven en el registro de constantes (R = [[[P]]+id*8]+8, P = global de FUN_141E5D4C0, como los puntos demoarea). Con TrainingWork se escalan las distancias a la minicancha con k = (dims/dims_grande)^e (e = ia_escala 0,5; 0,4 en apoyo/desmarque/pase) y un piso
// ia_piso_m (1,5 m): nuevo = max(orig*k, min(orig, piso)); siempre desde una copia de los originales (idempotente); se restaura al salir del entrenamiento / quitar la opción. NO se tocan ángulos, *Rate, ints, bytes ni los campos del PORTERO (gkl*, gks*, kickoffDfLineX, margin_restartX).
struct CampoIA { uint16_t off; char eje; };
struct GrupoIA { int id; const char* nom; bool apoyo; std::vector<CampoIA> c; uint8_t* R = nullptr; std::vector<float> orig, pat; bool parcheado = false; };
static std::vector<CampoIA> campos(std::initializer_list<uint16_t> d, std::initializer_list<uint16_t> x = {}, std::initializer_list<uint16_t> z = {}) {
    std::vector<CampoIA> v; for (uint16_t o : d) v.push_back({ o, 'd' }); for (uint16_t o : x) v.push_back({ o, 'x' }); for (uint16_t o : z) v.push_back({ o, 'z' }); return v;
}
volatile LONG g_iaChicaOn = 0; HANDLE g_hiloIaChica = nullptr; float g_iaE = 0.5f, g_iaEApoyo = 0.4f, g_iaPiso = 1.5f;
static void restaurarGrupoIA(GrupoIA& g) {
    if (g.parcheado && g.R) { for (size_t i = 0; i < g.c.size(); i++) memcpy(g.R + g.c[i].off, &g.orig[i], 4); logf("[IA_DATOS] %s (id %d): restaurado", g.nom, g.id); }
    g.parcheado = false; g.R = nullptr;
}
DWORD WINAPI hiloIaChica(LPVOID) {
    std::vector<GrupoIA> gr;
    gr.push_back({ 187, "DefenceCover", false, campos({ 0x68, 0x38, 0x3C, 0x88, 0x58, 0x4C, 0x50, 0x10, 0x18 }, { 0x74, 0x70, 0x80 }) });
    gr.push_back({ 186, "Defence", false, campos({ 0x18, 0x1C, 0x20, 0x0C, 0x88, 0x78, 0x3C, 0x4C, 0x6C, 0x70, 0x28, 0x2C }, { 0x64, 0x68, 0x74, 0x40, 0x44, 0x80 }, { 0x84, 0x48 }) });
    gr.push_back({ 183, "BasePosition", false, campos({ 0x21C, 0x220, 0x224, 0x228, 0x278, 0x27C, 0x280, 0x284, 0x28C, 0x22C, 0x230, 0x290, 0x298, 0x29C, 0xB8, 0xBC, 0xC0, 0x4C, 0x184, 0x188, 0x18C, 0x190, 0x31C, 0x320, 0x2EC, 0x2F0, 0x160, 0x164, 0x168, 0x10C, 0x110, 0x114, 0x118, 0x120, 0x128, 0x12C, 0x130, 0x2A4 }, { 0x2B0, 0x2B4, 0x210, 0x248, 0xC4 }, { 0x244, 0xC8 }) });
    gr.push_back({ 188, "DefenceMark", false, campos({ 0x8C, 0x98, 0xA4, 0x90, 0x9C, 0xA8, 0x94, 0xA0, 0xAC, 0xB0, 0xB4, 0xBC, 0xC0, 0xB8, 0xD0, 0xCC, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C, 0x5C, 0xD8, 0x10C, 0x110, 0x114, 0x118, 0x14, 0x18, 0x1C }, { 0xF4, 0xF8, 0x124, 0x128 }, { 0xFC, 0x100, 0x12C, 0x130, 0x88, 0x134 }) });
    gr.push_back({ 50, "Matchup", false, campos({ 0x30, 0x2C, 0x34, 0x0C }) });
    gr.push_back({ 197, "Support", true, campos({ 0x20, 0x24, 0x28, 0x44, 0x50, 0x68, 0x58, 0x5C, 0x60, 0x64, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0x78 }) });
    gr.push_back({ 195, "SpaceRun", true, campos({ 0x3C, 0x40, 0x44, 0x48, 0x4C, 0x50, 0x34, 0x38, 0x64, 0x68, 0x74, 0x78, 0x7C, 0x80, 0x9C, 0xA8 }, { 0x8C, 0x90, 0x94, 0x98, 0xA0, 0xA4 }, { 0xBC, 0xC0, 0xC4, 0xC8, 0xCC, 0xD0 }) });
    gr.push_back({ 189, "DiagonalRun", true, campos({ 0x1C, 0x20, 0x10, 0x14, 0x18 }, { 0x24, 0x28, 0x2C, 0x30 }) });
    gr.push_back({ 193, "PullAway", true, campos({ 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C }) });
    gr.push_back({ 61, "Throughpass", true, campos({ 0x118, 0x120, 0x188, 0x190, 0x18C, 0x194, 0x19C, 0x1A0, 0x70, 0x74, 0x154, 0x158, 0x17C, 0x184 }) });
    gr.push_back({ 53, "Passget", true, campos({ 0x64 }) });
    for (auto& g : gr) { g.orig.assign(g.c.size(), 0.0f); g.pat.assign(g.c.size(), 0.0f); }
    while (g_iaChicaOn) {
        Sleep(150);
        if (!trainingWorkEstable() || !g_regDemoGlobal) { for (auto& g : gr) restaurarGrupoIA(g); continue; }
        uint8_t* raiz = nullptr; float* dims = nullptr; float d[2] = { 0, 0 };
        if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz || !leerSeguro(raiz + 0x2AA8, &dims, 8) || !dims || !leerSeguro(dims, d, 8) || !(d[0] > 5.0f && d[0] < 60.0f && d[1] > 3.0f && d[1] < 40.0f)) continue;
        for (auto& g : gr) {
            uint8_t *P = nullptr, *A = nullptr, *E = nullptr, *R = nullptr;
            if (!leerSeguro(g_regDemoGlobal, &P, 8) || !P || !leerSeguro(P, &A, 8) || !A || !leerSeguro(A + (size_t)g.id * 8, &E, 8) || !E || !leerSeguro(E + 8, &R, 8) || !R) continue;
            std::vector<float> cur(g.c.size()); bool valido = true;
            for (size_t i = 0; i < g.c.size(); i++) { if (!leerSeguro(R + g.c[i].off, &cur[i], 4) || !(fabsf(cur[i]) < 1000.0f) || cur[i] != cur[i]) { valido = false; break; } }
            if (!valido) continue;
            if (R != g.R) { restaurarGrupoIA(g); g.R = R; g.orig = cur; }
            else if (g.parcheado && memcmp(cur.data(), g.pat.data(), cur.size() * 4) != 0) g.orig = cur;   // el juego recargó: nueva original
            else if (!g.parcheado) g.orig = cur;
            float e = g.apoyo ? g_iaEApoyo : g_iaE, kx = powf(d[0] / 52.56f, e), kz = powf(d[1] / 34.06f, e), kd = sqrtf(kx * kz);
            std::vector<float> nuevo(g.c.size()); bool cambia = false;
            for (size_t i = 0; i < g.c.size(); i++) {
                float k = g.c[i].eje == 'x' ? kx : g.c[i].eje == 'z' ? kz : kd, o = g.orig[i];
                float n = fmaxf(o * k, fminf(o, g_iaPiso)); nuevo[i] = (o < 0.0f) ? o * k : n; if (nuevo[i] != cur[i]) cambia = true;
            }
            if (cambia) { for (size_t i = 0; i < g.c.size(); i++) memcpy(R + g.c[i].off, &nuevo[i], 4); g.pat = nuevo; }
            if (!g.parcheado) {
                g.parcheado = true; char buf[300]; int n = sprintf_s(buf, "[IA_DATOS] %s (id %d) escalado (kd=%.2f kx=%.2f kz=%.2f):", g.nom, g.id, kd, kx, kz);
                for (size_t i = 0; i < g.c.size() && i < 4; i++) n += sprintf_s(buf + n, sizeof(buf) - n, " +%X %.2f->%.2f", g.c[i].off, g.orig[i], nuevo[i]);
                logf("%s", buf);
            }
        }
    }
    for (auto& g : gr) restaurarGrupoIA(g);
    return 0;
}
bool iaChicaInstalar(Feature& f) {
    if (!g_regDemoGlobal) { uint8_t* r = firmaUnica("codigo", kFirmaRegDemo, f.id); if (!r) { f.estado = "sin firma del registro de constantes (FUN_141E5D4C0)"; return false; } int32_t rel = 0; memcpy(&rel, r + 3, 4); g_regDemoGlobal = r + 7 + rel; }
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    g_iaChicaOn = 1; g_hiloIaChica = CreateThread(nullptr, 0, hiloIaChica, nullptr, 0, nullptr);
    if (!g_hiloIaChica) { g_iaChicaOn = 0; f.estado = "no se pudo crear el hilo de la IA chica"; return false; }
    char sb[170]; sprintf_s(sb, "ok: la IA de campo piensa en la minicancha (escala^%.2f, apoyo^%.2f, piso %.1f m); porteros intactos; se restaura al salir", g_iaE, g_iaEApoyo, g_iaPiso); f.estado = sb; return true;
}
bool iaChicaQuitar(Feature&) { g_iaChicaOn = 0; if (g_hiloIaChica) { WaitForSingleObject(g_hiloIaChica, 2000); CloseHandle(g_hiloIaChica); g_hiloIaChica = nullptr; } return true; }

// --- stats_sonda (0.4.88, frente Q): SOLO LECTURA. Bloque de stats vivo del jugador i (0..21) = *(raiz+(i+0xF3)*0x28)+0x70 (bytes 40..99; k19..44 habilidades, +0x40 u64 de trucos, +0x48 u32 estilo).
// Con TrainingWork anota, cada 10 s (max 6 rondas por sesion), los 22 jugadores para calibrar el mapa k->nombre antes de escribir nada (street_stats).
volatile LONG g_statsSondaOn = 0; HANDLE g_hiloStatsSonda = nullptr;
DWORD WINAPI hiloStatsSonda(LPVOID) {
    int rondas = 0; ULONGLONG t0 = 0;
    while (g_statsSondaOn) {
        Sleep(500);
        if (rondas >= 6 || !trainingWorkEstable()) continue;
        ULONGLONG ah = GetTickCount64(); if (ah - t0 < 10000) continue;
        uint8_t* raiz = nullptr; if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz) continue;
        int ok = 0; char buf[400];
        for (int i = 0; i < 22; i++) {
            uint8_t* pj = nullptr; uint8_t* B = nullptr; uint8_t k[0x50];
            if (!leerSeguro(raiz + (size_t)(i + 0xF3) * 0x28, &pj, 8) || !pj) continue;
            B = pj + 0x70; if (!leerSeguro(B, k, 0x50)) continue;
            uint64_t sk = 0; uint32_t est = 0; memcpy(&sk, k + 0x40, 8); memcpy(&est, k + 0x48, 4);
            int n = sprintf_s(buf, "[STATS_SONDA] r%d j%02d B=%p k19-44:", rondas, i, (void*)B);
            for (int q = 19; q <= 44 && n < (int)sizeof(buf) - 8; q++) n += sprintf_s(buf + n, sizeof(buf) - n, " %u", (unsigned)k[q]);
            sprintf_s(buf + n, sizeof(buf) - n, " | trucos=%llX estilo=%X", (unsigned long long)sk, est);
            logf("%s", buf); ok++;
        }
        if (ok) { rondas++; t0 = ah; } else t0 = ah - 5000;
    }
    return 0;
}
bool statsSondaInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    g_statsSondaOn = 1; g_hiloStatsSonda = CreateThread(nullptr, 0, hiloStatsSonda, nullptr, 0, nullptr);
    if (!g_hiloStatsSonda) { g_statsSondaOn = 0; f.estado = "no se pudo crear el hilo"; return false; }
    f.estado = "ok: solo lectura, anota las stats de los 22 jugadores (6 rondas por partido)"; return true;
}
bool statsSondaQuitar(Feature&) { g_statsSondaOn = 0; if (g_hiloStatsSonda) { WaitForSingleObject(g_hiloStatsSonda, 2000); CloseHandle(g_hiloStatsSonda); g_hiloStatsSonda = nullptr; } return true; }
bool celChicaQuitar(Feature&) {
    g_celChicaOn = 0; g_demoAreaOn = 0;
    if (g_hiloDemoArea) { WaitForSingleObject(g_hiloDemoArea, 2000); CloseHandle(g_hiloDemoArea); g_hiloDemoArea = nullptr; }
    return true;
}

// --- reloj_street_free (0.4.57, RE del frente J §2.5): el reloj del partido es una estructura en S+0x1168 (S = [raíz+0x270]): +0 fase, +4 sub-fase (1 en juego), +0xC float «transcurrido» (S+0x1174; 60 unidades = 1 s,
// escritura plana sin protección) y una base protegida por hash (NO se toca). El HUD (match2D::Screen::Time) muestra base + float. En el entrenamiento la función del reloj (FUN_14044CE80) sale si TrainingWork está activo, así que
// el reloj queda en 0:00. El Core, desde un hilo, suma 60·dt a S+0x1174 mientras la sub-fase es 1 (en juego) y reinicia a 0 al llegar a «street_minutos» (json, por defecto 5): nunca se alcanzan los 45:00 del fin de tiempo.
volatile LONG g_relojOn = 0; HANDLE g_hiloReloj = nullptr; double g_streetMinutos = 5.0; bool g_relojNativo = false; volatile LONG g_usaDuracion = 0; uint8_t* g_cfgDuracionGlob = nullptr;
// Duración elegida en «Opciones gener.» (frente J2): byte de minutos en S_cfg+0x1C, S_cfg = *(*(exe+0x3705E10)+0x50)+0x35960 (valores 5-15, 20, 25, 30). Devuelve 0 si no se puede leer.
static bool g_relojMFijado = false;
static int duracionElegida() {
    uint8_t *a = nullptr, *b = nullptr; uint8_t m = 0;
    if (!g_cfgDuracionGlob || !leerSeguro(g_cfgDuracionGlob, &a, 8) || !a || !leerSeguro(a + 0x50, &b, 8) || !b || !leerSeguro(b + 0x35960 + 0x1C, &m, 1)) return 0;
    return (m >= 5 && m <= 30) ? (int)m : 0;
}
DWORD WINAPI hiloReloj(LPVOID) {
    LARGE_INTEGER fq, t0; QueryPerformanceFrequency(&fq); QueryPerformanceCounter(&t0); LONG vueltas = 0; ULONGLONG tM = 0;
    while (g_relojOn) {
        Sleep(16);
        LARGE_INTEGER t1; QueryPerformanceCounter(&t1); double dt = (double)(t1.QuadPart - t0.QuadPart) / (double)fq.QuadPart; t0 = t1;
        if (dt <= 0 || dt > 0.5) continue;
        if (!trainingWorkEstable()) continue;
        uint8_t* raiz = nullptr; uint8_t* S = nullptr;
        if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz || !leerSeguro(raiz + 0x270, &S, 8) || !S) continue;
        uint32_t sub = 0; float f = 0;
        uint32_t fase = 0; static ULONGLONG tLog = 0;
        if (!leerSeguro(S + 0x116C, &sub, 4) || !leerSeguro(S + 0x1168, &fase, 4) || !leerSeguro(S + 0x1174, &f, 4) || !(f >= 0.0f) || f > 1.0e6f) continue;
        if (GetTickCount64() - tLog > 5000) { tLog = GetTickCount64(); uint32_t reg = 0; leerSeguro(S + 0x11B0, &reg, 4); logf("[RELOJ] fase=%u sub=%u transcurrido=%.0f (=%.1f s) S+0x11B0=0x%08X", fase, sub, f, f / 60.0f, reg); }
        if (!g_relojNativo) f += (float)(60.0 * dt);   // con R1 el juego ya lo avanza; el Core solo lo reinicia al llegar a street_minutos
        // 0.4.62 (frente J4): el juego suma al reloj de pantalla inc = 90/((M/(x5C·(60/G)))/x60) por tick con M = float [[raíz+0x2C0]+0x58] (la duración del menú, 5-30): 45:00 de pantalla = M minutos de juego vivo.
        // Con «ajustes_partido_free» se fuerza M = 45 (solo si coincide con el byte de duración del menú) y la pantalla corre 1:1 (tiempo real). S+0x1174 solo cuenta el tiempo muerto.
        if (g_usaDuracion && (GetTickCount64() - tM) > 100) {
            tM = GetTickCount64(); uint8_t* x = nullptr; float m = 0, c5 = 0, c6 = 0; int dm = duracionElegida();
            if (dm && leerSeguro(raiz + 0x2C0, &x, 8) && x && leerSeguro(x + 0x58, &m, 4) && leerSeguro(x + 0x5C, &c5, 4) && leerSeguro(x + 0x60, &c6, 4)) {
                static ULONGLONG tl = 0; if (GetTickCount64() - tl > 5000) { tl = GetTickCount64(); logf("[RELOJ] M=%.2f x5C=%.3f x60=%.3f duracion_menu=%d", m, c5, c6, dm); }
                if (fabsf(m - (float)dm) < 0.01f || (fabsf(m - 45.0f) < 0.01f && g_relojMFijado)) {
                    if (fabsf(m - 45.0f) > 0.01f) { float v45 = 45.0f; memcpy(x + 0x58, &v45, 4); g_relojMFijado = true; logf("[RELOJ] duración %d min -> M=45 (pantalla en tiempo real)", dm); }
                }
            }
        }
        double lim = g_streetMinutos;
        if (!g_usaDuracion && f >= (float)(lim * 60.0 * 60.0)) { f = 0.0f; memcpy(S + 0x1174, &f, 4); logf("[RELOJ] fin de los %.1f min (street_minutos): el reloj vuelve a 0 (vuelta %ld)", lim, (long)++vueltas); }
        if (!g_relojNativo) memcpy(S + 0x1174, &f, 4);
    }
    return 0;
}
bool relojInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    // 0.4.59: el Core escribía +60·dt pero el juego lo pisa a 0 cada fotograma en juego (log: transcurrido=0). Ruta del RE: R1 (RVA 0x44CEE2, 74->EB) saltea el chequeo de TrainingWork en FUN_14044CE80
    // y R1b (0x44CF02) el del objeto [raíz+0x3B0]; así corre el reloj nativo. Se aplican siempre (fuera del entrenamiento esos chequeos ya no se cumplen).
    bool r1 = localizarParche("80 B9 DB 27 00 00 00 48 8B F9 8B 91 94 27 00 00 41 0F 95 C0 48 81 C1 C0 27 00 00 E8 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 8B 88 00 04 00 00 48 85 C9 74 14", 49, 0x74, 0xEB, g_pdR1, f.id);
    bool r1b = localizarParche("48 8B 88 B0 03 00 00 48 85 C9 74 0D E8 ?? ?? ?? ?? 84 C0 0F 85 D0 01 00 00 E8 ?? ?? ?? ?? 84 C0 0F 85 C3 01 00 00", 10, 0x74, 0xEB, g_pdR1b, f.id);
    logf("[%s] parche R1 (TrainingWork) %s · R1b (objeto 0x3B0) %s", f.id, r1 ? "localizado" : "NO", r1b ? "localizado" : "NO");
    if (!r1b) g_pdR1b.dir = nullptr;
    if (r1) { asegurarHiloTrain(); g_relojR1On = 1; }   // 0.4.77: R1/R1b solo con TrainingWork (hiloTrainGlob)
    g_relojNativo = r1;
    g_relojOn = 1; g_hiloReloj = CreateThread(nullptr, 0, hiloReloj, nullptr, 0, nullptr);
    if (!g_hiloReloj) { g_relojOn = 0; f.estado = "no se pudo crear el hilo del reloj"; return false; }
    char b[140]; sprintf_s(b, "ok: el reloj nativo corre en el entrenamiento y reinicia a los %.1f min (escribe S+0x1174)", g_streetMinutos); f.estado = b; return true;
}
// --- ajustes_partido_free (0.4.60, frente J2): el menú «Opciones gener.» (menu::MatchGeneralSettings) sale gris en el modo 52 porque FUN_140AA90A0 compara modo==0x34 y pone [rsp+0x24]=0 (RVA 0xAA9261, `40 88 7C 24 24`),
// lo que acaba en MatchMenu+0x188=1. NOP de esos 5 bytes: filas 0..7 (nivel, duración, prórroga, pen., sustituciones, forma, lesiones) editables y guardadas. Reversible.
ParcheN g_pnAJ;
bool ajustesPartidoInstalar(Feature& f) {
    if (!localizarParcheN(g_pnAJ, "83 FD 34 75 09 40 88 7C 24 24 33 C9 EB ?? E8 ?? ?? ?? ?? 84 C0 75 ?? 40 88 BB 87 01 00 00", 5, { 0x40, 0x88, 0x7C, 0x24, 0x24 }, { 0x0F, 0x1F, 0x44, 0x00, 0x00 }, f.id)) { f.estado = "sin firma/byte inesperado en FUN_140AA90A0 (grises del menú de ajustes)"; return false; }
    ponerParcheN(g_pnAJ, true);
    g_cfgDuracionGlob = g_exe.base + 0x3705E10; g_usaDuracion = 0;   /* 0.4.62: Fralex quiere el ritmo NATIVO (45:00 = duración del menú): no se fuerza M=45 */   // RVA fija del exe v1.1.0.0 (frente J2)
    f.estado = g_pnAJ.puesto ? "ok: el menú «Opciones gener.» del entrenamiento ya no sale gris (parche de 5 bytes en FUN_140AA90A0)" : "no se pudo escribir el parche del menú";
    return g_pnAJ.puesto;
}
bool ajustesPartidoQuitar(Feature&) { g_usaDuracion = 0; ponerParcheN(g_pnAJ, false); return true; }
// --- sin_fin_tiempo_free (0.4.61, frente J §T1): con el reloj nativo corriendo, al llegar a 45:00 el Juez (FUN_14050B9B0) pita el medio tiempo y el entrenamiento «no sabe qué hacer» (captura de Fralex, 48:12 +2).
// `44 0F B6 C3 0F 28 CE E8 ?? ?? ?? ?? 84 C0 0F 84 60 08 00 00 83 BE 20 29 00 00 04` byte +12 (RVA 0x50BA3F): `84`→`32` (test al,al → xor al,al): la decisión por tiempo siempre es «no». Solo con TrainingWork; el reloj sigue sumando.
bool sinFinTiempoInstalar(Feature& f) {
    if (!localizarParche("44 0F B6 C3 0F 28 CE E8 ?? ?? ?? ?? 84 C0 0F 84 60 08 00 00 83 BE 20 29 00 00 04", 12, 0x84, 0x32, g_pdT1, f.id)) { f.estado = "sin firma/byte inesperado en el Juez (FUN_14050B9B0)"; return false; }
    asegurarHiloTrain(); g_sinFinOn = 1;
    f.estado = "ok: el entrenamiento no tiene fin de tiempo (parche de 1 byte en el Juez, solo con TrainingWork)"; return true;
}
bool sinFinTiempoQuitar(Feature&) { g_sinFinOn = 0; ponerParche(g_pdT1, false); return true; }
// --- street_dos_tiempos (0.4.63, frente J4 §9): lee el reloj de pantalla t (unidades; 162000 = 45:00) y hace el MEDIO TIEMPO propio a 45:00 y el FINAL a 90:00 (con T1 el Juez no pita). Lectura verificada por RE estático (sin medir en vivo):
// S = [[exe+0x36F3DC8]+0x270]; reloj espejo de 0x30 B en S+0x1168: +0 fase, +4 sub, +0xC float tiempo muerto, +0x10 hash FNV-1a 64, +0x18 contador, +0x1C índice, +0x20..+0x2F cuatro slots de la base. Decodificación (FUN_140A3C070):
// u = dword[+0x20+(dword[+0x1C]&3)*4], c = dword[+0x18]*3; b[k] = (u8)((u8)(u>>8k) + (u8)((c+4k)>>8k)) ^ (u8)(cc*0x3F); hash FNV-1a 64 sobre b[0..3] debe igualar qword[+0x10]; t = float(b) + float[+0xC].
// Bandera de seguridad: si la lectura es absurda (hash malo, t<0, t>400000, salto hacia atrás) NO actúa y solo anota. Acciones v1: pausa (pelota quieta en el centro unos segundos) y aviso en street_estado.json (lo lee el Lua); jugadores/cambio de campo: pendiente.
// --- saque central nativo SIN gol (0.4.65, frente J4 §10; RE estático, sin probar en vivo): el estado de parada es rec = job+0x49C0 (+0 sub: 1 centro; +4 equipo que SACA; +0x20 k: 3 gol, 4 fuera genérico),
// que rellena FUN_14050E770(judge=job+0x908, equipo, tipo, k, pos, p6, p7, p8). Con k=4 y sub=1 no hay gol ni celebración. Cueva en la entrada de FUN_14044CE80 (corre en el hilo del juego cada tick, RCX=job):
// si hay un pedido del Core y el job está ocioso, llama a E770(job+0x908, equipo, 1, 4, &centro,0,0,0); la cola nativa que se monta (0F 45 16 12 ..., con fundidos que esperan render) se reescribe a la probada 03 16 12 13 14 15 0E 34 19 1A.
// Para el FINAL, además se reinicia el reloj: FUN_140A3CBC0(job+0x2790, 0.0f) recifra la base y el float de tiempo muerto (job+0x279C) se pone a 0.
int g_sacaInicial = 0;   // json street_saca_inicial: equipo que sacó al inicio (0 local, 1 visita); el medio tiempo lo saca el otro
volatile LONG g_saquePedido = 0, g_saqueEquipo = 1, g_saqueResetReloj = 0, g_saqueFase = 0; volatile ULONGLONG g_saqueT = 0;
// --- street_pantalla_descanso (0.4.66, frente J5, A-min; RE estático, sin probar en vivo): a 45:00 se pone fase=2 y base 0 y se encola [0x22, 0x48...] (paso 0x22 = FUN_140449390: SOLO manda el mensaje 0x105001C que abre el menú REAL
// de descanso MatchStatsHalf/MatchHalfTime, sin la demo que cuelga); el relleno 0x48 aparca la cola. «Menú cerrado» = el job vuelve a ticar tras una pausa >1,5 s (el flujo suspende las tareas del partido mientras el menú está abierto;
// hipótesis) o a los 120 s como último recurso; entonces se limpia la cola y se pide el saque central nativo (que hace la fase 2->3). NO tocar «cambios» (plan) en esta prueba: puede colgar con el banquillo vacío.
// 0.4.81 (J5 §señal): ProcessMatchPostHalfTime::update (FUN_1412F7500, vf+0xC0; void(this)) corre por tick mientras ese proceso está activo, o sea cuando el menú de descanso se cerró y el flujo pasa a MatchPostHalfTime («proceed»).
// Gancho (crearDesvio 14 B): si lo armamos nosotros al encolar [0x22] y la instancia es PostHalfTime (vtable exe+0x28CCCB8), se marca g_menuCerrado. El Core lanza entonces el saque central. Sin gancho queda el límite de 120 s.
bool g_descansoFase3 = false;   // 0.4.89: por defecto false otra vez (con true en 0.4.87: cierre al volver al menú y marcador dudoso); json «descanso_fase3»: true = el 2.º tiempo se escribe en fase 3 con base 0 (HUD 45:00-90:00 sin +N); false (por defecto) = se restaura la fase anterior (0.4.82)
volatile LONG g_menuArmado = 0, g_menuCerrado = 0; volatile uint32_t g_faseAntesDescanso = 1; void (*g_postHalfOrig)(void*) = nullptr;
// 0.4.86 (J5): MatchPreHalfTime::update (FUN_1413090F0) espera el byte [obj+0x290] (obj = *(u64*)([exe+0x3705E10]+0x58)); lo pone a 1 el mensaje 0x105001B del paso de cola 0x25, que nuestra cola [0x22,0x48] no ejecuta: por eso el flujo quedaba en PreHalfTime.
// Con el menú armado se pone ese byte a 1 (el update lo baja a 0 y emite «proceed» hacia MatchHalfTime = menú nativo). Y en ProcessMatchPostHalfTime::update se ponen +0x89=1, +0x8A=0 para que emita «proceed» hacia MatchIdle.
void (*g_preHalfOrig)(void*) = nullptr;
void ganchoPreHalf(void* self) {
    if (g_menuArmado && !g_menuCerrado) {
        uint8_t *a = nullptr, *obj = nullptr;
        if (leerSeguro(g_exe.base + 0x3705E10, &a, 8) && a && leerSeguro(a + 0x58, &obj, 8) && obj) { uint8_t v = 0; leerSeguro(obj + 0x290, &v, 1); if (v == 0) { uint8_t uno = 1; memcpy(obj + 0x290, &uno, 1); static LONG n = 0; if (InterlockedIncrement(&n) <= 5) logf("[DESCANSO] MatchPreHalfTime: señal [obj+0x290]=1 escrita (el flujo pasa al menú de descanso)"); } }
    }
    g_preHalfOrig(self);
}
void ganchoPostHalf(void* self) {
    if (g_menuArmado && !g_menuCerrado) {
        uint64_t vt = 0; if (leerSeguro(self, &vt, 8) && vt == (uint64_t)(g_exe.base + 0x28CCCB8)) { uint8_t f89 = 0, f8a = 0; leerSeguro((uint8_t*)self + 0x89, &f89, 1); leerSeguro((uint8_t*)self + 0x8A, &f8a, 1); logf("[DESCANSO] ProcessMatchPostHalfTime activo (+0x89=%u +0x8A=%u): el menú se cerró", f89, f8a); g_menuCerrado = 1; g_menuArmado = 0; { uint8_t uno = 1, cero = 0; memcpy((uint8_t*)self + 0x89, &uno, 1); memcpy((uint8_t*)self + 0x8A, &cero, 1); } }
    }
    g_postHalfOrig(self);
}
volatile LONG g_descansoPedido = 0, g_descansoFase = 0; volatile ULONGLONG g_descansoT = 0, g_ultTickJob = 0; int g_descansoOn = 0;
using FnE770 = void (*)(uint8_t*, int, int, int, float*, uint64_t, uint64_t, uint64_t);
using FnBaseReloj = void (*)(void*, float);
FnE770 g_e770 = nullptr; FnBaseReloj g_baseReloj = nullptr; void (*g_arma4537)(uint8_t*) = nullptr;
// 0.4.75 (opción a de J4 §12, «árbitro invisible»): el Juez nativo SÍ corre en el entrenamiento (paso 0x3A), pero [juez+0x3B8] apunta a un tmp de la pila del paso y el valor puede ser 0 o basura cuando nuestra cueva lo mira.
// Se reproduce la preparación desde la cueva (hilo del juego): FUN_14050A8A0(tmp, *p, p[1]) con p = *(u64**)(job+0x17860) y FUN_14050B2B0(job+0x908, job+0x49C0, tmp) con un tmp PERSISTENTE; entonces E770 funciona sin cambios.
void (*g_a8a0)(uint64_t*, uint64_t, uint64_t) = nullptr; void (*g_b2b0)(uint8_t*, uint8_t*, uint64_t*) = nullptr; alignas(16) uint64_t g_tmpJuez[3] = { 0, 0, 0 };
static bool prepararJuez(uint8_t* job) {
    if (!g_a8a0 || !g_b2b0) return false;
    uint64_t* p = nullptr; if (!leerSeguro(job + 0x17860, &p, 8) || !p) return false;
    uint64_t a = 0, b2 = 0; if (!leerSeguro(p, &a, 8) || !a || !leerSeguro(p + 1, &b2, 8)) return false;
    g_a8a0(g_tmpJuez, a, b2); g_b2b0(job + 0x908, job + 0x49C0, g_tmpJuez); return true;
}
static void __stdcall ayudanteSaque(uint8_t* job) {
    ULONGLONG ahora = GetTickCount64(); ULONGLONG gap = g_ultTickJob ? ahora - g_ultTickJob : 0; g_ultTickJob = ahora;
    if (g_descansoPedido && !g_descansoFase) {
        uint32_t rec = 1, c878 = 1, k2794 = 0; uint8_t c8f0 = 1; uint64_t j3b8 = 0;
        bool ok = leerSeguro(job + 0x49C0, &rec, 4) && leerSeguro(job + 0x878, &c878, 4) && leerSeguro(job + 0x8F0, &c8f0, 1) && leerSeguro(job + 0x2794, &k2794, 4) && leerSeguro(job + 0x908 + 0x3B8, &j3b8, 8);
        bool ocioso = leerSeguro(job + 0x49C0, &rec, 4) && rec == 0;   // 0.4.74: [juez+0x3B8] es 0 en el entrenamiento (E770 no hace nada con eso): solo se exige que no haya parada en curso
        if (!ocioso) { if (!g_descansoT) g_descansoT = ahora; if (ahora - g_descansoT > 40000) { logf("[DESCANSO] el job no quedó ocioso en 40 s (rec=%u c878=%u c8f0=%u 2794=%u): pantalla cancelada, saque directo", rec, c878, c8f0, k2794); g_descansoPedido = 0; g_descansoT = 0; g_saquePedido = 1; } return; }
        // 0.4.82: el menú de descanso SOLO sale con fase=2 (con fase=1, 0.4.78-0.4.81, la cola [0x22] se consumía y no salía el menú: Fralex se quedó atascado). Se pone fase=2 mientras dura el menú y se VUELVE a la fase anterior al cerrarlo,
        // antes del saque: así FUN_140453530 no hace el 2->3 nativo (cambio de lado + IA/formación con el lado viejo). El reloj no se toca: sigue desde 45:00 y T1 evita el fin de tiempo.
        { uint32_t f0 = 1; leerSeguro(job + 0x2790, &f0, 4); g_faseAntesDescanso = f0; uint32_t fase2 = 2; memcpy(job + 0x2790, &fase2, 4);
            // 0.4.84: IDÉNTICO a 0.4.76 (la última que abrió el menú): base del reloj a 0 y float a 0 al abrir. Las versiones 0.4.78-0.4.83 sin este reinicio NO abrieron el menú (0.4.80: fase 1; 0.4.83: fase 2 sin reinicio; 0.4.76: fase 2 + reinicio = menú).
            if (g_baseReloj) g_baseReloj(job + 0x2790, 0.0f); float z0 = 0.0f; memcpy(job + 0x279C, &z0, 4); }
        uint32_t full[30]; for (int i = 0; i < 30; i++) full[i] = i == 0 ? 0x22 : 0x48;
        memcpy(job + 0x878, full, sizeof(full)); job[0x8F0] = 0; job[0x8F1] = 2;
        uint32_t sub2 = 2; memcpy(job + 0x2794, &sub2, 4);
        logf("[DESCANSO] fase=2, reloj a 0 y cola [0x22,0x48..] escritos: se abre el menú de descanso (como 0.4.76)");
        g_menuCerrado = 0; g_menuArmado = g_postHalfOrig ? 1 : 0;
        g_descansoPedido = 0; g_descansoFase = 1; g_descansoT = ahora; return;
    }
    if (g_descansoFase == 1) {
        static ULONGLONG tLogD = 0; if (ahora - tLogD > 3000) { tLogD = ahora; uint32_t q0 = 0, s94 = 0, f = 0; uint8_t cur = 0; leerSeguro(job + 0x878, &q0, 4); leerSeguro(job + 0x2794, &s94, 4); leerSeguro(job + 0x2790, &f, 4); leerSeguro(job + 0x8F0, &cur, 1); logf("[DESCANSO] esperando: cola0=0x%X cursor=%u fase=%u sub=%u gap=%llu ms", q0, (unsigned)cur, f, s94, (unsigned long long)gap); }
        if (g_menuCerrado || gap > 1500 || ahora - g_descansoT > 180000) {
            if (g_descansoFase3) {   // 0.4.83: 2.º tiempo exacto en 45:00 y sin «+N»: fase 3 escrita DIRECTA (sin pasar por 453530/14102CAD0 = sin el 2->3 nativo) y base del reloj a 0 (la pantalla suma 45:00 en fase 3)
                uint32_t f3 = 3; memcpy(job + 0x2790, &f3, 4); if (g_baseReloj) g_baseReloj(job + 0x2790, 0.0f); float z0 = 0.0f; memcpy(job + 0x279C, &z0, 4);
                g_relojPausa = 0; logf("[DESCANSO] 2.º tiempo: fase 3 directa y reloj a 0 (pantalla 45:00)");
            } else { uint32_t fa = g_faseAntesDescanso; memcpy(job + 0x2790, &fa, 4); g_relojPausa = 0; }   // 0.4.82: se restaura la fase
            uint32_t ceros[30] = {}; memcpy(job + 0x878, ceros, sizeof(ceros)); job[0x8F0] = 0; job[0x8F1] = 0; uint32_t sub1 = 1; memcpy(job + 0x2794, &sub1, 4);
            logf("[DESCANSO] menú cerrado (%s): cola limpia, saque central del 2.º tiempo", g_menuCerrado ? "señal de ProcessMatchPostHalfTime" : gap > 1500 ? "el job volvió a ticar tras la pausa" : "tiempo límite de 180 s"); g_menuArmado = 0;
            g_descansoFase = 0; g_descansoT = 0; g_saqueEquipo = g_sacaInicial ? 0 : 1; g_saqueResetReloj = 0; g_saqueT = 0; g_saquePedido = 1;
        }
        return;
    }
    if (!g_saquePedido && !g_saqueFase) return;
    if (g_saquePedido && !g_saqueFase) {
        uint32_t rec = 1, c878 = 1, k2794 = 0; uint8_t c8f0 = 1; uint64_t j3b8 = 0;
        bool ok = leerSeguro(job + 0x49C0, &rec, 4) && leerSeguro(job + 0x878, &c878, 4) && leerSeguro(job + 0x8F0, &c8f0, 1) && leerSeguro(job + 0x2794, &k2794, 4) && leerSeguro(job + 0x908 + 0x3B8, &j3b8, 8);
        bool ocioso = leerSeguro(job + 0x49C0, &rec, 4) && rec == 0;   // 0.4.74: [juez+0x3B8] es 0 en el entrenamiento (E770 no hace nada con eso): solo se exige que no haya parada en curso
        if (!ocioso) {
            if (!g_saqueT) g_saqueT = ahora;
            if (ahora - g_saqueT > 40000) { logf("[SAQUE] el job no quedó ocioso en 40 s (rec=%u c878=%u c8f0=%u 2794=%u): pedido cancelado", rec, c878, c8f0, k2794); g_saquePedido = 0; g_saqueT = 0; }
            return;
        }
        if (g_saqueResetReloj && g_baseReloj) { g_baseReloj(job + 0x2790, 0.0f); float z = 0.0f; memcpy(job + 0x279C, &z, 4); uint32_t f1 = 1; memcpy(job + 0x2790, &f1, 4); g_relojPausa = 0; logf("[SAQUE] reloj reiniciado a 0:00 (fase 1)"); }
        alignas(16) float centro[4] = { 0, 0, 0, 0 };
        (void)centro;
        bool hecho = false;
        if (prepararJuez(job)) {   // 0.4.75: con el juez enlazado E770 funciona sin cambios (k=7: lo que el gestor de entreno fuerza)
            g_e770(job + 0x908, (int)g_saqueEquipo, 1, 7, nullptr, 0, 0, 0);
            uint32_t r2 = 0; leerSeguro(job + 0x49C0, &r2, 4); hecho = (r2 == 1);
            logf("[SAQUE] juez enlazado (A8A0+B2B0) y E770 llamado: rec.sub=%u %s", r2, hecho ? "(ok)" : "(sin efecto: M2)");
        } else { uint32_t v5818 = 0, c0 = 0; leerSeguro(job + 0x5818, &v5818, 4); leerSeguro(job + 0xCC0, &c0, 4); logf("[SAQUE] no se pudo enlazar el juez (job+0x17860 nulo?) 5818=%u CC0=%u: M2", v5818, c0); }
        if (!hecho) {   // 0.4.74 (M2, J4 §11): E770 sale sin hacer nada si [juez+0x3B8]==0: se escribe el registro de parada directamente y se arma el reinicio con FUN_1404537B0
            uint8_t* rec = job + 0x49C0; uint32_t eq = (uint32_t)g_saqueEquipo;
            memset(rec + 8, 0, 0x18); uint32_t uno = 1, siete = 7, cero = 0, ff = 0xFF; uint32_t previo = 0;
            memcpy(rec, &uno, 4); memcpy(rec + 4, &eq, 4); memcpy(rec + 0x20, &siete, 4); rec[0x24] = 0;
            leerSeguro(job + 0x908 + 0xA0, &previo, 4); memcpy(rec + 0x28, &previo, 4); memcpy(job + 0x908 + 0xA0, &ff, 4);
            memcpy(rec + 0x34, &cero, 4); memcpy(rec + 0x3C, &cero, 4); memcpy(rec + 0x40, &cero, 4);
            uint8_t* raiz = nullptr; uint8_t* S = nullptr;
            if (leerSeguro(g_globalPartido, &raiz, 8) && raiz && leerSeguro(raiz + 0x270, &S, 8) && S) { memcpy(S + 0x1554, &uno, 4); memcpy(S + 0x1558, &eq, 4); memcpy(S + 0x1574, &siete, 4); }
            if (g_arma4537) g_arma4537(job);
        }
        logf("[SAQUE] saque central pedido: saca el equipo %d", (int)g_saqueEquipo);
        g_saquePedido = 0; g_saqueResetReloj = 0; g_saqueFase = 1; g_saqueT = ahora; return;
    }
    if (g_saqueFase == 1) {   // vigilar la cola montada; reescribir si empieza por 0F
        uint32_t q0 = 0; leerSeguro(job + 0x878, &q0, 4);
        if (q0 == 0x0F) {
            static const uint32_t cola[10] = { 0x03, 0x16, 0x12, 0x13, 0x14, 0x15, 0x0E, 0x34, 0x19, 0x1A };
            uint32_t full[30]; for (int i = 0; i < 30; i++) full[i] = i < 10 ? cola[i] : 0x48;
            memcpy(job + 0x878, full, sizeof(full)); job[0x8F1] = 10;
            logf("[SAQUE] cola nativa 0F... sustituida por 03 16 12 13 14 15 0E 34 19 1A"); g_saqueFase = 0; g_saqueT = 0; return;
        }
        if (q0 != 0 && ahora - g_saqueT > 200) { logf("[SAQUE] cola montada empieza por 0x%X (no 0F): se deja como está", q0); g_saqueFase = 0; g_saqueT = 0; return; }
        if (ahora - g_saqueT > 3000) { logf("[SAQUE] no se montó ninguna cola en 3 s"); g_saqueFase = 0; g_saqueT = 0; }
    }
}
bool saqueNativoInstalar(Feature& f) {
    uint8_t* e = firmaUnica("codigo", "48 8B C4 55 53 41 54 41 56 41 57 48 8D 68 C1 48 81 EC F0 00 00 00 48 83 B9 B8 03 00 00 00 45 8B F1 45 8B F8 44 8B E2 48 8B D9", f.id);
    uint8_t* r = firmaUnica("codigo", "F3 0F 11 4C 24 10 53 48 83 EC 10 8B 54 24 28 4C 8D 51 10 44 8B 0C 25 08 00 FE 7F", f.id);
    uint8_t* sitio = firmaUnica("codigo", "40 55 41 56 41 57 48 83 EC 40 48 8D 6C 24 20 48 89 5D 40 48 89 75 48 48 89 7D 50 4C 89 65 58 0F 29 75 10 48 8B 05 ?? ?? ?? ?? 48 33 C5", f.id);
    if (!e || !sitio) { f.estado = "sin firma de FUN_14050E770 / FUN_14044CE80"; return false; }
    g_e770 = reinterpret_cast<FnE770>(e); g_baseReloj = reinterpret_cast<FnBaseReloj>(r);
    uint8_t* a4537 = firmaUnica("codigo", "48 89 5C 24 08 57 48 83 EC 30 80 B9 11 4A 00 00 00 48 8B F9 75 39 8B 81 C0 49 00 00", f.id);
    if (!a4537) { f.estado = "sin firma de FUN_1404537B0 (armar el reinicio)"; return false; }
    g_arma4537 = reinterpret_cast<void (*)(uint8_t*)>(a4537);
    uint8_t* a8 = firmaUnica("codigo", "48 89 11 48 8B C1 4C 89 41 08 C7 41 10 FF 00 00 00 C3", f.id);
    uint8_t* b2 = firmaUnica("codigo", "4C 89 81 B8 03 00 00 48 89 91 C0 03 00 00 48 8D 4A 6C E9", f.id);
    if (a8 && b2) { g_a8a0 = reinterpret_cast<void (*)(uint64_t*, uint64_t, uint64_t)>(a8); g_b2b0 = reinterpret_cast<void (*)(uint8_t*, uint8_t*, uint64_t*)>(b2); }
    else logf("[%s] sin firma de FUN_14050A8A0/FUN_14050B2B0: solo M2", f.id);
    const uint8_t esp[10] = { 0x40, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x40 };
    uint8_t v[10]; if (!leerSeguro(sitio, v, 10) || memcmp(v, esp, 10) != 0) { f.estado = "bytes inesperados en FUN_14044CE80"; return false; }
    uint8_t* c = reservarCerca(sitio, 192);
    if (!c) { f.estado = "no hay memoria cerca del exe (saque)"; return false; }
    size_t o = 0; auto b = [&](std::initializer_list<uint8_t> l) { for (uint8_t x : l) c[o++] = x; };
    b({ 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53 });
    b({ 0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xE4, 0xF0, 0x48, 0x83, 0xEC, 0x60 });
    b({ 0x0F, 0x11, 0x44, 0x24, 0x20, 0x0F, 0x11, 0x4C, 0x24, 0x30, 0x0F, 0x11, 0x54, 0x24, 0x40, 0x0F, 0x11, 0x5C, 0x24, 0x50 });
    b({ 0x48, 0xB8 }); { uint64_t h = reinterpret_cast<uint64_t>(&ayudanteSaque); memcpy(c + o, &h, 8); o += 8; }
    b({ 0xFF, 0xD0 });
    b({ 0x0F, 0x10, 0x44, 0x24, 0x20, 0x0F, 0x10, 0x4C, 0x24, 0x30, 0x0F, 0x10, 0x54, 0x24, 0x40, 0x0F, 0x10, 0x5C, 0x24, 0x50 });
    b({ 0x48, 0x89, 0xEC, 0x5D });
    b({ 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58 });
    b({ 0x40, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x40 });
    size_t jb = o; b({ 0xE9, 0, 0, 0, 0 });
    if (!escribirRel32(c + jb, 0xE9, c + jb, sitio + 10)) { f.estado = "cueva fuera de alcance (saque)"; return false; }
    FlushInstructionCache(GetCurrentProcess(), c, 192);
    Parche p; p.dir = sitio; p.original.assign(sitio, sitio + 10); p.nuevo.assign(10, 0x90);
    if (!escribirRel32(p.nuevo.data(), 0xE9, sitio, c)) { f.estado = "cueva fuera de alcance (saque salto)"; return false; }
    if (!aplicarParche(p, f.id)) { f.estado = "no se pudo escribir el salto (saque)"; return false; }
    f.parches.push_back(p);
    char sbuf[160]; sprintf_s(sbuf, "ok: saque central nativo sin gol disponible (E770 exe+0x%llX, cueva en el reloj exe+0x%llX)%s", (unsigned long long)(e - g_exe.base), (unsigned long long)(sitio - g_exe.base), r ? "" : " · sin reinicio de reloj"); f.estado = sbuf; return true;
}
bool saqueNativoQuitar(Feature&) { g_saquePedido = 0; return true; }
// --- arquero_pase_corto (0.4.70, frente M): el selector de receptor del pase corto del portero de la IA (FUN_142132660, RVA 0x2132660; uint32 f(void** ctx, void*): ctx[0] = int* con el índice GLOBAL 0..21 del portero,
// ctx[5] = mundo IA; devuelve el índice GLOBAL del compañero, >=0x16 = ninguno) devuelve el PRIMER compañero que cumple los filtros (5-40 m), no el más cercano: en la mini cancha el pase «corto» podía ir al otro extremo con potencia
// alta (parecía un despeje). Gancho: tras la original, SOLO con TrainingWork y mientras lo maneja la IA, devuelve el compañero de campo disponible (estado 0xB en ctx[5]+0xB6C+0x3C+4*j) más cercano a >= 3 m y, si hay, a <= gk_pase_corto_m (10).
// No toca posición, cursor, salida corta, saque corto ni despeje de emergencia.
const char* kFirmaReceptor = "48 8B C4 48 89 58 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC D0 02 00 00 0F 29 78 A8 33 F6";
using FnRecv = uint32_t (*)(void**, void*);
FnRecv g_recvOrig = nullptr; volatile LONG g_pcOn = 0, g_pcCambios = 0; float g_pcMin = 3.0f, g_pcMax = 10.0f;
static bool posJugPC(uint8_t* raiz, int i, float& x, float& z) {
    uint8_t* rec = nullptr; float p[3];
    if (!leerSeguro(raiz + (size_t)(i + 0xBF) * 0x28, &rec, 8) || !rec) return false;
    if (!leerSeguro(rec + 0x55C, p, 12)) return false;
    x = p[0]; z = p[2]; return true;
}
uint32_t ganchoReceptor(void** ctx, void* rdx) {
    uint32_t r = g_recvOrig(ctx, rdx);
    if (!g_pcOn || GetTickCount64() - g_tTrainGlob > 3000) return r;
    uint8_t* raiz = nullptr; int* pg = nullptr; uint8_t* mundo = nullptr; int g = 0;
    if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz || !leerSeguro(ctx, &pg, 8) || !pg || !leerSeguro(ctx + 5, &mundo, 8) || !mundo || !leerSeguro(pg, &g, 4) || g < 0 || g > 21) return r;
    int base = (g > 10) ? 11 : 0; float gx, gz; if (!posJugPC(raiz, g, gx, gz)) return r;
    int best = -1, bestAny = -1; float dBest = 1e9f, dAny = 1e9f;
    for (int j = 1; j <= 10; j++) {
        int i = base + j; if (i == g) continue;
        int est = 0; if (!leerSeguro(mundo + 0xB6C + 0x3C + 4 * j, &est, 4) || est != 0xB) continue;
        float x, z; if (!posJugPC(raiz, i, x, z) || fabsf(x) > 20.0f) continue;
        float d = sqrtf((x - gx) * (x - gx) + (z - gz) * (z - gz));
        if (d < g_pcMin) continue;
        if (d < dAny) { dAny = d; bestAny = i; }
        if (d <= g_pcMax && d < dBest) { dBest = d; best = i; }
    }
    int nuevo = best >= 0 ? best : bestAny; if (nuevo < 0) return r;
    if ((uint32_t)nuevo != r) { LONG n = InterlockedIncrement(&g_pcCambios); if (n <= 40) logf("[PASE_CORTO] portero %d: receptor original %u -> %d (el más cercano disponible) #%ld", g, r, nuevo, (long)n); }
    return (uint32_t)nuevo;
}
bool paseCortoInstalar(Feature& f) {
    uint8_t* m = firmaUnica("codigo", kFirmaReceptor, f.id);
    if (!m) { f.estado = "sin firma: selector de receptor del pase corto del portero (FUN_142132660)"; return false; }
    if (!crearDesvio(f, m, 14, (void*)ganchoReceptor, (void**)&g_recvOrig)) { f.estado = "no se pudo desviar el selector de receptor"; return false; }
    asegurarHiloTrain(); g_pcOn = 1;
    char sb[170]; sprintf_s(sb, "ok: el pase corto del portero (IA) va al compañero más cercano disponible (%.0f-%.0f m) (exe+0x%llX)", g_pcMin, g_pcMax, (unsigned long long)(m - g_exe.base)); f.estado = sb; return true;
}
bool paseCortoQuitar(Feature&) { g_pcOn = 0; return true; }


// --- arquero_rama_normal (0.4.67, frente L §9): FUN_1409DC040 va a la rama alternativa del portero (FUN_1409DF7D0, ~3 KB, portero activo) si team+0x255 != 0. Con el jz de RVA 0x9DC8CF pasado a jmp
// (`80 B8 55 02 00 00 00 74 13 ...` byte +7: 74 -> EB) el portero usa siempre la rama normal FUN_1409DE8C0, como el portero COM. Solo con TrainingWork; restaurado al salir.
bool ramaNormalInstalar(Feature& f) {
    if (!localizarParche("80 B8 55 02 00 00 00 74 13 48 8D 85 A0 00 00 00 48 89 44 24 20 E8 ?? ?? ?? ?? EB 05 E8", 7, 0x74, 0xEB, g_pdKR, f.id)) { f.estado = "sin firma/byte inesperado en la rama del portero (FUN_1409DC040)"; return false; }
    asegurarHiloTrain(); g_krOn = 1;
    f.estado = "ok: el portero usa siempre la rama normal de posición (como el COM); solo en el entrenamiento"; return true;
}
bool ramaNormalQuitar(Feature&) { g_krOn = 0; ponerParche(g_pdKR, false); return true; }
bool descansoInstalar(Feature& f) {
    if (!g_postHalfOrig) { uint8_t* m = firmaUnica("codigo", "48 8B C4 55 48 8B EC 48 81 EC 80 00 00 00 48 C7 45 A8 FE FF FF FF 48 89 58 10 48 89 78 18 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 F0 48 8B D9 80 B9 89 00 00 00 00 0F 84", f.id);
        if (m && crearDesvio(f, m, 14, (void*)ganchoPostHalf, (void**)&g_postHalfOrig)) logf("[%s] señal de menú cerrado enganchada (exe+0x%llX)", f.id, (unsigned long long)(m - g_exe.base)); else logf("[%s] sin señal de menú cerrado: límite de 120 s", f.id); }
    if (!g_preHalfOrig) { uint8_t* m = firmaUnica("codigo", "40 57 48 83 EC 60 48 C7 44 24 28 FE FF FF FF 48 89 5C 24 78 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 50 48 8B F9 E8 ?? ?? ?? ?? 48 8B 58 58 48 8B CB E8 ?? ?? ?? ?? 84 C0 74", f.id);
        if (m && crearDesvio(f, m, 15, (void*)ganchoPreHalf, (void**)&g_preHalfOrig)) logf("[%s] MatchPreHalfTime enganchado (exe+0x%llX)", f.id, (unsigned long long)(m - g_exe.base)); else logf("[%s] sin gancho de MatchPreHalfTime", f.id); }
    g_descansoOn = 1; f.estado = "ok: a 45:00 se abre la pantalla REAL de medio tiempo (fase 2 + paso 0x22) y al cerrarla saque central del 2.º tiempo (requiere street_saque_nativo; NO tocar cambios)"; return true; }
bool descansoQuitar(Feature&) { g_descansoOn = 0; return true; }

volatile LONG g_dosTiemposOn = 0; HANDLE g_hiloDosTiempos = nullptr;
static bool leerRelojPantalla(double* tOut) {
    uint8_t* raiz = nullptr; uint8_t* S = nullptr;
    if (!leerSeguro(g_globalPartido, &raiz, 8) || !raiz || !leerSeguro(raiz + 0x270, &S, 8) || !S) return false;
    uint8_t r[0x30]; if (!leerSeguro(S + 0x1168, r, 0x30)) return false;
    uint32_t idx, cnt, slot[4]; float muerto; uint64_t hash;
    memcpy(&cnt, r + 0x18, 4); memcpy(&idx, r + 0x1C, 4); memcpy(slot, r + 0x20, 16); memcpy(&muerto, r + 0xC, 4); memcpy(&hash, r + 0x10, 8);
    uint32_t u = slot[idx & 3], c = cnt * 3; uint8_t b[4];
    for (int k = 0; k < 4; k++) { uint8_t cc = (uint8_t)((c + 4 * k) >> (8 * k)); b[k] = (uint8_t)((uint8_t)(u >> (8 * k)) + cc) ^ (uint8_t)(cc * 0x3F); }
    uint64_t h = 0xcbf29ce484222325ULL; for (int k = 0; k < 4; k++) { h ^= b[k]; h *= 0x100000001b3ULL; }
    if (h != hash) return false;
    float base; memcpy(&base, b, 4);
    double t = (double)base + (double)muerto;
    if (!(t >= 0.0 && t < 400000.0)) return false;
    *tOut = t; return true;
}
static void escribirEstadoStreet(const char* evento, double t) {
    char b[200]; sprintf_s(b, "{\"evento\":\"%s\",\"t\":%.0f,\"hora\":%llu}\n", evento, t, (unsigned long long)GetTickCount64());
    FILE* f = nullptr; if (fopen_s(&f, (g_carpeta + "street_estado.json").c_str(), "wb") == 0 && f) { fputs(b, f); fclose(f); }
}
// 0.4.84 (frente J5, blanco tras el descanso): sider.dll lee stadium/timeofday/weather/season de 4 dwords seguidos del struct de ajustes del partido (0xA0 B; FUN_1414C07B0 lo copia y ahí cuelga set_conditions).
// El origen es el mismo S_cfg de la duración: *(*(exe+0x3705E10)+0x50)+0x35960. Se anota [CONDICIONES] cada vez que cambia y, con «season_forzada» (json), si la temporada deja de ser la forzada (p. ej. tras el flujo del descanso) se REESCRIBE (solo con TrainingWork y valor 0..3).
int g_seasonCore = -1;
static void vigilarCondiciones() {
    static ULONGLONG t = 0; static uint32_t ult[4] = { 9, 9, 9, 9 }; ULONGLONG ah = GetTickCount64(); if (ah - t < 1000) return; t = ah;
    uint8_t *a = nullptr, *c = nullptr; if (!leerSeguro(g_exe.base + 0x3705E10, &a, 8) || !a || !leerSeguro(a + 0x50, &c, 8) || !c) return;
    uint32_t v[4]; if (!leerSeguro(c + 0x35960, v, 16)) return;
    if (memcmp(v, ult, 16) != 0) { logf("[CONDICIONES] stadium=%u timeofday=%u weather=%u season=%u (forzada=%d)", v[0], v[1], v[2], v[3], g_seasonCore); memcpy(ult, v, 16); }
    if (g_seasonCore >= 0 && g_seasonCore <= 3 && v[3] <= 3 && v[3] != (uint32_t)g_seasonCore && GetTickCount64() - g_tTrainGlob < 3000) {
        uint32_t nv = (uint32_t)g_seasonCore; memcpy(c + 0x35960 + 12, &nv, 4); logf("[CONDICIONES] season %u -> %u reescrita (tras el flujo del descanso)", v[3], nv);
    }
}
DWORD WINAPI hiloDosTiempos(LPVOID) {
    double prev = -1; int estado = 0; ULONGLONG tLog = 0; LONG malas = 0; ULONGLONG tFinal = 0; bool pendMedio = false, enTW = false;
    while (g_dosTiemposOn) {
        Sleep(50);
        vigilarCondiciones();
        if (!trainingWorkEstable()) {
            if (enTW) {   // 0.4.89: al salir del entrenamiento con la fase 3 escrita a mano, se devuelve a 1 antes de que el juego destruya la escena (candidata del cierre exe+0xB7AAA al volver al menú)
                enTW = false;
                if (g_descansoFase3) { uint8_t* rz = nullptr; uint8_t* Sx = nullptr; uint32_t f = 0;
                    if (leerSeguro(g_globalPartido, &rz, 8) && rz && leerSeguro(rz + 0x270, &Sx, 8) && Sx && leerSeguro(Sx + 0x1168, &f, 4) && f == 3) { uint32_t f1 = 1; memcpy(Sx + 0x1168, &f1, 4); logf("[TIEMPOS] salida del entrenamiento: fase 3 -> 1"); } }
            }
            continue;
        }
        enTW = true;
        uint32_t faseP = 0; { uint8_t* rz = nullptr; uint8_t* Sx = nullptr; if (leerSeguro(g_globalPartido, &rz, 8) && rz && leerSeguro(rz + 0x270, &Sx, 8) && Sx) leerSeguro(Sx + 0x1168, &faseP, 4); }
        double t = 0; if (!leerRelojPantalla(&t)) { if (++malas == 1 || malas % 400 == 0) logf("[TIEMPOS] lectura del reloj no válida (#%ld): no actúo", (long)malas); continue; }
        if (GetTickCount64() - tLog > 5000) { tLog = GetTickCount64(); logf("[TIEMPOS] t=%.0f (pantalla %d:%02d) estado=%d", t, (int)(t / 3600.0) + ((int)t % 3600 ? 1 : 0), (int)(t / 60.0) % 60, estado); }
        if (prev >= 0 && t < prev - 200.0 && estado == 1 && g_descansoFase3) { logf("[TIEMPOS] reloj a 0 por el descanso (%.0f -> %.0f): sigo en 2.º tiempo (final a 90:00)", prev, t); prev = t; continue; }   // 0.4.87: el descanso reinicia la base; no es un reinicio de partido
        if (prev >= 0 && t < prev - 200.0) { logf("[TIEMPOS] el reloj saltó hacia atrás (%.0f -> %.0f): reinicio la máquina de estados, no actúo", prev, t); estado = 0; prev = t; continue; }
        if (estado == 0 && t >= 162000.0 && (prev < 0 || prev < 162000.0 + 3600.0)) {
            estado = 1; logf("[TIEMPOS] MEDIO TIEMPO a t=%.0f: aviso y %s", t, g_e770 ? "saque central nativo del rival" : "pelota al centro 6 s");
            escribirEstadoStreet("medio_tiempo", t);
            pendMedio = true;   // 0.4.80: se pide cuando hayan pasado >10 s del último gol (la escena de gol + órdenes viejas provocó el cierre 0x4B712B)
        }
        if (estado == 1 && ((g_descansoFase3 && faseP == 3 && t >= 162000.0) || t >= 324000.0) && !(faseP == 2)) {
            estado = 2; logf("[TIEMPOS] FINAL a t=%.0f: aviso, pausa de 8 s y revancha con saque central y reloj a 0", t);
            g_bolaCentroHasta = GetTickCount64() + 8000; escribirEstadoStreet("final", t); tFinal = GetTickCount64() + 8000; g_relojPausa = 1;
        }
        if (pendMedio && GetTickCount64() - g_tUltGol > 10000) {
            pendMedio = false;
            if (g_e770 && g_descansoOn) { g_descansoT = 0; g_descansoPedido = 1; } else if (g_e770) { g_saqueEquipo = g_sacaInicial ? 0 : 1; g_saqueResetReloj = 0; g_saqueT = 0; g_saquePedido = 1; } else g_bolaCentroHasta = GetTickCount64() + 6000;
            logf("[TIEMPOS] medio tiempo pedido (último gol hace %.0f s)", (GetTickCount64() - g_tUltGol) / 1000.0);
        }
        if (estado == 2 && tFinal && GetTickCount64() >= tFinal && GetTickCount64() - g_tUltGol > 10000) {
            tFinal = 0;
            if (g_e770) { g_saqueEquipo = g_sacaInicial ? 1 : 0; g_saqueResetReloj = 1; g_saqueT = 0; g_saquePedido = 1; estado = 0; prev = -1; logf("[TIEMPOS] revancha pedida (reloj a 0:00); el marcador NO se reinicia todavía"); }
        }
        prev = t;
    }
    return 0;
}
bool dosTiemposInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    remove((g_carpeta + "street_estado.json").c_str());
    g_dosTiemposOn = 1; g_hiloDosTiempos = CreateThread(nullptr, 0, hiloDosTiempos, nullptr, 0, nullptr);
    if (!g_hiloDosTiempos) { g_dosTiemposOn = 0; f.estado = "no se pudo crear el hilo de los tiempos"; return false; }
    f.estado = "ok: medio tiempo a 45:00 y final a 90:00 propios (lee el reloj de pantalla; con T1; v1: pausa + aviso)"; return true;
}
bool dosTiemposQuitar(Feature&) { g_dosTiemposOn = 0; if (g_hiloDosTiempos) { WaitForSingleObject(g_hiloDosTiempos, 2000); CloseHandle(g_hiloDosTiempos); g_hiloDosTiempos = nullptr; } return true; }

bool relojQuitar(Feature&) { g_relojOn = 0; g_relojR1On = 0; ponerParche(g_pdR1, false); ponerParche(g_pdR1b, false); g_relojNativo = false; if (g_hiloReloj) { WaitForSingleObject(g_hiloReloj, 2000); CloseHandle(g_hiloReloj); g_hiloReloj = nullptr; } return true; }

// --- cursor_pt_log (0.4.61, frente L): SOLO LECTURA. FUN_140A571B0 (RVA 0xA571B0) = setControlado(reg, idx): `if (reg[+0x20] != idx) { reg[+0x28]=reg[+0x20]; reg[+0x20]=idx; reg[+0x38]=0; }`. Cueva que roba los 5 B
// `8B 41 20 3B C2` (mov eax,[rcx+0x20]; cmp eax,edx) y anota (idx nuevo, idx viejo, dirección de retorno del llamador) cuando el cursor pasa al portero (idx 0 u 11) y las primeras 40 llamadas, para saber QUIÉN pone el cursor en el portero propio.
volatile LONG g_cursorSinPtOn = 0, g_cursorCambiados = 0; bool g_cursorBloqueoCfg = false;   // json «cursor_bloqueo_pt»: false (por defecto) = solo log; true = bloquea el cambio automático hacia el portero (0.4.64; impide seleccionarlo)
static void __stdcall ayudanteCursor(uint8_t* reg, uint32_t idx, uint8_t* ret, uint32_t* idxPila) {
    uint32_t viejo = 0xFFFFFFFF; leerSeguro(reg + 0x20, &viejo, 4);
    static LONG nTodos = 0, nPt = 0; LONG t = InterlockedIncrement(&nTodos);
    bool pt = (idx == 0 || idx == 11);
    if (viejo == idx) return;
    long off = (long)(ret - g_exe.base); g_ultCursor = (LONG)idx;
    // 0.4.64: el cambio AUTOMÁTICO de cursor (llamadores exe+0x4403D2 = cambio por balón y exe+0x1FB98D2 = cursor inicial del reinicio) nunca debe caer en el portero propio (slot 0):
    // el log mostró «cursor -> portero idx=0 (antes 7/8/10)» desde 0x4403D2 y el portero del jugador quedó jugando de campo. Se cambia por el campista 9 (o se deja el cursor donde estaba si era un campista).
    if (g_cursorSinPtOn && idx == 0 && idxPila && (off == 0x4403D2 || off == 0x1FB98D2) && GetTickCount64() - g_tTrainGlob < 3000) {
        uint32_t nuevo = (viejo >= 1 && viejo <= 10) ? viejo : 9u;
        *idxPila = nuevo; LONG n = InterlockedIncrement(&g_cursorCambiados);
        if (n <= 40) logf("[CURSOR_PT] cursor al portero desde exe+0x%lX EVITADO: se queda en idx=%u (antes %u) #%ld", off, nuevo, viejo, (long)n);
        return;
    }
    if (pt) { LONG k = InterlockedIncrement(&nPt); if (k <= 300) logf("[CURSOR_PT] cursor -> portero idx=%u (antes %u) llamador=exe+0x%lX train=%d", idx, viejo, off, (int)(GetTickCount64() - g_tTrainGlob < 3000)); }
    else if (t <= 40) logf("[CURSOR_PT] cursor -> idx=%u (antes %u) llamador=exe+0x%lX", idx, viejo, off);
}
bool cursorPtLogInstalar(Feature& f) {
    uint8_t* sitio = firmaUnica("codigo", "8B 41 20 3B C2 74 0D 89 41 28 89 51 20 C7 41 38 00 00 00 00 C3", f.id);
    if (!sitio) { f.estado = "sin firma: FUN_140A571B0 (setControlado)"; return false; }
    uint8_t* c = reservarCerca(sitio, 160);
    if (!c) { f.estado = "no hay memoria cerca del exe (cursor_pt_log)"; return false; }
    size_t o = 0; auto b = [&](std::initializer_list<uint8_t> l) { for (uint8_t x : l) c[o++] = x; };
    b({ 0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53 });   // push rax,rcx,rdx,r8,r9,r10,r11 (7 = 56 B)
    b({ 0x4C, 0x8B, 0x44, 0x24, 0x38 });             // mov r8,[rsp+56] = dirección de retorno del llamador
    b({ 0x4C, 0x8D, 0x4C, 0x24, 0x20 });             // lea r9,[rsp+32] = dónde quedó guardado rdx (idx): el ayudante puede cambiarlo
    b({ 0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xE4, 0xF0, 0x48, 0x83, 0xEC, 0x20 });
    b({ 0x48, 0xB8 }); { uint64_t h = reinterpret_cast<uint64_t>(&ayudanteCursor); memcpy(c + o, &h, 8); o += 8; }   // mov rax,helper (rcx=reg, edx=idx, r8=ret ya puestos)
    b({ 0xFF, 0xD0 });
    b({ 0x48, 0x89, 0xEC, 0x5D });
    b({ 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58 });
    b({ 0x8B, 0x41, 0x20, 0x3B, 0xC2 });             // instrucciones robadas
    size_t jb = o; b({ 0xE9, 0, 0, 0, 0 });
    if (!escribirRel32(c + jb, 0xE9, c + jb, sitio + 5)) { f.estado = "cueva fuera de alcance (cursor_pt_log)"; return false; }
    FlushInstructionCache(GetCurrentProcess(), c, 160);
    Parche p; p.dir = sitio; p.original.assign(sitio, sitio + 5); p.nuevo.resize(5);
    if (!escribirRel32(p.nuevo.data(), 0xE9, sitio, c)) { f.estado = "cueva fuera de alcance (cursor_pt_log salto)"; return false; }
    if (!aplicarParche(p, f.id)) { f.estado = "no se pudo escribir el salto (cursor_pt_log)"; return false; }
    f.parches.push_back(p); g_cursorSinPtOn = g_cursorBloqueoCfg ? 1 : 0; asegurarHiloTrain();
    char sbuf[140]; sprintf_s(sbuf, "ok: el cursor automático no cae en el portero propio y anota quién lo mueve (exe+0x%llX)", (unsigned long long)(sitio - g_exe.base)); f.estado = sbuf; return true;
}
bool cursorPtLogQuitar(Feature&) { return true; }

// --- orden_jugador_segura (0.4.79, frente J4 §13): el cierre «PES2021.exe+0x4B712B c0000005» del 28-oct (2.º tiempo, ~6 s tras un gol) es `cmp byte [rax+1],1` con RAX=NULL dentro de FUN_1404B6ED0 (case 2 de la orden de un jugador): FUN_140A7CBA0(gestor,
// handle) devuelve NULL si el handle del jugador objetivo ya no resuelve (órdenes viejas tras un cambio de plantilla/lado, formación propia, escena de gol con rec.sub=0). Cueva anti-null como reinicio_seguro: si rax es NULL o [rax+1]!=1 se salta a la rama común.
bool ordenSeguraInstalar(Feature& f) {
    uint8_t* sig = firmaUnica("codigo", "41 8B 56 08 48 8B C8 E8 ?? ?? ?? ?? 80 78 01 01 0F 85 AE 01 00 00", f.id);
    if (!sig) { f.estado = "sin firma: orden de jugador FUN_1404B6ED0 (0x4B712B)"; return false; }
    uint8_t* sitio = sig + 12; const uint8_t esp[10] = { 0x80, 0x78, 0x01, 0x01, 0x0F, 0x85, 0xAE, 0x01, 0x00, 0x00 };
    uint8_t v[10]; if (!leerSeguro(sitio, v, 10) || memcmp(v, esp, 10) != 0) { f.estado = "bytes inesperados en FUN_1404B6ED0"; return false; }
    uint8_t* c = reservarCerca(sitio, 64); if (!c) { f.estado = "no hay memoria cerca del exe (orden_jugador_segura)"; return false; }
    uint8_t* skip = sitio + 10 + 0x1AE; uint8_t* ret = sitio + 10; size_t o = 0; auto b = [&](std::initializer_list<uint8_t> l) { for (uint8_t x : l) c[o++] = x; };
    auto rel = [&](uint8_t* destino) { int64_t d = destino - (c + o + 4); return (int32_t)d; };
    b({ 0x48, 0x85, 0xC0 });                                   // test rax,rax
    b({ 0x0F, 0x84 }); { int32_t r = rel(skip); memcpy(c + o, &r, 4); o += 4; }   // jz skip
    b({ 0x80, 0x78, 0x01, 0x01 });                             // cmp byte [rax+1],1
    b({ 0x0F, 0x85 }); { int32_t r = rel(skip); memcpy(c + o, &r, 4); o += 4; }   // jne skip
    b({ 0xE9 }); { int32_t r = rel(ret); memcpy(c + o, &r, 4); o += 4; }          // jmp ret
    FlushInstructionCache(GetCurrentProcess(), c, 64);
    Parche p; p.dir = sitio; p.original.assign(sitio, sitio + 10); p.nuevo.assign(10, 0x90);
    if (!escribirRel32(p.nuevo.data(), 0xE9, sitio, c)) { f.estado = "cueva fuera de alcance (orden_jugador_segura)"; return false; }
    if (!aplicarParche(p, f.id)) { f.estado = "no se pudo escribir el salto (orden_jugador_segura)"; return false; }
    f.parches.push_back(p);
    char sb[140]; sprintf_s(sb, "ok: red de seguridad de la orden de jugador (exe+0x%llX): handle nulo -> rama común, sin cierre", (unsigned long long)(sitio - g_exe.base)); f.estado = sb; return true;
}
bool ordenSeguraQuitar(Feature&) { return true; }

const char* kFirmaReinicio = "48 8B C4 53 41 57 48 83 EC 78 48 89 70 E8";
using FnReinicio = float* (*)(float*, uint32_t*, void*, void*);
FnReinicio g_reinicioOrig = nullptr; volatile LONG g_jugChOn = 0, g_jugChEscalados = 0;
int g_jugChModo = 1;   // json «jugadores_chica_modo»: 1 = solo recortar (lo que cae fuera de ±(dim-0.5) se escala hacia dentro; lo demás no se toca), 0 = escalar todo (0.4.43)
// --- jugadores_formacion (0.4.71, frente N): la tabla de reinicio (team+0x4B4C+slot*12, leída por FUN_1409F5530) trae un reparto fijo de ejercicio (defensas sobre su raya, jugadores pegados a las bandas), no una formación.
// Tras la original, solo en TrainingWork, fase 2/3 y parada de centro (S+0x1554==1), se sustituye la salida de los jugadores de campo (NO el portero slot 0, NO los dos pateadores S+0x173C/+0x1740) por una formación a escala EXACTA de la mini cancha
// (dims 15x10): slots 1/7 defensa izq (f=.50), 2/8 defensa der, 3/9 delantero izq (f=.17), 4/10 delantero der; el lado propio = signo de x de los slots 1..4 de la tabla.
bool g_formOn = false; float g_formDef = 0.50f, g_formDel = 0.17f; volatile LONG g_formAplic = 0;
static bool formacionSaque(uint8_t* raiz, uint32_t i, const float d[2], float* out) {
    if (i >= 22) return false;
    int slot = (int)(i % 11), side = (i > 10);
    if (slot == 0) return false;
    uint8_t* S = nullptr; uint32_t fase = 0, sub = 0, k1 = 99, k2 = 99;
    if (!leerSeguro(raiz + 0x270, &S, 8) || !S) return false;
    leerSeguro(S + 0x116C, &fase, 4); leerSeguro(S + 0x1554, &sub, 4);
    if ((fase != 2 && fase != 3) || sub != 1) return false;
    leerSeguro(S + 0x173C, &k1, 4); leerSeguro(S + 0x1740, &k2, 4);
    if (i == k1 || i == k2) return false;
    uint8_t* team = nullptr; if (!leerSeguro(raiz + (size_t)(side + 0xA1) * 0x28, &team, 8) || !team) return false;
    float sum = 0, t[3];
    for (int s = 1; s <= 4; s++) if (leerSeguro(team + 0x4B4C + s * 12, t, 12)) sum += t[0];
    float sg = sum >= 0 ? 1.f : -1.f;
    const float A = d[0] - 0.5f, B = d[1] - 0.5f;
    float f, zf; int r = (slot <= 4) ? slot - 1 : (slot >= 7 ? slot - 7 : -1);
    switch (r) { case 0: f = g_formDef; zf = -.40f; break; case 1: f = g_formDef; zf = +.40f; break;
                 case 2: f = g_formDel; zf = -.35f; break; case 3: f = g_formDel; zf = +.35f; break;
                 default: f = .33f; zf = ((slot % 3) - 1) * .40f; }
    out[0] = sg * A * f; out[2] = zf * B; return true;
}
float* ganchoReinicio(float* out, uint32_t* pIdx, void* ctx, void* team) {
    float sx = 0, sz = 0; float* e = nullptr; bool esc = false;
    if (g_jugChOn && GetTickCount64() - g_tTrainGlob < 3000) {
        uint8_t* raiz = nullptr; uint32_t i = 0; uint8_t* t = nullptr; float* dims = nullptr;
        if (leerSeguro(g_globalPartido, &raiz, 8) && raiz && leerSeguro(pIdx, &i, 4)) {
            int slot = (i < 22) ? (int)(i % 11) : 0, side = (i > 10 && i < 22) ? 1 : 0;
            if (leerSeguro(raiz + (size_t)(side + 0xA1) * 0x28, &t, 8) && t && leerSeguro(raiz + 0x2AA8, &dims, 8) && dims) {
                float d[2] = { 0, 0 }, tmp[3];
                if (leerSeguro(dims, d, 8) && d[0] > 5.0f && d[0] < 60.0f && d[1] > 3.0f && d[1] < 40.0f) {
                    e = reinterpret_cast<float*>(t + 0x4B4C + slot * 12);
                    if (leerSeguro(e, tmp, 12) && fabsf(tmp[2]) < 60.0f) {
                        if (g_jugChModo == 0) { sx = tmp[0]; sz = tmp[2]; e[0] = sx * (d[0] / 52.56f); e[2] = sz * (d[1] / 34.06f); esc = true; }
                        else {   // solo recortar: dentro de ±(dim-0.5) no se toca; fuera, se escala hacia dentro conservando la dirección
                            float Lx = d[0] - 0.5f, Wz = d[1] - 0.5f, ax = fabsf(tmp[0]), az = fabsf(tmp[2]);
                            if (ax > Lx || az > Wz) {
                                float k = 1.0f; if (ax > Lx) k = fminf(k, Lx / ax); if (az > Wz) k = fminf(k, Wz / az);
                                sx = tmp[0]; sz = tmp[2]; e[0] = sx * k; e[2] = sz * k; esc = true;
                            }
                        }
                    }
                }
            }
        }
    }
    float* r = g_reinicioOrig(out, pIdx, ctx, team);
    if (g_formOn && g_jugChOn && GetTickCount64() - g_tTrainGlob < 3000) {
        uint8_t* raiz = nullptr; uint32_t i = 0; float* dims = nullptr; float d[2] = { 0, 0 };
        if (leerSeguro(g_globalPartido, &raiz, 8) && raiz && leerSeguro(pIdx, &i, 4) && leerSeguro(raiz + 0x2AA8, &dims, 8) && dims && leerSeguro(dims, d, 8) && d[0] > 5.0f && d[0] < 60.0f && d[1] > 3.0f && d[1] < 40.0f) {
            float antes[3] = { out[0], out[1], out[2] };
            if (formacionSaque(raiz, i, d, out)) { LONG n = InterlockedIncrement(&g_formAplic); if (n <= 40) logf("[FORMACION] idx=%u (%.1f,%.1f) -> (%.1f,%.1f)", i, antes[0], antes[2], out[0], out[2]); }
        }
    }
    if (esc) { e[0] = sx; e[2] = sz; LONG n = InterlockedIncrement(&g_jugChEscalados); if (n <= 30) logf("[JUGCH] %s idx=%u tabla antes (%.1f,%.1f) · salida (%.1f,%.1f)", g_jugChModo ? "recorte" : "escala", *pIdx, sx, sz, out[0], out[2]); }
    return r;
}
bool jugChInstalar(Feature& f) {
    if (!g_globalPartido && !resolverGlobalPartido(f.id)) { f.estado = "sin firma del global del partido"; return false; }
    uint8_t* m = firmaUnica("codigo", kFirmaReinicio, f.id);
    if (!m) { f.estado = "sin firma: posición de reinicio FUN_1409F5530"; return false; }
    if (!crearDesvio(f, m, 14, (void*)ganchoReinicio, (void**)&g_reinicioOrig)) { f.estado = "no se pudo desviar la posición de reinicio"; return false; }
    asegurarHiloTrain(); g_jugChOn = 1;
    char b[150]; sprintf_s(b, "ok: escala a la cancha chica las posiciones de reinicio de los jugadores (exe+0x%llX)", (unsigned long long)(m - g_exe.base));
    f.estado = b; return true;
}
bool jugChQuitar(Feature&) { g_jugChOn = 0; return true; }

const char* kFirmaFilaBanda = "48 8B C4 55 41 56 41 57 48 8D 68 A1 48 81 EC 90 00 00 00 48 C7 45 F7 FE FF FF FF 48 89 58 08 48 89 70 10 48 89 78 18 44";
using FnFilaBanda = uint64_t (*)(float*, int);
FnFilaBanda g_filaBandaOrig = nullptr; volatile LONG g_supOcultosOn = 0, g_supOcultados = 0;
uint64_t ganchoFilaBanda(float* out, int idx) {
    uint64_t r = g_filaBandaOrig(out, idx);
    if (g_supOcultosOn && (r & 0xFF) && GetTickCount64() - g_tTrainGlob < 3000) {
        out[0] = 0.0f; out[2] = 100.0f;
        LONG n = InterlockedIncrement(&g_supOcultados); if (n <= 10) logf("[SUPL] suplente idx=%d aparcado en (0, 0, 100)", idx);
    }
    return r;
}
bool supOcultosInstalar(Feature& f) {
    uint8_t* m = firmaUnica("codigo", kFirmaFilaBanda, f.id);
    if (!m) { f.estado = "sin firma: fila de banda FUN_140A597D0"; return false; }
    if (!crearDesvio(f, m, 19, (void*)ganchoFilaBanda, (void**)&g_filaBandaOrig)) { f.estado = "no se pudo desviar la fila de banda"; return false; }
    asegurarHiloTrain(); g_supOcultosOn = 1;
    char b[150]; sprintf_s(b, "ok: los suplentes de la banda se aparcan en (0,0,100) (exe+0x%llX)", (unsigned long long)(m - g_exe.base));
    f.estado = b; return true;
}
bool supOcultosQuitar(Feature&) { g_supOcultosOn = 0; return true; }

std::vector<Feature> g_funciones = {   // orden = orden de arranque (gol_nativo_free necesita la sonda)
    { "diagnostico",     "Diagnóstico de firmas (solo lectura)",       diagInstalar,  diagQuitar  },
    { "sonda_partido",   "Sonda: estados y gol nativo (solo mira)",    sondaInstalar, sondaQuitar },
    { "sonda_pasos",     "Sonda: cola y pasos del gol (solo mira)",    pasosInstalar, pasosQuitar },
    { "gol_nativo_free", "Gol nativo en Entrenamiento libre (street)", golInstalar,   golQuitar   },
    { "saque_centro_free", "Tras el gol, saque de centro (entrenamiento)", saqueInstalar, saqueQuitar },
    { "celebracion_corta_free", "Gol sin celebración ni repetición (entrenamiento)", cortaInstalar, cortaQuitar },
    { "cancha_chica_free", "Cancha lógica pequeña (semilargo/semiancho del json) en el entrenamiento", canchaInstalar, canchaQuitar },
    { "siempre_disparo_free", "Cuadrado dispara siempre en el entrenamiento (sin despeje en campo propio)", disparoInstalar, disparoQuitar },
    { "jugadores_sonda", "Sonda de solo lectura de las posiciones de los 22 jugadores en el entrenamiento", jugSondaInstalar, jugSondaQuitar },
    { "malla_red_free", "La pelota en la red deforma la tela del arco (golpes GoalNet_Hit al paso de la pelota)", mallaInstalar, mallaQuitar },
    { "muros_core", "Muros lógicos del Core en el paso de la pelota (rebote contra la línea de la cancha chica)", murosInstalar, murosQuitar },
    { "red_frena_free", "La red de los arcos frena la pelota en el entrenamiento (gancho al paso de la pelota)", redInstalar, redQuitar },
    { "sin_saques_free", "Sin saques de banda/esquina/puerta en el entrenamiento: solo el gol reinicia (parches de 1 byte en el Juez)", sinSaquesInstalar, sinSaquesQuitar },
    { "arquero_propio_free", "El portero participa siempre en el entrenamiento, con cualquier número de jugadores y sin fijar el cursor", arqueroInstalar, arqueroQuitar },
    { "sin_despeje_emergencia", "La IA no hace despejes automáticos de emergencia en el entrenamiento (puede pasar, regatear y tirar)", iaClearInstalar, iaClearQuitar },
    { "arquero_saque_corto", "El portero IA no saca largo en el entrenamiento (anula GKPassLong y GKClear de la IA)", gkCortoInstalar, gkCortoQuitar },
    { "arquero_propio_cursor", "Con Cursor de PT fijo = Sí el portero propio no bloquea el cursor del usuario (variante del arquero propio)", arqCursorInstalar, arqCursorQuitar },
    { "arquero_salida_corta", "El portero sale poco de su arco en el entrenamiento (recorta la posición de salida a R m de la línea)", arqSalidaInstalar, arqSalidaQuitar },
    { "cursor_pt_log", "Solo lectura: anota quién pone el cursor del mando en el portero (llamador de setControlado)", cursorPtLogInstalar, cursorPtLogQuitar },
    { "orden_jugador_segura", "Red de seguridad: si la orden de un jugador (case 2) tiene un handle nulo, usa la rama común en vez de cerrar el juego (0x4B712B)", ordenSeguraInstalar, ordenSeguraQuitar },
    { "reinicio_seguro", "Red de seguridad: si la posición de reinicio del balón recibe una clave nula, usa la rama por defecto en vez de cerrar el juego", reinicioSeguroInstalar, reinicioSeguroQuitar },
    { "marcador_nativo_free", "El marcador y el reloj nativos del partido se muestran en el entrenamiento (parche de 1 byte en el HUD)", marcadorNativoInstalar, marcadorNativoQuitar },
    { "celebracion_chica", "La ancla de la celebración del gol se recorta a la cancha chica en el entrenamiento", celChicaInstalar, celChicaQuitar },
    { "ajustes_partido_free", "El menú «Opciones gener.» del entrenamiento libre se puede editar (duración del partido, etc.); el reloj usa esa duración", ajustesPartidoInstalar, ajustesPartidoQuitar },
    { "street_saque_nativo", "Saque central nativo sin gol para el medio tiempo y la revancha (cueva en el reloj + FUN_14050E770)", saqueNativoInstalar, saqueNativoQuitar },
    { "street_pantalla_descanso", "A 45:00 abre la pantalla real de medio tiempo del flujo (paso 0x22) y al cerrarla hace el saque central del 2.o tiempo (experimental)", descansoInstalar, descansoQuitar },
    { "arquero_rama_normal", "El portero usa siempre la rama normal de posición base (team+0x255 forzado a la normal) en el entrenamiento", ramaNormalInstalar, ramaNormalQuitar },
    { "arquero_pase_corto", "El pase corto del portero de la IA va al compañero disponible más cercano (no al primero que cumple los filtros)", paseCortoInstalar, paseCortoQuitar },
    { "ia_sonda", "Solo lectura: anota los datos de IA de campo (cobertura, defensa, marca, base, apoyo) para escalarlos a la minicancha", iaSondaInstalar, iaSondaQuitar },
    { "stats_sonda", "Solo lectura: anota en el log las stats de los 22 jugadores (calibrar el mapa de habilidades; apagado por defecto)", statsSondaInstalar, statsSondaQuitar },
    { "ia_cancha_chica", "La IA de campo usa distancias de cobertura, defensa, marca, apoyo y pase escaladas a la minicancha (con restauracion al salir; sin porteros)", iaChicaInstalar, iaChicaQuitar },
    { "street_dos_tiempos", "Medio tiempo a 45:00 y final a 90:00 propios del entrenamiento (lee el reloj de pantalla; v1: pausa + aviso)", dosTiemposInstalar, dosTiemposQuitar },
    { "sin_fin_tiempo_free", "El Juez no pita el fin de tiempo (medio tiempo/final) en el entrenamiento libre", sinFinTiempoInstalar, sinFinTiempoQuitar },
    { "reloj_street_free", "El reloj nativo corre en el entrenamiento y se reinicia a los street_minutos", relojInstalar, relojQuitar },
    { "jugadores_chica_free", "Escala a la cancha chica las posiciones de reinicio de los jugadores (entrenamiento)", jugChInstalar, jugChQuitar },
    { "suplentes_ocultos_free", "Aparca a los suplentes de la banda lejos (entrenamiento)", supOcultosInstalar, supOcultosQuitar },
    { "pelota_sonda", "Sonda de solo lectura de la pelota (posición/velocidad) en el entrenamiento", pelotaInstalar, pelotaQuitar },
    { "gol_directo_free", "Gol sin celebración, cámara ni repetición: solo reinicio (entrenamiento)", directoInstalar, directoQuitar },
    { "sin_1D_free", "Gol sin el paso 0x1D de la cola (entrenamiento; evita el congelón)", sin1DInstalar, sin1DQuitar },
    { "rescate_render_free", "Rescate del bloqueo juego↔render (experimental)", rescateInstalar, rescateQuitar },
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
    // opciones de gol_directo_free: «gol_directo_celebracion» (false = sin paso 0x04) y «celebracion_segundos» (0 = sin
    // celebración; N = se recorta a N s; por defecto 8). Lector simple: busca la clave y lee lo que sigue a los dos puntos.
    g_directoCelebracion = true; g_celebSeg = 8.0;
    auto valorDe = [&](const char* clave, std::string& v) {
        size_t k = txt.find(std::string("\"") + clave + "\"");
        if (k == std::string::npos) return false;
        k = txt.find(':', k);
        if (k == std::string::npos) return false;
        size_t j = txt.find_first_not_of(" \t\r\n", k + 1);
        if (j == std::string::npos) return false;
        size_t e = txt.find_first_of(",} \t\r\n", j);
        v = txt.substr(j, e == std::string::npos ? std::string::npos : e - j);
        return true;
    };
    std::string v;
    if (valorDe("gol_directo_celebracion", v) && v == "false") g_directoCelebracion = false;
    if (valorDe("celebracion_segundos", v)) g_celebSeg = atof(v.c_str());
    g_cajasFisicaGrandes = valorDe("cajas_fisica_grandes", v) && v == "true";
    if (valorDe("cancha_semilargo", v)) g_canchaLargo = atof(v.c_str());
    if (valorDe("cancha_semiancho", v)) g_canchaAncho = atof(v.c_str());
    if (g_canchaLargo < 8.0 || g_canchaLargo > 80.0) g_canchaLargo = 15.0;
    if (g_canchaAncho < 6.0 || g_canchaAncho > 50.0) g_canchaAncho = 10.0;
    if (valorDe("celebracion_modo", v)) g_celebModo = (v == "\"timer\"") ? 1 : (v == "\"salto\"") ? 2 : 0;
    if (valorDe("siempre_disparo_modo", v)) g_disparoModo = atoi(v.c_str());
    if (valorDe("jugadores_chica_modo", v)) g_jugChModo = atoi(v.c_str());
    if (valorDe("muro_rebote", v)) { g_muroRebote = (float)atof(v.c_str()); if (g_muroRebote < 0.0f || g_muroRebote > 1.0f) g_muroRebote = 0.6f; }
    if (valorDe("muro_friccion", v)) { g_muroFric = (float)atof(v.c_str()); if (g_muroFric < 0.0f || g_muroFric > 1.0f) g_muroFric = 0.92f; }
    if (valorDe("muro_x", v)) g_muroX = atof(v.c_str());
    if (valorDe("muro_z", v)) g_muroZ = atof(v.c_str());
    if (valorDe("arquero_salida_max", v)) { g_arqSalidaMax = (float)atof(v.c_str()); if (g_arqSalidaMax < 1.0f || g_arqSalidaMax > 12.0f) g_arqSalidaMax = 3.5f; }
    if (valorDe("gk_pase_corto_m", v)) { g_pcMax = (float)atof(v.c_str()); if (g_pcMax < 4.0f || g_pcMax > 40.0f) g_pcMax = 10.0f; }
    if (valorDe("jugadores_formacion", v)) g_formOn = (v == "true");
    if (valorDe("formacion_f_def", v)) g_formDef = (float)atof(v.c_str());
    if (valorDe("formacion_f_del", v)) g_formDel = (float)atof(v.c_str());
    if (valorDe("red_resorte", v)) g_redResorte = (v == "true");
    if (valorDe("red_k", v)) g_redK = (float)atof(v.c_str());
    if (valorDe("red_c", v)) g_redC = (float)atof(v.c_str());
    if (valorDe("red_holgura", v)) g_redHolgura = (float)atof(v.c_str());
    if (valorDe("ia_escala", v)) g_iaE = (float)atof(v.c_str());
    if (valorDe("ia_escala_apoyo", v)) g_iaEApoyo = (float)atof(v.c_str());
    if (valorDe("ia_piso_m", v)) g_iaPiso = (float)atof(v.c_str());
    if (valorDe("descanso_fase3", v)) g_descansoFase3 = (v == "true");
    if (valorDe("season_forzada", v)) g_seasonCore = atoi(v.c_str());
    if (valorDe("cursor_bloqueo_pt", v)) g_cursorBloqueoCfg = (v == "true");
    if (valorDe("street_minutos", v)) { g_streetMinutos = atof(v.c_str()); if (g_streetMinutos < 0.5 || g_streetMinutos > 44.0) g_streetMinutos = 5.0; }
    if (valorDe("red_factor", v)) { g_redFactor = (float)atof(v.c_str()); if (g_redFactor < 0.0f || g_redFactor > 1.0f) g_redFactor = 0.85f; }
    if (valorDe("vigia_escritura", v)) g_vigiaPedida = atoi(v.c_str());
    if (valorDe("vigia_pelota", v)) g_vigiaPelotaPedida = atoi(v.c_str());
    if (valorDe("medidor_real", v)) g_medidorReal = atoi(v.c_str());
    if (valorDe("siempre_disparo_carga", v)) g_disparoCarga = atoi(v.c_str());
    if (valorDe("celebracion_timer_final", v)) g_timerFinal = (uint32_t)atol(v.c_str());
    if (g_celebSeg <= 0.0) g_directoCelebracion = false;
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
        logf("arrancando.flag ya existía: el arranque anterior no terminó limpio en menos de 60 s (crash o cierre forzado) → MODO SEGURO");
    }
    // 0.4.87: aviso visible del modo seguro (el módulo Lua lo muestra en el overlay) y limpieza cuando el arranque es normal
    if (g_modoSeguro) escribirArchivo(g_carpeta + "MODO_SEGURO.txt", "Phoenix Core en MODO SEGURO: el arranque anterior se cortó (crash o cierre forzado antes de 60 s). Ninguna funcion del street esta activa. Cierra PES y vuelve a abrirlo (el proximo arranque sera normal).\r\n");
    else DeleteFileA((g_carpeta + "MODO_SEGURO.txt").c_str());
    escribirArchivo(flag, "Phoenix Core arrancando\r\n");
    g_tInicio = GetTickCount64();

    listarSecciones();
    leerConfig();
    // 0.4.87: primero lo que el menú previo y el HUD necesitan (ajustes_partido_free, marcador, reloj), luego el resto
    static const char* prioridad[] = { "ajustes_partido_free", "marcador_nativo_free", "reloj_street_free", "orden_jugador_segura", "reinicio_seguro", "cancha_chica_free" };
    for (const char* pn : prioridad) for (auto& f : g_funciones) if (strcmp(f.id, pn) == 0 && f.deseada) { encender(f); f.estado = f.estado; }
    for (auto& f : g_funciones) {
        bool yaHecha = false; for (const char* pn : prioridad) if (strcmp(f.id, pn) == 0) yaHecha = true;
        if (yaHecha && f.deseada) continue;
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
    InterlockedIncrement(&g_fotogramas);
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
        // 0.4.87: si PES se cierra de forma LIMPIA (el proceso llega aquí; un crash o un cierre forzado no pasa por DllMain), no es un crash: se borra la bandera y el próximo arranque NO entra en modo seguro
        DeleteFileA((g_carpeta + "arrancando.flag").c_str());
        for (auto& f : g_funciones) for (auto& p : f.parches) quitarParche(p);
    }
    return TRUE;
}
