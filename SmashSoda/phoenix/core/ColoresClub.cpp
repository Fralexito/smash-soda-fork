#include "ColoresClub.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace phoenix::campo {

namespace {
	struct Fila { const char* clave; const char* principal; const char* secundario; };

	// Camiseta principal y secundaria. Orden = prioridad (lo más específico primero: «inter miami» antes que «inter»).
	// Claves ya normalizadas (minúsculas, sin tildes). Se busca como trozo del nombre del juego.
	const Fila kTabla[] = {
		// ---- Selecciones (ConmeGOL: CONMEBOL y más) ----
		{ "argentina", "#75aadb", "#ffffff" }, { "brasil", "#f6d90f", "#009c3b" }, { "brazil", "#f6d90f", "#009c3b" },
		{ "uruguay", "#5cb8e6", "#111827" }, { "colombia", "#fcd116", "#003893" }, { "chile", "#d52b1e", "#0033a0" },
		{ "peru", "#d91023", "#ffffff" }, { "ecuador", "#ffd100", "#0033a0" }, { "paraguay", "#d52b1e", "#0038a8" },
		{ "bolivia", "#007934", "#d52b1e" }, { "venezuela", "#7b1e3a", "#fcd116" }, { "mexico", "#006847", "#ce1126" },
		{ "estados unidos", "#ffffff", "#bf0a30" }, { "usa", "#ffffff", "#bf0a30" }, { "canada", "#d52b1e", "#ffffff" },
		{ "espana", "#c8102e", "#fcd116" }, { "spain", "#c8102e", "#fcd116" }, { "francia", "#1e3a8a", "#ffffff" }, { "france", "#1e3a8a", "#ffffff" },
		{ "alemania", "#ffffff", "#111827" }, { "germany", "#ffffff", "#111827" }, { "italia", "#1d63d8", "#ffffff" }, { "italy", "#1d63d8", "#ffffff" },
		{ "inglaterra", "#ffffff", "#1e3a8a" }, { "england", "#ffffff", "#1e3a8a" }, { "portugal", "#c8102e", "#006600" },
		{ "paises bajos", "#ff6f00", "#1e3a8a" }, { "holanda", "#ff6f00", "#1e3a8a" }, { "netherlands", "#ff6f00", "#1e3a8a" },
		{ "belgica", "#d52b1e", "#fcd116" }, { "belgium", "#d52b1e", "#fcd116" }, { "croacia", "#e5e7eb", "#d52b1e" }, { "croatia", "#e5e7eb", "#d52b1e" },
		{ "japon", "#1d4ed8", "#ffffff" }, { "japan", "#1d4ed8", "#ffffff" }, { "marruecos", "#c1272d", "#006233" }, { "morocco", "#c1272d", "#006233" },
		// ---- Clubes de Perú ----
		{ "universitario", "#f1e3b0", "#8b1a1a" }, { "alianza lima", "#1e3a8a", "#ffffff" }, { "sporting cristal", "#6ec1e4", "#111827" },
		{ "cienciano", "#d52b1e", "#ffffff" }, { "melgar", "#d52b1e", "#111827" }, { "sport boys", "#f472b6", "#111827" },
		// ---- Clubes de Sudamérica ----
		{ "boca", "#1d4ed8", "#fcd116" }, { "river", "#ffffff", "#d52b1e" }, { "flamengo", "#d52b1e", "#111827" },
		{ "palmeiras", "#16a34a", "#ffffff" }, { "corinthians", "#ffffff", "#111827" }, { "sao paulo", "#ffffff", "#d52b1e" },
		{ "santos", "#ffffff", "#111827" }, { "gremio", "#38bdf8", "#111827" }, { "internacional", "#d52b1e", "#ffffff" },
		{ "independiente", "#d52b1e", "#ffffff" }, { "racing", "#7dd3fc", "#ffffff" }, { "san lorenzo", "#1e3a8a", "#d52b1e" },
		{ "penarol", "#fcd116", "#111827" }, { "atletico nacional", "#16a34a", "#ffffff" }, { "nacional", "#ffffff", "#1d4ed8" }, { "colo colo", "#ffffff", "#111827" },
		{ "olimpia", "#ffffff", "#111827" }, { "cerro porteno", "#1e3a8a", "#d52b1e" }, { "atletico nacional", "#16a34a", "#ffffff" },
		// ---- Europa ----
		{ "barcelona", "#a50044", "#004d98" }, { "barca", "#a50044", "#004d98" }, { "real madrid", "#ffffff", "#e5c34a" },
		{ "atletico", "#d52b1e", "#1e3a8a" }, { "sevilla", "#ffffff", "#d52b1e" }, { "valencia", "#ffffff", "#f97316" },
		{ "villarreal", "#fcd116", "#1e3a8a" }, { "betis", "#16a34a", "#ffffff" }, { "real sociedad", "#2563eb", "#ffffff" },
		{ "athletic", "#d52b1e", "#ffffff" }, { "bayern", "#dc052d", "#1e3a8a" }, { "dortmund", "#fde100", "#111827" },
		{ "leverkusen", "#e32221", "#111827" }, { "leipzig", "#d52b1e", "#ffffff" }, { "manchester city", "#6cabdd", "#1c2c5b" },
		{ "man city", "#6cabdd", "#1c2c5b" }, { "manchester united", "#da291c", "#111827" }, { "man united", "#da291c", "#111827" },
		{ "liverpool", "#c8102e", "#00b2a9" }, { "chelsea", "#034694", "#ffffff" }, { "arsenal", "#ef0107", "#ffffff" },
		{ "tottenham", "#ffffff", "#132257" }, { "newcastle", "#e5e7eb", "#111827" }, { "paris", "#1e3a8a", "#d52b1e" },
		{ "psg", "#1e3a8a", "#d52b1e" }, { "marseille", "#38bdf8", "#ffffff" }, { "lyon", "#ffffff", "#1d4ed8" },
		{ "monaco", "#d52b1e", "#ffffff" }, { "juventus", "#ffffff", "#111827" }, { "milan", "#e11d2e", "#111827" },
		{ "inter miami", "#f4a5c0", "#111827" }, { "internazionale", "#1d4ed8", "#111827" }, { "inter", "#1d4ed8", "#111827" }, { "napoli", "#38a3e8", "#ffffff" },
		{ "roma", "#8b1a1a", "#fcd116" }, { "lazio", "#7dc9f2", "#ffffff" }, { "ajax", "#ffffff", "#d52b1e" },
		{ "benfica", "#d52b1e", "#ffffff" }, { "porto", "#1d4ed8", "#ffffff" }, { "sporting", "#16a34a", "#ffffff" },
		{ "celtic", "#16a34a", "#ffffff" }, { "rangers", "#1d4ed8", "#ffffff" },
	};

