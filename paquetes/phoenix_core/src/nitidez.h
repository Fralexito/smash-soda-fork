// Phoenix Core · nitidez (0.5.0): filtro de nitidez adaptativa por contraste (CAS) sobre la imagen final.
//
// Crédito: el algoritmo es AMD FidelityFX CAS (Contrast Adaptive Sharpening, licencia MIT). Aquí está reescrito desde la
// fórmula pública (ver CREDITOS.txt, sección Phoenix Core). No se descarga ni se enlaza nada de AMD.
//
// Cómo funciona:
//  - Se engancha IDXGISwapChain::Present por la vtable (índice 8), igual que Sider. Sider ya parcheó esa vtable
//    («Hooking Present»): leemos la entrada actual (= el gancho de Sider), la guardamos como «siguiente» y ponemos el
//    nuestro delante. Así el filtro se aplica ANTES de que Sider dibuje su overlay (el overlay queda sin filtrar) y,
//    después, llamamos a Sider, que llama al Present real. Si Sider se engancha después de nosotros, nos encadena él.
//  - En cada Present (si está encendido): copia el backbuffer a una textura propia, dibuja un triángulo de pantalla
//    completa con el pixel shader CAS sobre el backbuffer, y restaura el estado del contexto DX11.
//  - Solo actúa sobre la imagen final (incluye el HUD del juego: no se puede separar antes del HUD).
//  - Formatos compatibles: R8G8B8A8_UNORM, B8G8R8A8_UNORM, R10G10B10A2_UNORM, sin MSAA. Con otro formato se apaga solo
//    y lo anota en el log. Cualquier fallo (shader que no compila, excepción) lo apaga solo: nunca debe tumbar PES.
//  - Teclas (con el juego en primer plano): Ctrl+F10 enciende/apaga; Ctrl+F11 baja y Ctrl+F12 sube la fuerza (pasos de 0,1).
//  - json: funcion «nitidez_activa» (por defecto false) y «nitidez_fuerza» (0.0–1.0, por defecto 0.5).
//
// Este archivo se incluye DENTRO del espacio de nombres anónimo de phoenix_core.cpp (después de Feature y logf).

