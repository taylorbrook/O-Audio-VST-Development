---
title: "O-ParametricEQ Stage 3 (GUI) — Phase 3.2 Execution Plan (interactive response display)"
created: 2026-11-15
juce_version: "8.0.15"
phase: "3.2"
summary: "Seven waves: (W0) pre-flight and baselines; (W1) page: compose the curve from each packet's effective placement and active flag (P3), open the gain gesture only when a drag can write gain (P5), the 25th-band point tip with key bands-full in en / fr / zh-Hans (P7); (W2) C++: the editor owns DisplayModel, requestEqSections sets a full-push flag, a 60 Hz pageReady-gated timer emits only changed bands, version 0.8.0; (W3) the G9 (b) gate: render harness --dump-g9 + tests/g9b/g9b.mjs lifting the shipping functions, vacuity guards, five negative controls; (W4) build, harnesses, G9 (b), five UI gates, install, pluginval 10, auval; (W5) editor bench rows: eqSections push / idle / transport stopped, create, drag gesture pairs (Low Cut zero gain pairs), wheel, double-click defaults (kills E15), delete, typed range ends exact, 25th-band tip, Debug E-cycle with eqSections live, node-to-curve lag and draw() time in WKWebView; (W6) summary, STATUS, one path-scoped commit. No processor, DSP or DisplayModel change. No tag."
stages: [3]
agents: [gui-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 3 / Phase 3.2 Execution Plan (interactive response display)

## Goal

The curve is the instrument:

- **True curve.** The editor pushes `eqSections` from the plugin's own `DisplayModel`. The page draws the U1 sum from the processor's packets, and G9 (b) holds it to the C++ closed form within 0.01 dB on all 260 XR-g9 draws, mono included.
- **Live with the transport stopped.** A host edit with no `processBlock` call reaches the painted curve within 2 ticks.
- **Nodes.** Click to create (U3), drag frequency / gain, wheel Q, double-click reset, delete. Every write sits inside a host gesture, one pair per parameter touched, and none on a parameter the action cannot write.
- **Strip.** Typed entry lands exactly on every range end (UI-04, DSP-06). Double-click resets land on the parameter's default.
- **U5.** The 25th-band click shows a keyed tip at the pointer, in all three languages.

Version 0.8.0 built, installed, validated. Audio output unchanged (no processor or DSP edit).

**In scope:** `Source/PluginEditor.h/.cpp`, `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `CMakeLists.txt` (version), `tests/render-harness/main.cpp` (`--dump-g9` only), `tests/g9b/` (new), `tests/i18n-states.json` (one state appended), `CHANGELOG.md`, the editor bench under `stages/3-gui/execute-3.2/`.

**Out of scope (and what 3.2 does instead):**

| Later | 3.2 behaviour |
|---|---|
| 3.3 analyzer, meter, fills | No `SpectrumAnalyzer`, no `outputLevel`. The timer emits `eqSections` only. |
| 3.3 listen (U2) | `setListenBand` unchanged from 3.1. A node drag does not engage listen. |
| 3.3 per-band response cache (P10) | `draw()` re-evaluates every band. 3.2 records its cost in WKWebView; 3.3 caches only if its 60 fps row needs it. |
| Stage 4 | fr / zh-Hans review of `bands-full` with the rest; host text units (VERIFICATION-3.1 Issue 4); the E16 / R2 preset rows (Issue 2); the timing flag hook decision. |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/3-gui/RESEARCH-3.2.md` | All of it. Q1 (wiring, cost, hidden page), Q2 (G9 (b) method and gate), Q3 (mono placement bug), Q4 (gestures, bench rows), Q5 (typed entry), Q6 (U5), Q7 (draw cost), P1 – P10. |
| `stages/3-gui/research-3.2/g9b-bench/` | `make_dump_main.py` (the dump patch P4 makes permanent), `g9b.js` (the runner P4 moves to `tests/g9b/`), `evalcost.js`, `results.txt`. |
| `stages/3-gui/CONTEXT.md` | U1, U3, U5, "Constraints". Binding. U5's localisation line is amended by this plan (below). |
| `stages/3-gui/PLAN-3.1.md`, `SUMMARY-3.1.md`, `VERIFICATION-3.1.md` | Editor shape (P2 – P3), the T11 bench, Issues 1 and 3, mutants E15 / E17. |
| `stages/3-gui/execute-3.1/editor-bench/` | The bench (`src/Main.cpp`, `src/BenchScript.h`, `run-all.sh`, `analyze.py`). W5 copies and extends it. |
| `Source/analysis/DisplayModel.h/.cpp` (wherever the tree has it), `Source/dsp/BandParams.h:264` (`effectivePlacement`) | `poll()`, `packetFor`, `headerVar`. Read-only. |
| `Source/PluginEditor.cpp` `:390` (`requestEqSections`), `:421` (timer start), `:638` (`timerCallback`) | The three edit points of P1 / P2. |
| `Source/ui/public/index.html` `:642` (`TYPE_HAS_GAIN`), `:1301` (`createBand`), `:1491` (`composeSum`), `:1533` (`sumView`), `:1801-1903` (node handlers), `:1930` / `:1970` / `:1995` (`eqSections` listener, init and post-preset pulls), `:2107` (`showTooltip`) | The page edit points. |
| `scripts/check-i18n.js` rules [1], [2], [5] | `bands-full` needs en / fr / zh-Hans, `reviewed` set; an unbound bodied key passes [2] while other bindings exist (read at plan, `:627-660`). |
| Memory: `pattern_fixed_tooltip_shrink_to_fit_edge`, `pattern_tooltip_clamp_gate_viewport_sensitive`, `pattern_i18n_states_new_state_goes_last`, `critical_webview_completion_gated_on_isvisible`, `pattern_webview_one_shot_state_push_stale_on_preset_load`, `pattern_pointerdown_preventdefault_kills_document_mousedown`, `pattern_gate_inherits_the_review_findings_scope`, `pattern_new_lint_zero_must_agree_with_independent_scan`, `pattern_source_grep_gate_hits_configure_copied_module_js`, `pattern_executor_watchdog_stall_run_long_commands_in_background` | Traps this phase walks through. |

---

## Plan-phase decisions

No call for Taylor (RESEARCH-3.2 "Calls: none"). Amendment made in this plan phase:

- **CONTEXT U5:** "French / zh-Hans drafted with the Stage 4 localisation" → machine drafts ship in 3.2 (`fr` `reviewed: false`, `zh-Hans` `reviewed: 'mt'`), forced by `check-i18n [1]`; Stage 4 reviews them with the rest. ROADMAP unchanged (P9: the gates fit the existing 3.2 criteria).

The plan's calls:

- **P1 Files.**

  | File | Status | Content |
  |---|---|---|
  | `Source/PluginEditor.h/.cpp` | edit | `DisplayModel` member, `fullPushPending`, `requestEqSections`, 60 Hz timer, `timerCallback` emit (P2). |
  | `Source/ui/public/index.html` | edit | P3, P5, P7 only. Any other edit recorded with its reason. |
  | `Source/ui/public/js/i18n.js` | edit | Key `bands-full` (P7). |
  | `CMakeLists.txt` | edit | `OPEQ_VERSION 0.8.0`; `DisplayModel.cpp` into the plugin's `target_sources` only if the editor fails to link without it. |
  | `tests/render-harness/main.cpp` | edit | `--dump-g9 <dir>` (P4). No row, stimulus or threshold edited. |
  | `tests/g9b/g9b.mjs`, `tests/g9b/run.sh`, `tests/g9b/README.md` | new | P4. |
  | `tests/i18n-states.json` | edit | One state appended last (P7). |
  | `CHANGELOG.md` | edit | `## [0.8.0]` entry (bracket form). |

  **No edit** to `Source/PluginProcessor.*`, `Source/dsp/*`, `Source/analysis/*` (DisplayModel included). `git diff HEAD -- Source/PluginProcessor.h Source/PluginProcessor.cpp Source/dsp Source/analysis` is empty at the commit.

- **P2 Editor: `eqSections` (RESEARCH P1, P2).**
  - Member `std::unique_ptr<DisplayModel> displayModel`, built in the constructor from the processor's parameters (per `DisplayModel.h`; the same construction the harness's XR-dm rows use). Declared after the attachments, before the plain state. Construction designs nothing.
  - `bool fullPushPending = true` (message thread only). `requestEqSections` sets it and completes `true`. The page calls it at init (before `pageReady`) and after every preset load; the flag waits for the next tick.
  - `pageReady`: `startTimerHz (60)` (was 30). Still idempotent; `pageIsReady` reset and `stopTimer()` first in the destructor (3.1).
  - `timerCallback()`: return unless `pageIsReady && webView`. `changed = displayModel->poll()`. If `fullPushPending`, emit all 24 and clear the flag; else emit `changed`; emit **nothing** on an empty list.
  - Payload `{ sampleRate, stereo, rateKnown, bands: [ packetFor (b) with band = b + 1 ] }`, one `emitEventIfBrowserIsVisible ("eqSections", …)` per tick. `sampleRate` / `stereo` / `rateKnown` from `headerVar()`.
  - Hidden page: the editor never hides the WebView (RESEARCH Q1). A comment at the emit says: if a later change hides it, skip `poll()` while hidden or set `fullPushPending` on show.
  - Nothing else in the editor changes. The 19 natives stay 19; the grep diff stays identical.

