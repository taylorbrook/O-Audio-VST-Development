# Stage 3 — GUI · Phase 3.2 (Buffer display + live grains) — Plan

**Plugin:** O-Granulator
**Stage:** 3 of 4 (GUI)
**Phase:** 3.2 of 3
**GSD phase:** plan
**Date:** 2026-11-06
**Branch:** `main` (trunk-based) @ `aadcee0c`
**Inputs:**
- `CONTEXT-3.2.md` (D30–D33 locked)
- `RESEARCH-3.2.md` (R117–R126). **Where it differs from CONTEXT-3.2, RESEARCH-3.2 wins**, as amended by the sign-offs and P113–P126 below. Two named overrides: the transport is a triple buffer, not a seqlock (R118), and the peak ring is allocated in the constructor, not in `prepareToPlay` (R120).
- Read at plan: `PluginProcessor.h:160-212` (`Telemetry`, `readTelemetry`, 3.2 fields already declared), `ScaleStateJson.h:113-119` (`telemetryToVar (t, torn)`), `PluginEditor.cpp:401` (torn comment), `index.html:3023-3100` (`drawBuffer`; band line `:3062`), `tests/render-harness/engine_gates_31.h:33, 389` (TL31), `engine_gates_26b.h:960` (M7 alloc gate), `Engine.h:270, 483, 698` (`kPoolLive` / `kPoolFade`)
- `Source/ui/public/index.html` == `.planning/mockups/v2-ui.html` (byte-identical at plan)

---

## Sign-offs (Taylor, 2026-11-06, at plan)

| # | Item | Decision |
|---|---|---|
| 1 | **R118** — transport | **Triple buffer.** It replaces CONTEXT's "seqlock + false-on-torn". The `torn` serialisation path is retired. |
| 2 | **R119** — dot pan | **Include the stream pan.** Display-only sum: `clamp(grainPan + streamPan, −1, 1)`. |
| 3 | **R124** — frozen band | **Accept the approximation.** It is an INFO row plus a hands-on watch item, with no per-grain-length correction. |

---

## Entry Check

- Re-hash the six contracts against the STATUS.md frontmatter. 3.2 edits none of them.
- `git diff --stat a368b4ae HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt modules scripts` is empty. That is the 3.1-verified tree; the commits since then are planning-only.
- `cmp Source/ui/public/index.html .planning/mockups/v2-ui.html` and `cmp Source/ui/public/js/i18n.js .planning/mockups/v2-i18n.js` are clean.

---

## Goal

**UI-02:** the buffer strip is live. It shows a dB-mapped waveform of the ring with the write head, the held span when frozen, per-stream position/spray bands at their **modulated** values, and one dot per sounding grain (delay, pitch, pan, envelope, gain, length). The "grains sounding N / limit" readout and the per-stream bars come from the same snapshot.

Concretely:
- New `Source/dsp/UiTelemetry.h`: `TripleBuffer<UiSnapshot>` (wait-free, no torn reads) and a `PeakRing` (2^18 `atomic<float>`, allocated in the constructor).
- The audio thread **copies only**: per-piece cell peaks read back from the ring, plus a ≤ 60 Hz POD grain/header snapshot at the end of `process()`. All math runs on the message thread inside `readTelemetry`.
- One scoped page edit (the D30 band source, the sanctioned D28 exception). The C++-generated overrides gain `bands`.
- **Audio does not move.** The carried diff against B is empty (the single attributed row change is TL31 → TL32), N-suites `--strict` + R12 rc 0, and there are 0 allocations from block 0.

Not in 3.2: meters, LFO phase, onset pulse, loop gain, mod-matrix view + rings, source meters (3.3); the deferred DAW pass (3.3 verify, D27).

---

## Plan-phase decisions

