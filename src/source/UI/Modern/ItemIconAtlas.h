#pragma once

#ifdef MU_RMLUI

#include <cstdint>
#include <string>

namespace UI::Modern::ItemIcons
{
// Item icons for the modern UI. MU items are 3D models, so each needed item is drawn once with
// the same code as the inventory (RenderItem3D, with its lighting and its excellent/level glow)
// into an offscreen "atlas page" texture, which RmlUi then shows: <img src="itemicons:N" rect="..">.
//
// A page has the window's size (the item camera of the UI is built for it) and holds the icons
// in a grid; when an item which isn't there yet is requested, the whole page is drawn again on the
// next frame (a handful of draws, rarely). The icons are still images: no hover rotation.

struct Key
{
    int type = -1;
    int level = 0;
    int excellentFlags = 0;
    int ancientDiscriminator = 0;

    bool operator==(const Key&) const = default;
};

struct Icon
{
    // The RmlUi image source, e.g. "itemicons:0:3" (page and generation).
    std::string source;
    // The part of the page with the icon, as RmlUi's "rect" attribute: "x y width height".
    std::string rect;
};

// Where the icon of the item is; requests it when it isn't drawn yet and returns false meanwhile.
[[nodiscard]] bool Find(const Key& key, Icon& icon);

// Draws the pages with new icons. Call once per frame inside the 2D pass, before the RmlUi
// documents are updated (the capture is replayed before the frame's main pass, so the icons are
// ready when RmlUi draws them in the same frame).
void RenderPending();

// The GPU texture of an "itemicons:" image source, for the RmlUi render interface; null if it
// doesn't exist (any more).
[[nodiscard]] void* FindTexture(const std::string& source, int& width, int& height);

// The prefix of the image sources.
inline constexpr const char* kSourceScheme = "itemicons:";
} // namespace UI::Modern::ItemIcons

#endif // MU_RMLUI
