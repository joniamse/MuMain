#pragma once

// Resolution-independent HUD rendered with RmlUi (HTML/CSS documents under Data/UI).
// Without RmlUi (MU_RMLUI undefined) both functions are no-ops.
namespace UI::Modern
{
// Lazily initialises RmlUi on the first call, then refreshes and draws the HUD.
// Call inside the main scene's 2D pass, after the legacy UI.
void RenderHud();

// Releases RmlUi and its GPU resources. Call before the renderer shuts down.
void Shutdown();
} // namespace UI::Modern
