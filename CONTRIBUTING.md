# Contributing to Veyra Sounds

Thanks for helping build the audio enhancer Windows always deserved.

---

## Licence

By contributing you agree your work is licensed under the **GNU GPLv3**, the same as
the rest of the project. Do not add closed-source or GPL-incompatible dependencies.
Third-party libraries go in `third_party/` with their original licence preserved.

---

## First steps

1. Read **[BUILD_GUIDE.md](BUILD_GUIDE.md)** and get a local build green:
   ```powershell
   # Developer PowerShell for VS 2022:
   cmake --preset windows-release
   cmake --build --preset windows-release
   ctest --test-dir build\windows-release --output-on-failure
   ```
2. Read **[ARCHITECTURE.md](ARCHITECTURE.md)** so your change lands in the right
   process (APO / service / UI / overlay).
3. Branch from `main`, keep PRs focused, and make sure CI is green before requesting review.

---

## Where to contribute

| Area | Directory | Notes |
|---|---|---|
| DSP effects | `modules/veyra-dsp/include/` | Header-only; must be allocation-free and RT-safe |
| APO COM | `apps/veyra-apo/` | Runs in `audiodg.exe`; see RT rules below |
| Service | `apps/veyra-service/` | Orchestration; may allocate freely |
| UI (JUCE) | `apps/veyra-ui/Source/` | JUCE 8.0.13; no OpenGL (DWM conflict) |
| Overlay | `apps/veyra-overlay/` | Win32 + GDI+ only |
| IPC protocol | `modules/veyra-common/include/veyra/ipc/` | Binary framed; keep backwards compatible |
| Config / Preset | `modules/veyra-common/include/veyra/` | Changes need JSON round-trip tests |
| Tests | `tests/` | Catch2; see test guide below |
| Translations | `resources/lang/` | Copy `en.json`, translate values, keep all keys |

---

## Real-time rules — APO (absolute)

`APOProcess` runs on `audiodg.exe`'s real-time thread. Violations cause audio glitches:

- **No heap allocation** — use pre-allocated buffers from `LockForProcess`
- **No locks** — use atomics, seqlock shared memory, or SPSC ring buffers
- **No filesystem / network / COM / logging calls**
- **No exceptions** — all RT paths must be `noexcept`
- **No denormals** — FTZ/DAZ is set at APO init; new filters must add the
  `1e-25f` anti-denormal guard to feedback state (see `Biquad.h`)

`LockForProcess` and `UnlockForProcess` are non-RT and may allocate freely.

---

## DSP contribution checklist

- [ ] Effect is header-only in `modules/veyra-dsp/include/`
- [ ] Implements `prepare(double sampleRate)` / `reset()` / `processStereo()` or `processMono()`
- [ ] Allocation-free after `prepare()` (fixed-size arrays / `std::array`)
- [ ] Added to `DspChain` in the correct position in the processing order
- [ ] Added to `DspParameters` and `VeyraParamsPayload`; wired in `refreshParametersFromShared()`
- [ ] Unit test in `tests/dsp_tests/`: silence passthrough, audible effect, no NaN/inf under abuse
- [ ] Performance test: asserts > 5× real-time on a 512-sample block at 48 kHz

---

## Performance budget

PRs that regress any of these are blocked:

| Metric | Budget |
|---|---|
| APO `APOProcess` per 10 ms block | < 0.5 ms CPU (Ryzen 5 5600) |
| Service idle CPU | < 0.1% |
| UI idle CPU (window hidden) | 0% |
| UI active CPU (visualiser running) | < 2% |
| Overlay CPU when active | < 0.5% |
| Combined RAM | < 50 MB |
| APO end-to-end latency | < 5 ms |
| UI cold start (launch → window visible) | < 300 ms |

---

## Code style

- Modern **C++20** (RAII, structured bindings, `std::expected` where it helps)
- Name things like a senior engineer: `computeRmsEnergy`, not `proc`
- Comment **intent and non-obvious constraints only** — not mechanics
- `UNICODE`, `NOMINMAX`, `WIN32_LEAN_AND_MEAN` are set project-wide; do not re-define
- Prefer `std::array` over raw arrays; `std::unique_ptr` over raw `new` outside APOs
- 4-space indent, no tabs

---

## Writing tests

```powershell
cmake --preset windows-release -DVEYRA_BUILD_TESTS=ON
cmake --build --preset windows-release
ctest --test-dir build\windows-release --output-on-failure
```

For a new DSP unit, add a file to `tests/dsp_tests/` following `test_chain.cpp`:
- Test silence passthrough (zeros in → zeros out after settle)
- Test the effect is audible (RMS with ON > RMS with OFF for a relevant signal)
- Test no NaN/inf after 1 s of full-scale noise
- If a limiter is involved, test the ceiling guarantee

---

## Submitting a PR

- Target `main`
- Title: `fix:` / `feat:` / `docs:` / `test:` prefix
- Describe **what** changed and **why** (not just what the diff shows)
- If touching the APO, confirm you have read the RT rules and the performance test still passes
- CI must be green (build + all tests)
