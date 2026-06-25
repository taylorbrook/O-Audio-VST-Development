# Stage 3 (GUI) — SUMMARY

**Plugin:** O-simpleSubtractive
**Stage:** 3 of 4 — GUI (WebView UI + parameter binding + headline visuals)
**Mode:** single execute pass (gui-agent) — all 15 tasks T1–T15 implemented
**Date:** 2026-06-25
**Build:** VST3 ✓ + AU ✓ (clean, macOS)

## What was built
The Stage-1 `GenericAudioProcessorEditor` placeholder was replaced with a single-page,
projector-readable Ouaricon-Naturalist WebView UI: a left→right signal-path layout
(OSC | FILTER | FILTER ADSR | AMP ADSR | VOICE/OUT) with all 20 parameters two-way bound,
the headline filter-curve-over-spectrum visual, an oscilloscope, a dual-ADSR display, a live
signal-path SVG diagram, per-control pedagogical tooltips, a preset-tour hook, and an
on-screen keyboard. The Stage-2 DSP→UI contract is consumed read-only on the 30 Hz
message-thread Timer — no DSP was modified.

## Files created
- `Source/ui/public/index.html` — 5-column signal-path layout; 16 knob cells + 4 `<select>` combos; headline + scope + 2 ADSR canvases; routing SVG; preset tour; keyboard. Every cell has `data-tip`.
- `Source/ui/public/css/styles.css` — Naturalist skin (FM base), re-flowed to 5 flex columns; added `.combo`/`.select-cell`/`.select-row`, `.adsr-canvas-wrap`, signal-path node classes. (Botanical overlay + preset-bar/dropdown dropped — not in scope.)
- `Source/ui/public/js/app.js` — full UI controller: knob/combo binding, headline/scope/dual-ADSR renderers, signal-path diagram, tooltips, preset tour, keyboard, viz-event listeners.
- `Source/ui/public/js/juce/index.js` — JUCE JS bridge, copied byte-for-byte from O-simpleFM (17959 B).
- `Source/ui/public/js/juce/check_native_interop.js` — copied byte-for-byte from O-simpleFM (4376 B).

## Files modified
- `Source/PluginEditor.h` — rewritten: derives `AudioProcessorEditor, private Timer`; member order relays → SubVizAnalyzer → WebBrowserComponent → attachments; `std::vector<unique_ptr>` slider+combo relay/attachment arrays (no toggles); `getResource` + `timerCallback`.
- `Source/PluginEditor.cpp` — rewritten: bare-path resource provider; 16 slider + 4 combo relays; options chain `withOptionsFrom` all 20; 3 native fns; `#if JUCE_WINDOWS` user-data-folder block; 3-arg attachments; 30 Hz `timerCallback` emitting 4 viz events.
- `Source/PluginProcessor.h` — T6 stub (FLAGGED): added `void applyFactoryPreset (const juce::String&);` declaration only.
- `Source/PluginProcessor.cpp` — T6 stub (FLAGGED): no-op `applyFactoryPreset` body (`ignoreUnused(name)`). Stage 4 fills the 8 snapshots.
- `CMakeLists.txt` — added single `juce_add_binary_data(O-simpleSubtractive_UIResources ...)` (5 SOURCES) + linked first in PRIVATE. WebView flags already present (confirmed, not duplicated).

## T6 — the ONLY processor change (flagged)
`applyFactoryPreset(const juce::String& name)` added as a wiring-only stub so the WebView
preset-tour native fn has a live bridge target. The `.cpp` body is a no-op
(`juce::ignoreUnused(name)`) with a comment that Stage 4 (FUNC-06) fills the 8 concept
snapshots. Touches no DSP — DSP headers (`OscillatorBank.h`, `SvfZDF.h`, `SubVoice.h`,
`SubVizAnalyzer.h`) and processBlock/voice code have empty diffs; the processor diff is
exactly these two additions. Model: `O-simpleGrain::applyFactoryPreset`.

## Native-function registry (grep-diff verified, both sides identical)
| Name | C++ handler | Purpose |
|------|-------------|---------|
| `uiMidi` | `processorRef.handleUiMidi(int,bool,float)` | on-screen keyboard → synth |
| `getSampleRate` | `complete(processorRef.getCurrentSampleRate())` | headline Nyquist label |
| `applyFactoryPreset` | `processorRef.applyFactoryPreset(args[0])` | preset-tour bridge (T6 stub) |

## Emitted events (grep-diff verified; curve emitted before spectrum each frame)
| Event | Payload | Consumer |
|-------|---------|----------|
| `filterCurveUpdate` | 256 dB floats (`getCurve()`) | headline curve line |
| `spectrumUpdate` | 256 dB floats (`getSpectrum()`) | headline spectrum bars |
| `scopeUpdate` | 128 floats [-1,1] (`getScope()`) | oscilloscope |
| `envUpdate` | `{filterEnv, ampEnv}` (0..1) | dual-ADSR playheads + diagram pulse |

