#include "stdafx.h"

#include "GameLogic/Vip/VipAura.h"

#include "Core/Utilities/_GlobalFunctions.h"
#include "Data/GameConfig/GameConfig.h"
#include "Engine/Object/w_CharacterInfo.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Render/Textures/ZzzOpenglUtil.h"

#include <array>
#include <cmath>
#include <cwchar>
#include <string>
#include <string_view>
#include <unordered_map>

namespace GameLogic::VipAura
{
namespace
{
constexpr std::wstring_view kPrefix = L"#MUAURA|";
constexpr float kPi = 3.1415926f;
constexpr float kTwoPi = 2.f * kPi;

enum Level
{
    kNone = 0,
    kPlata = 1,
    kOro = 2,
    kPlatino = 3,
    kDiamante = 4,
};

// The textures of tools/aura_textures.py (ours and some from Kenney's Particle Pack, CC0), loaded on
// first use; BITMAP_UNKNOWN when one is missing, and the part that uses it is left out.
struct Textures
{
    GLuint sigil = BITMAP_UNKNOWN;
    GLuint halo = BITMAP_UNKNOWN;
    GLuint orb = BITMAP_UNKNOWN;
    GLuint starSoft = BITMAP_UNKNOWN;
    GLuint starSharp = BITMAP_UNKNOWN;
    GLuint runes = BITMAP_UNKNOWN;
    GLuint vortex = BITMAP_UNKNOWN;
    GLuint vortexWide = BITMAP_UNKNOWN;
    GLuint pillar = BITMAP_UNKNOWN;
    std::array<GLuint, 2> flames{BITMAP_UNKNOWN, BITMAP_UNKNOWN};
    GLuint shockwave = BITMAP_UNKNOWN;
    std::array<GLuint, 4> arcs{BITMAP_UNKNOWN, BITMAP_UNKNOWN, BITMAP_UNKNOWN, BITMAP_UNKNOWN};
};

GLuint Load(const wchar_t* name)
{
    return Bitmaps.LoadImage(std::wstring(L"Data\\Effect\\Modern\\") + name + L".jpg", GL_LINEAR, GL_CLAMP_TO_EDGE);
}

const Textures& GetTextures()
{
    static const Textures textures = [] {
        Textures loaded;
        loaded.sigil = Load(L"aura_sigil");
        loaded.halo = Load(L"aura_halo");
        loaded.orb = Load(L"aura_orb");
        loaded.starSoft = Load(L"aura_star_soft");
        loaded.starSharp = Load(L"aura_star_sharp");
        loaded.runes = Load(L"aura_runes");
        loaded.vortex = Load(L"aura_vortex");
        loaded.vortexWide = Load(L"aura_vortex_wide");
        loaded.pillar = Load(L"aura_pillar");
        loaded.flames = {Load(L"aura_flame_1"), Load(L"aura_flame_2")};
        loaded.shockwave = Load(L"aura_shockwave");
        loaded.arcs = {Load(L"aura_arc_1"), Load(L"aura_arc_2"), Load(L"aura_arc_3"), Load(L"aura_arc_4")};
        return loaded;
    }();
    return textures;
}

GLuint Or(GLuint texture, GLuint fallback)
{
    return texture != BITMAP_UNKNOWN ? texture : fallback;
}

bool Has(GLuint texture)
{
    return texture != BITMAP_UNKNOWN;
}

std::unordered_map<std::wstring, int>& Levels()
{
    static std::unordered_map<std::wstring, int> levels;
    return levels;
}

struct Color
{
    float red;
    float green;
    float blue;

