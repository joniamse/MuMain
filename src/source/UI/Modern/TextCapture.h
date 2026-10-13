#pragma once

#include "UI/Legacy/UIControls.h"

#include <string>
#include <vector>

namespace UI::Modern
{
// Records what legacy UI code would draw through g_pRenderText instead of drawing it, so a
// modern window can present the same, already formatted, values. Use with
// CUIRenderText::SetCaptureTarget around a synchronous call.
class TextCapture final : public IUIRenderText
{
public:
    struct Line
    {
        int x;
        int y;
        std::wstring text;
        BYTE red;
        BYTE green;
        BYTE blue;
        bool bold;
    };

    void Clear()
    {
        m_lines.clear();
    }
    [[nodiscard]] const std::vector<Line>& Lines() const
    {
        return m_lines;
    }

    bool Create(HDC) override
    {
        return true;
    }
    void Release() override {}

    DWORD GetTextColor() const override;
    DWORD GetBgColor() const override;
    void SetTextColor(BYTE red, BYTE green, BYTE blue, BYTE alpha) override;
    void SetTextColor(DWORD color) override;
    void SetBgColor(BYTE red, BYTE green, BYTE blue, BYTE alpha) override;
    void SetBgColor(DWORD color) override;
    void SetFont(HFONT font) override;
    SIZE MeasureText(const wchar_t* text, int length) const override;
    void RenderText(int x, int y, const wchar_t* text, int boxWidth = 0, int boxHeight = 0, int sort = RT3_SORT_LEFT,
                    OUT SIZE* textSize = nullptr) override;

private:
    std::vector<Line> m_lines;
    DWORD m_textColor = 0xFFFFFFFFu;
    DWORD m_bgColor = 0;
    bool m_bold = false;
};
} // namespace UI::Modern
