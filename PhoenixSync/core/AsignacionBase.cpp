#include "AsignacionBase.h"
#include "BaseDatosParche.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include "../terceros/miniz/miniz.h"

namespace fs = std::filesystem;

namespace mercado::base {

	namespace {
		uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
		void pon32(uint8_t* p, uint32_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); p[2] = uint8_t(v >> 16); p[3] = uint8_t(v >> 24); }
		constexpr size_t kTam = 16;
	}

	Resultado<Asignaciones> Asignaciones::desdeBytes(const std::vector<uint8_t>& d) {
		using R = Resultado<Asignaciones>;
		auto raw = descomprimirWesys(d);
		if (!raw.ok()) return R{ std::nullopt, raw.error };
		const auto& b = *raw.valor;
		if (b.empty() || b.size() % kTam != 0) return R::mal("ASIGNACION_FORMATO", "Tamaño no múltiplo de 16");
		Asignaciones a;
		a._cabecera.assign(d.begin(), d.begin() + 8);
		for (size_t o = 0; o < b.size(); o += kTam) {
			const uint8_t* e = &b[o];
			Fila f;
			f.indice = le32(e); f.jugador = le32(e + 4); f.equipo = le32(e + 8);
			f.dorsal = uint8_t(e[12] + 1);
			f.orden = uint8_t(e[13] >> 2); f.bajosOrden = uint8_t(e[13] & 3);
			f.banderas = e[14]; f.extra = e[15];
			a._filas.push_back(f);
		}
		return R::bien(std::move(a));
	}

	Resultado<Asignaciones> Asignaciones::abrir(const std::string& ruta) {
		std::ifstream f(aRuta(ruta), std::ios::binary);
		if (!f) return Resultado<Asignaciones>::mal("ARCHIVO_NO_EXISTE", ruta);
		std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), {});
		return desdeBytes(d);
	}

	grupo::Plantillas Asignaciones::plantillas() const {
		std::map<uint32_t, std::vector<std::pair<uint8_t, uint32_t>>> t;
		for (const auto& f : _filas) t[f.equipo].push_back({ f.orden, f.jugador });
		grupo::Plantillas p;
		for (auto& [eq, v] : t) {
			std::sort(v.begin(), v.end());
			auto& w = p[eq];
			for (const auto& x : v) w.push_back(x.second);
		}
		return p;
	}

	Resultado<bool> Asignaciones::mover(uint32_t jugador, uint32_t hacia, uint32_t desde, uint16_t dorsal) {
		using R = Resultado<bool>;
		Fila* fila = nullptr;
		std::set<int> dorsales;
		int enDestino = 0, ordenMax = -1;
		for (auto& f : _filas) {
			if (f.jugador == jugador && f.equipo == desde) fila = &f;
			if (f.equipo == hacia) {
				if (f.jugador == jugador) return R::mal("YA_ESTA_EN_DESTINO");
				enDestino++; dorsales.insert(f.dorsal); ordenMax = std::max<int>(ordenMax, f.orden);
			}
		}
		if (!fila) return R::mal("JUGADOR_NO_ESTA_EN_ORIGEN");
		if (enDestino >= 40) return R::mal("PLANTILLA_LLENA", "El destino ya tiene 40 jugadores en la base");
		int d = (dorsal >= 1 && dorsal <= 99 && !dorsales.count(dorsal)) ? dorsal : 0;
		for (int k = 99; !d && k >= 1; k--) if (!dorsales.count(k)) d = k;
		if (!d) return R::mal("SIN_DORSAL");
		// El que se va: los que estaban detrás suben un puesto (orden seguido 0..n-1).
		const uint8_t ordenViejo = fila->orden;
		for (auto& f : _filas) if (f.equipo == desde && &f != fila && f.orden > ordenViejo) f.orden--;
		fila->equipo = hacia;
		fila->dorsal = uint8_t(d);
		fila->orden = uint8_t(ordenMax + 1);   // último del destino
		fila->banderas = 0;                     // no llega como capitán
		return R::bien(true);
	}

	Resultado<std::vector<uint8_t>> Asignaciones::bytes() const {
		using R = Resultado<std::vector<uint8_t>>;
		std::vector<uint8_t> plano(_filas.size() * kTam, 0);
		for (size_t i = 0; i < _filas.size(); i++) {
			const auto& f = _filas[i];
			uint8_t* e = &plano[i * kTam];
			pon32(e, f.indice); pon32(e + 4, f.jugador); pon32(e + 8, f.equipo);
			e[12] = uint8_t(f.dorsal - 1);
			e[13] = uint8_t((f.orden << 2) | f.bajosOrden);
			e[14] = f.banderas; e[15] = f.extra;
		}
		mz_ulong n = mz_compressBound(mz_ulong(plano.size()));
		std::vector<uint8_t> comp(n);
		if (mz_compress2(comp.data(), &n, plano.data(), mz_ulong(plano.size()), MZ_BEST_COMPRESSION) != MZ_OK) return R::mal("WESYS_ZLIB", "No se pudo comprimir");
		std::vector<uint8_t> out(16 + n);
		std::memcpy(out.data(), _cabecera.data(), 8);
		pon32(&out[8], uint32_t(n));
		pon32(&out[12], uint32_t(plano.size()));
		std::memcpy(&out[16], comp.data(), n);
		// Verificación: se vuelve a leer y debe dar lo mismo.
		auto re = desdeBytes(out);
		if (!re.ok() || re.valor->_filas.size() != _filas.size()) return R::mal("ASIGNACION_VERIFICACION", "La relectura no coincide");
		return R::bien(std::move(out));
	}

	Resultado<bool> Asignaciones::guardarComo(const std::string& ruta) const {
		std::error_code ec;
		if (fs::exists(aRuta(ruta), ec)) return Resultado<bool>::mal("DESTINO_OCUPADO", ruta);
		auto b = bytes();
		if (!b.ok()) return Resultado<bool>{ std::nullopt, b.error };
		std::ofstream f(aRuta(ruta), std::ios::binary);
		f.write(reinterpret_cast<const char*>(b.valor->data()), std::streamsize(b.valor->size()));
		if (!f) return Resultado<bool>::mal("ARCHIVO_ERROR", ruta);
		return Resultado<bool>::bien(true);
	}

	std::vector<grupo::ResultadoOp> aplicarOperaciones(Asignaciones& a, const std::vector<grupo::Operacion>& ops) {
		std::vector<grupo::ResultadoOp> r;
		for (const auto& op : ops) {
			if (op.tipo == "quitar") { r.push_back({ op.id, "conflicto", "dejar sin equipo todavía no se aplica en la base" }); continue; }
			const auto d = grupo::decidir(op, a.plantillas());
			if (d.v == grupo::Veredicto::YaAplicada) { r.push_back({ op.id, "aplicada", d.motivo }); continue; }
			if (d.v == grupo::Veredicto::Conflicto) { r.push_back({ op.id, "conflicto", d.motivo }); continue; }
			auto x = a.mover(op.jugador, op.equipoDestino, op.equipoOrigen, op.dorsal);
			r.push_back(x.ok() ? grupo::ResultadoOp{ op.id, "aplicada", "" } : grupo::ResultadoOp{ op.id, "conflicto", x.error.codigo + " " + x.error.detalle });
		}
		return r;
	}

}
