#include "UsuariosEnVivo.h"

#include <Windows.h>
#include <TlHelp32.h>

#include <cstring>

#include "PerfilesPes.h"

namespace phoenix::perfilespes::vivo {

	namespace {
		DWORD pidDePes() {
			DWORD pid = 0;
			HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (s == INVALID_HANDLE_VALUE) return 0;
			PROCESSENTRY32W p{}; p.dwSize = sizeof p;
			for (BOOL ok = Process32FirstW(s, &p); ok; ok = Process32NextW(s, &p))
				if (_wcsicmp(p.szExeFile, L"PES2021.exe") == 0) { pid = p.th32ProcessID; break; }
			CloseHandle(s);
			return pid;
		}

		uint64_t baseDelExe(DWORD pid) {
			uint64_t base = 0;
			HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
			if (s == INVALID_HANDLE_VALUE) return 0;
			MODULEENTRY32W m{}; m.dwSize = sizeof m;
			for (BOOL ok = Module32FirstW(s, &m); ok; ok = Module32NextW(s, &m))
				if (_wcsicmp(m.szModule, L"PES2021.exe") == 0) { base = reinterpret_cast<uint64_t>(m.modBaseAddr); break; }
			CloseHandle(s);
			return base;
		}

		constexpr size_t kTabla = size_t(kUsuarios) * kTamUsuario;   // 32 × 2.432 B
	}

	Conexion::~Conexion() { cerrar(); }

	void Conexion::cerrar() {
		if (_h) CloseHandle(_h);
		_h = nullptr; _base = 0; _tabla = 0;
	}

	bool Conexion::leer(uint64_t dir, void* dst, size_t n) {
		SIZE_T got = 0;
		return _h && ReadProcessMemory(_h, reinterpret_cast<LPCVOID>(dir), dst, n, &got) && got == n;
	}

	bool Conexion::escribir(uint64_t dir, const void* src, size_t n) {
		SIZE_T put = 0;
		return _h && WriteProcessMemory(_h, reinterpret_cast<LPVOID>(dir), src, n, &put) && put == n;
	}

	// 32 registros con forma de Usuario, con al menos un nombre y no todos iguales (una página de ceros no cuenta)
	bool Conexion::tablaValida(uint64_t dir) {
		std::vector<uint8_t> t(kTabla);
		if (!leer(dir, t.data(), t.size())) return false;
		bool algunNombre = false, distintos = false;
		for (int u = 0; u < kUsuarios; u++) {
			std::vector<uint8_t> r(t.begin() + size_t(u) * kTamUsuario, t.begin() + size_t(u + 1) * kTamUsuario);
			std::string motivo;
			if (!registroValido(r, motivo)) return false;
			// firma de un Usuario REAL: el nombre es texto válido (no 0xFF) y el mapa de botones (20 B desde +0x34) tiene valores pequeños (0…8 en los 32 de Fralex)
			size_t fin = 0;
			while (fin < kTamNombre && r[4 + fin]) fin++;
			if (!utf8Valido(std::string(reinterpret_cast<const char*>(&r[4]), fin))) return false;
			for (size_t k = 0x34; k < 0x34 + 20; k++) if (r[k] > 0x10) return false;
			if (fin >= 2) algunNombre = true;
			if (u > 0 && std::memcmp(t.data(), r.data(), kTamUsuario) != 0) distintos = true;
		}
		return algunNombre && distintos;
	}

