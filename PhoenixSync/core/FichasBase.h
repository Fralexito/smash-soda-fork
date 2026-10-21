#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Editar las STATS de un jugador en Player.bin (Phoenix-DB)
// -----------------------------------------------------------------------------
//  Formato (base-conocimiento/DATOS-EN-VIVO-Y-RECARGA.md («16-PLAYER-BIN-FICHA-COMPLETA»), calibrado con las
//  pantallas del juego): cabecera WESYS 16 B + zlib → registros de 312 B; el
//  ID del jugador es el u32 de +8. Cada campo son N bits del registro leído
//  como número little-endian. Stats: 6 bits, valor en pantalla = guardado + 40.
//  Probado en el juego (9 oct, 03:41): Phoenix-DB con 1 byte cambiado → la
//  pantalla de Lamine muestra Velocidad 99. Al reiniciar o con la recarga
//  completa (Editar → Cargar, tecla P, v0.18 en automático) el juego lo usa.
//  Reglas: solo se tocan los bits del campo; se guarda SIEMPRE en un archivo
//  nuevo; antes de devolver los bytes se relee y se compara todo.
//  Liga Máster: una carrera ya empezada NO lee Player.bin (guía 18); ahí las
//  stats se suben con las barras de crecimiento (guías 24–29), otro camino.
// =============================================================================

namespace mercado::base {

	struct CampoFicha {
		const char* nombre;   ///< clave estable (la usan la web y las operaciones del grupo)
		const char* texto;    ///< como se ve en el juego
		int bit, bits;        ///< posición y ancho en el registro de 312 B
		int sumar;            ///< valor en pantalla = guardado + sumar
		int minimo, maximo;   ///< rango permitido (en valores de pantalla)
	};

	/// Todos los campos que se pueden editar (stats, portero, pie malo, forma, lesiones y posiciones).
	const std::vector<CampoFicha>& camposFicha();
	/// nullptr si el nombre no existe.
	const CampoFicha* campoFicha(const std::string& nombre);

	class FichasBase {
	public:
		static Resultado<FichasBase> abrir(const std::string& ruta);
		static Resultado<FichasBase> desdeBytes(const std::vector<uint8_t>& archivo);

		size_t jugadores() const { return _plano.size() / 312; }
		bool existe(uint32_t jugador) const { return indice(jugador) >= 0; }

		/// Valor en pantalla de un campo (por ejemplo velocidad = 92).
		Resultado<int> leer(uint32_t jugador, const std::string& campo) const;
		/// Cambia un campo (valor en pantalla). Solo cambian sus bits. Errores: CAMPO_DESCONOCIDO,
		/// VALOR_FUERA_DE_RANGO, JUGADOR_NO_EXISTE.
		Resultado<bool> poner(uint32_t jugador, const std::string& campo, int valor);

		/// Archivo completo (WESYS + zlib nivel 9), verificado releyéndolo.
		Resultado<std::vector<uint8_t>> bytes() const;
		/// Guarda en un archivo NUEVO (falla si existe).
		Resultado<bool> guardarComo(const std::string& ruta) const;

		/// Registros que cambiaron respecto de otra base (para comprobar que solo se tocó lo pedido).
		std::vector<uint32_t> distintos(const FichasBase& otra) const;

	private:
		long indice(uint32_t jugador) const;
		std::vector<uint8_t> _cabecera;   // 8 primeros bytes de la cabecera WESYS original
		std::vector<uint8_t> _plano;      // registros descomprimidos
	};

}
