## 阶段一：基础通信层 (星间通信管理器)
```shell
coordinator/
  ├── inter_sat_comm.h          # 星间通信接口定义
  ├── inter_sat_comm.cpp        # 消息收发实现
  └── message_types.h           # 消息类型定义
```
### 开发内容：

- 定义通信协议消息结构（任务分配、状态同步、信号量请求/响应）
- 实现节点间的消息传递机制（可用 socket/共享内存/消息队列）
- 心跳检测与节点状态监控

## 阶段二：多节点协调框架 (多星协调器)
```shell
coordinator/
  ├── multi_sat_coordinator.h    # 协调器主类
  ├── multi_sat_coordinator.cpp
  └── node_registry.h           # 节点注册表
```
### 开发内容：

- 节点注册与发现机制
- 协调器主循环（接收调度结果 → 分发任务）
- 任务生命周期管理
- 节点健康状态监控

## 阶段三：分布式资源管理 (资源共享管理器 + 信号量池)
```shell
coordinator/
  ├── distributed_semaphore.h    # 分布式信号量
  ├── distributed_semaphore.cpp
  ├── resource_pool.h           # 资源池管理
  └── resource_pool.cpp
```
### 开发内容：

- 将现有 SemaphoreManager 扩展为分布式版本
- 实现跨节点信号量获取/释放（通过协调器中转）
- 死锁检测与解决（已有配置：deadlock_detection）
- 优先级队列支持（queue_policy: PRIORITY）

## 阶段四：选址模块 (任务分配决策)
```shell
coordinator/
  ├── site_selector.h           # 选址策略接口
  ├── site_selector.cpp
  └── strategies/
        ├── earliest_access.h    # 最早访问策略
        ├── min_maneuver.h      # 最小机动策略
        ├── max_resource.h      # 最大资源策略
        └── load_balance.h      # 负载均衡策略
```
### 开发内容：

- 基于 global.json 中的 site_selection.strategies 实现
- 加权多策略组合评估
- 动态选址（mode: DYNAMIC）支持
- 故障降级（fallback 策略）

## 阶段五：多星并行执行
```shell
executor/
  ├── node_executor.h           # 单星节点执行器封装
  ├── node_executor.cpp
  └── parallel_runner.h         # 并行执行调度
```
### 开发内容：

- 将现有 SimpleExecutor 改造为可独立运行的节点执行器
- 多线程/多进程并行执行支持
- 同步屏障实现（sync_points.barriers）
- 执行结果汇总与上报