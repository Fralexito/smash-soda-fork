#include "MandosExtra.h"

#include <Windows.h>
#include <ShlObj.h>

#include <cstring>
#include <ctime>
#include <fstream>

namespace phoenix::mandosextra {

	namespace {
		constexpr size_t kTamSettings = 612;
		constexpr size_t kCasillasSettings = 0x64;

		void u32(std::vector<unsigned char>& v, size_t o, uint32_t x) {
			v[o] = static_cast<unsigned char>(x); v[o + 1] = static_cast<unsigned char>(x >> 8);
			v[o + 2] = static_cast<unsigned char>(x >> 16); v[o + 3] = static_cast<unsigned char>(x >> 24);
		}
		uint32_t u32(const std::vector<unsigned char>& v, size_t o) {
			return v[o] | (v[o + 1] << 8) | (v[o + 2] << 16) | (static_cast<uint32_t>(v[o + 3]) << 24);
		}

		std::wstring sello() {
			std::time_t t = std::time(nullptr);
			std::tm tm{}; localtime_s(&tm, &t);
			wchar_t b[32]; wcsftime(b, 32, L"%Y%m%d-%H%M%S", &tm);
			return b;
		}
	}

	std::vector<unsigned char> construir(const std::vector<std::vector<unsigned char>>& guids,
		const std::vector<unsigned char>& mapa) {
		std::vector<unsigned char> r(kCabecera + guids.size() * kCasilla, 0);
		std::memcpy(r.data(), "PHXM", 4);
		u32(r, 4, kVersion);
		u32(r, 8, static_cast<uint32_t>(guids.size()));
		for (size_t i = 0; i < guids.size(); i++) {
			unsigned char* c = &r[kCabecera + i * kCasilla];
			if (guids[i].size() == 16) std::memcpy(c, guids[i].data(), 16);
			if (mapa.size() == kMapa) std::memcpy(c + 16, mapa.data(), kMapa);
		}
		return r;
	}

	bool esValido(const std::vector<unsigned char>& a) {
		if (a.size() < kCabecera || std::memcmp(a.data(), "PHXM", 4) != 0 || u32(a, 4) != kVersion) return false;
		return a.size() == kCabecera + static_cast<size_t>(u32(a, 8)) * kCasilla;
	}

	uint32_t cantidad(const std::vector<unsigned char>& a) {
		return esValido(a) ? u32(a, 8) : 0;
	}

	std::vector<unsigned char> guidDe(const std::vector<unsigned char>& a, size_t i) {
		if (i >= cantidad(a)) return {};
		const unsigned char* c = &a[kCabecera + i * kCasilla];
		static const unsigned char cero[16] = {};
		if (std::memcmp(c, cero, 16) == 0) return {};
		return std::vector<unsigned char>(c, c + 16);
	}

	std::vector<unsigned char> mapaDeCasilla1(const std::vector<unsigned char>& settings,
		const std::vector<unsigned char>& defecto) {
		if (settings.size() != kTamSettings || std::memcmp(settings.data(), "WECF", 4) != 0) return defecto;
		const unsigned char* m = &settings[kCasillasSettings + 16];
		bool todoCero = true;
		for (size_t i = 0; i < kMapa; i++) if (m[i]) { todoCero = false; break; }
		if (todoCero) return defecto;
		return std::vector<unsigned char>(m, m + kMapa);
	}

	std::wstring ruta() {
		PWSTR docs = nullptr;
		std::wstring r;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) r = docs;
		if (docs) CoTaskMemFree(docs);
		if (r.empty()) return r;
		return r + L"\\KONAMI\\eFootball PES 2021 SEASON UPDATE\\phoenix_mandos_extra.dat";
	}

	bool escribir(const std::wstring& ruta, const std::vector<unsigned char>& datos, bool copiar,
		std::string& copia, std::string& error) {
		if (ruta.empty()) { error = "No encuentro la carpeta de PES 2021 en Documentos."; return false; }
		if (copiar && GetFileAttributesW(ruta.c_str()) != INVALID_FILE_ATTRIBUTES) {
			const std::wstring c = ruta + L".phoenix-" + sello();
			if (!CopyFileW(ruta.c_str(), c.c_str(), TRUE)) { error = "No se pudo hacer la copia de seguridad de phoenix_mandos_extra.dat."; return false; }
			const std::wstring nombre = c.substr(c.find_last_of(L'\\') + 1);
			copia.clear();
			for (wchar_t ch : nombre) copia.push_back(static_cast<char>(ch));   // nombre ASCII
		}
		const std::wstring tmp = ruta + L".phoenix-tmp";
		bool ok = false;
		{
			std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
			out.write(reinterpret_cast<const char*>(datos.data()), static_cast<std::streamsize>(datos.size()));
			ok = out.good();
		}
		ok = ok && MoveFileExW(tmp.c_str(), ruta.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
		if (!ok) { DeleteFileW(tmp.c_str()); error = "No se pudo escribir phoenix_mandos_extra.dat."; return false; }
		return true;
	}
}
