#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/VipWindowDataModel.h"

#include "Audio/DSPlaySound.h"
#include "Core/Text/Utf8.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"
#include "UI/Modern/RmlFormat.h"

#include <algorithm>
#include <chrono>
#include <cwchar>
#include <string>

namespace UI::Modern
{
namespace
{
std::vector<std::wstring_view> Split(std::wstring_view text)
{
    std::vector<std::wstring_view> fields;
    std::size_t start = 0;
    while (true)
    {
        const std::size_t end = text.find(L'|', start);
        fields.push_back(text.substr(start, end == std::wstring_view::npos ? std::wstring_view::npos : end - start));
        if (end == std::wstring_view::npos)
        {
            return fields;
        }
        start = end + 1;
    }
}

std::int64_t Number(const std::vector<std::wstring_view>& fields, std::size_t index)
{
    if (index >= fields.size())
    {
        return 0;
    }
    const std::wstring value(fields[index]);
    return std::wcstoll(value.c_str(), nullptr, 10);
}

::Rml::String Text(const std::vector<std::wstring_view>& fields, std::size_t index)
{
    if (index >= fields.size())
    {
        return {};
    }
    const std::wstring value(fields[index]);
    return Core::Text::ToUtf8(value.c_str());
}

::Rml::String Grouped(std::int64_t value)
{
    ::Rml::String text;
    Format::Grouped(value, text);
    return text;
}

::Rml::String Tier(int level)
{
    return "tier-" + std::to_string(level < 0 || level > 4 ? 0 : level);
}

::Rml::String ScopeText(std::wstring_view code)
{
    return code == L"A" ? "Toda la cuenta" : "Un personaje";
}

std::int64_t NowUnix()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Sends a chat line as the chat box does; the server's chat commands answer it.
void SendCommand(const std::string& command)
{
    if (Hero == nullptr || SocketClient == nullptr)
    {
        return;
    }
    const std::wstring wide = Core::Text::FromUtf8(command);
    SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(Hero->ID), MU_C16(wide.c_str()));
}
} // namespace

bool VipWindowDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("vip");
    if (!constructor)
    {
        return false;
    }

    if (auto level = constructor.RegisterStruct<LevelView>())
    {
        level.RegisterMember("level", &LevelView::level);
        level.RegisterMember("name", &LevelView::name);
        level.RegisterMember("cost", &LevelView::cost);
        level.RegisterMember("scope_text", &LevelView::scopeText);
        level.RegisterMember("exp", &LevelView::exp);
        level.RegisterMember("drop", &LevelView::drop);
        level.RegisterMember("chaos", &LevelView::chaos);
        level.RegisterMember("vaults", &LevelView::vaults);
        level.RegisterMember("offstore", &LevelView::offstore);
        level.RegisterMember("helper", &LevelView::helper);
        level.RegisterMember("entries", &LevelView::entries);
        level.RegisterMember("tier", &LevelView::tier);
        level.RegisterMember("current", &LevelView::current);
        level.RegisterMember("affordable", &LevelView::affordable);
        level.RegisterMember("pending", &LevelView::pending);
        level.RegisterMember("action", &LevelView::action);
    }
    constructor.RegisterArray<std::vector<LevelView>>();

    if (auto package = constructor.RegisterStruct<PackageView>())
    {
        package.RegisterMember("id", &PackageView::id);
        package.RegisterMember("name", &PackageView::name);
        package.RegisterMember("price", &PackageView::price);
        package.RegisterMember("base", &PackageView::base);
        package.RegisterMember("bonus", &PackageView::bonus);
        package.RegisterMember("total", &PackageView::total);
        package.RegisterMember("has_bonus", &PackageView::hasBonus);
    }
    constructor.RegisterArray<std::vector<PackageView>>();

    constructor.BindFunc("balance", [this](::Rml::Variant& value) { value = Grouped(m_balance); });
    constructor.Bind("level", &m_level);
    constructor.Bind("level_name", &m_levelName);
    constructor.Bind("tier", &m_tier);
    constructor.Bind("scope_text", &m_scopeText);
    constructor.Bind("time_left", &m_timeLeft);
    constructor.Bind("expires_text", &m_expiresText);
    constructor.Bind("character", &m_character);
    constructor.Bind("active_vault", &m_activeVault);
    constructor.Bind("recharge_enabled", &m_rechargeEnabled);
    constructor.Bind("notice", &m_notice);
    constructor.Bind("has_notice", &m_hasNotice);
    constructor.Bind("loading", &m_loading);
    constructor.Bind("has_offstore", &m_hasOffStore);
    constructor.Bind("offstore_title", &m_offStoreTitle);
    constructor.Bind("offstore_left", &m_offStoreLeft);
    constructor.Bind("offstore_sales", &m_offStoreSales);
    constructor.RegisterArray<std::vector<::Rml::String>>();
    constructor.Bind("offstore_notes", &m_offStoreNotes);
    constructor.Bind("levels", &m_levels);
    constructor.Bind("packages", &m_packages);

    constructor.BindEventCallback("buy", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
        if (!arguments.empty())
        {
            Buy(arguments[0].Get<int>());
        }
    });
    constructor.BindEventCallback(
        "recharge", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
            if (!arguments.empty())
            {
                Recharge(arguments[0].Get<int>());
            }
        });
    constructor.BindEventCallback("refresh", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        PlayBuffer(SOUND_CLICK01);
        Request();
    });
    constructor.BindEventCallback("close", [this](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList&) {
        m_closeRequested = true;
        m_pendingLevel = 0;
        PlayBuffer(SOUND_CLICK01);
    });
    m_handle = constructor.GetModelHandle();
    return true;
}

