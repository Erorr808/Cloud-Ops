package score

import (
	"fmt"
	"strings"

	"cloudops/go-tools/internal/config"
)

type Report struct {
	Score           int
	Warnings        []string
	Recommendations []string
}

func Evaluate(cfg config.CloudOpsConfig) Report {
	report := Report{Score: 100}

	if cfg.HealthCheckIntervalSeconds <= 0 || cfg.ResourcePollIntervalSeconds <= 0 || cfg.AlertCheckIntervalSeconds <= 0 {
		report.Score -= 30
		report.Warnings = append(report.Warnings, "polling intervals must be greater than zero")
	}

	if cfg.AlertCheckIntervalSeconds > cfg.ResourcePollIntervalSeconds {
		report.Score -= 8
		report.Recommendations = append(report.Recommendations,
			"set AlertCheckIntervalSeconds <= ResourcePollIntervalSeconds for faster alerting")
	}

	dupes := duplicatedEndpoints(cfg.MonitoredEndpoints)
	if len(dupes) > 0 {
		report.Score -= 10
		report.Warnings = append(report.Warnings,
			fmt.Sprintf("duplicate endpoints found: %s", strings.Join(dupes, ", ")))
		report.Recommendations = append(report.Recommendations,
			"deduplicate monitored endpoints to avoid duplicate probes")
	}

	if len(cfg.MonitoredEndpoints) < 2 {
		report.Score -= 5
		report.Recommendations = append(report.Recommendations,
			"monitor at least two endpoints to improve health signal coverage")
	}

	thresholds := []float64{cfg.CPUAlertThresholdPercent, cfg.MemoryAlertThresholdPercent, cfg.DiskAlertThresholdPercent}
	for _, t := range thresholds {
		if t < 50 || t > 99 {
			report.Score -= 8
			report.Warnings = append(report.Warnings,
				"alert thresholds should typically stay between 50 and 99 percent")
			break
		}
	}

	if report.Score < 0 {
		report.Score = 0
	}

	return report
}

func duplicatedEndpoints(endpoints []string) []string {
	seen := map[string]int{}
	dupes := []string{}
	for _, ep := range endpoints {
		seen[ep]++
		if seen[ep] == 2 {
			dupes = append(dupes, ep)
		}
	}
	return dupes
}
