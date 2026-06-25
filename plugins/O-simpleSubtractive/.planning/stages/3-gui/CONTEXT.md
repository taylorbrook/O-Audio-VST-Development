# Stage 3 (GUI) — CONTEXT

**Plugin:** O-simpleSubtractive
**Stage:** 3 of 4 — GUI (WebView UI + parameter binding + headline visuals)
**Mode:** express (CONTEXT auto-compiled from BRIEF.md, ARCHITECTURE.md, parameter-spec-draft.md, REQUIREMENTS.md, ROADMAP.md, and the Stage-2 DSP contract — no interactive session)
**Date:** 2026-06-25

---

## Goal

Turn the audible, validated Stage-2 polyphonic synth into a single-page, projector-readable
WebView teaching instrument: all 20 parameters two-way bound, with the headline
**filter-curve-over-spectrum** visual plus oscilloscope, dual-ADSR display, live signal-path
diagram, and per-control pedagogical tooltips. The pedagogical north star (BRIEF): a curious
student reaches a genuine "oh, *that's* how subtractive works" moment in five minutes — by
**seeing each gesture's consequence** (lower cutoff → harmonics fall under the curve; raise
resonance → peak grows then self-oscillates; steepen slope → curve steepens; route filter env →
brightness moves independently of level).

## Phasing (per ROADMAP — 3 sub-phases)

- **3.1 Layout + Basic Controls + Cross-Platform Wiring** (UI-05/06, COMPAT-02)
  Single-page left→right signal-path layout, all 20 controls bound, cross-platform correct.
- **3.2 Headline Filter-Curve-Over-Spectrum + Oscilloscope + Dual-ADSR** (UI-01/02/04, PERF-01, QUAL-02)
  The teaching visuals, driven by the editor Timer (30 Hz).
- **3.3 Signal-Path Diagram + Tooltips + Preset Tour Hook** (UI-03/07, FUNC-06)
  Pedagogical scaffolding; preset *content* lands in Stage 4 (FUNC-06).

## Requirements in scope

| ID | Requirement | Priority |
|----|-------------|----------|
| UI-01 | Live filter-response-over-spectrum (headline "before/after filter") | **must** |
| UI-02 | Animated dual-ADSR display (filter env → cutoff, amp env → level, independent scales) | should |
| UI-03 | Live signal-path diagram (osc → filter → VCA, envelopes routing), active stage highlighted | should |
| UI-04 | Oscilloscope of output waveform morphing with cutoff/res/envelopes | should |
| UI-05 | On-hover pedagogical tooltips on every control | should |
| UI-06 | Single-page projector-readable left→right signal-path layout (sibling-consistent) | **must** |
| UI-07 | Preset tour selectable from the UI (hook now; content Stage 4) | should |
| COMPAT-02 | Windows WebView2 configured (`NEEDS_WEBVIEW2 TRUE` + static-linking define); renders on Windows | should |
| QUAL-02 | Filter curve + spectrum accurately reflect what is heard | should |

## DSP → UI contract (already built & validated in Stage 2 — do NOT modify DSP)

The processor exposes everything the UI needs; Stage 3 only *reads* it on the message thread.

- **Parameters (20, APVTS, source of truth `PluginProcessor.h` `OSimpleSubtractive::ParamIDs`):**
  - OSC/MIX: `oscWave` (choice: Saw/Square/Triangle/Sine), `subLevel` (0–1), `noiseLevel` (0–1)
  - FILTER: `filterType` (choice: LP/HP/BP/Notch), `filterSlope` (choice: 6/12/24 dB/oct),
    `cutoff` (Hz, log 20–20000), `resonance` (0–1), `filterEnvAmount` (bipolar −1..+1), `keyTrack` (0–1)
  - FILTER ADSR: `filterAttack` `filterDecay` `filterSustain` `filterRelease`
  - AMP ADSR: `ampAttack` `ampDecay` `ampSustain` `ampRelease`
  - VOICE/OUT: `voiceMode` (choice: Poly/Mono/Legato), `glide` (s), `outputLevel` (dB)
- **Visualization tap:** `getVizRing()` (lock-free ring), `getCurrentSampleRate()`,
  `SubVizAnalyzer` (message-thread FFT 4096 / Blackman-Harris → `getSpectrum()` 256 log-f bins;
  scope downsample → `getScope()` 128 pts; `updateCurve(cutoffHz,k,type,slope,sr)` → `getCurve()`
  256 log-f bins via the **closed-form SVF magnitude that equals the running filter** = QUAL-02 by construction).
