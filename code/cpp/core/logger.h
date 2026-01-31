#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <string>
#include <mutex>
#include <iostream>
#include <sstream>

namespace core {

/**
 * 线程安全的全局日志类
 * 确保多线程环境下日志输出不会交错
 */
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // 禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 日志输出方法 - 确保整行原子输出
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << message << std::endl;
    }

    void log(const std::string& prefix, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << prefix << message << std::endl;
    }

    void logError(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << message << std::endl;
    }

    void logError(const std::string& prefix, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << prefix << message << std::endl;
    }

    // 带格式的日志输出
    template<typename... Args>
    void logf(const char* format, Args... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        printf(format, args...);
        fflush(stdout);
    }

    // 多行日志 - 保证多行作为一个整体输出
    void logMultiLine(const std::vector<std::string>& lines) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& line : lines) {
            std::cout << line << std::endl;
        }
    }

    // 获取互斥锁（用于需要多次输出的场景）
    std::mutex& getMutex() {
        return mutex_;
    }

private:
    Logger() = default;
    std::mutex mutex_;
};

// 便捷宏定义
#define LOG(msg) core::Logger::getInstance().log(msg)
#define LOG_PREFIX(prefix, msg) core::Logger::getInstance().log(prefix, msg)
#define LOG_ERROR(msg) core::Logger::getInstance().logError(msg)
#define LOG_ERROR_PREFIX(prefix, msg) core::Logger::getInstance().logError(prefix, msg)

// 线程安全的日志锁定范围
#define LOG_LOCK() std::lock_guard<std::mutex> _log_lock_(core::Logger::getInstance().getMutex())

} // namespace core

#endif // CORE_LOGGER_H
