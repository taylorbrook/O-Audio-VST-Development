---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.4 Execution Plan (structural crossfades and bypass)"
created: 2026-11-14
juce_version: "8.0.15"
phase: "2.4"
summary: "Six waves, serial: (W0) pre-flight and baselines; (W1) the fade table, the bypass fade and the two-slot band as std-only code in Source/dsp, with their double-precision rows in the designer harness; (W2) the processor on the two-slot bands with one soft-bypass path, version 0.4.0; (W3) render-harness rows (steps, transparency timing, live slopes, changed-while-bypassed, stress, allocation, the rows carried from verify 2.3) and the named amendments of six existing rows; (W4) build, both harnesses, controls, the long G5 run, install, pluginval 10, auval; (W5) summary, STATUS, one path-scoped commit. No output stage, no auto gain, no listen path, no pre-roll. No tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 2 / Phase 2.4 Execution Plan (structural crossfades and bypass)

## Goal

Type, slope, placement, enable, band bypass and global bypass change without a click. A structural change designs the band's idle slot from the full target, starts it from rest and crossfades to it over 20 ms with raised-cosine weights; a request that arrives during a fade is latched. A gain-bearing band that has sat at exactly 0 dB for 50 ms leaves the active list, so "all bands at 0 dB" and "gain scale 0 %" are bit-transparent. Global bypass is a 20 ms crossfade to the dry input through one path for the `bypass` parameter and `processBlockBypassed`, after which the engine is idle; un-bypass starts every band on its current parameters. Version 0.4.0 is built, installed and validated.

Proved at two levels, as in 2.2 and 2.3: the engine in double by the designer harness, the plugin's float output by the render harness.

**In scope:** `eq::FadeTable`, `eq::BypassFade`, `eq::SlotBand` (two `eq::Band` slots, fade, latch, sleep, wake); the processor's request path, lane rule, bypass path and both entry points. Gates G3 (step rows, as amended), G2 (live part), G4 (d), (e), (f), G5 with structure, G6 with fades, block identity with structural events, "changed while bypassed is in effect after un-bypass". The rows carried from VERIFICATION-2.3 Issues 1 – 5.

**Out of scope (and what 2.4 does instead):**

