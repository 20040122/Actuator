#pragma once
#include <string>
#include <chrono>

namespace actuator {

enum class CommandState {
    NOT_STARTED,
    READY,
    RUNNING,
    SUCCESS,
    FAILED,
    BLOCKED
};

struct CommandStatus {
    std::string command_id;
    CommandState state;
    std::chrono::system_clock::time_point state_change_time;
    std::string error_message;
};

struct ConstraintResult {
    bool satisfied;
    bool can_wait;
    std::string reason;
    std::chrono::system_clock::time_point expected_ready_time;
};

}
