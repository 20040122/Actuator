#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "NodeType.h"

namespace actuator {
namespace bt {

enum class ParamType {
    INT,
    DOUBLE,
    STRING
};

class NodeParam {
public:
    NodeParam() : type_(ParamType::INT), int_val_(0) {}
    NodeParam(int val) : type_(ParamType::INT), int_val_(val) {}
    NodeParam(double val) : type_(ParamType::DOUBLE), double_val_(val) {}
    NodeParam(const std::string& val) : type_(ParamType::STRING), string_val_(val) {}
    NodeParam(const char* val) : type_(ParamType::STRING), string_val_(val) {}

    ParamType getType() const { return type_; }
    
    int asInt() const { return int_val_; }
    double asDouble() const { return double_val_; }
    const std::string& asString() const { return string_val_; }

private:
    ParamType type_;
    int int_val_;
    double double_val_;
    std::string string_val_;
};

struct BehaviorTreeNode {
    NodeType type;
    std::string name;
    std::vector<NodeParam> params;
    std::string expression;
    std::vector<std::shared_ptr<BehaviorTreeNode>> children;
};

typedef std::map<std::string, BehaviorTreeNode> BehaviorDefinitions;

}
}
