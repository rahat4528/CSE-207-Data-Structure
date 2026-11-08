#pragma once

#include <cstdint>
#include <chrono>
#include <atomic>
#include <memory>

namespace ttkv {

// Abstract Clock Interface for deterministic testing and time control
class IClock {
public:
    virtual ~IClock() = default;

    // Returns the current time as UTC microseconds since Unix Epoch (1970-01-01T00:00:00Z)
    virtual uint64_t nowMicros() = 0;
};

// System clock using std::chrono with Monotonic Floor guard (Hybrid Logical Clock behavior)
class SystemClock : public IClock {
public:
    SystemClock() : m_lastAssignedMicros(0) {}

    uint64_t nowMicros() override {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        uint64_t systemMicros = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
        );

        // Monotonic floor: ensure timestamps never regress or collide even if system clock shifts back
        uint64_t prev = m_lastAssignedMicros.load(std::memory_order_relaxed);
        uint64_t assigned = (systemMicros > prev) ? systemMicros : (prev + 1);

        while (!m_lastAssignedMicros.compare_exchange_weak(prev, assigned,
                                                          std::memory_order_release,
                                                          std::memory_order_relaxed)) {
            assigned = (systemMicros > prev) ? systemMicros : (prev + 1);
        }

        return assigned;
    }

private:
    std::atomic<uint64_t> m_lastAssignedMicros;
};

// Controllable Fake Clock for unit, property, and determinism testing
class FakeClock : public IClock {
public:
    explicit FakeClock(uint64_t initialMicros = 1700000000000000ULL) // 2023-11-14T22:13:20Z
        : m_currentMicros(initialMicros) {}

    uint64_t nowMicros() override {
        return m_currentMicros;
    }

    void setMicros(uint64_t micros) {
        m_currentMicros = micros;
    }

    void advanceMicros(uint64_t deltaMicros) {
        m_currentMicros += deltaMicros;
    }

    void advanceSeconds(uint64_t seconds) {
        m_currentMicros += seconds * 1000000ULL;
    }

private:
    uint64_t m_currentMicros;
};

} // namespace ttkv
