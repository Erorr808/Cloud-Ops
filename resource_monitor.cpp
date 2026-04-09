#include "resource_monitor.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace cloudops {

ResourceMonitor::ResourceMonitor(Thresholds thresholds)
    : thresholds_(thresholds) {}

void ResourceMonitor::set_on_metric(MetricCallback cb) { on_metric_ = std::move(cb); }
void ResourceMonitor::set_on_alert(MetricCallback cb)  { on_alert_  = std::move(cb); }

ResourceMetrics ResourceMonitor::evaluate(ResourceMetrics metrics) const {
    metrics.collected_at = std::chrono::system_clock::now();
    metrics.status = classify(metrics.cpu_percent, metrics.memory_percent, metrics.disk_percent);

    if (on_metric_) on_metric_(metrics);

    if (metrics.status == ResourceStatus::Critical ||
        metrics.status == ResourceStatus::Degraded) {
        if (on_alert_) on_alert_(metrics);
    }

    return metrics;
}

ResourceStatus ResourceMonitor::classify(double cpu, double mem, double disk) const {
    if (cpu  >= thresholds_.cpu_critical  ||
        mem  >= thresholds_.mem_critical  ||
        disk >= thresholds_.disk_critical) {
        return ResourceStatus::Critical;
    }
    if (cpu  >= thresholds_.cpu_warning   ||
        mem  >= thresholds_.mem_warning   ||
        disk >= thresholds_.disk_warning) {
        return ResourceStatus::Degraded;
    }
    return ResourceStatus::Healthy;
}

std::string ResourceMonitor::status_string(ResourceStatus s) {
    switch (s) {
        case ResourceStatus::Healthy:  return "HEALTHY";
        case ResourceStatus::Degraded: return "DEGRADED";
        case ResourceStatus::Critical: return "CRITICAL";
        default:                       return "UNKNOWN";
    }
}

std::string ResourceMonitor::format_bytes(long long bytes) {
    std::ostringstream oss;
    if      (bytes >= 1'000'000'000) oss << std::fixed << std::setprecision(2) << bytes / 1e9  << " GB";
    else if (bytes >= 1'000'000)     oss << std::fixed << std::setprecision(2) << bytes / 1e6  << " MB";
    else if (bytes >= 1'000)         oss << std::fixed << std::setprecision(2) << bytes / 1e3  << " KB";
    else                             oss << bytes << " B";
    return oss.str();
}

} // namespace cloudops


// ─── Demo main ──────────────────────────────────────────────────────────────
#include <cstdlib>
#include <ctime>

int main() {
    using namespace cloudops;

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    auto rand_pct = []{ return (std::rand() % 1000) / 10.0; };

    ResourceMonitor monitor;

    monitor.set_on_metric([](const ResourceMetrics& m) {
        std::cout << "[METRIC] " << m.resource_name
                  << " | CPU: "  << std::fixed << std::setprecision(1) << m.cpu_percent    << "%"
                  << " | Mem: "  << m.memory_percent << "%"
                  << " | Disk: " << m.disk_percent   << "%"
                  << " | "       << ResourceMonitor::status_string(m.status) << "\n";
    });

    monitor.set_on_alert([](const ResourceMetrics& m) {
        std::cerr << "[ALERT]  " << m.resource_name
                  << " is " << ResourceMonitor::status_string(m.status) << "!\n";
    });

    std::vector<std::string> resources = {
        "web-server-01", "db-primary", "cache-node", "storage-node"
    };

    std::cout << "=== CloudOps C++ Resource Monitor ===\n\n";

    for (const auto& name : resources) {
        ResourceMetrics m;
        m.resource_id   = name;
        m.resource_name = name;
        m.cpu_percent    = rand_pct();
        m.memory_percent = rand_pct();
        m.disk_percent   = rand_pct();
        m.net_in_bytes   = std::rand() % 100'000'000;
        m.net_out_bytes  = std::rand() % 50'000'000;

        monitor.evaluate(m);
    }

    return 0;
}
