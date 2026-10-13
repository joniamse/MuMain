#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/SkillBarDataModel.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "GameLogic/Skills/SkillAvailability.h"
#include "UI/Modern/RmlFormat.h"
#include "UI/Modern/RmlRenderInterface.h"
#include "UI/NewUI/HUD/Skills/SkillIcon.h"
#include "UI/NewUI/HUD/Skills/SkillTooltipModel.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"

#include <array>
#include <charconv>

namespace UI::Modern
{
namespace
{
constexpr int kHotkeyCount = 10;
constexpr double kFullPercent = 100.0;
constexpr int kCooldownDecimals = 1;
constexpr std::size_t kNumberBufferSize = 16;

// Bar order is 1..9 then 0, like the keyboard row.
constexpr std::array<int, kHotkeyCount> kBarOrder = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};

bool IsPetCommand(int skillIndex)
{
    return skillIndex >= AT_PET_COMMAND_DEFAULT && skillIndex < AT_PET_COMMAND_END;
}

// Hero skill slot (or pet command) -> skill type; 0 when the slot shows nothing.
ActionSkillType SkillTypeAt(int skillIndex)
{
    if (skillIndex < 0)
    {
        return static_cast<ActionSkillType>(0);
    }
    if (IsPetCommand(skillIndex))
    {
        return Hero->m_pPet != nullptr ? static_cast<ActionSkillType>(skillIndex) : static_cast<ActionSkillType>(0);
    }
    if (skillIndex >= MAX_MAGIC)
    {
        return static_cast<ActionSkillType>(0);
    }
    return CharacterAttribute->Skill[skillIndex];
}

void AppendNumber(::Rml::String& target, int value)
{
    std::array<char, kNumberBufferSize> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    target.append(buffer.data(), result.ptr);
}

void FillView(int skillIndex, SkillBarDataModel::SkillView& view)
{
    view.index = skillIndex;
    const ActionSkillType skill = SkillTypeAt(skillIndex);
    view.filled = skill != 0;
    view.selected = view.filled && Hero->CurrentSkill == skillIndex;
    if (!view.filled)
    {
        view.icon.clear();
        view.rect.clear();
        view.cooling = false;
        view.usable = true;
        return;
    }

    view.usable = GameLogic::Skills::Availability::CanUseNow(skill);
    const Skills::Icon::Source source = Skills::Icon::Lookup(skill, view.usable);
    view.icon.assign(kLegacyTextureScheme);
    AppendNumber(view.icon, source.bitmap);
    view.rect.clear();
    AppendNumber(view.rect, static_cast<int>(source.x));
    view.rect.push_back(' ');
    AppendNumber(view.rect, static_cast<int>(source.y));
    view.rect.push_back(' ');
    AppendNumber(view.rect, Skills::Icon::kCellWidth);
    view.rect.push_back(' ');
    AppendNumber(view.rect, Skills::Icon::kCellHeight);

    const bool hasCooldown = !IsPetCommand(skillIndex) && Skills::Icon::ShowsCooldown(skill, view.usable);
    const float ratio = hasCooldown ? Skills::Icon::CooldownRatio(skillIndex) : 0.0f;
    view.cooling = ratio > 0.0f;
    Format::CssPercent(ratio * kFullPercent, kCooldownDecimals, view.cooldown);
}

const char* ColorClass(Skills::Tooltip::LineColor color)
{
    switch (color)
    {
    case Skills::Tooltip::LineColor::Blue:    return "blue";
    case Skills::Tooltip::LineColor::Red:     return "red";
    case Skills::Tooltip::LineColor::DarkRed: return "warning";
    default:                                  return "white";
    }
}

template <typename T>
void UpdateList(::Rml::DataModelHandle handle, const char* name, std::vector<T>& bound, std::vector<T>& fresh)
{
    if (bound == fresh)
    {
        return;
    }
    bound.swap(fresh);
    handle.DirtyVariable(name);
}
} // namespace

bool SkillBarDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("skills");
    if (!constructor)
    {
        return false;
    }

    RegisterTypes(constructor);
    constructor.Bind("slots", &m_slots);
    constructor.Bind("current", &m_current);
    constructor.Bind("picker", &m_picker);
    constructor.Bind("picker_open", &m_pickerOpen);
    constructor.Bind("tooltip", &m_tooltip);
    BindEvents(constructor);
    m_handle = constructor.GetModelHandle();
    return true;
}

void SkillBarDataModel::RegisterTypes(::Rml::DataModelConstructor& constructor)
{
    if (auto view = constructor.RegisterStruct<SkillView>())
    {
        view.RegisterMember("filled", &SkillView::filled);
        view.RegisterMember("index", &SkillView::index);
        view.RegisterMember("key", &SkillView::key);
        view.RegisterMember("icon", &SkillView::icon);
        view.RegisterMember("rect", &SkillView::rect);
        view.RegisterMember("cooldown", &SkillView::cooldown);
        view.RegisterMember("cooling", &SkillView::cooling);
        view.RegisterMember("usable", &SkillView::usable);
        view.RegisterMember("selected", &SkillView::selected);
    }
    constructor.RegisterArray<std::vector<SkillView>>();

    if (auto line = constructor.RegisterStruct<TooltipLineView>())
    {
        line.RegisterMember("text", &TooltipLineView::text);
        line.RegisterMember("color", &TooltipLineView::color);
        line.RegisterMember("bold", &TooltipLineView::bold);
        line.RegisterMember("blank", &TooltipLineView::blank);
    }
    constructor.RegisterArray<std::vector<TooltipLineView>>();
}

