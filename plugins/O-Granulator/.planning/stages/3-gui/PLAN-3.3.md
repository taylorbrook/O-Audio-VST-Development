# Stage 3 — GUI · Phase 3.3 (Mod matrix view + mod rings) — Plan

**Plugin:** O-Granulator
**Stage:** 3 of 4 (GUI)
**Phase:** 3.3 of 3
**GSD phase:** plan
**Date:** 2026-11-07
**Branch:** `main` (trunk-based) @ `b11a8f6c`
**Inputs:**
- `CONTEXT-3.3.md` (D34–D41 locked)
- `RESEARCH-3.3.md` (R127–R136). **Where it differs from CONTEXT-3.3, RESEARCH-3.3 wins**, as amended by the sign-offs and P127–P142 below. Two named refinements: the onset pulse runs on a counter plus a reader wall clock with a 200 ms decay (R131 → D40), and the loop gain is computed from `fbKnobNow` (R133 → D34).
- Re-read at plan: `Engine.cpp:715–750` (`gridTick` feedback block: `fbKnobNow = k` on both branches), `Engine.cpp:477–483` (`eventOffset` from `analysis.processPiece`), `FeedbackChain.h:179–239` (`stepAgc`, `kAgcCeil`, plain `float env`), `ModSources.h:209–210` (`position()`, `getPhaseDeg()`), `PluginProcessor.h:165–219, 255` (3.3 `Telemetry` fields already declared, `readTelemetry (Telemetry&) const`, `telemetryLock`), `engine_gates_32.h:725–785` (TL32 asserts that the 3.3 fields are zero), `tests/i18n-states.json` (9 states)
- `git diff --stat 00a04a89 HEAD -- Source tests CMakeLists.txt modules scripts` is empty at plan: the tree is the one 3.2 verified.

---

## Sign-offs (Taylor, 2026-11-07, at plan)

| # | Item | Decision |
|---|---|---|
| 1 | **R131** (refines D40): onset pulse | **Audio-side event counter + reader wall clock, 200 ms linear decay.** `onsetPulse = clamp(1 − (now − pulseStartMs)/200, 0, 1)`. No hold. No sample clock. |
| 2 | **R133** (refines D34): loop-gain source | **`fbKnobNow` × AGC.** The value includes a matrix route on Feedback. "Open" iff the effective k == 0 exactly, or frozen. |
| 3 | **R130**: stale reads | **250 ms gap rule.** If `now − lastReadMs > 250` (this includes the first read), consume every accumulator and report 0 peaks / `onsetFired` false / pulse 0 for that frame. |

---

## Entry Check

- Re-hash the six contracts against the STATUS.md frontmatter. 3.3 edits none of them.
- `git diff --stat 00a04a89 HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt modules scripts` is empty.
- `cmp Source/ui/public/index.html .planning/mockups/v2-ui.html` and `cmp Source/ui/public/js/i18n.js .planning/mockups/v2-i18n.js` are clean.

---

## Goal

**UI-03:** the mod matrix view and the mod rings are complete and route edits are click-free. This closes the Stage 3 carried list.

Concretely:
- **Meters** (D38): `out[2]` = peak of the final output buffer, `inPeak` = peak after input gain, measured in the engine piece loop (R128). Both are max-since-read and use a CAS fetch-max once per block (R129).
- **LFO phase** (D39, R132): offset-free cycle position per LFO, derived from `position() − φ/360`. `Lfo` is unchanged.
- **Onset** (D40 as amended by sign-off 1): `onsetFired` + `onsetPulse` from an event counter and a reader wall clock.
- **Loop gain** (D34 as amended by sign-off 2): `20·log10(fbKnobNow · g_agc)`, absent at k == 0 or frozen, ≤ −0.4455 dB by construction.
- **Route edits** (D36, R127): a new UX1 gate replays the page's real write sequences. A DSP fix happens **only if UX1 fails**, and it is confined to `ModMatrix::tick` change cells.
- **i18n:** a new `streams-all-enabled` state (R136, closes 3.1 deviation 10).
- **The page is not edited** (D35, D28). Every consumer already exists.
- **Audio does not move.** The carried diff vs **C** (the 3.2 baseline) = TL32 → TL33 + new rows only. N-suites `--strict` + R12 rc 0. 0 allocations from block 0.
- **HANDS-ON-3.3.md** + **DAW-3.3.md** (the consolidated Logic AU pass: DAW-2.5a 1–8, DAW-2.5b 1–5, DAW-2.6 1–3, plus the 3.3 items). Taylor runs both at 3.3 verify (D37, D41).

