#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// =============================================================================
//  PERFILES DE AMIGOS · base propia de Link con el nombre y la configuración de cada amigo
// -----------------------------------------------------------------------------
//  PES solo guarda 32 «Datos personales». Link guarda TODOS los que quieras (aquí) y pone en los
//  32 huecos de PES solo los de quienes juegan hoy. Dos operaciones, las dos con PES CERRADO:
//    · traerDePes    PES → Link: guarda cada Usuario de PES como perfil (nuevo o actualizado).
//                    Solo LEE el guardado de PES. Es la «copia de seguridad» de todo.
//    · prepararSala  Link → PES: antes trae (nada se pierde) y luego pone los perfiles elegidos en
//                    huecos NO protegidos (de atrás hacia delante), con copia con sello del guardado.
//  Un perfil se identifica por su NOMBRE (el que se ve en PES). Un Usuario sin nombre se guarda como
//  «Usuario N» y se reconoce por tener el mismo registro. Detalle del formato: core/PerfilesPes.h.
//  Archivo: <carpeta>\perfiles-amigos.json (+ copias en <carpeta>\respaldos). Lógica pura: sin Windows.
// =============================================================================

namespace phoenix::perfilespes {

	struct PerfilAmigo {
		std::string id;                 // "p1", "p2"…
		std::string nombre;             // el de PES (o «Usuario N» si no tenía)
		std::string registro;           // hex del registro de 2.432 B
		std::string anterior;           // registro anterior (por si el cambio no gustó)
		std::string actualizado;        // sello «20261012-141500»
		bool sinNombre = false;
		uint32_t parsecId = 0;          // persona a la que pertenece (0 = ninguna): al sentarse se le pone este perfil solo
	};

	struct Asignacion {
		int hueco = 0;                  // 1…32
		std::string perfilId;
	};

	struct DatosBase {
		std::vector<PerfilAmigo> perfiles;
		std::vector<int> protegidos;            // huecos 1…32 que Link nunca toca
		std::vector<std::string> seleccion;     // ids elegidos «para hoy»
		std::vector<Asignacion> ultima;         // lo que dejó la última «Preparar sala»
		std::string ultimaSala;                 // sello
	};

	struct Informe {
		int nuevos = 0, actualizados = 0, iguales = 0;
		std::vector<Asignacion> puestos;        // prepararSala: quién quedó en qué hueco
		int escritos = 0;                       // huecos que hubo que escribir
		std::string copia;                      // copia del guardado hecha antes de escribir ("" si no se escribió)
		std::vector<std::string> avisos;
	};

	class Base {
	public:
		static constexpr size_t kMaxPerfiles = 500;
		static constexpr size_t kMaxRespaldos = 20;

		explicit Base(std::string carpeta) : _carpeta(std::move(carpeta)) {}
		/// Lee perfiles-amigos.json (si no existe, base vacía). false + error si está dañado (no se pisa).
		bool cargar(std::string& error);
		/// Guarda (temporal + relectura + reemplazo) y deja copia en respaldos\. `sello` lo pone quien llama.
		bool guardar(const std::string& sello, std::string& error);

		DatosBase datos;

		const PerfilAmigo* porId(const std::string& id) const;
		const PerfilAmigo* porNombre(const std::string& nombre) const;
		const PerfilAmigo* porParsec(uint32_t parsecId) const;      ///< el perfil de esa persona (nullptr si no tiene)
		/// Liga un perfil a una persona (parsecId 0 = quitar). Una persona tiene UN perfil: si ya tenía otro, se le quita.
		bool vincular(const std::string& id, uint32_t parsecId);
		bool protegido(int hueco) const;
		void proteger(int hueco, bool si);
		bool borrar(const std::string& id);

		nlohmann::json aJson() const;
		bool deJson(const nlohmann::json& j, std::string& error);

	private:
		std::string _carpeta;
	};

	/// PES → Link. Solo lee el guardado de PES.
	bool traerDePes(const std::string& rutaSystem, Base& base, const std::string& sello, Informe& informe, std::string& error);
	/// Link → PES. Trae primero (nada se pierde), luego escribe los perfiles `ids` (máx. huecos libres) con copia con sello.
	bool prepararSala(const std::string& rutaSystem, Base& base, const std::vector<std::string>& ids, const std::string& sello, Informe& informe, std::string& error);

	/// Nombre normalizado para comparar (recorta espacios y pasa ASCII a mayúsculas).
	std::string claveNombre(const std::string& nombre);
}