	bool Conexion::buscarTabla(std::string& error) {
		// la tabla está al principio (+0x10) de una región RW del heap; se prueban unos pocos desfases por región
		static const uint64_t desfases[] = { 0x10, 0x0, 0x20, 0x40, 0x50 };
		MEMORY_BASIC_INFORMATION mbi{};
		uint64_t a = 0;
		while (VirtualQueryEx(_h, reinterpret_cast<LPCVOID>(a), &mbi, sizeof mbi)) {
			const uint64_t ini = reinterpret_cast<uint64_t>(mbi.BaseAddress), tam = mbi.RegionSize;
			const bool rw = mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));
			if (rw && tam >= 0x50 + kTabla) {
				for (uint64_t d : desfases) if (tam >= d + kTabla && tablaValida(ini + d)) { _tabla = ini + d; return true; }
			}
			a = ini + tam;
		}
		error = "no encuentro la tabla de Usuarios en la memoria de PES (¿otra versión del juego, o PES todavía cargando?)";
		return false;
	}

	bool Conexion::abrir(std::string& error) {
		cerrar();
		const DWORD pid = pidDePes();
		if (!pid) { error = "PES no está abierto"; return false; }
		_h = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
		if (!_h) { error = "no pude abrir PES (error " + std::to_string(GetLastError()) + "); prueba abrir Link como administrador si PES lo está"; return false; }
		_base = baseDelExe(pid);
		if (!_base) { error = "no encuentro el programa PES2021.exe"; cerrar(); return false; }
		int mandos[kMandos]; std::string e2;
		if (!leerMandos(mandos, e2)) { error = e2; cerrar(); return false; }
		if (!buscarTabla(error)) { cerrar(); return false; }
		return true;
	}

	bool Conexion::leerMandos(int (&usuarios)[kMandos], std::string& error) {
		if (!_h) { error = "no conectado a PES"; return false; }
		uint64_t puntero = 0; uint32_t cuenta = 0;
		if (!leer(_base + kOffsetPunteroMandos, &puntero, 8) || !leer(_base + kOffsetPunteroMandos + 8, &cuenta, 4)) { error = "no pude leer la lista de mandos"; return false; }
		if (cuenta != kMandos || puntero < 0x10000) { error = "la lista de mandos no es la esperada (esta versión de PES no es la conocida)"; return false; }
		uint32_t v[kMandos];
		if (!leer(puntero, v, sizeof v)) { error = "no pude leer el Usuario de los mandos"; return false; }
		for (int i = 0; i < kMandos; i++) {
			// 32 = «este mando todavía no tiene Usuario» (se ve en PES al reconectarse los mandos); se devuelve -1
			if (v[i] > uint32_t(kUsuarios)) { error = "la lista de mandos tiene un valor imposible (" + std::to_string(v[i]) + ")"; return false; }
			usuarios[i] = v[i] == uint32_t(kUsuarios) ? -1 : static_cast<int>(v[i]);
		}
		return true;
	}

	bool Conexion::leerUsuario(int indice, std::vector<uint8_t>& registro, std::string& error) {
		if (indice < 0 || indice >= kUsuarios) { error = "el Usuario debe estar entre 0 y 31"; return false; }
		if (!_h) { error = "no conectado a PES"; return false; }
		if (!tablaValida(_tabla) && !buscarTabla(error)) return false;
		registro.assign(kTamUsuario, 0);
		if (!leer(_tabla + uint64_t(indice) * kTamUsuario, registro.data(), kTamUsuario)) { error = "no pude leer ese Usuario"; return false; }
		return true;
	}

	bool Conexion::escribirUsuario(int indice, const std::vector<uint8_t>& registro, std::vector<uint8_t>& anterior, std::string& error) {
		if (indice < 0 || indice >= kUsuarios) { error = "el Usuario debe estar entre 0 y 31"; return false; }
		if (!registroValido(registro, error)) return false;
		if (!leerUsuario(indice, anterior, error)) return false;
		const uint64_t dir = _tabla + uint64_t(indice) * kTamUsuario;
		if (!escribir(dir, registro.data(), registro.size())) { error = "PES no dejó escribir en ese Usuario"; return false; }
		std::vector<uint8_t> despues(kTamUsuario);
		if (!leer(dir, despues.data(), despues.size()) || despues != registro) {
			escribir(dir, anterior.data(), anterior.size());   // vuelve a dejar lo que había
			error = "la relectura no coincide con lo escrito; se restauró el Usuario original";
			return false;
		}
		return true;
	}
}
