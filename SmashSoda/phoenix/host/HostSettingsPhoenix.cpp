// =============================================================================
//  Phoenix Soda · Control de la sala desde la interfaz Phoenix
// -----------------------------------------------------------------------------
//  Métodos de HostSettingsWidget definidos fuera del archivo del autor para
//  que sincronizar versiones nuevas de Smash Soda no genere conflictos.
//  Reproducen exactamente el flujo del botón original «Start Hosting».
// =============================================================================

#include "../../widgets/HostSettingsWidget.h"

bool HostSettingsWidget::phoenixAbrirSala(std::string& error) {
	try {
		if (_hosting.isRunning()) return true;

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
