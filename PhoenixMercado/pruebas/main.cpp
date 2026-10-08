// =============================================================================
//  Phoenix Mercado · Pruebas del Core (sin internet, sin Windows)
//  Usa una web «falsa» (HttpFalso) para probar todas las respuestas posibles.
// =============================================================================

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "../core/ClienteMercado.h"
#include "../core/Copias.h"
#include "../core/Sha256.h"

using namespace mercado;
namespace fs = std::filesystem;

static int fallos = 0, total = 0;
#define CHECK(cond) do { total++; if (!(cond)) { fallos++; std::printf("  FALLA %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

struct HttpFalso : ClienteHttp {
	RespuestaHttp siguiente;
	std::vector<std::string> ultimasCabeceras;
	std::string ultimaUrl, contenidoDescarga;
	RespuestaHttp peticion(const std::string&, const std::string& url, const std::string&, const std::vector<std::string>& cab) override {
		ultimaUrl = url; ultimasCabeceras = cab; return siguiente;
	}
	bool descargar(const std::string&, const std::string& ruta, long long, std::string&) override {
		std::ofstream(aRuta(ruta), std::ios::binary) << contenidoDescarga; return true;
	}
};

struct TokenFalso : FuenteToken {
	std::string t;
	std::string token() override { return t; }
	bool guardar(const std::string& x) override { t = x; return true; }
	void borrar() override { t.clear(); }
	std::string nombreModo() const override { return "falso"; }
};

int main() {
	std::printf("SHA-256\n");
	CHECK(sha256::deTexto("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	CHECK(sha256::deTexto("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	CHECK(sha256::deTexto(std::string(1000000, 'a')) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

	std::printf("Sobre de respuestas\n");
	HttpFalso http; TokenFalso tok; std::string logs;
	ClienteMercado api(http, tok, [&](const std::string&, const std::string& t) { logs += t + "\n"; });

	http.siguiente = { 200, R"({"ok":true,"version_api":"mercado-0.1.0","datos":{"ok":true,"version_api":"mercado-0.1.0","hora":"2026-10-08T01:37:30.830Z"}})", "" };
	auto e = api.eco();
	CHECK(e.ok() && e.valor->versionApi == "mercado-0.1.0");
	CHECK(http.ultimaUrl == std::string(kBaseUrl) + "/eco");
	bool sinAuth = true; for (auto& c : http.ultimasCabeceras) if (c.rfind("Authorization", 0) == 0) sinAuth = false;
	CHECK(sinAuth); // /eco va sin token

	auto y0 = api.yo();
	CHECK(!y0.ok() && y0.error.codigo == "NO_VINCULADO"); // sin token ni siquiera llama

	tok.t = "pml_SECRETO1234567890abcdef";
	http.siguiente = { 401, R"({"ok":false,"version_api":"mercado-0.1.0","error":{"codigo":"TOKEN_INVALIDO"}})", "" };
	auto y1 = api.yo();
	CHECK(!y1.ok() && y1.error.codigo == "TOKEN_INVALIDO" && y1.error.http == 401);
	CHECK(tok.t.empty()); // token rechazado → se borra
	CHECK(logs.find("SECRETO") == std::string::npos); // el token jamás llega al log

	http.siguiente = { 502, "<html>Bad gateway</html>", "" };
	CHECK(api.eco().error.codigo == "RESPUESTA_INVALIDA");
	http.siguiente = { 0, "", "timeout" };
	CHECK(api.eco().error.codigo == "RED");

	CHECK(limpiarSecretos("Authorization: Bearer pml_abc xyz") == "Authorization: Bearer *** xyz");

	std::printf("Option oficial + verificación\n");
	tok.t = "pml_otro";
	const std::string contenido = "contenido-de-prueba";
	http.siguiente = { 200, std::string(R"({"ok":true,"datos":{"version":"v1","sha256":")") + sha256::deTexto(contenido)
		+ R"(","tamano":19,"url":"https://x/y?token=abc","expira_en_seg":300,"notas":null}})", "" };
	auto o = api.optionActual();
	CHECK(o.ok() && o.valor->version == "v1" && o.valor->url.find("?token=") != std::string::npos);

	const fs::path tmp = fs::temp_directory_path() / "phoenix-mercado-pruebas";
	fs::remove_all(tmp); fs::create_directories(tmp);
	http.contenidoDescarga = contenido;
	CHECK(api.descargarOption(*o.valor, (tmp / "bueno").string()).ok());
	CHECK(api.descargarOption(*o.valor, (tmp / "bueno").string()).error.codigo == "DESTINO_OCUPADO"); // no sobrescribe
	http.contenidoDescarga = "alterado";
	auto malo = api.descargarOption(*o.valor, (tmp / "malo").string());
	CHECK(!malo.ok() && malo.error.codigo == "OPTION_HUELLA_DISTINTA" && !fs::exists(tmp / "malo"));

	std::printf("Copias de seguridad\n");
	const fs::path original = tmp / "EDIT00000000";
	std::ofstream(original, std::ios::binary) << "datos del option file";
	const std::string huellaAntes = sha256::deArchivo(original.string());
	auto c1 = copias::crear(original.string(), (tmp / "copias").string());
	auto c2 = copias::crear(original.string(), (tmp / "copias").string());
	CHECK(c1.ok() && c2.ok() && c1.valor->ruta != c2.valor->ruta); // dos copias, ninguna pisa a otra
	CHECK(c1.ok() && c1.valor->sha256 == huellaAntes);
	CHECK(sha256::deArchivo(original.string()) == huellaAntes);   // original intacto
	CHECK(copias::crear((tmp / "no-existe").string(), tmp.string()).error.codigo == "ARCHIVO_NO_EXISTE");
	fs::remove_all(tmp);

	std::printf("\n%d/%d pruebas OK\n", total - fallos, total);
	return fallos == 0 ? 0 : 1;
}
