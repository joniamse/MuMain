#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"
#include "UI/Modern/WindowView.h"

#include <vector>

namespace UI::Modern
{
// Data model "shop": the modern look of the two personal shop windows (Data/UI/shop.rml), drawn
// under them like the inventory (see InventoryDataModel).
//
// RML names: `mine` (the player's own shop) and `other` (the shop of another player), each a
//   ShopView { visible, panel_left, panel_top, panel_width, panel_height, open,
//              title_field (Box), grid (Box), cells[], items[], buttons[] }
//   buttons: kind 'exit' | 'open' | 'close'; locked when it can't be used.
class ShopDataModel
{
public:
    using Box = WindowView::Box;

    struct ShopView
    {
        bool visible = false;
        float panelLeft = 0;
        float panelTop = 0;
        float panelWidth = 0;
        float panelHeight = 0;
        // The player's shop is open (selling).
        bool open = false;
        Box titleField;
        Box grid;
        std::vector<int> cells;
        std::vector<Box> items;
        std::vector<Box> buttons;

        bool operator==(const ShopView&) const = default;
    };

    bool Bind(::Rml::Context& context);
    void Refresh();

private:
    ::Rml::DataModelHandle m_handle;
    ShopView m_mine;
    ShopView m_other;
};
} // namespace UI::Modern

#endif // MU_RMLUI
