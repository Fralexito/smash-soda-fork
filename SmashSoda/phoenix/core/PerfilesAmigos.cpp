#include "PerfilesAmigos.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

#include "PerfilesPes.h"

namespace fs = std::filesystem;

namespace phoenix::perfilespes {

	namespace {
		fs::path rutaDe(const std::string& utf8) { return fs::u8path(utf8); }
		std::string textoDe(const fs::path& p) { const auto s = p.u8string(); return std::string(s.begin(), s.end()); }

		int numeroDeId(const std::string& id) {
			if (id.size() < 2 || id[0] != 'p') return 0;
			int n = 0;
			for (size_t i = 1; i < id.size(); i++) { if (id[i] < '0' || id[i] > '9') return 0; n = n * 10 + (id[i] - '0'); if (n > 100000000) return 0; }
			return n;
		}

		std::string idNuevo(const DatosBase& d) {
			int m = 0;
			for (auto& p : d.perfiles) m = (std::max)(m, numeroDeId(p.id));
			return "p" + std::to_string(m + 1);
		}

		bool nombreLibre(const DatosBase& d, const std::string& nombre) {
			const std::string c = claveNombre(nombre);
			for (auto& p : d.perfiles) if (claveNombre(p.nombre) == c) return false;
			return true;
		}

		// «Usuario N» o «Usuario N (2)»…: único entre los perfiles
		std::string nombreSinNombre(const DatosBase& d, int hueco) {
			std::string base = "Usuario " + std::to_string(hueco);
			if (nombreLibre(d, base)) return base;
			for (int k = 2; k < 1000; k++) { const std::string n = base + " (" + std::to_string(k) + ")"; if (nombreLibre(d, n)) return n; }
			return base + " (x)";
		}
	}

	std::string claveNombre(const std::string& nombre) {
		size_t a = 0, b = nombre.size();
		while (a < b && (nombre[a] == ' ' || nombre[a] == '\t')) a++;
		while (b > a && (nombre[b - 1] == ' ' || nombre[b - 1] == '\t')) b--;
		std::string r = nombre.substr(a, b - a);
		for (auto& c : r) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
		return r;
	}

	// ---- Base ------------------------------------------------------------------

	const PerfilAmigo* Base::porId(const std::string& id) const {
		for (auto& p : datos.perfiles) if (p.id == id) return &p;
		return nullptr;
	}

	const PerfilAmigo* Base::porNombre(const std::string& nombre) const {
		const std::string c = claveNombre(nombre);
		if (c.empty()) return nullptr;
		for (auto& p : datos.perfiles) if (claveNombre(p.nombre) == c) return &p;
		return nullptr;
	}

	bool Base::protegido(int hueco) const {
		return std::find(datos.protegidos.begin(), datos.protegidos.end(), hueco) != datos.protegidos.end();
	}

	void Base::proteger(int hueco, bool si) {
		if (hueco < 1 || hueco > kUsuarios) return;
		auto it = std::find(datos.protegidos.begin(), datos.protegidos.end(), hueco);
		if (si && it == datos.protegidos.end()) { datos.protegidos.push_back(hueco); std::sort(datos.protegidos.begin(), datos.protegidos.end()); }
		if (!si && it != datos.protegidos.end()) datos.protegidos.erase(it);
	}

	bool Base::borrar(const std::string& id) {
		auto it = std::find_if(datos.perfiles.begin(), datos.perfiles.end(), [&](const PerfilAmigo& p) { return p.id == id; });
		if (it == datos.perfiles.end()) return false;
		datos.perfiles.erase(it);
		datos.seleccion.erase(std::remove(datos.seleccion.begin(), datos.seleccion.end(), id), datos.seleccion.end());
		datos.ultima.erase(std::remove_if(datos.ultima.begin(), datos.ultima.end(), [&](const Asignacion& a) { return a.perfilId == id; }), datos.ultima.end());
		return true;
	}

