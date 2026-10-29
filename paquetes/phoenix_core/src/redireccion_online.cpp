#include "redireccion_online.h"

#include <windows.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace phx_online {
namespace {

// ---------------------------------------------------------------- estado
LogFn       g_log = nullptr;
bool        g_instalado = false;
char        g_ip[32] = "127.0.0.1";
wchar_t     g_ipW[32] = L"127.0.0.1";
volatile LONG g_otros = 0;   // nombres que NO son de Konami (solo se anotan los primeros)

using PfnGetAddrInfo  = int (*)(const char*, const char*, const void*, void**);
using PfnGetAddrInfoW = int (*)(const wchar_t*, const wchar_t*, const void*, void**);
using PfnGetHostByName = void* (*)(const char*);
using PfnWinHttpConnect = void* (*)(void*, const wchar_t*, WORD, DWORD);
using PfnGetProcAddress = FARPROC (WINAPI*)(HMODULE, LPCSTR);
using PfnConnect = int (*)(UINT_PTR, const void*, int);
using PfnSendTo = int (*)(UINT_PTR, const char*, int, int, const void*, int);
using PfnWSASendTo = int (*)(UINT_PTR, void*, DWORD, DWORD*, DWORD, const void*, int, void*, void*);

PfnGetAddrInfo    g_origGai = nullptr;
PfnGetAddrInfoW   g_origGaiW = nullptr;
PfnGetHostByName  g_origGhbn = nullptr;
PfnWinHttpConnect g_origWhc = nullptr;
PfnGetProcAddress g_origGpa = nullptr;
PfnConnect        g_origConnect = nullptr;
PfnSendTo         g_origSendTo = nullptr;
PfnWSASendTo      g_origWsaSendTo = nullptr;
uint32_t          g_ipNet = 0;   // nuestra IP en orden de red
HMODULE           g_ws2 = nullptr;

struct Casilla { uint64_t* dir; uint64_t original; };
std::vector<Casilla> g_casillas;

void anota(const char* fmt, ...) {
    if (!g_log) return;
    char b[400];
    va_list a; va_start(a, fmt); vsnprintf(b, sizeof b, fmt, a); va_end(a);
    b[sizeof b - 1] = 0;
    g_log(b);
}

// ---------------------------------------------------------------- nombres
bool termina(const char* s, const char* sufijo) {
    if (!s) return false;
    size_t n = strlen(s), m = strlen(sufijo);
    return n >= m && _stricmp(s + n - m, sufijo) == 0;
}
bool terminaW(const wchar_t* s, const wchar_t* sufijo) {
    if (!s) return false;
    size_t n = wcslen(s), m = wcslen(sufijo);
    return n >= m && _wcsicmp(s + n - m, sufijo) == 0;
}
bool esKonami(const char* s) { return termina(s, ".konami.net"); }
bool esKonamiW(const wchar_t* s) { return terminaW(s, L".konami.net"); }

void otro(const char* api, const char* nombre) {
    if (InterlockedIncrement(&g_otros) <= 40) anota("online_phoenix: %s(\"%s\") no es de Konami: sin cambios", api, nombre ? nombre : "(nulo)");
}

// ---------------------------------------------------------------- desvíos
int hookGetAddrInfo(const char* nodo, const char* serv, const void* pistas, void** res) {
    if (esKonami(nodo)) {
        anota("online_phoenix: getaddrinfo(\"%s\") -> %s", nodo, g_ip);
        return g_origGai(g_ip, serv, pistas, res);
    }
    otro("getaddrinfo", nodo);
    return g_origGai(nodo, serv, pistas, res);
}
int hookGetAddrInfoW(const wchar_t* nodo, const wchar_t* serv, const void* pistas, void** res) {
    if (esKonamiW(nodo)) {
        anota("online_phoenix: GetAddrInfoW(\"%ls\") -> %s", nodo, g_ip);
        return g_origGaiW(g_ipW, serv, pistas, res);
    }
    if (InterlockedIncrement(&g_otros) <= 40) anota("online_phoenix: GetAddrInfoW(\"%ls\") no es de Konami: sin cambios", nodo ? nodo : L"(nulo)");
    return g_origGaiW(nodo, serv, pistas, res);
}
void* hookGetHostByName(const char* nombre) {
    if (esKonami(nombre)) {
        anota("online_phoenix: gethostbyname(\"%s\") -> %s", nombre, g_ip);
        return g_origGhbn(g_ip);
    }
    otro("gethostbyname", nombre);
    return g_origGhbn(nombre);
}
void* hookWinHttpConnect(void* sesion, const wchar_t* servidor, WORD puerto, DWORD reservado) {
    if (esKonamiW(servidor)) {
        anota("online_phoenix: WinHttpConnect(\"%ls\":%u) -> %s", servidor, (unsigned)puerto, g_ip);
        return g_origWhc(sesion, g_ipW, puerto, reservado);
    }
    if (InterlockedIncrement(&g_otros) <= 40) anota("online_phoenix: WinHttpConnect(\"%ls\") no es de Konami: sin cambios", servidor ? servidor : L"(nulo)");
    return g_origWhc(sesion, servidor, puerto, reservado);
}

// IP fijas de Konami que lleva el exe en su tabla de servidores (4 IP de AWS Tokio). No pasan por ningún nombre: se reescribe el destino.
bool ipKonamiFija(uint32_t netip) {
    static const uint8_t fijas[][4] = { {52,199,19,186}, {18,176,195,37}, {54,178,167,19}, {18,182,217,155}, {18,165,61,88}, {40,86,103,106} };
    for (auto& f : fijas) if (memcmp(&netip, f, 4) == 0) return true;
    return false;
}
// sockaddr_in = { WORD familia; WORD puerto; DWORD ip; ...}; si el destino es una IP fija de Konami se entrega una copia con la nuestra.
bool reescribir(const void* sa, int n, uint8_t* copia /*16*/) {
    if (!sa || n < 16) return false;
    const uint8_t* b = static_cast<const uint8_t*>(sa);
    uint16_t fam; memcpy(&fam, b, 2);
    uint32_t ip; memcpy(&ip, b + 4, 4);
    if (fam != 2 || !ipKonamiFija(ip)) return false;
    memcpy(copia, b, 16); memcpy(copia + 4, &g_ipNet, 4);
    anota("online_phoenix: destino fijo %u.%u.%u.%u:%u -> %s", b[4], b[5], b[6], b[7], (unsigned)((b[2] << 8) | b[3]), g_ip);
    return true;
}
int hookConnect(UINT_PTR s, const void* sa, int n) {
    uint8_t c[16];
    if (reescribir(sa, n, c)) return g_origConnect(s, c, 16);
    return g_origConnect(s, sa, n);
}
int hookSendTo(UINT_PTR s, const char* buf, int len, int fl, const void* sa, int n) {
    uint8_t c[16];
    if (reescribir(sa, n, c)) return g_origSendTo(s, buf, len, fl, c, 16);
    return g_origSendTo(s, buf, len, fl, sa, n);
}
int hookWSASendTo(UINT_PTR s, void* bufs, DWORD nb, DWORD* enviados, DWORD fl, const void* sa, int n, void* ov, void* cb) {
    uint8_t c[16];
    if (reescribir(sa, n, c)) return g_origWsaSendTo(s, bufs, nb, enviados, fl, c, 16, ov, cb);
    return g_origWsaSendTo(s, bufs, nb, enviados, fl, sa, n, ov, cb);
}
// El getaddrinfo de WspiapiLoad se pide con GetProcAddress la primera vez que se usa: se entrega el nuestro.
FARPROC WINAPI hookGetProcAddress(HMODULE m, LPCSTR nombre) {
    if (m && m == g_ws2 && reinterpret_cast<uintptr_t>(nombre) > 0xFFFF) {
        if (strcmp(nombre, "getaddrinfo") == 0 && g_origGai) { anota("online_phoenix: GetProcAddress(ws2_32, getaddrinfo) -> desvío"); return reinterpret_cast<FARPROC>(&hookGetAddrInfo); }
        if (strcmp(nombre, "GetAddrInfoW") == 0 && g_origGaiW) { anota("online_phoenix: GetProcAddress(ws2_32, GetAddrInfoW) -> desvío"); return reinterpret_cast<FARPROC>(&hookGetAddrInfoW); }
        if (strcmp(nombre, "gethostbyname") == 0 && g_origGhbn) { anota("online_phoenix: GetProcAddress(ws2_32, gethostbyname) -> desvío"); return reinterpret_cast<FARPROC>(&hookGetHostByName); }
    }
    return g_origGpa(m, nombre);
}

// ---------------------------------------------------------------- búsqueda de casillas en la imagen del exe
struct Objetivo { uint64_t valor; uint64_t nuevo; const char* nombre; int halladas; };

// Recorre la imagen de PES2021.exe (solo regiones MEM_IMAGE legibles y no ejecutables) y cambia cada puntero de 8 bytes alineado
// que valga exactamente la dirección de una de las funciones objetivo.
void cambiarCasillas(std::vector<Objetivo>& objs) {
    auto* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    uint8_t* fin = base + nt->OptionalHeader.SizeOfImage;
    uint8_t* p = base;
    size_t regiones = 0, bytes = 0;
    while (p < fin) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(p, &mbi, sizeof mbi)) break;
        uint8_t* ini = static_cast<uint8_t*>(mbi.BaseAddress);
        uint8_t* fr = ini + mbi.RegionSize;
        if (fr > fin) fr = fin;
        DWORD pr = mbi.Protect & 0xFF;
        bool legible = mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
                       (pr == PAGE_READONLY || pr == PAGE_READWRITE || pr == PAGE_WRITECOPY);
        if (legible && mbi.Type == MEM_IMAGE) {
            regiones++; bytes += static_cast<size_t>(fr - ini);
            auto* q = reinterpret_cast<uint64_t*>((reinterpret_cast<uintptr_t>(ini) + 7) & ~uintptr_t(7));
            auto* qf = reinterpret_cast<uint64_t*>(fr);
            __try {
                for (; q + 1 <= qf; ++q) {
                    uint64_t v = *q;
                    if (v < 0x10000) continue;
                    for (auto& o : objs) {
                        if (v != o.valor) continue;
                        DWORD ant = 0;
                        if (!VirtualProtect(q, 8, PAGE_READWRITE, &ant)) continue;
                        g_casillas.push_back({ q, v });
                        *q = o.nuevo;
                        VirtualProtect(q, 8, ant, &ant);
                        o.halladas++;
                        anota("online_phoenix: casilla %s en exe+0x%llX cambiada", o.nombre, (unsigned long long)(reinterpret_cast<uint8_t*>(q) - base));
                        break;
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                anota("online_phoenix: lectura interrumpida cerca de exe+0x%llX (se sigue con la siguiente región)", (unsigned long long)(reinterpret_cast<uint8_t*>(q) - base));
            }
        }
        p = fr > p ? fr : p + mbi.RegionSize;
    }
    anota("online_phoenix: recorridas %zu regiones (%zu MB)", regiones, bytes >> 20);
}

// ---------------------------------------------------------------- opción del json
bool ipValida(const std::string& s) {
    int partes = 0, val = 0, dig = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        char c = i < s.size() ? s[i] : '.';
        if (c == '.') {
            if (!dig || val > 255) return false;
            ++partes; val = 0; dig = 0;
        } else if (c >= '0' && c <= '9') { val = val * 10 + (c - '0'); ++dig; if (dig > 3) return false; }
        else return false;
    }
    return partes == 4;
}

