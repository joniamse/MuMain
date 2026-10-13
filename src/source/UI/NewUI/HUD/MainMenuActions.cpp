#include "stdafx.h"

#include "UI/NewUI/HUD/MainMenuActions.h"

#include "I18N/All.h"
#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInterface.h"
#include "UI/NewUI/NewUISystem.h"
#include "World/MapInfra/MapManager.h"

#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
#include "GameShop/InGameShopSystem.h"
#endif

namespace UI::MainMenu
{
namespace
{
constexpr int kFriendMinimumLevel = 6;

void ToggleInventory()
{
    g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
}

void ToggleCharacter()
{
    g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER);
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
    {
        g_QuestMng.SendQuestIndexByEtcSelection();
    }
}

void ToggleFriends()
{
    if (gMapManager.InChaosCastle())
    {
        return;
    }
    if (CharacterAttribute->Level < kFriendMinimumLevel)
    {
        if (g_pSystemLogBox->CheckChatRedundancy(I18N::Game::YouMustBeAtLeastLevel6ToUseTheMyFriendFunction) == FALSE)
        {
            g_pSystemLogBox->AddText(I18N::Game::YouMustBeAtLeastLevel6ToUseTheMyFriendFunction,
                                     SEASON3B::TYPE_SYSTEM_MESSAGE);
        }
        return;
    }
    g_pNewUISystem->Toggle(SEASON3B::INTERFACE_FRIEND);
}

#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
// Returns false when the shop is closed server-side or its scripts could not be downloaded.
bool ToggleCashShop()
{
    if (g_pInGameShop->IsInGameShopOpen() == false)
    {
        return false;
    }

#ifdef KJH_MOD_SHOP_SCRIPT_DOWNLOAD
    if (g_InGameShopSystem->IsScriptDownload() && g_InGameShopSystem->ScriptDownload() == false)
    {
        return false;
    }
    if (g_InGameShopSystem->IsBannerDownload())
    {
        g_InGameShopSystem->BannerDownload();
    }
#endif // KJH_MOD_SHOP_SCRIPT_DOWNLOAD

    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP))
    {
        SocketClient->ToGameServer()->SendCashShopOpenState(1);
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_INGAMESHOP);
        return true;
    }

    if (g_InGameShopSystem->GetIsRequestShopOpenning() == false)
    {
        SocketClient->ToGameServer()->SendCashShopOpenState(0);
        g_InGameShopSystem->SetIsRequestShopOpenning(true);
#ifdef KJH_MOD_SHOP_SCRIPT_DOWNLOAD
        g_pMainFrame->SetBtnState(SEASON3B::MAINFRAME_BTN_PARTCHARGE, true);
#endif // KJH_MOD_SHOP_SCRIPT_DOWNLOAD
    }
    return true;
}
#endif // PBG_ADD_INGAMESHOP_UI_MAINFRAME

// While a window that locks the inventory is open (trade, storage, ...), only the
// inventory itself may be toggled.
bool IsAllowed(Action action)
{
    if (g_pNewUIHotKey->CanUpdateKeyEventRelatedMyInventory())
    {
        return action == Action::Inventory;
    }
    return g_pNewUIHotKey->CanUpdateKeyEvent();
}
} // namespace

bool Trigger(Action action)
{
    if (!IsAllowed(action))
    {
        return false;
    }

    switch (action)
    {
    case Action::Inventory:
        ToggleInventory();
        break;
    case Action::Character:
        ToggleCharacter();
        break;
    case Action::Friends:
        ToggleFriends();
        break;
    case Action::WindowMenu:
        g_pNewUISystem->Toggle(SEASON3B::INTERFACE_WINDOW_MENU);
        break;
    case Action::CashShop:
#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
        // The shop request is answered by the server; it plays no click sound.
        return ToggleCashShop();
#else
        return false;
#endif
    }

    PlayBuffer(SOUND_CLICK01);
    return true;
}

bool HasFriendAlert()
{
    return g_pFriendMenu->IsNewChatAlert() || g_pFriendMenu->IsNewMailAlert() || g_pLetterList->CheckNoReadLetter();
}

bool HasQuestAlert()
{
    if (g_QuestMng.IsQuestIndexByEtcListEmpty())
    {
        return false;
    }
    return !(g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_QUEST_PROGRESS_ETC) ||
             g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER));
}
} // namespace UI::MainMenu
