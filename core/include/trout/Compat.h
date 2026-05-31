#pragma once

#include <string>
#include <map>
#include <vector>

namespace trout {

enum class CompatSupport { Works, NeedsSetup, MirrorOnly, SidebarOnly, Unsupported };

struct CompatProfile {
    std::string bundleId;
    std::string displayName;
    CompatSupport support = CompatSupport::Works;
    std::string recipe;       // human-readable setup notes
    bool disableTabByDefault = false;
    bool terminalAgentMode = false; // only activate for AI-agent prompts
};

// Built-in compatibility database keyed by bundle id, derived from the PRD's
// observed matrix. Used to choose insertion strategy and warn the user.
class Compat {
public:
    Compat();

    const CompatProfile* lookup(const std::string& bundleId) const;
    const std::map<std::string, CompatProfile>& all() const { return table_; }

    static const char* supportLabel(CompatSupport s);

private:
    std::map<std::string, CompatProfile> table_;
};

} // namespace trout
