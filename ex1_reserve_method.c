// HW1 Experiment 1, reserve method: same as ex1.c, but each file's final size
// (64 MB) is reserved on disk and set before timing, so every write() overwrites
// existing space instead of growing the file.
// Files 01..18, file n uses block size 2^(n-1) bytes. Each phase writes the
// 256 KB buffer once to every file. 256 phases -> each file ends at 64 MB.
// Usage: ./ex1_reserve_method > ex1_reserve_method.csv
//        (per-phase CSV on stdout, per-file averages on stderr)
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
    off_t want = (off_t)PHASES * SIZE;  // final file size
    double t_reserve = now_ms();
    for (int n = 1; n <= NFILES; n++) {
        char name[8];
        snprintf(name, sizeof name, "%02d", n);
        fd[n] = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd[n] < 0) { perror(name); return 1; }
#ifdef F_PREALLOCATE
        // macOS: reserve disk blocks. ftruncate alone makes a sparse file, which is slower.
        fstore_t fs = {F_ALLOCATEALL, F_PEOFPOSMODE, 0, want, 0};
        if (fcntl(fd[n], F_PREALLOCATE, &fs) < 0) { perror("F_PREALLOCATE"); return 1; }
#else
        int err = posix_fallocate(fd[n], 0, want);  // Linux
        if (err) { fprintf(stderr, "posix_fallocate: error %d\n", err); return 1; }
#endif
        if (ftruncate(fd[n], want) < 0) { perror("ftruncate"); return 1; }
    }
    t_reserve = now_ms() - t_reserve;

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

    fprintf(stderr, "\n\nreserve_ms (all files, not in CSV): %.3f\n", t_reserve);
    fprintf(stderr, "\nfile,block_bytes,avg_ms,total_ms\n");
    for (int n = 1; n <= NFILES; n++) {
        close(fd[n]);
        fprintf(stderr, "%02d,%zu,%.3f,%.1f\n", n, (size_t)1 << (n - 1), total[n] / PHASES, total[n]);
    }
    free(buf);
    return 0;
}
