#include "EventLog.h"

namespace ttkv {

EventLog::EventLog() = default;

uint64_t EventLog::appendEvent(const VersionRecord& record) {
    uint64_t eid = m_nextEventId++;
    m_events.push_back(EventRecord::fromVersionRecord(eid, record));
    return eid;
}

const EventRecord* EventLog::getEvent(size_t index) const {
    if (index >= m_events.size()) return nullptr;
    return &m_events[index];
}

Result<void> EventLog::replayInto(KeyManager& targetKm) const {
    targetKm.clear();

    for (const auto& ev : m_events) {
        // Find existing keyId or assign a new one
        uint32_t keyId = 1;
        const auto* existing = targetKm.getKeyRecord(ev.key);
        if (existing) {
            keyId = existing->keyId;
        } else {
            keyId = targetKm.nextKeyId();
        }

        VersionRecord vr = ev.toVersionRecord(keyId);
        auto applyRes = targetKm.applyRecord(vr);
        if (applyRes.isError()) {
            return applyRes;
        }
    }

    return Result<void>::ok();
}

void EventLog::clear() {
    m_events.clear();
    m_nextEventId = 1;
}

} // namespace ttkv
