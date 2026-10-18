#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define N 50000

typedef struct {
    int id;
    int start;
    int end;
    int *arr;
} Thread;

static double time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

static void odd_even_sort_seq(int *a, int n) {
    for (int cnt = 0; cnt < n; cnt++) {
        int start = cnt % 2;
        for (int i = start; i + 1 < n; i += 2) {
            if (a[i] > a[i + 1]) swap(&a[i], &a[i + 1]);
        }
    }
}

static void *sort(void *arg) {
    Thread *t = (Thread *)arg;
    int *a = t->arr;

    for (int cnt = 0; cnt < N; cnt++) {
        int start = cnt % 2;
        int i = t->start;
        if ((i % 2) != start) i++;
        while (i + 1 < t->end && i + 1 < N) {
            if (a[i] > a[i + 1]) swap(&a[i], &a[i + 1]);
            i += 2;
        }
    }
    return NULL;
}

static void merge_parts(int *a, int l, int m, int r, int *buf) {
    int i = l, j = m + 1, k = l;

    while (i <= m && j <= r) {
        if (a[i] <= a[j]) buf[k++] = a[i++];
        else              buf[k++] = a[j++];
    }
    while (i <= m) buf[k++] = a[i++];
    while (j <= r) buf[k++] = a[j++];
    memcpy(&a[l], &buf[l], (r - l + 1) * sizeof(int));
}

static void odd_even_sort_parallel(int *a, int n, int threads) {
    pthread_t *tids = malloc(threads * sizeof(pthread_t));
    Thread *args = malloc(threads * sizeof(Thread));
    if (!tids || !args) {
        fprintf(stderr, "malloc failed\n");
        return;
    }
    int part = n / threads;
    for (int i = 0; i < threads; i++) {
        args[i].id = i;
        args[i].start = i * part;
        args[i].arr = a;
        if (i == threads - 1) args[i].end = n;
        else args[i].end = (i + 1) * part; 
        int t = pthread_create(&tids[i], NULL, sort, &args[i]);
        if (t != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", t);
            return;
        }
    }

    for (int i = 0; i < threads; i++) {
        int t = pthread_join(tids[i], NULL);
        if (t != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", t);
            return;
        }
    }

    int *buf = malloc(n * sizeof(int));
    if (!buf) {
        fprintf(stderr, "malloc failed\n");
        return;
    }

    int width = part;
    while (width < n) {
        for (int l = 0; l < n; l += 2 * width) {
            int m = l + width - 1;
            int r = l + 2 * width - 1;
            if (m >= n) break;
            if (r >= n) r = n - 1;
            merge_parts(a, l, m, r, buf);
        }
        width *= 2;
    }
    free(buf);
    free(tids);
    free(args);
}

static void print_array(int *a, int n) {
    for (int i = 0; i < n; i++) printf("%d ", a[i]);
    printf("\n");
}

static void demo() {
    int n = 10, d[10], s[10], p[10];
    srand(time(NULL));
    for (int i = 0; i < n; i++) d[i] = rand() % 100;

    memcpy(s, d, sizeof(d));
    memcpy(p, d, sizeof(d));
    printf("Before: ");
    print_array(d, n);

    odd_even_sort_seq(s, n);
    printf("After s: ");
    print_array(s, n);

    odd_even_sort_parallel(p, n, 2);
    printf("After p: ");
    print_array(p, n);
}

int main() {
    static int *arr;
    static int *arr_copy;
    int threads[] = {2, 4, 8, 10, 12, 16, 128, 1024};
    int threads_num = sizeof(threads) / sizeof(threads[0]);
    demo();

    arr = malloc(N * sizeof(int));
    arr_copy = malloc(N * sizeof(int));
    if (!arr || !arr_copy) {
    fprintf(stderr, "malloc failed\n");
    return 1;
    }
    for (int i = 0; i < N; i++) arr[i] = rand() % 10000;
    memcpy(arr_copy, arr, N * sizeof(int));
    double t0 = time_ms();
    odd_even_sort_seq(arr_copy, N);
    double time_s = time_ms() - t0;
    printf("Threads | Time (ms) | Speedup | Efficiency | Sorted\n");
    printf("--------|-----------|---------|------------|-------\n");
    int ok = 1;
        for (int i = 0; i + 1 < N; i++) if (arr_copy[i] > arr_copy[i+1]) {
            ok = 0;
            break;
        }
    printf("%7d | %9.3f | %7.2f | %10.2f | %s\n", 1, time_s, 1.0, 1.0, ok ? "yes" : "NO");

    for (int i = 0; i < threads_num; i++) {
        int p = threads[i];
        memcpy(arr_copy, arr, N * sizeof(int));
        t0 = time_ms();
        odd_even_sort_parallel(arr_copy, N, p);
        double time_p = time_ms() - t0;

        double speedup = time_s / time_p;
        double efficiency = speedup / p;
        int ok = 1;
        for (int i = 0; i + 1 < N; i++) if (arr_copy[i] > arr_copy[i+1]) {
            ok = 0;
            break;
        }
        printf("%7d | %9.3f | %7.2f | %10.2f | %s\n", p, time_p, speedup, efficiency, ok ? "yes" : "NO");
    }
    free(arr);
    free(arr_copy);
    return 0;
}