- **P113 — baseline on the unmodified tree first (T0).** Run the harness default set (474, in the background). The carried extract (sorted `[PASS]/[FAIL]`, INFO/timing rows dropped) must equal 3.1's **B**. If it does not, stop: the tree has drifted. Then N3 / N2 / N1 / N0 `--strict` + R12 rc 0, HA PASS, codec bytes 9/9 == B, param-dump diff-clean, and the five UI gates + probe 220 × 2 as the pre-state. No commit.

- **P114 — `UiTelemetry.h` (R118, R119, R120).** This header depends only on `juce_core` / `<atomic>`. It holds:
  - `template <class T> struct TripleBuffer`, exactly the RESEARCH §2 shape: `mid` (bits 0–1 = index, bit 2 = fresh), `writeBuffer()`, `publish()` (one `exchange`, `acq_rel`), `read()`. `static_assert (std::is_trivially_copyable_v<T>)`.
  - `struct UiGrain` (≈ 56 B POD): `stream1` (uint8, **`g.stream + 1`**, R125), `readAbs` (double), `phase` (double, `(g.age + g.frac) · g.invLen`, the same expression as `renderSegment`), `rate` (double), `lenSamples`, `gainL`, `gainR`, `killGain`, `tiltT/A/B` (float), `windowStep` (int16), `killing` (bool).
  - `struct UiSnapshot`: `seqHead`, `head` (int64, ring domain), `spanSamples` (double), `fs`, `frozen`, `bands[4][2]`, `streamPan[4]`, `limit`, `count`, `grains[128]`, `seqTail`. The two `seq` fields are written first and last; TB1 checks them. Each instance is ≈ 8.2 KB, and the three are `Engine` members (no heap).
  - `struct PeakRing`: `std::unique_ptr<std::atomic<float>[]>` of `kCells = 1 << 18`, allocated in the **constructor**. `prepare (ringSamples)` zero-fills with relaxed stores and sets `P = 32 · max(1, ceil(ringSamples / 32 / kCells))`. It never reallocates, so a `prepareToPlay` called off the message thread cannot free memory under the reader.

- **P115 — peak writer (R120).** In `Engine::process`, per piece and **after** step 3 (`CaptureWriter::write`), when the ring was written (not frozen): `ring.copyLevel (0, ch, wPiece, len, tmp)` into a `float[2][kCell]` stack buffer, take `pk = max |x|` over both channels, then `c = (wPiece / P) mod kCells`. If this is the cell's first write, store `pk`; otherwise store `max(old, pk)`, relaxed. **No change to `CaptureWriter`'s statements**: its header warns that edits there cause FP-contraction drift. While frozen, nothing is written.

- **P116 — snapshot publisher (R119, R121, R123, R124).** At the end of `process()`, when `absCount − lastPublish ≥ round(fs / 60)` or on the first block after `prepare`:
  - `head = ring.getWriteCount()`. This equals W0 while frozen, because the head stops.
  - `spanSamples` = the live `spanSamples` (modulated, after `matrixTick`). While frozen it is `min(spanSamples, fz.heldCaptured)`, the same `spanFrz` that `spawn` uses.
  - `frozen` = the engine's real frozen state (not `freezePending`).
  - `bands[s] = { gp.s[s].position, gp.s[s].spray }`, already tick-modulated fractions (R123). No new modulation math, and `destTelemetry` is not used.
  - `streamPan[s]` = the bus pan in −1…1. `limit = gp.grainLimit`.
  - Grains: copy live slots 0…127 first (`pool.isActive`), then the 16 fade slots while there is room, up to `kMaxGrains = 128`. Set `killing = g.killStep > 0 || spawner.isPendingKill (i) || i >= kPoolLive`.
  - Then `publish()`. Reads only: `pool`, `gp`, `ring`, `fz`, `spawner`. Writes only: telemetry members.

