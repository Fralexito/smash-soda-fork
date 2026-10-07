#include "Solicitudes.h"

#include <algorithm>
#include <cctype>
#include <chrono>

namespace phoenix {

	namespace {
		constexpr double kVidaSeg = 60.0;

		double ahoraSeg() {
			using namespace std::chrono;
			return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
		}

		std::string minusculas(std::string s) {
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		}
	}

	Solicitudes& Solicitudes::instancia() {
		static Solicitudes unica;
		return unica;
	}

	bool Solicitudes::procesarChat(const std::string& mensaje, uint32_t parsecId, const std::string& nombre,
		bool puedePedir, std::string& respuesta) {

		const std::string m = minusculas(mensaje);
		const bool esCambio = m.rfind("!cambio", 0) == 0 || m.rfind("!mando", 0) == 0;
		const bool esEquipo = m.rfind("!equipo", 0) == 0;
		if (!esCambio && !esEquipo) return false;

		if (!puedePedir) {
			respuesta = nombre + ": solo los jugadores pueden pedir cambios.";
			return true;
		}

		int destino = 0;
		if (esCambio) {
			const size_t p = m.find_first_of("0123456789");
			if (p != std::string::npos) destino = std::atoi(m.c_str() + p);
			if (destino < 1 || destino > 8) {
				respuesta = nombre + ": escribe !cambio y el número de mando (1 a 8).";
				return true;
			}
		}

		std::lock_guard<std::mutex> lock(_mutex);
		_lista.erase(std::remove_if(_lista.begin(), _lista.end(),
			[&](const Solicitud& s) { return s.parsecId == parsecId; }), _lista.end());
		_lista.push_back({ parsecId, nombre, destino, ahoraSeg() });
		respuesta = destino > 0
			? nombre + " pidió el mando " + std::to_string(destino) + ". El host debe aceptarlo."
			: nombre + " pidió cambiar de equipo. El host debe aceptarlo.";
		return true;
	}

	std::vector<Solicitud> Solicitudes::pendientes() {
		std::lock_guard<std::mutex> lock(_mutex);
		const double ahora = ahoraSeg();
		_lista.erase(std::remove_if(_lista.begin(), _lista.end(),
			[&](const Solicitud& s) { return ahora - s.creada > kVidaSeg; }), _lista.end());
		return _lista;
	}

	void Solicitudes::quitar(uint32_t parsecId) {
		std::lock_guard<std::mutex> lock(_mutex);
		_lista.erase(std::remove_if(_lista.begin(), _lista.end(),
			[&](const Solicitud& s) { return s.parsecId == parsecId; }), _lista.end());
	}

}