| Later phase | 2.4 behaviour |
|---|---|
| 2.5 output stage | `output_gain` and `auto_gain` are read by nothing. The bypass fade is the last stage before the conversion to float; 2.5 inserts its stages in front of it. |
| 2.6 | No taps, no display model, no listen crossfade. G6 "with the analyzer on" re-runs there. |
| Not planned | A pre-roll of the incoming slot (U2). A control ramp with smooth ends (VERIFICATION-2.3 Issue 6). A fade law other than the raised cosine. |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/RESEARCH-2.4.md` | All of it. Q9 is the recipe; Q10 the row list; "Pitfalls carried". |
| `stages/2-dsp/research-2.4/fade_bench.cpp` | Working prototype. `FBand` / `FEngine` (lines 98 – 467: `settle`, `endFade`, `setInput`, `tick`, `runBand`, `applyBypass`, `enterIdle`, `process`) are the logic to port; `stepRows`, `readRow`, `warmReference`, `localKappa`, `maxD2`, `stimulus` and the modes `g3`, `sleep`, `g4`, `mix`, `blk`, `g5`, `cpu`, `ident` are the measurements. |
| `stages/2-dsp/VERIFICATION-2.3.md` | "Issues Found" 1 – 5, 7: the rows this plan carries. |
| `stages/2-dsp/SUMMARY-2.3.md` | Deviations 2, 4, 5 (the `Band` API as built; the edit-during-bypass path this phase replaces; duplicate row IDs). |
| `stages/2-dsp/CONTEXT.md` | "Constraints carried". Binding. |
| `research/ARCHITECTURE.md` | "Structural changes: two-slot crossfade"; "Parameter Interactions"; "Processing Chain" routing notes; gates G2, G3, G4, G5 (as amended by this plan phase). |
| `Source/dsp/Band.h`, `Router.h`, `ControlRamp.h`, `BandParams.h` | The verified 2.2 / 2.3 engine. **Not edited in this phase** (P1). |
| `Source/PluginProcessor.h/.cpp` | `processBlock` as 2.3 left it. |
| `tests/designer-harness/main.cpp`, `tests/render-harness/main.cpp` | Row and tally conventions; the 2.3 driver, `refRender` / `hardRender`, the threaded G3 rows, `render` with its per-block callback, A1 / A2 (allocation window), the G2s measure. |
| Memory `index_dsp_gate_design.md`, `index_testing_render_harness_patterns.md`, `pattern_click_kappa_masked_by_level_change.md`, `pattern_run_flag_past_dirty_check_latches.md`, `pattern_gate_stimulus_below_threshold_is_vacuous.md`, `pattern_engine_state_synced_only_in_processblock_lost_when_bypassed.md`, `pattern_gate_inherits_the_review_findings_scope.md` | Suite gate recipes this phase leans on. |

---

## Plan-phase decisions

User calls taken in this phase (2026-11-14), contracts amended and checksums refreshed:

- **U1 G3 step rows** as research C1. (a) Both tones: the largest second difference of the render ≤ **2.0x** that of the louder of the two held renders. (b) 1 kHz tone: local κ ≤ **2.5x** the larger held reference; at 50 Hz the κ reading is printed and not gated. (c) Hard control: with the fades off, reading (a) ≥ 10x at block 512, or the row is void; void rows are named in P13. Plus the independent from-rest fade reference (P12).
- **U2 No pre-roll** as research C2. The incoming slot starts from rest at the fade's first sample. The fade table takes a pre-roll length that is 0 in v1. Verify gets a hands-on item: a 48 dB/oct or steeper Low Cut switched in under sustained bass.

The plan's calls:

- **P1 Files.** Two new std-only headers. `Band.h`, `Router.h`, `SectionEngine.h`, `ControlRamp.h`, `BandParams.h`, `Designer.h` are not edited; if the executor finds an edit unavoidable, it is a deviation recorded with its reason, and every 2.2 / 2.3 row that touches the edited file must print as in the baseline. Same rules as before (C++17, `noexcept`, no allocation, no framework, the gate tokens absent even from comments).

  | File | Content |
  |---|---|
  | `Source/dsp/Fade.h` (new) | Constants `fadeSeconds = 0.020`, `fadePrerollSeconds = 0.0`, `sleepSeconds = 0.050`. `fadeLengthSamples (fs)`, `sleepLengthTicks (fs, K)`. `eq::FadeTable`. `eq::BypassFade`. |
  | `Source/dsp/SlotBand.h` (new) | `eq::SlotBand`: two `eq::Band` slots, each with a placement and an off flag; the fade position; the realised input and the latest request; the sleep counter. |

  ARCHITECTURE names the two-slot band `eq::Band`. In the code `eq::Band` stays the one-slot runtime verified in 2.2 / 2.3 and the two-slot band is `eq::SlotBand` (research C3). The architecture text is amended to say so.

- **P2 `FadeTable`, exactly (C4).**
  - `prepare (fs)`: `N = round (fadeSeconds · fs)`, at least 1 (441, 640, 882, 960, 1764, 1920, 3528, 3840 at the eight rates); `pre = round (fadePrerollSeconds · fs)` = 0. Storage is a fixed `std::array` sized for 384 kHz (7680 + pre-roll room); above that rate `N` is clamped to the capacity and the fade is shorter than 20 ms. No allocation.
  - `weight (n)`, `n = 0 … length − 1`, `length = pre + N`: 0 for `n < pre`, else `0.5 − 0.5 cos (π (n − pre + 1) / (N + 1))`. No sample of the fade proper has weight exactly 0 or 1; the sample after the last one is the incoming signal alone.
  - Every mix in this phase is `a + w (b − a)` with `a` the outgoing and `b` the incoming signal. Never `(1 − w) a + w b`.
  - One table, owned by the processor (and by the harness driver), read by every band and by the bypass fade.
- **P3 `SlotBand` state.** `Band slot[2]`; `Placement place[2]`; `bool off[2]` (both true after prepare); `cur` (the slot that runs when no fade is in flight; during a fade, the outgoing one); `fading`, `pos`; `BandInput in` (what the incoming slot, or the current one, realises); `BandInput desired` (the latest request); `asleep`; `zeroTicks`. Counters for the harness: fades started, latched requests started, sleeps, wakes. "Off" is a flag: an off slot is never designed at unity and never processed.
- **P4 `SlotBand` API and rules, exactly (Q9; port of the bench's `FEngine`, without its `Opt` switches).**
  - `prepare (K, rampTicks)`: both slots `setControlGrid` + `reset`, both off, no fade, `in` and `desired` inactive.
  - `startSettled (input, designer)`: the band from rest on `input` with no fade and no ramp. Inactive → both off. Active gain type at exactly 0 dB → both off, `asleep`. Otherwise slot `cur` gets `snapTo (settings)` + `reset()`, its placement, off = false. A fade in flight is dropped first (both slots `reset` + `snapToTarget`, off). `in = desired = input`. Used for the first block after a prepare and for un-bypass (P8).
  - `request (input, designer)`: `desired = input`. If a fade is in flight: a change that is continuous against `in`, with `input` active, the band not asleep and the fade not a sleep fade, goes to the **incoming** slot as `setTarget` and updates `in`; anything else waits (latched; only `desired` is kept). If no fade is in flight: `settle (input)`.
  - `settle (next)` with `classifyBandChange (in, next)`:
    - identical: nothing.
    - continuous, `next` inactive: `in = next`.
    - continuous, asleep, gain still exactly 0: `in = next`.
    - continuous, asleep, gain no longer 0 (**wake**): the idle slot is configured from `next.settings` **with the gain set to 0 dB**, state at rest, placement set, then `setTarget (next.settings)`; fade from the off slot starts; `asleep = false`.
    - continuous otherwise: `slot[cur].setTarget (next.settings)`.
    - structural, target off (inactive, or an active gain type at exactly 0 dB) and the current slot already off: `in = next`, `asleep` as the target says, no fade.
    - structural otherwise: the idle slot is marked off (target off) or configured from the full target (`snapTo` + `reset`, placement, off = false); the fade starts; `in = next`; `asleep` as the target says; `zeroTicks = 0`.
  - `controlTick (designer, sleepTicks)`: `controlTick` on each slot that is not off (both during a fade). Sleep test, only when no fade is in flight: the current slot not off, `in` active, gain type, `in.settings.gainDbEff` exactly 0, `! slot[cur].isMoving()`, and the slot's `dynOffsetDb` exactly 0. `sleepTicks = round (sleepSeconds · fs / K)` consecutive ticks (69, 100, 138, 150, 138, 150, 138, 150 at the eight rates) → `asleep = true`, the idle slot marked off, the fade starts. Any other tick zeroes the count. The test is on the target and on the ramp having arrived, never on the ramped value, and on the gain only (CONTEXT: the skewed frequency and Q defaults are inexact).
  - `process (left, right, n, stereo, table, designer)`: no fade → the current slot through `Router::process` with its placement, or nothing when it is off. Fade → in pieces of at most 64 samples that end at the fade's end: two stack copies of the band input per lane (`double [2][64]`), each non-off slot run through the Router on its copy with its own placement (an off slot's copy stays the dry input), output `a + w (b − a)` per lane with `w = table.weight (pos + i)`; `pos` advances. At `pos == length`: `endFade`, and the rest of `n` continues in the same call.
  - `endFade`: the retired slot `reset()` + `snapToTarget`, marked off; `cur` flips; `fading = false`. Then, if `classifyBandChange (in, desired)` is not identical: `settle (desired)` (the latched request, compared with what the band realises **now**, so A → B → A inside one fade starts nothing).
  - `goIdle (designer)`: a fade in flight ends on its target (the retired slot cleared and off, `cur` flipped, the latch dropped); the current slot, if not off, `reset` + `snapToTarget` and off; `in.active = false`, `asleep = false`, `zeroTicks = 0`. `desired` is kept.
  - `clearState (designer)`: both slots `reset` + `snapToTarget`. Off flags, the fade and its position are untouched (C9).
  - `isRunning()`: `fading || ! off[cur]`. `touchesLane (lane, stereo)`: P7. `isFading()`, `isAsleep()`, the counters, and const access to the slots for the harness.
  - `slot[i].dynOffsetDb` stays the only storage of the dynamic offset (0 in v1, written by nothing). The processor's `getBandDynOffsetDb` reads slot 0. Wiring a non-zero offset to a two-slot band is v1.x.
- **P5 `BypassFade`, exactly (C8, Q4, Q5).** States: `processed`, `toBypass`, `bypassed` (idle), `toProcessed`; a position in samples; `wanted`.
  - `start (wanted)`: the first block after a prepare. State = `bypassed` or `processed`, no fade. A session that opens bypassed opens idle.
  - `request (wanted)` at a block start: stored. In `processed` or `bypassed` a differing request starts the fade (`toBypass` / `toProcessed`, position 0). During a fade it is latched: **the fade never reverses**.
  - Mix per sample while fading: `toBypass`: `out = proc + w (dry − proc)`; `toProcessed`: `out = dry + w (proc − dry)`; `w = table.weight (position)`, the same table as the bands.
  - End of `toBypass`: state `bypassed`. End of either fade: if `wanted` differs from the state reached, the opposite fade starts at the next sample.
  - `jump (wanted)`: state set without a fade (P8.4).
- **P6 One entry.** `processBlock` and `processBlockBypassed` both call one private function with `viaBypassed`. Bypass wanted = `bypass` parameter ≥ 0.5 **or** `viaBypassed` (JUCE's VST3 and AU wrappers never call `processBlockBypassed` for this plugin, RESEARCH Q5; `AudioProcessorGraph` and the harness do).
- **P7 Lanes converted for a host block.** The union, over the bands on the block's list, of the placements of every non-off slot and, where a request is latched, of `desired` when it is active and not a gain type at exactly 0 dB. A latched placement change or a mid-block un-bypass can then never reach a lane that was not converted. A lane converted and then not processed for part of the block (its last band fell asleep, a fade ended on off, the engine went idle mid-block) is still written back for that block: exact for normal float samples, not for denormal ones under flush-to-zero (C7). G4 (a), (b) keep the denormal tail; (d), (e), (f) read after their settling time, by when no such lane is converted.
- **P8 Per host block, in this order.**
  1. Clock advanced (2.3 P7), outputs without input cleared, the unprepared / narrow-buffer guard: as now.
  2. First block after a prepare: `bypass.start (wanted)`. Otherwise `bypass.request (wanted)`.
  3. State `bypassed` (after step 2): return. The buffer is not touched, no parameter is read, the snapshots are marked stale.
  4. Parameters. `restart` = first block after a prepare, or this block's request has just started `toProcessed` from `bypassed`. With `restart`: every band's snapshot is read and the band gets `startSettled (input)`. Otherwise the 2.2 two-level compare; on a difference `band.request (input)`. `classifyBandChange` is called inside `SlotBand`, no longer by the processor. Parameters keep being read and applied during `toBypass`.
  5. The list: bands with `isRunning()`. Empty → `bypass.jump (wanted)`, return, buffer untouched (with no running band a bypass toggle is a flag).
  6. Lanes per P7 to double, in chunks of at most the prepared block size (2.2 P6). Inside a chunk, segments that end at the next tick **and at the end of a bypass fade**. At a segment that starts on a tick: `controlTick` for each listed band, in index order. If a bypass fade is in flight: a stack copy of the segment's converted lanes (`double [2][64]`) is taken before the bands run. Then each listed band's `process`, in index order. Then the bypass mix.
  7. A segment that ends `toBypass`: every band `goIdle`. If the latch then starts `toProcessed` (un-bypass arrived during the fade): every band `startSettled (desired)` before the next sample.
  8. End of each chunk: the non-finite check of 2.3 P9, now `clearState` on every band; fades in flight carry on.
  9. Write-back of the converted lanes.
  Un-bypass therefore brings every band to its **current** parameters from rest, settled, with no band fade and no ramp; the bypass fade covers the start. This replaces SUMMARY-2.3 deviation 4.
- **P9 Block-size identity (Q6).** Every position is a sample count: a fade starts at the block start that sees the request (or at the sample the previous fade ends), runs `length` samples by table index, and sleep is counted in ticks. Claimed for block sizes that divide the event spacing (1, 32, 64, 512, 2048 with events on multiples of 2048). Blocks of 7 and 4096 see an event later: covered for stability only.
- **P10 No test switch in the engine or the plugin** (2.3 P11 stands). Every control is a harness render:
  - `hardStepRender`: the Phase 2.3 rule on plain `eq::Band` + `eq::Router` (structural → `snapTo` + `reset` at the block start; inactive → not processed; bypass a hard switch). The hard control of the step rows.
  - `neverSleepRender`: the same driver with a 0 dB band left on the list. The control of G4 (d), (e).
  - `reversingBypassRender`: two held renders mixed with a bypass envelope that turns round where it stands. The control of the bypass latch row.
- **P11 Step stimulus (Q1), both harnesses.** 48 kHz, stereo. L: the tone at -6 dBFS; R: the same tone at -10.5 dBFS, 1.1 rad later. 0.5 s at A, then B / A alternating every 100 ms, 8 steps; the first 0.3 s excluded from scoring. Tones 50 Hz and 1 kHz; host blocks 32 and 512. Readings, worst of the two channels: `maxD2` (largest `|y[n] − 2 y[n−1] + y[n−2]|`) and local κ (5 ms frames, frames below -80 dBFS skipped), both as in the bench. Row list = the bench's `stepRows`, 242 rows per tone:

  | Group | Rows | Content |
  |---|---|---|
  | Type pairs | 50 | 28 pairs of the eight `deep` settings at +18 dB; 22 pairs with at least one gain type at -18 dB |
  | Slope pairs | 140 | Low Cut, High Cut, Low Shelf, High Shelf, Tilt: every pair of the 8 slopes |
  | Placement pairs | 20 | Bell +18 dB and Low Cut 96 dB/oct: every pair of the 5 placements |
  | Enable | 15 | the bench's 15 subjects, off ↔ on |
  | Global bypass | 16 | the 15 subjects, and 24 × Bell +1 dB Q 4 |
  | Enable, 24 bells | 1 | 24 × Bell +1 dB Q 4, off ↔ on |

  242 rows × 2 block sizes = 484 readings per tone. The render harness adds **band bypass**: the 15 subjects through `bNN_bypass` (30 readings per tone); in the double engine band bypass and enable are the same `active` flag and are not run twice.
- **P12 The fade reference lives in the harness (C11).** `fadeRefRender`: two static chains by `Designer::design` → `toRuntime` → `svfTick` / `onePoleTick` and a placement algebra re-stated in harness code; the outgoing chain running from sample 0, the incoming one started from rest at the fade's first sample; mixed with weights computed from the formula of P2, re-stated. It calls no `SlotBand`, `FadeTable`, `Band` or `Router`. Scored sample by sample, not by κ: an engine that does what is specified equals it to rounding.
- **P13 Step verdicts and void rows.**
  - (a) `maxD2 (engine) ≤ 2.0 × max (maxD2 (held A), maxD2 (held B))`, all 484 readings, both tones. Bench worst 1.53x (50 Hz), 1.02x (1 kHz).
  - (b) 1 kHz: `κ (engine) ≤ 2.5 × max (κ (held A), κ (held B))`, all 484. Bench worst 2.20x. 50 Hz: printed (worst and count above 2.0x), not gated.
  - (c) `maxD2 (hardStepRender) ≥ 10 × ` the same held figure at block 512, per row and tone. **Void at 1 kHz, printed as void and not counted:** the subject "Bell 10 Hz Q 40 +12" in the Enable and Global bypass groups (and in the render harness's band-bypass group): it does nothing to a 1 kHz tone (bench hard 1.00x). Any other reading below 10x fails the row. Bench lowest non-void: 274x at 50 Hz, 17.9x at 1 kHz.
- **P14 G5 draw (C10).** RM-g5 / S5's generator, on the two-slot engine, plus: per band, type / slope / placement / enable / band bypass redrawn with probability 1/50 per block; global bypass toggled with probability 1/200 per block. A second, short run redraws the structure at every block. 24 bands, stereo, 8 rates, block sizes cycling as in 2.3. **60 s per rate by default, `--long` 600 s; the every-block run 10 s per rate in both.** Peak clause as amended in 2.3, with the stack taking per band the larger peak of its two latest requests. Afterwards all bands off → `memcmp` equal to the input within 100 ms (bench worst 40 ms).
- **P15 No audio golden** (2.2 P13 stands). Bit-identity rows compare renders of one binary.
- **P16 Negative controls live inside the harness** as rows `…n`; the designer harness gains `--neg fade-alloc` (a planted allocation inside the XF-alloc window).
- **P17 CPU is INFO.** G10 binds in Stage 4.
- **P18 Existing rows.** Baselines: designer harness **88 PASS / 0 FAIL / 14 INFO**, render harness **83 PASS / 0 FAIL**.
  - **Designer harness: no existing row is edited and all 88 print as in the baseline.** They drive `eq::Band` + `eq::Router`, which this phase does not touch.
  - **Render harness: these existing rows assert the 2.3 hard behaviour and are amended, by name, to the stated form. Nothing else in them changes.**

    | Row | 2.3 assertion | 2.4 form |
    |---|---|---|
    | H1 (ii) | bypassed blocks equal the input from the toggle; the output after un-bypass equals a fresh instance from that sample | equal to the input from the toggle + N samples; from un-bypass + N equal to a fresh instance started at the un-bypass sample, bit for bit; both routes |
    | M3 | band bypass on: input from that block; off: fresh instance from that block | on: input from the toggle + N; off: from the toggle + N equal to a fresh instance started at the toggle |
    | U2 | band disabled for one block, then enabled: equal to a fresh instance from the re-enable | disabled for at least N samples; from the re-enable + N equal to a fresh instance started at the re-enable |
    | A1 | the block after the global bypass toggle equals the input | checked on the first block after the fade has ended; the blocks of the fade stay inside the allocation window |
    | M6n | a type change on the ringing bell reads ≥ 100x | retired: a type change is now a crossfade. Replaced by XR-rest and by the hard control XR-g3h |
    | S5 | the 2.3 draw | the P14 draw; verdict text unchanged |

    N = `fadeLengthSamples (fs)`. H1 (i), P1 and TK (a), (c) open bypassed and stay as they are (P5 `start`). M2's 0 dB bell now sleeps and reads exactly 0 dB. Any other existing row whose verdict or figures change → STOP with the row and both readings; the executor lists in the summary every existing row that edits `enabled`, `bypass`, `type`, `slope` or `placement` mid-render and what it read.

---

## Tasks

Roles as in `PLAN-2.2.md`: **[agent]** = dsp-agent writes files (no Bash); **[run]** = the execute session. **BG** = `run_in_background`, log to `$S/<name>.log`; `$S` = the session scratchpad. Paths are relative to `plugins/O-ParametricEQ/` unless they start with `build/` or `scripts/`. `$H` = the designer harness binary, `$R` = the render harness binary.

### Wave 0 — pre-flight

**T1. Pre-flight and baselines** [run]
- Done when (any mismatch → STOP):
  - `shasum -a 256` of `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` equal STATUS `contract_checksums`.
  - `git branch --show-current` = `main`; `git status --short plugins/O-ParametricEQ` is empty. Save the full `git status --short` to `$S/git-status-T1.txt`.
  - (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness` → rc 0. `$H > $S/designer-base.log` → **88 PASS, 0 FAIL, 14 INFO**. `$R > $S/render-base.log` → **83 PASS, 0 FAIL**. `python3 tests/golden/check_spec.py` → `OK:`.

