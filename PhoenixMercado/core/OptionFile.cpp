#include "OptionFile.h"
#include "Alineacion.h"
#include "Sha256.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>

extern "C" {
#include "../terceros/pesxcrypter/crypt.h"
	extern const uint8_t MasterKeyPes21[];
}

namespace mercado {

	namespace {
		// --- Distribución del bloque de DATOS (PES 2021) -------------------
		constexpr size_t kNumJugadores = 96;        // u16
		constexpr size_t kNumEquipos = 100;         // u16
		constexpr size_t kInicioJugadores = 0x7C;
		constexpr size_t kTamJugador = 312;
		constexpr size_t kInicioEquipos = 0x8ED2FC;
		constexpr size_t kTamEquipo = 588;
		constexpr size_t kInicioPlantillas = 0x9D4648;
		constexpr size_t kTamPlantilla = 4 + 40 * 4 + 40 * 2 + 40;  // 284
		constexpr int kPlazas = 40;
		// Tácticas por equipo (verificado en el option file del ConmeGOL 26: 749 bloques, todos con el ID del equipo en +0;
		// el editor del juego deja el orden como identidad 0…39 y los equipos vacíos todo 0xFF).
		constexpr size_t kInicioTacticas = 0xA09880;
		constexpr size_t kTamTactica = 628;
		constexpr size_t kTacOfsOrden = 0x1E4;
		constexpr size_t kTacOfsRoles = 0x20C;

		uint32_t u32(const std::vector<uint8_t>& d, size_t o) {
			return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (uint32_t(d[o + 3]) << 24);
		}
		uint16_t u16(const std::vector<uint8_t>& d, size_t o) { return uint16_t(d[o] | (d[o + 1] << 8)); }
		void p32(std::vector<uint8_t>& d, size_t o, uint32_t v) { for (int i = 0; i < 4; i++) d[o + i] = uint8_t(v >> (8 * i)); }
		void p16(std::vector<uint8_t>& d, size_t o, uint16_t v) { d[o] = uint8_t(v); d[o + 1] = uint8_t(v >> 8); }

		/// Guarda/descarta el descriptor de libpesXcrypter (C) de forma segura.
		struct Descriptor {
			FileDescriptorNew* d = createFileDescriptorNew();
			~Descriptor() { if (d) destroyFileDescriptorNew(d); }
		};
	}

	uint32_t leerBits(const uint8_t* base, size_t bitPos, int bits) {
		uint32_t v = 0;
		for (int i = 0; i < bits; i++) {
			const size_t b = bitPos + i;
			v |= uint32_t((base[b >> 3] >> (b & 7)) & 1) << i;
		}
		return v;
	}

	std::string leerTexto(const uint8_t* p, size_t max) {
		size_t n = 0;
		while (n < max && p[n]) n++;
		return std::string(reinterpret_cast<const char*>(p), n);
	}

