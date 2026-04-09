use std::collections::HashMap;
use std::fmt;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

// ── Models ────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub enum Severity {
    Info,
    Warning,
    Critical,
}

impl fmt::Display for Severity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Severity::Info     => write!(f, "INFO"),
            Severity::Warning  => write!(f, "WARNING"),
            Severity::Critical => write!(f, "CRITICAL"),
        }
    }
}

#[derive(Debug, Clone)]
pub struct Alert {
    pub id:            String,
    pub title:         String,
    pub message:       String,
    pub severity:      Severity,
    pub resource_name: Option<String>,
    pub fired_at:      Instant,
    pub resolved_at:   Option<Instant>,
}

impl Alert {
    pub fn new(title: &str, message: &str, severity: Severity) -> Self {
        Self {
            id:            uuid_short(),
            title:         title.to_string(),
            message:       message.to_string(),
            severity,
            resource_name: None,
            fired_at:      Instant::now(),
            resolved_at:   None,
        }
    }

    pub fn with_resource(mut self, name: &str) -> Self {
        self.resource_name = Some(name.to_string());
        self
    }

    pub fn age(&self) -> Duration {
        self.fired_at.elapsed()
    }

    pub fn is_resolved(&self) -> bool {
        self.resolved_at.is_some()
    }
}

impl fmt::Display for Alert {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let icon = match self.severity {
            Severity::Critical => "🔴",
            Severity::Warning  => "🟡",
            Severity::Info     => "🔵",
        };
        write!(f, "{} [{}] {} — {}", icon, self.severity, self.title, self.message)?;
        if let Some(ref r) = self.resource_name {
            write!(f, " ({})", r)?;
        }
        Ok(())
    }
}

// ── Thresholds ────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct Thresholds {
    pub cpu_warning:  f64,
    pub cpu_critical: f64,
    pub mem_warning:  f64,
    pub mem_critical: f64,
    pub disk_warning: f64,
    pub disk_critical: f64,
}

impl Default for Thresholds {
    fn default() -> Self {
        Self {
            cpu_warning:   75.0,
            cpu_critical:  90.0,
            mem_warning:   80.0,
            mem_critical:  95.0,
            disk_warning:  85.0,
            disk_critical: 95.0,
        }
    }
}

// ── Resource metrics ──────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct ResourceMetrics {
    pub id:              String,
    pub name:            String,
    pub cpu_percent:     f64,
    pub memory_percent:  f64,
    pub disk_percent:    f64,
}

// ── Alert Engine ──────────────────────────────────────────────────────────

pub struct AlertEngine {
    thresholds:    Thresholds,
    active:        Arc<Mutex<HashMap<String, Alert>>>,
    history:       Arc<Mutex<Vec<Alert>>>,
}

impl AlertEngine {
    pub fn new(thresholds: Thresholds) -> Self {
        Self {
            thresholds,
            active:  Arc::new(Mutex::new(HashMap::new())),
            history: Arc::new(Mutex::new(Vec::new())),
        }
    }

    pub fn evaluate(&self, metrics: &ResourceMetrics) {
        self.check_metric(&metrics.id, "cpu",  metrics.cpu_percent,
            self.thresholds.cpu_warning, self.thresholds.cpu_critical,
            "CPU Usage", &metrics.name);

        self.check_metric(&metrics.id, "mem",  metrics.memory_percent,
            self.thresholds.mem_warning, self.thresholds.mem_critical,
            "Memory Usage", &metrics.name);

        self.check_metric(&metrics.id, "disk", metrics.disk_percent,
            self.thresholds.disk_warning, self.thresholds.disk_critical,
            "Disk Usage", &metrics.name);
    }

    fn check_metric(
        &self,
        resource_id: &str,
        metric: &str,
        value: f64,
        warn: f64,
        crit: f64,
        label: &str,
        resource_name: &str,
    ) {
        let key = format!("{}:{}", resource_id, metric);

        if value >= crit {
            self.fire(&key, Alert::new(
                &format!("Critical {}", label),
                &format!("{} at {:.1}% (threshold: {}%)", label, value, crit),
                Severity::Critical,
            ).with_resource(resource_name));
        } else if value >= warn {
            self.fire(&key, Alert::new(
                &format!("High {}", label),
                &format!("{} at {:.1}% (threshold: {}%)", label, value, warn),
                Severity::Warning,
            ).with_resource(resource_name));
        } else {
            self.resolve(&key);
        }
    }

    fn fire(&self, key: &str, alert: Alert) {
        let mut active = self.active.lock().unwrap();
        if active.contains_key(key) {
            return; // already firing — idempotent
        }
        println!("  ALERT FIRED: {}", alert);
        active.insert(key.to_string(), alert.clone());
        self.history.lock().unwrap().push(alert);
    }

    fn resolve(&self, key: &str) {
        let mut active = self.active.lock().unwrap();
        if let Some(mut alert) = active.remove(key) {
            alert.resolved_at = Some(Instant::now());
            println!("  ✓ Resolved: {} (age: {:.1}s)", alert.title, alert.age().as_secs_f64());
        }
    }

    pub fn active_count(&self) -> usize {
        self.active.lock().unwrap().len()
    }

    pub fn print_active(&self) {
        let active = self.active.lock().unwrap();
        if active.is_empty() {
            println!("  No active alerts.");
            return;
        }
        for alert in active.values() {
            println!("  {}", alert);
        }
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────

fn uuid_short() -> String {
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};
    let mut h = DefaultHasher::new();
    Instant::now().hash(&mut h);
    format!("{:X}", h.finish() & 0xFFFFFFFF)
}

// ── Main ──────────────────────────────────────────────────────────────────

fn main() {
    println!("=== CloudOps Rust Alert Engine ===\n");

    let engine = AlertEngine::new(Thresholds::default());

    let resources = vec![
        ResourceMetrics { id: "r1".into(), name: "web-server-01".into(), cpu_percent: 92.0, memory_percent: 55.0, disk_percent: 40.0 },
        ResourceMetrics { id: "r2".into(), name: "db-primary".into(),    cpu_percent: 60.0, memory_percent: 88.0, disk_percent: 50.0 },
        ResourceMetrics { id: "r3".into(), name: "cache-node".into(),    cpu_percent: 30.0, memory_percent: 40.0, disk_percent: 96.0 },
        ResourceMetrics { id: "r4".into(), name: "load-balancer".into(), cpu_percent: 45.0, memory_percent: 50.0, disk_percent: 30.0 },
    ];

    println!("--- Evaluating resources ---");
    for r in &resources {
        engine.evaluate(r);
    }

    println!("\n--- Active Alerts ({}) ---", engine.active_count());
    engine.print_active();

    // Simulate recovery
    println!("\n--- Simulating recovery (cpu drops to 40%) ---");
    let recovered = ResourceMetrics {
        id: "r1".into(), name: "web-server-01".into(),
        cpu_percent: 40.0, memory_percent: 55.0, disk_percent: 40.0,
    };
    engine.evaluate(&recovered);

    println!("\n--- Active Alerts ({}) ---", engine.active_count());
    engine.print_active();
}
