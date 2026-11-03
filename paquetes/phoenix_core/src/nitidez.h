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
float g_pielNit = 0.3f;          // json nitidez_piel (0..1): multiplica la nitidez en tono de piel
float g_pielSuav = 0.35f;        // json suavizado_piel (0..1): mezcla del suavizado bilateral solo en piel
bool  g_anisoCfg = true;         // json aniso_activo (por defecto true)
int   g_anisoNivel = 16;         // json aniso_nivel (1..16)
PresentFn g_sig = nullptr;       // lo que había en la vtable antes de nosotros (el gancho de Sider o el Present real)
void** g_vt = nullptr;           // vtable compartida de IDXGISwapChain
bool g_enganchado = false;
volatile LONG g_apagadoSolo = 0; // 1 = se desactivó por un fallo (ver log)
int g_dibujados = 0;
volatile LONG g_anisoOn = 0, g_sVistos = 0, g_sCambiados = 0;   // 0.5.3 aniso (ver más abajo)
void** g_ctxVt = nullptr;

ID3D11Device* g_dev = nullptr;
ID3D11VertexShader* g_vs = nullptr;
ID3D11PixelShader* g_ps = nullptr;
ID3D11Buffer* g_cb = nullptr;
ID3D11RasterizerState* g_rs = nullptr;
ID3D11BlendState* g_bs = nullptr;
ID3D11DepthStencilState* g_dss = nullptr;
ID3D11Texture2D* g_copia = nullptr;
ID3D11ShaderResourceView* g_srv = nullptr;
ID3D11Texture2D* g_stg = nullptr;   // 0.5.2: textura de lectura para la captura de prueba
UINT g_w = 0, g_h = 0;
DXGI_FORMAT g_fmt = DXGI_FORMAT_UNKNOWN;

template <class T> void liberar(T*& p) { if (p) { p->Release(); p = nullptr; } }

void liberarTodo() {
    liberar(g_srv); liberar(g_copia); liberar(g_dss); liberar(g_bs); liberar(g_rs);
    liberar(g_cb); liberar(g_ps); liberar(g_vs); liberar(g_dev); liberar(g_stg);
    g_w = g_h = 0; g_fmt = DXGI_FORMAT_UNKNOWN;
}

// CAS (FidelityFX, MIT) · pasada única sin escalado. Fuerza 0..1 en el buffer constante.
const char* kHlsl = R"(
cbuffer C : register(b0) { float4 p; };   // x = fuerza 0..1, y = nitidez_piel, z = suavizado_piel, w = modo (0 = 0.5.3; 1 = como la 0.5.1, sin proteccion de piel)
Texture2D<float4> T : register(t0);
struct V { float4 pos : SV_Position; };
V VS(uint id : SV_VertexID) {
    V o; float2 uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1); return o;
}
float3 L(int2 q, int2 d, int2 mx) { return T.Load(int3(clamp(q + d, int2(0, 0), mx), 0)).rgb; }
float Y(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }
// Peso de piel (0..1): tono 5-50 grados y saturacion 0.15-0.65, con bordes suaves
float pesoPiel(float3 c) {
    float mx = max(c.r, max(c.g, c.b)), mn = min(c.r, min(c.g, c.b)), dl = mx - mn;
    float s = mx > 1e-4 ? dl / mx : 0.0;
    float h = 0.0;
    if (dl > 1e-5) {
        if (mx == c.r) h = fmod((c.g - c.b) / dl + 6.0, 6.0);
        else if (mx == c.g) h = (c.b - c.r) / dl + 2.0;
        else h = (c.r - c.g) / dl + 4.0;
    }
    h *= 60.0;
    float wh = smoothstep(-3.0, 5.0, h) * (1.0 - smoothstep(50.0, 58.0, h));
    float ws = smoothstep(0.10, 0.15, s) * (1.0 - smoothstep(0.65, 0.72, s));
    return wh * ws;
}
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
    float wp = (p.w < 0.5) ? pesoPiel(e) : 0.0;          // proteccion de piel solo en el modo 0
    wgt *= lerp(1.0, saturate(p.y), wp);                  // nitidez_piel multiplica la nitidez en la piel
    float3 rcpW = 1.0 / (1.0 + 4.0 * wgt);
    float3 win = (b + d) + (f + hh);
    float3 outc = saturate((win * wgt + e) * rcpW);
    if (wp > 0.0 && p.z > 0.0) {                          // suavizado bilateral 5x5 SOLO en piel; tolerancia de bordes por luminancia
        float ye = Y(e); float3 acc = e; float aw = 1.0;
        [unroll] for (int dy = -2; dy <= 2; ++dy) {
            [unroll] for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                float3 n = L(q, int2(dx, dy), mx);
                float dY = Y(n) - ye;
                float wt = exp(-(dx * dx + dy * dy) / 4.5) * exp(-(dY * dY) / 0.005);
                acc += n * wt; aw += wt;
            }
        }
        outc = lerp(outc, acc / aw, saturate(p.z) * wp);
    }
    return float4(outc, e4.a);
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

