#pragma once

#include <functional>
#include <memory>
#include <string>

// =============================================================================
//  Phoenix Link · Anfitrión WebView2 (el navegador de Windows dentro de la app)
// -----------------------------------------------------------------------------
//  Crea un WebView2 hijo de la ventana principal y le sirve la carpeta `ui/`
//  (junto al exe) en https://phoenix.local/ sin red ni servidor: cada archivo se
//  entrega desde el disco con WebResourceRequested.
//
//  Fallo seguro: si falta WebView2Loader.dll, el Runtime de WebView2, o algo
//  falla al crear, el estado pasa a Fallo con un texto claro y la app sigue con
//  la interfaz ImGui. Nada de aquí lanza excepciones hacia afuera.
//
//  Este .cpp se compila aparte (sin pch ni unity build) para que WebView2.h no
//  se mezcle con el resto del proyecto.
// =============================================================================

namespace phoenix::web {

	enum class EstadoAnfitrion { Apagado, Creando, Cargando, Listo, Fallo };

	class AnfitrionWeb {
	public:
		using AlMensaje = std::function<void(const std::string& json)>;

		AnfitrionWeb();
		~AnfitrionWeb();
		AnfitrionWeb(const AnfitrionWeb&) = delete;
		AnfitrionWeb& operator=(const AnfitrionWeb&) = delete;

		/// `ventana` es el HWND principal. `carpetaUi`: carpeta con index.html.
		/// `carpetaDatos`: perfil propio de WebView2 (caché, localStorage).
		/// Devuelve false si no pudo ni empezar (ver error()).
		bool iniciar(void* ventana, const std::wstring& carpetaUi, const std::wstring& carpetaDatos, bool modoDesarrollo);
		void cerrar();

		/// Llamar en WM_SIZE / WM_MOVE de la ventana principal.
		void ajustar();
		void ventanaMovida();
		/// Minimizada: el WebView deja de dibujar (ahorra CPU/GPU).
		void minimizada(bool si);

		void mostrar(bool si);
		bool visible() const;
		void enfocar();
		void recargar();

		/// Envía JSON (texto UTF-8) a la página. Ignorado si no está cargada.
		void enviar(const std::string& json);
		void alMensaje(AlMensaje fn);

		EstadoAnfitrion estado() const;
		std::string error() const;
		std::string versionRuntime() const;
		double segundosDesdeInicio() const;

		/// ¿Hay WebView2 Runtime instalado? Devuelve su versión o "" (no crea nada).
		static std::string runtimeInstalado();

		struct Impl;
	private:
		std::shared_ptr<Impl> _impl;
	};

}
