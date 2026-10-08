#include "MonitorRed.h"

#include <algorithm>
#include <cmath>

namespace phoenix::web {

	void MonitorRed::registrar(const std::map<uint32_t, std::pair<std::string, int>>& pings, double ahoraSeg) {
		for (auto& par : _series) par.second.presente = false;
		for (const auto& par : pings) {
			Serie& s = _series[par.first];
			s.nombre = par.second.first;
			s.presente = true;
			s.vistoEn = ahoraSeg;
			s.muestras.push_back(par.second.second >= 0 ? par.second.second : -1);
			while (s.muestras.size() > kMuestras) s.muestras.pop_front();
		}
		// Quien se fue hace más de 2 minutos deja de ocupar memoria.
		for (auto it = _series.begin(); it != _series.end();) {
			if (!it->second.presente && ahoraSeg - it->second.vistoEn > 120.0) it = _series.erase(it);
			else ++it;
		}
	}

	std::string MonitorRed::semaforo(int pingMs, double jitterMs, const UmbralesRed& u) {
		int peor = -1; // -1 sin datos, 0 verde, 1 ámbar, 2 rojo
		if (pingMs >= 0) peor = (std::max)(peor, pingMs <= u.pingVerde ? 0 : (pingMs <= u.pingAmbar ? 1 : 2));
		if (jitterMs >= 0.0) peor = (std::max)(peor, jitterMs <= u.jitterVerde ? 0 : (jitterMs <= u.jitterAmbar ? 1 : 2));
		switch (peor) {
		case 0: return "verde";
		case 1: return "ambar";
		case 2: return "rojo";
		default: return "sin_datos";
		}
	}

	std::vector<ResumenRed> MonitorRed::resumen(const UmbralesRed& u) const {
		std::vector<ResumenRed> lista;
		for (const auto& par : _series) {
			const Serie& s = par.second;
			ResumenRed r;
			r.parsecId = par.first;
			r.nombre = s.nombre;
			r.presente = s.presente;
			r.serie.assign(s.muestras.begin(), s.muestras.end());

			// Ventana: últimas 30 muestras con dato
			const size_t desde = s.muestras.size() > kVentana ? s.muestras.size() - kVentana : 0;
			long long suma = 0;
			int n = 0, maximo = -1, previo = -1, pasos = 0;
			double variacion = 0.0;
			for (size_t i = desde; i < s.muestras.size(); i++) {
				const int v = s.muestras[i];
				if (v < 0) { previo = -1; continue; }
				suma += v;
				n++;
				maximo = (std::max)(maximo, v);
				if (v > u.pingAmbar) r.picos++;
				if (previo >= 0) { variacion += std::abs(v - previo); pasos++; }
				previo = v;
			}
			for (auto it = s.muestras.rbegin(); it != s.muestras.rend(); ++it) {
				if (*it >= 0) { r.ultimo = *it; break; }
			}
			if (n > 0) {
				r.media = static_cast<int>(std::lround(static_cast<double>(suma) / n));
				r.maximo = maximo;
			}
			if (pasos > 0) r.jitter = std::round(variacion / pasos * 10.0) / 10.0;

			// Alerta: 5 muestras seguidas (las últimas) por encima del ámbar
			if (s.muestras.size() >= 5) {
				bool todas = true;
				for (size_t i = s.muestras.size() - 5; i < s.muestras.size(); i++) {
					if (s.muestras[i] <= u.pingAmbar) { todas = false; break; }
				}
				r.alerta = todas && s.presente;
			}
			r.semaforo = semaforo(r.media, r.jitter, u);
			lista.push_back(std::move(r));
		}
		// Presentes primero; luego por peor media
		std::sort(lista.begin(), lista.end(), [](const ResumenRed& a, const ResumenRed& b) {
			if (a.presente != b.presente) return a.presente;
			return a.media > b.media;
		});
		return lista;
	}

}
