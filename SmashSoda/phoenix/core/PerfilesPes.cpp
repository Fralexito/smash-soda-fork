#include "PerfilesPes.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

extern "C" {
#include "../terceros/pesxcrypter/crypt.h"
#include "../terceros/pesxcrypter/masterkey.h"
	int pesxCabeceraNuevaValida(const uint8_t* input, size_t fileSize, const char* masterKey);   // pesxvalidar.c
}

namespace fs = std::filesystem;

namespace phoenix::perfilespes {

	namespace {
		constexpr size_t kBytesNombreDesc = 128;   // primeros bytes de la descripción del sobre
		constexpr size_t kMinArchivo = 0x1000;
		constexpr size_t kMaxArchivo = size_t(64) << 20;

		struct Descriptor {
			FileDescriptorNew* d = createFileDescriptorNew();
			Descriptor() = default;
			Descriptor(const Descriptor&) = delete;
			Descriptor& operator=(const Descriptor&) = delete;
			~Descriptor() { if (d) destroyFileDescriptorNew(d); }
		};

		const char* clave() { return reinterpret_cast<const char*>(MasterKeyPes21); }

		std::string textoHasta0(const uint8_t* p, size_t max) {
			size_t n = 0;
			while (n < max && p[n]) n++;
			return std::string(reinterpret_cast<const char*>(p), n);
		}

		fs::path aRuta(const std::string& utf8) { return fs::u8path(utf8); }
		std::string aUtf8(const fs::path& p) { const auto s = p.u8string(); return std::string(s.begin(), s.end()); }   // u8string es char8_t en C++20

		bool leerTodo(const std::string& ruta, std::vector<uint8_t>& bytes, std::string& error) {
			std::ifstream f(aRuta(ruta), std::ios::binary);
			if (!f) { error = "no se puede abrir " + ruta; return false; }
			bytes.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
			return true;
		}

		bool escribirTodo(const fs::path& destino, const std::vector<uint8_t>& bytes) {
			std::ofstream f(destino, std::ios::binary | std::ios::trunc);
			if (!f) return false;
			f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
			f.flush();
			return f.good();
		}
	}

	bool utf8Valido(const std::string& s) {
		size_t i = 0;
		while (i < s.size()) {
			const unsigned char c = static_cast<unsigned char>(s[i]);
			size_t n = 0;
			if (c < 0x80) n = 1;
			else if (c >= 0xC2 && c <= 0xDF) n = 2;
			else if (c >= 0xE0 && c <= 0xEF) n = 3;
			else if (c >= 0xF0 && c <= 0xF4) n = 4;
			else return false;
			if (i + n > s.size()) return false;
			for (size_t k = 1; k < n; k++) if ((static_cast<unsigned char>(s[i + k]) & 0xC0) != 0x80) return false;
			i += n;
		}
		return true;
	}

	bool registroValido(const std::vector<uint8_t>& r, std::string& motivo) {
		if (r.size() != kTamUsuario) { motivo = "el registro debe medir " + std::to_string(kTamUsuario) + " bytes"; return false; }
		// +0 es una marca pequeña (0 o 1 en los 32 Usuarios vistos); el nombre termina en 0 y PES puede dejar restos detrás (p. ej. «JOAO\0N»).
		if (r[1] || r[2] || r[3] || r[0] > 1) { motivo = "el registro no empieza como un Usuario"; return false; }
		size_t fin = 0;
		while (fin < kTamNombre && r[4 + fin]) fin++;
		if (fin == kTamNombre) { motivo = "el nombre no termina"; return false; }
		for (size_t i = 0; i < fin; i++) if (r[4 + i] < 0x20 || r[4 + i] == 0x7F) { motivo = "el nombre tiene caracteres de control"; return false; }
		return true;
	}

