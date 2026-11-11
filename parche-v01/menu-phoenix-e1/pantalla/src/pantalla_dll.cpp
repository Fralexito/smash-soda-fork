// Phoenix Pantalla (0.1.0, PRUEBA 2) · DLL independiente de Phoenix Core.
//
// Dibuja un panel con texto sobre el menu principal de PES 2021 cuando la zona del juego pasa de 7 (TOP_MENU) a 49 (ONLINE_SETTINGS),
// es decir, al pulsar «Phoenix» en Ajustes cuando el evento 20 hace ida y vuelta por TopMenu/TopMenu. Diseno: pes-menus\DISENO_PANTALLA_PROPIA_CORE.md
//
// No toca Phoenix Core ni ningun archivo del juego: se carga con su propio modulo de Sider (phoenix_pantalla_loader.lua) y se apaga
// quitando esa linea del sider.ini o creando el archivo SiderAddons\phoenix_pantalla\PHOENIX_PANTALLA_OFF.
// Solo hay UNA API: phx_pantalla_iniciar(carpeta). Engancha IDXGISwapChain::Present por la vtable (indice 8), igual que Sider y el filtro de nitidez
// del Core, y se encadena con lo que haya (si Sider se engancha despues, nos encadena el).
//
// Sin dependencias: HLSL con d3dcompiler_47.dll de Windows y texto con GDI (ver pantalla.h).
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>

namespace {

std::string g_carpeta;
FILE* g_log = nullptr;

void logf(const char* fmt, ...) {
    if (!g_log) return;
    SYSTEMTIME t; GetLocalTime(&t);
    fprintf(g_log, "%04d-%02d-%02d %02d:%02d:%02d.%03d  ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list a; va_start(a, fmt); vfprintf(g_log, fmt, a); va_end(a);
    fputs("\r\n", g_log); fflush(g_log);
}

// ---- Zona del juego: misma lectura que Phoenix Core (gestor = [exe+0x3704E38], modo = u32 [gestor+0xF0]) con comprobacion de firma del exe.
uint8_t* g_base = nullptr;
volatile LONG g_zonaFirma = 0;   // 0 sin comprobar, 1 ok, 2 exe distinto

bool leerSeguro(const void* dir, void* dst, size_t n) {
    SIZE_T leidos = 0;
    return ReadProcessMemory(GetCurrentProcess(), dir, dst, n, &leidos) && leidos == n;
}

bool zonaFirmaOk() {
    if (g_zonaFirma) return g_zonaFirma == 1;
    static const struct { size_t rva; uint8_t b[17]; size_t n; } C[3] = {
        { 0x149A470, { 0x48,0x8B,0x05,0xC1,0xA9,0x26,0x02,0xC3 }, 8 },
        { 0x149F4F0, { 0x8B,0x81,0xF0,0x00,0x00,0x00,0x33,0xC9,0x83,0xF8,0x3D,0x0F,0x43,0xC1 }, 14 },
        { 0x149F510, { 0x80,0xB9,0x2C,0x01,0x00,0x00,0x00,0x74,0x08,0x8B,0x81,0x30,0x01,0x00,0x00,0xEB,0x06 }, 17 } };
    if (!g_base) return false;
    for (auto& c : C) { uint8_t t[17] = {}; if (!leerSeguro(g_base + c.rva, t, c.n) || memcmp(t, c.b, c.n) != 0) { g_zonaFirma = 2; logf("[zona] firma del exe distinta: la pantalla no se activara"); return false; } }
    g_zonaFirma = 1; return true;
}

volatile LONG g_zonaCache = -1; volatile ULONGLONG g_zonaT = 0;
int zonaId() {   // -1 = desconocida; cache de 100 ms
    ULONGLONG a = GetTickCount64(); if (g_zonaT && a - g_zonaT < 100) return (int)g_zonaCache;
    int r = -1; uint8_t* g = nullptr; uint32_t id = 0;
    if (zonaFirmaOk() && leerSeguro(g_base + 0x3704E38, &g, 8) && (uintptr_t)g > 0x10000 && (uintptr_t)g < 0x7FFFFFFF0000ull && leerSeguro(g + 0xF0, &id, 4) && id < 200) r = (int)id;
    g_zonaCache = r; g_zonaT = a; return r;
}

}  // namespace

#include "pantalla.h"   // pant::dibujarSeguro (usa logf y zonaId de arriba, en el espacio de nombres anonimo)

namespace {

typedef HRESULT(STDMETHODCALLTYPE* PresentFn)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(WINAPI* CrearDispFn)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
                                      const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
PresentFn g_sig = nullptr;
void** g_vt = nullptr;
bool g_enganchado = false;
volatile LONG g_hiloVivo = 0;

HRESULT STDMETHODCALLTYPE ganchoPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!(flags & DXGI_PRESENT_TEST)) pant::dibujarSeguro(sc);
    return g_sig(sc, sync, flags);
}

