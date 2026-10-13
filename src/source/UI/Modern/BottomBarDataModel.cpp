#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/BottomBarDataModel.h"

#include "Audio/DSPlaySound.h"
#include "Character/ExperienceProgress.h"
#include "UI/Modern/ModernHud.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/Modern/RmlFormat.h"
#include "UI/NewUI/HUD/MainMenuActions.h"

#include <array>
#include <string_view>
#include <utility>

namespace UI::Modern
{
namespace
{
constexpr double kFullPercent = 100.0;
constexpr int kWidthDecimals = 2;
constexpr int kTooltipDecimals = 2;

constexpr std::array<std::pair<std::string_view, MainMenu::Action>, 5> kMenuActions = {{
    {"character", MainMenu::Action::Character},
    {"inventory", MainMenu::Action::Inventory},
    {"friends", MainMenu::Action::Friends},
    {"menu", MainMenu::Action::WindowMenu},
    {"shop", MainMenu::Action::CashShop},
}};

void OnMenuEvent(::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments)
{
    if (arguments.empty())
    {
        return;
    }
    const ::Rml::String name = arguments[0].Get<::Rml::String>();
    if (name == "settings")
    {
        PlayBuffer(SOUND_CLICK01);
        OpenSettings();
        return;
    }
    for (const auto& [actionName, action] : kMenuActions)
    {
        if (actionName == name)
        {
            MainMenu::Trigger(action);
            return;
        }
    }
    mu::log::Get("rmlui")->warn("unknown menu action '{}'", name);
}
} // namespace

bool BottomBarDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("bar");
    if (!constructor)
    {
        return false;
    }

    constructor.Bind("exp_level", &m_expLevel);
    constructor.Bind("exp_master", &m_expMaster);
    constructor.Bind("exp_width", &m_expWidth);
    constructor.Bind("exp_percent", &m_expPercent);
    constructor.Bind("exp_current", &m_expCurrent);
    constructor.Bind("exp_next", &m_expNext);
    constructor.Bind("friend_alert", &m_friendAlert);
    constructor.Bind("quest_alert", &m_questAlert);
    constructor.BindEventCallback("menu", &OnMenuEvent);
    m_handle = constructor.GetModelHandle();
    return true;
}

void BottomBarDataModel::Refresh()
{
    if (!m_handle || Hero == nullptr || CharacterAttribute == nullptr)
    {
        return;
    }
    RefreshExperience();
    RefreshAlerts();
}

void BottomBarDataModel::RefreshExperience()
{
    const Character::Experience::Progress progress = Character::Experience::HeroProgress();
    UpdateValue("exp_level", m_expLevel, progress.level);
    UpdateValue("exp_master", m_expMaster, progress.isMaster);

    const double percent = progress.ratio * kFullPercent;
    Format::CssPercent(percent, kWidthDecimals, m_scratch);
    UpdateValue("exp_width", m_expWidth, m_scratch);
    Format::CssPercent(percent, kTooltipDecimals, m_scratch);
    UpdateValue("exp_percent", m_expPercent, m_scratch);
    Format::Grouped(progress.experience, m_scratch);
    UpdateValue("exp_current", m_expCurrent, m_scratch);
    Format::Grouped(progress.levelEnd, m_scratch);
    UpdateValue("exp_next", m_expNext, m_scratch);
}

void BottomBarDataModel::RefreshAlerts()
{
    UpdateValue("friend_alert", m_friendAlert, MainMenu::HasFriendAlert());
    UpdateValue("quest_alert", m_questAlert, MainMenu::HasQuestAlert());
}

template <typename T>
void BottomBarDataModel::UpdateValue(const char* name, T& field, const T& value)
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
