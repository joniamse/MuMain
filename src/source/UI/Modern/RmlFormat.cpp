#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/RmlFormat.h"

#include <array>
#include <charconv>

namespace UI::Modern::Format
{
namespace
{
constexpr std::size_t kNumberBufferSize = 32;
constexpr int kDigitsPerGroup = 3;
constexpr char kGroupSeparator = ',';
} // namespace

void CssPercent(double percent, int decimals, ::Rml::String& target)
{
    std::array<char, kNumberBufferSize> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size() - 1, percent,
                                      std::chars_format::fixed, decimals);
    *result.ptr = '%';
    target.assign(buffer.data(), result.ptr + 1);
}

void Grouped(std::int64_t value, ::Rml::String& target)
{
    std::array<char, kNumberBufferSize> digits{};
    const auto result = std::to_chars(digits.data(), digits.data() + digits.size(), value);
    const char* begin = digits.data();
    target.clear();
    if (*begin == '-')
    {
        target.push_back('-');
        ++begin;
    }

    const auto count = static_cast<int>(result.ptr - begin);
    for (int i = 0; i < count; ++i)
    {
        if (i > 0 && (count - i) % kDigitsPerGroup == 0)
        {
            target.push_back(kGroupSeparator);
        }
        target.push_back(begin[i]);
    }
}
} // namespace UI::Modern::Format

#endif // MU_RMLUI
