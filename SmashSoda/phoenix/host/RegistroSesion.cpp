#include "RegistroSesion.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "parsec.h"

namespace phoenix::registro {

	namespace {
		// Por invitado: lo necesario para la pérdida por intervalo y el resumen final
		struct Acumulado {
			std::string nombre;
			uint32_t parsecId = 0;
			uint32_t prevEnviados = 0, prevReenvios = 0;
			int muestras = 0;
			double sumPing = 0, maxPing = 0, sumDeco = 0, sumMbps = 0, sumPerdida = 0, maxPerdida = 0;
			uint32_t maxCola = 0;
			int conPerdida = 0;   ///< sondeos con > 5 % de paquetes reenviados
		};

		FILE* gArchivo = nullptr;
		std::wstring gRutaCsv;
		time_t gInicio = 0;
		std::map<uint32_t, Acumulado> gInvitados;   // por id de invitado de Parsec (guest.id)
		int gSondeos = 0, gMuestrasCpu = 0;
		double gSumCpuPc = 0, gMaxCpuPc = 0, gSumCpuLink = 0, gSumEncode = 0;
		uint32_t gMaxAncho = 0, gMinAncho = UINT32_MAX;

		// CPU (deltas entre sondeos)
		ULONGLONG gPrevIdle = 0, gPrevKernel = 0, gPrevUser = 0, gPrevProc = 0, gPrevPared = 0;

		ULONGLONG ft(const FILETIME& f) { return (static_cast<ULONGLONG>(f.dwHighDateTime) << 32) | f.dwLowDateTime; }

		/// CPU total del PC y de Link en % desde el sondeo anterior (-1 la primera vez).
		void medirCpu(double& pc, double& link) {
			pc = link = -1;
			FILETIME idle, kernel, user, c, e, k, u, ahora;
			if (!GetSystemTimes(&idle, &kernel, &user)) return;
			if (!GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u)) return;
			GetSystemTimeAsFileTime(&ahora);
			const ULONGLONG I = ft(idle), K = ft(kernel), U = ft(user), P = ft(k) + ft(u), W = ft(ahora);
			if (gPrevPared != 0 && W - gPrevPared < 5000000) return;   // < 0,5 s desde la anterior: medida poco fiable, se espera
			if (gPrevPared != 0) {
				const ULONGLONG total = (K - gPrevKernel) + (U - gPrevUser);   // el kernel ya incluye el tiempo ocioso
				const ULONGLONG ocioso = I - gPrevIdle;
				if (total > 0) pc = 100.0 * static_cast<double>(total - (std::min)(ocioso, total)) / static_cast<double>(total);
				SYSTEM_INFO si; GetSystemInfo(&si);
				const ULONGLONG pared = W - gPrevPared;
				if (pared > 0 && si.dwNumberOfProcessors > 0)
					link = 100.0 * static_cast<double>(P - gPrevProc) / static_cast<double>(pared * si.dwNumberOfProcessors);
			}
			gPrevIdle = I; gPrevKernel = K; gPrevUser = U; gPrevProc = P; gPrevPared = W;
		}

		std::string limpio(const char* s) {   // sin comas ni saltos (CSV)
			std::string r(s ? s : "");
			for (char& ch : r) if (ch == ',' || ch == '\n' || ch == '\r' || ch == '"') ch = ' ';
			return r;
		}

		/// Deja solo los 40 registros más nuevos (csv + resumen).
		void podar(const std::wstring& dir) {
			try {
				std::vector<std::filesystem::path> csvs;
				for (const auto& en : std::filesystem::directory_iterator(dir))
					if (en.is_regular_file() && en.path().extension() == L".csv") csvs.push_back(en.path());
				std::sort(csvs.begin(), csvs.end());   // el nombre lleva la fecha: orden = antigüedad
				for (size_t i = 0; i + 40 < csvs.size(); i++) {
					std::error_code ec;
					std::filesystem::remove(csvs[i], ec);
					std::filesystem::path txt = csvs[i]; txt.replace_extension(L".txt");
					std::filesystem::remove(txt, ec);
				}
			}
			catch (...) {}
		}