	int hexVal(char c) {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		return -1;
	}
	bool aRgb(const std::string& h, int& r, int& g, int& b) {
		if (!hexValido(h)) return false;
		r = hexVal(h[1]) * 16 + hexVal(h[2]); g = hexVal(h[3]) * 16 + hexVal(h[4]); b = hexVal(h[5]) * 16 + hexVal(h[6]);
		return true;
	}
	constexpr const char* kGrisAzulado = "#9aa4b2";
	constexpr double kMinLuminancia = 0.12;     ///< por debajo se ve «negro» sobre el césped
	constexpr double kMinDistancia = 110.0;     ///< dos colores más cerca que esto se confunden
}

std::string normalizarNombre(const std::string& s) {
	std::string r;
	r.reserve(s.size());
	for (size_t i = 0; i < s.size(); i++) {
		const unsigned char c = static_cast<unsigned char>(s[i]);
		if (c == 0xC3 && i + 1 < s.size()) {   // tildes y ñ de UTF-8 (á é í ó ú ü ñ y sus mayúsculas)
			const unsigned char d = static_cast<unsigned char>(s[i + 1]);
			char o = 0;
			switch (d) {
				case 0xA1: case 0x81: case 0xA0: case 0x80: o = 'a'; break;
				case 0xA9: case 0x89: case 0xA8: case 0x88: o = 'e'; break;
				case 0xAD: case 0x8D: o = 'i'; break;
				case 0xB3: case 0x93: case 0xB2: case 0x92: o = 'o'; break;
				case 0xBA: case 0x9A: case 0xBC: case 0x9C: o = 'u'; break;
				case 0xB1: case 0x91: o = 'n'; break;
				default: break;
			}
			if (o) { r += o; i++; continue; }
		}
		if (c >= 'A' && c <= 'Z') r += static_cast<char>(c - 'A' + 'a');
		else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) r += static_cast<char>(c);
		else if (!r.empty() && r.back() != ' ') r += ' ';   // cualquier signo o espacio = un espacio
	}
	while (!r.empty() && r.back() == ' ') r.pop_back();
	return r;
}

