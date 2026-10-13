#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/RmlSystemInterface.h"

#include "Core/Utilities/Log/MuLogger.h"
#include "UI/Modern/RmlRenderInterface.h"

#include <SDL3/SDL_timer.h>

namespace UI::Modern
{
namespace
{
constexpr double kMillisecondsPerSecond = 1000.0;
}

double RmlSystemInterface::GetElapsedTime()
{
    return static_cast<double>(SDL_GetTicks()) / kMillisecondsPerSecond;
}

bool RmlSystemInterface::LogMessage(::Rml::Log::Type type, const ::Rml::String& message)
{
    const auto logger = mu::log::Get("rmlui");
    switch (type)
    {
    case ::Rml::Log::LT_ERROR:
    case ::Rml::Log::LT_ASSERT:
        logger->error("{}", message);
        break;
    case ::Rml::Log::LT_WARNING:
        logger->warn("{}", message);
        break;
    default:
        logger->info("{}", message);
        break;
    }
    return true;
}

void RmlSystemInterface::JoinPath(::Rml::String& translatedPath, const ::Rml::String& documentPath,
                                  const ::Rml::String& path)
{
    if (path.rfind(kLegacyTextureScheme, 0) == 0)
    {
        translatedPath = path;
        return;
    }
    ::Rml::SystemInterface::JoinPath(translatedPath, documentPath, path);
}
} // namespace UI::Modern

#endif // MU_RMLUI
