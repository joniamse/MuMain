#include "stdafx.h"

#include "UI/NewUI/HUD/Skills/SkillIcon.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInterface.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"

#include <algorithm>

namespace UI::Skills::Icon
{
namespace
{
using SkillList = SEASON3B::CNewUISkillList;

constexpr float kSheetSize = 256.0f;
constexpr float kMasterSheetSize = 512.0f;
constexpr int kMasterColumns = 25;
// Disabled (greyed-out) sheets follow their usable sheet at this index distance.
constexpr int kDisabledSheetOffset = SkillList::IMAGE_NON_SKILL1 - SkillList::IMAGE_SKILL1;
constexpr int kMasterUsableSheet = BITMAP_INTERFACE_MASTER_BEGIN + 2;
constexpr int kMasterDisabledSheet = BITMAP_INTERFACE_MASTER_BEGIN + 3;
// Rows of the second sheet that hold active master skills, addressed by their Magic_Icon number.
constexpr int kIconNumberColumns = 12;
constexpr int kIconNumberFirstRow = 4;

struct Cell
{
    int sheet;
    int column;
    int row;
};

// Fixed cells for skills whose position does not follow from their number.
bool LookupSpecialCell(ActionSkillType skill, Cell& cell)
{
    struct Entry
    {
        ActionSkillType skill;
        Cell cell;
    };
    static constexpr Entry kSpecialCells[] = {
        {AT_SKILL_PLASMA_STORM_FENRIR, {SkillList::IMAGE_COMMAND, 4, 0}},
        {AT_SKILL_ALICE_BERSERKER, {SkillList::IMAGE_SKILL2, 10, 3}},
        {AT_SKILL_SUMMON_POLLUTION, {SkillList::IMAGE_SKILL2, 11, 3}},
        {AT_SKILL_STRIKE_OF_DESTRUCTION, {SkillList::IMAGE_SKILL2, 7, 2}},
        {AT_SKILL_CHAOTIC_DISEIER, {SkillList::IMAGE_SKILL2, 3, 8}},
        {AT_SKILL_RECOVER, {SkillList::IMAGE_SKILL2, 9, 2}},
        {AT_SKILL_MULTI_SHOT, {SkillList::IMAGE_SKILL2, 0, 8}},
        {AT_SKILL_FLAME_STRIKE, {SkillList::IMAGE_SKILL2, 1, 8}},
        {AT_SKILL_GIGANTIC_STORM, {SkillList::IMAGE_SKILL2, 2, 8}},
        {AT_SKILL_LIGHTNING_SHOCK, {SkillList::IMAGE_SKILL2, 2, 3}},
        {AT_SKILL_EXPANSION_OF_WIZARDRY, {SkillList::IMAGE_SKILL2, 8, 2}},
    };
    for (const Entry& entry : kSpecialCells)
    {
        if (entry.skill == skill)
        {
            cell = entry.cell;
            return true;
        }
    }
    return false;
}

// Cells derived from the skill number within a family of consecutive skills.
Cell LookupCell(ActionSkillType skill)
{
    const int type = static_cast<int>(skill);
    Cell cell{};
    if (type >= AT_PET_COMMAND_DEFAULT && type <= AT_PET_COMMAND_END)
    {
        return {SkillList::IMAGE_COMMAND, (type - AT_PET_COMMAND_DEFAULT) % 8, (type - AT_PET_COMMAND_DEFAULT) / 8};
    }
    if (LookupSpecialCell(skill, cell))
    {
        return cell;
    }
    if (skill >= AT_SKILL_ALICE_DRAINLIFE && skill <= AT_SKILL_ALICE_THORNS)
    {
        return {SkillList::IMAGE_SKILL2, (type - AT_SKILL_ALICE_DRAINLIFE) % 8, 3};
    }
    if (skill >= AT_SKILL_ALICE_SLEEP && skill <= AT_SKILL_ALICE_BLIND)
    {
        return {SkillList::IMAGE_SKILL2, (type - AT_SKILL_ALICE_SLEEP + 4) % 8, 3};
    }
    if (skill >= AT_SKILL_ALICE_WEAKNESS && skill <= AT_SKILL_ALICE_ENERVATION)
    {
        return {SkillList::IMAGE_SKILL2, type - AT_SKILL_ALICE_WEAKNESS + 8, 3};
    }
    if (skill >= AT_SKILL_SUMMON_EXPLOSION && skill <= AT_SKILL_SUMMON_REQUIEM)
    {
        return {SkillList::IMAGE_SKILL2, (type - AT_SKILL_SUMMON_EXPLOSION + 6) % 8, 3};
    }
    if (SkillAttribute[skill].SkillUseType == SKILL_USE_TYPE_MASTERACTIVE)
    {
        const int icon = SkillAttribute[skill].Magic_Icon;
        return {SkillList::IMAGE_SKILL2, icon % kIconNumberColumns, icon / kIconNumberColumns + kIconNumberFirstRow};
    }
    if (skill >= AT_SKILL_KILLING_BLOW)
    {
        return {SkillList::IMAGE_SKILL3, (type - AT_SKILL_KILLING_BLOW) % 12, (type - AT_SKILL_KILLING_BLOW) / 12};
    }
    if (skill >= AT_SKILL_SPIRAL_SLASH)
    {
        return {SkillList::IMAGE_SKILL2, (type - AT_SKILL_SPIRAL_SLASH) % 8, (type - AT_SKILL_SPIRAL_SLASH) / 8};
    }
    return {SkillList::IMAGE_SKILL1, (type - 1) % 8, (type - 1) / 8};
}
} // namespace

Source Lookup(ActionSkillType skill, bool usable)
{
    if (skill >= AT_SKILL_MASTER_BEGIN)
    {
        const int icon = SkillAttribute[skill].Magic_Icon;
        return {usable ? kMasterUsableSheet : kMasterDisabledSheet,
                static_cast<float>((icon % kMasterColumns) * kCellWidth),
                static_cast<float>((icon / kMasterColumns) * kCellHeight), kMasterSheetSize, kMasterSheetSize};
    }

    const Cell cell = LookupCell(skill);
    return {usable ? cell.sheet : cell.sheet + kDisabledSheetOffset, static_cast<float>(cell.column * kCellWidth),
            static_cast<float>(cell.row * kCellHeight), kSheetSize, kSheetSize};
}

float CooldownRatio(int skillIndex)
{
    const int remaining = CharacterAttribute->SkillDelay[skillIndex];
    if (remaining <= 0)
    {
        return 0.0f;
    }

    const int skill = CharacterAttribute->Skill[skillIndex];
    // Fenrir's Plasma Storm only counts down while the hero can attack.
    if (skill == AT_SKILL_PLASMA_STORM_FENRIR && !CheckAttack())
    {
        return 0.0f;
    }

    const int total = SkillAttribute[skill].Delay;
    return total > 0 ? std::min(static_cast<float>(remaining) / static_cast<float>(total), 1.0f) : 0.0f;
}

bool ShowsCooldown(ActionSkillType skill, bool usable)
{
    switch (skill)
    {
    case AT_SKILL_INFINITY_ARROW:
    case AT_SKILL_INFINITY_ARROW_STR:
    case AT_SKILL_EXPANSION_OF_WIZARDRY:
    case AT_SKILL_EXPANSION_OF_WIZARDRY_STR:
    case AT_SKILL_EXPANSION_OF_WIZARDRY_MASTERY:
        return false;
    case AT_SKILL_CHAIN_DRIVE:
    case AT_SKILL_CHAIN_DRIVE_STR:
    case AT_SKILL_DRAGON_KICK:
    case AT_SKILL_DRAGON_ROAR:
    case AT_SKILL_DRAGON_ROAR_STR:
        return usable;
    default:
        return true;
    }
}
} // namespace UI::Skills::Icon
