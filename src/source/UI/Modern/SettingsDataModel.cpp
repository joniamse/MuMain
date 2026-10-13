#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/SettingsDataModel.h"

#include "Audio/DSPlaySound.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Data/GameConfig/GameConfig.h"
#include "UI/NewUI/NewUISystem.h"

namespace UI::Modern
{
bool SettingsDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("settings");
    if (!constructor)
    {
        return false;
    }

    // Textures are filtered when they load, so the value at startup is what is on screen.
    m_texturesAtStartup = GameConfig::GetInstance().GetEnhancedTextures();
    m_cursorAtStartup = GameConfig::GetInstance().GetHdCursor();

    constructor.Bind("hd_interface", &m_hdInterface);
    constructor.Bind("modern_windows", &m_modernWindows);
    constructor.Bind("wasd_movement", &m_wasdMovement);
    constructor.Bind("wheel_zoom", &m_wheelZoom);
    constructor.Bind("hd_cursor", &m_hdCursor);
    constructor.Bind("cursor_restart", &m_cursorRestart);
    constructor.Bind("enhanced_textures", &m_enhancedTextures);
    constructor.Bind("textures_restart", &m_texturesRestart);
    constructor.Bind("anti_aliasing", &m_antiAliasing);
    constructor.Bind("sharpening", &m_sharpening);
    constructor.Bind("color_enhance", &m_colorEnhance);
    constructor.Bind("bloom", &m_bloom);
    constructor.BindEventCallback(
        "toggle", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
            if (!arguments.empty())
            {
                Toggle(arguments[0].Get<::Rml::String>());
            }
        });
    constructor.BindEventCallback("close", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        m_closeRequested = true;
        PlayBuffer(SOUND_CLICK01);
    });
    m_handle = constructor.GetModelHandle();
    Load();
    return true;
}

void SettingsDataModel::Load()
{
    const GameConfig& config = GameConfig::GetInstance();
    m_hdInterface = config.GetModernHud();
    m_modernWindows = config.GetModernWindows();
    m_wasdMovement = config.GetWasdMovement();
    m_wheelZoom = config.GetWheelZoom();
    m_hdCursor = config.GetHdCursor();
    m_cursorRestart = m_hdCursor != m_cursorAtStartup;
    m_enhancedTextures = config.GetEnhancedTextures();
    m_texturesRestart = m_enhancedTextures != m_texturesAtStartup;
    m_antiAliasing = config.GetAntiAliasing();
    m_sharpening = config.GetSharpening();
    m_colorEnhance = config.GetColorEnhance();
    m_bloom = config.GetBloom();
    Dirty();
}

bool SettingsDataModel::ConsumeCloseRequest()
{
    const bool requested = m_closeRequested;
    m_closeRequested = false;
    return requested;
}

void SettingsDataModel::Toggle(const ::Rml::String& setting)
{
    GameConfig& config = GameConfig::GetInstance();
    if (setting == "hd")
    {
        config.SetModernHud(!config.GetModernHud());
        g_pNewUISystem->ApplyHudLayout();
    }
    else if (setting == "windows")
    {
        config.SetModernWindows(!config.GetModernWindows());
    }
    else if (setting == "cursor")
    {
        config.SetHdCursor(!config.GetHdCursor());
    }
    else if (setting == "zoom")
    {
        config.SetWheelZoom(!config.GetWheelZoom());
    }
    else if (setting == "wasd")
    {
        config.SetWasdMovement(!config.GetWasdMovement());
    }
    else if (setting == "textures")
    {
        config.SetEnhancedTextures(!config.GetEnhancedTextures());
    }
    else if (setting == "aa")
    {
        config.SetAntiAliasing(!config.GetAntiAliasing());
    }
    else if (setting == "sharpen")
    {
        config.SetSharpening(!config.GetSharpening());
    }
    else if (setting == "color")
    {
        config.SetColorEnhance(!config.GetColorEnhance());
    }
    else if (setting == "bloom")
    {
        config.SetBloom(!config.GetBloom());
    }
    else
    {
        mu::log::Get("rmlui")->warn("unknown modern setting '{}'", setting);
        return;
    }

    config.Save();
    PlayBuffer(SOUND_CLICK01);
    Load();
}

void SettingsDataModel::Dirty()
{
    if (!m_handle)
    {
        return;
    }
    m_handle.DirtyVariable("hd_interface");
    m_handle.DirtyVariable("modern_windows");
    m_handle.DirtyVariable("wasd_movement");
    m_handle.DirtyVariable("wheel_zoom");
    m_handle.DirtyVariable("hd_cursor");
    m_handle.DirtyVariable("cursor_restart");
    m_handle.DirtyVariable("enhanced_textures");
    m_handle.DirtyVariable("textures_restart");
    m_handle.DirtyVariable("anti_aliasing");
    m_handle.DirtyVariable("sharpening");
    m_handle.DirtyVariable("color_enhance");
    m_handle.DirtyVariable("bloom");
}
} // namespace UI::Modern

#endif // MU_RMLUI
