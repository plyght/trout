#include "trout/PromptRequestFactory.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expectContains(const std::string& name, const std::string& text, const std::string& needle) {
    if (text.find(needle) == std::string::npos) {
        std::cerr << name << " missing [" << needle << "] in [" << text << "]\n";
        std::exit(1);
    }
}

void expectNotContains(const std::string& name, const std::string& text, const std::string& needle) {
    if (text.find(needle) != std::string::npos) {
        std::cerr << name << " unexpectedly contained [" << needle << "] in [" << text << "]\n";
        std::exit(1);
    }
}

void testIncludesBaseInstructionAndTextBeforeCursor() {
    trout::CompletionContext context;
    context.textBeforeCursor = "Hello wor";

    trout::GlobalSettings settings;
    auto request = trout::makePromptRequest(context, settings, std::nullopt);

    expectContains("base instruction", request.prompt, "You are an inline writing autocomplete.");
    expectContains("text so far", request.prompt, "Text so far:\nHello wor");
}

void testIncludesGlobalAndAppInstructions() {
    trout::CompletionContext context;
    context.bundleId = "com.example.app";
    context.textBeforeCursor = "Draft";

    trout::GlobalSettings settings;
    settings.globalCustomInstructions = "Be concise";

    trout::AppProfile profile;
    profile.bundleId = context.bundleId;
    profile.customInstructions = "Use a warm tone";

    auto request = trout::makePromptRequest(context, settings, profile);

    expectContains("global instructions", request.prompt, "User style instructions: Be concise");
    expectContains("app instructions", request.prompt, "App-specific instructions: Use a warm tone");
}

void testContextFlagsGateClipboardAndScreenContext() {
    trout::CompletionContext context;
    context.clipboardText = "clipboard value";
    context.screenContext = "screen value";
    context.textBeforeCursor = "Draft";

    trout::GlobalSettings settings;

    auto disabled = trout::makePromptRequest(context, settings, std::nullopt);
    expectNotContains("clipboard disabled", disabled.prompt, "clipboard value");
    expectNotContains("screen disabled", disabled.prompt, "screen value");

    settings.useClipboardContext = true;
    settings.useScreenshotContext = true;

    auto enabled = trout::makePromptRequest(context, settings, std::nullopt);
    expectContains("clipboard enabled", enabled.prompt, "Clipboard context: clipboard value");
    expectContains("screen enabled", enabled.prompt, "Screen context: screen value");
}

}

int main() {
    testIncludesBaseInstructionAndTextBeforeCursor();
    testIncludesGlobalAndAppInstructions();
    testContextFlagsGateClipboardAndScreenContext();
    return 0;
}
