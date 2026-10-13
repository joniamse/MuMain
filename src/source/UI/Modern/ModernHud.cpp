#include "stdafx.h"

#include "UI/Modern/ModernHud.h"

#include <string_view>

#ifdef MU_RMLUI

#include "Core/Input/KeyState.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Data/GameConfig/GameConfig.h"
#include "UI/Modern/BottomBarDataModel.h"
#include "UI/Modern/ChatDataModel.h"
#include "UI/Modern/HudDataModel.h"
#include "UI/Modern/InventoryDataModel.h"
#include "UI/Modern/ItemIconAtlas.h"
#include "UI/Modern/PotionBarDataModel.h"
#include "UI/Modern/RmlInput.h"
#include "UI/Modern/RmlRenderInterface.h"
#include "UI/Modern/RmlSystemInterface.h"
#include "UI/Modern/SettingsDataModel.h"
#include "UI/Modern/SkillBarDataModel.h"
#include "UI/Modern/VipWindowDataModel.h"

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
// Drawn before the legacy UI: the modern look of legacy windows, under their items.
constexpr const char* kUnderlayContextName = "underlay";
constexpr const char* kInventoryDocument = "Data/UI/inventory.rml";
constexpr const char* kChatDocument = "Data/UI/chat.rml";
constexpr const char* kHudDocument = "Data/UI/hud.rml";
constexpr const char* kSettingsDocument = "Data/UI/settings.rml";
constexpr const char* kVipDocument = "Data/UI/vip.rml";
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
    BottomBarDataModel barModel;
    SkillBarDataModel skillModel;
    PotionBarDataModel potionModel;
    SettingsDataModel settingsModel;
    VipWindowDataModel vipModel;
    InventoryDataModel inventoryModel;
    ChatDataModel chatModel;
    ::Rml::Context* context = nullptr;
    ::Rml::Context* underlayContext = nullptr;
    ::Rml::ElementDocument* hudDocument = nullptr;
    ::Rml::ElementDocument* settingsDocument = nullptr;
    ::Rml::ElementDocument* vipDocument = nullptr;
    bool settingsOpen = false;
    bool vipOpen = false;
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

// The underlay is optional: without it (or with a document missing) the legacy windows keep
// their classic look, since they only skip their own drawing when the underlay is there.
void CreateUnderlay(HudState& state)
{
    state.underlayContext = ::Rml::CreateContext(kUnderlayContextName, WindowSize());
    if (state.underlayContext == nullptr || !state.inventoryModel.Bind(*state.underlayContext)
        || !state.chatModel.Bind(*state.underlayContext))
    {
        mu::log::Get("rmlui")->error("modern windows disabled: the underlay context could not be created");
        state.underlayContext = nullptr;
        return;
    }
    state.inventoryModel.Refresh();
    state.chatModel.Refresh();
    for (const char* path : {kInventoryDocument, kChatDocument})
    {
        ::Rml::ElementDocument* document = state.underlayContext->LoadDocument(path);
        if (document == nullptr)
        {
            mu::log::Get("rmlui")->error("modern windows disabled: {} could not be loaded", path);
            state.underlayContext = nullptr;
            return;
        }
        document->Show();
    }
}

bool CreateHud(HudState& state)
{
    state.context = ::Rml::CreateContext(kContextName, WindowSize());
    if (!state.context || !state.dataModel.Bind(*state.context) || !state.barModel.Bind(*state.context) ||
        !state.skillModel.Bind(*state.context) || !state.potionModel.Bind(*state.context) ||
        !state.settingsModel.Bind(*state.context) ||
        !state.vipModel.Bind(*state.context))
    {
        return false;
    }
    // Fill the models before the document is built so its first layout shows real values.
    state.dataModel.Refresh();
    state.barModel.Refresh();
    state.skillModel.Refresh();
    state.potionModel.Refresh();

    state.hudDocument = state.context->LoadDocument(kHudDocument);
    state.settingsDocument = state.context->LoadDocument(kSettingsDocument);
    // The VIP window is optional: without it (missing or broken document) the rest still works.
    state.vipDocument = state.context->LoadDocument(kVipDocument);
    if (state.vipDocument == nullptr)
    {
        mu::log::Get("rmlui")->error("VIP window disabled: {} could not be loaded", kVipDocument);
    }
    CreateUnderlay(state);
    return state.hudDocument != nullptr && state.settingsDocument != nullptr;
}

