package main

import (
	"flag"
	"fmt"
	"os"

	"cloudops/go-tools/internal/config"
	"cloudops/go-tools/internal/score"
)

func main() {
	configPath := flag.String("config", "appsettings.json", "path to appsettings.json")
	flag.Parse()

	cfg, err := config.Load(*configPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		os.Exit(1)
	}

	report := score.Evaluate(cfg)

	fmt.Printf("CloudOps configuration score: %d/100\n", report.Score)
	fmt.Printf("Environment: %s | Region: %s\n", cfg.Environment, cfg.Region)

	if len(report.Warnings) == 0 {
		fmt.Println("Warnings: none")
	} else {
		fmt.Println("Warnings:")
		for _, warning := range report.Warnings {
			fmt.Printf("  - %s\n", warning)
		}
	}

	if len(report.Recommendations) == 0 {
		fmt.Println("Recommendations: none")
	} else {
		fmt.Println("Recommendations:")
		for _, recommendation := range report.Recommendations {
			fmt.Printf("  - %s\n", recommendation)
		}
	}
}
