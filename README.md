# HW1: File I/O Benchmark in C

Two small C programs that measure raw file I/O using unbuffered system calls
(`open`, `write`, `read`, `lseek`, `close`), with no stdio buffering.

- **Experiment 1 (`ex1.c`)**: write time vs block size. Writes a 256 KB buffer to
  18 files using block sizes from 1 B (file `01`) to 128 KB (file `18`), 256 times.
- **Experiment 2 (`ex2.c`)**: sequential vs random reads. Builds a 256 MB file
  (`19`), then reads it start to end, then with 1,024 random 256 KB reads.

## Requirements

- A C compiler (`cc`, `clang` or `gcc`). On macOS: `xcode-select --install`.
- About 1.5 GB of free disk space while the programs run.
- macOS for accurate Experiment 2 results (see [Notes](#notes)).

## Run

```sh
cc -O2 -Wall -o ex1 ex1.c && ./ex1 > ex1.csv    # about 2 minutes, writes 1.15 GB
cc -O2 -Wall -o ex2 ex2.c && ./ex2 > ex2.csv    # about 3 seconds, writes 256 MB
```

`ex1` prints a progress counter and per-block-size averages to the terminal.

When finished, delete the data files:

```sh
rm 0[1-9] 1[0-9]
```

## Output

| File | Contents |
|---|---|
| `ex1.csv` | One row per phase per file: `phase,file,block_bytes,ms` (4,608 rows) |
| `ex2.csv` | One row per test: `test,bytes,ms,MB_per_s` |
| `01`–`19` | Test data written by the programs. Not committed (see `.gitignore`); recreated on every run. |

## Results

Measured on an Apple M4, 24 GB RAM, Apple SSD 512 GB, macOS 26.6.

**Experiment 1**: time to write 256 KB (median of 256 phases)

| Block size | `write()` calls | Median (ms) |
|---|---|---|
| 1 B | 262,144 | 243.141 |
| 1 KB | 256 | 0.256 |
| 16 KB | 16 | 0.036 |
| 128 KB | 2 | 0.021 |

![Experiment 1 chart](ex1_chart.png)

Each `write()` call costs about 1 µs, so up to about 4 KB the time halves every time
the block size doubles. Past about 16 KB the curve flattens, because the remaining cost
is copying 256 KB into the page cache.

**Experiment 2**: time to read 256 MB

| Test | Time (ms) | MB/s |
|---|---|---|
| Sequential | 164.5 | 1,557 |
| Random (1,024 × 256 KB) | 162.5 | 1,575 |

![Experiment 2 chart](ex2_chart.png)

On an SSD with large 256 KB reads, sequential and random reads perform about the same.

## Notes

- **Your numbers will differ.** Timings depend on the CPU, disk and OS, and every run
  produces its own data.
- **Page cache (Experiment 2).** File `19` is still cached in RAM right after it is
  written. On macOS, `ex2` turns on `F_NOCACHE` so the reads go to the disk. Linux has no
  `F_NOCACHE`: there, `ex2` reads from RAM and both tests measure memory speed, not disk.
- **Experiment 1 measures system-call cost**, not disk speed. `write()` returns once the
  data is in the page cache, and `ex1` never calls `fsync`.
- **Quick test build:** `cc -O2 -DPHASES=2 -o ex1 ex1.c` runs only 2 phases, which takes
  under a second.
