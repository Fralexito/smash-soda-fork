#pragma once

#include <vector>
#include "SDLGamepad.h"

using namespace std;

class SDLGamepadReader
{
public:
	vector<SDLGamepad>& getGamepads();
	void fetchGamepads();
	/// Phoenix: igual, pero sin los mandos virtuales de Link (casillas XInput que ocupan).
	void fetchGamepads(const vector<int>& xinputExcluidos, int virtualesXbox = 0);
	bool mostrar360 = false;   ///< Phoenix: el anfitrión juega con un Xbox 360 de verdad (no se ocultan los 045E:028E)
	int ocultos360 = 0;        ///< Phoenix: cuántos «Xbox 360 Controller» se ocultaron en la última búsqueda

private:
	void eraseDetachedGamepads();
	bool contains(SDL_Joystick* joystick);
	static GamepadState joystickToGamepadState(SDL_Joystick* pad, bool isXInput = false);

	vector<SDLGamepad> _gamepads;
};




