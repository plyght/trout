#include "trout/SuggestionTextNormalizer.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace trout {
namespace {

bool startsWithCaseInsensitive(const std::string& text, const std::string& prefix) {
    if (prefix.size() > text.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower((unsigned char)text[i]) != std::tolower((unsigned char)prefix[i])) return false;
    }
    return true;
}

void replaceAll(std::string& text, const std::string& needle, const std::string& replacement) {
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        text.replace(pos, needle.size(), replacement);
        pos += replacement.size();
    }
}

std::string trimNewlinesAndControls(const std::string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && (text[start] == '\n' || text[start] == '\r' || (unsigned char)text[start] < 32)) ++start;
    while (end > start && (text[end - 1] == '\n' || text[end - 1] == '\r' || (unsigned char)text[end - 1] < 32)) --end;
    return text.substr(start, end - start);
}

std::string trimWhitespace(const std::string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && std::isspace((unsigned char)text[start])) ++start;
    while (end > start && std::isspace((unsigned char)text[end - 1])) --end;
    return text.substr(start, end - start);
}

std::string dropLeadingWhitespace(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace((unsigned char)text[start])) ++start;
    return text.substr(start);
}

void stripThinkBlocks(std::string& text) {
    while (true) {
        size_t open = text.find("<think>");
        if (open == std::string::npos) return;
        size_t close = text.find("</think>", open + 7);
        if (close == std::string::npos) {
            text.erase(open);
            return;
        }
        text.erase(open, close + 8 - open);
    }
}

void stripLeadingScaffoldingLabels(std::string& text) {
    const std::vector<std::string> labels = {
        "Text before the caret:",
        "Text before caret:",
        "Text after the caret:",
        "Text after caret:",
        "User Profile Context:",
        "Your style preferences:",
        "Final instruction:",
        "Screen context:",
        "Screen content:",
        "User's clipboard:",
        "Continuation:",
        "Application:",
        "Task:",
        "App:"
    };

    while (true) {
        std::string leading = dropLeadingWhitespace(text);
        bool matched = false;
        for (const auto& label : labels) {
            if (startsWithCaseInsensitive(leading, label)) {
                text = leading.substr(label.size());
                matched = true;
                break;
            }
        }
        if (!matched) return;
    }
}

std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && std::isspace((unsigned char)text[i])) ++i;
        size_t start = i;
        while (i < text.size() && !std::isspace((unsigned char)text[i])) ++i;
        if (i > start) out.push_back(text.substr(start, i - start));
    }
    return out;
}

bool equalCaseInsensitive(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    }
    return true;
}

std::string stripEchoPrefix(const std::string& suggestion, const std::string& precedingText) {
    auto suggestionWords = words(suggestion);
    auto precedingWords = words(precedingText);
    if (suggestionWords.empty() || precedingWords.empty()) return suggestion;

    size_t maxSearchDepth = std::min<size_t>(precedingWords.size(), 15);
    size_t bestOverlap = 0;
    for (size_t depth = 1; depth <= maxSearchDepth && depth <= suggestionWords.size(); ++depth) {
        bool matches = true;
        for (size_t i = 0; i < depth; ++i) {
            if (!equalCaseInsensitive(precedingWords[precedingWords.size() - depth + i], suggestionWords[i])) {
                matches = false;
                break;
            }
        }
        if (matches) bestOverlap = depth;
    }
    if (bestOverlap == 0) return suggestion;
    if (bestOverlap >= suggestionWords.size()) return "";

    size_t pos = 0;
    for (size_t count = 0; count < bestOverlap && pos < suggestion.size(); ++count) {
        while (pos < suggestion.size() && std::isspace((unsigned char)suggestion[pos])) ++pos;
        while (pos < suggestion.size() && !std::isspace((unsigned char)suggestion[pos])) ++pos;
    }
    return suggestion.substr(pos);
}

bool duplicatesTrailingText(const std::string& suggestion, const std::string& trailingText) {
    std::string trimmedSuggestion = trimWhitespace(suggestion);
    std::string trimmedTrailing = trimWhitespace(trailingText);
    if (trimmedSuggestion.empty() || trimmedTrailing.empty()) return false;
    if (trimmedSuggestion.size() > trimmedTrailing.size()) return false;
    return startsWithCaseInsensitive(trimmedTrailing, trimmedSuggestion);
}

bool safeToInsert(const std::string& text) {
    if (trimWhitespace(text).empty()) return false;
    if (text.find("\xEF\xBF\xBD") != std::string::npos) return false;
    for (unsigned char c : text) {
        if (c < 32 && c != '\n' && c != '\t') return false;
    }
    return true;
}

}

std::string normalizeSuggestionText(const SuggestionNormalizationRequest& request) {
    std::string normalized = request.raw;
    replaceAll(normalized, "\r", "");
    replaceAll(normalized, "<|im_end|>", "");
    replaceAll(normalized, "<|im_start|>", "");
    stripThinkBlocks(normalized);

    if (!request.prompt.empty() && normalized.rfind(request.prompt, 0) == 0) {
        normalized.erase(0, request.prompt.size());
        normalized = trimNewlinesAndControls(normalized);
    }
    if (!request.precedingText.empty() && normalized.rfind(request.precedingText, 0) == 0) {
        normalized.erase(0, request.precedingText.size());
    }

    normalized = trimNewlinesAndControls(normalized);
    stripLeadingScaffoldingLabels(normalized);
    normalized = trimNewlinesAndControls(normalized);

    if (request.multiLine) {
        size_t blank = normalized.find("\n\n");
        if (blank != std::string::npos) normalized = normalized.substr(0, blank);
        normalized = trimWhitespace(normalized);
    } else {
        size_t nl = normalized.find('\n');
        if (nl != std::string::npos) normalized = normalized.substr(0, nl);
    }

    if (duplicatesTrailingText(normalized, request.trailingText)) return "";
    normalized = stripEchoPrefix(normalized, request.precedingText);

    if (!request.precedingText.empty() && std::isspace((unsigned char)request.precedingText.back())) {
        while (!normalized.empty() && std::isspace((unsigned char)normalized.front())) normalized.erase(normalized.begin());
    }

    if (!safeToInsert(normalized)) return "";
    return normalized;
}

}