### Wave 1 — the two-slot engine, in double

**T2. `Source/dsp/Fade.h`** [agent]
- P1, P2, P5. File header in the style of `ControlRamp.h`, stating the weight formula, the mix form and why the table holds no 0 or 1.
- Depends on: T1.

**T3. `Source/dsp/SlotBand.h`** [agent]
- P1, P3, P4. Port from `fade_bench.cpp` `FBand` / `FEngine`; the `Opt` branches are dropped (fades, sleep, sleep fade and wake fade always on), `std::vector` becomes fixed storage. `Band.h` and `Router.h` are included, not edited.
- The header comment states the request table of P4, the sleep test, and the three ways a slot's state is cleared (configure, retire, `clearState`).
- Depends on: T2.

**T4. `tests/designer-harness/main.cpp`: crossfade rows** [agent]
- A new heading "2.4 / structural crossfades and bypass (double)". A harness driver `XfEngine`: 24 `SlotBand`, one `FadeTable`, one `BypassFade`, the tick clock and the P8 loop on double lanes. `fadeRefRender` (P12), `hardStepRender`, `neverSleepRender`, `reversingBypassRender` (P10). Heavy rows threaded per reading as RM-g3 / NW2 are.

  | Row | Check | Threshold |
  |---|---|---|
  | XF-g3 | P11, P13 (a): 484 readings per tone | each ≤ 2.0x |
  | XF-g3k | P13 (b): 484 readings at 1 kHz; INFO line for 50 Hz (worst, count above 2.0x) | each ≤ 2.5x |
  | XF-g3h | P13 (c) at block 512: 242 readings per tone | every non-void reading ≥ 10x; the void readings printed as void |
  | XF-ref | Engine against `fadeRefRender` over the first fade of 30 rows (6 type pairs, 12 slope pairs incl. 6 and 18 dB/oct, 6 placement pairs, 3 enables, 3 disables), noise input, 48 kHz and 96 kHz | largest `|engine − reference|` ≤ 1e-9 of the input peak, from the render start to 50 ms after the fade |
  | XF-refn | The same compare with the reference's incoming chain started 1 sample late, and with linear weights | each must exceed 1e-6 |
  | XF-rest | VERIFICATION-2.3 Issue 4. The RM-keep stimulus (bell 40 Hz, Q 20, +24 dB, ringing); type → Low Shelf during the ring. From the fade's last sample + 1: output against a fresh `eq::Band` of the new settings started from rest at the fade's first sample, fed the same input | bit-identical; and during the fade the output differs from both |
  | XF-lat | Latch (Q4): (i) type toggled at every block for 0.7 s; (ii) type change 5 ms into a type fade; (iii) A → B → A inside one fade; (iv) type + frequency + gain in one block; (v) enable + gain 0 → +18 dB in one block. Both tones, blocks 32 and 512 | `maxD2` ≤ 2.0x held for all; (i) 34 to 38 fades and a latched count > 0; (ii) exactly 2 fades, the second starting on the sample the first ends; (iii) exactly 1 fade; (iv), (v) exactly 1 fade and 0 ramp ticks on the incoming slot |
  | XF-cont | A gain edit 5 ms into a type fade reaches the incoming slot: 100 ms after the fade the sections equal the static design of (new type, new gain), field by field; the outgoing slot's target was not changed | exact |
  | XF-lane | Left ↔ Right placement fade, noise on both lanes: outside the fade each lane the band does not touch is `memcmp` equal to its input; inside the fade both lanes differ from the input; Stereo ↔ Mid with L = R: the side component stays within -250 dB | as stated |
  | XF-ident | Q3: 20 000 random gain-type settings at exactly 0 dB per rate, five rates | every SVF half `(mL, mB, mH) == (1, k, 1)` and one-pole half `(cL, cH) == (1, 1)`, exactly |
  | XF-g4 | G4 (d), (e) timing in double, five rates × blocks 1, 16, 64, 512, 1024, 4096, 24 bands, noise: (d0) 24 bells enabled at exactly 0 dB; (d1) 24 bells 6 dB → 0; (e) 24 gain-bearing bands at ±12 dB, gain scale → 0 %; (f) bypass on, 24 bands at ±12 dB | output `memcmp` equal to input from: (d0) the first sample; (d1), (e) ≤ 100 ms (bench 86 – 89 ms); (f) exactly N samples after the block start that sees it; and every band off the list by then |
  | XF-g4n | `neverSleepRender` for (d1) and (e) at 200 ms | must differ from the input |
  | XF-slp | Sleep and wake click reading (Q3): gain 0 ↔ ±18 dB every 150 ms, the bench's seven shapes, both tones, blocks 32 and 512 (56 readings); κ against the same motion on plain `eq::Band` (never sleeps) | each ≤ 3.0x (bench worst 2.10x); sleeps and wakes counted > 0 in every render |
  | XF-slpc | A ramp from +6 dB through 0 to -6 dB; a band at 0.0001 dB for 1 s; a Low Cut for 1 s | 0 sleeps in each |
  | XF-wake | A sleeping bell, gain 0 → +12 dB: the first fade sample is on the block that sees it; the slot's design at that block is the 0 dB design; the gain arrives after `rampLengthTicks` ticks on the static design | exact |
  | XF-byp | Bypass latch (Q4): un-bypass 5 ms into the bypass fade; bypass on for one block; bypass 5 ms into a type fade. Both tones, blocks 32 and 512 | `maxD2` ≤ 2.0x held; the first case reads the full fade to idle, then the full fade back (2 × N samples of mixing) |
  | XF-bypn | `reversingBypassRender` on the first case, 50 Hz | ≥ 10x held (bench 48.5x) |
  | XF-idle | Fully bypassed: every slot's state is zero, no band running, no fade in flight; a parameter request made while idle changes no slot; after un-bypass the output from the fade's end equals a fresh engine on the latest requests started at the un-bypass sample | bit-identical |
  | XF-g2 | G2 live: Low Cut and High Cut, fc 100 Hz, 500 Hz, 2 kHz, 48 kHz, every ordered slope pair, changed on a running band. After the fade: sections equal the static design of the new slope field by field; an impulse sent 100 ms later measured with the G2s code | corner -3.01 ± 0.10 dB; stop-band slope within ±1.0 dB/oct of nominal |
  | XF-blk | Q6: 6 bands, stereo noise, 160 events on multiples of 2048, each band redrawing type and slope (p 0.5), placement (0.3), enable (0.3), frequency, gain (exactly 0 with p 0.25), Q; bypass toggled (0.1). Blocks 1, 32, 64, 512, 2048 at 44.1, 48, 96, 192 kHz | bit-identical per rate; fades, latched requests and wakes each > 0 |
  | XF-blkn | The same render without the events | must differ |
  | XF-ms | VERIFICATION-2.3 Issue 1. A three-parameter move on a Mid band with L = R and on a Side band with L = −R | equal to the Stereo band's render of the same move within 1e-12 of the peak; differs from the settled render |
  | XF-1p | Issue 2. G3 sweep motion 5 of PLAN-2.3 on a 6 dB/oct and an 18 dB/oct High Cut, against `refRender`; and `holdRender` with `Gc` held | ≤ 4.0x; the control ≥ 10x, or it is reported as not firing and the row fails |
  | XF-g5 | P14 at 8 rates, default 60 s / `--long` 600 s, and the every-block run | every sample finite; peak clause; then all bands off → `memcmp` equal within 100 ms; fades and latched requests per second printed |
  | XF-nan | One NaN input sample during a fade, then noise, with `clearState` at the end of the chunk | finite from the next chunk; the fade ends on schedule (same end sample as the render without the NaN) |
  | XF-alloc | 1e4 blocks with a structural request on every band whenever its fade ends, bypass toggled every 50 blocks, inside the `malloc_logger` window | 0 allocations, 0 frees; SKIPPED line without a logger |
  | XF-cpu | INFO. Q8's four cases: settled, always fading, one fade per second, 24 asleep | printed beside the bench's table |

