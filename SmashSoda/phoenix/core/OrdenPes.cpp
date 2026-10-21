#include "OrdenPes.h"
#include "MandosExtra.h"

#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>
#include <cfgmgr32.h>
#include <ShlObj.h>
#include <TlHelp32.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <thread>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "cfgmgr32.lib")

namespace phoenix::ordenpes {

	namespace {
		constexpr size_t kTam = 612;          // settings.dat de PES 2021
		constexpr size_t kCasillas = 0x64;    // primera casilla
		constexpr size_t kCasilla = 64;       // 16 GUID + 48 mapa
		constexpr size_t kMapa = 48;
		// Mapa de botones por defecto de un mando Xbox 360 en Settings de PES 2021 (visto en la PC de Fralex).
		const unsigned char kMapaXbox[kMapa] = {
			0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x03,0x04,0x04,0x06,0x04,0x00,0x04,0x00,0x05,
			0x00,0x07,0x00,0x06,0x00,0x08,0x00,0x09,0x0c,0x00,0x0d,0x00,0x0e,0x00,0x0f,0x00,
			0x06,0x01,0x04,0x01,0x06,0x00,0x04,0x00,0x06,0x03,0x04,0x03,0x06,0x02,0x04,0x02 };

		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<bool> activo{ false };
		bool hiloVivo = false;
		bool pedirYa = false;
		std::vector<unsigned long> series;   // por puesto
		Estado est;
		bool extraCopiado = false;           // phoenix_mandos_extra.dat ya pasó por la copia de esta sesión

		std::wstring rutaSettings() {
			PWSTR docs = nullptr;
			std::wstring r;
			if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) r = docs;
			if (docs) CoTaskMemFree(docs);
			if (r.empty()) return r;
			return r + L"\\KONAMI\\eFootball PES 2021 SEASON UPDATE\\settings.dat";
		}

		bool procesoAbierto(const wchar_t* exe) {
			HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (s == INVALID_HANDLE_VALUE) return false;
			PROCESSENTRY32W p{}; p.dwSize = sizeof(p);
			bool hay = false;
			for (BOOL ok = Process32FirstW(s, &p); ok; ok = Process32NextW(s, &p)) {
				if (_wcsicmp(p.szExeFile, exe) == 0) { hay = true; break; }
			}
			CloseHandle(s);
			return hay;
		}

		bool pesAbierto() {
			HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (s == INVALID_HANDLE_VALUE) return false;
			PROCESSENTRY32W p{}; p.dwSize = sizeof(p);
			bool hay = false;
			for (BOOL ok = Process32FirstW(s, &p); ok; ok = Process32NextW(s, &p)) {
				if (_wcsicmp(p.szExeFile, L"PES2021.exe") == 0 || _wcsicmp(p.szExeFile, L"Settings.exe") == 0) { hay = true; break; }
			}
			CloseHandle(s);
			return hay;
		}

		std::string hora() {
			std::time_t t = std::time(nullptr);
			std::tm tm{}; localtime_s(&tm, &t);
			char b[16]; std::strftime(b, sizeof(b), "%H:%M", &tm);
			return b;
		}

		std::wstring sello() {
			std::time_t t = std::time(nullptr);
			std::tm tm{}; localtime_s(&tm, &t);
			wchar_t b[32]; wcsftime(b, 32, L"%Y%m%d-%H%M%S", &tm);
			return b;
		}

		bool leer(const std::wstring& f, std::vector<unsigned char>& out) {
			std::ifstream in(f, std::ios::binary);
			if (!in) return false;
			out.assign(std::istreambuf_iterator<char>(in), {});
			return true;
		}

