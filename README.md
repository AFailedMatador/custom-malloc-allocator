# Custom Malloc Allocator

A heap memory allocator implemented from scratch in C (`smalloc.c`), providing `malloc`/`free`-style memory management without relying on the system allocator.

## Design

- **Heap management via `mmap`** — the allocator requests and extends its own heap region directly from the OS instead of using `sbrk`/libc malloc
- **Block headers** — each allocation is tracked with a header storing block size and free/allocated status
- **Free list traversal & coalescing** — adjacent free blocks are merged to reduce fragmentation
- **Block splitting** — oversized free blocks are split so the remainder stays available for future allocations

## Build & run

```bash
make
./test -t traces/short1-bal.txt -o outputs/short1-bal.out
```

## Testing

`traces/` contains 15 allocation/free traces (including edge cases like double-free and coalescing scenarios) with corresponding known-good results in `expected_outputs/`. `grading.py` replays each trace, checks the output against the expected result, and scores correctness across the full suite.

```bash
python3 grading.py
```
