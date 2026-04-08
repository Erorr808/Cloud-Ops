using CloudOps;
using CloudOps.Config;
using CloudOps.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

var host = Host.CreateDefaultBuilder(args)
    .ConfigureServices((ctx, services) =>
    {
        // Configuration
        services.Configure<CloudOpsConfig>(ctx.Configuration.GetSection("CloudOps"));

        // HTTP client for health checks
        services.AddHttpClient("health");

        // Core services — all singletons so state is shared across the service loops
        services.AddSingleton<ResourceManager>();
        services.AddSingleton<HealthMonitor>();
        services.AddSingleton<AlertEngine>();
        services.AddSingleton<DashboardReporter>();

        // Background service
        services.AddHostedService<CloudOpsService>();
    })
    .ConfigureLogging(logging =>
    {
        logging.ClearProviders();
        logging.AddConsole(opts => opts.FormatterName = "simple");
        logging.SetMinimumLevel(LogLevel.Information);
    })
    .Build();

await host.RunAsync();
