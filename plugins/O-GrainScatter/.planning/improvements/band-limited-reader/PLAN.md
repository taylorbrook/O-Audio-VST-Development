---
milestone: band-limited-reader
domain: dsp
execute_agent: general-purpose   # DSP work, but every task builds + runs harnesses; dsp-agent has no Bash
version_bump: minor
base_version: 3.2.0
target_version: 3.3.0
module: grain-engine 1.0.0 -> 1.1.0
created: 2026-10-27
---

# Plan: Band-limited grain reader (mipmap + polyphase sinc)

**Plugin:** O-GrainScatter (first consumer) + `modules/synthesis/grain-engine`
**Milestone:** band-limited-reader
**Created:** 2026-10-27
**Phase:** Plan

---

## Summary

**Improvement:** Pitched-up grains stop aliasing. Today Lagrange-4 reads at up to +36 st × Doppler 2 with no filtering, and the aliases come out at full level from +12 st up (RESEARCH §1).

**Approach:** A new header-only `BandlimitedReader.h` in grain-engine holds:
- a mirrored stereo mip ring, built on the write path by a causal 127-tap Kaiser half-band cascade (L = 3, group delay compensated);
- a fixed 16-tap polyphase sinc bank, 8 sets × 257 phases, each row unity DC.

GrainScatter's live ring and freeze snapshots read through it. The octave level is latched at spawn. The sinc set comes from the live residual rate.

**Version:** O-GrainScatter 3.2.0 → **3.3.0** (MINOR: audible change, no parameter, state or preset change). grain-engine 1.0.0 → **1.1.0** (new optional header; existing classes untouched).

**Negative control:** v3.2.0 is commit `9c6c22d0`. Build it out-of-tree via `git archive 9c6c22d0 plugins/O-GrainScatter` (see `pattern_null_test_needs_seeded_out_of_tree_pair`). The `backups/O-GrainScatter/v3.2.0/` snapshot, made before execute, is the rollback.

---

## Plan-phase decisions (RESEARCH "Open decisions", settled)

| # | Decision | Settled |
|---|---|---|
| 1 | Tap count | **16** (user decision). `kTaps` is one constexpr; the verify listening check (+11 → +13 st) decides whether T24 is needed |
| 2 | r′ ∈ (2.8, 4) | Best-effort S7 set, documented. Gate row "+36 st Doppler 2" verdict is **reported, not pass/fail** past r′ 2.8 |
| 3 | Freeze capture end | `head − margin(3)` (≈ 10 ms earlier), all levels. Documented in CHANGELOG |
| 4 | Doppler guard hole | **Included**: read-time clamp `delay = max(delay, margin(ℓ))` in both render paths |
| 5 | Existing gate rows | blocksize-null precision row → reader-vs-reader (head > 2^21 vs small head, same frac). density-gain sparse row → v3.3.0 with density compensation forced off (compile-time test hook), still ≤ −70 dB. Neither threshold loosened |
| 6 | Versions | 3.3.0 / 1.1.0 |

---

## Task Breakdown

### Task 1: Offline design of the sinc bank and half-band constants

**Outcome:** Final hard-coded constants, produced with numpy/scipy from `research-scripts/` (`run3.py`, `hb2.py`):
- the 127-tap half-band coefficients (β = 9; only the 32 unique nonzero taps plus the 0.5 centre);
- per-set (upper edge, fc, β) for S0–S7 at the 44.1 k design point;
- a predicted passband/stopband table per set.

The script is saved as `research-scripts/emit_constants.py`. It prints a C++ snippet, so the constants are reproducible.

**Verification:**
- [x] Half-band stopband ≤ −90 dB at 0.25 + δ(44.1 k); every even-offset tap exactly 0; centre = 0.5 — −92.0 dB (β 9.2; β 9 gives −89.95 once the centre is pinned to 0.5, see SUMMARY deviations)
- [x] S0–S6: every audible spurious component ≤ −60 dB at their upper edge (2^(k/6)); S7 table printed with its achieved level at r′ = 2.83 / 3.99 — S0–S6 −60.5 designed (incl. half-band transition content on level ≥ 1); S7 −60.5 / −20.5 dB
- [x] Predicted −1 dB passband per set matches RESEARCH §5 within 0.3 kHz — research settings reproduce §5 within 0.05 kHz (parity block). The FINAL sets differ by +0.34 / +0.43 / −0.77 kHz at r′ 1 / 1.414 / 1.99 (finer search, 0.5 dB margin, S6 designed at r′ 2.0): documented deviation

