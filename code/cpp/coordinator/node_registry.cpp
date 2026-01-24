#include "node_registry.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace coordinator {

NodeRegistry::NodeRegistry(uint64_t heartbeat_timeout_ms, uint64_t cleanup_interval_ms)
    : total_heartbeats_(0),
      total_timeouts_(0),
      total_registrations_(0),
      next_node_sequence_(1),
      heartbeat_timeout_ms_(heartbeat_timeout_ms),
      cleanup_interval_ms_(cleanup_interval_ms),
      initialized_(false),
      shutdown_(false) {
}

NodeRegistry::~NodeRegistry() {
    shutdown();
}

bool NodeRegistry::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return true;
    }
    initialized_ = true;
    shutdown_ = false;
    return true;
}

void NodeRegistry::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (shutdown_) {
        return;
    }
    shutdown_ = true;
    initialized_ = false;
    nodes_.clear();
    nodes_by_type_.clear();
    nodes_by_capability_.clear();
    listeners_.clear();
}

bool NodeRegistry::registerNode(const NodeRegisterMessage& register_msg,
                                std::string& node_id,
                                uint64_t& session_token) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_ || shutdown_) {
        return false;
    }
    
    node_id = generateNodeId(register_msg.node_id);
    session_token = generateSessionToken();
    
    NodeInfo info;
    info.node_id = node_id;
    info.node_name = register_msg.node_name;
    info.node_type = register_msg.node_type;
    info.ip_address = register_msg.ip_address;
    info.port = register_msg.port;
    info.status = NodeStatus::HEALTHY;
    info.register_time_ms = getCurrentTimeMs();
    info.last_update_time_ms = info.register_time_ms;
    info.last_heartbeat_ms = info.register_time_ms;
    info.session_token = session_token;
    info.is_online = true;
    info.capabilities = register_msg.capabilities;
    info.metadata = register_msg.metadata;
    
    nodes_[node_id] = info;
    nodes_by_type_[info.node_type].insert(node_id);
    
    for (const auto& cap : info.capabilities) {
        nodes_by_capability_[cap.capability_id].insert(node_id);
    }
    
    total_registrations_++;
    notifyNodeRegistered(node_id);
    
    return true;
}

bool NodeRegistry::unregisterNode(const std::string& node_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    const auto& info = it->second;
    nodes_by_type_[info.node_type].erase(node_id);
    
    for (const auto& cap : info.capabilities) {
        nodes_by_capability_[cap.capability_id].erase(node_id);
    }
    
    nodes_.erase(it);
    notifyNodeUnregistered(node_id, reason);
    
    return true;
}

bool NodeRegistry::validateSession(const std::string& node_id, uint64_t session_token) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    return it->second.session_token == session_token;
}

bool NodeRegistry::hasNode(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.find(node_id) != nodes_.end();
}

bool NodeRegistry::getNodeInfo(const std::string& node_id, NodeInfo& info) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    info = it->second;
    return true;
}

std::vector<std::string> NodeRegistry::getAllNodeIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    ids.reserve(nodes_.size());
    
    for (const auto& pair : nodes_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

std::vector<std::string> NodeRegistry::getNodesByType(const std::string& node_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_by_type_.find(node_type);
    if (it == nodes_by_type_.end()) {
        return {};
    }
    
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

std::vector<std::string> NodeRegistry::getNodesByStatus(NodeStatus status) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    for (const auto& pair : nodes_) {
        if (pair.second.status == status) {
            ids.push_back(pair.first);
        }
    }
    
    return ids;
}

std::vector<std::string> NodeRegistry::getOnlineNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    for (const auto& pair : nodes_) {
        if (pair.second.is_online) {
            ids.push_back(pair.first);
        }
    }
    
    return ids;
}

