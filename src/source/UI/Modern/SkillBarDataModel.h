#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

#include <vector>

namespace UI::Modern
{
// Data model "skills": the hotkey bar (1..0), the current skill, the skill picker and the
// tooltip of the hovered skill.
//
// RML names:
//   slots[] / current / picker[]  SkillView { filled, index, key, icon, rect, cooldown,
//                                 cooling, usable, selected }
//                                 icon/rect feed <img data-attr-src data-attr-rect>
//   picker_open                   the skill picker is shown
//   tooltip[]                     TooltipLineView { text, color, bold, blank } of the hovered skill
// Events:
//   use_slot(i)        make hotkey i (0..9) the current skill
//   toggle_picker      open/close the picker
//   pick(index)        make hero skill `index` current and close the picker
//   hover(index, from_picker) / unhover
//   While hovering a picker entry, Ctrl+1..0 assigns it to that hotkey (as in the original).
class SkillBarDataModel
{
public:
    struct SkillView
    {
        bool filled = false;
        int index = -1;
        ::Rml::String key;
        ::Rml::String icon;
        ::Rml::String rect;
        ::Rml::String cooldown = "0%";
        bool cooling = false;
        bool usable = true;
        bool selected = false;

        bool operator==(const SkillView&) const = default;
    };

    struct TooltipLineView
    {
        ::Rml::String text;
        ::Rml::String color;
        bool bold = false;
        bool blank = false;
    };

    bool Bind(::Rml::Context& context);
    void Refresh();

private:
    void RegisterTypes(::Rml::DataModelConstructor& constructor);
    void BindEvents(::Rml::DataModelConstructor& constructor);
    void RefreshSlots();
    void RefreshPicker();
    void AssignHoveredSkillToPressedHotkey();
    void RebuildTooltip();

    ::Rml::DataModelHandle m_handle;
    std::vector<SkillView> m_slots;
    SkillView m_current;
    std::vector<SkillView> m_picker;
    std::vector<TooltipLineView> m_tooltip;
    bool m_pickerOpen = false;

    int m_hoveredIndex = -1;
    bool m_hoveredInPicker = false;
    int m_tooltipIndex = -1;
    // Reused every frame to build views before comparing them with the bound ones.
    std::vector<SkillView> m_scratch;
};
} // namespace UI::Modern

#endif // MU_RMLUI
