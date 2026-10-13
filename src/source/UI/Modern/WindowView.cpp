#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/WindowView.h"

#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/NewUIBase.h"

#include <algorithm>

extern unsigned int WindowWidth;
extern unsigned int WindowHeight;

namespace UI::Modern::WindowView
{
void RegisterBox(::Rml::DataModelConstructor& constructor)
{
    if (auto box = constructor.RegisterStruct<Box>())
    {
        box.RegisterMember("left", &Box::left);
        box.RegisterMember("top", &Box::top);
        box.RegisterMember("width", &Box::width);
        box.RegisterMember("height", &Box::height);
        box.RegisterMember("kind", &Box::kind);
        box.RegisterMember("label", &Box::label);
        box.RegisterMember("tip", &Box::tip);
        box.RegisterMember("warning", &Box::warning);
        box.RegisterMember("filled", &Box::filled);
        box.RegisterMember("active", &Box::active);
        box.RegisterMember("locked", &Box::locked);
    }
    constructor.RegisterArray<std::vector<Box>>();
}

Mapper::Mapper(const SEASON3B::CNewUIObj& window, POINT origin)
    : m_transform(UI::Scaling::TransformForLayout(window.GetLayoutMode(), static_cast<int>(WindowWidth),
                                                  static_cast<int>(WindowHeight)))
    , m_originX(UI::Scaling::PositionX(m_transform, static_cast<float>(origin.x)))
    , m_originY(UI::Scaling::PositionY(m_transform, static_cast<float>(origin.y)))
{
}

Box Mapper::Map(const RECT& rect) const
{
    Box box;
    box.left = UI::Scaling::PositionX(m_transform, static_cast<float>(rect.left)) - m_originX;
    box.top = UI::Scaling::PositionY(m_transform, static_cast<float>(rect.top)) - m_originY;
    box.width = (rect.right - rect.left) * m_transform.scaleX;
    box.height = (rect.bottom - rect.top) * m_transform.scaleY;
    return box;
}

const char* ItemKind(const ITEM& item)
{
    if (item.AncientDiscriminator > 0)
    {
        return "ancient";
    }
    if (item.ExcellentFlags > 0)
    {
        return "excellent";
    }
    return item.Level >= 7 ? "high" : "normal";
}

void DescribeGrid(SEASON3B::CNewUIInventoryCtrl& grid, const Mapper& mapper, Box& box, std::vector<int>& cells,
                  std::vector<Box>& items)
{
    const POINT& pos = grid.GetPos();
    const int columns = grid.GetNumberOfColumn();
    const int rows = grid.GetNumberOfRow();
    constexpr int square = SEASON3B::INVENTORY_SQUARE_WIDTH;
    box = mapper.Map(RECT{pos.x, pos.y, pos.x + columns * square, pos.y + rows * square});
    cells.assign(static_cast<std::size_t>(columns * rows), 0);

    const float cellX = square * mapper.ScaleX();
    const float cellY = square * mapper.ScaleY();
    items.clear();
    for (std::size_t i = 0; i < grid.GetNumberOfItems(); ++i)
    {
        const ITEM* item = grid.GetItem(static_cast<int>(i));
        if (item == nullptr || item->Type < 0)
        {
            continue;
        }
        const ITEM_ATTRIBUTE& attribute = ItemAttribute[item->Type];
        Box itemBox;
        itemBox.left = item->x * cellX;
        itemBox.top = item->y * cellY;
        itemBox.width = std::max<int>(1, attribute.Width) * cellX;
        itemBox.height = std::max<int>(1, attribute.Height) * cellY;
        itemBox.kind = ItemKind(*item);
        items.push_back(std::move(itemBox));
    }
}
} // namespace UI::Modern::WindowView

#endif // MU_RMLUI
