#include "trout/Settings.h"
#include "trout/Store.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace trout {

static const char* kGlobalKey = "global_settings_v1";

static std::string lengthToStr(CompletionLength l) {
    switch (l) { case CompletionLength::Short: return "short";
                 case CompletionLength::Medium: return "medium";
                 case CompletionLength::Long: return "long"; }
    return "short";
}
static CompletionLength lengthFromStr(const std::string& s) {
    if (s == "medium") return CompletionLength::Medium;
    if (s == "long") return CompletionLength::Long;
    return CompletionLength::Short;
}
static const char* triToStr(TriState t) {
    switch (t) { case TriState::On: return "on"; case TriState::Off: return "off";
                 default: return "inherit"; }
}
static TriState triFromStr(const std::string& s) {
    if (s == "on") return TriState::On;
    if (s == "off") return TriState::Off;
    return TriState::Inherit;
}

static json globalToJson(const GlobalSettings& g) {
    return json{
        {"launchAtLogin", g.launchAtLogin},
        {"showMenuBarIcon", g.showMenuBarIcon},
        {"showAccessoryButton", g.showAccessoryButton},
        {"selectedModelId", g.selectedModelId},
        {"completionsEnabled", g.completionsEnabled},
        {"maxCompletionLength", lengthToStr(g.maxCompletionLength)},
        {"hideOnSuspectedTypo", g.hideOnSuspectedTypo},
        {"showInlineTypoFix", g.showInlineTypoFix},
        {"useScreenshotContext", g.useScreenshotContext},
        {"useScreenshotForAppearance", g.useScreenshotForAppearance},
        {"useClipboardContext", g.useClipboardContext},
        {"collectInputs", g.collectInputs},
        {"storeOnlyAcceptedSessions", g.storeOnlyAcceptedSessions},
        {"personalizationStrength", g.personalizationStrength},
        {"globalCustomInstructions", g.globalCustomInstructions},
        {"emojiSuggestions", g.emojiSuggestions},
        {"emojiSkinTone", g.emojiSkinTone},
        {"emojiNeutralVariant", g.emojiNeutralVariant},
        {"emojiGender", g.emojiGender},
        {"anonymousDiagnostics", g.anonymousDiagnostics},
        {"acceptWordShortcut", g.acceptWordShortcut},
        {"acceptFullShortcut", g.acceptFullShortcut},
        {"forceActivateShortcut", g.forceActivateShortcut},
        {"toggleAppShortcut", g.toggleAppShortcut},
        {"globalToggleShortcut", g.globalToggleShortcut},
        {"includeTrailingSpaceOnWordAccept", g.includeTrailingSpaceOnWordAccept},
    };
}

template <typename T>
static T getOr(const json& j, const char* key, T def) {
    if (j.contains(key) && !j[key].is_null()) {
        try { return j[key].get<T>(); } catch (...) {}
    }
    return def;
}

static GlobalSettings globalFromJson(const json& j) {
    GlobalSettings g;
    g.launchAtLogin = getOr(j, "launchAtLogin", g.launchAtLogin);
    g.showMenuBarIcon = getOr(j, "showMenuBarIcon", g.showMenuBarIcon);
    g.showAccessoryButton = getOr(j, "showAccessoryButton", g.showAccessoryButton);
    g.selectedModelId = getOr<std::string>(j, "selectedModelId", g.selectedModelId);
    g.completionsEnabled = getOr(j, "completionsEnabled", g.completionsEnabled);
    g.maxCompletionLength = lengthFromStr(getOr<std::string>(j, "maxCompletionLength", "short"));
    g.hideOnSuspectedTypo = getOr(j, "hideOnSuspectedTypo", g.hideOnSuspectedTypo);
    g.showInlineTypoFix = getOr(j, "showInlineTypoFix", g.showInlineTypoFix);
    g.useScreenshotContext = getOr(j, "useScreenshotContext", g.useScreenshotContext);
    g.useScreenshotForAppearance = getOr(j, "useScreenshotForAppearance", g.useScreenshotForAppearance);
    g.useClipboardContext = getOr(j, "useClipboardContext", g.useClipboardContext);
    g.collectInputs = getOr(j, "collectInputs", g.collectInputs);
    g.storeOnlyAcceptedSessions = getOr(j, "storeOnlyAcceptedSessions", g.storeOnlyAcceptedSessions);
    g.personalizationStrength = getOr(j, "personalizationStrength", g.personalizationStrength);
    g.globalCustomInstructions = getOr<std::string>(j, "globalCustomInstructions", g.globalCustomInstructions);
    g.emojiSuggestions = getOr(j, "emojiSuggestions", g.emojiSuggestions);
    g.emojiSkinTone = getOr<std::string>(j, "emojiSkinTone", g.emojiSkinTone);
    g.emojiNeutralVariant = getOr(j, "emojiNeutralVariant", g.emojiNeutralVariant);
    g.emojiGender = getOr<std::string>(j, "emojiGender", g.emojiGender);
    g.anonymousDiagnostics = getOr(j, "anonymousDiagnostics", g.anonymousDiagnostics);
    g.acceptWordShortcut = getOr<std::string>(j, "acceptWordShortcut", g.acceptWordShortcut);
    g.acceptFullShortcut = getOr<std::string>(j, "acceptFullShortcut", g.acceptFullShortcut);
    g.forceActivateShortcut = getOr<std::string>(j, "forceActivateShortcut", g.forceActivateShortcut);
    g.toggleAppShortcut = getOr<std::string>(j, "toggleAppShortcut", g.toggleAppShortcut);
    g.globalToggleShortcut = getOr<std::string>(j, "globalToggleShortcut", g.globalToggleShortcut);
    g.includeTrailingSpaceOnWordAccept = getOr(j, "includeTrailingSpaceOnWordAccept", g.includeTrailingSpaceOnWordAccept);
    return g;
}

