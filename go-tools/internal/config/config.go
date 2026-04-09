package config

import (
	"encoding/json"
	"fmt"
	"os"
)

// AppSettings maps the minimal JSON shape needed by this QA tool.
type AppSettings struct {
	CloudOps CloudOpsConfig `json:"CloudOps"`
}

// CloudOpsConfig mirrors the .NET appsettings section used by CloudOps.
type CloudOpsConfig struct {
	Environment                 string   `json:"Environment"`
	Region                      string   `json:"Region"`
	HealthCheckIntervalSeconds  int      `json:"HealthCheckIntervalSeconds"`
	ResourcePollIntervalSeconds int      `json:"ResourcePollIntervalSeconds"`
	AlertCheckIntervalSeconds   int      `json:"AlertCheckIntervalSeconds"`
	CPUAlertThresholdPercent    float64  `json:"CpuAlertThresholdPercent"`
	MemoryAlertThresholdPercent float64  `json:"MemoryAlertThresholdPercent"`
	DiskAlertThresholdPercent   float64  `json:"DiskAlertThresholdPercent"`
	MonitoredEndpoints          []string `json:"MonitoredEndpoints"`
}

func Load(path string) (CloudOpsConfig, error) {
	content, err := os.ReadFile(path)
	if err != nil {
		return CloudOpsConfig{}, fmt.Errorf("read appsettings: %w", err)
	}

	var cfg AppSettings
	if err := json.Unmarshal(content, &cfg); err != nil {
		return CloudOpsConfig{}, fmt.Errorf("parse appsettings: %w", err)
	}

	return cfg.CloudOps, nil
}