namespace nit {

typedef HRESULT(STDMETHODCALLTYPE* PresentFn)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(WINAPI* D3DCompileFn)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
typedef HRESULT(WINAPI* CrearDispFn)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
                                      const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

volatile LONG g_on = 0;          // 1 = filtrando
float g_fuerza = 0.5f;           // 0..1
PresentFn g_sig = nullptr;       // lo que había en la vtable antes de nosotros (el gancho de Sider o el Present real)
void** g_vt = nullptr;           // vtable compartida de IDXGISwapChain
bool g_enganchado = false;
volatile LONG g_apagadoSolo = 0; // 1 = se desactivó por un fallo (ver log)
int g_dibujados = 0;

ID3D11Device* g_dev = nullptr;
ID3D11VertexShader* g_vs = nullptr;
ID3D11PixelShader* g_ps = nullptr;
ID3D11Buffer* g_cb = nullptr;
ID3D11RasterizerState* g_rs = nullptr;
ID3D11BlendState* g_bs = nullptr;
ID3D11DepthStencilState* g_dss = nullptr;
ID3D11Texture2D* g_copia = nullptr;
ID3D11ShaderResourceView* g_srv = nullptr;
UINT g_w = 0, g_h = 0;
DXGI_FORMAT g_fmt = DXGI_FORMAT_UNKNOWN;

template <class T> void liberar(T*& p) { if (p) { p->Release(); p = nullptr; } }

void liberarTodo() {
    liberar(g_srv); liberar(g_copia); liberar(g_dss); liberar(g_bs); liberar(g_rs);
    liberar(g_cb); liberar(g_ps); liberar(g_vs); liberar(g_dev);
    g_w = g_h = 0; g_fmt = DXGI_FORMAT_UNKNOWN;
}

// CAS (FidelityFX, MIT) · pasada única sin escalado. Fuerza 0..1 en el buffer constante.
const char* kHlsl = R"(
cbuffer C : register(b0) { float4 p; };   // p.x = fuerza 0..1
Texture2D<float4> T : register(t0);
struct V { float4 pos : SV_Position; };
V VS(uint id : SV_VertexID) {
    V o; float2 uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1); return o;
}
float3 L(int2 q, int2 d, int2 mx) { return T.Load(int3(clamp(q + d, int2(0, 0), mx), 0)).rgb; }
float4 PS(V i) : SV_Target {
    uint w, h; T.GetDimensions(w, h);
    int2 mx = int2(w - 1, h - 1); int2 q = int2(i.pos.xy);
    float4 e4 = T.Load(int3(clamp(q, int2(0, 0), mx), 0));
    float3 a = L(q, int2(-1, -1), mx), b = L(q, int2(0, -1), mx), c = L(q, int2(1, -1), mx);
    float3 d = L(q, int2(-1, 0), mx),  e = e4.rgb,               f = L(q, int2(1, 0), mx);
    float3 g = L(q, int2(-1, 1), mx),  hh = L(q, int2(0, 1), mx), ii = L(q, int2(1, 1), mx);
    float3 mn = min(min(min(d, e), min(f, b)), hh);
    float3 mn2 = min(mn, min(min(a, c), min(g, ii))); mn += mn2;
    float3 mx3 = max(max(max(d, e), max(f, b)), hh);
    float3 mx2 = max(mx3, max(max(a, c), max(g, ii))); mx3 += mx2;
    float3 amp = saturate(min(mn, 2.0 - mx3) / max(mx3, 1e-5));
    float3 wgt = -sqrt(amp) / (8.0 - 3.0 * saturate(p.x));
    float3 rcpW = 1.0 / (1.0 + 4.0 * wgt);
    float3 win = (b + d) + (f + hh);
    return float4(saturate((win * wgt + e) * rcpW), e4.a);
}
)";

bool formatoValido(DXGI_FORMAT f) {
    return f == DXGI_FORMAT_R8G8B8A8_UNORM || f == DXGI_FORMAT_B8G8R8A8_UNORM || f == DXGI_FORMAT_R10G10B10A2_UNORM;
}

void apagarSolo(const char* motivo) {
    InterlockedExchange(&g_on, 0);
    InterlockedExchange(&g_apagadoSolo, 1);
    logf("[nitidez] APAGADA SOLA: %s", motivo);
}

// Crea shaders y estados para el dispositivo del juego. false = no se pudo (ya anotado en el log).
bool iniciarRecursos(ID3D11Device* dev) {
    HMODULE m = LoadLibraryA("d3dcompiler_47.dll");
    D3DCompileFn comp = m ? (D3DCompileFn)GetProcAddress(m, "D3DCompile") : nullptr;
    if (!comp) { apagarSolo("no hay d3dcompiler_47.dll"); return false; }
    ID3DBlob *bv = nullptr, *bp = nullptr, *err = nullptr;
    HRESULT hr = comp(kHlsl, strlen(kHlsl), "nitidez.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &bv, &err);
    if (FAILED(hr) || !bv) { char b[300]; snprintf(b, sizeof b, "VS no compila: %s", err ? (char*)err->GetBufferPointer() : "?"); apagarSolo(b); liberar(err); return false; }
    liberar(err);
    hr = comp(kHlsl, strlen(kHlsl), "nitidez.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &bp, &err);
    if (FAILED(hr) || !bp) { char b[300]; snprintf(b, sizeof b, "PS no compila: %s", err ? (char*)err->GetBufferPointer() : "?"); apagarSolo(b); liberar(err); liberar(bv); return false; }
    liberar(err);
    bool ok = SUCCEEDED(dev->CreateVertexShader(bv->GetBufferPointer(), bv->GetBufferSize(), nullptr, &g_vs))
           && SUCCEEDED(dev->CreatePixelShader(bp->GetBufferPointer(), bp->GetBufferSize(), nullptr, &g_ps));
    liberar(bv); liberar(bp);
    D3D11_BUFFER_DESC bd = {}; bd.ByteWidth = 16; bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ok = ok && SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &g_cb));
    D3D11_RASTERIZER_DESC rd = {}; rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
    ok = ok && SUCCEEDED(dev->CreateRasterizerState(&rd, &g_rs));
    D3D11_BLEND_DESC bld = {}; bld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ok = ok && SUCCEEDED(dev->CreateBlendState(&bld, &g_bs));
    D3D11_DEPTH_STENCIL_DESC dd = {}; dd.DepthEnable = FALSE; dd.StencilEnable = FALSE;
    ok = ok && SUCCEEDED(dev->CreateDepthStencilState(&dd, &g_dss));
    if (!ok) { apagarSolo("no se pudieron crear los recursos DX11"); return false; }
    g_dev = dev; dev->AddRef();
    logf("[nitidez] recursos creados para el dispositivo %p", (void*)dev);
    return true;
}

