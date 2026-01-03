#pragma once

namespace actuator::bt {

enum class NodeType {
    SEQUENCE,
    SELECTOR,
    PARALLEL,
    ACTION,
    CONDITION
};

}