- **P3 Page: compose from the packets (finding 3).**
  - `composeSum`: the bands composed are those whose packet is `active`, in index order; each band's matrix from `bandPackets[b].placement`. No relay read inside `composeSum`.
  - `sumView()`: the selected band's packet placement (nothing selected, or no packet yet → LL).
  - Unchanged: global bypass draws the sum flat and dashed; nodes, washes, bypassed ghosts and the node badge keep their relay reads (the badge shows what the user set; on mono the curve shows what runs).
  - Before the first packet arrives the page draws no curve (same as 3.1), not a relay-composed one.

- **P4 G9 (b) gate (RESEARCH Q2).**
  - **Harness:** `--dump-g9 <dir>` runs only `displayRowsXr`'s G9 jobs and writes, per draw, `{ rate, stereo, header, packets, grid, cpp }` exactly as `make_dump_main.py`'s env switch does, then exits 0. Without the flag the harness is byte-for-byte the same run (166 / 0).
  - **Runner** `tests/g9b/g9b.mjs <index.html> <dumpdir>`: lifts `evalBand`, `magDb`, `composeSum` by name from the shipping `Source/ui/public/index.html` (brace matching, never a copy), feeds the packets 1-based, composes `active` bands per P3, scores LL / RR / MM / SS at **0.01 dB** with XR-g9's floors (within 80 dB of the largest maximum of the four views, ≥ −120 dB). Prints worst per rate, scored count, exit 0 / 1.
  - **Vacuity guards** (fail the run): each lifted function found exactly once; scored points ≥ 100 000; each of 44.1 / 48 / 88.2 / 96 / 192 kHz and the mono set contributes points; no NaN below Nyquist.
  - **Negative controls**, run by the same script, each must exceed **0.05 dB** or the run fails: `k × 1.01` in every SVF section; `onePoles` dropped; first L and M bands swapped in order; Side composed with the Mid matrix (one-token mutation of the lifted `composeSum`); on the mono draws, packet placements replaced by relay-style placements (odd Left, even Right) **fed through the page's own placement source** — after P3 the composed result must be unchanged (control proves the page ignores relay placement), and the same substitution applied to the pre-P3 `composeSum` text from `HEAD` must exceed 0.05 dB (proves the control can see the bug).
  - `tests/g9b/run.sh`: build check, dump to a temp dir, node run; one command for the verify recipe. Needs node (the UI gates already do).