		void abrir() {
			const std::wstring dir = carpeta();
			if (dir.empty()) return;
			gInicio = time(nullptr);
			tm local{}; localtime_s(&local, &gInicio);
			wchar_t nombre[64];
			wcsftime(nombre, 64, L"partida-%Y%m%d-%H%M%S.csv", &local);
			gRutaCsv = dir + L"\\" + nombre;
			if (_wfopen_s(&gArchivo, gRutaCsv.c_str(), L"w") != 0) { gArchivo = nullptr; return; }
			fprintf(gArchivo, "hora,invitado,parsec_id,ping_ms,codificar_ms,decodificar_ms,mbps,cola,perdida_pct,ancho_total_mbps,fps,freno_pct,cpu_pc_pct,cpu_link_pct\n");
			gInvitados.clear();
			gSondeos = 0; gMuestrasCpu = 0; gSumCpuPc = gMaxCpuPc = gSumCpuLink = gSumEncode = 0; gMaxAncho = 0; gMinAncho = UINT32_MAX;
			gPrevPared = 0;
			podar(dir);
		}
	}

	std::wstring carpeta() {
		const wchar_t* ad = _wgetenv(L"APPDATA");
		if (ad == nullptr || *ad == 0) return std::wstring();
		std::wstring dir = std::wstring(ad) + L"\\Trybuchet\\Smash Soda\\phoenix-sesiones";
		std::error_code ec;
		std::filesystem::create_directories(dir, ec);
		return dir;
	}

	void anotar(const ParsecGuest* guests, int n, uint32_t anchoTotalMbps, uint32_t fps, int frenoPct) {
		try {
			if (guests == nullptr || n <= 0) return;
			if (gArchivo == nullptr) abrir();
			if (gArchivo == nullptr) return;
			double cpuPc, cpuLink;
			medirCpu(cpuPc, cpuLink);
			const time_t ahora = time(nullptr);
			tm local{}; localtime_s(&local, &ahora);
			char hora[16]; strftime(hora, sizeof(hora), "%H:%M:%S", &local);
			gSondeos++;
			if (cpuPc >= 0 && cpuLink >= 0) { gMuestrasCpu++; gSumCpuPc += cpuPc; gMaxCpuPc = (std::max)(gMaxCpuPc, cpuPc); gSumCpuLink += cpuLink; }
			gMaxAncho = (std::max)(gMaxAncho, anchoTotalMbps);
			gMinAncho = (std::min)(gMinAncho, anchoTotalMbps);
			for (int i = 0; i < n; i++) {
				const ParsecGuest& g = guests[i];
				const ParsecMetrics& m = g.metrics[0];
				Acumulado& a = gInvitados[g.id];
				if (a.parsecId == 0) { a.parsecId = g.userID; a.nombre = limpio(g.name); }
				const uint32_t reenvios = m.fastRTs + m.slowRTs;
				double perdida = 0;
				if (a.prevEnviados != 0 && m.packetsSent > a.prevEnviados) {
					const uint32_t dS = m.packetsSent - a.prevEnviados;
					const uint32_t dR = reenvios >= a.prevReenvios ? reenvios - a.prevReenvios : 0;
					perdida = 100.0 * dR / dS;
				}
				a.prevEnviados = m.packetsSent; a.prevReenvios = reenvios;
				a.muestras++;
				a.sumPing += m.networkLatency; a.maxPing = (std::max)(a.maxPing, static_cast<double>(m.networkLatency));
				a.sumDeco += m.decodeLatency; a.sumMbps += m.bitrate;
				a.sumPerdida += perdida; a.maxPerdida = (std::max)(a.maxPerdida, perdida);
				if (perdida > 5.0) a.conPerdida++;
				a.maxCola = (std::max)(a.maxCola, m.queuedFrames);
				gSumEncode += m.encodeLatency / n;
				fprintf(gArchivo, "%s,%s,%u,%.0f,%.1f,%.1f,%.1f,%u,%.1f,%u,%u,%d,%.0f,%.1f\n",
					hora, a.nombre.c_str(), g.userID, m.networkLatency, m.encodeLatency, m.decodeLatency, m.bitrate,
					m.queuedFrames, perdida, anchoTotalMbps, fps, frenoPct, cpuPc, cpuLink);
			}
			fflush(gArchivo);
		}
		catch (...) {}
	}

	void cerrarSesion() {
		try {
			if (gArchivo == nullptr) return;
			fclose(gArchivo);
			gArchivo = nullptr;
			std::wstring rutaTxt = gRutaCsv;
			const size_t p = rutaTxt.rfind(L".csv");
			if (p != std::wstring::npos) rutaTxt.replace(p, 4, L".txt");
			FILE* f = nullptr;
			if (_wfopen_s(&f, rutaTxt.c_str(), L"w") != 0 || f == nullptr) return;
			const time_t fin = time(nullptr);
			const int minutos = static_cast<int>((fin - gInicio) / 60);
			tm local{}; localtime_s(&local, &gInicio);
			char cuando[32]; strftime(cuando, sizeof(cuando), "%d/%m/%Y %H:%M", &local);
			fprintf(f, "RESUMEN DE LA PARTIDA (Phoenix Link)\n%s · %d min · %d sondeos (cada ~2 s)\n\n", cuando, minutos, gSondeos);
			if (gSondeos > 0) {
				if (gMuestrasCpu > 0)
					fprintf(f, "TU PC: CPU media %.0f %% (pico %.0f %%) · Link %.1f %% · codificar la imagen %.1f ms de media\n",
						gSumCpuPc / gMuestrasCpu, gMaxCpuPc, gSumCpuLink / gMuestrasCpu, gSumEncode / gSondeos);
				else fprintf(f, "TU PC: codificar la imagen %.1f ms de media\n", gSumEncode / gSondeos);
				fprintf(f, "Ancho total usado: entre %u y %u Mbps\n\n", gMinAncho == UINT32_MAX ? 0 : gMinAncho, gMaxAncho);
			}
			for (const auto& par : gInvitados) {
				const Acumulado& a = par.second;
				if (a.muestras == 0) continue;
				const double ping = a.sumPing / a.muestras, perd = a.sumPerdida / a.muestras;
				const char* veredicto = (perd > 5.0 || a.conPerdida * 5 > a.muestras) ? "MAL: pierde paquetes (su internet o WiFi)"
					: ping > 120 ? "REGULAR: ping alto"
					: (a.maxCola > 3) ? "REGULAR: su PC tarda en mostrar la imagen"
					: "BIEN";
				fprintf(f, "%s (#%u): %s\n  ping %.0f ms (pico %.0f) · pérdida %.1f %% (pico %.1f, %d de %d sondeos con > 5 %%) · %.1f Mbps · decodificar %.1f ms · cola máx %u\n\n",
					a.nombre.c_str(), a.parsecId, veredicto, ping, a.maxPing, perd, a.maxPerdida, a.conPerdida, a.muestras,
					a.sumMbps / a.muestras, a.sumDeco / a.muestras, a.maxCola);
			}
			fclose(f);
		}
		catch (...) {}
	}

}
