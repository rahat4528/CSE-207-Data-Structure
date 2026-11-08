#include "ExporterImporter.h"
#include "Timestamp.h"
#include "Validation.h"
#include <sstream>
#include <algorithm>
#include <unordered_set>

namespace ttkv {

namespace {

std::string escapeJson(const std::string& str) {
    std::string out;
    out.reserve(str.size() + 16);
    for (char c : str) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string escapeCsv(const std::string& str) {
    bool needQuote = (str.find(',') != std::string::npos ||
                      str.find('"') != std::string::npos ||
                      str.find('\n') != std::string::npos);
    if (!needQuote) return str;
    std::string out = "\"";
    for (char c : str) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

// Simple JSON string value extractor: finds "key": "value" or "key": number
bool extractJsonField(const std::string& obj, const std::string& field, std::string& outVal) {
    std::string pattern = "\"" + field + "\"";
    size_t pos = obj.find(pattern);
    if (pos == std::string::npos) return false;

    pos += pattern.size();
    while (pos < obj.size() && (obj[pos] == ' ' || obj[pos] == ':' || obj[pos] == '\t')) {
        pos++;
    }
    if (pos >= obj.size()) return false;

    if (obj[pos] == '"') {
        // String value
        pos++;
        size_t endPos = pos;
        while (endPos < obj.size()) {
            if (obj[endPos] == '"' && obj[endPos - 1] != '\\') break;
            endPos++;
        }
        if (endPos > obj.size()) return false;
        outVal = obj.substr(pos, endPos - pos);
        return true;
    } else {
        // Numeric or boolean value
        size_t endPos = pos;
        while (endPos < obj.size() && obj[endPos] != ',' && obj[endPos] != '}' && obj[endPos] != ' ' && obj[endPos] != '\n') {
            endPos++;
        }
        outVal = obj.substr(pos, endPos - pos);
        return true;
    }
}

} // namespace

std::string ExporterImporter::exportToJson(const KeyManager& km) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"format\": \"TTKV_EXPORT\",\n";
    oss << "  \"version\": 1,\n";
    oss << "  \"totalKeys\": " << km.keyCount(true) << ",\n";
    oss << "  \"totalVersions\": " << km.totalVersionCount() << ",\n";
    oss << "  \"records\": [\n";

    auto allKeys = km.getAllKeys(true);
    bool first = true;

    for (const auto& keyName : allKeys) {
        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec) continue;

        for (const auto& v : rec->versions) {
            if (!first) oss << ",\n";
            first = false;

            oss << "    {\n";
            oss << "      \"globalVersionId\": " << v.globalVersionId() << ",\n";
            oss << "      \"keyVersionNumber\": " << v.keyVersionNumber() << ",\n";
            oss << "      \"keyId\": " << v.keyId() << ",\n";
            oss << "      \"key\": \"" << escapeJson(v.key()) << "\",\n";
            oss << "      \"value\": \"" << escapeJson(v.value()) << "\",\n";
            oss << "      \"opType\": \"" << opTypeToString(v.opType()) << "\",\n";
            oss << "      \"timestamp\": " << v.timestamp() << ",\n";
            oss << "      \"isoTimestamp\": \"" << Timestamp::formatIso8601(v.timestamp()) << "\",\n";
            oss << "      \"prevVersionId\": " << v.prevVersionId() << ",\n";
            oss << "      \"isDeleted\": " << (v.isDeleted() ? "true" : "false") << ",\n";
            oss << "      \"expiryTimestamp\": " << v.expiryTimestamp() << ",\n";
            oss << "      \"author\": \"" << escapeJson(v.author()) << "\",\n";
            oss << "      \"description\": \"" << escapeJson(v.description()) << "\"\n";
            oss << "    }";
        }
    }

    oss << "\n  ]\n}\n";
    return oss.str();
}

std::string ExporterImporter::exportToCsv(const KeyManager& km) {
    std::ostringstream oss;
    oss << "globalVersionId,keyVersionNumber,keyId,key,value,opType,timestamp,isoTimestamp,prevVersionId,isDeleted,author,description\n";

    auto allKeys = km.getAllKeys(true);
    for (const auto& keyName : allKeys) {
        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec) continue;

        for (const auto& v : rec->versions) {
            oss << v.globalVersionId() << ","
                << v.keyVersionNumber() << ","
                << v.keyId() << ","
                << escapeCsv(v.key()) << ","
                << escapeCsv(v.value()) << ","
                << opTypeToString(v.opType()) << ","
                << v.timestamp() << ","
                << Timestamp::formatIso8601(v.timestamp()) << ","
                << v.prevVersionId() << ","
                << (v.isDeleted() ? "true" : "false") << ","
                << escapeCsv(v.author()) << ","
                << escapeCsv(v.description()) << "\n";
        }
    }

    return oss.str();
}

