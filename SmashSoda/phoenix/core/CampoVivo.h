#pragma once

// =============================================================================
//  Phoenix Link · Campo 2D en vivo — lógica pura (sin Windows; se prueba aparte en
//  pruebas-link/campo_test.cpp).
// -----------------------------------------------------------------------------
//  Las posiciones de los jugadores de PES 2021 se leen del proceso del juego (SOLO lectura) y se
//  validan aquí. Todo lo que sigue sale de ingeniería inversa propia (ver docs/CAMPO-2D-Y-ARBITRO.md («CAMPO-2D-INVESTIGACION»))
//  y vale para UN ejecutable concreto: si algo no cuadra, el lector se apaga solo y el juego sigue normal.
//
//  «Actor» = objeto de 640 bytes del juego que representa algo sobre el campo:
//    +208  float4 (x, y, z, 1.0)   posición en metros, centro del campo = (0,0)
//    +248  u32 tipo: 0x18 = cuerpo de jugador (campo y porteros), 0x07 = asistente, 0x00 = balón
//    +312  u32 «lado»: jugadores de campo 2 (0 en la tanda de penales), porteros 1 (0 en la tanda), balón 1
//    +440  u32 = (código << 16) | 3   (jugadores de campo: id de equipo de IA; porteros y balón: otro código)
//    +492  u32 marca: jugadores de campo 0x80002, porteros 0x80001/0x10001, balón 0x10001
//    +624  u32 = 0x10  y  +636 u32 = 0xDEAD   actor «vivo» (los obsoletos no la llevan)
//  Cada «equipo de IA» tiene SU lista con 20 jugadores de campo + árbitro + 2 asistentes; los porteros
//  van aparte; el balón del partido es un actor fijo todo el partido (se «aparca» en z = 100 en las repeticiones).
//  El EQUIPO de cada jugador no está en el actor: se deduce en un saque de centro (lado) y se recuerda por dirección.
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

	enum class Clase : uint8_t { Campo, Portero, Balon };

	/// Un actor encontrado en el juego.
	struct Actor {
		uint64_t dir = 0;       ///< dirección del objeto en el proceso del juego
		uint32_t codigo = 0;    ///< código (+440 >> 16): id de equipo de IA en los jugadores de campo
		Clase clase = Clase::Campo;
		float x = 0, y = 0, z = 0;
	};

	/// Equipo de una fichita: 0 = equipo A (el que ocupaba la izquierda en el primer saque visto),
	/// 1 = equipo B, 2 = árbitro/asistente, -1 = aún desconocido.
	constexpr int kEqA = 0, kEqB = 1, kEqArbitro = 2, kEqDesconocido = -1;

	/// ¿Posición creíble de alguien sobre (o junto a) el campo? Finita, |x| ≤ 57, |z| ≤ 50, 0 < y < 3.
	bool posicionValida(float x, float y, float z);
	/// ¿Posición creíble del balón? Finita, |x| ≤ 60, |z| ≤ 40 (o aparcado en |z| ≈ 100), 0 ≤ y < 25.
	bool posicionBalonValida(float x, float y, float z);

	/// Firma de un jugador de campo (kBytesObjeto bytes legibles). false si no lo es.
	bool leerActor(const uint8_t* obj, Actor& out);
	/// Firma de un portero. false si no lo es.
	bool leerPortero(const uint8_t* obj, Actor& out);
	/// Firma de un balón (del partido o de un recogepelotas: el del partido es el que se mueve). false si no lo es.
	bool leerBalon(const uint8_t* obj, Actor& out);

	/// Relectura rápida de una posición (x,y,z,w=1 plausibles). false = el objeto ya no es válido.
	bool leerPosicion(const uint8_t* obj16, float& x, float& y, float& z);
	/// Igual para el balón (admite el balón aparcado y en el aire).
	bool leerPosicionBalon(const uint8_t* obj16, float& x, float& y, float& z);

	/// De todos los jugadores de campo hallados elige la lista de UN equipo de IA: el código con más
	/// miembros (entre 18 y 24). Vacío si no hay una lista creíble.
	std::vector<Actor> elegirLista(const std::vector<Actor>& todos);
	/// De los porteros hallados (hay copias) deja como mucho uno por lado del campo.
	std::vector<Actor> elegirPorteros(const std::vector<Actor>& todos);

	/// ¿Asistente de línea? (pegado a la banda, |z| entre 34.8 y 36.5).
	bool pareceAsistente(const Actor& a);

	/// ¿Es un cuadro de saque de centro? Todos quietos (desplazamiento máximo entre lecturas < umbral),
	/// ≥ 7 por mitad, ≤ 3 en el centro y las dos mitades ocupadas de verdad. `maxDesplazamiento` lo
	/// calcula quien llama comparando con la lectura anterior.
	bool esSaqueDeCentro(const std::vector<Actor>& lista, float maxDesplazamiento);

	/// En un saque de centro reparte equipos: izquierda → kEqA, derecha → kEqB, asistentes → kEqArbitro,
	/// el árbitro (en el saque siempre se coloca en (±4.8, ±10.0)) → kEqArbitro, y los que están en el
	/// círculo central → el equipo que tiene menos jugadores (el que saca). Devuelve un equipo por índice.
	std::vector<int> repartirEquipos(const std::vector<Actor>& lista);

	/// Ejecutables de PES 2021 con los que se comprobó la firma (tamaño + SHA-256 en mayúsculas).
	bool exeConocido(uint64_t tamano, const std::string& sha256);

}