std::vector<std::string> NodeRegistry::getNodesByCapability(const std::string& capability_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_by_capability_.find(capability_id);
    if (it == nodes_by_capability_.end()) {
        return {};
    }
    
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

std::vector<std::string> NodeRegistry::queryNodes(const NodeQueryCriteria& criteria) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    
    for (const auto& pair : nodes_) {
        if (matchesCriteria(pair.second, criteria)) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

std::string NodeRegistry::findBestNode(const std::string& node_type,
                                      const std::string& capability_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string best_node_id;
    double best_score = -1.0;
    
    for (const auto& pair : nodes_) {
        const auto& info = pair.second;
        
        if (!info.is_online || info.status == NodeStatus::FAULT) {
            continue;
        }
        
        if (!node_type.empty() && info.node_type != node_type) {
            continue;
        }
        
        if (!capability_id.empty()) {
            bool has_cap = false;
            for (const auto& cap : info.capabilities) {
                if (cap.capability_id == capability_id) {
                    has_cap = true;
                    break;
                }
            }
            if (!has_cap) {
                continue;
            }
        }
        
        double score = calculateNodeScore(info);
        if (score > best_score) {
            best_score = score;
            best_node_id = pair.first;
        }
    }
    
    return best_node_id;
}

bool NodeRegistry::updateNodeStatus(const std::string& node_id, NodeStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    NodeStatus old_status = it->second.status;
    if (old_status != status) {
        it->second.status = status;
        it->second.last_update_time_ms = getCurrentTimeMs();
        notifyNodeStatusChanged(node_id, old_status, status);
    }
    
    return true;
}

bool NodeRegistry::updateHeartbeat(const std::string& node_id, const HeartbeatMessage& heartbeat) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    it->second.last_heartbeat_ms = getCurrentTimeMs();
    it->second.last_update_time_ms = it->second.last_heartbeat_ms;
    it->second.status = heartbeat.status;
    
    if (heartbeat.cpu_usage_percent >= 0) {
        it->second.stats.cpu_usage_percent = heartbeat.cpu_usage_percent;
    }
    if (heartbeat.memory_usage_percent >= 0) {
        it->second.stats.memory_usage_percent = heartbeat.memory_usage_percent;
    }
    if (heartbeat.battery_percent >= 0) {
        it->second.stats.battery_percent = heartbeat.battery_percent;
    }
    if (heartbeat.active_tasks >= 0) {
        it->second.stats.active_tasks = heartbeat.active_tasks;
    }
    
    total_heartbeats_++;
    
    return true;
}

bool NodeRegistry::updateNodeStats(const std::string& node_id, const NodeInfo::Stats& stats) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    it->second.stats = stats;
    it->second.last_update_time_ms = getCurrentTimeMs();
    
    return true;
}

bool NodeRegistry::markNodeOffline(const std::string& node_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    if (it->second.is_online) {
        it->second.is_online = false;
        it->second.last_update_time_ms = getCurrentTimeMs();
        notifyNodeTimeout(node_id);
    }
    
    return true;
}

bool NodeRegistry::markNodeOnline(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    if (!it->second.is_online) {
        it->second.is_online = true;
        it->second.last_update_time_ms = getCurrentTimeMs();
        it->second.last_heartbeat_ms = it->second.last_update_time_ms;
        notifyNodeRecovered(node_id);
    }
    
    return true;
}

bool NodeRegistry::addNodeCapability(const std::string& node_id, const NodeCapability& capability) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    it->second.capabilities.push_back(capability);
    nodes_by_capability_[capability.capability_id].insert(node_id);
    
    return true;
}

bool NodeRegistry::removeNodeCapability(const std::string& node_id, const std::string& capability_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    auto& caps = it->second.capabilities;
    caps.erase(std::remove_if(caps.begin(), caps.end(),
                             [&capability_id](const NodeCapability& cap) {
                                 return cap.capability_id == capability_id;
                             }),
              caps.end());
    
    nodes_by_capability_[capability_id].erase(node_id);
    
    return true;
}

bool NodeRegistry::hasCapability(const std::string& node_id, const std::string& capability_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false;
    }
    
    for (const auto& cap : it->second.capabilities) {
        if (cap.capability_id == capability_id) {
            return true;
        }
    }
    
    return false;
}

size_t NodeRegistry::getNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
}

size_t NodeRegistry::getOnlineNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& pair : nodes_) {
        if (pair.second.is_online) {
            count++;
        }
    }
    
    return count;
}

NodeRegistry::RegistryStats NodeRegistry::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    RegistryStats stats;
    stats.total_nodes = nodes_.size();
    stats.total_heartbeats = total_heartbeats_;
    stats.total_timeouts = total_timeouts_;
    
    for (const auto& pair : nodes_) {
        const auto& info = pair.second;
        
        if (info.is_online) {
            stats.online_nodes++;
        } else {
            stats.offline_nodes++;
        }
        
        switch (info.status) {
            case NodeStatus::HEALTHY:
                stats.healthy_nodes++;
                break;
            case NodeStatus::DEGRADED:
                stats.degraded_nodes++;
                break;
            case NodeStatus::FAULT:
                stats.faulty_nodes++;
                break;
            default:
                break;
        }
        
        stats.nodes_by_type[info.node_type]++;
    }
    
    return stats;
}

void NodeRegistry::registerListener(INodeRegistryListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (listener) {
        listeners_.push_back(listener);
    }
}

void NodeRegistry::unregisterListener(INodeRegistryListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener),
                    listeners_.end());
}

size_t NodeRegistry::checkAndCleanupTimeoutNodes() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t current_time = getCurrentTimeMs();
    size_t timeout_count = 0;
    
    for (auto& pair : nodes_) {
        auto& info = pair.second;
        
        if (info.is_online) {
            uint64_t time_since_heartbeat = current_time - info.last_heartbeat_ms;
            if (time_since_heartbeat > heartbeat_timeout_ms_) {
                info.is_online = false;
                total_timeouts_++;
                timeout_count++;
                notifyNodeTimeout(pair.first);
            }
        }
    }
    
    return timeout_count;
}