- **P117 — `readTelemetry` derivations (message thread, R119, R121, R122, D31, D32).** Take a reader-only `juce::SpinLock` (for a second editor or the harness), then `read()` and work from the snapshot alone. Never use the live head.
  - `delaySec = max(0, (head − readAbs) / fs)`; `pitchSt = 12 · log2(rate)`; `pan = clamp((4/π)·atan2(gainR, gainL) − 1 + streamPan[s], −1, 1)` (sign-off 2); `gain = sqrt((gainL² + gainR²) / 2)`; `env = window(phase) · killGain`, using `hannWindow` at step 0 / tilt 0.5, otherwise `WindowMorphBank::read (bank.table (step), tilt, phase)` (const, R107); `lengthSec = len / fs`.
  - `counts[s]` = slots of stream s with `! killing`. `limit` comes from the snapshot. `hasCounts = true` (D32 pulls this into 3.2).
  - Wave: `Wc = head / P`, `S = spanSamples / P`, `B = S / 464`, `k0 = floor((Wc − S) / B)`. Bin k = max over cells `[⌊(k0+k)·B⌋, ⌊(k0+k+1)·B⌋)`. Cells `≥ Wc` or `< Wc − ringCells` count as 0. Map with `v = pk > 0 ? clamp((20·log10(pk) + 48) / 48, 0, 1) : 0`. `waveCount = 464`.
  - `spanSec = spanSamples / fs`; `frozen`; `hasBuffer = true`. Add `bands` (`std::array<std::array<float,2>,4>`) to `Telemetry`, carried under `hasBuffer`.
  - Returns false **only** when `! engine.isPrepared()`. In that case `hasBuffer = hasCounts = false`, and the page falls back to the base knobs.

- **P118 — retire the torn path (sign-off 1).** Delete `telemetryToVar`'s `torn` parameter and its `{ scaleRev }`-only branch. The editor no longer branches on the return value: it always sends the full var, and when `readTelemetry` is false, `has*` false omits the keys. Update the `PluginEditor.cpp:401` comment and the `PluginProcessor.h` doc comment ("no seqlock in 3.1" → the triple-buffer contract). `telemetryToVar` emits `bands` as `[[pos, spray] × 4]` when `hasBuffer` is set.

- **P119 — payload size (RESEARCH §7).** PL1 INFO records the serialised bytes of the widest frame (464 wave values + 128 grains). **If it is > 32 KB**, quantise on the message thread before building the `var` (wave to 1/1000, delay/length to 1e-4 s, pitch to 0.01 st, pan/env/gain to 1/1000) and confirm that the JUCE formatter then emits the short form. This is a recorded deviation, not a stop.

- **P120 — harness rows, in a new `engine_gates_32.h`** (RESEARCH §8, with NCs that must FAIL their predicate):
  - **TB1**: threaded stress. The writer publishes at 10 kHz and the reader spins for 1 s. Every frame has `seqHead == seqTail`. **TB1n:** a single-buffer variant tears (mismatches > 0).
  - **WV1**: an impulse train at known ring positions produces peak bins at the expected delays ± 1 bin, live, at 44.1 / 48 / 96 / 192 k. **WV1n:** binning at the live head with a 512 block and a 1 s span misses the ± 1 bin tolerance.
  - **WV2**: D31 map. Silence → 0; −30 dBFS sine → 0.375 ± 0.01; 0 dBFS → 1; −60 dBFS → 0.
  - **WV3**: frozen `spanSec == min(span, heldCaptured) / fs`. An early freeze (less than one span captured) gives the short span.
  - **WV4**: while frozen, the wave is bitwise identical over 50 consecutive publishes. **WV4n:** fractional head-anchored bins differ.
  - **GD1**: dot delay vs the spawn log, `(head − readAbs) / fs` within 1e-6 s. Stream-1 dots are present and 1-based. **GD1n:** a 0-based copy drops stream 1.
  - **GD2**: spread 0 / gain_rand 0 → pan 0 and gain 1 exactly. Spread 1 → pan within ±1. gain_rand produces the expected cut. Pitch +7 st → 7. `env` == `windowCurve` at the phase. A hard-left stream pan shifts the dot pan (sign-off 2).
  - **CN1**: `Σ counts` == the Spawner census of alive slots, each ≤ limit, and `limit == grain_limit` across caps 16 / 32 / 64 / 128. **CN1n:** counting killing/fade slots exceeds the limit under the GL3 cap-lowered burst.
  - **BD1**: with an LFO routed to `s1_position`, the snapshot band ≠ base and == `gp.s[0].position` at the tick. Unrouted streams == base, bitwise.
  - **M8**: 0 allocations from block 0 with publishing and peak writes active. Run the 8192-after-512 block sequence. **M8L** liveness: a deliberate malloc on the render thread counts exactly 1.
  - **TL32**: replaces TL31. The 3.1 fields are unchanged, the 3.2 fields (wave, grains, span, frozen, bands, counts, limit) are live, and the 3.3 fields are still zero / NaN.
  - **FB1** (INFO, R124): the frozen band's offset from the measured spawn region with 2 s grains at a 30 s span.
  - **PL1** (INFO): widest-frame bytes (P119), plus publish cost in µs @ 48 k / 192 k.

