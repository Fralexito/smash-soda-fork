// Arnes de prueba de Phoenix Pantalla FUERA de PES: ventana + swapchain propia, carga la DLL, abre el panel y guarda un BMP del backbuffer.
// Uso: arnes.exe <ruta\phoenix_pantalla.dll> <carpeta con \ final> <salida.bmp>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <vector>
typedef int (*IniFn)(const char*); typedef void (*AbrirFn)(void);
LRESULT CALLBACK wp(HWND h, UINT m, WPARAM w, LPARAM l) { return DefWindowProcA(h, m, w, l); }
int main(int argc, char** argv) {
    if (argc < 4) { puts("uso: arnes dll carpeta salida.bmp"); return 2; }
    WNDCLASSA wc = {}; wc.lpfnWndProc = wp; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "ArnesPant"; RegisterClassA(&wc);
    const int W = 1280, H = 720;
    HWND w = CreateWindowA("ArnesPant", "arnes", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 50, 50, W + 16, H + 39, nullptr, nullptr, wc.hInstance, nullptr);
    DXGI_SWAP_CHAIN_DESC sd = {}; sd.BufferCount = 1; sd.BufferDesc.Width = W; sd.BufferDesc.Height = H; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = w; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    IDXGISwapChain* sc = nullptr; ID3D11Device* dev = nullptr; ID3D11DeviceContext* ctx = nullptr; D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx);
    if (FAILED(hr)) { printf("sin swapchain hr=%08X\n", (unsigned)hr); return 3; }
    HMODULE dll = LoadLibraryA(argv[1]); if (!dll) { puts("no carga la dll"); return 4; }
    IniFn ini = (IniFn)GetProcAddress(dll, "phx_pantalla_iniciar"); AbrirFn abrir = (AbrirFn)GetProcAddress(dll, "phx_pantalla_prueba_abrir");
    if (!ini || !abrir) { puts("faltan exports"); return 5; }
    printf("iniciar -> %d\n", ini(argv[2])); Sleep(3000);
    ID3D11Texture2D* bb = nullptr; sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    ID3D11RenderTargetView* rtv = nullptr; dev->CreateRenderTargetView(bb, nullptr, &rtv);
    // dos estados "del juego" que deben sobrevivir al dibujo: un viewport raro y un blend/RTV puestos
    D3D11_VIEWPORT vp0 = { 0, 0, (FLOAT)W, (FLOAT)H, 0, 1 }; ctx->RSSetViewports(1, &vp0); ctx->OMSetRenderTargets(1, &rtv, nullptr);
    abrir();
    for (int i = 0; i < 5; ++i) { FLOAT c[4] = { 0.30f, 0.45f, 0.20f, 1 }; ctx->ClearRenderTargetView(rtv, c); sc->Present(0, 0); Sleep(30); }
    D3D11_VIEWPORT vp1; UINT n = 1; ctx->RSGetViewports(&n, &vp1);
    ID3D11RenderTargetView* r1 = nullptr; ctx->OMGetRenderTargets(1, &r1, nullptr);
    printf("estado restaurado: viewport %.0fx%.0f, rtv %s\n", vp1.Width, vp1.Height, r1 == rtv ? "igual" : "DISTINTO");
    D3D11_TEXTURE2D_DESC d; bb->GetDesc(&d); d.BindFlags = 0; d.MiscFlags = 0; d.Usage = D3D11_USAGE_STAGING; d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* st = nullptr; dev->CreateTexture2D(&d, nullptr, &st); ctx->CopyResource(st, bb);
    D3D11_MAPPED_SUBRESOURCE m; if (FAILED(ctx->Map(st, 0, D3D11_MAP_READ, 0, &m))) { puts("no mapea"); return 6; }
    std::vector<unsigned char> bgr((size_t)W * H * 3);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) { const unsigned char* p = (const unsigned char*)m.pData + (size_t)y * m.RowPitch + x * 4; unsigned char* o = &bgr[((size_t)(H - 1 - y) * W + x) * 3]; o[0] = p[2]; o[1] = p[1]; o[2] = p[0]; }
    const unsigned char* pc = (const unsigned char*)m.pData + (size_t)(H / 2 + 150) * m.RowPitch + (W / 2) * 4;   // dentro del panel, sin texto
    const unsigned char* pf = (const unsigned char*)m.pData + (size_t)5 * m.RowPitch + 5 * 4;                    // esquina: solo velo
    printf("pixel panel RGB=%d,%d,%d  pixel velo RGB=%d,%d,%d  (fondo sin pantalla seria 77,115,51)\n", pc[0], pc[1], pc[2], pf[0], pf[1], pf[2]);
    FILE* f = fopen(argv[3], "wb"); BITMAPFILEHEADER fh = {}; BITMAPINFOHEADER ih = {}; ih.biSize = sizeof ih; ih.biWidth = W; ih.biHeight = H; ih.biPlanes = 1; ih.biBitCount = 24;
    fh.bfType = 0x4D42; fh.bfOffBits = sizeof fh + sizeof ih; fh.bfSize = fh.bfOffBits + (DWORD)bgr.size(); fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f); fwrite(bgr.data(), 1, bgr.size(), f); fclose(f);
    puts("BMP guardado"); return 0;
}
