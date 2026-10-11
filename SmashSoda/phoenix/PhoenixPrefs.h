#pragma once

#include <atomic>
#include <string>

// =============================================================================
//  Phoenix Link · Preferencias de interfaz
// -----------------------------------------------------------------------------
//  Archivo propio `phoenix-ui.json` en la carpeta de configuración de Smash
//  Soda. Separado del config.json del autor para no mezclar formatos.
//  Si el archivo falta o está dañado se usan los valores por defecto.
// =============================================================================

namespace phoenix {

	struct PhoenixPrefs {
		bool interfazPhoenix = true;  ///< false = interfaz clásica del Smash Soda
		bool interfazWeb = true;      ///< interfaz nueva en HTML (WebView2); si no se puede, se usa la ImGui
		std::string tema = "galaxy";  ///< tema de la interfaz web: galaxy | sudario
		std::string idioma = "es";
		int seccion = 0;              ///< sección abierta al iniciar

		// Sala (se enviarán a la web con PhoenixLink)
		std::string visibilidad = "amigos"; ///< publica | amigos | privada
		bool espectadores = true;
		bool entradaParsec = false; ///< dejar entrar a quien no está en la lista (el host decide)
		int limiteEspectadores = 4;
		std::string juego = "eFootball PES 2021";
		std::string parche = "Conmegol";
		std::string region = "Lima";
		int mandosActivos = 2;    ///< 2–8 mandos visibles en el tablero
		bool avisosEnJuego = true; ///< escribir los avisos de la web en el juego (buzon de Sider)
		std::string modoPes = "off";  ///< prueba «Solo PES 2021»: off | publicar (la sala solo se publica con PES abierto) | estricto (ademas no deja abrir la sala sin PES)
		std::string carpetaJuego;  ///< ultima carpeta de PES2021.exe detectada (UTF-8), para el repartidor de datos
		int equipoLocal = 1;      ///< cuántos mandos son del equipo local
		bool anchoAuto = true;    ///< ancho de banda automático: sube y baja solo según los invitados conectados
		int anchoPorPersona = 10;  ///< Mbps por persona cuando el ancho automático está activo (1–50)
		int subidaMbps = 0;       ///< subida de internet del anfitrión en Mbps (0 = no la sabe); el automático no pasa del 80 %
		bool competitivo = true;  ///< árbitro: con el partido en juego, Start/Back/Guía bloqueados a los invitados
		bool pausaAuto = false;   ///< árbitro: pausa si un jugador del partido se cae o pasa de 150 ms de ping 5 s (apagado hasta probarlo)
		bool marcadorAuto = true; ///< árbitro: con datos del juego, el marcador arranca, relata los goles y termina solo
		bool ordenPes = false;        ///< EXPERIMENTAL: Link ordena settings.dat de PES (casilla N = puesto N) y no desenchufa sus mandos al cerrar la sala
		// Anfitrión: su puesto al jugar (Mandos › TU MANDO)
		std::string miMando;             ///< «VID:PID» de su mando físico; Link lo vuelve a elegir al abrirse ("" = teclado)
		std::string miPuestoModo = "fijo";  ///< fijo | local | visitante (primero de ese equipo)
		int miPuestoNumero = 1;          ///< con modo fijo
		bool sentarmeAlAbrir = true;     ///< al abrir la sala, el anfitrión ya queda sentado (si tiene su mando y hay sitio)
		bool entrarPulsando = true;      ///< sin puesto, A/B/X/Y en su mando lo sienta en un puesto libre
		bool soloAutorizados = true;  ///< puestos: pulsar el mando solo da puesto a quien el host autorizó (los demás «quieren jugar»)
		std::string modoRecarga = "AUTO-SIEMPRE";  ///< phoenix.lua v0.18b (modo.txt): AUTO-SIEMPRE (por defecto) | AUTO-FICHAJES | ACTIVAR

		std::string cedidoExtras;  ///< modo cedido: más ejecutables permitidos, separados por comas (solo a mano en el archivo)

		static PhoenixPrefs& get();
		void cargar();
		void guardar() const;
	};

	/// Peor pérdida de paquetes entre los invitados, en décimas de por ciento (50 = 5 %). La escribe Hosting cada 2 s.
	inline std::atomic<int> g_perdidaPorMil{ 0 };

}
