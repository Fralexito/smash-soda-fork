// Mide cómo se mueven de verdad los jugadores en la memoria de PES2021 (SOLO lectura; no pausa ni escribe nada).
// Escanea como Link, lee los actores de campo de TODAS las listas (cada equipo de IA tiene la suya, con las mismas fichas
// ligeramente distintas) a ritmo máximo durante N segundos y calcula:
//   1) con qué frecuencia cambia de verdad la posición de un actor (ritmo del juego, huecos, saltos)
//   2) cuánto se parecen las dos listas (distancia media entre fichas emparejadas) y cuál va por delante (desfase en ms)
//   3) saltos imposibles (> 14 m/s) y fichas que no se mueven nunca
// Uso: medir [segundos=20]      (con el partido en juego)
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

struct Muestra { double t; float x, z; };

int main(int argc, char** argv) {
	const int segundos = argc > 1 ? std::atoi(argv[1]) : 20;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); if (!pid) { std::printf("PES2021 no esta abierto\n"); return 1; }
	HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	timeBeginPeriod(1);
	std::vector<Actor> campo, porteros;
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
				else if (leerPortero(buf.data() + i, x)) { x.dir = p + i; porteros.push_back(x); }
			}
		}
		a = base + m.RegionSize; } }
	std::map<unsigned, std::vector<Actor>> porCodigo;
	for (const Actor& a : campo) porCodigo[a.codigo].push_back(a);
	std::printf("actores de campo: %zu en %zu listas; porteros (copias): %zu\n", campo.size(), porCodigo.size(), porteros.size());
	for (auto& kv : porCodigo) std::printf("  lista cod=%u: %zu actores\n", kv.first, kv.second.size());
	for (const Actor& p : porteros) std::printf("  portero dir=%llX cod=%u x=%.1f z=%.1f\n", (unsigned long long)p.dir, p.codigo, p.x, p.z);
	if (campo.empty()) { std::printf("no hay partido en juego (o no se encontraron jugadores)\n"); return 3; }
	// seguimiento
	std::vector<Actor> todos = campo; for (const Actor& p : porteros) todos.push_back(p);
	std::vector<std::vector<Muestra>> serie(todos.size());     // solo cuando CAMBIA la posición
	std::vector<float> ux(todos.size(), 1e9f), uz(todos.size(), 1e9f);
	const auto t0 = Reloj::now();
	long vueltas = 0;
	while (std::chrono::duration<double>(Reloj::now() - t0).count() < segundos) {
		const double t = std::chrono::duration<double>(Reloj::now() - t0).count();
		for (size_t i = 0; i < todos.size(); i++) {
			uint8_t b[16]; SIZE_T g = 0; float x, y, z;
			if (!ReadProcessMemory(h, (LPCVOID)(todos[i].dir + kOffPosicion), b, 16, &g) || g != 16 || !leerPosicion(b, x, y, z)) continue;
			if (std::fabs(x - ux[i]) > 1e-4f || std::fabs(z - uz[i]) > 1e-4f) { serie[i].push_back({ t, x, z }); ux[i] = x; uz[i] = z; }
		}
		vueltas++;
		Sleep(1);
	}
	const double dur = std::chrono::duration<double>(Reloj::now() - t0).count();
	std::printf("medido %.1f s: %ld vueltas (%.0f lecturas/s de cada actor)\n", dur, vueltas, vueltas / dur);
	// 1) ritmo de cambios
	std::vector<double> medianas, huecosMax, saltos; int quietos = 0, conMov = 0; double sumaCamb = 0;
	for (size_t i = 0; i < todos.size(); i++) {
		auto& v = serie[i];
		if (v.size() < 5) { quietos++; continue; }
		conMov++; sumaCamb += v.size() / dur;
		std::vector<double> dts; double mx = 0; int sal = 0;
		for (size_t k = 1; k < v.size(); k++) {
			const double dt = v[k].t - v[k - 1].t; dts.push_back(dt); mx = std::max(mx, dt);
			const double d = std::sqrt((v[k].x - v[k - 1].x) * (v[k].x - v[k - 1].x) + (v[k].z - v[k - 1].z) * (v[k].z - v[k - 1].z));
			if (d / std::max(dt, 1e-3) > 14.0 && d > 0.5) sal++;
		}
		std::sort(dts.begin(), dts.end()); medianas.push_back(dts[dts.size() / 2] * 1000.0); huecosMax.push_back(mx * 1000.0); saltos.push_back(sal);
	}
	auto med = [](std::vector<double> v) { if (v.empty()) return 0.0; std::sort(v.begin(), v.end()); return v[v.size() / 2]; };
	std::printf("actores quietos todo el rato: %d de %zu; con movimiento: %d\n", quietos, todos.size(), conMov);
	std::printf("cambios de posicion por segundo y actor: %.1f (el juego actualiza a ese ritmo)\n", conMov ? sumaCamb / conMov : 0.0);
	std::printf("tiempo entre cambios: mediana %.1f ms; hueco maximo tipico %.0f ms\n", med(medianas), med(huecosMax));
	double totSaltos = 0; for (double s2 : saltos) totSaltos += s2;
	std::printf("saltos imposibles (> 14 m/s y > 0,5 m): %.0f en total\n", totSaltos);
	// 2) comparar listas: emparejar cada actor de la primera lista con el más cercano de la segunda (en el instante medio)
	if (porCodigo.size() >= 2) {
		auto it = porCodigo.begin(); const unsigned cA = it->first; ++it; const unsigned cB = it->first;
		auto posEn = [&](size_t i, double t, float& x, float& z) {   // posición «sostenida» (último cambio <= t)
			auto& v = serie[i]; if (v.empty()) return false;
			size_t lo = 0, hi = v.size(); while (lo < hi) { size_t m = (lo + hi) / 2; if (v[m].t <= t) lo = m + 1; else hi = m; }
			if (lo == 0) return false; x = v[lo - 1].x; z = v[lo - 1].z; return true; };
		std::vector<size_t> ia, ib;
		for (size_t i = 0; i < todos.size(); i++) { if (todos[i].clase != Clase::Campo) continue; if (todos[i].codigo == cA) ia.push_back(i); else if (todos[i].codigo == cB) ib.push_back(i); }
		const double tm = dur / 2; double suma = 0; int n = 0; std::vector<std::pair<size_t, size_t>> par;
		for (size_t i : ia) { float x, z; if (!posEn(i, tm, x, z)) continue; double mejor = 1e9; size_t mj = 0;
			for (size_t j : ib) { float x2, z2; if (!posEn(j, tm, x2, z2)) continue; double d = std::hypot(x - x2, z - z2); if (d < mejor) { mejor = d; mj = j; } }
			if (mejor < 3.0) { par.push_back({ i, mj }); suma += mejor; n++; } }
		std::printf("listas %u y %u: %d fichas emparejadas, distancia media %.2f m\n", cA, cB, n, n ? suma / n : 0.0);
		// desfase: lo que mejor alinea la lista B con la A
		double mejorLag = 0, mejorErr = 1e9;
		for (int lag = -150; lag <= 150; lag += 5) {
			double err = 0; int c = 0;
			for (double t = 2.0; t < dur - 2.0; t += 0.05) for (auto& p : par) { float x, z, x2, z2; if (posEn(p.first, t, x, z) && posEn(p.second, t + lag / 1000.0, x2, z2)) { err += std::hypot(x - x2, z - z2); c++; } }
			if (c && err / c < mejorErr) { mejorErr = err / c; mejorLag = lag; }
		}
		std::printf("desfase que mejor alinea la lista %u con la %u: %.0f ms (error medio %.3f m)\n", cB, cA, mejorLag, mejorErr);
	}
	timeEndPeriod(1);
	return 0;
}