- **P5 Page: gain gesture only when the drag can write gain (finding 4).**
  - At the drag threshold: `d.gain = TYPE_HAS_GAIN[type] && scale > 0.01`, decided once. Begin the gain gesture only when `d.gain`; write gain only when `d.gain`; end it only when `d.gain`.
  - `freq` keeps one begin at threshold and one end on up / cancel / lost capture (idempotent end, as now).
  - A type change by automation mid-drag cannot unbalance the pair.

- **P6 Interactions already in the page.** Create, wheel Q, double-click, delete and typed entry are not rewritten. They are gated by the W5 rows. A row that fails is fixed in the page, with the fix recorded.

- **P7 U5 tip.**
  - Refactor `showTooltip`'s measure / pin / clamp / shrink-to-fit body into `placeTip (rect, title, body)`. `showTooltip` calls it unchanged in behaviour; `showPointTip (x, y, key)` calls it with a zero-size rect at the pointer, body only.
  - On a refused create (`createBand` returns 0), `showPointTip` at the pointer for ~1.6 s; hidden by the next `pointerdown` (capture phase, so a dial that `preventDefault`s still dismisses it). Shown even with hover help off (feedback, not help).
  - `i18n.js`: key `bands-full`, `en` "All 24 bands are in use", `fr` draft `reviewed: false`, `zh-Hans` draft `reviewed: 'mt'`. No `TIP_BINDINGS` row (no DOM anchor; rule [2] passes with the existing bindings, read at plan). Copy only from the table, never a literal.
  - `tests/i18n-states.json`: append **last** a state that enables bands 1 – 24 through the stub and clicks the plot, so `check-ui-labels` measures the tip in en / fr / zh-Hans at 800 × 500. Bump the probe's state-count pin if it has one.

