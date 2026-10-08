#include "Firma.h"

#include <nlohmann/json.hpp>

extern "C" {
#include "../terceros/ed25519/ed25519.h"
}

namespace mercado {

	std::vector<uint8_t> desdeBase64(const std::string& texto) {
		static const std::string abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		std::vector<uint8_t> out;
		uint32_t acum = 0; int bits = 0;
		for (char c : texto) {
			if (c == '=' || c == '\n' || c == '\r' || c == ' ') continue;
			const size_t v = abc.find(c);
			if (v == std::string::npos) return {};
			acum = (acum << 6) | uint32_t(v); bits += 6;
			if (bits >= 8) { bits -= 8; out.push_back(uint8_t((acum >> bits) & 0xff)); }
		}
		return out;
	}

	std::string aBase64(const std::vector<uint8_t>& d) {
		static const char* abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		std::string out;
		size_t i = 0;
		for (; i + 2 < d.size(); i += 3) {
			const uint32_t v = (uint32_t(d[i]) << 16) | (uint32_t(d[i + 1]) << 8) | d[i + 2];
			out += abc[(v >> 18) & 63]; out += abc[(v >> 12) & 63]; out += abc[(v >> 6) & 63]; out += abc[v & 63];
		}
		if (i + 1 == d.size()) { const uint32_t v = uint32_t(d[i]) << 16; out += abc[(v >> 18) & 63]; out += abc[(v >> 12) & 63]; out += "=="; }
		else if (i + 2 == d.size()) { const uint32_t v = (uint32_t(d[i]) << 16) | (uint32_t(d[i + 1]) << 8); out += abc[(v >> 18) & 63]; out += abc[(v >> 12) & 63]; out += abc[(v >> 6) & 63]; out += '='; }
		return out;
	}

	bool verificarFirma(const ClavePublica& clave, const std::string& contenido, const std::string& firmaB64) {
		if (clave.publica.size() != 32) return false;
		const auto firma = desdeBase64(firmaB64);
		if (firma.size() != 64) return false;
		return ed25519_verify(firma.data(), reinterpret_cast<const unsigned char*>(contenido.data()), contenido.size(), clave.publica.data()) == 1;
	}

	Resultado<std::string> abrirSobreFirmado(const std::string& datosJson, const ClavePublica& clave) {
		using R = Resultado<std::string>;
		nlohmann::json j = nlohmann::json::parse(datosJson, nullptr, false);
		if (j.is_discarded() || !j.is_object()) return R::mal("SOBRE_INVALIDO", "La respuesta no es un objeto JSON");
		if (!j.contains("contenido") || !j["contenido"].is_string() || !j.contains("firma") || !j["firma"].is_string())
			return R::mal("SOBRE_INVALIDO", "Faltan «contenido» o «firma»");
		if (j.value("algoritmo", "") != "Ed25519") return R::mal("SOBRE_INVALIDO", "Algoritmo no es Ed25519: " + j.value("algoritmo", ""));
		if (j.value("clave_id", "") != clave.claveId) return R::mal("CLAVE_DESCONOCIDA", "clave_id " + j.value("clave_id", "") + " (se esperaba " + clave.claveId + ")");
		const std::string contenido = j["contenido"].get<std::string>();
		if (!verificarFirma(clave, contenido, j["firma"].get<std::string>())) return R::mal("FIRMA_INVALIDA", "La firma de la web no corresponde al contenido");
		return R::bien(contenido);
	}

	ParClaves parClavesDesdeSemilla(const std::vector<uint8_t>& semilla32) {
		ParClaves p; p.publica.resize(32); p.privada.resize(64);
		std::vector<uint8_t> s = semilla32; s.resize(32, 0);
		ed25519_create_keypair(p.publica.data(), p.privada.data(), s.data());
		return p;
	}

	std::string firmarTexto(const ParClaves& par, const std::string& contenido) {
		std::vector<uint8_t> firma(64);
		ed25519_sign(firma.data(), reinterpret_cast<const unsigned char*>(contenido.data()), contenido.size(), par.publica.data(), par.privada.data());
		return aBase64(firma);
	}

}
