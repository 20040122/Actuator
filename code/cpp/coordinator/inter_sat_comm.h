// #ifndef COORDINATOR_INTER_SAT_COMM_H
// #define COORDINATOR_INTER_SAT_COMM_H

// #include <string>
// #include <vector>
// #include <map>
// #include <queue>
// #include <memory>
// #include <mutex>
// #include <condition_variable>
// #include <thread>
// #include <atomic>
// #include <functional>
// #include <chrono>
// #include <future>

// #include "message_types.h"

// // 平台相关的 socket 类型定义
// #ifdef _WIN32
//     #include <winsock2.h>
//     typedef SOCKET socket_t;
//     #define INVALID_SOCKET_VALUE INVALID_SOCKET
// #else
//     typedef int socket_t;
//     #define INVALID_SOCKET_VALUE (-1)
// #endif

// namespace coordinator {

// class InterSatComm;
// class MessageSerializer;

// struct CommConfig {
//     std::string     node_id;
//     std::string     node_name;
//     std::string     node_type;
//     std::string     bind_address;
//     uint16_t        bind_port;
//     uint32_t        heartbeat_interval_ms;
//     uint32_t        heartbeat_timeout_ms;
//     uint32_t        max_missed_heartbeats;
//     uint32_t        max_latency_ms;
//     uint32_t        retry_count;
//     uint32_t        retry_interval_ms;
//     uint32_t        message_queue_size;
//     uint32_t        send_buffer_size;
//     uint32_t        recv_buffer_size;
    
//     static CommConfig getDefault() {
//         CommConfig config;
//         config.node_id = "NODE_DEFAULT";
//         config.node_name = "Default Node";
//         config.node_type = "SATELLITE";
//         config.bind_address = "0.0.0.0";
//         config.bind_port = 8800;
//         config.heartbeat_interval_ms = 5000;
//         config.heartbeat_timeout_ms = 15000;
//         config.max_missed_heartbeats = 3;
//         config.max_latency_ms = 500;
//         config.retry_count = 3;
//         config.retry_interval_ms = 1000;
//         config.message_queue_size = 1000;
//         config.send_buffer_size = 65536;
//         config.recv_buffer_size = 65536;
//         return config;
//     }
// };

// struct RemoteNode {
//     std::string     node_id;
//     std::string     node_name;
//     std::string     node_type;
//     std::string     ip_address;
//     uint16_t        port;
//     NodeStatus      status;
//     uint64_t        session_token;
//     uint64_t        last_heartbeat_time_ms;
//     uint32_t        missed_heartbeats;
//     uint64_t        round_trip_time_ms;
//     uint64_t        messages_sent;
//     uint64_t        messages_received;
//     uint64_t        bytes_sent;
//     uint64_t        bytes_received;
//     uint64_t        connect_time_ms;
//     socket_t        socket_fd;
    
//     RemoteNode() : port(0), status(NodeStatus::UNKNOWN), session_token(0),
//                    last_heartbeat_time_ms(0), missed_heartbeats(0), round_trip_time_ms(0),
//                    messages_sent(0), messages_received(0), bytes_sent(0), bytes_received(0),
//                    connect_time_ms(0), socket_fd(INVALID_SOCKET_VALUE) {}
// };

// class IMessageHandler {
// public:
//     virtual ~IMessageHandler() = default;
//     virtual void onMessageReceived(const Message& message) = 0;
//     virtual void onNodeConnected(const std::string& node_id) = 0;
//     virtual void onNodeDisconnected(const std::string& node_id, const std::string& reason) = 0;
//     virtual void onNodeStatusChanged(const std::string& node_id, NodeStatus old_status, NodeStatus new_status) = 0;
//     virtual void onHeartbeatReceived(const std::string& node_id, const HeartbeatMessage& heartbeat) = 0;
//     virtual void onHeartbeatTimeout(const std::string& node_id) = 0;
//     virtual void onError(ErrorCode code, const std::string& message) = 0;
// };

// class MessageQueue {
// public:
//     explicit MessageQueue(size_t max_size = 1000);
//     ~MessageQueue();
//     bool push(const Message& message);
//     bool pushPriority(const Message& message);
//     bool pop(Message& message, uint32_t timeout_ms = 0);
//     bool tryPop(Message& message);
//     size_t size() const;
//     bool empty() const;
//     bool full() const;
//     void clear();
    
// private:
//     struct PriorityMessage {
//         Message message;
//         int priority;
//         uint64_t timestamp;
        
//         bool operator<(const PriorityMessage& other) const {
//             if (priority != other.priority) return priority > other.priority;
//             return timestamp > other.timestamp;
//         }
//     };
    
//     std::priority_queue<PriorityMessage> queue_;
//     size_t max_size_;
//     mutable std::mutex mutex_;
//     std::condition_variable cv_;
// };