---

## Plan-phase decisions

- **P127 — baseline C on the unmodified tree first (T0).** Run the harness default set (491 rows, in the background). The carried extract (sorted `[PASS]/[FAIL]`, INFO/timing rows dropped) must equal 3.2 verify's 491-row extract. Call it **C**. If it differs, stop: the tree has drifted. Then record the pre-state: N3 / N2 / N1 / N0 `--strict` + R12 rc 0, HA PASS, codec bytes 9/9 == B, param-dump diff-clean, `generic-overrides.json` regenerated ×2 byte-identical, and the five UI gates + probe 220 × 2. No commit.

- **P128 — UX1 first, before any telemetry code (R127, D36).** UX1 runs on the unmodified engine, so it answers D36 before anything else changes. Put it in a new `engine_gates_33.h`:
  - Base: reuse `mf8Job` (sine 440, 4 streams, mix 50 %, LFO1 2 Hz at its peak, and Onset).
  - Destinations: `s1_level`, `mix`, `output_gain`, `feedback`.
  - Sequences: `clear` (source → 0, dest → 0, depth → 0), `drop` (dest → d, depth → 0.5, **source last**), `dest swap`, `source swap`, `depth drag` (10 retargets ±60 over 300 ms).
  - Multi-write sequences are spread across 0, 1 and 7 cells.
  - Verdicts: (a) trajectory via `mx3Check` ≤ 1.05× the analytic bound; (b) audio κ ≤ 2.0× the **held-route reference with the same motion** (pattern_step_click_gate_needs_same_motion_control), on the tonal source.
  - NCs (must FAIL): `UX1nR` = `noResidual` hook on `clear`, `UX1nD` = `noDepthRamp` hook on `drop`, each > 2× on at least one destination. Reuse the MX3nR / MX3nD hooks if they exist; otherwise add them behind the existing test-hook pattern (`#if OGRAN_TEST_HOOKS`).
  - **If UX1 passes:** D36 closes with zero audio change. Continue.
  - **If UX1 fails:** stop and report the failing sequence, destination and κ. The fix is confined to `ModMatrix::tick` and fires only on change cells. No-edit renders stay byte-identical by construction. Taylor signs it off at execute (D25 pattern) before it lands.

- **P129 — engine telemetry members (R128, R129, R132, R133, R134).** All are audio-thread writers and `prepare`-reset (message thread), next to `srcTelemetry`:
  - `mutable std::atomic<float> inPk { 0 }`, the in-meter accumulator (exchanged by the reader).
  - `std::atomic<uint32_t> onsetCount { 0 }`, single writer: `store (load + 1)` when `eventOffset >= 0` (at most once per piece).
  - `std::array<std::atomic<float>, 4> lfoPhaseTel`, stored in `tickSources` right after `lfos[n].tick`: `p = position() − getPhaseDeg()/360`, `ph = (float)(p − floor(p))`, `if (!(ph < 1.0f)) ph = 0`.
  - `std::atomic<float> loopK` (stored per `gridTick`, right after `fbKnobNow = k`), `loopAgc` (end of `process()`: `env > 0 ? min(1, kAgcCeil/env) : 1`), `std::atomic<bool> frozenTel` (end of `process()`, the stopped head, **not** `freezePending`).
  - Per piece in `Engine::process`, before `mixOut`: `pk = std::max (pk, std::max (|inL|, |inR|) · inGain.value (c))`, with the value as the **second** argument so a NaN is dropped. Once per block: `pushMax (inPk, pk)`.
  - `static_assert (std::atomic<float>::is_always_lock_free && std::atomic<uint32_t>::is_always_lock_free)`.
  - Add a `pushMax` helper (CAS loop, relaxed) to `UiTelemetry.h`.
  - Const consume getters (`consumeInPeak()` exchange 0; `onsetCountNow()`, `lfoPhase(n)`, `loopK()`, `loopAgc()`, `isFrozenTel()` loads).
  - **No DSP statement changes.** Every new statement reads DSP state and writes only telemetry members.

