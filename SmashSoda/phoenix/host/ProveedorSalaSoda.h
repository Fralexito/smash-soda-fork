#pragma once

#include "../core/ProveedorSala.h"

class Hosting;
class HostSettingsWidget;

// =============================================================================
//  Phoenix Soda · Proveedor de sala sobre el motor de Smash Soda (Parsec)
//  Solo existe en la forma completa (modo host).
// =============================================================================

namespace phoenix {

	class ProveedorSalaSoda : public ProveedorSala {
	public:
		ProveedorSalaSoda(Hosting& hosting, HostSettingsWidget& ajustes);

		bool abierta() override;
		bool abrir(std::string& error) override;
		void cerrar() override;
		std::string enlace() override;
		std::string nombreSala() override;
		int plazas() override;
		std::string cuentaHost() override;
		std::vector<AsientoVista> asientos(int maximo) override;
		std::vector<EspectadorVista> espectadores() override;
		int totalInvitados() override;
		void conectarMando(int indice) override;
		void desconectarMando(int indice) override;
		void alternarBloqueo(int indice) override;
		void liberarMando(int indice) override;
		bool asignarMando(int indice, uint32_t parsecId) override;

	private:
		Hosting& _hosting;
		HostSettingsWidget& _ajustes;
	};

}
