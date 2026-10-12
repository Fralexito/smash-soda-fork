#include "CampoVivo.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <thread>

#include <windows.h>
#include <tlhelp32.h>

#include "../core/CampoVivo.h"
#include "../link/Entrega.h"

namespace phoenix::campo {

	namespace {
		std::atomic<bool> g_vivo{ false };
		std::atomic<bool> g_parar{ false };
		std::thread g_hilo;
		std::mutex g_mutex;
		FotoCampo g_foto;

		void publicar(const FotoCampo& f) { std::lock_guard<std::mutex> l(g_mutex); g_foto = f; }
		/// Duerme en rebanadas de 25 ms: detener() nunca espera mas de eso.
		void dormir(int ms) { for (int t = 0; t < ms && !g_parar.load(); t += 25) std::this_thread::sleep_for(std::chrono::milliseconds(25)); }

		DWORD buscarPes() {
			HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (s == INVALID_HANDLE_VALUE) return 0;
			PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
			if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
			CloseHandle(s);
			return pid;
		}

		/// Ruta completa del exe del proceso (sin abrirlo para leer memoria).
		bool rutaDe(DWORD pid, std::wstring& ruta) {
			HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
			if (!h) return false;
			wchar_t b[32768]; DWORD n = 32768;
			const BOOL ok = QueryFullProcessImageNameW(h, 0, b, &n);
			CloseHandle(h);
			if (!ok) return false;
			ruta.assign(b, n);
			return true;
		}

		/// Busca todos los actores con la firma en la memoria privada escribible del juego (el montón) (~3-5 GB, unos 5 s, con pausas para no robar CPU).
		std::vector<Actor> escanear(HANDLE h) {
			std::vector<Actor> hallados;
			const size_t trozo = 1u << 20;
			std::vector<uint8_t> buf(trozo + kBytesObjeto);
			uint64_t a = 0; MEMORY_BASIC_INFORMATION m; int n = 0;
			while (VirtualQueryEx(h, reinterpret_cast<LPCVOID>(a), &m, sizeof m)) {
				const uint64_t base = reinterpret_cast<uint64_t>(m.BaseAddress), fin = base + m.RegionSize;
				const bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
				if (ok) {
					for (uint64_t p = base; p < fin; p += trozo) {
						if (g_parar.load() || !g_vivo.load()) return {};
						const SIZE_T quiero = static_cast<SIZE_T>(std::min<uint64_t>(trozo + kBytesObjeto, fin - p));
						SIZE_T leidos = 0;
						if (!ReadProcessMemory(h, reinterpret_cast<LPCVOID>(p), buf.data(), quiero, &leidos) || leidos < kBytesObjeto) continue;
						for (SIZE_T i = 0; i + kBytesObjeto <= leidos && i < trozo; i += 16) {
							Actor r;
							if (leerActor(buf.data() + i, r)) { r.dir = p + i; hallados.push_back(r); }
						}
						if ((++n & 31) == 0) dormir(1);
					}
				}
				a = base + m.RegionSize;
			}
			return hallados;
		}

		void bucle() {
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
			DWORD pidVerificado = 0;      // pid cuyo exe ya se comprobó (tamaño + SHA-256)
			bool compatible = false;
			std::string motivoNo;
			HANDLE h = nullptr; DWORD pidAbierto = 0;
			std::vector<Actor> lista;     // actores de UNA lista (direcciones fijas todo el partido)
			uint32_t seq = 0;
			auto cerrar = [&] { if (h) { CloseHandle(h); h = nullptr; } pidAbierto = 0; lista.clear(); };

			while (!g_parar.load()) {
				FotoCampo f;
				try {
					if (!g_vivo.load()) { cerrar(); f.estado = EstadoCampo::Apagado; publicar(f); dormir(250); continue; }
					const DWORD pid = buscarPes();
					if (!pid) { cerrar(); pidVerificado = 0; f.estado = EstadoCampo::Esperando; publicar(f); dormir(1000); continue; }

					if (pid != pidVerificado) {   // 1) versión del juego: tamaño + SHA-256 (una vez por proceso)
						cerrar();
						compatible = false; motivoNo.clear();
						std::wstring ruta;
						if (!rutaDe(pid, ruta)) { motivoNo = "no se pudo ver el ejecutable"; }
						else {
							std::error_code ec; const auto tam = std::filesystem::file_size(ruta, ec);
							std::string hex;
							if (ec) motivoNo = "no se pudo medir el ejecutable";
							else if (!phoenix::entrega::sha256Archivo(ruta, hex)) motivoNo = "no se pudo calcular la huella";
							else {
								for (char& c : hex) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
								compatible = exeConocido(tam, hex);
								if (!compatible) motivoNo = "otra versión de PES2021.exe";
							}
						}
						pidVerificado = pid;
					}
					if (!compatible) { f.estado = EstadoCampo::NoCompatible; f.motivo = motivoNo; publicar(f); dormir(1000); continue; }

					if (!h || pidAbierto != pid) {  // 2) abrir SOLO para leer
						cerrar();
						h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
						if (!h) { f.estado = EstadoCampo::NoCompatible; f.motivo = "no se pudo abrir el juego para leer"; publicar(f); dormir(2000); continue; }
						pidAbierto = pid;
					}

					if (lista.empty()) {            // 3) buscar a los jugadores por su firma
						f.estado = EstadoCampo::Buscando; publicar(f);
						lista = elegirLista(escanear(h));
						if (lista.empty()) { dormir(3000); continue; }
					}

					// 4) leer posiciones (~20/s): 16 bytes por jugador. Si más de 3 dejan de ser válidas, se vuelve a buscar.
					int malos = 0;
					std::vector<Pieza> piezas; piezas.reserve(lista.size());
					for (const Actor& a : lista) {
						uint8_t b[16]; SIZE_T g = 0; float x, y, z;
						if (!ReadProcessMemory(h, reinterpret_cast<LPCVOID>(a.dir + kOffPosicion), b, 16, &g) || g != 16 || !leerPosicion(b, x, y, z)) { malos++; continue; }
						Pieza p; p.x = x; p.z = z; p.eq = pareceAsistente(a) ? 2 : -1;
						piezas.push_back(p);
					}
					if (malos > 3) { lista.clear(); f.estado = EstadoCampo::Buscando; publicar(f); dormir(1500); continue; }
					f.estado = EstadoCampo::Activo; f.j = std::move(piezas); f.seq = ++seq;
					publicar(f);
					dormir(50);
				}
				catch (...) {
					// Cualquier error: se apaga esta pasada y se reintenta con calma. Nunca debe tocar el juego ni a Link.
					cerrar();
					FotoCampo e; e.estado = EstadoCampo::NoCompatible; e.motivo = "error interno del lector";
					publicar(e);
					dormir(3000);
				}
			}
			cerrar();
		}
	}

	void iniciar() {
		if (g_hilo.joinable()) return;
		g_parar = false;
		g_hilo = std::thread(bucle);
		static const bool alSalir = [] { std::atexit([] { detener(); }); return true; }(); // un std::thread sin join al salir tumba el proceso
		(void)alSalir;
	}

	void detener() {
		g_parar = true;
		if (g_hilo.joinable()) g_hilo.join();
	}

	void pedirVivo(bool si) { g_vivo = si; }

	FotoCampo instantanea() { std::lock_guard<std::mutex> l(g_mutex); return g_foto; }

}
