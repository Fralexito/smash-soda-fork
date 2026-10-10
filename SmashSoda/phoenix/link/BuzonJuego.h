#pragma once
// Buzon del juego: Phoenix Link escribe <juego>\SiderAddons\content\phoenix\avisos.txt
// (atomico, UTF-8 sin BOM, max 14 lineas / 1500 bytes). Solo escribe ese archivo de texto.
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::buzon {

struct Aviso {
	long long id = 0;
	std::string texto;
	long long creadoEn = 0; // segundos Unix UTC
};

constexpr int MAX_LINEAS = 14;
constexpr int MAX_BYTES = 1500;

// "hh:mm" en hora de Lima (UTC-5 fija).
std::string horaLima(long long unixSeg);
// "2026-10-09T05:42:31.000Z" -> segundos Unix UTC (0 si no se entiende).
long long segundosDeIso(const std::string& iso);
// Corta a como mucho `max` bytes sin partir un caracter UTF-8.
std::string cortarUtf8(const std::string& s, size_t max);
// Avisos (del mas nuevo al mas viejo) -> texto del archivo. `incluidos` recibe los ids que si caben.
std::string formatear(const std::vector<Aviso>& avisos, std::vector<long long>* incluidos = nullptr);
std::string textoSinAvisos();

enum class Puente { JuegoCerrado, NoInstalado, Listo };

struct EstadoJuego {
	Puente puente = Puente::JuegoCerrado;
	std::filesystem::path carpetaJuego;
	std::filesystem::path carpetaBuzon; // <juego>\SiderAddons\content\phoenix
};

// Busca PES2021.exe en ejecucion (solo Windows; en otros sistemas: JuegoCerrado).
EstadoJuego detectarJuego();
// Mira si existe la carpeta del buzon dentro de una carpeta de juego (nunca la crea).
EstadoJuego estadoDeCarpeta(const std::filesystem::path& carpetaJuego);

// avisos.tmp + reemplazo atomico. No crea carpetas. Reintenta hasta 10 veces cada 100 ms.
bool escribirAtomico(const std::filesystem::path& carpetaBuzon, const std::string& contenido, std::string* error = nullptr);

} // namespace phoenix::buzon
