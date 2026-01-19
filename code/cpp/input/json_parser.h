#ifndef INPUT_JSON_PARSER_H
#define INPUT_JSON_PARSER_H

#include <string>
#include <vector>
#include "../core/types.h"

class ScheduleParser {
public:
    std::vector<TaskSegment> parseSatelliteTasks(
        const std::string& schedule_file,
        const std::string& satellite_id
    );
};
class BehaviorLibraryParser {
public:
    BehaviorNode parseBehaviorDefinition(
        const std::string& library_file,
        const std::string& behavior_name
    );
};

#endif