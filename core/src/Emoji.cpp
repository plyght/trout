#include "trout/Emoji.h"

#include <algorithm>
#include <cctype>

#include "EmojiData.inc"

namespace trout {

// Provided by the generated table in EmojiData.inc.
extern const EmojiMatch kEmojiTable[];
extern const size_t kEmojiTableSize;

Emoji::Emoji() {
    table_.assign(kEmojiTable, kEmojiTable + kEmojiTableSize);
}

std::string Emoji::activeQuery(const std::string& textBeforeCursor) {
    // Find the last ':' and ensure what follows is a valid in-progress token
    // (letters, digits, underscore, no whitespace) with at least one char.
    size_t colon = textBeforeCursor.rfind(':');
    if (colon == std::string::npos) return "";
    std::string tok = textBeforeCursor.substr(colon + 1);
    if (tok.empty()) return "";
    for (char c : tok) {
        if (!(std::isalnum((unsigned char)c) || c == '_' || c == '+' || c == '-'))
            return "";
    }
    // Require the colon to be at start or preceded by whitespace/punctuation so
    // we don't fire inside URLs like http://
    if (colon > 0) {
        char prev = textBeforeCursor[colon - 1];
        if (prev == '/' || std::isalnum((unsigned char)prev)) return "";
    }
    return tok;
}

std::vector<EmojiMatch> Emoji::search(const std::string& query, size_t limit) const {
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    std::vector<std::pair<int, const EmojiMatch*>> ranked;
    for (const auto& e : table_) {
        auto pos = e.shortcode.find(q);
        if (pos == std::string::npos) continue;
        int score = 0;
        if (e.shortcode == q) score = 100;
        else if (pos == 0) score = 50 - (int)(e.shortcode.size() - q.size());
        else score = 20 - (int)pos;
        ranked.emplace_back(score, &e);
    }
    std::stable_sort(ranked.begin(), ranked.end(),
                     [](auto& a, auto& b){ return a.first > b.first; });

    std::vector<EmojiMatch> out;
    for (auto& r : ranked) {
        out.push_back(*r.second);
        if (out.size() >= limit) break;
    }
    return out;
}

std::string Emoji::applySkinTone(const std::string& glyph, const std::string& tone) {
    // Skin-tone modifiers (Fitzpatrick). Appended after the base emoji.
    static const std::pair<const char*, const char*> tones[] = {
        {"light", "\U0001F3FB"},
        {"medium-light", "\U0001F3FC"},
        {"medium", "\U0001F3FD"},
        {"medium-dark", "\U0001F3FE"},
        {"dark", "\U0001F3FF"},
    };
    if (tone.empty() || tone == "default") return glyph;
    for (auto& t : tones) {
        if (tone == t.first) return glyph + t.second;
    }
    return glyph;
}

} // namespace trout
