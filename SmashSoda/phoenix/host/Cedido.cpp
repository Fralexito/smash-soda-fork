#include "Cedido.h"

#include <windows.h>
#include <tlhelp32.h>

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <thread>

#include "../core/ModoCedido.h"
#include "../../services/InputControlService.h"

namespace phoenix::cedido {

	namespace {
		std::atomic<bool> gActivo{ false };
		std::atomic<uint32_t> gInvitado{ 0 };
		std::atomic<int64_t> gDesde{ 0 };
		std::atomic<unsigned> gGeneracion{ 0 };   ///< cambia en cada empezar/terminar: el hilo de entrada reinicia su filtro
		std::atomic<bool> gAtajo{ false };
		std::atomic<bool> gFallo{ false };
		std::atomic<bool> gGuardianVivo{ false };

		std::mutex gMx;                     ///< protege lo de abajo
		Cesion gCfg;
		std::string gMotivoFallo;
		std::set<DWORD> gHijos;             ///< procesos que cuelgan de Link (WebView2, overlay)
		std::map<DWORD, Ventana> gCache;    ///< pid → clase (se vacía al refrescar gHijos)
		std::thread gGuardian;

		// Teclas físicas del anfitrión (solo las mira el gancho)
		std::atomic<bool> gCtrlFis{ false }, gAltFis{ false };

		int64_t cedEpochMs() {
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}

		std::string cedUtf8(const std::wstring& w) {
			if (w.empty()) return std::string();
			const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
			std::string s(static_cast<size_t>((std::max)(n, 0)), '\0');
			if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
			return s;
		}

