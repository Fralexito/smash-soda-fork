#pragma once

#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
//  Phoenix Link · Contrato entre la interfaz y el motor de host
// -----------------------------------------------------------------------------
//  La interfaz (shell, pantalla Sala, barra superior) NUNCA habla con la clase
//  Hosting del autor: habla con esta interfaz. Así el mismo shell sirve para:
//    - la forma completa (con Parsec): ProveedorSalaSoda la implementa;
//    - la forma para todos (sin Parsec): no hay proveedor → las secciones de
//      host se ocultan y aparece «Activar modo host».
// =============================================================================

namespace phoenix {

	struct AsientoVista {
		int numero = 0;              ///< 1 = primer mando
		bool conectado = false;      ///< el mando virtual existe
		bool ocupado = false;
		std::string jugador;         ///< nombre Parsec del dueño
		uint32_t parsecId = 0;
		int pingMs = -1;             ///< -1 = sin dato
		bool bloqueado = false;
	};

	struct EspectadorVista {
		std::string nombre;
		uint32_t parsecId = 0;
		int pingMs = -1;
	};

	class ProveedorSala {
	public:
		virtual ~ProveedorSala() = default;

		virtual bool abierta() = 0;
		/// Abre la sala. Devuelve false y rellena `error` si no se pudo.
		virtual bool abrir(std::string& error) = 0;
		virtual void cerrar() = 0;

		virtual std::string enlace() = 0;
		virtual std::string nombreSala() = 0;
		virtual int plazas() = 0;
		virtual std::string cuentaHost() = 0;   ///< "Nombre #id" o vacío

		virtual std::vector<AsientoVista> asientos(int maximo) = 0;
		virtual std::vector<EspectadorVista> espectadores() = 0;
		virtual int totalInvitados() = 0;

		// Acciones sobre mandos (índice 0 = mando 1)
		virtual void conectarMando(int indice) = 0;
		virtual void desconectarMando(int indice) = 0;
		virtual void alternarBloqueo(int indice) = 0;
		virtual void liberarMando(int indice) = 0;
		/// Asigna el mando al invitado con ese ID Parsec (respeta la puerta de roles).
		virtual bool asignarMando(int indice, uint32_t parsecId) = 0;
		/// Sienta a cada jugador conectado en su mando reservado (≈ 1 vez por segundo).
		virtual void aplicarAsientosReservados() = 0;
		/// Saca de la sala al invitado con ese ID Parsec.
		virtual void expulsar(uint32_t parsecId) = 0;

		// --- Moderación y permisos por invitado (clave: ID Parsec) -------------
		virtual bool esMod(uint32_t parsecId) = 0;
		virtual bool esVip(uint32_t parsecId) = 0;
		/// Activa o quita el rol (si ya lo tiene, lo quita).
		virtual void alternarMod(uint32_t parsecId, const std::string& nombre) = 0;
		virtual void alternarVip(uint32_t parsecId, const std::string& nombre) = 0;
		/// Banea y saca de la sala. Devuelve false si no se pudo (p. ej. es policía de Soda).
		virtual bool banear(uint32_t parsecId, const std::string& nombre) = 0;
		virtual bool tecladoPermitido(uint32_t parsecId) = 0;
		virtual bool ratonPermitido(uint32_t parsecId) = 0;
		virtual void permitirTeclado(uint32_t parsecId, bool si) = 0;
		virtual void permitirRaton(uint32_t parsecId, bool si) = 0;
		/// El host maneja el mando (bloquea al dueño mientras tanto). activo=false lo devuelve.
		virtual void tomarControl(int indice, bool activo) = 0;
		/// Estado que el host envía al mando tomado (formato XInput: botones + stick izq.).
		virtual void inyectar(int indice, uint16_t botones, int16_t lx, int16_t ly) = 0;
		/// Calidad de transmisión (se aplica al instante y se guarda).
		virtual void aplicarVideo(int fps, int mbps) = 0;
	};

}