	bool Guardado::abrir(const std::string& ruta, Guardado& salida, std::string& error) {
		try {
			std::vector<uint8_t> bytes;
			if (!leerTodo(ruta, bytes, error)) return false;
			return desdeBytes(std::move(bytes), salida, error);
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}

	bool Guardado::desdeBytes(std::vector<uint8_t> cifrado, Guardado& salida, std::string& error) {
		try {
			if (cifrado.size() < kMinArchivo || cifrado.size() > kMaxArchivo) { error = "tamaño de archivo no válido"; return false; }
			// decryptWithKeyNew se fía de los tamaños de la cabecera: con un archivo cortado cerraría Link.
			if (!pesxCabeceraNuevaValida(cifrado.data(), cifrado.size(), clave())) { error = "el archivo está incompleto o no es un guardado de PES (cabecera incoherente)"; return false; }
			Guardado g;
			g._cifrado = std::move(cifrado);
			Descriptor desc;
			if (!desc.d) { error = "sin memoria"; return false; }
			decryptWithKeyNew(desc.d, g._cifrado.data(), clave());
			const FileHeaderNew* h = desc.d->fileHeader;
			if (!h || !desc.d->data || !desc.d->description) { error = "no se pudo descifrar"; return false; }
			if (textoHasta0(h->fileTypeString, sizeof(h->fileTypeString)) != "SYSTEM") { error = "no es un SYSTEM00000000 (tipo " + textoHasta0(h->fileTypeString, sizeof(h->fileTypeString)) + ")"; return false; }
			if (textoHasta0(h->gameVersionString, sizeof(h->gameVersionString)).find("2021") == std::string::npos) { error = "no es de PES 2021"; return false; }
			if (h->dataSize != kTamDatos) { error = "tamaño de datos desconocido (" + std::to_string(h->dataSize) + "); solo se conoce " + std::to_string(kTamDatos); return false; }
			g._datos.assign(desc.d->data, desc.d->data + h->dataSize);
			for (int i = 0; i < kUsuarios; i++) {
				std::vector<uint8_t> r(g._datos.begin() + kInicioTabla + size_t(i) * kTamUsuario, g._datos.begin() + kInicioTabla + size_t(i + 1) * kTamUsuario);
				std::string motivo;
				if (!registroValido(r, motivo)) { error = "Usuario " + std::to_string(i + 1) + " con forma desconocida: " + motivo; return false; }
			}
			salida = std::move(g);
			return true;
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}

	Usuario Guardado::usuario(int indice) const {
		Usuario u;
		u.indice = indice;
		if (indice < 0 || indice >= kUsuarios || _datos.size() != kTamDatos) return u;
		const size_t ini = kInicioTabla + size_t(indice) * kTamUsuario;
		u.bytes.assign(_datos.begin() + ini, _datos.begin() + ini + kTamUsuario);
		u.nombre = textoHasta0(u.bytes.data() + 4, kTamNombre);
		if (!utf8Valido(u.nombre)) u.nombre.clear();   // p. ej. 0xFF en huecos sin usar: se trata como «sin nombre»
		return u;
	}

	std::vector<Usuario> Guardado::usuarios() const {
		std::vector<Usuario> r;
		for (int i = 0; i < kUsuarios; i++) r.push_back(usuario(i));
		return r;
	}

	bool Guardado::ponerUsuario(int indice, const std::vector<uint8_t>& registro, std::string& error) {
		if (indice < 0 || indice >= kUsuarios) { error = "el hueco debe estar entre 1 y 32"; return false; }
		if (_datos.size() != kTamDatos) { error = "guardado no abierto"; return false; }
		if (!registroValido(registro, error)) return false;
		std::memcpy(_datos.data() + kInicioTabla + size_t(indice) * kTamUsuario, registro.data(), kTamUsuario);
		return true;
	}

	bool Guardado::rotular(int indice, const std::string& nombre, std::string& error) {
		if (indice < 0 || indice >= kUsuarios) { error = "el hueco debe estar entre 1 y 32"; return false; }
		if (nombre.size() > kMaxNombre) { error = "el nombre no puede pasar de " + std::to_string(kMaxNombre) + " bytes"; return false; }
		Usuario u = usuario(indice);
		if (u.bytes.size() != kTamUsuario) { error = "guardado no abierto"; return false; }
		std::memset(u.bytes.data() + 4, 0, kTamNombre - 2);   // los 2 últimos bytes del campo son opciones (+0x32 desplazamiento, +0x33 preajuste): no se borran
		std::memcpy(u.bytes.data() + 4, nombre.data(), nombre.size());
		return ponerUsuario(indice, u.bytes, error);
	}

	bool Guardado::cifrar(std::vector<uint8_t>& salida, std::string& error) const {
		try {
			if (_cifrado.empty() || _datos.size() != kTamDatos) { error = "guardado no abierto"; return false; }
			Descriptor desc;
			if (!desc.d) { error = "sin memoria"; return false; }
			decryptWithKeyNew(desc.d, _cifrado.data(), clave());
			FileHeaderNew* h = desc.d->fileHeader;
			if (!h || !desc.d->data || !desc.d->description) { error = "no se pudo reabrir el original"; return false; }
			uint8_t* nuevos = static_cast<uint8_t*>(std::malloc(_datos.size()));
			if (!nuevos) { error = "sin memoria"; return false; }
			std::memcpy(nuevos, _datos.data(), _datos.size());
			std::free(desc.d->data);
			desc.d->data = nuevos;
			h->dataSize = static_cast<uint32_t>(_datos.size());
			int tam = 0;
			uint8_t* out = encryptWithKeyNew(desc.d, &tam, clave());
			if (!out || tam <= 0) { error = "no se pudo cifrar"; return false; }
			salida.assign(out, out + tam);
			std::free(out);
			return true;
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}

	bool Guardado::guardarComo(const std::string& rutaNueva, std::string& error) const {
		try {
			const fs::path destino = aRuta(rutaNueva);
			if (fs::exists(destino)) { error = "no se sobrescribe: " + rutaNueva; return false; }
			std::vector<uint8_t> cif;
			if (!cifrar(cif, error)) return false;
			fs::path tmp = destino; tmp += ".phoenix-tmp";
			std::error_code ec;
			if (!escribirTodo(tmp, cif)) { fs::remove(tmp, ec); error = "no se pudo escribir " + rutaNueva; return false; }
			Guardado rel;
			std::string e2;
			if (!Guardado::abrir(aUtf8(tmp), rel, e2) || rel._datos != _datos) { fs::remove(tmp, ec); error = "la relectura no coincide; se descartó el archivo" + (e2.empty() ? std::string() : " (" + e2 + ")"); return false; }
			if (fs::exists(destino)) { fs::remove(tmp, ec); error = "apareció durante la escritura: " + rutaNueva; return false; }
			fs::rename(tmp, destino, ec);
			if (ec) { fs::remove(tmp, ec); error = "no se pudo renombrar el temporal"; return false; }
			return true;
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}

	bool Guardado::reemplazar(const std::string& ruta, const std::string& sello, std::string& error, std::string* copiaHecha) const {
		try {
			const fs::path destino = aRuta(ruta);
			std::error_code ec;
			if (!fs::exists(destino)) { error = "no existe " + ruta; return false; }
			if (sello.empty()) { error = "falta el sello de la copia"; return false; }
			// 1) el original tiene que ser un guardado válido: no se reemplaza algo que no entendemos
			Guardado actual;
			std::string e0;
			if (!Guardado::abrir(ruta, actual, e0)) { error = "el archivo actual no es un SYSTEM00000000 conocido: " + e0; return false; }
			// 2) copia con sello al lado
			fs::path copia = destino; copia += ".phoenix-" + sello;
			if (fs::exists(copia)) { error = "ya existe la copia " + aUtf8(copia); return false; }
			fs::copy_file(destino, copia, ec);
			if (ec) { error = "no se pudo hacer la copia de seguridad"; return false; }
			{
				std::vector<uint8_t> a, b;
				std::string e;
				if (!leerTodo(ruta, a, e) || !leerTodo(aUtf8(copia), b, e) || a != b) { error = "la copia de seguridad no coincide; no se toca el original"; return false; }
			}
			// 3) temporal + relectura + reemplazo
			std::vector<uint8_t> cif;
			if (!cifrar(cif, error)) return false;
			fs::path tmp = destino; tmp += ".phoenix-tmp";
			if (!escribirTodo(tmp, cif)) { fs::remove(tmp, ec); error = "no se pudo escribir el temporal"; return false; }
			Guardado rel;
			std::string e2;
			if (!Guardado::abrir(aUtf8(tmp), rel, e2) || rel._datos != _datos) { fs::remove(tmp, ec); error = "la relectura no coincide; no se toca el original" + (e2.empty() ? std::string() : " (" + e2 + ")"); return false; }
			fs::rename(tmp, destino, ec);   // en Windows reemplaza el destino
			if (ec) { fs::remove(tmp, ec); error = "no se pudo reemplazar el archivo (¿PES abierto?)"; return false; }
			if (copiaHecha) *copiaHecha = aUtf8(copia);
			return true;
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}

	std::string aHex(const std::vector<uint8_t>& b) {
		static const char* d = "0123456789abcdef";
		std::string s;
		s.reserve(b.size() * 2);
		for (uint8_t c : b) { s += d[c >> 4]; s += d[c & 15]; }
		return s;
	}

	bool deHex(const std::string& s, std::vector<uint8_t>& b) {
		if (s.size() % 2) return false;
		auto v = [](char c) -> int { if (c >= '0' && c <= '9') return c - '0'; if (c >= 'a' && c <= 'f') return c - 'a' + 10; if (c >= 'A' && c <= 'F') return c - 'A' + 10; return -1; };
		b.clear();
		b.reserve(s.size() / 2);
		for (size_t i = 0; i < s.size(); i += 2) {
			const int h = v(s[i]), l = v(s[i + 1]);
			if (h < 0 || l < 0) return false;
			b.push_back(static_cast<uint8_t>(h * 16 + l));
		}
		return true;
	}

	nlohmann::json aPerfil(const Usuario& u) {
		return nlohmann::json{ {"formato", 1}, {"juego", "PES2021"}, {"nombre", u.nombre}, {"tam", u.bytes.size()}, {"registro", aHex(u.bytes)} };
	}

	bool dePerfil(const nlohmann::json& p, std::vector<uint8_t>& registro, std::string& nombre, std::string& error) {
		if (!p.is_object() || p.value("formato", 0) != 1 || p.value("juego", std::string()) != "PES2021") { error = "no es un perfil de Phoenix (formato 1, PES2021)"; return false; }
		if (!p.contains("registro") || !p["registro"].is_string() || !deHex(p["registro"].get<std::string>(), registro)) { error = "el perfil no tiene un registro válido"; return false; }
		if (p.value("tam", size_t(0)) != registro.size()) { error = "el tamaño del perfil no coincide"; return false; }
		if (!registroValido(registro, error)) return false;
		nombre = textoHasta0(registro.data() + 4, kTamNombre);
		return true;
	}
}
