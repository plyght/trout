#include "trout/Compat.h"

namespace trout {

const char* Compat::supportLabel(CompatSupport s) {
    switch (s) {
        case CompatSupport::Works: return "Works";
        case CompatSupport::NeedsSetup: return "Needs setup";
        case CompatSupport::MirrorOnly: return "Mirror window only";
        case CompatSupport::SidebarOnly: return "Sidebar chat only";
        case CompatSupport::Unsupported: return "Unsupported";
    }
    return "?";
}

Compat::Compat() {
    auto add = [&](const std::string& bid, const std::string& name,
                   CompatSupport s, const std::string& recipe = "",
                   bool disableTab = false, bool termAgent = false) {
        CompatProfile p;
        p.bundleId = bid; p.displayName = name; p.support = s;
        p.recipe = recipe; p.disableTabByDefault = disableTab;
        p.terminalAgentMode = termAgent;
        table_[bid] = p;
    };

    // Browsers
    add("com.apple.Safari", "Safari", CompatSupport::Works);
    add("com.google.Chrome", "Google Chrome", CompatSupport::Works);
    add("com.brave.Browser", "Brave", CompatSupport::Works);
    add("com.microsoft.edgemac", "Microsoft Edge", CompatSupport::Works);
    add("company.thebrowser.Browser", "Arc", CompatSupport::NeedsSetup,
        "Enable Chromium text metrics at chrome://accessibility, or launch with "
        "--force-renderer-accessibility=complete. May not persist across restarts.");
    add("company.thebrowser.dia", "Dia", CompatSupport::NeedsSetup,
        "Same as Arc: enable Chromium accessibility text metrics.");
    add("org.mozilla.firefox", "Firefox", CompatSupport::MirrorOnly,
        "Inline metrics unavailable; use mirror window.");
    add("app.zen-browser.zen", "Zen Browser", CompatSupport::MirrorOnly,
        "Inline metrics unavailable; use mirror window.");

    // Email
    add("com.apple.mail", "Apple Mail", CompatSupport::Works);
    add("com.microsoft.Outlook", "Outlook", CompatSupport::Works);
    add("com.mimestream.Mimestream", "Mimestream", CompatSupport::Works);
    add("org.mozilla.thunderbird", "Thunderbird", CompatSupport::Unsupported);

    // Writing / documents
    add("com.microsoft.Word", "Microsoft Word", CompatSupport::Works);
    add("com.apple.TextEdit", "TextEdit", CompatSupport::Works);
    add("com.apple.iWork.Pages", "Pages", CompatSupport::Unsupported);
    add("com.literatureandlatte.scrivener3", "Scrivener", CompatSupport::Unsupported);

    // Notes / knowledge
    add("com.apple.Notes", "Apple Notes", CompatSupport::Works);
    add("notion.id", "Notion", CompatSupport::Works);
    add("md.obsidian", "Obsidian", CompatSupport::Works);
    add("com.microsoft.onenote.mac", "OneNote", CompatSupport::Unsupported);

    // Messaging
    add("com.apple.MobileSMS", "Messages", CompatSupport::Works);
    add("com.microsoft.teams2", "Microsoft Teams", CompatSupport::Works);
    add("desktop.WhatsApp", "WhatsApp", CompatSupport::Works);
    add("com.tinyspeck.slackmacgap", "Slack", CompatSupport::NeedsSetup,
        "Partial support; some compose fields may require compatibility mode.");

    // Code editors (sidebar chat only)
    add("com.microsoft.VSCode", "VS Code", CompatSupport::SidebarOnly,
        "Target the AI sidebar prompt, not the main editor buffer.", true);
    add("com.todesktop.230313mzl4w4u92", "Cursor", CompatSupport::SidebarOnly,
        "Target the AI sidebar prompt, not the main editor buffer.", true);
    add("com.exafunction.windsurf", "Windsurf", CompatSupport::SidebarOnly,
        "Target the AI sidebar prompt, not the main editor buffer.", true);
    add("com.barebones.bbedit", "BBEdit", CompatSupport::Unsupported);
    add("com.sublimetext.4", "Sublime Text", CompatSupport::Unsupported);

    // Terminals (agent-prompt mode)
    add("com.apple.Terminal", "Terminal", CompatSupport::Works,
        "Activates for AI-agent prompts; quiet for ordinary shell input.", true, true);
    add("com.googlecode.iterm2", "iTerm2", CompatSupport::Works,
        "Activates for AI-agent prompts; quiet for ordinary shell input.", true, true);
    add("com.mitchellh.ghostty", "Ghostty", CompatSupport::Unsupported);
    add("dev.warp.Warp-Stable", "Warp", CompatSupport::Unsupported);
}

const CompatProfile* Compat::lookup(const std::string& bundleId) const {
    auto it = table_.find(bundleId);
    return it == table_.end() ? nullptr : &it->second;
}

} // namespace trout