// class MessageSerializer {
// public:
//     static std::vector<uint8_t> serialize(const Message& message);
//     static std::vector<uint8_t> serializeHeader(const MessageHeader& header);
//     static bool deserialize(const std::vector<uint8_t>& data, Message& message);
//     static bool deserializeHeader(const std::vector<uint8_t>& data, MessageHeader& header);
//     static std::vector<uint8_t> serializeHeartbeat(const HeartbeatMessage& msg);
//     static std::vector<uint8_t> serializeHeartbeatAck(const HeartbeatAck& msg);
//     static std::vector<uint8_t> serializeNodeRegister(const NodeRegisterMessage& msg);
//     static std::vector<uint8_t> serializeNodeRegisterAck(const NodeRegisterAck& msg);
//     static std::vector<uint8_t> serializeTaskAssign(const TaskAssignMessage& msg);
//     static std::vector<uint8_t> serializeTaskAssignAck(const TaskAssignAck& msg);
//     static std::vector<uint8_t> serializeTaskProgress(const TaskProgressMessage& msg);
//     static std::vector<uint8_t> serializeTaskComplete(const TaskCompleteMessage& msg);
//     static std::vector<uint8_t> serializeSemAcquireRequest(const SemaphoreAcquireRequest& msg);
//     static std::vector<uint8_t> serializeSemAcquireResponse(const SemaphoreAcquireResponse& msg);
//     static std::vector<uint8_t> serializeSemRelease(const SemaphoreReleaseMessage& msg);
//     static std::vector<uint8_t> serializeSemReleaseAck(const SemaphoreReleaseAck& msg);
//     static std::vector<uint8_t> serializeStateSyncRequest(const StateSyncRequest& msg);
//     static std::vector<uint8_t> serializeStateSyncResponse(const StateSyncResponse& msg);
//     static std::vector<uint8_t> serializeBarrierWait(const BarrierWaitMessage& msg);
//     static std::vector<uint8_t> serializeBarrierRelease(const BarrierReleaseMessage& msg);
//     static std::vector<uint8_t> serializeError(const ErrorResponse& msg);
//     static bool deserializeHeartbeat(const std::vector<uint8_t>& data, HeartbeatMessage& msg);
//     static bool deserializeHeartbeatAck(const std::vector<uint8_t>& data, HeartbeatAck& msg);
//     static bool deserializeNodeRegister(const std::vector<uint8_t>& data, NodeRegisterMessage& msg);
//     static bool deserializeNodeRegisterAck(const std::vector<uint8_t>& data, NodeRegisterAck& msg);
//     static bool deserializeTaskAssign(const std::vector<uint8_t>& data, TaskAssignMessage& msg);
//     static bool deserializeTaskAssignAck(const std::vector<uint8_t>& data, TaskAssignAck& msg);
//     static bool deserializeTaskProgress(const std::vector<uint8_t>& data, TaskProgressMessage& msg);
//     static bool deserializeTaskComplete(const std::vector<uint8_t>& data, TaskCompleteMessage& msg);
//     static bool deserializeSemAcquireRequest(const std::vector<uint8_t>& data, SemaphoreAcquireRequest& msg);
//     static bool deserializeSemAcquireResponse(const std::vector<uint8_t>& data, SemaphoreAcquireResponse& msg);
//     static bool deserializeSemRelease(const std::vector<uint8_t>& data, SemaphoreReleaseMessage& msg);
//     static bool deserializeSemReleaseAck(const std::vector<uint8_t>& data, SemaphoreReleaseAck& msg);
//     static bool deserializeStateSyncRequest(const std::vector<uint8_t>& data, StateSyncRequest& msg);
//     static bool deserializeStateSyncResponse(const std::vector<uint8_t>& data, StateSyncResponse& msg);
//     static bool deserializeBarrierWait(const std::vector<uint8_t>& data, BarrierWaitMessage& msg);
//     static bool deserializeBarrierRelease(const std::vector<uint8_t>& data, BarrierReleaseMessage& msg);
//     static bool deserializeError(const std::vector<uint8_t>& data, ErrorResponse& msg);
//     static uint32_t calculateChecksum(const std::vector<uint8_t>& data);
// };

// class InterSatComm {
// public:
//     explicit InterSatComm(const CommConfig& config = CommConfig::getDefault());
//     ~InterSatComm();
//     InterSatComm(const InterSatComm&) = delete;
//     InterSatComm& operator=(const InterSatComm&) = delete;
//     bool initialize();
//     bool start();
//     void stop();
//     bool isRunning() const { return running_.load(); }
//     void registerHandler(IMessageHandler* handler);
//     void unregisterHandler(IMessageHandler* handler);
//     using MessageCallback = std::function<void(const Message&)>;
//     void registerCallback(MessageType type, MessageCallback callback);
//     bool connectToNode(const std::string& ip_address, uint16_t port);
//     void disconnectNode(const std::string& node_id);
//     bool registerToCoordinator(const std::string& coordinator_addr, uint16_t port);
//     bool getNodeInfo(const std::string& node_id, RemoteNode& info) const;
//     std::vector<std::string> getConnectedNodes() const;
//     NodeStatus getNodeStatus(const std::string& node_id) const;
//     void updateLocalStatus(NodeStatus status);
//     bool sendMessage(const std::string& dest_node_id, const Message& message);
//     bool sendMessageWithRetry(const std::string& dest_node_id, const Message& message, 
//                               int retry_count = -1);
//     bool broadcastMessage(const Message& message);
//     bool sendAndWaitResponse(const std::string& dest_node_id, const Message& request,
//                              Message& response, uint32_t timeout_ms = 5000);
//     bool sendHeartbeat(const std::string& dest_node_id);
//     bool sendTaskAssign(const std::string& dest_node_id, const TaskAssignMessage& task);
//     bool sendTaskProgress(const std::string& dest_node_id, const TaskProgressMessage& progress);
//     bool sendTaskComplete(const std::string& dest_node_id, const TaskCompleteMessage& complete);
//     bool sendSemaphoreRequest(const std::string& dest_node_id, const SemaphoreAcquireRequest& request);
//     bool sendSemaphoreRelease(const std::string& dest_node_id, const SemaphoreReleaseMessage& release);
//     bool sendStateSyncRequest(const std::string& dest_node_id, const StateSyncRequest& request);
//     bool sendBarrierWait(const std::string& dest_node_id, const BarrierWaitMessage& barrier);
    