		/// Serie ViGEm (USB\VID_045E&PID_028E\<serie>) del mando HID de esta ruta DirectInput; 0 = no es virtual.
		unsigned long serieDeRuta(const std::wstring& ruta) {
			std::wstring s = ruta;
			if (s.rfind(L"\\\\?\\", 0) == 0) s = s.substr(4);
			const size_t ult = s.rfind(L'#');
			if (ult != std::wstring::npos) s = s.substr(0, ult);
			for (auto& c : s) if (c == L'#') c = L'\\';
			DEVINST inst = 0;
			if (CM_Locate_DevNodeW(&inst, const_cast<DEVINSTID_W>(s.c_str()), CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS) return 0;
			const std::wstring pre = L"USB\\VID_045E&PID_028E\\";
			for (int n = 0; n < 6; n++) {
				DEVINST p = 0; wchar_t id[512];
				if (CM_Get_Parent(&p, inst, 0) != CR_SUCCESS || CM_Get_Device_IDW(p, id, 512, 0) != CR_SUCCESS) return 0;
				const std::wstring x = id;
				if (_wcsnicmp(x.c_str(), pre.c_str(), pre.size()) == 0) {
					try { return std::stoul(x.substr(pre.size()), nullptr, 10); }
					catch (...) { return 0; }
				}
				inst = p;
			}
			return 0;
		}

		struct Enum { IDirectInput8W* di; std::map<unsigned long, std::vector<unsigned char>> porSerie; };

		BOOL CALLBACK cadaMando(const DIDEVICEINSTANCEW* d, void* ctx) {
			Enum& e = *static_cast<Enum*>(ctx);
			IDirectInputDevice8W* dev = nullptr;
			if (FAILED(e.di->CreateDevice(d->guidInstance, &dev, nullptr))) return DIENUM_CONTINUE;
			DIPROPGUIDANDPATH p{};
			p.diph.dwSize = sizeof(p); p.diph.dwHeaderSize = sizeof(DIPROPHEADER); p.diph.dwHow = DIPH_DEVICE;
			if (SUCCEEDED(dev->GetProperty(DIPROP_GUIDANDPATH, &p.diph))) {
				const unsigned long serie = serieDeRuta(p.wszPath);
				if (serie) {
					std::vector<unsigned char> g(16);
					std::memcpy(g.data(), &d->guidInstance, 16);
					e.porSerie[serie] = g;
				}
			}
			dev->Release();
			return DIENUM_CONTINUE;
		}

		/// Escribe phoenix_mandos_extra.dat (copia la primera vez de la sesión). Deja el estado en «ordenado» o «error».
		bool escribirExtra(Estado& e, const std::wstring& ruta, const std::vector<unsigned char>& datos) {
			std::string copia, error;
			if (!mandosextra::escribir(ruta, datos, !extraCopiado, copia, error)) { e.estado = "error"; e.error = error; return false; }
			extraCopiado = true;
			if (!copia.empty()) e.copiaExtra = copia;
			e.estado = "ordenado"; e.ultimo = hora();
			for (auto& c : e.casillas) c.ok = true;
			return true;
		}

		void revisar(IDirectInput8W* di, bool& estableAntes, std::vector<std::vector<unsigned char>>& deseadoAntes) {
			std::vector<unsigned long> s;
			{ std::lock_guard<std::mutex> l(mtx); s = series; }
			Estado e; e.activo = true;
			e.ds4windows = procesoAbierto(L"DS4Windows.exe");
			{ std::lock_guard<std::mutex> l(mtx); e.ultimo = est.ultimo; e.copia = est.copia; e.copiaExtra = est.copiaExtra; }

			const std::wstring ruta = rutaSettings();
			std::vector<unsigned char> arch;
			if (ruta.empty() || !leer(ruta, arch) || !esSettingsValido(arch)) {
				e.estado = "sin_settings";
				std::lock_guard<std::mutex> l(mtx); est = e; return;
			}

			Enum en{ di, {} };
			di->EnumDevices(DI8DEVCLASS_GAMECTRL, cadaMando, &en, DIEDFL_ATTACHEDONLY);
			std::vector<std::vector<unsigned char>> deseado(8), deLink;
			// Puestos 9+ (EXPERIMENTAL): van a phoenix_mandos_extra.dat
			std::vector<std::vector<unsigned char>> extra(s.size() > 8 ? s.size() - 8 : 0);
			for (const auto& kv : en.porSerie) deLink.push_back(kv.second);
			bool faltan = false;
			for (size_t i = 0; i < s.size(); i++) {
				if (s[i] == 0) continue;                              // puesto desenchufado: casilla vacía
				auto it = en.porSerie.find(s[i]);
				if (it == en.porSerie.end()) { faltan = true; continue; }
				(i < 8 ? deseado[i] : extra[i - 8]) = it->second;
			}
			for (int n = 1; n <= 8; n++) {
				Casilla c; c.n = n; c.puesto = deseado[n - 1].empty() ? 0 : n;
				const unsigned char* g = &arch[kCasillas + (n - 1) * kCasilla];
				static const unsigned char cero[16] = {};
				c.ok = deseado[n - 1].empty() ? std::memcmp(g, cero, 16) == 0 : std::memcmp(g, deseado[n - 1].data(), 16) == 0;
				e.casillas.push_back(c);
			}
			bool todoOk = true;
			for (const auto& c : e.casillas) todoOk = todoOk && c.ok;

			// phoenix_mandos_extra.dat: casilla i = puesto 9+i, con el mapa de botones de la casilla 1 de settings.dat
			// (el que quedará después de ordenar). Sin puestos 9+ y sin archivo, no hay nada que escribir.
			const std::wstring rutaExtra = mandosextra::ruta();
			std::vector<unsigned char> archExtra;
			const bool hayExtra = !rutaExtra.empty() && leer(rutaExtra, archExtra);
			const std::vector<unsigned char> base = todoOk ? arch : reordenar(arch, deseado, deLink);
			const std::vector<unsigned char> nuevoExtra = mandosextra::construir(extra,
				mandosextra::mapaDeCasilla1(base, std::vector<unsigned char>(kMapaXbox, kMapaXbox + kMapa)));
			for (size_t i = 0; i < extra.size(); i++) {
				Casilla c; c.n = static_cast<int>(9 + i); c.extra = true;
				c.puesto = extra[i].empty() ? 0 : c.n;
				c.ok = hayExtra && mandosextra::cantidad(archExtra) == extra.size() && mandosextra::guidDe(archExtra, i) == extra[i];
				e.casillas.push_back(c);
			}
			const bool extraOk = extra.empty() ? (!hayExtra || mandosextra::cantidad(archExtra) == 0) : (hayExtra && archExtra == nuevoExtra);

			// Estable = el mismo reparto dos vueltas seguidas (Windows aún registra los mandos recién enchufados).
			std::vector<std::vector<unsigned char>> todos = deseado;
			todos.insert(todos.end(), extra.begin(), extra.end());
			const bool estable = !faltan && todos == deseadoAntes;
			deseadoAntes = todos;
			if (faltan) e.estado = "esperando";
			else if (todoOk && extraOk) e.estado = "al_dia";
			else if (pesAbierto()) e.estado = "pes_abierto";
			else if (!estable) e.estado = "esperando";
			else if (todoOk) {
				// settings.dat ya está bien: solo falta phoenix_mandos_extra.dat
				escribirExtra(e, rutaExtra, nuevoExtra);
			}
			else {
				// Copia de seguridad (una por sesión) y escritura atómica.
				if (e.copia.empty()) {
					const std::wstring copia = ruta + L".phoenix-" + sello();
					if (CopyFileW(ruta.c_str(), copia.c_str(), TRUE)) {
						const size_t barra = copia.find_last_of(L'\\');
						const std::wstring nombre = copia.substr(barra + 1);
						e.copia.assign(nombre.begin(), nombre.end());
					}
				}
				if (e.copia.empty()) { e.estado = "error"; e.error = "No se pudo hacer la copia de seguridad de settings.dat."; }
				else {
					const std::vector<unsigned char> nuevo = reordenar(arch, deseado, deLink);
					const std::wstring tmp = ruta + L".phoenix-tmp";
					bool ok = false;
					{ std::ofstream out(tmp, std::ios::binary | std::ios::trunc); out.write(reinterpret_cast<const char*>(nuevo.data()), nuevo.size()); ok = out.good(); }
					ok = ok && MoveFileExW(tmp.c_str(), ruta.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
					if (!ok) { DeleteFileW(tmp.c_str()); e.estado = "error"; e.error = "No se pudo escribir settings.dat."; }
					else if (extraOk || escribirExtra(e, rutaExtra, nuevoExtra)) {
						e.estado = "ordenado"; e.ultimo = hora();
						for (auto& c : e.casillas) c.ok = true;
					}
				}
			}
			estableAntes = estable;
			std::lock_guard<std::mutex> l(mtx);
			est = e;
		}

		void hilo() {
			CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			IDirectInput8W* di = nullptr;
			DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, reinterpret_cast<void**>(&di), nullptr);
			bool estableAntes = false;
			std::vector<std::vector<unsigned char>> deseadoAntes;
			for (;;) {
				{
					std::unique_lock<std::mutex> l(mtx);
					cv.wait_for(l, std::chrono::seconds(3), [] { return pedirYa; });
					pedirYa = false;
				}
				if (!activo.load()) {
					std::lock_guard<std::mutex> l(mtx);
					est = Estado();
					extraCopiado = false;
					estableAntes = false;
					deseadoAntes.clear();
					continue;
				}
				if (!di) { std::lock_guard<std::mutex> l(mtx); est.activo = true; est.estado = "error"; est.error = "DirectInput no disponible."; continue; }
				try { revisar(di, estableAntes, deseadoAntes); }
				catch (...) { std::lock_guard<std::mutex> l(mtx); est.estado = "error"; est.error = "Fallo al revisar los mandos."; }
			}
		}
	}

