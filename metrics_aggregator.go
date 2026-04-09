package main

import (
	"fmt"
	"math"
	"math/rand"
	"sort"
	"sync"
	"time"
)

// MetricPoint is a single timestamped measurement.
type MetricPoint struct {
	Value     float64
	Timestamp time.Time
}

// MetricSeries stores a sliding window of measurements for one metric.
type MetricSeries struct {
	Name     string
	Unit     string
	points   []MetricPoint
	maxSize  int
	mu       sync.RWMutex
}

func NewMetricSeries(name, unit string, windowSize int) *MetricSeries {
	return &MetricSeries{Name: name, Unit: unit, maxSize: windowSize}
}

func (s *MetricSeries) Record(value float64) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.points = append(s.points, MetricPoint{Value: value, Timestamp: time.Now().UTC()})
	if len(s.points) > s.maxSize {
		s.points = s.points[1:]
	}
}

func (s *MetricSeries) Stats() (min, max, mean, p95 float64, count int) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	if len(s.points) == 0 {
		return
	}

	vals := make([]float64, len(s.points))
	sum := 0.0
	for i, p := range s.points {
		vals[i] = p.Value
		sum += p.Value
	}
	sort.Float64s(vals)

	count = len(vals)
	min = vals[0]
	max = vals[count-1]
	mean = sum / float64(count)
	p95 = vals[int(math.Ceil(float64(count)*0.95))-1]
	return
}

// MetricsAggregator manages multiple named series.
type MetricsAggregator struct {
	series map[string]*MetricSeries
	mu     sync.RWMutex
}

func NewMetricsAggregator() *MetricsAggregator {
	return &MetricsAggregator{series: make(map[string]*MetricSeries)}
}

func (a *MetricsAggregator) Register(name, unit string, windowSize int) {
	a.mu.Lock()
	defer a.mu.Unlock()
	a.series[name] = NewMetricSeries(name, unit, windowSize)
}

func (a *MetricsAggregator) Record(name string, value float64) {
	a.mu.RLock()
	s, ok := a.series[name]
	a.mu.RUnlock()
	if ok {
		s.Record(value)
	}
}

func (a *MetricsAggregator) PrintReport() {
	a.mu.RLock()
	defer a.mu.RUnlock()

	fmt.Println("┌─────────────────────────────────────────────────────────────┐")
	fmt.Printf("│  Metrics Report  │  %s UTC\n", time.Now().UTC().Format("2006-01-02 15:04:05"))
	fmt.Println("├─────────────────┬──────────┬──────────┬──────────┬──────────┤")
	fmt.Printf("│ %-15s │ %8s │ %8s │ %8s │ %8s │\n", "Metric", "Min", "Max", "Mean", "P95")
	fmt.Println("├─────────────────┼──────────┼──────────┼──────────┼──────────┤")

	for _, s := range a.series {
		min, max, mean, p95, count := s.Stats()
		if count == 0 {
			continue
		}
		fmt.Printf("│ %-15s │ %7.1f%s │ %7.1f%s │ %7.1f%s │ %7.1f%s │\n",
			s.Name,
			min, s.Unit, max, s.Unit,
			mean, s.Unit, p95, s.Unit)
	}
	fmt.Println("└─────────────────┴──────────┴──────────┴──────────┴──────────┘")
}

func main() {
	agg := NewMetricsAggregator()
	agg.Register("cpu",    "%",  120)
	agg.Register("memory", "%",  120)
	agg.Register("disk",   "%",  120)
	agg.Register("net_in", "MB", 120)

	rng := rand.New(rand.NewSource(time.Now().UnixNano()))

	// Simulate 60 samples
	fmt.Println("[MetricsAggregator] Simulating 60 metric samples...")
	for i := 0; i < 60; i++ {
		agg.Record("cpu",    20+rng.Float64()*70)
		agg.Record("memory", 40+rng.Float64()*50)
		agg.Record("disk",   30+rng.Float64()*60)
		agg.Record("net_in", rng.Float64()*500)
	}

	agg.PrintReport()
}