// =============================================================================
//  Tanda de penales (lógica pura). Se alimenta con cada lectura (~20/s): balón, actores y sus equipos.
//  Lo que se ve en memoria durante una tanda (Barça–City, 11 oct 2026): el balón se queda clavado en el punto
//  penal (±41.5, 0); los que esperan se colocan en el círculo central o fuera de la banda; el pateador camina
//  hasta el balón (es un actor normal, su identidad se mantiene); el portero que ataja es el que está en la
//  línea (|x| > 50, |z| < 4); tras el golpeo el balón cruza |x| >= 52.5 dentro de los palos (|z| < 3.66,
//  y < 2.44) si es gol, y vuelve al punto o se aparca (z = 100) para el siguiente.
// =============================================================================
namespace phoenix::campo {

	struct Lanzamiento { int eq = kEqDesconocido; bool gol = false; };

	struct EstadoTanda {
		bool activa = false;                 ///< hay tanda en marcha (o acabó hace poco)
		std::vector<Lanzamiento> lanzamientos;
		int turnoEq = kEqDesconocido;        ///< equipo del que está a punto de patear (balón en el punto)
		bool listo = false;                  ///< balón en el punto y pateador al lado
		bool enVuelo = false;                ///< balón golpeado, aún sin resultado
	};

	class Tanda {
	public:
		/// Una lectura. t en segundos (cualquier origen, creciente). eq paralelo a lista.
		void alimentar(bool hayBola, float bx, float by, float bz, const std::vector<Actor>& lista, const std::vector<int>& eq, double t);
		const EstadoTanda& estado() const { return e_; }
		void reiniciar() { e_ = {}; fase_ = Fase::Nada; tListo_ = 0; }

	private:
		enum class Fase { Nada, Listo, EnVuelo };
		Fase fase_ = Fase::Nada;
		EstadoTanda e_;
		float ladoX_ = 0;          ///< signo del punto penal en uso (-1 izquierda, +1 derecha)
		double tListo_ = 0, tVuelo_ = 0, tUltimo_ = 0;
		int pateadorEq_ = kEqDesconocido;
		bool cruzo_ = false, golCruce_ = false;
		void terminar(bool gol, double t);
	};

	/// Un jugador para decidir quién lleva el balón (árbitros y asistentes no son elegibles).
	struct PuntoCampo { float x = 0, z = 0; bool elegible = true; };

