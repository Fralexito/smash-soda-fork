// ¿Cuál de las dos listas de jugadores del juego es la exacta? Solo lectura de PES2021.exe.
// Idea: el que lleva el balón está pegado a él. Se lee a ritmo máximo durante N segundos las dos listas de jugadores y el
// balón, y para cada lista se mide, en los momentos con el balón «rodando junto a un jugador» (velocidad < 8 m/s):
//   · distancia media del balón al jugador MÁS CERCANO de esa lista (menor = lista más exacta)
//   · en qué fracción de esos momentos cada lista tiene al jugador más cerca
//   · desfase medio entre listas con signo (dx, dz) y proyectado sobre la velocidad del jugador (¿una lista va «por delante»?)
// Uso: medir_balon [segundos=30]   (con el partido en juego)
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <timeapi.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <chrono>
#include <map>
#include <vector>
#include <algorithm>
#include "phoenix/core/CampoVivo.h"
#pragma comment(lib, "winmm.lib")
using namespace phoenix::campo;
using Reloj = std::chrono::steady_clock;

struct Foto { double t; std::vector<float> x, z; float bx = 0, bz = 0, by = 0; bool bola = false; };

int main(int argc, char** argv) {
	const int segundos = argc > 1 ? std::atoi(argv[1]) : 30;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); if (!pid) { std::printf("PES2021 no esta abierto\n"); return 1; }
	HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	timeBeginPeriod(1);
	std::vector<Actor> campo, balones;
	{ const size_t trozo = 1u << 20; std::vector<uint8_t> buf(trozo + kBytesObjeto); uint64_t a = 0; MEMORY_BASIC_INFORMATION m;
	  while (VirtualQueryEx(h, (LPCVOID)a, &m, sizeof m)) {
		const uint64_t base = (uint64_t)m.BaseAddress, fin = base + m.RegionSize;
		const bool ok = m.State == MEM_COMMIT && m.Type == MEM_PRIVATE && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
		if (ok) for (uint64_t p = base; p < fin; p += trozo) {
			SIZE_T q = (SIZE_T)std::min<uint64_t>(trozo + kBytesObjeto, fin - p), g = 0;
			if (!ReadProcessMemory(h, (LPCVOID)p, buf.data(), q, &g) || g < kBytesObjeto) continue;
			for (SIZE_T i = 0; i + kBytesObjeto <= g && i < trozo; i += 16) {
				Actor x;
				if (leerActor(buf.data() + i, x)) { x.dir = p + i; campo.push_back(x); }
				else if (leerBalon(buf.data() + i, x)) { x.dir = p + i; balones.push_back(x); }
			}
		}
		a = base + m.RegionSize; } }
	std::map<unsigned, std::vector<size_t>> listas;   // código → índices en «campo»
	for (size_t i = 0; i < campo.size(); i++) listas[campo[i].codigo].push_back(i);
	std::printf("jugadores: %zu en %zu listas; balones candidatos: %zu\n", campo.size(), listas.size(), balones.size());
	if (listas.size() < 2 || balones.empty()) { std::printf("sin partido en juego\n"); return 3; }
	auto itL = listas.begin(); const unsigned cA = itL->first; const std::vector<size_t> ia = itL->second; ++itL; const unsigned cB = itL->first; const std::vector<size_t> ib = itL->second;
	std::vector<Foto> fotos;
	const auto t0 = Reloj::now();
	while (std::chrono::duration<double>(Reloj::now() - t0).count() < segundos) {
		Foto f; f.t = std::chrono::duration<double>(Reloj::now() - t0).count(); f.x.assign(campo.size(), 1e9f); f.z.assign(campo.size(), 1e9f);
		for (size_t i = 0; i < campo.size(); i++) { uint8_t b[16]; SIZE_T g = 0; float x, y, z; if (ReadProcessMemory(h, (LPCVOID)(campo[i].dir + kOffPosicion), b, 16, &g) && g == 16 && leerPosicion(b, x, y, z)) { f.x[i] = x; f.z[i] = z; } }
		// el balón del partido: el primer candidato válido dentro del campo (los demás están aparcados o parados fuera)
		for (const Actor& bb : balones) { uint8_t b[16]; SIZE_T g = 0; float x, y, z; if (ReadProcessMemory(h, (LPCVOID)(bb.dir + kOffPosicion), b, 16, &g) && g == 16 && leerPosicionBalon(b, x, y, z) && std::fabs(z) <= 40.0f && std::fabs(x) <= 55.0f) { f.bx = x; f.bz = z; f.by = y; f.bola = true; break; } }
		fotos.push_back(std::move(f));
		Sleep(2);
	}
	std::printf("fotos: %zu (%.0f/s)\n", fotos.size(), fotos.size() / (double)segundos);
	// por foto: velocidad del balón con la foto anterior; distancia del balón al más cercano de cada lista
	double sumaA = 0, sumaB = 0; int n = 0, cercaA = 0, cercaB = 0; std::vector<double> vA, vB;
	for (size_t k = 1; k < fotos.size(); k++) {
		const Foto& f = fotos[k]; const Foto& p = fotos[k - 1];
		if (!f.bola || !p.bola) continue;
		const double dt = f.t - p.t; if (dt <= 0 || dt > 0.2) continue;
		const double vb = std::hypot(f.bx - p.bx, f.bz - p.bz) / dt;
		if (vb > 8.0 || f.by > 1.2f) continue;   // balón lento y a ras de suelo: va con alguien
		double dA = 1e9, dB = 1e9;
		for (size_t i : ia) if (f.x[i] < 1e8f) dA = std::min<double>(dA, std::hypot(f.x[i] - f.bx, f.z[i] - f.bz));
		for (size_t i : ib) if (f.x[i] < 1e8f) dB = std::min<double>(dB, std::hypot(f.x[i] - f.bx, f.z[i] - f.bz));
		if (std::min(dA, dB) > 2.5) continue;   // nadie lo lleva
		sumaA += dA; sumaB += dB; n++; if (dA < dB) cercaA++; else cercaB++;
		if (std::min(dA, dB) < 2.0) { vA.push_back(dA); vB.push_back(dB); }
	}
	std::printf("momentos con el balon pegado a alguien: %d\n", n);
	if (n) std::printf("distancia media balon-jugador mas cercano: lista %u = %.3f m, lista %u = %.3f m; la %u esta mas cerca el %.0f%% de las veces\n", cA, sumaA / n, cB, sumaB / n, cA, 100.0 * cercaA / n);
	{ auto pct = [](std::vector<double> v, double p) { if (v.empty()) return 0.0; std::sort(v.begin(), v.end()); return v[(size_t)(p * (v.size() - 1))]; };
	  std::printf("con el balon a < 2 m del mas cercano (%zu momentos): percentiles 10/25/50/75 de la distancia: lista %u = %.2f/%.2f/%.2f/%.2f m; lista %u = %.2f/%.2f/%.2f/%.2f m\n", vA.size(), cA, pct(vA,0.1), pct(vA,0.25), pct(vA,0.5), pct(vA,0.75), cB, pct(vB,0.1), pct(vB,0.25), pct(vB,0.5), pct(vB,0.75)); }
	// gemelos: cada jugador existe en las dos listas como objetos contiguos (a ± 0x280 bytes); ¿mejora la MEDIA de los dos?
	{
		std::map<uint64_t, size_t> porDir; for (size_t i = 0; i < campo.size(); i++) porDir[campo[i].dir] = i;
		std::vector<std::pair<size_t, size_t>> gem;
		for (size_t i : ia) for (int64_t off : { 0x280LL, -0x280LL }) { auto it = porDir.find(campo[i].dir + off); if (it != porDir.end() && campo[it->second].codigo == cB) { gem.push_back({ i, it->second }); break; } }
		std::printf("gemelos encontrados por direccion contigua: %zu\n", gem.size());
		if (gem.size() >= 10) {
			std::vector<double> gA, gB, gM; double dif = 0; long nd = 0;
			for (size_t k = 1; k < fotos.size(); k++) {
				const Foto& f = fotos[k]; const Foto& p = fotos[k - 1]; if (!f.bola || !p.bola) continue;
				const double dt = f.t - p.t; if (dt <= 0 || dt > 0.2) continue;
				if (std::hypot(f.bx - p.bx, f.bz - p.bz) / dt > 8.0 || f.by > 1.2f) continue;
				double mA = 1e9, mB = 1e9, mM = 1e9;
				for (auto& g : gem) { if (f.x[g.first] > 1e8f || f.x[g.second] > 1e8f) continue;
					mA = std::min<double>(mA, std::hypot(f.x[g.first] - f.bx, f.z[g.first] - f.bz)); mB = std::min<double>(mB, std::hypot(f.x[g.second] - f.bx, f.z[g.second] - f.bz));
					mM = std::min<double>(mM, std::hypot((f.x[g.first] + f.x[g.second]) / 2 - f.bx, (f.z[g.first] + f.z[g.second]) / 2 - f.bz));
					dif += std::hypot(f.x[g.first] - f.x[g.second], f.z[g.first] - f.z[g.second]); nd++; }
				if (std::min(mA, std::min(mB, mM)) < 2.0) { gA.push_back(mA); gB.push_back(mB); gM.push_back(mM); }
			}
			auto pct = [](std::vector<double> v, double p) { if (v.empty()) return 0.0; std::sort(v.begin(), v.end()); return v[(size_t)(p * (v.size() - 1))]; };
			std::printf("diferencia media entre gemelos: %.3f m\n", nd ? dif / nd : 0.0);
			std::printf("(%zu momentos) distancia balon-jugador mas cercano, percentiles 10/25/50/75: lista %u %.2f/%.2f/%.2f/%.2f | lista %u %.2f/%.2f/%.2f/%.2f | MEDIA %.2f/%.2f/%.2f/%.2f m\n", gA.size(), cA, pct(gA,0.1), pct(gA,0.25), pct(gA,0.5), pct(gA,0.75), cB, pct(gB,0.1), pct(gB,0.25), pct(gB,0.5), pct(gB,0.75), pct(gM,0.1), pct(gM,0.25), pct(gM,0.5), pct(gM,0.75));
		}
	}
	// desfase con signo entre listas (empareja por cercanía en la foto media) y proyección sobre la velocidad
	if (fotos.size() > 10) {
		const Foto& m = fotos[fotos.size() / 2]; std::vector<std::pair<size_t, size_t>> par;
		for (size_t i : ia) { if (m.x[i] > 1e8f) continue; double mejor = 1e9; size_t mj = 0; for (size_t j : ib) { if (m.x[j] > 1e8f) continue; double d = std::hypot(m.x[i] - m.x[j], m.z[i] - m.z[j]); if (d < mejor) { mejor = d; mj = j; } } if (mejor < 3.0) par.push_back({ i, mj }); }
		double sdx = 0, sdz = 0, sdx2 = 0, sdz2 = 0, sproy = 0, sdist = 0; int c = 0, cp = 0;
		for (size_t k = 2; k < fotos.size(); k++) {
			const Foto& f = fotos[k]; const Foto& p = fotos[k - 2];
			const double dt = f.t - p.t; if (dt <= 0 || dt > 0.3) continue;
			for (auto& pr : par) {
				if (f.x[pr.first] > 1e8f || f.x[pr.second] > 1e8f || p.x[pr.first] > 1e8f) continue;
				const double dx = f.x[pr.second] - f.x[pr.first], dz = f.z[pr.second] - f.z[pr.first];
				sdx += dx; sdz += dz; sdx2 += dx * dx; sdz2 += dz * dz; sdist += std::hypot(dx, dz); c++;
				const double vx = (f.x[pr.first] - p.x[pr.first]) / dt, vz = (f.z[pr.first] - p.z[pr.first]) / dt, v = std::hypot(vx, vz);
				if (v > 2.0) { sproy += (dx * vx + dz * vz) / v; cp++; }
			}
		}
		if (c) std::printf("diferencia lista %u - lista %u: media dx=%.3f dz=%.3f m (desv %.3f / %.3f), distancia media %.3f m\n", cB, cA, sdx / c, sdz / c, std::sqrt(std::max(0.0, sdx2 / c - (sdx / c) * (sdx / c))), std::sqrt(std::max(0.0, sdz2 / c - (sdz / c) * (sdz / c))), sdist / c);
		if (cp) std::printf("proyectada sobre la direccion de carrera (jugadores a > 2 m/s): %.3f m (positivo = la lista %u va POR DELANTE de la %u)\n", sproy / cp, cB, cA);
	}
	timeEndPeriod(1);
	return 0;
}
