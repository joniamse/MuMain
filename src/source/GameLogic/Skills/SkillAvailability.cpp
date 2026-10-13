#include "stdafx.h"

#include "GameLogic/Skills/SkillAvailability.h"

#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzInventory.h"
#include "GameLogic/Items/CSItemOption.h"
#include "GameLogic/Items/ItemCategories.h"
#include "GameLogic/Skills/SkillManager.h"
#include "GameLogic/Social/MonkSystem.h"
#include "World/MapInfra/MapManager.h"

namespace GameLogic::Skills::Availability
{
namespace
{
constexpr WORD kFireSlashStrength = 596;
constexpr WORD kIceArrowDexterity = 646;

bool IsAnyOf(ActionSkillType skill, std::initializer_list<ActionSkillType> skills)
{
    for (const ActionSkillType candidate : skills)
    {
        if (skill == candidate)
        {
            return true;
        }
    }
    return false;
}

[[maybe_unused]] bool IsPetCommand(ActionSkillType skill)
{
    return skill >= AT_PET_COMMAND_DEFAULT && skill < AT_PET_COMMAND_END;
}

bool IsEarthshake(ActionSkillType skill)
{
    return IsAnyOf(skill, {AT_SKILL_EARTHSHAKE, AT_SKILL_EARTHSHAKE_STR, AT_SKILL_EARTHSHAKE_MASTERY});
}

// Neither hand holds a staff, and the right hand holds something.
bool HoldsNonStaffWeapon()
{
    const int left = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
    const int right = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;
    const auto isStaff = [](int type) { return type >= ITEM_STAFF && type < ITEM_STAFF + MAX_ITEM_INDEX; };
    return right != -1 && !isStaff(right) && !isStaff(left);
}

bool HoldsSpear()
{
    const int left = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
    const int right = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;
    const auto isSpear = [](int type) { return type >= ITEM_SPEAR && type < ITEM_BOW; };
    return isSpear(left) || isSpear(right);
}

bool HasLivingDarkHorse()
{
    return CharacterMachine->Equipment[EQUIPMENT_HELPER].Durability != 0;
}

bool PassesAttributesAndBuffs(ActionSkillType skill)
{
    if (!gSkillManager.AreSkillAttributeRequirementsMet(skill) || !IsCanBCSkill(skill))
    {
        return false;
    }
    if (g_isCharacterBuff((&Hero->Object), eBuff_AddSkill) &&
        SkillAttribute[skill].SkillUseType == SKILL_USE_TYPE_BRAND)
    {
        return false;
    }
    // Self buffs that cannot be refreshed while active.
    const bool isAmplifier = IsAnyOf(skill, {AT_SKILL_INFINITY_ARROW, AT_SKILL_INFINITY_ARROW_STR,
                                             AT_SKILL_EXPANSION_OF_WIZARDRY, AT_SKILL_EXPANSION_OF_WIZARDRY_STR,
                                             AT_SKILL_EXPANSION_OF_WIZARDRY_MASTERY});
    const bool amplifierActive = g_isCharacterBuff((&Hero->Object), eBuff_InfinityArrow) ||
                                 g_isCharacterBuff((&Hero->Object), eBuff_SwellOfMagicPower);
    return !(isAmplifier && amplifierActive);
}

bool PassesMountRules(ActionSkillType skill)
{
    const bool onHorn = GameLogic::Items::IsHornMountModel(Hero->Helper.Type);
    if (skill == AT_SKILL_IMPALE && (!onHorn || !HoldsSpear()))
    {
        return false;
    }

    const bool groundOnly = (skill >= AT_SKILL_BLOCKING && skill <= AT_SKILL_SLASH) ||
                            IsAnyOf(skill, {AT_SKILL_FALLING_SLASH_STR, AT_SKILL_LUNGE_STR, AT_SKILL_CYCLONE_STR,
                                            AT_SKILL_CYCLONE_STR_MG, AT_SKILL_SLASH_STR, AT_SKILL_POWER_SLASH,
                                            AT_SKILL_POWER_SLASH_STR});
    if (onHorn && groundOnly)
    {
        return false;
    }

    if (IsEarthshake(skill) && (!HasLivingDarkHorse() || Hero->Helper.Type != MODEL_DARK_HORSE_ITEM))
    {
        return false;
    }
    return !g_CMonkSystem.IsRideNotUseSkill(skill, Hero->Helper.Type);
}

bool PassesMapAndPartyRules(ActionSkillType skill)
{
    if (skill == AT_SKILL_PARTY_TELEPORT &&
        (PartyNumber <= 0 || IsDoppelGanger1() || IsDoppelGanger2() || IsDoppelGanger3() || IsDoppelGanger4()))
    {
        return false;
    }

    if (gMapManager.InChaosCastle())
    {
        const bool blockedInChaosCastle = IsEarthshake(skill) || skill == AT_SKILL_RIDER ||
                                          (skill >= AT_PET_COMMAND_DEFAULT && skill <= AT_PET_COMMAND_TARGET);
        return !blockedInChaosCastle;
    }
    return !(IsEarthshake(skill) && !HasLivingDarkHorse());
}

bool PassesWeaponAndStatRules(ActionSkillType skill)
{
    if (IsAnyOf(skill, {AT_SKILL_FIRE_SLASH, AT_SKILL_FIRE_SLASH_STR}))
    {
        const WORD strength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
        if (strength < kFireSlashStrength || !HoldsNonStaffWeapon())
        {
            return false;
        }
    }

    if (IsAnyOf(skill, {AT_SKILL_ICE_ARROW, AT_SKILL_ICE_ARROW_STR}))
    {
        const WORD dexterity = CharacterAttribute->Dexterity + CharacterAttribute->AddDexterity;
        if (dexterity < kIceArrowDexterity)
        {
            return false;
        }
    }

    const bool needsNonStaffWeapon =
        IsAnyOf(skill, {AT_SKILL_TWISTING_SLASH, AT_SKILL_TWISTING_SLASH_STR, AT_SKILL_TWISTING_SLASH_STR_MG,
                        AT_SKILL_TWISTING_SLASH_MASTERY, AT_SKILL_RAGEFUL_BLOW, AT_SKILL_RAGEFUL_BLOW_STR,
                        AT_SKILL_RAGEFUL_BLOW_MASTERY, AT_SKILL_DEATHSTAB, AT_SKILL_DEATHSTAB_STR,
                        AT_SKILL_FLAME_STRIKE});
    if (needsNonStaffWeapon && !HoldsNonStaffWeapon())
    {
        return false;
    }

    if (skill == AT_SKILL_MULTI_SHOT && gCharacterManager.GetEquipedBowType_Skill() == BOWTYPE_NONE)
    {
        return false;
    }
    return g_csItemOption.IsNonWeaponSkillOrIsSkillEquipped(skill);
}

bool PassesRageFighterRules(ActionSkillType skill)
{
    if (!g_CMonkSystem.IsSwordformGlovesUseSkill(skill))
    {
        return false;
    }

    const ITEM& leftRing = CharacterMachine->Equipment[EQUIPMENT_RING_LEFT];
    const ITEM& rightRing = CharacterMachine->Equipment[EQUIPMENT_RING_RIGHT];
    const bool transformedByRing =
        g_CMonkSystem.IsChangeringNotUseSkill(leftRing.Type, rightRing.Type, leftRing.Level, rightRing.Level);
    return !(transformedByRing && gCharacterManager.GetBaseClass(Hero->Class) == CLASS_RAGEFIGHTER);
}

#ifdef PJH_FIX_SPRIT
bool PassesPetCharisma(ActionSkillType skill)
{
    if (!IsPetCommand(skill))
    {
        return true;
    }
    const int charisma = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;
    PET_INFO petInfo;
    giPetManager::GetPetInfo(petInfo, 421 - PET_TYPE_DARK_SPIRIT);
    const int requiredCharisma = 185 + (petInfo.m_wLevel * 15);
    return requiredCharisma <= charisma;
}
#endif // PJH_FIX_SPRIT
} // namespace

bool CanUseNow(ActionSkillType skill)
{
    const bool usable = PassesAttributesAndBuffs(skill) && PassesMountRules(skill) &&
                        PassesMapAndPartyRules(skill) && PassesWeaponAndStatRules(skill) &&
                        PassesRageFighterRules(skill);
#ifdef PJH_FIX_SPRIT
    return usable && PassesPetCharisma(skill);
#else
    return usable;
#endif
}

bool IsListed(ActionSkillType skill)
{
    if (skill == 0 || (skill >= AT_SKILL_STUN && skill <= AT_SKILL_REMOVAL_BUFF))
    {
        return false;
    }
    const BYTE useType = SkillAttribute[skill].SkillUseType;
    return useType != SKILL_USE_TYPE_MASTER && useType != SKILL_USE_TYPE_MASTERLEVEL;
}
} // namespace GameLogic::Skills::Availability
