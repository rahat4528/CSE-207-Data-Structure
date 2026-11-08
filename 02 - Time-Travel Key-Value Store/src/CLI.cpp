#include "CLI.h"
#include "Timestamp.h"
#include "ExporterImporter.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ttkv {

CLI::CLI(TimeTravelStore& store) : m_store(store) {}

std::vector<std::string> CLI::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string curr;
    bool inQuotes = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
                quoteChar = '\0';
            } else if (c == '\\' && i + 1 < line.size()) {
                curr += line[++i];
            } else {
                curr += c;
            }
        } else {
            if (c == '"' || c == '\'') {
                inQuotes = true;
                quoteChar = c;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (!curr.empty()) {
                    tokens.push_back(curr);
                    curr.clear();
                }
            } else {
                curr += c;
            }
        }
    }
    if (!curr.empty()) {
        tokens.push_back(curr);
    }
    return tokens;
}

std::string CLI::executeCommand(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.empty()) return "";

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "GET") {
        return handleGet(tokens);
    } else if (cmd == "SET" || cmd == "PUT") {
        return handleSet(tokens);
    } else if (cmd == "DELETE" || cmd == "DEL") {
        return handleDelete(tokens);
    } else if (cmd == "HISTORY") {
        return handleHistory(tokens);
    } else if (cmd == "DIFF") {
        return handleDiff(tokens);
    } else if (cmd == "SNAPSHOT") {
        return handleSnapshot(tokens);
    } else if (cmd == "ROLLBACK") {
        return handleRollback(tokens);
    } else if (cmd == "SEARCH") {
        return handleSearch(tokens);
    } else if (cmd == "BEGIN") {
        auto res = m_store.beginTransaction("cli_user");
        if (res.isOk()) return "Transaction begun (txId: " + std::to_string(res.value()) + ")";
        return "ERROR: " + res.message();
    } else if (cmd == "COMMIT") {
        auto res = m_store.commitTransaction();
        if (res.isOk()) return "Transaction committed (" + std::to_string(res.value()) + " operations applied)";
        return "ERROR: " + res.message();
    } else if (cmd == "ABORT") {
        auto res = m_store.abortTransaction();
        if (res.isOk()) return "Transaction aborted; staged operations discarded.";
        return "ERROR: " + res.message();
    } else if (cmd == "STATS") {
        return handleStats();
    } else if (cmd == "DEMO") {
        return runDemo();
    } else if (cmd == "EXPORT") {
        std::string format = (tokens.size() > 1) ? tokens[1] : "json";
        std::transform(format.begin(), format.end(), format.begin(), ::tolower);
        if (format == "csv") {
            return ExporterImporter::exportToCsv(m_store.keyManager());
        }
        return ExporterImporter::exportToJson(m_store.keyManager());
    } else if (cmd == "SWEEP") {
        size_t swept = m_store.sweepExpired();
        return "Swept " + std::to_string(swept) + " expired keys.";
    } else if (cmd == "HELP") {
        return handleHelp();
    } else if (cmd == "EXIT" || cmd == "QUIT") {
        return "EXIT";
    }

    return "ERROR: Unknown command '" + cmd + "'. Type HELP for available commands.";
}

std::string CLI::handleGet(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        return "ERROR: Usage: GET <key> [AT VERSION <n> | AT TIME <timestamp>]";
    }

    std::string key = tokens[1];
    if (tokens.size() == 2) {
        // GET key -> latest non-deleted value
        auto res = m_store.get(key);
        if (res.isError()) return "ERROR: " + res.message();
        const auto& q = res.value();
        if (q.status == QueryStatus::KEY_UNKNOWN) return "[KEY_UNKNOWN] Key never existed.";
        if (q.status == QueryStatus::DELETED) return "[DELETED] Key was deleted at version " + std::to_string(q.versionNumber);
        if (q.status == QueryStatus::EXPIRED) return "[EXPIRED] Key TTL has expired.";
        return q.value + " (version " + std::to_string(q.versionNumber) + ")";
    }

    if (tokens.size() >= 5) {
        std::string at = tokens[2];
        std::string target = tokens[3];
        std::transform(at.begin(), at.end(), at.begin(), ::toupper);
        std::transform(target.begin(), target.end(), target.begin(), ::toupper);

        if (at == "AT" && target == "VERSION") {
            uint32_t ver = static_cast<uint32_t>(std::stoul(tokens[4]));
            auto res = m_store.getAtVersion(key, ver);
            if (res.isError()) return "ERROR: " + res.message();
            const auto& q = res.value();
            if (q.status == QueryStatus::KEY_UNKNOWN) return "[KEY_UNKNOWN] Key never existed.";
            if (q.status == QueryStatus::INVALID_VERSION) return "[INVALID_VERSION] Version does not exist.";
            if (q.status == QueryStatus::PRUNED) return "[PRUNED] Version has been pruned by retention policy.";
            if (q.status == QueryStatus::DELETED) return "[DELETED] Key was in deleted state at version " + std::to_string(ver);
            return q.value;
        }

        if (at == "AT" && target == "TIME") {
            std::string timeStr = tokens[4];
            auto res = m_store.getAtTimeIso(key, timeStr);
            if (res.isError()) return "ERROR: " + res.message();
            const auto& q = res.value();
            if (q.status == QueryStatus::KEY_UNKNOWN) return "[KEY_UNKNOWN] Key never existed.";
            if (q.status == QueryStatus::NOT_YET_EXISTS) return "[NOT_YET_EXISTS] Key did not yet exist at " + timeStr;
            if (q.status == QueryStatus::DELETED) return "[DELETED] Key was in deleted state at " + timeStr;
            if (q.status == QueryStatus::EXPIRED) return "[EXPIRED] Key had expired by " + timeStr;
            return q.value + " (version " + std::to_string(q.versionNumber) + ")";
        }
    }

    return "ERROR: Invalid GET syntax. Usage: GET <key> [AT VERSION <n> | AT TIME <timestamp>]";
}

