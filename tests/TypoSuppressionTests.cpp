#include "trout/TypoSuppression.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expectEqual(const std::string& name, bool actual, bool expected) {
    if (actual != expected) {
        std::cerr << name << " expected [" << expected << "] got [" << actual << "]\n";
        std::exit(1);
    }
}

void testDoesNotSuppressShortWords() {
    expectEqual("short word", trout::suspectedTypo("abc"), false);
}

void testSuppressesLongWordWithoutVowels() {
    expectEqual("no vowels", trout::suspectedTypo("please fix thswrld"), true);
}

void testSuppressesThreeRepeatedCharacters() {
    expectEqual("repeated chars", trout::suspectedTypo("hello coool"), true);
}

void testDoesNotSuppressNonAlphaTokens() {
    expectEqual("non alpha", trout::suspectedTypo("ticket ABCD-1234"), false);
}

void testUsesCurrentWordOnlyAndTrimsTrailingSpace() {
    expectEqual("trailing space", trout::suspectedTypo("normal brrr "), true);
}

}

int main() {
    testDoesNotSuppressShortWords();
    testSuppressesLongWordWithoutVowels();
    testSuppressesThreeRepeatedCharacters();
    testDoesNotSuppressNonAlphaTokens();
    testUsesCurrentWordOnlyAndTrimsTrailingSpace();
    return 0;
}
