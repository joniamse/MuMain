#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/InventoryDataModel.h"

#include "Engine/Object/ZzzInfomation.h"
#include "UI/Modern/RmlFormat.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Scaling/UITransform.h"

#include <array>

extern unsigned int WindowWidth;
extern unsigned int WindowHeight;

namespace UI::Modern
{
namespace
{
// Shown in an empty equipment slot, by slot number.
constexpr std::array<const char*, MAX_EQUIPMENT_INDEX> kSlotLabels = {
    "Arma",    // EQUIPMENT_WEAPON_RIGHT
    "Escudo",  // EQUIPMENT_WEAPON_LEFT
    "Casco",   // EQUIPMENT_HELM
    "Armadura", // EQUIPMENT_ARMOR
    "Pantalón", // EQUIPMENT_PANTS
    "Guantes", // EQUIPMENT_GLOVES
    "Botas",   // EQUIPMENT_BOOTS
    "Alas",    // EQUIPMENT_WING
    "Mascota", // EQUIPMENT_HELPER
    "Collar",  // EQUIPMENT_AMULET
    "Anillo",  // EQUIPMENT_RING_RIGHT
    "Anillo",  // EQUIPMENT_RING_LEFT
};

const char* ItemKind(const ITEM& item)
{
    if (item.AncientDiscriminator > 0)
    {
        return "ancient";
    }
    if (item.ExcellentFlags > 0)
    {
        return "excellent";
    }
    return item.Level >= 7 ? "high" : "normal";
}

// Same tiers as the legacy zen colours (getGoldColor).
const char* ZenTier(DWORD zen)
{
    if (zen >= 10000000)
    {
        return "max";
    }
    if (zen >= 1000000)
    {
        return "high";
    }
    return zen >= 100000 ? "mid" : "low";
}

// Window-local rectangles of the inventory to pixels relative to its panel.
struct Mapper
{
    UI::Scaling::Transform transform;
    float originX;
    float originY;

    InventoryDataModel::Box Map(const RECT& rect) const
    {
        InventoryDataModel::Box box;
        box.left = UI::Scaling::PositionX(transform, static_cast<float>(rect.left)) - originX;
        box.top = UI::Scaling::PositionY(transform, static_cast<float>(rect.top)) - originY;
        box.width = (rect.right - rect.left) * transform.scaleX;
        box.height = (rect.bottom - rect.top) * transform.scaleY;
        return box;
    }
};

void AddButton(std::vector<InventoryDataModel::Box>& buttons, const Mapper& mapper,
               const SEASON3B::CNewUIMyInventory::ModernLayout::Box& source, const char* kind, const char* label,
               const char* tip, bool active = false, bool locked = false)
{
    if (!source.shown)
    {
        return;
    }
    InventoryDataModel::Box box = mapper.Map(source.rect);
    box.kind = kind;
    box.label = label;
    box.tip = tip;
    box.active = active;
    box.locked = locked;
    buttons.push_back(std::move(box));
}
} // namespace

bool InventoryDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("inventory");
    if (!constructor)
    {
        return false;
    }

    if (auto box = constructor.RegisterStruct<Box>())
    {
        box.RegisterMember("left", &Box::left);
        box.RegisterMember("top", &Box::top);
        box.RegisterMember("width", &Box::width);
        box.RegisterMember("height", &Box::height);
        box.RegisterMember("kind", &Box::kind);
        box.RegisterMember("label", &Box::label);
        box.RegisterMember("tip", &Box::tip);
        box.RegisterMember("filled", &Box::filled);
        box.RegisterMember("active", &Box::active);
        box.RegisterMember("locked", &Box::locked);
    }
    constructor.RegisterArray<std::vector<Box>>();
    constructor.RegisterArray<std::vector<int>>();

    constructor.Bind("visible", &m_view.visible);
    constructor.Bind("panel_left", &m_view.panelLeft);
    constructor.Bind("panel_top", &m_view.panelTop);
    constructor.Bind("panel_width", &m_view.panelWidth);
    constructor.Bind("panel_height", &m_view.panelHeight);
    constructor.Bind("zen", &m_view.zen);
    constructor.Bind("zen_tier", &m_view.zenTier);
    constructor.Bind("set_active", &m_view.setActive);
    constructor.Bind("socket_active", &m_view.socketActive);
    constructor.Bind("grid", &m_view.grid);
    constructor.Bind("cells", &m_view.cells);
    constructor.Bind("items", &m_view.items);
    constructor.Bind("slots", &m_view.slots);
    constructor.Bind("buttons", &m_view.buttons);
    m_handle = constructor.GetModelHandle();
    return true;
}

