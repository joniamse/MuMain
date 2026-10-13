#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/ItemIconAtlas.h"

#include "Camera/CameraProjection.h"
#include "Camera/CameraState.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInventory.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Scaling/UITransform.h"

#include <algorithm>
#include <charconv>
#include <vector>

extern int MouseX;
extern int MouseY;
extern unsigned int WindowWidth;
extern unsigned int WindowHeight;

namespace UI::Modern::ItemIcons
{
namespace
{
// Size of one inventory square in the atlas, in window pixels: icons are drawn larger than the
// inventory shows them (20 logical units, ~45 px at 1080p), so they stay sharp when scaled.
constexpr float kSquarePixels = 60.0f;
constexpr float kLogicalSquare = 20.0f;
// Room around each icon for glows which reach past the item's squares.
constexpr int kPaddingPixels = 6;
// Far from every icon, so no icon is drawn hovered (rotating) during the capture.
constexpr int kNoMouse = -100000;

struct Entry
{
    Key key;
    int page = 0;
    // In window pixels on the page.
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    // Drawn on its page. A page drawn again keeps its earlier icons in place.
    bool ready = false;
};

struct Page
{
    std::uint32_t textureId = 0;
    int width = 0;
    int height = 0;
    // Shelf packing: icons fill rows from the left; a new row starts below the tallest one.
    int cursorX = 0;
    int cursorY = 0;
    int rowHeight = 0;
    bool dirty = true;
};

struct State
{
    std::vector<Page> pages;
    std::vector<Entry> entries;
    int windowWidth = 0;
    int windowHeight = 0;
    int generation = 0;
};

State& GetState()
{
    static State state;
    return state;
}

// Logical coordinates map 1:1 to the page through this transform, scaled so a square is kSquarePixels.
UI::Scaling::Transform AtlasTransform()
{
    constexpr float scale = kSquarePixels / kLogicalSquare;
    return UI::Scaling::Transform{scale, scale, 0.0f, 0.0f, scale};
}

// Starts over when the window size changed: the pages have the window's size.
void SyncWindowSize(State& state)
{
    const int width = static_cast<int>(WindowWidth);
    const int height = static_cast<int>(WindowHeight);
    if (state.windowWidth == width && state.windowHeight == height)
    {
        return;
    }

    // The old textures stay alive: RmlUi may still hold them for a frame. A resize is rare.
    state.pages.clear();
    state.entries.clear();
    state.windowWidth = width;
    state.windowHeight = height;
    ++state.generation;
}

bool Allocate(State& state, int width, int height, Entry& entry)
{
    const int paddedWidth = width + 2 * kPaddingPixels;
    const int paddedHeight = height + 2 * kPaddingPixels;
    if (paddedWidth > state.windowWidth || paddedHeight > state.windowHeight)
    {
        return false;
    }

    auto tryPage = [&](Page& page, int pageIndex) {
        if (page.cursorX + paddedWidth > page.width)
        {
            page.cursorX = 0;
            page.cursorY += page.rowHeight;
            page.rowHeight = 0;
        }
        if (page.cursorY + paddedHeight > page.height)
        {
            return false;
        }
        entry.page = pageIndex;
        entry.x = page.cursorX + kPaddingPixels;
        entry.y = page.cursorY + kPaddingPixels;
        entry.width = width;
        entry.height = height;
        page.cursorX += paddedWidth;
        page.rowHeight = std::max(page.rowHeight, paddedHeight);
        page.dirty = true;
        return true;
    };

    if (!state.pages.empty() && tryPage(state.pages.back(), static_cast<int>(state.pages.size()) - 1))
    {
        return true;
    }

    Page page;
    page.width = state.windowWidth;
    page.height = state.windowHeight;
    state.pages.push_back(page);
    return tryPage(state.pages.back(), static_cast<int>(state.pages.size()) - 1);
}

std::string MakeSource(const State& state, int pageIndex)
{
    return std::string(kSourceScheme) + std::to_string(pageIndex) + ":" + std::to_string(state.generation);
}

// Draws every icon of the page into its texture, with the camera the UI's item windows use.
void RenderPage(State& state, int pageIndex)
{
    Page& page = state.pages[static_cast<std::size_t>(pageIndex)];
    const std::uint32_t texture = mu::GetRenderer().BeginOffscreenCapture(page.textureId, static_cast<std::uint32_t>(page.width),
                                                                          static_cast<std::uint32_t>(page.height), true);
    if (texture == 0u)
    {
        return; // retried next frame
    }
    page.textureId = texture;

    const int savedMouseX = MouseX;
    const int savedMouseY = MouseY;
    MouseX = kNoMouse;
    MouseY = kNoMouse;

    EndBitmap();
    auto& renderer = mu::GetRenderer();
    renderer.SetMatrixMode(GL_PROJECTION);
    renderer.PushMatrix();
    renderer.LoadIdentity();
    SetRenderViewport(0, 0, page.width, page.height);
    gluPerspective2(1.f, static_cast<float>(page.width) / static_cast<float>(page.height), RENDER_ITEMVIEW_NEAR,
                    RENDER_ITEMVIEW_FAR);
    renderer.SetMatrixMode(GL_MODELVIEW);
    renderer.PushMatrix();
    renderer.LoadIdentity();
    CameraProjection::GetOpenGLMatrix(g_Camera.Matrix);
    EnableDepthTest();
    EnableDepthMask();

    {
        UI::Scaling::ScopedActiveTransform layout(AtlasTransform());
        const float toLogical = kLogicalSquare / kSquarePixels;
        for (Entry& entry : state.entries)
        {
            if (entry.page != pageIndex)
            {
                continue;
            }
            entry.ready = true;
            RenderItem3D(entry.x * toLogical, entry.y * toLogical, entry.width * toLogical, entry.height * toLogical,
                         entry.key.type, entry.key.level, entry.key.excellentFlags, entry.key.ancientDiscriminator,
                         false);
        }
    }

    renderer.SetMatrixMode(GL_MODELVIEW);
    renderer.PopMatrix();
    renderer.SetMatrixMode(GL_PROJECTION);
    renderer.PopMatrix();
    BeginBitmap();

    MouseX = savedMouseX;
    MouseY = savedMouseY;
    renderer.EndOffscreenCapture();
    page.dirty = false;
}
} // namespace

bool Find(const Key& key, Icon& icon)
{
    if (key.type < 0 || ItemAttribute == nullptr)
    {
        return false;
    }

    State& state = GetState();
    SyncWindowSize(state);

    auto it = std::find_if(state.entries.begin(), state.entries.end(), [&](const Entry& e) { return e.key == key; });
    if (it == state.entries.end())
    {
        const ITEM_ATTRIBUTE& attribute = ItemAttribute[key.type];
        const int squaresX = std::max<int>(1, attribute.Width);
        const int squaresY = std::max<int>(1, attribute.Height);
        Entry entry;
        entry.key = key;
        if (!Allocate(state, static_cast<int>(squaresX * kSquarePixels), static_cast<int>(squaresY * kSquarePixels), entry))
        {
            return false;
        }
        state.entries.push_back(entry);
        return false; // drawn on the next RenderPending()
    }

    if (!it->ready)
    {
        return false;
    }

    icon.source = MakeSource(state, it->page);
    icon.rect = std::to_string(it->x) + " " + std::to_string(it->y) + " " + std::to_string(it->width) + " " +
                std::to_string(it->height);
    return true;
}

void RenderPending()
{
    State& state = GetState();
    SyncWindowSize(state);
    if (!mu::GetRenderer().IsFrameActive())
    {
        return;
    }

    for (std::size_t i = 0; i < state.pages.size(); ++i)
    {
        if (state.pages[i].dirty)
        {
            RenderPage(state, static_cast<int>(i));
        }
    }
}

void* FindTexture(const std::string& source, int& width, int& height)
{
    const std::string_view prefix(kSourceScheme);
    if (source.compare(0, prefix.size(), prefix) != 0)
    {
        return nullptr;
    }

    // "itemicons:<page>:<generation>"
    int pageIndex = -1;
    int generation = -1;
    const char* begin = source.data() + prefix.size();
    const char* end = source.data() + source.size();
    auto result = std::from_chars(begin, end, pageIndex);
    if (result.ec != std::errc() || result.ptr == end || *result.ptr != ':')
    {
        return nullptr;
    }
    result = std::from_chars(result.ptr + 1, end, generation);

    const State& state = GetState();
    if (result.ec != std::errc() || generation != state.generation || pageIndex < 0
        || pageIndex >= static_cast<int>(state.pages.size()))
    {
        return nullptr;
    }

    const Page& page = state.pages[static_cast<std::size_t>(pageIndex)];
    width = page.width;
    height = page.height;
    return page.textureId != 0u ? mu::GetRenderer().GetTexturePointer(page.textureId) : nullptr;
}
} // namespace UI::Modern::ItemIcons

#endif // MU_RMLUI
