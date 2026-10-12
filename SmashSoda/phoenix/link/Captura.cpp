#include "Captura.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>

extern "C" {
#include "../terceros/pesxcrypter/crypt.h"
#include "../terceros/pesxcrypter/masterkey.h"
	int pesxCabeceraNuevaValida(const uint8_t* input, size_t fileSize, const char* masterKey);   // pesxvalidar.c
}
#include "Entrega.h"

namespace phoenix::traspasos {

namespace {
	// Distribucion del bloque de DATOS del option file de PES 2021 (de PhoenixSync/core/OptionFile.cpp, verificada).
	constexpr size_t kNumEquipos = 100;          // u16
	constexpr size_t kInicioEquipos = 0x8ED2FC;
	constexpr size_t kTamEquipo = 588;
	constexpr size_t kInicioPlantillas = 0x9D4648;
	constexpr size_t kTamPlantilla = 4 + 40 * 4 + 40 * 2 + 40;   // 284
	constexpr int kPlazas = 40;
	constexpr size_t kMinArchivo = 0x1000, kMaxArchivo = size_t(512) << 20;

	uint32_t u32(const std::vector<uint8_t>& d, size_t o) { return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (uint32_t(d[o + 3]) << 24); }
	uint16_t u16(const std::vector<uint8_t>& d, size_t o) { return uint16_t(d[o] | (d[o + 1] << 8)); }
	std::string texto(const uint8_t* p, size_t max) { size_t n = 0; while (n < max && p[n]) n++; return std::string(reinterpret_cast<const char*>(p), n); }

	struct Descriptor {
		FileDescriptorNew* d = createFileDescriptorNew();
		Descriptor() = default;
		Descriptor(const Descriptor&) = delete;
		Descriptor& operator=(const Descriptor&) = delete;
		~Descriptor() { if (d) destroyFileDescriptorNew(d); }
	};
}

bool leerFoto(const std::string& cifrado, Foto& foto, std::string& error) {
	foto = Foto();
	try {
		if (cifrado.size() < kMinArchivo || cifrado.size() > kMaxArchivo) { error = "tamano de archivo no valido"; return false; }
		std::vector<uint8_t> bytes(cifrado.begin(), cifrado.end());
		// decryptWithKeyNew se fia de los tamanos de la cabecera: con un archivo cortado o a medio guardar cerraria Link. Se comprueba antes.
		if (!pesxCabeceraNuevaValida(bytes.data(), bytes.size(), reinterpret_cast<const char*>(MasterKeyPes21))) { error = "el archivo esta incompleto o no es un option file (cabecera incoherente)"; return false; }
		Descriptor desc;
		if (!desc.d) { error = "sin memoria"; return false; }
		decryptWithKeyNew(desc.d, bytes.data(), reinterpret_cast<const char*>(MasterKeyPes21));
		const FileHeaderNew* h = desc.d->fileHeader;
		if (!h || !desc.d->data) { error = "no se pudo descifrar"; return false; }
		const std::string tipo = texto(h->fileTypeString, sizeof(h->fileTypeString));
		const std::string juego = texto(h->gameVersionString, sizeof(h->gameVersionString));
		if (tipo != "EDIT" || juego.find("2021") == std::string::npos) { error = "no es un option file de PES 2021 (" + tipo + " / " + juego + ")"; return false; }
		if (h->dataSize < kInicioPlantillas || h->dataSize > kMaxArchivo) { error = "tamano de datos incoherente"; return false; }
		const std::vector<uint8_t> d(desc.d->data, desc.d->data + h->dataSize);
		const size_t ne = u16(d, kNumEquipos);
		if (ne == 0 || kInicioPlantillas + ne * kTamPlantilla > d.size() || kInicioEquipos + ne * kTamEquipo > kInicioPlantillas) { error = "cantidades fuera de rango (formato desconocido)"; return false; }
		std::set<uint32_t> ids;
		for (size_t i = 0; i < ne; i++) {
			const uint8_t* e = d.data() + kInicioEquipos + i * kTamEquipo;
			const uint32_t id = u32(d, kInicioEquipos + i * kTamEquipo);
			ids.insert(id);
			foto.equipos[id] = texto(e + 104, 70);
		}
		for (size_t i = 0; i < ne; i++) {
			const size_t base = kInicioPlantillas + i * kTamPlantilla;
			const uint32_t eq = u32(d, base);
			if (!ids.count(eq)) { error = "plantilla de un equipo desconocido (formato desconocido)"; return false; }
			std::vector<uint32_t>& pl = foto.plantillas[eq];
			for (int k = 0; k < kPlazas; k++) {
				const uint32_t j = u32(d, base + 4 + k * 4);
				if (j) pl.push_back(j);
			}
		}
		foto.sha256 = phoenix::entrega::sha256Hex(cifrado.data(), cifrado.size());
		return true;
	}
	catch (const std::exception& e) { error = e.what(); return false; }
}

bool leerFotoArchivo(const fs::path& archivo, Foto& foto, std::string& error) {
	std::ifstream f(archivo, std::ios::binary);
	if (!f) { error = "no se puede abrir el archivo"; return false; }
	const std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	return leerFoto(bytes, foto, error);
}

Diferencia comparar(const Foto& antes, const Foto& despues) {
	Diferencia r;
	// jugador -> equipos donde estaba / donde esta (solo equipos presentes en las dos fotos)
	std::map<uint32_t, std::set<uint32_t>> estaba, esta;
	for (const auto& [eq, pl] : antes.plantillas) {
		auto it = despues.plantillas.find(eq);
		if (it == despues.plantillas.end()) continue;
		r.equiposComparados++;
		for (uint32_t j : pl) estaba[j].insert(eq);
		for (uint32_t j : it->second) esta[j].insert(eq);
	}
	std::set<uint32_t> todos;
	for (const auto& p : estaba) todos.insert(p.first);
	for (const auto& p : esta) todos.insert(p.first);
	for (uint32_t j : todos) {
		const std::set<uint32_t> vacio;
		const std::set<uint32_t>& a = estaba.count(j) ? estaba[j] : vacio;
		const std::set<uint32_t>& b = esta.count(j) ? esta[j] : vacio;
		std::vector<uint32_t> sale, entra;
		std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(sale));
		std::set_difference(b.begin(), b.end(), a.begin(), a.end(), std::back_inserter(entra));
		if (sale.empty() && entra.empty()) continue;
		if (sale.size() == 1 && entra.size() == 1) {
			Movimiento m;
			m.jugador = j; m.desde = sale[0]; m.hasta = entra[0];
			auto n1 = antes.equipos.find(m.desde); if (n1 != antes.equipos.end()) m.desdeNombre = n1->second;
			auto n2 = despues.equipos.find(m.hasta); if (n2 != despues.equipos.end()) m.hastaNombre = n2->second;
			r.traspasos.push_back(m);
		}
		else if (sale.empty()) r.altas++;
		else if (entra.empty()) r.bajas++;
		else r.raros++;
	}
	return r;
}

} // namespace phoenix::traspasos
