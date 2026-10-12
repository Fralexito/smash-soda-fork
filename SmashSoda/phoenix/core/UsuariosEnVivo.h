#pragma once

#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
//  USUARIOS EN VIVO · leer y escribir los «Datos personales» (Usuario 1…32) en la memoria de PES 2021 con el juego abierto
// -----------------------------------------------------------------------------
//  Hallado y comprobado el 12 oct 2026 en la PC de Fralex (PES 2021 ConmeGOL; ver docs/PERFILES-DE-AMIGOS.md §9):
//   · TABLA de 32 registros de 2.432 B (la misma forma que en SYSTEM00000000, core/PerfilesPes.h): vive en el heap, al principio
//     de una región de memoria RW (+0x10). Es la ÚNICA copia de la configuración: lo que se escribe ahí PES lo usa al instante
//     (probado con el nombre; las opciones: pendiente de probar).
//   · LISTA DE MANDOS: 8 u32 = índice de Usuario (0…31) que tiene cada mando. Un puntero a esa lista está en una dirección FIJA del exe:
//     PES2021.exe + 0x36F5820 (puntero) y + 0x36F5828 (u32 = 8). PES no deja que dos mandos tengan el mismo Usuario.
//  Reglas de seguridad: solo escribe un registro que pase registroValido(); antes de escribir guarda el original; después relee y
//  compara; si algo no cuadra (tabla no reconocida, lista de mandos rara, PES cerrado) NO escribe nada. Solo se escribe el
//  registro de UN Usuario (2.432 B) cada vez. Nunca toca archivos de guardado.
//  ❓ Pendiente de probar: que un registro COMPLETO cambie las opciones en el partido; que la entrada N de la lista = puesto N de Link.
// =============================================================================

namespace phoenix::perfilespes::vivo {

	constexpr int kMandos = 8;
	constexpr uint64_t kOffsetPunteroMandos = 0x36F5820;   // desde la base de PES2021.exe

	class Conexion {
	public:
		Conexion() = default;
		~Conexion();
		Conexion(const Conexion&) = delete;
		Conexion& operator=(const Conexion&) = delete;

		/// Se engancha a PES2021.exe (lectura y escritura) y reconoce la lista de mandos y la tabla. false + error si no.
		bool abrir(std::string& error);
		void cerrar();
		bool abierta() const { return _h != nullptr; }

		/// Índice de Usuario (0…31) de cada mando (8 entradas); -1 = ese mando todavía no tiene Usuario (PES guarda 32).
		bool leerMandos(int (&usuarios)[kMandos], std::string& error);
		/// Registro completo (2.432 B) del Usuario `indice` (0…31).
		bool leerUsuario(int indice, std::vector<uint8_t>& registro, std::string& error);
		/// Escribe el registro de UN Usuario: valida el registro, relee y compara. `anterior` recibe lo que había.
		bool escribirUsuario(int indice, const std::vector<uint8_t>& registro, std::vector<uint8_t>& anterior, std::string& error);

		uint64_t direccionTabla() const { return _tabla; }

	private:
		bool leer(uint64_t dir, void* dst, size_t n);
		bool escribir(uint64_t dir, const void* src, size_t n);
		bool tablaValida(uint64_t dir);
		bool buscarTabla(std::string& error);

		void* _h = nullptr;
		uint64_t _base = 0;
		uint64_t _tabla = 0;
	};
}
