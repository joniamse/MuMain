#include "stdafx.h"

#include "UI/Modern/TextCapture.h"

#include "App/Platform/Windows/Winmain.h"

namespace UI::Modern
{
DWORD TextCapture::GetTextColor() const
{
    return m_textColor;
}

DWORD TextCapture::GetBgColor() const
{
    return m_bgColor;
}

void TextCapture::SetTextColor(BYTE red, BYTE green, BYTE blue, BYTE alpha)
{
    m_textColor = RGBA(red, green, blue, alpha);
}

void TextCapture::SetTextColor(DWORD color)
{
    m_textColor = color;
}

void TextCapture::SetBgColor(BYTE red, BYTE green, BYTE blue, BYTE alpha)
{
    m_bgColor = RGBA(red, green, blue, alpha);
}

void TextCapture::SetBgColor(DWORD color)
{
    m_bgColor = color;
}

void TextCapture::SetFont(HFONT font)
{
    m_bold = font == g_hFontBold || font == g_hFontBig;
}

// Layout is done by the modern document, so captured text needs no measurement.
SIZE TextCapture::MeasureText(const wchar_t* /*text*/, int /*length*/) const
{
    return SIZE{0, 0};
}

void TextCapture::RenderText(int x, int y, const wchar_t* text, int /*boxWidth*/, int /*boxHeight*/, int /*sort*/,
                             OUT SIZE* textSize)
{
    if (textSize)
    {
        *textSize = SIZE{0, 0};
    }
    if (!text || text[0] == L'\0')
    {
        return;
    }
    m_lines.push_back({x, y, text, GetRed(m_textColor), GetGreen(m_textColor), GetBlue(m_textColor), m_bold});
}
} // namespace UI::Modern
