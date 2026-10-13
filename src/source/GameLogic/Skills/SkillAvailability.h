#pragma once

#include "Core/Globals/_enum.h"

// Client-side rules that decide whether the hero can cast a skill right now. The skill bar
// greys out skills that fail them. The server validates casts on its own; these rules only
// mirror the original S6 client so the bar shows what will work.
namespace GameLogic::Skills::Availability
{
// Equipment (weapon kind, bow, mount, rings, gloves), map (Chaos Castle, Doppelganger),
// stats (Fire Slash 596 STR, Ice Arrow 646 DEX), active buffs and party membership.
[[nodiscard]] bool CanUseNow(ActionSkillType skill);

// Whether the skill appears in the skill list / picker: not an empty slot, not one of the
// internal buff skills (Stun..Removal Buff) and not a passive master skill.
[[nodiscard]] bool IsListed(ActionSkillType skill);
} // namespace GameLogic::Skills::Availability
