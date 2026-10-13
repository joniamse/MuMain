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

    constructor.Bind("hd_interface", &m_hdInterface);
    constructor.Bind("wasd_movement", &m_wasdMovement);
    constructor.Bind("enhanced_textures", &m_enhancedTextures);
    constructor.Bind("textures_restart", &m_texturesRestart);
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
    m_wasdMovement = config.GetWasdMovement();
    m_enhancedTextures = config.GetEnhancedTextures();
    m_texturesRestart = m_enhancedTextures != m_texturesAtStartup;
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
    else if (setting == "wasd")
    {
        config.SetWasdMovement(!config.GetWasdMovement());
    }
    else if (setting == "textures")
    {
        config.SetEnhancedTextures(!config.GetEnhancedTextures());
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
    m_handle.DirtyVariable("wasd_movement");
    m_handle.DirtyVariable("enhanced_textures");
    m_handle.DirtyVariable("textures_restart");
}
} // namespace UI::Modern

#endif // MU_RMLUI
