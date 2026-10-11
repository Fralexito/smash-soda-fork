#include "MasterOfPuppets.h"
// Singleton instance (Meyer's singleton pattern)
static MasterOfPuppets _masterOfPuppetsInstance;
MasterOfPuppets& MasterOfPuppets::instance = _masterOfPuppetsInstance;
MasterOfPuppets::MasterOfPuppets()
    : masterIndex(-1), isSDLEngine(true)
{
    stopwatch = Stopwatch(5000);
}

void MasterOfPuppets::init()
{
    _xinputReader.init();
}

void MasterOfPuppets::start()
{
    if (isRunning) return;

    isRunning = true;
    _thread = thread([&]() {
        stopwatch.start();

        while (isRunning)
        {
            inputMutex.lock();
            if (isSDLEngine) {
                if (stopwatch.isFinished())
                {
                    internalFetchSDLGamepads();
                    stopwatch.reset();
                }

                SDL_JoystickUpdate();
                mapFromSDL();
            }
            else
            {
                mapFromXInput();
            }

            if (_isVirtualPad)
            {
                mapFromVirtualMaster();
            }

            inputMutex.unlock();
            Sleep(8);
        }

        stopwatch.stop();
        _thread.detach();
        });
}

void MasterOfPuppets::stop()
{
    isRunning = false;
}

void MasterOfPuppets::fetchSDLGamepads()
{
    if (isSDLEngine)
    {
        inputMutex.lock();
        internalFetchSDLGamepads();
        inputMutex.unlock();
    }
}

void MasterOfPuppets::internalFetchSDLGamepads()
{
    if (isSDLEngine)
    {
        int virtualesXbox = 0;
        for (AGamepad* p : GamepadClient::instance.gamepads)
            if (p != nullptr && p->isConnected() && p->type() == AGamepad::Type::XBOX) virtualesXbox++;
        _sdlReader.fetchGamepads(xinputDeLink(), virtualesXbox);
        reubicarMaestro();
    }
}

vector<int> MasterOfPuppets::xinputDeLink()
{
    vector<int> r;
    for (AGamepad* p : GamepadClient::instance.gamepads)
    {
        ULONG i = 0;
        if (p != nullptr && p->indiceXInput(i)) r.push_back(static_cast<int>(i));
    }
    return r;
}

void MasterOfPuppets::reubicarMaestro()
{
    if (!isSDLEngine || _maestroSdl < 0) return;
    vector<SDLGamepad>& v = _sdlReader.getGamepads();
    int donde = -1;
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (SDL_JoystickInstanceID(v[i].joystick) == _maestroSdl) { donde = static_cast<int>(i); break; }
    }
    masterIndex = donde;   // si se desconectó, se deja de copiar (no salta a otro mando)
    GamepadClient::instance.isPuppetMaster = (masterIndex >= 0) || _isVirtualPad;
}

void MasterOfPuppets::mapInputs()
{
    if (isSDLEngine) mapFromSDL();
    else mapFromXInput();
}

void MasterOfPuppets::setMasterIndex(int index)
{
    masterIndex = index;
    vector<SDLGamepad>& v = _sdlReader.getGamepads();
    _maestroSdl = (isSDLEngine && index >= 0 && index < static_cast<int>(v.size())) ? SDL_JoystickInstanceID(v[index].joystick) : -1;
    GamepadClient::instance.isPuppetMaster = (index >= 0) || _isVirtualPad;
}

int MasterOfPuppets::getMasterIndex()
{
    return masterIndex;
}

void MasterOfPuppets::setVirtualMaster(bool enabled)
{
    _isVirtualPad = enabled;
    GamepadClient::instance.isPuppetMaster = (masterIndex >= 0) || _isVirtualPad;
}

bool MasterOfPuppets::getVirtualMaster()
{
    return _isVirtualPad;
}

void MasterOfPuppets::setVirtualMasterState(XINPUT_GAMEPAD vpad)
{
    ZeroMemory(&_vMasterState, sizeof(XINPUT_STATE));
    _vMasterState.Gamepad = vpad;
}

vector<SDLGamepad>& MasterOfPuppets::getSDLGamepads()
{
    return _sdlReader.getGamepads();
}

vector<GamepadState>& MasterOfPuppets::getXInputGamepads()
{
    return _xinputReader.readInputs();
}

void MasterOfPuppets::mapFromVirtualMaster()
{
    vector<AGamepad*>& puppets = GamepadClient::instance.gamepads;

    for (size_t i = 0; i < puppets.size(); ++i)
    {
        if (puppets[i]->isPuppet)
        {
            puppets[i]->setState(_vMasterState);
        }
    }
}

void MasterOfPuppets::mapFromSDL()
{
    vector<AGamepad*>& puppets = GamepadClient::instance.gamepads;
    vector<SDLGamepad>& gamepads = _sdlReader.getGamepads();

    if (masterIndex >= 0 && masterIndex < gamepads.size())
    {
        for (size_t i = 0; i < puppets.size(); ++i)
        {
            if (puppets[i]->isPuppet)
            {
                puppets[i]->setState(gamepads[masterIndex].getGamepadState().state);
            }
        }
    }
}

void MasterOfPuppets::mapFromXInput()
{
    vector<AGamepad*>& puppets = GamepadClient::instance.gamepads;
    vector<GamepadState>& gamepads = _xinputReader.readInputs();

    if (masterIndex >= 0 && masterIndex < gamepads.size())
    {
        for (size_t i = 0; i < puppets.size(); ++i)
        {
            if (puppets[i]->isPuppet)
            {
                puppets[i]->setState(gamepads[masterIndex].state);
            }
        }
    }
}