// ---- captura objetiva (0.5.2): si existe pes-graficos\nitidez\captura.flag, guarda el MISMO frame sin filtro y con filtro (BMP) y mide la diferencia.
const char* kCapDir = "D:\Frank\IA\CLAUDE\pes-graficos\nitidez\\";
UINT g_stgW = 0, g_stgH = 0; DXGI_FORMAT g_stgF = DXGI_FORMAT_UNKNOWN;

bool pidioCaptura() { std::string f = std::string(kCapDir) + "captura.flag"; return GetFileAttributesA(f.c_str()) != INVALID_FILE_ATTRIBUTES; }

// Copia `src` (misma resolución/formato que `d`) a memoria. out = filas de w*4 bytes (formato del backbuffer).
bool leerPixeles(ID3D11DeviceContext* ctx, ID3D11Texture2D* src, const D3D11_TEXTURE2D_DESC& d, std::vector<uint8_t>& out) {
    if (!g_stg || g_stgW != d.Width || g_stgH != d.Height || g_stgF != d.Format) {
        liberar(g_stg);
        D3D11_TEXTURE2D_DESC sd = d; sd.BindFlags = 0; sd.MiscFlags = 0; sd.Usage = D3D11_USAGE_STAGING; sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ; sd.MipLevels = 1; sd.ArraySize = 1;
        if (FAILED(g_dev->CreateTexture2D(&sd, nullptr, &g_stg))) return false;
        g_stgW = d.Width; g_stgH = d.Height; g_stgF = d.Format;
    }
    ctx->CopyResource(g_stg, src);
    D3D11_MAPPED_SUBRESOURCE m;
    if (FAILED(ctx->Map(g_stg, 0, D3D11_MAP_READ, 0, &m))) return false;
    out.resize((size_t)d.Width * d.Height * 4);
    for (UINT y = 0; y < d.Height; ++y) memcpy(&out[(size_t)y * d.Width * 4], (uint8_t*)m.pData + (size_t)y * m.RowPitch, (size_t)d.Width * 4);
    ctx->Unmap(g_stg, 0);
    return true;
}

