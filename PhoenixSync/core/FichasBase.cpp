#include "FichasBase.h"
#include "BaseDatosParche.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include "../terceros/miniz/miniz.h"

namespace fs = std::filesystem;

namespace mercado::base {

	namespace {
		constexpr size_t kTam = 312;
		uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
		void pon32(uint8_t* p, uint32_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); p[2] = uint8_t(v >> 16); p[3] = uint8_t(v >> 24); }
		uint32_t bitsDe(const uint8_t* e, int bit, int n) {
			uint32_t v = 0;
			for (int i = 0; i < n; i++) { const int b = bit + i; v |= uint32_t((e[b >> 3] >> (b & 7)) & 1) << i; }
			return v;
		}
		void ponBits(uint8_t* e, int bit, int n, uint32_t v) {
			for (int i = 0; i < n; i++) {
				const int b = bit + i;
				const uint8_t m = uint8_t(1u << (b & 7));
				if ((v >> i) & 1) e[b >> 3] |= m; else e[b >> 3] &= uint8_t(~m);
			}
		}
	}

	// Bits confirmados con las pantallas del juego (guía 16, §2–§4).
	const std::vector<CampoFicha>& camposFicha() {
		static const std::vector<CampoFicha> c = {
			{ "ataque", "Actitud ofensiva", 370, 6, 40, 40, 99 },
			{ "control", "Control del balón", 281, 6, 40, 40, 99 },
			{ "regate", "Regate", 352, 6, 40, 40, 99 },
			{ "regate_ajustado", "Conservación del balón", 416, 6, 40, 40, 99 },
			{ "pase_raso", "Pase raso", 263, 6, 40, 40, 99 },
			{ "pase_bombeado", "Pase bombeado", 402, 6, 40, 40, 99 },
			{ "finalizacion", "Finalización", 396, 6, 40, 40, 99 },
			{ "cabezazo", "Cabeceo", 288, 6, 40, 40, 99 },
			{ "balon_parado", "Balón parado", 250, 6, 40, 40, 99 },
			{ "efecto", "Efecto", 332, 6, 40, 40, 99 },
			{ "velocidad", "Velocidad", 306, 6, 40, 40, 99 },
			{ "aceleracion", "Aceleración", 344, 6, 40, 40, 99 },
			{ "equilibrio", "Equilibrio", 376, 6, 40, 40, 99 },
			{ "contacto_fisico", "Contacto físico", 390, 6, 40, 40, 99 },
			{ "salto", "Salto", 294, 6, 40, 40, 99 },
			{ "resistencia", "Resistencia", 338, 6, 40, 40, 99 },
			{ "defensa", "Actitud defensiva", 275, 6, 40, 40, 99 },
			{ "recuperacion", "Recuperación de balón", 312, 6, 40, 40, 99 },
			{ "agresividad", "Agresividad", 384, 6, 40, 40, 99 },
			{ "potencia_tiro", "Potencia de tiro", 358, 6, 40, 40, 99 },
			{ "pt_despejar", "Despejar (portero)", 269, 6, 40, 40, 99 },
			{ "pt_cobertura", "Cobertura (portero)", 300, 6, 40, 40, 99 },
			{ "pt_reflejos", "Reflejos (portero)", 320, 6, 40, 40, 99 },
			{ "pt_actitud", "Actitud de portero", 326, 6, 40, 40, 99 },
			{ "pt_atajar", "Atajar (portero)", 364, 6, 40, 40, 99 },
			{ "pie_malo_uso", "Uso del pie malo", 454, 2, 1, 1, 4 },
			{ "pie_malo_precision", "Precisión del pie malo", 462, 2, 1, 1, 4 },
			{ "forma", "Forma (regularidad)", 438, 3, 1, 1, 8 },
			{ "resistencia_lesiones", "Resistencia a lesiones", 458, 2, 1, 1, 3 },
			// Posiciones: 0 = C, 1 = B, 2 = A.
			{ "pos_pt", "Posición PT", 350, 2, 0, 0, 2 },
			{ "pos_dfc", "Posición DFC", 468, 2, 0, 0, 2 },
			{ "pos_li", "Posición LI", 318, 2, 0, 0, 2 },
			{ "pos_ld", "Posición LD", 474, 2, 0, 0, 2 },
			{ "pos_mcd", "Posición MCD", 414, 2, 0, 0, 2 },
			{ "pos_mc", "Posición MC", 456, 2, 0, 0, 2 },
			{ "pos_mi", "Posición MI", 466, 2, 0, 0, 2 },
			{ "pos_md", "Posición MD", 460, 2, 0, 0, 2 },
			{ "pos_mo", "Posición MO", 464, 2, 0, 0, 2 },
			{ "pos_ei", "Posición EI", 472, 2, 0, 0, 2 },
			{ "pos_ed", "Posición ED", 476, 2, 0, 0, 2 },
			{ "pos_sd", "Posición SD", 478, 2, 0, 0, 2 },
			{ "pos_dc", "Posición DC", 470, 2, 0, 0, 2 },
		};
		return c;
	}

	const CampoFicha* campoFicha(const std::string& nombre) {
		for (const auto& c : camposFicha()) if (nombre == c.nombre) return &c;
		return nullptr;
	}

	Resultado<FichasBase> FichasBase::desdeBytes(const std::vector<uint8_t>& d) {
		using R = Resultado<FichasBase>;
		auto raw = descomprimirWesys(d);
		if (!raw.ok()) return R{ std::nullopt, raw.error };
		if (raw.valor->empty() || raw.valor->size() % kTam != 0) return R::mal("PLAYERBIN_FORMATO", "Tamaño no múltiplo de 312");
		FichasBase f;
		f._cabecera.assign(d.begin(), d.begin() + 8);
		f._plano = std::move(*raw.valor);
		return R::bien(std::move(f));
	}

	Resultado<FichasBase> FichasBase::abrir(const std::string& ruta) {
		std::ifstream f(aRuta(ruta), std::ios::binary);
		if (!f) return Resultado<FichasBase>::mal("ARCHIVO_NO_EXISTE", ruta);
		std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), {});
		return desdeBytes(d);
	}

	long FichasBase::indice(uint32_t jugador) const {
		if (!jugador) return -1;
		for (size_t o = 0; o + kTam <= _plano.size(); o += kTam)
			if (le32(&_plano[o + 8]) == jugador) return long(o / kTam);
		return -1;
	}

	Resultado<int> FichasBase::leer(uint32_t jugador, const std::string& campo) const {
		const auto* c = campoFicha(campo);
		if (!c) return Resultado<int>::mal("CAMPO_DESCONOCIDO", campo);
		const long i = indice(jugador);
		if (i < 0) return Resultado<int>::mal("JUGADOR_NO_EXISTE", std::to_string(jugador));
		return Resultado<int>::bien(int(bitsDe(&_plano[size_t(i) * kTam], c->bit, c->bits)) + c->sumar);
	}

	Resultado<bool> FichasBase::poner(uint32_t jugador, const std::string& campo, int valor) {
		const auto* c = campoFicha(campo);
		if (!c) return Resultado<bool>::mal("CAMPO_DESCONOCIDO", campo);
		if (valor < c->minimo || valor > c->maximo)
			return Resultado<bool>::mal("VALOR_FUERA_DE_RANGO", std::string(c->texto) + ": " + std::to_string(c->minimo) + "–" + std::to_string(c->maximo));
		const long i = indice(jugador);
		if (i < 0) return Resultado<bool>::mal("JUGADOR_NO_EXISTE", std::to_string(jugador));
		const uint32_t crudo = uint32_t(valor - c->sumar);
		if (crudo >= (1u << c->bits)) return Resultado<bool>::mal("VALOR_FUERA_DE_RANGO", c->texto);
		ponBits(&_plano[size_t(i) * kTam], c->bit, c->bits, crudo);
		return Resultado<bool>::bien(true);
	}

	Resultado<std::vector<uint8_t>> FichasBase::bytes() const {
		using R = Resultado<std::vector<uint8_t>>;
		mz_ulong n = mz_compressBound(mz_ulong(_plano.size()));
		std::vector<uint8_t> comp(n);
		if (mz_compress2(comp.data(), &n, _plano.data(), mz_ulong(_plano.size()), MZ_BEST_COMPRESSION) != MZ_OK) return R::mal("WESYS_ZLIB", "No se pudo comprimir");
		std::vector<uint8_t> out(16 + n);
		std::memcpy(out.data(), _cabecera.data(), 8);
		pon32(&out[8], uint32_t(n));
		pon32(&out[12], uint32_t(_plano.size()));
		std::memcpy(&out[16], comp.data(), n);
		auto re = descomprimirWesys(out);   // verificación: ida y vuelta idéntica
		if (!re.ok() || *re.valor != _plano) return R::mal("PLAYERBIN_VERIFICACION", "La relectura no coincide");
		return R::bien(std::move(out));
	}

	Resultado<bool> FichasBase::guardarComo(const std::string& ruta) const {
		std::error_code ec;
		if (fs::exists(aRuta(ruta), ec)) return Resultado<bool>::mal("DESTINO_OCUPADO", ruta);
		auto b = bytes();
		if (!b.ok()) return Resultado<bool>{ std::nullopt, b.error };
		std::ofstream f(aRuta(ruta), std::ios::binary);
		f.write(reinterpret_cast<const char*>(b.valor->data()), std::streamsize(b.valor->size()));
		if (!f) return Resultado<bool>::mal("ARCHIVO_ERROR", ruta);
		return Resultado<bool>::bien(true);
	}

	std::vector<uint32_t> FichasBase::distintos(const FichasBase& otra) const {
		std::vector<uint32_t> r;
		const size_t n = std::min(_plano.size(), otra._plano.size());
		for (size_t o = 0; o + kTam <= n; o += kTam)
			if (std::memcmp(&_plano[o], &otra._plano[o], kTam) != 0) r.push_back(le32(&_plano[o + 8]));
		return r;
	}

}
