#pragma once

#include <cstdint>

// Progress of the hero through the current (master) level, as shown by the experience bar.
//
// The server sends the total experience and the total needed for the next level. The bar
// shows how far the hero is between the start of the current level and that target, so the
// start of the level is derived from the S6 experience formula:
//   normal level L starts at  10 * (9 + n) * n^2                       with n = L - 1,
//                             plus 1000 * (9 + m) * m^2 above level 255, m = n - 255;
//   master level M (total level T = M + 400, m = T - 255) starts at
//                             (10 * (9 + T) * T^2 + 1000 * (9 + m) * m^2 - 3892250000) / 2.
namespace Character::Experience
{
struct Progress
{
    bool isMaster = false;
    int level = 0;                  // master level while isMaster, else the normal level
    std::int64_t experience = 0;    // total experience as sent by the server
    std::int64_t levelStart = 0;    // total experience at the start of the current level
    std::int64_t levelEnd = 0;      // total experience needed for the next level
    double ratio = 0.0;             // 0..1 within the current level
};

[[nodiscard]] std::int64_t NormalLevelStart(int level);
[[nodiscard]] std::int64_t MasterLevelStart(int masterLevel);

// Reads the hero's current progress (master experience once it is active for the class).
[[nodiscard]] Progress HeroProgress();
} // namespace Character::Experience
