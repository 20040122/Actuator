#include "json_parser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;
static NodeType stringToNodeType(const std::string& type_str) {
    if (type_str == "Sequence") return NodeType::SEQUENCE;
    if (type_str == "Selector") return NodeType::SELECTOR;
    if (type_str == "Parallel") return NodeType::PARALLEL;
    if (type_str == "Action") return NodeType::ACTION;
    if (type_str == "Condition") return NodeType::CONDITION;
    if (type_str == "SubTree") return NodeType::SUBTREE;
    return NodeType::ACTION; 
}
static BehaviorNode parseNodeFromJson(const json& node_json) {
    BehaviorNode node;
    node.name = node_json.value("name", "");
    node.type = stringToNodeType(node_json.value("type", "Action"));
    node.description = node_json.value("description", "");
    node.command = node_json.value("command", "");
    node.expression = node_json.value("expression", ""); 
    if (node_json.contains("params")) {
        for (auto it = node_json["params"].begin(); it != node_json["params"].end(); ++it) {
            const std::string& key = it.key();
            const json& value = it.value();
            if (value.is_string()) {
                node.params[key] = value.get<std::string>();
            } else if (value.is_number()) {
                node.params[key] = std::to_string(value.get<double>());
            } else if (value.is_boolean()) {
                node.params[key] = value.get<bool>() ? "true" : "false";
            }
        }
    }
    if (node_json.contains("variables")) {
        for (auto& var : node_json["variables"]) {
            node.variables.push_back(var.get<std::string>());
        }
    }
    if (node_json.contains("children")) {
        for (auto& child_json : node_json["children"]) {
            node.children.push_back(parseNodeFromJson(child_json));
        }
    }
    return node;
}
std::vector<TaskSegment> ScheduleParser::parseSatelliteTasks(
    const std::string& schedule_file,
    const std::string& satellite_id) {
    std::vector<TaskSegment> tasks;
    try {
        std::ifstream file(schedule_file);
        if (!file.is_open()) {
            std::cerr << "无法打开调度文件: " << schedule_file << std::endl;
            return tasks;
        }
        json schedule_json;
        file >> schedule_json;
        file.close(); 
        std::cout << "解析任务调度文件: " << schedule_file << std::endl;
        if (!schedule_json.contains("satellites")) {
            std::cerr << "调度文件缺少satellites字段" << std::endl;
            return tasks;
        }
        for (auto& satellite : schedule_json["satellites"]) {
            std::string sat_id = satellite.value("satellite_id", "");
            if (sat_id == satellite_id && satellite.contains("scheduled_tasks")) {
                for (auto& task_json : satellite["scheduled_tasks"]) {
                    TaskSegment task;
                    task.satellite_id = satellite_id;
                    task.segment_id = task_json.value("segment_id", "");
                    task.task_id = task_json.value("task_id", "");
                    task.behavior_ref = task_json.value("behavior_ref", "");
                    if (task_json.contains("execution")) {
                        auto& exec = task_json["execution"];
                        task.execution.planned_start = exec.value("planned_start", "");
                        task.execution.planned_end = exec.value("planned_end", "");
                        task.execution.duration_s = exec.value("duration_s", 0);
                    }       
                    if (task_json.contains("window")) {
                        auto& win = task_json["window"];
                        task.window.window_id = win.value("window_id", "");
                        task.window.window_seq = win.value("window_seq", 0);
                        task.window.start = win.value("start", "");
                        task.window.end = win.value("end", "");
                    }      
                    if (task_json.contains("behavior_params")) {
                        for (auto it = task_json["behavior_params"].begin(); it != task_json["behavior_params"].end(); ++it) {
                            const std::string& key = it.key();
                            const json& value = it.value();
                            if (value.is_string()) {
                                task.behavior_params[key] = value.get<std::string>();
                            } else if (value.is_number_integer()) {
                                task.behavior_params[key] = std::to_string(value.get<int>());
                            } else if (value.is_number_float()) {
                                std::ostringstream oss;
                                oss << value.get<double>();
                                task.behavior_params[key] = oss.str();
                            } else if (value.is_boolean()) {
                                task.behavior_params[key] = value.get<bool>() ? "true" : "false";
                            } else {
                                task.behavior_params[key] = value.dump();
                            }
                        }
                    }                   
                    tasks.push_back(task);
                }
                break;
            }
        }       
        std::cout << "找到 " << tasks.size() << " 个任务用于卫星 " << satellite_id << std::endl;    
    } catch (const json::exception& e) {
        std::cerr << "JSON解析错误: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
    } 
    return tasks;
}
BehaviorNode BehaviorLibraryParser::parseBehaviorDefinition(
    const std::string& library_file,
    const std::string& behavior_name) {
    BehaviorNode root_node;
    try {
        std::ifstream file(library_file);
        if (!file.is_open()) {
            std::cerr << "无法打开行为库文件: " << library_file << std::endl;
            return root_node;
        }    
        json library_json;
        file >> library_json;
        file.close();
        std::cout << "解析行为库文件: " << library_file << std::endl;    
        if (!library_json.contains("behavior_definitions")) {
            std::cerr << "行为库文件缺少behavior_definitions字段" << std::endl;
            return root_node;
        }
        auto& behaviors = library_json["behavior_definitions"];
        if (!behaviors.contains(behavior_name)) {
            std::cerr << "未找到行为定义: " << behavior_name << std::endl;
            return root_node;
        }
        root_node = parseNodeFromJson(behaviors[behavior_name]);
        std::cout << "成功加载行为定义: " << behavior_name 
                  << " (类型: " << behaviors[behavior_name].value("type", "Unknown") << ")" << std::endl;    
    } catch (const json::exception& e) {
        std::cerr << "JSON解析错误: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
    }
    return root_node;
}
