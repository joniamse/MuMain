#include "stdafx.h"

#include "UI/Modern/ModernHud.h"

#ifdef MU_RMLUI

#include "Core/Utilities/Log/MuLogger.h"
#include "UI/Modern/HudDataModel.h"
#include "UI/Modern/RmlRenderInterface.h"
#include "UI/Modern/RmlSystemInterface.h"

#include "UI/Modern/RmlIncludes.h"

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

struct HudState
{
    RmlSystemInterface systemInterface;
    std::unique_ptr<RmlRenderInterface> renderInterface;
    HudDataModel dataModel;
    ::Rml::Context* context = nullptr;
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

void RenderHud()
{
    HudState& state = State();
    if (state.failed)
    {
        return;
    }
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
void RenderHud() {}
void Shutdown() {}
} // namespace UI::Modern

#endif // MU_RMLUI
