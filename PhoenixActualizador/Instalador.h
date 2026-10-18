#pragma once

#include <string>
#include <windows.h>

// =============================================================================
//  Phoenix Link · PhoenixActualizador: instala una versión ya bajada y comprobada
// -----------------------------------------------------------------------------
//  Lo lanza Link (Actualizacion.cpp) desde %LOCALAPPDATA%\Phoenix Link\actualizacion.
//  1. Espera a que Link se cierre.
//  2. aplicar: guarda en «respaldo» cada archivo que se va a pisar y anota los nuevos;
//     después copia lo nuevo. Si algo falla a medias, vuelve a dejarlo como estaba.
//  3. Abre Link nuevo con «--ok <archivo>» y espera a que diga que arrancó bien.
//  4. Si no arranca (se cierra o no avisa a tiempo): restaurar y abrir el Link de antes.
//  Nunca toca la carpeta Trybuchet (ajustes en modo portable), _actualizacion,
//  portable.txt ni imgui.ini. Si la carpeta de Link no es escribible (Program Files),
//  solo la copia se hace con permiso de administrador; Link siempre se abre normal.
// =============================================================================

namespace phoenix::instalador {

	bool sePuedeEscribir(const std::wstring& carpeta);
	/// Copia `nueva` sobre `destino` guardando lo pisado en `respaldo` (se vacía antes).
	bool aplicar(const std::wstring& destino, const std::wstring& nueva, const std::wstring& respaldo, std::string& error);
	/// Deja `destino` como antes de aplicar.
	bool restaurar(const std::wstring& destino, const std::wstring& respaldo, std::string& error);

	struct Supervision {
		DWORD pid = 0;                        ///< Link viejo: se espera a que termine
		std::wstring destino, nueva, respaldo, okArchivo, resultado;
		std::string version, anterior;
		std::wstring yoMismo;                 ///< ruta de este exe (para pedir permiso de administrador)
		std::wstring exe = L"PhoenixLink.exe";
		DWORD esperaCierreMs = 60000, esperaArranqueMs = 90000;
	};
	/// 0 = instalada · 1 = no arrancó y se restauró la anterior · 2 = no se cambió nada (fallo antes de copiar).
	int supervisar(const Supervision& s);

}
