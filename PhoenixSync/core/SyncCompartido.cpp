#include "SyncCompartido.h"
#include "AsignacionBase.h"
#include "Entrega.h"
#include "FichasBase.h"
#include "RutasJuego.h"
#include "Sha256.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace mercado::sync {

	// --- Option file de verdad ---------------------------------------------------------
	Resultado<LecturaOption> AccesoOptionReal::leer(const std::string& ruta) {
		auto of = OptionFile::abrir(ruta);
		if (!of.ok()) return Resultado<LecturaOption>{ std::nullopt, of.error };
		LecturaOption l;
		l.plantillas = grupo::plantillasDe(*of.valor);
		for (const auto& [eq, pl] : of.valor->plantillas()) for (const auto& p : pl) if (p.jugador) l.dorsales[eq][p.jugador] = p.dorsal;
		l.huellaEquipos = grupo::huellaEquipos(*of.valor);
		return Resultado<LecturaOption>::bien(std::move(l));
	}
	Resultado<AplicacionOption> AccesoOptionReal::aplicar(const std::string& ruta, const std::vector<grupo::Operacion>& ops, const std::string& salida) {
		auto of = OptionFile::abrir(ruta);
		if (!of.ok()) return Resultado<AplicacionOption>{ std::nullopt, of.error };
		grupo::EditorOption ed(*of.valor, _pos);
		AplicacionOption a;
		a.resultados = grupo::aplicarLote(ed, ops);
		for (const auto& r : a.resultados) if (r.estado == "aplicada" && r.motivo.empty()) a.escritas++;
		a.plantillasDespues = ed.plantillas();
		if (a.escritas) {
			auto g = of.valor->guardarComo(salida);
			if (!g.ok()) return Resultado<AplicacionOption>{ std::nullopt, g.error };
		}
		return Resultado<AplicacionOption>::bien(std::move(a));
	}

	Motor::Motor(ClienteSync& web, AccesoOption& option, Entorno entorno, Registro registro)
		: _web(web), _of(option), _en(std::move(entorno)), _log(std::move(registro)) {}

	// --- Utilidades ---------------------------------------------------------------------
	int64_t Motor::ahora() const {
		if (_en.ahora) return _en.ahora();
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
	std::string Motor::fechaIso() const {
		const std::time_t t = static_cast<std::time_t>(ahora());
		std::tm l{};
#ifdef _WIN32
		localtime_s(&l, &t);
#else
		localtime_r(&t, &l);
#endif
		char b[32]; std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%S", &l); return b;
	}
	std::string Motor::rutaEstado() const { return deRuta(aRuta(_en.carpetaDatos) / "sync" / "estado.json"); }
	std::string Motor::carpetaEntrega() const { return deRuta(aRuta(_en.carpetaDatos) / "entrega"); }
	std::string Motor::carpetaRespaldos() const { return deRuta(aRuta(_en.carpetaDatos) / "respaldos"); }
	std::string Motor::carpetaTrabajo() const { return deRuta(aRuta(_en.carpetaDatos) / "sync" / "trabajo"); }

	std::string Motor::editActivo() const {
		const auto s = rutas::saveActivo(_en.documentos ? _en.documentos() : std::vector<std::string>{});
		return s ? deRuta(aRuta(s->ruta) / "EDIT00000000") : std::string();
	}

	std::vector<respaldos::Origen> Motor::origenes() const {
		std::vector<respaldos::Origen> o;
		const auto s = rutas::saveActivo(_en.documentos ? _en.documentos() : std::vector<std::string>{});
		o.push_back({ "save", s ? s->ruta : std::string() });
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		const auto bases = rutas::buscarPlayerBin(juego);
		if (bases.empty()) o.push_back({ "phoenix-db", "" });   // queda anotado como omitido en el manifiesto
		for (const auto& b : bases) {
			const std::string et = "phoenix-db/" + rutas::etiquetaSegura(b.modo);
			o.push_back({ et, b.playerBin });
			const fs::path pa = aRuta(b.playerBin).parent_path() / "PlayerAssignment.bin";
			std::error_code ec;
			if (fs::is_regular_file(pa, ec)) o.push_back({ et, deRuta(pa) });
		}
		return o;
	}

	void Motor::historial(const grupo::Operacion& op, const std::string& estado, const std::string& motivo) {
		_e.anotarHistorial({ fechaIso(), op.id, grupo::resumenDe(op, _en.nombreEquipo), op.autor.empty() ? op.autorPc : op.autor, estado, motivo });
	}
	void Motor::confirmarLuego(const std::string& id, const std::string& estado, const std::string& motivo) {
		for (auto& c : _e.porConfirmar) if (c.id == id) { c.estado = estado; c.motivo = motivo; return; }
		_e.porConfirmar.push_back({ id, estado, motivo });
	}

	Resultado<bool> Motor::cargar() {
		auto r = grupo::cargarEstado(rutaEstado());
		if (!r.ok()) return Resultado<bool>{ std::nullopt, r.error };
		_e = std::move(*r.valor);
		// Al arrancar, el modo guardado solo se MUESTRA: hasta leer la web se actúa con autorización.
		_e.config.leida = false;
		_e.config.modo = grupo::Modo::Autorizacion;
		_e.config.motivoNoLeida = "todavía no se consultó la web";
		return Resultado<bool>::bien(true);
	}
	Resultado<bool> Motor::guardar() { return grupo::guardarEstado(rutaEstado(), _e); }

	std::vector<std::string> Motor::tomarAvisos() { auto v = std::move(_avisos); _avisos.clear(); return v; }

	// --- Botones -----------------------------------------------------------------------------
	void Motor::elegirGrupo(const std::string& grupoId) {
		if (grupoId == _e.grupoId) return;
		_e.grupoId = grupoId;
		_e.ultimaSeq = 0;
		_e.etagOperaciones.clear();
		_e.etagConfig.clear();
		_e.entrantes.clear();
		_e.porConfirmar.clear();
		_e.porPublicar.clear();
		_e.config = {};
		_ultimaConfig = -1000000;
	}
	void Motor::ponerPublicar(bool si) { _e.publicar = si; }
	void Motor::ponerPausa(bool si) { _e.pausado = si; }
	void Motor::ponerBaseEnEntrega(bool si) { _e.baseEnEntrega = si; }

	std::string Motor::asignacionBase() const {
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		for (const auto& b : rutas::buscarPlayerBin(juego)) {   // «principal» primero
			const fs::path pa = aRuta(b.playerBin).parent_path() / "PlayerAssignment.bin";
			std::error_code ec;
			if (fs::is_regular_file(pa, ec)) return deRuta(pa);
		}
		return "";
	}

	std::vector<respaldos::Info> Motor::respaldos() const { return respaldos::listar(carpetaRespaldos()); }

	Resultado<respaldos::Info> Motor::respaldarAhora(const std::string& motivo) {
		respaldos::Opciones op;
		op.raiz = carpetaRespaldos();
		op.motivo = motivo;
		op.versionSync = kVersionSync;
		op.espacioLibre = _en.espacioLibre;
		op.ahora = [this] { return static_cast<std::time_t>(ahora()); };
		auto r = respaldos::crear(origenes(), op);
		if (r.ok()) {
			const auto borrados = respaldos::aplicarRetencion(op.raiz, _en.retencion, static_cast<std::time_t>(ahora()));
			if (!borrados.empty()) log("info", "Retención: " + std::to_string(borrados.size()) + " respaldos viejos borrados");
		}
		return r;
	}

	Resultado<respaldos::ResultadoRestaurar> Motor::restaurar(const std::string& nombre, bool aunqueJuegoAbierto) {
		using R = Resultado<respaldos::ResultadoRestaurar>;
		if (!aunqueJuegoAbierto && _en.juegoAbierto && _en.juegoAbierto())
			return R::mal("JUEGO_ABIERTO", "El juego está abierto: confirma para restaurar igual y luego pulsa «Datos Actual. en vivo → Activar» (o cierra y abre el juego)");
		respaldos::Opciones op;
		op.raiz = carpetaRespaldos();
		op.versionSync = kVersionSync;
		op.espacioLibre = _en.espacioLibre;
		op.ahora = [this] { return static_cast<std::time_t>(ahora()); };
		auto r = respaldos::restaurar(op.raiz, nombre, origenes(), op);
		if (r.ok()) {
			// Lo restaurado NO es un fichaje nuevo: no se publica.
			const std::string edit = editActivo();
			if (!edit.empty()) {
				const std::string h = sha256::deArchivo(edit);
				_e.anotarConocida(h);
				if (auto of = _of.leer(edit); of.ok()) { _e.foto = of.valor->plantillas; _e.hayFoto = true; _e.ultimoHashVisto = h; }
			}
			_e.anotarHistorial({ fechaIso(), "", "Respaldo restaurado: " + nombre, _en.nombrePc, "restaurado", "" });
			guardar();
		}
		return r;
	}

	Resultado<int> Motor::traerAhora() {
		auto r = traer();
		if (r.ok()) { procesarEntrantes(); enviarConfirmaciones(); guardar(); }
		return r;
	}

	Resultado<int> Motor::aplicar(const std::vector<std::string>& ids) {
		std::vector<std::string> lista = ids;
		if (lista.empty())
			for (const auto& x : _e.entrantes) if (x.estado == "pendiente" || x.estado == "nuevo") lista.push_back(x.op.id);
		if (lista.empty()) return Resultado<int>::bien(0);
		auto r = aplicarInterno(lista, false);
		enviarConfirmaciones();
		guardar();
		return r;
	}

	Resultado<bool> Motor::rechazar(const std::string& id) {
		auto it = std::find_if(_e.entrantes.begin(), _e.entrantes.end(), [&](const auto& x) { return x.op.id == id; });
		if (it == _e.entrantes.end()) return Resultado<bool>::mal("NO_EXISTE", id);
		historial(it->op, "rechazada", "rechazada en esta PC");
		_e.aplicadas.insert(id);   // ya tratada: no vuelve
		confirmarLuego(id, "rechazada", "rechazada por el usuario");
		_e.entrantes.erase(it);
		enviarConfirmaciones();
		guardar();
		return Resultado<bool>::bien(true);
	}

	Resultado<std::string> Motor::deshacerUltimo() {
		using R = Resultado<std::string>;
		if (_e.ultima.respaldo.empty()) return R::mal("NADA_QUE_DESHACER", "No hay ningún cambio del grupo aplicado en esta PC");
		if (entrega::hayPendiente(carpetaEntrega()) || !_e.entregaEnCurso.empty())
			return R::mal("ENTREGA_PENDIENTE", "Phoenix Link todavía no colocó la entrega anterior; prueba en unos segundos");
		if (_e.ultima.stats) {
			std::string pb;
			for (const auto& a : respaldos::listar(carpetaRespaldos()))
				if (a.nombre == _e.ultima.respaldo)
					for (const auto& f : a.archivos)
						if (pb.empty() && f.etiqueta.rfind("phoenix-db", 0) == 0 && aRuta(f.relativo).filename() == "Player.bin")
							pb = deRuta(aRuta(a.carpeta) / aRuta(f.relativo));
			if (pb.empty()) return R::mal("RESPALDO_INVALIDO", "El respaldo " + _e.ultima.respaldo + " no tiene Player.bin");
			const std::string id = entrega::nuevoId();
			auto w = entrega::escribir(carpetaEntrega(), id, "Deshacer stats", { { "Player.bin", pb } }, fechaIso());
			if (!w.ok()) return R{ std::nullopt, w.error };
			_e.entregaEnCurso = id; _e.entregaConBase = true;
			_e.anotarHistorial({ fechaIso(), "", "Deshecho: " + std::to_string(_e.ultima.ops.size()) + " cambio(s) de stats", _en.nombrePc, "deshecho", "" });
			_e.ultima = {};
			guardar();
			return R::bien(id);
		}
		const fs::path edit = aRuta(carpetaRespaldos()) / aRuta(_e.ultima.respaldo) / "save" / "EDIT00000000";
		std::error_code ec;
		if (!fs::is_regular_file(edit, ec)) return R::mal("RESPALDO_INVALIDO", "El respaldo " + _e.ultima.respaldo + " ya no tiene el option file");
		const std::string actual = editActivo();
		if (!actual.empty() && fs::file_size(aRuta(actual), ec) != fs::file_size(edit, ec))
			return R::mal("TAMANO_DISTINTO", "El option file del respaldo no mide lo mismo que el actual (Link lo rechazaría)");
		std::vector<entrega::Archivo> archivos = { { "EDIT00000000", deRuta(edit) } };
		if (_e.baseEnEntrega) {   // la base del mismo respaldo (si se guardó)
			std::string paRespaldo;
			for (const auto& a : respaldos::listar(carpetaRespaldos()))
				if (a.nombre == _e.ultima.respaldo)
					for (const auto& f : a.archivos)
						if (paRespaldo.empty() && f.etiqueta.rfind("phoenix-db", 0) == 0 && aRuta(f.relativo).filename() == "PlayerAssignment.bin")
							paRespaldo = deRuta(aRuta(a.carpeta) / aRuta(f.relativo));
			if (!paRespaldo.empty()) archivos.push_back({ "PlayerAssignment.bin", paRespaldo });
		}
		const std::string id = entrega::nuevoId();
		auto w = entrega::escribir(carpetaEntrega(), id, "Deshacer cambio del grupo", archivos, fechaIso());
		if (!w.ok()) return R{ std::nullopt, w.error };
		const std::string h = sha256::deArchivo(deRuta(edit));
		_e.anotarConocida(h);   // lo que vuelve no es un fichaje nuevo
		if (auto of = _of.leer(deRuta(edit)); of.ok()) { _e.foto = of.valor->plantillas; _e.hayFoto = true; }
		_e.entregaEnCurso = id;
		_e.entregaConBase = archivos.size() > 1;
		_e.anotarHistorial({ fechaIso(), "", "Deshecho: " + std::to_string(_e.ultima.ops.size()) + " cambio(s) del grupo", _en.nombrePc, "deshecho", "" });
		_e.ultima = {};
		guardar();
		return R::bien(id);
	}

	// --- Vuelta ------------------------------------------------------------------------------
	void Motor::ciclo() {
		revisarEntrega();
		if (_e.pausado) { guardar(); return; }
		respaldoDiario();
		leerConfig(false);
		revisarOficial();
		detectarPropios();
		entregarMiBase();
		publicarPendientes();
		traer();
		procesarEntrantes();
		enviarConfirmaciones();
		guardar();
	}

	void Motor::revisarEntrega() {
		if (_e.entregaEnCurso.empty()) return;
		const int st = entrega::estadoDe(carpetaEntrega(), _e.entregaEnCurso);
		if (st == 1) {
			log("info", "Link colocó la entrega " + _e.entregaEnCurso);
			if (_e.entregaConBase) avisarRecarga();
			_e.entregaEnCurso.clear(); _e.entregaConBase = false;
		}
		else if (st == -1) {
			avisar("Phoenix Link rechazó la entrega (mira su historial). Tu option file no cambió.");
			_e.anotarHistorial({ fechaIso(), "", "Link rechazó la entrega " + _e.entregaEnCurso, _en.nombrePc, "rechazada", "ver historial.log de Link" });
			_e.entregaEnCurso.clear(); _e.entregaConBase = false;
		}
	}

	// Aviso «hay fichajes nuevos» para phoenix.lua v0.18 (contrato en base-conocimiento/DATOS-EN-VIVO-Y-RECARGA.md, parte «31 · modos de recarga»):
	// recargar.txt en cada buzón <SiderAddons>\content\phoenix que ya exista. En modo ACTIVAR
	// el juego lo ignora; en AUTO-FICHAJES recarga solo al volver al menú y entrar a un modo.
	void Motor::avisarRecarga() {
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		const std::string texto = fechaIso() + " " + _e.entregaEnCurso;
		for (const auto& b : rutas::buscarBuzones(juego)) {
			const fs::path tmp = aRuta(b) / "recargar.tmp", fin = aRuta(b) / "recargar.txt";
			std::error_code ec;
			{ std::ofstream f(tmp, std::ios::binary | std::ios::trunc); f << texto; if (!f) { log("warn", "No se pudo escribir " + deRuta(tmp)); continue; } }
			fs::rename(tmp, fin, ec);
			if (ec) { fs::remove(tmp, ec); log("warn", "No se pudo escribir " + deRuta(fin)); }
			else log("info", "Aviso de recarga escrito en " + deRuta(fin));
		}
	}

	// Mis propios fichajes también en MI base (PlayerAssignment.bin), para verlos con «Activar».
	void Motor::entregarMiBase() {
		if (_e.paraMiBase.empty()) return;
		if (!_e.baseEnEntrega) { _e.paraMiBase.clear(); return; }
		if (entrega::hayPendiente(carpetaEntrega()) || !_e.entregaEnCurso.empty()) return;   // se reintenta en la próxima vuelta
		if (ahora() < _esperarAplicarHasta) return;
		const std::string pa = asignacionBase();
		if (pa.empty()) { avisar("No hay PlayerAssignment.bin en Phoenix-DB: tus fichajes entrarán solo con Editar → Cargar"); _e.paraMiBase.clear(); return; }
		auto a = base::Asignaciones::abrir(pa);
		if (!a.ok()) { avisar("No se pudo leer PlayerAssignment.bin: " + a.error.codigo); _e.paraMiBase.clear(); return; }

		const auto ops = _e.paraMiBase;
		int cambios = 0;
		std::set<std::string> hechas;
		for (const auto& r : base::aplicarOperaciones(*a.valor, ops)) {
			if (r.estado == "aplicada" && r.motivo.empty()) { cambios++; hechas.insert(r.id); }
			else if (r.estado != "aplicada")
				for (const auto& op : ops) if (op.id == r.id) historial(op, "no en tu base", r.motivo);
		}
		if (!cambios) { _e.paraMiBase.clear(); return; }

		auto resp = respaldarAhora("antes de poner mis fichajes en mi base");
		if (!resp.ok()) {
			_esperarAplicarHasta = ahora() + 300;
			avisar("No se pudo respaldar: " + resp.error.detalle + ". Tus fichajes no se pusieron en tu base (se reintenta).");
			return;
		}
		const std::string id = entrega::nuevoId();
		const fs::path dir = aRuta(carpetaTrabajo());
		std::error_code ec;
		fs::create_directories(dir, ec);
		const std::string salida = deRuta(dir / ("PlayerAssignment.bin." + id));
		if (auto g = a.valor->guardarComo(salida); !g.ok()) { avisar("No se pudo preparar PlayerAssignment.bin: " + g.error.codigo); _e.paraMiBase.clear(); return; }
		const std::string resumen = cambios == 1 ? entrega::recortarResumen("Tu fichaje: " + grupo::resumenDe(ops.front(), _en.nombreEquipo), 60)
			: std::to_string(cambios) + " fichajes tuyos";
		auto w = entrega::escribir(carpetaEntrega(), id, resumen, { { "PlayerAssignment.bin", salida } }, fechaIso());
		if (!w.ok()) { avisar("No se pudo dejar la entrega para Link: " + w.error.detalle); return; }
		_e.entregaEnCurso = id; _e.entregaConBase = true;
		for (const auto& op : ops) if (hechas.count(op.id)) historial(op, "en tu base", "");
		_e.paraMiBase.clear();
		avisar(std::to_string(cambios) + (cambios == 1 ? " fichaje tuyo listo" : " fichajes tuyos listos") + " para «Activar»: Phoenix Link lo coloca y avisa en el juego");
	}

	void Motor::respaldoDiario() {
		if (!respaldos::faltaDiario(carpetaRespaldos(), static_cast<std::time_t>(ahora()))) return;
		auto r = respaldarAhora("diario");
		if (!r.ok() && r.error.codigo != "NADA_QUE_RESPALDAR") avisar("No se pudo hacer el respaldo diario: " + r.error.detalle);
	}

	void Motor::leerConfig(bool forzar) {
		if (_e.grupoId.empty()) { _e.config = {}; _e.config.motivoNoLeida = "sin grupo"; return; }
		const int64_t t = ahora();
		if (!forzar && t - _ultimaConfig < _en.segundosConfig) return;
		if (!_ritmo.puede("config", t)) return;
		_ultimaConfig = t;
		bool sin = false;
		auto r = _web.config(_e.grupoId, _e.config.leida ? _e.etagConfig : "", sin);
		if (r.ok()) {
			_ritmo.exito("config", t);
			if (!sin) { _e.config = *r.valor; _e.etagConfig = _e.config.etag; }
			return;
		}
		// Sin interruptor legible → lo más seguro.
		_e.config.leida = false;
		_e.config.modo = grupo::Modo::Autorizacion;
		_e.config.motivoNoLeida = r.error.codigo;
		_ritmo.fallo("config", t, r.error.codigo == "RUTA_NO_ENCONTRADA" ? 300 : _web.reintentarEn());
	}

	bool Motor::calcularCompat() {
		_compat.baseDatos = baseDatos();
		const std::string edit = editActivo();
		if (edit.empty()) return false;
		auto of = _of.leer(edit);
		if (!of.ok()) return false;
		_compat.huellaBase = of.valor->huellaEquipos;
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		_compat.parche = juego.empty() ? "desconocido" : rutas::nombreParche(juego);
		return true;
	}

	void Motor::detectarPropios() {
		const std::string edit = editActivo();
		if (edit.empty()) return;
		std::error_code ec;
		const long long tam = static_cast<long long>(fs::file_size(aRuta(edit), ec));
		if (ec) return;
		// Fecha con toda su precisión (el EDIT mide siempre lo mismo: dos guardados en el mismo segundo no deben confundirse).
		const long long fecha = static_cast<long long>(fs::last_write_time(aRuta(edit), ec).time_since_epoch().count());
		const int64_t t = ahora();
		if (tam != _obsTam || fecha != _obsFecha) { _obsTam = tam; _obsFecha = fecha; _obsDesde = t; return; }   // acaba de cambiar: esperar
		if (t - _obsDesde < _en.segundosEstable) return;
		if (tam == _anTam && fecha == _anFecha && t - _ultimoHash < 60) return;   // ya analizado (cada 60 s se vuelve a mirar la huella igual)
		_ultimoHash = t;

		const std::string h = sha256::deArchivo(edit);
		if (h.empty()) return;
		if (h == _e.ultimoHashVisto) { _anTam = tam; _anFecha = fecha; return; }
		auto of = _of.leer(edit);
		if (!of.ok()) { log("warn", "Option file ilegible (¿a medias?): " + of.error.codigo); return; }   // se reintenta en la próxima vuelta
		_anTam = tam; _anFecha = fecha;
		_compat.huellaBase = of.valor->huellaEquipos;
		_compat.baseDatos = baseDatos();
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		_compat.parche = juego.empty() ? "desconocido" : rutas::nombreParche(juego);

		const auto& actual = of.valor->plantillas;
		const bool deFuera = _e.esConocida(h) || h == _e.ultimoHashSubido;
		// Mis fichajes también a MI base (con sync-base si), se publiquen o no.
		if (_e.baseEnEntrega && _e.hayFoto && !deFuera) {
			for (const auto& m : grupo::diferencias(_e.foto, actual)) {
				if (!m.desde || !m.hacia) continue;   // altas sin origen y bajas: la base aún no las hace
				grupo::Operacion op;
				op.id = "mia-" + entrega::nuevoId().substr(5);
				op.tipo = "mover";
				op.jugador = m.jugador; op.equipoOrigen = m.desde; op.equipoDestino = m.hacia;
				if (of.valor->dorsales.count(m.hacia) && of.valor->dorsales.at(m.hacia).count(m.jugador))
					op.dorsal = of.valor->dorsales.at(m.hacia).at(m.jugador);
				op.autorPc = _en.nombrePc; op.creadoEn = fechaIso();
				_e.paraMiBase.push_back(op);
			}
			while (_e.paraMiBase.size() > 500) _e.paraMiBase.erase(_e.paraMiBase.begin());
		}
		if (!_e.hayFoto || deFuera || !_e.publicar || _e.grupoId.empty()) {
			// Primera foto, cambio que llegó de fuera (anti-bucle) o no se publica: solo se actualiza la base.
			if (deFuera) log("info", "Option file con huella conocida: no se publica (anti-bucle)");
			_e.foto = actual; _e.hayFoto = true; _e.ultimoHashVisto = h;
			return;
		}

		const auto movs = grupo::diferencias(_e.foto, actual);
		int n = 0;
		for (const auto& m : movs) {
			if (!m.desde) {   // alta sin equipo de origen: aún no se comparte
				_e.anotarHistorial({ fechaIso(), "", "Jugador " + std::to_string(m.jugador) + " apareció en el equipo " + std::to_string(m.hacia), _en.nombrePc, "no compartido", "altas sin origen todavía no se comparten" });
				continue;
			}
			grupo::Operacion op;
			op.id = "op-" + entrega::nuevoId().substr(5);
			op.tipo = m.hacia ? "mover" : "quitar";
			op.jugador = m.jugador; op.equipoOrigen = m.desde; op.equipoDestino = m.hacia;
			if (m.hacia && of.valor->dorsales.count(m.hacia) && of.valor->dorsales.at(m.hacia).count(m.jugador))
				op.dorsal = of.valor->dorsales.at(m.hacia).at(m.jugador);
			op.autorPc = _en.nombrePc; op.creadoEn = fechaIso();
			op.base = _e.ultimoHashVisto; op.shaResultado = h; op.baseSeq = _e.ultimaSeq;
			op.compat = _compat;
			op.resumen = entrega::recortarResumen(grupo::resumenDe(op, _en.nombreEquipo), 60);
			_e.porPublicar.push_back(op);
			_e.anotarHistorial({ fechaIso(), op.id, op.resumen, _en.nombrePc, "por publicar", "" });
			n++;
		}
		while (_e.porPublicar.size() > 500) _e.porPublicar.erase(_e.porPublicar.begin());
		if (n) log("info", std::to_string(n) + " fichaje(s) propios detectados");
		_e.foto = actual; _e.hayFoto = true; _e.ultimoHashVisto = h; _e.ultimoHashSubido = h;
	}

	void Motor::publicarPendientes() {
		if (_e.grupoId.empty() || !_e.publicar || _e.porPublicar.empty()) return;
		const int64_t t = ahora();
		if (!_ritmo.puede("publicar", t)) return;
		const auto op = _e.porPublicar.front();
		auto r = _web.publicar(_e.grupoId, op);
		if (r.ok() || r.error.codigo == "OPERACION_DUPLICADA") {   // duplicada = ya estaba subida (reintento)
			_ritmo.exito("publicar", t);
			_e.propias.insert(op.id);
			_e.porPublicar.erase(_e.porPublicar.begin());
			for (auto it = _e.historial.rbegin(); it != _e.historial.rend(); ++it)
				if (it->opId == op.id) { it->estado = "publicada"; break; }
			return;
		}
		_ritmo.fallo("publicar", t, r.error.codigo == "RUTA_NO_ENCONTRADA" ? 300 : _web.reintentarEn());
	}

	Resultado<int> Motor::traer() {
		using R = Resultado<int>;
		if (_e.grupoId.empty()) return R::mal("SIN_GRUPO", "Esta PC no está en ningún grupo");
		const int64_t t = ahora();
		if (!_ritmo.puede("operaciones", t)) return R::mal("ESPERA", "Espera unos segundos (" + std::to_string(_ritmo.siguiente("operaciones") - t) + " s)");
		auto r = _web.operaciones(_e.grupoId, _e.ultimaSeq, _e.etagOperaciones);
		if (!r.ok()) {
			_ritmo.fallo("operaciones", t, r.error.codigo == "RUTA_NO_ENCONTRADA" ? 300 : _web.reintentarEn());
			return R{ std::nullopt, r.error };
		}
		_ritmo.exito("operaciones", t);
		const Lote& l = *r.valor;
		if (l.sinCambios) return R::bien(0);
		if (_compat.huellaBase.empty()) calcularCompat();
		int nuevas = 0;
		for (const auto& op : l.operaciones) {
			if (_e.propias.count(op.id) || _e.aplicadas.count(op.id)) continue;   // anti-bucle / idempotencia
			if (std::any_of(_e.porPublicar.begin(), _e.porPublicar.end(), [&](const auto& x) { return x.id == op.id; })) { _e.propias.insert(op.id); continue; }
			if (std::any_of(_e.entrantes.begin(), _e.entrantes.end(), [&](const auto& x) { return x.op.id == op.id; })) continue;
			const std::string inc = grupo::motivoIncompatible(_compat, op.compat);
			if (!inc.empty()) {
				_e.entrantes.push_back({ op, "incompatible", inc });
				historial(op, "incompatible", inc);
				confirmarLuego(op.id, "incompatible", inc);
				avisar("1 cambio omitido: " + inc);
				continue;
			}
			_e.entrantes.push_back({ op, "nuevo", "" });
			nuevas++;
		}
		while (_e.entrantes.size() > 300) _e.entrantes.erase(_e.entrantes.begin());
		_e.ultimaSeq = std::max(_e.ultimaSeq, l.hasta);
		_e.etagOperaciones = l.etag;
		return R::bien(nuevas);
	}

	void Motor::procesarEntrantes() {
		std::vector<std::string> propias;
		for (const auto& x : _e.entrantes) if (x.estado == "propia") propias.push_back(x.op.id);
		if (!propias.empty() && ahora() >= _esperarAplicarHasta) { aplicarInterno(propias, false); return; }   // lo pidió el dueño de esta PC
		std::vector<std::string> nuevos;
		for (const auto& x : _e.entrantes) if (x.estado == "nuevo") nuevos.push_back(x.op.id);
		if (nuevos.empty()) return;
		if (_e.config.leida && _e.config.modo == grupo::Modo::Automatico) {
			if (ahora() < _esperarAplicarHasta) return;
			aplicarInterno(nuevos, true);
			return;
		}
		for (auto& x : _e.entrantes)
			if (x.estado == "nuevo") { x.estado = "pendiente"; historial(x.op, "pendiente", "espera tu autorización"); }
		avisar(std::to_string(nuevos.size()) + (nuevos.size() == 1 ? " cambio del grupo espera" : " cambios del grupo esperan") + " tu autorización");
	}

	void Motor::enviarConfirmaciones() {
		if (_e.grupoId.empty() || _e.porConfirmar.empty()) return;
		const int64_t t = ahora();
		if (!_ritmo.puede("aplicada", t)) return;
		const auto c = _e.porConfirmar.front();
		auto r = _web.confirmar(_e.grupoId, c.id, c.estado, c.motivo);
		if (r.ok() || r.error.codigo == "NO_EXISTE") {
			_ritmo.exito("aplicada", t);
			_e.porConfirmar.erase(_e.porConfirmar.begin());
			return;
		}
		_ritmo.fallo("aplicada", t, r.error.codigo == "RUTA_NO_ENCONTRADA" ? 300 : _web.reintentarEn());
		while (_e.porConfirmar.size() > 500) _e.porConfirmar.erase(_e.porConfirmar.begin());
	}

	Resultado<int> Motor::aplicarInterno(const std::vector<std::string>& ids, bool automatico) {
		using R = Resultado<int>;
		std::vector<grupo::Operacion> ops;
		for (const auto& x : _e.entrantes)
			if (std::find(ids.begin(), ids.end(), x.op.id) != ids.end() && x.estado != "incompatible") ops.push_back(x.op);
		if (ops.empty()) return R::bien(0);
		std::sort(ops.begin(), ops.end(), [](const auto& a, const auto& b) { return a.seq < b.seq; });
		// Las «stats» van a Player.bin, en una entrega propia. Primero las plantillas; las stats, en la siguiente vuelta.
		{
			std::vector<grupo::Operacion> plantilla, stats;
			for (const auto& op : ops) (op.tipo == "stats" ? stats : plantilla).push_back(op);
			if (plantilla.empty()) return aplicarStats(stats, automatico);
			ops = std::move(plantilla);
		}

		if (entrega::hayPendiente(carpetaEntrega()) || !_e.entregaEnCurso.empty())
			return R::mal("ENTREGA_PENDIENTE", "Phoenix Link todavía no colocó la entrega anterior; se reintenta solo");
		const std::string edit = editActivo();
		if (edit.empty()) return R::mal("SIN_OPTION_FILE", "No se encontró tu option file (Documentos\\KONAMI\\…\\save\\EDIT00000000)");
		const std::string h0 = sha256::deArchivo(edit);

		// 1) Respaldo SIEMPRE antes. Si falla, no se aplica nada.
		auto resp = respaldarAhora("antes de aplicar cambios del grupo");
		if (!resp.ok()) {
			_esperarAplicarHasta = ahora() + 300;
			avisar("No se pudo respaldar: " + resp.error.detalle + ". No se aplicó ningún cambio.");
			return R::mal("RESPALDO_FALLIDO", resp.error.detalle);
		}

		// 2) Aplicar sobre una copia (archivo nuevo en trabajo\).
		const std::string entregaNueva = entrega::nuevoId();
		const fs::path dir = aRuta(carpetaTrabajo());
		std::error_code ec;
		fs::create_directories(dir, ec);
		const std::string salida = deRuta(dir / ("EDIT00000000." + entregaNueva));
		auto ap = _of.aplicar(edit, ops, salida);
		if (!ap.ok()) return R::mal(ap.error.codigo, "No se pudo aplicar en tu option file: " + ap.error.detalle);
		const auto& res = ap.valor->resultados;
		const int escritas = ap.valor->escritas;
		int conflictos = 0;
		std::vector<std::string> hechas, aplicadasOp;
		for (const auto& r : res) {
			if (r.estado == "aplicada") { aplicadasOp.push_back(r.id); if (r.motivo.empty()) hechas.push_back(r.id); }
			else conflictos++;
		}

		// 2b) La BASE (PlayerAssignment.bin), para que el fichaje entre con «Activar».
		//     Solo si Link ya acepta ese archivo (interruptor local) y existe en Phoenix-DB.
		std::string salidaBase;
		std::map<std::string, std::string> notaBase;   // op → motivo si no se pudo en la base
		if (_e.baseEnEntrega) {
			const std::string pa = asignacionBase();
			if (pa.empty()) avisar("No hay PlayerAssignment.bin en Phoenix-DB: el fichaje entrará solo con Editar → Cargar");
			else if (auto a = base::Asignaciones::abrir(pa); !a.ok()) avisar("No se pudo leer PlayerAssignment.bin: " + a.error.codigo);
			else {
				std::vector<grupo::Operacion> paraBase;
				for (const auto& op : ops) if (std::find(aplicadasOp.begin(), aplicadasOp.end(), op.id) != aplicadasOp.end()) paraBase.push_back(op);
				int cambiosBase = 0;
				for (const auto& r : base::aplicarOperaciones(*a.valor, paraBase)) {
					if (r.estado == "aplicada" && r.motivo.empty()) cambiosBase++;
					else if (r.estado != "aplicada") notaBase[r.id] = r.motivo;
				}
				if (cambiosBase) {
					salidaBase = deRuta(dir / ("PlayerAssignment.bin." + entregaNueva));
					auto g = a.valor->guardarComo(salidaBase);
					if (!g.ok()) { avisar("No se pudo preparar PlayerAssignment.bin: " + g.error.codigo); salidaBase.clear(); }
				}
			}
		}

		// 3) Si algo cambió de verdad: entregar a Link.
		std::string entregaId;
		if (escritas || !salidaBase.empty()) {
			entregaId = entregaNueva;
			std::vector<entrega::Archivo> archivos;
			if (escritas) {
				if (fs::file_size(aRuta(salida), ec) != fs::file_size(aRuta(edit), ec)) {
					fs::remove(aRuta(salida), ec);
					return R::mal("TAMANO_DISTINTO", "El option file nuevo no mide lo mismo que el tuyo; Link lo rechazaría. No se aplicó nada.");
				}
				if (sha256::deArchivo(edit) != h0) {
					fs::remove(aRuta(salida), ec);
					return R::mal("OPTION_CAMBIO", "Tu option file cambió mientras se preparaba el cambio; se reintenta");
				}
				archivos.push_back({ "EDIT00000000", salida });
			}
			if (!salidaBase.empty()) archivos.push_back({ "PlayerAssignment.bin", salidaBase });
			std::string resumen;
			if (hechas.size() == 1) for (const auto& op : ops) if (op.id == hechas.front()) resumen = grupo::resumenDe(op, _en.nombreEquipo);
			if (resumen.empty()) resumen = std::to_string(std::max<size_t>(hechas.size(), 1)) + " fichajes del grupo";
			auto w = entrega::escribir(carpetaEntrega(), entregaId, resumen, archivos, fechaIso());
			if (!w.ok()) return R::mal(w.error.codigo, "No se pudo dejar la entrega para Link: " + w.error.detalle);
			if (escritas) {
				_e.anotarConocida(sha256::deArchivo(salida));   // anti-bucle: cuando Link lo coloque, no es un fichaje mío
				_e.foto = ap.valor->plantillasDespues; _e.hayFoto = true;
			}
			_e.entregaEnCurso = entregaId;
			_e.entregaConBase = !salidaBase.empty();
			_e.ultima = { entregaId, resp.valor->nombre, false, {}, fechaIso() };
			// Limpieza: deja solo los 10 últimos archivos de trabajo.
			std::vector<fs::path> viejos;
			for (fs::directory_iterator it(dir, ec), fin; !ec && it != fin; it.increment(ec)) viejos.push_back(it->path());
			std::sort(viejos.begin(), viejos.end(), [&](const fs::path& x, const fs::path& y) { std::error_code e1, e2; return fs::last_write_time(x, e1) < fs::last_write_time(y, e2); });
			for (size_t i = 0; i + 10 < viejos.size(); i++) fs::remove(viejos[i], ec);
		}
		else if (fs::exists(aRuta(salida), ec)) fs::remove(aRuta(salida), ec);

		// 4) Anotar cada operación.
		for (const auto& r : res) {
			auto it = std::find_if(_e.entrantes.begin(), _e.entrantes.end(), [&](const auto& x) { return x.op.id == r.id; });
			if (it == _e.entrantes.end()) continue;
			if (r.estado == "aplicada") {
				std::string nota = r.motivo.empty() ? (automatico ? "automático" : "autorizada") : r.motivo;
				if (notaBase.count(r.id)) nota += " · base: " + notaBase[r.id];
				historial(it->op, "aplicada", nota);
				_e.aplicadas.insert(r.id);
				if (!entregaId.empty()) _e.ultima.ops.push_back(r.id);
				confirmarLuego(r.id, "aplicada", r.motivo);
				_e.entrantes.erase(it);
			}
			else {
				it->estado = "conflicto"; it->motivo = r.motivo;
				historial(it->op, "conflicto", r.motivo);
				confirmarLuego(r.id, "conflicto", r.motivo);
			}
		}
		if (conflictos) avisar(std::to_string(conflictos) + (conflictos == 1 ? " cambio omitido: conflicto" : " cambios omitidos: conflicto"));
		const int listos = static_cast<int>(entregaId.empty() ? 0 : std::max<size_t>(hechas.size(), 1));
		if (listos) avisar(std::to_string(listos) + (listos == 1 ? " fichaje del grupo listo" : " fichajes del grupo listos") + ": Phoenix Link lo coloca y avisa en el juego");
		return R::bien(listos);
	}

	void Motor::anotarResultado(const grupo::ResultadoOp& r, bool automatico, bool conEntrega) {
		auto it = std::find_if(_e.entrantes.begin(), _e.entrantes.end(), [&](const auto& x) { return x.op.id == r.id; });
		if (it == _e.entrantes.end()) return;
		const bool propia = it->estado == "propia";
		if (r.estado == "aplicada") {
			historial(it->op, "aplicada", r.motivo.empty() ? (propia ? "pedido en esta PC" : automatico ? "automático" : "autorizada") : r.motivo);
			_e.aplicadas.insert(r.id);
			if (conEntrega) _e.ultima.ops.push_back(r.id);
			if (!propia) confirmarLuego(r.id, "aplicada", r.motivo);
			_e.entrantes.erase(it);
		}
		else {
			historial(it->op, "conflicto", r.motivo);
			if (!propia) confirmarLuego(r.id, "conflicto", r.motivo);
			if (propia) { avisar("No se pudo: " + r.motivo); _e.aplicadas.insert(r.id); _e.entrantes.erase(it); }
			else { it->estado = "conflicto"; it->motivo = r.motivo; }
		}
	}

	// Stats: se escriben en una COPIA del Player.bin de Phoenix-DB y Link la coloca (con respaldo antes).
	Resultado<int> Motor::aplicarStats(const std::vector<grupo::Operacion>& ops, bool automatico) {
		using R = Resultado<int>;
		if (ops.empty()) return R::bien(0);
		if (entrega::hayPendiente(carpetaEntrega()) || !_e.entregaEnCurso.empty())
			return R::mal("ENTREGA_PENDIENTE", "Phoenix Link todavía no colocó la entrega anterior; se reintenta solo");
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		const auto bases = rutas::buscarPlayerBin(juego);
		if (bases.empty()) {
			for (const auto& op : ops) anotarResultado({ op.id, "conflicto", "no hay Player.bin en Phoenix-DB (usa sync-juego)" }, automatico, false);
			return R::mal("SIN_PLAYERBIN", "No hay Player.bin en Phoenix-DB");
		}
		auto fb = base::FichasBase::abrir(bases.front().playerBin);
		if (!fb.ok()) return R::mal(fb.error.codigo, "No se pudo leer Player.bin: " + fb.error.detalle);
		const auto antes = *fb.valor;

		std::vector<grupo::ResultadoOp> res;
		int cambios = 0;
		for (const auto& op : ops) {
			auto actual = fb.valor->leer(op.jugador, op.campo);
			if (!actual.ok()) { res.push_back({ op.id, "conflicto", actual.error.codigo + (actual.error.detalle.empty() ? "" : ": " + actual.error.detalle) }); continue; }
			if (*actual.valor == op.valor) { res.push_back({ op.id, "aplicada", "ya tenía ese valor" }); continue; }
			auto p = fb.valor->poner(op.jugador, op.campo, op.valor);
			if (!p.ok()) { res.push_back({ op.id, "conflicto", p.error.codigo + ": " + p.error.detalle }); continue; }
			res.push_back({ op.id, "aplicada", "" });
			cambios++;
		}
		// Comprobación: solo cambiaron los jugadores pedidos.
		for (uint32_t j : fb.valor->distintos(antes))
			if (std::none_of(ops.begin(), ops.end(), [&](const auto& o) { return o.jugador == j; }))
				return R::mal("PLAYERBIN_VERIFICACION", "Cambió un jugador que no se pidió; no se aplicó nada");

		std::string entregaId;
		if (cambios) {
			auto resp = respaldarAhora("antes de aplicar stats");
			if (!resp.ok()) {
				_esperarAplicarHasta = ahora() + 300;
				avisar("No se pudo respaldar: " + resp.error.detalle + ". No se aplicó ningún cambio de stats.");
				return R::mal("RESPALDO_FALLIDO", resp.error.detalle);
			}
			const std::string id = entrega::nuevoId();
			const fs::path dir = aRuta(carpetaTrabajo());
			std::error_code ec;
			fs::create_directories(dir, ec);
			const std::string salida = deRuta(dir / ("Player.bin." + id));
			if (auto g = fb.valor->guardarComo(salida); !g.ok()) return R::mal(g.error.codigo, "No se pudo preparar Player.bin: " + g.error.detalle);
			std::string resumen;
			for (const auto& op : ops) if (resumen.empty()) resumen = grupo::resumenDe(op, _en.nombreEquipo);
			if (cambios > 1) resumen = std::to_string(cambios) + " cambios de stats";
			auto w = entrega::escribir(carpetaEntrega(), id, entrega::recortarResumen(resumen, 60), { { "Player.bin", salida } }, fechaIso());
			if (!w.ok()) return R::mal(w.error.codigo, "No se pudo dejar la entrega para Link: " + w.error.detalle);
			entregaId = id;
			_e.entregaEnCurso = id; _e.entregaConBase = true;
			_e.ultima = { id, resp.valor->nombre, true, {}, fechaIso() };
		}
		for (const auto& r : res) anotarResultado(r, automatico, !entregaId.empty());
		if (cambios) avisar(std::to_string(cambios) + (cambios == 1 ? " cambio de stats listo" : " cambios de stats listos") + ": Phoenix Link lo coloca y avisa en el juego");
		return R::bien(cambios);
	}

	Resultado<std::string> Motor::cambiarStats(uint32_t jugador, const std::string& campo, int valor) {
		using R = Resultado<std::string>;
		const auto* c = base::campoFicha(campo);
		if (!c) return R::mal("CAMPO_DESCONOCIDO", campo);
		if (valor < c->minimo || valor > c->maximo) return R::mal("VALOR_FUERA_DE_RANGO", std::string(c->texto) + ": " + std::to_string(c->minimo) + "–" + std::to_string(c->maximo));
		if (!jugador) return R::mal("JUGADOR_NO_EXISTE", "0");
		grupo::Operacion op;
		op.id = "op-" + entrega::nuevoId().substr(5);
		op.tipo = "stats"; op.jugador = jugador; op.campo = campo; op.valor = valor;
		op.autorPc = _en.nombrePc; op.creadoEn = fechaIso(); op.baseSeq = _e.ultimaSeq;
		if (_compat.huellaBase.empty()) calcularCompat();
		op.compat = _compat;
		op.resumen = entrega::recortarResumen(grupo::resumenDe(op, _en.nombreEquipo), 60);
		_e.entrantes.push_back({ op, "propia", "" });   // se aplica aquí en la próxima vuelta (o ya, abajo)
		if (!_e.grupoId.empty() && _e.publicar) { _e.porPublicar.push_back(op); _e.anotarHistorial({ fechaIso(), op.id, op.resumen, _en.nombrePc, "por publicar", "" }); }
		procesarEntrantes();
		guardar();
		return R::bien(op.id);
	}

	// --- Option file oficial ---------------------------------------------------------------
	void Motor::revisarOficial() {
		if (_e.grupoId.empty()) return;
		const int64_t t = ahora();
		if (t - _ultimoOficial < _en.segundosOficial) return;
		if (!_ritmo.puede("oficial", t)) return;
		_ultimoOficial = t;
		auto o = _web.oficial(_e.grupoId);
		if (!o.ok()) {
			_ritmo.fallo("oficial", t, (o.error.codigo == "RUTA_NO_ENCONTRADA" || o.error.codigo == "SIN_OFICIAL") ? 300 : _web.reintentarEn());
			return;
		}
		_ritmo.exito("oficial", t);
		if (o.valor->version <= _e.versionOficial) return;
		// La primera foto (PC nueva en el grupo) siempre se pone; después, solo en modo automático.
		const bool auto_ = _e.versionOficial == 0 || (_e.config.leida && _e.config.modo == grupo::Modo::Automatico);
		if (!auto_) {
			if (_avisadoOficial != o.valor->version) { avisar("Hay un option file oficial nuevo (versión " + std::to_string(o.valor->version) + "): PhoenixSync sync-oficial"); _avisadoOficial = o.valor->version; }
			return;
		}
		auto r = ponerOficial(*o.valor);
		if (!r.ok()) {
			if (r.error.codigo == "ENTREGA_PENDIENTE") _ultimoOficial = t - _en.segundosOficial + 10;   // reintento pronto
			else avisar("No se pudo poner el option file oficial: " + r.error.detalle);
		}
	}

	Resultado<int> Motor::bajarOficial(bool forzar) {
		using R = Resultado<int>;
		if (_e.grupoId.empty()) return R::mal("SIN_GRUPO", "Esta PC no está en ningún grupo");
		auto o = _web.oficial(_e.grupoId);
		if (!o.ok()) return R{ std::nullopt, o.error };
		if (!forzar && o.valor->version <= _e.versionOficial) return R::bien(0);
		auto r = ponerOficial(*o.valor);
		guardar();
		return r;
	}

	Resultado<int> Motor::ponerOficial(const Oficial& o) {
		using R = Resultado<int>;
		if (entrega::hayPendiente(carpetaEntrega()) || !_e.entregaEnCurso.empty())
			return R::mal("ENTREGA_PENDIENTE", "Phoenix Link todavía no colocó la entrega anterior; se reintenta solo");
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		const std::string parche = juego.empty() ? "desconocido" : rutas::nombreParche(juego);
		if (!o.parche.empty() && o.parche != "desconocido" && parche != "desconocido" && o.parche != parche)
			return R::mal("INCOMPATIBLE", "La foto oficial es del parche «" + o.parche + "» y tú tienes «" + parche + "»");
		const std::string edit = editActivo();
		if (edit.empty()) return R::mal("SIN_OPTION_FILE", "No se encontró tu option file (Documentos\\KONAMI\\…\\save\\EDIT00000000)");
		const bool hayBase = !asignacionBase().empty();
		const bool hayPlayer = !rutas::buscarPlayerBin(juego).empty();
		// ¿Cambio de base de datos (p. ej. de la provisional a la oficial del Phoenix Patch)?
		const bool cambioDeBase = !o.baseDatos.empty() && o.baseDatos != baseDatos();
		if (cambioDeBase) {
			auto trae = [&](const char* n) { return std::any_of(o.archivos.begin(), o.archivos.end(), [&](const auto& a) { return a.nombre == n; }); };
			if (!trae("Player.bin") || !trae("PlayerAssignment.bin"))
				return R::mal("CAMBIO_DE_BASE_INCOMPLETO", "La foto cambia la base de datos a «" + o.baseDatos + "» pero no trae Player.bin y PlayerAssignment.bin");
			if (!hayBase || !hayPlayer)
				return R::mal("SIN_PHOENIX_DB", std::string("Para cambiar de base hace falta la carpeta ") + rutas::kCarpetaBase + " con Player.bin y PlayerAssignment.bin (Phoenix Link la instala)");
		}

		const fs::path dir = aRuta(carpetaTrabajo());
		std::error_code ec;
		fs::create_directories(dir, ec);
		std::vector<entrega::Archivo> archivos;
		std::string rutaEdit, shaEdit;
		for (const auto& a : o.archivos) {
			if (a.nombre == "PlayerAssignment.bin" && !hayBase) { avisar("La foto oficial trae PlayerAssignment.bin pero no tienes Phoenix-DB: se omite"); continue; }
			if (a.nombre == "Player.bin" && !hayPlayer) { avisar("La foto oficial trae Player.bin pero no tienes Phoenix-DB: se omite"); continue; }
			const std::string destino = deRuta(dir / (a.nombre + ".oficial-v" + std::to_string(o.version)));
			fs::remove(aRuta(destino), ec);
			auto d = _web.descargar(a, destino);
			if (!d.ok()) return R{ std::nullopt, d.error };
			if (a.nombre == "EDIT00000000") { rutaEdit = destino; shaEdit = a.sha256; }
			archivos.push_back({ a.nombre, destino });
		}
		if (rutaEdit.empty()) return R::mal("RESPUESTA_INVALIDA", "La foto oficial no trae el option file");
		if (fs::file_size(aRuta(rutaEdit), ec) != fs::file_size(aRuta(edit), ec))
			return R::mal("TAMANO_DISTINTO", "El option file oficial no mide lo mismo que el tuyo (¿otro parche?); Link lo rechazaría");
		auto leido = _of.leer(rutaEdit);
		if (!leido.ok()) return R::mal("OPTION_INVALIDO", "El option file oficial no se pudo leer: " + leido.error.codigo);

		auto resp = respaldarAhora("antes de la foto oficial v" + std::to_string(o.version));
		if (!resp.ok()) return R::mal("RESPALDO_FALLIDO", resp.error.detalle);
		const std::string id = entrega::nuevoId();
		auto w = entrega::escribir(carpetaEntrega(), id, "Option file oficial v" + std::to_string(o.version), archivos, fechaIso());
		if (!w.ok()) return R::mal(w.error.codigo, "No se pudo dejar la entrega para Link: " + w.error.detalle);

		_e.anotarConocida(shaEdit);                       // anti-bucle: no es un fichaje mío
		_e.foto = leido.valor->plantillas; _e.hayFoto = true; _e.ultimoHashVisto = shaEdit;
		_e.entregaEnCurso = id;
		_e.entregaConBase = archivos.size() > 1;
		_e.ultima = { id, resp.valor->nombre, false, {}, fechaIso() };
		_e.versionOficial = o.version;
		if (cambioDeBase) {   // lo pendiente era de la base anterior: no vale para la nueva
			for (const auto& x : _e.entrantes) { historial(x.op, "omitida", "de la base anterior (" + baseDatos() + ")"); _e.aplicadas.insert(x.op.id); if (x.estado != "propia") confirmarLuego(x.op.id, "omitida", "cambio de base de datos"); }
			_e.entrantes.clear();
			for (const auto& op : _e.porPublicar) historial(op, "omitida", "no se publicó: cambió la base de datos");
			_e.porPublicar.clear();
			_e.anotarHistorial({ fechaIso(), "", "Cambio de base de datos: " + baseDatos() + " → " + o.baseDatos, _en.nombrePc, "aplicada", "" });
		}
		if (!o.baseDatos.empty()) { _e.baseDatos = o.baseDatos; _compat.baseDatos = o.baseDatos; }
		if (o.seq > _e.ultimaSeq) { _e.ultimaSeq = o.seq; _e.etagOperaciones.clear(); }
		_e.paraMiBase.clear();                           // la base oficial manda
		for (auto it = _e.entrantes.begin(); it != _e.entrantes.end();) {   // lo que ya viene dentro de la foto
			if (it->op.seq > 0 && it->op.seq <= o.seq) {
				historial(it->op, "aplicada", "incluida en la foto oficial v" + std::to_string(o.version));
				_e.aplicadas.insert(it->op.id);
				confirmarLuego(it->op.id, "aplicada", "incluida en la foto oficial");
				it = _e.entrantes.erase(it);
			}
			else ++it;
		}
		_e.anotarHistorial({ fechaIso(), "", "Option file oficial v" + std::to_string(o.version) + (o.publicadoPor.empty() ? "" : " (de " + o.publicadoPor + ")"), _en.nombrePc, "aplicada", "" });
		avisar("Option file oficial v" + std::to_string(o.version) + " listo: Phoenix Link lo coloca y avisa en el juego");
		return R::bien(1);
	}

	Resultado<int64_t> Motor::publicarOficial(const std::string& baseNueva) {
		using R = Resultado<int64_t>;
		const std::string etiqueta = baseNueva.empty() ? baseDatos() : baseNueva;
		if (etiqueta.size() > 64 || etiqueta.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_.") != std::string::npos)
			return R::mal("BASE_INVALIDA", "La etiqueta de base solo admite minúsculas, números, - _ . (máx. 64)");
		if (_e.grupoId.empty()) return R::mal("SIN_GRUPO", "Esta PC no está en ningún grupo");
		const std::string edit = editActivo();
		if (edit.empty()) return R::mal("SIN_OPTION_FILE", "No se encontró tu option file");
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		std::vector<std::pair<std::string, std::string>> locales = { { "EDIT00000000", edit } };
		if (const std::string pa = asignacionBase(); !pa.empty()) locales.push_back({ "PlayerAssignment.bin", pa });
		if (const auto pb = rutas::buscarPlayerBin(juego); !pb.empty()) locales.push_back({ "Player.bin", pb.front().playerBin });
		if (etiqueta != baseDatos() && locales.size() < 3) return R::mal("CAMBIO_DE_BASE_INCOMPLETO", "Para cambiar de base hacen falta Player.bin y PlayerAssignment.bin en tu Phoenix-DB");
		// Copia quieta de cada archivo (el juego podría estar escribiendo): se sube la copia.
		const fs::path dir = aRuta(carpetaTrabajo());
		std::error_code ec;
		fs::create_directories(dir, ec);
		std::vector<ArchivoOficial> subidos;
		for (const auto& [nombre, ruta] : locales) {
			const fs::path copia = dir / (nombre + ".subida");
			fs::copy_file(aRuta(ruta), copia, fs::copy_options::overwrite_existing, ec);
			if (ec) return R::mal("ARCHIVO_ERROR", "No se pudo copiar " + ruta);
			const std::string sha = sha256::deArchivo(deRuta(copia));
			const long long tam = static_cast<long long>(fs::file_size(copia, ec));
			auto s = _web.pedirSubida(_e.grupoId, nombre, sha, tam);
			if (!s.ok()) return R{ std::nullopt, s.error };
			auto u = _web.subirArchivo(s.valor->url, deRuta(copia));
			if (!u.ok()) return R{ std::nullopt, u.error };
			subidos.push_back(*s.valor);
			if (nombre == "EDIT00000000") _e.anotarConocida(sha);
		}
		// La huella de la tabla de equipos va con la foto: la web la pone en los cambios que crea desde el panel,
		// así las PCs comprueban que son del mismo parche (motivoIncompatible).
		std::string huella;
		if (auto of = _of.leer(deRuta(dir / "EDIT00000000.subida")); of.ok()) huella = of.valor->huellaEquipos;
		if (huella.empty()) return R::mal("OPTION_INVALIDO", "No se pudo leer tu option file para calcular la huella");
		auto v = _web.publicarOficial(_e.grupoId, juego.empty() ? "desconocido" : rutas::nombreParche(juego), huella, etiqueta, _e.ultimaSeq, subidos);
		if (!v.ok()) return R{ std::nullopt, v.error };
		_e.versionOficial = *v.valor;
		_e.baseDatos = etiqueta; _compat.baseDatos = etiqueta;
		_e.anotarHistorial({ fechaIso(), "", "Publicaste el option file oficial v" + std::to_string(*v.valor), _en.nombrePc, "publicada", "" });
		guardar();
		return R::bien(*v.valor);
	}

	json Motor::resumen() const {
		json pend = json::array(), hist = json::array();
		for (const auto& x : _e.entrantes)
			pend.push_back({ { "id", x.op.id }, { "seq", x.op.seq }, { "resumen", grupo::resumenDe(x.op, _en.nombreEquipo) },
				{ "autor", x.op.autor.empty() ? x.op.autorPc : x.op.autor }, { "estado", x.estado }, { "motivo", x.motivo } });
		const size_t desde = _e.historial.size() > 20 ? _e.historial.size() - 20 : 0;
		for (size_t i = _e.historial.size(); i-- > desde;) {
			const auto& h = _e.historial[i];
			hist.push_back({ { "fecha", h.fecha }, { "resumen", h.resumen }, { "autor", h.autor }, { "estado", h.estado }, { "motivo", h.motivo } });
		}
		return {
			{ "grupo_id", _e.grupoId }, { "pausado", _e.pausado }, { "publicar", _e.publicar },
			{ "modo", grupo::aTexto(_e.config.leida ? _e.config.modo : grupo::Modo::Autorizacion) },
			{ "modo_leido_de_la_web", _e.config.leida }, { "modo_motivo", _e.config.motivoNoLeida },
			{ "modo_puesto_por", _e.config.actualizadoPor }, { "modo_puesto_en", _e.config.actualizadoEn },
			{ "nota_modo", "Lo decide el administrador desde la web" },
			{ "ultima_seq", _e.ultimaSeq }, { "por_publicar", _e.porPublicar.size() }, { "por_confirmar", _e.porConfirmar.size() },
			{ "entrantes", pend }, { "historial", hist },
			{ "ultimo_cambio_aplicado", { { "fecha", _e.ultima.fecha }, { "ops", _e.ultima.ops.size() }, { "respaldo", _e.ultima.respaldo } } },
			{ "entrega_en_curso", _e.entregaEnCurso }, { "version_oficial", _e.versionOficial }, { "base_datos", baseDatos() } };
	}

}
