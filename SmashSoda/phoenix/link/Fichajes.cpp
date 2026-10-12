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
		if (ncol > 200 || nrows > 200000) { error = "tabla del CPK demasiado grande"; return false; }
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

bool extraerDeCpk(const std::string& d, const std::string& carpeta, const std::string& nombre, std::string& bytes, std::string& error) {
	if (d.size() < 64 || d.compare(0, 4, "CPK ") != 0) { error = "no es un archivo CPK"; return false; }
	std::vector<FilaUtf> cab;
	if (!leerUtf(d, 16, cab, error)) return false;
	if (cab.empty() || !cab[0].count("TocOffset") || !cab[0].count("ContentOffset")) { error = "CPK sin tabla de archivos"; return false; }
	const unsigned long long toc = static_cast<unsigned long long>(cab[0]["TocOffset"].n), contenido = static_cast<unsigned long long>(cab[0]["ContentOffset"].n);
	if (toc + 16 >= d.size() || d.compare(static_cast<size_t>(toc), 4, "TOC ") != 0) { error = "CPK: tabla de archivos no encontrada"; return false; }
	std::vector<FilaUtf> filas;
	if (!leerUtf(d, static_cast<size_t>(toc) + 16, filas, error)) return false;
	for (FilaUtf& f : filas) {
		if (f["FileName"].s != nombre || f["DirName"].s != carpeta) continue;
		const long long tam = f["FileSize"].n, ext = f["ExtractSize"].n, off = f["FileOffset"].n;
		if (tam != ext) { error = "el CPK guarda " + nombre + " comprimido (no soportado)"; return false; }
		const unsigned long long ini = contenido + static_cast<unsigned long long>(off);
		if (tam <= 0 || ini + static_cast<unsigned long long>(tam) > d.size()) { error = "CPK: " + nombre + " fuera de rango"; return false; }
		bytes = d.substr(static_cast<size_t>(ini), static_cast<size_t>(tam));
		return true;
	}
	error = "el CPK no tiene " + carpeta + "/" + nombre;
	return false;
}

// ─── PlayerAssignment / Player ───────────────────────────────────────────────────────────────────
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

