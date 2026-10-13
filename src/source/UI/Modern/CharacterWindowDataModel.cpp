#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/CharacterWindowDataModel.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/NewUI/Character/NewUICharacterInfoWindow.h"
#include "UI/NewUI/NewUISystem.h"

#include <array>

namespace UI::Modern
{
namespace
{
using LegacyWindow = SEASON3B::CNewUICharacterInfoWindow;

// Layout of the legacy window (relative to its position) that tells the captured lines apart.
constexpr int kSubjectNameY = 12;
constexpr int kSubjectClassY = 27;
constexpr int kTableTop = 48;
constexpr int kTableLevelY = 58;
constexpr int kTablePointsX = 110;
constexpr int kStatHeaderOffsetY = 6;
constexpr int kStatValueX = 86;
constexpr std::array kStatRows = {LegacyWindow::HEIGHT_STRENGTH, LegacyWindow::HEIGHT_DEXTERITY,
                                  LegacyWindow::HEIGHT_VITALITY, LegacyWindow::HEIGHT_ENERGY,
                                  LegacyWindow::HEIGHT_CHARISMA};

::Rml::String HexColor(const TextCapture::Line& line)
{
    constexpr char kDigits[] = "0123456789abcdef";
    ::Rml::String color = "#000000";
    const BYTE channels[3] = {line.red, line.green, line.blue};
    for (int i = 0; i < 3; ++i)
    {
        color[1 + i * 2] = kDigits[channels[i] >> 4];
        color[2 + i * 2] = kDigits[channels[i] & 0x0F];
    }
    return color;
}

CharacterWindowDataModel::LineView ToView(const TextCapture::Line& line)
{
    return {mu_wchar_to_utf8(line.text.c_str()), HexColor(line), line.bold};
}

// Index of the stat block a row belongs to, or -1 above the first block.
int StatBlockAt(int relativeY)
{
    int block = -1;
    for (std::size_t i = 0; i < kStatRows.size(); ++i)
    {
        if (relativeY >= kStatRows[i])
        {
            block = static_cast<int>(i);
        }
    }
    return block;
}

template <typename T>
void PublishValue(::Rml::DataModelHandle handle, const char* name, T& bound, T& next)
{
    if (bound == next)
    {
        return;
    }
    std::swap(bound, next);
    handle.DirtyVariable(name);
}
} // namespace

bool CharacterWindowDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("character");
    if (!constructor)
    {
        return false;
    }
    RegisterTypes(constructor);
    constructor.Bind("name", &m_name);
    constructor.Bind("name_color", &m_nameColor);
    constructor.Bind("class_name", &m_className);
    constructor.Bind("server_name", &m_serverName);
    constructor.Bind("level_text", &m_levelText);
    constructor.Bind("points_text", &m_pointsText);
    constructor.Bind("has_points", &m_hasPoints);
    constructor.Bind("can_master", &m_canMaster);
    constructor.Bind("table", &m_table);
    constructor.Bind("stats", &m_stats);
    BindEvents(constructor);
    m_handle = constructor.GetModelHandle();
    return true;
}

void CharacterWindowDataModel::RegisterTypes(::Rml::DataModelConstructor& constructor)
{
    if (auto line = constructor.RegisterStruct<LineView>())
    {
        line.RegisterMember("text", &LineView::text);
        line.RegisterMember("color", &LineView::color);
        line.RegisterMember("bold", &LineView::bold);
    }
    constructor.RegisterArray<std::vector<LineView>>();

    if (auto stat = constructor.RegisterStruct<StatView>())
    {
        stat.RegisterMember("name", &StatView::name);
        stat.RegisterMember("value", &StatView::value);
        stat.RegisterMember("color", &StatView::color);
        stat.RegisterMember("index", &StatView::index);
        stat.RegisterMember("can_add", &StatView::canAdd);
        stat.RegisterMember("details", &StatView::details);
    }
    constructor.RegisterArray<std::vector<StatView>>();
}

