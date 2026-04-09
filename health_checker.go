package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"
)

// HealthStatus represents the result of a single endpoint check.
type HealthStatus struct {
	Endpoint   string        `json:"endpoint"`
	Status     string        `json:"status"`
	StatusCode int           `json:"status_code,omitempty"`
	LatencyMs  int64         `json:"latency_ms"`
	CheckedAt  time.Time     `json:"checked_at"`
	Error      string        `json:"error,omitempty"`
}

// HealthChecker runs concurrent health checks against a list of endpoints.
type HealthChecker struct {
	endpoints []string
	interval  time.Duration
	client    *http.Client
	results   []HealthStatus
	mu        sync.RWMutex
}

func NewHealthChecker(endpoints []string, interval time.Duration) *HealthChecker {
	return &HealthChecker{
		endpoints: endpoints,
		interval:  interval,
		client: &http.Client{
			Timeout: 10 * time.Second,
		},
	}
}

func (h *HealthChecker) check(ctx context.Context, endpoint string) HealthStatus {
	result := HealthStatus{
		Endpoint:  endpoint,
		CheckedAt: time.Now().UTC(),
	}

	start := time.Now()
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, endpoint, nil)
	if err != nil {
		result.Status = "error"
		result.Error = err.Error()
		return result
	}

	resp, err := h.client.Do(req)
	result.LatencyMs = time.Since(start).Milliseconds()

	if err != nil {
		result.Status = "unreachable"
		result.Error = err.Error()
		return result
	}
	defer resp.Body.Close()

	result.StatusCode = resp.StatusCode
	switch {
	case resp.StatusCode >= 200 && resp.StatusCode < 300 && result.LatencyMs < 2000:
		result.Status = "healthy"
	case resp.StatusCode >= 200 && resp.StatusCode < 300:
		result.Status = "degraded"
	default:
		result.Status = "unhealthy"
	}

	return result
}

func (h *HealthChecker) RunOnce(ctx context.Context) []HealthStatus {
	var wg sync.WaitGroup
	results := make([]HealthStatus, len(h.endpoints))

	for i, ep := range h.endpoints {
		wg.Add(1)
		go func(idx int, url string) {
			defer wg.Done()
			results[idx] = h.check(ctx, url)
		}(i, ep)
	}

	wg.Wait()

	h.mu.Lock()
	h.results = results
	h.mu.Unlock()

	return results
}

func (h *HealthChecker) Start(ctx context.Context) {
	ticker := time.NewTicker(h.interval)
	defer ticker.Stop()

	log.Printf("[HealthChecker] Starting — %d endpoints, interval: %s\n", len(h.endpoints), h.interval)

	for {
		select {
		case <-ctx.Done():
			log.Println("[HealthChecker] Shutting down.")
			return
		case <-ticker.C:
			results := h.RunOnce(ctx)
			h.printResults(results)
		}
	}
}

func (h *HealthChecker) printResults(results []HealthStatus) {
	for _, r := range results {
		icon := map[string]string{
			"healthy":     "✓",
			"degraded":    "⚠",
			"unhealthy":   "✗",
			"unreachable": "✗",
			"error":       "!",
		}[r.Status]

		fmt.Printf("  %s %-50s | %-12s | %dms\n", icon, r.Endpoint, r.Status, r.LatencyMs)
	}
}

func (h *HealthChecker) LatestResultsJSON() ([]byte, error) {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return json.MarshalIndent(h.results, "", "  ")
}

func main() {
	endpoints := []string{
		"https://httpbin.org/status/200",
		"https://httpbin.org/get",
		"https://httpbin.org/status/200",
	}

	checker := NewHealthChecker(endpoints, 30*time.Second)

	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer cancel()

	// Run once immediately on startup
	log.Println("[CloudOps] Go HealthChecker starting...")
	results := checker.RunOnce(ctx)
	checker.printResults(results)

	// Then run on interval
	checker.Start(ctx)
}
