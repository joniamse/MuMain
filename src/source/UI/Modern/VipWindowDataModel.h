#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace UI::Modern
{
// Data model "vip" for the VIP window (Data/UI/vip.rml): fichas balance, membership and the
// offers of the server.
//
// The server (MuCustom plugins of OpenMU) sends the data as notices that start with "#MUVIP|"
// (see VipUiProtocol.cs on the server): B (begin), S (status), L (level), P (package),
// M (notice), E (end). The buttons answer with chat commands, so no new packet is needed:
// "/vip ui" asks for the data, "/vip comprar N ui" buys, "/recargar N ui" recharges.
//
// RML names:
//   balance, level, level_name, tier ("tier-0".."tier-4"), scope_text, time_left, expires_text,
//   character, active_vault, recharge_enabled, notice, has_notice, loading
//   has_offstore, offstore_title, offstore_left, offstore_sales   (the first open offline store)
//   levels[] LevelView { level, name, cost, scope_text, exp, drop, chaos, vaults, offstore, helper,
//                        entries, tier, current, affordable, pending, action }
//   packages[] PackageView { id, name, price, base, bonus, total, has_bonus }
// Events: buy(level), recharge(id), refresh, close
class VipWindowDataModel
{
public:
    struct LevelView
    {
        int level = 0;
        ::Rml::String name;
        int cost = 0;
        ::Rml::String scopeText;
        ::Rml::String exp;
        ::Rml::String drop;
        ::Rml::String chaos;
        int vaults = 0;
        ::Rml::String offstore;
        ::Rml::String helper;
        int entries = 0;
        ::Rml::String tier;
        bool current = false;
        bool affordable = false;
        bool pending = false;
        ::Rml::String action;
    };

    struct PackageView
    {
        int id = 0;
        ::Rml::String name;
        ::Rml::String price;
        int base = 0;
        int bonus = 0;
        int total = 0;
        bool hasBonus = false;
    };

    // The prefix of the notices which carry the window data.
    static constexpr std::wstring_view kPrefix = L"#MUVIP|";

    bool Bind(::Rml::Context& context);

    // Takes one notice of the server. Returns true when it was VIP window data (which must not be
    // shown in the chat); sets the open request when the data is complete.
    bool HandleNotice(std::wstring_view text);

    // Asks the server for the data; the window opens when it arrives.
    void Request();

    // Updates the remaining time; call each frame while the window is shown.
    void Tick();

    // True once after the data arrived (the window should open).
    bool ConsumeOpenRequest();
    // True once after the window asked to be closed.
    bool ConsumeCloseRequest();

private:
    void Begin();
    void ReadStatus(const std::vector<std::wstring_view>& fields);
    void ReadLevel(const std::vector<std::wstring_view>& fields);
    void ReadPackage(const std::vector<std::wstring_view>& fields);
    void ReadOffStore(const std::vector<std::wstring_view>& fields);
    void Finish();
    void Buy(int level);
    void Recharge(int packageId);
    void UpdateLevelStates();
    void DirtyAll();

    ::Rml::DataModelHandle m_handle;

    long long m_balance = 0;
    int m_level = 0;
    ::Rml::String m_levelName = "Free";
    ::Rml::String m_tier = "tier-0";
    ::Rml::String m_scopeText;
    ::Rml::String m_timeLeft;
    ::Rml::String m_expiresText;
    ::Rml::String m_character;
    int m_activeVault = 1;
    bool m_rechargeEnabled = false;
    ::Rml::String m_notice;
    bool m_hasNotice = false;
    bool m_loading = false;
    std::vector<LevelView> m_levels;
    std::vector<PackageView> m_packages;
    bool m_hasOffStore = false;
    ::Rml::String m_offStoreTitle;
    ::Rml::String m_offStoreLeft;
    ::Rml::String m_offStoreSales;
    // Without an open offline store: the shop open right now, the last closed store (O lines).
    std::vector<::Rml::String> m_offStoreNotes;
    std::int64_t m_offStoreExpiresUnix = 0;

    std::int64_t m_expiresUnix = 0;
    std::int64_t m_shownMinute = -1;
    int m_pendingLevel = 0;
    bool m_receiving = false;
    bool m_openRequested = false;
    bool m_closeRequested = false;
};
} // namespace UI::Modern

#endif // MU_RMLUI
