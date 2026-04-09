package score

import (
	"testing"

	"cloudops/go-tools/internal/config"
)

func TestEvaluateHealthyConfig(t *testing.T) {
	report := Evaluate(config.CloudOpsConfig{
		Environment:                 "Production",
		Region:                      "eu-west-1",
		HealthCheckIntervalSeconds:  30,
		ResourcePollIntervalSeconds: 20,
		AlertCheckIntervalSeconds:   15,
		CPUAlertThresholdPercent:    80,
		MemoryAlertThresholdPercent: 85,
		DiskAlertThresholdPercent:   90,
		MonitoredEndpoints: []string{
			"https://example.com/health",
			"https://example.com/ready",
		},
	})

	if report.Score != 100 {
		t.Fatalf("expected score 100, got %d", report.Score)
	}
	if len(report.Warnings) > 0 {
		t.Fatalf("expected no warnings, got %+v", report.Warnings)
	}
}

func TestEvaluateDuplicateEndpointPenalty(t *testing.T) {
	report := Evaluate(config.CloudOpsConfig{
		HealthCheckIntervalSeconds:  30,
		ResourcePollIntervalSeconds: 20,
		AlertCheckIntervalSeconds:   15,
		CPUAlertThresholdPercent:    80,
		MemoryAlertThresholdPercent: 85,
		DiskAlertThresholdPercent:   90,
		MonitoredEndpoints: []string{
			"https://example.com/health",
			"https://example.com/health",
		},
	})

	if report.Score >= 100 {
		t.Fatalf("expected score penalty for duplicate endpoints")
	}
	if len(report.Warnings) == 0 {
		t.Fatalf("expected warning for duplicate endpoints")
	}
}
