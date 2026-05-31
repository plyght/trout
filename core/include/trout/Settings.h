#pragma once

#include <string>
#include <map>
#include <mutex>
#include <optional>

namespace trout {

enum class CompletionLength { Short, Medium, Long };

enum class TriState { Inherit, On, Off };

struct AppProfile {
    std::string bundleId;
    std::string displayName;
    TriState completionsEnabled = TriState::Inherit;
    TriState midLineEnabled = TriState::Inherit;
    TriState autocorrectEnabled = TriState::Inherit;
    TriState tabKeyEnabled = TriState::Inherit;
    bool compatibilityMode = false;
    bool collectTypingHistory = false;
    std::string customInstructions;
};

struct GlobalSettings {
    // launch / appearance
    bool launchAtLogin = false;
    bool showMenuBarIcon = true;
    bool showAccessoryButton = false;

    // model
    std::string selectedModelId;

    // completions
    bool completionsEnabled = true;
    CompletionLength maxCompletionLength = CompletionLength::Short;

    // typo / autocorrect
    bool hideOnSuspectedTypo = true;
    bool showInlineTypoFix = false;

    // context
    bool useScreenshotContext = false;
    bool useScreenshotForAppearance = false;
    bool useClipboardContext = false;

    // personalization
    bool collectInputs = false;
    bool storeOnlyAcceptedSessions = true;
    float personalizationStrength = 0.5f;
    std::string globalCustomInstructions;

    // emoji
    bool emojiSuggestions = true;
    std::string emojiSkinTone = "default";
    bool emojiNeutralVariant = true;
    std::string emojiGender = "neutral";

    // privacy / diagnostics
    bool anonymousDiagnostics = false;

    // shortcuts (stored as human-readable descriptors; the macOS layer maps them)
    std::string acceptWordShortcut = "tab";
    std::string acceptFullShortcut = "grave";
    std::string forceActivateShortcut = "ctrl+grave";
    std::string toggleAppShortcut = "ctrl+opt+cmd+grave";
    std::string globalToggleShortcut = "";

    bool includeTrailingSpaceOnWordAccept = true;
};

// Thread-safe settings container backed by the Store (SQLite). The macOS layer
// and the engine both read/write through this. Changes are persisted via the
// supplied Store on save().
class Store;

class Settings {
public:
    explicit Settings(Store* store);

    GlobalSettings global() const;
    void setGlobal(const GlobalSettings& g);

    std::optional<AppProfile> appProfile(const std::string& bundleId) const;
    void setAppProfile(const AppProfile& profile);
    std::map<std::string, AppProfile> allAppProfiles() const;
    void deleteAppProfile(const std::string& bundleId);

    // Resolve effective completion enablement for an app, applying inheritance.
    bool completionsEnabledFor(const std::string& bundleId) const;

    void load();
    void save();

private:
    Store* store_;
    mutable std::mutex mutex_;
    GlobalSettings global_;
    std::map<std::string, AppProfile> profiles_;
};

} // namespace trout
