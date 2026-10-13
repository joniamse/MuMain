#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/PotionBarDataModel.h"

#include "Audio/DSPlaySound.h"
#include "UI/Modern/ItemIconAtlas.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUISystem.h"

#include <array>
#include <cstdint>

#include <SDL3/SDL_timer.h>

namespace UI::Modern
{
namespace
{
constexpr std::array kKeys = {"Q", "W", "E", "R"};
constexpr std::array kHotKeys = {SEASON3B::HOTKEY_Q, SEASON3B::HOTKEY_W, SEASON3B::HOTKEY_E, SEASON3B::HOTKEY_R};

// Dropping an item taken from the inventory on a slot assigns it to that hotkey (the original
// client only had Ctrl+Q/W/E/R over the item); the item goes back to its place.
// Returns true when an item was being carried, whether it could be assigned or not.
bool AssignCarriedItem(int hotKey)
{
    SEASON3B::CNewUIPickedItem* picked = g_pPickedItem;
    if (picked == nullptr || picked->GetItem() == nullptr)
    {
        return false;
    }

    const ITEM* item = picked->GetItem();
    const bool fromInventory = picked->GetOwnerInventory() == g_pMyInventory->GetInventoryCtrl();
    if (fromInventory && SEASON3B::CNewUIMyInventory::CanRegisterItemHotKey(item->Type))
    {
        g_pMainFrame->SetItemHotKey(hotKey, item->Type, item->Level);
        PlayBuffer(SOUND_CLICK01);
    }
    SEASON3B::CNewUIInventoryCtrl::BackupPickedItem();
    return true;
}

// When a release over a slot assigned a carried item: the click event which follows the same
// release (if the press was on the slot too) must not use the item.
std::uint64_t g_assignedAtTicks = 0;
constexpr std::uint64_t kSameReleaseMs = 250;
} // namespace

bool PotionBarDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("potions");
    if (!constructor)
    {
        return false;
    }

    if (auto potion = constructor.RegisterStruct<PotionView>())
    {
        potion.RegisterMember("hotkey", &PotionView::hotkey);
        potion.RegisterMember("key", &PotionView::key);
        potion.RegisterMember("filled", &PotionView::filled);
        potion.RegisterMember("icon", &PotionView::icon);
        potion.RegisterMember("rect", &PotionView::rect);
        potion.RegisterMember("count", &PotionView::count);
        potion.RegisterMember("has_count", &PotionView::hasCount);
    }
    constructor.RegisterArray<std::vector<PotionView>>();
    constructor.Bind("slots", &m_slots);

    constructor.BindEventCallback("use", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
        if (arguments.empty() || g_pMainFrame == nullptr || g_pMyInventory == nullptr)
        {
            return;
        }
        const int hotKey = arguments[0].Get<int>();
        const bool sameRelease = SDL_GetTicks() - g_assignedAtTicks < kSameReleaseMs;
        if (sameRelease || AssignCarriedItem(hotKey))
        {
            return;
        }
        if (g_pMainFrame->GetItemHotKeys().UseItem(hotKey))
        {
            PlayBuffer(SOUND_CLICK01);
        }
    });
    // A drag from the inventory ends with a release over the slot, without a click on it.
    constructor.BindEventCallback("drop", [](::Rml::DataModelHandle, ::Rml::Event&, const ::Rml::VariantList& arguments) {
        if (!arguments.empty() && g_pMainFrame != nullptr && g_pMyInventory != nullptr)
        {
            if (AssignCarriedItem(arguments[0].Get<int>()))
            {
                g_assignedAtTicks = SDL_GetTicks();
            }
        }
    });
    m_handle = constructor.GetModelHandle();
    return true;
}

void PotionBarDataModel::Refresh()
{
    if (!m_handle || g_pMainFrame == nullptr || g_pMyInventory == nullptr)
    {
        return;
    }

    auto& hotKeys = g_pMainFrame->GetItemHotKeys();
    m_scratch.clear();
    for (std::size_t i = 0; i < kKeys.size(); ++i)
    {
        PotionView view;
        view.hotkey = kHotKeys[i];
        view.key = kKeys[i];

        const int inventoryIndex = hotKeys.GetItemInventoryIndex(kHotKeys[i]);
        const ITEM* item = inventoryIndex != -1 ? g_pMyInventory->FindItem(inventoryIndex) : nullptr;
        ItemIcons::Icon icon;
        if (item != nullptr
            && ItemIcons::Find({item->Type, item->Level, item->ExcellentFlags, item->AncientDiscriminator}, icon))
        {
            view.filled = true;
            view.icon = std::move(icon.source);
            view.rect = std::move(icon.rect);
        }

        const int count = item != nullptr ? hotKeys.GetItemCount(kHotKeys[i]) : 0;
        view.hasCount = count > 0;
        view.count = view.hasCount ? std::to_string(count) : ::Rml::String();
        m_scratch.push_back(std::move(view));
    }

    if (m_scratch != m_slots)
    {
        std::swap(m_scratch, m_slots);
        m_handle.DirtyVariable("slots");
    }
}
} // namespace UI::Modern

#endif // MU_RMLUI
