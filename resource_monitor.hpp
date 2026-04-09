#pragma once
#ifndef RESOURCE_MONITOR_HPP
#define RESOURCE_MONITOR_HPP

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <optional>

namespace cloudops {

enum class ResourceStatus { Healthy, Degraded, Critical, Unknown };

struct ResourceMetrics {
    std::string resource_id;
    std::string resource_name;
    double      cpu_percent    = 0.0;
    double      memory_percent = 0.0;
    double      disk_percent   = 0.0;
    long long   net_in_bytes   = 0;
    long long   net_out_bytes  = 0;
    ResourceStatus status      = ResourceStatus::Unknown;
    std::chrono::system_clock::time_point collected_at;
};

struct Thresholds {
    double cpu_warning    = 75.0;
    double cpu_critical   = 90.0;
    double mem_warning    = 80.0;
    double mem_critical   = 95.0;
    double disk_warning   = 85.0;
    double disk_critical  = 95.0;
};

using MetricCallback = std::function<void(const ResourceMetrics&)>;

class ResourceMonitor {
public:
    explicit ResourceMonitor(Thresholds thresholds = {});

    void set_on_metric(MetricCallback cb);
    void set_on_alert(MetricCallback cb);

    ResourceMetrics evaluate(ResourceMetrics metrics) const;

    static std::string status_string(ResourceStatus s);
    static std::string format_bytes(long long bytes);

private:
    Thresholds      thresholds_;
    MetricCallback  on_metric_;
    MetricCallback  on_alert_;

    ResourceStatus classify(double cpu, double mem, double disk) const;
};

} // namespace cloudops

#endif // RESOURCE_MONITOR_HPP