// Una pasada del filtro sobre el backbuffer de `sc`. Sin objetos C++ con destructor (se llama dentro de __try).
bool aplicar(IDXGISwapChain* sc) {
    ID3D11Device* dev = nullptr;
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&dev)) || !dev) return false;
    if (dev != g_dev) { liberarTodo(); if (!iniciarRecursos(dev)) { dev->Release(); return false; } }
    dev->Release();
    ID3D11DeviceContext* ctx = nullptr; g_dev->GetImmediateContext(&ctx);
    if (!ctx) return false;
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb) { ctx->Release(); return false; }
    D3D11_TEXTURE2D_DESC d; bb->GetDesc(&d);
    if (d.SampleDesc.Count != 1 || !formatoValido(d.Format)) {
        char b[160]; snprintf(b, sizeof b, "formato no compatible (fmt %d, muestras %u, %ux%u)", (int)d.Format, d.SampleDesc.Count, d.Width, d.Height);
        apagarSolo(b); bb->Release(); ctx->Release(); return false;
    }
    if (!g_copia || d.Width != g_w || d.Height != g_h || d.Format != g_fmt) {
        liberar(g_srv); liberar(g_copia);
        D3D11_TEXTURE2D_DESC cd = d; cd.BindFlags = D3D11_BIND_SHADER_RESOURCE; cd.MiscFlags = 0; cd.CPUAccessFlags = 0; cd.Usage = D3D11_USAGE_DEFAULT; cd.MipLevels = 1; cd.ArraySize = 1;
        if (FAILED(g_dev->CreateTexture2D(&cd, nullptr, &g_copia)) || FAILED(g_dev->CreateShaderResourceView(g_copia, nullptr, &g_srv))) {
            apagarSolo("no se pudo crear la textura de copia"); bb->Release(); ctx->Release(); return false;
        }
        g_w = d.Width; g_h = d.Height; g_fmt = d.Format;
        logf("[nitidez] backbuffer %ux%u formato %d (compatible): filtro listo", g_w, g_h, (int)g_fmt);
    }
    ID3D11RenderTargetView* rtv = nullptr;
    if (FAILED(g_dev->CreateRenderTargetView(bb, nullptr, &rtv)) || !rtv) { bb->Release(); ctx->Release(); return false; }

    // ---- guardar estado
    ID3D11InputLayout* sIL = nullptr; D3D11_PRIMITIVE_TOPOLOGY sTop; ctx->IAGetInputLayout(&sIL); ctx->IAGetPrimitiveTopology(&sTop);
    ID3D11VertexShader* sVS = nullptr; ID3D11PixelShader* sPS = nullptr; ID3D11GeometryShader* sGS = nullptr; ID3D11HullShader* sHS = nullptr; ID3D11DomainShader* sDS = nullptr;
    ctx->VSGetShader(&sVS, nullptr, nullptr); ctx->PSGetShader(&sPS, nullptr, nullptr); ctx->GSGetShader(&sGS, nullptr, nullptr); ctx->HSGetShader(&sHS, nullptr, nullptr); ctx->DSGetShader(&sDS, nullptr, nullptr);
    ID3D11ShaderResourceView* sSRV = nullptr; ctx->PSGetShaderResources(0, 1, &sSRV);
    ID3D11Buffer* sCB = nullptr; ctx->PSGetConstantBuffers(0, 1, &sCB);
    ID3D11RenderTargetView* sRTV[1] = { nullptr }; ID3D11DepthStencilView* sDSV = nullptr; ctx->OMGetRenderTargets(1, sRTV, &sDSV);
    ID3D11BlendState* sBS = nullptr; FLOAT sBF[4]; UINT sSM = 0; ctx->OMGetBlendState(&sBS, sBF, &sSM);
    ID3D11DepthStencilState* sDSS = nullptr; UINT sSR = 0; ctx->OMGetDepthStencilState(&sDSS, &sSR);
    ID3D11RasterizerState* sRS = nullptr; ctx->RSGetState(&sRS);
    D3D11_VIEWPORT sVP[16]; UINT nVP = 16; ctx->RSGetViewports(&nVP, sVP);

    // ---- dibujar
    ctx->CopyResource(g_copia, bb);
    D3D11_MAPPED_SUBRESOURCE mp;
    if (SUCCEEDED(ctx->Map(g_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp))) { float* v = (float*)mp.pData; v[0] = g_fuerza; v[1] = v[2] = v[3] = 0; ctx->Unmap(g_cb, 0); }
    D3D11_VIEWPORT vp = { 0, 0, (FLOAT)d.Width, (FLOAT)d.Height, 0.0f, 1.0f };
    FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->IASetInputLayout(nullptr); ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_vs, nullptr, 0); ctx->PSSetShader(g_ps, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0); ctx->HSSetShader(nullptr, nullptr, 0); ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &g_srv); ctx->PSSetConstantBuffers(0, 1, &g_cb);
    ctx->OMSetRenderTargets(1, &rtv, nullptr); ctx->OMSetBlendState(g_bs, bf, 0xFFFFFFFF); ctx->OMSetDepthStencilState(g_dss, 0);
    ctx->RSSetState(g_rs); ctx->RSSetViewports(1, &vp);
    ctx->Draw(3, 0);

    // ---- restaurar estado
    ID3D11ShaderResourceView* nul = nullptr; ctx->PSSetShaderResources(0, 1, &nul);
    ctx->OMSetRenderTargets(1, sRTV, sDSV);
    ctx->OMSetBlendState(sBS, sBF, sSM); ctx->OMSetDepthStencilState(sDSS, sSR); ctx->RSSetState(sRS);
    if (nVP) ctx->RSSetViewports(nVP, sVP);
    ctx->IASetInputLayout(sIL); ctx->IASetPrimitiveTopology(sTop);
    ctx->VSSetShader(sVS, nullptr, 0); ctx->PSSetShader(sPS, nullptr, 0); ctx->GSSetShader(sGS, nullptr, 0); ctx->HSSetShader(sHS, nullptr, 0); ctx->DSSetShader(sDS, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &sSRV); ctx->PSSetConstantBuffers(0, 1, &sCB);
    liberar(sIL); liberar(sVS); liberar(sPS); liberar(sGS); liberar(sHS); liberar(sDS); liberar(sSRV); liberar(sCB);
    liberar(sRTV[0]); liberar(sDSV); liberar(sBS); liberar(sDSS); liberar(sRS);
    rtv->Release(); bb->Release(); ctx->Release();
    ++g_dibujados;
    return true;
}