	bool esSettingsValido(const std::vector<unsigned char>& s) {
		return s.size() == kTam && std::memcmp(s.data(), "WECF", 4) == 0;
	}

	std::vector<unsigned char> reordenar(const std::vector<unsigned char>& s,
		const std::vector<std::vector<unsigned char>>& guids,
		const std::vector<std::vector<unsigned char>>& guidsDeLink) {
		std::vector<unsigned char> r = s;
		if (!esSettingsValido(s)) return r;
		// Mapa Xbox: el más repetido entre las casillas que ya tienen un mando virtual; si no hay, el de fábrica.
		std::map<std::vector<unsigned char>, int> cuenta;
		for (size_t n = 0; n < 8; n++) {
			const unsigned char* c = &s[kCasillas + n * kCasilla];
			for (const auto& g : guidsDeLink) {
				if (g.size() == 16 && std::memcmp(c, g.data(), 16) == 0) { cuenta[std::vector<unsigned char>(c + 16, c + kCasilla)]++; break; }
			}
		}
		std::vector<unsigned char> mapa(kMapaXbox, kMapaXbox + kMapa);
		int mejor = 0;
		for (const auto& kv : cuenta) if (kv.second > mejor) { mejor = kv.second; mapa = kv.first; }
		for (size_t n = 0; n < 8; n++) {
			unsigned char* c = &r[kCasillas + n * kCasilla];
			if (n < guids.size() && guids[n].size() == 16) {
				std::memcpy(c, guids[n].data(), 16);
				std::memcpy(c + 16, mapa.data(), kMapa);
			}
			else std::memset(c, 0, 16);
		}
		return r;
	}

	void activar(bool si) {
		activo = si;
		std::lock_guard<std::mutex> l(mtx);
		if (si && !hiloVivo) { hiloVivo = true; std::thread(hilo).detach(); }
		if (!si) est = Estado();
		pedirYa = true;
		cv.notify_all();
	}

	void publicarSeries(const std::vector<unsigned long>& seriePorPuesto) {
		std::lock_guard<std::mutex> l(mtx);
		series = seriePorPuesto;
	}

	void revisarYa() {
		std::lock_guard<std::mutex> l(mtx);
		pedirYa = true;
		cv.notify_all();
	}

	Estado estado() {
		std::lock_guard<std::mutex> l(mtx);
		Estado e = est;
		e.activo = activo.load();
		return e;
	}
}
