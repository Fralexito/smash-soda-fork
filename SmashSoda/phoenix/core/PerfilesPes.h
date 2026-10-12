#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// =============================================================================
//  PERFILES PES · los 32 «Datos personales» (Usuario 1…32) de PES 2021
// -----------------------------------------------------------------------------
//  Dónde viven (prueba ZZTEST del 12 oct 2026, PC de Fralex, ConmeGOL):
//    Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<SteamID>\save\SYSTEM00000000
//  Es un guardado cifrado igual que EDIT/ML (cabecera + descripción + logo + DATOS + serial).
//  Los DATOS miden 219.568 B y empiezan con una cabecera de 0x50 B; a partir de 0x50 hay
//  una TABLA FIJA de 32 registros de 2.432 B (uno por Usuario) y detrás sigue otra información
//  (historial de partidos, etc.) que empieza justo en 0x13050. Por eso el 32 no se puede ampliar
//  sin tocar el ejecutable: el formato tiene la tabla pegada a los demás datos.
//  Registro de un Usuario (visto, no todo entendido):
//    +0x00  u32 (0 ó 1; marca desconocida)
//    +0x04  nombre, 48 B terminados en 0 (el que se ve al elegir el Usuario)
//    +0x34  configuración de comandos (mapa de botones, 20 B «Normal» por defecto), cursor,
//           ayudas de pase, tipo de disparo… y los mapas «Personalizado»; el resto hasta 2.432 B.
//  El guardado NO se escribe al editar el Usuario en PES: PES lo escribe al terminar un partido.
//
//  Reglas de este módulo: solo trabaja con PES CERRADO (lo decide quien llama), nunca toca el
//  original sin copia con sello, escribe a un temporal, lo relee y valida y solo entonces
//  reemplaza (todo o nada). Si el formato no es el conocido, se niega a escribir.
//  La cabecera de 32 B en +0x30 cambia en cada guardado de PES; aquí se conserva tal cual.
//  ❓ Sin probar en el juego: que PES acepte un SYSTEM00000000 escrito por Link (para ML y EDIT
//  el juego no valida la cabecera; para SYSTEM falta la prueba con permiso de Fralex).
// =============================================================================

namespace phoenix::perfilespes {

	constexpr int kUsuarios = 32;
	constexpr size_t kTamUsuario = 2432;
	constexpr size_t kInicioTabla = 0x50;
	constexpr size_t kTamNombre = 48;
	constexpr size_t kTamDatos = 219568;      // única versión conocida (PES 2021 SEASON UPDATE)
	constexpr size_t kMaxNombre = 31;         // bytes UTF-8 que acepta rotular() (límite propio, conservador)

	struct Usuario {
		int indice = 0;                       // 0…31 (Usuario N = indice + 1)
		std::string nombre;                   // vacío = hueco con nombre por defecto («Usuario N»)
		std::vector<uint8_t> bytes;           // el registro completo, 2.432 B
	};

	class Guardado {
	public:
		/// Abre y descifra un SYSTEM00000000. false + error si no es del formato conocido.
		static bool abrir(const std::string& ruta, Guardado& salida, std::string& error);
		static bool desdeBytes(std::vector<uint8_t> cifrado, Guardado& salida, std::string& error);

		const std::vector<uint8_t>& datos() const { return _datos; }
		Usuario usuario(int indice) const;                    ///< 0…31
		std::vector<Usuario> usuarios() const;                ///< los 32
		/// Cambia SOLO en memoria el registro de un hueco. false si el registro no tiene forma válida.
		bool ponerUsuario(int indice, const std::vector<uint8_t>& registro, std::string& error);
		/// Cambia SOLO el nombre (en memoria) de un hueco. Máx. kMaxNombre bytes UTF-8.
		bool rotular(int indice, const std::string& nombre, std::string& error);

		/// Cifra con la cabecera, logo y serial del original (mismo cifrado que el juego).
		bool cifrar(std::vector<uint8_t>& salida, std::string& error) const;
		/// Escribe un archivo NUEVO (nunca sobrescribe), releyéndolo y validándolo.
		bool guardarComo(const std::string& rutaNueva, std::string& error) const;
		/// Reemplaza `ruta` (el SYSTEM00000000 real): copia «.phoenix-<sello>» al lado, escritura atómica y relectura.
		/// `sello` lo pone quien llama (p. ej. «20261012-141500») para poder probarlo.
		bool reemplazar(const std::string& ruta, const std::string& sello, std::string& error, std::string* copiaHecha = nullptr) const;

	private:
		std::vector<uint8_t> _cifrado;        // original (reabierto al cifrar)
		std::vector<uint8_t> _datos;
	};

	/// ¿Es texto UTF-8 válido? (PES deja a veces bytes 0xFF en nombres de huecos sin usar: no sirven como nombre ni para JSON)
	bool utf8Valido(const std::string& texto);

	/// ¿Tiene un registro forma de Usuario? (u32 inicial 0/1, nombre imprimible terminado en 0, tamaño exacto)
	bool registroValido(const std::vector<uint8_t>& registro, std::string& motivo);

	// ---- Perfiles (JSON propio de Phoenix) -------------------------------------
	/// {formato:1, juego:"PES2021", nombre, tam, registro:"<hex>"}
	nlohmann::json aPerfil(const Usuario& u);
	bool dePerfil(const nlohmann::json& perfil, std::vector<uint8_t>& registro, std::string& nombre, std::string& error);

	std::string aHex(const std::vector<uint8_t>& b);
	bool deHex(const std::string& s, std::vector<uint8_t>& b);
}
