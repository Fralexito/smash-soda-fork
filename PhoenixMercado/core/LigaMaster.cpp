#include "LigaMaster.h"

#include <algorithm>
#include <cstdio>
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

		// --- Orden de formación (lo comparten el equipo del usuario y los de la IA) -------------------
		constexpr size_t kBytesOrden = 40;          // 40 índices de plantilla (relleno 0xff)
		constexpr size_t kOrdenARoles = 0x28;       // los 6 roles van justo después de los 40 bytes
		constexpr size_t kBytesRoles = 6;
		constexpr size_t kTitulares = 11;           // posiciones 0–10 titulares; después la banca (12 en este guardado) y las reservas

		// --- Bloques de alineación de los equipos de la IA (ESTRUCTURA-ML.md §10) -----------------------
		//  Un bloque de 600 B por equipo, en el orden de los bloques de equipo: [índice u32][ID option u32]
		//  [nombre del técnico]…[orden de formación 40 B en +0x220][roles 6 B]. El juego lee las PRIMERAS n
		//  entradas del orden (n = plantilla): si una es 0xff, en la pantalla Alineación sale un jugador
		//  en blanco con valoración 0 (lo que pasó con Guéhi en el Real Madrid, 8 oct 2026).
		constexpr size_t kTamBloqueAli = 600;
		constexpr size_t kAliOfsEquipo = 4;
		constexpr size_t kAliOfsOrden = 0x220;
		constexpr int kMinBloquesAli = 528;         // al menos todos los clubes y selecciones

		struct BloquesAli { size_t inicio = 0; int cantidad = 0; };

		BloquesAli buscarBloquesAli(const Datos& d) {
			for (size_t s = kFinBloques; s + 4 * kTamBloqueAli <= d.size(); s += 4) {
				if (u32(d, s) != 0 || u32(d, s + kTamBloqueAli) != 1 || u32(d, s + 2 * kTamBloqueAli) != 2 || u32(d, s + 3 * kTamBloqueAli) != 3)
					continue;
				int n = 0;
				while (s + size_t(n + 1) * kTamBloqueAli <= d.size() && u32(d, s + size_t(n) * kTamBloqueAli) == uint32_t(n)) n++;
				if (n < kMinBloquesAli) continue;
				// Confirmación: casi todos los bloques llevan el ID option de su equipo (el del usuario no).
				int coinciden = 0;
				const int revisar = std::min(n, kNumEquipos);
				for (int k = 0; k < revisar; k++)
					coinciden += u32(d, s + size_t(k) * kTamBloqueAli + kAliOfsEquipo) == u32(d, baseEquipo(k) + kOfsIdOption);
				if (coinciden * 10 >= revisar * 9) return { s, n };
			}
			return {};
		}

		/// Alineación de un equipo de la IA: las primeras n entradas del orden (n = plantilla) y los roles.
		struct AliIA { size_t ofs = 0; std::vector<uint8_t> orden; std::array<uint8_t, 6> roles{}; };

		Resultado<AliIA> leerAliIA(const Datos& d, const BloquesAli& b, int k) {
			using R = Resultado<AliIA>;
			const std::string eq = "equipo " + std::to_string(k);
			if (!b.cantidad) return R::mal("ALINEACIONES_IA_NO_HALLADAS", "No se encontró el arreglo de alineaciones de la IA");
			if (k < 0 || k >= b.cantidad) return R::mal("SIN_ALINEACION_IA", "El " + eq + " no tiene bloque de alineación");
			const size_t s = b.inicio + size_t(k) * kTamBloqueAli;
			if (u32(d, s + kAliOfsEquipo) != u32(d, baseEquipo(k) + kOfsIdOption))
				return R::mal("ALINEACION_IA_NO_COINCIDE", "El bloque de alineación no corresponde al " + eq);
			const size_t n = leerPlantilla(d, k).size();
			AliIA a;
			a.ofs = s + kAliOfsOrden;
			uint64_t visto = 0;
			for (size_t i = 0; i < n; i++) {
				const uint8_t v = d[a.ofs + i];
				if (v >= n || (visto >> v) & 1) return R::mal("ALINEACION_IA_INVALIDA", "La alineación del " + eq + " no es válida: no se toca");
				visto |= uint64_t(1) << v;
				a.orden.push_back(v);
			}
			for (size_t i = 0; i < kBytesRoles; i++) {
				a.roles[i] = d[a.ofs + kOrdenARoles + i];
				if (n && a.roles[i] >= n) return R::mal("ALINEACION_IA_INVALIDA", "Roles del " + eq + " fuera de la plantilla: no se toca");
			}
			return R::bien(std::move(a));
		}

		/// Escribe en formato compacto (como lo deja el juego): n entradas y 0xff hasta 40; luego los roles.
		void escribirAliIA(Datos& d, const AliIA& a) {
			for (size_t i = 0; i < kBytesOrden; i++) d[a.ofs + i] = i < a.orden.size() ? a.orden[i] : 0xff;
			for (size_t i = 0; i < kBytesRoles; i++) d[a.ofs + kOrdenARoles + i] = a.roles[i];
		}

		/// Quita `idx` del orden y de los roles. Con sustituto (idxS >= 0), este ocupa el puesto del que se va y
		/// deja libre el suyo. Los índices mayores que idx bajan uno (la plantilla se compacta igual).
		bool quitarDeOrden(std::vector<uint8_t>& orden, std::array<uint8_t, 6>& roles, int idx, int idxS, std::string& porque) {
			auto itH = std::find(orden.begin(), orden.end(), uint8_t(idx));
			if (itH == orden.end()) { porque = "el jugador no está en el orden de formación"; return false; }
			const size_t posH = size_t(itH - orden.begin());
			if (idxS >= 0) {
				auto itS = std::find(orden.begin(), orden.end(), uint8_t(idxS));
				if (itS == orden.end()) { porque = "el sustituto no está en el orden de formación"; return false; }
				const size_t posS = size_t(itS - orden.begin());
				orden[posH] = uint8_t(idxS);
				orden.erase(orden.begin() + posS);
			}
			else orden.erase(orden.begin() + posH);
			for (auto& v : orden) if (v > idx) v--;
			for (auto& v : roles) {
				if (v == idx) { if (idxS < 0) { porque = "el jugador tiene un rol y no hay sustituto"; return false; } v = uint8_t(idxS); }
				if (v > idx) v--;
			}
			return true;
		}

		/// ¿Hace falta sustituto? Solo si es TITULAR (puestos 0–10: cada uno es un sitio de la formación) o tiene un rol.
		/// Así lo hace el juego (guardados del 8 oct): si se va un titular, otro ocupa su puesto; si se va un suplente o
		/// una reserva, los de atrás simplemente suben (la banca y las reservas son una lista, no sitios en la cancha).
		/// No depende del tamaño de la banca (en este guardado es de 12; en otros puede ser de 7).
		bool necesitaSustituto(const std::vector<uint8_t>& orden, const std::array<uint8_t, 6>& roles, int idx) {
			const auto it = std::find(orden.begin(), orden.end(), uint8_t(idx));
			if (it != orden.end() && size_t(it - orden.begin()) < kTitulares) return true;
			return std::any_of(roles.begin(), roles.end(), [&](uint8_t v) { return v == idx; });
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

	Resultado<uint16_t> GuardadoLM::moverEntreIA(int origen, int destino, uint32_t pid, uint16_t dorsal, uint32_t pidSustituto) {
		using R = Resultado<uint16_t>;
		try {
			if (origen < 0 || origen >= kNumEquipos || destino < 0 || destino >= kNumEquipos)
				return R::mal("EQUIPO_INVALIDO", std::to_string(origen) + " → " + std::to_string(destino));
			if (origen == destino) return R::mal("MISMO_EQUIPO", std::to_string(origen));
			if (pid == 0) return R::mal("JUGADOR_INVALIDO", "pid 0");
			if (pidSustituto == pid) return R::mal("SUSTITUTO_INVALIDO", "El sustituto es el mismo jugador");
			if (esEquipoUsuario(origen) || esEquipoUsuario(destino))
				return R::mal("EQUIPO_DEL_USUARIO", "El equipo del usuario guarda tablas, alineación y listas extra: no se edita por el camino IA↔IA");

			auto po = leerPlantilla(_datos, origen);
			auto pd = leerPlantilla(_datos, destino);
			if (_datos[baseEquipo(origen) + kOfsContador] != po.size() || _datos[baseEquipo(destino) + kOfsContador] != pd.size())
				return R::mal("ML_INCONSISTENTE", "El contador de plantilla no coincide con la lista (¿guardado dañado?)");

			int idx = -1, idxS = -1;
			for (size_t i = 0; i < po.size(); i++) { if (po[i].pid == pid) idx = int(i); if (pidSustituto && po[i].pid == pidSustituto) idxS = int(i); }
			if (idx < 0) return R::mal("JUGADOR_NO_ESTA", "El jugador no está en el equipo de origen");
			if (pidSustituto && idxS < 0) return R::mal("SUSTITUTO_NO_ESTA", "El sustituto no está en el equipo de origen");
			if (std::any_of(pd.begin(), pd.end(), [&](const Plaza& p) { return p.pid == pid; }))
				return R::mal("JUGADOR_YA_EN_DESTINO", "Ya está en la plantilla de destino");
			if (pd.size() >= size_t(kMaxPlantilla)) return R::mal("PLANTILLA_LLENA", "El destino ya tiene 40 jugadores");

			// Alineaciones de los dos equipos (la pantalla Alineación las lee: tienen que seguir a la plantilla).
			const auto bloques = buscarBloquesAli(_datos);
			auto ao = leerAliIA(_datos, bloques, origen);
			if (!ao.ok()) return R::mal(ao.error.codigo, ao.error.detalle);
			auto ad = leerAliIA(_datos, bloques, destino);
			if (!ad.ok()) return R::mal(ad.error.codigo, ad.error.detalle);
			if (idxS < 0 && necesitaSustituto(ao.valor->orden, ao.valor->roles, idx))
				return R::mal("FALTA_SUSTITUTO", "Es titular o tiene un rol en su equipo: hace falta un sustituto");

			auto libre = [&](uint16_t d) {
				return d >= 1 && d <= 99 && std::none_of(pd.begin(), pd.end(), [&](const Plaza& p) { return p.dorsal == d; });
			};
			if (!libre(dorsal)) {
				dorsal = 99;
				while (dorsal > 1 && !libre(dorsal)) dorsal--;
				if (!libre(dorsal)) return R::mal("SIN_DORSAL", "No queda ningún dorsal libre en el destino");
			}

			// --- Todo cuadra: se trabaja sobre una copia y solo al final se adopta (todo o nada) ---------
			AliIA nuevaO = *ao.valor, nuevaD = *ad.valor;
			std::string porque;
			if (!quitarDeOrden(nuevaO.orden, nuevaO.roles, idx, idxS, porque)) return R::mal("ALINEACION_IA_INVALIDA", porque);
			nuevaD.orden.push_back(uint8_t(pd.size()));       // el que llega: al final de la plantilla y última reserva

			Plaza movida = po[size_t(idx)];
			po.erase(po.begin() + idx);
			movida.dorsal = dorsal;
			pd.push_back(movida);

			Datos d = _datos;
			escribirPlantilla(d, origen, po);
			escribirPlantilla(d, destino, pd);
			escribirAliIA(d, nuevaO);
			escribirAliIA(d, nuevaD);
			_datos.swap(d);
			return R::bien(dorsal);
		}
		catch (const std::exception& e) { return R::mal("ML_ERROR", e.what()); }
	}

	Resultado<uint32_t> GuardadoLM::sugerirSustituto(int k, uint32_t pid, const std::function<int(uint32_t)>& posicionDe) const {
		using R = Resultado<uint32_t>;
		try {
			if (k < 0 || k >= kNumEquipos) return R::mal("EQUIPO_INVALIDO", std::to_string(k));
			const auto pl = leerPlantilla(_datos, k);
			int idx = -1;
			for (size_t i = 0; i < pl.size(); i++) if (pl[i].pid == pid) idx = int(i);
			if (idx < 0) return R::mal("JUGADOR_NO_ESTA", "El jugador no está en ese equipo");

			std::vector<uint8_t> orden;
			std::array<uint8_t, 6> roles{};
			if (esEquipoUsuario(k)) {
				auto a = alineacionDe(k);
				if (!a.ok()) return R::mal(a.error.codigo, a.error.detalle);
				orden = a.valor->orden; roles = a.valor->roles;
			}
			else {
				auto a = leerAliIA(_datos, buscarBloquesAli(_datos), k);
				if (!a.ok()) return R::mal(a.error.codigo, a.error.detalle);
				orden = a.valor->orden; roles = a.valor->roles;
			}
			if (!necesitaSustituto(orden, roles, idx)) return R::bien(0u);

			const size_t posH = size_t(std::find(orden.begin(), orden.end(), uint8_t(idx)) - orden.begin());
			// Grupo fino y línea de cada posición: PT | DC | LI-LD | MCD-MC | II-ID | MP | EI-ED | SD-DC (delantero)
			//                                        línea: 0 portero · 1 defensa · 2 medio · 3 ataque
			auto grupo = [](int c) { static const int g[13] = { 0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 7 }; return c < 0 || c > 12 ? -1 : g[c]; };
			auto linea = [](int c) { return c < 0 ? -1 : c == 0 ? 0 : c <= 3 ? 1 : c <= 8 ? 2 : 3; };
			const int cH = posicionDe ? posicionDe(pid) : -1;
			const bool esPortero = cH == 0 || (cH < 0 && posH == 0);   // el puesto 0 de la formación es siempre el portero

			// Candidatos: todos los que NO son titulares, empezando por el final de la lista (las reservas) y
			// subiendo hacia la banca. Así no hace falta saber de cuántos es la banca (7 o 12).
			std::vector<size_t> cand;
			for (size_t p = orden.size(); p-- > kTitulares;) if (p != posH) cand.push_back(p);

			int mejor = -1; size_t elegido = 0;
			for (size_t p : cand) {
				const int c = posicionDe ? posicionDe(pl[orden[p]].pid) : -1;
				int puntos;
				if (esPortero) { if (c != 0) continue; puntos = 3; }
				else {
					if (c == 0) continue;                                  // un portero nunca cubre a un jugador de campo
					puntos = cH < 0 || c < 0 ? 0 : c == cH ? 3 : grupo(c) == grupo(cH) ? 2 : linea(c) == linea(cH) ? 1 : 0;
				}
				if (puntos > mejor) { mejor = puntos; elegido = p; }
			}
			if (mejor < 0) return R::mal("SIN_SUSTITUTO", esPortero ? "No hay otro portero para cubrir el puesto" : "No hay ningún jugador libre para cubrir el puesto");
			return R::bien(pl[orden[elegido]].pid);
		}
		catch (const std::exception& e) { return R::mal("ML_ERROR", e.what()); }
	}

	// =========================================================================
	//  Equipo del usuario: alineación (orden + roles + K) y venta a la IA
	// =========================================================================
	namespace {
		constexpr size_t kTamK = 16;               // [flag u32][reg u32][pid u32][0 u32]
		constexpr uint32_t kFlagLibreMin = 0xc0;    // primer registro libre de K (0xc0 con 25 jugadores, 0xc1 con 26…)
		constexpr uint32_t kFlagFinK = 0xc7;        // los que siguen al primer libre
		constexpr int kMaxRegistrosTabla = 200;     // tope de seguridad al contar registros de una tabla
		constexpr size_t kTablasEsperadas = 12;     // A B C D E F G H M I J + L (ESTRUCTURA-ML.md §4)

		bool registroVacio(const Datos& d, size_t ofsReg) {
			const uint32_t reg = u32(d, ofsReg), pid = u32(d, ofsReg + 4);
			return pid == 0 && (reg == 0 || reg == 0xffff || reg == kRegVacio);
		}

		/// Dirección del campo `reg` del registro i de una tabla alineada.
		long long regDe(const TablaAlineada& t, long long i) { return (long long)t.ofsReg0 + t.dir * i * (long long)t.stride; }
		/// Un registro empieza 4 bytes antes de `reg` ([x][reg][pid]…) y mide `stride`.
		long long inicioDe(const TablaAlineada& t, long long i) { return regDe(t, i) - 4; }

		/// Cuántos registros usados tiene la tabla (hasta el primer vacío). -1 = no termina (tabla dañada).
		int usadosEn(const Datos& d, const TablaAlineada& t) {
			for (int n = 0; n <= kMaxRegistrosTabla; n++) {
				const long long r = regDe(t, n);
				if (r < (long long)kFinBloques || size_t(r) + 8 > d.size()) return -1;
				if (registroVacio(d, size_t(r))) return n;
			}
			return -1;
		}

		/// Quita el registro `idx` compactando hacia él; el último usado queda como el primer vacío (plantilla).
		/// Igual que `quitar_registro` / `L_quitar` del prototipo probado en el juego.
		bool quitarRegistro(Datos& d, const TablaAlineada& t, int idx, std::string& porque) {
			const int n = usadosEn(d, t);
			if (n < 0) { porque = "tabla sin fin"; return false; }
			if (idx < 0 || idx >= n) { porque = "índice fuera de la tabla (" + std::to_string(idx) + "/" + std::to_string(n) + ")"; return false; }
			const long long lo = std::min(inicioDe(t, 0), inicioDe(t, n)), hi = std::max(inicioDe(t, 0), inicioDe(t, n)) + (long long)t.stride;
			if (lo < (long long)kFinBloques || size_t(hi) > d.size()) { porque = "tabla fuera del archivo"; return false; }
			std::vector<uint8_t> vacio(d.begin() + inicioDe(t, n), d.begin() + inicioDe(t, n) + (long long)t.stride);
			for (int i = idx; i < n - 1; i++)
				std::memmove(&d[size_t(inicioDe(t, i))], &d[size_t(inicioDe(t, i + 1))], t.stride);
			std::memcpy(&d[size_t(inicioDe(t, n - 1))], vacio.data(), t.stride);
			return true;
		}

		/// ¿`orden[0..n)` es una permutación de 0..n-1 y lo que sigue hasta 40 es 0xff?
		bool esOrdenValido(const Datos& d, size_t ofs, size_t n) {
			if (n == 0 || n > kBytesOrden || ofs + kBytesOrden > d.size()) return false;
			uint64_t visto = 0;
			for (size_t i = 0; i < n; i++) {
				const uint8_t v = d[ofs + i];
				if (v >= n || (visto >> v) & 1) return false;
				visto |= uint64_t(1) << v;
			}
			for (size_t i = n; i < kBytesOrden; i++) if (d[ofs + i] != 0xff) return false;
			return true;
		}
	}

	Resultado<Alineacion> GuardadoLM::alineacionDe(int k) const {
		using R = Resultado<Alineacion>;
		if (k < 0 || k >= kNumEquipos) return R::mal("EQUIPO_INVALIDO", std::to_string(k));
		const auto pl = leerPlantilla(_datos, k);
		const size_t n = pl.size();
		if (n < 11) return R::mal("PLANTILLA_CORTA", "Menos de 11 jugadores");
		const Datos& d = _datos;

		// 1) Candidatos a `orden`: ventana de 40 B con permutación de 0..n-1 + relleno 0xff y 6 roles < n después.
		std::vector<size_t> candidatos;
		for (size_t o = kFinBloques; o + kBytesOrden + kOrdenARoles + kBytesRoles <= d.size(); o++) {
			if (n < kBytesOrden && d[o + n] != 0xff) continue;
			if (d[o] >= n) continue;
			if (!esOrdenValido(d, o, n)) continue;
			bool rolesOk = true;
			for (size_t i = 0; i < kBytesRoles && rolesOk; i++) rolesOk = d[o + kOrdenARoles + i] < n;
			if (rolesOk) candidatos.push_back(o);
		}
		if (candidatos.empty()) return R::mal("ALINEACION_NO_HALLADA", "No se encontró el orden de formación del equipo");

		// 2) Para cada candidato, la lista K debe ser su espejo: K[i] = plantilla[orden[i]] para los n, y luego un libre.
		std::vector<Alineacion> halladas;
		for (size_t o : candidatos) {
			std::vector<uint8_t> orden(d.begin() + o, d.begin() + o + n);
			uint8_t patron[8];
			for (int i = 0; i < 4; i++) { patron[i] = uint8_t(pl[orden[0]].reg >> (8 * i)); patron[4 + i] = uint8_t(pl[orden[0]].pid >> (8 * i)); }
			const uint8_t* base = d.data();
			size_t h = kFinBloques + 4;
			while (h + 8 <= d.size()) {
				const void* f = std::memchr(base + h, patron[0], d.size() - 8 + 1 - h);
				if (!f) break;
				h = size_t(static_cast<const uint8_t*>(f) - base);
				if (std::memcmp(base + h, patron, 8) == 0 && u32(d, h + 8) == 0) {
					const size_t k0 = h - 4;
					bool ok = k0 + kTamK * (n + 2) <= d.size();
					for (size_t i = 0; i < n && ok; i++) {
						const size_t r = k0 + kTamK * i;
						ok = u32(d, r + 4) == pl[orden[i]].reg && u32(d, r + 8) == pl[orden[i]].pid && u32(d, r + 12) == 0;
					}
					if (ok) {
						const size_t libre = k0 + kTamK * n;
						ok = u32(d, libre + 4) == 0xffff && u32(d, libre + 8) == 0 && u32(d, libre) >= kFlagLibreMin
							&& u32(d, libre + kTamK + 4) == 0xffff && u32(d, libre + kTamK + 8) == 0;
					}
					if (ok) {
						Alineacion a;
						a.ofsOrden = o; a.ofsRoles = o + kOrdenARoles; a.ofsK = k0; a.orden = orden;
						for (size_t i = 0; i < kBytesRoles; i++) a.roles[i] = d[a.ofsRoles + i];
						for (size_t i = 0; i < n; i++) a.flagsK.push_back(u32(d, k0 + kTamK * i));
						a.flagLibreK = u32(d, k0 + kTamK * n);
						halladas.push_back(std::move(a));
					}
				}
				h++;
			}
		}
		if (halladas.empty()) return R::mal("ALINEACION_NO_HALLADA", "Orden de formación sin lista K que lo refleje");
		if (halladas.size() > 1) return R::mal("ALINEACION_AMBIGUA", std::to_string(halladas.size()) + " candidatas; no se toca nada");
		return R::bien(std::move(halladas[0]));
	}

	Resultado<uint16_t> GuardadoLM::moverUsuarioAIA(int kUsuario, int kDestino, uint32_t pid, uint16_t dorsal, uint32_t pidSustituto) {
		using R = Resultado<uint16_t>;
		try {
			if (kUsuario < 0 || kUsuario >= kNumEquipos || kDestino < 0 || kDestino >= kNumEquipos)
				return R::mal("EQUIPO_INVALIDO", std::to_string(kUsuario) + " → " + std::to_string(kDestino));
			if (kUsuario == kDestino) return R::mal("MISMO_EQUIPO", std::to_string(kUsuario));
			if (pid == 0) return R::mal("JUGADOR_INVALIDO", "pid 0");
			if (pidSustituto == pid) return R::mal("SUSTITUTO_INVALIDO", "El sustituto es el mismo jugador");

			// --- Leer y validar todo ANTES de tocar un byte -------------------------------------
			const auto pu = leerPlantilla(_datos, kUsuario);
			const auto pd = leerPlantilla(_datos, kDestino);
			if (_datos[baseEquipo(kUsuario) + kOfsContador] != pu.size() || _datos[baseEquipo(kDestino) + kOfsContador] != pd.size())
				return R::mal("ML_INCONSISTENTE", "El contador de plantilla no coincide con la lista (¿guardado dañado?)");
			const int n = int(pu.size());
			if (n <= 11) return R::mal("PLANTILLA_MINIMA", "El usuario se quedaría sin 11 jugadores");

			int idx = -1, idxS = -1;
			for (int i = 0; i < n; i++) { if (pu[i].pid == pid) idx = i; if (pidSustituto && pu[i].pid == pidSustituto) idxS = i; }
			if (idx < 0) return R::mal("JUGADOR_NO_ESTA", "El jugador no está en el equipo del usuario");
			if (pidSustituto && idxS < 0) return R::mal("SUSTITUTO_NO_ESTA", "El sustituto no está en el equipo del usuario");
			if (std::any_of(pd.begin(), pd.end(), [&](const Plaza& p) { return p.pid == pid; }))
				return R::mal("JUGADOR_YA_EN_DESTINO", "Ya está en la plantilla de destino");
			if (pd.size() >= size_t(kMaxPlantilla)) return R::mal("PLANTILLA_LLENA", "El destino ya tiene 40 jugadores");

			// Tablas alineadas del usuario: tienen que ser exactamente las 12 conocidas y cuadrar con TODA la plantilla.
			auto tablas = tablasDe(kUsuario);
			if (tablas.empty()) return R::mal("NO_ES_USUARIO", "Ese equipo no tiene tablas alineadas: no es el equipo del usuario");
			if (!tablasDe(kDestino).empty()) return R::mal("DESTINO_ES_USUARIO", "El destino también es un equipo del usuario");
			auto ali = alineacionDe(kUsuario);
			if (!ali.ok()) return R::mal(ali.error.codigo, ali.error.detalle);
			Alineacion a = *ali.valor;
			// Alineación del equipo de la IA que recibe: el nuevo tiene que entrar también ahí (si no, sale «en blanco»).
			auto aliDestino = leerAliIA(_datos, buscarBloquesAli(_datos), kDestino);
			if (!aliDestino.ok()) return R::mal(aliDestino.error.codigo, aliDestino.error.detalle);

			// En cada tabla el jugador se busca por su (reg, pid): los fichajes recientes no están en su índice de
			// plantilla sino al final (tablas C y D: 24 del primer equipo, 32 juveniles y luego los fichados).
			struct Objetivo { TablaAlineada t; int j; int usados; int repetidos; };
			std::vector<Objetivo> objetivos;
			const Plaza& quien = pu[size_t(idx)];
			for (const auto& t : tablas) {
				if (t.ofsReg0 == a.ofsK + 4) continue;                   // la lista K no es una tabla: se trata aparte
				if (std::any_of(objetivos.begin(), objetivos.end(), [&](const Objetivo& o) { return o.t.ofsReg0 == t.ofsReg0 && o.t.stride == t.stride && o.t.dir == t.dir; })) continue;
				const int usados = usadosEn(_datos, t);
				if (usados < n) return R::mal("TABLA_DANADA", "Tabla de paso " + std::to_string(t.stride) + " con menos registros que la plantilla");
				int j = -1, rep = 0;
				for (int i = 0; i < usados; i++) {
					const size_t r = size_t(regDe(t, i));
					if (u32(_datos, r) == quien.reg && u32(_datos, r + 4) == quien.pid) { if (j < 0) j = i; else rep++; }
				}
				if (j < 0) return R::mal("JUGADOR_NO_EN_TABLA", "El jugador no aparece en la tabla de paso " + std::to_string(t.stride) + (t.dir < 0 ? "↓" : ""));
				objetivos.push_back({ t, j, usados, rep });
			}
			// Tienen que estar las 12 tablas conocidas (ESTRUCTURA-ML.md §4); si hay más (p. ej. una que el juego
			// rellena al avanzar la temporada) también se compactan, porque van en el mismo orden que la plantilla.
			{
				struct Firma { size_t stride; int dir; int veces; };
				Firma firmas[] = { {24,+1,1}, {24,-1,1}, {44,+1,1}, {368,+1,1}, {192,+1,1}, {52,+1,1}, {108,+1,3}, {5628,+1,1}, {48,+1,1}, {16,+1,1} };
				std::string faltan;
				for (const auto& f : firmas) {
					const int hay = int(std::count_if(objetivos.begin(), objetivos.end(), [&](const Objetivo& o) { return o.t.stride == f.stride && o.t.dir == f.dir; }));
					if (hay < f.veces) faltan += " " + std::to_string(f.stride) + (f.dir < 0 ? "↓" : "");
				}
				if (!faltan.empty()) return R::mal("TABLAS_INESPERADAS", "Faltan tablas conocidas del equipo del usuario:" + faltan);
				if (objetivos.size() < kTablasEsperadas) return R::mal("TABLAS_INESPERADAS", "Solo " + std::to_string(objetivos.size()) + " tablas");
			}
			_ultimoInforme.clear();
			for (const auto& o : objetivos) {
				_ultimoInforme += "tabla paso " + std::to_string(o.t.stride) + (o.t.dir < 0 ? "↓" : "") + " en 0x";
				char hex[32]; std::snprintf(hex, sizeof hex, "%zx", o.t.ofsReg0); _ultimoInforme += hex;
				_ultimoInforme += ": registro " + std::to_string(o.j) + " de " + std::to_string(o.usados);
				if (o.repetidos) _ultimoInforme += " (quedan " + std::to_string(o.repetidos) + " repetidos sin tocar)";
				_ultimoInforme += "\n";
			}

			// Alineación: dónde está el que se va y, si hace falta, el sustituto.
			int posH = -1, posS = -1;
			for (int i = 0; i < n; i++) { if (a.orden[size_t(i)] == idx) posH = i; if (idxS >= 0 && a.orden[size_t(i)] == idxS) posS = i; }
			if (posH < 0) return R::mal("ALINEACION_INCOHERENTE", "El jugador no aparece en el orden de formación");
			if (idxS >= 0 && posS < 0) return R::mal("ALINEACION_INCOHERENTE", "El sustituto no aparece en el orden de formación");
			if (idxS < 0 && size_t(posH) < kTitulares) return R::mal("FALTA_SUSTITUTO", "El jugador es titular: hace falta un sustituto");
			if (idxS < 0 && std::any_of(a.roles.begin(), a.roles.end(), [&](uint8_t v) { return v == idx; }))
				return R::mal("FALTA_SUSTITUTO", "El jugador tiene un rol (capitán o lanzador): hace falta un sustituto");

			auto libre = [&](uint16_t x) { return x >= 1 && x <= 99 && std::none_of(pd.begin(), pd.end(), [&](const Plaza& p) { return p.dorsal == x; }); };
			if (!libre(dorsal)) {
				dorsal = 99;
				while (dorsal > 1 && !libre(dorsal)) dorsal--;
				if (!libre(dorsal)) return R::mal("SIN_DORSAL", "No queda ningún dorsal libre en el destino");
			}

			// --- Todo cuadra: se trabaja sobre una copia y solo al final se adopta ----------------
			Datos d = _datos;
			std::string porque;
			for (const auto& o : objetivos)
				if (!quitarRegistro(d, o.t, o.j, porque)) return R::mal("TABLA_DANADA", porque);

			// Orden de formación y roles (prototipo v6): el sustituto ocupa el puesto; se quita de su sitio; los índices > idx bajan 1.
			std::vector<uint8_t> orden = a.orden;
			std::vector<uint32_t> flags = a.flagsK;
			if (idxS >= 0) {
				orden[size_t(posH)] = uint8_t(idxS);
				orden.erase(orden.begin() + posS);
				flags.erase(flags.begin() + posS);
			}
			else {
				orden.erase(orden.begin() + posH);
				flags.erase(flags.begin() + posH);
			}
			for (auto& v : orden) if (v > idx) v--;
			std::array<uint8_t, 6> roles = a.roles;
			for (auto& v : roles) { if (v == idx) v = uint8_t(idxS); if (v > idx) v--; }
			for (size_t i = 0; i < kBytesOrden; i++) d[a.ofsOrden + i] = i < orden.size() ? orden[i] : 0xff;
			for (size_t i = 0; i < kBytesRoles; i++) d[a.ofsRoles + i] = roles[i];

			// Plantilla y dorsales del usuario; destino al final con su dorsal.
			std::vector<Plaza> pu2 = pu;
			Plaza movida = pu2[size_t(idx)];
			pu2.erase(pu2.begin() + idx);
			escribirPlantilla(d, kUsuario, pu2);
			std::vector<Plaza> pd2 = pd;
			movida.dorsal = dorsal;
			pd2.push_back(movida);
			escribirPlantilla(d, kDestino, pd2);
			AliIA nuevaD = *aliDestino.valor;
			nuevaD.orden.push_back(uint8_t(pd.size()));      // el que llega: última reserva (el técnico de la IA decide después)
			escribirAliIA(d, nuevaD);

			// Lista K = espejo del nuevo orden con los flags que quedaron; luego el libre (flag anterior − 1, mínimo 0xc0) y 0xc7.
			for (size_t i = 0; i < orden.size(); i++) {
				const size_t r = a.ofsK + kTamK * i;
				p32(d, r, flags[i]); p32(d, r + 4, pu2[orden[i]].reg); p32(d, r + 8, pu2[orden[i]].pid); p32(d, r + 12, 0);
			}
			const uint32_t flagLibre = a.flagLibreK > kFlagLibreMin ? a.flagLibreK - 1 : kFlagLibreMin;
			size_t r = a.ofsK + kTamK * orden.size();
			p32(d, r, flagLibre); p32(d, r + 4, 0xffff); p32(d, r + 8, 0); p32(d, r + 12, 0);
			r += kTamK;
			p32(d, r, kFlagFinK); p32(d, r + 4, 0xffff); p32(d, r + 8, 0); p32(d, r + 12, 0);

			_datos.swap(d);
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