// Vtable de IDXGISwapChain creando un dispositivo y una swapchain minimos (se destruyen enseguida). Mismo metodo que nitidez.h del Core.
void** vtableSwapChain(char* diag, size_t nd) {
    diag[0] = 0;
    HMODULE d3d = GetModuleHandleA("d3d11.dll"); if (!d3d) d3d = LoadLibraryA("d3d11.dll");
    if (!d3d) { snprintf(diag, nd, "no hay d3d11.dll (err %lu)", GetLastError()); return nullptr; }
    CrearDispFn crear = (CrearDispFn)GetProcAddress(d3d, "D3D11CreateDeviceAndSwapChain");
    if (!crear) { snprintf(diag, nd, "no existe D3D11CreateDeviceAndSwapChain"); return nullptr; }
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "PhoenixPantallaDummy";
    RegisterClassA(&wc);
    HWND w = CreateWindowA(wc.lpszClassName, "p", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    if (!w) { snprintf(diag, nd, "CreateWindow fallo (err %lu)", GetLastError()); return nullptr; }
    DXGI_SWAP_CHAIN_DESC sd = {}; sd.BufferCount = 1; sd.BufferDesc.Width = 64; sd.BufferDesc.Height = 64; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = w; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    void** vt = nullptr;
    const D3D_DRIVER_TYPE tipos[2] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
    for (int i = 0; i < 2 && !vt; ++i) {
        IDXGISwapChain* sc = nullptr; ID3D11Device* dv = nullptr; ID3D11DeviceContext* cx = nullptr; D3D_FEATURE_LEVEL fl;
        HRESULT hr = crear(nullptr, tipos[i], nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dv, &fl, &cx);
        if (SUCCEEDED(hr) && sc) vt = *(void***)sc;
        else { size_t l = strlen(diag); snprintf(diag + l, nd - l, "%s hr=0x%08X; ", i ? "WARP" : "HARDWARE", (unsigned)hr); }
        if (sc) sc->Release(); if (cx) cx->Release(); if (dv) dv->Release();
    }
    DestroyWindow(w); UnregisterClassA(wc.lpszClassName, wc.hInstance);
    return vt;
}

void** vtableSegura(char* diag, size_t nd) {
    __try { return vtableSwapChain(diag, nd); }
    __except (EXCEPTION_EXECUTE_HANDLER) { snprintf(diag, nd, "excepcion 0x%08lX al crear la swapchain de prueba", (unsigned long)GetExceptionCode()); return nullptr; }
}

// Engancha Present en un hilo aparte con reintentos (cada 1 s, hasta 2 min), cuando el juego ya tiene ventana/DX11.
DWORD WINAPI hiloEnganche(LPVOID) {
    for (int i = 0; i < 120 && !g_enganchado; ++i) {
        char diag[240];
        void** vt = vtableSegura(diag, sizeof diag);
        if (vt) {
            DWORD old;
            if (!VirtualProtect(&vt[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) { logf("VirtualProtect fallo (err %lu)", GetLastError()); break; }
            void* actual = vt[8];
            if (actual != (void*)ganchoPresent) { g_sig = (PresentFn)actual; vt[8] = (void*)ganchoPresent; }
            VirtualProtect(&vt[8], sizeof(void*), old, &old);
            g_vt = vt; g_enganchado = true;
            logf("Present enganchado (intento %d) en vtable %p: siguiente (Sider o el real) = %p", i + 1, (void*)vt, (void*)g_sig);
            InterlockedExchange(&pant::g_on, 1);
            break;
        }
        if (i < 3 || i % 20 == 0) logf("intento %d sin vtable: %s", i + 1, diag);
        Sleep(1000);
    }
    if (!g_enganchado) logf("NO se pudo enganchar Present tras los reintentos; la pantalla queda apagada");
    InterlockedExchange(&g_hiloVivo, 0);
    return 0;
}

}  // namespace

extern "C" {

__declspec(dllexport) const char* phx_pantalla_version(void) { return "0.1.0-prueba2"; }
// Solo para pruebas fuera de PES (arnes): abre el panel sin esperar la zona 7->49.
__declspec(dllexport) void phx_pantalla_prueba_abrir(void) { pant::g_abierta = true; pant::g_tAbierta = GetTickCount64(); }

}

static int iniciarInterno(const char* carpeta) {
    g_carpeta = carpeta ? carpeta : "";
    std::string off = g_carpeta + "PHOENIX_PANTALLA_OFF";
    if (GetFileAttributesA(off.c_str()) != INVALID_FILE_ATTRIBUTES) return 0;
    if (!g_log) g_log = fopen((g_carpeta + "phoenix_pantalla.log").c_str(), "ab");
    g_base = reinterpret_cast<uint8_t*>(GetModuleHandleA(nullptr));
    logf("==== Phoenix Pantalla 0.1.0-prueba2 · exe base %p · F7 abre/cierra a mano en el menu principal, F8 cierra", (void*)g_base);
    if (InterlockedCompareExchange(&g_hiloVivo, 1, 0) == 0 && !g_enganchado) {
        HANDLE h = CreateThread(nullptr, 0, hiloEnganche, nullptr, 0, nullptr);
        if (!h) { InterlockedExchange(&g_hiloVivo, 0); logf("no se pudo crear el hilo de enganche"); return -1; }
        CloseHandle(h);
    }
    return 1;
}

extern "C" {

// Devuelve 1 si arranca, 0 si esta apagada (PHOENIX_PANTALLA_OFF), -1 si hubo un error.
__declspec(dllexport) int phx_pantalla_iniciar(const char* carpeta) {
    __try { return iniciarInterno(carpeta); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
}

}
