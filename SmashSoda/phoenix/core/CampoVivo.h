#pragma once

// =============================================================================
//  Phoenix Link · Campo 2D en vivo — lógica pura (sin Windows; se prueba aparte en
//  pruebas-link/campo_test.cpp).
// -----------------------------------------------------------------------------
//  Las posiciones de los jugadores de PES 2021 se leen del proceso del juego (SOLO lectura) y se
//  validan aquí. Todo lo que sigue sale de ingeniería inversa propia (ver docs/CAMPO-2D-INVESTIGACION.md)
//  y vale para UN ejecutable concreto: si algo no cuadra, el lector se apaga solo y el juego sigue normal.
//
//  «Actor» = objeto de 640 bytes del juego que representa a un jugador (o asistente) sobre el campo:
//    +208  float4 (x, y, z, 1.0)   posición en metros, centro del campo = (0,0); y ≈ 0.9 (cadera)
//    +248  u32 = 0x18              tipo «cuerpo en el campo»
//    +312  u32 = 1 o 2
//    +440  u32 = (id de equipo de IA << 16) | 3
//    +492  u32 = 0x80000 | (valor de +312)
//    +624  u32 = 0x10  y  +636 u32 = 0xDEAD   marca de actor «normal» (los obsoletos no la llevan)
//  Cada «equipo de IA» tiene SU lista con las posiciones de todos: para dibujar basta una lista.
// =============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace phoenix::campo {

	constexpr uint32_t kBytesObjeto = 640;
	constexpr uint32_t kOffPosicion = 208;
	constexpr uint32_t kOffTipo = 248;
	constexpr uint32_t kOffLado = 312;
	constexpr uint32_t kOffCodigo = 440;
	constexpr uint32_t kOffMarca = 492;
	constexpr uint32_t kOffReg0 = 624;
	constexpr uint32_t kOffReg3 = 636;

	/// Un actor encontrado en el juego.
	struct Actor {
		uint64_t dir = 0;       ///< dirección del objeto en el proceso del juego
		uint32_t codigo = 0;    ///< id de equipo de IA (lista a la que pertenece)
		float x = 0, y = 0, z = 0;
	};

	/// ¿Es una posición creíble de alguien sobre (o junto a) el campo? Finita, |x| ≤ 57, |z| ≤ 40, 0 < y < 3.
	bool posicionValida(float x, float y, float z);

	/// Firma completa de un actor (kBytesObjeto bytes legibles). Devuelve false si no es un actor.
	bool leerActor(const uint8_t* obj, Actor& out);

	/// Relectura rápida de la posición de un actor ya conocido (x,y,z,w=1 plausibles). false = el objeto ya no es válido.
	bool leerPosicion(const uint8_t* obj16, float& x, float& y, float& z);

	/// De todos los actores hallados elige la lista de UN equipo de IA: el código con más miembros
	/// (entre 18 y 24). Vacío si no hay una lista creíble.
	std::vector<Actor> elegirLista(const std::vector<Actor>& todos);

	/// ¿Es el asistente de línea? (posición pegada a la banda, |z| entre 34.8 y 36.5). Solo sirve en una foto: allí van los dos.
	bool pareceAsistente(const Actor& a);

	/// Ejecutables de PES 2021 con los que se comprobó la firma (tamaño + SHA-256 en mayúsculas).
	bool exeConocido(uint64_t tamano, const std::string& sha256);

}
