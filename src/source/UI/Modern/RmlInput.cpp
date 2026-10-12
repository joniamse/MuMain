#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/RmlInput.h"

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>

namespace UI::Modern::Input
{
namespace
{
constexpr int kLeftButton = 0;
constexpr int kRightButton = 1;
constexpr int kMiddleButton = 2;
constexpr int kUnsupportedButton = -1;

::Rml::Input::KeyIdentifier ConvertLetterOrDigit(SDL_Keycode key)
{
    using ::Rml::Input::KeyIdentifier;
    if (key >= SDLK_A && key <= SDLK_Z)
    {
        return static_cast<KeyIdentifier>(::Rml::Input::KI_A + (key - SDLK_A));
    }
    if (key >= SDLK_0 && key <= SDLK_9)
    {
        return static_cast<KeyIdentifier>(::Rml::Input::KI_0 + (key - SDLK_0));
    }
    return ::Rml::Input::KI_UNKNOWN;
}
} // namespace

::Rml::Input::KeyIdentifier ConvertKey(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_BACKSPACE: return ::Rml::Input::KI_BACK;
    case SDLK_TAB:       return ::Rml::Input::KI_TAB;
    case SDLK_RETURN:    return ::Rml::Input::KI_RETURN;
    case SDLK_KP_ENTER:  return ::Rml::Input::KI_NUMPADENTER;
    case SDLK_ESCAPE:    return ::Rml::Input::KI_ESCAPE;
    case SDLK_SPACE:     return ::Rml::Input::KI_SPACE;
    case SDLK_DELETE:    return ::Rml::Input::KI_DELETE;
    case SDLK_INSERT:    return ::Rml::Input::KI_INSERT;
    case SDLK_HOME:      return ::Rml::Input::KI_HOME;
    case SDLK_END:       return ::Rml::Input::KI_END;
    case SDLK_PAGEUP:    return ::Rml::Input::KI_PRIOR;
    case SDLK_PAGEDOWN:  return ::Rml::Input::KI_NEXT;
    case SDLK_LEFT:      return ::Rml::Input::KI_LEFT;
    case SDLK_RIGHT:     return ::Rml::Input::KI_RIGHT;
    case SDLK_UP:        return ::Rml::Input::KI_UP;
    case SDLK_DOWN:      return ::Rml::Input::KI_DOWN;
    default:             return ConvertLetterOrDigit(key);
    }
}

int ConvertModifiers(SDL_Keymod modifiers)
{
    int result = 0;
    if (modifiers & SDL_KMOD_CTRL)
    {
        result |= ::Rml::Input::KM_CTRL;
    }
    if (modifiers & SDL_KMOD_SHIFT)
    {
        result |= ::Rml::Input::KM_SHIFT;
    }
    if (modifiers & SDL_KMOD_ALT)
    {
        result |= ::Rml::Input::KM_ALT;
    }
    if (modifiers & SDL_KMOD_CAPS)
    {
        result |= ::Rml::Input::KM_CAPSLOCK;
    }
    return result;
}

int ConvertMouseButton(Uint8 button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:   return kLeftButton;
    case SDL_BUTTON_RIGHT:  return kRightButton;
    case SDL_BUTTON_MIDDLE: return kMiddleButton;
    default:                return kUnsupportedButton;
    }
}
} // namespace UI::Modern::Input

#endif // MU_RMLUI
