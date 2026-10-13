#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

namespace UI::Modern
{
// Data model "bar" for the bottom HUD: experience progress, menu actions and alerts.
//
// RML names:
//   exp_level, exp_master        current (master) level, whether master experience is shown
//   exp_width                    CSS width of the bar ("42.5%")
//   exp_percent                  "42.53%" for the tooltip
//   exp_current, exp_next        total experience and target, digit-grouped
//   friend_alert, quest_alert    the friend/character buttons should draw attention
// Events:
//   menu('character' | 'inventory' | 'friends' | 'menu' | 'shop')
class BottomBarDataModel
{
public:
    bool Bind(::Rml::Context& context);
    void Refresh();

private:
    void RefreshExperience();
    void RefreshAlerts();
    template <typename T>
    void UpdateValue(const char* name, T& field, const T& value);

    ::Rml::DataModelHandle m_handle;
    int m_expLevel = 0;
    bool m_expMaster = false;
    ::Rml::String m_expWidth = "0%";
    ::Rml::String m_expPercent = "0%";
    ::Rml::String m_expCurrent;
    ::Rml::String m_expNext;
    bool m_friendAlert = false;
    bool m_questAlert = false;
    // Scratch buffers reused every frame.
    ::Rml::String m_scratch;
};
} // namespace UI::Modern

#endif // MU_RMLUI
