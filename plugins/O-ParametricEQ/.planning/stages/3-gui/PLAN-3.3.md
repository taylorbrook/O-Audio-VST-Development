---
title: "O-ParametricEQ Stage 3 (GUI) — Phase 3.3 Execution Plan (analyzer, meter, fills, listen)"
created: 2026-11-16
juce_version: "8.0.15"
phase: "3.3"
summary: "Eight waves: (W0) pre-flight and baselines, reproduce N1e 12 / 16; (W1) DSP: the listen NaN fix (flag taken unconditionally every chunk) + render rows N1e, N1e-u, AO1, M1; (W2) C++ editor: owned SpectrumAnalyzer processed every tick with measured dt, per-tap emits on change as tenths of a dB with null above Nyquist, outputLevel from readAndResetOutputPeak held over H reads (pure PeakHold.h), vu-meter removed from the plugin (C1: page-drawn meter); (W3) page: analyzer events decoded + span-centre x, a page-drawn canvas meter with dB-per-second ballistics, the listen state machine (latch, dragListen, syncListen, disable hook on the enabled relays); (W4) tests/meter-ballistics node gate; (W5) build, harnesses, five UI gates, install, pluginval 10, auval; (W6) editor bench rows: analyzer modes / idle / peak / Nyquist, meter, fills, listen, rAF intervals, audio-thread time, the four VERIFICATION-3.2 rows, plus 12 mutants; (W7) summary, STATUS, one path-scoped commit. Version 0.9.0. No tag."
stages: [3]
agents: [dsp-agent, gui-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 3 / Phase 3.3 Execution Plan (analyzer, meter, fills, listen)

## Goal

The display shows what is playing:

- **Analyzer.** Off / Pre / Post / Pre+Post from the `UI` state. Off stops the audio-thread writes. The trace sits on the right frequency (span centres) and never draws a wall above Nyquist.
- **Meter.** A page-drawn two-bar output meter, −60 to +6 dB, fixed zones, peak hold, ruler inside the box. A −6 dBFS sine reads −6 within 0.5 dB at every host block size. Ballistics in dB per second, so 60 Hz and 120 Hz displays agree.
- **Fills.** Each enabled, not-bypassed band washed in its pigment; bypassed bands unwashed. Already drawn; 3.3 gates it.
- **Listen.** Engages on the dragged band past the drag threshold, returns to the latch (or off) on release; the strip button latches; disabling or deleting the listened band from any source ends it.
- **Listen NaN fix.** A NaN inside listen's input ramp with no band running is caught at its own chunk (N1e 16 / 16). Listen-off output bit-identical.
- **60 fps.** Dragging with Pre+Post on holds 60 fps and does not raise audio-thread time.

Version 0.9.0 built, installed, validated. Stage 3 complete at verify 3.3.

**In scope:** `Source/dsp/Listen.h`, `Source/PluginProcessor.h/.cpp` (step 8 only, and the `vu-meter` include removed), `Source/PluginEditor.h/.cpp`, `Source/editor/PeakHold.h` (new), `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js` (only if a meter key is needed), `CMakeLists.txt`, `tests/render-harness/main.cpp` (four rows), `tests/meter-ballistics/` (new), `tests/i18n-states.json` (only if a state is needed), `CHANGELOG.md`, the editor bench under `stages/3-gui/execute-3.3/`, `modules/registry.yaml` (one `used_by` row), ROADMAP / CONTEXT amendments made in this plan phase.

**Out of scope:**

| Later | 3.3 behaviour |
|---|---|
| Per-band response cache | None (RESEARCH-3.3 P7: p99 11 – 12 ms without it; a frequency drag defeats it). |
| `composeSum` speed-up | None; recorded only. |
| Extracting the meter into `vu-meter` 1.1.0 | Not now. The page meter can be lifted into the module later. |
| Point-tip housekeeping (VERIFICATION-3.2 Issue 3) | Stage 4 i18n review. |
| Symmetric G9 floors | Later `/improve` (CONTEXT U8). |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/3-gui/RESEARCH-3.3.md` | All of it. Q1 – Q7, P1 – P9. |
| `stages/3-gui/research-3.3/probe/` | `probe33.cpp` (rows `n`, `a`, `m` become render rows), `listen-fix.diff` (the fix as prototyped), `build.sh`, outputs. |
| `stages/3-gui/research-3.3/draw-bench/` | README tables, the analyzer-injection and real-drag code W6 reuses. |
| `stages/3-gui/CONTEXT.md` | U2, U7 (amended below), U8, "Constraints". Binding. |
| `stages/3-gui/PLAN-3.2.md`, `SUMMARY-3.2.md`, `VERIFICATION-3.2.md` | Editor and page shape after 3.2; the bench; Issues 1, 2, 4. |
| `stages/3-gui/execute-3.2/editor-bench/` | The bench (`src/Main.cpp`, `src/BenchScript.h`, `run-all.sh`, `analyze.py`). W6 copies and extends it. |
| `stages/2-dsp/VERIFICATION-2.6.md` Issue 1, `stages/2-dsp/verify-2.6/probe.cpp` | N1e's origin. |
| `Source/dsp/Listen.h` `:415` (`goIdle`), `:776` (`renderVoice`); `Source/PluginProcessor.cpp` `:1215-1232` (step 8) | P1 edit points. |
| `Source/PluginEditor.cpp` `:28-33` (header comment), `:646` (`timerCallback`), `:743` (`/modules/vu-meter.js` branch); `Source/PluginProcessor.h` `:183-186` (vu-meter include), `:273` (`readAndResetOutputPeak`) | P2 / P4 / P5 edit points. |
| `Source/ui/public/index.html` `:24`, `:305-307`, `:562-563` (meter mount), `:661` (`ANALYZER_FLOOR_DB`), `:1242` (strip listen), `:1298` (`setListen`), `:1326` (`deleteBand`), `:1603` (`drawAnalyzerTrace`), `:1765` (`listenPulse`), `:1834` (`d.moved = true`, the P5 threshold block), `:1974-1977` (analyzer listeners, `outputLevel` placeholder) | Page edit points. |
| `mockups/v1-ui-test.html` `:692-718` (`drawMeter`) | The approved meter: geometry, zones, ruler marks, peak line. Colours become page tokens. |
| `CMakeLists.txt` `:70-111` | `vu-meter` module call, embed, symbol comment. |
| Memory: `pattern_canvas_text_needs_font_load_repaint`, `pattern_ruler_outside_the_bordered_box_drifts_linearly`, `pattern_pointerdown_preventdefault_kills_document_mousedown`, `pattern_gate_inherits_the_review_findings_scope`, `pattern_gate_stimulus_below_threshold_is_vacuous`, `pattern_source_grep_gate_hits_configure_copied_module_js`, `pattern_locked_screen_throttles_wkwebview_bench`, `pattern_executor_watchdog_stall_run_long_commands_in_background`, `index_git_shared_checkout` | Traps this phase walks through. |

---

## Plan-phase decisions

**C1 (Taylor, 2026-11-16): page-drawn meter (U7's fallback), as recommended.** `vu-meter` 1.0.0 cannot draw the approved meter (moving zone colours, no ruler, per-frame decay, a bridge that overwrites per block). Amendments made in this plan phase:

- **CONTEXT U7:** amended to the page-drawn meter fed by an `outputLevel` event; `vu-meter` dropped from the plugin.
- **ROADMAP:** Phase 3.3 component "`vu-meter` output meter" → "page-drawn output meter (`outputLevel` event)"; the risk row for `vu-meter` closed (retired by C1). STATUS roadmap checksum refreshed.

No other call. The plan's calls:

- **P1 Listen NaN fix (RESEARCH Q5).**
  - `Listen.h`: member `bool nonFiniteVoice = false;`. In `renderVoice`, after `soloRender`: if `outL[count − 1]` (and `outR[count − 1]` when stereo) is not finite, set it. `bool takeNonFiniteVoice() noexcept` returns and clears it. `goIdle()` clears it. Audio thread only; no atomic.
  - Step 8 (`PluginProcessor.cpp`), before `if (! finite)`, on its own line: `if (listen.takeNonFiniteVoice()) finite = false;`. **Never** inside the `! finite ||` expression (short-circuit leaves the flag set and clears every band again on the next clean chunk). A comment says why.
  - Nothing else in the processor changes.

- **P2 Editor: analyzer (RESEARCH Q1).**
  - `std::unique_ptr<SpectrumAnalyzer> analyzer`, built in the constructor after `displayModel`; **reset first** in the destructor (before `pageIsReady` / `stopTimer` work completes, and before anything it reads goes), so the consumer is unregistered first. One analyzer per processor; a comment says no second consumer may run while an editor is open.
  - `timerCallback`: after the `eqSections` block, `analyzer->process (dt)` **every tick, whatever the mode**. `dt` from `Time::getMillisecondCounterHiRes()` deltas, clamped to [0, 0.25] s; the first tick uses 1 / 60.
  - Emit `analyzerPre` / `analyzerPost` only when the mode includes that tap **and** its encoded array differs from the last one sent for that tap (`memcmp` on the 256 ints). A mode change that adds a tap clears that tap's last-sent copy so it emits once.
  - Encoding (one free function, `encodeAnalyzerPoints`, in `PeakHold.h`'s neighbour or the editor's anonymous namespace; harness-testable if it lives in a header): 256 ints, `round (dB × 10)`, clamped to [−1200, +600]; a span whose start `10 × 3000^(p/256)` is ≥ `fs / 2` → `var()` (JSON `null`).
  - Idle: when audio stops, the release reaches −1200 and the arrays stop changing, so emits stop (idle row).

- **P3 Page: analyzer decode and x (RESEARCH Q1, finding 8).**
  - Listeners: `a.map (v => v === null ? NaN : v / 10)`; non-array → `null` as now. Explicit `null` → `NaN` (an implicit `ay(null)` reads 0 dBFS: a wall at the right edge at 44.1 / 48 kHz).
  - `drawAnalyzerTrace`: `xs[i] = PLOT_MARGIN.l + (i + 0.5) / n × plotW`. `tracePath` already skips `NaN`.
  - Nothing else in the analyzer drawing changes.

- **P4 Editor: meter feed (RESEARCH Q2, M1).**
  - New `Source/editor/PeakHold.h` (header-only, no JUCE GUI dependency): a fixed-capacity ring (capacity 16) per channel; `setDepth (H)`; `push (gain)`; `max()`. `H = min (16, ceil (blockSize / fs × 60) + 1)` from `getBlockSize()` / `getSampleRate()`, recomputed when either changes (checked each tick; `fs ≤ 0` → H = 2).
  - Each tick: `readAndResetOutputPeak (0)` and `(1)` → `push`; emit `outputLevel` as `[tL, tR]` (mono: `[tL]`), tenths of a dB of the held max, clamped to [−600, +120]. Emit only when the pair differs from the last sent (silence costs nothing once it reaches −600).
  - No processor change.

- **P5 Page: meter (C1, U7 amended).**
  - `#meter-mount` holds one `<canvas id="meter">`, sized like the plot canvases (dpr-aware, `ResizeObserver` on the mount). Drawing ported from `drawMeter`: two bars, `top = 9`, `bot = h − 9`, `lo = −60`, `hi = 6`; zones fixed by dB (sap below −6, ochre −6 to 0, madder above 0), drawn as fixed segments clipped at the level, never a stretched gradient; peak line 1.5 px, madder above 0 dB else ink; ruler marks 6 / 0 / −6 / −12 / −24 / −36 / −48 / −60 with the mockup's skip rule, **inside the same box and canvas** as the bars. Colours from the page's CSS tokens (both themes); add tokens if the three zone colours have none.
  - Ballistics, pure function `meterStep (state, target, dt)` (lifted by name by W4's gate): attack instant; release 20 dB/s; peak holds 1 s then falls at 20 dB/s, never below the level. `dt` from rAF timestamps, clamped to [0, 0.1] s.
  - Animation: the meter has its own rAF loop that runs only while some bar or peak is above −60 or a target is above −60, and stops at rest (no idle CPU). It does not call the plot's `requestRender`.
  - Mono (`outputLevel` length 1): two equal bars.
  - Ruler text: `document.fonts.load()` then repaint (memory). Numbers only; the existing `meter` aria key stays; no new i18n key unless the executor adds visible text (then en / fr / zh-Hans per check-i18n [1]).
  - Reduced motion: the bars still move (it is a measurement); no other effect.

- **P6 Page: listen state machine (RESEARCH Q4).**
  - State `ui.latch` (0 or band), `ui.dragListen` (0 or band). `ui.listen` becomes derived: `dragListen || latch`. One function `syncListen()` recomputes it, calls `setListenBand` only when it changed, and updates the strip button and `listenPulse()`.
  - Drag threshold (the P5 block at `:1834`, where `d.moved` becomes true): `dragListen = d.b; syncListen()`. **Not** at pointerdown.
  - `onUp` (pointerup / cancel / lost capture; already idempotent): `dragListen = 0; syncListen()`.
  - Strip Listen (`:1242`): `latch = (latch === sel) ? 0 : sel; syncListen()`.
  - Disable hook: each band's `enabled` relay `valueChangedEvent` → if false: clear `latch` / `dragListen` when equal to that band; `syncListen()`. It fires for host automation and preset loads. `deleteBand` drops its own listen line and relies on the hook.
  - Bypass does not end listen. Typed entry, wheel and keyboard do not listen.
  - `setListen` is removed or becomes a thin wrapper over `latch`; grep: one call site of `setListenBand` (inside `syncListen`).

- **P7 Remove `vu-meter` from the plugin (C1).**
  - `CMakeLists.txt`: drop `ouaricon_add_module(… vu-meter)`, the `vu-meter.js` embed, and both comment lines.
  - `PluginProcessor.h :183-186`: drop the compile-proof include and its comment.
  - `PluginEditor.cpp :743`: drop the `/modules/vu-meter.js` resource branch.
  - `index.html`: comments at `:24`, `:305`, `:562` and the `outputLevel` placeholder comment rewritten for the page meter.
  - Delete the configure-copied, gitignored `Source/ui/public/modules/vu-meter.js` and reconfigure, so no stale copy feeds a gate (memory).
  - `modules/registry.yaml`: remove the `O-ParametricEQ 0.1.0` row under `vu-meter`'s `used_by` (leaves `used_by: []`). **Another session has uncommitted hunks in this file**: commit only this hunk with the temp-index recipe (`index_git_shared_checkout`), never the whole file.
  - Grep at T9: `vu-meter`, `VUMeter`, `vumeter` absent from `plugins/O-ParametricEQ/{CMakeLists.txt,Source}` (excluding `.planning/`).

- **P8 No other audio change.** Listen-off output is bit-identical: XR-lid sessions SHA-256-equal to the 0.5.0 hashes (16 / 16); designer 150 / 0 / 18; RESEARCH's no-NaN cross-build hash `6274ff9b5b0e488a` reproduced by the probe on the edited tree. `git diff HEAD -- Source/dsp Source/analysis Source/PluginProcessor.cpp` touches only `Listen.h` and step 8.

- **P9 Version and docs.** 0.9.0; `CHANGELOG.md` `## [0.9.0]`.

---

## Tasks

### Wave 0 — pre-flight

**T1. Baselines** [run]
- `git branch --show-current` = main; `git status --short plugins/O-ParametricEQ` clean except `.planning/`.
- Configure `-DOUARICON_BUILD_TESTS=ON`; build both harnesses; tallies (expect 150 / 0 / 18, 166 / 0); XR-lid 16 / 16 equal to 0.5.0. Log to the scratchpad.
- Build `research-3.3/probe` against `HEAD`: `probe33 n` → N1e **12 / 16** and hash `6274ff9b5b0e488a` (the bug is reproduced before any edit).
- Copy `execute-3.2/editor-bench/` to `execute-3.3/editor-bench/`; build Release; re-run the 3.1 + 3.2 rows on the unmodified editor: all pass.
- Depends on: none.

### Wave 1 — DSP fix and render rows

**T2. Listen NaN fix (P1)** [dsp-agent]
- `Listen.h` flag, `renderVoice`, `takeNonFiniteVoice`, `goIdle`; step 8 line with its comment.
- Depends on: T1.

**T3. Render-harness rows** [dsp-agent]
- **N1e** (from `probe33 n`): NaN at `t1 + 700` inside an engage's 30 ms ramp, 48 kHz; mono / stereo × blocks 64 / 512 × band on / off × engage / change = 16 cases; PASS when no non-finite run starts after the NaN's own chunk.
- **N1e-u** (the unconditional take): a NaN in a listen voice's ramp while a band lane is **also** non-finite in the same chunk; from two chunks after, the output is bit-equal to a render with the NaN sample zeroed. Catches the `||` placement.
- **AO1:** consumer registered, mode 0 for 1 s of noise → 0 / 0 samples drained; mode 2 the next second → pre 0, post = the second's sample count.
- **M1:** `PeakHold` (the shipping header) against the RESEARCH table (44.1 k / 1024, 2048, 4096; 48 k / 1024, 4096; 192 k / 2048), blocks completing in real time, 60 Hz reads of a −6 dBFS 1 kHz sine for 3 s: every tick after the first H reads within ±0.5 dB of −6. Vacuity guard: the same run with `H = 1` must show empty-tick reads (≥ 1 tick at −200 for 1024 / 44.1 k), or the row fails.
- Tally becomes 170 / 0 (record the exact count). No existing row, stimulus or threshold edited.
- Depends on: T2 (N1e rows), T5 (M1 includes `PeakHold.h`).

### Wave 2 — C++ editor

**T4. Remove `vu-meter` (P7)** [gui-agent]
- CMake, include, resource branch, page comments, delete the stale copy, reconfigure. Registry row edited in the working tree (committed at T12 by temp index).
- Depends on: T1.

**T5. `PeakHold.h` + meter feed (P4)** [gui-agent]
- Depends on: T4.

**T6. Analyzer (P2)** [gui-agent]
- Member, construction / destruction order, `process (dt)` every tick, encoded per-tap emits on change, mode-add re-emit. Header comments at `PluginEditor.cpp:28-33` and `PluginEditor.h:37` updated (no "3.3 adds").
- Bench hooks (behind `OPEQ_EDITOR_BENCH` only): `onAnalyzerEmit (tap, ints)`, `onOutputLevel (ints)`.
- Depends on: T5.

**T7. Version (P9)** [gui-agent]
- `OPEQ_VERSION 0.9.0`; CHANGELOG.
- Depends on: T6.

### Wave 3 — the page

**T8. Page: P3, P5, P6** [gui-agent]
- Analyzer decode + span centres; the meter canvas, `meterStep`, its rAF loop, tokens; the listen state machine and disable hook.
- Greps: one `setListenBand` call site; no `i / (n - 1)` in `drawAnalyzerTrace`; no `linear-gradient` / stretched gradient in the meter; `meterStep` defined once.
- Depends on: T6 (event shapes).

### Wave 4 — meter ballistics gate

**T9a. `tests/meter-ballistics/`** [gui-agent]
- `ballistics.mjs <index.html>`: lifts `meterStep` by name (exactly once, else FAIL). Rows: a step from −60 to −6 reaches −6 on the first step (attack instant); a drop −6 → −60 at 60 Hz and at 120 Hz steps gives levels within 0.1 dB of each other at 0.25 / 0.5 / 1 s, and −6 − 20 × t within 0.1 dB; peak holds −6 for 1.0 s ± one step then falls at 20 dB/s at both rates; peak never below level. Negative control: a per-frame decay variant (mutating the lifted text to scale by steps, not `dt`) must differ by > 1 dB between rates. `run.sh` wrapper. Needs node.
- Depends on: T8.

### Wave 5 — build, gates, install, validators

**T9. Build and run** [run] — long commands in the background, each with its own log (600 s watchdog).
1. Reconfigure (T4); `ninja OuariconParametricEQ_VST3 OuariconParametricEQ_AU OuariconParametricEQ_Standalone` → no new warnings in the plugin's TUs.
2. Render harness default → 170 / 0 (N1e 16 / 16, N1e-u, AO1, M1 PASS); `--long` → 0 FAIL. Designer → 150 / 0 / 18. XR-lid 16 / 16 SHA-equal to 0.5.0. Probe `probe33 n` on the edited tree → 16 / 16 and hash `6274ff9b5b0e488a`.
3. `tests/g9b/run.sh` → PASS (unchanged from 3.2). `tests/meter-ballistics/run.sh` → PASS, control fires.
4. UI gates from the repo root, exit codes recorded: `check-i18n` 0, `i18n-fr-lint` 0, `i18n-zh-lint` 0, `check-ui-labels` 0, `boot-all-uis --strict-tips` 0 with 0 DEAD. **77 is not a pass.**
5. Greps: native-function diff identical (19 / 19); `vu-meter` absent (P7); `git diff HEAD -- Source/dsp Source/analysis Source/PluginProcessor.*` limited to P1 and the include removal; `OPEQ_EDITOR_BENCH` absent from the plugin's compile commands; `Source/ui/public/modules/*.js` untracked and `vu-meter.js` gone.
- **On a failure:** an existing harness row red, or XR-lid unequal → STOP; compare against `listen-fix.diff` before anything else. A gate red → fix the page / i18n / fixture, never the gate; re-run all five.
- Depends on: T2 – T9a.

**T10. Install and validate** [run]
1. `./scripts/build-and-install.sh O-ParametricEQ` → rc 0, two `-dev` bundles, 0.9.0, no alternate-variant orphan.
2. pluginval strictness 10, `--timeout-ms 600000`: VST3 ×2, AU ×1 → SUCCESS.
3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`. Never `auval -a`.
4. Standalone with audio input: analyzer and meter move (screenshot to the scratchpad), page console 0 errors.
- Depends on: T9.

### Wave 6 — editor bench rows (real editor, real page)

**T11. Bench** [run; gui-agent for the bench code]
- `execute-3.3/editor-bench/` from T1, re-pointed at the edited editor and page. A bench thread feeds `processBlock` in real time where a row needs audio. Pixel probes read the plot / meter canvases. **Screen unlocked** (lockstate checked before each run; memory).
- 3.1 + 3.2 rows: all re-run, all pass. E-idle extended: 10 s of silence after decay → 0 relay messages, 0 `eqSections`, 0 analyzer, 0 `outputLevel`.
- New rows:
  - **E-an-modes (UI-03):** with noise playing, mode 0 / 1 / 2 / 3 → emitted taps exactly {} / {Pre} / {Post} / {Pre, Post}; mode 0 → the processor drains 0 samples; a mode change that adds a tap emits it within 1 tick.
  - **E-an-stale:** Post with noise, switch Off, silence 3 s, switch Post → the first emitted Post frame is ≤ −1000 (tenths) everywhere (process ran while Off).
  - **E-an-peak:** 1 kHz −6 dBFS sine, Post → the trace's highest pixel within 1 px of `fx(1000)`.
  - **E-an-nyq:** 48 kHz, Post with noise → the emitted array has `null` from the first span ≥ 24 kHz; no analyzer ink above the plot floor in the columns right of `fx(24000)`.
  - **E-meter (UI-06):** −6 dBFS sine 0.5 s → both bar tops at `my(−6)` ± 1 px. Repeated with the processor prepared at 44.1 k / 2048 and blocks of 2048 → bar top range ≤ 1 px over 2 s (no sawtooth). Mono → two equal bars.
  - **E-meter-rest:** after silence, the meter's rAF loop stops (no meter frames for 2 s after the peak reaches −60).
  - **E-fills (UI-07):** bands 1 – 3 enabled at distinct frequencies, ±9 dB → the pixel halfway between each band's curve and 0 dB at its peak has hue within ±15° of `pigment(b)`; band 2 bypassed → that pixel is the background.
  - **E-listen:** `listenBand` read through the hook. (a) drag past threshold → band; release → −1. (b) click without movement → never set (0 writes). (c) latch 3 → drag 5 → during = 4 (0-based) → release → 2. (d) latch → host `setValueNotifyingHost (b_enabled, 0)` → −1. (e) latch → preset load disabling it → −1. (f) latch → preset load keeping it enabled → unchanged. (g) latch → bypass → unchanged. (h) latch → Delete key → −1. (i) strip Listen twice → band then −1. (j) cancel and lost capture end a drag listen.
  - **E-fps (ROADMAP criterion 5, page side):** Pre+Post with noise, 192-section E-draw session, scripted 5 s node drag at 60 Hz → rAF intervals median ≤ 17.5 ms, p99 ≤ 25 ms; `draw()` median / p99 recorded.
  - **E-audio (criterion 5, audio side):** `processBlock` (24 × 96 dB/oct, 48 kHz, 512) paced in real time for 10 s, three runs: editor closed; editor open, Off, no drag; editor open, Pre+Post, scripted 60 fps drag. Run 3 median within 10 % of run 1, p99 within 25 %. Quiet machine; on a fail re-run once and record both.
  - **E-rate (VERIFICATION-3.2 Issue 1):** during the scripted drag, median interval between `eqSections` emits ≤ 20 ms.
  - **E-create-seeded (Issue 2):** band 1 seeded High Shelf / Side / bypassed / disabled → plot click → band 1 Bell / Stereo / not bypassed.
  - **E-gbypass (Issue 4, V7):** global bypass on → the sum's ink pixels lie on `gy(0)` ± 1 px.
  - **E-sumview (Issue 4, V8):** a Mid and a Side band, the Side band selected → the ink matches the SS composition at three frequencies (pixel probe).
  - **E-cycle-dbg:** 10 open / close cycles in Debug with Pre+Post, meter and listen live → 0 assertions, no leak report; after each close `listenBand` = −1 and no consumer registered.
- **Mutant control** (`execute-3.3/mutants/`, each must fail its row; restore and rebuild after each):

  | Mutant | Must fail |
  |---|---|
  | Listen flag never set | N1e |
  | Flag taken inside `! finite \|\|` | N1e-u |
  | `analyzer->process` skipped while mode 0 | E-an-stale |
  | Emit every tick (no change test) | E-idle |
  | Page maps `null` to 0 (no explicit `NaN`) | E-an-nyq |
  | Span centre reverted to `i / (n − 1)` | E-an-peak |
  | `PeakHold` depth fixed at 1 | E-meter (2048) and M1 |
  | Listen set at pointerdown | E-listen (b) |
  | No return to the latch on release | E-listen (c) |
  | Disable hook only in `deleteBand` | E-listen (d), (e) |
  | Timer at 30 Hz | E-rate |
  | Meter decay per frame, not per second | meter-ballistics |
- **On a failure:** fix the editor / page, re-run W5 from T9 step 1.
- Depends on: T10.

### Wave 7 — bookkeeping + commit

**T12. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/3-gui/SUMMARY-3.3.md`: tallies; N1e before / after; probe hash; the five gate exit codes; validators; every E-row with figures; the mutant table; deviations (every edit beyond P1 – P7 with its reason).
- Carry-forward for verify 3.3 (hands-on, Taylor; Logic AU and one VST3 host): analyzer modes on program material; meter against Logic's own channel meter on a −6 dBFS sine; drag listen and latch by ear (engage / return clean, no click); disabling the listened band by automation ends it; a 2048 buffer setting shows a steady meter.
- `PLUGINS.md` version cell → 0.9.0 (check `git diff PLUGINS.md` first; other sessions' rows → temp-index recipe or leave out and say so).
- STATUS: `status: stage_3_phase_3_3_executed`, `next_action: stage_3_verify_3_3`.
- Commit on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` new files by name (`Source/editor/PeakHold.h`, `tests/meter-ballistics/*`, `SUMMARY-3.3.md`, `execute-3.3/` minus build dirs), then `git commit -- plugins/O-ParametricEQ` (+ `PLUGINS.md` if clean). `modules/registry.yaml`: the single `used_by` hunk through a temp index, in the same commit or a second path-scoped one. Never `-A` / `-a`. `git log -1 --stat` lists only these paths; no `Source/ui/public/modules/*.js`. No tag.
- Depends on: T11.

---

## Dependency graph

```
T1 ─┬→ T2 ─────────────┐
    │                  ├→ T3 ─────────────────────┐
    └→ T4 → T5 ────────┘                          │
             └→ T6 → T7                           │
                 └→ T8 → T9a ─────────────────────┤
                                                  ↓
          T9 (build, harnesses, gates) → T10 (install, validators) → T11 (bench + mutants) → T12
           ↑                                                              │
           └────────────── fix → rebuild → rerun from T9 ←────────────────┘
```

T2 (dsp-agent) and T4 – T8 (gui-agent) touch disjoint files and can run in parallel; T3 waits for both T2 and T5.

## Success criteria

| Criterion (ROADMAP 3.3; UI-03, UI-06, UI-07, FUNC-08) | Proven by |
|---|---|
| Analyzer Off / Pre / Post / Pre+Post (UI-03) | E-an-modes, E-an-stale, E-an-peak, E-an-nyq |
| Off stops the audio-thread writes | AO1, E-an-modes (0 drained) |
| Meter tracks a −6 dBFS sine within 0.5 dB (UI-06) | M1 (all block sizes), E-meter (pixel, 2048), meter-ballistics |
| Fills coloured per band (UI-07) | E-fills |
| Listen engages on drag, releases cleanly, latch, disable / delete ends it (FUNC-08) | E-listen (a) – (j); 3 mutants; hands-on at verify |
| NaN inside listen's ramp caught at its own chunk | N1e 16 / 16, N1e-u; 2 mutants |
| 60 fps drag with the analyzer on, audio-thread time unchanged | E-fps, E-audio |
| VERIFICATION-3.2 gaps closed | E-rate, E-create-seeded, E-gbypass, E-sumview |
| Listen-off audio unchanged | XR-lid 16 / 16; probe hash; 150 / 0 / 18 |
| No idle traffic | E-idle extended, E-meter-rest |
| 0.9.0 validates | T10 |

## Risks

- **R1 Gate scope.** The listen rows must exercise host- and preset-driven disables, not only the page's delete, or they inherit the research's path (memory: `pattern_gate_inherits_the_review_findings_scope`). Rows (d), (e) and the "disable hook only in `deleteBand`" mutant are that check.
- **R2 Vacuous meter gate.** At block ≤ 512 the hold rule is never exercised. M1 and E-meter run 2048 / 4096 blocks, and M1's `H = 1` guard proves the stimulus produces empty ticks.
- **R3 Locked screen throttles WKWebView.** rAF and timers stall when the screen is locked; E-fps / E-rate / E-meter would read as failures. Check lockstate before each bench run.
- **R4 Bench timing noise.** E-audio compares medians on one machine; a concurrent build ruins it (RESEARCH's contended run). Quiet machine, one re-run allowed, both recorded.
- **R5 Single-reader taps.** The bench must not register its own analyzer while the editor's exists; AO1 runs in the harness with no editor.
- **R6 Shared checkout.** `modules/registry.yaml` carries another session's uncommitted edits; `.planning/STATE.md`, agent memory and other plugins too. Temp-index for the registry hunk, path-scope everything else, re-check immediately before the commit.
- **R7 Canvas text on first paint.** Ruler digits drawn before EB Garamond loads keep the fallback face; repaint after `document.fonts.load()`.
