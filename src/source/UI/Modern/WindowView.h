#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"
#include "UI/Scaling/UITransform.h"

#include <vector>

struct tagITEM;
typedef tagITEM ITEM;

namespace SEASON3B
{
class CNewUIObj;
class CNewUIInventoryCtrl;
} // namespace SEASON3B

// Shared pieces of the modern looks drawn under legacy windows (the underlay, see ModernHud):
// boxes placed in window pixels where the legacy window has its parts.
namespace UI::Modern::WindowView
{
// A rectangle in window pixels, relative to its panel, with what the document needs to style it.
struct Box
{
    float left = 0;
    float top = 0;
    float width = 0;
    float height = 0;
    ::Rml::String kind;
    ::Rml::String label;
    ::Rml::String tip;
    // Equipment slots: "" | "worn" | "worn-more" | "broken".
    ::Rml::String warning;
    bool filled = false;
    bool active = false;
    bool locked = false;

    bool operator==(const Box&) const = default;
};

// Registers Box ({ left, top, width, height, kind, label, tip, warning, filled, active, locked })
// and std::vector<Box> in a data model.
void RegisterBox(::Rml::DataModelConstructor& constructor);

// Window-local rectangles of a legacy window to pixels relative to its panel.
class Mapper
{
public:
    // The panel's top-left corner is `origin`, in the window's local coordinates.
    Mapper(const SEASON3B::CNewUIObj& window, POINT origin);

    [[nodiscard]] Box Map(const RECT& rect) const;
    [[nodiscard]] float OriginX() const { return m_originX; }
    [[nodiscard]] float OriginY() const { return m_originY; }
    [[nodiscard]] float ScaleX() const { return m_transform.scaleX; }
    [[nodiscard]] float ScaleY() const { return m_transform.scaleY; }

private:
    UI::Scaling::Transform m_transform;
    float m_originX;
    float m_originY;
};

// 'normal' | 'high' (+7 and up) | 'excellent' | 'ancient': the frame colour of an item.
[[nodiscard]] const char* ItemKind(const ITEM& item);

// An item grid: its box, one int per square (for data-for) and a box per item, relative to the
// grid, with the item's kind.
void DescribeGrid(SEASON3B::CNewUIInventoryCtrl& grid, const Mapper& mapper, Box& box, std::vector<int>& cells,
                  std::vector<Box>& items);
} // namespace UI::Modern::WindowView

#endif // MU_RMLUI
