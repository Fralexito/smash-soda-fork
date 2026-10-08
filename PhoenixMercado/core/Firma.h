#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Mercado · Firma Ed25519 de lo que manda la web (docs/mercado-api.md)
// -----------------------------------------------------------------------------
//  La web firma el texto EXACTO de «contenido» (UTF-8) con su clave privada
//  (que nunca sale del servidor). Mercado lleva la clave pública incrustada
//  (GET /clave-publica → { clave_id, publica }) y solo acepta un contenido si
//  la firma es válida y el clave_id es el esperado. Recién entonces lo parsea.
//  Implementación: terceros/ed25519 (orlp, zlib).
// =============================================================================

namespace mercado {

	/// Decodifica base64 estándar (con o sin relleno «=»). Devuelve vacío si hay un carácter inválido.
	std::vector<uint8_t> desdeBase64(const std::string& texto);
	std::string aBase64(const std::vector<uint8_t>& datos);

	/// Clave pública de la web (32 bytes) con su identificador.
	struct ClavePublica {
		std::string claveId;
		std::vector<uint8_t> publica;   // 32 bytes
	};

	/// Verifica Ed25519(publica, contenido, firma). `firmaB64` = 64 bytes en base64.
	bool verificarFirma(const ClavePublica& clave, const std::string& contenido, const std::string& firmaB64);

	/// Abre un sobre firmado tal como lo manda la web: `datosJson` es el JSON de «datos»
	/// `{ contenido, firma, clave_id, algoritmo }`. Devuelve `contenido` SOLO si el algoritmo es Ed25519,
	/// el clave_id coincide y la firma es válida. Si no: FIRMA_INVALIDA / CLAVE_DESCONOCIDA / SOBRE_INVALIDO.
	Resultado<std::string> abrirSobreFirmado(const std::string& datosJson, const ClavePublica& clave);

	/// Solo para pruebas: par de claves determinista a partir de una semilla de 32 bytes y firma de un texto.
	struct ParClaves { std::vector<uint8_t> publica, privada; };
	ParClaves parClavesDesdeSemilla(const std::vector<uint8_t>& semilla32);
	std::string firmarTexto(const ParClaves& par, const std::string& contenido);   // base64 de la firma

}