- **P121 — audio-neutral proof.** After T2–T4, run the harness default set. The carried extract diff vs B must be **empty except TL31 → TL32**, recorded as an attributed row change (the A‴ precedent). Then N3 / N2 / N1 / N0 `--strict` + R12 rc 0, HA PASS, codec bytes 9/9 == B, param-dump clean. If any carried audio row moves, stop and ask: that means a telemetry statement feeds DSP state.

- **P122 — page edit (D30, the only D28 exception).** Replace `index.html:3062` with the RESEARCH §5 two-liner (`t.bands[n − 1]` with a fallback to `scaled(…) / 100`). Nothing else in the page changes. Mirror the edit into `.planning/mockups/v2-ui.html` so the byte-identical relation holds. Record it as a deviation entry with a diff excerpt.

- **P123 — overrides.** `--dump-ui-overrides` adds `bands` to the `getTelemetry` widest frame: every stream at pos 0.5 / spray 1.0, plus every 3.2 key with realistic widest values (464 wave bins, 128 grains spread across streams 1–4, `frozen: false`, counts 32 × 4, limit 128). Generate it twice: the output must be byte-identical, and `git diff` after the gate step must be clean. `ui_frontend_check.js`'s "emitted telemetry keys ⊆ page-read keys" check must now include `bands`. Extend its key list from the C++ side, not by hand.

- **P124 — install + validate.** Run `./scripts/build-and-install.sh O-Granulator`. VST3 + AU + Standalone with 0 warnings; `auval -v aufx …`; pluginval 10 VST3 + AU.

- **P125 — gate run order (T7).** Regenerate overrides (diff-clean) → `ui_frontend_check.js` (+ NCs) → `ui_binding_probe.js` 220 × 2 (+ NC) → check-i18n → fr-lint → zh-lint → check-ui-labels (banner `+ overrides`, `states: default + 8`) → boot-all-uis `--strict-tips`. No new i18n strings are expected. If one appears, it goes in en / fr / zh-Hans with the glossary. Exit 77 = not run = not a pass.

- **P126 — HANDS-ON-3.2.md (D33), not run in execute.** It covers RESEARCH §9 items 1–8, plus: a reopen of the editor ×10 with no stale strip, and a second editor window (AU + Standalone, or two instances) to exercise the reader SpinLock. Expected 3.3 stub artefacts are listed so that verify does not flag them: level lane flat, LFO phase dot at left, no onset pulse, loop gain "Open". Taylor runs it in the Standalone at **3.2 verify**.

---

## Source layout (deltas)