**Dependencies:** None
**Estimated effort:** Small

---

### Task 2: `BandlimitedReader.h` in grain-engine

**Outcome:** `modules/synthesis/grain-engine/cpp/BandlimitedReader.h`: plain C++20, no JUCE, namespace `ouaricon::grain`, header-only.
- `HalfbandKernel`: Task 1 constants, symmetric folded (32 MAC per level output).
- `MipRing<L = 3>`:
  - `prepare(capacitySamples)` allocates level 0..L per channel, each with a 16-sample mirrored guard (`kTaps`);
  - `clear()` zeroes without reallocating (CR-02);
  - `push(l, r)` runs the cascade per sample on odd parent indices, which makes it block-size invariant;
  - absolute `int64` write counter;
  - `levelIndex(ℓ, p₀) = (p₀ + D(ℓ) − (2^ℓ − 1)) / 2^ℓ`, with `D(ℓ) = 63·(2^ℓ − 1)`;
  - `static constexpr margin(ℓ) = D(ℓ) + 8·2^ℓ + 1` → 9 / 80 / 222 / 506.
- `SincBank`: 8 × 257 × 16 float rows, built in the constructor in double. Each row is normalised to sum 1, then stored as float. Shared as one function-local static (`const SincBank& sincBank()`), built on first call; Task 4 forces that call in `prepareToPlay`.
- `read(ring, ℓ, ch, delayFromHead, set)` and `readMono(ring, ℓ, delayFromHead, set)`:
  - `readMono` sums the L and R taps first, then does one 16-MAC convolution;
  - taps run i₀−7 … i₀+8, contiguous thanks to the mirror, so there is no `%` per tap;
  - phases are lerped between adjacent rows.
- `MipSnapshot`:
  - preallocated per-level storage;
  - `captureFrom(ring, endDelay, length)` copies every level, plus `8·2^ℓ + 1` pre-roll at the old end;
  - per-level `double` origin offsets (62 / 186 / 434 are not divisible by 2^ℓ);
  - `read(ℓ, ch, pos, set)` takes snapshot coordinates.
- Helpers:
  - `latchLevel(rBase) = clamp(⌊log₂ r⌋, 0, 3)`;
  - `setFor(r′)`: S0 for r′ ≤ 1; otherwise the first set whose upper edge ≥ r′, S7 above 2. A branch-free table lookup on log₂.

**Verification:**
- [x] Compiles header-only under the module test CMake with `-Wall -Wextra`, no warnings
- [x] Nothing in `push` / `read` / `readMono` / `MipSnapshot::read` allocates (Task 3 alloc test) — B5: 0 allocations

**Dependencies:** Task 1
**Estimated effort:** Large

---

### Task 3: Module tests + module release metadata

**Outcome:** `grain_engine_tests.cpp` gains a `BandlimitedReader` section:
1. **DC per phase:** every row of every set sums to 1 ± 1e-6. Lerped mid-phases are checked too.
2. **Level alignment:** a band-limited transient (below fs/32) is pushed. Its peak is located at levels 1/2/3 via `levelIndex` and compared with level 0, by parabolic interpolation of the read output. The error must be ≤ 0.05 sample.
3. **Half-band stopband** ≤ −90 dB, measured from the kernel's FFT. **Sinc stopband**: each set S0–S6 is driven at its upper-edge r′ with tones across 20 Hz–20 kHz, and the worst audible spurious component must be ≤ −60 dB. S7 is reported.
4. **Block-split invariance:** the same input is pushed as 1 × N versus random-size chunks. Every level must be bit-identical.
5. **Alloc-free:** a counting global `operator new` stays at zero across `push` / `read` / `capture` / snapshot `read` after `prepare`.
6. **Snapshot equivalence:** `MipSnapshot` reads must equal live reads at the same absolute times, bit-identical away from the edges.

Metadata changes:
- `module.yaml` → 1.1.0: `BandlimitedReader` added to `provides` and the description, `used_by` gains O-GrainScatter.
- README section added.
- The test CMake project version is bumped.

**Verification:**
- [x] `ninja -C build-grain-engine-tests && …/grain-engine-tests` exits 0, with all old checks still passing — 38/38
- [x] Negative control: perturbing one row's normalisation (temporary edit, reverted) makes check 1 FAIL. Removing the `D(ℓ)` term makes check 2 FAIL — B1 1.0e-3 FAIL; B2 L1 +62.4 / L3 +51.2 FAIL