		std::wstring cedAncho(const std::string& s) {
			if (s.empty()) return std::wstring();
			const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
			std::wstring w(static_cast<size_t>((std::max)(n, 0)), L'\0');
			if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], n);
			return w;
		}

		std::string rutaDeProceso(DWORD pid) {
			HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
			if (h == nullptr) return std::string();
			wchar_t buf[MAX_PATH * 2] = {};
			DWORD n = static_cast<DWORD>(sizeof(buf) / sizeof(buf[0]));
			std::string r;
			if (QueryFullProcessImageNameW(h, 0, buf, &n)) r = cedUtf8(std::wstring(buf, n));
			CloseHandle(h);
			return r;
		}

		/// Descendientes del proceso de Link (para reconocer sus WebView2).
		std::set<DWORD> buscarHijos() {
			std::set<DWORD> hijos;
			HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snap == INVALID_HANDLE_VALUE) return hijos;
			std::multimap<DWORD, DWORD> porPadre;
			PROCESSENTRY32W pe = {};
			pe.dwSize = sizeof(pe);
			if (Process32FirstW(snap, &pe)) {
				do { porPadre.emplace(pe.th32ParentProcessID, pe.th32ProcessID); } while (Process32NextW(snap, &pe));
			}
			CloseHandle(snap);
			std::vector<DWORD> pendientes{ GetCurrentProcessId() };
			while (!pendientes.empty() && hijos.size() < 512) {
				const DWORD p = pendientes.back();
				pendientes.pop_back();
				auto r = porPadre.equal_range(p);
				for (auto it = r.first; it != r.second; ++it) {
					if (it->second != p && hijos.insert(it->second).second) pendientes.push_back(it->second);
				}
			}
			return hijos;
		}

		/// Clase de un proceso (con caché). Llamar con gMx tomado.
		Ventana clasePidBloqueado(DWORD pid) {
			if (pid == 0) return Ventana::Nada;
			auto it = gCache.find(pid);
			if (it != gCache.end()) return it->second;
			const std::string ruta = rutaDeProceso(pid);
			const std::string nombre = nombreExe(ruta);
			const bool esLink = pid == GetCurrentProcessId()
				|| nombre == "phoenixglass.exe" || nombre == "smashglass.exe"
				|| (gHijos.count(pid) > 0 && nombre == "msedgewebview2.exe");
			const Ventana v = clasificar(ruta, esLink, gCfg.carpetaJuego, gCfg.exeJuego, gCfg.extras);
			if (gCache.size() > 256) gCache.clear();
			gCache[pid] = v;
			return v;
		}

		Ventana claseVentana(HWND h) {
			if (h == nullptr) return Ventana::Nada;
			DWORD pid = 0;
			GetWindowThreadProcessId(h, &pid);
			HWND raiz = GetAncestor(h, GA_ROOTOWNER);
			DWORD pidRaiz = 0;
			if (raiz != nullptr) GetWindowThreadProcessId(raiz, &pidRaiz);
			std::lock_guard<std::mutex> l(gMx);
			const Ventana propia = clasePidBloqueado(pid);
			if (propia == Ventana::Pes || propia == Ventana::Link) return propia;
			// Ventana emergente de otro proceso pero colgada de PES/Link (lista desplegable de WebView2…)
			if (pidRaiz != 0 && pidRaiz != pid) {
				const Ventana r = clasePidBloqueado(pidRaiz);
				if (r == Ventana::Pes || r == Ventana::Link) return r;
			}
			return propia;
		}

		struct BuscaPes { HWND hallada = nullptr; };
		BOOL CALLBACK alEnumerar(HWND h, LPARAM p) {
			if (!IsWindowVisible(h) || GetWindow(h, GW_OWNER) != nullptr) return TRUE;
			if (claseVentana(h) == Ventana::Pes) {
				reinterpret_cast<BuscaPes*>(p)->hallada = h;
				return FALSE;
			}
			return TRUE;
		}
		HWND ventanaPes() {
			BuscaPes b;
			EnumWindows(alEnumerar, reinterpret_cast<LPARAM>(&b));
			return b.hallada;
		}

		void traerAlFrente(HWND objetivo) {
			if (objetivo == nullptr || !IsWindow(objetivo)) return;
			if (IsIconic(objetivo)) ShowWindow(objetivo, SW_RESTORE);
			HWND fg = GetForegroundWindow();
			const DWORD hiloFg = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
			const DWORD yo = GetCurrentThreadId();
			const bool unido = hiloFg != 0 && hiloFg != yo && AttachThreadInput(yo, hiloFg, TRUE);
			SetForegroundWindow(objetivo);
			BringWindowToTop(objetivo);
			if (unido) AttachThreadInput(yo, hiloFg, FALSE);
		}

		void bucleGuardian() {
			try {
				ULONGLONG ultimoHijos = 0, ultimoRescate = 0;
				while (gGuardianVivo.load()) {
					const ULONGLONG ahora = GetTickCount64();
					if (ahora - ultimoHijos >= 2000) {
						std::set<DWORD> hijos = buscarHijos();
						std::lock_guard<std::mutex> l(gMx);
						gHijos.swap(hijos);
						gCache.clear();
						ultimoHijos = ahora;
					}
					const ULONGLONG fisico = InputControlService::instance().lastHostPhysicalInputMs();
					const uint64_t desdeFisico = fisico == 0 || ahora < fisico ? UINT64_MAX : ahora - fisico;
					const Ventana foco = claseVentana(GetForegroundWindow());
					if (debeRecuperarFoco(foco, desdeFisico) && ahora - ultimoRescate >= 1000) {
						ultimoRescate = ahora;
						HWND destino = ventanaPes();
						if (destino == nullptr) {
							std::lock_guard<std::mutex> l(gMx);
							destino = static_cast<HWND>(gCfg.ventanaLink);
						}
						traerAlFrente(destino);
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(250));
				}
			}
			catch (const std::exception& e) {
				std::lock_guard<std::mutex> l(gMx);
				gMotivoFallo = e.what();
				gFallo = true;
			}
			catch (...) {
				std::lock_guard<std::mutex> l(gMx);
				gMotivoFallo = "error inesperado del guardián";
				gFallo = true;
			}
		}

		// Filtro del hilo de entrada (solo lo toca ese hilo)
		Filtro gFiltro;
		unsigned gGenFiltro = ~0u;
		void filtroAlDia() {
			const unsigned g = gGeneracion.load();
			if (g != gGenFiltro) { gFiltro.reiniciar(); gGenFiltro = g; }
		}
	}

	bool activo() { return gActivo.load(); }
	uint32_t invitado() { return gInvitado.load(); }
	int64_t desdeMs() { return gDesde.load(); }
	std::string nombreInvitado() { std::lock_guard<std::mutex> l(gMx); return gCfg.nombre; }

	void empezar(const Cesion& c) {
		terminar();
		{
			std::lock_guard<std::mutex> l(gMx);
			gCfg = c;
			gCache.clear();
			gMotivoFallo.clear();
		}
		gFallo = false;
		gAtajo = false;
		gInvitado = c.parsecId;
		gDesde = cedEpochMs();
		gGeneracion++;
		gActivo = true;
		gGuardianVivo = true;
		gGuardian = std::thread(bucleGuardian);
	}

	void terminar() {
		const bool estaba = gActivo.exchange(false);
		gGuardianVivo = false;
		if (gGuardian.joinable()) gGuardian.join();
		gGeneracion++;
		gInvitado = 0;
		if (estaba) InputControlService::instance().soltarTodo();   // nada queda pulsado
	}

	bool tomarFallo(std::string& motivo) {
		if (!gFallo.exchange(false)) return false;
		std::lock_guard<std::mutex> l(gMx);
		motivo = gMotivoFallo;
		return true;
	}

	bool tomarAtajo() { return gAtajo.exchange(false); }

	bool anfitrionTocando(uint64_t ventanaMs) {
		const ULONGLONG fisico = InputControlService::instance().lastHostPhysicalInputMs();
		const ULONGLONG ahora = GetTickCount64();
		return fisico != 0 && ahora >= fisico && ahora - fisico <= ventanaMs;
	}

	void teclaFisica(unsigned long vk, bool pulsada) {
		switch (vk) {
		case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: gCtrlFis = pulsada; return;
		case VK_MENU: case VK_LMENU: case VK_RMENU: gAltFis = pulsada; return;
		case VK_F12: if (pulsada && gCtrlFis.load() && gAltFis.load() && gActivo.load()) gAtajo = true; return;
		default: return;
		}
	}

	bool dejarTecla(uint16_t vk, bool pulsada) {
		filtroAlDia();
		return gFiltro.tecla(vk, pulsada, claseVentana(GetForegroundWindow()));
	}

	bool dejarBoton(int boton, bool pulsado) {
		filtroAlDia();
		Ventana bajo = Ventana::Nada;
		if (pulsado) {   // al soltar no hace falta mirar: el filtro recuerda qué pasó
			POINT p = {};
			if (GetCursorPos(&p)) bajo = claseVentana(WindowFromPoint(p));
		}
		return gFiltro.boton(boton, pulsado, bajo);
	}

	bool dejarRueda() {
		POINT p = {};
		if (!GetCursorPos(&p)) return false;
		return gFiltro.rueda(claseVentana(WindowFromPoint(p)));
	}

	bool pesAbierto(const std::string& carpetaJuego, const std::string& exeJuego) {
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snap == INVALID_HANDLE_VALUE) return false;
		bool hay = false;
		PROCESSENTRY32W pe = {};
		pe.dwSize = sizeof(pe);
		const std::string exe = nombreExe(exeJuego);
		if (Process32FirstW(snap, &pe)) {
			do {
				const std::string n = nombreExe(cedUtf8(pe.szExeFile));
				if (n == "pes2021.exe" || (!exe.empty() && n == exe)) { hay = true; break; }
			} while (Process32NextW(snap, &pe));
		}
		CloseHandle(snap);
		(void)carpetaJuego;
		return hay;
	}

	bool abrirPes(const std::string& ruta, const std::string& parametros, const std::string& carpetaJuego, std::string& error) {
		if (pesAbierto(carpetaJuego, ruta)) {
			HWND h = ventanaPes();
			if (h != nullptr) traerAlFrente(h);
			return true;
		}
		if (ruta.empty()) { error = "No hay juego elegido en la biblioteca (Sala › Juegos)."; return false; }
		const std::wstring w = cedAncho(ruta);
		const DWORD attr = GetFileAttributesW(w.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) { error = "No se encuentra " + ruta; return false; }
		std::wstring carpeta = w;
		const size_t b = carpeta.find_last_of(L"\\/");
		carpeta = b == std::wstring::npos ? std::wstring() : carpeta.substr(0, b);
		std::wstring linea = L"\"" + w + L"\"";
		if (!parametros.empty()) linea += L" " + cedAncho(parametros);
		STARTUPINFOW si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = {};
		if (!CreateProcessW(w.c_str(), &linea[0], nullptr, nullptr, FALSE, 0, nullptr,
				carpeta.empty() ? nullptr : carpeta.c_str(), &si, &pi)) {
			error = "Windows no dejó abrir el juego (código " + std::to_string(GetLastError()) + ").";
			return false;
		}
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return true;
	}

}