int aplicarSeguro(IDXGISwapChain* sc) {
    __try { return aplicar(sc) ? 1 : 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
}

bool primerPlano() {
    DWORD pid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &pid); return pid == GetCurrentProcessId();
}

void teclas() {
    static bool a10 = false, a11 = false, a12 = false;
    if (!primerPlano()) return;
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool k10 = ctrl && (GetAsyncKeyState(VK_F10) & 0x8000), k11 = ctrl && (GetAsyncKeyState(VK_F11) & 0x8000), k12 = ctrl && (GetAsyncKeyState(VK_F12) & 0x8000);
    if (k10 && !a10) { LONG n = g_on ? 0 : 1; if (n && g_apagadoSolo) logf("[nitidez] estaba apagada sola; se reintenta"); InterlockedExchange(&g_apagadoSolo, 0); InterlockedExchange(&g_on, n); logf("[nitidez] Ctrl+F10: %s (fuerza %.2f)", n ? "ENCENDIDA" : "APAGADA", g_fuerza); }
    if (k11 && !a11) { g_fuerza = (std::max)(0.0f, g_fuerza - 0.1f); logf("[nitidez] fuerza %.2f", g_fuerza); }
    if (k12 && !a12) { g_fuerza = (std::min)(1.0f, g_fuerza + 0.1f); logf("[nitidez] fuerza %.2f", g_fuerza); }
    a10 = k10; a11 = k11; a12 = k12;
}

