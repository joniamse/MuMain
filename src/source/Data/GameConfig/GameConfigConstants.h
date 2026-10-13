#pragma once

namespace CfgSections
{
    inline constexpr wchar_t CfgSectionWindow[]     = L"Window";
    inline constexpr wchar_t CfgSectionGraphics[]   = L"Graphics";
    inline constexpr wchar_t CfgSectionAudio[]      = L"Audio";
    inline constexpr wchar_t CfgSectionUI[]         = L"UI";
    inline constexpr wchar_t CfgSectionLogin[]      = L"LOGIN";
    inline constexpr wchar_t CfgSectionConnectionSettings[] = L"CONNECTION SETTINGS";
    inline constexpr wchar_t CfgSectionCamera[] = L"Camera";
    inline constexpr wchar_t CfgSectionRender[] = L"Render";
    // Optional modernizations, each switchable in the in-game "Modern settings" window.
    inline constexpr wchar_t CfgSectionModern[] = L"Modern";
}

namespace CfgKeys
{
    // Window
    inline constexpr wchar_t CfgKeyWidth[]      = L"Width";
    inline constexpr wchar_t CfgKeyHeight[]     = L"Height";
    inline constexpr wchar_t CfgKeyWindowed[]   = L"Windowed";

    // Audio — volume 0 = off, >0 = on (no separate Enabled flag).
    inline constexpr wchar_t CfgKeySoundVolume[]  = L"SoundVolume";
    inline constexpr wchar_t CfgKeyMusicVolume[] = L"MusicVolume";

    // Login
    inline constexpr wchar_t CfgKeyRememberMe[]        = L"RememberMe";
    inline constexpr wchar_t CfgKeySavePassword[]      = L"SavePassword";
    inline constexpr wchar_t CfgKeyLanguage[]          = L"Language";
    inline constexpr wchar_t CfgKeyEncryptedUsername[] = L"EncryptedUsername";
    inline constexpr wchar_t CfgKeyEncryptedPassword[] = L"EncryptedPassword";

    // Connection
    inline constexpr wchar_t CfgKeyServerIP[]   = L"ServerIP";
    inline constexpr wchar_t CfgKeyServerPort[] = L"ServerPort";

    // UI
    inline constexpr wchar_t CfgKeyUILocale[] = L"Locale";
    inline constexpr wchar_t CfgKeyFont[]     = L"Font";
    // 1 = HTML/CSS (RmlUi) HUD replaces the legacy gauges; 0 = legacy HUD only.
    inline constexpr wchar_t CfgKeyModernHud[] = L"ModernHud";

    // Camera
    inline constexpr wchar_t CfgKeyZoom[] = L"Zoom";

    // Render
    // DXP-08: Core Profile GL context flip. 0 = compatibility (rollback), 1 = core.
    inline constexpr wchar_t CfgKeyCoreProfile[] = L"CoreProfile";
    inline constexpr wchar_t CfgKeySortParticleDraws[] = L"SortParticleDraws";
    inline constexpr wchar_t CfgKeyVSync[] = L"VSync";

    // Modern
    // Move the hero with W/A/S/D; the original W/A/S/D actions move to Shift+W/A/S/D.
    inline constexpr wchar_t CfgKeyWasdMovement[] = L"WasdMovement";
    // Mipmaps + anisotropic filtering for textures loaded from then on (restart to apply fully).
    inline constexpr wchar_t CfgKeyEnhancedTextures[] = L"EnhancedTextures";
}

namespace CfgDefaults
{
    inline constexpr int  CfgDefaultWindowWidth  = 1024;
    inline constexpr int  CfgDefaultWindowHeight = 768;
    inline constexpr bool CfgDefaultWindowed     = true;

    inline constexpr int  CfgDefaultSoundVolume = 5;
    inline constexpr int  CfgDefaultMusicVolume = 5;

    inline constexpr bool CfgDefaultRememberMe = false;
    inline constexpr bool CfgDefaultSavePassword = false;
    inline constexpr wchar_t CfgDefaultLanguage[] = L"Eng";
    inline constexpr wchar_t CfgDefaultEncryptedUsername[] = L"";
    inline constexpr wchar_t CfgDefaultEncryptedPassword[] = L"";

    inline constexpr wchar_t CfgDefaultServerIP[] = L"localhost";
    inline constexpr int CfgDefaultServerPort = 44405;

    inline constexpr int CfgDefaultZoom = 1735;  // OrbitalCamera DEFAULT_RADIUS — matches Default-cam camera-to-Hero distance

    // I18N locale code; "en" is the default the resx generator falls back to.
    inline constexpr wchar_t CfgDefaultUILocale[] = L"en";

    // UI font family name. Empty = each platform's built-in default (Tahoma on
    // Windows, fontconfig "sans-serif" on Linux), so the look is unchanged until
    // the user picks a font. Any value is passed through as the GDI face name.
    inline constexpr wchar_t CfgDefaultFont[] = L"";

    // Only effective in builds with RmlUi (MU_RMLUI).
    inline constexpr bool CfgDefaultModernHud = true;

    // Legacy config compatibility only. SDL GPU ignores this key and default.
    inline constexpr bool CfgDefaultCoreProfile = true;

    // Opt-in until real effects have been visually checked on target hardware.
    inline constexpr bool CfgDefaultSortParticleDraws = false;
    inline constexpr bool CfgDefaultVSync = true;

    // WASD changes long-standing hotkeys, so players opt in.
    inline constexpr bool CfgDefaultWasdMovement = false;
    inline constexpr bool CfgDefaultEnhancedTextures = true;
}
