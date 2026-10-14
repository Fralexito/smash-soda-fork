#include "CampoVivo.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <thread>

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

#include "../core/CampoVivo.h"
#include "../link/Entrega.h"

namespace phoenix::campo {

	namespace {
		std::atomic<bool> g_vivo{ false };
		std::atomic<bool> g_parar{ false };
		std::atomic<int> g_detenido{ 0 };   // 0 = en juego, 1 = reloj parado, 2 = descanso
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
		/// Los porteros tienen DOS copias en el juego (una se mueve, la otra se queda en el sitio del saque): por cada lado hay que
		/// leer la que se mueve. «slot» es el puesto en la lista de lectura; «cs» las copias; «elegido» la que se lee ahora.
		struct GrupoPortero { size_t slot = 0; std::vector<Candidato> cs; size_t elegido = 0; };

		void bucle() {
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
			struct Resolucion { Resolucion() { timeBeginPeriod(1); } ~Resolucion() { timeEndPeriod(1); } } resolucion;   // sueño de 1 ms (por defecto 15,6 ms: la lectura iba a ~16/s)
			DWORD pidVerificado = 0;      // pid cuyo exe ya se comprobó (tamaño + SHA-256)
			bool compatible = false;
			std::string motivoNo;
			HANDLE h = nullptr; DWORD pidAbierto = 0;
			std::vector<Actor> lista;     // jugadores de campo + porteros (direcciones fijas todo el partido)
			std::vector<int> equipos;     // equipo por índice de lista (kEqDesconocido hasta ver un saque)
			std::vector<float> antes;     // posiciones de la lectura anterior (para detectar el saque: todos quietos)
			std::vector<Candidato> balones;
			Tanda tanda;                  // tanda de penales (lógica pura)
			double reloj = 0;             // segundos reales desde el arranque del hilo
			const auto tInicio = std::chrono::steady_clock::now();
			auto proximaLectura = tInicio;
			Poseedor poseedor;            // quién lleva el balón (histéresis)
			std::vector<GrupoPortero> grupos;   // porteros: un grupo por lado (con sus copias)
			int portCopias = 0;
			std::vector<uint64_t> gemelos;   // por ficha de la lista: dirección de su «gemelo» en la otra lista del juego (0 = sin gemelo)
			Estadisticas estad;           // estadísticas derivadas del partido
			// diagnóstico: ventana de ~1 s
			auto ventanaDesde = tInicio; int ventanaN = 0; double ventanaSuma = 0, ventanaMax = 0, lps = 0, pasadaMedia = 0, pasadaMax = 0;
			uint32_t seq = 0;
			FotoCampo ultima;             // última lectura en vivo buena (se congela en repeticiones y cortes de lectura)
			int fallos = 0, sinBola = 0, quietos = 0;   // lecturas seguidas sin fichitas válidas / sin balón en juego / sin movimiento
			bool enRepe = false;
			auto cerrar = [&] { if (h) { CloseHandle(h); h = nullptr; } pidAbierto = 0; lista.clear(); equipos.clear(); antes.clear(); balones.clear(); tanda.reiniciar(); poseedor.reiniciar(); grupos.clear(); portCopias = 0; gemelos.clear(); estad.reiniciar(); ultima = FotoCampo(); fallos = sinBola = quietos = 0; enRepe = false; };

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
						// Cada jugador existe dos veces en el juego (una por lista de equipo de IA): objetos contiguos que difieren ≈ 0,6 m sin
						// sentido fijo (medido con partido real: ninguna es siempre la buena). Se leen las dos y se promedian.
						gemelos.assign(lista.size(), 0);
						for (size_t i = 0; i < lista.size(); i++) {
							for (const Actor& b : hz.campo) {
								if (b.codigo == lista[i].codigo) continue;
								const uint64_t d1 = b.dir > lista[i].dir ? b.dir - lista[i].dir : lista[i].dir - b.dir;
								if (d1 != 0x280) continue;
								if (std::hypot(b.x - lista[i].x, b.z - lista[i].z) < 3.0f) { gemelos[i] = b.dir; break; }
							}
						}
						// porteros: por cada lado, todas sus copias; se empieza por la de menor dirección y se pasa a la que se mueva
						grupos.clear(); portCopias = static_cast<int>(hz.porteros.size());
						for (const bool izquierda : { true, false }) {
							GrupoPortero g;
							for (const Actor& p : hz.porteros) if ((p.x < 0) == izquierda && std::fabs(p.x) >= 30.0f && std::fabs(p.z) <= 22.0f) {   /* solo cerca de la portería: en el juego hay actores con la misma firma (mismo +492) que NO son porteros */ Candidato c; c.a = p; c.ux = p.x; c.uz = p.z; g.cs.push_back(c); }
							if (g.cs.empty()) continue;
							std::sort(g.cs.begin(), g.cs.end(), [](const Candidato& a, const Candidato& b) { return a.a.dir < b.a.dir; });
							g.slot = lista.size(); g.elegido = 0;
							lista.push_back(g.cs.front().a);
							grupos.push_back(std::move(g));
						}
						{   // volcado del escaneo para diagnóstico (se sobrescribe en cada escaneo)
							try {
								FILE* fd = nullptr;
								if (fopen_s(&fd, (std::filesystem::temp_directory_path() / "phoenix-campo-escaneo.txt").string().c_str(), "w") == 0 && fd) {
									fprintf(fd, "jugadores de campo encontrados: %zu, lista elegida: %zu, porteros (copias): %zu, balones: %zu\n", hz.campo.size(), lista.size() - grupos.size(), hz.porteros.size(), hz.balones.size());
									for (const Actor& p : hz.porteros) fprintf(fd, "portero dir=%llX cod=%u x=%.1f y=%.2f z=%.1f\n", static_cast<unsigned long long>(p.dir), p.codigo, p.x, p.y, p.z);
									for (const GrupoPortero& g : grupos) fprintf(fd, "grupo lado %s: %zu copias\n", g.cs.front().a.x < 0 ? "izquierdo" : "derecho", g.cs.size());
									fclose(fd);
								}
							} catch (...) {}
						}
						equipos.assign(lista.size(), kEqDesconocido);
						antes.clear();
						balones.clear();
						for (const Actor& b : hz.balones) { Candidato c; c.a = b; c.ux = b.x; c.uz = b.z; balones.push_back(c); }
					}

