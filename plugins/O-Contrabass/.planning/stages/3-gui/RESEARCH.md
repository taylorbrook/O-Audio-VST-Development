# Stage 3: GUI — Research

**Date:** 2026-07-10
**Depth:** DEEP (tier 6) — 4 parallel investigations: O-Bowed reference UI, O-Contrabass DSP feed points, finalized mockup v1 artifacts, shared-module survey
**Input:** CONTEXT.md rev-1, mockup v1 (finalized, commit 19f51d9), ROADMAP Phases 3.1–3.3

---

## Executive Summary

The finalized mockup v1 is unusually complete: all **31 parameter bindings verified 1:1** against parameter-spec.md (29 WebSliderRelay + 1 WebComboBoxRelay + 1 WebToggleButtonRelay), editor size settled at **1000×650 fixed**, and the three visualizations are already implemented in JS — only the **VU level feed is fake** (constant −20 dB placeholder). Stage 3 execution is primarily: harness protection → file copy → 3 real data feeds → preset bar wiring → validation gates.

Two discrepancies found that the plan must fix:
1. **Mockup body-spectrum mode table ≠ DSP truth** — JS uses `[58,74,98,132,178,238,320,430]` Hz; `BodyResonator.h:65-70` is `{60,98,115,175,235,340,700,1200}` Hz with per-mode Q `{14,11,9,8,7,6,5,2.5}` and gains `{−2,0,−1,−3,−4,−5,−7,−6}` dB.
2. **preset-manager module is v1.0.4** (module.yaml authoritative), not the v1.0.2 in CONTEXT/registry.yaml — v1.0.3 added the `applyPresetJson` reset-to-defaults fix and factory-version sentinel; v1.0.4 sanitizes factory preset names.

---

## Answers to CONTEXT.md Open Questions

### Q1: C++→JS eventing — how does O-Bowed feed visuals, and what should O-Contrabass use?

**O-Bowed's pattern is the OLD one — do not copy it.** O-Bowed has no editor Timer and no event push; its JS polls a `getVisualizationState` native fn at ~15 Hz (`O-Bowed/Resources/ui/index.html:1750`), reading APVTS atomics only (no audio-thread data).

**The fleet-standard (newer) pattern is editor `juce::Timer` @ 30 Hz + `emitEventIfBrowserIsVisible`**, and the mockup scaffolding already implements it (`v1-PluginEditor.cpp:230, 267-268`, `startTimerHz(30)` emitting `vuLevel`). Precedents:
- **Simplest (matches our VU need):** O-AnalogEQ — `std::atomic<float> outputLevelDB` stored in processBlock (`O-AnalogEQ/Source/PluginProcessor.cpp:372`), 30 Hz timer emits, JS eases needle in rAF (`PluginEditor.cpp:333-338`, `index.html:896-914`).
- **Structured snapshots:** O-GrainScatter — `TripleBuffer<Snapshot>` published at end of processBlock, timer early-outs when `!webView->isShowing()`, compact-key JSON payloads (`O-GrainScatter/Source/PluginEditor.cpp:212-257`, `dsp/TripleBuffer.h`).

**Recommendation:** keep the mockup's 30 Hz timer; add atomic taps on the processor/voice (no TripleBuffer needed — 3 floats suffice). Payload contract is set by the mockup JS: `vuLevel` = JSON string `{"db": <number>}` (note: O-AnalogEQ emits a bare float — our JS expects JSON, keep the mockup contract).

### Q2: Schelleng wedge math — cheapest correct bounds + what the dot tracks

**Bounds (wedge shape):** compute in JS, no C++ feed. History: Phase 2.3's closed-form wedge math was **removed in Phase 2.4a**, replaced by an empirical 27-point trilinear table (`Source/DSP/SchellengCalibration.h:98-126`) used for sub-harmonic depth safety — not display-suitable. `fMax = 2Z·v_b/(β·Δμ)` survives in code (`Source/DSP/SubHarmonicBias.h:57-62`, Z=0.5); `fMin ∝ 1/β²` exists only as the ARCHITECTURE §499-500 formula, and its scale constant R was deleted with the Phase 2.3 code. The mockup's stylized JS bounds (`p_max = 20·v·(0.10/β)`, `p_min = 2·v·(0.10/β)²`, `v1-ui.html:1182-1183`) encode the correct 1/β and 1/β² laws. **Keep them** — the wedge is a musician-facing guide, not a physics readout; the empirical table proves the closed form wasn't trustworthy anyway.

