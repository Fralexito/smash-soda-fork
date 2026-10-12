#include "Fichajes.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>

#include "BuzonJuego.h"
#include "Entrega.h"
#include "ModulosJuego.h"

namespace phoenix::fichajes {

namespace {
	constexpr size_t kMaxArchivo = 64u * 1024u * 1024u;   // ni la base más grande pasa de unos 10 MB
	constexpr size_t kMaxCambios = 5000;
	constexpr uint32_t kMaxId = 0x0FFFFFFF;

	std::string sha(const std::string& s) { return phoenix::entrega::sha256Hex(s.data(), s.size()); }

	bool leerArchivo(const fs::path& ruta, std::string& out, uintmax_t maximo = kMaxArchivo) {
		std::error_code ec;
		const uintmax_t n = fs::file_size(ruta, ec);
		if (ec || n > maximo) return false;
		std::ifstream f(ruta, std::ios::binary);
		if (!f) return false;
		out.assign(static_cast<size_t>(n), '\0');
		if (n > 0) f.read(&out[0], static_cast<std::streamsize>(n));
		return static_cast<uintmax_t>(f.gcount()) == n;
	}

	std::string sello() {
		std::time_t t = std::time(nullptr);
		std::tm lt{};
#ifdef _WIN32
		localtime_s(&lt, &t);
#else
		localtime_r(&t, &lt);
#endif
		char b[32];
		std::strftime(b, sizeof(b), "%Y%m%d-%H%M%S", &lt);
		return b;
	}
	std::string fechaLegible() {
		std::time_t t = std::time(nullptr);
		std::tm lt{};
#ifdef _WIN32
		localtime_s(&lt, &t);
#else
		localtime_r(&t, &lt);
#endif
		char b[32];
		std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &lt);
		return b;
	}

	uint32_t le32(const std::string& s, size_t p) {
		return static_cast<uint32_t>(static_cast<unsigned char>(s[p])) | (static_cast<uint32_t>(static_cast<unsigned char>(s[p + 1])) << 8)
			| (static_cast<uint32_t>(static_cast<unsigned char>(s[p + 2])) << 16) | (static_cast<uint32_t>(static_cast<unsigned char>(s[p + 3])) << 24);
	}
	void poner32(std::string& s, uint32_t v) {
		for (int i = 0; i < 4; i++) s.push_back(static_cast<char>((v >> (8 * i)) & 0xFF));
	}