					const auto tPasada = std::chrono::steady_clock::now();
					{   // porteros: ¿qué copia se mueve? (la otra se queda donde el saque y no sirve)
						const double ahoraS = std::chrono::duration<double>(tPasada - tInicio).count();
						for (GrupoPortero& g : grupos) {
							for (Candidato& c : g.cs) {
								uint8_t b[16]; SIZE_T gg = 0; float x, y, z;
								if (!ReadProcessMemory(h, reinterpret_cast<LPCVOID>(c.a.dir + kOffPosicion), b, 16, &gg) || gg != 16 || !leerPosicion(b, x, y, z)) continue;
								if (std::fabs(x - c.ux) > 0.02f || std::fabs(z - c.uz) > 0.02f) { c.ultimoCambio = ahoraS; c.ux = x; c.uz = z; }
								c.a.x = x; c.a.y = y; c.a.z = z;
							}
							size_t mejor = g.elegido;
							for (size_t k = 0; k < g.cs.size(); k++) if (g.cs[k].ultimoCambio > g.cs[mejor].ultimoCambio) mejor = k;
							// cambia solo si la elegida lleva > 1 s quieta y la otra se movió hace menos de 0,4 s (sin parpadeo entre dos copias vivas)
							if (mejor != g.elegido && ahoraS - g.cs[g.elegido].ultimoCambio > 1.0 && ahoraS - g.cs[mejor].ultimoCambio < 0.4) { g.elegido = mejor; lista[g.slot].dir = g.cs[mejor].a.dir; }
						}
					}
					// 4) leer posiciones (~30/s): 16 bytes por actor. Si más de 3 dejan de ser válidas, se vuelve a buscar.
					int malos = 0; float maxDesp = 0;
					std::vector<Actor> ahora = lista;
					for (size_t i = 0; i < lista.size(); i++) {
						uint8_t b[16]; SIZE_T g = 0; float x, y, z;
						if (!ReadProcessMemory(h, reinterpret_cast<LPCVOID>(lista[i].dir + kOffPosicion), b, 16, &g) || g != 16 || !leerPosicion(b, x, y, z)) { malos++; ahora[i].x = 1e9f; continue; }
						ahora[i].x = x; ahora[i].y = y; ahora[i].z = z;
						if (i < gemelos.size() && gemelos[i] != 0) {   // promedio con el gemelo (si lo lee bien y está cerca)
							uint8_t b2[16]; SIZE_T g2 = 0; float x2, y2, z2;
							if (ReadProcessMemory(h, reinterpret_cast<LPCVOID>(gemelos[i] + kOffPosicion), b2, 16, &g2) && g2 == 16 && leerPosicion(b2, x2, y2, z2) && std::hypot(x2 - x, z2 - z) < 3.0f) {
								ahora[i].x = (x + x2) * 0.5f; ahora[i].y = (y + y2) * 0.5f; ahora[i].z = (z + z2) * 0.5f;
							}
						}
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
					reloj = std::chrono::duration<double>(std::chrono::steady_clock::now() - tInicio).count();
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
					const int detenido = g_detenido.load();
					if (detenido != 0 && !enRepe && ultima.estado == EstadoCampo::Activo) {
						// El reloj del juego está parado: lo que se lee es una repetición, una celebración o una pausa. Se congela lo último en vivo.
						f = ultima; f.fase = detenido == 2 ? "descanso" : "detenido"; f.hayBola = false; f.seq = ++seq; publicar(f); dormir(50); continue;
					}
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
					{
						std::vector<PuntoCampo> pc; pc.reserve(f.j.size());
						for (const Pieza& p : f.j) { PuntoCampo c; c.x = p.x; c.z = p.z; c.elegible = p.eq != kEqArbitro; pc.push_back(c); }
						f.poseedor = poseedor.actualizar(pc, f.hayBola, f.bx, f.by, f.bz, reloj);
						// estadísticas: A defiende el lado donde está su portero (cambia en el descanso)
						int ladoA = 0;
						for (const GrupoPortero& g : grupos) {
							if (g.slot >= equipos.size() || ahora[g.slot].x > 1e8f) continue;
							const int e = equipos[g.slot];
							if (e == kEqA) { ladoA = ahora[g.slot].x < 0 ? -1 : 1; break; }
							if (e == kEqB) { ladoA = ahora[g.slot].x < 0 ? 1 : -1; break; }
						}
						std::vector<int> eqs; eqs.reserve(f.j.size());
						for (const Pieza& p : f.j) eqs.push_back(p.eq);
						if (f.fase == "vivo" || f.fase == "pausa") estad.alimentar(reloj, f.hayBola, f.bx, f.by, f.bz, f.poseedor, eqs, pc, ladoA);
						f.est = estad.estado();
					}
					f.tanda = tanda.estado();
					f.equiposListos = !sinEquipos;
					f.estado = EstadoCampo::Activo; f.seq = ++seq;
					f.fase = quietos >= 80 ? "pausa" : "vivo";
					// diagnóstico del lector: pasadas por segundo y duración de cada una (jitter)
					{
						const auto fin = std::chrono::steady_clock::now();
						const double ms = std::chrono::duration<double, std::milli>(fin - tPasada).count();
						ventanaN++; ventanaSuma += ms; ventanaMax = (std::max)(ventanaMax, ms);
						const double seg = std::chrono::duration<double>(fin - ventanaDesde).count();
						if (seg >= 1.0) { lps = ventanaN / seg; pasadaMedia = ventanaSuma / ventanaN; pasadaMax = ventanaMax; ventanaDesde = fin; ventanaN = 0; ventanaSuma = 0; ventanaMax = 0; }
						f.lecturasPorSeg = lps; f.pasadaMediaMs = pasadaMedia; f.pasadaMaxMs = pasadaMax;
						f.portCopias = portCopias; f.portPuestos = static_cast<int>(grupos.size());
						f.tMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					}
					ultima = f;
					publicar(f);
					// siguiente lectura a un ritmo fijo de ~30/s (hora de reloj, no «dormir 50 ms» acumulando retraso)
					proximaLectura += std::chrono::milliseconds(33);
					{
						const auto ya = std::chrono::steady_clock::now();
						if (proximaLectura < ya) proximaLectura = ya;   // si nos atrasamos no se intenta recuperar a ráfagas
						while (!g_parar.load() && std::chrono::steady_clock::now() < proximaLectura) std::this_thread::sleep_for(std::chrono::milliseconds(2));
					}
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
	void fijarDetenido(bool detenido, bool descanso) { g_detenido = descanso ? 2 : (detenido ? 1 : 0); }

	FotoCampo instantanea() { std::lock_guard<std::mutex> l(g_mutex); return g_foto; }

}
