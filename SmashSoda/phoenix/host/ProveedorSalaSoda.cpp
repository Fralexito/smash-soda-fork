#include "ProveedorSalaSoda.h"

#include <set>

#include "../../Hosting.h"
#include "../../widgets/HostSettingsWidget.h"

namespace phoenix {

	namespace {
		int pingDe(Hosting& hosting, uint32_t idInterno) {
			try {
				const MyMetrics m = hosting.getMetrics(idInterno);
				const float ms = m.metrics.networkLatency;
				return ms > 0.0f ? static_cast<int>(ms + 0.5f) : -1;
			}
			catch (...) {
				return -1;
			}
		}
	}

	ProveedorSalaSoda::ProveedorSalaSoda(Hosting& hosting, HostSettingsWidget& ajustes)
		: _hosting(hosting), _ajustes(ajustes) {
	}

	bool ProveedorSalaSoda::abierta() { return _hosting.isRunning(); }

	bool ProveedorSalaSoda::abrir(std::string& error) { return _ajustes.phoenixAbrirSala(error); }

	void ProveedorSalaSoda::cerrar() { _ajustes.phoenixCerrarSala(); }

	std::string ProveedorSalaSoda::enlace() { return _ajustes.phoenixEnlace(); }

	std::string ProveedorSalaSoda::nombreSala() { return _ajustes.phoenixNombreSala(); }

	int ProveedorSalaSoda::plazas() { return _ajustes.phoenixPlazas(); }

	std::string ProveedorSalaSoda::cuentaHost() {
		Guest& host = _hosting.getHost();
		if (!host.isValid()) return std::string();
		return host.name + "  #" + std::to_string(host.userID);
	}

	std::vector<AsientoVista> ProveedorSalaSoda::asientos(int maximo) {
		std::vector<AsientoVista> lista;
		try {
			GamepadClient& mandos = _hosting.getGamepadClient();
			int numero = 0;
			for (AGamepad* pad : mandos.gamepads) {
				if (pad == nullptr) continue;
				if (numero >= maximo) break;
				AsientoVista a;
				a.numero = ++numero;
				a.conectado = pad->isConnected();
				a.ocupado = pad->isOwned();
				if (a.ocupado) {
					a.jugador = pad->owner.guest.name;
					a.parsecId = pad->owner.guest.userID;
					a.pingMs = pingDe(_hosting, pad->owner.guest.id);
				}
				lista.push_back(a);
			}
		}
		catch (...) {
			// Lista parcial antes que tumbar la interfaz.
		}
		return lista;
	}

	std::vector<EspectadorVista> ProveedorSalaSoda::espectadores() {
		std::vector<EspectadorVista> lista;
		try {
			std::set<uint32_t> conMando;
			for (AGamepad* pad : _hosting.getGamepadClient().gamepads) {
				if (pad != nullptr && pad->isOwned()) conMando.insert(pad->owner.guest.userID);
			}
			const uint32_t idHost = _hosting.getHost().userID;
			for (Guest& g : _hosting.getGuests()) {
				if (g.userID == idHost || conMando.count(g.userID)) continue;
				EspectadorVista e;
				e.nombre = g.name;
				e.parsecId = g.userID;
				e.pingMs = pingDe(_hosting, g.id);
				lista.push_back(e);
			}
		}
		catch (...) {
		}
		return lista;
	}

	int ProveedorSalaSoda::totalInvitados() {
		try {
			return static_cast<int>(_hosting.getGuests().size());
		}
		catch (...) {
			return 0;
		}
	}

}
