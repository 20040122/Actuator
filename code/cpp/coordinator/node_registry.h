#ifndef COORDINATOR_NODE_REGISTRY_H
#define COORDINATOR_NODE_REGISTRY_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <set>
#include <algorithm>

#include "message_types.h"

namespace coordinator {

struct NodeInfo {
    std::string     node_id;
    std::string     node_name;
    std::string     node_type;
    std::string     ip_address;
    uint16_t        port;
    NodeStatus      status;
    uint64_t        register_time_ms;
    uint64_t        last_update_time_ms;
    uint64_t        last_heartbeat_ms;
    uint64_t        session_token;
    bool            is_online;
    std::vector<NodeCapability> capabilities;
    std::map<std::string, std::string> metadata;
    
    struct Stats {
        uint64_t    uptime_ms;
        uint8_t     cpu_usage_percent;
        uint8_t     memory_usage_percent;
        int8_t      battery_percent;
        uint32_t    active_tasks;
        uint32_t    completed_tasks;
        uint32_t    failed_tasks;
        uint64_t    total_messages_sent;
        uint64_t    total_messages_received;
        uint64_t    average_rtt_ms;
        
        Stats() : uptime_ms(0), cpu_usage_percent(0), memory_usage_percent(0),
                  battery_percent(-1), active_tasks(0), completed_tasks(0),
                  failed_tasks(0), total_messages_sent(0), total_messages_received(0),
                  average_rtt_ms(0) {}
    } stats;
    
    NodeInfo() : port(0), status(NodeStatus::UNKNOWN), 
                 register_time_ms(0), last_update_time_ms(0), 
                 last_heartbeat_ms(0), session_token(0), is_online(false) {}
};

struct NodeQueryCriteria {
    std::set<std::string>   node_ids;
    std::set<std::string>   node_types;
    std::set<NodeStatus>    statuses;
    bool                    only_online;
    bool                    has_capability;
    std::string             capability_id;
    uint32_t                min_available_capacity;
    
    NodeQueryCriteria() : only_online(false), has_capability(false), 
                          min_available_capacity(0) {}
};

class INodeRegistryListener {
public:
    virtual ~INodeRegistryListener() = default;
    virtual void onNodeRegistered(const std::string& node_id) = 0;
    virtual void onNodeUnregistered(const std::string& node_id, const std::string& reason) = 0;
    virtual void onNodeStatusChanged(const std::string& node_id, 
                                     NodeStatus old_status, 
                                     NodeStatus new_status) = 0;
    virtual void onNodeTimeout(const std::string& node_id) = 0;
    virtual void onNodeRecovered(const std::string& node_id) = 0;
};

class NodeRegistry {
public:
    explicit NodeRegistry(uint64_t heartbeat_timeout_ms = 30000,
                         uint64_t cleanup_interval_ms = 10000);
    ~NodeRegistry();
    NodeRegistry(const NodeRegistry&) = delete;
    NodeRegistry& operator=(const NodeRegistry&) = delete;
    
    bool initialize();
    void shutdown();
    
    bool registerNode(const NodeRegisterMessage& register_msg,
                     std::string& node_id,
                     uint64_t& session_token);
    bool unregisterNode(const std::string& node_id, const std::string& reason = "");
    bool validateSession(const std::string& node_id, uint64_t session_token) const;
    
    bool hasNode(const std::string& node_id) const;
    bool getNodeInfo(const std::string& node_id, NodeInfo& info) const;
    std::vector<std::string> getAllNodeIds() const;
    std::vector<std::string> getNodesByType(const std::string& node_type) const;
    std::vector<std::string> getNodesByStatus(NodeStatus status) const;
    std::vector<std::string> getOnlineNodes() const;
    std::vector<std::string> getNodesByCapability(const std::string& capability_id) const;
    std::vector<std::string> queryNodes(const NodeQueryCriteria& criteria) const;
    std::string findBestNode(const std::string& node_type = "",
                            const std::string& capability_id = "") const;
    
    bool updateNodeStatus(const std::string& node_id, NodeStatus status);
    bool updateHeartbeat(const std::string& node_id, const HeartbeatMessage& heartbeat);
    bool updateNodeStats(const std::string& node_id, const NodeInfo::Stats& stats);
    bool markNodeOffline(const std::string& node_id, const std::string& reason = "");
    bool markNodeOnline(const std::string& node_id);
    
    bool addNodeCapability(const std::string& node_id, const NodeCapability& capability);
    bool removeNodeCapability(const std::string& node_id, const std::string& capability_id);
    bool hasCapability(const std::string& node_id, const std::string& capability_id) const;
    
    size_t getNodeCount() const;
    size_t getOnlineNodeCount() const;
    
    struct RegistryStats {
        size_t total_nodes;
        size_t online_nodes;
        size_t offline_nodes;
        size_t healthy_nodes;
        size_t degraded_nodes;
        size_t faulty_nodes;
        uint64_t total_heartbeats;
        uint64_t total_timeouts;
        std::map<std::string, size_t> nodes_by_type;
        
        RegistryStats() : total_nodes(0), online_nodes(0), offline_nodes(0),
                         healthy_nodes(0), degraded_nodes(0), faulty_nodes(0),
                         total_heartbeats(0), total_timeouts(0) {}
    };
    
    RegistryStats getStatistics() const;
    
    void registerListener(INodeRegistryListener* listener);
    void unregisterListener(INodeRegistryListener* listener);
    
    size_t checkAndCleanupTimeoutNodes();
    size_t cleanupOfflineNodes();
    void resetNodeStats(const std::string& node_id);
    std::string exportToJson() const;
    void printSummary() const;

private:
    std::string generateNodeId(const std::string& base_id);
    uint64_t generateSessionToken();
    uint64_t getCurrentTimeMs() const;
    double calculateNodeScore(const NodeInfo& info) const;
    void notifyNodeRegistered(const std::string& node_id);
    void notifyNodeUnregistered(const std::string& node_id, const std::string& reason);
    void notifyNodeStatusChanged(const std::string& node_id, 
                                NodeStatus old_status, 
                                NodeStatus new_status);
    void notifyNodeTimeout(const std::string& node_id);
    void notifyNodeRecovered(const std::string& node_id);
    bool matchesCriteria(const NodeInfo& info, const NodeQueryCriteria& criteria) const;
    
    std::unordered_map<std::string, NodeInfo> nodes_;
    std::unordered_map<std::string, std::set<std::string>> nodes_by_type_;
    std::unordered_map<std::string, std::set<std::string>> nodes_by_capability_;
    std::vector<INodeRegistryListener*> listeners_;
    uint64_t total_heartbeats_;
    uint64_t total_timeouts_;
    uint64_t total_registrations_;
    uint32_t next_node_sequence_;
    uint64_t heartbeat_timeout_ms_;
    uint64_t cleanup_interval_ms_;
    mutable std::mutex mutex_;
    bool initialized_;
    bool shutdown_;
};

}

#endif
