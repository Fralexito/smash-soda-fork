#pragma once

// =============================================================================
//  Phoenix Link · Modo cedido, parte de Windows.
//  - Estado compartido entre hilos (atómico): ¿activo? ¿a quién?
//  - dejarPasar(): filtro de la entrada del invitado ANTES de inyectarla
//    (lo llama InputControlService en el hilo de entrada).
//  - Guardián: hilo que devuelve el foco a PES o a Link si se va a otra ventana
//    y el anfitrión no está usando su PC.
//  - Atajo físico del anfitrión (Ctrl+Alt+F12) desde el gancho de teclado.
//  La lógica pura (qué tecla pasa, PIN, acciones) está en phoenix/core/ModoCedido.
// =============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace phoenix::cedido {

	struct Cesion {
		uint32_t parsecId = 0;
		std::string nombre;
		void* ventanaLink = nullptr;        ///< HWND principal de Link
		std::string carpetaJuego;           ///< carpeta de PES2021.exe (UTF-8)
		std::string exeJuego;               ///< ejecutable del juego de la biblioteca (UTF-8)
		std::vector<std::string> extras;    ///< más ejecutables permitidos
	};

	// ---- Cualquier hilo ---------------------------------------------------------
	bool activo();
	uint32_t invitado();
	std::string nombreInvitado();
	int64_t desdeMs();                      ///< hora (epoch ms) en que empezó

	// ---- Hilo de la ventana -------------------------------------------------------
	void empezar(const Cesion& c);
	/// Apaga todo y suelta lo que el invitado tuviera pulsado. Idempotente.
	void terminar();
	/// El guardián tuvo un problema: devuelve true una vez (y el motivo).
	bool tomarFallo(std::string& motivo);
	/// El anfitrión pulsó Ctrl+Alt+F12 en su teclado físico: true una vez.
	bool tomarAtajo();
	/// ¿El anfitrión tocó su teclado o ratón físico hace menos de `ventanaMs`? En ese rato la entrada
	/// del invitado está en pausa (InputControlService, 300 ms), así que lo que pase en la ventana es suyo.
	bool anfitrionTocando(uint64_t ventanaMs = 300);
	/// ¿Hay un proceso de PES abierto? (por nombre y carpeta)
	bool pesAbierto(const std::string& carpetaJuego, const std::string& exeJuego);
	/// Trae PES al frente o lo abre. false + error si no se pudo.
	bool abrirPes(const std::string& ruta, const std::string& parametros, const std::string& carpetaJuego, std::string& error);

	// ---- Hilo de entrada --------------------------------------------------------------
	/// true = inyectar. Solo se llama con el modo activo (ver activo()).
	bool dejarTecla(uint16_t vk, bool pulsada);
	bool dejarBoton(int boton, bool pulsado);   ///< 0 izq, 1 der, 2 medio, 3 X1, 4 X2
	bool dejarRueda();

	// ---- Gancho de teclado (hilo del gancho; tiene que ser rapidísimo) ----------------
	/// Tecla física del anfitrión (no inyectada). Detecta Ctrl+Alt+F12.
	void teclaFisica(unsigned long vk, bool pulsada);

	constexpr const char* ATAJO = "Ctrl+Alt+F12";

}
