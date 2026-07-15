#pragma once

#include "SafetyState.hpp"
#include <mutex>
#include <string>

namespace redfox::safety {

// Appends one line per safety event to logDirectory/safety.log. When the
// active file exceeds maxBytes, it is renamed to safety.log.1 (overwriting
// any previous .1) and a fresh safety.log is started. This is intentionally
// simple (single backup file) rather than a general rotation scheme -- safety
// events are rare, so unbounded growth is the actual risk being guarded
// against, not needing deep history.
class SafetyEventLog {
public:
    explicit SafetyEventLog(std::string logDirectory, std::size_t maxBytes = 1'000'000);

    void record(SafetyEventCode code, SafetyState state);

private:
    void rotateIfNeeded();

    std::string logDirectory_;
    std::string activeLogPath_;
    std::string backupLogPath_;
    std::size_t maxBytes_;
    std::mutex mutex_;
};

} // namespace redfox::safety
