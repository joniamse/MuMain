#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"
#include "UI/Modern/WindowView.h"

#include <vector>

namespace UI::Modern
{
// Data model "inventory": the modern look of the inventory window (Data/UI/inventory.rml).
// The document is drawn under the legacy UI, exactly where the legacy window is: the legacy
// window still draws the items (3D), their warning colours, the carried item and the tooltips,
// and handles every click. This model only describes what is under them.
//
// Lengths are window pixels; boxes are placed relative to the panel.
// RML names:
//   visible, panel_left, panel_top, panel_width, panel_height
//   zen, zen_tier ('low' | 'mid' | 'high' | 'max'), set_active, socket_active
//   grid (Box), cells[] (one int per square, for data-for)
//   items[], slots[], buttons[]  Box { left, top, width, height, kind, label, tip, warning, filled, active, locked }
//     items:   kind 'normal' | 'high' (+7 and up) | 'excellent' | 'ancient'
//     slots:   label of the empty slot, filled, kind of the equipped item
//     buttons: kind 'exit' | 'repair' | 'shop' | 'expand', active (repair mode on / shop open)
class InventoryDataModel
{
public:
    using Box = WindowView::Box;

    struct View
    {
        bool visible = false;
        float panelLeft = 0;
        float panelTop = 0;
        float panelWidth = 0;
        float panelHeight = 0;
        ::Rml::String zen;
        ::Rml::String zenTier;
        bool setActive = false;
        bool socketActive = false;
        Box grid;
        std::vector<int> cells;
        std::vector<Box> items;
        std::vector<Box> slots;
        std::vector<Box> buttons;

        bool operator==(const View&) const = default;
    };

    bool Bind(::Rml::Context& context);
    void Refresh();

private:
    ::Rml::DataModelHandle m_handle;
    View m_view;
    View m_scratch;
};
} // namespace UI::Modern

#endif // MU_RMLUI
