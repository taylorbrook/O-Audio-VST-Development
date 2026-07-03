# Stage 3: GUI — Context

## Discussion Summary

**Date:** 2026-06-26
**Participants:** User, Claude
**Stage entry state:** Stage 2 (DSP) COMPLETE — render-harness gate ALL 9 PASS; 21 params frozen; build clean (VST3+AU+Standalone); auval/pluginval@5 SUCCESS; installed. All GUI consumption hooks already exposed by the DSP layer (`getDisplayCutoffHz`, `getDisplayK`, `getVizRing`, `getDisplayPlayhead`, snap atomics). **No UI directory or mockup exists yet — Stage 3 builds the WebView from scratch.**

## Requirements Confirmed

Stage 3 delivers the WebView GUI per ROADMAP Phases 3.1–3.3:
- **3.1** — Single-page projector-readable signal-path layout; all 21 controls two-way bound; cross-platform (macOS VST3+AU + Windows VST3); load-your-own (drag-drop + file picker). (FUNC-03, UI-05, COMPAT-02)
- **3.2** — Interactive waveform editor (draggable start/end + shaded loop region + root-key indicator + live playhead), Repitch-vs-Stretch visible behaviour, filter-response curve, amp-ADSR animation. (UI-01/02/03, QUAL-02)
- **3.3** — On-hover pedagogical tooltips on every control; preset-tour UI hook (content lands Stage 4). (UI-04, FUNC-07)

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **UI design approach** | **Clone O-simpleGrain look, build direct** — no separate mockup cycle | O-simpleGrain is already the code template (drop module reuse); cloning its WebView CSS/layout gives instant sibling-consistency and the fastest path to a working UI. Adapt its signal-path layout to the sampler's groups. Design iteration happens live in the build. (Overrides the BRIEF's "UI design in a mockup phase" note — user elected build-direct.) |
| **Execution batching** | **Checkpoint after Phase 3.1** — STOP for human DAW A/B + visual review before 3.2 | Builds the playable shell first (21 controls bound + load-your-own), then batches the **deferred Stage-2 human DAW gate** (loop @ 0/10/100 ms by ear, Repitch↔Stretch A/B, Vintage/filter feel) plus a layout/feel review BEFORE the expensive waveform-editor canvas work. Mirrors the Stage-2 2.1 checkpoint pattern; catches layout/feel issues before 3.2 builds on top. |
| **Built-in sample set** | **Piano-only now + load-your-own** — curated found-sounds deferred to Stage 4 | Matches the existing Stage-2 decision (`builtins: piano_only_for_now`). Selector shows the single piano built-in; drag-drop + file-picker covers the in-class "load one found sound" activity. Sourcing/embedding the curated set (vocal fragment, instrument hit, found texture, percussive) is Stage-4 content work. |

## Constraints Identified

**Clone source (load-bearing):** `plugins/O-simpleGrain/Source/ui/public/` — `index.html`, `css/styles.css`, `js/app.js`, `modules/webview-drop-streaming.js`. The drop module is reused directly; CSS/layout is the visual base.

**Project-standard WebView gotchas (must carry over — from memory + ROADMAP):**
- PluginEditor member order: **relays → WebView → attachments**. `WebSliderRelay`/`WebComboBoxRelay`/`WebToggleButtonRelay` + matching 3-arg attachments (`nullptr` undoManager). `getComboBoxState` for `sourceSample`/`loopMode`/`pitchMode`; `getToggleState` for `reverse`.
- Resource provider receives **bare paths** — compare with direct equality (`url == "/"`), do NOT strip scheme/host.
- Pass the **`Juce` ES-module namespace** (not `window.__JUCE__`) to any shared panel/module that calls `getNativeFunction` (silent-TypeError trap).
- Drag-drop: `webkitGetAsEntry()` content-streaming; **`juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`** (non-standard format trap). Truncate >30 s sources + notice.
- Canvas: explicit `width: calc(...)`/DPR-aware backing store (`canvas.width = clientWidth*dpr`) — replaced-element gotcha; crisp on Retina.
- Cross-platform: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`; Windows `withUserDataFolder(tempDir)`.
- **Dual binary-data NAMESPACE:** embedded piano sample (`BinaryData`) + WebView UI resources (`UIBinaryData`/distinct) — give the 2nd target a distinct `NAMESPACE` to avoid the O-simpleGrain duplicate-symbol collision.
- After WebView editor lands, **re-run the render-harness at the START of Stage 4** — it compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0` and silently breaks when the editor gains WebView types (guard `createEditor` with `#if JUCE_WEB_BROWSER` + drop `PluginEditor.cpp` from harness sources).

**DSP hooks already available (no DSP changes needed for viz):**
- `getDisplayCutoffHz()` / `getDisplayK()` — closed-form filter curve (k = 1/Q = JUCE R2 → `SubFilterCurve::magnitudeDb`).
- `getDisplayPlayhead()` — lead-voice normalized playhead (drives the waveform playhead + Repitch/Stretch indicator).
- `getVizRing()` — lock-free output scope/spectrum tap (editor runs the FFT; copy window BEFORE FFT).
- Snap atomics (`pendingSnap`, `snapRegionStart/End`, `snapLoopStart/End`) — zero-crossing snap already wired in DSP for waveform-editor handles.

## Open Questions (for research/plan)

- Waveform peak-bin generation: push once on load via native fn vs. computed editor-side — and at what bin resolution for projector readability.
- Start/end + loop handle drag → param mapping precision; whether the UI surfaces the existing zero-cross snap as a visible affordance or applies it silently.
- Repitch-vs-Stretch indicator form: a text/label readout vs. relying on playhead motion alone (ROADMAP allows either).
- Whether to ship the optional output scope/spectrum in 3.2 or defer to Stage 4.
- Preset-tour hook UI affordance (dropdown vs. named buttons) — hook only in 3.3; content in Stage 4.

## Next Phase

Ready for: **research** phase (investigate the O-simpleGrain clone adaptation + waveform-editor canvas approach), or **plan** phase directly (ROADMAP already specifies 3.1–3.3 in detail).