- **P8 No audio change.** XR-lid sessions stay SHA-256-equal to the 0.5.0 hashes (16 / 16); designer 150 / 0 / 18; render 166 / 0 default and `--long`.

- **P9 Version and docs.** 0.8.0; `CHANGELOG.md` `## [0.8.0]`; ROADMAP 3.2 unchanged.

- **P10 Editor bench timing.** The bench records node-to-curve lag at 60 Hz and `draw()` time in WKWebView on the 94-section draw at 1600 × 1000. Both are recorded, not gated, except the lag row's upper bound (W5).

---

## Tasks

### Wave 0 — pre-flight

**T1. Baselines** [run]
- `git branch --show-current` = main; `git status --short plugins/O-ParametricEQ` clean except `.planning/`.
- Configure `-DOUARICON_BUILD_TESTS=ON`; build both harnesses; default tallies (expect 150 / 0 / 18, 166 / 0). XR-lid hashes recorded (expect 16 / 16 equal to 0.5.0). Log to the scratchpad.
- Run `research-3.2/g9b-bench/g9b.js` against `HEAD`'s page with a scratch dump, `packet` and `raw-mono` modes: reproduce 1.37e-8 dB and the 59.5 dB mono error before any edit.
- Copy `execute-3.1/editor-bench/` to `execute-3.2/editor-bench/`; build Release; re-run the 3.1 rows: 13 / 13 on the unmodified editor.
- Depends on: none.

### Wave 1 — the page

**T2. Curve from packets (P3)** [gui-agent]
- `composeSum`, `sumView`, the `active` filter. Grep: no `bandChoice (b, 'placement')` / relay enable or bypass read inside either function.
- Depends on: T1.

**T3. Gain gesture (P5)** [gui-agent]
- `onMove` threshold block and the end path.
- Depends on: T1.

**T4. U5 tip (P7)** [gui-agent]
- `placeTip` refactor, `showPointTip`, the plot handler's refused-create branch, `bands-full` in `i18n.js`, the appended i18n state.
- Depends on: T1.

### Wave 2 — C++

**T5. Editor `eqSections` (P2)** [gui-agent]
- Member, flag, native body, 60 Hz in `pageReady`, `timerCallback`. Comments updated (`PluginEditor.cpp:28-30`, `:393-394`, `PluginEditor.h:36` no longer say "3.2 adds").
- Depends on: T1.

