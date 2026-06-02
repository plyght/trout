#pragma once

#include <string>

namespace trout {

struct SuggestionNormalizationRequest {
    std::string raw;
    std::string prompt;
    std::string precedingText;
    std::string trailingText;
    bool multiLine = false;
};

std::string normalizeSuggestionText(const SuggestionNormalizationRequest& request);

}