| File | Change |
|---|---|
| `Source/dsp/UiTelemetry.h` (new) | `TripleBuffer`, `UiGrain`, `UiSnapshot`, `PeakRing` (P114) |
| `Source/dsp/Engine.h/.cpp` | members (`TripleBuffer<UiSnapshot>`, `PeakRing`, `lastPublish`); peak write after `CaptureWriter::write` (P115); publish at the end of `process()` (P116); a const accessor for the reader. **No DSP statement changes.** |
| `Source/PluginProcessor.h/.cpp` | `Telemetry::bands`; `readTelemetry` derivations + wave binning + reader SpinLock (P117); doc comment (P118) |
| `Source/ScaleStateJson.h` | `torn` removed; `bands` emitted; optional quantisation (P118, P119) |
| `Source/PluginEditor.cpp` | stops branching on torn; comment (P118) |
| `Source/ui/public/index.html` + `.planning/mockups/v2-ui.html` | the `drawBuffer` band line only (P122) |
| `tests/render-harness/engine_gates_32.h` (new), `engine_gates_31.h`, `main.cpp` | P120 rows; TL31 → TL32; overrides `bands` (P123) |
| `tests/ui-stub/generic-overrides.json` | regenerated (P123) |
| `tests/ui_frontend_check.js` | telemetry key set includes `bands` (P123) |

Unchanged: `modules/**`, `scripts/**`, every contract file, `CaptureWriter.h`, every DSP statement.

---

## Tasks

### Wave 0 — precondition

1. **T0 — Entry Check + baseline** (P113). Log everything under `${TMPDIR:-/tmp}/ogran-32/`. Run the harness in the background (the 600 s watchdog applies). No commit.
   - Depends on: none

### Wave 1 — engine side (audio-neutral)

2. **T1 — `UiTelemetry.h`** (P114). Header only, with static_asserts for triviality and size.
   - Files: `Source/dsp/UiTelemetry.h`
   - Depends on: T0
3. **T2 — peak writer + publisher** (P115, P116).
   - Files: `Source/dsp/Engine.h/.cpp`
   - Depends on: T1

### Wave 2 — processor + serialisation

4. **T3 — `readTelemetry` + torn retirement** (P117, P118, P119).
   - Files: `PluginProcessor.h/.cpp`, `ScaleStateJson.h`, `PluginEditor.cpp`
   - Depends on: T2
5. **T4 — harness rows + neutral proof** (P120, P121). Create `engine_gates_32.h`, rewrite TL31 → TL32, and run every NC. Then the full default set, N-suites, R12, HA, codec bytes, param-dump. **Commit** (path-scoped).
   - Files: `tests/render-harness/engine_gates_32.h`, `engine_gates_31.h`, `main.cpp`
   - Depends on: T3

### Wave 3 — page + install

6. **T5 — page band edit + overrides** (P122, P123). The page line, the `v2-ui.html` mirror, overrides `bands` + regeneration ×2, and the `ui_frontend_check.js` key set.
   - Files: `index.html`, `v2-ui.html`, `main.cpp`, `generic-overrides.json`, `ui_frontend_check.js`
   - Depends on: T4
7. **T6 — build + install + validate** (P124). One Standalone `screencapture` of a live strip for the SUMMARY (any input, freeze off). **Commit.**
   - Depends on: T5

### Wave 4 — UI gates

8. **T7 — full gate run** (P125). Everything green, with exit codes recorded. A page defect is fixed under D28 as a recorded deviation, then the whole T7 sequence re-runs. **Commit.**
   - Depends on: T6

### Wave 5 — docs + handoff

9. **T8 — docs.** `HANDS-ON-3.2.md` (P126); `SUMMARY-3.2.md` (rows, counts, the TL31 → TL32 attribution, NC results, PL1 / FB1 INFO values, deviations); STATUS.md (`stage_3_phase_3_2_execute_complete`, `next_action: stage_3_phase_3_2_verify`); REQUIREMENTS UI-02 → `partial` (hands-on pending). **Commit** path-scoped: `git commit -- plugins/O-Granulator`.
   - Depends on: T7

---

## Dependency graph

```
T0 ── T1 ── T2 ── T3 ── T4 ── T5 ── T6 ── T7 ── T8
```

The chain is linear on purpose: each step is gated by the previous step's neutrality proof.

---

