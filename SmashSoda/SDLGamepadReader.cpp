#include "SDLGamepadReader.h"

vector<SDLGamepad>& SDLGamepadReader::getGamepads()
{
    return _gamepads;
}

void SDLGamepadReader::fetchGamepads()
{
    fetchGamepads(vector<int>());
}

void SDLGamepadReader::fetchGamepads(const vector<int>& xinputExcluidos)
{
    // Phoenix: un mando virtual de Link nunca puede ser maestro (se copiaría a sí mismo o a otro puesto).
    const auto excluido = [&](SDL_Joystick* joy) {
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

            _gamepads.push_back(SDLGamepad(joy));
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