	// ─── inflate (algoritmo de «puff» de Mark Adler, reescrito) ──────────────────────────────────
	constexpr int MAXBITS = 15, MAXLCODES = 286, MAXDCODES = 30, FIXLCODES = 288;
	struct Huff { short count[MAXBITS + 1]; short symbol[FIXLCODES]; };
	struct Flujo {
		const unsigned char* in = nullptr; size_t n = 0, pos = 0;
		long bitbuf = 0; int bitcnt = 0;
		std::string* out = nullptr; size_t maximo = 0;
		std::string error;
	};
	int bits(Flujo& s, int need) {
		long val = s.bitbuf;
		while (s.bitcnt < need) {
			if (s.pos >= s.n) { if (s.error.empty()) s.error = "los datos se cortan antes de tiempo"; return 0; }
			val |= static_cast<long>(s.in[s.pos++]) << s.bitcnt;
			s.bitcnt += 8;
		}
		s.bitbuf = val >> need;
		s.bitcnt -= need;
		return static_cast<int>(val & ((1L << need) - 1));
	}
	int decodificar(Flujo& s, const Huff& h) {
		int code = 0, first = 0, index = 0;
		for (int len = 1; len <= MAXBITS; len++) {
			code |= bits(s, 1);
			if (!s.error.empty()) return -1;
			const int count = h.count[len];
			if (code - count < first) return h.symbol[index + (code - first)];
			index += count; first += count; first <<= 1; code <<= 1;
		}
		return -1;
	}
	int construir(Huff& h, const short* length, int n) {
		for (int len = 0; len <= MAXBITS; len++) h.count[len] = 0;
		for (int sym = 0; sym < n; sym++) h.count[length[sym]]++;
		if (h.count[0] == n) return 0;
		int left = 1;
		for (int len = 1; len <= MAXBITS; len++) { left <<= 1; left -= h.count[len]; if (left < 0) return left; }
		short offs[MAXBITS + 1];
		offs[1] = 0;
		for (int len = 1; len < MAXBITS; len++) offs[len + 1] = static_cast<short>(offs[len] + h.count[len]);
		for (int sym = 0; sym < n; sym++) if (length[sym] != 0) h.symbol[offs[length[sym]]++] = static_cast<short>(sym);
		return left;
	}
	bool codigos(Flujo& s, const Huff& lc, const Huff& dc) {
		static const short lens[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
		static const short lext[29] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
		static const short dists[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
		static const short dext[30] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };
		int sym;
		do {
			sym = decodificar(s, lc);
			if (sym < 0) { if (s.error.empty()) s.error = "código de compresión no válido"; return false; }
			if (sym < 256) {
				if (s.out->size() >= s.maximo) { s.error = "el archivo descomprimido es demasiado grande"; return false; }
				s.out->push_back(static_cast<char>(sym));
			}
			else if (sym > 256) {
				sym -= 257;
				if (sym >= 29) { s.error = "símbolo de longitud no válido"; return false; }
				const int len = lens[sym] + bits(s, lext[sym]);
				const int ds = decodificar(s, dc);
				if (ds < 0 || ds >= 30) { if (s.error.empty()) s.error = "símbolo de distancia no válido"; return false; }
				const size_t dist = static_cast<size_t>(dists[ds] + bits(s, dext[ds]));
				if (!s.error.empty()) return false;
				if (dist > s.out->size()) { s.error = "distancia fuera de rango"; return false; }
				if (s.out->size() + static_cast<size_t>(len) > s.maximo) { s.error = "el archivo descomprimido es demasiado grande"; return false; }
				for (int k = 0; k < len; k++) s.out->push_back((*s.out)[s.out->size() - dist]);
			}
		} while (sym != 256);
		return true;
	}
	bool bloqueFijo(Flujo& s) {
		static bool listo = false;
		static Huff lc, dc;
		if (!listo) {
			short lengths[FIXLCODES];
			int sym = 0;
			for (; sym < 144; sym++) lengths[sym] = 8;
			for (; sym < 256; sym++) lengths[sym] = 9;
			for (; sym < 280; sym++) lengths[sym] = 7;
			for (; sym < FIXLCODES; sym++) lengths[sym] = 8;
			construir(lc, lengths, FIXLCODES);
			for (sym = 0; sym < MAXDCODES; sym++) lengths[sym] = 5;
			construir(dc, lengths, MAXDCODES);
			listo = true;
		}
		return codigos(s, lc, dc);
	}
	bool bloqueDinamico(Flujo& s) {
		static const short orden[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
		short lengths[MAXLCODES + MAXDCODES];
		const int nlen = bits(s, 5) + 257, ndist = bits(s, 5) + 1, ncode = bits(s, 4) + 4;
		if (!s.error.empty()) return false;
		if (nlen > MAXLCODES || ndist > MAXDCODES) { s.error = "tabla de compresión no válida"; return false; }
		int idx = 0;
		for (; idx < ncode; idx++) lengths[orden[idx]] = static_cast<short>(bits(s, 3));
		for (; idx < 19; idx++) lengths[orden[idx]] = 0;
		Huff lc, dc;
		if (construir(lc, lengths, 19) != 0) { s.error = "tabla de compresión no válida"; return false; }
		idx = 0;
		while (idx < nlen + ndist) {
			int sym = decodificar(s, lc);
			if (sym < 0) { if (s.error.empty()) s.error = "tabla de compresión no válida"; return false; }
			if (sym < 16) lengths[idx++] = static_cast<short>(sym);
			else {
				int len = 0, rep;
				if (sym == 16) { if (idx == 0) { s.error = "tabla de compresión no válida"; return false; } len = lengths[idx - 1]; rep = 3 + bits(s, 2); }
				else if (sym == 17) rep = 3 + bits(s, 3);
				else rep = 11 + bits(s, 7);
				if (idx + rep > nlen + ndist) { s.error = "tabla de compresión no válida"; return false; }
				while (rep--) lengths[idx++] = static_cast<short>(len);
			}
		}
		if (!s.error.empty()) return false;
		if (lengths[256] == 0) { s.error = "tabla de compresión no válida"; return false; }
		int err = construir(lc, lengths, nlen);
		if (err && (err < 0 || nlen != lc.count[0] + lc.count[1])) { s.error = "tabla de compresión no válida"; return false; }
		err = construir(dc, lengths + nlen, ndist);
		if (err && (err < 0 || ndist != dc.count[0] + dc.count[1])) { s.error = "tabla de compresión no válida"; return false; }
		return codigos(s, lc, dc);
	}
	uint32_t adler32(const std::string& d) {
		uint32_t a = 1, b = 0;
		for (unsigned char c : d) { a = (a + c) % 65521u; b = (b + a) % 65521u; }
		return (b << 16) | a;
	}
}

bool inflarZlib(const std::string& z, std::string& salida, size_t maximo, std::string& error) {
	salida.clear();
	if (z.size() < 6) { error = "flujo zlib demasiado corto"; return false; }
	const unsigned cmf = static_cast<unsigned char>(z[0]), flg = static_cast<unsigned char>(z[1]);
	if ((cmf & 0x0F) != 8 || ((cmf << 8) | flg) % 31 != 0 || (flg & 0x20)) { error = "cabecera zlib no válida"; return false; }
	Flujo s;
	s.in = reinterpret_cast<const unsigned char*>(z.data());
	s.n = z.size();
	s.pos = 2;
	s.out = &salida;
	s.maximo = maximo;
	bool ultimo;
	do {
		ultimo = bits(s, 1) != 0;
		const int tipo = bits(s, 2);
		if (!s.error.empty()) { error = s.error; return false; }
		bool ok = false;
		if (tipo == 0) {
			s.bitbuf = 0; s.bitcnt = 0;
			if (s.pos + 4 > s.n) { error = "los datos se cortan antes de tiempo"; return false; }
			const unsigned len = s.in[s.pos] | (s.in[s.pos + 1] << 8), nlen = s.in[s.pos + 2] | (s.in[s.pos + 3] << 8);
			s.pos += 4;
			if (len != (~nlen & 0xFFFFu)) { error = "bloque sin comprimir dañado"; return false; }
			if (s.pos + len > s.n) { error = "los datos se cortan antes de tiempo"; return false; }
			if (salida.size() + len > maximo) { error = "el archivo descomprimido es demasiado grande"; return false; }
			salida.append(reinterpret_cast<const char*>(s.in + s.pos), len);
			s.pos += len;
			ok = true;
		}
		else if (tipo == 1) ok = bloqueFijo(s);
		else if (tipo == 2) ok = bloqueDinamico(s);
		else { error = "tipo de bloque no válido"; return false; }
		if (!ok) { error = s.error.empty() ? "datos dañados" : s.error; return false; }
	} while (!ultimo);
	if (s.pos + 4 > s.n) { error = "falta la suma de comprobación"; return false; }
	const uint32_t esperado = (static_cast<uint32_t>(s.in[s.pos]) << 24) | (static_cast<uint32_t>(s.in[s.pos + 1]) << 16) | (static_cast<uint32_t>(s.in[s.pos + 2]) << 8) | s.in[s.pos + 3];
	if (adler32(salida) != esperado) { error = "la suma de comprobación no coincide (archivo dañado)"; return false; }
	return true;
}

std::string zlibAlmacenado(const std::string& crudo) {
	std::string z;
	z.push_back('\x78'); z.push_back('\x01');
	size_t pos = 0;
	do {
		const size_t n = (std::min<size_t>)(65535, crudo.size() - pos);
		const bool ultimo = pos + n >= crudo.size();
		z.push_back(ultimo ? '\x01' : '\x00');
		z.push_back(static_cast<char>(n & 0xFF)); z.push_back(static_cast<char>((n >> 8) & 0xFF));
		z.push_back(static_cast<char>(~n & 0xFF)); z.push_back(static_cast<char>((~n >> 8) & 0xFF));
		z.append(crudo, pos, n);
		pos += n;
	} while (pos < crudo.size());
	const uint32_t a = adler32(crudo);
	for (int i = 3; i >= 0; i--) z.push_back(static_cast<char>((a >> (8 * i)) & 0xFF));
	return z;
}

bool leerBin(const std::string& a, std::string& crudo, std::string& error) {
	if (a.size() < 24 || static_cast<unsigned char>(a[0]) != 0xFF || a.compare(3, 5, "WESYS") != 0) { error = "no es un .bin de PES (falta la cabecera WESYS)"; return false; }
	const uint32_t comp = le32(a, 8), real = le32(a, 12);
	if (static_cast<size_t>(comp) + 16 != a.size()) { error = "el tamaño de la cabecera no coincide con el archivo"; return false; }
	if (real > kMaxArchivo) { error = "archivo demasiado grande"; return false; }
	if (!inflarZlib(a.substr(16), crudo, kMaxArchivo, error)) return false;
	if (crudo.size() != real) { error = "el tamaño descomprimido no coincide con la cabecera"; return false; }
	return true;
}

std::string escribirBin(const std::string& crudo) {
	const std::string z = zlibAlmacenado(crudo);
	std::string r;
	r.push_back('\xFF'); r.push_back('\x10'); r.push_back('\x81');
	r += "WESYS";
	poner32(r, static_cast<uint32_t>(z.size()));
	poner32(r, static_cast<uint32_t>(crudo.size()));
	r += z;
	return r;
}

// ─── CPK ─────────────────────────────────────────────────────────────────────────────────────────
namespace {
	struct Val { long long n = 0; std::string s; };
	using FilaUtf = std::map<std::string, Val>;

	bool be(const std::string& b, size_t p, int bytes, unsigned long long& v) {
		if (p + static_cast<size_t>(bytes) > b.size()) return false;
		v = 0;
		for (int i = 0; i < bytes; i++) v = (v << 8) | static_cast<unsigned char>(b[p + static_cast<size_t>(i)]);
		return true;
	}

	unsigned long long le64(const std::string& b, size_t p) {
		unsigned long long v = 0;
		for (int i = 7; i >= 0; i--) v = (v << 8) | static_cast<unsigned char>(b[p + static_cast<size_t>(i)]);
		return v;
	}

	bool leerUtf(const std::string& buf, size_t off, std::vector<FilaUtf>& filas, std::string& error) {
		if (off + 16 > buf.size()) { error = "CPK cortado"; return false; }
		unsigned long long tam = 0;
		std::string t;
		if (buf.compare(off, 4, "@UTF") != 0) {
			if (off < 8 || off + 8 > buf.size() || (tam = le64(buf, off - 8)) < 32 || off + tam > buf.size()) { error = "tabla del CPK no válida"; return false; }
			t = buf.substr(off, static_cast<size_t>(tam));
			unsigned k = 0x655F;
			for (char& c : t) { c = static_cast<char>(static_cast<unsigned char>(c) ^ (k & 0xFF)); k = (k * 0x4115) & 0xFFFF; }
		}
		else {
			unsigned long long ts = 0;
			if (!be(buf, off + 4, 4, ts) || off + 8 + ts > buf.size()) { error = "tabla del CPK no válida"; return false; }
			t = buf.substr(off, static_cast<size_t>(ts) + 8);
		}
		if (t.compare(0, 4, "@UTF") != 0) { error = "tabla del CPK no válida (¿no es un CPK de PES?)"; return false; }
		unsigned long long rowsOff, strOff, dataOff, ncol, rowW, nrows;
		if (!be(t, 10, 2, rowsOff) || !be(t, 12, 4, strOff) || !be(t, 16, 4, dataOff) || !be(t, 24, 2, ncol) || !be(t, 26, 2, rowW) || !be(t, 28, 4, nrows)) { error = "tabla del CPK cortada"; return false; }
		const size_t base = 8;
		if (ncol > 200 || nrows > 400000) { error = "tabla del CPK demasiado grande"; return false; }
		struct Col { std::string nombre; int st, ty; Val cte; };
		std::vector<Col> cols;
		size_t p = base + 24;
		auto cadena = [&](unsigned long long o, std::string& out) {
			const size_t i = base + static_cast<size_t>(strOff) + static_cast<size_t>(o);
			if (i >= t.size()) return false;
			const size_t f = t.find('\0', i);
			if (f == std::string::npos) return false;
			out = t.substr(i, f - i);
			return true;
		};
		auto valor = [&](size_t& q, int ty, Val& v) {
			unsigned long long x = 0;
			switch (ty) {
			case 0: case 1: if (!be(t, q, 1, x)) return false; q += 1; v.n = static_cast<long long>(x); return true;
			case 2: case 3: if (!be(t, q, 2, x)) return false; q += 2; v.n = static_cast<long long>(x); return true;
			case 4: case 5: case 8: if (!be(t, q, 4, x)) return false; q += 4; v.n = static_cast<long long>(x); return true;
			case 6: case 7: if (!be(t, q, 8, x)) return false; q += 8; v.n = static_cast<long long>(x); return true;
			case 0xA: if (!be(t, q, 4, x)) return false; q += 4; return cadena(x, v.s);
			case 0xB: if (!be(t, q, 8, x)) return false; q += 8; return true;
			default: return false;
			}
		};
		for (unsigned long long c = 0; c < ncol; c++) {
			if (p + 5 > t.size()) { error = "tabla del CPK cortada"; return false; }
			Col col;
			const int fl = static_cast<unsigned char>(t[p]);
			unsigned long long no = 0;
			be(t, p + 1, 4, no);
			p += 5;
			col.st = fl >> 4; col.ty = fl & 15;
			if (!cadena(no, col.nombre)) { error = "tabla del CPK: nombre de columna no válido"; return false; }
			if (col.st == 3 && !valor(p, col.ty, col.cte)) { error = "tabla del CPK: valor no válido"; return false; }
			cols.push_back(col);
		}
		for (unsigned long long r = 0; r < nrows; r++) {
			size_t q = base + static_cast<size_t>(rowsOff) + static_cast<size_t>(r) * static_cast<size_t>(rowW);
			FilaUtf fila;
			for (const Col& c : cols) {
				Val v;
				if (c.st == 5) { if (!valor(q, c.ty, v)) { error = "tabla del CPK: fila no válida"; return false; } }
				else if (c.st == 3) v = c.cte;
				fila[c.nombre] = v;
			}
			filas.push_back(std::move(fila));
		}
		return true;
	}
}

bool entradasCpk(const LectorCpk& leer, uint64_t tam, const std::string& carpeta, std::vector<EntradaCpk>& entradas, std::string& error) {
	entradas.clear();
	std::string cab;
	if (tam < 64 || !leer(0, 16, cab) || cab.size() < 16 || cab.compare(0, 4, "CPK ") != 0) { error = "no es un archivo CPK"; return false; }
	const unsigned long long tcab = le64(cab, 8);
	if (tcab < 32 || tcab > 16ull * 1024 * 1024 || 16 + tcab > tam) { error = "CPK: cabecera no válida"; return false; }
	std::string h;
	if (!leer(0, static_cast<size_t>(16 + tcab), h) || h.size() != 16 + tcab) { error = "CPK: no se pudo leer la cabecera"; return false; }
	std::vector<FilaUtf> cabf;
	if (!leerUtf(h, 16, cabf, error)) return false;
	if (cabf.empty() || !cabf[0].count("TocOffset") || !cabf[0].count("ContentOffset")) { error = "CPK sin tabla de archivos"; return false; }
	const unsigned long long toc = static_cast<unsigned long long>(cabf[0]["TocOffset"].n), contenido = static_cast<unsigned long long>(cabf[0]["ContentOffset"].n);
	std::string t16;
	if (toc + 32 > tam || !leer(toc, 16, t16) || t16.size() < 16 || t16.compare(0, 4, "TOC ") != 0) { error = "CPK: tabla de archivos no encontrada"; return false; }
	const unsigned long long ttoc = le64(t16, 8);
	if (ttoc < 32 || ttoc > 256ull * 1024 * 1024 || toc + 16 + ttoc > tam) { error = "CPK: tabla de archivos no válida"; return false; }
	std::string tb;
	if (!leer(toc, static_cast<size_t>(16 + ttoc), tb) || tb.size() != 16 + ttoc) { error = "CPK: no se pudo leer la tabla de archivos"; return false; }
	std::vector<FilaUtf> filas;
	if (!leerUtf(tb, 16, filas, error)) return false;
	for (FilaUtf& f : filas) {
		if (f["DirName"].s != carpeta) continue;
		EntradaCpk e;
		e.carpeta = carpeta; e.nombre = f["FileName"].s;
		e.desde = contenido + static_cast<unsigned long long>(f["FileOffset"].n);
		e.tam = static_cast<uint64_t>(f["FileSize"].n); e.tamReal = static_cast<uint64_t>(f["ExtractSize"].n);
		entradas.push_back(e);
	}
	return true;
}

bool extraerDeCpk(const std::string& d, const std::string& carpeta, const std::string& nombre, std::string& bytes, std::string& error) {
	const LectorCpk lector = [&d](uint64_t desde, size_t cuanto, std::string& out) {
		if (desde > d.size() || desde + cuanto > d.size()) return false;
		out = d.substr(static_cast<size_t>(desde), cuanto);
		return true;
	};
	std::vector<EntradaCpk> es;
	if (!entradasCpk(lector, d.size(), carpeta, es, error)) return false;
	for (const EntradaCpk& e : es) {
		if (e.nombre != nombre) continue;
		if (e.tam != e.tamReal) { error = "el CPK guarda " + nombre + " comprimido (no soportado)"; return false; }
		if (e.tam == 0 || e.desde + e.tam > d.size()) { error = "CPK: " + nombre + " fuera de rango"; return false; }
		bytes = d.substr(static_cast<size_t>(e.desde), static_cast<size_t>(e.tam));
		return true;
	}
	error = "el CPK no tiene " + carpeta + "/" + nombre;
	return false;
}

std::map<std::string, int> leerDpFileList(const std::string& b) {
	std::map<std::string, int> r;
	// registros de 48 bytes: la prioridad (u32) va 12 bytes antes del nombre y el nombre del CPK en +16 (32 bytes)
	for (size_t off = 16; off + 32 <= b.size(); off += 48) {
		const size_t fin = b.find('\0', off);
		const std::string nombre = b.substr(off, (fin == std::string::npos || fin > off + 32 ? off + 32 : fin) - off);
		if (nombre.size() < 5) continue;
		std::string m = nombre;
		for (char& c : m) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		if (m.compare(m.size() - 4, 4, ".cpk") != 0) continue;
		r[m] = static_cast<int>(le32(b, off - 12));
	}
	return r;
}

// ─── PlayerAssignment / Player / Team ────────────────────────────────────────────────────────────
bool leerAsignacion(const std::string& crudo, std::vector<Fila>& filas, std::string& error) {
	filas.clear();
	if (crudo.size() % 16 != 0 || crudo.size() < 16 * 1000 || crudo.size() > 16 * 400000) { error = "PlayerAssignment.bin con un tamaño que no es de PES 2021"; return false; }
	const size_t n = crudo.size() / 16;
	filas.reserve(n);
	for (size_t i = 0; i < n; i++) {
		Fila f{ le32(crudo, i * 16), le32(crudo, i * 16 + 4), le32(crudo, i * 16 + 8), le32(crudo, i * 16 + 12) };
		if (f.jugador == 0 || f.jugador > kMaxId || f.equipo == 0 || f.equipo > kMaxId) { error = "PlayerAssignment.bin: fila " + std::to_string(i) + " con jugador o equipo imposible"; filas.clear(); return false; }
		filas.push_back(f);
	}
	return true;
}

std::string escribirAsignacion(const std::vector<Fila>& filas) {
	std::string r;
	r.reserve(filas.size() * 16);
	for (const Fila& f : filas) { poner32(r, f.id); poner32(r, f.jugador); poner32(r, f.equipo); poner32(r, f.empaq); }
	return r;
}

std::string plegarNombre(const std::string& s) {
	// ¿UTF-8 válido? Si no, se toma como Latin-1.
	bool utf8 = true;
	for (size_t i = 0; i < s.size() && utf8;) {
		const unsigned char c = static_cast<unsigned char>(s[i]);
		const size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
		if (n == 0 || i + n > s.size()) { utf8 = false; break; }
		for (size_t k = 1; k < n; k++) if ((static_cast<unsigned char>(s[i + k]) & 0xC0) != 0x80) utf8 = false;
		i += n;
	}
	std::vector<uint32_t> cps;
	if (utf8) {
		for (size_t i = 0; i < s.size();) {
			const unsigned char c = static_cast<unsigned char>(s[i]);
			uint32_t cp; size_t n;
			if (c < 0x80) { cp = c; n = 1; }
			else if ((c >> 5) == 0x6) { cp = c & 0x1F; n = 2; }
			else if ((c >> 4) == 0xE) { cp = c & 0x0F; n = 3; }
			else { cp = c & 0x07; n = 4; }
			for (size_t k = 1; k < n; k++) cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
			cps.push_back(cp); i += n;
		}
	}
	else for (unsigned char c : s) cps.push_back(c);
	static const char* const lat1 = "aaaaaaaceeeeiiiidnooooo*ouuuuyts" "aaaaaaaceeeeiiiidnooooo/ouuuuyty";   // U+00C0…U+00FF
	std::string r;
	for (uint32_t cp : cps) {
		if (cp < 0x80) { if (std::isalnum(static_cast<int>(cp))) r.push_back(static_cast<char>(std::tolower(static_cast<int>(cp)))); }
		else if (cp >= 0xC0 && cp <= 0xFF) { const char c = lat1[cp - 0xC0]; if (c != '*' && c != '/') r.push_back(c); }
		else if (cp == 0x141 || cp == 0x142) r.push_back('l');
		else if (cp == 0x110 || cp == 0x111) r.push_back('d');
		else if (cp == 0x15E || cp == 0x15F || cp == 0x160 || cp == 0x161) r.push_back('s');
		else if (cp == 0x130 || cp == 0x131) r.push_back('i');
		else if (cp == 0x11E || cp == 0x11F) r.push_back('g');
		else if (cp == 0x10C || cp == 0x10D || cp == 0x106 || cp == 0x107) r.push_back('c');
		else if (cp == 0x17D || cp == 0x17E || cp == 0x179 || cp == 0x17A || cp == 0x17B || cp == 0x17C) r.push_back('z');
	}
	return r;
}

bool leerJugadores(const std::string& crudo, std::set<uint32_t>& ids, std::string& error, std::map<uint32_t, std::string>* nombres) {
	ids.clear();
	if (nombres) nombres->clear();
	// cabecera de 8 bytes + registros de 312 bytes; el archivo real termina con el último registro recortado (8 bytes menos)
	if (crudo.size() < 8 + 312 * 1000 || ((crudo.size() - 8) % 312 != 0 && (crudo.size() - 8) % 312 != 304)) { error = "Player.bin con un tamaño que no es de PES 2021"; return false; }
	for (size_t o = 8; o + 4 <= crudo.size(); o += 312) {
		const uint32_t id = le32(crudo, o);
		if (!id) continue;
		ids.insert(id);
		if (nombres && o + 243 + 61 <= crudo.size()) {
			const std::string n(crudo.data() + o + 243, 61);
			(*nombres)[id] = plegarNombre(n.substr(0, n.find('\0')));
		}
	}
	if (ids.size() < 1000) { error = "Player.bin con muy pocos jugadores"; return false; }
	return true;
}

bool leerEquipos(const std::string& crudo, std::map<uint32_t, std::string>& nombres, std::string& error) {
	nombres.clear();
	const size_t R = 1532;
	if (crudo.size() < R * 100 || crudo.size() % R != 0) { error = "Team.bin con un tamaño que no es de PES 2021"; return false; }
	for (size_t o = 0; o + R <= crudo.size(); o += R) {
		const uint32_t id = le32(crudo, o);
		const std::string n(crudo.data() + o + 368, 70);
		const std::string f = plegarNombre(n.substr(0, n.find('\0')));
		if (id && !f.empty()) nombres[id] = f;
	}
	if (nombres.size() < 50) { error = "Team.bin con muy pocos equipos"; return false; }
	return true;
}

// ─── lista ───────────────────────────────────────────────────────────────────────────────────────
bool leerLista(const std::string& texto, Lista& lista, std::string& error) {
	lista = Lista();
	lista.sha256 = sha(texto);
	const nlohmann::json j = nlohmann::json::parse(texto, nullptr, false);
	if (j.is_discarded() || !j.is_object()) { error = "fichajes.json no es un JSON válido"; return false; }
	if (!j.contains("version") || !j["version"].is_number_integer()) { error = "fichajes.json: falta «version»"; return false; }
	lista.version = j["version"].get<int>();
	if (lista.version < 1 || lista.version > kVersionLista) { error = "fichajes.json: versión " + std::to_string(lista.version) + " no soportada (esta Link entiende la 1 y la " + std::to_string(kVersionLista) + ")"; return false; }
	if (j.contains("regla")) {
		const std::string r = j["regla"].is_string() ? j["regla"].get<std::string>() : std::string();
		if (r == "estricta") lista.estricta = true;
		else if (r == "relajada") lista.estricta = false;
		else { error = "fichajes.json: «regla» debe ser «relajada» o «estricta»"; return false; }
	}
	if (j.contains("nombre") && j["nombre"].is_string()) lista.nombre = j["nombre"].get<std::string>().substr(0, 80);
	if (!j.contains("cambios") || !j["cambios"].is_array()) { error = "fichajes.json: falta la lista «cambios»"; return false; }
	if (j["cambios"].size() > kMaxCambios) { error = "fichajes.json: demasiados cambios"; return false; }
	auto id = [](const nlohmann::json& o, const char* k, uint32_t& out, bool& hay) {
		hay = o.contains(k);
		if (!hay) return true;
		if (!o[k].is_number_integer()) return false;
		const long long v = o[k].get<long long>();
		if (v < 0 || v > kMaxId) return false;
		out = static_cast<uint32_t>(v);
		return true;
	};
	auto nombre = [](const nlohmann::json& o, const char* k, std::string& out, bool& hay) {
		hay = o.contains(k);
		if (!hay) return true;
		if (!o[k].is_string()) return false;
		out = o[k].get<std::string>();
		return !out.empty() && out.size() <= 80;
	};
	for (size_t i = 0; i < j["cambios"].size(); i++) {
		const nlohmann::json& o = j["cambios"][i];
		const std::string donde = "fichajes.json: el cambio " + std::to_string(i + 1);
		Cambio c;
		if (!o.is_object()) { error = donde + " no es un objeto"; return false; }
		bool hJug = false, hEq = false, hOri = false, hNJug = false, hNEq = false, hNOri = false, hNEq2 = false;
		if (!id(o, "player_id", c.jugador, hJug) || !id(o, "team_id", c.equipo, hEq) || !id(o, "origen", c.origen, hOri)) { error = donde + " tiene ids que no son números válidos"; return false; }
		if ((hJug && c.jugador == 0) || (hEq && c.equipo == 0)) { error = donde + ": player_id y team_id no pueden ser 0"; return false; }
		if (!nombre(o, "jugador", c.jugadorNombre, hNJug) || !nombre(o, "equipo_destino", c.equipoNombre, hNEq) || !nombre(o, "equipo_origen", c.origenNombre, hNOri)) { error = donde + " tiene nombres vacíos o demasiado largos"; return false; }
		if (!hNEq && !nombre(o, "equipo", c.equipoNombre, hNEq2)) { error = donde + " tiene un nombre de equipo no válido"; return false; }
		hNEq = hNEq || hNEq2;
		if (lista.version == 1 && (hNJug || hNEq || hNOri)) { error = donde + ": los nombres necesitan «version»: 2"; return false; }
		if ((!hJug && !hNJug) || (!hEq && !hNEq)) { error = donde + " necesita jugador (player_id o «jugador») y equipo destino (team_id o «equipo_destino»)"; return false; }
		if ((hJug && hNJug) || (hEq && hNEq) || (hOri && hNOri)) { error = donde + ": usa o el id o el nombre, no los dos"; return false; }
		if (o.contains("nota") && o["nota"].is_string()) c.nota = o["nota"].get<std::string>().substr(0, 120);
		lista.cambios.push_back(c);
	}
	return true;
}

// ─── aplicar ─────────────────────────────────────────────────────────────────────────────────────
namespace {
	/// Puestos (>>10) y números (&1023) ocupados en un equipo.
	void ocupados(const std::vector<Fila>& f, uint32_t equipo, std::set<uint32_t>& puestos, std::set<uint32_t>& numeros, int& cuantos) {
		puestos.clear(); numeros.clear(); cuantos = 0;
		for (const Fila& x : f) if (x.equipo == equipo) { puestos.insert(x.empaq >> 10); numeros.insert(x.empaq & 1023); cuantos++; }
	}
	/// ids cuyo nombre plegado coincide EXACTAMENTE.
	std::vector<uint32_t> porNombre(const std::map<uint32_t, std::string>& m, const std::string& plegado) {
		std::vector<uint32_t> r;
		if (plegado.empty()) return r;
		for (const auto& e : m) if (e.second == plegado) r.push_back(e.first);
		return r;
	}
}

Resultado aplicar(const std::vector<Fila>& base, const std::set<uint32_t>& jugadores, const Lista& lista, const Nombres* nombres) {
	Resultado r;
	r.filas = base;
	std::set<uint32_t> equipos;
	for (const Fila& f : base) equipos.insert(f.equipo);
	uint32_t maxId = 0;
	for (const Fila& f : base) maxId = (std::max)(maxId, f.id);
	for (const Cambio& original : lista.cambios) {
		Cambio c = original;
		auto salto = [&](const char* cod, const std::string& mot) { r.saltados.push_back({ c, cod, mot }); };
		// Versión 2: nombres → ids, contra la base de ESTE PC (coincidencia exacta; si hay duda, se salta)
		if (c.jugador == 0 || c.equipo == 0 || (c.origen == 0 && !c.origenNombre.empty())) {
			if (!nombres || nombres->jugador.empty() || nombres->equipo.empty()) { salto("nombres_no_disponibles", "Este cambio se indica por nombre y no se pudieron leer los nombres de jugadores y equipos de la base de este parche."); continue; }
			bool fallo = false;
			auto equipoPorNombre = [&](const std::string& nom, const char* que, uint32_t& destino) {
				const std::vector<uint32_t> v = porNombre(nombres->equipo, plegarNombre(nom));
				if (v.empty()) { salto("equipo_no_encontrado", std::string("No hay ningún equipo llamado «") + nom + "» (" + que + ") en la base de este parche."); fallo = true; }
				else if (v.size() > 1) { salto("equipo_ambiguo", std::string("Hay ") + std::to_string(v.size()) + " equipos llamados «" + nom + "» (" + que + "): no sé cuál es."); fallo = true; }
				else destino = v[0];
			};
			if (c.equipo == 0) equipoPorNombre(c.equipoNombre, "destino", c.equipo);
			if (!fallo && c.origen == 0 && !c.origenNombre.empty()) equipoPorNombre(c.origenNombre, "origen", c.origen);
			if (fallo) continue;
			if (c.jugador == 0) {
				std::vector<uint32_t> v = porNombre(nombres->jugador, plegarNombre(c.jugadorNombre));
				if (c.origen != 0) {
					std::vector<uint32_t> en;
					for (uint32_t id : v) for (const Fila& f : r.filas) if (f.jugador == id && f.equipo == c.origen) { en.push_back(id); break; }
					v = en;
				}
				if (v.empty()) { salto("jugador_no_encontrado", "No hay ningún jugador llamado «" + c.jugadorNombre + "»" + (c.origen ? " en el equipo de origen indicado" : "") + " en la base de este parche."); continue; }
				if (v.size() > 1) { salto("jugador_ambiguo", "Hay " + std::to_string(v.size()) + " jugadores llamados «" + c.jugadorNombre + "»" + (c.origen ? " en ese equipo" : "") + ": indica el equipo de origen o usa player_id."); continue; }
				c.jugador = v[0];
			}
		}
		if (!jugadores.count(c.jugador)) { salto("jugador_no_existe", "El jugador " + std::to_string(c.jugador) + " no existe en la base de este parche."); continue; }
		if (!equipos.count(c.equipo)) { salto("equipo_no_existe", "El equipo " + std::to_string(c.equipo) + " no existe en la base de este parche."); continue; }
		std::vector<size_t> suyas;
		bool yaEsta = false;
		for (size_t i = 0; i < r.filas.size(); i++) if (r.filas[i].jugador == c.jugador) { suyas.push_back(i); if (r.filas[i].equipo == c.equipo) yaEsta = true; }
		if (yaEsta) { r.yaEstaban++; continue; }
		long long fila = -1;   // fila que se mueve (-1 = alta)
		std::string que = "movido";
		if (c.origen != 0) {
			for (size_t i : suyas) if (r.filas[i].equipo == c.origen) fila = static_cast<long long>(i);
			if (fila < 0) {
				if (lista.estricta) { salto("origen_cambiado", "El jugador ya no está en el equipo " + std::to_string(c.origen) + " (regla estricta: no se toca)."); continue; }
				if (suyas.size() > 1) { salto("ambiguo", "Tiene varios equipos y ninguno es el " + std::to_string(c.origen) + ": no sé cuál mover."); continue; }
				if (suyas.size() == 1) { fila = static_cast<long long>(suyas[0]); que = "movido_origen_distinto"; }
			}
		}
		else {
			if (suyas.size() > 1) { salto("ambiguo", "Tiene varios equipos: indica el equipo de origen (campo «origen» o «equipo_origen»)."); continue; }
			if (suyas.size() == 1) fila = static_cast<long long>(suyas[0]);
		}
		if (fila < 0) que = "alta";
		std::set<uint32_t> puestos, numeros;
		int cuantos = 0;
		ocupados(r.filas, c.equipo, puestos, numeros, cuantos);
		if (cuantos >= kMaxPorEquipo) { salto("equipo_lleno", "El equipo " + std::to_string(c.equipo) + " ya tiene " + std::to_string(kMaxPorEquipo) + " jugadores."); continue; }
		uint32_t puesto = 0;
		while (puestos.count(puesto)) puesto++;
		uint32_t numero = fila >= 0 ? (r.filas[static_cast<size_t>(fila)].empaq & 1023) : 1;
		if (fila < 0 || numeros.count(numero)) { numero = 1; while (numeros.count(numero) && numero < 1023) numero++; }
		const uint32_t empaq = (puesto << 10) | numero;
		if (fila >= 0) { r.filas[static_cast<size_t>(fila)].equipo = c.equipo; r.filas[static_cast<size_t>(fila)].empaq = empaq; }
		else r.filas.push_back({ ++maxId, c.jugador, c.equipo, empaq });
		r.aplicados.push_back({ c, que });
	}
	return r;
}

bool validar(const std::vector<Fila>& base, const std::vector<Fila>& nuevas, std::string& error) {
	std::set<uint32_t> ids;
	std::set<std::pair<uint32_t, uint32_t>> pares;
	std::map<uint32_t, std::set<uint32_t>> puestos;
	std::map<uint32_t, int> cuantos;
	for (const Fila& f : nuevas) {
		if (!ids.insert(f.id).second) { error = "id de fila repetido: " + std::to_string(f.id); return false; }
		if (!pares.insert({ f.jugador, f.equipo }).second) { error = "el jugador " + std::to_string(f.jugador) + " saldría dos veces en el equipo " + std::to_string(f.equipo); return false; }
		cuantos[f.equipo]++;
		if (!puestos[f.equipo].insert(f.empaq >> 10).second) { error = "puesto repetido en el equipo " + std::to_string(f.equipo); return false; }
	}
	std::map<uint32_t, int> antes;
	for (const Fila& f : base) antes[f.equipo]++;
	for (const auto& c : cuantos) if (c.second > kMaxPorEquipo && c.second > antes[c.first]) { error = "el equipo " + std::to_string(c.first) + " pasaría de " + std::to_string(kMaxPorEquipo) + " jugadores"; return false; }
	return true;
}

// ─── juego real: detector de base por evidencia ──────────────────────────────────────────────────
namespace {
	const char* const kCarpetaPesdb = "common/etc/pesdb";

	fs::path dePesdb(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb"; }
	fs::path dePhoenixDB(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB"; }

	std::string minus(std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); return s; }

	/// ".\livecpk\X" → livecpk/X (o una ruta absoluta tal cual)
	fs::path rutaRaiz(const std::string& r) {
		std::string x = r;
		std::replace(x.begin(), x.end(), '\\', '/');
		while (x.compare(0, 2, "./") == 0) x.erase(0, 2);
		return phoenix::entrega::deU8(x);
	}
	bool esPhoenixDB(const std::string& r) { std::string m = minus(r); std::replace(m.begin(), m.end(), '/', '\\'); while (!m.empty() && m.back() == '\\') m.pop_back(); return m.size() >= 10 && m.compare(m.size() - 10, 10, "phoenix-db") == 0; }
	bool esWesys(const std::string& b) { return b.size() >= 8 && static_cast<unsigned char>(b[0]) == 0xFF && b.compare(3, 5, "WESYS") == 0; }

	struct InfoCpk {
		bool ok = false;
		std::string error;
		std::map<std::string, EntradaCpk> pesdb;   // nombre → entrada
	};
	struct Crudo { bool ok = false; std::string datos; };

	/// Caché de lecturas pesadas dentro de una misma pasada (varios modos comparten la misma base).
	struct Cache {
		std::map<std::string, Crudo> crudos;     // clave → bytes reales (o error)
		std::map<std::string, InfoCpk> cpks;     // ruta|tamaño|fecha → tabla del CPK
	};

	std::string claveArchivo(const fs::path& p) {
		std::error_code ec;
		const uintmax_t n = fs::file_size(p, ec);
		const auto t = fs::last_write_time(p, ec);
		return phoenix::entrega::aU8(p) + "|" + std::to_string(ec ? 0 : n) + "|" + std::to_string(static_cast<long long>(t.time_since_epoch().count()));
	}

	const InfoCpk& escanearCpk(Cache& c, const fs::path& ruta) {
		const std::string k = claveArchivo(ruta);
		auto it = c.cpks.find(k);
		if (it != c.cpks.end()) return it->second;
		InfoCpk info;
		std::error_code ec;
		const uintmax_t tam = fs::file_size(ruta, ec);
		std::ifstream f(ruta, std::ios::binary);
		if (ec || !f) info.error = "no se pudo abrir";
		else {
			const LectorCpk lector = [&f](uint64_t desde, size_t cuanto, std::string& out) {
				f.clear(); f.seekg(static_cast<std::streamoff>(desde));
				out.assign(cuanto, '\0');
				f.read(&out[0], static_cast<std::streamsize>(cuanto));
				return static_cast<size_t>(f.gcount()) == cuanto;
			};
			std::vector<EntradaCpk> es;
			if (entradasCpk(lector, tam, kCarpetaPesdb, es, info.error)) { info.ok = true; for (const EntradaCpk& e : es) info.pesdb[e.nombre] = e; }
		}
		return c.cpks.emplace(k, info).first->second;
	}

	/// Dónde está un archivo de la base, tal como lo buscaría el juego.
	struct Fuente {
		bool hay = false;
		bool esCpk = false;
		fs::path ruta;           // carpeta pesdb (cpk.root) o el CPK
		EntradaCpk ent;
		std::string etiqueta;    // «cpk.root #2 «.\olmosjr23\Database»» | «CGP_database.cpk (prioridad 1)»
		std::string error;       // hay varias bases con la misma prioridad…
	};

	struct CandCpk { fs::path ruta; long rango = 0; EntradaCpk ent; std::string nota; };

	struct Contexto {
		Cache& cache;
		fs::path juego;
		Objetivo o;
		std::vector<std::pair<std::string, fs::path>> raices;   // (etiqueta, carpeta pesdb) en orden de prioridad, solo las que existen
		bool cpksListos = false;
		std::vector<fs::path> cpkRutas; std::vector<long> cpkRangos; std::vector<std::string> cpkNotas;
		explicit Contexto(Cache& c) : cache(c) {}
	};

	void prepararRaices(Contexto& x) {
		std::string ini;
		leerArchivo(x.o.ini, ini, 512 * 1024);
		std::error_code ec;
		int n = 0;
		for (const std::string& r : phoenix::modulos::raicesActivas(ini)) {
			n++;
			if (esPhoenixDB(r)) continue;
			const fs::path rel = rutaRaiz(r);
			std::vector<fs::path> bases;
			if (rel.is_absolute()) bases.push_back(rel);
			else { bases.push_back(x.o.siderAddons / rel); if (!x.o.esRaiz) bases.push_back(x.juego / "SiderAddons" / rel); }
			for (const fs::path& b : bases) {
				const fs::path p = b / "common" / "etc" / "pesdb";
				if (fs::is_directory(p, ec)) { x.raices.push_back({ "cpk.root #" + std::to_string(n) + " «" + r + "»" + (b == bases[0] ? "" : " (de la raíz del juego)"), p }); break; }
			}
		}
	}

	void prepararCpks(Contexto& x) {
		if (x.cpksListos) return;
		x.cpksListos = true;
		std::error_code ec;
		auto listar = [&](const fs::path& dir, long rangoDefecto, bool propio, const std::set<std::string>& excluir, const std::map<std::string, int>& rangos) {
			int n = 0;
			for (fs::directory_iterator it(dir, ec), fin; !ec && it != fin && n < 400; it.increment(ec), ++n) {
				std::error_code e2;
				if (!it->is_regular_file(e2)) continue;
				const std::string nombre = minus(phoenix::entrega::aU8(it->path().filename()));
				if (nombre.size() < 5 || nombre.compare(nombre.size() - 4, 4, ".cpk") != 0 || excluir.count(nombre)) continue;
				long rango = rangoDefecto;
				std::string nota = "sin prioridad en DpFileList.bin";
				const auto r = rangos.find(nombre);
				if (r != rangos.end()) { rango = r->second; nota = "prioridad " + std::to_string(r->second) + " en DpFileList.bin"; }
				else if (propio) { rango = -1000; nota = "CPK propio del modo"; }
				x.cpkRutas.push_back(it->path()); x.cpkRangos.push_back(rango); x.cpkNotas.push_back(nota);
			}
		};
		auto rangosDe = [&](const fs::path& dir) {
			std::string b;
			if (leerArchivo(dir / "DpFileList.bin", b, 1024 * 1024)) return leerDpFileList(b);
			return std::map<std::string, int>();
		};
		std::set<std::string> propios;
		if (!x.o.esRaiz) {
			const fs::path dirModo = x.o.cpk.parent_path();
			listar(dirModo, -1000, true, {}, rangosDe(dirModo));
			for (const fs::path& p : x.cpkRutas) propios.insert(minus(phoenix::entrega::aU8(p.filename())));
		}
		listar(x.juego / "download", 500, false, propios, rangosDe(x.juego / "download"));
		listar(x.juego / "Data", 5000, false, {}, {});
	}

	/// Resuelve un archivo de la base como lo haría el juego: primero las cpk.root (en orden), luego los CPK (el de mejor prioridad).
	Fuente resolver(Contexto& x, const char* nombre) {
		Fuente f;
		std::error_code ec;
		for (const auto& r : x.raices) {
			if (fs::is_regular_file(r.second / nombre, ec)) { f.hay = true; f.ruta = r.second; f.etiqueta = r.first; return f; }
		}
		prepararCpks(x);
		std::vector<CandCpk> cands;
		for (size_t i = 0; i < x.cpkRutas.size(); i++) {
			const InfoCpk& info = escanearCpk(x.cache, x.cpkRutas[i]);
			if (!info.ok) continue;
			const auto e = info.pesdb.find(nombre);
			if (e == info.pesdb.end()) continue;
			cands.push_back({ x.cpkRutas[i], x.cpkRangos[i], e->second, x.cpkNotas[i] });
		}
		if (cands.empty()) return f;
		long mejor = cands[0].rango;
		for (const CandCpk& c : cands) mejor = (std::min)(mejor, c.rango);
		std::vector<const CandCpk*> top;
		for (const CandCpk& c : cands) if (c.rango == mejor) top.push_back(&c);
		if (top.size() > 1) {
			std::string nombres;
			for (const CandCpk* c : top) nombres += (nombres.empty() ? "" : ", ") + phoenix::entrega::aU8(c->ruta.filename());
			f.hay = true; f.error = "Hay varios CPK con " + std::string(nombre) + " y la misma prioridad (" + nombres + "): no sé cuál usa el juego.";
			return f;
		}
		f.hay = true; f.esCpk = true; f.ruta = top[0]->ruta; f.ent = top[0]->ent;
		f.etiqueta = phoenix::entrega::aU8(top[0]->ruta.filename()) + " (" + top[0]->nota + ")";
		return f;
	}

	/// Bytes reales de un archivo de la base (descomprimido por el formato WESYS).
	bool leerFuente(Contexto& x, const Fuente& f, const char* nombre, std::string& crudo, std::string& error) {
		const std::string clave = (f.esCpk ? phoenix::entrega::aU8(f.ruta) + "|cpk|" + std::to_string(f.ent.desde) : phoenix::entrega::aU8(f.ruta / nombre)) + "|" + claveArchivo(f.esCpk ? f.ruta : f.ruta / nombre);
		auto it = x.cache.crudos.find(clave);
		if (it == x.cache.crudos.end()) {
			Crudo c;
			std::string bytes, e;
			if (f.esCpk) {
				if (f.ent.tam != f.ent.tamReal) e = std::string("el CPK guarda ") + nombre + " comprimido (formato que Link no lee)";
				else if (f.ent.tam == 0 || f.ent.tam > kMaxArchivo) e = "tamaño imposible";
				else {
					std::ifstream in(f.ruta, std::ios::binary);
					in.seekg(static_cast<std::streamoff>(f.ent.desde));
					bytes.assign(static_cast<size_t>(f.ent.tam), '\0');
					in.read(&bytes[0], static_cast<std::streamsize>(bytes.size()));
					if (static_cast<uint64_t>(in.gcount()) != f.ent.tam) e = "no se pudo leer del CPK";
				}
			}
			else if (!leerArchivo(f.ruta / nombre, bytes)) e = std::string("no se pudo leer ") + nombre;
			if (e.empty()) {
				if (!esWesys(bytes)) e = std::string(nombre) + " no tiene el formato de PES de los parches (cabecera WESYS): son datos cifrados o de otro formato";
				else c.ok = leerBin(bytes, c.datos, e);
			}
			if (!c.ok) c.datos = e;
			it = x.cache.crudos.emplace(clave, c).first;
		}
		if (!it->second.ok) { error = it->second.datos; return false; }
		crudo = it->second.datos;
		return true;
	}

	struct Base {
		bool ok = false;
		std::string parche, origen, error, queHacer, asignacionSha;
		bool generica = false, nombresOk = false;
		std::vector<std::string> evidencias;
		std::vector<Fila> filas;
		std::set<uint32_t> jugadores;
		std::string jugadoresSha;
		Nombres nombres;
	};

	struct Huella { const char* sha; const char* nombre; };
	const Huella kHuellas[] = {
		{ "358c51b8ef11", "ConmeGOL 26 (base olmosjr23)" },
		{ "2f9a1776b74f", "ConmeGOL Patch 26 (CPK)" },
		{ "6e050ede4ec7", "ConmeGOL Patch 27" },
		{ "19dafb11bfa3", "UEFAGOL 27" },
		{ "b00403cb0527", "ConmeGOL Clásicos" },
	};

	std::string primeraLinea(const fs::path& p) {
		std::string t;
		if (!leerArchivo(p, t, 4096)) return std::string();
		if (t.compare(0, 3, "\xEF\xBB\xBF") == 0) t.erase(0, 3);
		t = t.substr(0, t.find_first_of("\r\n"));
		while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
		return t;
	}

	/// Identifica el parche por señales (no por una lista cerrada): huella de la base, carpeta del modo, último modo lanzado, nombre del CPK.
	void identificar(Base& b, const Fuente& fuenteAsig, const Contexto& x) {
		const std::string huella = b.asignacionSha.substr(0, 12);
		for (const Huella& h : kHuellas) if (huella == h.sha) { b.parche = h.nombre; b.evidencias.push_back("La huella de PlayerAssignment.bin (" + huella + ") coincide con la conocida de «" + std::string(h.nombre) + "»."); }
		const std::string cpkNombre = fuenteAsig.esCpk ? minus(phoenix::entrega::aU8(fuenteAsig.ruta.filename())) : std::string();
		if (b.parche.empty() && !x.o.esRaiz) { b.parche = x.o.nombre; b.evidencias.push_back("Es el modo «" + x.o.nombre + "» del cambiador de parches (nombre de su carpeta)."); }
		if (b.parche.empty() && x.o.esRaiz) {
			const std::string va = primeraLinea(x.juego / "version_actual.txt");
			if (!va.empty()) {
				for (const fs::path& m : phoenix::modulos::carpetasModos(x.juego)) if (phoenix::entrega::aU8(m.filename()) == va) { b.parche = va; break; }
				if (!b.parche.empty()) b.evidencias.push_back("version_actual.txt dice que el último modo lanzado fue «" + va + "» y existe su carpeta.");
			}
		}
		if (b.parche.empty() && cpkNombre.compare(0, 2, "dt") == 0) { b.parche = "PES 2021 original"; b.evidencias.push_back("La base sale de un CPK oficial del juego («" + cpkNombre + "»)."); }
		if (b.parche.empty()) {
			b.parche = cpkNombre.empty() ? "parche desconocido (base en carpeta)" : "parche desconocido (" + cpkNombre + ")";
			b.generica = true;
			b.evidencias.push_back("No reconozco este parche por nombre ni huella, pero su base se lee y cumple el formato: la uso como base genérica.");
		}
	}

	Base resolverBase(Contexto& x, bool necesitaNombres) {
		Base b;
		prepararRaices(x);
		const Fuente fa = resolver(x, "PlayerAssignment.bin");
		const Fuente fj = resolver(x, "Player.bin");
		if (!fa.hay) {
			b.error = "No encontré PlayerAssignment.bin ni en las cpk.root del sider.ini ni en ningún CPK del juego (download, Data).";
			b.queHacer = "Comprueba que la carpeta del juego es la correcta y que el parche trae su base de datos. No se crea nada.";
			return b;
		}
		if (!fa.error.empty()) { b.error = fa.error; b.queHacer = "Link no adivina: no se crea nada. Deja un solo CPK con la base o ponle la prioridad en DpFileList.bin."; return b; }
		if (!fj.hay || !fj.error.empty()) { b.error = fj.hay ? fj.error : "No encontré Player.bin junto a PlayerAssignment.bin."; b.queHacer = "No se crea nada."; return b; }
		b.origen = fa.etiqueta;
		b.evidencias.push_back(std::string(fa.esCpk ? "Ninguna cpk.root trae PlayerAssignment.bin; lo trae el CPK " : "Lo trae la ") + fa.etiqueta + (fa.esCpk ? ", el de mejor prioridad." : ", la de más prioridad en sider.ini."));
		if (fj.etiqueta != fa.etiqueta) b.evidencias.push_back("Player.bin sale de " + fj.etiqueta + ".");
		std::string crudoAsig, crudoJug, e;
		const bool vanilla = fa.esCpk && minus(phoenix::entrega::aU8(fa.ruta.filename())).compare(0, 2, "dt") == 0;
		if (!leerFuente(x, fa, "PlayerAssignment.bin", crudoAsig, e)) {
			b.error = "La base de «" + fa.etiqueta + "» no se puede leer: " + e + ".";
			b.queHacer = vanilla
				? "Es la data original cifrada de PES 2021: los fichajes necesitan un parche cuya base (CGP_database.cpk, olmosjr23\\Database…) esté en el formato de los parches. No se crea nada."
				: "Si es un parche distinto, mándame el informe (Sala › Conexión › Diagnóstico del PC). No se crea nada.";
			if (vanilla) { b.parche = "PES 2021 original"; b.evidencias.push_back("La base sale de un CPK oficial del juego («" + phoenix::entrega::aU8(fa.ruta.filename()) + "»)."); }
			else b.parche = "parche desconocido";
			return b;
		}
		if (!leerFuente(x, fj, "Player.bin", crudoJug, e)) { b.error = "Player.bin de «" + fj.etiqueta + "» no se puede leer: " + e + "."; b.queHacer = "No se crea nada."; return b; }
		if (!leerAsignacion(crudoAsig, b.filas, e)) { b.error = "La base no tiene el formato esperado: " + e + "."; b.queHacer = "No se crea nada."; return b; }
		if (!leerJugadores(crudoJug, b.jugadores, e, necesitaNombres ? &b.nombres.jugador : nullptr)) { b.error = "La base no tiene el formato esperado: " + e + "."; b.queHacer = "No se crea nada."; return b; }
		b.asignacionSha = sha(crudoAsig);
		b.jugadoresSha = sha(crudoJug);
		b.evidencias.push_back("PlayerAssignment.bin: " + std::to_string(b.filas.size()) + " filas de 16 bytes; Player.bin: " + std::to_string(b.jugadores.size()) + " jugadores (cabecera " + std::to_string(le32(crudoJug, 0)) + ").");
		if (necesitaNombres) {
			const Fuente ft = resolver(x, "Team.bin");
			std::string crudoEq, e2;
			if (ft.hay && ft.error.empty() && leerFuente(x, ft, "Team.bin", crudoEq, e2) && leerEquipos(crudoEq, b.nombres.equipo, e2) && !b.nombres.jugador.empty()) b.nombresOk = true;
			else b.evidencias.push_back("No pude leer los nombres de los equipos (Team.bin): los cambios por nombre se saltarán.");
		}
		identificar(b, fa, x);
		b.ok = true;
		return b;
	}

	std::string leerEstadoSalida(const fs::path& sider, nlohmann::json& j) {
		std::string t;
		if (!leerArchivo(dePhoenixDB(sider) / "fichajes_aplicados.json", t, 1024 * 1024)) return std::string();
		j = nlohmann::json::parse(t, nullptr, false);
		if (j.is_discarded() || !j.is_object()) { j = nlohmann::json(); return std::string(); }
		return j.value("salidaSha", std::string());
	}

	bool cambiarNombre(const fs::path& de, const fs::path& a) { std::error_code ec; fs::rename(de, a, ec); return !ec; }
}

std::vector<Objetivo> objetivos(const fs::path& juego) {
	std::vector<Objetivo> v;
	if (juego.empty()) return v;
	std::error_code ec;
	if (fs::is_directory(juego / "SiderAddons", ec)) {
		Objetivo r;
		r.nombre = "Raíz del juego"; r.esRaiz = true;
		r.siderAddons = juego / "SiderAddons"; r.cpk = juego / "download" / "CGP_database.cpk"; r.ini = r.siderAddons / "sider.ini";
		v.push_back(r);
	}
	for (const fs::path& m : phoenix::modulos::carpetasModos(juego)) {
		Objetivo o;
		o.nombre = phoenix::entrega::aU8(m.filename());
		o.siderAddons = m / "SiderAddons"; o.cpk = m / "download" / "CGP_database.cpk";
		o.ini = fs::is_regular_file(o.siderAddons / "sider.ini", ec) ? o.siderAddons / "sider.ini" : juego / "SiderAddons" / "sider.ini";
		v.push_back(o);
	}
	return v;
}

namespace {
	std::vector<Analisis> analizarCon(Cache& cache, const fs::path& juego, const Lista& lista) {
		std::vector<Analisis> res;
		const bool nombres = lista.usaNombres();
		for (const Objetivo& o : objetivos(juego)) {
			Analisis a;
			a.objetivo = o;
			Contexto x(cache);
			x.juego = juego; x.o = o;
			const Base b = resolverBase(x, nombres);
			a.baseOk = b.ok; a.baseOrigen = b.origen; a.error = b.error; a.queHacer = b.queHacer;
			a.parche = b.parche; a.baseGenerica = b.generica; a.comoLoSupe = b.evidencias; a.nombresDisponibles = b.nombresOk;
			if (!b.ok) { a.estado = "sin_base"; res.push_back(std::move(a)); continue; }
			a.filasBase = static_cast<int>(b.filas.size());
			a.baseSha = b.asignacionSha;
			Nombres nm = b.nombres;
			Resultado r = aplicar(b.filas, b.jugadores, lista, b.nombresOk ? &nm : nullptr);
			a.aplicados = static_cast<int>(r.aplicados.size());
			a.yaEstaban = r.yaEstaban;
			a.hechos = r.aplicados;
			a.saltados = r.saltados;
			std::string e;
			if (!validar(b.filas, r.filas, e)) { a.baseOk = false; a.error = "El resultado no pasa la validación: " + e; a.queHacer = "No se crea nada."; a.estado = "sin_base"; res.push_back(std::move(a)); continue; }
			if (a.aplicados > 0) { a.salida = escribirBin(escribirAsignacion(r.filas)); a.salidaSha = sha(a.salida); }
			// ¿Phoenix-DB tiene un Player.bin que no es el de la base? Taparía a los jugadores del parche.
			std::string pb;
			if (leerArchivo(dePesdb(o.siderAddons) / "Player.bin", pb)) {
				std::string crudo, e2;
				a.conPlayerBinAjeno = !(leerBin(pb, crudo, e2) && sha(crudo) == b.jugadoresSha);
			}
			// Estado respecto a lo que Link generó antes
			nlohmann::json st;
			const std::string shaGuardada = leerEstadoSalida(o.siderAddons, st);
			std::string actual;
			const bool hayArchivo = leerArchivo(dePesdb(o.siderAddons) / "PlayerAssignment.bin", actual);
			if (shaGuardada.empty()) a.estado = a.aplicados > 0 ? "sin_aplicar" : "nada";
			else if (!hayArchivo || sha(actual) != shaGuardada) a.estado = "cambiado";
			else if (a.aplicados == 0) a.estado = "desfasado";
			else if (a.salidaSha == shaGuardada && st.value("baseSha", std::string()) == b.asignacionSha && st.value("listaSha", std::string()) == lista.sha256) a.estado = "aplicado";
			else a.estado = "desfasado";
			res.push_back(std::move(a));
		}
		return res;
	}
}

std::vector<Analisis> analizar(const fs::path& juego, const Lista& lista) {
	Cache cache;
	return analizarCon(cache, juego, lista);
}

namespace {
	struct Deshacer {
		fs::path destino;        // archivo escrito
		fs::path copia;          // copia del que había (vacío = no había)
		fs::path estado;         // fichajes_aplicados.json escrito
		fs::path playerOrigen, playerMovido;   // Player.bin apartado
		fs::path estadoCopia;
	};
	void deshacerTodo(std::vector<Deshacer>& hechos) {
		std::error_code ec;
		for (size_t i = hechos.size(); i-- > 0;) {
			Deshacer& d = hechos[i];
			if (!d.copia.empty()) { fs::copy_file(d.copia, d.destino, fs::copy_options::overwrite_existing, ec); }
			else fs::remove(d.destino, ec);
			if (!d.estadoCopia.empty()) fs::copy_file(d.estadoCopia, d.estado, fs::copy_options::overwrite_existing, ec);
			else if (!d.estado.empty()) fs::remove(d.estado, ec);
			if (!d.playerMovido.empty()) cambiarNombre(d.playerMovido, d.playerOrigen);
		}
		hechos.clear();
	}
}

Informe aplicarAlJuego(const fs::path& juego, const Lista& lista) {
	Informe inf;
	Cache cache;
	if (juego.empty()) { inf.mensaje = "No sé dónde está tu juego: no se tocó nada."; return inf; }
	if (lista.cambios.empty()) {
		inf.ok = true;
		inf.objetivos = analizarCon(cache, juego, lista);
		inf.mensaje = "La lista de fichajes está vacía: no hay nada que aplicar y no se tocó el juego.";
		return inf;
	}
	inf.objetivos = analizarCon(cache, juego, lista);
	// Qué sitios se escriben: con base reconocida y al menos un cambio aplicable. Si había algo nuestro y ya no aplica nada, se retira.
	std::vector<size_t> escribir, retirar;
	for (size_t i = 0; i < inf.objetivos.size(); i++) {
		const Analisis& a = inf.objetivos[i];
		if (!a.baseOk) continue;
		if (a.aplicados > 0) escribir.push_back(i);
		else if (a.estado == "desfasado") retirar.push_back(i);
	}
	if (escribir.empty() && retirar.empty()) {
		inf.ok = true;
		inf.mensaje = "Ningún fichaje de la lista se puede aplicar en tu juego (mira los motivos de cada modo). No se tocó nada.";
		return inf;
	}
	const std::string marca = sello();
	std::vector<Deshacer> hechos;
	std::error_code ec;
	auto fallar = [&](const std::string& msg) { deshacerTodo(hechos); inf.ok = false; inf.mensaje = msg + " Se dejó todo como estaba."; inf.escritos = 0; return inf; };

	for (size_t i : escribir) {
		const Analisis& a = inf.objetivos[i];
		const fs::path pesdb = dePesdb(a.objetivo.siderAddons), phx = dePhoenixDB(a.objetivo.siderAddons);
		const fs::path copiaDir = phx / ("_antes-de-fichajes-" + marca);
		fs::create_directories(pesdb, ec);
		if (ec) return fallar("No se pudo crear Phoenix-DB en " + a.objetivo.nombre + " (¿sin permisos?).");
		Deshacer d;
		d.destino = pesdb / "PlayerAssignment.bin";
		d.estado = phx / "fichajes_aplicados.json";
		// copias de lo que había
		if (fs::exists(d.destino, ec)) {
			fs::create_directories(copiaDir, ec);
			d.copia = copiaDir / "PlayerAssignment.bin";
			if (!fs::copy_file(d.destino, d.copia, fs::copy_options::overwrite_existing, ec)) return fallar("No se pudo guardar la copia de PlayerAssignment.bin en " + a.objetivo.nombre + ".");
			inf.respaldos.push_back(phoenix::entrega::aU8(d.copia));
		}
		if (fs::exists(d.estado, ec)) {
			fs::create_directories(copiaDir, ec);
			d.estadoCopia = copiaDir / "fichajes_aplicados.json";
			fs::copy_file(d.estado, d.estadoCopia, fs::copy_options::overwrite_existing, ec);
		}
		// Un Player.bin que no es el de la base taparía a los jugadores del parche: se aparta (no se borra)
		if (a.conPlayerBinAjeno) {
			fs::create_directories(copiaDir, ec);
			const fs::path destinoPlayer = copiaDir / "Player.bin";
			if (!cambiarNombre(pesdb / "Player.bin", destinoPlayer)) return fallar("No se pudo apartar el Player.bin de Phoenix-DB en " + a.objetivo.nombre + ".");
			d.playerOrigen = pesdb / "Player.bin"; d.playerMovido = destinoPlayer;
			inf.respaldos.push_back(phoenix::entrega::aU8(destinoPlayer));
		}
		hechos.push_back(d);
		std::string err;
		if (!phoenix::buzon::escribirAtomicoComo(pesdb, "PlayerAssignment", ".bin", a.salida, &err)) return fallar("No se pudo escribir PlayerAssignment.bin en " + a.objetivo.nombre + " (" + err + "; ¿PES abierto o el antivirus lo bloquea?).");
		// Comprobación: leerlo de nuevo y que sea lo esperado
		std::string leido, crudo, e2;
		if (!leerArchivo(d.destino, leido) || sha(leido) != a.salidaSha || !leerBin(leido, crudo, e2)) return fallar("Lo escrito en " + a.objetivo.nombre + " no quedó idéntico a lo previsto.");
		nlohmann::json st = {
			{"version", 1}, {"fecha", fechaLegible()}, {"modo", a.objetivo.nombre}, {"baseOrigen", a.baseOrigen}, {"baseSha", a.baseSha},
			{"listaSha", lista.sha256}, {"listaFuente", lista.fuente}, {"salidaSha", a.salidaSha},
			{"aplicados", a.aplicados}, {"saltados", a.saltados.size()}, {"yaEstaban", a.yaEstaban}
		};
		if (!phoenix::buzon::escribirAtomicoComo(phx, "fichajes_aplicados", ".json", st.dump(1), &err)) return fallar("No se pudo guardar el estado de los fichajes en " + a.objetivo.nombre + ".");
	}
	// Lo que ya no aplica nada: se retira lo que generó Link antes
	for (size_t i : retirar) {
		const Analisis& a = inf.objetivos[i];
		const fs::path pesdb = dePesdb(a.objetivo.siderAddons), phx = dePhoenixDB(a.objetivo.siderAddons);
		Deshacer d;
		d.destino = pesdb / "PlayerAssignment.bin"; d.estado = phx / "fichajes_aplicados.json";
		const fs::path copiaDir = phx / ("_antes-de-fichajes-" + marca);
		fs::create_directories(copiaDir, ec);
		d.copia = copiaDir / "PlayerAssignment.bin"; d.estadoCopia = copiaDir / "fichajes_aplicados.json";
		fs::copy_file(d.destino, d.copia, fs::copy_options::overwrite_existing, ec);
		fs::copy_file(d.estado, d.estadoCopia, fs::copy_options::overwrite_existing, ec);
		hechos.push_back(d);
		fs::remove(d.destino, ec); fs::remove(d.estado, ec);
	}
	// sider.ini: la raíz Phoenix-DB tiene que estar (una línea, con copia previa)
	std::set<fs::path> inis;
	for (size_t i : escribir) {
		const Objetivo& o = inf.objetivos[i].objetivo;
		const fs::path carpeta = o.ini.parent_path();
		inis.insert(carpeta);
	}
	if (!inis.empty()) {
		const phoenix::modulos::Resultado r = phoenix::modulos::asegurarRaizPhoenixDB(std::vector<fs::path>(inis.begin(), inis.end()));
		if (!r.ok) return fallar(r.mensaje);
		for (const std::string& x : r.respaldos) inf.respaldos.push_back(x);
	}
	inf.escritos = static_cast<int>(escribir.size());
	inf.ok = true;
	int aplicados = 0, saltados = 0;
	for (size_t i : escribir) { aplicados += inf.objetivos[i].aplicados; saltados += static_cast<int>(inf.objetivos[i].saltados.size()); }
	inf.mensaje = "Fichajes generados sobre tu base en " + std::to_string(escribir.size()) + (escribir.size() == 1 ? " sitio: " : " sitios: ") + std::to_string(aplicados) + " aplicados, "
		+ std::to_string(saltados) + " saltados. Abre PES: se ven al cargar los datos del juego.";
	// El análisis ya refleja lo escrito
	inf.objetivos = analizarCon(cache = Cache(), juego, lista);
	return inf;
}

Informe quitarDelJuego(const fs::path& juego) {
	Informe inf;
	if (juego.empty()) { inf.mensaje = "No sé dónde está tu juego: no se tocó nada."; return inf; }
	std::error_code ec;
	int quitados = 0, ajenos = 0;
	std::vector<std::string> notas;
	const std::string marca = sello();
	for (const Objetivo& o : objetivos(juego)) {
		nlohmann::json st;
		const std::string guardada = leerEstadoSalida(o.siderAddons, st);
		if (guardada.empty()) continue;
		const fs::path pesdb = dePesdb(o.siderAddons), phx = dePhoenixDB(o.siderAddons);
		std::string actual;
		if (leerArchivo(pesdb / "PlayerAssignment.bin", actual) && sha(actual) != guardada) {
			ajenos++;
			notas.push_back(o.nombre + ": el PlayerAssignment.bin ya no es el que generó Link (lo cambió otra herramienta): no se borra");
			continue;
		}
		const fs::path copiaDir = phx / ("_antes-de-fichajes-" + marca);
		fs::create_directories(copiaDir, ec);
		if (fs::exists(pesdb / "PlayerAssignment.bin", ec)) {
			if (!cambiarNombre(pesdb / "PlayerAssignment.bin", copiaDir / "PlayerAssignment.bin")) { inf.mensaje = "No se pudo quitar PlayerAssignment.bin en " + o.nombre + " (¿PES abierto?)."; return inf; }
			inf.respaldos.push_back(phoenix::entrega::aU8(copiaDir / "PlayerAssignment.bin"));
		}
		cambiarNombre(phx / "fichajes_aplicados.json", copiaDir / "fichajes_aplicados.json");
		quitados++;
	}
	inf.ok = true;
	if (quitados == 0 && ajenos == 0) inf.mensaje = "No había fichajes generados por Link: tu juego ya usa la base original del parche.";
	else {
		inf.mensaje = "Fichajes quitados en " + std::to_string(quitados) + (quitados == 1 ? " sitio" : " sitios") + ": el juego vuelve a la base original del parche (la copia queda en una carpeta _antes-de-fichajes-… dentro de Phoenix-DB).";
		for (const std::string& n : notas) inf.mensaje += " " + n + ".";
	}
	return inf;
}

// ─── lista de cambios del usuario / del paquete ──────────────────────────────────────────────────
fs::path carpetaListaUsuario() {
	const char* ap = std::getenv("APPDATA");
	if (!ap || !*ap) return {};
	return fs::path(ap) / "Phoenix Mercado" / "fichajes";
}

fs::path carpetaListaPaquete() {
#ifdef _WIN32
	wchar_t ruta[MAX_PATH * 2];
	const DWORD n = GetModuleFileNameW(nullptr, ruta, static_cast<DWORD>(sizeof(ruta) / sizeof(ruta[0])));
	if (n == 0 || n >= sizeof(ruta) / sizeof(ruta[0])) return {};
	return fs::path(std::wstring(ruta, n)).parent_path() / L"fichajes";
#else
	return fs::path("fichajes");
#endif
}

bool cargarLista(const fs::path& usuario, const fs::path& paquete, Lista& lista, std::string& error) {
	lista = Lista();
	std::error_code ec;
	for (const fs::path& dir : { usuario, paquete }) {
		if (dir.empty()) continue;
		const fs::path f = dir / "fichajes.json";
		if (!fs::is_regular_file(f, ec)) continue;
		std::string t;
		if (!leerArchivo(f, t, 2 * 1024 * 1024)) { error = "No se pudo leer " + phoenix::entrega::aU8(f); return false; }
		if (t.compare(0, 3, "\xEF\xBB\xBF") == 0) t.erase(0, 3);
		if (!leerLista(t, lista, error)) { error += " (" + phoenix::entrega::aU8(f) + ")"; return false; }
		lista.fuente = phoenix::entrega::aU8(f);
		return true;
	}
	lista.fuente = "(no hay lista de fichajes)";
	return true;
}

} // namespace phoenix::fichajes