## Gate criteria (3.2)

| Gate | Pass |
|---|---|
| Harness default set | all PASS; carried extract diff vs B = **TL31 → TL32 only** (attributed); P120 rows PASS; TB1n / WV1n / WV4n / GD1n / CN1n each FAIL their predicate; M8L == 1 |
| N3 / N2 / N1 / N0 `--strict`, R12 | rc 0 |
| HA host-cache · codec bytes · param-dump | PASS · 9/9 == B · diff-clean |
| Build | VST3 + AU + Standalone, 0 warnings |
| auval, pluginval 10 VST3 + AU | PASS |
| `generic-overrides.json` | regenerated twice, byte-identical; diff-clean vs committed |
| `ui_frontend_check.js` | 0 failures (key set incl. `bands`); NCs FAIL |
| `ui_binding_probe.js` | 220 / 220 ×2; NC FAILS |
| check-i18n · fr · zh · check-ui-labels · boot-all-uis `--strict-tips` | 0 · 0 · 0 · 0 (+ overrides, default + 8) · clean |
| Page | `index.html` == `v2-ui.html`; diff vs 3.1 = the band line only |

---

## Risks / watch items

- **Neutrality.** Every new audio-thread statement must only read DSP state. The peak writer uses `copyLevel` (a read) and never touches `CaptureWriter`. If a carried audio row moves, stop: never re-baseline to make it pass.
- **Allocation.** `PeakRing` is allocated in the constructor and the snapshots are members. M8 must arm from block 0 *with publishing active*, because a first-block publish is the likeliest place for an accidental allocation.
- **Reader concurrency.** A `TripleBuffer` with a single reader is the precondition. The SpinLock covers multiple editors and the harness, and the audio thread never takes it.
- **Ring domain.** `delaySec` and the wave both come from the snapshot `head` (R121). If any code path reads `ring.getWriteCount()` on the message thread, dots and wave misalign by up to a block. WV1n proves the gate catches this.
- **Frozen.** `spanFrz` must be the same value `spawn` uses. If they diverge, the "held for N s" caption and the wave extent disagree with where grains land (WV3).
- **Page under D28.** One line changes. Any other page edit is a defect fix with its own deviation entry.
- **Windows.** There is no new WebView surface, and the existing null-guards cover it.

## Deviations protocol

As in 3.1: record every departure from this plan in SUMMARY-3.2 (what, why, evidence, which gate). Stop and ask only for an audio-moving change, a contract edit, a module change, or a v2 design change beyond a defect.

## Carried (not 3.2)

- 3.3: meters, LFO phase, onset, loop gain, mod-matrix view + rings, source meters. The DAW pass (DAW-2.5a 1–8, DAW-2.5b 1–5, DAW-2.6 1–3) runs at 3.3 verify (D27).
- Stage 4: Env 1e30 pin. Harness items: C7 machine-speed calibration, event-aware `renderPool` (Z6a).
- W2o: widen seeds, never the threshold.

## Success criteria (gate for verify)

- [ ] Buffer strip shows a live dB-mapped waveform (D31) anchored to the snapshot head, scrolling in whole-bin steps (WV1, WV2)
- [ ] Freeze: held span from `min(span, heldCaptured)`, wave bitwise stable (WV3, WV4)
- [ ] Grain dots: delay in the wave's time domain, 1-based streams, exact pan/gain/pitch/env recovery incl. stream pan (GD1, GD2)
- [ ] Bands follow modulated position/spray; base when unrouted (BD1, D30)
- [ ] Counts never exceed the limit and match the census (CN1, D32)
- [ ] Triple buffer never tears; `readTelemetry` false only when unprepared; torn path removed (TB1, P118)
- [ ] 0 allocations from block 0 (M8); audio unchanged (carried diff = TL31 → TL32 only; N-suites + R12 + HA)
- [ ] All UI gates green; the page diff is the band line only
- [ ] HANDS-ON-3.2.md ready for verify (UI-02 completes there)
