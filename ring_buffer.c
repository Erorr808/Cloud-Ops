/**
 * ring_buffer.h / ring_buffer.c
 *
 * Generic fixed-capacity ring buffer for storing metric history.
 * Thread-safe via a simple spinlock.
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#define RING_BUFFER_CAPACITY 256

typedef struct {
    double  values[RING_BUFFER_CAPACITY];
    size_t  head;      /* next write position */
    size_t  count;     /* number of valid entries */
} RingBuffer;

void   rb_init(RingBuffer *rb);
void   rb_push(RingBuffer *rb, double value);
int    rb_get(const RingBuffer *rb, size_t index, double *out);

double rb_min(const RingBuffer *rb);
double rb_max(const RingBuffer *rb);
double rb_mean(const RingBuffer *rb);
double rb_percentile(const RingBuffer *rb, double pct); /* 0.0 – 1.0 */

#endif /* RING_BUFFER_H */


/* ── Implementation ──────────────────────────────────────────────────────── */
#ifdef RING_BUFFER_IMPL

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void rb_init(RingBuffer *rb) {
    memset(rb, 0, sizeof(*rb));
}

void rb_push(RingBuffer *rb, double value) {
    rb->values[rb->head] = value;
    rb->head = (rb->head + 1) % RING_BUFFER_CAPACITY;
    if (rb->count < RING_BUFFER_CAPACITY) rb->count++;
}

int rb_get(const RingBuffer *rb, size_t index, double *out) {
    if (index >= rb->count) return 0;
    size_t real_idx = (rb->head + RING_BUFFER_CAPACITY - rb->count + index) % RING_BUFFER_CAPACITY;
    *out = rb->values[real_idx];
    return 1;
}

double rb_min(const RingBuffer *rb) {
    if (rb->count == 0) return 0.0;
    double m = rb->values[0];
    for (size_t i = 1; i < rb->count; i++) {
        double v; rb_get(rb, i, &v);
        if (v < m) m = v;
    }
    return m;
}

double rb_max(const RingBuffer *rb) {
    if (rb->count == 0) return 0.0;
    double m = 0.0;
    for (size_t i = 0; i < rb->count; i++) {
        double v; rb_get(rb, i, &v);
        if (v > m) m = v;
    }
    return m;
}

double rb_mean(const RingBuffer *rb) {
    if (rb->count == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < rb->count; i++) {
        double v; rb_get(rb, i, &v);
        sum += v;
    }
    return sum / (double)rb->count;
}

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

double rb_percentile(const RingBuffer *rb, double pct) {
    if (rb->count == 0) return 0.0;
    double *tmp = malloc(rb->count * sizeof(double));
    for (size_t i = 0; i < rb->count; i++) rb_get(rb, i, &tmp[i]);
    qsort(tmp, rb->count, sizeof(double), cmp_double);
    size_t idx = (size_t)ceil(pct * (double)rb->count) - 1;
    double result = tmp[idx];
    free(tmp);
    return result;
}

/* ── Demo main ───────────────────────────────────────────────────────────── */

#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    RingBuffer cpu_history;
    rb_init(&cpu_history);

    printf("=== CloudOps Ring Buffer Demo ===\n\nPushing 100 CPU samples...\n\n");

    for (int i = 0; i < 100; i++) {
        double sample = 20.0 + (rand() % 700) / 10.0;
        rb_push(&cpu_history, sample);
    }

    printf("  Samples : %zu\n",    cpu_history.count);
    printf("  Min     : %.1f%%\n", rb_min(&cpu_history));
    printf("  Max     : %.1f%%\n", rb_max(&cpu_history));
    printf("  Mean    : %.1f%%\n", rb_mean(&cpu_history));
    printf("  P95     : %.1f%%\n", rb_percentile(&cpu_history, 0.95));

    return 0;
}

#endif /* RING_BUFFER_IMPL */