void InventoryDataModel::Refresh()
{
    if (!m_handle)
    {
        return;
    }

    View& view = m_scratch;
    view = View{};
    SEASON3B::CNewUIMyInventory* inventory = g_pMyInventory;
    SEASON3B::CNewUIInventoryCtrl* grid = inventory != nullptr ? inventory->GetInventoryCtrl() : nullptr;
    view.visible = grid != nullptr && CharacterMachine != nullptr && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY);
    if (view.visible)
    {
        const auto transform = UI::Scaling::TransformForLayout(inventory->GetLayoutMode(), static_cast<int>(WindowWidth),
                                                               static_cast<int>(WindowHeight));
        const POINT& pos = inventory->GetPos();
        const Mapper mapper{transform, UI::Scaling::PositionX(transform, static_cast<float>(pos.x)),
                            UI::Scaling::PositionY(transform, static_cast<float>(pos.y))};
        view.panelLeft = mapper.originX;
        view.panelTop = mapper.originY;
        view.panelWidth = SEASON3B::CNewUIMyInventory::GetWidth() * transform.scaleX;
        view.panelHeight = SEASON3B::CNewUIMyInventory::GetHeight() * transform.scaleY;

        const DWORD zen = CharacterMachine->Gold;
        Format::Grouped(zen, view.zen);
        view.zenTier = ZenTier(zen);

        SEASON3B::CNewUIMyInventory::ModernLayout layout;
        inventory->GetModernLayout(layout);
        view.setActive = layout.setOptionActive;
        view.socketActive = layout.socketOptionActive;

        // The grid and the items in it.
        const POINT& gridPos = grid->GetPos();
        const int columns = grid->GetNumberOfColumn();
        const int rows = grid->GetNumberOfRow();
        constexpr int square = SEASON3B::INVENTORY_SQUARE_WIDTH;
        view.grid = mapper.Map(RECT{gridPos.x, gridPos.y, gridPos.x + columns * square, gridPos.y + rows * square});
        view.cells.assign(static_cast<std::size_t>(columns * rows), 0);
        const float cellX = square * transform.scaleX;
        const float cellY = square * transform.scaleY;
        for (std::size_t i = 0; i < grid->GetNumberOfItems(); ++i)
        {
            const ITEM* item = grid->GetItem(static_cast<int>(i));
            if (item == nullptr || item->Type < 0)
            {
                continue;
            }
            const ITEM_ATTRIBUTE& attribute = ItemAttribute[item->Type];
            Box box;
            box.left = item->x * cellX;
            box.top = item->y * cellY;
            box.width = std::max<int>(1, attribute.Width) * cellX;
            box.height = std::max<int>(1, attribute.Height) * cellY;
            box.kind = ItemKind(*item);
            view.items.push_back(std::move(box));
        }

        for (int i = 0; i < MAX_EQUIPMENT_INDEX; ++i)
        {
            if (!layout.equipment[i].shown)
            {
                continue;
            }
            Box box = mapper.Map(layout.equipment[i].rect);
            const ITEM& item = CharacterMachine->Equipment[i];
            box.filled = item.Type >= 0;
            box.kind = box.filled ? ItemKind(item) : "empty";
            box.label = kSlotLabels[static_cast<std::size_t>(i)];
            view.slots.push_back(std::move(box));
        }

        AddButton(view.buttons, mapper, layout.exit, "exit", "Cerrar", "Cerrar (I / V)");
        AddButton(view.buttons, mapper, layout.repair, "repair", "Reparar", "Reparar items (L)", layout.repairing);
        AddButton(view.buttons, mapper, layout.myShop, "shop", "Tienda",
                  layout.myShopCloses ? "Cerrar tienda personal (S)" : "Abrir tienda personal (S)", layout.myShopCloses,
                  layout.myShopLocked);
        AddButton(view.buttons, mapper, layout.expand, "expand", "Extra", "Inventario extendido (K)");
    }

    if (m_scratch == m_view)
    {
        return;
    }
    std::swap(m_scratch, m_view);
    m_handle.DirtyAllVariables();
}
} // namespace UI::Modern

#endif // MU_RMLUI