- `--neg fade-alloc`: a planted allocation inside the XF-alloc window is counted; exit 0 only if it is.
- No existing row is edited (P18).
- Depends on: T3.

### Wave 2 — the processor

**T5. `Source/PluginProcessor.h/.cpp`: two-slot bands and the soft bypass** [agent]
- P6, P7, P8. `bands` becomes `std::array<eq::SlotBand, 24>`; members added: one `eq::FadeTable`, one `eq::BypassFade`, the sleep length in ticks, a `started` flag cleared in `prepareToPlay`. `BandSlot` keeps the snapshot and its validity; the realised input lives in the band.
- `prepareToPlay`: table and sleep length from the rate; every band `prepare`; clock 0; snapshots invalid; `started = false`.
- `clearAndSnapAllBands` is replaced by the two calls P8 names (`goIdle`, `clearState`).
- `getBandDynOffsetDb` reads slot 0 (P4). No test switch.
- Still no lock, no allocation, no listener, no `AsyncUpdater`, no `juce::SmoothedValue`, none of `juce::dsp::IIR`, `StateVariableTPTFilter`, `juce::dsp::Gain`.
- The file comments (the per-block list above `processBlock`, the note on `processBlockBypassed` in the header) are rewritten for 2.4.
- Depends on: T3.

