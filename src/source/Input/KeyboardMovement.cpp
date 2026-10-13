#include "stdafx.h"

#include "Input/KeyboardMovement.h"

#include "Camera/CameraState.h"
#include "Core/Input/KeyState.h"
#include "Data/GameConfig/GameConfig.h"
#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzObject.h"
#include "GameLogic/Items/PersonalShopTitleImp.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"

#include <cmath>

extern int TargetX;
extern int TargetY;

namespace Input::KeyboardMovement
{
namespace
{
// How far ahead (in tiles) a walk is planned; shorter keeps the hero responsive on release.
constexpr int kLookAheadTiles = 2;
// The hero's anchor on screen in HUD coordinates, as MoveHero() uses for mouse aiming.
constexpr float kHeroScreenY = 180.0f;
// Distance of the virtual cursor that turns a key direction into a facing angle.
constexpr float kVirtualCursorDistance = 100.0f;
constexpr float kFullTurn = 360.0f;
constexpr int kTerrainSize = TERRAIN_SIZE;

struct State
{
    bool moving = false;
    int lastTargetX = -1;
    int lastTargetY = -1;
    int lastHeroX = -1;
    int lastHeroY = -1;
};

State& GetState()
{
    static State state;
    return state;
}

bool IsMovementKey(int key)
{
    return key == 'W' || key == 'A' || key == 'S' || key == 'D';
}

bool IsModifierHeld()
{
    return Core::Input::IsKeyDown(VK_SHIFT) || Core::Input::IsKeyDown(VK_CONTROL);
}

bool IsTyping()
{
    return g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX) ||
           CUITextInputBox::GetFocusedPortable() != nullptr;
}

// Screen-space direction of the held keys: x to the right, y downwards.
bool ReadDirection(float& x, float& y)
{
    x = static_cast<float>(Core::Input::IsKeyDown('D')) - static_cast<float>(Core::Input::IsKeyDown('A'));
    y = static_cast<float>(Core::Input::IsKeyDown('S')) - static_cast<float>(Core::Input::IsKeyDown('W'));
    return x != 0.0f || y != 0.0f;
}

// Same transform MoveHero() applies to turn the hero towards the cursor, so "up" follows
// the camera's yaw for every camera mode.
float FacingAngle(float screenX, float screenY)
{
    const float heroX = static_cast<float>(GetScreenWidth()) / 2.0f;
    const float cursorX = heroX + screenX * kVirtualCursorDistance;
    const float cursorY = kHeroScreenY + screenY * kVirtualCursorDistance;
    const float angle = -(CreateAngle(cursorX, cursorY, heroX, kHeroScreenY) + g_Camera.Angle[2]) + kFullTurn;
    return std::fmod(angle, kFullTurn);
}

bool IsWalkable(int tileX, int tileY)
{
    if (tileX < 0 || tileY < 0 || tileX >= kTerrainSize || tileY >= kTerrainSize)
    {
        return false;
    }
    const WORD attribute = TerrainWall[tileY * kTerrainSize + tileX];
    const bool blocked = attribute >= TW_NOGROUND && (attribute & TW_ACTION) != TW_ACTION &&
                         (attribute & TW_HEIGHT) != TW_HEIGHT;
    return !blocked;
}

// Plans a walk towards the facing direction, trying the full look-ahead first.
bool WalkTowards(CHARACTER* hero, OBJECT* heroObject, float facing)
{
    State& state = GetState();
    vec3_t angles = {0.0f, 0.0f, facing};
    float matrix[3][4];
    AngleMatrix(angles, matrix);
    vec3_t forward = {0.0f, -1.0f, 0.0f};
    vec3_t direction;
    VectorRotate(forward, matrix, direction);

    for (int tiles = kLookAheadTiles; tiles >= 1; --tiles)
    {
        const float reach = static_cast<float>(tiles) * TERRAIN_SCALE;
        const int tileX = static_cast<int>((heroObject->Position[0] + direction[0] * reach) / TERRAIN_SCALE);
        const int tileY = static_cast<int>((heroObject->Position[1] + direction[1] * reach) / TERRAIN_SCALE);
        if ((tileX == hero->PositionX && tileY == hero->PositionY) || !IsWalkable(tileX, tileY))
        {
            continue;
        }
        if (!PathFinding2(hero->PositionX, hero->PositionY, tileX, tileY, &hero->Path))
        {
            continue;
        }

        TargetX = tileX;
        TargetY = tileY;
        hero->MovementType = MOVEMENT_MOVE;
        SendMove(hero, heroObject);
        state.lastTargetX = tileX;
        state.lastTargetY = tileY;
        return true;
    }
    return false;
}

void StopIfMoving(CHARACTER* hero)
{
    State& state = GetState();
    if (!state.moving)
    {
        return;
    }
    state.moving = false;
    state.lastTargetX = state.lastTargetY = -1;
    LetHeroStop(hero, TRUE);
    SetPlayerStop(hero);
}
} // namespace

bool IsEnabled()
{
    return GameConfig::GetInstance().GetWasdMovement();
}

bool Update(CHARACTER* hero, OBJECT* heroObject)
{
    float screenX = 0.0f;
    float screenY = 0.0f;
    // A character with its personal store open doesn't walk.
    const bool wantsMove = IsEnabled() && !IsModifierHeld() && !IsTyping() && !IsShopInViewport(hero)
                           && ReadDirection(screenX, screenY);
    if (!wantsMove)
    {
        StopIfMoving(hero);
        return false;
    }

    State& state = GetState();
    // Re-plan when standing, or once per tile so the path keeps extending ahead of the hero.
    const bool enteredNewTile = hero->PositionX != state.lastHeroX || hero->PositionY != state.lastHeroY;
    if (hero->Movement && !enteredNewTile)
    {
        return true;
    }
    state.lastHeroX = hero->PositionX;
    state.lastHeroY = hero->PositionY;

    if (WalkTowards(hero, heroObject, FacingAngle(screenX, screenY)))
    {
        state.moving = true;
    }
    return true;
}

bool IsHotkeyPressed(int key)
{
    if (!SEASON3B::IsPress(key))
    {
        return false;
    }
    return !IsEnabled() || !IsMovementKey(key) || Core::Input::IsKeyDown(VK_SHIFT);
}
} // namespace Input::KeyboardMovement
