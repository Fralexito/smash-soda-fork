#pragma once

namespace phoenix {

	class ProveedorSala;

	// =========================================================================
	//  Tablero de mandos: hasta 8 mandos repartidos en LOCAL y VISITANTE
	//  (1v1 … 4v4, 1v7, 3v5…). Sin scroll: todo cabe en una vista.
	//  - Arrastrar un espectador o jugador a un mando = asignárselo
	//  - Clic en un mando desconectado = conectarlo
	//  - Al pasar el mouse: bloquear / liberar
	//  Se dibuja DENTRO de la ventana actual (lo usan Sala y Mandos).
	// =========================================================================
	class TableroMandos {
	public:
		/// `compacto` = versión para la pantalla Sala (sin barra de formación grande).
		static void render(ProveedorSala& sala, float ancho, bool compacto);
	};

}