- **P130 — out meter in the processor (R128).** In `processBlock`, after `engine.process`: per channel in `[0, numOut)`, take `FloatVectorOperations::findMinAndMax` → `max(|min|, |max|)`, then `pushMax (outPkL / outPkR)`. Mono out → R = L. The two atomics are `mutable` processor members. Bypass doesn't call `processBlock`, so the meters read 0 there (correct).

- **P131 — `readTelemetry (Telemetry&, double nowMs) const` (R130, R131, R133, R134, sign-offs).** The existing 1-arg form forwards `Time::getMillisecondCounterHiRes()`. Reader state is `mutable double lastReadMs = -1e9, pulseStartMs = -1e9; mutable uint32_t lastOnsetCount = 0;`, all under `telemetryLock`.
  - `stale = now − lastReadMs > 250`, then `lastReadMs = now`.
  - Meters: exchange all three accumulators to 0. Clamp non-finite values to 0. `hasMeters = true` once prepared; values = 0 if stale.
  - Onset: `c = onsetCount`. If `c != lastOnsetCount`: `lastOnsetCount = c`, and unless stale, `onsetFired = true`, `pulseStartMs = now`. `onsetPulse = clamp(1 − (now − pulseStartMs)/200, 0, 1)` (0 if stale). `hasOnset = true` once prepared.
  - LFO: `lfoPhase[n] = engine.lfoPhase(n)`.
  - Loop: `k = loopK`, `g = loopAgc`. `hasLoopGain = !(k == 0.0f || frozen)`. If present, `loopGainDb = 20·log10(k·g)`; else NaN.
  - All 3.1 / 3.2 behaviour is unchanged. It returns false only when unprepared, and then every `has*` is false.

- **P132 — serialisation.** `telemetryToVar` already emits every 3.3 key behind its flag (CONTEXT table). Confirm `loopGainDb` is emitted only when finite. **No shape change is expected.**

- **P133 — harness rows in `engine_gates_33.h`** (RESEARCH §8; NCs must FAIL their predicate; every TL / MT / ON row injects `nowMs` and is designed for destructive reads):
  - **MT1**: a −6 dBFS sine at mix 0 → `out` = 0.5012 ± 1e-5 on both channels. +6 dB output gain → × 1.995. Mono out → R = L.
  - **MT2**: a −12 dBFS sine with input_gain +6 → `inPeak` 0.5012 ± 1e-5. Feedback 80 % does **not** raise it. **MT2n:** the ring-peak measure (the 3.2 `PeakRing`) rises with feedback.
  - **MT3**: max since read. A 1-sample 0.9 spike in block 3 of 10 → the read returns 0.9. An immediate second read → 0.
  - **MT4**: in-place. Silent input with a loud frozen wet tail → `inPeak` 0. **MT4n:** measuring after `engine.process` reads ≠ 0.
  - **MT5**: staleness. A gap > 250 ms → 0 / `onsetFired` false. The next read is normal. This includes the first read after prepare.
  - **ON1**: a 4 Hz click train with reads every 33 ms (injected clock). `onsetFired` is set once per event-bearing read. Pulse 1 → 0.5 at +100 ms → 0 at +200 ms.
  - **ON2**: idle host. There are no `process` calls while the clock advances, and the pulse reaches 0. **ON2n:** a sample-domain pulse stays > 0.5.
  - **LP1**: Free 1 Hz: phase = frac(N·32/fs) ± 1e-6 after N cells. With `lfo1_phase` 90°: the phase is unchanged, and `lfo` = sin(2π(phase + 0.25)) ± 1e-6.
  - **LP2**: Tempo 1/4 at 120 bpm: phase = frac((ppq − bar)/1) ± 1e-6.
  - **LG1**: fb 50 % with quiet input (AGC idle) → −6.465 dB ± 0.01. fb 100 % with hot input → < 20·log10(0.95) and strictly below the knob-only value.
  - **LG2**: fb 0 → absent. Freeze on (after the edge) → absent. Unfreeze → present. `freezePending` alone → still present.
  - **LG3**: knob 0 + LFO1 → Feedback 50 % → present, finite, ≤ −0.4455. **LG3n:** a ramp-based k reads absent.
  - **UX1 / UX1nR / UX1nD**: P128.
  - **TL33**: replaces TL32 (R135). The 3.1 + 3.2 fields == engine (unchanged checks). The 3.3 fields are live: `hasMeters` / `hasOnset` true once prepared, `hasLoopGain` per the state, `lfoPhase[0]` ≠ 0 after 1 s at 1 Hz. TL32u's "false before prepare" sub-row is kept as TL33u.
  - **M9**: M8 extended. 0 allocations from block 0 with every 3.3 write active (CAS, counter, phase, loop, out-meter scan), including the 8192-after-512 sequence. **M9L** liveness == 1.
  - **C8** (INFO): CPU at the C7 config, with the delta vs 3.2's C7 recorded. Expected ≈ 0.