bool hexValido(const std::string& s) {
	if (s.size() != 7 || s[0] != '#') return false;
	for (size_t i = 1; i < 7; i++) if (hexVal(s[i]) < 0) return false;
	return true;
}

double luminancia(const std::string& hex) {
	int r, g, b;
	if (!aRgb(hex, r, g, b)) return 0.0;
	return (0.2126 * r + 0.7152 * g + 0.0722 * b) / 255.0;
}

double distanciaColor(const std::string& a, const std::string& b) {
	int r1, g1, b1, r2, g2, b2;
	if (!aRgb(a, r1, g1, b1) || !aRgb(b, r2, g2, b2)) return 0.0;
	return std::sqrt(static_cast<double>((r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2)));
}

bool colorDeEquipo(const std::string& nombre, ColorEquipo& salida) {
	const std::string n = normalizarNombre(nombre);
	if (n.empty()) return false;
	const std::string conEspacios = " " + n + " ";
	for (const Fila& f : kTabla) {
		const std::string clave = f.clave;
		// «usa», «roma», «inter»… solo como palabra entera; el resto, como trozo del nombre
		const bool corta = clave.size() <= 5;
		const bool coincide = corta ? conEspacios.find(" " + clave + " ") != std::string::npos : n.find(clave) != std::string::npos;
		if (!coincide) continue;
		salida.principal = f.principal;
		salida.secundario = f.secundario;
		return true;
	}
	return false;
}

namespace {
	/// El color utilizable de un equipo: el principal si se ve sobre el césped; si no, el secundario; si tampoco, gris azulado.
	std::string visible(const ColorEquipo& c) {
		if (luminancia(c.principal) >= kMinLuminancia) return c.principal;
		if (luminancia(c.secundario) >= kMinLuminancia) return c.secundario;
		return kGrisAzulado;
	}
}

ColoresPartido coloresDelPartido(const std::string& nombreA, const std::string& nombreB) {
	ColoresPartido r;
	r.a = kColorPorDefectoA;
	r.b = kColorPorDefectoB;
	ColorEquipo ca, cb;
	r.conocidoA = colorDeEquipo(nombreA, ca);
	r.conocidoB = colorDeEquipo(nombreB, cb);
	if (r.conocidoA) r.a = visible(ca);
	if (r.conocidoB) r.b = visible(cb);
	if (distanciaColor(r.a, r.b) >= kMinDistancia) return r;
	// Se confunden: B prueba su secundario y, si sigue parecido (o no hay), el color por defecto más lejano de A.
	if (r.conocidoB && luminancia(cb.secundario) >= kMinLuminancia && distanciaColor(r.a, cb.secundario) >= kMinDistancia) { r.b = cb.secundario; return r; }
	const std::string opciones[] = { kColorPorDefectoB, kColorPorDefectoA, "#f5c451" };
	double mejor = -1;
	for (const std::string& o : opciones) {
		const double d = distanciaColor(r.a, o);
		if (d > mejor) { mejor = d; r.b = o; }
	}
	return r;
}

}