	nlohmann::json Base::aJson() const {
		nlohmann::json j;
		j["version"] = 1;
		j["perfiles"] = nlohmann::json::array();
		for (auto& p : datos.perfiles)
			j["perfiles"].push_back({ {"id", p.id}, {"nombre", p.nombre}, {"registro", p.registro}, {"anterior", p.anterior}, {"actualizado", p.actualizado}, {"sinNombre", p.sinNombre} });
		j["protegidos"] = datos.protegidos;
		j["seleccion"] = datos.seleccion;
		j["ultima"] = nlohmann::json::array();
		for (auto& a : datos.ultima) j["ultima"].push_back({ {"hueco", a.hueco}, {"perfil", a.perfilId} });
		j["ultimaSala"] = datos.ultimaSala;
		return j;
	}

	bool Base::deJson(const nlohmann::json& j, std::string& error) {
		try {
			if (!j.is_object() || j.value("version", 0) != 1) { error = "perfiles-amigos.json no es de la versión 1"; return false; }
			DatosBase d;
			std::set<std::string> ids;
			for (auto& x : j.value("perfiles", nlohmann::json::array())) {
				PerfilAmigo p;
				p.id = x.value("id", std::string());
				p.nombre = x.value("nombre", std::string());
				p.registro = x.value("registro", std::string());
				p.anterior = x.value("anterior", std::string());
				p.actualizado = x.value("actualizado", std::string());
				p.sinNombre = x.value("sinNombre", false);
				std::vector<uint8_t> b;
				if (p.id.empty() || !ids.insert(p.id).second || !deHex(p.registro, b) || b.size() != kTamUsuario) { error = "un perfil de perfiles-amigos.json está dañado (" + p.id + ")"; return false; }
				d.perfiles.push_back(std::move(p));
			}
			for (auto& h : j.value("protegidos", nlohmann::json::array())) { const int n = h.get<int>(); if (n >= 1 && n <= kUsuarios) d.protegidos.push_back(n); }
			std::sort(d.protegidos.begin(), d.protegidos.end());
			d.protegidos.erase(std::unique(d.protegidos.begin(), d.protegidos.end()), d.protegidos.end());
			for (auto& s : j.value("seleccion", nlohmann::json::array())) d.seleccion.push_back(s.get<std::string>());
			for (auto& a : j.value("ultima", nlohmann::json::array())) d.ultima.push_back({ a.value("hueco", 0), a.value("perfil", std::string()) });
			d.ultimaSala = j.value("ultimaSala", std::string());
			datos = std::move(d);
			return true;
		}
		catch (const std::exception& e) { error = std::string("perfiles-amigos.json ilegible: ") + e.what(); return false; }
	}

