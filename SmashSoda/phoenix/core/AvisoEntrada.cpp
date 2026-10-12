#include "AvisoEntrada.h"

#include <Windows.h>
#include <mmsystem.h>
#include <shellapi.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "../PhoenixPrefs.h"
#include "../link/PhoenixLink.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")

namespace phoenix::aviso {

	namespace {

		std::wstring aWide(const std::string& s) {
			if (s.empty()) return std::wstring();
			const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
			std::wstring w(static_cast<size_t>(n > 0 ? n : 0), L'\0');
			if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], n);
			return w;
		}

		/// Campanita de dos notas (mi5 → la5), 22 050 Hz, 16 bits, mono. Se crea una vez y vive todo el programa
		/// (PlaySound asíncrono lee de esta memoria mientras suena).
		const std::vector<unsigned char>& campanita() {
			static const std::vector<unsigned char> wav = [] {
				const int hz = 22050;
				const double notas[2] = { 659.25, 880.0 };
				const double inicio[2] = { 0.0, 0.17 };
				const int total = static_cast<int>(hz * 0.75);
				std::vector<int16_t> pcm(static_cast<size_t>(total), 0);
				for (int n = 0; n < 2; n++) {
					const int desde = static_cast<int>(inicio[n] * hz);
					for (int i = desde; i < total; i++) {
						const double t = static_cast<double>(i - desde) / hz;
						const double ataque = (std::min)(1.0, t / 0.005);
						const double env = ataque * std::exp(-t * 6.5);
						const double v = std::sin(2.0 * 3.14159265358979 * notas[n] * t) + 0.25 * std::sin(2.0 * 3.14159265358979 * notas[n] * 2.0 * t);
						pcm[static_cast<size_t>(i)] = static_cast<int16_t>((std::max)(-32767.0, (std::min)(32767.0, pcm[static_cast<size_t>(i)] + v * env * 9000.0)));
					}
				}
				const uint32_t datos = static_cast<uint32_t>(pcm.size() * 2);
				std::vector<unsigned char> b(44 + datos);
				auto u32 = [&](size_t o, uint32_t v) { for (int k = 0; k < 4; k++) b[o + k] = static_cast<unsigned char>((v >> (8 * k)) & 0xFF); };
				auto u16 = [&](size_t o, uint16_t v) { b[o] = static_cast<unsigned char>(v & 0xFF); b[o + 1] = static_cast<unsigned char>(v >> 8); };
				std::memcpy(&b[0], "RIFF", 4); u32(4, 36 + datos);
				std::memcpy(&b[8], "WAVEfmt ", 8); u32(16, 16); u16(20, 1); u16(22, 1); u32(24, hz); u32(28, hz * 2); u16(32, 2); u16(34, 16);
				std::memcpy(&b[36], "data", 4); u32(40, datos);
				std::memcpy(&b[44], pcm.data(), datos);
				return b;
			}();
			return wav;
		}

		std::wstring carpetaDelExe() {
			wchar_t ruta[MAX_PATH * 2] = {};
			const DWORD n = GetModuleFileNameW(nullptr, ruta, MAX_PATH * 2);
			std::wstring s(ruta, n);
			const size_t p = s.find_last_of(L"\/");
			return p == std::wstring::npos ? std::wstring() : s.substr(0, p);
		}

		void sonido() {
			try {
				const std::wstring propio = carpetaDelExe() + L"\SFX\entrada.wav";
				if (GetFileAttributesW(propio.c_str()) != INVALID_FILE_ATTRIBUTES) {
					PlaySoundW(propio.c_str(), nullptr, SND_FILENAME | SND_NODEFAULT | SND_ASYNC);
					return;
				}
				PlaySoundW(reinterpret_cast<LPCWSTR>(campanita().data()), nullptr, SND_MEMORY | SND_NODEFAULT | SND_ASYNC);
			}
			catch (...) {}
		}

		/// Globo de notificación de Windows: icono temporal en el área de notificaciones que se quita a los ~6 s.
		void globo(const std::string& titulo, const std::string& texto) {
			std::thread([titulo, texto] {
				try {
					static std::once_flag unaVez;
					static std::atomic<UINT> contador{ 100 };
					const wchar_t* clase = L"PhoenixAvisoEntrada";
					HINSTANCE hi = GetModuleHandleW(nullptr);
					std::call_once(unaVez, [&] {
						WNDCLASSW wc = {};
						wc.lpfnWndProc = DefWindowProcW;
						wc.hInstance = hi;
						wc.lpszClassName = clase;
						RegisterClassW(&wc);
					});
					HWND h = CreateWindowExW(0, clase, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hi, nullptr);
					if (h == nullptr) return;
					NOTIFYICONDATAW nid = {};
					nid.cbSize = sizeof(nid);
					nid.hWnd = h;
					nid.uID = contador++;
					nid.uFlags = NIF_ICON | NIF_TIP | NIF_INFO;
					wchar_t exe[MAX_PATH * 2] = {};
					GetModuleFileNameW(nullptr, exe, MAX_PATH * 2);
					HICON ic = ExtractIconW(hi, exe, 0);
					nid.hIcon = (ic != nullptr && reinterpret_cast<uintptr_t>(ic) > 1) ? ic : LoadIconW(nullptr, IDI_APPLICATION);
					wcsncpy_s(nid.szTip, L"Phoenix Link", _TRUNCATE);
					wcsncpy_s(nid.szInfoTitle, aWide(titulo).c_str(), _TRUNCATE);
					wcsncpy_s(nid.szInfo, aWide(texto).c_str(), _TRUNCATE);
					nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;   // el sonido lo pone Link
					if (Shell_NotifyIconW(NIM_ADD, &nid)) {
						const ULONGLONG fin = GetTickCount64() + 6000;
						MSG m;
						while (GetTickCount64() < fin) {
							while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); }
							Sleep(100);
						}
						Shell_NotifyIconW(NIM_DELETE, &nid);
					}
					DestroyWindow(h);
				}
				catch (...) {}
			}).detach();
		}

		void lanzar(const std::string& nombre) {
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			if (pr.avisoEntradaSonido) sonido();
			if (pr.avisoEntradaNotif) globo("Phoenix Link", nombre + " entró a la sala");
			if (pr.avisoEntradaJuego) {
				try { PhoenixLink::instancia().avisoLocalDiferido(nombre + " entró a la sala"); }
				catch (...) {}
			}
		}

	}

	void entro(const std::string& nombre) {
		try {
			if (!PhoenixPrefs::get().avisoEntrada) return;
			lanzar(nombre.empty() ? std::string("Alguien") : nombre);
		}
		catch (...) {}
	}

	void probar() {
		try { lanzar("Invitado de prueba"); }
		catch (...) {}
	}

}
