#pragma once

#include <functional>
#include <string>

class Hosting;
class HostSettingsWidget;

// =============================================================================
//  Phoenix Link · Interfaz nueva (HTML dentro de la app)
// -----------------------------------------------------------------------------
//  Une el WebView2 (AnfitrionWeb), el Puente y el motor de Smash Soda:
//    - registra las acciones que la interfaz puede pedir (abrir sala, asignar
//      mando, banear…), cada una por el MISMO camino que su botón original;
//    - manda ~5 veces por segundo una foto del estado (solo si cambió);
//    - manda eventos (chat, actividad, resultados de la web).
//
//  Fallo seguro: si WebView2 no está o falla, cubreVentana() es false y la app
//  sigue con la interfaz ImGui de siempre; renderImGui() avisa y ofrece
//  reintentar o instalar el componente de Microsoft.
// =============================================================================

namespace phoenix {
	class ProveedorSala;
}

namespace phoenix::web {

	struct ContextoWeb {
		Hosting* hosting = nullptr;
		HostSettingsWidget* ajustesSala = nullptr;
		ProveedorSala* sala = nullptr;
		void* ventana = nullptr;                  ///< HWND principal
		std::function<void()> alMensajeChat;      ///< mismo aviso que el chat clásico (parpadeo + sonido)
	};

	class InterfazWeb {
	public:
		static InterfazWeb& instancia();

		void iniciar(const ContextoWeb& contexto);
		void cerrar();

		/// Cada frame, en el hilo de la ventana. `permitida` = false mientras se
		/// muestran pantallas propias de ImGui (inicio de sesión de Parsec, etc.).
		void tick(bool permitida);

		/// La interfaz web cubre la ventana: ImGui no debe dibujar el shell.
		bool cubreVentana() const;

		/// La interfaz nueva está arrancando (WebView2 cargando): se muestra una pantalla de carga
		/// en vez de la interfaz antigua, para que no «parpadee» al abrir.
		bool cargando() const;
		/// Pantalla de carga (fondo oscuro con el nombre del programa).
		void renderCarga();
		/// Logo para la pantalla de carga (ID3D11ShaderResourceView*; nullptr = solo texto).
		void fijarLogoCarga(void* textura);

		/// Dibuja (con ImGui) el botón «Volver a la interfaz nueva» cuando se está
		/// en el panel clásico, o el aviso si la interfaz nueva no pudo abrirse.
		void renderImGui();

		// Ganchos de la ventana (WndProc)
		void alRedimensionar(bool minimizada);
		void alMoverVentana();

		/// Cambia la preferencia (desde la interfaz ImGui: «Usar la interfaz nueva»).
		void usarWeb(bool si);
		bool disponible() const;
		std::string motivoNoDisponible() const;
	};

}
