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

#define PHOENIX_CORE_EXPORTS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>
#include "phoenix_api.h"

#define PHX_CORE_VERSION "0.1.0"

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

// Busca el patrón en una sección. Solo vale si aparece EXACTAMENTE una vez (si no, la función queda apagada).
uint8_t* firmaUnica(const char* sec, const char* patron, const char* quien) {
    Patron p;
    if (!parsearPatron(patron, p)) { logf("[%s] patrón mal escrito", quien); return nullptr; }
    uint8_t* ini; size_t tam;
    if (!seccion(sec, ini, tam)) { logf("[%s] no hay sección %s", quien, sec); return nullptr; }
    uint8_t* r; int n = buscar(p, ini, tam, r);
    if (n != 1) { logf("[%s] firma %s (%d coincidencias en %s)", quien, n ? "repetida" : "no encontrada", n, sec); return nullptr; }
    return r;
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
    uint8_t* r = firmaUnica(".rdata", "46 52 45 45 5F 54 52 41 49 4E 49 4E 47 5F 42 52 45 41 4B 00", f.id); // "FREE_TRAINING_BREAK\0"
    if (!r) { f.estado = "firma no encontrada"; return false; }
    char b[96]; sprintf_s(b, "ok: FREE_TRAINING_BREAK en exe+0x%llX", (unsigned long long)(r - g_exe.base));
    f.estado = b;
    return true;
}
bool diagQuitar(Feature&) { return true; }

// --- función 2: gol nativo en el Entrenamiento libre. Fase 1: aún sin firma (la está buscando REA).
bool golInstalar(Feature& f) { f.estado = "pendiente: sin firma todavía (fase 1)"; return false; }
bool golQuitar(Feature&) { return true; }

std::vector<Feature> g_funciones = {
    { "diagnostico",     "Diagnóstico de firmas (solo lectura)",       diagInstalar, diagQuitar },
    { "gol_nativo_free", "Gol nativo en Entrenamiento libre (street)", golInstalar,  golQuitar  },
};

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
    logf("[%s] no activada · %s", f.id, f.estado.c_str());
    return false;
}

bool apagar(Feature& f) {
    if (!f.activa) return true;
    int r = llamarSeguro(f.quitar, f);
    bool ok = r == 1;
    for (auto& p : f.parches) ok = quitarParche(p) && ok;
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
    if (g_flagBorrada || g_codigoInicio <= 0) return;
    if (GetTickCount64() - g_tInicio < 60000) return;
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