bool guardarBmp(const std::string& ruta, const std::vector<uint8_t>& px, UINT w, UINT h, DXGI_FORMAT fmt) {
    bool rgba = fmt == DXGI_FORMAT_R8G8B8A8_UNORM;
    std::vector<uint8_t> fila((size_t)w * 3 + 3);
    UINT rowB = (w * 3 + 3) & ~3u; UINT tam = 54 + rowB * h;
    uint8_t cab[54] = { 'B', 'M' }; *(uint32_t*)(cab + 2) = tam; *(uint32_t*)(cab + 10) = 54; *(uint32_t*)(cab + 14) = 40;
    *(int32_t*)(cab + 18) = (int32_t)w; *(int32_t*)(cab + 22) = (int32_t)h; *(uint16_t*)(cab + 26) = 1; *(uint16_t*)(cab + 28) = 24; *(uint32_t*)(cab + 34) = rowB * h;
    HANDLE f = CreateFileA(ruta.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD n; WriteFile(f, cab, 54, &n, nullptr);
    std::vector<uint8_t> buf(rowB, 0);
    for (int y = (int)h - 1; y >= 0; --y) {
        const uint8_t* p = &px[(size_t)y * w * 4];
        for (UINT x = 0; x < w; ++x) { buf[x * 3 + 0] = rgba ? p[x * 4 + 2] : p[x * 4 + 0]; buf[x * 3 + 1] = p[x * 4 + 1]; buf[x * 3 + 2] = rgba ? p[x * 4 + 0] : p[x * 4 + 2]; }
        WriteFile(f, buf.data(), rowB, &n, nullptr);
    }
    CloseHandle(f); return true;
}

// Una pasada de dibujo con el modo dado (0 = 0.5.3, 1 = como la 0.5.1): escribe el buffer constante y dibuja.
void pasada(ID3D11DeviceContext* ctx, float modo) {
    D3D11_MAPPED_SUBRESOURCE mp;
    if (SUCCEEDED(ctx->Map(g_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp))) { float* v = (float*)mp.pData; v[0] = g_fuerza; v[1] = g_pielNit; v[2] = g_pielSuav; v[3] = modo; ctx->Unmap(g_cb, 0); }
    ctx->Draw(3, 0);
}

static bool esPielCpu(const uint8_t* px, bool rgba) {
    float r = (rgba ? px[0] : px[2]) / 255.0f, g = px[1] / 255.0f, b = (rgba ? px[2] : px[0]) / 255.0f;
    float mx = (std::max)(r, (std::max)(g, b)), mn = (std::min)(r, (std::min)(g, b)), dl = mx - mn;
    if (mx < 1e-4f || dl < 1e-5f) return false;
    float s = dl / mx, h;
    if (mx == r) h = fmodf((g - b) / dl + 6.0f, 6.0f); else if (mx == g) h = (b - r) / dl + 2.0f; else h = (r - g) / dl + 4.0f;
    h *= 60.0f; float Y = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    return h >= 5.0f && h <= 50.0f && s >= 0.15f && s <= 0.65f && Y > 0.05f && Y < 0.9f;
}
static void lumas(const std::vector<uint8_t>& v, bool rgba, std::vector<float>& out) {
    size_t n = v.size() / 4; out.resize(n);
    for (size_t i = 0; i < n; ++i) { float r = rgba ? v[i * 4] : v[i * 4 + 2], g = v[i * 4 + 1], b = rgba ? v[i * 4 + 2] : v[i * 4]; out[i] = 0.2126f * r + 0.7152f * g + 0.0722f * b; }
}
// «Grano» = varianza de la alta frecuencia (luminancia menos su media 3x3) en los pixeles de piel de la mascara.
static double granoPiel(const std::vector<float>& Lm, const std::vector<char>& mask, UINT w, UINT h) {
    double sq = 0; size_t n = 0;
    for (UINT y = 1; y + 1 < h; ++y) for (UINT x = 1; x + 1 < w; ++x) {
        size_t i = (size_t)y * w + x; if (!mask[i]) continue;
        float m = 0; for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) m += Lm[i + (size_t)dy * w + dx];
        double hf = Lm[i] - m / 9.0f; sq += hf * hf; ++n;
    }
    return n ? sq / n : 0.0;
}