bool leerOpcion(const std::string& carpeta, bool& activo, std::string& ip) {
    std::string ruta = carpeta + "phoenix_core.json", txt;
    FILE* f = nullptr;
    if (fopen_s(&f, ruta.c_str(), "rb") != 0 || !f) return false;
    char b[4096]; size_t n;
    while ((n = fread(b, 1, sizeof b, f)) > 0) txt.append(b, n);
    fclose(f);
    // Dos formas: plana (la que escribe el panel de Sider):  "online_phoenix": true, "online_phoenix_ip": "127.0.0.1"
    //             o anidada:  "online_phoenix": {"activo": true, "ip": "127.0.0.1"}
    auto texto = [&](const std::string& src, const char* clave, std::string& v) {
        size_t i = src.find(std::string("\"") + clave + "\"");
        if (i == std::string::npos) return false;
        i = src.find(':', i);
        size_t q1 = (i == std::string::npos) ? i : src.find_first_not_of(" \t\r\n", i + 1);
        if (q1 == std::string::npos || src[q1] != '"') return false;
        size_t q2 = src.find('"', q1 + 1);
        if (q2 == std::string::npos) return false;
        v = src.substr(q1 + 1, q2 - q1 - 1);
        return true;
    };
    size_t k = txt.find("\"online_phoenix\"");
    if (k == std::string::npos) return false;
    size_t c = txt.find(':', k);
    size_t j = (c == std::string::npos) ? c : txt.find_first_not_of(" \t\r\n", c + 1);
    if (j == std::string::npos) return false;
    if (txt[j] == '{') {
        size_t z = txt.find('}', j);
        if (z == std::string::npos) return false;
        std::string bloque = txt.substr(j, z - j + 1);
        size_t i = bloque.find("\"activo\"");
        if (i != std::string::npos) {
            i = bloque.find(':', i);
            size_t jj = (i == std::string::npos) ? i : bloque.find_first_not_of(" \t\r\n", i + 1);
            activo = (jj != std::string::npos && bloque.compare(jj, 4, "true") == 0);
        }
        texto(bloque, "ip", ip);
    } else {
        activo = txt.compare(j, 4, "true") == 0;
        texto(txt, "online_phoenix_ip", ip);
    }
    return true;
}

}  // namespace