**T6. Version 0.4.0** [agent]
- `CMakeLists.txt`: `OPEQ_VERSION 0.4.0`. Nothing else in the file changes.
- `CHANGELOG.md`: `## [0.4.0] - <execute date>` (bracket form). Changed: type, slope, placement, enable and band bypass changes crossfade over 20 ms and no longer click; global bypass crossfades to the dry signal and the engine rests while bypassed; a band at exactly 0 dB costs nothing and leaves the signal bit-identical. Fixed: an edit made while bypassed is in effect, settled, at un-bypass. Known in this build: output gain and auto gain do nothing yet; a steep low cut switched in under sustained bass dips for a few milliseconds.
- Depends on: T5.

### Wave 3 — render-harness rows

**T7. `tests/render-harness/main.cpp`** [agent]
- New section headings for 2.4; the row index in the file header extended. `fadeRefRender` and `hardStepRender` as P10 / P12, fed the plugin's own float input, widened, compared in double. The six amendments of P18, each with a comment naming this plan.

  | Row | Check | Threshold |
  |---|---|---|
  | XR-g3 | P11 through `processBlock`: the 242 rows + band bypass (15), both tones, blocks 32 and 512; global bypass through the `bypass` parameter | P13 (a): each ≤ 2.0x |
  | XR-g3k | P13 (b) on the same renders | 1 kHz each ≤ 2.5x; 50 Hz printed |
  | XR-g3h | `hardStepRender` at block 512 | as XF-g3h, the band-bypass void reading included |
  | XR-g3b | The Global bypass group through `processBlockBypassed` (the bypassed blocks arrive by that entry) | P13 (a), (b) |
  | XR-ref | XF-ref's 30 rows through the plugin, float output against `fadeRefRender` | ≤ -120 dB re the output peak |
  | XR-rest | XF-rest through the plugin: after a type change on the ringing bell, from the fade's end the output equals a fresh instance on the new parameters prepared and started at the fade's first sample | `memcmp` equal |
  | XR-g4d | G4 (d): 24 bells enabled at 0 dB: equal from the first block; 24 bells 6 dB → 0, read 200 ms after the edit. Blocks 1, 16, 64, 512, 1024, 4096 and a random sequence; mono and stereo | `memcmp` equal to the input |
  | XR-g4e | G4 (e): 24 gain-bearing bands at ±12 dB (bells, shelves, tilt), `gain_scale` → 0 %, 200 ms later; same block sizes | `memcmp` equal |
  | XR-g4f | G4 (f): 24 bands at ±12 dB, bypass on, 100 ms later; by the parameter and by `processBlockBypassed`; same block sizes | `memcmp` equal; and equal from exactly N samples after the block that sees it |
  | XR-g4n | (d), (e) with one band at 0.001 dB instead of 0; (f) read 5 ms after the toggle | each must differ |
  | XR-g2 | G2 live: Low Cut and High Cut at 500 Hz, each of the 8 slopes reached from its neighbour on a running band; an impulse 100 ms after the fade | corner -3.01 ± 0.10 dB; slope ±1.0 dB/oct where the float response reaches -50 dB, else against the prototype's as G2s-short does |
  | XR-byp | Changed while bypassed. Six mixed bands, noise. Bypass on; after the fade: type, slope, placement, frequency, gain, Q and enable edited across the bands; un-bypass. Variants: by the parameter; by `processBlockBypassed`; a continuous edit only (SUMMARY-2.3 deviation 4); bypass engaged in the middle of a gain ramp (VERIFICATION-2.3 Issue 3) | from un-bypass + N: `memcmp` equal to a fresh instance on the edited parameters started at the un-bypass sample |
  | XR-bypn | The same compare against a fresh instance on the parameters from before the bypass | must differ |
  | XR-big | Issue 3. Host blocks of 4096 on a prepared size of 512, with moves and structural events | bit-identical to the same script on a prepared size of 4096 |
  | XR-prep | Issue 3. Re-prepare at the same rate after 1 s; then two moves and one type change in blocks of 500 | `memcmp` equal to a fresh instance |
  | XR-blk | XF-blk's script through the plugin, plus `gain_scale` and bypass events; blocks 32, 64, 512, 2048 at 44.1, 48, 96, 192 kHz | bit-identical per rate; differs from the render without events |
  | S5 | Amended (P18): the P14 draw, seeded, 8 rates; plus the every-block run | stack guard; every sample finite; peak printed; then all bands off → `memcmp` equal within 100 ms |
  | A3 | G6 with fades: the `malloc_logger` window around `processBlock` and `processBlockBypassed`, 20 s of the P14 every-block draw at 48 kHz, first block included | 0 allocations, 0 frees; liveness probe |
  | A3n | A planted allocation inside the window | counted |
  | X-cpu3 | INFO. Q8's four cases through `processBlock` | printed beside the bench's table and 2.3's X-cpu2 |

