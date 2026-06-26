# Stage 3 (GUI) — RESEARCH

> Express mode, non-interactive. Patterns verified against the in-repo sibling
> implementations (O-simpleFM, O-simpleSubtractive) and the project memory's
> critical WebView notes. No external research needed — every pattern below is
> already shipping in a sibling.

## JUCE 8 WebView bridge (verified in siblings + memory)

- **Two namespaces, do not confuse them** (memory: *JUCE WebView namespace vs
  postMessage*): `import * as Juce from "./juce/index.js"` exposes
  `getSliderState`, `getComboBoxState`, `getToggleState`, `getNativeFunction`.
  `window.__JUCE__.backend.addEventListener(name, cb)` is the low-level channel for
  **C++→JS pushed events** only. Native fns are `Juce.getNativeFunction(...)`, NOT
  `window.__JUCE__`.
- **State objects:**
  - slider: `getNormalisedValue()`, `setNormalisedValue(n)`, `getScaledValue()`,
    `sliderDragStarted()/Ended()`, `valueChangedEvent`/`propertiesChangedEvent`.
  - combo: `properties.choices`, `getChoiceIndex()`, `setChoiceIndex(i)`,
    `valueChangedEvent`/`propertiesChangedEvent` (build `<option>`s on BOTH or the
    select renders empty on first load).
  - toggle: `getValue()`, `setValue(bool)`, `valueChangedEvent`.
- **Editor member order is load-bearing** (memory + O-simpleFM header): relays →
  WebView → attachments (destroyed in reverse → WebView still alive when attachments
  die). `WebSliderParameterAttachment` is 3-arg in JUCE 8 (`*param, *relay, nullptr`).
- **Resource provider receives a BARE PATH** (memory): compare `url == "/" ||
  url == "/index.html"` etc.; never strip scheme/host. Serve `text/...; charset=utf-8`.
- **Cross-platform:** CMake already carries `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`,
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`
  (set at Foundation). Add `withWinWebView2Options(...withUserDataFolder(tempDir))`
  under `#if JUCE_WINDOWS` (memory: DAW hosts deny the default WebView2 data folder →
  silent IE fallback → blank page).
- **Native-fn bridge gaps fail silently** (memory: *webview native-fn bridge gap*):
  every `Juce.getNativeFunction(name)` in JS must have a matching
  `.withNativeFunction(name, ...)` in C++. Grep-diff before building.

## Binary-data namespace

- The plugin has **no embedded-sample** binary target (voices are synthesised), so the
  WebView `juce_add_binary_data(O-simpleBeatmaker_UIResources …)` is the **only**
  binary-data target → default `BinaryData` namespace is correct. The O-simpleGrain
  dual-namespace collision only applies when a *second* target exists. (ROADMAP note.)

## Reused module files (copied verbatim from a sibling)

- `js/juce/index.js` and `js/juce/check_native_interop.js` — the JUCE-emitted ES
  module + interop guard. Copied from O-simpleSubtractive (identical across siblings).

## Grid ↔ DSP contract (this plugin's new surface)

- Native fns to register (C++ ↔ message thread):
  - `setStep(voice, step, velocity)` → `processor.setStep(...)`; returns nothing.
  - `getGrid()` → returns a flat `6×32` velocity array (var array) for paint-on-load
    and after host state restore.
  - `clearGrid()` → `processor.clearGrid()` (used by the "clear" affordance).
  - `getSampleRate()` → for the tempo-normalised timing lane scale (BPM arrives live
    on the per-frame event below, so no separate `getBpm` native fn is needed).
- Pushed event (C++→JS, on the 60 Hz editor Timer that drains `VizAnalyzer`):
  - **As built, consolidated into ONE `frame` event** `{ph, bpm, sync, hits[]}` (one
    emit/tick, atomic) rather than the separate `playhead`/`triggers` events first
    sketched here. `ph` = fractional step index (`getPlayheadStepPhase()`) for the
    sweep; `bpm`/`sync` = the advisory transport taps; `hits` = array of
    `{v, s, vel, src, d}` drained this frame, `d = appliedSampleInBar −
    nominalSampleInBar`. The one stream feeds the grid flash, the timing lane, AND
    the MIDI readout (the "two views of one MIDI stream" invariant).

## Pitfalls carried in from memory / siblings

- Canvas is a CSS **replaced element**: size the backing store with
  `canvas.width = clientWidth * dpr` + `ctx.setTransform(dpr,…)` (memory: O-TextureForge
  cursor/Retina bug) — reused `makeCanvas()` helper from the sibling.
- A JS `ReferenceError` on module load silently kills the whole WebView UI but passes
  C++ build/pluginval (memory: *module extraction regression check*) — keep `app.js`
  self-contained, guard every `getElementById`.
- Host-only stale behaviour after install-while-DAW-open is a cached instance, not a
  code bug (memory) — quit/reopen host when smoke-testing.
