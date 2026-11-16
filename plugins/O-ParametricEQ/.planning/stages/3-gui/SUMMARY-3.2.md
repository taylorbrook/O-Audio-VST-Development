---
title: "O-ParametricEQ Stage 3 (GUI) — Phase 3.2 Summary (interactive response display)"
date: 2026-11-16
phase: "3.2"
plan: PLAN-3.2.md
version: 0.8.0
status: executed
---

# O-ParametricEQ — Phase 3.2 Summary

The editor now draws the curve. It comes from the plugin's own `DisplayModel` and is live with the transport stopped. Nodes can be created, dragged, scrolled, reset and deleted, and each action opens a host gesture only on the parameters it writes. A click with all 24 bands in use shows a keyed tip in three languages. Version 0.8.0 is built, installed and validated, and the audio output is unchanged.

## Results at a glance

| Check | Result |
|---|---|
| Designer harness | 150 PASS / 0 FAIL / 18 INFO (T1 baseline 150 / 0 / 18) |
| Render harness, default / `--long` | **166 PASS / 0 FAIL** both, before and after every change (T1 baseline 166 / 0) |
| Audio unchanged | XR-lid sessions rendered by 0.8.0 are SHA-256-equal to the 0.5.0 hashes 16 / 16 (`2-dsp/verify-2.6/sessions-050.sha`) |
| `git diff HEAD -- Source/PluginProcessor.* Source/dsp Source/analysis` | empty |
| **G9 (b)** `tests/g9b/run.sh` | **PASS**: worst **1.37e-8 dB** (gate 0.01) over **148 956** scored points; per rate 44.1 k 5.2e-9, 48 k 8.3e-10, 88.2 k 7.6e-10, 96 k 3.5e-10, 192 k 1.37e-8; mono set 875 points; 0 NaN; 6 / 6 controls (below) |
| UI gates (repo root) | `check-i18n` 0, `i18n-fr-lint` 0, `i18n-zh-lint` 0, `check-ui-labels` 0, `boot-all-uis --strict-tips` 0 with 0 DEAD (final tree) |
| Native functions | 19 / 19; grep diff (page + `preset-manager.js` vs `PluginEditor.cpp`) empty |
| `OPEQ_EDITOR_BENCH` | 0 hits in the plugin's compile commands |
| Tracked `Source/ui/public/modules/*.js` | none |
| `bands-full` English literal in `index.html` | none |
| Build warnings in the plugin's TUs | none |
| Install | `build-and-install.sh` rc 0, `O-ParametricEQ-dev.vst3` / `.component` at 0.8.0, no alternate-variant orphan |
| pluginval strictness 10 | VST3 ×2 SUCCESS, AU ×1 SUCCESS (final tree, re-run after the last source edit) |
| auval `aufx OuPQ OuDv` | AU VALIDATION SUCCEEDED |
| **Editor bench** | **5 / 5 Release processes + Debug: ALL-PASS** (`verdict32.py`); Debug 10 cycles, 0 assertions, 0 leak lines |
| **Mutant control** | **7 / 7 caught**, control ALL-PASS |

## G9 (b) negative controls (each must exceed 0.05 dB)

| Control | Worst |
|---|---|
| 1 `k × 1.01` in every SVF section | 19.7 dB |
| 2 `onePoles` dropped | 172.9 dB |
| 3 first Left and Mid bands swapped in order | 238.0 dB |
| 4 Side composed with the Mid matrix (one-token edit of the lifted `composeSum`) | 236.6 dB |
| 5a shipping `composeSum`, relay placements (odd Left, even Right) vs packet placements on the 10 mono draws | **0 dB difference** over 2035 points (must be identical) |
| 5b the 0.7.0 `composeSum` (`tests/g9b/composeSum-0.7.0.js`) fed the same relay placements | **59.5 dB** (`m000`, 1.72 kHz): the research bug, reproduced |
| 6 (added) the `active` filter removed | 256.8 dB |

T1's research reproduction is part of the gate itself: the main score equals RESEARCH-3.2's 1.37e-8 dB to the digit, and control 5b equals its 59.5 dB `raw-mono` error.

## Editor bench (real editor, real page; `execute-3.2/editor-bench/`)

