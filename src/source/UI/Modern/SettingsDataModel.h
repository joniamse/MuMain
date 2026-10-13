#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

namespace UI::Modern
{
// Data model "settings" for the Modern settings window (Data/UI/settings.rml): one switch
// per optional modernization, saved to config.ini as soon as it changes.
//
// RML names:
//   hd_interface        [UI] ModernHud, applies immediately
//   wasd_movement       [Modern] WasdMovement, applies immediately
//   enhanced_textures   [Modern] EnhancedTextures, applies to textures loaded afterwards
//   textures_restart    the texture setting differs from the one the client started with
// Events:
//   toggle('hd' | 'wasd' | 'textures'), close
class SettingsDataModel
{
public:
    bool Bind(::Rml::Context& context);
    // Re-reads the saved settings, e.g. when the window opens.
    void Load();
    // True once after the window asked to be closed.
    bool ConsumeCloseRequest();

private:
    void Toggle(const ::Rml::String& setting);
    void Dirty();

    ::Rml::DataModelHandle m_handle;
    bool m_hdInterface = true;
    bool m_wasdMovement = false;
    bool m_enhancedTextures = true;
    bool m_texturesAtStartup = true;
    bool m_texturesRestart = false;
    bool m_closeRequested = false;
};
} // namespace UI::Modern

#endif // MU_RMLUI
