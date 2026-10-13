#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/WindowView.h"
#include "UI/Modern/RmlIncludes.h"

#include <vector>

namespace UI::Modern
{
// Data model "chat": the modern look of the legacy chat input box (Data/UI/chat.rml), drawn
// under it like the inventory (see InventoryDataModel): the legacy box still draws the typed text
// and the tooltips and takes the clicks and keys.
//
// RML names (window pixels, boxes relative to the panel):
//   visible, panel_left, panel_top, panel_width, panel_height, whisper_off
//   whisper_field, chat_field (Box)
//   buttons[] Box { left, top, width, height, kind, label, active }
//     kind 'type' (normal / party / guild / gens: the selected one is active) | 'toggle'
class ChatDataModel
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
        bool whisperOff = false;
        Box whisperField;
        Box chatField;
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
