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

// The textures of tools/aura_textures.py, loaded on first use; BITMAP_UNKNOWN if missing.
struct Textures
{
    GLuint sigil = BITMAP_UNKNOWN;
    GLuint halo = BITMAP_UNKNOWN;
    GLuint beam = BITMAP_UNKNOWN;
    GLuint orb = BITMAP_UNKNOWN;
};

const Textures& GetTextures()
{
    static const Textures textures = [] {
        Textures loaded;
        loaded.sigil = Bitmaps.LoadImage(L"Data\\Effect\\Modern\\aura_sigil.jpg", GL_LINEAR, GL_CLAMP_TO_EDGE);
        loaded.halo = Bitmaps.LoadImage(L"Data\\Effect\\Modern\\aura_halo.jpg", GL_LINEAR, GL_CLAMP_TO_EDGE);
        loaded.beam = Bitmaps.LoadImage(L"Data\\Effect\\Modern\\aura_beam.jpg", GL_LINEAR, GL_CLAMP_TO_EDGE);
        loaded.orb = Bitmaps.LoadImage(L"Data\\Effect\\Modern\\aura_orb.jpg", GL_LINEAR, GL_CLAMP_TO_EDGE);
        return loaded;
    }();
    return textures;
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

// Orbs circling the chest, each with a fading trail behind it.
void Orbs(OBJECT* object, float time, int count, float radius, float height, const Color& color, GLuint texture)
{
    constexpr int kTrail = 5;
    vec3_t position, light;
    for (int i = 0; i < count; ++i)
    {
        const float base = static_cast<float>(i) * kTwoPi / static_cast<float>(count);
        for (int t = 0; t < kTrail; ++t)
        {
            const float angle = base + time * 0.0025f - static_cast<float>(t) * 0.12f;
            position[0] = object->Position[0] + std::cosf(angle) * radius;
            position[1] = object->Position[1] + std::sinf(angle) * radius;
            // A gentle bob, different for each orb.
            position[2] = object->Position[2] + height + 18.f * std::sinf(time * 0.002f + base);
            const float fade = 1.f - static_cast<float>(t) / kTrail;
            color.To(fade * (t == 0 ? 1.f : 0.6f), light);
            CreateSprite(static_cast<int>(texture), position, (t == 0 ? 0.55f : 0.4f) * fade + 0.1f, light, object);
        }
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
        Glow(object, 40.f + static_cast<float>(i) * 55.f, 2.6f, color, 0.28f * fade * flicker, texture);
    }
}

void RenderPlata(OBJECT* object, float time)
{
    vec3_t light;
    kSilver.To(Pulse(time), light);
    RenderTerrainAlphaBitmap(BITMAP_GM_AURORA, object->Position[0], object->Position[1], 1.15f, 1.15f, light, time * 0.01f);
    Glow(object, 25.f, 2.2f, kSilver, 0.3f);
}

void RenderOro(OBJECT* object, float time)
{
    vec3_t light;
    const float pulse = Pulse(time);
    kGold.To(pulse, light);
    RenderTerrainAlphaBitmap(BITMAP_GM_AURORA, object->Position[0], object->Position[1], 1.3f, 1.3f, light, time * 0.01f);
    kGold.To(pulse * 0.8f, light);
    RenderTerrainAlphaBitmap(BITMAP_GM_AURORA, object->Position[0], object->Position[1], 0.9f, 0.9f, light, -time * 0.014f);
    Glow(object, 25.f, 2.6f, kGold, 0.35f);
    Motes(object, time, 5, kGold, BITMAP_LIGHT, 0.45f, 170.f);
}

void RenderPlatino(OBJECT* object, float time, const Textures& textures)
{
    vec3_t light;
    const float pulse = Pulse(time);
    kCyan.To(pulse * 0.9f, light);
    if (Has(textures.sigil))
    {
        RenderTerrainAlphaBitmap(textures.sigil, object->Position[0], object->Position[1], 1.6f, 1.6f, light, time * 0.006f);
    }
    kCyan.To(pulse * 0.7f, light);
    RenderTerrainAlphaBitmap(BITMAP_GM_AURORA, object->Position[0], object->Position[1], 1.0f, 1.0f, light, -time * 0.014f);
    Glow(object, 25.f, 2.8f, kCyan, 0.4f);
    Motes(object, time, 7, kCyan, BITMAP_LIGHT, 0.45f, 190.f);
    if (Has(textures.orb))
    {
        Orbs(object, time, 2, 60.f, 110.f, kCyan, textures.orb);
    }
}

// The most glorious: a prismatic sigil turning on the ground over a second ring of runes, a
// column of light, a halo over the head, three orbs with trails and rising star sparkles.
void RenderDiamante(OBJECT* object, float time, const Textures& textures)
{
    vec3_t light;
    const float pulse = Pulse(time, 0.0025f, 0.2f);
    const Color main = Prismatic(time);
    const Color second = Prismatic(time, 2.f);

    if (Has(textures.sigil))
    {
        main.To(pulse, light);
        RenderTerrainAlphaBitmap(textures.sigil, object->Position[0], object->Position[1], 2.1f, 2.1f, light, time * 0.005f);
        second.To(pulse * 0.6f, light);
        RenderTerrainAlphaBitmap(textures.sigil, object->Position[0], object->Position[1], 1.25f, 1.25f, light, -time * 0.009f);
    }
    kWhiteGold.To(pulse * 0.5f, light);
    RenderTerrainAlphaBitmap(BITMAP_MAGIC + 1, object->Position[0], object->Position[1], 2.9f, 2.9f, light, time * 0.002f);

    if (Has(textures.beam))
    {
        Column(object, time, main, textures.beam);
    }
    Glow(object, 25.f, 3.4f, main, 0.55f);

    if (Has(textures.halo))
    {
        kWhiteGold.To(Pulse(time, 0.004f, 0.15f), light);
        RenderTerrainAlphaBitmap(textures.halo, object->Position[0], object->Position[1], 0.55f, 0.55f, light, time * 0.01f,
                                 1.f, 205.f);
    }

    if (Has(textures.orb))
    {
        Orbs(object, time, 3, 70.f, 120.f, second, textures.orb);
    }
    Motes(object, time, 12, kWhiteGold, BITMAP_SHINY, 0.6f, 230.f);
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
    EnableAlphaBlend();
    switch (found->second)
    {
    case kPlata: RenderPlata(object, time); break;
    case kOro: RenderOro(object, time); break;
    case kPlatino: RenderPlatino(object, time, GetTextures()); break;
    case kDiamante: RenderDiamante(object, time, GetTextures()); break;
    default: break;
    }
}
} // namespace GameLogic::VipAura
