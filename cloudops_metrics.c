/**
 * cloudops_metrics.c
 *
 * Lightweight system metrics collector in C.
 * Reads CPU, memory, and disk stats from /proc and /sys on Linux.
 * Falls back to simulated values on non-Linux platforms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __linux__
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#endif

#define MAX_RESOURCES   16
#define NAME_LEN        64

/* ── Data Structures ─────────────────────────────────────────────────────── */

typedef enum {
    STATUS_HEALTHY  = 0,
    STATUS_DEGRADED = 1,
    STATUS_CRITICAL = 2,
    STATUS_UNKNOWN  = 3
} ResourceStatus;

typedef struct {
    char   name[NAME_LEN];
    double cpu_percent;
    double memory_percent;
    double disk_percent;
    long   net_in_bytes;
    long   net_out_bytes;
    ResourceStatus status;
    time_t collected_at;
} ResourceMetrics;

typedef struct {
    double cpu_warn;
    double cpu_crit;
    double mem_warn;
    double mem_crit;
    double disk_warn;
    double disk_crit;
} Thresholds;

/* ── Default Thresholds ──────────────────────────────────────────────────── */

static const Thresholds DEFAULT_THRESHOLDS = {
    .cpu_warn  = 75.0, .cpu_crit  = 90.0,
    .mem_warn  = 80.0, .mem_crit  = 95.0,
    .disk_warn = 85.0, .disk_crit = 95.0
};

/* ── Metric Collection ───────────────────────────────────────────────────── */

#ifdef __linux__
static double read_cpu_percent(void) {
    /* Read two samples from /proc/stat and compute usage between them */
    FILE *fp;
    unsigned long long u1, n1, s1, i1, u2, n2, s2, i2;
    unsigned long long total1, total2, idle1, idle2;
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000L }; /* 100ms */

    fp = fopen("/proc/stat", "r");
    if (!fp) return -1.0;
    fscanf(fp, "cpu %llu %llu %llu %llu", &u1, &n1, &s1, &i1);
    fclose(fp);

    nanosleep(&ts, NULL);

    fp = fopen("/proc/stat", "r");
    if (!fp) return -1.0;
    fscanf(fp, "cpu %llu %llu %llu %llu", &u2, &n2, &s2, &i2);
    fclose(fp);

    total1 = u1 + n1 + s1 + i1;
    total2 = u2 + n2 + s2 + i2;
    idle1  = i1;
    idle2  = i2;

    unsigned long long d_total = total2 - total1;
    unsigned long long d_idle  = idle2  - idle1;

    if (d_total == 0) return 0.0;
    return 100.0 * (1.0 - (double)d_idle / (double)d_total);
}

static double read_memory_percent(void) {
    struct sysinfo info;
    if (sysinfo(&info) != 0) return -1.0;
    unsigned long used = info.totalram - info.freeram - info.bufferram;
    return 100.0 * (double)used / (double)info.totalram;
}

static double read_disk_percent(const char *path) {
    struct statvfs st;
    if (statvfs(path, &st) != 0) return -1.0;
    unsigned long used = st.f_blocks - st.f_bfree;
    return 100.0 * (double)used / (double)st.f_blocks;
}
#else
/* Simulated fallback for non-Linux */
static double read_cpu_percent(void)              { return 20.0 + (rand() % 700) / 10.0; }
static double read_memory_percent(void)           { return 40.0 + (rand() % 500) / 10.0; }
static double read_disk_percent(const char *path) { (void)path; return 30.0 + (rand() % 600) / 10.0; }
#endif

/* ── Status Classification ───────────────────────────────────────────────── */

static ResourceStatus classify(const ResourceMetrics *m, const Thresholds *t) {
    if (m->cpu_percent    >= t->cpu_crit  ||
        m->memory_percent >= t->mem_crit  ||
        m->disk_percent   >= t->disk_crit) return STATUS_CRITICAL;

    if (m->cpu_percent    >= t->cpu_warn  ||
        m->memory_percent >= t->mem_warn  ||
        m->disk_percent   >= t->disk_warn) return STATUS_DEGRADED;

    return STATUS_HEALTHY;
}

static const char *status_str(ResourceStatus s) {
    switch (s) {
        case STATUS_HEALTHY:  return "HEALTHY";
        case STATUS_DEGRADED: return "DEGRADED";
        case STATUS_CRITICAL: return "CRITICAL";
        default:              return "UNKNOWN";
    }
}

/* ── Collection ──────────────────────────────────────────────────────────── */

static ResourceMetrics collect(const char *name, const Thresholds *thresholds) {
    ResourceMetrics m;
    memset(&m, 0, sizeof(m));
    strncpy(m.name, name, NAME_LEN - 1);

    m.cpu_percent    = read_cpu_percent();
    m.memory_percent = read_memory_percent();
    m.disk_percent   = read_disk_percent("/");
    m.net_in_bytes   = rand() % 100000000;
    m.net_out_bytes  = rand() % 50000000;
    m.collected_at   = time(NULL);
    m.status         = classify(&m, thresholds);

    return m;
}

static void print_metric(const ResourceMetrics *m) {
    printf("  %-20s | CPU: %5.1f%% | Mem: %5.1f%% | Disk: %5.1f%% | %s\n",
           m->name,
           m->cpu_percent,
           m->memory_percent,
           m->disk_percent,
           status_str(m->status));
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void) {
    srand((unsigned)time(NULL));

    const char *resources[] = {
        "web-server-01",
        "web-server-02",
        "db-primary",
        "cache-node",
        NULL
    };

    printf("=== CloudOps C Metrics Collector ===\n\n");
    printf("  %-20s | %-12s | %-12s | %-12s | Status\n",
           "Resource", "CPU", "Memory", "Disk");
    printf("  %s\n", "------------------------------------------------------------");

    for (int i = 0; resources[i] != NULL; i++) {
        ResourceMetrics m = collect(resources[i], &DEFAULT_THRESHOLDS);
        print_metric(&m);

        if (m.status == STATUS_CRITICAL) {
            fprintf(stderr, "  [ALERT] %s is CRITICAL!\n", m.name);
        }
    }

    return 0;
}