	bool Base::cargar(std::string& error) {
		try {
			const fs::path f = rutaDe(_carpeta) / "perfiles-amigos.json";
			if (!fs::exists(f)) { datos = DatosBase(); return true; }
			std::ifstream in(f, std::ios::binary);
			if (!in) { error = "no se puede abrir perfiles-amigos.json"; return false; }
			const auto j = nlohmann::json::parse(in, nullptr, false);
			if (j.is_discarded()) { error = "perfiles-amigos.json está dañado (no se toca; hay copias en respaldos)"; return false; }
			return deJson(j, error);
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}

	bool Base::guardar(const std::string& sello, std::string& error) {
		try {
			if (datos.perfiles.size() > kMaxPerfiles) { error = "demasiados perfiles"; return false; }
			const fs::path dir = rutaDe(_carpeta);
			std::error_code ec;
			fs::create_directories(dir / "respaldos", ec);
			const std::string texto = aJson().dump(1);
			const fs::path destino = dir / "perfiles-amigos.json";
			fs::path tmp = destino; tmp += ".tmp";
			{
				std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
				o.write(texto.data(), static_cast<std::streamsize>(texto.size()));
				o.flush();
				if (!o.good()) { o.close(); fs::remove(tmp, ec); error = "no se pudo escribir perfiles-amigos.json"; return false; }
			}
			{   // relectura: lo escrito tiene que volver a leerse igual
				std::ifstream in(tmp, std::ios::binary);
				const auto j = nlohmann::json::parse(in, nullptr, false);
				Base prueba(_carpeta);
				std::string e2;
				if (j.is_discarded() || !prueba.deJson(j, e2) || prueba.aJson() != aJson()) { fs::remove(tmp, ec); error = "la relectura de perfiles-amigos.json no coincide; no se tocó el original"; return false; }
			}
			fs::rename(tmp, destino, ec);
			if (ec) { fs::remove(tmp, ec); error = "no se pudo reemplazar perfiles-amigos.json"; return false; }
			// copia con sello (una por segundo como máximo) y poda de las más viejas
			fs::path copia = dir / "respaldos" / ("perfiles-" + sello + ".json");
			if (!fs::exists(copia)) fs::copy_file(destino, copia, ec);
			std::vector<fs::path> todas;
			for (auto& e : fs::directory_iterator(dir / "respaldos", ec)) if (e.path().extension() == ".json") todas.push_back(e.path());
			std::sort(todas.begin(), todas.end());
			while (todas.size() > kMaxRespaldos) { fs::remove(todas.front(), ec); todas.erase(todas.begin()); }
			return true;
		}
		catch (const std::exception& e) { error = e.what(); return false; }
	}

	// ---- PES → Link --------------------------------------------------------------

	bool traerDePes(const std::string& rutaSystem, Base& base, const std::string& sello, Informe& informe, std::string& error) {
		Guardado g;
		if (!Guardado::abrir(rutaSystem, g, error)) return false;
		std::set<std::string> vistos;   // nombres ya tratados en esta pasada (dos huecos con el mismo nombre: solo el primero)
		Base copia = base;              // todo o nada: si algo falla, la base original queda igual
		for (int i = 0; i < kUsuarios; i++) {
			const Usuario u = g.usuario(i);
			const std::string hex = aHex(u.bytes);
			const std::string clave = claveNombre(u.nombre);
			if (!clave.empty()) {
				if (!vistos.insert(clave).second) { informe.avisos.push_back("Usuario " + std::to_string(i + 1) + ": el nombre «" + u.nombre + "» ya estaba en otro hueco; no se guarda este"); continue; }
				const PerfilAmigo* p = copia.porNombre(u.nombre);
				if (p) {
					PerfilAmigo* m = const_cast<PerfilAmigo*>(p);
					if (m->registro == hex) { informe.iguales++; continue; }
					m->anterior = m->registro; m->registro = hex; m->actualizado = sello; m->sinNombre = false;
					informe.actualizados++;
				}
				else {
					if (copia.datos.perfiles.size() >= Base::kMaxPerfiles) { error = "se alcanzó el máximo de perfiles"; return false; }
					PerfilAmigo n; n.id = idNuevo(copia.datos); n.nombre = u.nombre; n.registro = hex; n.actualizado = sello;
					copia.datos.perfiles.push_back(std::move(n));
					informe.nuevos++;
				}
			}
			else {
				// sin nombre: se reconoce por tener el MISMO registro; si no existe se guarda como «Usuario N» (nunca se pisa otro)
				const bool ya = std::any_of(copia.datos.perfiles.begin(), copia.datos.perfiles.end(), [&](const PerfilAmigo& p) { return p.registro == hex; });
				if (ya) { informe.iguales++; continue; }
				if (copia.datos.perfiles.size() >= Base::kMaxPerfiles) { error = "se alcanzó el máximo de perfiles"; return false; }
				PerfilAmigo n; n.id = idNuevo(copia.datos); n.nombre = nombreSinNombre(copia.datos, i + 1); n.registro = hex; n.actualizado = sello; n.sinNombre = true;
				copia.datos.perfiles.push_back(std::move(n));
				informe.nuevos++;
			}
		}
		base = std::move(copia);
		return true;
	}

	// ---- Link → PES --------------------------------------------------------------

	bool prepararSala(const std::string& rutaSystem, Base& base, const std::vector<std::string>& idsPedidos, const std::string& sello, Informe& informe, std::string& error) {
		if (!traerDePes(rutaSystem, base, sello, informe, error)) return false;   // 1) nada se pierde
		// los elegidos, sin repetir y que existan
		std::vector<std::string> ids;
		for (auto& id : idsPedidos) {
			if (!base.porId(id)) { informe.avisos.push_back("El perfil " + id + " ya no existe; se salta"); continue; }
			if (std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
		}
		if (ids.empty()) { error = "elige al menos un amigo"; return false; }

		Guardado g;
		if (!Guardado::abrir(rutaSystem, g, error)) return false;
		const auto us = g.usuarios();
		std::vector<std::string> hexHueco(kUsuarios);
		for (int i = 0; i < kUsuarios; i++) hexHueco[i] = aHex(us[i].bytes);

		// huecos cuyo contenido NO quedó guardado en ningún perfil (p. ej. nombre repetido): no se pisan
		std::vector<bool> intocable(kUsuarios, false);
		for (int i = 0; i < kUsuarios; i++) {
			const bool guardado = std::any_of(base.datos.perfiles.begin(), base.datos.perfiles.end(), [&](const PerfilAmigo& p) { return p.registro == hexHueco[i]; });
			if (!guardado || base.protegido(i + 1)) intocable[i] = true;
		}

		std::vector<bool> usado(kUsuarios, false);
		std::vector<Asignacion> puestos;
		std::vector<const PerfilAmigo*> pendientes;
		for (auto& id : ids) {   // 2) los que ya están en algún hueco con ese mismo contenido no se escriben
			const PerfilAmigo* p = base.porId(id);
			int donde = -1;
			for (int i = 0; i < kUsuarios && donde < 0; i++) if (!usado[i] && hexHueco[i] == p->registro) donde = i;
			if (donde >= 0) { usado[donde] = true; puestos.push_back({ donde + 1, id }); }
			else pendientes.push_back(p);
		}
		// 3) huecos libres: sin proteger, sin usar, de atrás hacia delante
		std::vector<int> libres;
		for (int i = kUsuarios - 1; i >= 0; i--) if (!usado[i] && !intocable[i]) libres.push_back(i);
		if (pendientes.size() > libres.size()) {
			error = "no caben: elegiste " + std::to_string(ids.size()) + " y hay sitio para " + std::to_string(puestos.size() + libres.size()) + " (huecos protegidos o con datos sin guardar: " + std::to_string(kUsuarios - static_cast<int>(puestos.size() + libres.size())) + ")";
			return false;
		}
		for (size_t k = 0; k < pendientes.size(); k++) {
			std::vector<uint8_t> reg;
			std::string e2;
			if (!deHex(pendientes[k]->registro, reg) || !g.ponerUsuario(libres[k], reg, e2)) { error = "el perfil «" + pendientes[k]->nombre + "» no se puede escribir: " + e2; return false; }
			puestos.push_back({ libres[k] + 1, pendientes[k]->id });
		}
		std::sort(puestos.begin(), puestos.end(), [](const Asignacion& a, const Asignacion& b) { return a.hueco < b.hueco; });
		if (!pendientes.empty()) {   // 4) escritura segura: copia con sello, temporal, relectura, reemplazo
			if (!g.reemplazar(rutaSystem, sello, error, &informe.copia)) return false;
			informe.escritos = static_cast<int>(pendientes.size());
		}
		base.datos.seleccion = ids;
		base.datos.ultima = puestos;
		base.datos.ultimaSala = sello;
		informe.puestos = puestos;
		return true;
	}
}
