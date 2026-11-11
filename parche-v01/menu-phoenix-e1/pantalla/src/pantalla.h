// Phoenix Core · pantalla propia (0.5.67, PRUEBA 2): dibuja un panel con texto sobre el menu principal al detectar la zona 7 -> 49.
//
// Diseno: pes-menus\DISENO_PANTALLA_PROPIA_CORE.md (secciones 2, 5 y 6). Sin librerias: HLSL compilado con d3dcompiler_47.dll (de Windows),
// texto con un atlas hecho por GDI (fuente de Windows) y el mismo esquema de guardar/restaurar estado DX11 que nitidez.h.
//
// Que hace en esta prueba:
//  - Apagada por defecto: json funcion «pantalla_phoenix_activa» (true para encender). Con PHOENIX_CORE_OFF no hace nada (lo decide el Core).
//  - Dispara cuando la zona del juego (zonaId()) pasa de 7 (TOP_MENU) a 49 (ONLINE_SETTINGS): es lo que ocurre al pulsar «Phoenix»
//    en Ajustes cuando el evento 20 hace ida y vuelta por TopMenu/TopMenu.
//  - Dibuja: velo oscuro + panel + titulo + 3 lineas de texto. NO toca el flujo del juego ni las pulsaciones.
//  - Se cierra con F8, o sola a los 20 s (red de seguridad), o si la zona deja de ser 7/49 (por ejemplo, al entrar a un partido).
//  - Prueba manual sin depender de la zona: F7 abre/cierra el panel cuando el juego esta en el menu principal (zona 7).
//  - Cualquier fallo (shader, textura, excepcion) la apaga sola y lo anota en phoenix_core.log como [pantalla]. Nunca debe tumbar PES.
//
// Este archivo se incluye DENTRO del espacio de nombres anonimo de phoenix_core.cpp, despues de zonaId()/logf y ANTES de nitidez.h
// (el gancho de Present de nitidez.h llama a pant::dibujarSeguro).

