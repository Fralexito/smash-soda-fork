#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

// =============================================================================
//  Phoenix Link · Monitor de red por jugador
// -----------------------------------------------------------------------------
//  Guarda una muestra de ping por segundo de cada invitado (últimos 2 min) y
//  calcula media, máximo, jitter y el semáforo con la MISMA regla que la web
//  (contrato §13): cada métrica contra sus umbrales verde/ámbar, manda la peor.
//  Umbrales por defecto del perfil «amistoso»: ping 60/100 ms, jitter 10/25 ms.
//  Lógica pura (sin Windows): se prueba en Linux.
// =============================================================================

namespace phoenix::web {

	struct UmbralesRed {
		int pingVerde = 60;
		int pingAmbar = 100;
		double jitterVerde = 10.0;
		double jitterAmbar = 25.0;
	};

	struct ResumenRed {
		uint32_t parsecId = 0;
		std::string nombre;
		bool presente = false;      ///< sigue en la sala
		int ultimo = -1;            ///< último ping (ms); -1 = sin dato
		int media = -1;             ///< media de los últimos 30 s
		int maximo = -1;            ///< máximo de los últimos 30 s
		double jitter = -1.0;       ///< variación media entre muestras seguidas (ms)
		int picos = 0;              ///< muestras por encima del ámbar en los últimos 30 s
		bool alerta = false;        ///< los últimos 5 s seguidos por encima del ámbar
		std::string semaforo = "sin_datos"; ///< verde | ambar | rojo | sin_datos
		std::vector<int> serie;     ///< hasta 120 muestras (-1 = sin dato), la última al final
	};

	class MonitorRed {
	public:
		static constexpr size_t kMuestras = 120;
		static constexpr size_t kVentana = 30;

		/// Registra una muestra por invitado presente. Llamar ≈ 1 vez por segundo.
		/// `pings`: parsecId → ping en ms (-1 = sin dato). `ahoraSeg`: reloj monótono.
		void registrar(const std::map<uint32_t, std::pair<std::string, int>>& pings, double ahoraSeg);

		std::vector<ResumenRed> resumen(const UmbralesRed& u = UmbralesRed()) const;

		/// Semáforo de una sola métrica de ping y jitter (regla del contrato §13).
		static std::string semaforo(int pingMs, double jitterMs, const UmbralesRed& u);

		void limpiar() { _series.clear(); }

	private:
		struct Serie {
			std::string nombre;
			std::deque<int> muestras;
			double vistoEn = 0.0;
			bool presente = false;
		};
		std::map<uint32_t, Serie> _series;
	};

}
