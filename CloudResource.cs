namespace CloudOps.Models;

public enum ResourceStatus
{
    Healthy,
    Degraded,
    Critical,
    Unknown
}

public enum ResourceType
{
    Compute,
    Storage,
    Network,
    Database,
    LoadBalancer
}

public class CloudResource
{
    public string Id { get; set; } = Guid.NewGuid().ToString("N")[..8].ToUpper();
    public string Name { get; set; } = string.Empty;
    public ResourceType Type { get; set; }
    public ResourceStatus Status { get; set; } = ResourceStatus.Unknown;
    public string Region { get; set; } = string.Empty;
    public Dictionary<string, string> Tags { get; set; } = new();
    public DateTime LastUpdated { get; set; } = DateTime.UtcNow;

    public double CpuUsagePercent { get; set; }
    public double MemoryUsagePercent { get; set; }
    public double DiskUsagePercent { get; set; }
    public long NetworkInBytes { get; set; }
    public long NetworkOutBytes { get; set; }

    public override string ToString() =>
        $"[{Id}] {Name} ({Type}) | Status: {Status} | CPU: {CpuUsagePercent:F1}% | Mem: {MemoryUsagePercent:F1}%";
}
