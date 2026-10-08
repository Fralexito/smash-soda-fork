#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// =============================================================================
//  Phoenix Link · Perfiles de sala (ajustes guardados con nombre)
// -----------------------------------------------------------------------------
//  Ej.: «Liga oficial», «Amistosos». Cada perfil guarda los mismos valores que
//  se cambian a mano en Sala y Mandos (plazas, limitador, visibilidad, juego,
//  formación, calidad…). Aplicarlo usa exactamente los mismos caminos que los
//  botones de la interfaz. Archivo: phoenix-perfiles-sala.json (máx. 20).
//  Lógica pura de archivo: se prueba en Linux.
// =============================================================================

namespace phoenix::web {

	struct PerfilSala {
		std::string nombre;
		nlohmann::json valores = nlohmann::json::object();
	};

	class PerfilesSala {
	public:
		static constexpr size_t kMaximo = 20;
		static constexpr size_t kLargoNombre = 40;

		explicit PerfilesSala(std::string ruta) : _ruta(std::move(ruta)) {}

		std::vector<PerfilSala> lista() const;
		std::optional<PerfilSala> obtener(const std::string& nombre) const;
		/// Crea o reemplaza (mismo nombre sin distinguir mayúsculas). Devuelve false
		/// si el nombre es inválido, si ya hay 20 o si no se pudo escribir.
		bool guardar(const std::string& nombre, const nlohmann::json& valores);
		bool borrar(const std::string& nombre);

		/// Limpia un nombre: sin espacios sobrantes, máx. 40 caracteres (UTF-8 seguro).
		static std::string normalizar(const std::string& nombre);

	private:
		bool escribir(const std::vector<PerfilSala>& lista) const;
		std::string _ruta;
	};

}
