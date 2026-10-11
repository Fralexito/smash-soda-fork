// Analisis sin conexion de hist.bin (lo escribe escaner.exe). Busca arrays con paso constante de 22+ entradas
// (tolera huecos) cuyos valores parecen coordenadas de campo. Uso: analiza hist.bin out.txt [maxAbs]
#define NOMINMAX
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <intrin.h>

struct C { uint64_t a; float v; uint64_t h; };
int main(int argc, char** argv) {
	if (argc < 3) return 1;
	float maxAbs = argc > 3 ? (float)atof(argv[3]) : 120.f;
	std::vector<int> ticks;
	{ FILE* f = fopen(argv[2], "r"); char l[512]; while (f && fgets(l, sizeof l, f)) { int t, n; if (sscanf(l, "t%d=%d", &t, &n) == 2) { if ((int)ticks.size() <= t) ticks.resize(t + 1); ticks[t] = n; } } if (f) fclose(f); }
	int mx = 0; for (int n : ticks) mx = std::max(mx, n);
	uint64_t activos = 0, parados = 0; int na = 0;
	for (size_t t = 0; t < ticks.size() && t < 64; t++) { if (ticks[t] > mx * 0.4) { activos |= 1ULL << t; na++; } else if (ticks[t] < mx * 0.005) parados |= 1ULL << t; }
	std::vector<C> v;
	FILE* f = fopen(argv[1], "rb"); C c;
	while (fread(&c.a, 8, 1, f) == 1 && fread(&c.v, 4, 1, f) == 1 && fread(&c.h, 8, 1, f) == 1) {
		if (c.h & parados) continue;
		if ((int)__popcnt64(c.h & activos) < std::max(2, na / 2)) continue;
		if (std::fabs(c.v) > maxAbs) continue;
		v.push_back(c);
	}
	fclose(f);
	std::sort(v.begin(), v.end(), [](const C& x, const C& y) { return x.a < y.a; });
	printf("candidatos tras filtro: %zu (activos=%d)\n", v.size(), na); fflush(stdout);
	std::unordered_map<uint64_t, float> mapa; mapa.reserve(v.size() * 2);
	for (auto& x : v) mapa[x.a] = x.v;
	std::unordered_map<uint64_t, size_t> hist;
	for (size_t i = 0; i < v.size(); i++) for (size_t k = 1; k <= 12 && i + k < v.size(); k++) { uint64_t d = v[i + k].a - v[i].a; if (d <= 0x2000) hist[d]++; }
	std::vector<std::pair<size_t, uint64_t>> top; for (auto& h : hist) top.push_back({ h.second, h.first });
	std::sort(top.rbegin(), top.rend()); if (top.size() > 40) top.resize(40);
	struct R { double score; std::string s; };
	std::vector<R> res;
	for (auto& t : top) {
		uint64_t S = t.second;
		for (auto& x : v) {
			if (mapa.count(x.a - S)) continue;
			int hits = 0, miss = 0; bool ok = true; double s = 0, s2 = 0; int ns = 0; float mn = 1e9, mxv = -1e9;
			std::string vals;
			for (int k = 0; k < 22; k++) {
				auto it = mapa.find(x.a + k * S);
				if (it == mapa.end()) { if (++miss > 5) { ok = false; break; } vals += " _"; continue; }
				hits++; float y = it->second; s += y; s2 += (double)y * y; ns++; mn = std::min(mn, y); mxv = std::max(mxv, y);
				char b[24]; snprintf(b, sizeof b, " %.1f", y); vals += b;
			}
			if (!ok || hits < 17) continue;
			double mean = s / ns, var = s2 / ns - mean * mean;
			if (var < 36 || mxv - mn < 30) continue; // desviacion >= 6 m y rango >= 30 m
			char h[96]; snprintf(h, sizeof h, "S=%llu inicio=%llX hits=%d sd=%.1f:", (unsigned long long)S, (unsigned long long)x.a, hits, std::sqrt(var));
			res.push_back({ std::sqrt(var) * hits, std::string(h) + vals });
		}
	}
	std::sort(res.begin(), res.end(), [](const R& a, const R& b) { return a.score > b.score; });
	FILE* o = fopen("rank.txt", "w");
	for (auto& r : res) fprintf(o, "%s\n", r.s.c_str());
	fclose(o);
	printf("cadenas: %zu (rank.txt)\n", res.size());
}