void SkillBarDataModel::BindEvents(::Rml::DataModelConstructor& constructor)
{
    constructor.BindEventCallback(
        "use_slot", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
            const int hotkey = arguments.empty() ? -1 : arguments[0].Get<int>();
            if (hotkey < 0 || hotkey >= kHotkeyCount)
            {
                return;
            }
            const int skillIndex = g_pSkillList->GetHotKey(hotkey);
            if (SkillTypeAt(skillIndex) != 0)
            {
                g_pSkillList->SelectCurrentSkill(skillIndex);
            }
        });

    constructor.BindEventCallback("toggle_picker",
                                  [this](::Rml::DataModelHandle handle, ::Rml::Event&, const ::Rml::VariantList&) {
                                      m_pickerOpen = !m_pickerOpen;
                                      handle.DirtyVariable("picker_open");
                                      PlayBuffer(SOUND_CLICK01);
                                  });

    constructor.BindEventCallback(
        "pick", [this](::Rml::DataModelHandle handle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
            const int skillIndex = arguments.empty() ? -1 : arguments[0].Get<int>();
            if (SkillTypeAt(skillIndex) != 0)
            {
                g_pSkillList->SelectCurrentSkill(skillIndex);
            }
            m_pickerOpen = false;
            handle.DirtyVariable("picker_open");
        });

    constructor.BindEventCallback(
        "hover", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
            m_hoveredIndex = arguments.empty() ? -1 : arguments[0].Get<int>();
            m_hoveredInPicker = arguments.size() > 1 && arguments[1].Get<bool>();
        });

    constructor.BindEventCallback("unhover", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        m_hoveredIndex = -1;
        m_hoveredInPicker = false;
    });
}

void SkillBarDataModel::Refresh()
{
    if (!m_handle || Hero == nullptr || CharacterAttribute == nullptr || g_pSkillList == nullptr)
    {
        return;
    }
    RefreshSlots();
    if (m_pickerOpen)
    {
        RefreshPicker();
        AssignHoveredSkillToPressedHotkey();
    }
    if (m_hoveredIndex != m_tooltipIndex)
    {
        RebuildTooltip();
    }
}

void SkillBarDataModel::RefreshSlots()
{
    m_scratch.resize(kHotkeyCount);
    for (std::size_t position = 0; position < kBarOrder.size(); ++position)
    {
        const int hotkey = kBarOrder[position];
        SkillView& view = m_scratch[position];
        FillView(g_pSkillList->GetHotKey(hotkey), view);
        view.key.assign(1, static_cast<char>('0' + hotkey));
    }
    UpdateList(m_handle, "slots", m_slots, m_scratch);

    SkillView current = m_current;
    FillView(Hero->CurrentSkill, current);
    if (!(current == m_current))
    {
        m_current = current;
        m_handle.DirtyVariable("current");
    }
}

void SkillBarDataModel::RefreshPicker()
{
    m_scratch.clear();
    for (int skillIndex = 0; skillIndex < MAX_MAGIC; ++skillIndex)
    {
        if (GameLogic::Skills::Availability::IsListed(CharacterAttribute->Skill[skillIndex]))
        {
            FillView(skillIndex, m_scratch.emplace_back());
        }
    }
    if (Hero->m_pPet != nullptr)
    {
        for (int command = AT_PET_COMMAND_DEFAULT; command < AT_PET_COMMAND_END; ++command)
        {
            FillView(command, m_scratch.emplace_back());
        }
    }
    UpdateList(m_handle, "picker", m_picker, m_scratch);
}

// Same shortcut as the original skill list: hover a skill, hold Ctrl and press a number.
void SkillBarDataModel::AssignHoveredSkillToPressedHotkey()
{
    if (!m_hoveredInPicker || m_hoveredIndex < 0 || !SEASON3B::IsRepeat(VK_CONTROL))
    {
        return;
    }
    for (int digit = 0; digit < kHotkeyCount; ++digit)
    {
        if (SEASON3B::IsPress('0' + digit))
        {
            g_pSkillList->SetHotKey(digit, m_hoveredIndex);
            return;
        }
    }
}

void SkillBarDataModel::RebuildTooltip()
{
    m_tooltipIndex = m_hoveredIndex;
    m_tooltip.clear();

    const ActionSkillType skill = SkillTypeAt(m_hoveredIndex);
    if (skill != 0 && !IsPetCommand(m_hoveredIndex))
    {
        Skills::Tooltip::BuildOptions options{};
        options.skillType = skill;
        options.skillSlotIndex = m_hoveredIndex;
        options.includeCharacterSpecific = true;

        static Skills::Tooltip::Model model;
        Skills::Tooltip::BuildModel(options, model);
        for (int i = 0; i < model.count; ++i)
        {
            const Skills::Tooltip::Line& line = model.lines[i];
            m_tooltip.push_back({mu_wchar_to_utf8(line.text), ColorClass(line.color), line.isBold, line.isBlank});
        }
    }
    m_handle.DirtyVariable("tooltip");
}
} // namespace UI::Modern

#endif // MU_RMLUI
