#include "stdafx.h"

#include "Character/ExperienceProgress.h"

#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Network/Server/WSclient.h"

#include <algorithm>

namespace Character::Experience
{
namespace
{
constexpr std::int64_t kNormalFactor = 10;
constexpr std::int64_t kHighLevelFactor = 1000;
constexpr std::int64_t kFormulaOffset = 9;
constexpr std::int64_t kHighLevelThreshold = 255;
constexpr std::int64_t kMasterLevelBase = 400;
constexpr std::int64_t kMasterExperienceOffset = 3892250000;
constexpr std::int64_t kMasterExperienceDivisor = 2;

std::int64_t CubicTerm(std::int64_t n)
{
    return (kFormulaOffset + n) * n * n;
}

double RatioWithin(std::int64_t experience, std::int64_t start, std::int64_t end)
{
    const double span = static_cast<double>(end - start);
    if (span <= 0.0)
    {
        return 0.0;
    }
    const double clamped = std::clamp(static_cast<double>(experience), static_cast<double>(start),
                                      static_cast<double>(end));
    return std::clamp((clamped - static_cast<double>(start)) / span, 0.0, 1.0);
}
} // namespace

std::int64_t NormalLevelStart(int level)
{
    const std::int64_t prior = static_cast<std::int64_t>(level) - 1;
    if (prior <= 0)
    {
        return 0;
    }

    std::int64_t start = CubicTerm(prior) * kNormalFactor;
    if (prior > kHighLevelThreshold)
    {
        start += CubicTerm(prior - kHighLevelThreshold) * kHighLevelFactor;
    }
    return start;
}

std::int64_t MasterLevelStart(int masterLevel)
{
    const std::int64_t totalLevel = static_cast<std::int64_t>(masterLevel) + kMasterLevelBase;
    const std::int64_t overLevel = totalLevel - kHighLevelThreshold;
    const std::int64_t data = CubicTerm(totalLevel) * kNormalFactor + CubicTerm(overLevel) * kHighLevelFactor;
    return (data - kMasterExperienceOffset) / kMasterExperienceDivisor;
}

Progress HeroProgress()
{
    Progress progress;
    progress.isMaster = gCharacterManager.IsMasterExperienceActive(CharacterAttribute->Class, CharacterAttribute->Level);
    if (progress.isMaster)
    {
        progress.level = Master_Level_Data.nMLevel;
        progress.experience = static_cast<std::int64_t>(Master_Level_Data.lMasterLevel_Experince);
        progress.levelEnd = static_cast<std::int64_t>(Master_Level_Data.lNext_MasterLevel_Experince);
        progress.levelStart = MasterLevelStart(progress.level);
    }
    else
    {
        progress.level = CharacterAttribute->Level;
        progress.experience = static_cast<std::int64_t>(CharacterAttribute->Experience);
        progress.levelEnd = static_cast<std::int64_t>(CharacterAttribute->NextExperience);
        progress.levelStart = NormalLevelStart(progress.level);
    }

    progress.levelEnd = std::max(progress.levelEnd, progress.levelStart);
    progress.ratio = RatioWithin(progress.experience, progress.levelStart, progress.levelEnd);
    return progress;
}
} // namespace Character::Experience
