#include "SobrePes.h"
#include "Sha256.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

extern "C" {
#include "../terceros/pesxcrypter/crypt.h"
	extern const uint8_t MasterKeyPes21[];
}

namespace mercado {

	namespace {
		constexpr size_t kBytesNombre = 128;           // primeros bytes de la descripción: nombre (no visible en el menú Cargar)
		constexpr size_t kMinArchivo = 0x1000;
		constexpr size_t kMaxArchivo = size_t(512) << 20;

		/// Guarda/descarta el descriptor de libpesXcrypter (C) de forma segura.
		struct Descriptor {
			FileDescriptorNew* d = createFileDescriptorNew();
			Descriptor() = default;
			Descriptor(const Descriptor&) = delete;
			Descriptor& operator=(const Descriptor&) = delete;
			~Descriptor() { if (d) destroyFileDescriptorNew(d); }
		};

		std::string textoHasta0(const uint8_t* p, size_t max) {
			size_t n = 0;
			while (n < max && p[n]) n++;
			return std::string(reinterpret_cast<const char*>(p), n);
		}

		const char* clave() { return reinterpret_cast<const char*>(MasterKeyPes21); }
	}

	Resultado<SobrePes> SobrePes::abrir(const std::string& ruta, const std::string& tipoEsperado) {
		using R = Resultado<SobrePes>;
		try {
			std::ifstream f(aRuta(ruta), std::ios::binary);
			if (!f) return R::mal("ARCHIVO_NO_EXISTE", ruta);
			std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
			return desdeBytes(std::move(bytes), tipoEsperado);
		}
		catch (const std::exception& e) { return R::mal("SOBRE_ERROR", e.what()); }
	}

	Resultado<SobrePes> SobrePes::desdeBytes(std::vector<uint8_t> cifrado, const std::string& tipoEsperado) {
		using R = Resultado<SobrePes>;
		try {
			if (cifrado.size() < kMinArchivo || cifrado.size() > kMaxArchivo)
				return R::mal("SOBRE_INVALIDO", "Tamaño de archivo no válido");
			SobrePes s;
			s._cifrado = std::move(cifrado);

			Descriptor desc;
			if (!desc.d) return R::mal("SIN_MEMORIA", "createFileDescriptorNew");
			decryptWithKeyNew(desc.d, s._cifrado.data(), clave());
			const FileHeaderNew* h = desc.d->fileHeader;
			if (!h || !desc.d->data || !desc.d->description) return R::mal("SOBRE_INVALIDO", "No se pudo descifrar");
			if (h->dataSize == 0 || h->descSize <= kBytesNombre || h->dataSize > kMaxArchivo)
				return R::mal("SOBRE_INVALIDO", "Cabecera incoherente");

			s._tipo = textoHasta0(h->fileTypeString, sizeof(h->fileTypeString));
			s._version = textoHasta0(h->gameVersionString, sizeof(h->gameVersionString));
			if (!tipoEsperado.empty() && s._tipo != tipoEsperado)
				return R::mal("SOBRE_TIPO", "Se esperaba " + tipoEsperado + " y es '" + s._tipo + "'");
			if (s._version.find("2021") == std::string::npos)
				return R::mal("SOBRE_NO_PES21", s._version);

			s._datos.assign(desc.d->data, desc.d->data + h->dataSize);
			s._textoInfo = textoHasta0(desc.d->description + kBytesNombre, h->descSize - kBytesNombre);
			return R::bien(std::move(s));
		}
		catch (const std::exception& e) { return R::mal("SOBRE_ERROR", e.what()); }
	}

	Resultado<std::vector<uint8_t>> SobrePes::cifrar(const std::vector<uint8_t>& datos) const {
		using R = Resultado<std::vector<uint8_t>>;
		try {
			if (datos.empty() || datos.size() > kMaxArchivo) return R::mal("SOBRE_INVALIDO", "Tamaño de datos no válido");
			Descriptor desc;
			if (!desc.d) return R::mal("SIN_MEMORIA", "createFileDescriptorNew");
			decryptWithKeyNew(desc.d, _cifrado.data(), clave());
			FileHeaderNew* h = desc.d->fileHeader;
			if (!h || !desc.d->data || !desc.d->description) return R::mal("SOBRE_INVALIDO", "No se pudo reabrir el original");

			if (_infoCambiado) {
				const size_t cabe = h->descSize - kBytesNombre;
				if (_textoInfo.size() >= cabe) return R::mal("SOBRE_INFO_LARGO", "El texto info no cabe (máx. " + std::to_string(cabe - 1) + ")");
				std::memset(desc.d->description + kBytesNombre, 0, cabe);
				std::memcpy(desc.d->description + kBytesNombre, _textoInfo.data(), _textoInfo.size());
			}

			// Los DATOS se sustituyen por un bloque nuevo del tamaño que toque (el descriptor libera el viejo).
			uint8_t* nuevos = static_cast<uint8_t*>(std::malloc(datos.size()));
			if (!nuevos) return R::mal("SIN_MEMORIA", "datos");
			std::memcpy(nuevos, datos.data(), datos.size());
			std::free(desc.d->data);
			desc.d->data = nuevos;
			h->dataSize = static_cast<uint32_t>(datos.size());

			int tam = 0;
			uint8_t* salida = encryptWithKeyNew(desc.d, &tam, clave());
			if (!salida || tam <= 0) return R::mal("SOBRE_ERROR", "No se pudo cifrar");
			std::vector<uint8_t> resultado(salida, salida + tam);
			std::free(salida);
			return R::bien(std::move(resultado));
		}
		catch (const std::exception& e) { return R::mal("SOBRE_ERROR", e.what()); }
	}

	Resultado<std::string> SobrePes::guardarComo(const std::string& rutaNueva, const std::vector<uint8_t>& datos) const {
		using R = Resultado<std::string>;
		namespace fs = std::filesystem;
		std::error_code ec;
		try {
			const fs::path destino = aRuta(rutaNueva);
			if (fs::exists(destino)) return R::mal("DESTINO_OCUPADO", "No se sobrescribe: " + rutaNueva);

			auto cifrado = cifrar(datos);
			if (!cifrado.ok()) return R::mal(cifrado.error.codigo, cifrado.error.detalle);

			// 1) temporal junto al destino  2) relectura  3) renombrar. Si algo falla, no queda nada a medias.
			fs::path temporal = destino;
			temporal += ".pm-tmp";
			{
				std::ofstream f(temporal, std::ios::binary | std::ios::trunc);
				f.write(reinterpret_cast<const char*>(cifrado.valor->data()), static_cast<std::streamsize>(cifrado.valor->size()));
				f.flush();
				if (!f.good()) { f.close(); fs::remove(temporal, ec); return R::mal("ARCHIVO_ERROR", "No se pudo escribir " + rutaNueva); }
			}
			auto relectura = SobrePes::abrir(deRuta(temporal), _tipo);
			if (!relectura.ok() || relectura.valor->_datos != datos) {
				fs::remove(temporal, ec);
				return R::mal("SOBRE_VERIFICACION_FALLIDA", "La relectura no coincide; se descartó el archivo");
			}
			if (fs::exists(destino)) { fs::remove(temporal, ec); return R::mal("DESTINO_OCUPADO", "Apareció durante la escritura: " + rutaNueva); }
			fs::rename(temporal, destino);
			return R::bien(sha256::deArchivo(rutaNueva));
		}
		catch (const std::exception& e) { return R::mal("SOBRE_ERROR", e.what()); }
	}

}