HRESULT STDMETHODCALLTYPE ganchoPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!(flags & DXGI_PRESENT_TEST)) {
        teclas();
        if (g_on) {
            int r = aplicarSeguro(sc);
            if (r < 0) apagarSolo("excepción dentro del filtro");
        }
    }
    return g_sig(sc, sync, flags);
}

// Obtiene la vtable de IDXGISwapChain creando un dispositivo y una swapchain mínimos (se destruyen enseguida).
void** vtableSwapChain() {
    HMODULE d3d = GetModuleHandleA("d3d11.dll"); if (!d3d) d3d = LoadLibraryA("d3d11.dll");
    CrearDispFn crear = d3d ? (CrearDispFn)GetProcAddress(d3d, "D3D11CreateDeviceAndSwapChain") : nullptr;
    if (!crear) return nullptr;
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "PhoenixNitidezDummy";
    RegisterClassA(&wc);
    HWND w = CreateWindowA(wc.lpszClassName, "n", WS_OVERLAPPED, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    if (!w) return nullptr;
    DXGI_SWAP_CHAIN_DESC sd = {}; sd.BufferCount = 1; sd.BufferDesc.Width = 64; sd.BufferDesc.Height = 64; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = w; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* sc = nullptr; ID3D11Device* dv = nullptr; ID3D11DeviceContext* cx = nullptr; D3D_FEATURE_LEVEL fl;
    void** vt = nullptr;
    if (SUCCEEDED(crear(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dv, &fl, &cx)) && sc) vt = *(void***)sc;
    liberar(sc); liberar(cx); liberar(dv);
    DestroyWindow(w); UnregisterClassA(wc.lpszClassName, wc.hInstance);
    return vt;
}

bool nitInstalar(Feature& f) {
    if (!g_enganchado) {
        void** vt = vtableSwapChain();
        if (!vt) { f.estado = "no se pudo obtener la vtable de IDXGISwapChain"; return false; }
        DWORD old;
        if (!VirtualProtect(&vt[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) { f.estado = "VirtualProtect falló"; return false; }
        void* actual = vt[8];
        if (actual == (void*)ganchoPresent) { f.estado = "ya enganchado"; }
        else { g_sig = (PresentFn)actual; vt[8] = (void*)ganchoPresent; }
        VirtualProtect(&vt[8], sizeof(void*), old, &old);
        g_vt = vt; g_enganchado = true;
        logf("[nitidez] Present enganchado en vtable %p: siguiente (Sider o el real) = %p", (void*)vt, (void*)g_sig);
    }
    g_apagadoSolo = 0; InterlockedExchange(&g_on, 1);
    f.estado = "activa (Ctrl+F10 enciende/apaga, Ctrl+F11/F12 fuerza)";
    return true;
}

// Apagar solo detiene el filtro; el gancho se queda como paso transparente (si Sider se encadenó detrás de nosotros no se puede quitar sin romperlo).
bool nitQuitar(Feature&) { InterlockedExchange(&g_on, 0); return true; }

}  // namespace nit
