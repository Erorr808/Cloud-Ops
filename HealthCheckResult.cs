namespace CloudOps.Models;

public enum HealthStatus
{
    Healthy,
    Unhealthy,
    Degraded,
    Timeout
}

public class HealthCheckResult
{
    public string EndpointUrl { get; set; } = string.Empty;
    public HealthStatus Status { get; set; }
    public int? HttpStatusCode { get; set; }
    public long ResponseTimeMs { get; set; }
    public string? ErrorMessage { get; set; }
    public DateTime CheckedAt { get; set; } = DateTime.UtcNow;

    public override string ToString()
    {
        var statusIcon = Status switch
        {
            HealthStatus.Healthy => "✓",
            HealthStatus.Degraded => "⚠",
            HealthStatus.Unhealthy => "✗",
            HealthStatus.Timeout => "⏱",
            _ => "?"
        };
        return $"{statusIcon} {EndpointUrl} | {Status} | {ResponseTimeMs}ms" +
               (ErrorMessage != null ? $" | {ErrorMessage}" : "");
    }
}
