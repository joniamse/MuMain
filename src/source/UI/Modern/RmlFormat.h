#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

#include <cstdint>

// Locale-independent number formatting for values bound into RML documents. Each function
// overwrites `target`, reusing its capacity, so per-frame refreshes do not allocate.
namespace UI::Modern::Format
{
// "42.5%" - always a '.' decimal separator, valid as a CSS length.
void CssPercent(double percent, int decimals, ::Rml::String& target);

// "1,234,567"
void Grouped(std::int64_t value, ::Rml::String& target);
} // namespace UI::Modern::Format

#endif // MU_RMLUI
