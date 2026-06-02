#include "trout/SuggestionTextNormalizer.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expectEqual(const std::string& name, const std::string& actual, const std::string& expected) {
    if (actual != expected) {
        std::cerr << name << " expected [" << expected << "] got [" << actual << "]\n";
        std::exit(1);
    }
}

void testStripsPromptEchoAndMarkers() {
    trout::SuggestionNormalizationRequest request;
    request.prompt = "Continue this";
    request.raw = "Continue this\n<|im_start|>Continuation: natural text<|im_end|>";
    expectEqual("prompt echo", trout::normalizeSuggestionText(request), " natural text");
}

void testStripsThinkBlocks() {
    trout::SuggestionNormalizationRequest request;
    request.raw = "<think>reasoning nobody should see</think> sounds good";
    expectEqual("think block", trout::normalizeSuggestionText(request), " sounds good");
}

void testStripsPrecedingEcho() {
    trout::SuggestionNormalizationRequest request;
    request.precedingText = "I like";
    request.raw = "I like trout a lot";
    expectEqual("preceding echo", trout::normalizeSuggestionText(request), " trout a lot");
}

void testDropsTrailingDuplicate() {
    trout::SuggestionNormalizationRequest request;
    request.trailingText = "already exists after caret";
    request.raw = "already exists";
    expectEqual("trailing duplicate", trout::normalizeSuggestionText(request), "");
}

void testAvoidsDoubleSpace() {
    trout::SuggestionNormalizationRequest request;
    request.precedingText = "hello ";
    request.raw = " world";
    expectEqual("double space", trout::normalizeSuggestionText(request), "world");
}

}

int main() {
    testStripsPromptEchoAndMarkers();
    testStripsThinkBlocks();
    testStripsPrecedingEcho();
    testDropsTrailingDuplicate();
    testAvoidsDoubleSpace();
    return 0;
}
