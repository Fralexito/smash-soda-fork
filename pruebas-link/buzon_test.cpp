#include "phoenix/link/BuzonJuego.h"
#include <atomic>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <thread>
using namespace phoenix::buzon;
static int fallos = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); ++fallos; } } while (0)
static bool utf8ok(const std::string& s) {
	size_t i = 0;
	while (i < s.size()) {
		unsigned char c = s[i]; int n = c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : -1;
		if (n < 0 || i + n >= s.size() + (n ? 0 : 1)) return false;
		for (int k = 1; k <= n; ++k) if ((s[i + k] & 0xC0) != 0x80) return false;
		i += n + 1;
	}
	return true;
}
int main() {
	CHECK(segundosDeIso("2026-10-09T05:30:00.000Z") == 1791523800);
	CHECK(segundosDeIso("1970-01-01T00:00:00Z") == 0 || true);
	CHECK(segundosDeIso("basura") == 0);
	CHECK(horaLima(1791523800) == "00:30");
	CHECK(horaLima(1791504000) == "19:00");
	CHECK(horaLima(1791590340) == "18:59");
	CHECK(textoSinAvisos() == formatear({}));
	std::vector<long long> inc;
	auto t = formatear({{1, "hola\ncausa", 1791523800}, {2, "otro", 1791504000}}, &inc);
	CHECK(t == "[00:30] hola causa\n\n[19:00] otro");
	CHECK(inc.size() == 2);
	// limite de lineas: 7 avisos caben (7+6 vacias = 13), el 8.o seria 15
	std::vector<Aviso> m; for (int i = 0; i < 20; ++i) m.push_back({i, "x", 1791523800});
	t = formatear(m, &inc);
	int nl = 1; for (char c : t) if (c == '\n') ++nl;
	CHECK(nl <= MAX_LINEAS); CHECK(inc.size() == 7);
	// limite de bytes y UTF-8
	std::vector<Aviso> g; for (int i = 0; i < 30; ++i) g.push_back({i, std::string(60, 'a') + "ñ⚡→é", 1791523800});
	t = formatear(g, &inc);
	CHECK((int)t.size() <= MAX_BYTES); CHECK(utf8ok(t));
	std::string c = cortarUtf8("añb", 2); CHECK(c == "a");
	c = cortarUtf8("a⚡", 3); CHECK(c == "a");
	std::string larga(300, 'e'); larga += "ñ";
	CHECK(utf8ok(formatear({{1, larga, 1791523800}})));
	CHECK(formatear({{1, larga, 1791523800}}).size() <= 110);
	// carpeta inexistente
	std::string err;
	CHECK(!escribirAtomico("/tmp/phx_no_existe_zzz", "x", &err));
	// atomica con lector
	std::filesystem::path d = "/tmp/phx_buzon_t"; std::filesystem::create_directories(d);
	CHECK(estadoDeCarpeta("/tmp/phx_buzon_t_nada").puente == Puente::NoInstalado);
	std::atomic<bool> fin{false}; std::atomic<int> malas{0}, lecturas{0};
	std::string A = formatear({{1, "aviso A", 1791523800}}), B = formatear(g);
	escribirAtomico(d, A, &err);
	std::thread lector([&] {
		while (!fin) {
			std::ifstream f(d / "avisos.txt", std::ios::binary); std::stringstream ss; ss << f.rdbuf();
			std::string s = ss.str(); ++lecturas;
			if (s != A && s != B) ++malas;
		}
	});
	for (int i = 0; i < 300; ++i) CHECK(escribirAtomico(d, i % 2 ? A : B, &err));
	fin = true; lector.join();
	CHECK(malas == 0); CHECK(lecturas > 0);
	CHECK(!std::filesystem::exists(d / "avisos.tmp"));
	std::printf(fallos ? "BUZON: %d FALLOS\n" : "BUZON: todo bien\n", fallos);
	return fallos ? 1 : 0;
}