void iniciar(const std::string& carpeta, bool modoSeguro, LogFn log) {
    g_log = log;
    bool activo = false; std::string ip = "127.0.0.1";
    leerOpcion(carpeta, activo, ip);
    if (!activo) { anota("online_phoenix: apagado (phoenix_core.json)"); return; }
    if (modoSeguro) { anota("online_phoenix: encendido en el json pero el Core está en MODO SEGURO: no se instala"); return; }
    if (g_instalado) return;
    if (!ipValida(ip)) { anota("online_phoenix: ip \"%s\" no válida: no se instala", ip.c_str()); return; }
    strncpy_s(g_ip, ip.c_str(), _TRUNCATE);
    for (size_t i = 0; i < sizeof g_ipW / sizeof g_ipW[0]; ++i) g_ipW[i] = static_cast<wchar_t>(static_cast<unsigned char>(g_ip[i]));
    { unsigned a1, a2, a3, a4; sscanf_s(g_ip, "%u.%u.%u.%u", &a1, &a2, &a3, &a4); uint8_t nb[4] = { (uint8_t)a1, (uint8_t)a2, (uint8_t)a3, (uint8_t)a4 }; memcpy(&g_ipNet, nb, 4); }
    anota("online_phoenix: encendido, ip %s; se buscan las casillas del exe", g_ip);

    g_ws2 = LoadLibraryA("ws2_32.dll");
    HMODULE wh = LoadLibraryA("winhttp.dll");
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    if (!g_ws2 || !k32) { anota("online_phoenix: no se pudo cargar ws2_32/kernel32: no se instala"); return; }
    g_origGpa = reinterpret_cast<PfnGetProcAddress>(GetProcAddress(k32, "GetProcAddress"));
    g_origGai = reinterpret_cast<PfnGetAddrInfo>(GetProcAddress(g_ws2, "getaddrinfo"));
    g_origGaiW = reinterpret_cast<PfnGetAddrInfoW>(GetProcAddress(g_ws2, "GetAddrInfoW"));
    g_origGhbn = reinterpret_cast<PfnGetHostByName>(GetProcAddress(g_ws2, "gethostbyname"));
    g_origConnect = reinterpret_cast<PfnConnect>(GetProcAddress(g_ws2, "connect"));
    g_origSendTo = reinterpret_cast<PfnSendTo>(GetProcAddress(g_ws2, "sendto"));
    g_origWsaSendTo = reinterpret_cast<PfnWSASendTo>(GetProcAddress(g_ws2, "WSASendTo"));
    if (wh) g_origWhc = reinterpret_cast<PfnWinHttpConnect>(GetProcAddress(wh, "WinHttpConnect"));

    std::vector<Objetivo> objs;
    auto agrega = [&](void* orig, void* nuevo, const char* nombre) {
        if (orig && nuevo) objs.push_back({ reinterpret_cast<uint64_t>(orig), reinterpret_cast<uint64_t>(nuevo), nombre, 0 });
        else anota("online_phoenix: %s no disponible en esta PC", nombre);
    };
    agrega(reinterpret_cast<void*>(g_origGai), reinterpret_cast<void*>(&hookGetAddrInfo), "getaddrinfo");
    agrega(reinterpret_cast<void*>(g_origGaiW), reinterpret_cast<void*>(&hookGetAddrInfoW), "GetAddrInfoW");
    agrega(reinterpret_cast<void*>(g_origGhbn), reinterpret_cast<void*>(&hookGetHostByName), "gethostbyname");
    agrega(reinterpret_cast<void*>(g_origConnect), reinterpret_cast<void*>(&hookConnect), "connect");
    agrega(reinterpret_cast<void*>(g_origSendTo), reinterpret_cast<void*>(&hookSendTo), "sendto");
    agrega(reinterpret_cast<void*>(g_origWsaSendTo), reinterpret_cast<void*>(&hookWSASendTo), "WSASendTo");
    agrega(reinterpret_cast<void*>(g_origWhc), reinterpret_cast<void*>(&hookWinHttpConnect), "WinHttpConnect");
    agrega(reinterpret_cast<void*>(g_origGpa), reinterpret_cast<void*>(&hookGetProcAddress), "GetProcAddress");

    cambiarCasillas(objs);
    for (auto& o : objs) anota("online_phoenix: %s: %d casilla(s) en el exe", o.nombre, o.halladas);
    g_instalado = true;
    anota("online_phoenix: listo (si ninguna casilla aparece, el exe todavía no tenía los punteros: reintentar más tarde)");
}

void detener() {
    if (!g_instalado) return;
    for (auto& c : g_casillas) {
        DWORD ant = 0;
        if (VirtualProtect(c.dir, 8, PAGE_READWRITE, &ant)) { *c.dir = c.original; VirtualProtect(c.dir, 8, ant, &ant); }
    }
    g_casillas.clear();
    g_instalado = false;
}

}  // namespace phx_online