bool VipWindowDataModel::HandleNotice(std::wstring_view text)
{
    if (!text.starts_with(kPrefix))
    {
        return false;
    }

    const std::vector<std::wstring_view> fields = Split(text.substr(kPrefix.size()));
    const std::wstring_view kind = fields.front();
    if (kind == L"B")
    {
        Begin();
    }
    else if (!m_receiving)
    {
        // A part of a message whose beginning was missed: ignored.
    }
    else if (kind == L"S")
    {
        ReadStatus(fields);
    }
    else if (kind == L"L")
    {
        ReadLevel(fields);
    }
    else if (kind == L"P")
    {
        ReadPackage(fields);
    }
    else if (kind == L"O")
    {
        m_offStoreNotes.push_back(Text(fields, 1));
    }
    else if (kind == L"T")
    {
        ReadOffStore(fields);
    }
    else if (kind == L"M")
    {
        m_notice = Text(fields, 1);
        m_hasNotice = !m_notice.empty();
    }
    else if (kind == L"E")
    {
        Finish();
    }
    return true;
}

void VipWindowDataModel::Request()
{
    m_loading = true;
    if (m_handle)
    {
        m_handle.DirtyVariable("loading");
    }
    SendCommand("/vip ui");
}

void VipWindowDataModel::Tick()
{
    const std::int64_t now = NowUnix();
    const std::int64_t minute = now / 60;
    if (minute == m_shownMinute)
    {
        return;
    }
    m_shownMinute = minute;

    if (m_hasOffStore)
    {
        if (m_offStoreExpiresUnix < 0)
        {
            m_offStoreLeft = "Sin limite";
        }
        else
        {
            const std::int64_t left = std::max<std::int64_t>(0, m_offStoreExpiresUnix - now);
            m_offStoreLeft = left == 0 ? "Cerrandose"
                                       : std::to_string(left / 3600) + "h " + std::to_string(left % 3600 / 60) + "m";
        }
        if (m_handle)
        {
            m_handle.DirtyVariable("offstore_left");
        }
    }

    if (m_level == 0 || m_expiresUnix <= now)
    {
        m_timeLeft = m_level == 0 ? "Sin membresia" : "Vencida";
    }
    else
    {
        const std::int64_t left = m_expiresUnix - now;
        const std::int64_t days = left / 86400;
        const std::int64_t hours = left % 86400 / 3600;
        const std::int64_t minutes = left % 3600 / 60;
        m_timeLeft = days > 0 ? std::to_string(days) + "d " + std::to_string(hours) + "h"
                              : std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }
    if (m_handle)
    {
        m_handle.DirtyVariable("time_left");
    }
}

bool VipWindowDataModel::ConsumeOpenRequest()
{
    const bool requested = m_openRequested;
    m_openRequested = false;
    return requested;
}

bool VipWindowDataModel::ConsumeCloseRequest()
{
    const bool requested = m_closeRequested;
    m_closeRequested = false;
    return requested;
}

void VipWindowDataModel::Begin()
{
    m_receiving = true;
    m_levels.clear();
    m_packages.clear();
    m_notice.clear();
    m_hasNotice = false;
    m_hasOffStore = false;
    m_offStoreNotes.clear();
}

// S|balance|level|scope|expiresUnix|character|vault|recharge
void VipWindowDataModel::ReadStatus(const std::vector<std::wstring_view>& fields)
{
    m_balance = Number(fields, 1);
    m_level = static_cast<int>(Number(fields, 2));
    m_tier = Tier(m_level);
    m_scopeText = m_level == 0 ? "-" : ScopeText(fields.size() > 3 ? fields[3] : std::wstring_view{});
    m_expiresUnix = Number(fields, 4);
    m_character = Text(fields, 5);
    m_activeVault = static_cast<int>(Number(fields, 6));
    m_rechargeEnabled = Number(fields, 7) != 0;

    if (m_expiresUnix > 0)
    {
        const std::time_t expires = static_cast<std::time_t>(m_expiresUnix);
        std::tm utc{};
        gmtime_s(&utc, &expires);
        char buffer[32]{};
        std::strftime(buffer, sizeof buffer, "%d/%m/%Y %H:%M UTC", &utc);
        m_expiresText = buffer;
    }
    else
    {
        m_expiresText.clear();
    }
}