// Con el estado ya preparado y el frame original en g_copia: dibuja y mide TRES versiones del MISMO frame:
// sin filtro, la 0.5.1 tal cual (modo 1) y la 0.5.3 (modo 0). Guarda 3 BMP y anota diferencias y «grano» de piel en el log.
// Deja en el backbuffer la 0.5.3 (la version que se muestra).
void capturaTres(ID3D11DeviceContext* ctx, ID3D11Texture2D* bb, const D3D11_TEXTURE2D_DESC& d, const std::vector<uint8_t>& raw) {
    DeleteFileA((std::string(kCapDir) + "captura.flag").c_str());
    bool rgba = d.Format == DXGI_FORMAT_R8G8B8A8_UNORM;
    if (!rgba && d.Format != DXGI_FORMAT_B8G8R8A8_UNORM) { logf("[nitidez] captura: formato %d no soportado para BMP", (int)d.Format); pasada(ctx, 0.0f); return; }
    std::vector<uint8_t> v51, v52;
    pasada(ctx, 1.0f); bool ok1 = leerPixeles(ctx, bb, d, v51);
    pasada(ctx, 0.0f); bool ok2 = leerPixeles(ctx, bb, d, v52);
    if (raw.empty() || !ok1 || !ok2) { logf("[nitidez] captura: no se pudo leer algun frame"); return; }
    CreateDirectoryA((std::string(kCapDir) + "capturas").c_str(), nullptr);
    SYSTEMTIME st; GetLocalTime(&st); char tag[64]; snprintf(tag, sizeof tag, "%02d%02d%02d_f%02d", st.wHour, st.wMinute, st.wSecond, (int)(g_fuerza * 100));
    std::string base = std::string(kCapDir) + "capturas\\" + tag;
    bool a = guardarBmp(base + "_1_sin_filtro.bmp", raw, d.Width, d.Height, d.Format);
    bool b = guardarBmp(base + "_2_v051.bmp", v51, d.Width, d.Height, d.Format);
    bool c = guardarBmp(base + "_3_v053.bmp", v52, d.Width, d.Height, d.Format);
    size_t np = (size_t)d.Width * d.Height;
    double d51 = 0, d52 = 0; for (size_t i = 0; i < np; ++i) for (int k = 0; k < 3; ++k) { d51 += abs((int)raw[i * 4 + k] - (int)v51[i * 4 + k]); d52 += abs((int)raw[i * 4 + k] - (int)v52[i * 4 + k]); }
    std::vector<char> mask(np, 0); size_t nPiel = 0;
    for (size_t i = 0; i < np; ++i) if (esPielCpu(&raw[i * 4], rgba)) { mask[i] = 1; ++nPiel; }
    std::vector<float> l0, l1, l2; lumas(raw, rgba, l0); lumas(v51, rgba, l1); lumas(v52, rgba, l2);
    double g0 = granoPiel(l0, mask, d.Width, d.Height), g1 = granoPiel(l1, mask, d.Width, d.Height), g2 = granoPiel(l2, mask, d.Width, d.Height);
    logf("[nitidez] CAPTURA %s %ux%u fuerza %.2f piel %.2f suav %.2f · diferencia media vs sin filtro: v051 %.3f /255, v053 %.3f /255", tag, d.Width, d.Height, g_fuerza, g_pielNit, g_pielSuav, d51 / (np * 3.0), d52 / (np * 3.0));
    logf("[nitidez] CAPTURA GRANO de piel (varianza de alta frecuencia, 0-255 al cuadrado): sin filtro %.3f · v051 %.3f · v053 %.3f · pixeles de piel %zu (%.1f%% del frame) · BMP %d%d%d", g0, g1, g2, nPiel, 100.0 * nPiel / np, (int)a, (int)b, (int)c);
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
    std::vector<uint8_t> raw; bool cap = pidioCaptura(); if (cap && !leerPixeles(ctx, g_copia, d, raw)) raw.clear();
    D3D11_VIEWPORT vp = { 0, 0, (FLOAT)d.Width, (FLOAT)d.Height, 0.0f, 1.0f };
    FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->IASetInputLayout(nullptr); ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_vs, nullptr, 0); ctx->PSSetShader(g_ps, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0); ctx->HSSetShader(nullptr, nullptr, 0); ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &g_srv); ctx->PSSetConstantBuffers(0, 1, &g_cb);
    ctx->OMSetRenderTargets(1, &rtv, nullptr); ctx->OMSetBlendState(g_bs, bf, 0xFFFFFFFF); ctx->OMSetDepthStencilState(g_dss, 0);
    ctx->RSSetState(g_rs); ctx->RSSetViewports(1, &vp);
    if (cap) capturaTres(ctx, bb, d, raw); else pasada(ctx, 0.0f);

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
    static bool a9 = false, a10 = false, a11 = false, a12 = false;
    if (!primerPlano()) return;
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool k10 = ctrl && (GetAsyncKeyState(VK_F10) & 0x8000), k11 = ctrl && (GetAsyncKeyState(VK_F11) & 0x8000), k12 = ctrl && (GetAsyncKeyState(VK_F12) & 0x8000);
    bool k9 = ctrl && (GetAsyncKeyState(VK_F9) & 0x8000);
    if (k9 && !a9) { LONG n = g_anisoOn ? 0 : 1; if (g_ctxVt) { InterlockedExchange(&g_anisoOn, n); logf("[aniso] Ctrl+F9: %s (samplers cambiados %ld; los que ya estan puestos se actualizan cuando PES los vuelve a asignar)", n ? "ON" : "OFF", g_sCambiados); } }
    a9 = k9;
    if (k10 && !a10) { LONG n = g_on ? 0 : 1; if (n && g_apagadoSolo) logf("[nitidez] estaba apagada sola; se reintenta"); InterlockedExchange(&g_apagadoSolo, 0); InterlockedExchange(&g_on, n); logf("[nitidez] Ctrl+F10: %s (fuerza %.2f)", n ? "ENCENDIDA" : "APAGADA", g_fuerza); }
    if (k11 && !a11) { g_fuerza = (std::max)(0.0f, g_fuerza - 0.1f); logf("[nitidez] fuerza %.2f", g_fuerza); }
    if (k12 && !a12) { g_fuerza = (std::min)(1.0f, g_fuerza + 0.1f); logf("[nitidez] fuerza %.2f", g_fuerza); }
    a10 = k10; a11 = k11; a12 = k12;
}