namespace pant {

volatile LONG g_on = 0;          // 1 = la funcion esta encendida (hay que vigilar la zona)
volatile LONG g_apagadoSolo = 0;
bool g_abierta = false;
ULONGLONG g_tAbierta = 0;
int g_prevZona = -1;
int g_abierturas = 0;

ID3D11Device* g_dev = nullptr;
ID3D11VertexShader* g_vs = nullptr;
ID3D11PixelShader* g_ps = nullptr;
ID3D11Buffer* g_cb = nullptr;
ID3D11BlendState* g_bs = nullptr;
ID3D11RasterizerState* g_rs = nullptr;
ID3D11DepthStencilState* g_dss = nullptr;
ID3D11SamplerState* g_smp = nullptr;
ID3D11Texture2D* g_atlasTex = nullptr;
ID3D11ShaderResourceView* g_atlasSrv = nullptr;

// Atlas: caracteres 32..255 (Latin-1: incluye a e i o u n u con acento, signos de apertura) en una rejilla de 16 columnas.
const int kAtlasW = 512, kAtlasH = 512, kCol = 16, kCelW = 32, kCelH = 36, kPrimero = 32, kUltimo = 255;
int g_anchoCar[256];

template <class T> void libera(T*& p) { if (p) { p->Release(); p = nullptr; } }

void liberarTodo() {
    libera(g_atlasSrv); libera(g_atlasTex); libera(g_smp); libera(g_dss); libera(g_rs); libera(g_bs);
    libera(g_cb); libera(g_ps); libera(g_vs); libera(g_dev);
}

void apagaSola(const char* motivo) {
    InterlockedExchange(&g_apagadoSolo, 1); g_abierta = false;
    logf("[pantalla] APAGADA SOLA: %s", motivo);
}

static const char* kHlsl =
    "cbuffer C : register(b0) { float4 rect; float4 color; float4 uv; float2 pantalla; float modo; float pad; };\n"
    "Texture2D atlas : register(t0); SamplerState smp : register(s0);\n"
    "struct VO { float4 p : SV_Position; float2 t : TEXCOORD0; };\n"
    "VO VS(uint id : SV_VertexID) {\n"
    "  float2 c = float2(id & 1, id >> 1);\n"
    "  float2 px = rect.xy + c * rect.zw;\n"
    "  VO o; o.p = float4(px.x / pantalla.x * 2.0 - 1.0, 1.0 - px.y / pantalla.y * 2.0, 0.0, 1.0);\n"
    "  o.t = uv.xy + c * uv.zw; return o; }\n"
    "float4 PS(VO i) : SV_Target {\n"
    "  if (modo < 0.5) return color;\n"
    "  float a = atlas.Sample(smp, i.t).r;\n"
    "  return float4(color.rgb, color.a * a); }\n";

typedef HRESULT(WINAPI* CompFn)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

// Hace el atlas con GDI. Devuelve los pixeles R8 (kAtlasW*kAtlasH) o vector vacio si falla.
std::vector<uint8_t> hacerAtlas() {
    std::vector<uint8_t> out;
    HDC dc = CreateCompatibleDC(nullptr); if (!dc) return out;
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth = kAtlasW; bi.bmiHeader.biHeight = -kAtlasH;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr; HBITMAP bm = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bm || !bits) { DeleteDC(dc); return out; }
    HGDIOBJ viejoBm = SelectObject(dc, bm);
    HFONT f = CreateFontW(-26, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HGDIOBJ viejoF = f ? SelectObject(dc, f) : nullptr;
    memset(bits, 0, (size_t)kAtlasW * kAtlasH * 4);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255)); SetTextAlign(dc, TA_LEFT | TA_TOP);
    memset(g_anchoCar, 0, sizeof g_anchoCar);
    for (int c = kPrimero; c <= kUltimo; ++c) {
        int i = c - kPrimero, cx = (i % kCol) * kCelW, cy = (i / kCol) * kCelH;
        wchar_t w = (wchar_t)c; int ancho = 0;
        if (!GetCharWidth32W(dc, w, w, &ancho) || ancho <= 0 || ancho > kCelW - 2) ancho = 14;
        g_anchoCar[c] = ancho;
        TextOutW(dc, cx + 1, cy + 2, &w, 1);
    }
    GdiFlush();
    out.resize((size_t)kAtlasW * kAtlasH);
    const uint8_t* p = (const uint8_t*)bits;
    for (size_t k = 0; k < out.size(); ++k) out[k] = p[k * 4 + 1];   // canal G del texto blanco = cobertura
    if (viejoF) SelectObject(dc, viejoF);
    if (f) DeleteObject(f);
    SelectObject(dc, viejoBm); DeleteObject(bm); DeleteDC(dc);
    return out;
}

bool iniciarRecursos(ID3D11Device* dev) {
    HMODULE m = LoadLibraryA("d3dcompiler_47.dll");
    CompFn comp = m ? (CompFn)GetProcAddress(m, "D3DCompile") : nullptr;
    if (!comp) { apagaSola("no hay d3dcompiler_47.dll"); return false; }
    ID3DBlob *bv = nullptr, *bp = nullptr, *err = nullptr;
    HRESULT hr = comp(kHlsl, strlen(kHlsl), "pantalla.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &bv, &err);
    if (FAILED(hr) || !bv) { char b[300]; snprintf(b, sizeof b, "VS no compila: %s", err ? (char*)err->GetBufferPointer() : "?"); apagaSola(b); libera(err); return false; }
    libera(err);
    hr = comp(kHlsl, strlen(kHlsl), "pantalla.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &bp, &err);
    if (FAILED(hr) || !bp) { char b[300]; snprintf(b, sizeof b, "PS no compila: %s", err ? (char*)err->GetBufferPointer() : "?"); apagaSola(b); libera(err); libera(bv); return false; }
    libera(err);
    bool ok = SUCCEEDED(dev->CreateVertexShader(bv->GetBufferPointer(), bv->GetBufferSize(), nullptr, &g_vs))
           && SUCCEEDED(dev->CreatePixelShader(bp->GetBufferPointer(), bp->GetBufferSize(), nullptr, &g_ps));
    libera(bv); libera(bp);
    if (!ok) { apagaSola("no se pudieron crear los shaders"); return false; }
    D3D11_BUFFER_DESC bd = {}; bd.ByteWidth = 64; bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(dev->CreateBuffer(&bd, nullptr, &g_cb))) { apagaSola("no se pudo crear el buffer constante"); return false; }
    D3D11_RASTERIZER_DESC rd = {}; rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
    D3D11_BLEND_DESC bl = {}; bl.RenderTarget[0].BlendEnable = TRUE;
    bl.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; bl.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; bl.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; bl.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO; bl.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    D3D11_DEPTH_STENCIL_DESC dd = {}; dd.DepthEnable = FALSE; dd.StencilEnable = FALSE;
    D3D11_SAMPLER_DESC sd = {}; sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; sd.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(dev->CreateRasterizerState(&rd, &g_rs)) || FAILED(dev->CreateBlendState(&bl, &g_bs)) || FAILED(dev->CreateDepthStencilState(&dd, &g_dss)) || FAILED(dev->CreateSamplerState(&sd, &g_smp))) {
        apagaSola("no se pudieron crear los estados"); return false;
    }
    std::vector<uint8_t> px = hacerAtlas();
    if (px.empty()) { apagaSola("no se pudo hacer el atlas de fuente con GDI"); return false; }
    D3D11_TEXTURE2D_DESC td = {}; td.Width = kAtlasW; td.Height = kAtlasH; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8_UNORM; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init = { px.data(), (UINT)kAtlasW, 0 };
    if (FAILED(dev->CreateTexture2D(&td, &init, &g_atlasTex)) || FAILED(dev->CreateShaderResourceView(g_atlasTex, nullptr, &g_atlasSrv))) { apagaSola("no se pudo crear la textura del atlas"); return false; }
    g_dev = dev; dev->AddRef();
    logf("[pantalla] recursos listos (atlas %dx%d, caracteres %d..%d)", kAtlasW, kAtlasH, kPrimero, kUltimo);
    return true;
}

