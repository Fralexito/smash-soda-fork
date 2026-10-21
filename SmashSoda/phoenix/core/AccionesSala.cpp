#include "AccionesSala.h"

#include <vector>

#include "ProveedorSala.h"
#include "Solicitudes.h"
#include "../PhoenixPrefs.h"
#include "../PhoenixRoles.h"

namespace phoenix {

	namespace AccionesSala {

		bool asignarConectando(ProveedorSala& sala, int indice, uint32_t parsecId) {
			const std::vector<AsientoVista> asientos = sala.asientos(equipos::kPuestos);
			if (indice < 0 || indice >= static_cast<int>(asientos.size()) || parsecId == 0) return false;
			if (!asientos[indice].conectado) sala.conectarMando(indice);
			return sala.asignarMando(indice, parsecId);
		}

		bool aceptarSolicitud(ProveedorSala& sala, uint32_t parsecId) {
			Solicitud sol;
			bool hay = false;
			for (const Solicitud& s : Solicitudes::instancia().pendientes()) {
				if (s.parsecId == parsecId) { sol = s; hay = true; break; }
			}
			if (!hay) {
				// «Quiere jugar»: pulsó su mando sin permiso (espectador o no autorizado) → primer puesto libre
				for (uint32_t id : PhoenixRoles::instancia().quierenJugar()) {
					if (id != parsecId) continue;
					PhoenixRoles::instancia().olvidarPedido(parsecId, false);
					decidirEspera(sala, parsecId, DecisionEspera::Jugador);
					return true;
				}
				return false;
			}

			const std::vector<AsientoVista> asientos = sala.asientos(equipos::kPuestos);
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			int destino = sol.mandoDestino - 1;
			if (sol.mandoDestino == 0) {
				// Pasar al otro equipo: primer mando libre del otro lado (o el primero de ese lado)
				int actual = -1;
				for (size_t i = 0; i < asientos.size(); i++) {
					if (asientos[i].ocupado && asientos[i].parsecId == sol.parsecId) actual = static_cast<int>(i);
				}
				// El otro equipo según el mapa por puesto (quien no está sentado pasa al A)
				const char otro = (actual >= 0 && pr.equipoDe(actual) == 'A') ? 'B' : 'A';
				destino = -1;
				for (int i = 0; i < static_cast<int>(asientos.size()); i++) {
					if (pr.equipoDe(i) != otro) continue;
					if (destino < 0) destino = i;   // por si están todos ocupados: el primero de ese equipo
					if (!asientos[i].ocupado) { destino = i; break; }
				}
			}
			if (destino >= 0 && destino < static_cast<int>(asientos.size())) {
				if (!asientos[destino].conectado) sala.conectarMando(destino);
				sala.asignarMando(destino, sol.parsecId);
			}
			Solicitudes::instancia().quitar(sol.parsecId);
			return true;
		}

		void rechazarSolicitud(uint32_t parsecId) {
			Solicitudes::instancia().quitar(parsecId);
			PhoenixRoles::instancia().olvidarPedido(parsecId, true);   // «quiere jugar» rechazado: 60 s sin volver a pedir
		}

		void decidirEspera(ProveedorSala& sala, uint32_t parsecId, DecisionEspera decision) {
			switch (decision) {
			case DecisionEspera::Jugador: {
				PhoenixRoles::instancia().admitir(parsecId, true);
				const std::vector<AsientoVista> asientos = sala.asientos(equipos::kPuestos);
				const PhoenixPrefs& pr = PhoenixPrefs::get();
				const int n = (std::min)(pr.mandosActivos, static_cast<int>(asientos.size()));
				const auto sirve = [&](int i) { return i >= 0 && i < n && !asientos[i].ocupado && pr.equipoDe(i) != '-'; };   // un puesto fuera de los equipos no se da
				int destino = -1;
				// Puesto fijado (candado): el suyo; si está ocupado, el siguiente libre (dando la vuelta)
				const int fijado = pr.puestoFijadoDe(parsecId);
				if (fijado >= 1) {
					for (int k = 0; k < n && destino < 0; k++) {
						const int i = (fijado - 1 + k) % n;
						if (sirve(i)) destino = i;
					}
				}
				for (int i = 0; i < n && destino < 0; i++) if (sirve(i)) destino = i;
				if (destino >= 0) {
					if (!asientos[destino].conectado) sala.conectarMando(destino);
					sala.asignarMando(destino, parsecId);
				}
				break;
			}
			case DecisionEspera::Espectador:
				PhoenixRoles::instancia().admitir(parsecId, false);
				PhoenixRoles::instancia().mandarAMirar(parsecId);   // aunque pulse su mando, no toma puesto
				break;
			case DecisionEspera::Expulsar:
				PhoenixRoles::instancia().admitir(parsecId, false);
				sala.expulsar(parsecId);
				break;
			}
		}

		bool intercambiar(ProveedorSala& sala, int a, int b) {
			const std::vector<AsientoVista> asientos = sala.asientos(equipos::kPuestos);
			const int n = static_cast<int>(asientos.size());
			if (a < 0 || b < 0 || a >= n || b >= n || a == b) return false;
			const uint32_t idA = asientos[a].ocupado ? asientos[a].parsecId : 0;
			const uint32_t idB = asientos[b].ocupado ? asientos[b].parsecId : 0;
			if (idA == 0 && idB == 0) return false;
			if (!asientos[a].conectado) sala.conectarMando(a);
			if (!asientos[b].conectado) sala.conectarMando(b);
			// asignarMando suelta cualquier otro mando del jugador y pisa al dueño anterior,
			// así que con dos llamadas los dos quedan cruzados.
			if (idA != 0 && idB != 0) {
				sala.asignarMando(b, idA);
				sala.asignarMando(a, idB);
			}
			else if (idA != 0) {
				sala.asignarMando(b, idA);
			}
			else {
				sala.asignarMando(a, idB);
			}
			return true;
		}

	}

}
