#include "BlobLM.h"

#include <cstring>

extern "C" {
#include "../terceros/miniz/miniz.h"
}

namespace mercado::lm {

	namespace {
		uint32_t le32(const std::vector<uint8_t>& d, size_t o) { return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (uint32_t(d[o + 3]) << 24); }
		uint32_t be32(const std::vector<uint8_t>& d, size_t o) { return (uint32_t(d[o]) << 24) | (d[o + 1] << 16) | (d[o + 2] << 8) | d[o + 3]; }
		void pLe32(std::vector<uint8_t>& d, size_t o, uint32_t v) { for (int i = 0; i < 4; i++) d[o + i] = uint8_t(v >> (8 * i)); }
		void pBe32(std::vector<uint8_t>& d, size_t o, uint32_t v) { for (int i = 0; i < 4; i++) d[o + i] = uint8_t(v >> (8 * (3 - i))); }
		constexpr size_t kCab = 0x1c;   // cabecera rel antes del primer separador
		constexpr size_t kSep = 12;

		/// Descomprime un flujo zlib que empieza en `p` (máx. `max` bytes). Devuelve los bytes consumidos (0 = error).
		size_t inflarFlujo(const uint8_t* p, size_t max, std::vector<uint8_t>& out) {
			mz_stream s{};
			if (mz_inflateInit(&s) != MZ_OK) return 0;
			out.assign(BlobLM::kTamTramo + 4096, 0);
			s.next_in = p; s.avail_in = mz_uint32(max);
			s.next_out = out.data(); s.avail_out = mz_uint32(out.size());
			const int r = mz_inflate(&s, MZ_FINISH);
			const size_t usados = size_t(s.total_in), salida = size_t(s.total_out);
			mz_inflateEnd(&s);
			if (r != MZ_STREAM_END) return 0;
			out.resize(salida);
			return usados;
		}
	}

	Resultado<BlobLM> BlobLM::leer(const std::vector<uint8_t>& d) {
		using R = Resultado<BlobLM>;
		if (d.size() < kOfsTam + 4 + kCab + kSep) return R::mal("BLOB_NO_HALLADO", "El guardado es demasiado corto");
		const size_t base = kOfsTam + 4;
		const uint32_t tam = le32(d, kOfsTam);
		// La palabra de tamaño no cuenta los últimos 12 B (el último tramo termina 12 B después): zona real = tam + 12.
		if (base + size_t(tam) + kSep > d.size() || tam < kCab + kSep) return R::mal("BLOB_NO_HALLADO", "Tamaño de la zona comprimida fuera de rango");
		if (le32(d, base) != 0x0007458c) return R::mal("BLOB_NO_HALLADO", "La cabecera de la zona comprimida no es la esperada");
		BlobLM b;
		b._cabecera.assign(d.begin() + long(base), d.begin() + long(base + kCab));
		b._finZona = base + tam + kSep;
		size_t pos = base + kCab;
		uint64_t sumaDesc = 0, sumaComp = 0;
		while (pos + kSep <= b._finZona) {
			Tramo t;
			t.campoDesc = be32(d, pos); t.campoComp = be32(d, pos + 4); t.campoFin = be32(d, pos + 8);
			std::vector<uint8_t> plano;
			const size_t usados = inflarFlujo(d.data() + pos + kSep, b._finZona - pos - kSep, plano);
			if (!usados) return R::mal("BLOB_DANADO", "Un tramo comprimido no se puede leer (" + std::to_string(b._tramos.size()) + ")");
			t.comprimido.assign(d.begin() + long(pos + kSep), d.begin() + long(pos + kSep + usados));
			t.ini = b._plano.size(); t.len = plano.size();
			b._plano.insert(b._plano.end(), plano.begin(), plano.end());
			sumaDesc += t.campoDesc; sumaComp += t.campoComp;
			pos += kSep + usados;
			const bool ultimo = pos >= b._finZona;
			// Los campos dicen la verdad en todos los tramos (el último lleva fin = 0); así se comprueba que se entendió el formato.
			if (t.campoDesc != t.len || t.campoComp != usados || t.campoFin != (ultimo ? 0u : uint32_t(pos - kSep - usados - base) + t.campoComp))
				return R::mal("BLOB_FORMATO", "Los campos del tramo " + std::to_string(b._tramos.size()) + " no cuadran");
			b._tramos.push_back(std::move(t));
		}
		if (pos != b._finZona || b._tramos.empty()) return R::mal("BLOB_FORMATO", "Los tramos no terminan donde dice el tamaño");
		if (be32(d, base + 0x14) != uint32_t(sumaDesc) || be32(d, base + 0x18) != uint32_t(sumaComp))
			return R::mal("BLOB_FORMATO", "Las sumas de la cabecera no cuadran con los tramos");
		b._planoOriginal = b._plano;
		return R::bien(std::move(b));
	}

