#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/InventoryDataModel.h"

#include "Engine/Object/ZzzInfomation.h"
#include "UI/Modern/RmlFormat.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUISystem.h"

#include <array>

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

// The classic warning tints of equipped items, as style classes.
const char* WarningKind(unsigned int color)
{
    switch (color)
    {
    case 0: return "";
    case 0x40FFFF00u: return "worn";       // half durability
    case 0x40FF8000u: return "worn-more";  // 30%
    default: return "broken";              // 20%, none left or unusable
    }
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

void AddButton(std::vector<InventoryDataModel::Box>& buttons, const WindowView::Mapper& mapper,
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

    WindowView::RegisterBox(constructor);
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
        const WindowView::Mapper mapper(*inventory, inventory->GetPos());
        view.panelLeft = mapper.OriginX();
        view.panelTop = mapper.OriginY();
        view.panelWidth = SEASON3B::CNewUIMyInventory::GetWidth() * mapper.ScaleX();
        view.panelHeight = SEASON3B::CNewUIMyInventory::GetHeight() * mapper.ScaleY();

        const DWORD zen = CharacterMachine->Gold;
        Format::Grouped(zen, view.zen);
        view.zenTier = ZenTier(zen);

        SEASON3B::CNewUIMyInventory::ModernLayout layout;
        inventory->GetModernLayout(layout);
        view.setActive = layout.setOptionActive;
        view.socketActive = layout.socketOptionActive;
        WindowView::DescribeGrid(*grid, mapper, view.grid, view.cells, view.items);

        for (int i = 0; i < MAX_EQUIPMENT_INDEX; ++i)
        {
            if (!layout.equipment[i].shown)
            {
                continue;
            }
            Box box = mapper.Map(layout.equipment[i].rect);
            const ITEM& item = CharacterMachine->Equipment[i];
            box.filled = item.Type >= 0;
            box.kind = box.filled ? WindowView::ItemKind(item) : "empty";
            box.warning = WarningKind(layout.equipmentWarning[i]);
            box.label = kSlotLabels[static_cast<std::size_t>(i)];
            view.slots.push_back(std::move(box));
        }

        AddButton(view.buttons, mapper, layout.exit, "exit", "×", "Cerrar (I / V)");
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