//     struct Statistics {
//         uint64_t total_messages_sent;
//         uint64_t total_messages_received;
//         uint64_t total_bytes_sent;
//         uint64_t total_bytes_received;
//         uint64_t failed_sends;
//         uint64_t timeouts;
//         uint64_t heartbeats_sent;
//         uint64_t heartbeats_received;
//         uint32_t connected_nodes;
//         uint64_t uptime_ms;
//     };
    
//     Statistics getStatistics() const;
//     void resetStatistics();
//     const CommConfig& getConfig() const { return config_; }
//     void updateConfig(const CommConfig& config);
    
// private:
//     void acceptThreadFunc();
//     void receiveThreadFunc();
//     void sendThreadFunc();
//     void heartbeatThreadFunc();
//     void processReceivedMessage(const Message& message);
//     void handleHeartbeat(const Message& message);
//     void handleNodeRegister(const Message& message);
//     void handleTaskMessage(const Message& message);
//     void handleSemaphoreMessage(const Message& message);
//     void handleStateSyncMessage(const Message& message);
//     void addNode(const RemoteNode& node);
//     void removeNode(const std::string& node_id);
//     void updateNodeHeartbeat(const std::string& node_id);
//     void checkNodeTimeouts();
//     Message buildMessage(MessageType type, const std::string& dest_node_id,
//                          const std::vector<uint8_t>& payload);
//     uint32_t getNextSequenceId();
//     uint64_t getCurrentTimeMs() const;
//     bool initializeSocket();
//     void cleanupSocket();
//     bool bindSocket();
//     bool acceptConnection(RemoteNode& node);
//     bool connectSocket(const std::string& ip, uint16_t port, socket_t& socket_fd);
//     bool sendData(socket_t socket_fd, const std::vector<uint8_t>& data);
//     bool receiveData(socket_t socket_fd, std::vector<uint8_t>& data);
    
//     CommConfig config_;
//     std::atomic<bool> running_;
//     std::atomic<bool> initialized_;
//     std::map<std::string, RemoteNode> nodes_;
//     mutable std::mutex nodes_mutex_;
//     MessageQueue send_queue_;
//     MessageQueue recv_queue_;
//     std::vector<IMessageHandler*> handlers_;
//     std::mutex handlers_mutex_;
//     std::map<MessageType, std::vector<MessageCallback>> callbacks_;
//     std::mutex callbacks_mutex_;
//     std::atomic<uint32_t> sequence_id_;
//     struct PendingRequest {
//         Message request;
//         std::shared_ptr<std::promise<Message>> response_promise;
//         uint64_t send_time_ms;
//         uint32_t timeout_ms;
//     };
//     std::map<uint32_t, PendingRequest> pending_requests_;
//     std::mutex pending_mutex_;
//     Statistics stats_;
//     mutable std::mutex stats_mutex_;
//     uint64_t start_time_ms_;
//     socket_t listen_socket_;
//     std::unique_ptr<std::thread> accept_thread_;
//     std::unique_ptr<std::thread> receive_thread_;
//     std::unique_ptr<std::thread> send_thread_;
//     std::unique_ptr<std::thread> heartbeat_thread_;
//     NodeStatus local_status_;
//     std::mutex local_status_mutex_;
// };

// class MessageHandlerBase : public IMessageHandler {
// public:
//     void onMessageReceived(const Message& message) override {}
//     void onNodeConnected(const std::string& node_id) override {}
//     void onNodeDisconnected(const std::string& node_id, const std::string& reason) override {}
//     void onNodeStatusChanged(const std::string& node_id, NodeStatus old_status, NodeStatus new_status) override {}
//     void onHeartbeatReceived(const std::string& node_id, const HeartbeatMessage& heartbeat) override {}
//     void onHeartbeatTimeout(const std::string& node_id) override {}
//     void onError(ErrorCode code, const std::string& message) override {}
// };

// } // namespace coordinator

// #endif // COORDINATOR_INTER_SAT_COMM_H
