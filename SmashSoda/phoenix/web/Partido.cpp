#include "Partido.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace phoenix::web {

	namespace detalle_partido {
		json ladoJson(const LadoPartido& l) {
			json jugadores = json::array();
			for (const JugadorPartido& p : l.jugadores) jugadores.push_back({ {"parsecId", p.parsecId}, {"nombre", p.nombre} });
			return { {"nombre", l.nombre}, {"jugadores", jugadores}, {"goles", l.goles} };
		}

		std::optional<LadoPartido> ladoDesde(const json& j) {
			if (!j.is_object()) return std::nullopt;
			LadoPartido l;
			l.nombre = j.value("nombre", "");
			l.goles = (std::max)(0, (std::min)(99, j.value("goles", 0)));
			if (j.contains("jugadores") && j["jugadores"].is_array()) {
				for (const json& p : j["jugadores"]) {
					if (!p.is_object()) continue;
					JugadorPartido jp;
					jp.parsecId = p.value("parsecId", 0u);
					jp.nombre = p.value("nombre", "");
					l.jugadores.push_back(jp);
				}
			}
			return l;
		}
	}

	json RegistroPartido::comoJson() const {
		return { {"id", id}, {"inicioMs", inicioMs}, {"duracionSeg", duracionSeg},
			{"a", detalle_partido::ladoJson(a)}, {"b", detalle_partido::ladoJson(b)}, {"ganador", ganador} };
	}

	std::optional<RegistroPartido> RegistroPartido::desdeJson(const json& j) {
		try {
			if (!j.is_object()) return std::nullopt;
			RegistroPartido r;
			r.id = j.value("id", static_cast<int64_t>(0));
			r.inicioMs = j.value("inicioMs", static_cast<int64_t>(0));
			r.duracionSeg = (std::max)(0, j.value("duracionSeg", 0));
			auto a = detalle_partido::ladoDesde(j.value("a", json::object()));
			auto b = detalle_partido::ladoDesde(j.value("b", json::object()));
			if (!a || !b) return std::nullopt;
			r.a = *a;
			r.b = *b;
			r.ganador = j.value("ganador", "");
			return r;
		}
		catch (...) {
			return std::nullopt;
		}
	}

	// ---------------------------------------------------------------------
	const char* Partido::nombreFase(FasePartido f) {
		switch (f) {
		case FasePartido::Listo: return "listo";
		case FasePartido::EnJuego: return "en_juego";
		case FasePartido::Pausado: return "pausado";
		default: return "libre";
		}
	}

	bool Partido::preparar(LadoPartido a, LadoPartido b) {
		if (_fase == FasePartido::EnJuego || _fase == FasePartido::Pausado) return false;
		if (a.jugadores.empty() || b.jugadores.empty()) return false;
		for (const JugadorPartido& x : a.jugadores)
			for (const JugadorPartido& y : b.jugadores)
				if (x.parsecId != 0 && x.parsecId == y.parsecId) return false; // nadie juega en los dos lados
		a.goles = 0;
		b.goles = 0;
		if (a.nombre.empty()) a.nombre = a.jugadores.front().nombre;
		if (b.nombre.empty()) b.nombre = b.jugadores.front().nombre;
		_a = std::move(a);
		_b = std::move(b);
		_fase = FasePartido::Listo;
		_pausaTotal = 0.0;
		return true;
	}

	bool Partido::iniciar(int64_t epochMs, double ahoraSeg) {
		if (_fase != FasePartido::Listo) return false;
		_fase = FasePartido::EnJuego;
		_inicioMs = epochMs;
		_inicioSeg = ahoraSeg;
		_pausaTotal = 0.0;
		_a.goles = 0;
		_b.goles = 0;
		return true;
	}

	bool Partido::pausar(bool pausa, double ahoraSeg) {
		if (pausa && _fase == FasePartido::EnJuego) {
			_fase = FasePartido::Pausado;
			_pausadoDesde = ahoraSeg;
			return true;
		}
		if (!pausa && _fase == FasePartido::Pausado) {
			_fase = FasePartido::EnJuego;
			_pausaTotal += (std::max)(0.0, ahoraSeg - _pausadoDesde);
			return true;
		}
		return false;
	}

	bool Partido::gol(int lado, int delta) {
		if (_fase != FasePartido::EnJuego && _fase != FasePartido::Pausado) return false;
		if (lado != 0 && lado != 1) return false;
		LadoPartido& l = lado == 0 ? _a : _b;
		l.goles = (std::max)(0, (std::min)(99, l.goles + delta));
		return true;
	}

	int Partido::segundosJugados(double ahoraSeg) const {
		if (_fase != FasePartido::EnJuego && _fase != FasePartido::Pausado) return 0;
		double t = ahoraSeg - _inicioSeg - _pausaTotal;
		if (_fase == FasePartido::Pausado) t -= (ahoraSeg - _pausadoDesde);
		return static_cast<int>((std::max)(0.0, t));
	}

	std::optional<RegistroPartido> Partido::finalizar(double ahoraSeg) {
		if (_fase != FasePartido::EnJuego && _fase != FasePartido::Pausado) return std::nullopt;
		RegistroPartido r;
		r.id = _inicioMs;
		r.inicioMs = _inicioMs;
		r.duracionSeg = segundosJugados(ahoraSeg);
		r.a = _a;
		r.b = _b;
		r.ganador = _a.goles > _b.goles ? "a" : (_b.goles > _a.goles ? "b" : "empate");
		_fase = FasePartido::Libre;
		_invertido = false;
		return r;
	}

	void Partido::cancelar() {
		_fase = FasePartido::Libre;
		_invertido = false;
		_a = LadoPartido();
		_b = LadoPartido();
	}

	std::string Partido::marcador() const {
		return std::to_string(_a.goles) + "-" + std::to_string(_b.goles);
	}

	json Partido::comoJson(double ahoraSeg) const {
		return {
			{"fase", nombreFase(_fase)},
			{"a", detalle_partido::ladoJson(_a)},
			{"b", detalle_partido::ladoJson(_b)},
			{"segundos", segundosJugados(ahoraSeg)},
			{"inicioMs", _inicioMs},
			{"invertido", _invertido},
		};
	}

	// ---------------------------------------------------------------------
	std::vector<RegistroPartido> HistorialPartidos::lista() const {
		std::vector<RegistroPartido> r;
		try {
			std::ifstream f(_ruta, std::ios::binary);
			if (!f) return r;
			std::stringstream ss;
			ss << f.rdbuf();
			const json j = json::parse(ss.str(), nullptr, false);
			if (j.is_discarded()) return r;
			const json& arr = j.is_object() && j.contains("partidos") ? j["partidos"] : j;
			if (!arr.is_array()) return r;
			for (const json& x : arr) {
				auto reg = RegistroPartido::desdeJson(x);
				if (reg) r.push_back(*reg);
			}
			std::sort(r.begin(), r.end(), [](const RegistroPartido& a, const RegistroPartido& b) { return a.inicioMs > b.inicioMs; });
		}
		catch (...) {
			r.clear();
		}
		return r;
	}

	bool HistorialPartidos::agregar(const RegistroPartido& nuevo) {
		try {
			std::vector<RegistroPartido> todos = lista();
			todos.insert(todos.begin(), nuevo);
			if (todos.size() > kMaximo) todos.resize(kMaximo);
			json arr = json::array();
			for (const RegistroPartido& r : todos) arr.push_back(r.comoJson());
			const json archivo = { {"version", 1}, {"partidos", arr} };

			const std::string temporal = _ruta + ".tmp";
			{
				std::ofstream f(temporal, std::ios::binary | std::ios::trunc);
				if (!f) return false;
				f << archivo.dump(1, '\t', false, json::error_handler_t::replace);
				if (!f.good()) return false;
			}
			std::error_code ec;
			std::filesystem::rename(temporal, _ruta, ec); // reemplaza el anterior (Windows y Linux)
			if (ec) {
				std::filesystem::remove(temporal, ec);
				return false;
			}
			return true;
		}
		catch (...) {
			return false;
		}
	}

}