## Requirement satisfaction
- **UI-01:** one `headlineCanvas`; `drawHeadline()` plots spectrum bars then strokes the filter curve on top, both at `x=(i/(n-1))*w` over the same 256-bin log-f axis (bin index == frequency, proven in `SubVizAnalyzer.h`), shared dB window `[-90,+18]` so the resonance peak doesn't flat-top (R2).
- **UI-02:** two canvases (`filterAdsrCanvas` amber/"→ cutoff", `ampAdsrCanvas` green/"→ level"); shapes from each envelope's 4 ADSR scaled values (sqrt-compressed A/D/R x-budget + fixed sustain plateau); redraw on the 8 ADSR `valueChangedEvent`s; vertical-only live playhead (R1).
- **UI-03:** SVG `routingSvg` OSC→FILTER→VCA→♪ + two envelope routes; `updateDiagram()` reflects `oscWave`, `filterType`+`filterSlope`, scales the filter-env arrow by `|filterEnvAmount|` (amber +, cool-blue −), pulses FILTER/VCA with live envs; every `getElementById` `if (el)`-guarded.
- **UI-04:** `drawScope()` (128 pts) from post-filter `getScope()`.
- **UI-05:** `TIPS` map (30 entries — all 20 params + headline/scope/filterAdsr/ampAdsr/routing + 5 lessons) + `setupTooltips()` (pointer + focus a11y + Escape); every `data-tip` key matches a `TIPS` entry (verified).
- **UI-06:** flex-row 5-column layout; frame scrolls; editor 1180×820, resizable.
- **UI-07:** 5 `.tour-btn`s + `#tourCaption`; `setupPresets()` wires each to `applyFactoryPreset` (bridge live; content Stage 4).
- **COMPAT-02:** `NEEDS_WEB_BROWSER/NEEDS_WEBVIEW2 TRUE`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0` (confirmed); `#if JUCE_WINDOWS` `withUserDataFolder(tempDir/"OsimpleSubtractive_WebView")` + status-bar/error-page disabled.
- **QUAL-02:** curve is `updateCurve()`→`getCurve()`, the closed-form of the running SVF — by construction; editor only consumes it.
- **PERF-01:** no audio-thread work added; all FFT/curve/scope on the 30 Hz Timer.

## Pre-execute checklist (a)–(i): all ✓
| # | Check | Result |
|---|-------|--------|
| a | Native-fn parity | ✓ `{uiMidi, getSampleRate, applyFactoryPreset}` both sides |
| b | No module-load ReferenceError | ✓ all 15 `boot()` helpers defined; `node --check` clean |
| c | 20 params relay+attachment | ✓ 16 sliderIds + 4 comboIds (IDs are `ParamIDs` constants → drift = compile error) |
| d | Canvas DPR sizing | ✓ `makeCanvas` backing-store + `setTransform` |
| e | `Juce` namespace not `window.__JUCE__` | ✓ states/native-fns on `Juce.*`; backend only for 4 events |
| f | Resource-provider parity | ✓ 5 paths ↔ 5 BinaryData symbols ↔ 5 SOURCES; bare-path equality |
| g | Event parity | ✓ `{filterCurveUpdate, spectrumUpdate, scopeUpdate, envUpdate}` both sides |
| h | DSP untouched | ✓ DSP headers empty diff; processor diff = only the T6 stub |
| i | Tree builds | ✓ VST3 + AU |

## Deviations from PLAN.md (with rationale)
1. Editor made resizable (`setResizable(true,true)` + `setResizeLimits(820,560,1700,1200)`) — JS `rewireResize()` already re-fits/redraws all four canvases; WebView fills `getLocalBounds()` in `resized()`. Low risk.
2. Routing readout shows cutoff + "Res" (not FM ratio/index — subtractive has no C:M); "Res" not "Q" because `resonance` is a 0–1 control.
3. Preset names are subtractive concepts (Pluck / Sweep Pad / Acid Bass / Self-Oscillation / Brass Stab); three-way name parity (HTML `data-preset` ≡ JS `LESSONS` ≡ future C++ `name==`) in place so Stage 4 only fills C++ bodies.
4. Dual-ADSR x-budget uses sqrt compression of the 0–5 s A/D/R times + a fixed sustain plateau so a 5 ms attack and 5 s release are both legible on a 60 px canvas. Cosmetic; playhead vertical-only.

No other deviations. No new external deps. Single binary-data target → default `BinaryData` namespace (no collision risk).

## Not done in execute (deferred to verify)
Install to system folders, AU cache clear + dual-variant sweep, `auval`, `pluginval`, in-DAW/standalone visual UAT.
