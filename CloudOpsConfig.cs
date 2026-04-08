namespace CloudOps.Config;

public class CloudOpsConfig
{
    public int HealthCheckIntervalSeconds { get; set; } = 30;
    public int ResourcePollIntervalSeconds { get; set; } = 60;
    public int AlertCheckIntervalSeconds { get; set; } = 15;

    public double CpuAlertThresholdPercent { get; set; } = 80.0;
    public double MemoryAlertThresholdPercent { get; set; } = 85.0;
    public double DiskAlertThresholdPercent { get; set; } = 90.0;

    public List<string> MonitoredEndpoints { get; set; } = new()
    {
        "https://httpbin.org/status/200",
        "https://httpbin.org/status/200"
    };

    public string Environment { get; set; } = "Production";
    public string Region { get; set; } = "eu-west-1";
}
