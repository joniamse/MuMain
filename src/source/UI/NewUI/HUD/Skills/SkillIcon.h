#pragma once

#include "Core/Globals/_enum.h"

// Where a skill's icon lives in the legacy skill icon sheets. Each icon is a 20x28 cell;
// usable and greyed-out icons come from parallel sheets.
namespace UI::Skills::Icon
{
inline constexpr int kCellWidth = 20;
inline constexpr int kCellHeight = 28;

struct Source
{
    int bitmap = 0;          // texture index (Bitmaps[]), 0 when the skill has no icon
    float x = 0.0f;          // cell position in sheet pixels
    float y = 0.0f;
    float sheetWidth = 0.0f; // sheet size the cell coordinates are relative to
    float sheetHeight = 0.0f;
};

[[nodiscard]] Source Lookup(ActionSkillType skill, bool usable);

// Remaining cooldown of the skill in hero slot `skillIndex`, 0 (ready) .. 1 (just cast).
[[nodiscard]] float CooldownRatio(int skillIndex);

// Self buffs (Infinity Arrow, Expansion of Wizardry) never show a cooldown overlay, and
// the Rage Fighter combo skills hide it while unusable.
[[nodiscard]] bool ShowsCooldown(ActionSkillType skill, bool usable);
} // namespace UI::Skills::Icon
