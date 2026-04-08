# CloudOps Service (.NET 10)

A production-ready CloudOps monitoring service built with .NET 10 Generic Host.

## Architecture

```
CloudOpsService (BackgroundService)
├── Loop 1 — ResourceManager        polls cloud resource metrics every 20s
├── Loop 2 — HealthMonitor          HTTP-checks endpoints every 30s
└── Loop 3 — AlertEngine            evaluates thresholds & prints dashboard every 15s
```

## Project Structure

```
CloudOps/
├── Program.cs                  Entry point, DI wiring
├── CloudOpsService.cs          Orchestrating BackgroundService
├── appsettings.json            Configuration
├── Config/
│   └── CloudOpsConfig.cs       Strongly-typed settings
├── Models/
│   ├── CloudResource.cs        Resource + metrics model
│   ├── HealthCheckResult.cs    Health check result model
│   └── Alert.cs                Alert model (fire/resolve lifecycle)
└── Services/
    ├── ResourceManager.cs      Tracks resources & simulates metric polling
    ├── HealthMonitor.cs        Async HTTP health checks
    ├── AlertEngine.cs          Threshold evaluation, alert lifecycle
    └── DashboardReporter.cs    Console dashboard output
```

## Configuration (`appsettings.json`)

| Key | Default | Description |
|-----|---------|-------------|
| `Environment` | `Production` | Deployment environment label |
| `Region` | `eu-west-1` | Cloud region |
| `HealthCheckIntervalSeconds` | `30` | How often to probe endpoints |
| `ResourcePollIntervalSeconds` | `20` | How often to refresh metrics |
| `AlertCheckIntervalSeconds` | `15` | How often to evaluate thresholds |
| `CpuAlertThresholdPercent` | `80` | CPU % that fires a warning alert |
| `MemoryAlertThresholdPercent` | `85` | Memory % threshold |
| `DiskAlertThresholdPercent` | `90` | Disk % threshold |
| `MonitoredEndpoints` | `[...]` | List of URLs to health-check |

## Running

```bash
dotnet run --project CloudOps.csproj
```

## Extending for Real Cloud Providers

- **AWS**: Replace `ResourceManager.PollMetricsAsync` with calls to `Amazon.CloudWatch` SDK
- **Azure**: Use `Azure.Monitor.Query` to pull metrics from Azure Monitor
- **GCP**: Use `Google.Cloud.Monitoring.V3` client library
- **Alerting**: Swap `AlertEngine` log output for webhooks to PagerDuty / Slack / OpsGenie
