// HW1 Experiment 1: write() time vs block size.
// Files 01..18, file n uses block size 2^(n-1) bytes. Each phase writes the
// 256 KB buffer once to every file. 256 phases -> each file ends at 64 MB.
// Usage: ./ex1 > ex1.csv   (per-phase CSV on stdout, per-file averages on stderr)
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SIZE (256 * 1024)
#define NFILES 18
#ifndef PHASES
#define PHASES 256
#endif

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

int main(void) {
    unsigned char *buf = malloc(SIZE);
    if (buf == NULL) { perror("malloc failed"); return 1; }
    for (size_t i = 0; i < SIZE; i++) buf[i] = (i % 255) + 1;  // 1..255 cyclic

    int fd[NFILES + 1];
    for (int n = 1; n <= NFILES; n++) {
        char name[8];
        snprintf(name, sizeof name, "%02d", n);
        fd[n] = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd[n] < 0) { perror(name); return 1; }
    }

    double total[NFILES + 1] = {0};
    printf("phase,file,block_bytes,ms\n");
    for (int p = 1; p <= PHASES; p++) {
        for (int n = 1; n <= NFILES; n++) {
            size_t block = (size_t)1 << (n - 1);
            double t0 = now_ms();
            for (size_t off = 0; off < SIZE;) {
                size_t len = SIZE - off < block ? SIZE - off : block;
                ssize_t w = write(fd[n], buf + off, len);
                if (w < 0) { perror("write"); return 1; }
                off += w;  // handles short writes
            }
            double ms = now_ms() - t0;
            total[n] += ms;
            printf("%d,%02d,%zu,%.3f\n", p, n, block, ms);
        }
        fprintf(stderr, "\rphase %d/%d", p, PHASES);
    }

    fprintf(stderr, "\n\nfile,block_bytes,avg_ms,total_ms\n");
    for (int n = 1; n <= NFILES; n++) {
        close(fd[n]);
        fprintf(stderr, "%02d,%zu,%.3f,%.1f\n", n, (size_t)1 << (n - 1), total[n] / PHASES, total[n]);
    }
    free(buf);
    return 0;
}
