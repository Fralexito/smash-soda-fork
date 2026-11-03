#pragma once

#include <d3d11.h>
#include <string>
#include <vector>

class WGCapture
{
public:
	WGCapture();
	~WGCapture();

	static bool isSupported();

	/// 0.3.61: si `window` no es nulo se captura solo esa ventana (WGC); si es nulo, el monitor entero.
	bool start(ID3D11Device* device, ID3D11DeviceContext* context, HMONITOR monitor, HWND window = nullptr);
	/// 0.3.61: títulos (UTF-8) de las ventanas abiertas que se pueden capturar.
	static std::vector<std::string> listWindowTitles();
	/// 0.3.61: busca una ventana visible cuyo título sea exactamente `title` (UTF-8). nullptr si no existe.
	static HWND findWindowByTitle(const std::string& title);
	void stop();
	bool isActive() const;

	ID3D11Texture2D* acquireFrame();
	/// Phoenix: espera hasta `ms` a que Windows entregue un cuadro nuevo. true = hay uno listo para acquireFrame().
	bool waitFrame(unsigned ms);
	void getFrameSize(int& width, int& height) const;

private:
	struct Impl;
	Impl* _impl;
};