void CharacterWindowDataModel::BindEvents(::Rml::DataModelConstructor& constructor)
{
    constructor.BindEventCallback(
        "add_stat", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
            if (!arguments.empty())
            {
                g_pCharacterInfoWindow->IncreaseStat(arguments[0].Get<int>());
            }
        });
    constructor.BindEventCallback("quest", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        g_pCharacterInfoWindow->ToggleQuestWindow();
        PlayBuffer(SOUND_CLICK01);
    });
    constructor.BindEventCallback("pet", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        g_pCharacterInfoWindow->TogglePetWindow();
        PlayBuffer(SOUND_CLICK01);
    });
    constructor.BindEventCallback("master", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        g_pCharacterInfoWindow->ToggleMasterLevelWindow();
        PlayBuffer(SOUND_CLICK01);
    });
    constructor.BindEventCallback("close", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_CHARACTER);
        PlayBuffer(SOUND_CLICK01);
    });
}

void CharacterWindowDataModel::Refresh()
{
    LegacyWindow* window = g_pCharacterInfoWindow;
    if (!m_handle || !window || Hero == nullptr || CharacterAttribute == nullptr)
    {
        return;
    }

    m_capture.Clear();
    window->CaptureTexts(m_capture);

    m_nextName.clear();
    m_nextNameColor.clear();
    m_nextClassName.clear();
    m_nextServerName.clear();
    m_nextLevelText.clear();
    m_nextPointsText.clear();
    m_nextTable.clear();
    m_nextStats.clear();

    const POINT origin = window->GetPos();
    for (const TextCapture::Line& line : m_capture.Lines())
    {
        Assign(line, line.x - origin.x, line.y - origin.y);
    }

    const bool hasPoints = CharacterAttribute->LevelUpPoint > 0;
    const int addableStats = window->StatButtonCount();
    for (StatView& stat : m_nextStats)
    {
        stat.canAdd = hasPoints && stat.index < addableStats;
    }

    PublishValue(m_handle, "name", m_name, m_nextName);
    PublishValue(m_handle, "name_color", m_nameColor, m_nextNameColor);
    PublishValue(m_handle, "class_name", m_className, m_nextClassName);
    PublishValue(m_handle, "server_name", m_serverName, m_nextServerName);
    PublishValue(m_handle, "level_text", m_levelText, m_nextLevelText);
    PublishValue(m_handle, "points_text", m_pointsText, m_nextPointsText);
    PublishValue(m_handle, "table", m_table, m_nextTable);
    PublishValue(m_handle, "stats", m_stats, m_nextStats);

    bool nextHasPoints = hasPoints;
    bool canMaster = window->CanOpenMasterLevelWindow();
    PublishValue(m_handle, "has_points", m_hasPoints, nextHasPoints);
    PublishValue(m_handle, "can_master", m_canMaster, canMaster);
}

void CharacterWindowDataModel::Assign(const TextCapture::Line& line, int relativeX, int relativeY)
{
    if (relativeY < kTableTop)
    {
        if (relativeY == kSubjectNameY)
        {
            m_nextName = mu_wchar_to_utf8(line.text.c_str());
            m_nextNameColor = HexColor(line);
        }
        else if (relativeY == kSubjectClassY)
        {
            // Class first, then the server name; the legacy window cross-fades the two.
            (m_nextClassName.empty() ? m_nextClassName : m_nextServerName) = mu_wchar_to_utf8(line.text.c_str());
        }
        return;
    }

    const int block = StatBlockAt(relativeY);
    if (block < 0)
    {
        if (relativeY == kTableLevelY)
        {
            (relativeX >= kTablePointsX ? m_nextPointsText : m_nextLevelText) = mu_wchar_to_utf8(line.text.c_str());
        }
        else
        {
            m_nextTable.push_back(ToView(line));
        }
        return;
    }

    if (m_nextStats.empty() || m_nextStats.back().index != block)
    {
        StatView stat;
        stat.index = block;
        m_nextStats.push_back(std::move(stat));
    }
    StatView& stat = m_nextStats.back();
    if (relativeY == kStatRows[static_cast<std::size_t>(block)] + kStatHeaderOffsetY)
    {
        if (relativeX >= kStatValueX)
        {
            stat.value = mu_wchar_to_utf8(line.text.c_str());
            stat.color = HexColor(line);
        }
        else
        {
            stat.name = mu_wchar_to_utf8(line.text.c_str());
        }
        return;
    }
    stat.details.push_back(ToView(line));
}
} // namespace UI::Modern

#endif // MU_RMLUI