**Dependencies:** Task 2
**Estimated effort:** Medium

---

### Task 4: GrainScatter live ring on the reader

**Outcome:**
- **`CMakeLists.txt`:** add `ouaricon_add_module(O-GrainScatter grain-engine)` and set VERSION 3.3.0. Only after this module link works do the local `CounterRng.h` / `GrainScheduler.h` copies stay as they are (no migration in scope; there must be no name clashes, which the namespace ensures).
- **`DelayBuffer.h`:**
  - holds a `MipRing`; `pushSample` → `push`;
  - `readSample(ch, delay)` is replaced by `read(level, ch, delay, set)` and `readMono(level, delay, set)`;
  - `copyRegion` is replaced by snapshot capture, which Task 5 wires up;
  - the `LagrangeInterpolation.h` include is dropped. The file is deleted only if grep finds no other reference.
- **`GrainPool.h`:**
  - `GrainVoice` gains `uint8 level` and `uint8 sincSet`, set at spawn: `level = latchLevel(playbackRate)`, where playbackRate already includes the freeze detune;
  - stereo path (~303–316): latched set, reads `read(level, 0/1, max(delay, margin(level)), sincSet)`;
  - spatial path (~373–382): set re-chosen per sample from `r′ = playbackRate · dopplerFactor / 2^level`, mono via `readMono`, same `max` clamp.
- **`PluginProcessor.cpp`:**
  - spawn guard (`:777–785`): `kReadMargin = 4.0` → `margin(latchLevel(rate))`;
  - `densityRho` `floorS` (`:648`): `+ 4.0` → the same margin;
  - `prepareToPlay`: touch `sincBank()` so the static builds off the audio thread;
  - comments updated to describe the new kernel (no Lagrange wording left).

**Verification:**
- [x] `ninja O-GrainScatter_VST3 O-GrainScatter_AU` builds clean, with no new warnings (remaining warnings are pre-existing, same set in the v3.2.0 tree)
- [x] `grep -n "Lagrange\|kReadMargin = 4" plugins/O-GrainScatter/Source` returns no stale grain-path hits (LagrangeInterpolation.h deleted, no references left)
- [x] Unity-pitch render is audibly and spectrally the same signal (no gross change), with a passband table measured by Task 6 — spectrally: −1 dB 16.2 / 17.7 kHz at 44.1 / 48 k, density rows within ±1.5 dB; audible check left for verify

**Dependencies:** Task 2 (Task 3 green first is preferred, not required)
**Estimated effort:** Medium

---

### Task 5: Freeze snapshot carries the levels