	Resultado<OptionFile> OptionFile::abrir(const std::string& ruta) {
		using R = Resultado<OptionFile>;
		try {
			std::ifstream f(aRuta(ruta), std::ios::binary);
			if (!f) return R::mal("ARCHIVO_NO_EXISTE", ruta);
			OptionFile o;
			o._cifrado.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
			if (o._cifrado.size() < 0x1000) return R::mal("OPTION_INVALIDO", "Archivo demasiado pequeño");

			Descriptor desc;
			decryptWithKeyNew(desc.d, o._cifrado.data(), reinterpret_cast<const char*>(MasterKeyPes21));
			if (!desc.d->fileHeader || !desc.d->data) return R::mal("OPTION_INVALIDO", "No se pudo descifrar");
			const std::string tipo = leerTexto(desc.d->fileHeader->fileTypeString, 32);
			const std::string juego = leerTexto(desc.d->fileHeader->gameVersionString, 32);
			if (tipo != "EDIT" || juego.find("2021") == std::string::npos)
				return R::mal("OPTION_NO_PES21", tipo + " / " + juego);
			const uint32_t tam = desc.d->fileHeader->dataSize;
			o._datos.assign(desc.d->data, desc.d->data + tam);
			const auto& d = o._datos;

			const size_t nj = u16(d, kNumJugadores), ne = u16(d, kNumEquipos);
			if (kInicioPlantillas + ne * kTamPlantilla > d.size()
				|| kInicioTacticas + ne * kTamTactica > d.size()
				|| kInicioJugadores + nj * kTamJugador > kInicioEquipos)
				return R::mal("OPTION_FORMATO_DESCONOCIDO", "Cantidades fuera de rango");

			for (size_t i = 0; i < nj; i++) {
				const uint8_t* e = d.data() + kInicioJugadores + i * kTamJugador;
				JugadorEditado j;
				j.id = leerBits(e, 0, 32);
				j.nacionalidad = int(leerBits(e, 64, 16));
				j.altura = int(leerBits(e, 80, 8));
				j.peso = int(leerBits(e, 88, 8));
				j.edad = int(leerBits(e, 263, 6));
				j.posicion = int(leerBits(e, 269, 4));
				j.nombre = leerTexto(e + 54, 61);
				j.nombreCamiseta = leerTexto(e + 115, 21);
				if (j.id) o._editados.push_back(j);
			}
			for (size_t i = 0; i < ne; i++) {
				const uint8_t* e = d.data() + kInicioEquipos + i * kTamEquipo;
				EquipoLocal t;
				t.id = leerBits(e, 0, 32);
				t.nombre = leerTexto(e + 104, 70);
				t.abreviatura = leerTexto(e + 174, 3);
				o._equipos.push_back(t);
			}
			std::set<uint32_t> ids;
			for (const auto& t : o._equipos) ids.insert(t.id);
			for (size_t i = 0; i < ne; i++) {
				const size_t base = kInicioPlantillas + i * kTamPlantilla;
				const uint32_t eq = u32(d, base);
				if (!ids.count(eq)) return R::mal("OPTION_FORMATO_DESCONOCIDO", "Plantilla de equipo desconocido");
				std::vector<PlazaPlantilla> pl;
				for (int k = 0; k < kPlazas; k++) {
					const uint32_t j = u32(d, base + 4 + k * 4);
					if (j) pl.push_back({ j, u16(d, base + 4 + kPlazas * 4 + k * 2) });
				}
				o._plantillas[eq] = pl;
				o._offsetPlantilla[eq] = base;
				// Bloque de tácticas del mismo equipo (mismo orden que las plantillas; lleva el ID en +0).
				const size_t tac = kInicioTacticas + i * kTamTactica;
				if (u32(d, tac) != eq) return R::mal("OPTION_FORMATO_DESCONOCIDO", "El bloque de tácticas no corresponde al equipo " + std::to_string(eq));
				o._offsetTactica[eq] = tac;
			}
			return R::bien(std::move(o));
		}
		catch (const std::exception& e) { return R::mal("OPTION_ERROR", e.what()); }
	}

	void OptionFile::escribirPlantilla(uint32_t equipo) {
		const size_t base = _offsetPlantilla.at(equipo);
		const auto& pl = _plantillas.at(equipo);
		for (int k = 0; k < kPlazas; k++) {
			const bool hay = k < int(pl.size());
			p32(_datos, base + 4 + k * 4, hay ? pl[k].jugador : 0);
			p16(_datos, base + 4 + kPlazas * 4 + k * 2, hay ? pl[k].dorsal : 0);
		}
		// Los 40 bytes finales («Unknown A» en 4ccEditor) no se tocan.
	}

	Resultado<AlineacionLocal> OptionFile::leerAlineacion(uint32_t equipo, size_t n) const {
		using R = Resultado<AlineacionLocal>;
		const auto it = _offsetTactica.find(equipo);
		if (it == _offsetTactica.end()) return R::mal("EQUIPO_NO_EXISTE", std::to_string(equipo));
		const uint8_t* p = _datos.data() + it->second;
		AlineacionLocal a;
		std::string porque;
		if (!alineacion::leerOrden(p + kTacOfsOrden, n, a.orden, porque)) return R::mal("ALINEACION_INVALIDA", "Equipo " + std::to_string(equipo) + ": " + porque);
		if (!alineacion::leerRoles(p + kTacOfsRoles, n, a.roles, porque)) return R::mal("ALINEACION_INVALIDA", "Equipo " + std::to_string(equipo) + ": " + porque);
		a.colaIdentidad = alineacion::colaEsIdentidad(p + kTacOfsOrden, n);
		return R::bien(std::move(a));
	}

	void OptionFile::escribirAlineacion(uint32_t equipo, const AlineacionLocal& a) {
		uint8_t* p = _datos.data() + _offsetTactica.at(equipo);
		alineacion::escribirOrden(p + kTacOfsOrden, a.orden, a.colaIdentidad);
		alineacion::escribirRoles(p + kTacOfsRoles, a.roles);
	}