	/// Quién tiene el balón, con histéresis (sin parpadeos): distancia al balón + altura del balón + velocidad del balón.
	///  · Lo adquiere el más cercano a ≤ 1,4 m, con el balón a menos de 2,4 m de altura y sin ir lanzado, y que se mantenga ≥ 80 ms.
	///  · Lo conserva mientras siga a ≤ 2,8 m; si se aleja, o el balón sube o sale disparado (pase, tiro), lo «sostiene» 350 ms y,
	///    si no vuelve el contacto, el balón queda LIBRE (-1).
	///  · Otro jugador se lo quita solo si está a ≤ 1,0 m y el poseedor ya está a más de 1,8 m (disputa).
	///  · Sin balón visible (repetición, aparcado) conserva 350 ms y luego libre.
	class Poseedor {
	public:
		/// Una lectura. t en segundos (creciente). Devuelve el índice en `j` del poseedor, o -1 (balón libre).
		int actualizar(const std::vector<PuntoCampo>& j, bool hayBola, float bx, float by, float bz, double t);
		int actual() const { return poseedor_; }
		void reiniciar() { *this = Poseedor(); }

	private:
		int poseedor_ = -1;
		int candidato_ = -1;
		double tCandidato_ = 0, tUltimo_ = 0, sinContacto_ = 0;
		bool previo_ = false;
		float px_ = 0, pz_ = 0;
	};

	/// Estadísticas del partido DERIVADAS de las posiciones y del poseedor del balón (aproximadas: el juego no las da).
	/// Índice 0 = equipo A, 1 = equipo B.
	struct EstadisticasPartido {
		double tPosesion[2] = { 0, 0 };   ///< segundos con el balón
		int pases[2] = { 0, 0 };          ///< pases completados (el balón pasa a otro jugador del mismo equipo en <= 3 s)
		int perdidas[2] = { 0, 0 };       ///< pérdidas (el balón pasa al otro equipo en <= 3 s)
		int recuperaciones[2] = { 0, 0 }; ///< recuperaciones (las pérdidas del otro equipo)
		int tiros[2] = { 0, 0 };          ///< tiros: balón lanzado > 14 m/s hacia la portería rival, a <= 32 m, que apunta al marco ampliado (|z| < 7)
		int tirosPuerta[2] = { 0, 0 };    ///< tiros cuya trayectoria entra por el marco (|z| < 4,2 al llegar a la línea)
		int corners[2] = { 0, 0 };        ///< córners a favor
		int saquesBanda[2] = { 0, 0 };    ///< saques de banda a favor
		int saquesMeta[2] = { 0, 0 };     ///< saques de puerta a favor
	};

	class Estadisticas {
	public:
		/// Una lectura en juego en vivo (no en repeticiones). t en segundos (creciente). eq por ficha (0/1/2/-1), poseedor = índice en j o -1.
		/// ladoDefensaA: -1 = A defiende la portería de x < 0 (ataca hacia x > 0), +1 = al revés, 0 = aún no se sabe.
		void alimentar(double t, bool hayBola, float bx, float by, float bz, int poseedor, const std::vector<int>& eq, const std::vector<PuntoCampo>& j, int ladoDefensaA);
		const EstadisticasPartido& estado() const { return e_; }
		void reiniciar() { *this = Estadisticas(); }

	private:
		EstadisticasPartido e_;
		double tUltimo_ = 0, tSoltado_ = -10, tTiro_ = -10, tDentro_ = 0;
		int ultimoEq_ = -1;        ///< equipo del último que llevó el balón
		int eqAnterior_ = -1;      ///< equipo del poseedor anterior (para pases/pérdidas)
		int piezaAnterior_ = -1;   ///< índice (en j) del poseedor anterior
		bool fuera_ = false;
		bool previo_ = false; float px_ = 0, pz_ = 0;
	};

}