5 fresh Release processes (10 open / close cycles each; the rows run in cycle 0 with 10 reps × 5 preset paths; 10 s idle) and 1 Debug process (10 cycles, `eqSections` live). All events are JS-dispatched `PointerEvent` / `WheelEvent` / `MouseEvent` / `KeyboardEvent` on the real elements. The bench never calls `processBlock`, so **every row runs with the transport stopped**. "Painted curve" checks are pixel probes: ink runs of the summed line (#3C2F2F, full alpha) in 9 – 11 canvas columns, compared with an independent C++ closed form over a bench-owned `DisplayModel`'s packets (±2.5 px, columns in the plot and clear of nodes).

### 3.1 rows (re-run; verify-3.1's `verdict.py` rules)

All 13 pass in all 6 processes. E-open cold 255.2 / 264.7 ms median / worst (gate 500), warm 132.5 / 149.3. **E-idle: 0 relay events, 0 posts, 0 `eqSections` over 10 s** in all 5. E-preset: 250 loads by 5 paths, every one painted in 1 frame, 0 host mismatches.

### 3.2 rows

| Row | Result (5 Release processes) |
|---|---|
| **E-eq-init** | First push after `pageReady`: 24 bands, `band` 1 … 24, 48 kHz, `stereo`, `rateKnown` — **50 / 50 opens**. Preset paths: each load's next pushes carry every band whose parameters changed (**0 missing over 250 loads**), and the painted curve matches the engine at **2075 / 2075** probe columns |
| **E-eq-idle** | 10 s idle: **0** C++ emits, **0** `eqSections` at the page, 0 events of any kind |
| **E-eq-reload** (added) | The page reloads inside a live editor with nothing changed: the first push on the new page carries 24 bands |
| **E-stopped** | Host `setValueNotifyingHost` on `b03_freq`, no `processBlock`: the first emit after the edit carries band 3, after 0.1 / 10.5 / 14.2 / 4.0 / 5.3 ms (bound 2 ticks = 33.3 ms + 4); the painted curve matches before and after and moved |
| **E-mono** | Prepared mono, bands 4 / 5 set Left / Right: packets carry Stereo (relays still Left / Right), header `stereo: false`, all 24 re-pushed on the layout change. Painted curve = the Stereo-placement curve at 9 / 9 columns, and 3 of those columns are > 6 px off the relay-placement curve (the probe can see the bug) |
| **E-create** (UI-01) | Click on the empty plot: band 1 enabled, Bell / Stereo / not bypassed, Q back to its default from 7, slope left at 12 dB/oct, freq = `xf(x)` (≤ 1e-4 rel), gain = `round(yg(y), 0.1)`; one gesture pair each on freq / gain / q, none outside band 1. With 1 – 23 enabled the click creates band 24 |
| **E-full** (U5) | 24 enabled: the click changes **0** parameters (0 gestures, 0 value changes). Tip visible at the pointer, inside the window, `data-i18n = bands-full`; text = `LABELS['bands-full']` in en, fr, zh-Hans; hidden by the next `pointerdown`; shows with hover help off; stays inside at the bottom-right corner. 10 / 10 checks |
| **E-drag** | Bell: one pair on freq and on gain, values follow the pointer. **Low Cut: one pair on freq, zero on gain**, gain value untouched. Bell at gain scale 0 %: zero on gain. `pointercancel` and `lostpointercapture` close both pairs. Bell → Low Cut by automation mid-drag: pairs stay balanced. 6 / 6 |
| **E-wheel** | 10 events in one burst: one pair on q, ln(q1 / q0) = 2.5 (Shift 0.6), within 2e-3 |
| **E-dblclick** (VERIFICATION-3.1 Issue 1) | Node gain, strip freq / gain / Q fields, `output_gain` and `gain_scale` knobs: host value = `getDefaultValue()` (normalised error ≤ 1e-6), one pair each. A node created within 450 ms keeps its gain. 7 / 7 |
| **E-delete** | Delete key and strip button: `enabled` off, one gesture, nearest lower band (3) selected. 2 / 2 |
| **E-ends** (UI-04, DSP-06) | 20 / 20 typed entries land **exactly** (`get() ==`) on the range start / end: freq "10", "30k", "30000", "5", "99k", "0", "1e9", "-99"; gain "-30", "30", "-99", "99k", "1e9", "-1e9"; Q "0.025", "40", "0", "99k", "1e9", "-99" |
| **E-cycle-dbg** (VERIFICATION-3.1 Issue 3) | Debug, 10 open / close cycles with `eqSections` live: 0 assertions, 0 leak lines, ALL-PASS |
| **E-lag** (P10) | Scripted 2 s Bell drag at 60 Hz pointer rate, 119 moves: relay write (C++ parameter change) → `eqSections` emit → painted (page frame after arrival, reported back): median **16.7 – 18.5 ms**, p99 **31.1 – 31.7 ms**, max ≤ 34.2 ms (bound 2 ticks + 1 frame = 50 ms) |
| **E-draw** (P10, recorded) | `draw()` in WKWebView on a 24-band, **192-section** draw (shelves / cuts at 96 dB/oct; plot 1458 × 838 in a 1600 × 1000 editor): median **9 – 10 ms**, p99 12 – 14 ms over 120 draws. Research's V8 figure was 3.83 / 5.10 ms on a 94-section draw |

## Mutant control (`execute-3.2/mutants/`)

One edit each to a scratch copy of `Source/` (`-DPEQ_SRC`), the bench rebuilt and run once (1 cycle, 1 rep, 3 s idle). The control run uses the unmodified copy.

| Mutant | Target row | Rows failing | Caught |
|---|---|---|---|
| control (unmodified) | — | none (ALL-PASS) | — |
| E15 `getParameterDefaults` returns normalised | E-dblclick | E-dblclick, E-create (Q default) | ✓ |
| `onUp` skips the freq `sliderDragEnded` | E-drag | E-drag | ✓ |
| P5 reverted (gain begin / end always) | E-drag | E-drag (Low Cut, scale 0 %) | ✓ |
| P3 reverted (`composeSum` reads relay placement) | E-mono, G9 (b) | E-mono; **G9 (b) FAIL** (control 5a: 200.6 dB) | ✓ |
| `requestEqSections` never sets `fullPushPending` | E-eq-reload | E-eq-reload | ✓ |
| `timerCallback` emits on an empty list | E-eq-idle | E-idle, E-eq-idle, E-eq-reload | ✓ |
| refused-create branch silent | E-full | E-full | ✓ |

The mutant runs were made before the E-lag pairing fix (below), so each also lists E-lag. The control was re-run after the fix and passes everything. `mutants-results.json` holds both.

## What changed

| File | Change |
|---|---|
| `Source/PluginEditor.h/.cpp` | `std::unique_ptr<DisplayModel> displayModel` (after the attachments) and `bool fullPushPending = true`. `requestEqSections` sets the flag. `pageReady` starts the timer at **60 Hz**. `timerCallback`: `poll()`, all 24 after the flag, else the changed bands, nothing on an empty list. Payload `{ sampleRate, stereo, rateKnown, bands: [packet + band (1-based)] }`, one `emitEventIfBrowserIsVisible` per tick. Comment at the emit for a hidden WebView. Bench-only `onEqSections` hook under `OPEQ_EDITOR_BENCH`. Comments no longer say "3.2 adds" |
| `Source/ui/public/index.html` | P3: `composeSum` composes the bands whose packet is `active`, with the packet's placement, and reads no relay. `sumView` uses the selected band's packet placement. `draw()` evaluates every relay-enabled or packet-active band and passes the packet-active list. P5: `d.gain` is decided at the drag threshold; the gain begin / write / end happen only when it is true. P7: `placeTip(anchor)` refactor; `showBandsFullTip(x, y)` on a refused create; `clearPointTip`; a MutationObserver re-places the point tip after a language change; `.tooltip.tip-point` style; `BANDS_FULL_TIP_MS = 1600`. `parseEntry` accepts exponent forms (deviation 2) |
| `Source/ui/public/js/i18n.js` | `LABELS['bands-full']`: en "All 24 bands are in use", fr "Les 24 bandes sont toutes utilisées" (`reviewed: false`), zh-Hans "全部 24 个频段均已使用" (`'mt'`) |
| `tests/render-harness/main.cpp` | `--dump-g9 <dir>`: runs only the XR-g9 jobs, writes 260 draws, exits 0 only if all were written. No row, stimulus or threshold changed (166 / 0 unchanged) |
| `tests/g9b/` (new) | `g9b.mjs`, `run.sh`, `README.md`, `composeSum-0.7.0.js` |
| `tests/i18n-states.json` | State 5, appended last: bands 1 – 24 enabled through the stub, a `pointerdown` on the plot near its right edge |
| `CMakeLists.txt` | `OPEQ_VERSION 0.8.0` (`DisplayModel.cpp` was already in `target_sources`) |
| `CHANGELOG.md` | `## [0.8.0] - 2026-11-15` |

## Deviations

1. **`bands-full` is a `LABELS` entry, not `I18N`.** `check-i18n [1]` requires `t` and `b` on every `I18N` entry. The tip is written with `setLabel` (a literal key, which passes [13] and the dead-key rule), so `check-ui-labels` measures it and a language change re-localizes it. A computed key in `showPointTip (x, y, key)` would fail [13], so the function is `showBandsFullTip (x, y)`.
2. **`parseEntry` accepts exponent forms** (P6 allows "a row that fails is fixed in the page"). E-ends' "1e9" read as 1 (the leading digits): the start on freq, and mid-range on gain and Q. It now clamps to the end.
3. **The point tip is the label element itself, and it is re-placed on re-localization.** The first `check-ui-labels` run failed [7] on the U5 state: `#tooltip dh = 15.5` in fr, because the French text re-wrapped inside the English-pinned width. Now `.tooltip.tip-point` carries `data-i18n`, and a MutationObserver re-runs measure / pin / clamp when its text changes. `clearPointTip` drops the binding before the box is reused by a hover tip. All five gates were re-run.
4. **E-eq-reload row added.** The plan's mutant "`fullPushPending` never set by `requestEqSections`" cannot be seen on a preset load: `poll()` already reports the changed bands. A page reload inside a live editor is the only case the flag serves, so the row reloads the page and requires 24 bands on the new page's first push. It catches the mutant.
5. **G9 (b) control 5 uses a committed 0.7.0 fixture, not `HEAD`.** After this commit `HEAD`'s `composeSum` is the fixed one, so the pre-P3 text is kept verbatim in `tests/g9b/composeSum-0.7.0.js`. **Control 6** (the `active` filter removed) was added.
6. **Bench-only editor hook.** `opeq::bench::Hooks::onEqSections`, compiled only under `OPEQ_EDITOR_BENCH`, gives E-lag and E-stopped the C++ emit times.
7. **Bench run conditions.** A locked screen occludes the bench window, and WKWebView then throttles `requestAnimationFrame` and timers. The unmodified 3.1 bench failed cycle 0 the same way on 2026-11-15. `run-all.sh` and `run-mutants.py` now wait for an unlocked screen (`lockstate.swift`), run under `caffeinate -d -u -i`, and mark a run void if the screen locked during it (none did).
8. **E-lag pairing fix (bench).** The first full run failed E-lag in every process (0 moves matched). E-eq-reload resets the page's `eqSections` count while the C++ emit list kept counting, so the pairing of the k-th send with the k-th arrival was off. The C++ list now restarts at the reload. Bench code only. The run was repeated in full and passes.
9. **T9.4 Standalone screenshot not taken.** The real editor's painted curve is covered by the bench's pixel probes (2075 / 2075 columns over 250 preset loads; E-stopped, E-mono), and console errors by E-props / console-after-tests (0 / 0).

## Carry-forward

**For verify 3.2 (hands-on, Taylor; Logic AU and one VST3 host):**
- In Touch mode, a Bell node drag records the `freq` and `gain` lanes together, and one ⌘Z undoes it.
- A Low Cut drag records only `freq`.
- A wheel burst records one Q stretch.
- Click-to-create in Latch writes the new band.
- The curve follows automation with the transport stopped.
- A mono track with Left / Right bands draws the Stereo curve while the badges still show L / R.
- The 25th-band tip in all three languages.
- Plus 3.1's owed items: open in Logic / a VST3 host, the timing flag log, DAW preset round trip.

**For 3.3:**
- **`draw()` is 9 – 10 ms median, 12 – 14 ms p99 in WKWebView on a 192-section draw.** That is most of a 16.7 ms frame before the analyzer, so the per-band response cache (P10) is likely needed for the 60 fps row. It is about 2.5× the V8 research figure, on twice the sections.
- The 60 Hz timer also drives `SpectrumAnalyzer::process` (measured dt; analyzer every second tick if needed).
- Listen on drag hooks into P5's drag start / end (`d.moved` / `d.gain` block, `onUp`).

**Stage 4:** fr / zh-Hans review of `bands-full`; host text units (VERIFICATION-3.1 Issue 4); E16 / R2 preset rows (Issue 2); the timing-flag hook decision.

## Logs

The session scratchpad holds the T1 baselines, the post-change harness runs, the gate logs, the validator logs, the void bench runs (locked screen) and the first full run (E-lag pairing). The committed bench results are `execute-3.2/editor-bench/results/*.json` and `execute-3.2/mutants/mutants-results.json`.
