#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/ChatDataModel.h"

#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/NewUIChatInputBox.h"

#include <array>

namespace UI::Modern
{
namespace
{
constexpr std::array kTypeLabels = {"Todos", "Grupo", "Guild", "Gens"};

} // namespace

bool ChatDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("chat");
    if (!constructor)
    {
        return false;
    }

    WindowView::RegisterBox(constructor);

    constructor.Bind("visible", &m_view.visible);
    constructor.Bind("panel_left", &m_view.panelLeft);
    constructor.Bind("panel_top", &m_view.panelTop);
    constructor.Bind("panel_width", &m_view.panelWidth);
    constructor.Bind("panel_height", &m_view.panelHeight);
    constructor.Bind("whisper_off", &m_view.whisperOff);
    constructor.Bind("whisper_field", &m_view.whisperField);
    constructor.Bind("chat_field", &m_view.chatField);
    constructor.Bind("buttons", &m_view.buttons);
    m_handle = constructor.GetModelHandle();
    return true;
}

void ChatDataModel::Refresh()
{
    if (!m_handle)
    {
        return;
    }

    View& view = m_scratch;
    view = View{};
    SEASON3B::CNewUIChatInputBox* input = g_pNewUISystem != nullptr ? g_pChatInputBox : nullptr;
    view.visible = input != nullptr && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX);
    if (view.visible)
    {
        SEASON3B::CNewUIChatInputBox::ModernLayout layout;
        input->GetModernLayout(layout);
        const WindowView::Mapper mapper(*input, POINT{layout.window.left, layout.window.top});
        const Box window = mapper.Map(layout.window);
        view.panelLeft = mapper.OriginX();
        view.panelTop = mapper.OriginY();
        view.panelWidth = window.width;
        view.panelHeight = window.height;
        view.whisperOff = layout.whisperOff;
        view.whisperField = mapper.Map(layout.whisperField);
        view.chatField = mapper.Map(layout.chatField);

        const auto add = [&](const SEASON3B::CNewUIChatInputBox::ModernLayout::Button& source, const char* kind,
                             const char* label) {
            Box box = mapper.Map(source.rect);
            box.kind = kind;
            box.label = label;
            box.active = source.active;
            view.buttons.push_back(std::move(box));
        };
        for (std::size_t i = 0; i < kTypeLabels.size(); ++i)
        {
            add(layout.types[i], "type", kTypeLabels[i]);
        }
        add(layout.blockWhisper, "toggle block", "Priv");
        add(layout.systemMessages, "toggle", "Sist");
        add(layout.chatLog, "toggle", "Chat");
        add(layout.frame, "toggle", "Marco");
        if (layout.frameShown)
        {
            add(layout.size, "toggle", "Tam");
            add(layout.transparency, "toggle", "Alfa");
        }
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