// Shows each document while it is wanted: the HUD with the HD interface, the settings window
// while open (also with the classic interface).
void SyncDocumentVisibility(HudState& state, bool hudEnabled)
{
    if (state.settingsModel.ConsumeCloseRequest())
    {
        state.settingsOpen = false;
    }
    if (state.vipModel.ConsumeCloseRequest())
    {
        state.vipOpen = false;
    }
    const auto setVisible = [](::Rml::ElementDocument* document, bool visible) {
        if (document->IsVisible() != visible)
        {
            visible ? document->Show() : document->Hide();
        }
    };
    setVisible(state.hudDocument, hudEnabled);
    setVisible(state.settingsDocument, state.settingsOpen);
    if (state.vipDocument != nullptr)
    {
        setVisible(state.vipDocument, state.vipOpen);
    }
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

// The game also reads the mouse buttons from the device state (Core::Input::IsKeyDown), not only
// from these events: a button held on the UI must read as up there.
void SetCapturedForGame(int rmlButton, bool captured)
{
    constexpr std::array kVirtualKeys = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
    if (rmlButton >= 0 && rmlButton < static_cast<int>(kVirtualKeys.size()))
    {
        Core::Input::SetMouseButtonCaptured(kVirtualKeys[static_cast<std::size_t>(rmlButton)], captured);
    }
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
        SetCapturedForGame(button, captured);
        return captured;
    }

    context.ProcessMouseButtonUp(button, modifiers);
    const bool wasCaptured = captured;
    captured = false;
    SetCapturedForGame(button, false);
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

// Something of the modern UI is on screen: the HUD, or one of its windows.
bool IsActive()
{
    return !State().failed && (IsEnabled() || State().settingsOpen || State().vipOpen);
}

bool GetStatusWindowTop(float& windowY)
{
    const HudState& state = State();
    if (!IsEnabled() || state.hudDocument == nullptr)
    {
        return false;
    }
    ::Rml::Element* status = state.hudDocument->GetElementById("status");
    if (status == nullptr)
    {
        return false;
    }
    windowY = status->GetAbsoluteTop();
    return true;
}

bool AreWindowsModern()
{
    const HudState& state = State();
    return IsEnabled() && state.initialised && state.underlayContext != nullptr
           && GameConfig::GetInstance().GetModernWindows();
}

void RenderUnderlay()
{
    if (!AreWindowsModern())
    {
        return;
    }
    HudState& state = State();
    UpdateLayout(*state.underlayContext);
    state.inventoryModel.Refresh();
    state.chatModel.Refresh();
    state.underlayContext->Update();

    state.renderInterface->BeginFrame(static_cast<int>(WindowWidth), static_cast<int>(WindowHeight));
    state.underlayContext->Render();
    state.renderInterface->EndFrame();
}

void OpenSettings()
{
    HudState& state = State();
    state.settingsOpen = true;
    if (state.context)
    {
        state.settingsModel.Load();
    }
}

bool IsSettingsOpen()
{
    return State().settingsOpen;
}

void OpenVip()
{
    State().vipModel.Request();
}

bool IsVipOpen()
{
    return State().vipOpen;
}

bool HandleServerNotice(const wchar_t* text)
{
    if (text == nullptr)
    {
        return false;
    }
    HudState& state = State();
    if (!state.vipModel.HandleNotice(text))
    {
        return false;
    }
    if (state.vipModel.ConsumeOpenRequest() && !state.failed)
    {
        state.vipOpen = true;
    }
    return true;
}

void RenderHud()
{
    if (!IsActive())
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

    const bool hudEnabled = IsEnabled();
    SyncDocumentVisibility(state, hudEnabled);
    UpdateLayout(*state.context);
    if (hudEnabled)
    {
        // New item icons are drawn first: the capture runs before this frame's main pass.
        ItemIcons::RenderPending();
        state.dataModel.Refresh();
        state.barModel.Refresh();
        state.skillModel.Refresh();
        state.potionModel.Refresh();
    }
    if (state.vipOpen)
    {
        state.vipModel.Tick();
    }
    state.context->Update();

    state.renderInterface->BeginFrame(static_cast<int>(WindowWidth), static_cast<int>(WindowHeight));
    state.context->Render();
    state.renderInterface->EndFrame();
}

bool ProcessEvent(const SDL_Event& event)
{
    HudState& state = State();
    if (!IsActive() || !state.context)
    {
        return false;
    }

    ::Rml::Context& context = *state.context;
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_MOTION:
        context.ProcessMouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y),
                                 CurrentModifiers());
        // The underlay only takes motion, for its hover styles: the legacy windows over it
        // handle the clicks.
        if (state.underlayContext != nullptr)
        {
            state.underlayContext->ProcessMouseMove(static_cast<int>(event.motion.x),
                                                    static_cast<int>(event.motion.y), CurrentModifiers());
        }
        return false;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return ProcessMouseButton(state, event);
    case SDL_EVENT_MOUSE_WHEEL:
        return ProcessMouseWheel(context, event);
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        context.ProcessMouseLeave();
        if (state.underlayContext != nullptr)
        {
            state.underlayContext->ProcessMouseLeave();
        }
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
void OpenSettings() {}
bool IsSettingsOpen()
{
    return false;
}
void OpenVip() {}
bool IsVipOpen()
{
    return false;
}
// Without the modern UI the VIP data is still kept out of the chat.
bool HandleServerNotice(const wchar_t* text)
{
    return text != nullptr && std::wstring_view(text).starts_with(L"#MUVIP|");
}
bool ProcessEvent(const SDL_Event& /*event*/)
{
    return false;
}
void RenderHud() {}
bool AreWindowsModern()
{
    return false;
}
bool GetStatusWindowTop(float& /*windowY*/)
{
    return false;
}
void RenderUnderlay() {}
void Shutdown() {}
} // namespace UI::Modern

#endif // MU_RMLUI
