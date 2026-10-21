#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
//  OPCIONES DE UN USUARIO DE PES · leer y poner las opciones personales dentro del registro de 2.432 B
// -----------------------------------------------------------------------------
//  Es el «Editar Datos personales» de PES 2021 (mapa decodificado el 12 oct 2026, docs/MANDOS.md («PERFILES-DE-AMIGOS») §11).
//  Trabaja con BYTES, no con textos: los nombres de las opciones cambian de un PES o parche a otro, por eso las
//  etiquetas viven en la interfaz (por parche) y aquí solo hay números. Solo se tocan los bytes conocidos; todo lo demás
//  del registro (los campos sin identificar) se conserva tal cual. Lógica pura: sin Windows.
//  Offsets: +0x04 nombre (48 B) · +0x32 desplazamiento · +0x33 preajuste · +0x34 mapa de botones (2×10 B)
//           +0x48 cambiar cursor · +0x4B cursor siguiente jugador · +0x4C tipo de cursor · +0x4D nivel de pases
//           +0x4E pase en profundidad · +0x4F disparo · +0x50..52 guías (dirección, objetivo, posicionamiento)
//           +0x53 amago automático · +0x54 entrada automática · +0x55 controles de compañero
// =============================================================================

namespace phoenix::perfilespes {

	/// Valores tal como los guarda PES (los textos van en la interfaz). 0 = Sí / 1 = No en las guías y el amago.
	struct OpcionesUsuario {
		int desplazamiento = 0;        // 0 stick izquierdo · 1 cruceta
		int preajuste = 0;             // 0 Normal · 1 Alternativa · 2 Personalizado (sin comprobar)
		std::array<int, 20> botones{}; // 2 bloques de 10: ataque y defensa. × 0 ○ 1 □ 2 △ 3 L1 4 R1 5 L2 6 R2 7 · 8 ninguno
		int cambiarCursor = 1;         // 1 Semiasistido · 2 Sin ayuda · 0 (sin comprobar)
		int cursorSiguiente = 0;       // 0 Sí · 1 No
		int tipoCursor = 4;            // 4 «Datos personales» · 0 Nombre del jugador
		int nivelPases = 1;            // 1 Nivel 1 · 2 Nivel 2
		int paseProfundidad = 0;       // 0 Básico · 1 Avanzado
		int disparo = 0;               // 0 Básico · 1 Avanzado
		int guiaDireccion = 0, guiaObjetivo = 0, guiaPosicion = 0;   // 0 Sí · 1 No
		int amago = 1;                 // 0 Sí · 1 No
		int entrada = 1;               // 1 Semiautomático · 2 No · 0 (sin comprobar)
		int companero = 0;             // 0 Asistido · 1 Manual
	};

	/// Lee las opciones de un registro de 2.432 B. false si el tamaño no es el esperado.
	bool leerOpciones(const std::vector<uint8_t>& registro, OpcionesUsuario& o);
	/// Comprueba rangos (cada valor entre los que PES conoce). `motivo` dice cuál falla.
	bool opcionesValidas(const OpcionesUsuario& o, std::string& motivo);
	/// Escribe SOLO las opciones conocidas en el registro (los demás bytes no se tocan). false si no son válidas.
	bool ponerOpciones(std::vector<uint8_t>& registro, const OpcionesUsuario& o, std::string& motivo);
	/// Pone el nombre (máx. kMaxNombre bytes UTF-8, sin caracteres de control) en el registro.
	bool ponerNombre(std::vector<uint8_t>& registro, const std::string& nombre, std::string& motivo);
}