// ================= 0.5.3 · filtro anisotropico forzado =================
// PES crea y asigna sus samplers desde el arranque (antes de que el Core exista), asi que no basta enganchar CreateSamplerState:
// se engancha PSSetSamplers del contexto inmediato REAL (vtable[10], tomada del contexto de la swapchain en el primer Present)
// y cada sampler MIN_MAG_MIP_LINEAR se sustituye por una version ANISOTROPIC (cache por puntero). No se tocan los de
// comparacion (sombras) ni los POINT / mixtos. Ademas se vuelven a asignar los samplers que ya estaban puestos.
typedef void(STDMETHODCALLTYPE* PSSetSamplersFn)(ID3D11DeviceContext*, UINT, UINT, ID3D11SamplerState* const*);
PSSetSamplersFn g_psSetOrig = nullptr; ID3D11Device* g_devAniso = nullptr;
struct EntradaS { ID3D11SamplerState* orig; ID3D11SamplerState* nuevo; };
EntradaS g_cacheS[256];
int g_histoFiltros[8][2]; int g_nHisto = 0;

ID3D11SamplerState* aSampler(ID3D11SamplerState* s) {
    if (!s || !g_devAniso) return s;
    size_t h = ((uintptr_t)s >> 4) & 255;
    for (int i = 0; i < 256; ++i) {
        EntradaS& e = g_cacheS[(h + i) & 255];
        if (e.orig == s) return e.nuevo;
        if (!e.orig) {
            D3D11_SAMPLER_DESC d; s->GetDesc(&d);
            InterlockedIncrement(&g_sVistos);
            ID3D11SamplerState* n = s;
            bool hallado = false; for (int k = 0; k < g_nHisto; ++k) if (g_histoFiltros[k][0] == (int)d.Filter) { ++g_histoFiltros[k][1]; hallado = true; }
            if (!hallado && g_nHisto < 8) { g_histoFiltros[g_nHisto][0] = (int)d.Filter; g_histoFiltros[g_nHisto][1] = 1; ++g_nHisto; }
            if (d.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR) {
                d.Filter = D3D11_FILTER_ANISOTROPIC; d.MaxAnisotropy = (UINT)g_anisoNivel;
                ID3D11SamplerState* t = nullptr;
                if (SUCCEEDED(g_devAniso->CreateSamplerState(&d, &t)) && t) { n = t; InterlockedIncrement(&g_sCambiados); }
            }
            s->AddRef();   // se conserva el original vivo: asi su direccion no se reutiliza mientras este en la cache
            e.orig = s; e.nuevo = n; return n;
        }
    }
    return s;   // tabla llena: se deja pasar
}
ID3D11SamplerState* aSamplerSeguro(ID3D11SamplerState* s) {
    __try { return aSampler(s); }
    __except (EXCEPTION_EXECUTE_HANDLER) { InterlockedExchange(&g_anisoOn, 0); return s; }
}
void STDMETHODCALLTYPE ganchoPSSetSamplers(ID3D11DeviceContext* c, UINT start, UINT num, ID3D11SamplerState* const* pp) {
    if (g_anisoOn && pp && num && num <= 16) {
        ID3D11SamplerState* tmp[16];
        for (UINT i = 0; i < num; ++i) tmp[i] = aSamplerSeguro(pp[i]);
        g_psSetOrig(c, start, num, tmp);
    } else g_psSetOrig(c, start, num, pp);
}
void anisoIniciar(IDXGISwapChain* sc) {
    static int intentos = 0;
    if (g_ctxVt || !g_anisoCfg || intentos >= 3) return;
    ++intentos;
    ID3D11Device* dev = nullptr;
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&dev)) || !dev) { logf("[aniso] intento %d: sin dispositivo", intentos); return; }
    ID3D11DeviceContext* ctx = nullptr; dev->GetImmediateContext(&ctx);
    if (!ctx) { logf("[aniso] intento %d: sin contexto inmediato", intentos); dev->Release(); return; }
    void** vt = *(void***)ctx; DWORD old;
    if (!VirtualProtect(&vt[10], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) { logf("[aniso] VirtualProtect fallo (err %lu)", GetLastError()); ctx->Release(); dev->Release(); return; }
    void* actual = vt[10];
    g_devAniso = dev; dev->AddRef();
    if (actual != (void*)ganchoPSSetSamplers) { g_psSetOrig = (PSSetSamplersFn)actual; vt[10] = (void*)ganchoPSSetSamplers; }
    VirtualProtect(&vt[10], sizeof(void*), old, &old);
    g_ctxVt = vt; InterlockedExchange(&g_anisoOn, 1);
    logf("[aniso] PSSetSamplers enganchado en vtable del contexto %p (original %p); nivel %d", (void*)vt, (void*)g_psSetOrig, g_anisoNivel);
    // volver a asignar los samplers ya puestos (por si PES no los repite)
    ID3D11SamplerState* cur[16] = {}; ctx->PSGetSamplers(0, 16, cur);
    bool alguno = false; for (int i = 0; i < 16; ++i) if (cur[i]) alguno = true;
    if (alguno) ctx->PSSetSamplers(0, 16, cur);   // pasa por nuestro gancho
    for (int i = 0; i < 16; ++i) liberar(cur[i]);
    ctx->Release(); dev->Release();
}