    void To(float factor, vec3_t light) const { Vector(red * factor, green * factor, blue * factor, light); }
};

constexpr Color kSilver{0.70f, 0.78f, 0.95f};
constexpr Color kGold{1.00f, 0.70f, 0.20f};
constexpr Color kCyan{0.25f, 0.85f, 1.00f};
constexpr Color kWhiteGold{1.00f, 0.92f, 0.65f};

// Diamante's colour drifts through violet, magenta and sky blue.
Color Prismatic(float time, float offset = 0.f)
{
    const float t = time * 0.0006f + offset;
    return {0.65f + 0.35f * std::sinf(t), 0.35f + 0.25f * std::sinf(t + 2.1f), 0.95f + 0.05f * std::sinf(t + 4.2f)};
}

float Pulse(float time, float speed = 0.003f, float depth = 0.25f)
{
    return 1.f - depth + depth * std::sinf(time * speed);
}

void Glow(OBJECT* object, float height, float scale, const Color& color, float strength, GLuint texture = BITMAP_LIGHT)
{
    vec3_t position, light;
    VectorCopy(object->Position, position);
    position[2] += height;
    color.To(strength, light);
    CreateSprite(static_cast<int>(texture), position, scale, light, object);
}

// Motes of light rising around the character on a spiral, fading at the top.
void Motes(OBJECT* object, float time, int count, const Color& color, GLuint texture, float scale, float height)
{
    vec3_t position, light;
    for (int i = 0; i < count; ++i)
    {
        const float phase = static_cast<float>(i) / static_cast<float>(count);
        const float cycle = std::fmod(time * 0.0004f + phase * 1.7f, 1.f);
        const float angle = phase * kTwoPi + time * 0.0012f;
        const float radius = 55.f * (1.f - cycle * 0.45f);
        position[0] = object->Position[0] + std::cosf(angle) * radius;
        position[1] = object->Position[1] + std::sinf(angle) * radius;
        position[2] = object->Position[2] + 15.f + cycle * height;
        color.To(std::sinf(cycle * kPi), light);
        CreateSprite(static_cast<int>(texture), position, scale, light, object, time * 0.1f + phase * 360.f);
    }
}

// A stable pseudo-random number in [0, 1) for a step of an effect and an index.
float Hash(int step, int index, float seed)
{
    const float x = std::sinf(static_cast<float>(step) * 12.9898f + static_cast<float>(index) * 78.233f + seed * 3.17f) * 43758.547f;
    return x - std::floor(x);
}

// Electric arcs crackling around the body: each one jumps to a new place every few hundredths of
// a second, sometimes skipping a beat, so they flicker like real sparks.
void Electricity(OBJECT* object, float time, int count, const Color& color, const Textures& textures, float seed)
{
    constexpr float kStepMs = 90.f;
    const int step = static_cast<int>(time / kStepMs);
    vec3_t position, light;
    for (int i = 0; i < count; ++i)
    {
        if (Hash(step, i + 50, seed) < 0.3f)
        {
            continue;
        }
        const GLuint texture = textures.arcs[static_cast<std::size_t>(Hash(step, i + 20, seed) * 4.f) % 4];
        if (!Has(texture))
        {
            continue;
        }
        const float angle = Hash(step, i, seed) * kTwoPi;
        const float radius = 18.f + Hash(step, i + 10, seed) * 22.f;
        position[0] = object->Position[0] + std::cosf(angle) * radius;
        position[1] = object->Position[1] + std::sinf(angle) * radius;
        position[2] = object->Position[2] + 40.f + Hash(step, i + 30, seed) * 120.f;
        color.To(0.9f, light);
        CreateSprite(static_cast<int>(texture), position, 0.55f + Hash(step, i + 40, seed) * 0.3f, light, object,
                     Hash(step, i + 60, seed) * 360.f);
    }
}

// Glowing embers drifting up from the ground, swaying and dimming from gold to red as they rise.
void Embers(OBJECT* object, float time, int count, const Textures& textures, float seed)
{
    const GLuint texture = Or(textures.orb, BITMAP_LIGHT);
    vec3_t position, light;
    for (int i = 0; i < count; ++i)
    {
        const float speed = 0.00025f + Hash(0, i, seed) * 0.00025f;
        const float cycle = std::fmod(time * speed + Hash(1, i, seed), 1.f);
        const float angle = Hash(2, i, seed) * kTwoPi;
        const float radius = 15.f + Hash(3, i, seed) * 55.f;
        const float sway = std::sinf(time * 0.002f + static_cast<float>(i)) * 12.f * cycle;
        position[0] = object->Position[0] + std::cosf(angle) * radius + sway;
        position[1] = object->Position[1] + std::sinf(angle) * radius;
        position[2] = object->Position[2] + 5.f + cycle * 230.f;
        const float glow = std::sinf(cycle * kPi) * (0.7f + 0.3f * std::sinf(time * 0.02f + static_cast<float>(i)));
        Vector(glow, glow * (0.75f - 0.45f * cycle), glow * 0.25f * (1.f - cycle), light);
        CreateSprite(static_cast<int>(texture), position, 0.09f + Hash(4, i, seed) * 0.06f, light, object);
    }
}

// A column of light: soft vertical lights stacked from the feet up, fading with height.
void Column(OBJECT* object, float time, const Color& color, GLuint texture)
{
    constexpr int kSegments = 6;
    const float flicker = Pulse(time, 0.004f, 0.2f);
    for (int i = 0; i < kSegments; ++i)
    {
        const float fade = 1.f - static_cast<float>(i) / kSegments;
        Glow(object, 40.f + static_cast<float>(i) * 55.f, 2.2f, color, 0.18f * fade * flicker, texture);
    }
}

// A ground decal centred on the character.
void Decal(OBJECT* object, GLuint texture, float size, const Color& color, float strength, float rotation,
           float height = 5.f)
{
    if (!Has(texture))
    {
        return;
    }
    vec3_t light;
    color.To(strength, light);
    RenderTerrainAlphaBitmap(static_cast<int>(texture), object->Position[0], object->Position[1], size, size, light,
                             rotation, 1.f, height);
}

// Flames licking up around the feet on a ring, each flickering on its own: it grows, sways and
// fades in a short loop, and the ring turns slowly.
void Flames(OBJECT* object, float time, int count, const Color& tint, const Textures& textures, float seed)
{
    vec3_t position, light;
    for (int i = 0; i < count; ++i)
    {
        const GLuint texture = textures.flames[static_cast<std::size_t>(i) % textures.flames.size()];
        if (!Has(texture))
        {
            continue;
        }
        const float cycle = std::fmod(time * 0.0011f + Hash(5, i, seed), 1.f);
        const float angle = static_cast<float>(i) * kTwoPi / static_cast<float>(count) + time * 0.0004f;
        const float radius = 48.f + 6.f * std::sinf(time * 0.003f + static_cast<float>(i));
        position[0] = object->Position[0] + std::cosf(angle) * radius;
        position[1] = object->Position[1] + std::sinf(angle) * radius;
        position[2] = object->Position[2] + 20.f + cycle * 35.f;
        // Fire colours, drawn towards the aura's tint: bright at the start, fading as it rises.
        const float heat = std::sinf(cycle * kPi);
        Vector(heat * (0.6f + 0.4f * tint.red), heat * (0.35f + 0.3f * tint.green), heat * (0.15f + 0.45f * tint.blue), light);
        CreateSprite(static_cast<int>(texture), position, 0.45f + 0.25f * (1.f - cycle), light, object,
                     std::sinf(time * 0.004f + static_cast<float>(i)) * 12.f);
    }
}

// Pale ash drifting up slowly, higher than the embers, swaying more.
void Ash(OBJECT* object, float time, int count, const Textures& textures, float seed)
{
    const GLuint texture = Or(textures.orb, BITMAP_LIGHT);
    vec3_t position, light;
    for (int i = 0; i < count; ++i)
    {
        const float cycle = std::fmod(time * (0.00012f + Hash(6, i, seed) * 0.0001f) + Hash(7, i, seed), 1.f);
        const float angle = Hash(8, i, seed) * kTwoPi + cycle * 1.5f;
        const float radius = 25.f + Hash(9, i, seed) * 70.f;
        position[0] = object->Position[0] + std::cosf(angle) * radius + std::sinf(time * 0.0015f + i) * 20.f * cycle;
        position[1] = object->Position[1] + std::sinf(angle) * radius;
        position[2] = object->Position[2] + 30.f + cycle * 280.f;
        const float grey = 0.35f * std::sinf(cycle * kPi);
        Vector(grey, grey * 0.95f, grey * 0.9f, light);
        CreateSprite(static_cast<int>(texture), position, 0.06f + Hash(10, i, seed) * 0.04f, light, object);
    }
}

// Every few seconds a ring of fire spreads from the feet over the ground and fades out.
void Shockwave(OBJECT* object, float time, const Color& tint, const Textures& textures, float seed)
{
    constexpr float kPeriod = 4200.f;
    constexpr float kLength = 900.f;
    const float phase = std::fmod(time + seed * 997.f, kPeriod);
    if (phase > kLength)
    {
        return;
    }
    const float t = phase / kLength;
    const Color fire{0.6f + 0.4f * tint.red, 0.4f + 0.3f * tint.green, 0.2f + 0.5f * tint.blue};
    Decal(object, textures.shockwave, 0.6f + t * 2.6f, fire, (1.f - t) * 0.9f, seed);
}

void RenderPlata(OBJECT* object, float time, const Textures& textures)
{
    vec3_t light;
    kSilver.To(Pulse(time), light);
    RenderTerrainAlphaBitmap(BITMAP_GM_AURORA, object->Position[0], object->Position[1], 1.15f, 1.15f, light, time * 0.01f);
    Glow(object, 25.f, 2.2f, kSilver, 0.3f);
    Motes(object, time, 3, kSilver, Or(textures.starSoft, BITMAP_LIGHT), 0.35f, 140.f);
}

void RenderOro(OBJECT* object, float time, const Textures& textures)
{
    vec3_t light;
    const float pulse = Pulse(time);
    Decal(object, textures.runes, 1.7f, kGold, pulse * 0.8f, time * 0.004f);
    kGold.To(pulse, light);
    RenderTerrainAlphaBitmap(BITMAP_GM_AURORA, object->Position[0], object->Position[1], 1.3f, 1.3f, light, time * 0.01f);
    kGold.To(pulse * 0.8f, light);
    RenderTerrainAlphaBitmap(BITMAP_GM_AURORA, object->Position[0], object->Position[1], 0.9f, 0.9f, light, -time * 0.014f);
    Glow(object, 25.f, 2.6f, kGold, 0.35f);
    Motes(object, time, 6, kGold, Or(textures.starSoft, BITMAP_LIGHT), 0.5f, 180.f);
}

void RenderPlatino(OBJECT* object, float time, const Textures& textures, float seed)
{
    const float pulse = Pulse(time);
    Decal(object, textures.vortex, 2.0f, kCyan, pulse * 0.75f, -time * 0.008f);
    Decal(object, textures.sigil, 1.6f, kCyan, pulse * 0.9f, time * 0.006f);
    Glow(object, 25.f, 2.8f, kCyan, 0.4f);
    Motes(object, time, 8, kCyan, Or(textures.starSharp, BITMAP_LIGHT), 0.4f, 200.f);
    Electricity(object, time, 2, kCyan, textures, seed);
}

// The most glorious: a prismatic vortex and two sigils turning on the ground, a column of light,
// a halo over the head, crackling electricity, a ring of flames, rising embers and ash and,
// every few seconds, a ring of fire spreading over the ground.
void RenderDiamante(OBJECT* object, float time, const Textures& textures, float seed)
{
    const float pulse = Pulse(time, 0.0025f, 0.2f);
    const Color main = Prismatic(time);
    const Color second = Prismatic(time, 2.f);

    Decal(object, textures.vortexWide, 2.6f, second, pulse * 0.7f, -time * 0.006f);
    Decal(object, textures.sigil, 2.1f, main, pulse, time * 0.005f);
    Decal(object, textures.sigil, 1.25f, second, pulse * 0.6f, -time * 0.009f);
    vec3_t light;
    kWhiteGold.To(pulse * 0.45f, light);
    RenderTerrainAlphaBitmap(BITMAP_MAGIC + 1, object->Position[0], object->Position[1], 2.9f, 2.9f, light, time * 0.002f);

    if (Has(textures.pillar))
    {
        Column(object, time, main, textures.pillar);
    }
    Glow(object, 25.f, 3.0f, main, 0.4f);
    Decal(object, textures.halo, 0.55f, kWhiteGold, Pulse(time, 0.004f, 0.15f), time * 0.01f, 205.f);

    Electricity(object, time, 3, main, textures, seed);
    Flames(object, time, 10, main, textures, seed);
    Embers(object, time, 24, textures, seed);
    Ash(object, time, 18, textures, seed);
    Shockwave(object, time, main, textures, seed);
}
} // namespace

bool HandleNotice(const wchar_t* text)
{
    if (text == nullptr)
    {
        return false;
    }
    const std::wstring_view notice(text);
    if (!notice.starts_with(kPrefix))
    {
        return false;
    }

    // "#MUAURA|name|level"
    const std::wstring_view rest = notice.substr(kPrefix.size());
    const std::size_t bar = rest.find(L'|');
    if (bar == std::wstring_view::npos || bar == 0)
    {
        return true;
    }
    const std::wstring name(rest.substr(0, bar));
    const int level = static_cast<int>(std::wcstol(std::wstring(rest.substr(bar + 1)).c_str(), nullptr, 10));
    if (level <= kNone || level > kDiamante)
    {
        Levels().erase(name);
    }
    else
    {
        Levels()[name] = level;
    }
    return true;
}

void Clear()
{
    Levels().clear();
}

void Render(CHARACTER* character, OBJECT* object)
{
    if (character == nullptr || object == nullptr || !GameConfig::GetInstance().GetVipAuras())
    {
        return;
    }
    const auto found = Levels().find(character->ID);
    if (found == Levels().end() || g_isCharacterBuff(object, eBuff_Cloaking) || object->Alpha < 0.5f)
    {
        return;
    }

    const float time = static_cast<float>(WorldTime);
    // Each character's own rhythm for the random-looking parts.
    const float seed = static_cast<float>((character->Key & 0x7FFF) % 97);
    EnableAlphaBlend();
    switch (found->second)
    {
    case kPlata: RenderPlata(object, time, GetTextures()); break;
    case kOro: RenderOro(object, time, GetTextures()); break;
    case kPlatino: RenderPlatino(object, time, GetTextures(), seed); break;
    case kDiamante: RenderDiamante(object, time, GetTextures(), seed); break;
    default: break;
    }
}
} // namespace GameLogic::VipAura
