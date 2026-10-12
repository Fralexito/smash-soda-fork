#pragma once

#include <nlohmann/json.hpp>

#include "../host/CampoVivo.h"

// =============================================================================
//  Phoenix Link · Campo 2D en vivo para el tablero de la web
// -----------------------------------------------------------------------------
//  Con «Campo en vivo en la web» encendido (Partido › CAMPO, preferencia campoWeb) y un partido en juego que
//  ya sale en «En vivo ahora» (oficial, o amistoso en sala no privada), Link manda a la web, como mucho 1 vez
//  por segundo, las fichitas, el balón y la posesión aproximada (POST /v1/liga/campo → Realtime Broadcast).
//  Al pulsar la tarjeta en la web se abre el tablero con el campo y las estadísticas.
//  Antes iba al overlay como mini-radar: Fralex lo quiere en la web, no sobre el juego.
// =============================================================================

namespace phoenix::web {

	struct Interno;

	/// ¿Hay que leer el juego y mandar el campo a la web ahora mismo?
	bool campoWebQuiere(Interno& in);
	/// Con cada lectura nueva del juego: acumula la posesión y, como mucho 1/s, la manda con las fichitas (`ev`).
	void campoWebFoto(Interno& in, const phoenix::campo::FotoCampo& fc, const nlohmann::json& ev, double ahora);

}
