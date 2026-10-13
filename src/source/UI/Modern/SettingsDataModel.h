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
//   modern_windows      [Modern] ModernWindows, applies immediately (with the HD interface)
//   wheel_zoom          [Modern] WheelZoom, applies immediately
//   hd_cursor           [Modern] HdCursor, read at startup; cursor_restart: differs from the startup value
//   modern_skin         [Modern] ModernSkin, read at startup; skin_restart: differs from the startup value
//   enhanced_textures   [Modern] EnhancedTextures, applies to textures loaded afterwards
//   textures_restart    the texture setting differs from the one the client started with
//   anti_aliasing, sharpening, color_enhance, bloom   [Modern] post-processing, immediate
// Events:
//   toggle('hd' | 'windows' | 'zoom' | 'cursor' | 'skin' | 'textures' | 'aa' | 'sharpen' | 'color' | 'bloom'), close
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
    bool m_modernWindows = true;
    bool m_wheelZoom = true;
    bool m_hdCursor = true;
    bool m_cursorAtStartup = true;
    bool m_cursorRestart = false;
    bool m_modernSkin = true;
    bool m_skinAtStartup = true;
    bool m_skinRestart = false;
    bool m_enhancedTextures = true;
    bool m_antiAliasing = true;
    bool m_sharpening = true;
    bool m_colorEnhance = true;
    bool m_bloom = true;
    bool m_texturesAtStartup = true;
    bool m_texturesRestart = false;
    bool m_closeRequested = false;
};
} // namespace UI::Modern

#endif // MU_RMLUI
