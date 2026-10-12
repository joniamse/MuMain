#pragma once

// Resolution-independent HUD rendered with RmlUi (HTML/CSS documents under Data/UI).
// Without RmlUi (MU_RMLUI undefined) the functions are no-ops and IsEnabled() is false.
namespace UI::Modern
{
// True when the build has RmlUi, [UI] ModernHud=1 and initialisation has not failed.
// Legacy HUD parts that the modern HUD replaces skip their rendering when this is true.
[[nodiscard]] bool IsEnabled();

// Lazily initialises RmlUi on the first call, then refreshes and draws the HUD.
// Call inside the main scene's 2D pass, after the legacy UI.
void RenderHud();

// Releases RmlUi and its GPU resources. Call before the renderer shuts down.
void Shutdown();
} // namespace UI::Modern
