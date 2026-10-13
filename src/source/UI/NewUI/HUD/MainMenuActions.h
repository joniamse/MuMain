#pragma once

// The main menu buttons of the bottom HUD (character, inventory, friends, window menu, cash
// shop). Shared by the legacy buttons and the modern HUD so both apply the same rules.
namespace UI::MainMenu
{
enum class Action
{
    CashShop,
    Character,
    Inventory,
    Friends,
    WindowMenu,
};

// Opens/closes the window behind the action, with its click sound. Returns false when the
// action is not available right now (e.g. only the inventory may be toggled while trading).
bool Trigger(Action action);

// True while the friend button should flash (unread mail or chat).
[[nodiscard]] bool HasFriendAlert();

// True while the character button should flash (a quest is waiting).
[[nodiscard]] bool HasQuestAlert();
} // namespace UI::MainMenu