// tiempo de frame: media de 300 frames, anotada con el estado de aniso/nitidez para poder comparar
LARGE_INTEGER g_qpf = {}, g_ultPres = {}; double g_sumDt = 0; int g_nDt = 0;
void medirFrame() {
    LARGE_INTEGER n; QueryPerformanceCounter(&n);
    if (!g_qpf.QuadPart) QueryPerformanceFrequency(&g_qpf);
    if (g_ultPres.QuadPart) {
        double dt = (double)(n.QuadPart - g_ultPres.QuadPart) * 1000.0 / (double)g_qpf.QuadPart;
        if (dt > 0.0 && dt < 500.0) { g_sumDt += dt; ++g_nDt; }
        if (g_nDt >= 300) {
            logf("[nitidez] frame medio %.2f ms (%.1f fps) en %d frames · aniso %s (samplers vistos %ld, cambiados %ld) · nitidez %s fuerza %.2f", g_sumDt / g_nDt, 1000.0 / (g_sumDt / g_nDt), g_nDt, g_anisoOn ? "ON" : "OFF", g_sVistos, g_sCambiados, g_on ? "ON" : "OFF", g_fuerza);
            g_sumDt = 0; g_nDt = 0;
        }
    }
    g_ultPres = n;
}

HRESULT STDMETHODCALLTYPE ganchoPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!(flags & DXGI_PRESENT_TEST)) {
        medirFrame();
        anisoIniciar(sc);
        teclas();
        if (g_on) {
            int r = aplicarSeguro(sc);
            if (r < 0) apagarSolo("excepción dentro del filtro");
        }
    }
    return g_sig(sc, sync, flags);
}

