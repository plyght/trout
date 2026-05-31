#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <mutex>

struct sqlite3;

namespace trout {

// Daily aggregate of accepted-completion activity.
struct DayStat {
    std::string day; // YYYY-MM-DD (local)
    int completions = 0;
    int words = 0;
    int characters = 0;
};

// SQLite-backed persistence for settings key/values, app profiles, statistics,
// and optional typing-history metadata. Thread-safe via an internal mutex;
// SQLite is opened in serialized mode.
class Store {
public:
    Store();
    ~Store();

    bool open(const std::string& path);
    void close();
    bool isOpen() const { return db_ != nullptr; }

    // Generic key/value used for GlobalSettings JSON blob.
    bool putKV(const std::string& key, const std::string& value);
    std::string getKV(const std::string& key, const std::string& fallback = "") const;

    // App profiles stored as JSON blobs keyed by bundle id.
    bool putProfile(const std::string& bundleId, const std::string& json);
    std::map<std::string, std::string> allProfiles() const;
    bool deleteProfile(const std::string& bundleId);

    // Statistics.
    bool recordAcceptance(int words, int characters); // increments today's row
    std::vector<DayStat> dayStats(const std::string& fromDay, const std::string& toDay) const;
    DayStat todayStat() const;

    // Typing history metadata (raw text optional; stored only when user opts in).
    bool recordTypingSample(const std::string& bundleId, int charCount, bool accepted);
    int typingSampleCount(const std::string& bundleId = "") const;
    bool deleteTypingHistory(const std::string& bundleId = ""); // empty = all

private:
    bool exec(const std::string& sql);
    bool migrate();

    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;
};

} // namespace trout
