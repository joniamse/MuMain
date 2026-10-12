#include "stdafx.h"

#include "UI/Modern/ModernHud.h"

#ifdef MU_RMLUI

#include "Core/Utilities/Log/MuLogger.h"
#include "Data/GameConfig/GameConfig.h"
#include "UI/Modern/HudDataModel.h"
#include "UI/Modern/RmlInput.h"
#include "UI/Modern/RmlRenderInterface.h"
#include "UI/Modern/RmlSystemInterface.h"

#include "UI/Modern/RmlIncludes.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>

#include <array>
#include <memory>

extern unsigned int WindowWidth;
extern unsigned int WindowHeight;

namespace UI::Modern
{
namespace
{
constexpr const char* kContextName = "hud";
constexpr const char* kHudDocument = "Data/UI/hud.rml";
// Documents are authored in dp at this window height; larger windows scale up.
constexpr float kReferenceWindowHeight = 720.0f;
constexpr std::array kFontFaces = {
    "fonts/LiberationSans-Regular.ttf",
    "fonts/LiberationSans-Bold.ttf",
};

constexpr int kMouseButtonCount = 3;

struct HudState
{
    RmlSystemInterface systemInterface;
    std::unique_ptr<RmlRenderInterface> renderInterface;
    HudDataModel dataModel;
    ::Rml::Context* context = nullptr;
    // Buttons whose press went to the UI; their release must not reach the game either.
    std::array<bool, kMouseButtonCount> capturedButtons{};
    bool initialised = false;
    bool failed = false;
};

HudState& State()
{
    static HudState state;
    return state;
}

::Rml::Vector2i WindowSize()
{
    return {static_cast<int>(WindowWidth), static_cast<int>(WindowHeight)};
}

bool LoadFonts()
{
    bool anyLoaded = false;
    for (const char* face : kFontFaces)
    {
        anyLoaded = ::Rml::LoadFontFace(face) || anyLoaded;
    }
    return anyLoaded;
}

bool CreateHud(HudState& state)
{
    state.context = ::Rml::CreateContext(kContextName, WindowSize());
    if (!state.context || !state.dataModel.Bind(*state.context))
    {
        return false;
    }
    // Fill the model before the document is built so its first layout shows real values.
    state.dataModel.Refresh();

    ::Rml::ElementDocument* document = state.context->LoadDocument(kHudDocument);
    if (!document)
    {
        return false;
    }
    document->Show();
    return true;
}

bool Initialise(HudState& state)
{
    state.renderInterface = std::make_unique<RmlRenderInterface>();
    ::Rml::SetSystemInterface(&state.systemInterface);
    ::Rml::SetRenderInterface(state.renderInterface.get());
    if (!::Rml::Initialise())
    {
        return false;
    }
    if (!LoadFonts())
    {
        mu::log::Get("rmlui")->error("no font face could be loaded");
        return false;
    }
    return CreateHud(state);
}

int CurrentModifiers()
{
    return Input::ConvertModifiers(SDL_GetModState());
}

bool IsTextFieldFocused(::Rml::Context& context)
{
    const ::Rml::Element* focus = context.GetFocusElement();
    if (!focus)
    {
        return false;
    }
    const ::Rml::String& tag = focus->GetTagName();
    return tag == "input" || tag == "textarea";
}

bool ProcessMouseButton(HudState& state, const SDL_Event& event)
{
    const int button = Input::ConvertMouseButton(event.button.button);
    if (button < 0)
    {
        return false;
    }

    ::Rml::Context& context = *state.context;
    const int modifiers = CurrentModifiers();
    context.ProcessMouseMove(static_cast<int>(event.button.x), static_cast<int>(event.button.y), modifiers);

    bool& captured = state.capturedButtons[static_cast<std::size_t>(button)];
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        captured = !context.ProcessMouseButtonDown(button, modifiers);
        return captured;
    }

    context.ProcessMouseButtonUp(button, modifiers);
    const bool wasCaptured = captured;
    captured = false;
    return wasCaptured;
}

bool ProcessMouseWheel(::Rml::Context& context, const SDL_Event& event)
{
    if (!context.IsMouseInteracting())
    {
        return false;
    }
    // RmlUi scrolls down for positive values; SDL reports wheel-up as positive.
    const ::Rml::Vector2f delta{-event.wheel.x, -event.wheel.y};
    context.ProcessMouseWheel(delta, CurrentModifiers());
    return true;
}

bool ProcessKey(::Rml::Context& context, const SDL_Event& event)
{
    if (!IsTextFieldFocused(context))
    {
        return false;
    }
    const ::Rml::Input::KeyIdentifier key = Input::ConvertKey(event.key.key);
    const int modifiers = Input::ConvertModifiers(event.key.mod);
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        context.ProcessKeyDown(key, modifiers);
    }
    else
    {
        context.ProcessKeyUp(key, modifiers);
    }
    return true;
}

bool ProcessText(::Rml::Context& context, const SDL_Event& event)
{
    if (!IsTextFieldFocused(context))
    {
        return false;
    }
    context.ProcessTextInput(::Rml::String(event.text.text));
    return true;
}

void UpdateLayout(::Rml::Context& context)
{
    const ::Rml::Vector2i size = WindowSize();
    if (context.GetDimensions() != size)
    {
        context.SetDimensions(size);
    }
    context.SetDensityIndependentPixelRatio(static_cast<float>(size.y) / kReferenceWindowHeight);
}
} // namespace

bool IsEnabled()
{
    return !State().failed && GameConfig::GetInstance().GetModernHud();
}

void RenderHud()
{
    if (!IsEnabled())
    {
        return;
    }
    HudState& state = State();
    if (!state.initialised)
    {
        state.initialised = true;
        if (!Initialise(state))
        {
            mu::log::Get("rmlui")->error("modern HUD disabled: initialisation failed");
            state.failed = true;
            return;
        }
    }

    UpdateLayout(*state.context);
    state.dataModel.Refresh();
    state.context->Update();

    state.renderInterface->BeginFrame(static_cast<int>(WindowWidth), static_cast<int>(WindowHeight));
    state.context->Render();
    state.renderInterface->EndFrame();
}

bool ProcessEvent(const SDL_Event& event)
{
    HudState& state = State();
    if (!IsEnabled() || !state.context)
    {
        return false;
    }

    ::Rml::Context& context = *state.context;
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_MOTION:
        context.ProcessMouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y),
                                 CurrentModifiers());
        return false;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return ProcessMouseButton(state, event);
    case SDL_EVENT_MOUSE_WHEEL:
        return ProcessMouseWheel(context, event);
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        context.ProcessMouseLeave();
        return false;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return ProcessKey(context, event);
    case SDL_EVENT_TEXT_INPUT:
        return ProcessText(context, event);
    default:
        return false;
    }
}

void Shutdown()
{
    HudState& state = State();
    if (!state.initialised)
    {
        return;
    }
    ::Rml::Shutdown();
    state.renderInterface.reset();
    state.context = nullptr;
    state.initialised = false;
}
} // namespace UI::Modern

#else // MU_RMLUI

namespace UI::Modern
{
bool IsEnabled()
{
    return false;
}
bool ProcessEvent(const SDL_Event& /*event*/)
{
    return false;
}
void RenderHud() {}
void Shutdown() {}
} // namespace UI::Modern

#endif // MU_RMLUI
