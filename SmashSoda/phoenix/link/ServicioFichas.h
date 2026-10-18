#pragma once
// Estadísticas e información oficiales de la web (migración 126) · el servicio de Link: un hilo propio que cada minuto baja los
// cambios «stats» / «info» de GET /functions/v1/traspasos/v1/cambios?destino=base (contrato §3c), los pone en el Player.bin de la
// Phoenix-DB de cada modo del juego (link/Fichas: sobre la base, con copia, todo o nada) y manda el resultado a POST /pc/informe (§4).
// Encendido siempre que el PC esté vinculado (los cambios oficiales no piden aprobación, igual que los fichajes oficiales).
#include <nlohmann/json.hpp>

namespace phoenix::fichas {

class Servicio {
public:
	static Servicio& get();
	void iniciar();   ///< seguro llamarlo varias veces
	void detener();
	nlohmann::json estado();   ///< para la interfaz: último mensaje, cambios conocidos, resultado por modo
	/// «reintentar» (vuelve a bajar todo de la web y aplica ya) | «quitar» (devuelve el juego a las stats de la base). Lanza std::runtime_error.
	void accion(const std::string& que);
	~Servicio();
private:
	Servicio();
	struct Impl;
	Impl* _i;
};

} // namespace phoenix::fichas