// L|level|name|cost|scope|exp%|drop%|chaos|vaults|offstoreHours|whilePlaying|helper%|entries
void VipWindowDataModel::ReadLevel(const std::vector<std::wstring_view>& fields)
{
    LevelView level;
    level.level = static_cast<int>(Number(fields, 1));
    level.name = Text(fields, 2);
    level.cost = static_cast<int>(Number(fields, 3));
    level.scopeText = ScopeText(fields.size() > 4 ? fields[4] : std::wstring_view{});
    level.exp = "+" + std::to_string(Number(fields, 5)) + "%";
    level.drop = "+" + std::to_string(Number(fields, 6)) + "%";
    level.chaos = "+" + std::to_string(Number(fields, 7)) + "%";
    level.vaults = static_cast<int>(Number(fields, 8));
    const std::int64_t offstoreHours = Number(fields, 9);
    level.offstore = offstoreHours < 0 ? "Ilimitada" : offstoreHours == 0 ? "-" : std::to_string(offstoreHours) + "h";
    if (Number(fields, 10) != 0)
    {
        level.offstore += " + jugar";
    }
    const std::int64_t helper = Number(fields, 11);
    level.helper = helper >= 100 ? "Gratis" : "-" + std::to_string(helper) + "%";
    level.entries = static_cast<int>(Number(fields, 12));
    level.tier = Tier(level.level);
    m_levels.push_back(std::move(level));
}

// P|id|name|priceUsd|base|bonus|total
void VipWindowDataModel::ReadPackage(const std::vector<std::wstring_view>& fields)
{
    PackageView package;
    package.id = static_cast<int>(Number(fields, 1));
    package.name = Text(fields, 2);
    package.price = "USD " + Text(fields, 3);
    package.base = static_cast<int>(Number(fields, 4));
    package.bonus = static_cast<int>(Number(fields, 5));
    package.total = static_cast<int>(Number(fields, 6));
    package.hasBonus = package.bonus > 0;
    m_packages.push_back(std::move(package));
}

// T|character|store|expiresUnix|soldItems|soldZen - the window shows the first one.
void VipWindowDataModel::ReadOffStore(const std::vector<std::wstring_view>& fields)
{
    if (m_hasOffStore)
    {
        return;
    }
    m_hasOffStore = true;
    m_offStoreTitle = "\"" + Text(fields, 2) + "\" de " + Text(fields, 1);
    m_offStoreExpiresUnix = Number(fields, 3);
    m_offStoreSales = std::to_string(Number(fields, 4)) + " vendidos, " + Grouped(Number(fields, 5)) + " zen";
}

void VipWindowDataModel::Finish()
{
    m_receiving = false;
    m_loading = false;
    m_pendingLevel = 0;
    m_levelName = "Free";
    for (const LevelView& level : m_levels)
    {
        if (level.level == m_level)
        {
            m_levelName = level.name;
        }
    }
    m_shownMinute = -1;
    UpdateLevelStates();
    Tick();
    DirtyAll();
    m_openRequested = true;
}

// The first click arms the button ("Confirmar"), the second one buys: a stray click spends nothing.
void VipWindowDataModel::Buy(int level)
{
    PlayBuffer(SOUND_CLICK01);
    if (m_pendingLevel != level)
    {
        m_pendingLevel = level;
        UpdateLevelStates();
        if (m_handle)
        {
            m_handle.DirtyVariable("levels");
        }
        return;
    }

    m_pendingLevel = 0;
    m_loading = true;
    if (m_handle)
    {
        m_handle.DirtyVariable("loading");
    }
    SendCommand("/vip comprar " + std::to_string(level) + " ui");
}

void VipWindowDataModel::Recharge(int packageId)
{
    PlayBuffer(SOUND_CLICK01);
    m_loading = true;
    if (m_handle)
    {
        m_handle.DirtyVariable("loading");
    }
    SendCommand("/recargar " + std::to_string(packageId) + " ui");
}

void VipWindowDataModel::UpdateLevelStates()
{
    for (LevelView& level : m_levels)
    {
        level.current = level.level == m_level && m_level > 0;
        level.affordable = m_balance >= level.cost;
        level.pending = level.level == m_pendingLevel;
        if (level.pending)
        {
            level.action = "Confirmar " + Grouped(level.cost);
        }
        else if (level.level < m_level)
        {
            level.action = "Incluido";
        }
        else
        {
            level.action = level.current ? "Renovar" : "Comprar";
        }
    }
}

void VipWindowDataModel::DirtyAll()
{
    if (!m_handle)
    {
        return;
    }
    m_handle.DirtyAllVariables();
}
} // namespace UI::Modern

#endif // MU_RMLUI
