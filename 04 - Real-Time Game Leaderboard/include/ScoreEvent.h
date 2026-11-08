#ifndef SCORE_EVENT_H
#define SCORE_EVENT_H

#include "Common.h"
#include <string>

namespace LeaderboardEngine {

struct ScoreEvent {
    std::string eventId;
    std::string playerId;
    std::string category;
    score_t value{0};
    ScoreUpdateMode mode{ScoreUpdateMode::SET};
    timestamp_t timestamp{0};
    std::string metadata;

    ScoreEvent() = default;
    ScoreEvent(std::string eid, std::string pid, std::string cat, score_t val,
               ScoreUpdateMode m = ScoreUpdateMode::SET, timestamp_t ts = 0, std::string meta = "")
        : eventId(std::move(eid)), playerId(std::move(pid)), category(std::move(cat)),
          value(val), mode(m), timestamp(ts), metadata(std::move(meta)) {}

    bool operator==(const ScoreEvent& other) const {
        return eventId == other.eventId &&
               playerId == other.playerId &&
               category == other.category &&
               value == other.value &&
               mode == other.mode;
    }
};

struct ScoreUpdateResult {
    std::string playerId;
    score_t oldScore{0};
    score_t newScore{0};
    size_t oldRank{0};
    size_t newRank{0};
    uint64_t boardVersion{0};
    bool rankChanged{false};
};

} // namespace LeaderboardEngine

#endif // SCORE_EVENT_H