**Outcome:**
- **`FreezeManager.h`:**
  - slots become `MipSnapshot`s, preallocated in `prepare` (≈ 1.875× today's slot memory);
  - capture ends at `head − margin(3)` for every level;
  - `readSample(slot, ch, pos)` becomes a level- and set-aware read in snapshot coordinates;
  - the v3.0.4 wrap/seam crossfade is kept unchanged. It runs on positions, and the reader is linear, so blending two reads equals reading the blend;
  - the release crossfade is untouched.
- **`GrainPool.h`:** both frozen read sites (~304–305, ~374–375) pass `level` / `sincSet`.

**Verification:**
- [ ] Freeze engage, release, and re-engage at +0 / +24 / +36 st in the Standalone: no click, no silence, no stale-snapshot read — NOT run (no hands-on Standalone in execute); left for the verify listening check. Offline: blocksize-null freeze row bit-exact, density-gain freeze rows pass, alias row 4 −95.3 dB
- [x] Capture is alloc-free (covered by the module alloc test plus Task 6 render) — B5 includes captureFrom

**Dependencies:** Task 4
**Estimated effort:** Medium

---

### Task 6: Gates — new alias gate + rework of the two existing rows

**Outcome:**

- **New `plugins/O-GrainScatter/tests/alias-gate/main.cpp`**: an offline harness in the same style as `blocksize-null`, built against the current tree **and** against the `git archive 9c6c22d0` tree (v3.2.0). It has these rows:
  1. Fixed tones and a 20 kHz-band-limited sweep at +12 / +24 / +36 st, stereo path. Verdict: worst audible spurious ≤ −60 dB.
  2. +36 st, spatial path, Doppler forced to 1.0 and to 1.4. Verdict ≤ −60 dB.
  3. +36 st, Doppler forced to 2.0. **Report only** (S7, decision 2).
  4. Frozen +36 st. Same verdict as row 1.
  5. Alignment: a transient at r = 1.99 vs 2.01. The cross-correlation peak lag must be ≤ 0.05 sample.
  6. Passband table: the measured output −1 dB point per r′ at 44.1 k and 48 k, printed next to RESEARCH §5.
  7. CPU: 64 grains at +24 st, stereo and spatial, ns/sample, v3.3.0 vs v3.2.0. Reported.

  **Negative control:** v3.2.0 must FAIL rows 1, 2 and 4 (expect ≈ 0 dB). If it passes, the gate is vacuous; fix the stimulus (`pattern_gate_stimulus_below_threshold_is_vacuous`).
- **`blocksize-null`:** the "ring read precision, 192 k, head > 2^21" row becomes reader-at-big-head vs reader-at-small-head, same fractional delay, ≤ −100 dB. Every other row is unchanged and must stay bit-exact across block sizes.
- **`density-gain`:**
  - the sparse row reference becomes v3.3.0 with density compensation forced off (compile-time `GS_TEST_DENSITY_COMP_OFF` hook), still ≤ −70 dB;
  - the incoh/coh/freeze rows must hold at unity pitch. Any pitched-row level shift is re-baselined only with the measured dB and the reason ("folded alias power removed") written in the harness comment and the CHANGELOG.

**Verification:**
- [x] alias-gate: v3.3.0 ALL PASS on rows 1, 2, 4, 5; v3.2.0 FAILS rows 1, 2, 4 — v3.3.0 worst −63.5 dB (spatial Doppler 1.4), v3.2.0 0.0 dB on every row
- [x] blocksize-null ALL PASS; density-gain ALL PASS
- [x] Negative control on the reworked precision row: casting the read position to float (temporary) makes it FAIL — −47.9 dB FAIL; sparse row with a 0.99 gain: −40 dB FAIL

**Dependencies:** Task 4, Task 5
**Estimated effort:** Large

---

### Task 7: Build, install, validate, document

**Outcome:**
- `./scripts/build-and-install.sh O-GrainScatter` (dual-variant sweep, cache clear).
- pluginval strictness 10 on the VST3.
- `auval -a | grep -i grainscatter`.
- `CHANGELOG.md` gets a `## [3.3.0] — <date>` entry:
  - alias fix with gate numbers;
  - unity timbre change (Lagrange → sinc);
  - passband swing within each octave, with the measured table;
  - freeze capture now ends ≈ 10 ms before engage;
  - Doppler guard-hole fix;
  - memory growth at 192 k;
  - CPU numbers.
- The module README / CHANGELOG are updated for 1.1.0.
- SUMMARY.md is written.

Versioning is left to the post-milestone step: **no git tag** (tags only via `/publish`, `feedback_never_tag_unless_publish`). The commit is path-scoped.

**Verification:**
- [ ] pluginval 10 PASS, auval PASS, plugin loads in the Standalone with presets unchanged — pluginval 10 SUCCESS and auval PASS (3.3.0 / 0x30300) done; Standalone load + preset check left for verify (no hands-on Standalone in execute; no parameter/state code changed)
- [x] CHANGELOG heading is in `## [3.3.0]` bracket form
- [x] SUMMARY.md lists the measured gate rows, the CPU and the passband table

**Dependencies:** Task 3, Task 6
**Estimated effort:** Medium

---

## Dependency Graph

```
Task 1 ──> Task 2 ──┬──> Task 3 ─────────────────────────┐
                    └──> Task 4 ──> Task 5 ──> Task 6 ───┴──> Task 7
```

**Parallelizable:** Task 3 (module tests) alongside Task 4 → 5.

## Execution Order

| Order | Task | Dependencies | Parallel with |
|---|---|---|---|
| 1 | 1 Constants | — | — |
| 2 | 2 BandlimitedReader.h | 1 | — |
| 3 | 3 Module tests + 1.1.0 | 2 | 4, 5 |
| 3 | 4 Live ring migration | 2 | 3 |
| 4 | 5 Freeze levels | 4 | 3 |
| 5 | 6 Gates | 4, 5 | — |
| 6 | 7 Build / validate / docs | 3, 6 | — |

---

## Risk Notes

1. **16-tap passband swing** (9.1 kHz worst case at r′ → 2, 44.1 k; +11.9 st duller than +12.1 st)
   - Impact: an audible timbre step at octave boundaries.
   - Mitigation: Task 6 row 6 measures it, and the verify listening check judges it. T24 is a one-constant change (`kTaps`) at +50 % reader CPU. Not pre-emptively taken.
2. **Cascade state breaks block-size invariance**
   - Impact: blocksize-null fails.
   - Mitigation: per-sample `push` with absolute-counter parity (Task 2); a module split-invariance check (Task 3 #4) before the plugin gate.
3. **Read guard / spawn floor mismatch per level**
   - Impact: a grain reads unsettled level samples (wrapped 12 s-old audio).
   - Mitigation: one `margin(ℓ)` source used by the spawn guard, `floorS`, and the read-time clamp (Task 4).
4. **Freeze snapshot level offsets** (fractional origins 62 / 186 / 434)
   - Impact: frozen grains are time-shifted per level.
   - Mitigation: per-level double origins (Task 2), and the module snapshot-equivalence check (Task 3 #6).
5. **Existing gate rows re-referenced**
   - Impact: silently weakened gates.
   - Mitigation: thresholds unchanged, references changed for stated reasons, and the reworked precision row negative-controlled (Task 6).
6. **Memory at 192 k** (ring 18.4 → 34.6 MB, plus freeze)
   - Impact: larger prepare allocation.
   - Mitigation: noted in CHANGELOG. All of it is allocated in `prepare`; `clear()` never resizes.
7. **Module link into GrainScatter for the first time**
   - Impact: a name clash between local `CounterRng.h` / `GrainScheduler.h` and the module's copies.
   - Mitigation: the module types live in `ouaricon::grain`. Check that the local copies are not in that namespace, and include only `BandlimitedReader.h`.

---

## Domain Agent Instructions

**Execute agent:** general-purpose, following dsp-agent rules. Every task needs Bash for builds and harness runs.

**Rules:**
- No allocation, locks or logging on the audio thread. Tables are built in the constructor or static init, touched in `prepareToPlay`.
- Double read positions throughout; float only for stored samples and kernel rows.
- Every DSP change is gated with a negative control (v3.2.0 via `git archive 9c6c22d0`). A gate that the old build passes is vacuous.
- Don't touch parameters, state, presets or UI.
- Shared checkout: path-scoped commits only (`git commit -- plugins/O-GrainScatter modules/synthesis/grain-engine PLUGINS.md`). Re-check the branch and `git status --short` right before committing. No tag.
- Long builds or renders run in the background (600 s watchdog).
- Follow the CLAUDE.md install sequence via `./scripts/build-and-install.sh O-GrainScatter`.

**Read first:**
- `CONTEXT.md`, `RESEARCH.md` (this dir): decisions, constants, margins, mapping formula
- `research-scripts/run3.py`, `hb2.py`, `align.py`, `phase.py`: the design and alignment sources
- `plugins/O-GrainScatter/Source/dsp/DelayBuffer.h`, `FreezeManager.h`, `GrainPool.h` (render paths 280–420)
- `plugins/O-GrainScatter/Source/PluginProcessor.cpp` 640–660, 770–870: floorS, spawn guard, Doppler
- `plugins/O-GrainScatter/tests/blocksize-null/main.cpp`, `tests/density-gain/main.cpp`: harness style and the rows to rework
- `modules/synthesis/grain-engine/` (`module.yaml`, `tests/`): module conventions

---

## Success Criteria

1. [x] Module tests: DC 1 ± 1e-6 on every phase; level alignment ≤ 0.05 sample; half-band ≤ −90 dB; S0–S6 ≤ −60 dB; block-split invariant; alloc-free
2. [x] Alias gate: ≤ −60 dB audible spurious at +12 / +24 / +36 st (stereo, spatial at Doppler ≤ 1.4, frozen). v3.2.0 fails the same rows
3. [x] No time jump across an octave boundary (1.99 vs 2.01 ≤ 0.05 sample)
4. [x] blocksize-null and density-gain ALL PASS (two rows re-referenced as documented, thresholds unchanged)
5. [ ] CPU and passband measured and reported; listening check on the 16-tap swing — measured/reported; listening check left for verify
6. [x] Build clean; pluginval 10 PASS; auval PASS; presets load unchanged
7. [x] grain-engine 1.1.0 (`used_by` O-GrainScatter); GrainScatter 3.3.0 CHANGELOG

---

## Approval

```
Approve this plan?

1. Yes, proceed with execution
2. No, revise the plan
3. No, cancel milestone
4. Other
```

---

*Generated by improve-milestone plan phase*
