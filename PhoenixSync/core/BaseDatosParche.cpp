#include "BaseDatosParche.h"
#include "OptionFile.h"   // leerBits, leerTexto

#include <cstring>
#include <fstream>
#include "../terceros/miniz/miniz.h"

namespace mercado {

	const char* const kPosiciones[13] = { "GK","CB","LB","RB","DMF","CMF","LMF","RMF","AMF","LWF","RWF","SS","CF" };

	namespace {
		using Bytes = std::vector<uint8_t>;

		uint32_t be32(const uint8_t* p) { return (uint32_t(p[0]) << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
		uint16_t be16(const uint8_t* p) { return uint16_t((p[0] << 8) | p[1]); }
		uint32_t le32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24); }

		// --- Tabla @UTF de CRI -------------------------------------------
		struct Valor { bool texto = false; uint64_t n = 0; std::string s; };
		using Fila = std::map<std::string, Valor>;

		Bytes desenmascarar(Bytes b) {
			uint32_t m = 0x655f;
			for (auto& c : b) { c ^= uint8_t(m); m = (m * 0x4115) & 0xffff; }
			return b;
		}

		std::vector<Fila> tablaUtf(Bytes b) {
			if (b.size() < 8) throw std::runtime_error("UTF corta");
			if (std::memcmp(b.data(), "@UTF", 4) != 0) b = desenmascarar(b);
			if (std::memcmp(b.data(), "@UTF", 4) != 0) throw std::runtime_error("UTF inválida");
			const uint32_t tam = be32(&b[4]);
			if (8 + tam > b.size()) throw std::runtime_error("UTF truncada");
			const uint8_t* t = &b[8];
			const uint32_t filasOff = be32(t), strOff = be32(t + 4), datOff = be32(t + 8);
			const uint16_t ncols = be16(t + 16), largoFila = be16(t + 18);
			const uint32_t nfilas = be32(t + 20);
			auto texto = [&](uint32_t o) {
				const char* s = reinterpret_cast<const char*>(t + strOff + o);
				return std::string(s, strnlen(s, tam - strOff - o));
			};
			auto leer = [&](const uint8_t*& p, int tipo) {
				Valor v;
				static const int tam[] = { 1,1,2,2,4,4,8,8,4 };
				if (tipo == 0xA) { v.texto = true; v.s = texto(be32(p)); p += 4; }
				else if (tipo == 0xB) { p += 8; }
				else if (tipo <= 8) {
					for (int i = 0; i < tam[tipo]; i++) v.n = (v.n << 8) | p[i];
					p += tam[tipo];
				}
				else throw std::runtime_error("UTF tipo desconocido");
				return v;
			};
			struct Col { std::string nombre; int alm, tipo; Valor cte; };
			std::vector<Col> cols;
			const uint8_t* p = t + 24;
			for (int i = 0; i < ncols; i++) {
				Col c; const int f = *p; c.nombre = texto(be32(p + 1)); p += 5;
				c.alm = f & 0xf0; c.tipo = f & 0x0f;
				if (c.alm == 0x30) c.cte = leer(p, c.tipo);
				cols.push_back(c);
			}
			std::vector<Fila> filas;
			for (uint32_t r = 0; r < nfilas; r++) {
				const uint8_t* q = t + filasOff + r * largoFila;
				Fila fila;
				for (const auto& c : cols) {
					if (c.alm == 0x50) fila[c.nombre] = leer(q, c.tipo);
					else if (c.alm == 0x30) fila[c.nombre] = c.cte;
				}
				filas.push_back(fila);
			}
			(void)datOff;
			return filas;
		}

		// --- Compresión CRILAYLA ------------------------------------------
		Bytes crilayla(const Bytes& c) {
			const uint32_t usize = le32(&c[8]), hoff = le32(&c[12]);
			if (16 + hoff + 0x100 > c.size()) throw std::runtime_error("CRILAYLA corrupto");
			Bytes out(usize + 0x100);
			std::memcpy(out.data(), &c[16 + hoff], 0x100);
			const uint8_t* comp = &c[16];
			long pos = long(hoff) - 1; int bits = 0; uint8_t pool = 0;
			auto get = [&](int n) {
				int v = 0;
				while (n > 0) {
					if (bits == 0) { if (pos < 0) throw std::runtime_error("CRILAYLA sin datos"); pool = comp[pos--]; bits = 8; }
					const int toma = std::min(bits, n);
					v = (v << toma) | ((pool >> (bits - toma)) & ((1 << toma) - 1));
					bits -= toma; n -= toma;
				}
				return v;
			};
			long w = long(usize) - 1;
			uint8_t* u = out.data() + 0x100;
			while (w >= 0) {
				if (get(1)) {
					long off = w + get(13) + 3;
					int largo = 3; static const int nv[] = { 2,3,5,8 }; int i = 0;
					for (;;) { const int b = nv[std::min(i, 3)]; const int x = get(b); largo += x; if (x != (1 << b) - 1) break; i++; }
					for (int k = 0; k < largo && w >= 0; k++) {
						if (off >= long(usize)) throw std::runtime_error("CRILAYLA fuera de rango");
						u[w--] = u[off--];
					}
				}
				else u[w--] = uint8_t(get(8));
			}
			return out;
		}

		struct EntradaCpk { std::string ruta; uint64_t offset, tam; };

