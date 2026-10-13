#pragma once

class CHARACTER;
class OBJECT;

// [Modern] VipAuras: an aura around the characters with a VIP membership, one look per level
// (Plata, Oro, Platino, Diamante). The server tells every client the level of each character in
// the game server with notices "#MUAURA|name|level" (level 0: none).
namespace GameLogic::VipAura
{
// Takes a server notice. Returns true when it was an aura notice, which must not be shown.
[[nodiscard]] bool HandleNotice(const wchar_t* text);

// Forgets every level, e.g. when leaving the game server.
void Clear();

// Draws the aura of a player character, if it has one. Call during the 3D pass, before the
// character's model is drawn (the aura lies on the ground under it).
void Render(CHARACTER* character, OBJECT* object);
} // namespace GameLogic::VipAura