- Depends on: T5.

### Wave 4 — build, gates, install, validators

**T8. Build and run** [run]
- Done when, in order:
  1. (BG) `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` (version bump), then `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness > $S/build.log 2>&1` → rc 0; `grep "O-ParametricEQ/.*warning:" $S/build.log` → no lines.
  2. (BG) `$H > $S/designer.log 2>&1` → **0 FAIL**. The 88 / 14 lines of `$S/designer-base.log` print the same figures (P18); every T4 row ID appears; XF-g3k's 50 Hz INFO figures are recorded.
  3. `$H --neg fade-alloc` → rc 0. The nine existing `--neg` controls → rc 0 each.
  4. (BG) `$R > $S/render.log 2>&1` → **0 FAIL**. Against `$S/render-base.log`: 82 of the 83 rows are present (M6n retired); the rows not named in P18 print as in the baseline; every T7 row ID appears; every `…n` row reports that its control fired.
  5. (BG, one at a time) `$H --long > $S/designer-long.log 2>&1` and `$R --long > $S/render-long.log 2>&1` → 0 FAIL each; XF-g5, RM-g5 and S5 print 600 s per rate.
  6. `python3 tests/golden/check_spec.py` → `OK:`; `$R --dump-params | diff - tests/golden/params.tsv` → empty.
  7. Static gates, from the repo root:
     - `! grep -rnE "juce|Juce|std::vector|std::function|new |malloc|throw " plugins/O-ParametricEQ/Source/dsp`
     - `! grep -rnE "fast-math|Ofast|fp:fast|ffp-contract" plugins/O-ParametricEQ/CMakeLists.txt`
     - `! grep -rnE "IIR::|StateVariableTPT|dsp::Gain|SmoothedValue|AsyncUpdater|callAsync|CriticalSection|std::mutex|addParameterListener" plugins/O-ParametricEQ/Source/PluginProcessor.h plugins/O-ParametricEQ/Source/PluginProcessor.cpp`
     - `git diff --stat HEAD -- plugins/O-ParametricEQ/Source/dsp` lists `Fade.h` and `SlotBand.h` only (P1), or the summary records the deviation.
     - `grep -rn "dynOffsetDb" plugins/O-ParametricEQ/Source` → the declaration and its reads only.
     - `grep -n "1.0 - w\|(1 - w)" plugins/O-ParametricEQ/Source/dsp/Fade.h plugins/O-ParametricEQ/Source/dsp/SlotBand.h plugins/O-ParametricEQ/Source/PluginProcessor.cpp` → no mix of the complementary form (P2).
     - Read the bodies of `fadeRefRender` and `hardStepRender` in both harnesses: none of `SlotBand`, `FadeTable`, `BypassFade` appears in them; `fadeRefRender` also uses no `Band` and no `Router` (P12).