struct CB { float rect[4]; float color[4]; float uv[4]; float pant[2]; float modo; float pad; };

void quad(ID3D11DeviceContext* ctx, float W, float H, float x, float y, float w, float h, const float col[4], const float* uv, float modo) {
    CB c = {}; c.rect[0] = x; c.rect[1] = y; c.rect[2] = w; c.rect[3] = h; memcpy(c.color, col, 16);
    if (uv) memcpy(c.uv, uv, 16);
    c.pant[0] = W; c.pant[1] = H; c.modo = modo;
    D3D11_MAPPED_SUBRESOURCE mp;
    if (SUCCEEDED(ctx->Map(g_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp))) { memcpy(mp.pData, &c, sizeof c); ctx->Unmap(g_cb, 0); }
    ctx->Draw(4, 0);
}

// Texto Latin-1 (cada byte = un caracter 32..255). Devuelve el ancho dibujado.
float texto(ID3D11DeviceContext* ctx, float W, float H, float x, float y, float escala, const float col[4], const char* s) {
    float cx = x;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        int c = *p; if (c < kPrimero || c > kUltimo) continue;
        int i = c - kPrimero, ax = (i % kCol) * kCelW, ay = (i / kCol) * kCelH;
        float uv[4] = { ax / (float)kAtlasW, ay / (float)kAtlasH, kCelW / (float)kAtlasW, kCelH / (float)kAtlasH };
        if (c != 32) quad(ctx, W, H, cx, y, kCelW * escala, kCelH * escala, col, uv, 1.0f);
        cx += (g_anchoCar[c] > 0 ? g_anchoCar[c] : 14) * escala;
    }
    return cx - x;
}

bool primerPlano() { DWORD pid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &pid); return pid == GetCurrentProcessId(); }

// Lee zona y teclas, decide abrir/cerrar. Una vez por Present.
void actualizarEstado() {
    int z = zonaId();
    ULONGLONG ahora = GetTickCount64();
    if (z != g_prevZona) {
        if (g_prevZona == 7 && z == 49 && !g_abierta) {
            g_abierta = true; g_tAbierta = ahora; ++g_abierturas;
            logf("[pantalla] ABIERTA por el flanco de zona 7 -> 49 (apertura %d)", g_abierturas);
        }
        g_prevZona = z;
    }
    static bool a7 = false, a8 = false;
    bool k7 = false, k8 = false;
    if (primerPlano()) { k7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0; k8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0; }
    if (k7 && !a7) {
        if (g_abierta) { g_abierta = false; logf("[pantalla] cerrada con F7"); }
        else if (z == 7) { g_abierta = true; g_tAbierta = ahora; ++g_abierturas; logf("[pantalla] ABIERTA a mano con F7 en el menu principal (apertura %d)", g_abierturas); }
    }
    if (k8 && !a8 && g_abierta) { g_abierta = false; logf("[pantalla] cerrada con F8"); }
    a7 = k7; a8 = k8;
    if (g_abierta) {
        if (ahora - g_tAbierta > 20000) { g_abierta = false; logf("[pantalla] cerrada sola a los 20 s (red de seguridad)"); }
        else if (z != 7 && z != 49 && z >= 0) { g_abierta = false; logf("[pantalla] cerrada: la zona cambio a %d", z); }
    }
}