	Resultado<AlineacionLocal> OptionFile::alineacion(uint32_t equipo) const {
		using R = Resultado<AlineacionLocal>;
		const auto it = _plantillas.find(equipo);
		if (it == _plantillas.end()) return R::mal("EQUIPO_NO_EXISTE", std::to_string(equipo));
		return leerAlineacion(equipo, it->second.size());
	}

	Resultado<uint32_t> OptionFile::sugerirSustituto(uint32_t equipo, uint32_t jugador, const std::function<int(uint32_t)>& posicionDe) const {
		using R = Resultado<uint32_t>;
		const auto it = _plantillas.find(equipo);
		if (it == _plantillas.end()) return R::mal("EQUIPO_NO_EXISTE", std::to_string(equipo));
		const auto& pl = it->second;
		int idx = -1;
		for (size_t i = 0; i < pl.size(); i++) if (pl[i].jugador == jugador) idx = int(i);
		if (idx < 0) return R::mal("JUGADOR_NO_ESTA", "El jugador no está en ese equipo");
		auto a = leerAlineacion(equipo, pl.size());
		if (!a.ok()) return R::mal(a.error.codigo, a.error.detalle);
		if (!alineacion::necesitaSustituto(a.valor->orden, a.valor->roles, idx)) return R::bien(0u);
		std::function<int(int)> posDeIdx;
		if (posicionDe) posDeIdx = [&](int i) { return i >= 0 && size_t(i) < pl.size() ? posicionDe(pl[size_t(i)].jugador) : -1; };
		const int elegido = alineacion::elegirSustituto(a.valor->orden, idx, posDeIdx);
		if (elegido < 0) return R::mal("SIN_SUSTITUTO", "No hay ningún jugador libre (o ningún otro portero) para cubrir el puesto");
		return R::bien(pl[size_t(elegido)].jugador);
	}

	namespace {
		struct SalidaPreparada { int idx = -1; AlineacionLocal ali; };
	}

	/// Comprueba la salida de `jugador` de `origen` (índice, sustituto, alineación nueva) sin tocar nada.
	static Resultado<SalidaPreparada> prepararSalida(const OptionFile& of, const std::map<uint32_t, std::vector<PlazaPlantilla>>& plantillas,
		uint32_t jugador, uint32_t origen, uint32_t sustituto) {
		using R = Resultado<SalidaPreparada>;
		if (!plantillas.count(origen)) return R::mal("EQUIPO_NO_EXISTE", std::to_string(origen));
		const auto& po = plantillas.at(origen);
		SalidaPreparada s; int idxS = -1;
		for (size_t i = 0; i < po.size(); i++) { if (po[i].jugador == jugador) s.idx = int(i); if (sustituto && po[i].jugador == sustituto) idxS = int(i); }
		if (s.idx < 0) return R::mal("JUGADOR_NO_ESTA_EN_ORIGEN");
		if (sustituto && idxS < 0) return R::mal("SUSTITUTO_NO_ESTA", "El sustituto no está en el equipo de origen");
		if (sustituto && sustituto == jugador) return R::mal("SUSTITUTO_INVALIDO", "El sustituto es el mismo jugador");
		auto a = of.alineacion(origen);
		if (!a.ok()) return R::mal(a.error.codigo, a.error.detalle);
		s.ali = *a.valor;
		if (idxS < 0 && alineacion::necesitaSustituto(s.ali.orden, s.ali.roles, s.idx))
			return R::mal("FALTA_SUSTITUTO", "El jugador es titular o tiene un rol en la alineación: hace falta un sustituto (sugerirSustituto)");
		std::string porque;
		if (!alineacion::quitarDeOrden(s.ali.orden, s.ali.roles, s.idx, idxS, porque)) return R::mal("ALINEACION_INVALIDA", porque);
		return R::bien(std::move(s));
	}

