use std::collections::VecDeque;
use std::fmt;

// ── MetricSeries ─────────────────────────────────────────────────────────

/// A fixed-capacity sliding window of f64 measurements.
#[derive(Debug)]
pub struct MetricSeries {
    pub name:     String,
    pub unit:     String,
    window:       VecDeque<f64>,
    capacity:     usize,
}

impl MetricSeries {
    pub fn new(name: &str, unit: &str, capacity: usize) -> Self {
        Self {
            name:     name.to_string(),
            unit:     unit.to_string(),
            window:   VecDeque::with_capacity(capacity),
            capacity,
        }
    }

    pub fn record(&mut self, value: f64) {
        if self.window.len() == self.capacity {
            self.window.pop_front();
        }
        self.window.push_back(value);
    }

    pub fn len(&self) -> usize { self.window.len() }
    pub fn is_empty(&self) -> bool { self.window.is_empty() }

    pub fn min(&self) -> Option<f64> {
        self.window.iter().cloned().reduce(f64::min)
    }

    pub fn max(&self) -> Option<f64> {
        self.window.iter().cloned().reduce(f64::max)
    }

    pub fn mean(&self) -> Option<f64> {
        if self.is_empty() { return None; }
        Some(self.window.iter().sum::<f64>() / self.len() as f64)
    }

    pub fn stddev(&self) -> Option<f64> {
        let mean = self.mean()?;
        let variance = self.window.iter()
            .map(|v| (v - mean).powi(2))
            .sum::<f64>() / self.len() as f64;
        Some(variance.sqrt())
    }

    pub fn percentile(&self, pct: f64) -> Option<f64> {
        if self.is_empty() { return None; }
        let mut sorted: Vec<f64> = self.window.iter().cloned().collect();
        sorted.sort_by(f64::total_cmp);
        let idx = ((pct / 100.0) * sorted.len() as f64).ceil() as usize;
        sorted.get(idx.saturating_sub(1)).cloned()
    }

    pub fn last(&self) -> Option<f64> {
        self.window.back().cloned()
    }

    pub fn stats(&self) -> MetricStats {
        MetricStats {
            name:   self.name.clone(),
            unit:   self.unit.clone(),
            count:  self.len(),
            min:    self.min().unwrap_or(0.0),
            max:    self.max().unwrap_or(0.0),
            mean:   self.mean().unwrap_or(0.0),
            stddev: self.stddev().unwrap_or(0.0),
            p50:    self.percentile(50.0).unwrap_or(0.0),
            p95:    self.percentile(95.0).unwrap_or(0.0),
            p99:    self.percentile(99.0).unwrap_or(0.0),
        }
    }
}

// ── MetricStats ───────────────────────────────────────────────────────────

#[derive(Debug)]
pub struct MetricStats {
    pub name:   String,
    pub unit:   String,
    pub count:  usize,
    pub min:    f64,
    pub max:    f64,
    pub mean:   f64,
    pub stddev: f64,
    pub p50:    f64,
    pub p95:    f64,
    pub p99:    f64,
}

impl fmt::Display for MetricStats {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{:<12} n={:<5} min={:6.1}{u} max={:6.1}{u} mean={:6.1}{u} p50={:6.1}{u} p95={:6.1}{u} p99={:6.1}{u} σ={:.2}",
            self.name, self.count,
            self.min, self.max, self.mean,
            self.p50, self.p95, self.p99,
            self.stddev,
            u = &self.unit,
        )
    }
}

// ── Main ──────────────────────────────────────────────────────────────────

fn main() {
    use std::time::{SystemTime, UNIX_EPOCH};

    // Simple deterministic pseudo-random for demo
    let seed = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().subsec_nanos();
    let mut rng = seed as f64;
    let mut next = move |min: f64, max: f64| -> f64 {
        rng = (rng * 1664525.0 + 1013904223.0) % 4294967296.0;
        min + (rng / 4294967296.0) * (max - min)
    };

    let mut cpu  = MetricSeries::new("cpu",    "%",  120);
    let mut mem  = MetricSeries::new("memory", "%",  120);
    let mut disk = MetricSeries::new("disk",   "%",  120);
    let mut lat  = MetricSeries::new("latency", "ms", 120);

    for _ in 0..100 {
        cpu.record(next(10.0, 95.0));
        mem.record(next(40.0, 90.0));
        disk.record(next(20.0, 80.0));
        lat.record(next(50.0, 3000.0));
    }

    println!("=== CloudOps Rust Metric Series ===\n");
    println!("{}", cpu.stats());
    println!("{}", mem.stats());
    println!("{}", disk.stats());
    println!("{}", lat.stats());
}