		std::vector<EntradaCpk> indice(const Bytes& d) {
			if (d.size() < 0x800 || std::memcmp(d.data(), "CPK ", 4) != 0) throw std::runtime_error("No es un CPK");
			const uint32_t th = le32(&d[8]);
			auto cab = tablaUtf(Bytes(d.begin() + 16, d.begin() + 16 + th));
			if (cab.empty()) throw std::runtime_error("CPK sin cabecera");
			const uint64_t toc = cab[0]["TocOffset"].n;
			uint64_t base = toc;
			if (cab[0].count("ContentOffset") && cab[0]["ContentOffset"].n) base = std::min(base, cab[0]["ContentOffset"].n);
			if (toc + 16 > d.size()) throw std::runtime_error("CPK sin TOC");
			const uint32_t tt = le32(&d[toc + 8]);
			if (toc + 16 + tt > d.size()) throw std::runtime_error("TOC truncado");
			std::vector<EntradaCpk> r;
			for (auto& f : tablaUtf(Bytes(d.begin() + toc + 16, d.begin() + toc + 16 + tt))) {
				std::string dir = f["DirName"].s, nom = f["FileName"].s;
				r.push_back({ dir.empty() ? nom : dir + "/" + nom, base + f["FileOffset"].n, f["FileSize"].n });
			}
			return r;
		}

		Bytes leerArchivo(const std::string& ruta) {
			std::ifstream f(aRuta(ruta), std::ios::binary);
			if (!f) throw std::runtime_error("No se pudo abrir " + ruta);
			return Bytes(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
		}
	}

	Resultado<std::vector<std::string>> cpk::listar(const std::string& rutaCpk) {
		try {
			std::vector<std::string> r;
			for (auto& e : indice(leerArchivo(rutaCpk))) r.push_back(e.ruta);
			return Resultado<std::vector<std::string>>::bien(r);
		}
		catch (const std::exception& e) { return Resultado<std::vector<std::string>>::mal("CPK_ERROR", e.what()); }
	}

	Resultado<std::vector<uint8_t>> cpk::extraer(const std::string& rutaCpk, const std::string& rutaInterna) {
		using R = Resultado<std::vector<uint8_t>>;
		try {
			const Bytes d = leerArchivo(rutaCpk);
			for (auto& e : indice(d)) {
				if (e.ruta != rutaInterna) continue;
				if (e.offset + e.tam > d.size()) return R::mal("CPK_ERROR", "Entrada fuera del archivo");
				Bytes b(d.begin() + e.offset, d.begin() + e.offset + e.tam);
				if (b.size() >= 16 && std::memcmp(b.data(), "CRILAYLA", 8) == 0) b = crilayla(b);
				return R::bien(b);
			}
			return R::mal("CPK_SIN_ARCHIVO", rutaInterna);
		}
		catch (const std::exception& e) { return R::mal("CPK_ERROR", e.what()); }
	}

	Resultado<std::vector<uint8_t>> descomprimirWesys(const std::vector<uint8_t>& d) {
		using R = Resultado<std::vector<uint8_t>>;
		if (d.size() < 16 || std::memcmp(&d[3], "WESYS", 5) != 0) return R::mal("WESYS_INVALIDO");
		const uint32_t comp = le32(&d[8]), tam = le32(&d[12]);
		if (16 + comp > d.size() || tam > 256u * 1024 * 1024) return R::mal("WESYS_INVALIDO", "Tamaños fuera de rango");
		Bytes out(tam);
		mz_ulong largo = tam;
		if (mz_uncompress(out.data(), &largo, &d[16], comp) != MZ_OK || largo != tam) return R::mal("WESYS_ZLIB");
		return R::bien(out);
	}

	Resultado<std::map<uint32_t, FichaJugador>> leerPlayerBin(const std::vector<uint8_t>& raw) {
		using R = Resultado<std::map<uint32_t, FichaJugador>>;
		constexpr size_t kTam = 312;
		if (raw.empty() || raw.size() % kTam != 0) return R::mal("PLAYERBIN_FORMATO", "Tamaño no múltiplo de 312");
		// Habilidad → posición de bit (6 bits, valor real = guardado + 40)
		static const std::pair<const char*, int> kHab[] = {
			{"ataque",370},{"control",281},{"regate",352},{"regate_ajustado",416},{"pase_raso",263},{"pase_bombeado",402},
			{"finalizacion",396},{"cabezazo",288},{"balon_parado",250},{"efecto",332},{"velocidad",306},{"aceleracion",344},
			{"equilibrio",376},{"contacto_fisico",390},{"salto",294},{"resistencia",338},{"defensa",275},{"recuperacion",312},
			{"agresividad",384} };
		std::map<uint32_t, FichaJugador> r;
		for (size_t o = 0; o < raw.size(); o += kTam) {
			const uint8_t* e = &raw[o];
			FichaJugador f;
			f.id = le32(e + 8);
			if (!f.id) continue;
			f.nombre = leerTexto(e + 251, 61);
			f.nombreCamiseta = leerTexto(e + 129, 61);
			f.nacionalidad = int(leerBits(e, 233, 9));
			f.altura = int(leerBits(e, 216, 8)) + 100;
			f.peso = int(leerBits(e, 256, 7)) + 30;
			f.edad = int(leerBits(e, 408, 6)) + 15;
			f.posicion = int(leerBits(e, 434, 4));
			for (const auto& [nom, bit] : kHab) f.habilidades[nom] = int(leerBits(e, bit, 6)) + 40;
			r[f.id] = std::move(f);
		}
		return R::bien(std::move(r));
	}

	Resultado<std::map<uint32_t, FichaJugador>> leerBaseDatos(const std::string& rutaCpk) {
		using R = Resultado<std::map<uint32_t, FichaJugador>>;
		auto b = cpk::extraer(rutaCpk, "common/etc/pesdb/Player.bin");
		if (!b.ok()) return R{ std::nullopt, b.error };
		auto raw = descomprimirWesys(*b.valor);
		if (!raw.ok()) return R{ std::nullopt, raw.error };
		return leerPlayerBin(*raw.valor);
	}

}
