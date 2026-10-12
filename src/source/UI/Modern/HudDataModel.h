#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

#include <string>

namespace Rml
{
class Context;
}

namespace UI::Modern
{
// Exposes the hero's vitals to RML documents as the data model "hud".
// Values are refreshed once per frame; only changed ones are marked dirty.
class HudDataModel
{
public:
    bool Bind(::Rml::Context& context);
    void Refresh();

private:
    // RML names: <name>, <name>_max and <name>_width (a CSS percentage such as "42.5%").
    // The width is formatted here: RmlUi's float-to-string follows the process locale,
    // which writes "42,5" on Spanish/German Windows and yields invalid CSS.
    struct Gauge
    {
        ::Rml::String currentName;
        ::Rml::String maximumName;
        ::Rml::String widthName;
        int current = 0;
        int maximum = 0;
        ::Rml::String width = "0%";
    };

    void BindGauge(::Rml::DataModelConstructor& constructor, const ::Rml::String& name, Gauge& gauge);
    void UpdateGauge(Gauge& gauge, int current, int maximum);
    template <typename T>
    void UpdateValue(const char* name, T& field, const T& value);

    void RefreshName();

    ::Rml::DataModelHandle m_handle;
    std::wstring m_heroId;
    ::Rml::String m_widthBuffer;
    ::Rml::String m_name;
    int m_level = 0;
    int m_masterLevel = 0;
    Gauge m_life;
    Gauge m_mana;
    Gauge m_shield;
    Gauge m_ability;
};
} // namespace UI::Modern

#endif // MU_RMLUI
