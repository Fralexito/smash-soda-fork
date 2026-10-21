#pragma once
// Traspasos en vivo · el servicio de Link: un hilo propio (no toca el bucle de la web) que cada pocos segundos pasa el Motor
// (anfitrion: mira el option file; amigo: trae lotes y los aplica). Todo APAGADO por defecto. docs/FICHAJES-Y-SYNC.md («TRASPASOS-EN-VIVO»)
#include <nlohmann/json.hpp>
#include <string>

namespace phoenix::traspasos {

class Servicio {
public:
	static Servicio& get();
	/// Carga los ajustes guardados y arranca el hilo si algun interruptor esta encendido. Seguro llamarlo varias veces.
	void iniciar();
	void detener();
	/// Estado completo para la interfaz (ajustes + motor + juego + rutas).
	nlohmann::json estado();
	/// Cambia ajustes (campos opcionales: publicar, recibir, confirmarAntes, transporte, carpeta, autor, optionFile). Lanza std::runtime_error con texto legible.
	void poner(const nlohmann::json& datos);
	/// publicar|descartar|deshacer|olvidarReferencia|reintentar (propias) y, con la web: rotarCodigo|seguirCodigo{codigo}|dejarDeSeguir{anfitrion_id}. `dato` = el código o el id. Lanza std::runtime_error.
	std::string accion(const std::string& que, const std::string& dato = std::string());
	~Servicio();

private:
	Servicio();
	struct Impl;
	Impl* _i;
};

} // namespace phoenix::traspasos
