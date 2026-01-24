#ifndef INPUT_JSON_PARSER_H
#define INPUT_JSON_PARSER_H

#include <string>
#include <vector>
#include <cstdint>
#include "../core/types.h"

class ScheduleParser {
public:
    std::vector<TaskSegment> parseSatelliteTasks(
        const std::string& schedule_file,
        const std::string& satellite_id
    );
    // Structure to hold multi-satellite schedule data
    struct MultiSatSchedule {
        std::string plan_id;
        std::string schedule_id;
        std::map<std::string, std::vector<TaskSegment>> satellite_tasks;  
        std::vector<std::string> satellite_ids;
    };
    MultiSatSchedule parseAllSatellites(const std::string& schedule_file);
};
class BehaviorLibraryParser {
public:
    BehaviorNode parseBehaviorDefinition(
        const std::string& library_file,
        const std::string& behavior_name
    );
};
// Parser for global configuration settings
class GlobalConfigParser {
public:
    struct SemaphoreConfig {
        std::string semaphore_id;
        std::string resource_name;
        std::string resource_type;
        uint32_t max_permits;
        uint32_t available_permits;
        std::string queue_policy;
        uint32_t timeout_s;
        bool priority_enabled;
    };
    
    struct SyncBarrierConfig {
        std::string sync_id;
        std::string type;
        std::vector<std::string> participants;
        std::string anchor_time;
        uint32_t window_s;
        uint32_t timeout_s;
    };
    
    struct NodeCommConfig {
        std::string protocol;
        uint32_t max_latency_ms;
        uint32_t retry_count;
    };
    
    struct GlobalConfig {
        std::string plan_id;
        uint32_t total_nodes;
        std::vector<std::string> active_nodes;
        NodeCommConfig node_communication;
        std::vector<SemaphoreConfig> semaphores;
        std::vector<SyncBarrierConfig> barriers;
        bool deadlock_detection;
        std::string deadlock_resolution;
    };
    GlobalConfig parse(const std::string& config_file);
};

#endif