size_t NodeRegistry::cleanupOfflineNodes() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> to_remove;
    
    for (const auto& pair : nodes_) {
        if (!pair.second.is_online) {
            to_remove.push_back(pair.first);
        }
    }
    
    for (const auto& node_id : to_remove) {
        auto it = nodes_.find(node_id);
        if (it != nodes_.end()) {
            const auto& info = it->second;
            nodes_by_type_[info.node_type].erase(node_id);
            
            for (const auto& cap : info.capabilities) {
                nodes_by_capability_[cap.capability_id].erase(node_id);
            }
            
            nodes_.erase(it);
        }
    }
    
    return to_remove.size();
}

void NodeRegistry::resetNodeStats(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        it->second.stats = NodeInfo::Stats();
    }
}

std::string NodeRegistry::exportToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"total_nodes\": " << nodes_.size() << ",\n";
    oss << "  \"nodes\": [\n";
    
    bool first = true;
    for (const auto& pair : nodes_) {
        if (!first) {
            oss << ",\n";
        }
        first = false;
        
        const auto& info = pair.second;
        oss << "    {\n";
        oss << "      \"node_id\": \"" << info.node_id << "\",\n";
        oss << "      \"node_name\": \"" << info.node_name << "\",\n";
        oss << "      \"node_type\": \"" << info.node_type << "\",\n";
        oss << "      \"status\": " << static_cast<int>(info.status) << ",\n";
        oss << "      \"is_online\": " << (info.is_online ? "true" : "false") << "\n";
        oss << "    }";
    }
    
    oss << "\n  ]\n";
    oss << "}\n";
    
    return oss.str();
}

void NodeRegistry::printSummary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "Node Registry Summary:\n";
    std::cout << "  Total Nodes: " << nodes_.size() << "\n";
    std::cout << "  Online Nodes: " << getOnlineNodeCount() << "\n";
    std::cout << "  Total Heartbeats: " << total_heartbeats_ << "\n";
    std::cout << "  Total Timeouts: " << total_timeouts_ << "\n";
}


std::string NodeRegistry::generateNodeId(const std::string& base_id) {
    if (nodes_.find(base_id) == nodes_.end()) {
        return base_id;
    }
    
    uint32_t seq = next_node_sequence_++;
    return base_id + "_" + std::to_string(seq);
}

uint64_t NodeRegistry::generateSessionToken() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    return dis(gen);
}

uint64_t NodeRegistry::getCurrentTimeMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

double NodeRegistry::calculateNodeScore(const NodeInfo& info) const {
    double score = 1000.0;
    
    switch (info.status) {
        case NodeStatus::HEALTHY:
            score += 100.0;
            break;
        case NodeStatus::DEGRADED:
            score += 50.0;
            break;
        case NodeStatus::FAULT:
            score -= 100.0;
            break;
        default:
            break;
    }
    
    score -= info.stats.cpu_usage_percent * 0.5;

    score -= info.stats.memory_usage_percent * 0.3;

    score -= info.stats.active_tasks * 10.0;

    score -= info.stats.average_rtt_ms * 0.1;
    
    return score;
}

void NodeRegistry::notifyNodeRegistered(const std::string& node_id) {
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onNodeRegistered(node_id);
        }
    }
}

void NodeRegistry::notifyNodeUnregistered(const std::string& node_id, const std::string& reason) {
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onNodeUnregistered(node_id, reason);
        }
    }
}

void NodeRegistry::notifyNodeStatusChanged(const std::string& node_id, 
                                          NodeStatus old_status, 
                                          NodeStatus new_status) {
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onNodeStatusChanged(node_id, old_status, new_status);
        }
    }
}

void NodeRegistry::notifyNodeTimeout(const std::string& node_id) {
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onNodeTimeout(node_id);
        }
    }
}

void NodeRegistry::notifyNodeRecovered(const std::string& node_id) {
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onNodeRecovered(node_id);
        }
    }
}

bool NodeRegistry::matchesCriteria(const NodeInfo& info, const NodeQueryCriteria& criteria) const {
    if (!criteria.node_ids.empty() && criteria.node_ids.find(info.node_id) == criteria.node_ids.end()) {
        return false;
    }
    
    if (!criteria.node_types.empty() && criteria.node_types.find(info.node_type) == criteria.node_types.end()) {
        return false;
    }
    
    if (!criteria.statuses.empty() && criteria.statuses.find(info.status) == criteria.statuses.end()) {
        return false;
    }
    
    if (criteria.only_online && !info.is_online) {
        return false;
    }
    
    if (criteria.has_capability) {
        bool found = false;
        for (const auto& cap : info.capabilities) {
            if (cap.capability_id == criteria.capability_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    
    return true;
}

} // namespace coordinator
