#pragma once

#include "../core/ProveedorSala.h"

class Hosting;
class HostSettingsWidget;

// =============================================================================
//  Phoenix Link · Proveedor de sala sobre el motor de Smash Soda (Parsec)
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
		void aplicarAsientosReservados() override;
		void expulsar(uint32_t parsecId) override;
		bool esMod(uint32_t parsecId) override;
		bool esVip(uint32_t parsecId) override;
		void alternarMod(uint32_t parsecId, const std::string& nombre) override;
		void alternarVip(uint32_t parsecId, const std::string& nombre) override;
		bool banear(uint32_t parsecId, const std::string& nombre) override;
		bool tecladoPermitido(uint32_t parsecId) override;
		bool ratonPermitido(uint32_t parsecId) override;
		void permitirTeclado(uint32_t parsecId, bool si) override;
		void permitirRaton(uint32_t parsecId, bool si) override;
		void tomarControl(int indice, bool activo) override;
		void inyectar(int indice, uint16_t botones, int16_t lx, int16_t ly) override;
		void aplicarVideo(int fps, int mbps) override;

	private:
		Hosting& _hosting;
		HostSettingsWidget& _ajustes;
	};

}