**T6. Version (P9)** [gui-agent]
- `OPEQ_VERSION 0.8.0`; CHANGELOG entry.
- Depends on: T5.

### Wave 3 — G9 (b) gate

**T7. `--dump-g9` + `tests/g9b/` (P4)** [gui-agent]
- Harness flag from `make_dump_main.py`'s patch; `g9b.mjs` from `g9b.js` with guards and the five controls; `run.sh`; README with the recipe.
- Depends on: T2 (the runner composes per P3).

### Wave 4 — build, gates, install, validators

**T8. Build and run** [run] — long commands in the background, each with its own log (600 s watchdog).
1. `ninja OuariconParametricEQ_VST3 OuariconParametricEQ_AU OuariconParametricEQ_Standalone` → no new warnings in the plugin's TUs.
2. Render harness default → 166 / 0; `--long` → 0 FAIL. Designer → 150 / 0 / 18. XR-lid 16 / 16 SHA-equal.
3. `tests/g9b/run.sh` → PASS: worst ≤ 0.01 dB (expect ~1e-8), scored ≥ 100 000, every rate and mono present, 5 / 5 controls > 0.05 dB (control 5 as P4: unchanged after P3, > 0.05 dB on `HEAD`'s `composeSum`).
4. UI gates from the repo root, exit codes recorded: `check-i18n` 0, `i18n-fr-lint` 0, `i18n-zh-lint` 0, `check-ui-labels` 0, `boot-all-uis --strict-tips` 0 with 0 DEAD. **77 is not a pass.**
5. Greps: native diff identical, 19 / 19; `git diff HEAD -- Source/PluginProcessor.* Source/dsp Source/analysis` empty; `OPEQ_EDITOR_BENCH` absent from the plugin's compile commands; `Source/ui/public/modules/*.js` untracked; no `bands-full` English literal in `index.html`.
- **On a failure:** a harness row red → STOP (3.2 changes no DSP). G9 (b) red → compare to T1's research reproduction before touching the page. A gate red → fix the page / i18n / fixture, never the gate script; re-run all five.
- Depends on: T2 – T7.

**T9. Install and validate** [run]
1. `./scripts/build-and-install.sh O-ParametricEQ` → rc 0, two `-dev` bundles, 0.8.0, no alternate-variant orphan.
2. pluginval strictness 10, `--timeout-ms 600000`: VST3 ×2, AU ×1 → SUCCESS.
3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`. Never `auval -a`.
4. Standalone: curve drawn on open (screenshot to the scratchpad), page console 0 errors.
- Depends on: T8.

### Wave 5 — editor bench rows (real editor, real page)

**T10. Bench** [run; gui-agent for the bench code]
- `execute-3.2/editor-bench/` from T1, re-pointed at the edited editor and page. Events are JS-dispatched `PointerEvent` / `WheelEvent` / `MouseEvent ('dblclick')` / `KeyboardEvent`. A bench listener counts `parameterGestureChanged` begin / end per parameter and `eqSections` events.
- 3.1 rows: all 13 re-run, all pass (E-idle now: 0 relay messages and 0 `eqSections` after the initial push).
- New rows:
  - **E-eq-init:** after `pageReady` the first `eqSections` carries 24 bands, `band` 1 … 24, `sampleRate` / `stereo` / `rateKnown` present. A preset load by each of the four 3.1 paths → the next push carries the changed bands and the painted curve matches (pixel probe at three x).
  - **E-eq-idle:** 10 s idle → 0 `eqSections` after the initial push.
  - **E-stopped (ROADMAP: curve updates while the transport is stopped):** processor prepared, no `processBlock`; host `setValueNotifyingHost` on `b03_freq` → an `eqSections` event for band 3 within 2 ticks, and the painted curve changes (pixel probe).
  - **E-mono:** processor prepared mono, bands set Left / Right → packets carry Stereo and the painted curve equals the Stereo-placement curve (pixel probe); the node badge still shows the relay placement.
  - **E-create (UI-01):** a click at (x, y) on the empty plot enables the first free band with freq = `xf (x)`, gain = `round (yg (y) / scale, 0.1)`, Bell / Stereo / not bypassed / default Q, slope unchanged; host values match. With 1 – 23 enabled → band 24.
  - **E-full (U5):** with 24 enabled, a plot click changes no parameter (0 gestures), the tip shows `tr('bands-full')` at the pointer inside the viewport, hides on the next `pointerdown`, shows with hover help off; in fr and zh-Hans the text is that language's entry.
  - **E-drag:** a Bell drag → exactly one begin / end pair on `freq` and one on `gain`, values follow the pointer. A Low Cut drag → one pair on `freq`, **zero** on `gain`. A Bell at `gain_scale` 0 % → zero on `gain`. Cancel and lost capture also close every opened pair.
  - **E-wheel:** a burst of 10 wheel events → one pair on `q`, value moved in the log domain; Shift is finer.
  - **E-dblclick (VERIFICATION-3.1 Issue 1):** on a node → `gain` equals its default within the float step; on each strip field (freq, gain, Q) and each knob (`output_gain`, `gain_scale`) → host value equals `getDefaultValue()`; a node created within 450 ms is not reset.
  - **E-delete:** Delete key and the strip button → `enabled` off, one gesture, the nearest band selected.
  - **E-ends (UI-04, DSP-06):** typed "10", "30k", "30000" (freq), "-30", "30" (gain), "0.025", "40" (Q), and "5", "99k", "-99", "0", "1e9" → host raw value **equals** the range start / end (exact, not within tolerance). For each field, both ends.
  - **E-cycle-dbg (VERIFICATION-3.1 Issue 3):** 10 open / close cycles in Debug with `eqSections` live → 0 assertions, no leak report.
  - **E-lag (P10):** scripted 2 s Bell drag at 60 Hz pointer rate → relay write → `eqSections` → painted, median / p99 recorded; p99 ≤ 2 ticks + 1 frame (≈ 50 ms).
  - **E-draw (P10):** `draw()` time on the 94-section draw at 1600 × 1000, median / p99 in WKWebView (V8 was 3.83 / 5.10 ms). Recorded for 3.3.
- **Mutant control** (`execute-3.2/mutants/`, each must fail its row; restore and rebuild after each):
  - E15 `getParameterDefaults` returns normalised → E-dblclick.
  - `onUp` skips one `sliderDragEnded` → E-drag.
  - P5 reverted (gain gesture always opened) → E-drag (Low Cut).
  - P3 reverted (`composeSum` reads relay placement) → E-mono and G9 (b) control 5.
  - `fullPushPending` never set by `requestEqSections` → E-eq-init (preset path).
  - `timerCallback` emits on an empty list → E-eq-idle.
  - Refused-create branch removed → E-full.
- **On a failure:** fix the editor / page, re-run W4 from T8 step 1.
- Depends on: T9.

### Wave 6 — bookkeeping + commit

**T11. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/3-gui/SUMMARY-3.2.md`: tallies; G9 (b) worst per rate, scored count, control margins; the five gate exit codes; validators; every E-row with figures; the mutant table; deviations (every page edit beyond P3 / P5 / P7 with its reason).
- Carry-forward:
  - **For verify 3.2 (hands-on, Taylor; Logic AU and one VST3 host):** Touch mode, Bell node drag → `freq` and `gain` lanes record together, one ⌘Z undoes it; Low Cut drag → only `freq` records; a wheel burst → one Q stretch; click-to-create in Latch writes the new band; curve follows automation with the transport stopped; a mono track with Left / Right bands draws the Stereo curve; the 25th-band tip in all three languages. Plus 3.1's owed hands-on items.
  - **For 3.3:** the timer also drives `SpectrumAnalyzer::process` (measured dt; analyzer every second tick if needed); per-band cache only if the 60 fps row needs it (E-draw figure); listen on drag hooks into P5's drag start / end.
- `PLUGINS.md` version cell → 0.8.0 (check `git diff PLUGINS.md` first; other sessions' rows → temp-index recipe or leave out and say so).
- STATUS: `status: stage_3_phase_3_2_executed`, `next_action: stage_3_verify_3_2`.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` new files by name (`tests/g9b/*`, `.planning/stages/3-gui/SUMMARY-3.2.md`, `execute-3.2/` minus build dirs), then `git commit -- plugins/O-ParametricEQ` (+ `PLUGINS.md` if clean). Never `-A` / `-a`. `git log -1 --stat` lists only these paths; no `Source/ui/public/modules/*.js`. No tag.
- Depends on: T10.

---

## Dependency graph

```
T1 ─┬→ T2 ──┬→ T7 ───────────┐
    ├→ T3 ──┤                │
    ├→ T4 ──┤                │
    └→ T5 → T6 ──────────────┤
                             ↓
           T8 (build, harnesses, G9 (b), gates) → T9 (install, validators) → T10 (bench + mutants) → T11
            ↑                                                                   │
            └──────────────── fix → rebuild → rerun from T8 ←───────────────────┘
```

T2, T3, T4 edit different regions of `index.html`; run them serially in one agent or merge carefully. T5 is independent of the page.

## Success criteria

| Criterion (ROADMAP 3.2; UI-01, UI-02, UI-04, DSP-06) | Proven by |
|---|---|
| G9 (b): JS evaluator vs C++ within 0.01 dB (UI-02) | T8.3 `tests/g9b/run.sh`, guards, 5 / 5 controls |
| Curve updates while the transport is stopped | E-stopped |
| The curve is the engine's on every layout | E-mono, G9 (b) mono set + control 5 |
| Creating a band on empty space sets freq and gain from the click (UI-01) | E-create |
| Typed entry reaches every range end (UI-04, DSP-06) | E-ends (exact) |
| Drag gestures recorded as automation in a VST3 host and in Logic | E-drag / E-wheel / E-delete gesture counts in-process; Logic + VST3 host hands-on at verify |
| Double-click resets land on the default | E-dblclick; E15 caught |
| 25th band: keyed tip, three languages, clamp rules | E-full; `check-ui-labels` appended state; `boot-all-uis --strict-tips` |
| `eqSections` only on change, full push on request / preset | E-eq-init, E-eq-idle; mutants |
| No Debug assertion with something emitted | E-cycle-dbg |
| No audio change | XR-lid 16 / 16; 150 / 0 / 18; 166 / 0; processor / DSP diff empty |
| 0.8.0 validates | T9 |

## Risks

- **R1 Synthetic events vs real pointer input.** `setPointerCapture` throws on a synthetic pointer id (the page catches it); lost-capture is dispatched by the bench, not by WebKit. The in-process gesture rows are strong evidence; Logic recording stays a hands-on row.
- **R2 Gate scope.** The research found two page bugs that no existing row saw. The G9 (b) and E-mono rows must exercise mono and the relay ≠ packet case, or the gate inherits the research's scope and prints green over a regression (memory: `pattern_gate_inherits_the_review_findings_scope`). Control 5 is that check.
- **R3 Lifted-function drift.** `g9b.mjs` lifts by name from the shipping page. A rename makes it find 0 or 2 copies; the "exactly once" guard turns that into a FAIL, not a skip.
- **R4 Tip dismiss.** A dismisser on `mousedown` dies under the page's `pointerdown` `preventDefault` (memory). Use a capture-phase `pointerdown`.
- **R5 Tip clamp is viewport-sensitive.** A click near the right or bottom edge at 800 × 500 must keep the tip inside; the appended i18n state clicks near the plot's right edge, and E-full checks the rect.
- **R6 i18n state order.** Append last; `check-ui-labels` flips toggles after each state (memory).
- **R7 Shared checkout.** Other sessions have uncommitted files (`modules/registry.yaml`, `.planning/STATE.md`, agent memory, other plugins). Path-scope the commit and re-check immediately before it.
