# Limit Order Book & Matching Engine

A deterministic, single-threaded C++17 matching engine implementing
price-time-priority execution. It supports multi-level matching, partial
fills, aggressive sweeps, duplicate-order validation, and iterator-backed
cancellation.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The reusable implementation is built as the `order_book` library. `matching_engine` runs the local benchmark in `src/main.cpp`, while `order_book_tests` validates matching behavior through CTest.

## Run

Run the correctness tests:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Run the benchmark:

```powershell
.\build\Release\matching_engine.exe
```

The benchmark reports insertion, matching, and cancellation throughput
together with p50, p99, and p99.9 latency over five repetitions.

## Architecture

- Bid price levels use a descending `std::map`; ask price levels use an
  ascending `std::map`, so the best price is always at the beginning of the
  relevant book.
- Each price level stores orders in a `std::list`, preserving FIFO time
  priority and allowing stable iterators.
- `orderMap` maps each active order ID to its side, price level, and list
  iterator. This avoids scanning the book during cancellation.
- Matching emits the maker order ID, taker order ID, execution price, and
  execution quantity. Resting orders execute at their price.
- Duplicate IDs and zero-quantity orders are rejected before matching.

### Complexity

- Cancellation is average-case `O(1)` to locate and remove an order through
  the hash map and stable list iterator. Removing an empty price level adds
  `O(log P)`.
- Adding an order is `O(log P + K)`, where `P` is the number of price levels
  and `K` is the number of resting orders consumed by the match.

## Matching example

Given these resting orders:

```text
SELL 100: quantity 5
SELL 101: quantity 4
```

An incoming `BUY 101: quantity 7` executes:

```text
5 units at 100
2 units at 101
```

The remaining quantity at `SELL 101` is 2. Orders at the same price are
always executed in FIFO order.

## Tests

The CTest suite covers:

- Price-time priority across multiple price levels
- Partial fills and remaining quantities
- Aggressive multi-level matching
- Cancellation using order IDs
- Duplicate and zero-quantity order rejection
- Non-crossing orders resting in the book

Run the tests with:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

## Benchmark

The benchmark in `src/main.cpp` runs five repetitions and reports operation
throughput plus p50, p99, and p99.9 latency. Setup work is performed before
timed sections:

- **Insertion:** 100,000 passive orders per repetition
- **Matching:** 1,000 independent sweeps per repetition; each sweep matches
  50 resting orders across one price level
- **Cancellation:** 10,000 active orders per repetition

One local Windows Release run produced:

| Operation | Operations | Throughput (ops/sec) | p50 (ns) | p99 (ns) | p99.9 (ns) |
|---|---:|---:|---:|---:|---:|
| Insertion | 500,000 | 2,829,684 | 200 | 600 | 25,700 |
| Matching | 5,000 | 124,127 | 7,800 | 21,600 | 51,000 |
| Cancellation | 50,000 | 5,774,072 | 100 | 200 | 700 |

Benchmark environment:

```text
Build: Release
Compiler: MSVC 19.50 (195035730)
Platform: Windows, 64-bit
Logical processors: 22
```

Results vary with CPU frequency, compiler version, operating-system
scheduling, and build configuration. The benchmark uses per-operation
steady-clock measurements, so the reported latency and throughput include
measurement overhead.
