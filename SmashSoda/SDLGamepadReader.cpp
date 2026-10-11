#include "SDLGamepadReader.h"

vector<SDLGamepad>& SDLGamepadReader::getGamepads()
{
    return _gamepads;
}

void SDLGamepadReader::fetchGamepads()
{
    fetchGamepads(vector<int>());
}

void SDLGamepadReader::fetchGamepads(const vector<int>& xinputExcluidos, int virtualesXbox)
{
    // Phoenix: un mando virtual de Link nunca puede ser maestro (se copiaría a sí mismo o a otro puesto).
    // Los de Link son «Xbox 360 Controller» (045E:028E). Windows solo da casilla XInput a 4, así que además:
    // si hay tantos 045E:028E como mandos virtuales Xbox de Link, todos son de Link y se ocultan.
    SDL_JoystickUpdate();
    int x360 = 0;
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
        if (SDL_JoystickGetDeviceVendor(i) == 0x045E && SDL_JoystickGetDeviceProduct(i) == 0x028E) x360++;
    const bool ocultar360 = virtualesXbox > 0 && x360 <= virtualesXbox;
    const auto excluido = [&](SDL_Joystick* joy) {
        const bool es360 = SDL_JoystickGetVendor(joy) == 0x045E && SDL_JoystickGetProduct(joy) == 0x028E;
        if (ocultar360 && es360) return true;
        // El número de jugador solo identifica a los mandos XInput (los de Link son Xbox 360). SDL también
        // numera al DS4 y a otros mandos: comparar sin este filtro borraba el mando del anfitrión (Fralex, 0.2.11).
        if (!es360) return false;
        const int jugador = SDL_JoystickGetPlayerIndex(joy);
        for (int x : xinputExcluidos) if (x == jugador && jugador >= 0) return true;
        return false;
    };
    for (size_t i = 0; i < _gamepads.size();) {
        if (excluido(_gamepads[i].joystick)) _gamepads.erase(_gamepads.begin() + i);
        else ++i;
    }
    SDL_JoystickUpdate();

    eraseDetachedGamepads();

    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        SDL_Joystick* joy = SDL_JoystickOpen(i);
        if (SDL_JoystickGetAttached(joy))
        {
            if (excluido(joy))
            {
                continue;
            }

            if (contains(joy))
            {
                continue;
            }

            // Phoenix: un mando de Sony (VID 054C) usa el esquema DS4 (L1=9, R1=10, L3=7, R3=8); con el de Xbox
            // salían L1→R3, R1 nada, R3→L3 y L3→Options (Fralex, 0.2.13). El resto empieza como Xbox.
            const bool sony = SDL_JoystickGetVendor(joy) == 0x054C;
            _gamepads.push_back(SDLGamepad(joy, sony ? SDLGamepad::Type::DS4 : SDLGamepad::Type::XBox));
        }
    }
}

void SDLGamepadReader::eraseDetachedGamepads()
{
    vector<SDLGamepad>::iterator it;
    for (it = _gamepads.begin(); it != _gamepads.end(); ++it)
    {
        if (_gamepads.empty())
        {
            break;
        }

        if (!SDL_JoystickGetAttached((*it).joystick))
        {
            SDL_JoystickClose((*it).joystick);
            _gamepads.erase(it);

            if (_gamepads.empty())
            {
                break;
            }
            else
            {
                it = _gamepads.begin();
            }
        }
    }
}

bool SDLGamepadReader::contains(SDL_Joystick* joystick)
{
    vector<SDLGamepad>::iterator it;
    for (it = _gamepads.begin(); it != _gamepads.end(); ++it)
    {
        if ((*it).joystick == joystick)
        {
            return true;
        }
    }

    return false;
}


