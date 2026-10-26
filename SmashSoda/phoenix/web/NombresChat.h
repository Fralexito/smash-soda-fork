#pragma once
// Nombre propio para cada mando (0.3.52): el comando de chat !nombre <texto> de Parsec cambia el nombre del perfil de PES
// de quien lo escribe; «Usuario automático» se lo pone al sentarse (pantalla de elegir lados). Tras el interruptor
// «Nombres por chat y por web» de GENTE/JUGAR › Perfil PES (apagado de serie). Lógica en web/AccionesPerfilesWeb.cpp.
#include <cstdint>
#include <string>

namespace phoenix::web {
	/// Lo que pide `!nombre <texto>` un invitado (su ID de Parsec). Devuelve la respuesta para el chat (siempre corta).
	std::string nombrePorChat(uint32_t parsecId, const std::string& texto);
}
