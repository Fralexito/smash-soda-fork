#include "Respaldos.h"
#include "Sha256.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <thread>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace mercado::respaldos {

	namespace {
		std::tm local(std::time_t t) {
			std::tm r{};
#ifdef _WIN32
			localtime_s(&r, &t);
#else
			localtime_r(&t, &r);
#endif
			return r;
		}
		std::string fecha(std::time_t t) {   // AAAA-MM-DD
			char b[16]; const std::tm l = local(t); std::strftime(b, sizeof(b), "%Y-%m-%d", &l); return b;
		}
		std::string iso(std::time_t t) {
			char b[32]; const std::tm l = local(t); std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%S", &l); return b;
		}
		std::time_t ahoraDe(const Opciones& op) {
			return op.ahora ? op.ahora() : std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		}
		/// Nombre de respaldo válido: AAAA-MM-DD_HHMMSS con sufijo opcional -N.
		bool nombreValido(const std::string& n) {
			if (n.size() < 17) return false;
			for (size_t i = 0; i < 17; i++) {
				const char c = n[i];
				if (i == 4 || i == 7) { if (c != '-') return false; }
				else if (i == 10) { if (c != '_') return false; }
				else if (c < '0' || c > '9') return false;
			}
			if (n.size() == 17) return true;
			if (n[17] != '-' || n.size() == 18) return false;
			for (size_t i = 18; i < n.size(); i++) if (n[i] < '0' || n[i] > '9') return false;
			return true;
		}
		/// Archivos (no carpetas) que se copiarían de un origen.
		std::vector<fs::path> archivosDe(const fs::path& p) {
			std::vector<fs::path> r;
			std::error_code ec;
			if (fs::is_regular_file(p, ec)) { r.push_back(p); return r; }
			if (!fs::is_directory(p, ec)) return r;
			for (fs::directory_iterator it(p, fs::directory_options::skip_permission_denied, ec), fin; !ec && it != fin; it.increment(ec)) {
				std::error_code e2;
				if (it->is_regular_file(e2)) r.push_back(it->path());
			}
			std::sort(r.begin(), r.end());
			return r;
		}
		bool leerManifiesto(const fs::path& carpeta, Info& info) {
			try {
				std::ifstream f(carpeta / "manifiesto.json", std::ios::binary);
				if (!f) return false;
				const json j = json::parse(f);
				if (j.value("formato", "") != kFormato) return false;
				info.nombre = deRuta(carpeta.filename());
				info.carpeta = deRuta(carpeta);
				info.creadoEn = j.value("creado_en", "");
				info.motivo = j.value("motivo", "");
				info.versionSync = j.value("version_sync", "");
				info.bytes = 0;
				for (const auto& a : j.at("archivos")) {
					ArchivoRespaldado x{ a.at("etiqueta"), a.at("relativo"), a.at("ruta_original"), a.at("sha256"), a.at("bytes") };
					info.bytes += x.bytes;
					info.archivos.push_back(std::move(x));
				}
				for (const auto& o : j.value("omitidos", json::array())) info.omitidos.push_back(o.get<std::string>());
				return true;
			}
			catch (...) { return false; }
		}
		void borrarSilencioso(const fs::path& p) { std::error_code ec; fs::remove_all(p, ec); }
	}

	std::string sello(std::time_t t) {
		char b[32]; const std::tm l = local(t); std::strftime(b, sizeof(b), "%Y-%m-%d_%H%M%S", &l); return b;
	}

	Resultado<std::string> copiarVerificado(const std::string& origen, const std::string& destino) {
		try {
			std::string err;
			const std::string hOrig = sha256::deArchivo(origen, &err);
			if (hOrig.empty()) return Resultado<std::string>::mal("ARCHIVO_ILEGIBLE", origen + " " + err);
			const fs::path dst = aRuta(destino);
			const fs::path tmp = aRuta(destino + ".tmp");
			std::error_code ec;
			fs::remove(tmp, ec);
			fs::copy_file(aRuta(origen), tmp, fs::copy_options::overwrite_existing);
			if (sha256::deArchivo(deRuta(tmp), &err) != hOrig) {
				fs::remove(tmp, ec);
				return Resultado<std::string>::mal("COPIA_CORRUPTA", "La huella de la copia no coincide: " + destino);
			}
			// Reemplazo atómico con reintentos (el juego o un antivirus pueden tener el archivo abierto un momento).
			for (int i = 0; i < 10; i++) {
				std::error_code er;
				fs::rename(tmp, dst, er);
				if (!er) return Resultado<std::string>::bien(hOrig);
				if (i == 9) { fs::remove(tmp, ec); return Resultado<std::string>::mal("ARCHIVO_EN_USO", destino + ": " + er.message()); }
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
			}
			return Resultado<std::string>::mal("ARCHIVO_EN_USO", destino);
		}
		catch (const std::exception& e) {
			return Resultado<std::string>::mal("ARCHIVO_ERROR", e.what());
		}
	}

	Resultado<Info> crear(const std::vector<Origen>& origenes, const Opciones& op) {
		if (op.raiz.empty()) return Resultado<Info>::mal("ARCHIVO_ERROR", "Falta la carpeta de respaldos");
		const std::time_t t = ahoraDe(op);
		Info info;
		info.creadoEn = iso(t);
		info.motivo = op.motivo;
		info.versionSync = op.versionSync;

		// 1) Qué hay que copiar y cuánto pesa.
		struct Plan { std::string etiqueta; fs::path src; };
		std::vector<Plan> plan;
		uint64_t total = 0;
		for (const auto& o : origenes) {
			std::error_code ec;
			const fs::path p = aRuta(o.ruta);
			if (o.ruta.empty() || !fs::exists(p, ec)) { info.omitidos.push_back(o.etiqueta + ": no existe (" + o.ruta + ")"); continue; }
			const auto lista = archivosDe(p);
			if (lista.empty()) { info.omitidos.push_back(o.etiqueta + ": sin archivos"); continue; }
			for (const auto& a : lista) {
				std::error_code e2;
				total += fs::file_size(a, e2);
				plan.push_back({ o.etiqueta, a });
			}
		}
		if (plan.empty()) return Resultado<Info>::mal("NADA_QUE_RESPALDAR", "No se encontró ningún archivo para respaldar");

		try {
			const fs::path raiz = aRuta(op.raiz);
			fs::create_directories(raiz);

			// 2) Espacio libre.
			uint64_t libre = 0;
			if (op.espacioLibre) libre = op.espacioLibre(op.raiz);
			else { std::error_code ec; const auto s = fs::space(raiz, ec); libre = ec ? 0 : s.available; }
			if (libre < total + op.margenBytes)
				return Resultado<Info>::mal("SIN_ESPACIO", "Hacen falta " + std::to_string((total + op.margenBytes) / (1024 * 1024)) +
					" MB libres y hay " + std::to_string(libre / (1024 * 1024)) + " MB");

			// 3) Nombre libre (nunca pisar).
			const std::string base = sello(t);
			std::string nombre = base;
			for (int i = 2; fs::exists(raiz / aRuta(nombre)) || fs::exists(raiz / aRuta(nombre + ".parcial")); i++) {
				if (i > 99) return Resultado<Info>::mal("COPIA_NOMBRE_OCUPADO", base);
				nombre = base + "-" + std::to_string(i);
			}
			const fs::path parcial = raiz / aRuta(nombre + ".parcial");
			fs::create_directories(parcial);

			// 4) Copiar y verificar cada archivo.
			json archivos = json::array();
			for (const auto& p : plan) {
				const fs::path sub = parcial / aRuta(p.etiqueta);
				fs::create_directories(sub);
				const std::string rel = p.etiqueta + "/" + deRuta(p.src.filename());
				auto c = copiarVerificado(deRuta(p.src), deRuta(sub / p.src.filename()));
				if (!c.ok()) { borrarSilencioso(parcial); return Resultado<Info>::mal(c.error.codigo, c.error.detalle); }
				std::error_code ec;
				const long long bytes = static_cast<long long>(fs::file_size(sub / p.src.filename(), ec));
				ArchivoRespaldado a{ p.etiqueta, rel, deRuta(p.src), *c.valor, bytes };
				info.bytes += bytes;
				archivos.push_back({ { "etiqueta", a.etiqueta }, { "relativo", a.relativo }, { "ruta_original", a.rutaOriginal },
					{ "bytes", a.bytes }, { "sha256", a.sha256 } });
				info.archivos.push_back(std::move(a));
			}

			// 5) Manifiesto (al final) y nombre definitivo.
			const json m = { { "formato", kFormato }, { "creado_en", info.creadoEn }, { "motivo", info.motivo },
				{ "version_sync", info.versionSync }, { "archivos", archivos }, { "omitidos", info.omitidos } };
			{
				std::ofstream f(parcial / "manifiesto.json.tmp", std::ios::binary | std::ios::trunc);
				f << m.dump(2);
				if (!f) { borrarSilencioso(parcial); return Resultado<Info>::mal("ARCHIVO_ERROR", "No se pudo escribir el manifiesto"); }
			}
			fs::rename(parcial / "manifiesto.json.tmp", parcial / "manifiesto.json");
			const fs::path final_ = raiz / aRuta(nombre);
			std::error_code er;
			fs::rename(parcial, final_, er);
			if (er) { borrarSilencioso(parcial); return Resultado<Info>::mal("ARCHIVO_ERROR", "No se pudo cerrar el respaldo: " + er.message()); }
			info.nombre = nombre;
			info.carpeta = deRuta(final_);
			return Resultado<Info>::bien(std::move(info));
		}
		catch (const std::exception& e) {
			return Resultado<Info>::mal("ARCHIVO_ERROR", e.what());
		}
	}

	std::vector<Info> listar(const std::string& raiz) {
		std::vector<Info> r;
		std::error_code ec;
		const fs::path p = aRuta(raiz);
		if (!fs::is_directory(p, ec)) return r;
		for (fs::directory_iterator it(p, ec), fin; !ec && it != fin; it.increment(ec)) {
			std::error_code e2;
			if (!it->is_directory(e2)) continue;
			if (!nombreValido(deRuta(it->path().filename()))) continue;
			Info i;
			if (leerManifiesto(it->path(), i)) r.push_back(std::move(i));
		}
		// Más nuevo primero: por sello y luego por sufijo numérico.
		auto clave = [](const std::string& n) { return std::make_pair(n.substr(0, 17), n.size() > 18 ? std::stoi(n.substr(18)) : 1); };
		std::sort(r.begin(), r.end(), [&](const Info& a, const Info& b) { return clave(a.nombre) > clave(b.nombre); });
		return r;
	}

	std::vector<std::string> aplicarRetencion(const std::string& raiz, const Retencion& ret, std::time_t ahora) {
		std::vector<std::string> borrados;
		const auto todos = listar(raiz);   // más nuevo primero
		std::vector<bool> conservar(todos.size(), false);
		for (size_t i = 0; i < todos.size() && static_cast<int>(i) < ret.ultimos; i++) conservar[i] = true;
		// El primero (más viejo) de cada uno de los últimos `dias` días.
		for (int d = 0; d < ret.dias; d++) {
			const std::string dia = fecha(ahora - static_cast<std::time_t>(d) * 86400);
			for (size_t i = todos.size(); i-- > 0;)
				if (todos[i].nombre.substr(0, 10) == dia) { conservar[i] = true; break; }
		}
		for (size_t i = 0; i < todos.size(); i++) {
			if (conservar[i]) continue;
			std::error_code ec;
			fs::remove_all(aRuta(todos[i].carpeta), ec);
			if (!ec) borrados.push_back(todos[i].nombre);
		}
		return borrados;
	}

	bool faltaDiario(const std::string& raiz, std::time_t ahora) {
		const std::string hoy = fecha(ahora);
		for (const auto& i : listar(raiz)) if (i.nombre.substr(0, 10) == hoy) return false;
		return true;
	}

	Resultado<ResultadoRestaurar> restaurar(const std::string& raiz, const std::string& nombre,
		const std::vector<Origen>& origenesActuales, const Opciones& op) {
		if (!nombreValido(nombre)) return Resultado<ResultadoRestaurar>::mal("RESPALDO_INVALIDO", nombre);
		Info elegido;
		if (!leerManifiesto(aRuta(raiz) / aRuta(nombre), elegido)) return Resultado<ResultadoRestaurar>::mal("RESPALDO_INVALIDO", nombre);

		// Verificar el respaldo completo ANTES de tocar nada.
		for (const auto& a : elegido.archivos) {
			std::string err;
			if (sha256::deArchivo(deRuta(aRuta(elegido.carpeta) / aRuta(a.relativo)), &err) != a.sha256)
				return Resultado<ResultadoRestaurar>::mal("RESPALDO_DANADO", a.relativo);
		}

		Opciones previo = op;
		previo.motivo = "antes de restaurar " + nombre;
		auto pre = crear(origenesActuales, previo);
		if (!pre.ok() && pre.error.codigo != "NADA_QUE_RESPALDAR")
			return Resultado<ResultadoRestaurar>::mal(pre.error.codigo, "No se pudo respaldar el estado actual: " + pre.error.detalle);

		ResultadoRestaurar r;
		if (pre.ok()) r.respaldoPrevio = *pre.valor;
		for (const auto& a : elegido.archivos) {
			const fs::path destino = aRuta(a.rutaOriginal);
			std::error_code ec;
			if (!fs::is_directory(destino.parent_path(), ec)) { r.saltados.push_back(a.rutaOriginal + " (la carpeta ya no existe)"); continue; }
			auto c = copiarVerificado(deRuta(aRuta(elegido.carpeta) / aRuta(a.relativo)), a.rutaOriginal);
			if (!c.ok()) return Resultado<ResultadoRestaurar>::mal(c.error.codigo, c.error.detalle + " (lo anterior quedó en el respaldo " + r.respaldoPrevio.nombre + ")");
			r.colocados++;
		}
		return Resultado<ResultadoRestaurar>::bien(std::move(r));
	}

}
