#include "LigaMaster.h"
#include "Alineacion.h"

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
		constexpr uint32_t kMarcaUsuario = 0xfffffff5;               // -11 en +656 del bloque: el equipo lo lleva el usuario (§17)
		constexpr uint32_t kMaxReg = 20000, kMaxPid = 1000000;       // plausibilidad de una ficha (reg: 16.422 en el universo; pid del option)
		constexpr uint32_t kPrefijoRegGenerado = 0xdb65;             // jugadores que CREA el juego (regens): reg = 0xdb65xxxx, pid > 126.000 (§17)
		/// ¿`reg` puede ser una ficha de jugador? (del universo del parche o creado por el juego)
		bool regPlausible(uint32_t reg) { return reg < kMaxReg || (reg >> 16) == kPrefijoRegGenerado; }

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
		// Orden de formación y roles: lógica compartida con el option file (Alineacion.h).
		constexpr size_t kBytesOrden = alineacion::kBytesOrden;      // 40 índices de plantilla (relleno 0xff)
		constexpr size_t kOrdenARoles = alineacion::kOrdenARoles;    // los 6 roles van justo después de los 40 bytes
		constexpr size_t kBytesRoles = alineacion::kBytesRoles;
		constexpr size_t kTitulares = alineacion::kTitulares;        // posiciones 0–10 titulares; después la banca (7 o 12) y las reservas
		using alineacion::quitarDeOrden;
		using alineacion::necesitaSustituto;

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

	Resultado<uint32_t> GuardadoLM::idOptionDe(int k) const {
		using R = Resultado<uint32_t>;
		if (k < 0 || k >= kNumEquipos) return R::mal("EQUIPO_INVALIDO", std::to_string(k));
		const uint32_t id = u32(_datos, baseEquipo(k) + kOfsIdOption);
		if (id != kMarcaUsuario) return R::bien(id);
		// Equipo del usuario: el ID real está en la cabecera de su tabla de contratos (paso 48): [índice][club = (id << 14) | k]…
		for (const auto& t : tablasDe(k)) {
			if (t.stride != 48) continue;
			const uint32_t club = u32(_datos, t.ofsReg0 - 28 + 4);
			if ((club & 0x3fff) == uint32_t(k)) return R::bien(club >> 14);
		}
		return R::mal("ID_USUARIO_NO_HALLADO", "El bloque lleva la marca de usuario (-11) pero no se halló su ID en los contratos");
	}

	Resultado<int> GuardadoLM::indicePorIdOption(uint32_t idOption) const {
		using R = Resultado<int>;
		int usuario = -1;
		for (int k = 0; k < kNumEquipos; k++) {
			const uint32_t id = u32(_datos, baseEquipo(k) + kOfsIdOption);
			if (id == idOption) return R::bien(k);
			if (id == kMarcaUsuario && usuario < 0) usuario = k;
		}
		if (usuario >= 0) {
			auto id = idOptionDe(usuario);
			if (id.ok() && *id.valor == idOption) return R::bien(usuario);
		}
		return R::mal("EQUIPO_NO_ESTA", "Ningún equipo de la Liga Máster tiene el ID " + std::to_string(idOption));
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

		// Se busca el par (reg, pid) del primer jugador y, en cada acierto, se comprueban los 4 siguientes a la distancia
		// de cada tabla conocida. Entre medio puede haber HUECOS (registros vacíos que deja el juego cuando se va un
		// jugador, §12) o registros de jugadores que ya no están (A2 hasta que el juego la rehace): se saltan, hasta 8.
		// Solo se mira después de los bloques de equipo.
		uint8_t patron[8];
		for (int i = 0; i < 4; i++) { patron[i] = uint8_t(pl[0].reg >> (8 * i)); patron[4 + i] = uint8_t(pl[0].pid >> (8 * i)); }
		const uint8_t* base = _datos.data();
		const size_t n = _datos.size();
		constexpr int kMaxHuecosAncla = 8;
		size_t h = kFinBloques;
		while (h + 8 <= n) {
			const void* f = std::memchr(base + h, patron[0], n - 8 + 1 - h);
			if (!f) break;
			h = size_t(static_cast<const uint8_t*>(f) - base);
			if (std::memcmp(base + h, patron, 8) == 0) {
				for (const auto& e : especs) {
					bool ok = true;
					int huecos = 0;
					size_t siguiente = 1;                 // jugador de la plantilla que toca encontrar
					for (long long r = 1; siguiente <= 4 && ok; r++) {
						const long long pos = (long long)h + e.dir * (long long)(size_t(r) * e.stride);
						if (pos < (long long)kFinBloques || size_t(pos) + 8 > n) { ok = false; break; }
						const uint32_t reg = u32(_datos, size_t(pos)), pid = u32(_datos, size_t(pos) + 4);
						if (reg == pl[siguiente].reg && pid == pl[siguiente].pid) { siguiente++; continue; }
						// Se salta un hueco (vacío) o la ficha de alguien que ya no está en la plantilla (A2 conserva al vendido
						// hasta que el juego la rehace). Cualquier otra cosa (otro jugador de la plantilla, datos sueltos) descarta.
						const bool vacio = pid == 0 && (reg == 0 || reg == 0xffff || reg == kRegVacio);
						const bool ajeno = !vacio && pid && pid < kMaxPid && regPlausible(reg)
							&& std::none_of(pl.begin(), pl.end(), [&](const Plaza& p) { return p.pid == pid || p.reg == reg; });
						if ((vacio || ajeno) && ++huecos <= kMaxHuecosAncla) continue;
						ok = false;
					}
					if (ok) out.push_back({ h, e.stride, e.dir, e.stride == 48 ? size_t(28) : size_t(4) });
				}
			}
			h++;
		}
		return out;
	}

	namespace {
		/// Candidatos de dorsal: el pedido, el que tiene en su club de origen y el de sus otros equipos (selección), en ese orden.
		std::vector<uint16_t> candidatosDorsal(const Datos& d, uint16_t pedido, uint16_t enOrigen, uint32_t pid, int origen, int destino) {
			std::vector<uint16_t> c = { pedido, enOrigen };
			for (int k = 0; k < kNumEquipos; k++) {
				if (k == origen || k == destino) continue;
				for (const auto& p : leerPlantilla(d, k)) if (p.pid == pid && p.dorsal) c.push_back(p.dorsal);
			}
			return c;
		}
		std::vector<uint16_t> dorsalesDe(const std::vector<Plaza>& pl) { std::vector<uint16_t> v; for (const auto& p : pl) v.push_back(p.dorsal); return v; }
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

			dorsal = alineacion::elegirDorsal(candidatosDorsal(_datos, dorsal, po[size_t(idx)].dorsal, pid, origen, destino), dorsalesDe(pd));
			if (!dorsal) return R::mal("SIN_DORSAL", "No queda ningún dorsal libre en el destino");

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

			// Elección compartida con el option file (Alineacion.h): posición del índice de plantilla vía `posicionDe(pid)`.
			std::function<int(int)> posDeIdx;
			if (posicionDe) posDeIdx = [&](int i) { return i >= 0 && size_t(i) < pl.size() ? posicionDe(pl[size_t(i)].pid) : -1; };
			const int elegido = alineacion::elegirSustituto(orden, idx, posDeIdx);
			if (elegido < 0) {
				const int cH = posicionDe ? posicionDe(pid) : -1;
				const bool esPortero = cH == 0 || (cH < 0 && !orden.empty() && orden[0] == uint8_t(idx));
				return R::mal("SIN_SUSTITUTO", esPortero ? "No hay otro portero para cubrir el puesto" : "No hay ningún jugador libre para cubrir el puesto");
			}
			return R::bien(pl[size_t(elegido)].pid);
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
		constexpr int kMaxRegistrosTabla = 200;     // tope de seguridad al recorrer una tabla
		constexpr int kVaciosFin = 3;               // tantos vacíos seguidos = fin de la tabla (puede haber huecos sueltos)
		constexpr size_t kTablasEsperadas = 11;     // A B C D E F G H×3 M I J (A2 es opcional: vacía al empezar la carrera) menos K (§4)
		constexpr size_t kDistanciaA2 = 976;        // A2 está a 40 registros de 24 B + 16 B de A
		constexpr size_t kRegEnContrato = 28;       // tabla I: el registro empieza 28 B antes de `reg` (§12)
		constexpr size_t kTamNegociacion = 60;      // lista de negociaciones abiertas (§12)
		constexpr size_t kRegEnNegociacion = 32;
		constexpr int kMaxNegociaciones = 64;

		bool registroVacio(const Datos& d, size_t ofsReg) {
			const uint32_t reg = u32(d, ofsReg), pid = u32(d, ofsReg + 4);
			return pid == 0 && (reg == 0 || reg == 0xffff || reg == kRegVacio);
		}

		/// Dirección del campo `reg` del registro i de una tabla alineada.
		long long regDe(const TablaAlineada& t, long long i) { return (long long)t.ofsReg0 + t.dir * i * (long long)t.stride; }
		/// Dónde empieza el registro i: `inicioRel` bytes antes de `reg` (4 en casi todas; 28 en la de contratos).
		long long inicioDe(const TablaAlineada& t, long long i) { return regDe(t, i) - (long long)t.inicioRel; }

		/// Cuántos registros ocupa la tabla hasta el último usado (inclusive). Igual que el juego, una tabla puede tener
		/// HUECOS (reg 0xffff, pid 0) en medio: se sigue hasta ver `kVaciosFin` vacíos seguidos. -1 = no termina (dañada).
		/// El registro devuelto (índice = extensión) está vacío: sirve de plantilla de «registro nunca usado».
		int extensionDe(const Datos& d, const TablaAlineada& t) {
			int ultimoUsado = -1, vacios = 0;
			for (int n = 0; n < kMaxRegistrosTabla; n++) {
				const long long r = regDe(t, n);
				if (r < (long long)kFinBloques || size_t(r) + 8 > d.size()) return -1;
				if (registroVacio(d, size_t(r))) { if (++vacios >= kVaciosFin) return ultimoUsado + 1; }
				else { vacios = 0; ultimoUsado = n; }
			}
			return -1;
		}

		bool rangoValido(const Datos& d, const TablaAlineada& t, int ext) {
			const long long lo = std::min(inicioDe(t, 0), inicioDe(t, ext)), hi = std::max(inicioDe(t, 0), inicioDe(t, ext)) + (long long)t.stride;
			return lo >= (long long)kFinBloques && size_t(hi) <= d.size();
		}

		/// Quita el registro `idx` COMPACTANDO: los siguientes suben uno y el último usado queda como el primer vacío.
		/// Así lo hace el juego en G, H e I (Stones, 8 oct). `ext` = extensión de la tabla (extensionDe).
		bool quitarRegistro(Datos& d, const TablaAlineada& t, int idx, int ext, std::string& porque) {
			if (ext < 0) { porque = "tabla sin fin"; return false; }
			if (idx < 0 || idx >= ext) { porque = "índice fuera de la tabla (" + std::to_string(idx) + "/" + std::to_string(ext) + ")"; return false; }
			if (!rangoValido(d, t, ext)) { porque = "tabla fuera del archivo"; return false; }
			std::vector<uint8_t> vacio(d.begin() + inicioDe(t, ext), d.begin() + inicioDe(t, ext) + (long long)t.stride);
			for (int i = idx; i < ext - 1; i++)
				std::memmove(&d[size_t(inicioDe(t, i))], &d[size_t(inicioDe(t, i + 1))], t.stride);
			std::memcpy(&d[size_t(inicioDe(t, ext - 1))], vacio.data(), t.stride);
			return true;
		}

		/// Deja el registro `idx` vacío EN SU SITIO, exactamente como lo deja el juego (Stones, 8 oct; A B C D E F J M):
		/// copia el primer registro nunca usado desde `reg` hasta el final del registro (lo que hay antes de `reg` se
		/// conserva), pone reg = 0xffff y aplica los retoques vistos en el juego:
		///   A (24): fecha vacía 0xffff en +16 · B (24↓): conserva la constante de +10 · F (52): 0 en +40 ·
		///   M (5628): también 0 en la palabra justo antes del siguiente `reg` (es el final de su propio registro).
		bool vaciarRegistro(Datos& d, const TablaAlineada& t, int idx, int ext, std::string& porque) {
			if (ext < 0) { porque = "tabla sin fin"; return false; }
			if (idx < 0 || idx >= ext) { porque = "índice fuera de la tabla (" + std::to_string(idx) + "/" + std::to_string(ext) + ")"; return false; }
			if (!rangoValido(d, t, ext)) { porque = "tabla fuera del archivo"; return false; }
			const size_t r = size_t(regDe(t, idx)), plantilla = size_t(regDe(t, ext)), largo = t.stride - 4;
			const uint16_t constanteB = u16(d, r + 10);
			std::memmove(&d[r], &d[plantilla], largo);
			p32(d, r, 0xffff); p32(d, r + 4, 0);
			if (t.stride == 24 && t.dir > 0) p16(d, r + 16, 0xffff);
			if (t.stride == 24 && t.dir < 0) p16(d, r + 10, constanteB);
			if (t.stride == 52) p32(d, r + 40, 0);
			if (t.stride == 5628 && r + t.stride <= d.size()) p32(d, r + t.stride - 4, 0);   // M: su registro acaba en la palabra anterior al siguiente `reg`
			return true;
		}

		/// Qué hace el juego con cada tabla cuando un jugador deja el equipo del usuario (§12):
		enum class Politica { Hueco, Compactar, NoTocar };
		Politica politicaDe(const TablaAlineada& t, bool esA2) {
			if (t.stride == 24 && t.dir > 0) return esA2 ? Politica::NoTocar : Politica::Hueco;
			if (t.stride == 108 || t.stride == 48) return Politica::Compactar;
			return Politica::Hueco;
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

		/// Lista de negociaciones abiertas (tras el blob; §12). Registro de 60 B:
		///   [estado u16][club que negocia u32][banderas][0xffff][-1][-1][0xffff u16] [club del jugador u32][reg][pid][monto][monto][0xffff][0xffff][0]
		/// El juego borra las del jugador que se va y compacta la lista (Stones, 8 oct). Devuelve cuántas quitó (-1 = lista rota).
		int quitarNegociaciones(Datos& d, uint32_t club, uint32_t reg, uint32_t pid) {
			uint8_t patron[12];
			for (int i = 0; i < 4; i++) { patron[i] = uint8_t(club >> (8 * i)); patron[4 + i] = uint8_t(reg >> (8 * i)); patron[8 + i] = uint8_t(pid >> (8 * i)); }
			auto esNegociacion = [&](size_t h) {   // h = campo «club del jugador»
				return h >= kFinBloques + 28 && h + 32 <= d.size()
					&& u32(d, h - 16) == 0xffff && u32(d, h - 12) == 0xffffffff && u32(d, h - 8) == 0xffffffff && u16(d, h - 4) == 0xffff
					&& u32(d, h + 20) == 0xffff && u32(d, h + 24) == 0xffff && u32(d, h + 28) == 0;
			};
			auto usada = [&](size_t ini) { return ini + kTamNegociacion <= d.size() && u32(d, ini + 28) != 0xffffffff; };
			static const uint32_t vacia[15] = { 0xffff, 0xffffffff, 0, 0xffff, 0xffffffff, 0xffffffff, 0xffff, 0xffffffff, 0xffff, 0, 0, 0, 0xffff, 0xffff, 0 };
			int quitadas = 0;
			for (int vuelta = 0; vuelta < kMaxNegociaciones; vuelta++) {
				const uint8_t* base = d.data();
				size_t h = kFinBloques + 28; bool hallada = false;
				while (h + 12 <= d.size()) {
					const void* f = std::memchr(base + h, patron[0], d.size() - 12 + 1 - h);
					if (!f) break;
					h = size_t(static_cast<const uint8_t*>(f) - base);
					if (std::memcmp(base + h, patron, 12) == 0 && esNegociacion(h)) { hallada = true; break; }
					h++;
				}
				if (!hallada) return quitadas;
				const size_t ini = h - 28;
				int n = 1;                                               // registros usados desde esta negociación
				while (n < kMaxNegociaciones && usada(ini + size_t(n) * kTamNegociacion)) n++;
				if (n >= kMaxNegociaciones) return -1;
				std::memmove(&d[ini], &d[ini + kTamNegociacion], kTamNegociacion * size_t(n - 1));
				for (int i = 0; i < 15; i++) p32(d, ini + kTamNegociacion * size_t(n - 1) + 4 * size_t(i), vacia[i]);
				quitadas++;
			}
			return -1;
		}
	}

	namespace {
		/// Busca la lista K del equipo del usuario por su ESTRUCTURA (no hace falta que refleje el orden actual: el juego la
		/// deja atrasada un tiempo tras un cambio de plantilla, p. ej. al despedir a Stones, 8 oct):
		///   K0 = [0][reg][pid][0] de alguien de la plantilla; luego registros [flag 0xc0–0xc6][reg][pid][0] de jugadores
		///   (casi todos de la plantilla; se admiten hasta 2 que ya no estén), un libre [flag 0xc0–0xc6][0xffff][0][0] y
		///   por lo menos un [0xc7][0xffff][0][0]. Devuelve las posiciones de K0 halladas.
		struct KHallada { size_t k0 = 0; std::vector<std::array<uint32_t, 3>> usados; uint32_t flagLibre = 0; };

		std::vector<KHallada> buscarK(const Datos& d, const std::vector<Plaza>& pl) {
			std::vector<KHallada> out;
			const uint8_t* base = d.data();
			auto esFlagUsado = [](uint32_t f) { return f >= kFlagLibreMin && f < kFlagFinK; };
			for (const Plaza& p : pl) {
				uint8_t patron[8];
				for (int i = 0; i < 4; i++) { patron[i] = uint8_t(p.reg >> (8 * i)); patron[4 + i] = uint8_t(p.pid >> (8 * i)); }
				size_t h = kFinBloques + 4;
				while (h + 12 <= d.size()) {
					const void* f = std::memchr(base + h, patron[0], d.size() - 12 + 1 - h);
					if (!f) break;
					h = size_t(static_cast<const uint8_t*>(f) - base);
					if (std::memcmp(base + h, patron, 8) == 0 && u32(d, h - 4) == 0 && u32(d, h + 8) == 0) {
						const size_t k0 = h - 4;
						KHallada k; k.k0 = k0; k.usados.push_back({ 0, p.reg, p.pid });
						bool ok = true; size_t r = k0 + kTamK;
						for (;; r += kTamK) {
							if (r + 2 * kTamK > d.size() || k.usados.size() > kMaxPlantilla) { ok = false; break; }
							const uint32_t fl = u32(d, r), reg = u32(d, r + 4), pid = u32(d, r + 8), z = u32(d, r + 12);
							if (z != 0 || !esFlagUsado(fl)) { ok = false; break; }
							if (pid == 0 && reg == 0xffff) { k.flagLibre = fl; break; }     // el libre
							if (pid == 0 || !regPlausible(reg) || pid >= kMaxPid) { ok = false; break; }
							k.usados.push_back({ fl, reg, pid });
						}
						if (ok) ok = u32(d, r + kTamK) == kFlagFinK && u32(d, r + kTamK + 4) == 0xffff && u32(d, r + kTamK + 8) == 0;
						if (ok && k.usados.size() >= kTitulares) {
							size_t enPlantilla = 0;
							for (const auto& u : k.usados) enPlantilla += std::any_of(pl.begin(), pl.end(), [&](const Plaza& q) { return q.reg == u[1] && q.pid == u[2]; });
							if (enPlantilla + 2 >= k.usados.size() && enPlantilla + 2 >= pl.size()) out.push_back(std::move(k));
						}
					}
					h++;
				}
			}
			std::sort(out.begin(), out.end(), [](const KHallada& a, const KHallada& b) { return a.k0 < b.k0; });
			out.erase(std::unique(out.begin(), out.end(), [](const KHallada& a, const KHallada& b) { return a.k0 == b.k0; }), out.end());
			return out;
		}
	}

	Resultado<Alineacion> GuardadoLM::alineacionDe(int k) const {
		using R = Resultado<Alineacion>;
		if (k < 0 || k >= kNumEquipos) return R::mal("EQUIPO_INVALIDO", std::to_string(k));
		const auto pl = leerPlantilla(_datos, k);
		const size_t n = pl.size();
		if (n < kTitulares) return R::mal("PLANTILLA_CORTA", "Menos de 11 jugadores");
		const Datos& d = _datos;

		// 1) El orden de formación del usuario vive en el arreglo de alineaciones (§10): en el primer bloque que lleva el
		//    ID del equipo del usuario (-11 en el bloque de equipo) y NO es el bloque del propio índice (ese es su copia
		//    «como equipo de la IA»). En el guardado de referencia es el bloque 627 (0x18f9d8); el 628 es su reserva.
		const auto bloques = buscarBloquesAli(d);
		if (!bloques.cantidad) return R::mal("ALINEACIONES_IA_NO_HALLADAS", "No se encontró el arreglo de alineaciones");
		const uint32_t idUsuario = u32(d, baseEquipo(k) + kOfsIdOption);
		size_t ofsOrden = 0; int bloqueUsuario = -1;
		for (int b = 0; b < bloques.cantidad && bloqueUsuario < 0; b++) {
			if (b == k) continue;
			const size_t s = bloques.inicio + size_t(b) * kTamBloqueAli;
			if (u32(d, s + kAliOfsEquipo) != idUsuario) continue;
			if (esOrdenValido(d, s + kAliOfsOrden, n)) { bloqueUsuario = b; ofsOrden = s + kAliOfsOrden; }
		}
		if (bloqueUsuario < 0) return R::mal("ALINEACION_NO_HALLADA", "Ningún bloque de alineación con el ID del usuario tiene un orden válido para " + std::to_string(n) + " jugadores");
		Alineacion a;
		a.ofsOrden = ofsOrden; a.ofsRoles = ofsOrden + kOrdenARoles;
		a.orden.assign(d.begin() + long(ofsOrden), d.begin() + long(ofsOrden + n));
		for (size_t i = 0; i < kBytesRoles; i++) {
			a.roles[i] = d[a.ofsRoles + i];
			if (a.roles[i] >= n) return R::mal("ALINEACION_NO_HALLADA", "Roles fuera de la plantilla");
		}

		// 2) La lista K, por estructura. Lo normal es que refleje el orden (K[i] = plantilla[orden[i]]); si el juego aún no
		//    la rehízo (queda atrasada tras un cambio de plantilla), se conservan los flags por jugador y se reconstruye.
		auto ks = buscarK(d, pl);
		if (ks.empty()) return R::mal("ALINEACION_NO_HALLADA", "No se encontró la lista K del equipo");
		if (ks.size() > 1) return R::mal("ALINEACION_AMBIGUA", std::to_string(ks.size()) + " listas K candidatas; no se toca nada");
		const KHallada& kh = ks[0];
		a.ofsK = kh.k0; a.flagLibreK = kh.flagLibre;
		a.kEspejo = kh.usados.size() == n;
		for (size_t i = 0; i < n; i++) {
			const Plaza& p = pl[a.orden[i]];
			uint32_t flag = i == 0 ? 0 : kFlagLibreMin;
			bool hallado = false;
			for (size_t j = 0; j < kh.usados.size(); j++)
				if (kh.usados[j][1] == p.reg && kh.usados[j][2] == p.pid) { flag = kh.usados[j][0]; hallado = true; if (j != i) a.kEspejo = false; break; }
			if (!hallado) a.kEspejo = false;
			if (i == 0) flag = 0;
			a.flagsK.push_back(flag);
		}
		return R::bien(std::move(a));
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
			struct Objetivo { TablaAlineada t; std::vector<int> j; int ext; Politica politica; };
			std::vector<Objetivo> objetivos;
			std::string descartadas;
			const Plaza& quien = pu[size_t(idx)];
			for (const auto& t : tablas) {
				if (t.ofsReg0 == a.ofsK + 4) continue;                   // la lista K no es una tabla: se trata aparte
				if (std::any_of(objetivos.begin(), objetivos.end(), [&](const Objetivo& o) { return o.t.ofsReg0 == t.ofsReg0 && o.t.stride == t.stride && o.t.dir == t.dir; })) continue;
				const int ext = extensionDe(_datos, t);
				// Una tabla de la plantilla tiene a TODOS los jugadores. Si no (p. ej. la ficha del último partido, que
				// el juego crea al jugar y lista solo a los que jugaron, en orden de plantilla), no es de las nuestras:
				// es historial y no se toca. Si faltara una tabla de verdad, la comprobación de firmas de abajo lo detecta.
				bool estanTodos = ext >= n;
				for (int i = 0; i < n && estanTodos; i++) {
					bool esta = false;
					for (int r = 0; r < ext && !esta; r++) {
						const size_t o = size_t(regDe(t, r));
						esta = u32(_datos, o) == pu[size_t(i)].reg && u32(_datos, o + 4) == pu[size_t(i)].pid;
					}
					estanTodos = esta;
				}
				if (!estanTodos) {
					char hexd[32]; std::snprintf(hexd, sizeof hexd, "%zx", t.ofsReg0);
					descartadas += "descartada (no es de la plantilla: " + std::to_string(ext) + " registros) paso " + std::to_string(t.stride) + " en 0x" + hexd + "\n";
					continue;
				}
				std::vector<int> j;
				for (int i = 0; i < ext; i++) {
					const size_t r = size_t(regDe(t, i));
					if (u32(_datos, r) == quien.reg && u32(_datos, r + 4) == quien.pid) j.push_back(i);
				}
				if (j.empty()) return R::mal("JUGADOR_NO_EN_TABLA", "El jugador no aparece en la tabla de paso " + std::to_string(t.stride) + (t.dir < 0 ? "↓" : ""));
				// A2 es la segunda tabla de 24 B, exactamente a 976 B de A (40 registros + 16 B); el juego no la toca al momento.
				const bool esA2 = t.stride == 24 && t.dir > 0 && std::any_of(tablas.begin(), tablas.end(), [&](const TablaAlineada& x) {
					return x.stride == 24 && x.dir > 0 && x.ofsReg0 + kDistanciaA2 == t.ofsReg0; });
				objetivos.push_back({ t, j, ext, politicaDe(t, esA2) });
			}
			// Tienen que estar las 12 tablas conocidas (ESTRUCTURA-ML.md §4); si hay más (p. ej. una que el juego
			// rellena al avanzar la temporada) también se tratan, porque llevan los mismos (reg, pid).
			{
				struct Firma { size_t stride; int dir; int veces; };
				// A2 (la segunda de 24 B) está vacía en una carrera recién empezada (respaldo r0): no se exige.
				Firma firmas[] = { {24,+1,1}, {24,-1,1}, {44,+1,1}, {368,+1,1}, {192,+1,1}, {52,+1,1}, {108,+1,3}, {5628,+1,1}, {48,+1,1}, {16,+1,1} };
				std::string faltan;
				for (const auto& f : firmas) {
					const int hay = int(std::count_if(objetivos.begin(), objetivos.end(), [&](const Objetivo& o) { return o.t.stride == f.stride && o.t.dir == f.dir; }));
					if (hay < f.veces) faltan += " " + std::to_string(f.stride) + (f.dir < 0 ? "↓" : "");
				}
				if (!faltan.empty()) return R::mal("TABLAS_INESPERADAS", "Faltan tablas conocidas del equipo del usuario:" + faltan);
				if (objetivos.size() < kTablasEsperadas) return R::mal("TABLAS_INESPERADAS", "Solo " + std::to_string(objetivos.size()) + " tablas");
				// Las de un solo registro por jugador no pueden tenerlo repetido (solo la de contratos: ofertas abiertas).
				for (const auto& o : objetivos)
					if (o.j.size() > 1 && o.t.stride != 48) return R::mal("TABLA_DANADA", "El jugador está repetido en la tabla de paso " + std::to_string(o.t.stride));
			}
			// Club interno del usuario = (ID option << 14) | índice del bloque; está en la cabecera de cada contrato (§12).
			uint32_t clubUsuario = 0;
			for (const auto& o : objetivos) if (o.t.stride == 48) clubUsuario = u32(_datos, o.t.ofsReg0 - kRegEnContrato + 4);
			if ((clubUsuario & 0x3fff) != uint32_t(kUsuario))
				return R::mal("CONTRATOS_INESPERADOS", "La tabla de contratos no lleva el club del usuario (¿otra versión?)");

			_ultimoInforme = descartadas;
			for (const auto& o : objetivos) {
				_ultimoInforme += "tabla paso " + std::to_string(o.t.stride) + (o.t.dir < 0 ? "↓" : "") + " en 0x";
				char hex[32]; std::snprintf(hex, sizeof hex, "%zx", o.t.ofsReg0); _ultimoInforme += hex;
				_ultimoInforme += ": registro";
				for (int j : o.j) _ultimoInforme += " " + std::to_string(j);
				_ultimoInforme += " de " + std::to_string(o.ext) + (o.politica == Politica::Hueco ? " → hueco" : o.politica == Politica::Compactar ? " → compacta" : " → no se toca (A2)") + "\n";
			}

			// Alineación: dónde está el que se va y, si hace falta, el sustituto.
			int posH = -1, posS = -1;
			for (int i = 0; i < n; i++) { if (a.orden[size_t(i)] == idx) posH = i; if (idxS >= 0 && a.orden[size_t(i)] == idxS) posS = i; }
			if (posH < 0) return R::mal("ALINEACION_INCOHERENTE", "El jugador no aparece en el orden de formación");
			if (idxS >= 0 && posS < 0) return R::mal("ALINEACION_INCOHERENTE", "El sustituto no aparece en el orden de formación");
			if (idxS < 0 && size_t(posH) < kTitulares) return R::mal("FALTA_SUSTITUTO", "El jugador es titular: hace falta un sustituto");
			if (idxS < 0 && std::any_of(a.roles.begin(), a.roles.end(), [&](uint8_t v) { return v == idx; }))
				return R::mal("FALTA_SUSTITUTO", "El jugador tiene un rol (capitán o lanzador): hace falta un sustituto");

			dorsal = alineacion::elegirDorsal(candidatosDorsal(_datos, dorsal, pu[size_t(idx)].dorsal, pid, kUsuario, kDestino), dorsalesDe(pd));
			if (!dorsal) return R::mal("SIN_DORSAL", "No queda ningún dorsal libre en el destino");

			// --- Todo cuadra: se trabaja sobre una copia y solo al final se adopta ----------------
			Datos d = _datos;
			std::string porque;
			for (const auto& o : objetivos) {
				if (o.politica == Politica::NoTocar) continue;
				if (o.politica == Politica::Hueco) { if (!vaciarRegistro(d, o.t, o.j[0], o.ext, porque)) return R::mal("TABLA_DANADA", porque); continue; }
				// Compactar: de atrás hacia adelante para que los índices anteriores sigan valiendo (contratos: TODOS los suyos).
				int ext = o.ext;
				for (auto it = o.j.rbegin(); it != o.j.rend(); ++it, --ext)
					if (!quitarRegistro(d, o.t, *it, ext, porque)) return R::mal("TABLA_DANADA", porque);
			}
			// Negociaciones abiertas por el jugador (ofertas de otros clubes): el juego las borra y compacta la lista.
			const int negociaciones = quitarNegociaciones(d, clubUsuario, quien.reg, quien.pid);
			if (negociaciones < 0) return R::mal("TABLA_DANADA", "La lista de negociaciones no termina");
			_ultimoInforme += "negociaciones abiertas quitadas: " + std::to_string(negociaciones) + "\n";

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

	// =========================================================================
	//  Dinero del usuario (§13, §17): bloque a distancia fija de la tabla A (el «club del usuario» es una estructura fija
	//  del motor; A está en 0xbe5fc8 y el dinero en 0xc7dd00 en todos los guardados vistos).
	// =========================================================================
	namespace {
		constexpr size_t kFinanzasDesdeA = 0xc7dd00 - 0xbe5fc8;    // 0x97d38
		constexpr size_t kFinOfsActual = 0x00, kFinOfsInicial = 0x10, kFinOfsTope = 0x14;
		constexpr uint64_t kMaxEuros = 40000000000ull;               // 40.000 M: por encima no cabe en u32 ×100 con margen
		constexpr uint8_t kContratoVigente = 5;

		struct TablasUsuario { TablaAlineada a; TablaAlineada contratos; bool ok = false; };

		/// Localiza A (la primera tabla de 24 B hacia delante; A2, si existe, está 976 B después) y la de contratos.
		TablasUsuario tablasClave(const GuardadoLM& g, int k) {
			TablasUsuario t;
			const auto tablas = g.tablasDe(k);
			bool hayA = false, hayI = false;
			for (const auto& x : tablas) {
				if (x.stride == 24 && x.dir > 0 && (!hayA || x.ofsReg0 < t.a.ofsReg0)) { t.a = x; hayA = true; }
				if (x.stride == 48 && !hayI) { t.contratos = x; hayI = true; }
			}
			t.ok = hayA && hayI;
			return t;
		}

		/// Suma de sueldos anuales (/100) de los contratos vigentes del club.
		uint64_t sueldosDe(const Datos& d, const TablaAlineada& contratos, uint32_t club) {
			uint64_t suma = 0;
			const int ext = extensionDe(d, contratos);
			for (int i = 0; i < ext; i++) {
				const size_t r = size_t(regDe(contratos, i));
				if (registroVacio(d, r)) continue;
				if (d[r - kRegEnContrato + 8] == kContratoVigente && u32(d, r - kRegEnContrato + 4) == club) suma += u32(d, r + 8);
			}
			return suma;
		}
	}

	Resultado<Finanzas> GuardadoLM::finanzas(int k) const {
		using R = Resultado<Finanzas>;
		if (k < 0 || k >= kNumEquipos) return R::mal("EQUIPO_INVALIDO", std::to_string(k));
		const auto t = tablasClave(*this, k);
		if (!t.ok) return R::mal("NO_ES_USUARIO", "Ese equipo no tiene las tablas del usuario (A y contratos)");
		const size_t ofs = t.a.ofsReg0 + kFinanzasDesdeA;
		if (ofs + 0x40 > _datos.size()) return R::mal("ML_FORMATO", "El bloque de dinero cae fuera del archivo");
		const uint32_t club = u32(_datos, t.contratos.ofsReg0 - kRegEnContrato + 4);
		if ((club & 0x3fff) != uint32_t(k)) return R::mal("CONTRATOS_INESPERADOS", "La tabla de contratos no lleva el club del usuario");
		Finanzas f;
		f.ofs = ofs;
		f.presupuestoFichajes = uint64_t(u32(_datos, ofs + kFinOfsActual)) * 100;
		f.presupuestoFichajesInicial = uint64_t(u32(_datos, ofs + kFinOfsInicial)) * 100;
		f.topeSalarial = uint64_t(u32(_datos, ofs + kFinOfsTope)) * 100;
		f.sueldosActuales = sueldosDe(_datos, t.contratos, club) * 100;
		// Validación del ancla: cifras con sentido (tope que cubre los sueldos y presupuestos por debajo del máximo).
		if (f.topeSalarial < f.sueldosActuales || f.topeSalarial > kMaxEuros || f.presupuestoFichajes > kMaxEuros || f.sueldosActuales == 0)
			return R::mal("FINANZAS_NO_HALLADAS", "El bloque de dinero no cuadra con los contratos (¿otra versión del guardado?)");
		return R::bien(f);
	}

	Resultado<Finanzas> GuardadoLM::fijarFinanzas(int k, uint64_t presupuestoFichajesEur, uint64_t topeSalarialEur) {
		using R = Resultado<Finanzas>;
		auto f = finanzas(k);
		if (!f.ok()) return f;
		if (presupuestoFichajesEur > kMaxEuros || topeSalarialEur > kMaxEuros) return R::mal("IMPORTE_INVALIDO", "Más de 40.000 millones");
		if (presupuestoFichajesEur % 100 || topeSalarialEur % 100) return R::mal("IMPORTE_INVALIDO", "El juego guarda el dinero en múltiplos de 100 €");
		if (topeSalarialEur && topeSalarialEur < f.valor->sueldosActuales)
			return R::mal("TOPE_INSUFICIENTE", "El tope salarial no cubre los sueldos actuales (" + std::to_string(f.valor->sueldosActuales) + " €)");
		Datos d = _datos;
		if (presupuestoFichajesEur) p32(d, f.valor->ofs + kFinOfsActual, uint32_t(presupuestoFichajesEur / 100));
		if (topeSalarialEur) p32(d, f.valor->ofs + kFinOfsTope, uint32_t(topeSalarialEur / 100));
		_datos.swap(d);
		return finanzas(k);
	}

	Resultado<std::string> GuardadoLM::guardarComo(const std::string& rutaNueva, const std::string& textoInfo) const {
		using R = Resultado<std::string>;
		if (!_sobre) return R::mal("SIN_SOBRE", "Este guardado vino de datos sueltos: no hay envoltura para cifrar");
		SobrePes sobre = *_sobre;   // copia: así el texto info cambiado no altera el original
		if (!textoInfo.empty()) sobre.ponerTextoInfo(textoInfo);
		return sobre.guardarComo(rutaNueva, _datos);
	}

}