- **Headline-curve atomics (lead voice):** `getDisplayCutoffHz()`, `getDisplayK()` (k = 1/Q),
  `getDisplayType()`, `getDisplaySlope()`, `getFilterEnvValue()`, `getAmpEnvValue()`.
- **On-screen keyboard:** `handleUiMidi(noteNumber, noteOn, velocity)` — already declared; editor injects from WebView.

## Editor architecture (must follow O-simpleFM sibling pattern exactly)

- Member order in `PluginEditor`: **relays → WebBrowserComponent → attachments** (relays must outlive attachments).
- `WebSliderRelay` / `WebComboBoxRelay` / `WebToggleButtonRelay` + matching **3-arg** attachments (`nullptr` undoManager).
- Resource provider receives **bare paths** (`/`, `/index.html`, `/js/app.js`) — compare by **direct equality**
  (project critical pattern; do NOT strip scheme/host).
- `type="module"` scripts; `import * as Juce from './js/juce/index.js'`; pass **`Juce`** (not `window.__JUCE__`) to any
  helper needing `getNativeFunction` (project memory: namespace-vs-postmessage bug).
- Editor `Timer` @ 30 Hz: copy scope window **before** FFT (in-place transform clobbers buffer) →
  `emitEventIfBrowserIsVisible("scopeUpdate"/"spectrumUpdate"/"filterCurveUpdate"/"envUpdate", ...)`.
- Verify every JS `getNativeFunction(name)` has a matching C++ `withNativeFunction(name,...)` (silent-bridge-gap pattern).

## CMake / cross-platform (COMPAT-02)

`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`;
defines `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`;
Windows `withUserDataFolder(File::tempDirectory child)` (avoid silent IE fallback → blank UI).
**BinaryData:** v1.0 ships ONE `juce_add_binary_data` target (UI resources). If a 2nd target is ever
added (embedded presets), give it a **distinct `NAMESPACE`** (O-simpleGrain Stage 3.1 collision lesson).

## Layout (BRIEF + UI-06)

Left→right signal path, one page, classroom/projector-readable:
**OSC** (wave / sub / noise) | **FILTER** (type / slope / cutoff / res / envAmt / keyTrack) |
**FILTER ADSR** | **AMP ADSR** | **VOICE/OUT** (mode / glide / output), with the headline
filter-curve-over-spectrum as the dominant visual and the live signal-path diagram keeping
osc → filter → amp + the two envelope routes visible at all times.

## Constraints & non-goals

- **Do NOT touch Stage-2 DSP** beyond wiring the editor (the audio→UI contract is frozen & validated).
- Audio thread stays copy-only (PERF-01): all FFT/curve/scope work is on the message-thread Timer.
- Preset **content** (the 8 concept presets, FUNC-06) is Stage 4 — Stage 3 ships only the selectable UI hook.
- Tooltip copy must be plain-language pedagogy (what cutoff does, why resonance whistles, what "poles" mean,
  filter-env vs amp-env) — per BRIEF.

## Sibling references

- **O-simpleFM** (primary template): `Source/ui/public/{index.html,css/styles.css,js/app.js,js/juce/...}`,
  `PluginEditor.{h,cpp}` relay/attachment + Timer + emit pattern, CMake WebView config.
- **O-simpleAdditive**, **O-simpleGrain**: additional WebView synth siblings (binding + viz patterns).

## Success criteria (stage exit)

1. WebView opens; single-page left→right signal-path layout renders, projector-readable (UI-06).
2. All 20 controls two-way bound (drag → DSP; host automation → UI).
3. Sweeping cutoff/res/slope/type/filter-env moves the filter curve over the live spectrum; harmonics above cutoff visibly attenuated; **curve matches what's heard** (QUAL-02); self-osc shows a peak at cutoff.
4. Oscilloscope shows the filtered waveform morphing; dual-ADSR shows the two envelopes moving independently.
5. Live signal-path diagram reflects osc/filter/envelope state; every control has a pedagogical tooltip; preset tour selectable from UI.
6. Builds VST3 + AU on macOS; CMake configured for Windows VST3 (no blank-UI flags missing). pluginval + auval pass.
7. No audio-thread FFT/alloc; UI smooth at 30 Hz.
