#include "trout/TypoSuppression.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace trout {

bool suspectedTypo(const std::string& textBeforeCursor) {
    size_t end = textBeforeCursor.size();
    while (end > 0 && std::isspace((unsigned char)textBeforeCursor[end - 1])) --end;
    size_t start = end;
    while (start > 0 && !std::isspace((unsigned char)textBeforeCursor[start - 1])) --start;
    std::string word = textBeforeCursor.substr(start, end - start);
    if (word.size() < 4) return false;

    int repeat = 1, maxRepeat = 1;
    bool hasVowel = false;
    for (size_t i = 0; i < word.size(); ++i) {
        char c = (char)std::tolower((unsigned char)word[i]);
        if (std::strchr("aeiou", c)) hasVowel = true;
        if (i > 0 && word[i] == word[i - 1]) { if (++repeat > maxRepeat) maxRepeat = repeat; }
        else repeat = 1;
    }
    bool alpha = std::all_of(word.begin(), word.end(),
                             [](unsigned char c){ return std::isalpha(c); });
    if (!alpha) return false;
    return (!hasVowel && word.size() >= 5) || maxRepeat >= 3;
}

}
