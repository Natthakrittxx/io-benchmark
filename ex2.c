// HW1 Experiment 2: sequential vs random read() on a 256 MB file.
// File 19 = the 256 KB buffer written 1024 times. Test 1 reads it start to end,
// test 2 does 1024 reads of 256 KB at random offsets (lseek + read).
// Usage: ./ex2 > ex2.csv
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SIZE (256 * 1024)
#define COUNT 1024
#define FILE_SIZE ((off_t)SIZE * COUNT)
#define PAGE 4096

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

// File 19 was just written, so without this every read would come from the
// OS page cache (RAM) and both tests would measure memcpy, not the disk.
static int open19(int flags) {
    int fd = open("19", flags, 0644);
    if (fd < 0) { perror("19"); exit(1); }
#ifdef F_NOCACHE
    fcntl(fd, F_NOCACHE, 1);  // macOS. Linux: run `echo 3 > /proc/sys/vm/drop_caches` first
#endif
    return fd;
}

static void read_full(int fd, unsigned char *dst) {
    for (size_t got = 0; got < SIZE;) {
        ssize_t r = read(fd, dst + got, SIZE - got);
        if (r <= 0) { perror("read"); exit(1); }
        got += r;
    }
}

int main(void) {
    unsigned char *buf = malloc(SIZE), *rbuf = malloc(SIZE);
    if (buf == NULL || rbuf == NULL) { perror("malloc failed"); return 1; }
    for (size_t i = 0; i < SIZE; i++) buf[i] = (i % 255) + 1;  // 1..255 cyclic

    // Create file 19: write the buffer 1024 times -> 256 MB.
    int fd = open19(O_WRONLY | O_CREAT | O_TRUNC);
    for (int i = 0; i < COUNT; i++) {
        for (size_t off = 0; off < SIZE;) {
            ssize_t w = write(fd, buf + off, SIZE - off);
            if (w < 0) { perror("write"); return 1; }
            off += w;
        }
    }
    fsync(fd);  // data on disk before timing reads
    close(fd);
    // SSD keeps flushing for ~1-2 s after a 256 MB write; without this pause
    // test 1 (sequential) measured ~35% slower than its real speed.
    sleep(2);

    // Test 1: sequential read, start to end.
    fd = open19(O_RDONLY);
    off_t total = 0;
    ssize_t r;
    double t0 = now_ms();
    while ((r = read(fd, rbuf, SIZE)) > 0) total += r;
    double seq_ms = now_ms() - t0;
    if (r < 0) { perror("read"); return 1; }
    close(fd);
    if (total != FILE_SIZE) { fprintf(stderr, "short file: %lld\n", (long long)total); return 1; }

    // Test 2: 1024 reads of 256 KB at random page-aligned offsets.
    // ponytail: random() % n has tiny modulo bias, irrelevant for 65 K slots
    fd = open19(O_RDONLY);
    srandom(time(NULL));
    t0 = now_ms();
    for (int i = 0; i < COUNT; i++) {
        off_t off = (off_t)(random() % ((FILE_SIZE - SIZE) / PAGE + 1)) * PAGE;
        if (lseek(fd, off, SEEK_SET) < 0) { perror("lseek"); return 1; }
        read_full(fd, rbuf);
    }
    double rnd_ms = now_ms() - t0;
    close(fd);

    printf("test,bytes,ms,MB_per_s\n");
    printf("sequential,%lld,%.3f,%.1f\n", (long long)FILE_SIZE, seq_ms, FILE_SIZE / 1048576.0 / (seq_ms / 1e3));
    printf("random,%lld,%.3f,%.1f\n", (long long)FILE_SIZE, rnd_ms, FILE_SIZE / 1048576.0 / (rnd_ms / 1e3));
    free(buf);
    free(rbuf);
    return 0;
}
