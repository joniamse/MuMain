#pragma once

// Resolution-independent HUD rendered with RmlUi (HTML/CSS documents under Data/UI).
// Without RmlUi (MU_RMLUI undefined) the functions are no-ops and IsEnabled() is false.
union SDL_Event;

namespace UI::Modern
{
// True when the build has RmlUi, [UI] ModernHud=1 and initialisation has not failed.
// Legacy HUD parts that the modern HUD replaces skip their rendering when this is true.
[[nodiscard]] bool IsEnabled();

// True when legacy windows (the inventory) use the modern look: the HD interface is on, so is
// [Modern] ModernWindows, and the underlay loaded. Those windows then draw only their items.
[[nodiscard]] bool AreWindowsModern();

// Draws the modern look of the open legacy windows. Call inside the main scene's 2D pass,
// right before the legacy UI, which draws the items over it.
void RenderUnderlay();

// Opens the "Modern settings" window (Data/UI/settings.rml). It works with the classic
// interface too: RmlUi is then drawn just for this window.
void OpenSettings();
[[nodiscard]] bool IsSettingsOpen();

// Asks the server for the VIP data; the VIP window (Data/UI/vip.rml) opens when it arrives.
// Works with the classic interface too.
void OpenVip();
[[nodiscard]] bool IsVipOpen();

// Takes a server notice. Returns true when it carried data for the modern UI (VIP window,
// prefix "#MUVIP|"), which must not be shown in the chat.
[[nodiscard]] bool HandleServerNotice(const wchar_t* text);

// Lazily initialises RmlUi on the first call, then refreshes and draws the HUD.
// Call inside the main scene's 2D pass, after the legacy UI.
void RenderHud();

// Forwards an SDL input event to the modern UI. Returns true when the UI consumed it,
// in which case the game must not handle it:
//  - mouse buttons/wheel over an element with "pointer-events: auto" (class "interactive");
//    a release is consumed when its press was;
//  - keys and text while a modern UI text field has focus.
// Mouse motion is never consumed: the game still tracks the cursor.
[[nodiscard]] bool ProcessEvent(const SDL_Event& event);

// Releases RmlUi and its GPU resources. Call before the renderer shuts down.
void Shutdown();
} // namespace UI::Modern
