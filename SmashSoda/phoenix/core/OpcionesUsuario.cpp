#include "OpcionesUsuario.h"

#include <algorithm>

#include "PerfilesPes.h"

namespace phoenix::perfilespes {

	namespace {
		constexpr size_t kDesplazamiento = 0x32, kPreajuste = 0x33, kBotones = 0x34;
		constexpr size_t kCambiarCursor = 0x48, kCursorSiguiente = 0x4B, kTipoCursor = 0x4C, kNivelPases = 0x4D;
		constexpr size_t kPaseProfundidad = 0x4E, kDisparo = 0x4F, kGuiaDireccion = 0x50, kGuiaObjetivo = 0x51, kGuiaPosicion = 0x52;
		constexpr size_t kAmago = 0x53, kEntrada = 0x54, kCompanero = 0x55;

		bool enRango(int v, int a, int b) { return v >= a && v <= b; }
	}

	bool leerOpciones(const std::vector<uint8_t>& r, OpcionesUsuario& o) {
		if (r.size() != kTamUsuario) return false;
		o.desplazamiento = r[kDesplazamiento]; o.preajuste = r[kPreajuste];
		for (size_t i = 0; i < o.botones.size(); i++) o.botones[i] = r[kBotones + i];
		o.cambiarCursor = r[kCambiarCursor]; o.cursorSiguiente = r[kCursorSiguiente]; o.tipoCursor = r[kTipoCursor];
		o.nivelPases = r[kNivelPases]; o.paseProfundidad = r[kPaseProfundidad]; o.disparo = r[kDisparo];
		o.guiaDireccion = r[kGuiaDireccion]; o.guiaObjetivo = r[kGuiaObjetivo]; o.guiaPosicion = r[kGuiaPosicion];
		o.amago = r[kAmago]; o.entrada = r[kEntrada]; o.companero = r[kCompanero];
		return true;
	}

	bool opcionesValidas(const OpcionesUsuario& o, std::string& motivo) {
		struct R { const char* nombre; int v, a, b; };
		const R reglas[] = {
			{ "Desplazamiento de jugadores", o.desplazamiento, 0, 1 }, { "Preajuste de comandos", o.preajuste, 0, 2 },
			{ "Cambiar cursor", o.cambiarCursor, 0, 2 }, { "Cursor de siguiente jugador", o.cursorSiguiente, 0, 1 },
			{ "Nivel de ayuda en pases", o.nivelPases, 1, 2 }, { "Pase en profundidad", o.paseProfundidad, 0, 1 },
			{ "Tipo de disparo", o.disparo, 0, 1 }, { "Guía de dirección", o.guiaDireccion, 0, 1 },
			{ "Guía de objetivo", o.guiaObjetivo, 0, 1 }, { "Guía de posicionamiento", o.guiaPosicion, 0, 1 },
			{ "Amago automático", o.amago, 0, 1 }, { "Entrada automática", o.entrada, 0, 2 }, { "Controles de compañero", o.companero, 0, 1 },
		};
		for (const R& r : reglas) if (!enRango(r.v, r.a, r.b)) { motivo = std::string("Valor no válido en «") + r.nombre + "»."; return false; }
		if (o.tipoCursor != 0 && o.tipoCursor != 4) { motivo = "Valor no válido en «Tipo de cursor»."; return false; }
		for (int b : o.botones) if (!enRango(b, 0, 8)) { motivo = "Un botón de la configuración de comandos no es válido."; return false; }
		return true;
	}

	bool ponerOpciones(std::vector<uint8_t>& r, const OpcionesUsuario& o, std::string& motivo) {
		if (r.size() != kTamUsuario) { motivo = "El registro no tiene el tamaño esperado."; return false; }
		if (!opcionesValidas(o, motivo)) return false;
		r[kDesplazamiento] = static_cast<uint8_t>(o.desplazamiento); r[kPreajuste] = static_cast<uint8_t>(o.preajuste);
		for (size_t i = 0; i < o.botones.size(); i++) r[kBotones + i] = static_cast<uint8_t>(o.botones[i]);
		r[kCambiarCursor] = static_cast<uint8_t>(o.cambiarCursor); r[kCursorSiguiente] = static_cast<uint8_t>(o.cursorSiguiente);
		r[kTipoCursor] = static_cast<uint8_t>(o.tipoCursor); r[kNivelPases] = static_cast<uint8_t>(o.nivelPases);
		r[kPaseProfundidad] = static_cast<uint8_t>(o.paseProfundidad); r[kDisparo] = static_cast<uint8_t>(o.disparo);
		r[kGuiaDireccion] = static_cast<uint8_t>(o.guiaDireccion); r[kGuiaObjetivo] = static_cast<uint8_t>(o.guiaObjetivo);
		r[kGuiaPosicion] = static_cast<uint8_t>(o.guiaPosicion); r[kAmago] = static_cast<uint8_t>(o.amago);
		r[kEntrada] = static_cast<uint8_t>(o.entrada); r[kCompanero] = static_cast<uint8_t>(o.companero);
		return true;
	}

	bool ponerNombre(std::vector<uint8_t>& r, const std::string& nombre, std::string& motivo) {
		if (r.size() != kTamUsuario) { motivo = "El registro no tiene el tamaño esperado."; return false; }
		if (std::all_of(nombre.begin(), nombre.end(), [](unsigned char c) { return c == ' ' || c == '\t'; })) { motivo = "Escribe un nombre."; return false; }
		if (nombre.size() > kMaxNombre) { motivo = "El nombre es demasiado largo (máx. " + std::to_string(kMaxNombre) + " letras)."; return false; }
		for (unsigned char c : nombre) if (c < 0x20 || c == 0x7F) { motivo = "El nombre tiene caracteres no válidos."; return false; }
		if (!utf8Valido(nombre)) { motivo = "El nombre no es texto válido."; return false; }
		// Ojo: los 2 últimos bytes del campo de 48 B (+0x32 desplazamiento, +0x33 preajuste) son opciones: no se borran
		std::fill(r.begin() + 4, r.begin() + 4 + kTamNombre - 2, uint8_t(0));
		std::copy(nombre.begin(), nombre.end(), r.begin() + 4);
		return true;
	}
}
