#pragma once

#include "trout/Engine.h"
#include "trout/Settings.h"

#include <optional>
#include <string>

namespace trout {

struct PromptRequest {
    std::string prompt;
};

PromptRequest makePromptRequest(
    const CompletionContext& context,
    const GlobalSettings& globalSettings,
    const std::optional<AppProfile>& appProfile
);

}
