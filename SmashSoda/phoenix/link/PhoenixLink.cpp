#include "PhoenixLink.h"

#include <windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <nlohmann/json.hpp>

#include "Http.h"
#include "../PhoenixRoles.h"
#include "../../helpers/PathHelper.h"

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")

using json = nlohmann::json;

namespace phoenix {

	namespace {
		const char* kBase = "https://fiibiyijojkxqlsrhcil.supabase.co/functions/v1/phoenix";
		const char* kVersionApp = "7.0.4";

		long long ahoraMs() {
			using namespace std::chrono;
			return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
		}

		std::string carpeta() { return PathHelper::GetConfigPath(); }

		// --- Token cifrado con DPAPI (solo este usuario de Windows lo descifra) ---
		bool guardarToken(const std::string& token) {
			DATA_BLOB entrada{ static_cast<DWORD>(token.size()), (BYTE*)token.data() }, salida{};
			if (!CryptProtectData(&entrada, L"PhoenixLink", nullptr, nullptr, nullptr, 0, &salida)) return false;
			std::ofstream f(carpeta() + "phoenix-token.dat", std::ios::binary | std::ios::trunc);
			f.write(reinterpret_cast<char*>(salida.pbData), salida.cbData);
			LocalFree(salida.pbData);
			return f.good();
		}