- **On a failure:**
  - An XF row red → the engine. Fix in `Source/dsp/Fade.h` / `SlotBand.h`, rebuild, rerun 2 – 7 in full.
  - XF rows green and a render row red → the processor glue or the row's float scope. Fix the glue. A threshold, row list or stimulus level is not changed in this session; if a row cannot pass as specified with the XF rows green, STOP and escalate with the figures.
  - A control that does not fire → the row is vacuous. Fix the row, not the control.
  - XF-g3h or XR-g3h below 10x on a reading not named void in P13 → STOP with the figures (the void list is contract text in G3).
  - XF-g3 or XR-g3 above 2.0x, or XF-g3k / XR-g3k above 2.5x, with XF-ref green → STOP with the figures (the engine does what is specified and the specified fade misses the contract; U1 is Taylor's).
  - An existing row changes outside P18 → STOP.
- Depends on: T4, T6, T7.

**T9. Install and validate** [run]
- One validator at a time, each BG with its own log.
  1. `./scripts/build-and-install.sh O-ParametricEQ > $S/install.log 2>&1` → rc 0; exactly the two `-dev` bundles in the system folders, version 0.4.0.
  2. pluginval, strictness 10, `--timeout-ms 600000`, VST3: **2 runs** → `SUCCESS`. AU: 1 run → `SUCCESS`.
  3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`; channel capabilities still `[1, 1] [2, 2]`. Never `auval -a`.
  4. Standalone smoke as in 2.2 T9 (rebuild the Standalone target first) → `ALIVE`.
- Any failure: fix the root cause in `Source/`, then rerun T8 and T9 in full. A pluginval NaN or `inf` report is a finding about stacked gain (2.2 R3): record the configuration, insert nothing in the signal path.
- Depends on: T8.

### Wave 5 — bookkeeping + commit

**T10. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/2-dsp/SUMMARY-2.4.md`: tallies of both harnesses (default and `--long`); XF-g3 / XR-g3 as the bench's table per group (engine `maxD2` worst, κ worst at 1 kHz, κ at 50 Hz worst and count, hard lowest, void readings), float beside double; XF-ref / XR-ref residuals; XF-g4 / XR-g4 times per rate; XF-slp range; XF-lat counts; XF-g5 / S5 per rate; A3 counts; XF-cpu / X-cpu3 beside the bench; the P18 amendments as made and the list of existing rows with structural edits and what they read; controls; validator results; deviations.
- Carry-forward in the summary: for 2.5: `output_gain`, `auto_gain`, the bypass fade stays last; for 2.6: G6 with the analyzer on, the listen crossfade; for verify: hands-on listening (a steep low cut enabled under bass, U2; whether 20 ms feels immediate on enable and bypass; several bands changing type at once, a preset-like change), the `--long` runs once more, a mutation run over `SlotBand.h` and the bypass path.
- `REQUIREMENTS.md`: no status edit (requirements close at verify).
- `PLUGINS.md`: version cell → 0.4.0. Check `git diff PLUGINS.md` first; if other sessions' rows are modified, use the temp-index recipe (memory `index_git_shared_checkout.md`) or leave it out and say so.
- STATUS: phase rows, `status: stage_2_phase_2_4_executed`, `next_action: stage_2_verify_phase_2_4`, files list.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` the new files by name (`Source/dsp/Fade.h`, `Source/dsp/SlotBand.h`, `.planning/stages/2-dsp/SUMMARY-2.4.md`), then `git commit -- plugins/O-ParametricEQ`. Never `-A` / `-a`. `git log -1 --stat` lists only this plugin's paths. No tag.
- Depends on: T9.

---

## Dependency graph

```
T1 → T2 → T3 ─┬→ T4 ───────────────┐
              └→ T5 → T6 ──┐        │
                   └→ T7 ──┴────────┴→ T8 (build, harnesses, controls, --long) → T9 (install, validators) → T10
                                        ↑   │
                                        └───┘ fix → rebuild → rerun T8 in full (and T9 if Source changed)
```

T4 and T5 / T7 touch different files and can be written in either order; one build covers all of them.

## Success criteria

| Criterion (ROADMAP Phase 2.4 as amended; ARCHITECTURE G2, G3, G4, G5) | Proven by |
|---|---|
| G3 step rows: second difference ≤ 2.0x the louder held render, both tones (DSP-04, FUNC-01, FUNC-04) | XF-g3, XR-g3, XR-g3b, XF-lat, XF-byp |
| G3 step rows: local κ ≤ 2.5x held at 1 kHz | XF-g3k, XR-g3k |
| G3 hard control fires, void rows named | XF-g3h, XR-g3h |
| The engine does the specified fade (incoming slot from rest, raised cosine, `a + w (b − a)`) | XF-ref, XF-refn, XR-ref, XF-rest, XR-rest |
| A request during a fade is latched, latest wins, nothing needless starts | XF-lat, XF-cont |
| The bypass fade is latched and never reverses | XF-byp, XF-bypn |
| G2 live: slope changes on a running band (DSP-03) | XF-g2, XR-g2 |
| G4 (d) 24 bells at 0 dB, (e) gain scale 0 %: bit-equal after 200 ms (QUAL-01, FUNC-05) | XF-ident, XF-g4, XF-g4n, XR-g4d, XR-g4e, XR-g4n |
| G4 (f) global bypass: bit-equal 100 ms after the toggle (FUNC-04) | XF-g4, XR-g4f, XR-g4n; H1 (amended) |
| Sleep keys on gain exactly 0 with the ramp arrived, and on nothing else | XF-slpc, XF-wake, XF-slp |
| A parameter changed while bypassed is in effect after un-bypass | XR-byp, XR-bypn, XF-idle |
| Fully bypassed: buffer untouched, state cleared | XF-idle, H1 (i), P1 |
| `processBlockBypassed` takes the same path | XR-g3b, XR-g4f, XR-byp, TK (c) |
| A fade leaves untouched lanes exact (FUNC-03) | XF-lane; T1b, T1c unchanged |
| G5 with structural parameters and bypass randomised (DSP-04, DSP-06) | XF-g5, S5 (amended); `--long` once |
| Non-finite input during a fade does not stick | XF-nan; S6 unchanged |
| G6 with fades (PERF-01) | XF-alloc, A3, A3n, `--neg fade-alloc`; A1 (amended) |
| Structural events on multiples of 2048 render bit-identically at 32, 64, 512, 2048 | XF-blk, XF-blkn, XR-blk |
| VERIFICATION-2.3 Issue 1 (Mid / Side ramping path) | XF-ms |
| Issue 2 (one-pole on the ramping path) | XF-1p |
| Issue 3 (bypass during a move; oversize host block; clock after re-prepare) | XR-byp, XR-big, XR-prep |
| Issue 4 (state rule of a structural change) | XF-rest, XR-rest |
| Issue 5 (G5 peak clause) | kept as amended in 2.3, larger slot per band (P14) |
| Stage 1, 2.2 and 2.3 rows, layout golden unchanged outside P18 | T8.2, T8.4, T8.6 |
| 0.4.0 validates | T9 (pluginval 10 ×3, `auval`, Standalone) |

## Risks

- **R1 The bench is double and is not the plugin.** The float step readings are unmeasured. If XR-g3 / XR-g3k miss with the XF rows green, that is a finding: STOP (T8), do not move 2.0x or 2.5x.
- **R2 XF-g3k's margin is thin** (bench 2.20x against 2.5x, and the eight readings near it are the depth of the level change, equal in a warm reference). A reading between 2.2x and 2.5x is recorded in the summary beside the bench's; above 2.5x is the STOP of T8.
- **R3 XF-slp has no firing control and cannot have one:** the 0 dB design is the identity (XF-ident), so the sleep and wake fades change nothing measurable (bench: the same readings with and without them). The row guards the wake path against a regression to "designed at the target gain from rest"; XF-wake states that rule exactly. 3.0x is the plan's figure on a bench worst of 2.10x; a miss is escalated with the figures, not tuned.
- **R4 The lane rule (P7) was not exercised by the bench,** which converts both lanes always. A latched Left → Right change with no other band on the right lane is the case; XF-lane and XR-blk (placement events under latch) cover it. The T5 review reads the union against P7 line by line.
- **R5 The first block after a prepare must not fade.** Every static row of 2.2 / 2.3 scores from the first sample; a fade-in there moves 60 rows at once. `startSettled` (P4) and `BypassFade::start` (P5) are the rule; T8.4's baseline compare is the gate.
- **R6 The bypass fade here uses the band table's form** (`(n + 1) / (N + 1)`, N mixing samples), where the bench's bypass used `p / N`. Same shape, one table, exact end. XF-byp / XF-bypn re-measure it; (f) reads "exactly N samples".
- **R7 Latched state is the suite's run-flag trap.** `asleep`, `fading` and the latch are each set and cleared in one place (P4). XF-lat (iii), XF-slpc and XF-idle are the rows; a band that stays awake at 0 dB or asleep at a non-zero gain shows in XF-g4 and XF-wake.
- **R8 Mid-block transitions.** A fade end, a latched start, idle and un-bypass can all fall inside one host block, at any segment. XF-blk at block 2048 against block 1 is the row that sees a mistake there; do not drop block size 1.
- **R9 Run time.** XF-g3 is 968 readings × 4 renders of 1.3 s; XF-g5 is 8 rates × 60 s (bench: 285 s of machine time for the 600 s run). Everything BG, heavy rows threaded. Do not cut counts.
- **R10 pluginval fuzzes 196 parameters, now with fades and sleep.** Expected to pass; a NaN / `inf` report is stacked gain unless S5 is also red.
- **R11 Gate tokens in prose.** T8.7 greps `Source/dsp` comments too: no "new ", "throw ", "malloc", "juce" in the two new headers' comments. "SmoothedValue" may not appear in the processor even in a comment.
- **R12 Shared checkout.** Other sessions have uncommitted files in the tree. Path-scope the commit and re-check immediately before it (T10).
