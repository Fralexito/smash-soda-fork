#include "Copias.h"
#include "Sha256.h"

#include <chrono>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

namespace mercado::copias {

	std::string nombreConFecha(const std::string& nombreArchivo) {
		const std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm local{};
#ifdef _WIN32
		localtime_s(&local, &t);
#else
		localtime_r(&t, &local);
#endif
		char sello[32];
		std::strftime(sello, sizeof(sello), "%Y-%m-%d_%H-%M-%S", &local);
		return nombreArchivo + "." + sello + ".bak";
	}

	Resultado<Copia> crear(const std::string& origen, const std::string& carpetaCopias) {
		try {
			const fs::path src = aRuta(origen);
			if (!fs::is_regular_file(src)) return Resultado<Copia>::mal("ARCHIVO_NO_EXISTE", origen);

			std::string err;
			const std::string huellaOrigen = sha256::deArchivo(origen, &err);
			if (huellaOrigen.empty()) return Resultado<Copia>::mal("ARCHIVO_ILEGIBLE", err);

			const fs::path carpeta = aRuta(carpetaCopias);
			fs::create_directories(carpeta);

			// Nunca sobrescribir: si ya existe (dos copias en el mismo segundo), se añade -2, -3…
			const std::string base = nombreConFecha(deRuta(src.filename()));
			fs::path destino = carpeta / aRuta(base);
			for (int i = 2; fs::exists(destino); i++) {
				if (i > 99) return Resultado<Copia>::mal("COPIA_NOMBRE_OCUPADO", deRuta(destino));
				destino = carpeta / aRuta(base.substr(0, base.size() - 4) + "-" + std::to_string(i) + ".bak");
			}

			fs::copy_file(src, destino, fs::copy_options::none); // none = falla si existe
			const std::string huellaCopia = sha256::deArchivo(deRuta(destino), &err);
			if (huellaCopia != huellaOrigen) {
				std::error_code ec; fs::remove(destino, ec); // solo borra la copia defectuosa que acaba de crear
				return Resultado<Copia>::mal("COPIA_CORRUPTA", "La huella de la copia no coincide");
			}
			return Resultado<Copia>::bien({ deRuta(destino), huellaCopia, static_cast<long long>(fs::file_size(destino)) });
		}
		catch (const std::exception& e) {
			return Resultado<Copia>::mal("ARCHIVO_ERROR", e.what());
		}
	}

}
