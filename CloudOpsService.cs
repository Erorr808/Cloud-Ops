using CloudOps.Config;
using CloudOps.Services;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

namespace CloudOps;

/// <summary>
/// The core CloudOps hosted service. Coordinates three parallel loops:
///   1. Resource metric polling
///   2. Endpoint health checks
///   3. Alert evaluation + dashboard reporting
/// </summary>
public class CloudOpsService : BackgroundService
{
    private readonly ILogger<CloudOpsService> _logger;
    private readonly CloudOpsConfig _config;
    private readonly ResourceManager _resourceManager;
    private readonly HealthMonitor _healthMonitor;
    private readonly AlertEngine _alertEngine;
    private readonly DashboardReporter _dashboard;

    public CloudOpsService(
        ILogger<CloudOpsService> logger,
        IOptions<CloudOpsConfig> config,
        ResourceManager resourceManager,
        HealthMonitor healthMonitor,
        AlertEngine alertEngine,
        DashboardReporter dashboard)
    {
        _logger          = logger;
        _config          = config.Value;
        _resourceManager = resourceManager;
        _healthMonitor   = healthMonitor;
        _alertEngine     = alertEngine;
        _dashboard       = dashboard;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        _logger.LogInformation(
            "CloudOps Service starting — Env: {Env} | Region: {Region}",
            _config.Environment, _config.Region);

        // Run the three loops concurrently
        await Task.WhenAll(
            ResourcePollingLoopAsync(stoppingToken),
            HealthCheckLoopAsync(stoppingToken),
            AlertAndDashboardLoopAsync(stoppingToken)
        );

        _logger.LogInformation("CloudOps Service stopped.");
    }

    // ── Loop 1: Resource Metrics ─────────────────────────────────────────────

    private async Task ResourcePollingLoopAsync(CancellationToken ct)
    {
        _logger.LogInformation("Resource polling started (every {S}s).", _config.ResourcePollIntervalSeconds);

        while (!ct.IsCancellationRequested)
        {
            try
            {
                await _resourceManager.PollMetricsAsync(ct);
            }
            catch (Exception ex) when (ex is not OperationCanceledException)
            {
                _logger.LogError(ex, "Error during resource metric polling.");
            }

            await DelayAsync(_config.ResourcePollIntervalSeconds, ct);
        }
    }

    // ── Loop 2: Health Checks ────────────────────────────────────────────────

    private async Task HealthCheckLoopAsync(CancellationToken ct)
    {
        _logger.LogInformation(
            "Health monitor started (every {S}s) — {N} endpoints.",
            _config.HealthCheckIntervalSeconds,
            _config.MonitoredEndpoints.Count);

        while (!ct.IsCancellationRequested)
        {
            try
            {
                var results = await _healthMonitor.CheckAllAsync(_config.MonitoredEndpoints, ct);
                _alertEngine.EvaluateHealthChecks(results);
            }
            catch (Exception ex) when (ex is not OperationCanceledException)
            {
                _logger.LogError(ex, "Error during health checks.");
            }

            await DelayAsync(_config.HealthCheckIntervalSeconds, ct);
        }
    }

    // ── Loop 3: Alert Evaluation + Dashboard ─────────────────────────────────

    private async Task AlertAndDashboardLoopAsync(CancellationToken ct)
    {
        _logger.LogInformation("Alert engine started (every {S}s).", _config.AlertCheckIntervalSeconds);

        while (!ct.IsCancellationRequested)
        {
            try
            {
                _alertEngine.EvaluateResources(_resourceManager.GetAll());
                _dashboard.PrintSummary();
            }
            catch (Exception ex) when (ex is not OperationCanceledException)
            {
                _logger.LogError(ex, "Error in alert/dashboard loop.");
            }

            await DelayAsync(_config.AlertCheckIntervalSeconds, ct);
        }
    }

    private static Task DelayAsync(int seconds, CancellationToken ct) =>
        Task.Delay(TimeSpan.FromSeconds(seconds), ct).ContinueWith(_ => { }, ct);
}
