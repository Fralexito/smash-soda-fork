// =============================================================================
//  Phoenix Link · Control de la sala desde la interfaz Phoenix
// -----------------------------------------------------------------------------
//  Métodos de HostSettingsWidget definidos fuera del archivo del autor para
//  que sincronizar versiones nuevas de Smash Soda no genere conflictos.
//  Reproducen exactamente el flujo del botón original «Start Hosting».
// =============================================================================

#include "../../widgets/HostSettingsWidget.h"
#include "../../services/OverlayService.h"
#include "../PhoenixBuild.h"

#include <algorithm>
#include <cstring>

bool HostSettingsWidget::phoenixAbrirSala(std::string& error) {
	try {
		if (_hosting.isRunning()) return true;

		// Los volúmenes vivos están en Config (los cambia el panel de audio, clásico o web):
		// se copian antes de savePreferences para que abrir la sala no los devuelva atrás.
		_micVolume = static_cast<int>(Config::cfg.audio.micVolume);
		_speakersVolume = static_cast<int>(Config::cfg.audio.speakersVolume);

		Config::cfg.Save();
		savePreferences();

		if (!validateSettings()) {
			error = _validateError.empty() ? "Revisa la configuración de la sala." : _validateError;
			return false;
		}

		_hosting.setHostConfig("", _gameID, _maxGuests, false, _secret);
		_hosting.applyHostConfig();
		_hosting.startHosting();
		if (_onHostRunningStatusCallback != nullptr) _onHostRunningStatusCallback(true);
		updateSecretLink();
		return true;
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}
	catch (...) {
		error = "No se pudo abrir la sala.";
		return false;
	}
}

void HostSettingsWidget::phoenixCerrarSala() {
	try {
		if (!_hosting.isRunning()) return;
		_hosting.stopHosting();
		if (_onHostRunningStatusCallback != nullptr) _onHostRunningStatusCallback(false);
	}
	catch (...) {
		// Cerrar nunca debe tumbar la app.
	}
}

std::string HostSettingsWidget::phoenixEnlace() {
	updateSecretLink();
	return std::string(_shareLink);
}

std::string HostSettingsWidget::phoenixNombreSala() {
	return std::string(_roomName);
}

int HostSettingsWidget::phoenixPlazas() {
	return static_cast<int>(_maxGuests);
}

// =============================================================================
//  Opciones de sala para la interfaz web. Cada método reproduce el efecto del
//  control original de render(); además guarda Config al momento (en el
//  original se guardaba al pulsar «Start/Update Hosting»).
// =============================================================================

void HostSettingsWidget::phoenixLeerOpciones(std::string& nombre, int& plazas, bool& limitador, int& limiteMs,
	std::string& biblioteca, bool& pendiente) {
	nombre = std::string(_gameName);
	plazas = static_cast<int>(_maxGuests);
	limitador = _latencyLimiter;
	limiteMs = static_cast<int>(_latencyLimit);
	biblioteca = _libraryGame;
	pendiente = _hosting.isRunning() && isDirty();
}

void HostSettingsWidget::phoenixCambiarNombre(const std::string& nombre) {
	try {
		std::string n = nombre.substr(0, 50); // el original recorta a 50
		strcpy_s(_gameName, n.c_str());
		Config::cfg.room.game = _gameName;
		Config::cfg.Save();
		if (_hosting.isRunning()) _updated = true;
	}
	catch (...) {}
}

void HostSettingsWidget::phoenixCambiarPlazas(int plazas) {
	_maxGuests = (std::max)(0, (std::min)(20, plazas)); // mismo rango que «Guest Slots»
	Config::cfg.room.guestLimit = _maxGuests;
	Config::cfg.Save();
	if (_hosting.isRunning()) _updated = true;
}

void HostSettingsWidget::phoenixCambiarLimitador(bool activo, int limiteMs) {
	_latencyLimiter = activo;
	_latencyLimit = (std::max)(0, (std::min)(64, limiteMs)); // mismo rango que «Latency Limit»
	Config::cfg.room.latencyLimit = _latencyLimiter;
	Config::cfg.room.latencyLimitThreshold = _latencyLimit;
	Config::cfg.Save();
	if (_hosting.isRunning()) _updated = true;
}

bool HostSettingsWidget::phoenixCambiarQuiosco(bool activo) {
	// Igual que el original: el modo quiosco solo existe con un juego de la biblioteca
	if (activo && _libraryGame == "Default") {
		Config::cfg.kioskMode.enabled = false;
		return false;
	}
	Config::cfg.kioskMode.enabled = activo;
	Config::cfg.Save();
	return true;
}

void HostSettingsWidget::phoenixCambiarOverlay(bool activo) {
	Config::cfg.overlay.enabled = activo;
	Config::cfg.Save();
	if (_hosting.isRunning()) {
		if (activo) OverlayService::instance().start();
		else OverlayService::instance().stop();
	}
}

void HostSettingsWidget::phoenixElegirBiblioteca(const std::string& juego) {
	try {
		_libraryGame = "Default";
		for (size_t i = 0; i < Cache::cache.gameList.getGames().size(); ++i) {
			if (Cache::cache.gameList.getGames()[i].name == juego) {
				_libraryGame = juego;
				strcpy_s(_gameName, Cache::cache.gameList.getGames()[i].name.c_str());
				Config::cfg.room.game = _gameName;
				break;
			}
		}
		if (_libraryGame == "Default" && Config::cfg.kioskMode.enabled) Config::cfg.kioskMode.enabled = false;
		Config::cfg.Save();
		if (_hosting.isRunning()) _updated = true;
	}
	catch (...) {
		_libraryGame = "Default";
	}
}

bool HostSettingsWidget::phoenixAplicarCambios(std::string& error) {
	try {
		if (!_hosting.isRunning()) return true; // se aplican solos al abrir la sala
		if (!validateSettings()) {
			error = _validateError.empty() ? "Revisa la configuración de la sala." : _validateError;
			return false;
		}
		_micVolume = static_cast<int>(Config::cfg.audio.micVolume);
		_speakersVolume = static_cast<int>(Config::cfg.audio.speakersVolume);
		savePreferences();
		// Mismo camino que «Update Settings» del original
		_hosting.setHostConfig(_roomName, _gameID, _maxGuests, !Config::cfg.room.privateRoom, _secret);
		_hosting.applyHostConfig();
		if (phoenix::kSodaArcadeHabilitado) {
			if (!Config::cfg.room.privateRoom) Arcade::instance.createPost();
			else Arcade::instance.deletePost();
		}
		_updated = false;
		updateSecretLink();
		return true;
	}
	catch (const std::exception& e) {
		error = e.what();
		return false;
	}
	catch (...) {
		error = "No se pudieron aplicar los cambios.";
		return false;
	}
}
