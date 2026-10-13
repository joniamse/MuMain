#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

#include <vector>

namespace UI::Modern
{
// Data model "potions": the item hotkeys Q, W, E and R, with the icon of the item each one uses
// (see ItemIconAtlas) and how many of them are in the inventory.
//
// RML names:
//   slots[] PotionView { hotkey, key, filled, icon, rect, count, has_count }
//           icon/rect feed <img data-attr-src data-attr-rect>
// Events:
//   use(hotkey)   uses the item, like pressing the key
class PotionBarDataModel
{
public:
    struct PotionView
    {
        int hotkey = 0;
        ::Rml::String key;
        bool filled = false;
        ::Rml::String icon;
        ::Rml::String rect;
        ::Rml::String count;
        bool hasCount = false;

        bool operator==(const PotionView&) const = default;
    };

    bool Bind(::Rml::Context& context);
    void Refresh();

private:
    ::Rml::DataModelHandle m_handle;
    std::vector<PotionView> m_slots;
    std::vector<PotionView> m_scratch;
};
} // namespace UI::Modern

#endif // MU_RMLUI
