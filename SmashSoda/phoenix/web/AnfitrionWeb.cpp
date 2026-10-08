// Compilado sin pch ni unity build (ver CMakeLists.txt): WebView2.h vive solo aquí.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "WebView2.h"
#include "AnfitrionWeb.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

namespace phoenix::web {

	namespace detalle_anfitrion {

		const wchar_t* kBase = L"https://phoenix.local/";

		/// Manejador COM genérico: implementa la interfaz `I` con una lambda.
		/// (Equivale a Microsoft::WRL::Callback, sin depender de WRL.)
		template <class I, class F, class... A>
		class Manejador final : public I {
		public:
			explicit Manejador(F f) : _fn(std::move(f)) {}
			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
				if (ppv == nullptr) return E_POINTER;
				if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(I))) {
					*ppv = static_cast<I*>(this);
					AddRef();
					return S_OK;
				}
				*ppv = nullptr;
				return E_NOINTERFACE;
			}
			ULONG STDMETHODCALLTYPE AddRef() override { return ++_ref; }
			ULONG STDMETHODCALLTYPE Release() override {
				const ULONG r = --_ref;
				if (r == 0) delete this;
				return r;
			}
			HRESULT STDMETHODCALLTYPE Invoke(A... a) override {
				try { return _fn(a...); }
				catch (...) { return S_OK; } // nunca dejar escapar una excepción hacia COM
			}
		private:
			std::atomic<ULONG> _ref{ 1 };
			F _fn;
		};

		/// Crea el manejador (con 1 referencia: quien lo pasa a WebView2 la suelta luego).
		template <class I, class... A, class F>
		I* manejador(F f) { return new Manejador<I, F, A...>(std::move(f)); }

		/// Suelta un puntero COM y lo deja en nullptr.
		template <class T>
		void soltar(T*& p) {
			if (p != nullptr) { p->Release(); p = nullptr; }
		}

		std::string aUtf8(const std::wstring& w) {
			if (w.empty()) return std::string();
			const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
			std::string s(static_cast<size_t>(n), '\0');
			WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
			return s;
		}

		std::wstring aAncho(const std::string& s) {
			if (s.empty()) return std::wstring();
			const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
			std::wstring w(static_cast<size_t>(n), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
			return w;
		}

		std::wstring tomarTexto(LPWSTR p) {
			std::wstring r = p != nullptr ? std::wstring(p) : std::wstring();
			if (p != nullptr) CoTaskMemFree(p);
			return r;
		}

		std::string hex(HRESULT hr) {
			char b[16];
			std::snprintf(b, sizeof(b), "0x%08lX", static_cast<unsigned long>(hr));
			return b;
		}

		std::wstring carpetaExe() {
			wchar_t ruta[MAX_PATH] = {};
			const DWORD n = GetModuleFileNameW(nullptr, ruta, MAX_PATH);
			std::wstring r(ruta, n);
			const size_t barra = r.find_last_of(L"\\/");
			return barra == std::wstring::npos ? std::wstring(L".") : r.substr(0, barra);
		}

		const wchar_t* tipoMime(const std::wstring& ruta) {
			const size_t punto = ruta.find_last_of(L'.');
			std::wstring ext = punto == std::wstring::npos ? L"" : ruta.substr(punto + 1);
			std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
			if (ext == L"html") return L"text/html; charset=utf-8";
			if (ext == L"css") return L"text/css; charset=utf-8";
			if (ext == L"js" || ext == L"mjs") return L"text/javascript; charset=utf-8";
			if (ext == L"json") return L"application/json; charset=utf-8";
			if (ext == L"svg") return L"image/svg+xml";
			if (ext == L"png") return L"image/png";
			if (ext == L"jpg" || ext == L"jpeg") return L"image/jpeg";
			if (ext == L"webp") return L"image/webp";
			if (ext == L"ico") return L"image/x-icon";
			if (ext == L"ttf") return L"font/ttf";
			if (ext == L"otf") return L"font/otf";
			if (ext == L"woff") return L"font/woff";
			if (ext == L"woff2") return L"font/woff2";
			if (ext == L"wav") return L"audio/wav";
			if (ext == L"mp3") return L"audio/mpeg";
			if (ext == L"ogg") return L"audio/ogg";
			return L"application/octet-stream";
		}

		/// Ruta pedida → ruta relativa segura (solo letras, números y - _ . /, sin «..»).
		bool rutaSegura(const std::wstring& pedida, std::wstring& salida) {
			if (pedida.empty()) { salida = L"index.html"; return true; }
			if (pedida.size() > 200 || pedida[0] == L'/' || pedida.find(L"..") != std::wstring::npos) return false;
			for (wchar_t c : pedida) {
				const bool ok = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9')
					|| c == L'-' || c == L'_' || c == L'.' || c == L'/';
				if (!ok) return false;
			}
			salida = pedida;
			std::replace(salida.begin(), salida.end(), L'/', L'\\');
			return true;
		}

		bool leerArchivo(const std::wstring& ruta, std::string& datos) {
			std::ifstream f(ruta.c_str(), std::ios::binary);
			if (!f) return false;
			f.seekg(0, std::ios::end);
			const std::streamoff tam = f.tellg();
			if (tam < 0 || tam > 32 * 1024 * 1024) return false; // nada razonable de la interfaz pesa más
			datos.resize(static_cast<size_t>(tam));
			f.seekg(0, std::ios::beg);
			if (tam > 0) f.read(&datos[0], tam);
			return f.good() || f.eof();
		}

		/// Abre un enlace externo en el navegador del usuario (solo https).
		void abrirFuera(const std::wstring& uri) {
			if (uri.rfind(L"https://", 0) != 0 || uri.rfind(kBase, 0) == 0) return;
			ShellExecuteW(nullptr, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		}

		using FnCrear = HRESULT(STDAPICALLTYPE*)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
			ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);
		using FnVersion = HRESULT(STDAPICALLTYPE*)(PCWSTR, LPWSTR*);

		/// WebView2Loader.dll se carga una sola vez desde la carpeta del exe.
		struct Cargador {
			HMODULE dll = nullptr;
			FnCrear crear = nullptr;
			FnVersion version = nullptr;
			std::string error;
			bool intentado = false;
		};

		Cargador& cargador() {
			static Cargador c;
			if (!c.intentado) {
				c.intentado = true;
				const std::wstring ruta = carpetaExe() + L"\\WebView2Loader.dll";
				c.dll = LoadLibraryExW(ruta.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (c.dll == nullptr) {
					c.error = "No se encontró WebView2Loader.dll junto a PhoenixLink.exe.";
				}
				else {
					c.crear = reinterpret_cast<FnCrear>(GetProcAddress(c.dll, "CreateCoreWebView2EnvironmentWithOptions"));
					c.version = reinterpret_cast<FnVersion>(GetProcAddress(c.dll, "GetAvailableCoreWebView2BrowserVersionString"));
					if (c.crear == nullptr || c.version == nullptr) c.error = "WebView2Loader.dll está dañado o es de otra versión.";
				}
			}
			return c;
		}

		double segundos() {
			using namespace std::chrono;
			return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
		}
	}

	using namespace detalle_anfitrion;

	// =========================================================================
	struct AnfitrionWeb::Impl : public std::enable_shared_from_this<AnfitrionWeb::Impl> {
		HWND ventana = nullptr;
		ICoreWebView2Environment* entorno = nullptr;
		ICoreWebView2Controller* controlador = nullptr;
		ICoreWebView2* vista = nullptr;
		EventRegistrationToken tMensaje{}, tRecurso{}, tNavInicio{}, tNavFin{}, tVentanaNueva{}, tFallo{}, tTeclas{};

		mutable std::mutex mutex; // estado y error se leen desde otras partes
		EstadoAnfitrion estado = EstadoAnfitrion::Apagado;
		std::string error;
		std::string version;

		std::wstring carpetaUi;
		bool desarrollo = false;
		bool visible = false;
		bool minimizada = false;
		bool cerrado = false;
		double inicio = 0.0;
		int recargasPorFallo = 0;
		double ultimaRecargaPorFallo = 0.0;
		AlMensaje alMensaje;
		std::map<std::wstring, std::string> cache; // archivos ya leídos (fuera del modo desarrollo)

		void cambiar(EstadoAnfitrion e, const std::string& texto = std::string()) {
			std::lock_guard<std::mutex> l(mutex);
			estado = e;
			if (!texto.empty()) error = texto;
		}

		EstadoAnfitrion leerEstado() const {
			std::lock_guard<std::mutex> l(mutex);
			return estado;
		}

		void aplicarVisibilidad() {
			if (controlador != nullptr) controlador->put_IsVisible((visible && !minimizada) ? TRUE : FALSE);
		}

		void ajustar() {
			if (controlador == nullptr || ventana == nullptr) return;
			RECT r{};
			GetClientRect(ventana, &r);
			controlador->put_Bounds(r);
		}

		void liberar() {
			if (vista != nullptr) {
				vista->remove_WebMessageReceived(tMensaje);
				vista->remove_WebResourceRequested(tRecurso);
				vista->remove_NavigationStarting(tNavInicio);
				vista->remove_NavigationCompleted(tNavFin);
				vista->remove_NewWindowRequested(tVentanaNueva);
				vista->remove_ProcessFailed(tFallo);
			}
			if (controlador != nullptr) {
				controlador->remove_AcceleratorKeyPressed(tTeclas);
				controlador->Close();
			}
			soltar(vista);
			soltar(controlador);
			soltar(entorno);
		}

		void servir(ICoreWebView2WebResourceRequestedEventArgs* args) {
			if (args == nullptr || entorno == nullptr) return;
			ICoreWebView2WebResourceRequest* pedido = nullptr;
			if (FAILED(args->get_Request(&pedido)) || pedido == nullptr) return;
			LPWSTR uriBruta = nullptr;
			pedido->get_Uri(&uriBruta);
			pedido->Release();
			std::wstring uri = tomarTexto(uriBruta);
			if (uri.rfind(kBase, 0) != 0) return; // no es nuestro: que siga su camino

			std::wstring ruta = uri.substr(wcslen(kBase));
			const size_t corte = ruta.find_first_of(L"?#");
			if (corte != std::wstring::npos) ruta.resize(corte);

			std::wstring relativa;
			std::string datos;
			bool encontrado = false;
			if (rutaSegura(ruta, relativa)) {
				auto it = cache.find(relativa);
				if (it != cache.end() && !desarrollo) {
					datos = it->second;
					encontrado = true;
				}
				else if (leerArchivo(carpetaUi + L"\\" + relativa, datos)) {
					encontrado = true;
					if (!desarrollo) cache[relativa] = datos;
				}
			}

			ICoreWebView2WebResourceResponse* respuesta = nullptr;
			if (encontrado) {
				IStream* flujo = SHCreateMemStream(reinterpret_cast<const BYTE*>(datos.data()), static_cast<UINT>(datos.size()));
				const std::wstring cabeceras = std::wstring(L"Content-Type: ") + tipoMime(relativa)
					+ L"\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff";
				if (flujo != nullptr) {
					entorno->CreateWebResourceResponse(flujo, 200, L"OK", cabeceras.c_str(), &respuesta);
					flujo->Release();
				}
			}
			if (respuesta == nullptr) {
				entorno->CreateWebResourceResponse(nullptr, 404, L"Not Found", L"Content-Type: text/plain", &respuesta);
			}
			if (respuesta != nullptr) {
				args->put_Response(respuesta);
				respuesta->Release();
			}
		}

		HRESULT alCrearControlador(HRESULT resultado, ICoreWebView2Controller* creado) {
			if (cerrado) return S_OK;
			if (FAILED(resultado) || creado == nullptr) {
				cambiar(EstadoAnfitrion::Fallo, "No se pudo crear la vista web (" + hex(resultado) + ").");
				return S_OK;
			}
			controlador = creado;
			controlador->AddRef();
			if (FAILED(controlador->get_CoreWebView2(&vista)) || vista == nullptr) {
				cambiar(EstadoAnfitrion::Fallo, "La vista web no respondió.");
				liberar();
				return S_OK;
			}

			ICoreWebView2Settings* ajustes = nullptr;
			if (SUCCEEDED(vista->get_Settings(&ajustes)) && ajustes != nullptr) {
				ajustes->put_IsScriptEnabled(TRUE);
				ajustes->put_IsWebMessageEnabled(TRUE);
				ajustes->put_AreDefaultScriptDialogsEnabled(FALSE); // sin alert/confirm nativos
				ajustes->put_IsStatusBarEnabled(FALSE);
				ajustes->put_IsZoomControlEnabled(FALSE);
				ajustes->put_AreDevToolsEnabled(desarrollo ? TRUE : FALSE);
				ajustes->put_AreDefaultContextMenusEnabled(desarrollo ? TRUE : FALSE);
				ajustes->Release();
			}

			std::weak_ptr<Impl> debil = shared_from_this();

			// Mensajes de la página → C++
			auto* hMensaje = manejador<ICoreWebView2WebMessageReceivedEventHandler, ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs*>(
				[debil](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
					auto yo = debil.lock();
					if (!yo || yo->cerrado || args == nullptr) return S_OK;
					LPWSTR bruto = nullptr;
					if (SUCCEEDED(args->get_WebMessageAsJson(&bruto))) {
						const std::string texto = aUtf8(tomarTexto(bruto));
						if (yo->alMensaje && !texto.empty()) yo->alMensaje(texto);
					}
					return S_OK;
				});
			vista->add_WebMessageReceived(hMensaje, &tMensaje);
			hMensaje->Release();

			// Archivos de la interfaz (https://phoenix.local/…) desde la carpeta ui/
			vista->AddWebResourceRequestedFilter(L"https://phoenix.local/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
			auto* hRecurso = manejador<ICoreWebView2WebResourceRequestedEventHandler, ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs*>(
				[debil](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
					auto yo = debil.lock();
					if (yo && !yo->cerrado) yo->servir(args);
					return S_OK;
				});
			vista->add_WebResourceRequested(hRecurso, &tRecurso);
			hRecurso->Release();

			// Solo se navega dentro de la interfaz; lo demás se abre en el navegador del usuario
			auto* hNavInicio = manejador<ICoreWebView2NavigationStartingEventHandler, ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*>(
				[](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
					if (args == nullptr) return S_OK;
					LPWSTR bruto = nullptr;
					args->get_Uri(&bruto);
					const std::wstring uri = tomarTexto(bruto);
					if (uri.rfind(kBase, 0) != 0) {
						args->put_Cancel(TRUE);
						abrirFuera(uri);
					}
					return S_OK;
				});
			vista->add_NavigationStarting(hNavInicio, &tNavInicio);
			hNavInicio->Release();

			auto* hNavFin = manejador<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*>(
				[debil](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
					auto yo = debil.lock();
					if (!yo || yo->cerrado || args == nullptr) return S_OK;
					BOOL ok = FALSE;
					args->get_IsSuccess(&ok);
					if (ok) yo->cambiar(EstadoAnfitrion::Listo);
					else {
						COREWEBVIEW2_WEB_ERROR_STATUS st = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
						args->get_WebErrorStatus(&st);
						yo->cambiar(EstadoAnfitrion::Fallo, "La interfaz no cargó (error " + std::to_string(static_cast<int>(st)) + ").");
					}
					return S_OK;
				});
			vista->add_NavigationCompleted(hNavFin, &tNavFin);
			hNavFin->Release();

			auto* hVentana = manejador<ICoreWebView2NewWindowRequestedEventHandler, ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs*>(
				[](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
					if (args == nullptr) return S_OK;
					args->put_Handled(TRUE); // nunca abrir ventanas emergentes
					LPWSTR bruto = nullptr;
					args->get_Uri(&bruto);
					abrirFuera(tomarTexto(bruto));
					return S_OK;
				});
			vista->add_NewWindowRequested(hVentana, &tVentanaNueva);
			hVentana->Release();

			auto* hFallo = manejador<ICoreWebView2ProcessFailedEventHandler, ICoreWebView2*, ICoreWebView2ProcessFailedEventArgs*>(
				[debil](ICoreWebView2*, ICoreWebView2ProcessFailedEventArgs* args) -> HRESULT {
					auto yo = debil.lock();
					if (!yo || yo->cerrado || args == nullptr) return S_OK;
					COREWEBVIEW2_PROCESS_FAILED_KIND tipo = COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED;
					args->get_ProcessFailedKind(&tipo);
					const double ahora = segundos();
					if (tipo != COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED && yo->vista != nullptr) {
						// La página se colgó o se cerró: se recarga (máx. 3 veces en 2 minutos).
						if (ahora - yo->ultimaRecargaPorFallo > 120.0) yo->recargasPorFallo = 0;
						yo->ultimaRecargaPorFallo = ahora;
						if (++yo->recargasPorFallo <= 3) {
							yo->cambiar(EstadoAnfitrion::Cargando);
							yo->vista->Reload();
							return S_OK;
						}
					}
					yo->cambiar(EstadoAnfitrion::Fallo, "El motor de la interfaz se cerró.");
					return S_OK;
				});
			vista->add_ProcessFailed(hFallo, &tFallo);
			hFallo->Release();

			// Teclas del navegador que no tienen sentido en la app (recargar, imprimir, buscar…)
			auto* hTeclas = manejador<ICoreWebView2AcceleratorKeyPressedEventHandler, ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs*>(
				[debil](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
					auto yo = debil.lock();
					if (!yo || args == nullptr || yo->desarrollo) return S_OK;
					COREWEBVIEW2_KEY_EVENT_KIND tipo = COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN;
					args->get_KeyEventKind(&tipo);
					if (tipo != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN && tipo != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN) return S_OK;
					UINT tecla = 0;
					args->get_VirtualKey(&tecla);
					const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
					const bool bloquear = tecla == VK_F5 || tecla == VK_F7 || tecla == VK_F3 || tecla == VK_F12
						|| (ctrl && (tecla == 'R' || tecla == 'P' || tecla == 'F' || tecla == 'G' || tecla == 'U' || tecla == 'S' || tecla == 'O' || tecla == 'H' || tecla == 'J'));
					if (bloquear) args->put_Handled(TRUE);
					return S_OK;
				});
			controlador->add_AcceleratorKeyPressed(hTeclas, &tTeclas);
			hTeclas->Release();

			ajustar();
			aplicarVisibilidad();
			cambiar(EstadoAnfitrion::Cargando);
			const HRESULT hr = vista->Navigate(L"https://phoenix.local/index.html");
			if (FAILED(hr)) cambiar(EstadoAnfitrion::Fallo, "No se pudo abrir la interfaz (" + hex(hr) + ").");
			return S_OK;
		}

		HRESULT alCrearEntorno(HRESULT resultado, ICoreWebView2Environment* creado) {
			if (cerrado) return S_OK;
			if (FAILED(resultado) || creado == nullptr) {
				cambiar(EstadoAnfitrion::Fallo, "No se pudo iniciar WebView2 (" + hex(resultado) + ").");
				return S_OK;
			}
			entorno = creado;
			entorno->AddRef();
			std::weak_ptr<Impl> debil = shared_from_this();
			auto* h = manejador<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, HRESULT, ICoreWebView2Controller*>(
				[debil](HRESULT r, ICoreWebView2Controller* c) -> HRESULT {
					auto yo = debil.lock();
					return yo ? yo->alCrearControlador(r, c) : S_OK;
				});
			const HRESULT hr = entorno->CreateCoreWebView2Controller(ventana, h);
			h->Release();
			if (FAILED(hr)) cambiar(EstadoAnfitrion::Fallo, "No se pudo crear la vista web (" + hex(hr) + ").");
			return S_OK;
		}
	};

	// =========================================================================
	AnfitrionWeb::AnfitrionWeb() : _impl(std::make_shared<Impl>()) {}

	AnfitrionWeb::~AnfitrionWeb() {
		cerrar();
	}

	std::string AnfitrionWeb::runtimeInstalado() {
		Cargador& c = cargador();
		if (c.version == nullptr) return std::string();
		LPWSTR v = nullptr;
		const HRESULT hr = c.version(nullptr, &v);
		const std::wstring texto = tomarTexto(v);
		return SUCCEEDED(hr) ? aUtf8(texto) : std::string();
	}

	bool AnfitrionWeb::iniciar(void* ventana, const std::wstring& carpetaUi, const std::wstring& carpetaDatos, bool modoDesarrollo) {
		Impl& y = *_impl;
		try {
			if (y.leerEstado() != EstadoAnfitrion::Apagado && y.leerEstado() != EstadoAnfitrion::Fallo) return true;
			y.cerrado = false;
			y.ventana = static_cast<HWND>(ventana);
			y.carpetaUi = carpetaUi;
			y.desarrollo = modoDesarrollo;
			y.inicio = segundos();
			y.cache.clear();

			if (GetFileAttributesW((carpetaUi + L"\\index.html").c_str()) == INVALID_FILE_ATTRIBUTES) {
				y.cambiar(EstadoAnfitrion::Fallo, "Falta la carpeta ui junto a PhoenixLink.exe.");
				return false;
			}

			// WebView2 exige un hilo STA (el hilo de la ventana).
			APTTYPE tipo = APTTYPE_CURRENT;
			APTTYPEQUALIFIER calificador = APTTYPEQUALIFIER_NONE;
			if (CoGetApartmentType(&tipo, &calificador) == CO_E_NOTINITIALIZED) {
				CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			}
			else if (tipo == APTTYPE_MTA) {
				y.cambiar(EstadoAnfitrion::Fallo, "El hilo de la ventana no es STA: WebView2 no puede usarse.");
				return false;
			}

			Cargador& c = cargador();
			if (c.crear == nullptr) {
				y.cambiar(EstadoAnfitrion::Fallo, c.error);
				return false;
			}
			y.version = runtimeInstalado();
			if (y.version.empty()) {
				y.cambiar(EstadoAnfitrion::Fallo, "Falta el componente WebView2 de Microsoft (Edge).");
				return false;
			}

			SHCreateDirectoryExW(nullptr, carpetaDatos.c_str(), nullptr);
			// Fondo oscuro mientras carga (sin destello blanco).
			SetEnvironmentVariableW(L"WEBVIEW2_DEFAULT_BACKGROUND_COLOR", L"FF05060F");

			y.cambiar(EstadoAnfitrion::Creando);
			std::weak_ptr<Impl> debil = _impl;
			auto* h = manejador<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, HRESULT, ICoreWebView2Environment*>(
				[debil](HRESULT r, ICoreWebView2Environment* e) -> HRESULT {
					auto yo = debil.lock();
					return yo ? yo->alCrearEntorno(r, e) : S_OK;
				});
			const HRESULT hr = c.crear(nullptr, carpetaDatos.c_str(), nullptr, h);
			h->Release();
			if (FAILED(hr)) {
				y.cambiar(EstadoAnfitrion::Fallo, "No se pudo iniciar WebView2 (" + hex(hr) + ").");
				return false;
			}
			return true;
		}
		catch (...) {
			y.cambiar(EstadoAnfitrion::Fallo, "Error inesperado al iniciar la interfaz.");
			return false;
		}
	}

	void AnfitrionWeb::cerrar() {
		if (!_impl) return;
		try {
			_impl->cerrado = true;
			_impl->liberar();
			_impl->cambiar(EstadoAnfitrion::Apagado);
		}
		catch (...) {}
	}

	void AnfitrionWeb::ajustar() { try { _impl->ajustar(); } catch (...) {} }

	void AnfitrionWeb::ventanaMovida() {
		try { if (_impl->controlador != nullptr) _impl->controlador->NotifyParentWindowPositionChanged(); }
		catch (...) {}
	}

	void AnfitrionWeb::minimizada(bool si) {
		try {
			_impl->minimizada = si;
			_impl->aplicarVisibilidad();
		}
		catch (...) {}
	}

	void AnfitrionWeb::mostrar(bool si) {
		try {
			if (_impl->visible == si) return;
			_impl->visible = si;
			_impl->ajustar();
			_impl->aplicarVisibilidad();
			if (si) enfocar();
		}
		catch (...) {}
	}

	bool AnfitrionWeb::visible() const { return _impl->visible && _impl->controlador != nullptr; }

	void AnfitrionWeb::enfocar() {
		try { if (_impl->controlador != nullptr) _impl->controlador->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC); }
		catch (...) {}
	}

	void AnfitrionWeb::recargar() {
		try {
			if (_impl->vista == nullptr) return;
			_impl->cache.clear();
			_impl->cambiar(EstadoAnfitrion::Cargando);
			_impl->vista->Reload();
		}
		catch (...) {}
	}

	void AnfitrionWeb::enviar(const std::string& json) {
		try {
			if (_impl->vista == nullptr || _impl->cerrado) return;
			const std::wstring w = aAncho(json);
			_impl->vista->PostWebMessageAsJson(w.c_str());
		}
		catch (...) {}
	}

	void AnfitrionWeb::alMensaje(AlMensaje fn) { _impl->alMensaje = std::move(fn); }

	EstadoAnfitrion AnfitrionWeb::estado() const { return _impl->leerEstado(); }

	std::string AnfitrionWeb::error() const {
		std::lock_guard<std::mutex> l(_impl->mutex);
		return _impl->error;
	}

	std::string AnfitrionWeb::versionRuntime() const { return _impl->version; }

	double AnfitrionWeb::segundosDesdeInicio() const {
		return _impl->inicio > 0.0 ? segundos() - _impl->inicio : 0.0;
	}

}
