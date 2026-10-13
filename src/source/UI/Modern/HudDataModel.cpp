#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/HudDataModel.h"

#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInterface.h"
#include "Network/Server/WSclient.h"
#include "UI/Modern/RmlFormat.h"

#include "UI/Modern/RmlIncludes.h"

#include <algorithm>

namespace UI::Modern
{
namespace
{
constexpr float kFullPercent = 100.0f;
constexpr int kPercentDecimals = 1;

struct Vitals
{
    int life;
    int lifeMax;
    int mana;
    int manaMax;
    int shield;
    int shieldMax;
    int ability;
    int abilityMax;
};

// Master-level characters take their maxima from the master level data, like the legacy HUD.
Vitals ReadHeroVitals()
{
    const bool isMaster = gCharacterManager.IsMasterLevel(Hero->Class);
    Vitals vitals{};
    vitals.lifeMax = isMaster ? Master_Level_Data.wMaxLife : CharacterAttribute->LifeMax;
    vitals.manaMax = isMaster ? Master_Level_Data.wMaxMana : CharacterAttribute->ManaMax;
    vitals.shieldMax = isMaster ? Master_Level_Data.wMaxShield : CharacterAttribute->ShieldMax;
    vitals.abilityMax = isMaster ? Master_Level_Data.wMaxBP : CharacterAttribute->SkillManaMax;
    vitals.life = static_cast<int>(CharacterAttribute->Life);
    vitals.mana = static_cast<int>(CharacterAttribute->Mana);
    vitals.shield = static_cast<int>(CharacterAttribute->Shield);
    vitals.ability = static_cast<int>(CharacterAttribute->SkillMana);
    return vitals;
}
} // namespace

bool HudDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("hud");
    if (!constructor)
    {
        return false;
    }

    constructor.Bind("name", &m_name);
    constructor.Bind("compact", &m_compact);
    constructor.BindEventCallback("toggle_compact",
                                  [this](::Rml::DataModelHandle handle, ::Rml::Event&, const ::Rml::VariantList&) {
                                      m_compact = !m_compact;
                                      handle.DirtyVariable("compact");
                                  });
    constructor.Bind("level", &m_level);
    constructor.Bind("master_level", &m_masterLevel);
    BindGauge(constructor, "life", m_life);
    BindGauge(constructor, "mana", m_mana);
    BindGauge(constructor, "shield", m_shield);
    BindGauge(constructor, "ability", m_ability);
    m_handle = constructor.GetModelHandle();
    return true;
}

void HudDataModel::BindGauge(::Rml::DataModelConstructor& constructor, const ::Rml::String& name, Gauge& gauge)
{
    gauge.currentName = name;
    gauge.maximumName = name + "_max";
    gauge.percentName = name + "_pct";
    gauge.widthName = name + "_width";
    constructor.Bind(gauge.currentName, &gauge.current);
    constructor.Bind(gauge.maximumName, &gauge.maximum);
    constructor.Bind(gauge.percentName, &gauge.percent);
    constructor.Bind(gauge.widthName, &gauge.width);
}

void HudDataModel::Refresh()
{
    if (!m_handle || Hero == nullptr || CharacterAttribute == nullptr)
    {
        return;
    }

    RefreshName();
    UpdateValue("level", m_level, static_cast<int>(CharacterAttribute->Level));
    UpdateValue("master_level", m_masterLevel, static_cast<int>(Master_Level_Data.nMLevel));

    const Vitals vitals = ReadHeroVitals();
    UpdateGauge(m_life, vitals.life, vitals.lifeMax);
    UpdateGauge(m_mana, vitals.mana, vitals.manaMax);
    UpdateGauge(m_shield, vitals.shield, vitals.shieldMax);
    UpdateGauge(m_ability, vitals.ability, vitals.abilityMax);
}

void HudDataModel::RefreshName()
{
    if (m_heroId == Hero->ID)
    {
        return;
    }
    m_heroId = Hero->ID;
    m_name = mu_wchar_to_utf8(Hero->ID);
    m_handle.DirtyVariable("name");
}

void HudDataModel::UpdateGauge(Gauge& gauge, int current, int maximum)
{
    const int clampedMaximum = std::max(maximum, 0);
    const int clampedCurrent = std::clamp(current, 0, clampedMaximum);
    const float percent =
        clampedMaximum > 0 ? kFullPercent * static_cast<float>(clampedCurrent) / static_cast<float>(clampedMaximum)
                           : 0.0f;
    UpdateValue(gauge.currentName.c_str(), gauge.current, clampedCurrent);
    UpdateValue(gauge.maximumName.c_str(), gauge.maximum, clampedMaximum);
    UpdateValue(gauge.percentName.c_str(), gauge.percent, static_cast<int>(percent));
    Format::CssPercent(percent, kPercentDecimals, m_widthBuffer);
    UpdateValue(gauge.widthName.c_str(), gauge.width, m_widthBuffer);
}

template <typename T>
void HudDataModel::UpdateValue(const char* name, T& field, const T& value)
{
    if (field == value)
    {
        return;
    }
    field = value;
    m_handle.DirtyVariable(name);
}
} // namespace UI::Modern

#endif // MU_RMLUI
