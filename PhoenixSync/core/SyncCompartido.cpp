#include "SyncCompartido.h"
#include "AsignacionBase.h"
#include "Entrega.h"
#include "RutasJuego.h"
#include "Sha256.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <map>

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
		detectarPropios();
		publicarPendientes();
		traer();
		procesarEntrantes();
		enviarConfirmaciones();
		guardar();
	}

	void Motor::revisarEntrega() {
		if (_e.entregaEnCurso.empty()) return;
		const int st = entrega::estadoDe(carpetaEntrega(), _e.entregaEnCurso);
		if (st == 1) { log("info", "Link colocó la entrega " + _e.entregaEnCurso); _e.entregaEnCurso.clear(); }
		else if (st == -1) {
			avisar("Phoenix Link rechazó la entrega (mira su historial). Tu option file no cambió.");
			_e.anotarHistorial({ fechaIso(), "", "Link rechazó la entrega " + _e.entregaEnCurso, _en.nombrePc, "rechazada", "ver historial.log de Link" });
			_e.entregaEnCurso.clear();
		}
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
		const std::string juego = _en.carpetaJuego ? _en.carpetaJuego() : std::string();
		_compat.parche = juego.empty() ? "desconocido" : rutas::nombreParche(juego);

		const auto& actual = of.valor->plantillas;
		const bool deFuera = _e.esConocida(h) || h == _e.ultimoHashSubido;
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
			_e.ultima = { entregaId, resp.valor->nombre, {}, fechaIso() };
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
			{ "entrega_en_curso", _e.entregaEnCurso } };
	}

}