// Obtiene la vtable de IDXGISwapChain creando un dispositivo y una swapchain mínimos (se destruyen enseguida).
// `diag` recibe el motivo si falla (con HRESULT / GetLastError).
void** vtableSwapChain(char* diag, size_t nd) {
    diag[0] = 0;
    HMODULE d3d = GetModuleHandleA("d3d11.dll"); if (!d3d) d3d = LoadLibraryA("d3d11.dll");
    if (!d3d) { snprintf(diag, nd, "no hay d3d11.dll (err %lu)", GetLastError()); return nullptr; }
    CrearDispFn crear = (CrearDispFn)GetProcAddress(d3d, "D3D11CreateDeviceAndSwapChain");
    if (!crear) { snprintf(diag, nd, "no existe D3D11CreateDeviceAndSwapChain"); return nullptr; }
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "PhoenixNitidezDummy";
    RegisterClassA(&wc);   // si ya existe de un intento anterior, no pasa nada
    HWND w = CreateWindowA(wc.lpszClassName, "n", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    if (!w) { snprintf(diag, nd, "CreateWindow falló (err %lu)", GetLastError()); return nullptr; }
    DXGI_SWAP_CHAIN_DESC sd = {}; sd.BufferCount = 1; sd.BufferDesc.Width = 64; sd.BufferDesc.Height = 64; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = w; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    void** vt = nullptr;
    const D3D_DRIVER_TYPE tipos[2] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
    for (int i = 0; i < 2 && !vt; ++i) {
        IDXGISwapChain* sc = nullptr; ID3D11Device* dv = nullptr; ID3D11DeviceContext* cx = nullptr; D3D_FEATURE_LEVEL fl;
        HRESULT hr = crear(nullptr, tipos[i], nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dv, &fl, &cx);
        if (SUCCEEDED(hr) && sc) vt = *(void***)sc;
        else { size_t l = strlen(diag); snprintf(diag + l, nd - l, "%s hr=0x%08X; ", i ? "WARP" : "HARDWARE", (unsigned)hr); }
        liberar(sc); liberar(cx); liberar(dv);
    }
    DestroyWindow(w); UnregisterClassA(wc.lpszClassName, wc.hInstance);
    return vt;
}

void** vtableSegura(char* diag, size_t nd) {
    __try { return vtableSwapChain(diag, nd); }
    __except (EXCEPTION_EXECUTE_HANDLER) { snprintf(diag, nd, "excepción 0x%08lX al crear la swapchain de prueba", (unsigned long)GetExceptionCode()); return nullptr; }
}

volatile LONG g_quiere = 0, g_hiloVivo = 0;

// Engancha Present en un hilo aparte con reintentos (cada 1 s, hasta 2 min): así se espera a que el juego ya tenga su
// ventana/DX11 y no se hace nada pesado dentro del arranque del Core.
DWORD WINAPI hiloEnganche(LPVOID) {
    for (int i = 0; i < 120 && g_quiere && !g_enganchado; ++i) {
        char diag[240];
        void** vt = vtableSegura(diag, sizeof diag);
        if (vt) {
            DWORD old;
            if (!VirtualProtect(&vt[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) { logf("[nitidez] VirtualProtect falló (err %lu)", GetLastError()); break; }
            void* actual = vt[8];
            if (actual != (void*)ganchoPresent) { g_sig = (PresentFn)actual; vt[8] = (void*)ganchoPresent; }
            VirtualProtect(&vt[8], sizeof(void*), old, &old);
            g_vt = vt; g_enganchado = true;
            logf("[nitidez] Present enganchado (intento %d) en vtable %p: siguiente (Sider o el real) = %p", i + 1, (void*)vt, (void*)g_sig);
            InterlockedExchange(&g_on, 1);
            break;
        }
        if (i < 3 || i % 20 == 0) logf("[nitidez] intento %d sin vtable: %s", i + 1, diag);
        Sleep(1000);
    }
    if (!g_enganchado) logf("[nitidez] NO se pudo enganchar Present tras los reintentos; el filtro queda apagado");
    InterlockedExchange(&g_hiloVivo, 0);
    return 0;
}

bool nitInstalar(Feature& f) {
    g_apagadoSolo = 0; InterlockedExchange(&g_quiere, 1);
    if (g_enganchado) { InterlockedExchange(&g_on, 1); f.estado = "activa (ya enganchada)"; return true; }
    if (InterlockedCompareExchange(&g_hiloVivo, 1, 0) == 0) {
        HANDLE h = CreateThread(nullptr, 0, hiloEnganche, nullptr, 0, nullptr);
        if (!h) { InterlockedExchange(&g_hiloVivo, 0); f.estado = "no se pudo crear el hilo de enganche"; return false; }
        CloseHandle(h);
    }
    f.estado = "enganchando Present (reintentos en un hilo; ver [nitidez] en el log); Ctrl+F10 enciende/apaga, Ctrl+F11/F12 fuerza";
    return true;
}

// Apagar solo detiene el filtro; el gancho se queda como paso transparente (si Sider se encadenó detrás de nosotros no se puede quitar sin romperlo).
bool nitQuitar(Feature&) { InterlockedExchange(&g_quiere, 0); InterlockedExchange(&g_on, 0); return true; }

}  // namespace nit