- **P134 — audio-neutral proof.** After T3–T5, run the harness default set. The carried extract diff vs C must be **TL32 → TL33 (attributed, as R126 / A‴) + new rows only**. Then N3 / N2 / N1 / N0 `--strict` + R12 rc 0, HA PASS, codec bytes 9/9 == B, param-dump clean, and the block-size null (N9 / N10) unchanged. If any carried audio row moves, **stop**: that means a telemetry statement feeds DSP state. Never re-baseline to make it pass.

- **P135 — i18n state (R136).** Append `streams-all-enabled` to `tests/i18n-states.json` with the RESEARCH §7 eval. This makes the check-ui-labels banner `states: default + 10`. A label overflow in any language is a page defect, fixed under D28 with a deviation entry.

- **P136 — overrides check.** Regenerate `generic-overrides.json` with `--dump-ui-overrides` twice. The file is expected to be **byte-identical to the committed one**. A diff means the telemetry shape moved. In that case, regenerate the binding-probe stub and record it.

- **P137 — install + validate.** Run `./scripts/build-and-install.sh O-Granulator`. VST3 + AU + Standalone with 0 warnings, **and rebuild the Standalone after the install** (3.2 process note). Then `auval -v aufx …` and pluginval 10 VST3 + AU.

- **P138 — gate run order (T7).** overrides (P136) → `ui_frontend_check.js` (+ NCs) → `ui_binding_probe.js` 220 × 2 (+ NC) → check-i18n → fr-lint → zh-lint → check-ui-labels (`+ overrides`, `default + 10`) → boot-all-uis `--strict-tips`. Exit 77 = not run = not a pass.

- **P139 — HANDS-ON-3.3.md (D41).** It covers RESEARCH §10 plus:
  - Matrix end to end: route via chip drag, via select, clear a slot, slot count.
  - Rings: the arc plus a live dot on a routed knob, and Alt-drag depth.
  - Source-chip tints.
  - The S&H / random LFO dot leaving the stylised curve (accepted, D39).
  - The saturation note on the loop-gain readout (D34).
  - Route edits by ear on a sustained tone (D36).
  - A two-window check (AU + Standalone) for the reader lock with destructive reads: both windows' meters move. With two editors each one sees part of the peaks; that is accepted and recorded as a note.

- **P140 — DAW-3.3.md (D37).** One Logic Pro (AU) session doc that references DAW-2.5a 1–8, DAW-2.5b 1–5 and DAW-2.6 1–3 verbatim by item, plus the 3.3 Logic items:
  - Stopped transport on an unarmed track: the pills go dark within 200 ms (R131).
  - Editor closed during a loud passage, then reopened: no phantom peak or onset tick (R130).
  - Tempo LFO dot jumps on transport start / locate.
  - Meters in Logic's mono-insert and stereo configurations.

  Each item carries its FAIL consequence (FUNC-04 / QUAL-03, FUNC-08 / DSP-08, the D21 600 s fallback). Not run in execute.

- **P141 — page.** The page is not edited (D35). If a 3.3 field shows a consumer defect at T7 or in the Standalone smoke check, the fix is a scoped D28 exception, mirrored into `v2-ui.html` and recorded as a deviation, and the whole T7 sequence re-runs.

- **P142 — REQUIREMENTS.** UI-03 → `partial` at execute (hands-on pending), `complete` at verify. Stage 3 closes at 3.3 verify only if HANDS-ON-3.3 and DAW-3.3 both PASS.

---

## Source layout (deltas)

