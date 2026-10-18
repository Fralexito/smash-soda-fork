// PhoenixActualizador.exe — ver Instalador.h. Sin ventana; deja su registro en actualizador.log junto a resultado.json.
//   supervisar --pid N --destino D --nueva N --respaldo R --ok O --resultado J --version V --anterior A
//   aplicar    --destino D --nueva N --respaldo R      (con permiso de administrador, lo lanza «supervisar»)
//   restaurar  --destino D --respaldo R
#include "Instalador.h"

#include <shellapi.h>
#include <map>
#include <string>

#pragma comment(lib, "shell32.lib")

namespace {
	std::string utf8(const std::wstring& w) {
		if (w.empty()) return {};
		const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
		std::string s(n, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
		return s;
	}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	int n = 0;
	LPWSTR* a = CommandLineToArgvW(GetCommandLineW(), &n);
	if (!a || n < 2) return 2;
	const std::wstring modo = a[1];
	std::map<std::wstring, std::wstring> o;
	for (int i = 2; i + 1 < n; i += 2) o[a[i]] = a[i + 1];
	wchar_t yo[MAX_PATH * 2] = {};
	GetModuleFileNameW(nullptr, yo, MAX_PATH * 2);
	LocalFree(a);
	using namespace phoenix::instalador;
	std::string error;
	if (modo == L"aplicar") return aplicar(o[L"--destino"], o[L"--nueva"], o[L"--respaldo"], error) ? 0 : 1;
	if (modo == L"restaurar") return restaurar(o[L"--destino"], o[L"--respaldo"], error) ? 0 : 1;
	if (modo != L"supervisar") return 2;
	Supervision s;
	s.pid = static_cast<DWORD>(_wtoi(o[L"--pid"].c_str()));
	s.destino = o[L"--destino"]; s.nueva = o[L"--nueva"]; s.respaldo = o[L"--respaldo"];
	s.okArchivo = o[L"--ok"]; s.resultado = o[L"--resultado"];
	s.version = utf8(o[L"--version"]); s.anterior = utf8(o[L"--anterior"]);
	s.yoMismo = yo;
	if (s.destino.empty() || s.nueva.empty() || s.respaldo.empty() || s.okArchivo.empty()) return 2;
	return supervisar(s);
}
