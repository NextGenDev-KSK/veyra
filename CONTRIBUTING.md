# Contributing to Veyra Sounds

Thanks for helping build the audio enhancer Windows always deserved.

## License

By contributing you agree your work is licensed under the **GNU GPLv3**, like
the rest of the project. Do not add closed-source or GPL-incompatible
dependencies.

## Getting started

1. Read **[BUILD_GUIDE.md](BUILD_GUIDE.md)** and get a local build green.
2. Read **[ARCHITECTURE.md](ARCHITECTURE.md)** so your change lands in the right
   process (APO / service / UI / overlay).
3. Branch from `main`, keep PRs focused, and make sure CI is green.

## Performance budget (PRs that regress these are rejected)

| Metric | Budget |
|---|---|
| APO `APOProcess` per 10 ms buffer | < 0.5 ms CPU (Ryzen 5 5600) |
| Service idle CPU | < 0.1% |
| UI idle CPU (window hidden) | 0% (rendering suspended) |
| UI active CPU (visualizer running) | < 2% |
| Overlay CPU when active | < 0.5% |
| RAM combined | < 50 MB |
| Round-trip latency (Standard) | < 5 ms |
| Round-trip latency (Ultra-low, 32-sample) | < 2 ms |
| Cold start (UI to window visible) | < 300 ms |

### Real-time rules for the APO (absolute)

Inside `APOProcess` (runs on `audiodg.exe`'s real-time thread): **no** heap
allocation, **no** locks, **no** filesystem/network/COM calls, **no** logging.
Use pre-allocated buffers, atomics + sequence-locked shared memory for
parameters, and lock-free SPSC ring buffers for analyzer output.

## Code style

Modern C++20 (RAII, `std::expected`, ranges where they help). Comment intent,
not mechanics. Name things like a senior engineer. Optimise for clarity until
profiling demands otherwise — then optimise ruthlessly.

## Translations

UI strings go through JUCE's `TRANS()` macro; translations live in
`resources/lang/`. See the per-language files when the UI lands (Phase 4+).