| File | Change |
|---|---|
| `Source/dsp/UiTelemetry.h` | `pushMax` CAS helper + lock-free static_asserts (P129) |
| `Source/dsp/Engine.h/.cpp` | telemetry atomics; in-peak per piece; onset counter; LFO phase in `tickSources`; `loopK` in `gridTick`; `loopAgc` / `frozenTel` at the end of `process()`; `prepare` resets; consume getters (P129). **No DSP statement changes.** |
| `Source/dsp/FeedbackChain.h` | a const `env` accessor only, if one is not already reachable |
| `Source/dsp/ModMatrix.h` | **only if UX1 fails** (P128, signed off at execute) |
| `Source/PluginProcessor.h/.cpp` | out-meter atomics + scan (P130); `readTelemetry (t, nowMs)` + reader state (P131); doc comment |
| `Source/ScaleStateJson.h` | none expected (P132) |
| `tests/render-harness/engine_gates_33.h` (new), `engine_gates_32.h`, `main.cpp` | P133 rows; TL32 → TL33 |
| `tests/i18n-states.json` | `streams-all-enabled` (P135) |
| `.planning/stages/3-gui/HANDS-ON-3.3.md`, `DAW-3.3.md` (new) | P139, P140 |

Unchanged: `modules/**`, `scripts/**`, every contract file, `CaptureWriter.h`, `ModSources.h`, `Analysis.h`, the page (unless P141 applies).

---

## Tasks

### Wave 0 — precondition

1. **T0 — Entry Check + baseline C** (P127). Log everything under `${TMPDIR:-/tmp}/ogran-33/`. Run the harness in the background (the 600 s watchdog applies). No commit.
   - Depends on: none

### Wave 1 — D36 answer on the unmodified engine

2. **T1 — UX1 + NCs** (P128). Create `engine_gates_33.h` with UX1 only and wire it into `main.cpp`. Run UX1 / UX1nR / UX1nD (`--only33` filter).
   - **PASS** → continue.
   - **FAIL** → stop and ask (P128).
   - Files: `tests/render-harness/engine_gates_33.h`, `main.cpp`
   - Depends on: T0

### Wave 2 — engine + processor telemetry (audio-neutral)

3. **T2 — engine telemetry writers** (P129).
   - Files: `UiTelemetry.h`, `Engine.h/.cpp` (+ `FeedbackChain.h` accessor)
   - Depends on: T1
4. **T3 — out meter + `readTelemetry`** (P130, P131, P132).
   - Files: `PluginProcessor.h/.cpp` (+ `ScaleStateJson.h` only if P132 finds a gap)
   - Depends on: T2

### Wave 3 — gates + neutral proof

5. **T4 — harness rows** (P133). MT1–5, ON1–2, LP1–2, LG1–3, TL32 → TL33, M9, C8, plus every NC. Run `--only33` until green.
   - Files: `engine_gates_33.h`, `engine_gates_32.h`, `main.cpp`
   - Depends on: T3
6. **T5 — neutral proof** (P134). Full default set, N-suites, R12, HA, codec bytes, param-dump, block-size null. **Commit** (path-scoped: `git commit -- plugins/O-Granulator`, after re-checking the branch and `git status`).
   - Depends on: T4

### Wave 4 — UI + install

7. **T6 — i18n state + overrides + install** (P135, P136, P137). Take one Standalone `screencapture` with meters, LFO dot and loop gain live (feedback 50 %, an LFO routed) for the SUMMARY. **Commit.**
   - Files: `tests/i18n-states.json` (+ overrides / stub only if P136 diffs)
   - Depends on: T5
8. **T7 — full UI gate run** (P138, P141). Everything green with exit codes recorded. **Commit** if anything changed.
   - Depends on: T6

### Wave 5 — docs + handoff

9. **T8 — docs.**
   - `HANDS-ON-3.3.md` (P139) and `DAW-3.3.md` (P140).
   - `SUMMARY-3.3.md`: rows, counts, the UX1 verdict (D36 answer), the TL32 → TL33 attribution, NC results, C8, deviations.
   - STATUS.md: `stage_3_phase_3_3_execute_complete`, `next_action: stage_3_phase_3_3_verify`.
   - REQUIREMENTS: UI-03 → `partial` (P142).
   - **Commit** path-scoped.
   - Depends on: T7

---

## Dependency graph

```
T0 ── T1 ──┬─(UX1 PASS)── T2 ── T3 ── T4 ── T5 ── T6 ── T7 ── T8
           └─(UX1 FAIL)── STOP → sign-off → ModMatrix fix → re-run T1
```

The chain is linear: each step is gated by the previous step's proof. T1 comes first because UX1 is the phase's ROADMAP test, and its answer decides whether any audio change happens at all.

