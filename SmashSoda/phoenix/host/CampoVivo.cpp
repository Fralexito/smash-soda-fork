#include "CampoVivo.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <thread>

#define NOMINMAX
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

		/// Busca jugadores de campo, porteros y balones por su firma en la memoria privada escribible del juego
		/// (el montón, ~3-5 GB, unos 12 s, con pausas para no robar CPU).
		struct Hallazgos { std::vector<Actor> campo, porteros, balones; };
		Hallazgos escanear(HANDLE h) {
			Hallazgos r;
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
							Actor x;
							if (leerActor(buf.data() + i, x)) { x.dir = p + i; r.campo.push_back(x); }
							else if (leerPortero(buf.data() + i, x)) { x.dir = p + i; r.porteros.push_back(x); }
							else if (leerBalon(buf.data() + i, x)) { x.dir = p + i; r.balones.push_back(x); }
						}
						if ((++n & 31) == 0) dormir(1);
					}
				}
				a = base + m.RegionSize;
			}
			return r;
		}

		/// Un balón candidato: el del partido es el que se mueve (los de los recogepelotas no).
		struct Candidato { Actor a; float ux = 0, uz = 0; double ultimoCambio = 0; };

		void bucle() {
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
			DWORD pidVerificado = 0;      // pid cuyo exe ya se comprobó (tamaño + SHA-256)
			bool compatible = false;
			std::string motivoNo;
			HANDLE h = nullptr; DWORD pidAbierto = 0;
			std::vector<Actor> lista;     // jugadores de campo + porteros (direcciones fijas todo el partido)
			std::vector<int> equipos;     // equipo por índice de lista (kEqDesconocido hasta ver un saque)
			std::vector<float> antes;     // posiciones de la lectura anterior (para detectar el saque: todos quietos)
			std::vector<Candidato> balones;
			Tanda tanda;                  // tanda de penales (lógica pura)
			double reloj = 0;             // segundos (aprox.) desde el arranque del hilo
			uint32_t seq = 0;
			FotoCampo ultima;             // última lectura en vivo buena (se congela en repeticiones y cortes de lectura)
			int fallos = 0, sinBola = 0, quietos = 0;   // lecturas seguidas sin fichitas válidas / sin balón en juego / sin movimiento
			bool enRepe = false;
			auto cerrar = [&] { if (h) { CloseHandle(h); h = nullptr; } pidAbierto = 0; lista.clear(); equipos.clear(); antes.clear(); balones.clear(); tanda.reiniciar(); ultima = FotoCampo(); fallos = sinBola = quietos = 0; enRepe = false; };

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

					if (lista.empty()) {            // 3) buscar a todos por su firma
						f.estado = EstadoCampo::Buscando; publicar(f);
						const Hallazgos hz = escanear(h);
						lista = elegirLista(hz.campo);
						if (lista.empty()) { dormir(3000); continue; }
						for (const Actor& p : elegirPorteros(hz.porteros)) lista.push_back(p);
						equipos.assign(lista.size(), kEqDesconocido);
						antes.clear();
						balones.clear();
						for (const Actor& b : hz.balones) { Candidato c; c.a = b; c.ux = b.x; c.uz = b.z; balones.push_back(c); }
					}

					// 4) leer posiciones (~20/s): 16 bytes por actor. Si más de 3 dejan de ser válidas, se vuelve a buscar.
					int malos = 0; float maxDesp = 0;
					std::vector<Actor> ahora = lista;
					for (size_t i = 0; i < lista.size(); i++) {
						uint8_t b[16]; SIZE_T g = 0; float x, y, z;
						if (!ReadProcessMemory(h, reinterpret_cast<LPCVOID>(lista[i].dir + kOffPosicion), b, 16, &g) || g != 16 || !leerPosicion(b, x, y, z)) { malos++; ahora[i].x = 1e9f; continue; }
						ahora[i].x = x; ahora[i].y = y; ahora[i].z = z;
						if (antes.size() == lista.size() * 2) maxDesp = (std::max)(maxDesp, (std::max)(std::fabs(x - antes[i * 2]), std::fabs(z - antes[i * 2 + 1])));
					}
					if (malos > 3) {
						// Repetición o cinemática: las fichitas dejan de leerse un rato. Se congela la última posición buena (los actores
						// suelen volver solos) y solo si dura más de 30 s se vuelve a buscar.
						if (++fallos > 600 || ultima.estado != EstadoCampo::Activo) { lista.clear(); ultima = FotoCampo(); fallos = 0; f.estado = EstadoCampo::Buscando; publicar(f); dormir(1500); continue; }
						f = ultima; f.fase = "otro"; f.seq = ++seq; publicar(f); dormir(50); continue;
					}
					fallos = 0;
					const bool habiaAntes = antes.size() == lista.size() * 2;
					antes.resize(lista.size() * 2);
					for (size_t i = 0; i < lista.size(); i++) { antes[i * 2] = ahora[i].x; antes[i * 2 + 1] = ahora[i].z; }

					// 5) equipos: en el primer saque de centro que se vea (todos quietos y en formación) se reparten y se recuerdan
					const bool sinEquipos = std::count(equipos.begin(), equipos.end(), kEqDesconocido) > 4;
					quietos = (habiaAntes && maxDesp < 0.02f) ? quietos + 1 : 0;
					if (sinEquipos && habiaAntes && !enRepe && esSaqueDeCentro(ahora, maxDesp)) equipos = repartirEquipos(ahora);

					// 6) balón: el candidato que se ha movido hace menos tiempo (los de los recogepelotas no se mueven)
					reloj += 0.05;
					const Candidato* mejor = nullptr;
					bool bolaEnJuego = balones.empty();   // sin candidatos no hay información: no se declara repetición
					for (Candidato& c : balones) {
						uint8_t b[16]; SIZE_T g = 0; float x, y, z;
						if (!ReadProcessMemory(h, reinterpret_cast<LPCVOID>(c.a.dir + kOffPosicion), b, 16, &g) || g != 16 || !leerPosicionBalon(b, x, y, z)) continue;
						if (std::fabs(x - c.ux) > 0.01f || std::fabs(z - c.uz) > 0.01f) { c.ultimoCambio = reloj; c.ux = x; c.uz = z; }
						c.a.x = x; c.a.y = y; c.a.z = z;
						if (std::fabs(z) > 40.0f) continue; // aparcado (repetición)
						bolaEnJuego = true;
						if (!mejor || c.ultimoCambio > mejor->ultimoCambio) mejor = &c;
					}
					if (mejor && reloj - mejor->ultimoCambio < 120.0) { f.hayBola = true; f.bx = mejor->a.x; f.by = mejor->a.y; f.bz = mejor->a.z; }

					sinBola = bolaEnJuego ? 0 : sinBola + 1;
					enRepe = sinBola >= 10;   // ~0,5 s con el balón aparcado: repetición; vuelve en la primera lectura con balón en juego
					if (enRepe && ultima.estado == EstadoCampo::Activo) {
						// Repetición: las posiciones que se leen son de la película, no del juego. Se dejan las últimas buenas.
						f = ultima; f.fase = "repeticion"; f.hayBola = false; f.seq = ++seq; publicar(f); dormir(50); continue;
					}
					f.j.reserve(lista.size());
					for (size_t i = 0; i < lista.size(); i++) {
						if (ahora[i].x > 1e8f) continue;
						Pieza p; p.x = ahora[i].x; p.z = ahora[i].z;
						p.eq = pareceAsistente(ahora[i]) ? kEqArbitro : equipos[i];
						f.j.push_back(p);
					}
					tanda.alimentar(f.hayBola, f.bx, f.by, f.bz, ahora, equipos, reloj);
					f.tanda = tanda.estado();
					f.equiposListos = !sinEquipos;
					f.estado = EstadoCampo::Activo; f.seq = ++seq;
					f.fase = quietos >= 80 ? "pausa" : "vivo";
					ultima = f;
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