**Dot (operating point):** the mockup dot currently tracks raw knob values via SliderStates. The DSP's *true* effective operating point diverges substantially: Slow LFO scales speed ×(1+0.6·mod) and pressure ×(1+0.5·mod) (`BowedContrabassVoice.cpp:519-520`), Expression Macro adds ×(1+0.4·macro) / ×(1+0.6·macro) (:532-533), then MPE expression/pressure multiply in at :546-556. Since EXPRESSION_MACRO is the featured 70 px knob and the wedge is the UI centerpiece, **recommend adding a `bowState` feed in v1.0**: relaxed-store `effectiveBowSpeed` / `effectiveBowPressure` / `effectivePosition` (β, :878) atomics on the voice — same single-writer pattern as the existing `lastSafeDepth` instrumentation (`BowedContrabassVoice.h:193`) — packed into the existing 30 Hz timer as event `bowState` `{"v":,"p":,"b":,"active":}`. JS falls back to SliderState values when no voice is active (silence = dot rests at knob position). The mockup comments already reserve this event name (`v1-PluginEditor.cpp:272-273`, `v1-ui.html:1169-1171`).

**Gap to resolve in plan:** `getActiveVoice()` returns voice 0 only (`PluginProcessor.cpp:339-344`) but `kNumVoices=4` (one per string). Options: aggregate the loudest/most-recent active voice, or publish from whichever voice is bowing. Cheap resolution: per-voice atomics + processor picks the most recently started active voice.

### Q3: Body spectrum — read coefficients or recompute?

**Recompute in JS — zero data feed needed.** `BodyResonator::recomputeCoefficients()` (`BodyResonator.cpp:75-108`) derives everything from the two params with closed-form math trivially mirrored in JS:
- `sizeScalar = 0.85 + 0.30·size` → `fc = defaultFreq / sizeScalar` (clamped 20 Hz–0.45·sr)
- `qScalar = max(0.15, 1 − 0.85·damping)` → `qEff = max(0.10, defaultQ·qScalar)`
- `gDb = defaultGainDb + 1.5·(size − 0.75)`

Coefficients are opaque biquads recomputed on the audio thread — reading them would be both racy and pointless. **Plan action:** replace the mockup's placeholder mode table/formulas with the exact `kDefaultFreq/kDefaultQ/kDefaultGainDb` tables + formulas above (discrepancy #1). The mockup's dirty-flag redraw (only on param change) is already the right update model — spectrum is param-deterministic, not signal-reactive.

### Q4: Preset browser — adopt now or defer?

**Adopt in Stage 3 (Phase 3.2), the O-Wind way.** ROADMAP 3.2 lists "preset selection menu"; the mockup ships a placeholder preset bar (`v1-ui.html:1499-1505` logs to console only). Wiring the *mechanism* now means Stage 4.1 only authors the 10 factory presets.

- **Version: v1.0.4** (`modules/persistence/preset-manager/module.yaml:5`, 2026-07-10). Includes the applyPresetJson reset-to-defaults fix (WR-01), factory-version sentinel (WR-04), and factory-name sanitization (IN-17).
- **Consumption style: CMake include of the canonical module dir** — `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp` (O-Wind pattern, `O-Wind/CMakeLists.txt:57`) — the only style that stays current automatically. All 7 vendored-copy plugins have drifted (O-Bells is 662 diff lines from canonical).
- **C++:** `OuariconPresetManager presetManager { apvts, "O-Contrabass" }` on the processor + `CustomSaveCallback`/`CustomLoadCallback` for tuning state (Scala file path / custom intervals must survive preset round-trips).
- **JS:** decide in plan — canonical `js/preset-manager.js` module (requires 10 native fns: save/savePresetWithDialog/load/loadPresetFromFile/getPresetList/getCurrentPreset/selectNext/selectPrevious/delete/isFactoryPreset; constructor takes the `Juce.getNativeFunction` factory + `onConfirmDelete` hook — `window.confirm` is dead in WKWebViews) **or** O-Bowed's lighter hand-rolled bar calling 8 fns directly. The mockup bar has prev/next/name/Save — the canonical module is the better fit for staying-current; reference registrations at `O-Wind/Source/PluginEditor.cpp:191-230`.
- **Caveats:** preset names must never contain "/" (filename verbatim); presets live in `~/Library/<PluginName>/Presets/` — do not relocate without migration.

### Q5: Editor dimensions

