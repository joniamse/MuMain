#pragma once

namespace Core::Input
{
    void RecordLeftMouseButtonPressEdge();
    void ClearLeftMouseButtonPressEdge();

    // A mouse button (VK_LBUTTON, VK_RBUTTON or VK_MBUTTON) pressed on the modern UI: it reads as
    // up for the game until it is released, so a click on the UI never reaches the world (dropping
    // a carried item, walking, attacking).
    void SetMouseButtonCaptured(int virtualKey, bool captured);

    // Portable replacement for the Win32 "is this key currently down" check
    // (HIBYTE(GetAsyncKeyState(vk)) == 128 / & 0x8000), backed by SDL keyboard
    // state. The argument is a Win32 virtual-key code (VK_*) or an ASCII letter
    // or digit, matching what the existing call sites already pass.
    bool IsKeyDown(int virtualKey);
}
