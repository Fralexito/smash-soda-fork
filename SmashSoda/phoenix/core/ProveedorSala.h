#pragma once

#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
//  Phoenix Soda · Contrato entre la interfaz y el motor de host
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
	};

}