static json profileToJson(const AppProfile& p) {
    return json{
        {"bundleId", p.bundleId},
        {"displayName", p.displayName},
        {"completionsEnabled", triToStr(p.completionsEnabled)},
        {"midLineEnabled", triToStr(p.midLineEnabled)},
        {"autocorrectEnabled", triToStr(p.autocorrectEnabled)},
        {"tabKeyEnabled", triToStr(p.tabKeyEnabled)},
        {"compatibilityMode", p.compatibilityMode},
        {"collectTypingHistory", p.collectTypingHistory},
        {"customInstructions", p.customInstructions},
    };
}

static AppProfile profileFromJson(const json& j) {
    AppProfile p;
    p.bundleId = getOr<std::string>(j, "bundleId", "");
    p.displayName = getOr<std::string>(j, "displayName", "");
    p.completionsEnabled = triFromStr(getOr<std::string>(j, "completionsEnabled", "inherit"));
    p.midLineEnabled = triFromStr(getOr<std::string>(j, "midLineEnabled", "inherit"));
    p.autocorrectEnabled = triFromStr(getOr<std::string>(j, "autocorrectEnabled", "inherit"));
    p.tabKeyEnabled = triFromStr(getOr<std::string>(j, "tabKeyEnabled", "inherit"));
    p.compatibilityMode = getOr(j, "compatibilityMode", false);
    p.collectTypingHistory = getOr(j, "collectTypingHistory", false);
    p.customInstructions = getOr<std::string>(j, "customInstructions", "");
    return p;
}

Settings::Settings(Store* store) : store_(store) {}

void Settings::load() {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!store_) return;
    std::string raw = store_->getKV(kGlobalKey, "");
    if (!raw.empty()) {
        try { global_ = globalFromJson(json::parse(raw)); } catch (...) {}
    }
    profiles_.clear();
    for (auto& [bid, jraw] : store_->allProfiles()) {
        try { profiles_[bid] = profileFromJson(json::parse(jraw)); } catch (...) {}
    }
}

void Settings::save() {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!store_) return;
    store_->putKV(kGlobalKey, globalToJson(global_).dump());
}

GlobalSettings Settings::global() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return global_;
}

void Settings::setGlobal(const GlobalSettings& g) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        global_ = g;
    }
    save();
}

std::optional<AppProfile> Settings::appProfile(const std::string& bundleId) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = profiles_.find(bundleId);
    if (it == profiles_.end()) return std::nullopt;
    return it->second;
}

void Settings::setAppProfile(const AppProfile& profile) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        profiles_[profile.bundleId] = profile;
    }
    if (store_) store_->putProfile(profile.bundleId, profileToJson(profile).dump());
}

std::map<std::string, AppProfile> Settings::allAppProfiles() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return profiles_;
}

void Settings::deleteAppProfile(const std::string& bundleId) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        profiles_.erase(bundleId);
    }
    if (store_) store_->deleteProfile(bundleId);
}

bool Settings::completionsEnabledFor(const std::string& bundleId) const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!global_.completionsEnabled) return false;
    auto it = profiles_.find(bundleId);
    if (it != profiles_.end()) {
        if (it->second.completionsEnabled == TriState::Off) return false;
        if (it->second.completionsEnabled == TriState::On) return true;
    }
    return true;
}

} // namespace trout
