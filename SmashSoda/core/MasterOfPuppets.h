#pragma once

#include "GamepadClient.h"
#include "../SDLGamepadReader.h"
#include "../XInputReader.h"
#include "Stopwatch.h"
#include <thread>
#include <mutex>

class MasterOfPuppets
{
public:
	static MasterOfPuppets& instance;
	
	MasterOfPuppets();
	void init();
	
	void start();
	void stop();

	void fetchSDLGamepads();
	void mapInputs();
	void setMasterIndex(int index);
	int getMasterIndex();
	void setVirtualMaster(bool enabled);
	bool getVirtualMaster();
	void setVirtualMasterState(XINPUT_GAMEPAD vpad);

	vector<SDLGamepad>& getSDLGamepads();
	vector<GamepadState>& getXInputGamepads();
	SDLGamepadReader& lectorSdl() { return _sdlReader; }   // Phoenix: mostrar360 / ocultos360

	mutex inputMutex;
	Stopwatch stopwatch;
	bool isSDLEngine;

private:
	void internalFetchSDLGamepads();
	void mapFromVirtualMaster();
	void mapFromSDL();
	void mapFromXInput();
	bool isRunning = false;
	bool _isVirtualPad = false;
	int _maestroSdl = -1;          // Phoenix: SDL_JoystickID del maestro (la lista se reordena; el ID no)
	vector<int> xinputDeLink();    // Phoenix: casillas XInput de los mandos virtuales de Link
	void reubicarMaestro();        // Phoenix: masterIndex = posición actual del maestro
	
	int masterIndex;

	SDLGamepadReader _sdlReader;
	XInputReader _xinputReader;
	XINPUT_STATE _vMasterState;

	thread _thread;
};