std::string CLI::handleSet(const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) {
        return "ERROR: Usage: SET <key> <value> [ttl_seconds]";
    }
    std::string key = tokens[1];
    std::string value = tokens[2];
    uint64_t ttlMicros = 0;
    if (tokens.size() > 3) {
        uint64_t ttlSec = std::stoull(tokens[3]);
        ttlMicros = ttlSec * 1000000ULL;
    }

    auto res = m_store.set(key, value, ttlMicros);
    if (res.isOk()) {
        return "OK (seqId: " + std::to_string(res.value()) + ")";
    }
    return "ERROR: " + res.message();
}

std::string CLI::handleDelete(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        return "ERROR: Usage: DELETE <key>";
    }
    std::string key = tokens[1];
    auto res = m_store.del(key);
    if (res.isOk()) {
        return "OK (tombstone created, seqId: " + std::to_string(res.value()) + ")";
    }
    return "ERROR: " + res.message();
}

std::string CLI::handleHistory(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        return "ERROR: Usage: HISTORY <key> [limit] [offset]";
    }
    std::string key = tokens[1];
    size_t limit = (tokens.size() > 2) ? std::stoul(tokens[2]) : 0;
    size_t offset = (tokens.size() > 3) ? std::stoul(tokens[3]) : 0;

    auto res = m_store.history(key, limit, offset);
    if (res.isError()) return "ERROR: " + res.message();
    return HistoryEngine::formatHistoryTable(key, res.value());
}

std::string CLI::handleDiff(const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) {
        return "ERROR: Usage: DIFF <key> <TIME_A> <TIME_B>";
    }
    std::string key = tokens[1];
    auto tARes = Timestamp::parseIso8601(tokens[2]);
    if (tARes.isError()) return "ERROR in TIME_A: " + tARes.message();
    auto tBRes = Timestamp::parseIso8601(tokens[3]);
    if (tBRes.isError()) return "ERROR in TIME_B: " + tBRes.message();

    auto res = m_store.diff(key, tARes.value(), tBRes.value());
    if (res.isError()) return "ERROR: " + res.message();

    const auto& d = res.value();
    std::ostringstream oss;
    oss << "DIFF for '" << key << "' between " << tokens[2] << " and " << tokens[3] << ":\n";
    oss << "  Change Type: " << d.changeType << "\n";
    oss << "  At TIME_A:   " << queryStatusToString(d.stateA.status) << " (val: '" << d.stateA.value << "')\n";
    oss << "  At TIME_B:   " << queryStatusToString(d.stateB.status) << " (val: '" << d.stateB.value << "')\n";
    return oss.str();
}