**Settled: 1000×650, fixed, non-resizable** (`v1-PluginEditor.cpp:233-234`, yaml:17-20, checklist header). No `vh`/`vw` anywhere (mockup constraint honored). Grid: 42 px header / two 252 px rows / 64 px Microtonal footer strip.

---

## Integration Architecture (from verified mockup scaffolding)

The mockup ships production-grade scaffolding — **Stage 3 execute is a merge, not a rewrite:**

| Artifact | Disposition |
|---|---|
| `v1-ui.html` (1510 lines, self-contained) | → `Source/ui/public/index.html` verbatim, then fix body-mode table (Q3) |
| `v1-PluginEditor.{h,cpp}` | → `Source/PluginEditor.{h,cpp}` — 31 relays/attachments, correct member order (relays → webView → attachments; attachments call `evaluateJavascript()` in dtors), resource provider with bare-path equality + MIME types, 30 Hz timer, SafePointer FileChooser |
| `v1-CMakeLists.txt` | Merge snippet only: `juce_add_binary_data(OContrabass_UIResources ...)` — default `BinaryData` namespace is fine (single binary-data target); WebView flags **already present** in the real CMakeLists since Stage 1 (`NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` at :19-20, :81-82) — do not duplicate |
| `v1-integration-checklist.md` | The execute-phase playbook; §0 (harness) is DO-FIRST |
| JUCE frontend | Copy 8.0.9 `index.js` + `check_native_interop.js` → `Source/ui/public/js/juce/` |

**Native-fn surface (mockup):** only 2 — `getParameterDefaults` (skew-correct dblclick reset, already scaffolded) and `openTuningFilePicker`. Grows to ~12 with preset-manager adoption. Run the grep-diff bridge gate (checklist §4) after every addition.

**Render harness (DO FIRST — regression bar):** `tests/render-harness/CMakeLists.txt:26` compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0` — the known trap. At execute start: guard `createEditor()` (`PluginProcessor.cpp:332-335`, currently unguarded) with `#if JUCE_WEB_BROWSER`, drop PluginEditor.cpp from harness sources, rebuild, **re-run 19-entry `reproduce-goldens.sh` byte-identical** — and again before verify. New atomic taps are read-only and goldens-safe; never insert/reorder arithmetic in the signal path.

## The Three Data Feeds (new C++ work)

1. **`vuLevel` (real):** `std::atomic<float>` RMS(dB) on the processor, stored at end of processBlock **after the output-gain loop** (`PluginProcessor.cpp:315` — the true final signal; saturator → width → limiter → gain). Replace the placeholder emit (`v1-PluginEditor.cpp:266`, constant −20). Keep JSON `{"db":N}` payload. Meter chain-position matters: post-limiter is what makes it "genuinely useful" per CONTEXT.
2. **`bowState` (recommended, Q2):** 3 relaxed atomics per voice (effective speed/pressure/β) + active flag; emitted from the same 30 Hz timer. Resolve the 4-voice selection question in plan.
3. **`bodyModes`: NOT needed** (Q3) — JS recompute; delete the reserved event or leave the comment.

Timer hygiene: early-out `!webView->isShowing()` (O-GrainScatter IN-15 pattern) on top of `emitEventIfBrowserIsVisible`.

## Tuning Integration (scoped to mockup design)

The finalized mockup's Microtonal section is a **compact footer strip** (Reference Pitch mini-knob, Tuning System dropdown, Load .scl/.tun button, NE toggle) — **not** the full shared `tuning-panel.js` overlay used by O-Bowed/O-Wind. The design gate is passed; keep the strip for v1.0 and log the full tuning-panel overlay (20 native fns, canonical v3.0.0 at `modules/tuning/scala-tuning-engine/js/tuning-panel.js`) as a v1.1 option.

Stage-3 tuning work is therefore only:
- Implement `openTuningFilePicker` → `.scl` via `TuningEngine::loadScalaFile` (scala-tuning-engine C++ already CMake-wired since Stage 1).
- **`.tun` route is an open TODO** (checklist §9, `v1-PluginEditor.cpp:346-348`): the picker accepts `*.tun` but no loader exists. Plan must check whether TuningEngine 2.1.0 parses TUN; if not, either implement the parse or restrict the picker filter to `*.scl` for v1.0 (TUNING_SYSTEM choice label says "Scala/TUN" — a dead .tun path is a silent-failure trap).
- SafePointer + bare-return-on-null already correct in scaffolding.
- If the full panel is ever adopted: pass the **`Juce` ES-module namespace**, never `window.__JUCE__` (tuning-panel.js:17-28 docstring, corrected in O-Wind v1.16.1).

