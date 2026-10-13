#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/ShopDataModel.h"

#include "UI/NewUI/Inventory/NewUIMyShopInventory.h"
#include "UI/NewUI/Inventory/NewUIPurchaseShopInventory.h"
#include "UI/NewUI/NewUISystem.h"

namespace UI::Modern
{
namespace
{
WindowView::Box Button(const WindowView::Mapper& mapper, SEASON3B::CNewUIButton& button, const char* kind,
                       const char* label, const char* tip)
{
    const POINT& pos = button.GetPos();
    const POINT& size = button.GetSize();
    WindowView::Box box = mapper.Map(RECT{pos.x, pos.y, pos.x + size.x, pos.y + size.y});
    box.kind = kind;
    box.label = label;
    box.tip = tip;
    box.locked = button.IsLock();
    return box;
}

// Fills `view` from a shop window; returns the mapper of its panel.
template <typename Window>
WindowView::Mapper DescribePanel(Window& window, ShopDataModel::ShopView& view)
{
    const WindowView::Mapper mapper(window, window.GetPos());
    view.panelLeft = mapper.OriginX();
    view.panelTop = mapper.OriginY();
    view.panelWidth = Window::GetWidth() * mapper.ScaleX();
    view.panelHeight = Window::GetHeight() * mapper.ScaleY();
    const POINT& pos = window.GetPos();
    // The title field of the classic frame: 169 x 26 at (12, 49).
    view.titleField = mapper.Map(RECT{pos.x + 12, pos.y + 49, pos.x + 12 + 169, pos.y + 49 + 26});
    if (SEASON3B::CNewUIInventoryCtrl* grid = window.GetInventoryCtrl())
    {
        WindowView::DescribeGrid(*grid, mapper, view.grid, view.cells, view.items);
    }
    return mapper;
}

void Commit(::Rml::DataModelHandle& handle, ShopDataModel::ShopView& current, ShopDataModel::ShopView& next,
            const char* name)
{
    if (current == next)
    {
        return;
    }
    std::swap(current, next);
    handle.DirtyVariable(name);
}
} // namespace

bool ShopDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("shop");
    if (!constructor)
    {
        return false;
    }

    WindowView::RegisterBox(constructor);
    constructor.RegisterArray<std::vector<int>>();
    if (auto shop = constructor.RegisterStruct<ShopView>())
    {
        shop.RegisterMember("visible", &ShopView::visible);
        shop.RegisterMember("panel_left", &ShopView::panelLeft);
        shop.RegisterMember("panel_top", &ShopView::panelTop);
        shop.RegisterMember("panel_width", &ShopView::panelWidth);
        shop.RegisterMember("panel_height", &ShopView::panelHeight);
        shop.RegisterMember("open", &ShopView::open);
        shop.RegisterMember("title_field", &ShopView::titleField);
        shop.RegisterMember("grid", &ShopView::grid);
        shop.RegisterMember("cells", &ShopView::cells);
        shop.RegisterMember("items", &ShopView::items);
        shop.RegisterMember("buttons", &ShopView::buttons);
    }
    constructor.Bind("mine", &m_mine);
    constructor.Bind("other", &m_other);
    m_handle = constructor.GetModelHandle();
    return true;
}

void ShopDataModel::Refresh()
{
    if (!m_handle || g_pNewUISystem == nullptr)
    {
        return;
    }

    ShopView mine;
    SEASON3B::CNewUIMyShopInventory* myShop = g_pMyShopInventory;
    mine.visible = myShop != nullptr && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MYSHOP_INVENTORY);
    if (mine.visible)
    {
        const WindowView::Mapper mapper = DescribePanel(*myShop, mine);
        mine.open = myShop->IsEnablePersonalShop();
        mine.buttons.push_back(Button(mapper, myShop->GetExitButton(), "exit", "×", "Cerrar ventana"));
        mine.buttons.push_back(Button(mapper, myShop->GetOpenButton(), "open", mine.open ? "Aplicar" : "Abrir tienda",
                                      mine.open ? "Aplicar los cambios de la tienda" : "Abrir la tienda"));
        mine.buttons.push_back(Button(mapper, myShop->GetCloseButton(), "close", "Cerrar tienda", "Cerrar la tienda"));
    }
    Commit(m_handle, m_mine, mine, "mine");

    ShopView other;
    SEASON3B::CNewUIPurchaseShopInventory* purchaseShop = g_pPurchaseShopInventory;
    other.visible = purchaseShop != nullptr && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_PURCHASESHOP_INVENTORY);
    if (other.visible)
    {
        const WindowView::Mapper mapper = DescribePanel(*purchaseShop, other);
        const POINT& pos = purchaseShop->GetPos();
        // The close button: in the title bar with the modern look (see its SyncLayout).
        WindowView::Box exit = mapper.Map(RECT{pos.x + 166, pos.y + 7, pos.x + 183, pos.y + 24});
        exit.kind = "exit";
        exit.label = "×";
        exit.tip = "Cerrar ventana";
        other.buttons.push_back(std::move(exit));
    }
    Commit(m_handle, m_other, other, "other");
}
} // namespace UI::Modern

#endif // MU_RMLUI
