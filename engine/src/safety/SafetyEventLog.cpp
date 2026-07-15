#include "SafetyEventLog.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>

namespace redfox::safety {

namespace {

const char* eventName(SafetyEventCode code) {
    switch (code) {
        case SafetyEventCode::None: return "None";
        case SafetyEventCode::UiHeartbeatLost: return "UiHeartbeatLost";
        case SafetyEventCode::FrameStall: return "FrameStall";
        case SafetyEventCode::EmergencyStop: return "EmergencyStop";
        case SafetyEventCode::ManualArm: return "ManualArm";
        case SafetyEventCode::ManualReArm: return "ManualReArm";
    }
    return "Unknown";
}

const char* stateName(SafetyState state) {
    switch (state) {
        case SafetyState::Blanked: return "Blanked";
        case SafetyState::Armed: return "Armed";
        case SafetyState::EStopLatched: return "EStopLatched";
    }
    return "Unknown";
}

std::string timestampNow() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTimeT = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuffer{};
    localtime_s(&tmBuffer, &nowTimeT);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmBuffer);
    return std::string(buffer);
}

} // namespace

SafetyEventLog::SafetyEventLog(std::string logDirectory, std::size_t maxBytes)
    : logDirectory_(std::move(logDirectory)), maxBytes_(maxBytes) {
    std::filesystem::create_directories(logDirectory_);
    activeLogPath_ = logDirectory_ + "\\safety.log";
    backupLogPath_ = logDirectory_ + "\\safety.log.1";
}

void SafetyEventLog::rotateIfNeeded() {
    std::error_code ec;
    const auto size = std::filesystem::file_size(activeLogPath_, ec);
    if (ec || size < maxBytes_) {
        return;
    }
    std::filesystem::remove(backupLogPath_, ec);
    std::filesystem::rename(activeLogPath_, backupLogPath_, ec);
}

void SafetyEventLog::record(SafetyEventCode code, SafetyState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    rotateIfNeeded();

    std::ofstream file(activeLogPath_, std::ios::app);
    if (!file.is_open()) {
        return;
    }
    file << timestampNow() << " event=" << eventName(code)
         << " state=" << stateName(state) << "\n";
}

} // namespace redfox::safety
