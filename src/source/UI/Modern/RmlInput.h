#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

#include <SDL3/SDL_events.h>

namespace UI::Modern::Input
{
// SDL key code -> RmlUi key identifier (KI_UNKNOWN for keys RmlUi does not need).
::Rml::Input::KeyIdentifier ConvertKey(SDL_Keycode key);

// SDL modifier state -> RmlUi key modifier flags.
int ConvertModifiers(SDL_Keymod modifiers);

// SDL mouse button -> RmlUi button index (0 left, 1 right, 2 middle), or -1 if unsupported.
int ConvertMouseButton(Uint8 button);
} // namespace UI::Modern::Input

#endif // MU_RMLUI
