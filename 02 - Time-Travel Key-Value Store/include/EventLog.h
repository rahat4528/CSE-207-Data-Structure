#pragma once

#include "Common.h"
#include "DataModel.h"
#include "KeyManager.h"
#include <vector>
#include <cstdint>

namespace ttkv {

// Immutable event representing a state-changing write mutation in the system
struct EventRecord {
    uint64_t eventId{0};
    uint64_t globalVersionId{0};
    uint64_t timestamp{0};
    OpType opType{OpType::OP_SET};
    std::string key;
    std::string value;
    uint64_t prevVersionId{0};
    uint32_t keyVersionNumber{0};
    uint64_t expiryTimestamp{0};
    bool isDeleted{false};
    std::string author;
    std::string description;

    static EventRecord fromVersionRecord(uint64_t eventId, const VersionRecord& v) {
        return EventRecord{
            eventId,
            v.globalVersionId(),
            v.timestamp(),
            v.opType(),
            v.key(),
            v.value(),
            v.prevVersionId(),
            v.keyVersionNumber(),
            v.expiryTimestamp(),
            v.isDeleted(),
            v.author(),
            v.description()
        };
    }

    VersionRecord toVersionRecord(uint32_t keyId) const {
        return VersionRecord(
            globalVersionId,
            keyVersionNumber,
            keyId,
            key,
            value,
            opType,
            timestamp,
            prevVersionId,
            isDeleted,
            expiryTimestamp,
            author,
            description
        );
    }
};

class EventLog {
public:
    EventLog();

    // Appends a new write mutation event
    uint64_t appendEvent(const VersionRecord& record);

    // Read-only access to sequential events
    const std::vector<EventRecord>& events() const { return m_events; }
    size_t count() const { return m_events.size(); }
    const EventRecord* getEvent(size_t index) const;

    // Replays all events from beginning to reconstruct a KeyManager state from scratch
    Result<void> replayInto(KeyManager& targetKm) const;

    void clear();

private:
    std::vector<EventRecord> m_events;
    uint64_t m_nextEventId{1};
};

} // namespace ttkv
