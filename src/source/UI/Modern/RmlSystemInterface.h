#pragma once

#ifdef MU_RMLUI

#include "UI/Modern/RmlIncludes.h"

namespace UI::Modern
{
// Supplies RmlUi with the game clock and routes its log output to the client log.
class RmlSystemInterface final : public ::Rml::SystemInterface
{
public:
    double GetElapsedTime() override;
    bool LogMessage(::Rml::Log::Type type, const ::Rml::String& message) override;
};
} // namespace UI::Modern

#endif // MU_RMLUI
