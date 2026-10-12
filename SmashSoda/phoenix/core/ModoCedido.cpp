#include "ModoCedido.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace phoenix::cedido {

	using json = nlohmann::json;

	namespace {
		std::string cedMinusculas(std::string s) {
			for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return s;
		}
		std::string cedBarras(std::string s) {
			std::replace(s.begin(), s.end(), '/', '\\');
			while (!s.empty() && s.back() == '\\') s.pop_back();
			return s;
		}
	}

	// ---- Acciones ---------------------------------------------------------------
	bool accionPermitida(const std::string& accion, const json& datos) {
		// Lo que el invitado necesita para jugar: ver la sala, el partido y repartir mandos.
		static const std::set<std::string> permitidas = {
			"ui.seccion", "ui.copiar", "ui.recargar",
			"sala.copiarEnlace", "chat.enviar", "chatGlobal.abierto",
			"mandos.conectar", "mandos.desconectar", "mandos.bloquear", "mandos.liberar", "mandos.asignar",
			"mandos.intercambiar", "mandos.formacion", "mandos.equipo", "mandos.tomar", "mandos.herramienta",
			"solicitud.aceptar", "solicitud.rechazar",
			"partido.preparar", "partido.iniciar", "partido.gol", "partido.pausa", "partido.cambiarLados",
			"partido.finalizar", "partido.cancelar", "partido.historial",
			"cedido.salir", "cedido.abrirPes",
		};
		if (accion == "espera.decidir") {
			// Sentar o dejar mirando sí; expulsar no (eso es del anfitrión).
			const std::string como = datos.is_object() && datos.contains("como") && datos["como"].is_string() ? datos["como"].get<std::string>() : "";
			return como == "jugador" || como == "espectador";
		}
		return permitidas.count(accion) > 0;
	}

	// ---- Ventanas ----------------------------------------------------------------
	std::string nombreExe(const std::string& ruta) {
		const std::string r = cedBarras(ruta);
		const size_t b = r.find_last_of('\\');
		return cedMinusculas(b == std::string::npos ? r : r.substr(b + 1));
	}

	Ventana clasificar(const std::string& rutaExe, bool esLink, const std::string& carpetaJuego,
		const std::string& exeJuego, const std::vector<std::string>& extras) {
		if (esLink) return Ventana::Link;
		if (rutaExe.empty()) return Ventana::Otra;
		const std::string nombre = nombreExe(rutaExe);
		if (nombre == "pes2021.exe") return Ventana::Pes;
		if (!exeJuego.empty() && nombre == nombreExe(exeJuego)) return Ventana::Pes;
		const std::string carpeta = cedMinusculas(cedBarras(carpetaJuego));
		if (!carpeta.empty()) {
			const std::string ruta = cedMinusculas(cedBarras(rutaExe));
			if (ruta.size() > carpeta.size() && ruta.compare(0, carpeta.size(), carpeta) == 0 && ruta[carpeta.size()] == '\\') return Ventana::Pes;
		}
		for (const std::string& e : extras) {
			if (!e.empty() && nombre == nombreExe(e)) return Ventana::Pes;
		}
		return Ventana::Otra;
	}

	bool debeRecuperarFoco(Ventana foco, uint64_t msDesdeFisico, uint64_t quietoMs) {
		return foco == Ventana::Otra && msDesdeFisico >= quietoMs;
	}

	// ---- Filtro ---------------------------------------------------------------------
	bool Filtro::ctrl() const { return _abajo[vk::CTRL] || _abajo[vk::LCTRL] || _abajo[vk::RCTRL]; }
	bool Filtro::alt() const { return _abajo[vk::ALT] || _abajo[vk::LALT] || _abajo[vk::RALT]; }
	bool Filtro::shift() const { return _abajo[vk::SHIFT] || _abajo[vk::LSHIFT] || _abajo[vk::RSHIFT]; }

	bool Filtro::decidir(uint16_t c, Ventana foco) const {
		// Siempre fuera: tecla Windows (menú Inicio y todos sus atajos) e Impr Pant (recortes).
		if (c == vk::LWIN || c == vk::RWIN || c == vk::IMPR) return false;
		// Teclas que abren programas (correo, multimedia, app 1/2).
		if (c >= 0xB4 && c <= 0xB7) return false;
		// Los modificadores solos no hacen nada fuera de su ventana: siempre pasan.
		const bool modificador = c == vk::SHIFT || c == vk::CTRL || c == vk::ALT || (c >= vk::LSHIFT && c <= vk::RALT);
		if (modificador) return true;
		// Fuera de PES y Link no se escribe nada (el guardián devuelve el foco enseguida).
		if (foco != Ventana::Pes && foco != Ventana::Link) return false;
		const bool conAlt = alt(), conCtrl = ctrl();
		// Cambiar de ventana o abrir menús de Windows: Alt+Tab, Alt+Esc, Alt+Espacio, Ctrl+Esc, Ctrl+Shift+Esc.
		if (conAlt && (c == vk::TAB || c == vk::ESC || c == vk::ESPACIO)) return false;
		if (conCtrl && c == vk::ESC) return false;
		// Alt+F4: cierra PES (se vuelve a abrir con ABRIR PES), pero nunca Link.
		if (conAlt && c == vk::F4) return foco == Ventana::Pes;
		// Shift+Tab: overlay de Steam (trae navegador web) si PES es de Steam.
		if (shift() && c == vk::TAB) return false;
		if (foco == Ventana::Link) {
			// Atajos del navegador interno: imprimir, guardar, abrir, ventanas nuevas, código, descargas…
			if (c == vk::F7 || c == vk::F12) return false;
			if (conCtrl && !conAlt) {   // Ctrl+Alt = AltGr (arroba, etc.): se deja
				switch (c) {
				case 'N': case 'O': case 'P': case 'S': case 'T': case 'U': case 'J': case 'H': case 'D': case 'G':
					return false;
				case 'I': case 'C':
					return !shift();   // Ctrl+Shift+I/C = herramientas; Ctrl+C solo = copiar
				default: break;
				}
			}
		}
		return true;
	}

	bool Filtro::tecla(uint16_t codigo, bool pulsada, Ventana foco) {
		const size_t i = codigo & 0xFF;
		if (!pulsada) {
			if (_abajo[i]) { _abajo[i] = false; return true; }
			if (_tragada[i]) { _tragada[i] = false; return false; }
			return true;   // soltar algo que no conocemos (p. ej. pulsado antes del modo): soltar no hace daño
		}
		if (_abajo[i] && !decidir(codigo, foco)) {
			// Repetición de una tecla ya pulsada que ahora formaría un atajo prohibido
			// (Tab sostenido + Alt): se traga la repetición; su «soltar» sí pasará.
			return false;
		}
		if (_abajo[i]) return true;
		if (decidir(codigo, foco)) { _abajo[i] = true; _tragada[i] = false; return true; }
		_tragada[i] = true;
		return false;
	}

	bool Filtro::boton(int b, bool pulsado, Ventana bajo) {
		if (b < 0 || b >= 5) return false;
		const size_t i = static_cast<size_t>(b);
		if (!pulsado) {
			if (_botonAbajo[i]) { _botonAbajo[i] = false; return true; }
			if (_botonTragado[i]) { _botonTragado[i] = false; return false; }
			return true;
		}
		if (_botonAbajo[i]) return true;
		if (bajo == Ventana::Pes || bajo == Ventana::Link) { _botonAbajo[i] = true; _botonTragado[i] = false; return true; }
		_botonTragado[i] = true;
		return false;
	}

	void Filtro::reiniciar() {
		_abajo.fill(false);
		_tragada.fill(false);
		_botonAbajo.fill(false);
		_botonTragado.fill(false);
	}

}
