#include "InterfazWebInterno.h"

#include "../link/ServicioTraspasos.h"

// =============================================================================
//  Acciones de la interfaz web: «Traspasos en vivo» (SYNC › Traspasos).
//  El anfitrion hace un traspaso en Editar de PES; al guardar, Link lo detecta y lo publica como un lote; el Link del amigo lo
//  aplica sobre SU base y avisa a phoenix.lua. Todo apagado por defecto. Detalle: docs/TRASPASOS-EN-VIVO.md.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	void registrarAccionesTraspasos(Interno& in) {
		Puente& p = *in.puente;
		phoenix::traspasos::Servicio::get().iniciar();   // si dejaste algo encendido, sigue encendido al abrir Link

		// Solo lee (el hilo del servicio deja el estado listo: la interfaz nunca espera).
		p.registrar("traspasos.estado", [](const json&, uint64_t) -> std::optional<json> {
			return phoenix::traspasos::Servicio::get().estado();
		});

		// Cambia ajustes: publicar, recibir, confirmarAntes (booleanos); carpeta, autor, optionFile (texto); transporte ("carpeta").
		p.registrar("traspasos.config", [](const json& d, uint64_t) -> std::optional<json> {
			try { phoenix::traspasos::Servicio::get().poner(d); }
			catch (const std::exception& e) { throw ErrorAccion("DATOS_INVALIDOS", e.what()); }
			return phoenix::traspasos::Servicio::get().estado();
		});

		// publicar | descartar (lo detectado en espera de confirmar) | deshacer (el ultimo lote aplicado) | olvidarReferencia | reintentar (volver a comprobar la web)
		// con la web: rotarCodigo | pausarCanal | reanudarCanal | aprobar{usuario_id} | quitarSeguidor{usuario_id} | seguirAmigo{usuario_id} | seguirCodigo{codigo} | dejarDeSeguir{canal_id}
		p.registrar("traspasos.accion", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string que = texto(d, "accion", 20);
			std::string dato;
			for (const char* k : { "usuario_id", "codigo", "canal_id" }) if (d.contains(k) && d[k].is_string()) dato = texto(d, k, 64);
			try { phoenix::traspasos::Servicio::get().accion(que, dato); }
			catch (const std::exception& e) { throw ErrorAccion("NO_SE_PUDO", e.what()); }
			return phoenix::traspasos::Servicio::get().estado();
		});
	}

}
