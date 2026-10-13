#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"
#include "UI/Modern/TextCapture.h"

#include <vector>

namespace UI::Modern
{
// Data model "character" for the modern character window (Data/UI/character.rml).
//
// The values come from the legacy window itself: its text output is captured
// (CNewUICharacterInfoWindow::CaptureTexts) and sorted into the header, the level table
// and one block per stat, so both windows always show the same, identically calculated
// numbers, colours included (e.g. blue for item bonuses, orange under a potion).
//
// RML names:
//   name, name_color, class_name, server_name
//   level_text, points_text, has_points, table[] (LineView)
//   stats[] StatView { name, value, color, index, can_add, details[] (LineView) }
//   can_master
//   LineView { text, color, bold }
// Events: add_stat(index), quest, pet, master, close
class CharacterWindowDataModel
{
public:
    struct LineView
    {
        ::Rml::String text;
        ::Rml::String color;
        bool bold = false;

        bool operator==(const LineView&) const = default;
    };

    struct StatView
    {
        ::Rml::String name;
        ::Rml::String value;
        ::Rml::String color;
        int index = 0;
        bool canAdd = false;
        std::vector<LineView> details;

        bool operator==(const StatView&) const = default;
    };

    bool Bind(::Rml::Context& context);
    // Re-captures the legacy window's texts; call each frame while the window is shown.
    void Refresh();

private:
    void RegisterTypes(::Rml::DataModelConstructor& constructor);
    void BindEvents(::Rml::DataModelConstructor& constructor);
    void Assign(const TextCapture::Line& line, int relativeX, int relativeY);

    ::Rml::DataModelHandle m_handle;
    TextCapture m_capture;

    // Bound values.
    ::Rml::String m_name;
    ::Rml::String m_nameColor;
    ::Rml::String m_className;
    ::Rml::String m_serverName;
    ::Rml::String m_levelText;
    ::Rml::String m_pointsText;
    bool m_hasPoints = false;
    bool m_canMaster = false;
    std::vector<LineView> m_table;
    std::vector<StatView> m_stats;

    // Built each refresh, then compared with the bound values.
    ::Rml::String m_nextName;
    ::Rml::String m_nextNameColor;
    ::Rml::String m_nextClassName;
    ::Rml::String m_nextServerName;
    ::Rml::String m_nextLevelText;
    ::Rml::String m_nextPointsText;
    std::vector<LineView> m_nextTable;
    std::vector<StatView> m_nextStats;
};
} // namespace UI::Modern

#endif // MU_RMLUI