---

## Gate criteria (3.3)

| Gate | Pass |
|---|---|
| Harness default set | all PASS; carried extract diff vs C = **TL32 → TL33 + new rows only** (attributed); P133 rows PASS; UX1nR / UX1nD / MT2n / MT4n / ON2n / LG3n each FAIL their predicate; M9L == 1 |
| UX1 (ROADMAP test) | trajectory ≤ 1.05× bound, κ ≤ 2.0× held-same-motion reference, all sequences × spreads × 4 destinations |
| N3 / N2 / N1 / N0 `--strict`, R12, block-size null | rc 0; N9 / N10 null 0 |
| HA · codec bytes · param-dump | PASS · 9/9 == B · diff-clean |
| Build | VST3 + AU + Standalone, 0 warnings; Standalone rebuilt after install |
| auval, pluginval 10 VST3 + AU | PASS |
| `generic-overrides.json` | regenerated ×2, byte-identical to committed (or the diff is recorded + stub regenerated) |
| `ui_frontend_check.js` · `ui_binding_probe.js` | 0 failures + NCs FAIL · 220 / 220 ×2 + NC FAILS |
| check-i18n · fr · zh · check-ui-labels · boot-all-uis `--strict-tips` | 0 · 0 · 0 · 0 (+ overrides, default + 10) · clean |
| Page | `index.html` == `v2-ui.html`, and unchanged vs 3.2 (unless P141, recorded) |
| Loop-gain bound | every LG row ≤ −0.4455 dB when present |

---

## Risks / watch items

- **Neutrality.** The in-peak statement sits in the piece loop next to DSP code. It must only read `inL / inR / inGain.value(c)`, and `inGain` must not be ticked an extra time. If a carried row moves, stop.
- **NaN in `std::max`.** The argument order matters (P129). MT rows include a NaN-input case through the R96 guard path. The reader also clamps non-finite values.
- **Destructive reads.** Any existing TL / harness code that calls `readTelemetry` twice in a row now sees 0 the second time for meters. Audit the TL32 call sites before rewriting them. `dumpUiOverrides` overwrites after its read, so it stays deterministic.
- **Wall clock in the harness.** Every row injects `nowMs`. No harness row may call the 1-arg form, or rows become timing-dependent (C7 contention precedent).
- **`frozen` vs `freezePending`.** LG2 gates that the pending fade still reads "present".
- **UX1 negative controls.** If the MX3 hooks don't fit the page sequences, the new hooks go behind the test-hook guard only. A hook that compiles into release is a defect.
- **Two editors.** Destructive reads split the peaks between readers. This is a display-only artefact, recorded in hands-on, not fixed.
- **Windows.** There is no new WebView surface. The out-meter scan uses JUCE FloatVectorOperations, which is portable.

## Deviations protocol

As in 3.1 / 3.2: record every departure from this plan in SUMMARY-3.3 (what, why, evidence, which gate). Stop and ask only for an audio-moving change (including a UX1-driven `ModMatrix` fix), a contract edit, a module change, or a v2 design change beyond a defect.

## Carried (not 3.3)

- Stage 4: Env 1e30 pin (predicate at the env input). Harness items: C7 machine-speed calibration, event-aware `renderPool` (Z6a). A VST3-host DAW pass if COMPAT-03 calls for it.
- W2o: widen seeds, never the threshold.
- `uiSnap` not reset in `prepare` (3.2 note, visual only).

## Success criteria (gate for verify)

- [ ] Route edits from every page write sequence are click-free (UX1 + NCs), D36 answered
- [ ] Out / in meters correct, in-place safe, max-since-read, never stale on reopen (MT1–MT5)
- [ ] Onset pills fire once per event and go dark within 200 ms, including with the host idle (ON1, ON2)
- [ ] LFO dot on the analytic phase, offset-free, Free and Tempo (LP1, LP2)
- [ ] Loop gain = effective k × AGC, "Open" exactly at k 0 / frozen, ≤ −0.4455 dB (LG1–LG3)
- [ ] 0 allocations from block 0 (M9); audio unchanged (carried diff = TL32 → TL33 + new rows; N-suites + R12 + HA + block null)
- [ ] All UI gates green including `streams-all-enabled`; page unchanged
- [ ] HANDS-ON-3.3.md and DAW-3.3.md ready for verify (UI-03 completes there; Stage 3 closes there)
