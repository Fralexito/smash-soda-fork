#include "LigaMaster.h"

#include <algorithm>
#include <cstring>

namespace mercado::lm {

	namespace {
		// --- Distribución de los DATOS (ver liga-master/ESTRUCTURA-ML.md §2) ---
		constexpr size_t kInicioEquipos = 0x50;
		constexpr size_t kTamBloque = 1680;
		constexpr size_t kFinBloques = kInicioEquipos + kTamBloque * kNumEquipos;   // 0x11f210
		constexpr size_t kOfsNombre = 4, kMaxNombre = 64;
		constexpr size_t kOfsIdOption = 656;
		constexpr size_t kOfsPlantilla = 0x14c;                      // dentro del bloque
		constexpr size_t kOfsDorsales = kOfsPlantilla + 4 + 0x14a;   // 40 × u16
		constexpr size_t kOfsContador = kOfsPlantilla + 4 + 0x2d6;   // 1 byte
		constexpr uint32_t kRegVacio = 65535;                        // relleno de la lista: (65535, 0)

		using Datos = std::vector<uint8_t>;

		uint32_t u32(const Datos& d, size_t o) {
			return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (uint32_t(d[o + 3]) << 24);
		}
		uint16_t u16(const Datos& d, size_t o) { return uint16_t(d[o] | (d[o + 1] << 8)); }
		void p32(Datos& d, size_t o, uint32_t v) { for (int i = 0; i < 4; i++) d[o + i] = uint8_t(v >> (8 * i)); }
		void p16(Datos& d, size_t o, uint16_t v) { d[o] = uint8_t(v); d[o + 1] = uint8_t(v >> 8); }

		size_t baseEquipo(int k) { return kInicioEquipos + kTamBloque * size_t(k); }

		std::vector<Plaza> leerPlantilla(const Datos& d, int k) {
			std::vector<Plaza> out;
			const size_t s = baseEquipo(k) + kOfsPlantilla;
			for (int i = 0; i < kMaxPlantilla; i++) {
				const uint32_t reg = u32(d, s + 8 * size_t(i)), pid = u32(d, s + 8 * size_t(i) + 4);
				if (reg == kRegVacio && pid == 0) break;
				out.push_back({ reg, pid, u16(d, baseEquipo(k) + kOfsDorsales + 2 * size_t(i)) });
			}
			return out;
		}

		/// Escribe la plantilla completa: los libres se rellenan con (65535, 0) y dorsal 0.
		void escribirPlantilla(Datos& d, int k, const std::vector<Plaza>& plazas) {
			const size_t s = baseEquipo(k) + kOfsPlantilla;
			for (int i = 0; i < kMaxPlantilla; i++) {
				const bool hay = size_t(i) < plazas.size();
				p32(d, s + 8 * size_t(i), hay ? plazas[size_t(i)].reg : kRegVacio);
				p32(d, s + 8 * size_t(i) + 4, hay ? plazas[size_t(i)].pid : 0);
				p16(d, baseEquipo(k) + kOfsDorsales + 2 * size_t(i), hay ? plazas[size_t(i)].dorsal : 0);
			}
			d[baseEquipo(k) + kOfsContador] = uint8_t(plazas.size());
		}

		Resultado<bool> validarFormato(const Datos& d) {
			using R = Resultado<bool>;
			if (d.size() < kFinBloques) return R::mal("ML_FORMATO", "Archivo demasiado corto para una Liga Máster");
			// Cabecera de 80 B: 10, 0x50, …, 700, 700 (equipos). Si no coincide es otra versión: no se toca.
			if (u32(d, 0) != 10 || u32(d, 4) != kInicioEquipos || u32(d, 16) != kNumEquipos || u32(d, 20) != kNumEquipos)
				return R::mal("ML_FORMATO", "Cabecera desconocida (¿otra versión del juego o del parche?)");
			return R::bien(true);
		}
	}

	Resultado<GuardadoLM> GuardadoLM::abrir(const std::string& ruta) {
		using R = Resultado<GuardadoLM>;
		auto sobre = SobrePes::abrir(ruta, "ML");
		if (!sobre.ok()) return R::mal(sobre.error.codigo, sobre.error.detalle);
		GuardadoLM g;
		g._datos = sobre.valor->tomarDatos();
		auto v = validarFormato(g._datos);
		if (!v.ok()) return R::mal(v.error.codigo, v.error.detalle);
		g._sobre = std::move(*sobre.valor);
		return R::bien(std::move(g));
	}

	Resultado<GuardadoLM> GuardadoLM::desdeDatos(std::vector<uint8_t> datos) {
		using R = Resultado<GuardadoLM>;
		auto v = validarFormato(datos);
		if (!v.ok()) return R::mal(v.error.codigo, v.error.detalle);
		GuardadoLM g;
		g._datos = std::move(datos);
		return R::bien(std::move(g));
	}

	Resultado<EquipoLM> GuardadoLM::equipo(int k) const {
		using R = Resultado<EquipoLM>;
		if (k < 0 || k >= kNumEquipos) return R::mal("EQUIPO_INVALIDO", std::to_string(k));
		EquipoLM e;
		e.indice = k;
		const size_t b = baseEquipo(k);
		size_t n = 0;
		while (n < kMaxNombre && _datos[b + kOfsNombre + n]) n++;
		e.nombre.assign(reinterpret_cast<const char*>(&_datos[b + kOfsNombre]), n);
		e.idOption = u32(_datos, b + kOfsIdOption);
		e.plantilla = leerPlantilla(_datos, k);
		return R::bien(std::move(e));
	}