std::string CLI::handleSnapshot(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        return "ERROR: Usage: SNAPSHOT TIME <timestamp> | SNAPSHOT CREATE <name> | SNAPSHOT LIST | SNAPSHOT RESTORE <name>";
    }

    std::string sub = tokens[1];
    std::transform(sub.begin(), sub.end(), sub.begin(), ::toupper);

    if (sub == "TIME") {
        if (tokens.size() < 3) return "ERROR: Usage: SNAPSHOT TIME <timestamp>";
        auto tRes = Timestamp::parseIso8601(tokens[2]);
        if (tRes.isError()) return "ERROR: " + tRes.message();

        auto snap = m_store.snapshotAtTime(tRes.value()).value();
        std::ostringstream oss;
        oss << "SNAPSHOT as of " << tokens[2] << " (" << snap.activeEntries.size() << " active keys):\n";
        oss << "+----------------------+---------+----------------------+\n";
        oss << "| Key                  | Version | Value                |\n";
        oss << "+----------------------+---------+----------------------+\n";
        for (const auto& e : snap.activeEntries) {
            oss << "| " << std::setw(20) << e.key << " | "
                << std::setw(7) << e.versionNumber << " | "
                << std::setw(20) << e.value << " |\n";
        }
        oss << "+----------------------+---------+----------------------+\n";
        return oss.str();
    } else if (sub == "CREATE") {
        if (tokens.size() < 3) return "ERROR: Usage: SNAPSHOT CREATE <name> [description]";
        std::string desc = (tokens.size() > 3) ? tokens[3] : "";
        auto res = m_store.createNamedSnapshot(tokens[2], "cli_user", desc);
        if (res.isOk()) return "Snapshot '" + tokens[2] + "' created successfully.";
        return "ERROR: " + res.message();
    } else if (sub == "LIST") {
        auto list = m_store.listNamedSnapshots();
        if (list.empty()) return "No named snapshots recorded.";
        std::ostringstream oss;
        oss << "Named Snapshots (" << list.size() << " total):\n";
        for (const auto& s : list) {
            oss << " - " << s.name << " (timestamp: " << Timestamp::formatIso8601(s.timestamp)
                << ", seqId: " << s.globalVersionId << ", desc: '" << s.description << "')\n";
        }
        return oss.str();
    } else if (sub == "DROP") {
        if (tokens.size() < 3) return "ERROR: Usage: SNAPSHOT DROP <name>";
        auto res = m_store.dropNamedSnapshot(tokens[2]);
        if (res.isOk()) return "Snapshot '" + tokens[2] + "' dropped.";
        return "ERROR: " + res.message();
    } else if (sub == "RESTORE") {
        if (tokens.size() < 3) return "ERROR: Usage: SNAPSHOT RESTORE <name>";
        auto res = m_store.restoreNamedSnapshot(tokens[2], "cli_user");
        if (res.isOk()) return "Snapshot '" + tokens[2] + "' restored successfully.";
        return "ERROR: " + res.message();
    }

    return "ERROR: Unknown SNAPSHOT subcommand. Type HELP for syntax.";
}

std::string CLI::handleRollback(const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) {
        return "ERROR: Usage: ROLLBACK <key> VERSION <n> | ROLLBACK <key> TIME <timestamp>";
    }
    std::string key = tokens[1];
    std::string targetType = tokens[2];
    std::transform(targetType.begin(), targetType.end(), targetType.begin(), ::toupper);

    if (targetType == "VERSION") {
        uint32_t ver = static_cast<uint32_t>(std::stoul(tokens[3]));
        auto res = m_store.rollbackToVersion(key, ver, "cli_user");
        if (res.isOk()) return "OK: Rolled back '" + key + "' to version " + std::to_string(ver) + " (new seqId: " + std::to_string(res.value()) + ")";
        return "ERROR: " + res.message();
    } else if (targetType == "TIME") {
        auto tRes = Timestamp::parseIso8601(tokens[3]);
        if (tRes.isError()) return "ERROR: " + tRes.message();
        auto res = m_store.rollbackToTime(key, tRes.value(), "cli_user");
        if (res.isOk()) return "OK: Rolled back '" + key + "' to time " + tokens[3] + " (new seqId: " + std::to_string(res.value()) + ")";
        return "ERROR: " + res.message();
    }

    return "ERROR: Invalid ROLLBACK syntax. Usage: ROLLBACK <key> VERSION <n> | ROLLBACK <key> TIME <timestamp>";
}

std::string CLI::handleSearch(const std::vector<std::string>& tokens) {
    std::string pattern = (tokens.size() > 1) ? tokens[1] : "";
    auto matches = m_store.searchKeys(pattern);
    std::ostringstream oss;
    oss << "Search matches for '" << pattern << "' (" << matches.size() << " keys):\n";
    for (const auto& k : matches) {
        oss << "  - " << k << "\n";
    }
    return oss.str();
}

std::string CLI::handleStats() {
    std::ostringstream oss;
    oss << "=== Time-Travel Key-Value Store Statistics ===\n";
    oss << "Total Keys Registered: " << m_store.keyManager().keyCount(true) << "\n";
    oss << "Active (Non-Deleted):  " << m_store.keyManager().keyCount(false) << "\n";
    oss << "Total Versions Count:  " << m_store.keyManager().totalVersionCount() << "\n";
    oss << "Events in Event Log:   " << m_store.eventLog().count() << "\n";
    oss << "Audit Log Records:     " << m_store.auditLogger().count() << "\n";
    oss << "Named Snapshots:       " << m_store.listNamedSnapshots().size() << "\n";
    return oss.str();
}