bool dibujar(IDXGISwapChain* sc) {
    ID3D11Device* dev = nullptr;
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&dev)) || !dev) return false;
    if (dev != g_dev) { liberarTodo(); if (!iniciarRecursos(dev)) { dev->Release(); return false; } }
    dev->Release();
    ID3D11DeviceContext* ctx = nullptr; g_dev->GetImmediateContext(&ctx);
    if (!ctx) return false;
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb) { ctx->Release(); return false; }
    D3D11_TEXTURE2D_DESC d; bb->GetDesc(&d);
    if (d.SampleDesc.Count != 1 || !(d.Format == DXGI_FORMAT_R8G8B8A8_UNORM || d.Format == DXGI_FORMAT_B8G8R8A8_UNORM || d.Format == DXGI_FORMAT_R10G10B10A2_UNORM)) {
        char b[160]; snprintf(b, sizeof b, "formato no compatible (fmt %d, muestras %u)", (int)d.Format, d.SampleDesc.Count);
        apagaSola(b); bb->Release(); ctx->Release(); return false;
    }
    ID3D11RenderTargetView* rtv = nullptr;
    if (FAILED(g_dev->CreateRenderTargetView(bb, nullptr, &rtv)) || !rtv) { bb->Release(); ctx->Release(); return false; }

    // ---- guardar estado (como nitidez.h, mas constantes del VS y sampler del PS)
    ID3D11InputLayout* sIL = nullptr; D3D11_PRIMITIVE_TOPOLOGY sTop; ctx->IAGetInputLayout(&sIL); ctx->IAGetPrimitiveTopology(&sTop);
    ID3D11VertexShader* sVS = nullptr; ID3D11PixelShader* sPS = nullptr; ID3D11GeometryShader* sGS = nullptr; ID3D11HullShader* sHS = nullptr; ID3D11DomainShader* sDS = nullptr;
    ctx->VSGetShader(&sVS, nullptr, nullptr); ctx->PSGetShader(&sPS, nullptr, nullptr); ctx->GSGetShader(&sGS, nullptr, nullptr); ctx->HSGetShader(&sHS, nullptr, nullptr); ctx->DSGetShader(&sDS, nullptr, nullptr);
    ID3D11ShaderResourceView* sSRV = nullptr; ctx->PSGetShaderResources(0, 1, &sSRV);
    ID3D11Buffer* sCB = nullptr; ctx->PSGetConstantBuffers(0, 1, &sCB);
    ID3D11Buffer* sVCB = nullptr; ctx->VSGetConstantBuffers(0, 1, &sVCB);
    ID3D11SamplerState* sSmp = nullptr; ctx->PSGetSamplers(0, 1, &sSmp);
    ID3D11RenderTargetView* sRTV[1] = { nullptr }; ID3D11DepthStencilView* sDSV = nullptr; ctx->OMGetRenderTargets(1, sRTV, &sDSV);
    ID3D11BlendState* sBS = nullptr; FLOAT sBF[4]; UINT sSM = 0; ctx->OMGetBlendState(&sBS, sBF, &sSM);
    ID3D11DepthStencilState* sDSS = nullptr; UINT sSR = 0; ctx->OMGetDepthStencilState(&sDSS, &sSR);
    ID3D11RasterizerState* sRS = nullptr; ctx->RSGetState(&sRS);
    D3D11_VIEWPORT sVP[16]; UINT nVP = 16; ctx->RSGetViewports(&nVP, sVP);

    // ---- dibujar
    const float W = (float)d.Width, H = (float)d.Height, s = H / 1080.0f;
    D3D11_VIEWPORT vp = { 0, 0, W, H, 0.0f, 1.0f };
    FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->IASetInputLayout(nullptr); ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ctx->VSSetShader(g_vs, nullptr, 0); ctx->PSSetShader(g_ps, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0); ctx->HSSetShader(nullptr, nullptr, 0); ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_cb); ctx->PSSetConstantBuffers(0, 1, &g_cb);
    ctx->PSSetShaderResources(0, 1, &g_atlasSrv); ctx->PSSetSamplers(0, 1, &g_smp);
    ctx->OMSetRenderTargets(1, &rtv, nullptr); ctx->OMSetBlendState(g_bs, bf, 0xFFFFFFFF); ctx->OMSetDepthStencilState(g_dss, 0);
    ctx->RSSetState(g_rs); ctx->RSSetViewports(1, &vp);

    const float velo[4] = { 0.02f, 0.04f, 0.10f, 0.55f };
    const float panel[4] = { 0.09f, 0.15f, 0.30f, 0.95f };
    const float barra[4] = { 0.95f, 0.95f, 0.98f, 1.0f };
    const float blanco[4] = { 1, 1, 1, 1 }, oscuro[4] = { 0.07f, 0.10f, 0.20f, 1.0f }, gris[4] = { 0.78f, 0.82f, 0.92f, 1.0f };
    quad(ctx, W, H, 0, 0, W, H, velo, nullptr, 0.0f);
    const float pw = 760 * s, ph = 330 * s, px = (W - pw) / 2, py = (H - ph) / 2;
    quad(ctx, W, H, px, py, pw, ph, panel, nullptr, 0.0f);
    quad(ctx, W, H, px, py, pw, 64 * s, barra, nullptr, 0.0f);
    texto(ctx, W, H, px + 28 * s, py + 12 * s, 1.15f * s, oscuro, "Phoenix");
    texto(ctx, W, H, px + 28 * s, py + 100 * s, 1.0f * s, blanco, "Pantalla propia de Phoenix Core (prueba 2)");
    texto(ctx, W, H, px + 28 * s, py + 156 * s, 0.9f * s, gris, "Texto con acentos: \xe1\xe9\xed\xf3\xfa \xf1 \xfc \xbf\xa1");
    texto(ctx, W, H, px + 28 * s, py + 212 * s, 0.9f * s, gris, "F8 = cerrar   (se cierra sola a los 20 s)");
    texto(ctx, W, H, px + 28 * s, py + 262 * s, 0.8f * s, gris, "Esto solo dibuja: no cambia el juego.");

    // ---- restaurar estado
    ID3D11ShaderResourceView* nul = nullptr; ctx->PSSetShaderResources(0, 1, &nul);
    ctx->OMSetRenderTargets(1, sRTV, sDSV);
    ctx->OMSetBlendState(sBS, sBF, sSM); ctx->OMSetDepthStencilState(sDSS, sSR); ctx->RSSetState(sRS);
    if (nVP) ctx->RSSetViewports(nVP, sVP);
    ctx->IASetInputLayout(sIL); ctx->IASetPrimitiveTopology(sTop);
    ctx->VSSetShader(sVS, nullptr, 0); ctx->PSSetShader(sPS, nullptr, 0); ctx->GSSetShader(sGS, nullptr, 0); ctx->HSSetShader(sHS, nullptr, 0); ctx->DSSetShader(sDS, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &sSRV); ctx->PSSetConstantBuffers(0, 1, &sCB); ctx->VSSetConstantBuffers(0, 1, &sVCB); ctx->PSSetSamplers(0, 1, &sSmp);
    libera(sIL); libera(sVS); libera(sPS); libera(sGS); libera(sHS); libera(sDS); libera(sSRV); libera(sCB); libera(sVCB); libera(sSmp);
    libera(sRTV[0]); libera(sDSV); libera(sBS); libera(sDSS); libera(sRS);
    rtv->Release(); bb->Release(); ctx->Release();
    return true;
}

int dibujarSeguro(IDXGISwapChain* sc) {
    if (!g_on || g_apagadoSolo) return 0;
    __try {
        actualizarEstado();
        if (!g_abierta) return 0;
        return dibujar(sc) ? 1 : 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { g_abierta = false; apagaSola("excepcion al dibujar"); return -1; }
}

}  // namespace pant
