#include "trout/Store.h"
#include "trout/Log.h"

#include <sqlite3.h>
#include <ctime>

namespace trout {

Store::Store() = default;
Store::~Store() { close(); }

bool Store::open(const std::string& path) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (db_) return true;
    int rc = sqlite3_open_v2(path.c_str(), &db_,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                             nullptr);
    if (rc != SQLITE_OK) {
        TROUT_ERROR("sqlite open failed: " << sqlite3_errmsg(db_));
        if (db_) { sqlite3_close(db_); db_ = nullptr; }
        return false;
    }
    sqlite3_busy_timeout(db_, 2000);
    return migrate();
}

void Store::close() {
    std::lock_guard<std::mutex> lk(mutex_);
    if (db_) { sqlite3_close(db_); db_ = nullptr; }
}

bool Store::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        TROUT_ERROR("sqlite exec failed: " << (err ? err : "?"));
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

bool Store::migrate() {
    return exec(
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT);"
        "CREATE TABLE IF NOT EXISTS profiles (bundle_id TEXT PRIMARY KEY, json TEXT);"
        "CREATE TABLE IF NOT EXISTS stats ("
        "  day TEXT PRIMARY KEY, completions INT DEFAULT 0,"
        "  words INT DEFAULT 0, characters INT DEFAULT 0);"
        "CREATE TABLE IF NOT EXISTS typing_history ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT, bundle_id TEXT,"
        "  char_count INT, accepted INT, ts INTEGER);");
}

bool Store::putKV(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return false;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO kv(key,value) VALUES(?,?) "
                            "ON CONFLICT(key) DO UPDATE SET value=excluded.value;",
                       -1, &st, nullptr);
    sqlite3_bind_text(st, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_finalize(st);
    return ok;
}

std::string Store::getKV(const std::string& key, const std::string& fallback) const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return fallback;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_, "SELECT value FROM kv WHERE key=?;", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    std::string out = fallback;
    if (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char* v = sqlite3_column_text(st, 0);
        if (v) out = reinterpret_cast<const char*>(v);
    }
    sqlite3_finalize(st);
    return out;
}

bool Store::putProfile(const std::string& bundleId, const std::string& json) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return false;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_, "INSERT INTO profiles(bundle_id,json) VALUES(?,?) "
                            "ON CONFLICT(bundle_id) DO UPDATE SET json=excluded.json;",
                       -1, &st, nullptr);
    sqlite3_bind_text(st, 1, bundleId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, json.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_finalize(st);
    return ok;
}

std::map<std::string, std::string> Store::allProfiles() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::map<std::string, std::string> out;
    if (!db_) return out;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_, "SELECT bundle_id,json FROM profiles;", -1, &st, nullptr);
    while (sqlite3_step(st) == SQLITE_ROW) {
        out[reinterpret_cast<const char*>(sqlite3_column_text(st, 0))] =
            reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
    }
    sqlite3_finalize(st);
    return out;
}

bool Store::deleteProfile(const std::string& bundleId) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return false;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_, "DELETE FROM profiles WHERE bundle_id=?;", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, bundleId.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_finalize(st);
    return ok;
}

static std::string today() {
    std::time_t t = std::time(nullptr);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return buf;
}

bool Store::recordAcceptance(int words, int characters) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return false;
    std::string d = today();
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_,
        "INSERT INTO stats(day,completions,words,characters) VALUES(?,1,?,?) "
        "ON CONFLICT(day) DO UPDATE SET completions=completions+1, "
        "words=words+excluded.words, characters=characters+excluded.characters;",
        -1, &st, nullptr);
    sqlite3_bind_text(st, 1, d.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 2, words);
    sqlite3_bind_int(st, 3, characters);
    bool ok = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_finalize(st);
    return ok;
}

std::vector<DayStat> Store::dayStats(const std::string& fromDay, const std::string& toDay) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<DayStat> out;
    if (!db_) return out;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_,
        "SELECT day,completions,words,characters FROM stats "
        "WHERE day>=? AND day<=? ORDER BY day;", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, fromDay.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, toDay.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(st) == SQLITE_ROW) {
        DayStat s;
        s.day = reinterpret_cast<const char*>(sqlite3_column_text(st, 0));
        s.completions = sqlite3_column_int(st, 1);
        s.words = sqlite3_column_int(st, 2);
        s.characters = sqlite3_column_int(st, 3);
        out.push_back(s);
    }
    sqlite3_finalize(st);
    return out;
}

DayStat Store::todayStat() const {
    std::string d = today();
    auto rows = dayStats(d, d);
    if (!rows.empty()) return rows.front();
    DayStat s; s.day = d; return s;
}

bool Store::recordTypingSample(const std::string& bundleId, int charCount, bool accepted) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return false;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_,
        "INSERT INTO typing_history(bundle_id,char_count,accepted,ts) VALUES(?,?,?,?);",
        -1, &st, nullptr);
    sqlite3_bind_text(st, 1, bundleId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 2, charCount);
    sqlite3_bind_int(st, 3, accepted ? 1 : 0);
    sqlite3_bind_int64(st, 4, (sqlite3_int64)std::time(nullptr));
    bool ok = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_finalize(st);
    return ok;
}

int Store::typingSampleCount(const std::string& bundleId) const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return 0;
    sqlite3_stmt* st = nullptr;
    if (bundleId.empty()) {
        sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM typing_history;", -1, &st, nullptr);
    } else {
        sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM typing_history WHERE bundle_id=?;", -1, &st, nullptr);
        sqlite3_bind_text(st, 1, bundleId.c_str(), -1, SQLITE_TRANSIENT);
    }
    int n = 0;
    if (sqlite3_step(st) == SQLITE_ROW) n = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return n;
}

bool Store::deleteTypingHistory(const std::string& bundleId) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!db_) return false;
    sqlite3_stmt* st = nullptr;
    if (bundleId.empty()) {
        sqlite3_prepare_v2(db_, "DELETE FROM typing_history;", -1, &st, nullptr);
    } else {
        sqlite3_prepare_v2(db_, "DELETE FROM typing_history WHERE bundle_id=?;", -1, &st, nullptr);
        sqlite3_bind_text(st, 1, bundleId.c_str(), -1, SQLITE_TRANSIENT);
    }
    bool ok = sqlite3_step(st) == SQLITE_DONE;
    sqlite3_finalize(st);
    return ok;
}

} // namespace trout