## Validation Tooling

- **`ui_frontend_check.js`** — exists ONLY at `backups/O-MicrotonalSampler/v1.23.7/Source/tests/ui_frontend_check.js` (live O-MicrotonalSampler tree is v1.23.3 without it). Port as a template: checks 1 (node --check syntax), 2 (bridge-gap grep-diff), 6 (dblclick uses paramDefaults), 8 (no window.confirm) are generic; the rest are hardcoded to sampler-app.js names. Adjust paths/regexes for O-Contrabass's registration style.
- **Checklist exit bar (§11):** goldens byte-identical + auval (aumu OCbs OuDv) + pluginval strictness 10 + Logic open/close editor ×10 in Release (destruction-order UAF test).
- **`getScaledValue()` spot-check** on the 6 skewed params: BOW_SPEED, BOW_PRESSURE, BRIGHTNESS, VIBRATO_RATE, SLOW_LFO_RATE, REFERENCE_PITCH (checklist §8).

## Risks & Flags for Plan Phase

| # | Risk / gap | Action |
|---|---|---|
| R1 | Body-mode table mismatch (mockup JS vs BodyResonator truth) | Fix during HTML copy (Phase 3.3 task); test criterion "spectrum reflects Size/Damping" implies DSP-true peaks |
| R2 | `.tun` loader absent while UI advertises it | Decide: implement TUN parse, or restrict filter to `.scl` v1.0 |
| R3 | `getActiveVoice()` hardcodes voice 0; kNumVoices=4 | Per-voice atomics or most-recent-active selection for bowState |
| R4 | **STRING_TENSION has no DSP consumer** (only STRING_STIFFNESS is read, `BowedContrabassVoice.cpp:564`) — the UI will ship a knob that does nothing | Surface to user in plan phase: wire it in DSP (careful — activating a dead param changes default timbre; its default 0.5 is NOT a no-op value) or annotate as known-inert v1.0 |
| R5 | Registry staleness (preset-manager 1.0.2→1.0.4, scala-tuning-engine 2.0.0→2.1.0/js 3.0.0) | Trust per-module module.yaml; consider registry.yaml refresh as a side task |
| R6 | Windows `withUserDataFolder` | Verify present in scaffolding options chain during execute (O-Bowed reference: `PluginEditor.cpp:77-83`) |
| R7 | Mockup-internal inconsistencies (yaml says VU 150×36 / mini-knob 40 px; HTML implements 150×46 / 34 px) | HTML is the production artifact — yaml is descriptive only |

## Recommended Phase Mapping (input to plan)

- **Phase 3.1 (Layout + Bindings):** checklist §0–§6 — harness protection + goldens, file copies, editor merge, CMake merge, resource-provider audit, Debug install, 7 sections render at 1000×650. All 31 bindings land here (scaffolded).
- **Phase 3.2 (Interaction + Presets):** checklist §7–§9 — Release open/close ×10, 31-param binding validation, skew spot-checks, Scala picker (+ R2 decision), preset-manager v1.0.4 adoption (module include + native fns + bar wiring), `ui_frontend_check.js` port.
- **Phase 3.3 (Visualizations):** vuLevel real RMS tap, bowState feed (+ R3), body-mode table fix (R1), CPU check (30 Hz timer + dirty-flag rAF already throttled; test criterion = no spikes, 60 fps max).

## Key References

- O-Bowed editor (relay/attachment order, resource provider, 30 native fns): `plugins/O-Bowed/Source/PluginEditor.cpp`
- O-AnalogEQ meter feed: `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:372`, `PluginEditor.cpp:333-338`
- O-GrainScatter snapshot viz: `plugins/O-GrainScatter/Source/PluginEditor.cpp:212-257`, `Source/dsp/TripleBuffer.h`
- O-Wind module consumption + tuning fns: `plugins/O-Wind/CMakeLists.txt:57,92-93`, `Source/PluginEditor.cpp:191-510`
- Mockup scaffolding: `.planning/mockups/v1-*` (checklist is the execute playbook)
- DSP feed points: `Source/PluginProcessor.cpp:278-315` (output chain), `Source/DSP/BodyResonator.cpp:75-108` (mode math), `BowedContrabassVoice.cpp:519-556,878` (effective bow state)
