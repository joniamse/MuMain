#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/ChatDataModel.h"

#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/NewUIChatInputBox.h"
#include "UI/Scaling/UITransform.h"

#include <array>

extern unsigned int WindowWidth;
extern unsigned int WindowHeight;

namespace UI::Modern
{
namespace
{
constexpr std::array kTypeLabels = {"Todos", "Grupo", "Guild", "Gens"};

struct Mapper
{
    UI::Scaling::Transform transform;
    float originX;
    float originY;

    ChatDataModel::Box Map(const RECT& rect) const
    {
        ChatDataModel::Box box;
        box.left = UI::Scaling::PositionX(transform, static_cast<float>(rect.left)) - originX;
        box.top = UI::Scaling::PositionY(transform, static_cast<float>(rect.top)) - originY;
        box.width = (rect.right - rect.left) * transform.scaleX;
        box.height = (rect.bottom - rect.top) * transform.scaleY;
        return box;
    }
};
} // namespace

bool ChatDataModel::Bind(::Rml::Context& context)
{
    ::Rml::DataModelConstructor constructor = context.CreateDataModel("chat");
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
        box.RegisterMember("active", &Box::active);
    }
    constructor.RegisterArray<std::vector<Box>>();

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
        const auto transform = UI::Scaling::TransformForLayout(input->GetLayoutMode(), static_cast<int>(WindowWidth),
                                                               static_cast<int>(WindowHeight));
        const Mapper mapper{transform, UI::Scaling::PositionX(transform, static_cast<float>(layout.window.left)),
                            UI::Scaling::PositionY(transform, static_cast<float>(layout.window.top))};
        const Box window = mapper.Map(layout.window);
        view.panelLeft = mapper.originX;
        view.panelTop = mapper.originY;
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
