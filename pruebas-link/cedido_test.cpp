// Prueba de la lógica pura del modo cedido (phoenix/core/ModoCedido) y del portero del Puente.
// MSVC: cl /nologo /std:c++17 /EHsc /O2 /utf-8 /I SmashSoda /I external pruebas-link\cedido_test.cpp SmashSoda\phoenix\core\ModoCedido.cpp SmashSoda\phoenix\link\Entrega.cpp SmashSoda\phoenix\web\Puente.cpp
// g++:  g++ -std=c++17 -O2 -I SmashSoda -I external pruebas-link/cedido_test.cpp SmashSoda/phoenix/core/ModoCedido.cpp SmashSoda/phoenix/link/Entrega.cpp SmashSoda/phoenix/web/Puente.cpp
#include "phoenix/core/ModoCedido.h"
#include "phoenix/web/Puente.h"

#include <iostream>
#include <string>
#include <vector>

using namespace phoenix::cedido;
using json = nlohmann::json;

static int fallos = 0;
#define COMPROBAR(c) do { if (!(c)) { std::cerr << "FALLA linea " << __LINE__ << ": " #c "\n"; fallos++; } } while (0)

int main() {
	const Ventana P = Ventana::Pes, L = Ventana::Link, O = Ventana::Otra;

	// 1. Lista blanca de acciones
	{
		COMPROBAR(accionPermitida("mandos.asignar", json::object()));
		COMPROBAR(accionPermitida("partido.gol", json::object()));
		COMPROBAR(accionPermitida("cedido.abrirPes", json::object()));
		COMPROBAR(accionPermitida("cedido.salir", json::object()));
		COMPROBAR(accionPermitida("chat.enviar", json::object()));
		COMPROBAR(!accionPermitida("cedido.empezar", json::object()));
		COMPROBAR(!accionPermitida("cedido.pin", json::object()));
		COMPROBAR(!accionPermitida("ajustes.general", json::object()));
		COMPROBAR(!accionPermitida("web.desvincular", json::object()));
		COMPROBAR(!accionPermitida("gente.banear", json::object()));
		COMPROBAR(!accionPermitida("gente.teclado", json::object()));
		COMPROBAR(!accionPermitida("sala.cerrar", json::object()));
		COMPROBAR(!accionPermitida("sync.modulo", json::object()));
		COMPROBAR(!accionPermitida("ui.abrir", json::object()));
		COMPROBAR(!accionPermitida("ui.panelClasico", json::object()));
		COMPROBAR(!accionPermitida("ui.interfaz", json::object()));
		COMPROBAR(!accionPermitida("chatGlobal.enviar", json::object()));
		COMPROBAR(!accionPermitida("accion.nueva.que.nadie.conoce", json::object()));
		COMPROBAR(accionPermitida("espera.decidir", json{ {"como", "jugador"} }));
		COMPROBAR(accionPermitida("espera.decidir", json{ {"como", "espectador"} }));
		COMPROBAR(!accionPermitida("espera.decidir", json{ {"como", "expulsar"} }));
		COMPROBAR(!accionPermitida("espera.decidir", json::object()));
	}

	// 2. PIN
	{
		COMPROBAR(pinConFormato("1234"));
		COMPROBAR(pinConFormato("12345678"));
		COMPROBAR(!pinConFormato("123"));
		COMPROBAR(!pinConFormato("123456789"));
		COMPROBAR(!pinConFormato("12a4"));
		COMPROBAR(!pinConFormato(""));
		const std::string g = guardarPin("2580", "abcdef0123456789");
		COMPROBAR(g.rfind("abcdef0123456789$", 0) == 0);
		COMPROBAR(g.find("2580") == std::string::npos);   // nunca el PIN en claro
		COMPROBAR(pinCorrecto("2580", g));
		COMPROBAR(!pinCorrecto("2581", g));
		COMPROBAR(!pinCorrecto("25800", g));
		COMPROBAR(!pinCorrecto("2580", ""));
		COMPROBAR(!pinCorrecto("2580", "sinDolar"));
		COMPROBAR(guardarPin("2580", "otraSal") != g);    // la sal cambia el hash
	}

	// 3. Freno: 5 fallos → 60 s
	{
		Freno f;
		for (int i = 0; i < 4; ++i) { COMPROBAR(f.puedeIntentar(100.0)); f.fallo(100.0); }
		COMPROBAR(f.puedeIntentar(100.0));
		f.fallo(100.0);
		COMPROBAR(!f.puedeIntentar(100.0));
		COMPROBAR(!f.puedeIntentar(159.0));
		COMPROBAR(f.espera(130.0) > 29.0 && f.espera(130.0) < 31.0);
		COMPROBAR(f.puedeIntentar(160.0));
		f.acierto();
		COMPROBAR(f.puedeIntentar(0.0));
	}

	// 4. Clasificar procesos
	{
		const std::string carpeta = "D:\\Frank\\Games_\\PES 2021";
		const std::vector<std::string> nada;
		COMPROBAR(nombreExe("C:\\X\\Y\\PES2021.EXE") == "pes2021.exe");
		COMPROBAR(nombreExe("C:/X/sider.exe") == "sider.exe");
		COMPROBAR(clasificar("C:\\Steam\\PES2021.exe", false, "", "", nada) == P);
		COMPROBAR(clasificar("d:\\frank\\games_\\pes 2021\\sider.exe", false, carpeta, "", nada) == P);
		COMPROBAR(clasificar("D:\\Frank\\Games_\\PES 2021\\Settings.exe", false, carpeta + "\\", "", nada) == P);
		COMPROBAR(clasificar("D:\\Frank\\Games_\\PES 2021 copia\\x.exe", false, carpeta, "", nada) == O);   // no confundir carpetas parecidas
		COMPROBAR(clasificar("C:\\Windows\\explorer.exe", false, carpeta, "", nada) == O);
		COMPROBAR(clasificar("C:\\Windows\\System32\\cmd.exe", false, carpeta, "", nada) == O);
		COMPROBAR(clasificar("C:\\Program Files\\Google\\Chrome\\chrome.exe", false, carpeta, "", nada) == O);
		COMPROBAR(clasificar("C:\\Juegos\\Lanzador.exe", false, carpeta, "E:\\otro\\lanzador.EXE", nada) == P);
		COMPROBAR(clasificar("C:\\X\\cualquiera.exe", true, carpeta, "", nada) == L);
		COMPROBAR(clasificar("", false, carpeta, "", nada) == O);   // sin ruta (sistema) = no permitido
		COMPROBAR(clasificar("C:\\Tools\\DS4Windows.exe", false, carpeta, "", { "ds4windows.exe" }) == P);
	}

	// 5. ¿Devolver el foco?
	{
		COMPROBAR(debeRecuperarFoco(O, 20000));
		COMPROBAR(!debeRecuperarFoco(O, 3000));     // el anfitrión está usando su PC
		COMPROBAR(!debeRecuperarFoco(P, 99999));
		COMPROBAR(!debeRecuperarFoco(L, 99999));
		COMPROBAR(!debeRecuperarFoco(Ventana::Nada, 99999));
	}

	// 6. Teclado
	{
		Filtro f;
		// Tecla Windows: nunca, ni pulsar ni soltar
		COMPROBAR(!f.tecla(vk::LWIN, true, P));
		COMPROBAR(!f.tecla(vk::LWIN, false, P));
		COMPROBAR(!f.tecla(vk::RWIN, true, L));
		COMPROBAR(!f.tecla(vk::IMPR, true, P));
		COMPROBAR(!f.tecla(0xB6, true, P));   // lanzar app 1
		// Letras normales en PES y Link
		COMPROBAR(f.tecla('W', true, P)); COMPROBAR(f.tecla('W', false, P));
		COMPROBAR(f.tecla('A', true, L)); COMPROBAR(f.tecla('A', false, L));
		// Fuera de PES/Link no se escribe; el soltar de lo tragado también se traga
		COMPROBAR(!f.tecla('A', true, O)); COMPROBAR(!f.tecla('A', false, O));
		// Alt+Tab
		COMPROBAR(f.tecla(vk::LALT, true, P));
		COMPROBAR(!f.tecla(vk::TAB, true, P));
		COMPROBAR(!f.tecla(vk::TAB, false, P));
		// Alt+Esc, Alt+Espacio
		COMPROBAR(!f.tecla(vk::ESC, true, P)); COMPROBAR(!f.tecla(vk::ESC, false, P));
		COMPROBAR(!f.tecla(vk::ESPACIO, true, L)); COMPROBAR(!f.tecla(vk::ESPACIO, false, L));
		// Alt+F4: en PES sí, en Link no
		COMPROBAR(f.tecla(vk::F4, true, P)); COMPROBAR(f.tecla(vk::F4, false, P));
		COMPROBAR(!f.tecla(vk::F4, true, L)); COMPROBAR(!f.tecla(vk::F4, false, L));
		COMPROBAR(f.tecla(vk::LALT, false, P));
		COMPROBAR(!f.alt());
		// Shift+Tab (overlay de Steam) nunca
		COMPROBAR(f.tecla(vk::LSHIFT, true, P));
		COMPROBAR(!f.tecla(vk::TAB, true, P)); COMPROBAR(!f.tecla(vk::TAB, false, P));
		COMPROBAR(f.tecla(vk::LSHIFT, false, P));
		// Sin Alt, Tab y Esc pasan (PES los usa)
		COMPROBAR(f.tecla(vk::TAB, true, P)); COMPROBAR(f.tecla(vk::TAB, false, P));
		COMPROBAR(f.tecla(vk::ESC, true, P)); COMPROBAR(f.tecla(vk::ESC, false, P));
		// Ctrl+Esc y Ctrl+Shift+Esc
		COMPROBAR(f.tecla(vk::LCTRL, true, P));
		COMPROBAR(!f.tecla(vk::ESC, true, P)); COMPROBAR(!f.tecla(vk::ESC, false, P));
		COMPROBAR(f.tecla(vk::LSHIFT, true, P));
		COMPROBAR(!f.tecla(vk::ESC, true, P)); COMPROBAR(!f.tecla(vk::ESC, false, P));
		COMPROBAR(f.tecla(vk::LSHIFT, false, P));
		COMPROBAR(f.tecla(vk::LCTRL, false, P));
		// Atajos del navegador interno de Link
		COMPROBAR(f.tecla(vk::LCTRL, true, L));
		COMPROBAR(!f.tecla('P', true, L)); COMPROBAR(!f.tecla('P', false, L));
		COMPROBAR(!f.tecla('N', true, L)); COMPROBAR(!f.tecla('N', false, L));
		COMPROBAR(f.tecla('C', true, L)); COMPROBAR(f.tecla('C', false, L));    // copiar sí
		COMPROBAR(f.tecla('V', true, L)); COMPROBAR(f.tecla('V', false, L));    // pegar sí
		COMPROBAR(f.tecla('P', true, P)); COMPROBAR(f.tecla('P', false, P));    // en PES Ctrl+P es del juego
		COMPROBAR(f.tecla(vk::RALT, true, L));                                  // AltGr = Ctrl+Alt
		COMPROBAR(f.tecla('Q', true, L)); COMPROBAR(f.tecla('Q', false, L));    // arroba
		COMPROBAR(f.tecla(vk::RALT, false, L));
		COMPROBAR(f.tecla(vk::LCTRL, false, L));
		COMPROBAR(!f.tecla(vk::F12, true, L)); COMPROBAR(!f.tecla(vk::F12, false, L));
		COMPROBAR(f.tecla(vk::F12, true, P)); COMPROBAR(f.tecla(vk::F12, false, P));
		// Tab sostenido y luego Alt: la repetición se traga, el soltar de Tab pasa (no queda pegado)
		COMPROBAR(f.tecla(vk::TAB, true, P));
		COMPROBAR(f.tecla(vk::LALT, true, P));
		COMPROBAR(!f.tecla(vk::TAB, true, P));
		COMPROBAR(f.tecla(vk::TAB, false, P));
		COMPROBAR(f.tecla(vk::LALT, false, P));
		// Soltar algo desconocido (pulsado antes del modo) pasa: soltar no hace daño
		COMPROBAR(f.tecla('Z', false, P));
		// Una tecla que pasó en PES se suelta aunque el foco ya esté en otra ventana
		COMPROBAR(f.tecla('D', true, P));
		COMPROBAR(f.tecla('D', false, O));
		// Reiniciar olvida todo
		COMPROBAR(!f.tecla('X', true, O));
		f.reiniciar();
		COMPROBAR(f.tecla('X', false, O));
	}

	// 7. Ratón
	{
		Filtro f;
		COMPROBAR(f.boton(0, true, P)); COMPROBAR(f.boton(0, false, Ventana::Nada));
		COMPROBAR(f.boton(1, true, L)); COMPROBAR(f.boton(1, false, O));         // arrastrar fuera y soltar: pasa
		COMPROBAR(!f.boton(0, true, O)); COMPROBAR(!f.boton(0, false, P));        // clic en la barra de tareas: ni pulsar ni soltar
		COMPROBAR(!f.boton(2, true, Ventana::Nada)); COMPROBAR(!f.boton(2, false, Ventana::Nada));
		COMPROBAR(!f.boton(7, true, P));
		COMPROBAR(f.rueda(P)); COMPROBAR(f.rueda(L)); COMPROBAR(!f.rueda(O));
	}

	// 8. Portero del Puente
	{
		std::vector<std::string> salida;
		phoenix::web::Puente p([&](const std::string& s) { salida.push_back(s); });
		int ejecutadas = 0;
		p.registrar("ajustes.general", [&](const json&, uint64_t) -> std::optional<json> { ejecutadas++; return json::object(); });
		p.registrar("partido.gol", [&](const json&, uint64_t) -> std::optional<json> { ejecutadas++; return json::object(); });
		bool cedida = false;
		p.fijarPortero([&](const std::string& a, const json& d) -> std::optional<std::string> {
			if (!cedida || accionPermitida(a, d)) return std::nullopt;
			return std::string("cedida");
		}, "PC_CEDIDA");
		p.recibir(R"({"t":"pedir","id":1,"accion":"ajustes.general","datos":{}})");
		COMPROBAR(ejecutadas == 1);
		cedida = true;
		p.recibir(R"({"t":"pedir","id":2,"accion":"ajustes.general","datos":{}})");
		COMPROBAR(ejecutadas == 1);
		COMPROBAR(!salida.empty() && salida.back().find("PC_CEDIDA") != std::string::npos);
		p.recibir(R"({"t":"pedir","id":3,"accion":"partido.gol","datos":{}})");
		COMPROBAR(ejecutadas == 2);
		// Un portero que lanza deja la acción cerrada
		p.fijarPortero([](const std::string&, const json&) -> std::optional<std::string> { throw 1; }, "PC_CEDIDA");
		p.recibir(R"({"t":"pedir","id":4,"accion":"partido.gol","datos":{}})");
		COMPROBAR(ejecutadas == 2);
	}

	if (fallos == 0) std::cout << "cedido_test: TODO OK\n";
	else std::cout << "cedido_test: " << fallos << " FALLOS\n";
	return fallos == 0 ? 0 : 1;
}