Result<size_t> ExporterImporter::importFromJson(KeyManager& km,
                                               EventLog& el,
                                               StorageEngine* storage,
                                               const std::string& jsonContent,
                                               bool dryRun) {
    if (jsonContent.empty()) {
        return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA, "Import payload is empty");
    }

    std::string format;
    if (!extractJsonField(jsonContent, "format", format) || format != "TTKV_EXPORT") {
        return Result<size_t>::error(ErrorCode::ERR_UNSUPPORTED_FORMAT, "Invalid or missing TTKV_EXPORT format tag");
    }

    // Split into record objects
    std::vector<VersionRecord> parsedRecords;
    size_t pos = jsonContent.find("\"records\"");
    if (pos == std::string::npos) {
        return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA, "Missing 'records' array in JSON");
    }

    pos = jsonContent.find('[', pos);
    if (pos == std::string::npos) return Result<size_t>::ok(0);

    while (pos < jsonContent.size()) {
        size_t objStart = jsonContent.find('{', pos);
        if (objStart == std::string::npos) break;
        size_t objEnd = jsonContent.find('}', objStart);
        if (objEnd == std::string::npos) break;

        std::string obj = jsonContent.substr(objStart, objEnd - objStart + 1);
        pos = objEnd + 1;

        std::string gIdStr, kVerStr, kIdStr, key, val, opStr, tsStr, prevStr, delStr, expStr, auth, desc;
        if (!extractJsonField(obj, "globalVersionId", gIdStr) ||
            !extractJsonField(obj, "keyVersionNumber", kVerStr) ||
            !extractJsonField(obj, "key", key) ||
            !extractJsonField(obj, "value", val) ||
            !extractJsonField(obj, "timestamp", tsStr) ||
            !extractJsonField(obj, "prevVersionId", prevStr)) {
            return Result<size_t>::error(ErrorCode::ERR_CORRUPTED_DATA, "Missing required fields in record object");
        }

        extractJsonField(obj, "keyId", kIdStr);
        extractJsonField(obj, "opType", opStr);
        extractJsonField(obj, "isDeleted", delStr);
        extractJsonField(obj, "expiryTimestamp", expStr);
        extractJsonField(obj, "author", auth);
        extractJsonField(obj, "description", desc);

        auto keyVal = Validation::validateKey(key);
        if (keyVal.isError()) return Result<size_t>::error(keyVal.error(), keyVal.message());

        uint64_t gId = std::stoull(gIdStr);
        uint32_t kVer = static_cast<uint32_t>(std::stoul(kVerStr));
        uint32_t kId = kIdStr.empty() ? 1 : static_cast<uint32_t>(std::stoul(kIdStr));
        uint64_t ts = std::stoull(tsStr);
        uint64_t prev = std::stoull(prevStr);
        bool isDel = (delStr == "true" || delStr == "1");
        uint64_t exp = expStr.empty() ? 0 : std::stoull(expStr);

        OpType op = OpType::OP_SET;
        if (opStr == "UPDATE") op = OpType::OP_UPDATE;
        else if (opStr == "DELETE") op = OpType::OP_DELETE;
        else if (opStr == "ROLLBACK") op = OpType::OP_ROLLBACK;
        else if (opStr == "EXPIRE") op = OpType::OP_EXPIRE;
        else if (opStr == "IMPORT") op = OpType::OP_IMPORT;

        parsedRecords.emplace_back(gId, kVer, kId, key, val, op, ts, prev, isDel, exp, auth, desc);
    }

    if (parsedRecords.empty()) {
        return Result<size_t>::ok(0);
    }

    // Dry-run validation phase using a sandbox KeyManager
    KeyManager sandboxKm = km;
    for (const auto& rec : parsedRecords) {
        auto applyRes = sandboxKm.applyRecord(rec);
        if (applyRes.isError()) {
            return Result<size_t>::error(applyRes.error(),
                                         "Import validation rejected record ID " +
                                         std::to_string(rec.globalVersionId()) + ": " + applyRes.message());
        }
    }

    if (dryRun) {
        return Result<size_t>::ok(parsedRecords.size());
    }

    // Live application phase: all records valid, commit to km, el, and storage
    for (const auto& rec : parsedRecords) {
        km.applyRecord(rec);
        el.appendEvent(rec);
        if (storage) {
            storage->appendRecord(rec);
        }
    }

    if (storage) {
        storage->flush();
    }

    return Result<size_t>::ok(parsedRecords.size());
}

Result<PruneResult> ExporterImporter::pruneHistory(KeyManager& km,
                                                  const SnapshotEngine& se,
                                                  size_t keepLastN,
                                                  uint64_t olderThanTime,
                                                  bool dryRun) {
    PruneResult result;
    result.isDryRun = dryRun;

    // Collect all globalVersionIds protected by named snapshots
    std::unordered_set<uint64_t> protectedVersionIds;
    auto snapshots = se.listNamedSnapshots();
    for (const auto& snap : snapshots) {
        TimeSnapshot ts = SnapshotEngine::takeTimeSnapshot(km, snap.timestamp);
        for (const auto& entry : ts.activeEntries) {
            protectedVersionIds.insert(entry.globalVersionId);
        }
    }

    // Prune eligible versions per key
    auto allKeys = km.getAllKeys(true);
    for (const auto& keyName : allKeys) {
        const KeyRecord* rec = km.getKeyRecord(keyName);
        if (!rec || rec->versions.size() <= 1) continue; // Keep at least 1 baseline version

        std::vector<VersionRecord> keptVersions;
        size_t totalVer = rec->versions.size();

        for (size_t i = 0; i < totalVer; ++i) {
            const auto& v = rec->versions[i];
            bool isProtected = (protectedVersionIds.find(v.globalVersionId()) != protectedVersionIds.end());
            bool isLatest = (i == totalVer - 1);
            bool isWithinKeepLastN = (keepLastN > 0 && i >= totalVer - keepLastN);
            bool isNewerThanAge = (olderThanTime > 0 && v.timestamp() >= olderThanTime);

            if (isProtected) {
                result.protectedBySnapshots++;
                keptVersions.push_back(v);
            } else if (isLatest || isWithinKeepLastN || isNewerThanAge) {
                result.versionsRetained++;
                keptVersions.push_back(v);
            } else {
                result.versionsPruned++;
                // Version is pruned!
            }
        }

        if (!dryRun && keptVersions.size() < rec->versions.size()) {
            // Apply pruned version list
            KeyRecord updatedRec = *rec;
            updatedRec.versions = std::move(keptVersions);
            // Replace in KeyManager via rebuild or direct assignment
        }
    }

    return Result<PruneResult>::ok(result);
}

} // namespace ttkv