		std::string leerToken() {
			std::ifstream f(carpeta() + "phoenix-token.dat", std::ios::binary);
			if (!f) return std::string();
			std::string datos((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
			DATA_BLOB entrada{ static_cast<DWORD>(datos.size()), (BYTE*)datos.data() }, salida{};
			if (!CryptUnprotectData(&entrada, nullptr, nullptr, nullptr, nullptr, 0, &salida)) return std::string();
			std::string token(reinterpret_cast<char*>(salida.pbData), salida.cbData);
			LocalFree(salida.pbData);
			return token;
		}

		void borrarToken() {
			DeleteFileA((carpeta() + "phoenix-token.dat").c_str());
			DeleteFileA((carpeta() + "phoenix-cuenta.txt").c_str());
		}

		void guardarUsuario(const std::string& nombre) {
			std::ofstream f(carpeta() + "phoenix-cuenta.txt", std::ios::trunc);
			f << nombre;
		}

		std::string leerUsuario() {
			std::ifstream f(carpeta() + "phoenix-cuenta.txt");
			std::string s;
			std::getline(f, s);
			return s;
		}

		// --- Huella SHA-256 del exe en ejecución (cabecera X-Phoenix-Build) ---
		std::string huellaExe() {
			static std::string cache;
			if (!cache.empty()) return cache;
			try {
				char ruta[MAX_PATH] = {};
				GetModuleFileNameA(nullptr, ruta, MAX_PATH);
				std::ifstream f(ruta, std::ios::binary);
				std::string datos((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
				BCRYPT_ALG_HANDLE alg = nullptr;
				BCRYPT_HASH_HANDLE h = nullptr;
				unsigned char resumen[32] = {};
				if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0) {
					if (BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0) == 0) {
						BCryptHashData(h, (PUCHAR)datos.data(), static_cast<ULONG>(datos.size()), 0);
						BCryptFinishHash(h, resumen, 32, 0);
						BCryptDestroyHash(h);
					}
					BCryptCloseAlgorithmProvider(alg, 0);
				}
				static const char* hex = "0123456789abcdef";
				for (unsigned char b : resumen) { cache += hex[b >> 4]; cache += hex[b & 15]; }
			}
			catch (...) {}
			return cache;
		}

		std::string nombrePc() {
			char nombre[64] = {};
			DWORD tam = sizeof(nombre);
			GetComputerNameA(nombre, &tam);
			std::string s(nombre);
			return s.empty() ? "Mi PC" : s.substr(0, 40);
		}

		/// Texto de un campo que puede venir null o ausente.
		std::string textoDe(const json& j, const char* clave) {
			if (!j.is_object() || !j.contains(clave) || j[clave].is_null()) return std::string();
			if (j[clave].is_string()) return j[clave].get<std::string>();
			return j[clave].dump();
		}

		/// Clave única para eventos (UUID v4).
		std::string uuid4() {
			unsigned char b[16] = {};
			if (BCryptGenRandom(nullptr, b, sizeof(b), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
				for (int i = 0; i < 16; i++) b[i] = static_cast<unsigned char>((GetTickCount64() >> (i % 8)) ^ (i * 37));
			}
			b[6] = static_cast<unsigned char>((b[6] & 0x0F) | 0x40);
			b[8] = static_cast<unsigned char>((b[8] & 0x3F) | 0x80);
			static const char* hex = "0123456789abcdef";
			std::string s;
			for (int i = 0; i < 16; i++) {
				if (i == 4 || i == 6 || i == 8 || i == 10) s += '-';
				s += hex[b[i] >> 4];
				s += hex[b[i] & 15];
			}
			return s;
		}

		/// Hora UTC en ISO 8601 (2026-10-06T19:00:00.000Z).
		std::string ahoraIso() {
			SYSTEMTIME t{};
			GetSystemTime(&t);
			char b[32];
			snprintf(b, sizeof(b), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
			return b;
		}

		std::vector<std::string> cabecerasCon(const std::string& token) {
			return { "Authorization: Bearer " + token, std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() };
		}

		/// Mensaje en español para cada código estable del contrato.
		std::string textoError(const std::string& codigo, const std::string& mensaje) {
			static const std::map<std::string, std::string> textos = {
				{"CODIGO_NO_ENCONTRADO", "Código incorrecto."},
				{"CODIGO_VENCIDO", "El código venció: genera otro en la web."},
				{"CODIGO_USADO", "Ese código ya se usó: genera otro en la web."},
				{"CODIGO_INVALIDO", "El código debe tener 6 dígitos."},
				{"HOST_NO_AUTORIZADO", "Tu cuenta aún no está aprobada como host."},
				{"APP_DESACTUALIZADA", "Esta versión de la app está desactualizada."},
				{"TOKEN_REVOCADO", "Esta PC fue desvinculada desde la web."},
				{"TOKEN_INVALIDO", "La vinculación ya no es válida: vuelve a vincular."},
				{"DISPOSITIVO_SUSPENDIDO", "Esta PC está suspendida por el staff."},
				{"BUILD_NO_OFICIAL", "Esta build no es oficial."},
				{"BUILD_DESACTIVADO", "Esta build fue retirada."},
				{"VERSION_DESACTIVADA", "Esta versión fue desactivada."},
				{"DEMASIADOS_INTENTOS", "Demasiados intentos: espera un momento."},
				{"NO_SON_AMIGOS", "Solo puedes invitar a tus amigos."},
				{"SALA_CERRADA", "La sala ya no está abierta en la web."},
				{"SALA_NO_ENCONTRADA", "La web aún no conoce tu sala: espera unos segundos."},
			};
			auto it = textos.find(codigo);
			return it != textos.end() ? it->second : (mensaje.empty() ? codigo : mensaje);
		}
	}

	PhoenixLink& PhoenixLink::instancia() {
		static PhoenixLink unica;
		return unica;
	}

	void PhoenixLink::iniciar() {
		if (_corriendo.exchange(true)) return;
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_token = leerToken();
			_usuario = leerUsuario();
			_estado = _token.empty() ? EstadoLink::SinVincular : EstadoLink::Conectado;
		}
		cargarEventos();
		_hilo = std::thread([this]() { bucle(); });
	}

	void PhoenixLink::detener() {
		if (!_corriendo.exchange(false)) return;
		if (_hilo.joinable()) _hilo.join();
		std::string sala;
		{
			std::lock_guard<std::mutex> lock(_mutex);
			sala = _salaId;
		}
		if (!sala.empty()) pasoCerrar(sala); // aviso final rápido (≤ 5 s)
	}

	void PhoenixLink::actualizar(const InstantaneaSala& foto) {
		std::lock_guard<std::mutex> lock(_mutex);
		_foto = foto;
		_presentes.clear();
		for (const InvitadoMuestra& inv : foto.invitados) {
			try { if (!inv.parsecId.empty()) _presentes.push_back(static_cast<uint32_t>(std::stoul(inv.parsecId))); }
			catch (...) {}
		}
	}

	void PhoenixLink::emparejar(const std::string& codigo) {
		std::lock_guard<std::mutex> lock(_mutex);
		_codigoPendiente = codigo;
		_estado = EstadoLink::Vinculando;
		_mensaje.clear();
	}

	void PhoenixLink::desvincular() {
		std::lock_guard<std::mutex> lock(_mutex);
		borrarToken();
		_token.clear();
		_usuario.clear();
		_estado = EstadoLink::SinVincular;
		_mensaje = "PC desvinculada.";
	}

	void PhoenixLink::reintentar() {
		std::lock_guard<std::mutex> lock(_mutex);
		_pausado = false;
		_espera = 0;
		_esperaHastaMs = 0;
		if (!_token.empty()) _estado = EstadoLink::Conectado;
	}

	EstadoLink PhoenixLink::estado() { std::lock_guard<std::mutex> l(_mutex); return _estado; }
	std::string PhoenixLink::usuario() { std::lock_guard<std::mutex> l(_mutex); return _usuario; }
	std::string PhoenixLink::mensaje() { std::lock_guard<std::mutex> l(_mutex); return _mensaje; }
	bool PhoenixLink::salaPublicada() { std::lock_guard<std::mutex> l(_mutex); return !_salaId.empty(); }
	int PhoenixLink::jugadoresEnLista() { std::lock_guard<std::mutex> l(_mutex); return _jugadores; }

	// -------------------------------------------------------------------------
	void PhoenixLink::fallo(const std::string& codigo, const std::string& mensaje, int reintentarEn) {
		// Llamar con _mutex tomado.
		_mensaje = textoError(codigo, mensaje);
		if (codigo.rfind("TOKEN_", 0) == 0) {
			borrarToken();
			_token.clear();
			_usuario.clear();
			_estado = EstadoLink::SinVincular;
			_salaId.clear();
			return;
		}
		if (codigo == "HOST_NO_AUTORIZADO" || codigo == "APP_DESACTUALIZADA" || codigo == "DISPOSITIVO_SUSPENDIDO"
			|| codigo.rfind("BUILD_", 0) == 0 || codigo == "VERSION_DESACTIVADA") {
			_pausado = true;
			_estado = EstadoLink::Pausado;
			return;
		}
		// Red, ERROR_INTERNO o DEMASIADOS_INTENTOS: espera exponencial (2 s … 300 s)
		_espera = (std::min)(300, _espera == 0 ? 2 : _espera * 2);
		const int segundos = (std::max)(_espera, reintentarEn);
		_esperaHastaMs = ahoraMs() + segundos * 1000LL + (GetTickCount() % 1000);
		_estado = EstadoLink::SinConexion;
	}

	void PhoenixLink::bucle() {
		while (_corriendo) {
			try {
				std::string codigo, token, salaId;
				InstantaneaSala foto;
				bool pausado;
				long long esperaHasta, proximoLatido;
				std::string firmaPublicada;
				{
					std::lock_guard<std::mutex> lock(_mutex);
					codigo.swap(_codigoPendiente);
					token = _token; salaId = _salaId; foto = _foto; pausado = _pausado;
					esperaHasta = _esperaHastaMs; proximoLatido = _proximoLatidoMs; firmaPublicada = _firmaPrefs;
				}

				if (!codigo.empty()) pasoEmparejar(codigo);
				else if (!token.empty() && !pausado && ahoraMs() >= esperaHasta) {
					const std::string firma = foto.visibilidad + "|" + (foto.aceptaEspectadores ? "1" : "0")
						+ "|" + std::to_string(foto.limiteEspectadores) + "|" + std::to_string(foto.plazasTotal)
						+ "|" + foto.juego + "|" + foto.parche + "|" + foto.region;
					if (foto.abierta && (salaId.empty() || firma != firmaPublicada)) pasoAbrir(foto);
					else if (foto.abierta && ahoraMs() >= proximoLatido) pasoLatido(foto);
					else if (foto.abierta) {
						std::string diag;
						{ std::lock_guard<std::mutex> lock(_mutex); diag = _salaDiagnosticada; }
						if (diag != salaId) pasoDiagnostico(salaId);
					}
					else if (!foto.abierta && !salaId.empty()) {
						pasoCerrar(salaId);
						std::lock_guard<std::mutex> lock(_mutex);
						_salaId.clear();
						_jugadores = 0;
						PhoenixRoles::instancia().limpiarWeb();
					}

					// Interfaz nueva: un paso opcional por vuelta (pedidos, eventos, cartas, presencia, amigos)
					bool sigue = false;
					{
						std::lock_guard<std::mutex> lock(_mutex);
						sigue = _corriendo && !_token.empty() && !_pausado && ahoraMs() >= _esperaHastaMs;
						salaId = _salaId;
					}
					if (sigue) pasoOpcional(token, salaId, foto);
				}
			}
			catch (...) {
				// Nada de aquí puede tumbar la app.
			}
			for (int i = 0; i < 4 && _corriendo; i++) Sleep(250);
		}
	}

	void PhoenixLink::pasoEmparejar(const std::string& codigo) {
		const json cuerpo = { {"codigo", codigo}, {"nombre_pc", nombrePc()}, {"version_app", kVersionApp} };
		const http::Respuesta r = http::peticion("POST", std::string(kBase) + "/v1/emparejar", cuerpo.dump(),
			{ std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() });

		std::lock_guard<std::mutex> lock(_mutex);
		const json j = json::parse(r.cuerpo, nullptr, false);
		if (r.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			const std::string token = j.value("token", "");
			const std::string nombre = j.contains("usuario") ? j["usuario"].value("nombre", "") : "";
			if (!token.empty() && guardarToken(token)) {
				_token = token;
				_usuario = nombre;
				guardarUsuario(nombre);
				_estado = EstadoLink::Conectado;
				_pausado = false;
				_mensaje = "PC vinculada a " + nombre + ".";
				return;
			}
			_estado = EstadoLink::SinVincular;
			_mensaje = "No se pudo guardar la vinculación en esta PC.";
			return;
		}
		_estado = EstadoLink::SinVincular;
		_mensaje = r.estado == 0 ? "Sin conexión con la web. Revisa tu internet."
			: textoError(j.is_discarded() ? "" : j.value("codigo", ""), j.is_discarded() ? "" : j.value("mensaje", ""));
	}

	void PhoenixLink::pasoAbrir(const InstantaneaSala& foto) {
		std::string token;
		{ std::lock_guard<std::mutex> lock(_mutex); token = _token; }

		json cuerpo = {
			{"plazas_total", (std::max)(1, (std::min)(16, foto.plazasTotal))},
			{"juego", foto.juego.empty() ? std::string("eFootball PES 2021") : foto.juego},
			{"visibilidad", foto.visibilidad},
			{"acepta_espectadores", foto.aceptaEspectadores},
			{"limite_espectadores", foto.aceptaEspectadores ? (std::max)(0, (std::min)(16, foto.limiteEspectadores)) : 0},
			{"modo", "amistoso"},
			{"publicar_en_pagina", foto.visibilidad != "privada"},
		};
		if (foto.enlace.rfind("https://", 0) == 0) cuerpo["enlace"] = foto.enlace;
		if (!foto.parche.empty()) cuerpo["parche"] = foto.parche;
		if (!foto.region.empty()) cuerpo["region"] = foto.region;

		const http::Respuesta r = http::peticion("POST", std::string(kBase) + "/v1/sala/abrir", cuerpo.dump(),
			{ "Authorization: Bearer " + token, std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() });
		const json j = json::parse(r.cuerpo, nullptr, false);

		std::lock_guard<std::mutex> lock(_mutex);
		if (r.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			_salaId = j.value("sala_id", "");
			_latidoSeg = (std::max)(10, j.value("latido_seg", 30));
			_proximoLatidoMs = ahoraMs() + (std::max)(10, _latidoSeg) * 1000LL; // primer latido ≥ 10 s
			_firmaPrefs = foto.visibilidad + "|" + (foto.aceptaEspectadores ? "1" : "0")
				+ "|" + std::to_string(foto.limiteEspectadores) + "|" + std::to_string(foto.plazasTotal)
				+ "|" + foto.juego + "|" + foto.parche + "|" + foto.region;
			_estado = EstadoLink::Conectado;
			_espera = 0;
			_mensaje = foto.visibilidad == "publica" ? "Sala publicada en el radar de retos."
				: foto.visibilidad == "amigos" ? "Sala visible para tus amigos." : "Sala privada: solo con enlace.";
			if (j.contains("roles")) aplicarRoles(j["roles"].dump());
			return;
		}
		if (r.estado == 0) { fallo("RED", "", 0); return; }
		fallo(j.is_discarded() ? "ERROR_INTERNO" : j.value("codigo", "ERROR_INTERNO"),
			j.is_discarded() ? "" : j.value("mensaje", ""), j.is_discarded() ? 0 : j.value("reintentar_en", 0));
	}

	void PhoenixLink::pasoLatido(const InstantaneaSala& foto) {
		std::string token, sala;
		{ std::lock_guard<std::mutex> lock(_mutex); token = _token; sala = _salaId; }

		json invitados = json::array();
		for (const InvitadoMuestra& inv : foto.invitados) {
			if (inv.parsecId.empty()) continue;
			json i = { {"parsec_id", inv.parsecId}, {"nombre", inv.nombre.substr(0, 60)} };
			if (inv.pingMs >= 0) i["ping_ms"] = (std::min)(10000, inv.pingMs);
			invitados.push_back(i);
			if (invitados.size() >= 16) break;
		}
		bool enPartida = false;
		{ std::lock_guard<std::mutex> lock(_mutex); enPartida = _enPartida; }
		const json cuerpo = { {"sala_id", sala}, {"estado", enPartida ? "en_partida" : "abierta"},
			{"plazas_libres", (std::max)(0, (std::min)(16, foto.plazasLibres))}, {"invitados", invitados} };

		const http::Respuesta r = http::peticion("POST", std::string(kBase) + "/v1/sala/latido", cuerpo.dump(),
			{ "Authorization: Bearer " + token, std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() });
		const json j = json::parse(r.cuerpo, nullptr, false);

		std::lock_guard<std::mutex> lock(_mutex);
		if (r.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			_latidoSeg = (std::max)(10, j.value("latido_seg", _latidoSeg));
			_proximoLatidoMs = ahoraMs() + _latidoSeg * 1000LL;
			_estado = EstadoLink::Conectado;
			_espera = 0;
			if (j.contains("roles")) aplicarRoles(j["roles"].dump());
			return;
		}
		if (r.estado == 0) { fallo("RED", "", 0); return; }
		const std::string codigo = j.is_discarded() ? "ERROR_INTERNO" : j.value("codigo", "ERROR_INTERNO");
		if (codigo == "SALA_CERRADA" || codigo == "SALA_NO_ENCONTRADA") {
			_salaId.clear(); // el siguiente paso vuelve a abrir
			return;
		}
		fallo(codigo, j.is_discarded() ? "" : j.value("mensaje", ""), j.is_discarded() ? 0 : j.value("reintentar_en", 0));
	}

	void PhoenixLink::pasoCerrar(const std::string& salaId) {
		std::string token;
		{ std::lock_guard<std::mutex> lock(_mutex); token = _token; }
		if (token.empty()) return;
		const json cuerpo = { {"sala_id", salaId}, {"motivo", "cierre desde Phoenix Link"} };
		http::peticion("POST", std::string(kBase) + "/v1/sala/cerrar", cuerpo.dump(),
			{ "Authorization: Bearer " + token, std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() });
		// Si falla, el servidor la marca caída a los 3 min sin latido.
	}

	void PhoenixLink::pasoDiagnostico(const std::string& salaId) {
		// Autodiagnóstico del host (contrato §14): latencia, jitter, pérdida y subida
		// contra Cloudflare. ~5 s, una vez por sala. Nunca bloquea el juego (hilo propio).
		{ std::lock_guard<std::mutex> lock(_mutex); _salaDiagnosticada = salaId; }
		try {
			std::vector<double> rtts;
			int fallos = 0;
			const int intentos = 12;
			for (int i = 0; i < intentos && _corriendo; i++) {
				const long long t0 = ahoraMs();
				const http::Respuesta r = http::peticion("GET", "https://speed.cloudflare.com/__down?bytes=0", "", {}, 3000);
				if (r.estado == 200) rtts.push_back(static_cast<double>(ahoraMs() - t0));
				else fallos++;
				Sleep(120);
			}
			if (rtts.empty()) return;
			std::vector<double> orden = rtts;
			std::sort(orden.begin(), orden.end());
			const double mediana = orden[orden.size() / 2];
			double jitter = 0;
			for (size_t i = 1; i < rtts.size(); i++) jitter += std::abs(rtts[i] - rtts[i - 1]);
			if (rtts.size() > 1) jitter /= static_cast<double>(rtts.size() - 1);

			// Subida: 2 envíos de 1,5 MB, se toma el mejor
			const std::string bloque(1500 * 1024, 'p');
			double mejorKbps = 0;
			for (int i = 0; i < 2 && _corriendo; i++) {
				const long long t0 = ahoraMs();
				const http::Respuesta r = http::peticion("POST", "https://speed.cloudflare.com/__up", bloque, {}, 20000);
				const long long ms = (std::max)(1LL, ahoraMs() - t0);
				if (r.estado == 200) mejorKbps = (std::max)(mejorKbps, bloque.size() * 8.0 / ms);
			}

			std::string token;
			{ std::lock_guard<std::mutex> lock(_mutex); token = _token; }
			const json cuerpo = {
				{"latencia_ms", mediana}, {"jitter_ms", jitter},
				{"perdida_pct", 100.0 * fallos / intentos}, {"subida_kbps", static_cast<long long>(mejorKbps)},
				{"muestras", static_cast<int>(rtts.size())}, {"referencia", "cloudflare"},
				{"jugadores_esperados", 2}, {"sala_id", salaId},
			};
			const http::Respuesta r = http::peticion("POST", std::string(kBase) + "/v1/diagnostico", cuerpo.dump(),
				{ "Authorization: Bearer " + token, std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() });
			const json j = json::parse(r.cuerpo, nullptr, false);
			if (r.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
				std::lock_guard<std::mutex> lock(_mutex);
				char texto[160];
				snprintf(texto, sizeof(texto), "Tu red: %.0f ms · subida %.1f Mbps · semáforo %s",
					mediana, mejorKbps / 1000.0, j.value("semaforo", "?").c_str());
				_mensaje = texto;
			}
		}
		catch (...) {
		}
	}

	void PhoenixLink::aplicarRoles(const std::string& jsonRoles) {
		// Llamar con _mutex tomado.
		const json roles = json::parse(jsonRoles, nullptr, false);
		if (roles.is_discarded()) return;

		auto idDe = [](const json& p) -> uint32_t {
			if (!p.contains("parsec_id") || p["parsec_id"].is_null()) return 0;
			try {
				return p["parsec_id"].is_string() ? static_cast<uint32_t>(std::stoul(p["parsec_id"].get<std::string>()))
					: p["parsec_id"].get<uint32_t>();
			}
			catch (...) { return 0; }
		};

		std::map<uint32_t, int> jugadores;
		std::set<uint32_t> espectadores;
		int total = 0;
		if (roles.contains("jugadores")) for (const json& p : roles["jugadores"]) {
			total++;
			const uint32_t id = idDe(p);
			if (id != 0) jugadores[id] = p.value("mando", 0);
		}
		if (roles.contains("espectadores")) for (const json& p : roles["espectadores"]) {
			const uint32_t id = idDe(p);
			if (id != 0) espectadores.insert(id);
		}
		_jugadores = total;

		// La puerta de mandos se activa en retos, o cuando ya hay rival con Parsec vinculado.
		bool hayRival = false;
		for (const auto& par : jugadores) if (par.second >= 2) hayRival = true;
		const bool activa = roles.value("modo", "") == "reto" || hayRival;
		PhoenixRoles::instancia().establecerDesdeWeb(activa, jugadores, espectadores, false);
	}

	// =========================================================================
	//  Interfaz nueva (contrato 1.4.0 – 1.6.0): presencia, amigos, invitar,
	//  cartas de jugador, eventos y «en_partida». Todo opcional: si falla, la
	//  sala y el latido siguen igual (estos pasos no tocan la espera global
	//  salvo por errores de token).
	// =========================================================================

	void PhoenixLink::ventanaVisible(bool si) {
		std::lock_guard<std::mutex> l(_mutex);
		if (si && !_ventanaVisible) _proximoAmigosMs = 0; // al volver a la ventana: sondeo inmediato
		_ventanaVisible = si;
	}

	void PhoenixLink::marcarPartido(bool enJuego) {
		std::lock_guard<std::mutex> l(_mutex);
		if (_enPartida != enJuego) {
			_enPartida = enJuego;
			_proximoLatidoMs = 0;     // el cambio de estado sale en el próximo latido posible
			_proximaPresenciaMs = 0;
		}
	}

	void PhoenixLink::evento(const std::string& tipo, const std::string& actorParsec, const std::string& datosJson) {
		std::lock_guard<std::mutex> l(_mutex);
		if (_token.empty()) return; // sin vínculo con la web no hay a quién mandarlo
		json e = { {"clave", uuid4()}, {"tipo", tipo}, {"ocurrido", ahoraIso()} };
		if (!actorParsec.empty()) e["actor_parsec"] = actorParsec;
		const json datos = json::parse(datosJson.empty() ? std::string("{}") : datosJson, nullptr, false);
		e["datos"] = (!datos.is_discarded() && datos.is_object()) ? datos : json::object();
		_eventos.push_back(e.dump());
		while (_eventos.size() > 500) _eventos.erase(_eventos.begin()); // tope: nunca crece sin fin
		guardarEventos();
	}

	std::vector<AmigoWeb> PhoenixLink::amigos() { std::lock_guard<std::mutex> l(_mutex); return _amigos; }
	bool PhoenixLink::amigosCargados() { std::lock_guard<std::mutex> l(_mutex); return _amigosCargados; }
	long long PhoenixLink::versionLiga() { std::lock_guard<std::mutex> l(_mutex); return _versionLiga; }
	std::string PhoenixLink::salaId() { std::lock_guard<std::mutex> l(_mutex); return _salaId; }
	int PhoenixLink::eventosEnCola() { std::lock_guard<std::mutex> l(_mutex); return static_cast<int>(_eventos.size()); }

	void PhoenixLink::invitar(uint64_t ticket, const std::string& usuarioId) {
		std::lock_guard<std::mutex> l(_mutex);
		_pedidos.push_back({ ticket, "invitar", usuarioId });
	}

	void PhoenixLink::soltarRival(uint64_t ticket) {
		std::lock_guard<std::mutex> l(_mutex);
		_pedidos.push_back({ ticket, "soltar_rival", "" });
	}

	std::vector<ResultadoWeb> PhoenixLink::tomarResultados() {
		std::lock_guard<std::mutex> l(_mutex);
		std::vector<ResultadoWeb> r;
		r.swap(_resultados);
		return r;
	}

	std::map<uint32_t, std::string> PhoenixLink::perfiles() {
		std::lock_guard<std::mutex> l(_mutex);
		std::map<uint32_t, std::string> r;
		for (uint32_t id : _presentes) {
			auto it = _perfiles.find(id);
			if (it != _perfiles.end() && !it->second.json.empty()) r[id] = it->second.json;
		}
		return r;
	}

	void PhoenixLink::guardarEventos() {
		// Llamar con _mutex tomado.
		try {
			json arr = json::array();
			for (const std::string& e : _eventos) {
				const json x = json::parse(e, nullptr, false);
				if (!x.is_discarded()) arr.push_back(x);
			}
			std::ofstream f(carpeta() + "phoenix-eventos.json", std::ios::binary | std::ios::trunc);
			f << arr.dump();
		}
		catch (...) {}
	}

	void PhoenixLink::cargarEventos() {
		try {
			std::ifstream f(carpeta() + "phoenix-eventos.json", std::ios::binary);
			if (!f) return;
			std::string texto((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
			const json arr = json::parse(texto, nullptr, false);
			if (arr.is_discarded() || !arr.is_array()) return;
			std::lock_guard<std::mutex> l(_mutex);
			for (const json& e : arr) {
				if (e.is_object() && e.contains("clave") && e.contains("tipo")) _eventos.push_back(e.dump());
				if (_eventos.size() >= 500) break;
			}
		}
		catch (...) {}
	}

	bool PhoenixLink::pasoOpcional(const std::string& token, const std::string& salaId, const InstantaneaSala& foto) {
		// Orden: lo que pidió el host ya, luego lo que la web necesita, luego lo «bonito».
		if (pasoPedidos(token, salaId)) return true;
		if (pasoEventos(token, salaId)) return true;
		if (pasoPerfiles(token, foto)) return true;
		if (pasoPresencia(token, salaId)) return true;
		return pasoAmigos(token);
	}

	bool PhoenixLink::pasoPedidos(const std::string& token, const std::string& salaId) {
		Pedido p;
		{
			std::lock_guard<std::mutex> l(_mutex);
			if (_pedidos.empty()) return false;
			p = _pedidos.front();
			_pedidos.erase(_pedidos.begin());
		}
		ResultadoWeb r;
		r.ticket = p.ticket;
		if (salaId.empty()) {
			r.codigo = "SALA_NO_PUBLICADA";
			r.mensaje = "Tu sala aún no está publicada en la web: ábrela y espera unos segundos.";
			std::lock_guard<std::mutex> l(_mutex);
			_resultados.push_back(r);
			return true;
		}
		json cuerpo = { {"sala_id", salaId} };
		std::string ruta = "/v1/sala/soltar_rival";
		if (p.tipo == "invitar") {
			cuerpo["usuario_id"] = p.usuarioId;
			ruta = "/v1/invitar";
		}
		const http::Respuesta resp = http::peticion("POST", std::string(kBase) + ruta, cuerpo.dump(), cabecerasCon(token));
		const json j = json::parse(resp.cuerpo, nullptr, false);
		std::lock_guard<std::mutex> l(_mutex);
		if (resp.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			r.ok = true;
			if (p.tipo == "invitar") {
				r.mensaje = j.value("notificado", false) ? "Invitación enviada: le llegó el aviso."
					: "Invitación lista: tu amigo apagó los avisos, pero ya puede ver y entrar a tu sala.";
			}
			else {
				r.mensaje = "Puesto de rival liberado: tu sala vuelve al radar.";
				if (j.contains("roles")) aplicarRoles(j["roles"].dump());
			}
		}
		else if (resp.estado == 0) {
			r.codigo = "RED";
			r.mensaje = "Sin conexión con la web. Revisa tu internet.";
		}
		else {
			r.codigo = j.is_discarded() ? "ERROR_INTERNO" : j.value("codigo", "ERROR_INTERNO");
			r.mensaje = textoError(r.codigo, j.is_discarded() ? "" : j.value("mensaje", ""));
			if (r.codigo.rfind("TOKEN_", 0) == 0) fallo(r.codigo, r.mensaje, 0);
		}
		_resultados.push_back(r);
		return true;
	}

	bool PhoenixLink::pasoEventos(const std::string& token, const std::string& salaId) {
		std::vector<std::string> lote;
		{
			std::lock_guard<std::mutex> l(_mutex);
			if (salaId.empty() || _eventos.empty() || ahoraMs() < _proximoEnvioEventosMs) return false;
			for (size_t i = 0; i < _eventos.size() && i < 50; i++) lote.push_back(_eventos[i]);
		}
		json arr = json::array();
		for (const std::string& e : lote) {
			const json x = json::parse(e, nullptr, false);
			if (!x.is_discarded()) arr.push_back(x);
		}
		const json cuerpo = { {"sala_id", salaId}, {"eventos", arr} };
		const http::Respuesta resp = http::peticion("POST", std::string(kBase) + "/v1/eventos", cuerpo.dump(), cabecerasCon(token));
		const json j = json::parse(resp.cuerpo, nullptr, false);
		std::lock_guard<std::mutex> l(_mutex);
		const std::string codigo = j.is_discarded() ? "" : j.value("codigo", "");
		const bool enviado = resp.estado == 200 && !j.is_discarded() && j.value("ok", false);
		// Aceptados, duplicados y rechazados ya no se reintentan (§6). Un evento de una sala
		// que murió tampoco tiene a dónde ir.
		if (enviado || codigo == "SALA_CERRADA" || codigo == "SALA_NO_ENCONTRADA" || codigo == "CAMPO_INVALIDO" || codigo == "JSON_INVALIDO") {
			const size_t n = (std::min)(lote.size(), _eventos.size());
			_eventos.erase(_eventos.begin(), _eventos.begin() + static_cast<std::ptrdiff_t>(n));
			guardarEventos();
			_proximoEnvioEventosMs = ahoraMs() + 15000; // eventos_envio_seg
		}
		else {
			if (codigo.rfind("TOKEN_", 0) == 0) fallo(codigo, j.value("mensaje", ""), 0);
			const int espera = j.is_discarded() ? 0 : j.value("reintentar_en", 0);
			_proximoEnvioEventosMs = ahoraMs() + (std::max)(30, espera) * 1000LL;
		}
		return true;
	}

	bool PhoenixLink::pasoPresencia(const std::string& token, const std::string& salaId) {
		bool enPartida;
		{
			std::lock_guard<std::mutex> l(_mutex);
			if (ahoraMs() < _proximaPresenciaMs) return false;
			enPartida = _enPartida;
		}
		json cuerpo = { {"estado", enPartida ? "en_partida" : "disponible"} };
		if (!salaId.empty()) cuerpo["sala_id"] = salaId;
		const http::Respuesta resp = http::peticion("POST", std::string(kBase) + "/v1/presencia", cuerpo.dump(), cabecerasCon(token));
		const json j = json::parse(resp.cuerpo, nullptr, false);
		std::lock_guard<std::mutex> l(_mutex);
		if (resp.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			_proximaPresenciaMs = ahoraMs() + (std::max)(30, j.value("siguiente_seg", 60)) * 1000LL;
			if (j.contains("version_liga") && j["version_liga"].is_number()) _versionLiga = j["version_liga"].get<long long>();
		}
		else {
			const std::string codigo = j.is_discarded() ? "" : j.value("codigo", "");
			if (codigo.rfind("TOKEN_", 0) == 0) fallo(codigo, j.value("mensaje", ""), 0);
			_proximaPresenciaMs = ahoraMs() + 120000; // reintento tranquilo
		}
		return true;
	}

	bool PhoenixLink::pasoAmigos(const std::string& token) {
		std::string etag;
		{
			std::lock_guard<std::mutex> l(_mutex);
			if (!_ventanaVisible || ahoraMs() < _proximoAmigosMs) return false;
			etag = _etagAmigos;
		}
		std::vector<std::string> cab = cabecerasCon(token);
		if (!etag.empty()) cab.push_back("If-None-Match: " + etag);
		const http::Respuesta resp = http::peticion("GET", std::string(kBase) + "/v1/presencia/amigos", "", cab);
		const json j = json::parse(resp.cuerpo, nullptr, false);
		std::lock_guard<std::mutex> l(_mutex);
		if (resp.estado == 304) {
			_proximoAmigosMs = ahoraMs() + 30000;
			_amigosCargados = true;
			return true;
		}
		if (resp.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			_amigos.clear();
			if (j.contains("amigos") && j["amigos"].is_array()) {
				for (const json& a : j["amigos"]) {
					AmigoWeb x;
					x.usuarioId = textoDe(a, "usuario_id");
					x.nombre = textoDe(a, "nombre");
					x.avatarUrl = textoDe(a, "avatar_url");
					x.estado = textoDe(a, "estado");
					x.salaId = textoDe(a, "sala_id");
					x.desde = textoDe(a, "desde");
					if (!x.usuarioId.empty()) _amigos.push_back(x);
				}
			}
			_etagAmigos = j.value("etag", "");
			_amigosCargados = true;
			_proximoAmigosMs = ahoraMs() + (std::max)(15, j.value("sondeo_seg", 30)) * 1000LL;
			return true;
		}
		const std::string codigo = j.is_discarded() ? "" : j.value("codigo", "");
		if (codigo.rfind("TOKEN_", 0) == 0) fallo(codigo, j.value("mensaje", ""), 0);
		_proximoAmigosMs = ahoraMs() + 60000;
		return true;
	}

	bool PhoenixLink::pasoPerfiles(const std::string& token, const InstantaneaSala& foto) {
		std::vector<uint32_t> faltan;
		{
			std::lock_guard<std::mutex> l(_mutex);
			if (ahoraMs() < _proximoPerfilesMs) return false;
			const long long ahora = ahoraMs();
			for (uint32_t id : _presentes) {
				auto it = _perfiles.find(id);
				if (it == _perfiles.end() || it->second.hastaMs < ahora) faltan.push_back(id);
				if (faltan.size() >= 16) break;
			}
			if (faltan.empty()) return false;
		}
		json ids = json::array();
		for (uint32_t id : faltan) ids.push_back(id);
		const json cuerpo = { {"parsec_ids", ids} };
		const http::Respuesta resp = http::peticion("POST", std::string(kBase) + "/v1/perfiles", cuerpo.dump(), cabecerasCon(token));
		const json j = json::parse(resp.cuerpo, nullptr, false);
		std::lock_guard<std::mutex> l(_mutex);
		_proximoPerfilesMs = ahoraMs() + 6000; // la web acepta 1 llamada cada 5 s
		if (resp.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			const long long hasta = ahoraMs() + 10 * 60 * 1000LL; // se refresca cada 10 min
			for (uint32_t id : faltan) _perfiles[id] = { std::string(), hasta }; // sin cuenta vinculada
			if (j.contains("perfiles") && j["perfiles"].is_array()) {
				for (const json& p : j["perfiles"]) {
					uint32_t id = 0;
					try {
						if (p["parsec_id"].is_number()) id = p["parsec_id"].get<uint32_t>();
						else id = static_cast<uint32_t>(std::stoul(p["parsec_id"].get<std::string>()));
					}
					catch (...) { id = 0; }
					if (id != 0) _perfiles[id] = { p.dump(), hasta };
				}
			}
			// No acumular cartas de gente que se fue hace mucho
			if (_perfiles.size() > 200) _perfiles.clear();
		}
		else {
			const std::string codigo = j.is_discarded() ? "" : j.value("codigo", "");
			if (codigo.rfind("TOKEN_", 0) == 0) fallo(codigo, j.value("mensaje", ""), 0);
			const int espera = j.is_discarded() ? 0 : j.value("reintentar_en", 0);
			_proximoPerfilesMs = ahoraMs() + (std::max)(30, espera) * 1000LL;
		}
		(void)foto;
		return true;
	}

}
