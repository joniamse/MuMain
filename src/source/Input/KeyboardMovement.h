#pragma once

class CHARACTER;
class OBJECT;

// Optional W/A/S/D movement ([Modern] WasdMovement). The keys pick a screen direction
// (W up, S down, A left, D right, diagonals combine); the hero walks a short path in that
// direction, re-planned tile by tile while the keys stay down, and stops when released.
// Movement uses the same path finding and walk packets as clicking on the ground.
//
// While enabled, the original actions of W/A/S/D (W potion slot, A master skill tree,
// D command window, S personal shop) need Shift. Ctrl and Shift never move the hero.
namespace Input::KeyboardMovement
{
[[nodiscard]] bool IsEnabled();

// Called once per hero update. Returns true when the keyboard drives the hero this tick,
// so the mouse-driven ground movement is skipped.
bool Update(CHARACTER* hero, OBJECT* heroObject);

// Use instead of SEASON3B::IsPress for hotkeys bound to W/A/S/D.
[[nodiscard]] bool IsHotkeyPressed(int key);
} // namespace Input::KeyboardMovement