bool leerJugadores(const std::string& crudo, std::set<uint32_t>& ids, std::string& error) {
	ids.clear();
	// cabecera de 8 bytes + registros de 312 bytes; el archivo real termina con el último registro recortado (8 bytes menos)
	if (crudo.size() < 8 + 312 * 1000 || ((crudo.size() - 8) % 312 != 0 && (crudo.size() - 8) % 312 != 304)) { error = "Player.bin con un tamaño que no es de PES 2021"; return false; }
	for (size_t o = 8; o + 4 <= crudo.size(); o += 312) { const uint32_t id = le32(crudo, o); if (id) ids.insert(id); }
	if (ids.size() < 1000) { error = "Player.bin con muy pocos jugadores"; return false; }
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
	if (lista.version != kVersionLista) { error = "fichajes.json: versión " + std::to_string(lista.version) + " no soportada (esta Link entiende la " + std::to_string(kVersionLista) + ")"; return false; }
	if (j.contains("regla")) {
		const std::string r = j["regla"].is_string() ? j["regla"].get<std::string>() : std::string();
		if (r == "estricta") lista.estricta = true;
		else if (r == "relajada") lista.estricta = false;
		else { error = "fichajes.json: «regla» debe ser «relajada» o «estricta»"; return false; }
	}
	if (j.contains("nombre") && j["nombre"].is_string()) lista.nombre = j["nombre"].get<std::string>().substr(0, 80);
	if (!j.contains("cambios") || !j["cambios"].is_array()) { error = "fichajes.json: falta la lista «cambios»"; return false; }
	if (j["cambios"].size() > kMaxCambios) { error = "fichajes.json: demasiados cambios"; return false; }
	auto id = [](const nlohmann::json& o, const char* k, bool obligatorio, uint32_t& out) {
		if (!o.contains(k)) return !obligatorio;
		if (!o[k].is_number_integer()) return false;
		const long long v = o[k].get<long long>();
		if (v < 0 || v > kMaxId || (obligatorio && v == 0)) return false;
		out = static_cast<uint32_t>(v);
		return true;
	};
	for (size_t i = 0; i < j["cambios"].size(); i++) {
		const nlohmann::json& o = j["cambios"][i];
		Cambio c;
		if (!o.is_object() || !id(o, "player_id", true, c.jugador) || !id(o, "team_id", true, c.equipo) || !id(o, "origen", false, c.origen)) {
			error = "fichajes.json: el cambio " + std::to_string(i + 1) + " no tiene player_id y team_id numéricos válidos";
			return false;
		}
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
}

Resultado aplicar(const std::vector<Fila>& base, const std::set<uint32_t>& jugadores, const Lista& lista) {
	Resultado r;
	r.filas = base;
	std::set<uint32_t> equipos;
	for (const Fila& f : base) equipos.insert(f.equipo);
	uint32_t maxId = 0;
	for (const Fila& f : base) maxId = (std::max)(maxId, f.id);
	for (const Cambio& c : lista.cambios) {
		auto salto = [&](const char* cod, const std::string& mot) { r.saltados.push_back({ c, cod, mot }); };
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
			if (suyas.size() > 1) { salto("ambiguo", "Tiene varios equipos: indica el equipo de origen (campo «origen»)."); continue; }
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

// ─── juego real ──────────────────────────────────────────────────────────────────────────────────
namespace {
	const char* const kCarpetaPesdb = "common/etc/pesdb";

	fs::path dePesdb(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb"; }
	fs::path dePhoenixDB(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB"; }

	std::string minus(std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); return s; }

	/// ".\livecpk\X" → livecpk/X
	fs::path rutaRaiz(const std::string& r) {
		std::string x = r;
		std::replace(x.begin(), x.end(), '\\', '/');
		while (x.compare(0, 2, "./") == 0) x.erase(0, 2);
		return phoenix::entrega::deU8(x);
	}
	bool esPhoenixDB(const std::string& r) { std::string m = minus(r); std::replace(m.begin(), m.end(), '/', '\\'); return m.size() >= 10 && m.compare(m.size() - 10, 10, "phoenix-db") == 0; }

	struct Base {
		bool ok = false;
		std::string origen, error, filaSha;
		std::string asignacionCrudo, jugadoresCrudoSha;
		std::vector<Fila> filas;
		std::set<uint32_t> jugadores;
	};

	/// Caché de lecturas pesadas dentro de una misma pasada (varios modos comparten la misma base).
	struct Cache {
		std::map<std::string, std::pair<bool, std::string>> crudos;   // ruta|nombre → (ok, crudo o error)
	};

	bool crudoDeCarpeta(Cache& c, const fs::path& dir, const char* nombre, std::string& crudo, std::string& error) {
		const std::string clave = phoenix::entrega::aU8(dir) + "|" + nombre;
		auto it = c.crudos.find(clave);
		if (it == c.crudos.end()) {
			std::string bytes, e, out;
			bool ok = leerArchivo(dir / nombre, bytes);
			if (!ok) e = std::string("no se pudo leer ") + nombre;
			else ok = leerBin(bytes, out, e);
			it = c.crudos.emplace(clave, std::make_pair(ok, ok ? out : e)).first;
		}
		if (!it->second.first) { error = it->second.second; return false; }
		crudo = it->second.second;
		return true;
	}
	bool crudoDeCpk(Cache& c, const fs::path& cpk, const char* nombre, std::string& crudo, std::string& error) {
		const std::string clave = phoenix::entrega::aU8(cpk) + "|cpk|" + nombre;
		auto it = c.crudos.find(clave);
		if (it == c.crudos.end()) {
			std::string cpkBytes, bytes, e, out;
			bool ok = leerArchivo(cpk, cpkBytes);
			if (!ok) e = "no se pudo leer " + phoenix::entrega::aU8(cpk.filename());
			else ok = extraerDeCpk(cpkBytes, kCarpetaPesdb, nombre, bytes, e) && leerBin(bytes, out, e);
			it = c.crudos.emplace(clave, std::make_pair(ok, ok ? out : e)).first;
		}
		if (!it->second.first) { error = it->second.second; return false; }
		crudo = it->second.second;
		return true;
	}

	/// La base que usa el juego en este sitio: la primera cpk.root (sin Phoenix-DB) que trae PlayerAssignment.bin; si no hay, el CPK del modo.
	Base resolverBase(Cache& cache, const fs::path& juego, const Objetivo& o) {
		Base b;
		std::string ini;
		leerArchivo(o.ini, ini, 512 * 1024);
		fs::path dirPesdb;
		std::error_code ec;
		for (const std::string& r : phoenix::modulos::raicesActivas(ini)) {
			if (esPhoenixDB(r)) continue;
			const fs::path rel = rutaRaiz(r);
			for (const fs::path& sider : { o.siderAddons, juego / "SiderAddons" }) {
				const fs::path cand = sider / rel / "common" / "etc" / "pesdb";
				if (fs::is_regular_file(cand / "PlayerAssignment.bin", ec)) { dirPesdb = cand; b.origen = phoenix::entrega::aU8(rel) + (sider == o.siderAddons ? "" : " (de la raíz del juego)"); break; }
			}
			if (!dirPesdb.empty()) break;
		}
		std::string crudoPlayer;
		if (!dirPesdb.empty()) {
			if (!crudoDeCarpeta(cache, dirPesdb, "PlayerAssignment.bin", b.asignacionCrudo, b.error)) { b.error = "No se reconoce la base de " + b.origen + ": " + b.error; return b; }
			if (!crudoDeCarpeta(cache, dirPesdb, "Player.bin", crudoPlayer, b.error)) {
				// esa carpeta solo trae PlayerAssignment: los jugadores se leen del CPK
				std::string e2;
				if (!crudoDeCpk(cache, o.cpk, "Player.bin", crudoPlayer, e2)) { b.error = "No se pueden leer los jugadores de la base: " + e2; return b; }
				b.error.clear();
			}
		}
		else {
			fs::path cpk = o.cpk;
			if (!fs::is_regular_file(cpk, ec)) cpk = juego / "download" / "CGP_database.cpk";
			b.origen = "CGP_database.cpk";
			if (!crudoDeCpk(cache, cpk, "PlayerAssignment.bin", b.asignacionCrudo, b.error) || !crudoDeCpk(cache, cpk, "Player.bin", crudoPlayer, b.error)) { b.error = "No se reconoce la base (" + b.error + ")"; return b; }
		}
		if (!leerAsignacion(b.asignacionCrudo, b.filas, b.error) || !leerJugadores(crudoPlayer, b.jugadores, b.error)) { b.error = "No se reconoce la base: " + b.error; return b; }
		b.filaSha = sha(b.asignacionCrudo);
		b.jugadoresCrudoSha = sha(crudoPlayer);
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
		for (const Objetivo& o : objetivos(juego)) {
			Analisis a;
			a.objetivo = o;
			const Base b = resolverBase(cache, juego, o);
			a.baseOk = b.ok; a.baseOrigen = b.origen; a.error = b.error;
			if (!b.ok) { a.estado = "sin_base"; res.push_back(std::move(a)); continue; }
			a.filasBase = static_cast<int>(b.filas.size());
			a.baseSha = b.filaSha;
			Resultado r = aplicar(b.filas, b.jugadores, lista);
			a.aplicados = static_cast<int>(r.aplicados.size());
			a.yaEstaban = r.yaEstaban;
			a.hechos = r.aplicados;
			a.saltados = r.saltados;
			std::string e;
			if (!validar(b.filas, r.filas, e)) { a.baseOk = false; a.error = "El resultado no pasa la validación: " + e; a.estado = "sin_base"; res.push_back(std::move(a)); continue; }
			if (a.aplicados > 0) { a.salida = escribirBin(escribirAsignacion(r.filas)); a.salidaSha = sha(a.salida); }
			// ¿Phoenix-DB tiene un Player.bin que no es el de la base? Taparía a los jugadores del parche.
			std::string pb;
			if (leerArchivo(dePesdb(o.siderAddons) / "Player.bin", pb)) {
				std::string crudo, e2;
				a.conPlayerBinAjeno = !(leerBin(pb, crudo, e2) && sha(crudo) == b.jugadoresCrudoSha);
			}
			// Estado respecto a lo que Link generó antes
			nlohmann::json st;
			const std::string shaGuardada = leerEstadoSalida(o.siderAddons, st);
			std::string actual;
			const bool hayArchivo = leerArchivo(dePesdb(o.siderAddons) / "PlayerAssignment.bin", actual);
			if (shaGuardada.empty()) a.estado = a.aplicados > 0 ? "sin_aplicar" : "nada";
			else if (!hayArchivo || sha(actual) != shaGuardada) a.estado = "cambiado";
			else if (a.aplicados == 0) a.estado = "desfasado";
			else if (a.salidaSha == shaGuardada && st.value("baseSha", std::string()) == b.filaSha && st.value("listaSha", std::string()) == lista.sha256) a.estado = "aplicado";
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