std::string CLI::handleHelp() {
    std::ostringstream oss;
    oss << "Available Commands:\n";
    oss << "  GET <key>                               - Retrieve latest non-deleted value\n";
    oss << "  GET <key> AT VERSION <n>                - Retrieve value at specific version number\n";
    oss << "  GET <key> AT TIME <ISO-8601>            - Retrieve value at specific historical timestamp\n";
    oss << "  SET <key> <value> [ttl_seconds]         - Set or update key value with optional TTL\n";
    oss << "  DELETE <key>                            - Delete key creating an immutable tombstone\n";
    oss << "  HISTORY <key> [limit] [offset]          - Display full historical versions table\n";
    oss << "  DIFF <key> <TIME_A> <TIME_B>            - Compare key state between two timestamps\n";
    oss << "  SNAPSHOT TIME <ISO-8601>                - Display all active keys as of a past timestamp\n";
    oss << "  SNAPSHOT CREATE <name> [desc]           - Create named lightweight snapshot marker\n";
    oss << "  SNAPSHOT LIST                           - List all named snapshots\n";
    oss << "  SNAPSHOT RESTORE <name>                 - Restore named snapshot state via forward versions\n";
    oss << "  SNAPSHOT DROP <name>                    - Delete a named snapshot marker\n";
    oss << "  ROLLBACK <key> VERSION <n>              - Restore prior version as a new forward version\n";
    oss << "  ROLLBACK <key> TIME <timestamp>         - Restore state at timestamp as a new forward version\n";
    oss << "  SEARCH <pattern>                        - Search keys by substring or prefix\n";
    oss << "  BEGIN / COMMIT / ABORT                  - Manage atomic transactions with staged overlay\n";
    oss << "  EXPORT [json|csv]                       - Export full store history\n";
    oss << "  SWEEP                                   - Sweep expired keys and record tombstones\n";
    oss << "  STATS                                   - Display store health and version statistics\n";
    oss << "  DEMO                                    - Execute 10-step time-travel walkthrough\n";
    oss << "  HELP / EXIT                             - Display help or exit application\n";
    return oss.str();
}

std::string CLI::runDemo() {
    std::ostringstream oss;
    oss << "\n======================================================================\n";
    oss << "   RUNNING TIME-TRAVEL STORE DEMONSTRATION WALKTHROUGH\n";
    oss << "======================================================================\n\n";

    // Step 1: User username updates
    oss << "1. Creating key 'username' with value 'Alice'...\n";
    executeCommand("SET username Alice");

    oss << "2. Updating key 'username' to 'Bob'...\n";
    executeCommand("SET username Bob");

    oss << "3. Updating key 'username' to 'Charlie'...\n";
    executeCommand("SET username Charlie");

    oss << "4. Deleting key 'username' (creates tombstone)...\n";
    executeCommand("DELETE username");

    oss << "\n5. Querying 'username' at different version numbers:\n";
    oss << "   GET username AT VERSION 1 -> " << executeCommand("GET username AT VERSION 1") << "\n";
    oss << "   GET username AT VERSION 2 -> " << executeCommand("GET username AT VERSION 2") << "\n";
    oss << "   GET username AT VERSION 3 -> " << executeCommand("GET username AT VERSION 3") << "\n";
    oss << "   GET username AT VERSION 4 -> " << executeCommand("GET username AT VERSION 4") << "\n";
    oss << "   GET username (current)   -> " << executeCommand("GET username") << "\n\n";

    oss << "6. Full History Table for 'username':\n";
    oss << executeCommand("HISTORY username") << "\n";

    oss << "7. Rolling back 'username' to Version 2 ('Bob'):\n";
    oss << "   " << executeCommand("ROLLBACK username VERSION 2") << "\n";
    oss << "   GET username (current now) -> " << executeCommand("GET username") << "\n\n";

    oss << "======================================================================\n";
    oss << "   DEMO COMPLETED SUCCESSFULLY!\n";
    oss << "======================================================================\n";
    return oss.str();
}

void CLI::runInteractive(std::istream& in, std::ostream& out) {
    out << "==========================================================\n";
    out << "   Time-Travel Key-Value Store (Versioned Storage Engine)\n";
    out << "   ISO C++20 | Type HELP for syntax | Type EXIT to quit\n";
    out << "==========================================================\n\n";

    std::string line;
    while (true) {
        out << "ttkv> ";
        if (!std::getline(in, line)) break;
        if (line.empty()) continue;

        std::string result = executeCommand(line);
        if (result == "EXIT") {
            out << "Goodbye!\n";
            break;
        }
        if (!result.empty()) {
            out << result << "\n";
        }
    }
}

} // namespace ttkv