	Resultado<std::vector<uint8_t>> BlobLM::aplicar(const std::vector<uint8_t>& d) const {
		using R = Resultado<std::vector<uint8_t>>;
		if (_plano.size() != _planoOriginal.size()) return R::mal("BLOB_TAMANO", "El contenido del blob cambió de tamaño");
		std::vector<uint8_t> zona = _cabecera;
		uint64_t sumaDesc = 0, sumaComp = 0;
		for (size_t i = 0; i < _tramos.size(); i++) {
			const Tramo& t = _tramos[i];
			std::vector<uint8_t> comp;
			uint32_t campoDesc = t.campoDesc, campoComp = t.campoComp;
			if (std::memcmp(_plano.data() + t.ini, _planoOriginal.data() + t.ini, t.len) == 0) comp = t.comprimido;
			else {
				mz_ulong n = mz_compressBound(mz_ulong(t.len));
				comp.resize(n);
				if (mz_compress2(comp.data(), &n, _plano.data() + t.ini, mz_ulong(t.len), MZ_BEST_COMPRESSION) != MZ_OK)
					return R::mal("BLOB_COMPRIMIR", "No se pudo comprimir el tramo " + std::to_string(i));
				comp.resize(n);
				campoComp = uint32_t(comp.size());
			}
			const size_t relSep = zona.size();
			std::vector<uint8_t> sep(kSep);
			pBe32(sep, 0, campoDesc); pBe32(sep, 4, campoComp);
			pBe32(sep, 8, i + 1 < _tramos.size() ? uint32_t(relSep + campoComp) : 0u);
			zona.insert(zona.end(), sep.begin(), sep.end());
			zona.insert(zona.end(), comp.begin(), comp.end());
			sumaDesc += campoDesc; sumaComp += campoComp;
		}
		pBe32(zona, 0x14, uint32_t(sumaDesc)); pBe32(zona, 0x18, uint32_t(sumaComp));
		std::vector<uint8_t> out;
		out.reserve(d.size() + zona.size());
		out.insert(out.end(), d.begin(), d.begin() + long(kOfsTam));
		std::vector<uint8_t> tam(4); pLe32(tam, 0, uint32_t(zona.size() - kSep));
		out.insert(out.end(), tam.begin(), tam.end());
		out.insert(out.end(), zona.begin(), zona.end());
		out.insert(out.end(), d.begin() + long(_finZona), d.end());
		// Comprobación: el resultado se tiene que poder leer y dar exactamente el contenido nuevo.
		auto re = leer(out);
		if (!re.ok()) return R::mal("BLOB_VERIFICACION", re.error.detalle);
		if (re.valor->_plano != _plano) return R::mal("BLOB_VERIFICACION", "El blob reescrito no se lee igual");
		return R::bien(std::move(out));
	}

	long long BlobLM::fichaDe(uint32_t reg, uint32_t pid) const {
		// Fichas de 156 B desde +0x1e, una por `reg` (la ficha del reg r empieza en 0x1e + 156·r; `reg` y `pid` en +2 y +6).
		// Los jugadores con reg 0xdb65xxxx (creados o añadidos por el juego) siguen en el MISMO arreglo, justo después:
		// su posición usa los 16 bits bajos (el primero es 0xdb654026 = índice 16.422 en el ConmeGOL 26). §21.
		const uint32_t indice = (reg >> 16) == 0xdb65 ? (reg & 0xffffu) : reg;
		const size_t o = 0x1e + size_t(indice) * kTamFicha;
		if (o + kTamFicha > _plano.size()) return -1;
		if (le32(_plano, o + 2) != reg || le32(_plano, o + 6) != pid) return -1;
		return (long long)o;
	}

}