	std::vector<int> GuardadoLM::equiposDe(uint32_t pid) const {
		std::vector<int> out;
		for (int k = 0; k < kNumEquipos; k++) {
			const auto pl = leerPlantilla(_datos, k);
			if (std::any_of(pl.begin(), pl.end(), [&](const Plaza& p) { return p.pid == pid; })) out.push_back(k);
		}
		return out;
	}

	std::vector<TablaAlineada> GuardadoLM::tablasDe(int k) const {
		std::vector<TablaAlineada> out;
		if (k < 0 || k >= kNumEquipos) return out;
		const auto pl = leerPlantilla(_datos, k);
		if (pl.size() < 5) return out;                         // hacen falta 5 para verificar con seguridad

		struct Espec { size_t stride; int dir; };
		static constexpr Espec especs[] = {
			{ 24, +1 }, { 24, -1 }, { 44, +1 }, { 368, +1 }, { 192, +1 },
			{ 52, +1 }, { 108, +1 }, { 5628, +1 }, { 48, +1 }, { 16, +1 } };

		// Se busca el par (reg, pid) del primer jugador y, en cada acierto, se comprueban los 4 siguientes
		// a la distancia de cada tabla conocida. Solo se mira después de los bloques de equipo.
		uint8_t patron[8];
		for (int i = 0; i < 4; i++) { patron[i] = uint8_t(pl[0].reg >> (8 * i)); patron[4 + i] = uint8_t(pl[0].pid >> (8 * i)); }
		const uint8_t* base = _datos.data();
		const size_t n = _datos.size();
		size_t h = kFinBloques;
		while (h + 8 <= n) {
			const void* f = std::memchr(base + h, patron[0], n - 8 + 1 - h);
			if (!f) break;
			h = size_t(static_cast<const uint8_t*>(f) - base);
			if (std::memcmp(base + h, patron, 8) == 0) {
				for (const auto& e : especs) {
					bool ok = true;
					for (size_t i = 1; i <= 4 && ok; i++) {
						const long long pos = (long long)h + e.dir * (long long)(i * e.stride);
						if (pos < (long long)kFinBloques || size_t(pos) + 8 > n) { ok = false; break; }
						ok = u32(_datos, size_t(pos)) == pl[i].reg && u32(_datos, size_t(pos) + 4) == pl[i].pid;
					}
					if (ok) out.push_back({ h, e.stride, e.dir });
				}
			}
			h++;
		}
		return out;
	}

	Resultado<uint16_t> GuardadoLM::moverEntreIA(int origen, int destino, uint32_t pid, uint16_t dorsal) {
		using R = Resultado<uint16_t>;
		try {
			if (origen < 0 || origen >= kNumEquipos || destino < 0 || destino >= kNumEquipos)
				return R::mal("EQUIPO_INVALIDO", std::to_string(origen) + " → " + std::to_string(destino));
			if (origen == destino) return R::mal("MISMO_EQUIPO", std::to_string(origen));
			if (pid == 0) return R::mal("JUGADOR_INVALIDO", "pid 0");
			if (esEquipoUsuario(origen) || esEquipoUsuario(destino))
				return R::mal("EQUIPO_DEL_USUARIO", "El equipo del usuario guarda tablas, alineación y listas extra: no se edita por el camino IA↔IA");

			auto po = leerPlantilla(_datos, origen);
			auto pd = leerPlantilla(_datos, destino);
			if (_datos[baseEquipo(origen) + kOfsContador] != po.size() || _datos[baseEquipo(destino) + kOfsContador] != pd.size())
				return R::mal("ML_INCONSISTENTE", "El contador de plantilla no coincide con la lista (¿guardado dañado?)");

			auto it = std::find_if(po.begin(), po.end(), [&](const Plaza& p) { return p.pid == pid; });
			if (it == po.end()) return R::mal("JUGADOR_NO_ESTA", "El jugador no está en el equipo de origen");
			if (std::any_of(pd.begin(), pd.end(), [&](const Plaza& p) { return p.pid == pid; }))
				return R::mal("JUGADOR_YA_EN_DESTINO", "Ya está en la plantilla de destino");
			if (pd.size() >= size_t(kMaxPlantilla)) return R::mal("PLANTILLA_LLENA", "El destino ya tiene 40 jugadores");

			auto libre = [&](uint16_t d) {
				return d >= 1 && d <= 99 && std::none_of(pd.begin(), pd.end(), [&](const Plaza& p) { return p.dorsal == d; });
			};
			if (!libre(dorsal)) {
				dorsal = 99;
				while (dorsal > 1 && !libre(dorsal)) dorsal--;
				if (!libre(dorsal)) return R::mal("SIN_DORSAL", "No queda ningún dorsal libre en el destino");
			}

			Plaza movida = *it;
			po.erase(it);
			movida.dorsal = dorsal;
			pd.push_back(movida);

			// Todo validado: recién ahora se escribe (todo o nada).
			escribirPlantilla(_datos, origen, po);
			escribirPlantilla(_datos, destino, pd);
			return R::bien(dorsal);
		}
		catch (const std::exception& e) { return R::mal("ML_ERROR", e.what()); }
	}

	Resultado<std::string> GuardadoLM::guardarComo(const std::string& rutaNueva, const std::string& textoInfo) const {
		using R = Resultado<std::string>;
		if (!_sobre) return R::mal("SIN_SOBRE", "Este guardado vino de datos sueltos: no hay envoltura para cifrar");
		SobrePes sobre = *_sobre;   // copia: así el texto info cambiado no altera el original
		if (!textoInfo.empty()) sobre.ponerTextoInfo(textoInfo);
		return sobre.guardarComo(rutaNueva, _datos);
	}

}
