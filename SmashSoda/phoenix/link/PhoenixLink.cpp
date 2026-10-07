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
			if (!CryptProtectData(&entrada, L"PhoenixSoda", nullptr, nullptr, nullptr, 0, &salida)) return false;
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
						+ "|" + std::to_string(foto.limiteEspectadores) + "|" + std::to_string(foto.plazasTotal);
					if (foto.abierta && (salaId.empty() || firma != firmaPublicada)) pasoAbrir(foto);
					else if (foto.abierta && ahoraMs() >= proximoLatido) pasoLatido(foto);
					else if (!foto.abierta && !salaId.empty()) {
						pasoCerrar(salaId);
						std::lock_guard<std::mutex> lock(_mutex);
						_salaId.clear();
						_jugadores = 0;
						PhoenixRoles::instancia().limpiarWeb();
					}
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
			{"juego", "eFootball PES 2021"},
			{"visibilidad", foto.visibilidad},
			{"acepta_espectadores", foto.aceptaEspectadores},
			{"limite_espectadores", foto.aceptaEspectadores ? (std::max)(0, (std::min)(16, foto.limiteEspectadores)) : 0},
			{"modo", "amistoso"},
			{"publicar_en_pagina", foto.visibilidad != "privada"},
		};
		if (foto.enlace.rfind("https://", 0) == 0) cuerpo["enlace"] = foto.enlace;

		const http::Respuesta r = http::peticion("POST", std::string(kBase) + "/v1/sala/abrir", cuerpo.dump(),
			{ "Authorization: Bearer " + token, std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() });
		const json j = json::parse(r.cuerpo, nullptr, false);

		std::lock_guard<std::mutex> lock(_mutex);
		if (r.estado == 200 && !j.is_discarded() && j.value("ok", false)) {
			_salaId = j.value("sala_id", "");
			_latidoSeg = (std::max)(10, j.value("latido_seg", 30));
			_proximoLatidoMs = ahoraMs() + (std::max)(10, _latidoSeg) * 1000LL; // primer latido ≥ 10 s
			_firmaPrefs = foto.visibilidad + "|" + (foto.aceptaEspectadores ? "1" : "0")
				+ "|" + std::to_string(foto.limiteEspectadores) + "|" + std::to_string(foto.plazasTotal);
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
		const json cuerpo = { {"sala_id", sala}, {"estado", "abierta"},
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
		const json cuerpo = { {"sala_id", salaId}, {"motivo", "cierre desde Phoenix Soda"} };
		http::peticion("POST", std::string(kBase) + "/v1/sala/cerrar", cuerpo.dump(),
			{ "Authorization: Bearer " + token, std::string("X-Phoenix-Version: ") + kVersionApp, "X-Phoenix-Build: " + huellaExe() });
		// Si falla, el servidor la marca caída a los 3 min sin latido.
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

}
