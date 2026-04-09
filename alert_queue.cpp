#include <iostream>
#include <queue>
#include <string>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace cloudops {

enum class Severity { Info, Warning, Critical };

struct Alert {
    std::string id;
    std::string title;
    std::string message;
    Severity    severity;
    std::chrono::system_clock::time_point fired_at;
};

inline std::string severity_str(Severity s) {
    switch (s) {
        case Severity::Info:     return "INFO";
        case Severity::Warning:  return "WARNING";
        case Severity::Critical: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

/// Thread-safe alert queue — producers push, a consumer thread drains it.
class AlertQueue {
public:
    void push(Alert alert) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            queue_.push(std::move(alert));
        }
        cv_.notify_one();
    }

    /// Blocks until an alert is available or stop() is called.
    bool pop(Alert& out) {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [this]{ return !queue_.empty() || stopped_; });
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void stop() {
        stopped_ = true;
        cv_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return queue_.size();
    }

private:
    std::queue<Alert>           queue_;
    mutable std::mutex          mu_;
    std::condition_variable     cv_;
    std::atomic<bool>           stopped_{false};
};

} // namespace cloudops


// ─── Demo ────────────────────────────────────────────────────────────────────
int main() {
    using namespace cloudops;

    AlertQueue queue;

    // Consumer thread — drains the queue and prints alerts
    std::thread consumer([&queue] {
        Alert alert;
        while (queue.pop(alert)) {
            std::cout << "[" << severity_str(alert.severity) << "] "
                      << alert.title << " — " << alert.message << "\n";
        }
        std::cout << "[AlertQueue] Consumer done.\n";
    });

    // Producer — push some sample alerts
    std::vector<Alert> samples = {
        {"A001", "High CPU",       "web-server-01 CPU at 93%",      Severity::Critical},
        {"A002", "Disk Warning",   "storage-node disk at 86%",      Severity::Warning},
        {"A003", "Endpoint Down",  "api.example.com unreachable",   Severity::Critical},
        {"A004", "Memory Info",    "db-primary memory at 61%",      Severity::Info},
    };

    std::cout << "=== CloudOps C++ Alert Queue ===\n\n";
    for (auto& a : samples) {
        a.fired_at = std::chrono::system_clock::now();
        queue.push(a);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    queue.stop();
    consumer.join();

    return 0;
}
