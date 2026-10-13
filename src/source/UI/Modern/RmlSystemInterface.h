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
    // Leaves "legacy:<index>" texture sources untouched instead of resolving them as paths.
    void JoinPath(::Rml::String& translatedPath, const ::Rml::String& documentPath,
                  const ::Rml::String& path) override;
};
} // namespace UI::Modern

#endif // MU_RMLUI
