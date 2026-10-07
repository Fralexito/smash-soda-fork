#include "ProveedorSalaSoda.h"

#include <set>

#include "../../Hosting.h"
#include "../../widgets/HostSettingsWidget.h"
#include "../PhoenixRoles.h"

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
				a.bloqueado = pad->isLocked();
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

	namespace {
		AGamepad* padEn(Hosting& h, int i) {
			std::vector<AGamepad*>& lista = h.getGamepadClient().gamepads;
			return (i >= 0 && i < static_cast<int>(lista.size())) ? lista[i] : nullptr;
		}
	}

	void ProveedorSalaSoda::conectarMando(int i) { try { if (AGamepad* p = padEn(_hosting, i)) p->connect(); } catch (...) {} }
	void ProveedorSalaSoda::desconectarMando(int i) { try { if (AGamepad* p = padEn(_hosting, i)) p->disconnect(); } catch (...) {} }
	void ProveedorSalaSoda::alternarBloqueo(int i) { try { if (AGamepad* p = padEn(_hosting, i)) p->toggleLocked(); } catch (...) {} }
	void ProveedorSalaSoda::liberarMando(int i) { try { _hosting.stripGamepad(i); } catch (...) {} }

	bool ProveedorSalaSoda::asignarMando(int i, uint32_t parsecId) {
		try {
			AGamepad* p = padEn(_hosting, i);
			if (p == nullptr) return false;
			// El host manda: mover a un jugador cambia su asiento reservado.
			// Un espectador marcado por la web nunca recibe mando.
			if (!PhoenixRoles::instancia().moverAsiento(parsecId, i + 1)) return false;
			for (Guest& g : _hosting.getGuests()) {
				if (g.userID != parsecId) continue;
				// Si ya tenía otro mando, lo suelta (un jugador = un mando al moverlo)
				for (AGamepad* otro : _hosting.getGamepadClient().gamepads) {
					if (otro != nullptr && otro != p && otro->isOwned() && otro->owner.guest.userID == parsecId) otro->clearOwner();
				}
				_hosting.setOwner(*p, g, 0);
				return true;
			}
		}
		catch (...) {}
		return false;
	}

	void ProveedorSalaSoda::aplicarAsientosReservados() {
		try {
			if (!_hosting.isRunning()) return;
			std::vector<AGamepad*>& mandos = _hosting.getGamepadClient().gamepads;
			for (Guest& g : _hosting.getGuests()) {
				const int asiento = PhoenixRoles::instancia().asientoDe(g.userID);
				if (asiento <= 0 || asiento > static_cast<int>(mandos.size())) continue;
				AGamepad* p = mandos[asiento - 1];
				if (p == nullptr || (p->isOwned() && p->owner.guest.userID == g.userID)) continue;
				if (!p->isConnected()) p->connect();
				for (AGamepad* otro : mandos) {   // suelta cualquier otro mando que tuviera
					if (otro != nullptr && otro != p && otro->isOwned() && otro->owner.guest.userID == g.userID) otro->clearOwner();
				}
				if (p->isOwned()) p->clearOwner(); // el asiento es suyo por reserva
				_hosting.setOwner(*p, g, 0);
			}
		}
		catch (...) {}
	}

	void ProveedorSalaSoda::expulsar(uint32_t parsecId) {
		try {
			for (Guest& g : _hosting.getGuests()) {
				if (g.userID == parsecId) { ParsecHostKickGuest(_hosting.getParsec(), g.id); return; }
			}
		}
		catch (...) {}
	}

	void ProveedorSalaSoda::aplicarVideo(int fps, int mbps) {
		try {
			_hosting.setHostVideoConfig(static_cast<uint32_t>(fps), static_cast<uint32_t>(mbps));
			if (_hosting.isRunning()) _hosting.applyHostConfig();
			Config::cfg.Save();
		}
		catch (...) {}
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