	Resultado<bool> OptionFile::mover(uint32_t jugador, uint32_t destino, uint32_t origen, uint16_t dorsal, uint32_t sustituto) {
		using R = Resultado<bool>;
		if (!_plantillas.count(destino)) return R::mal("EQUIPO_NO_EXISTE", std::to_string(destino));
		const auto& pdAntes = _plantillas.at(destino);
		for (const auto& p : pdAntes) if (p.jugador == jugador) return R::mal("YA_ESTA_EN_DESTINO");
		if (int(pdAntes.size()) >= kPlazas) return R::mal("PLANTILLA_LLENA", "El destino ya tiene 40 jugadores");

		if (origen == 0) {
			for (const auto& [eq, pl] : _plantillas)
				for (const auto& p : pl) if (p.jugador == jugador && eq != destino && !origen) origen = eq;
		}
		// --- Se comprueba TODO antes de tocar nada (todo o nada) ---------------
		SalidaPreparada salida;
		if (origen) {
			auto s = prepararSalida(*this, _plantillas, jugador, origen, sustituto);
			if (!s.ok()) return R::mal(s.error.codigo, s.error.detalle);
			salida = *s.valor;
		}
		AlineacionLocal aliDestino;
		{
			auto a = leerAlineacion(destino, pdAntes.size());
			if (!a.ok()) return R::mal(a.error.codigo, a.error.detalle);
			aliDestino = *a.valor;
			aliDestino.orden.push_back(uint8_t(pdAntes.size()));   // el que llega es la última reserva
		}
		if (dorsal == 0) {
			dorsal = 99;
			while (dorsal > 1 && std::any_of(pdAntes.begin(), pdAntes.end(), [&](const PlazaPlantilla& p) { return p.dorsal == dorsal; })) dorsal--;
		}
		// --- Todo cuadra: se aplica ---------------------------------------------
		if (origen) {
			auto& po = _plantillas[origen];
			po.erase(po.begin() + salida.idx);   // se compacta: sin huecos en la lista (los índices mayores bajan uno, igual que en el orden)
			escribirPlantilla(origen);
			escribirAlineacion(origen, salida.ali);
		}
		auto& pd = _plantillas[destino];
		pd.push_back({ jugador, dorsal });
		escribirPlantilla(destino);
		escribirAlineacion(destino, aliDestino);
		return R::bien(true);
	}

	Resultado<bool> OptionFile::quitar(uint32_t jugador, uint32_t equipo, uint32_t sustituto) {
		using R = Resultado<bool>;
		auto s = prepararSalida(*this, _plantillas, jugador, equipo, sustituto);
		if (!s.ok()) return R::mal(s.error.codigo, s.error.detalle);
		auto& po = _plantillas[equipo];
		po.erase(po.begin() + s.valor->idx);
		escribirPlantilla(equipo);
		escribirAlineacion(equipo, s.valor->ali);
		return R::bien(true);
	}

	Resultado<std::string> OptionFile::guardarComo(const std::string& rutaNueva) const {
		using R = Resultado<std::string>;
		try {
			namespace fs = std::filesystem;
			if (fs::exists(aRuta(rutaNueva))) return R::mal("DESTINO_OCUPADO", "No se sobrescribe: " + rutaNueva);
			Descriptor desc;
			decryptWithKeyNew(desc.d, _cifrado.data(), reinterpret_cast<const char*>(MasterKeyPes21));
			if (!desc.d->data || desc.d->fileHeader->dataSize != _datos.size()) return R::mal("OPTION_ERROR", "Descriptor inconsistente");
			std::memcpy(desc.d->data, _datos.data(), _datos.size());
			int tam = 0;
			uint8_t* salida = encryptWithKeyNew(desc.d, &tam, reinterpret_cast<const char*>(MasterKeyPes21));
			if (!salida || tam <= 0) return R::mal("OPTION_ERROR", "No se pudo cifrar");
			{
				std::ofstream f(aRuta(rutaNueva), std::ios::binary);
				f.write(reinterpret_cast<const char*>(salida), tam);
				free(salida);
				if (!f.good()) return R::mal("ARCHIVO_ERROR", "No se pudo escribir " + rutaNueva);
			}
			// Verificación: releer y comprobar que los datos quedaron exactamente iguales.
			auto relectura = OptionFile::abrir(rutaNueva);
			if (!relectura.ok() || relectura.valor->_datos != _datos) {
				std::error_code ec; fs::remove(aRuta(rutaNueva), ec);
				return R::mal("OPTION_VERIFICACION_FALLIDA", "La relectura no coincide; se descartó el archivo");
			}
			return R::bien(sha256::deArchivo(rutaNueva));
		}
		catch (const std::exception& e) { return R::mal("OPTION_ERROR", e.what()); }
	}

}
