#pragma once

#include "parsec-dso.h"
#include "ACommand.h"
#include "../../../phoenix/web/NombresChat.h"

// Phoenix (0.3.52): «!nombre JUAN» pone JUAN como nombre del Usuario de PES de quien lo escribe (cada invitado con SU teclado).
// Solo funciona con el interruptor «Nombres por chat y por web» encendido. No se usa «!name»: ya es el comando que renombra la sala.
class CommandNombre : public ACommand
{
public:
	std::string usage = "Uso: !nombre TU NOMBRE (1 a 15 letras). Se ve en PES al elegir lados.";

	CommandNombre(const char* msg, Guest& sender) : ACommand(msg, sender) {}

	bool run() override {
		std::string texto = getMsg() ? std::string(getMsg()) : std::string();
		const size_t esp = texto.find(' ');
		texto = esp == std::string::npos ? std::string() : texto.substr(esp + 1);
		if (texto.find_first_not_of(' ') == std::string::npos) { setReply(usage); return false; }
		setReply(phoenix::web::nombrePorChat(_sender.userID, texto));
		return true;
	}

	static vector<const char*> prefixes() {
		return vector<const char*> { "!nombre" };
	}

protected:
	static vector<const char*> internalPrefixes() {
		return vector<const char*> { "!nombre " };
	}
};
