#pragma once

// =============================================================================
//  Phoenix Link · Orden de mandos en PES 2021 (EXPERIMENTAL)
// -----------------------------------------------------------------------------
//  PES 2021 guarda en Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\settings.dat
//  8 casillas de jugador (64 bytes desde 0x64): GUID de instancia DirectInput del
//  mando + su mapa de botones. Windows baraja esos GUID cada vez que los mandos
//  virtuales (ViGEm) se enchufan, así que el puesto N de Link acababa siendo otro
//  jugador en PES. Este módulo lee el GUID actual de cada puesto (serie ViGEm →
//  USB\VID_045E&PID_028E\<serie> → HID → DirectInput) y escribe casilla N = puesto N.
//  Comprobado en la PC de Fralex el 2026-10-10: PES respeta el orden de las casillas.
//
//  Reglas: solo con PES cerrado (lo lee al arrancar); copia settings.dat.phoenix-<fecha>
//  antes del primer cambio de cada sesión; escritura atómica; casillas sin puesto quedan
//  vacías (los mandos físicos que no estén en ninguna casilla PES los ignora); los
//  puestos de Link usan el mapa de botones Xbox que ya tuviera el archivo.
//  Todo corre en un hilo propio; el motor solo le pasa las series de sus mandos.
//  Puestos 9+ («Puestos máximos», EXPERIMENTAL): no caben en settings.dat; van a
//  phoenix_mandos_extra.dat con las mismas reglas (ver MandosExtra.h).
// =============================================================================

#include <string>
#include <vector>

namespace phoenix::ordenpes {

	struct Casilla {
		int n = 0;            ///< 1..8 (settings.dat) o 9+ (phoenix_mandos_extra.dat)
		int puesto = 0;       ///< puesto de Link que debería estar (0 = ninguno)
		bool ok = false;      ///< el archivo ya tiene ese mando ahí
		bool extra = false;   ///< casilla de phoenix_mandos_extra.dat (necesita el módulo de Sider de mandos extra)
	};

	struct Estado {
		bool activo = false;
		/// apagado | esperando | al_dia | ordenado | pes_abierto | sin_settings | error
		std::string estado = "apagado";
		std::vector<Casilla> casillas;
		std::string ultimo;    ///< hora del último orden escrito (HH:MM)
		std::string copia;     ///< nombre de la copia de seguridad de esta sesión
		std::string copiaExtra;   ///< copia de phoenix_mandos_extra.dat de esta sesión (vacía si no había archivo antes)
		std::string error;
		bool ds4windows = false;   ///< DS4Windows abierto: mete su propio mando virtual y desplaza a los de Link
	};

	/// Enciende o apaga el módulo (arranca el hilo la primera vez).
	void activar(bool si);
	/// El motor publica la serie ViGEm de cada puesto (índice = puesto-1; 0 = desenchufado). Del 9 en adelante = mandos extra.
	void publicarSeries(const std::vector<unsigned long>& seriePorPuesto);
	/// Fuerza una revisión ya (botón «Ordenar ahora»).
	void revisarYa();
	Estado estado();

	/// Utilidades puras (también para pruebas): leen/escriben las 8 casillas de settings.dat.
	bool esSettingsValido(const std::vector<unsigned char>& s);
	/// Devuelve el archivo con casilla N = guids[N-1] (16 bytes; vacío = casilla vacía) y mapa Xbox.
	std::vector<unsigned char> reordenar(const std::vector<unsigned char>& s,
		const std::vector<std::vector<unsigned char>>& guids,
		const std::vector<std::vector<unsigned char>>& guidsDeLink);
}
