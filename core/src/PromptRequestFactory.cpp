#include "trout/PromptRequestFactory.h"

#include <sstream>

namespace trout {

PromptRequest makePromptRequest(
    const CompletionContext& context,
    const GlobalSettings& globalSettings,
    const std::optional<AppProfile>& appProfile
) {
    std::ostringstream p;
    p << "You are an inline writing autocomplete. Continue the user's text in "
         "their own voice. Output only the continuation, no explanations, no "
         "quotes. Keep it short and natural.\n";

    if (!globalSettings.globalCustomInstructions.empty())
        p << "User style instructions: " << globalSettings.globalCustomInstructions << "\n";

    if (appProfile) {
        if (!appProfile->customInstructions.empty())
            p << "App-specific instructions: " << appProfile->customInstructions << "\n";
    }

    if (globalSettings.useClipboardContext && !context.clipboardText.empty())
        p << "Clipboard context: " << context.clipboardText << "\n";
    if (globalSettings.useScreenshotContext && !context.screenContext.empty())
        p << "Screen context: " << context.screenContext << "\n";

    p << "\nText so far:\n" << context.textBeforeCursor;
    return { p.str() };
}

}
