// «Link falso» para actualizacion_test.cpp: hace de PhoenixLink.exe en una carpeta de prueba.
// Si junto a él hay romper.txt, se cierra al instante con código 7 (versión que no arranca).
// Si no, apunta su línea de órdenes en ultima-orden.txt y, si le pasan «--ok <archivo>», lo escribe (arrancó bien).
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#pragma comment(lib, "shell32.lib")
namespace fs = std::filesystem;

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	wchar_t yo[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, yo, MAX_PATH);
	const fs::path carpeta = fs::path(yo).parent_path();
	if (fs::exists(carpeta / L"romper.txt")) return 7;
	{ std::wofstream f(carpeta / L"ultima-orden.txt", std::ios::trunc); f << GetCommandLineW(); }
	int n = 0;
	LPWSTR* a = CommandLineToArgvW(GetCommandLineW(), &n);
	for (int i = 1; a && i + 1 < n; i++) if (std::wstring(a[i]) == L"--ok") { Sleep(400); const fs::path ok(a[i + 1]); std::ofstream f(ok); f << "ok"; }
	if (a) LocalFree(a);
	Sleep(800);
	return 0;
}
