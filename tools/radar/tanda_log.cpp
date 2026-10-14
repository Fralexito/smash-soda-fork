// Registro en vivo de una tanda de penales (SOLO lectura de PES2021.exe). Cada 0,25 s imprime: balón (x, z, altura), cuántos
// jugadores de campo se mueven, el jugador más cercano al balón y las copias de portero que se mueven con su posición.
// Sirve para ver cómo se comporta la memoria en los penales (lado = 0, balón en el punto, pateador, portero que se lanza).
// Uso: tanda_log [segundos=40]
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <timeapi.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <chrono>
#include <vector>
#include <algorithm>
#include "phoenix/core/CampoVivo.h"
#pragma comment(lib, "winmm.lib")
using namespace phoenix::campo;
using Reloj = std::chrono::steady_clock;

int main(int argc, char** argv) {
	const int segundos = argc > 1 ? std::atoi(argv[1]) : 40;
	HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe{}; pe.dwSize = sizeof pe; DWORD pid = 0;
	if (Process32FirstW(s, &pe)) do { if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; } } while (Process32NextW(s, &pe));
	CloseHandle(s); if (!pid) { std::printf("PES2021 no esta abierto\n"); return 1; }
	HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); if (!h) return 2;
	timeBeginPeriod(1);
	std::vector<Actor> campo, porteros, balones;
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
				else if (leerBalon(buf.data() + i, x)) { x.dir = p + i; balones.push_back(x); }
			}
		}
		a = base + m.RegionSize; } }
	std::printf("campo %zu, porteros(copias) %zu, balones %zu\n", campo.size(), porteros.size(), balones.size());
	std::vector<float> cx(campo.size(), 1e9f), cz(campo.size(), 1e9f), px(porteros.size(), 1e9f), pz(porteros.size(), 1e9f);
	std::vector<double> pCamb(porteros.size(), -1e9), cCamb(campo.size(), -1e9);
	const auto t0 = Reloj::now(); double ultimo = -1;
	while (true) {
		const double t = std::chrono::duration<double>(Reloj::now() - t0).count(); if (t > segundos) break;
		// actualizar posiciones y detectar cuáles se mueven
		for (size_t i = 0; i < campo.size(); i++) { uint8_t b[16]; SIZE_T g = 0; float x, y, z; if (ReadProcessMemory(h, (LPCVOID)(campo[i].dir + kOffPosicion), b, 16, &g) && g == 16 && leerPosicion(b, x, y, z)) { if (std::fabs(x - cx[i]) > 0.01f || std::fabs(z - cz[i]) > 0.01f) cCamb[i] = t; cx[i] = x; cz[i] = z; } }
		for (size_t i = 0; i < porteros.size(); i++) { uint8_t b[16]; SIZE_T g = 0; float x, y, z; if (ReadProcessMemory(h, (LPCVOID)(porteros[i].dir + kOffPosicion), b, 16, &g) && g == 16 && leerPosicion(b, x, y, z)) { if (std::fabs(x - px[i]) > 0.01f || std::fabs(z - pz[i]) > 0.01f) pCamb[i] = t; px[i] = x; pz[i] = z; } }
		if (t - ultimo >= 0.25) {
			ultimo = t;
			float bx = 0, bz = 0, by = 0; bool hay = false;
			for (const Actor& bb : balones) { uint8_t b[16]; SIZE_T g = 0; float x, y, z; if (ReadProcessMemory(h, (LPCVOID)(bb.dir + kOffPosicion), b, 16, &g) && g == 16 && leerPosicionBalon(b, x, y, z) && std::fabs(z) <= 40.0f) { bx = x; bz = z; by = y; hay = true; break; } }
			int mov = 0; int cerca = -1; double dm = 1e9;
			for (size_t i = 0; i < campo.size(); i++) { if (t - cCamb[i] < 0.5) mov++; if (hay && cx[i] < 1e8f) { double d = std::hypot(cx[i] - bx, cz[i] - bz); if (d < dm) { dm = d; cerca = (int)i; } } }
			std::printf("t=%5.1f balon=%s(%.1f,%.1f,y=%.2f) mov=%d cercano=%s", t, hay ? "" : "NO ", bx, bz, by, mov, cerca >= 0 ? "" : "-");
			if (cerca >= 0) std::printf("(%.1f,%.1f d=%.1f)", cx[cerca], cz[cerca], dm);
			std::printf(" porteros:");
			for (size_t i = 0; i < porteros.size(); i++) if (t - pCamb[i] < 1.0) std::printf(" [%.1f,%.1f]", px[i], pz[i]);
			std::printf("\n"); std::fflush(stdout);
		}
		Sleep(2);
	}
	timeEndPeriod(1);
	return 0;
}
