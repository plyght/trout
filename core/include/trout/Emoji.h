#pragma once

#include <string>
#include <vector>

namespace trout {

struct EmojiMatch {
    std::string shortcode; // e.g. "sweat_smile"
    std::string glyph;     // the emoji character(s)
};

// Colon-triggered emoji search. Detects a trailing ":name" token in the text
// before the cursor and returns ranked matches. Skin-tone/gender variants are
// applied by the caller using the user's preferences.
class Emoji {
public:
    Emoji();

    // If text ends with an active ":query" token, returns the query (without
    // the leading colon). Returns empty string if no active emoji trigger.
    static std::string activeQuery(const std::string& textBeforeCursor);

    std::vector<EmojiMatch> search(const std::string& query, size_t limit = 8) const;

    // Apply skin tone ("default","light","medium-light","medium","medium-dark","dark")
    // to a base glyph when applicable.
    static std::string applySkinTone(const std::string& glyph, const std::string& tone);

private:
    std::vector<EmojiMatch> table_;
};

} // namespace trout
