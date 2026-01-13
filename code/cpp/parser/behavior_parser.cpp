#include "behavior_parser.h"
#include <iostream>
#include <sstream>


static std::string replaceVariables(const std::string& text, const std::map<std::string, std::string>& params) {
    std::string result = text;
    size_t pos = 0;
    while ((pos = result.find("${", pos)) != std::string::npos) {
        size_t end_pos = result.find("}", pos);
        if (end_pos == std::string::npos) break;
        std::string var_name = result.substr(pos + 2, end_pos - pos - 2);
        auto it = params.find(var_name);
        if (it != params.end()) {
            result.replace(pos, end_pos - pos + 1, it->second);
            pos += it->second.length();
        } else {
            pos = end_pos + 1;
        }
    }
    return result;
}

static void expandNode(
    const BehaviorNode& node,
    const std::map<std::string, std::string>& params,
    std::vector<BehaviorNode>& result) {
    BehaviorNode expanded_node = node;
    for (std::map<std::string, std::string>::iterator it = expanded_node.params.begin(); 
         it != expanded_node.params.end(); ++it) {
        it->second = replaceVariables(it->second, params);
    }
    if (node.type == NodeType::SEQUENCE || 
        node.type == NodeType::SELECTOR || 
        node.type == NodeType::PARALLEL) {
        result.push_back(expanded_node);
        std::cout << "展开容器节点: " << node.name 
                  << " (类型: ";
        if (node.type == NodeType::SEQUENCE) std::cout << "Sequence";
        else if (node.type == NodeType::SELECTOR) std::cout << "Selector";
        else if (node.type == NodeType::PARALLEL) std::cout << "Parallel";
        std::cout << ", 子节点数: " << node.children.size() << ")" << std::endl;
        for (size_t i = 0; i < node.children.size(); ++i) {
            expandNode(node.children[i], params, result);
        }
    } 
    else {
        result.push_back(expanded_node);
        std::cout << "展开叶子节点: " << node.name 
                  << " (类型: ";
        if (node.type == NodeType::ACTION) std::cout << "Action";
        else if (node.type == NodeType::CONDITION) std::cout << "Condition";
        std::cout << ", 命令: " << expanded_node.command << ")" << std::endl;
    }
}

std::vector<BehaviorNode> BehaviorTreeParser::instantiate(
    const BehaviorNode& definition,
    const std::map<std::string, std::string>& params) {
    std::vector<BehaviorNode> nodes;
    std::cout << "\n开始实例化行为树: " << definition.name << std::endl;
    std::cout << "参数列表:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = params.begin(); 
         it != params.end(); ++it) {
        std::cout << "  " << it->first << " = " << it->second << std::endl;
    }
    std::cout << std::endl;
    expandNode(definition, params, nodes);
    std::cout << "\n实例化完成，共生成 " << nodes.size() << " 个节点\n" << std::endl;
    return nodes;
}

void BehaviorTreeParser::substituteVariables(
    BehaviorNode& node,
    const std::map<std::string, std::string>& params) {
    for (std::map<std::string, std::string>::iterator it = node.params.begin(); 
         it != node.params.end(); ++it) {
        it->second = replaceVariables(it->second, params);
    }
}
