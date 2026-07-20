# Stage 3: GUI — Research

**Date:** 2026-06-26
**Plugin:** O-simpleSampler
**Stage entry:** Stage 2 (DSP) COMPLETE — 21 params frozen; build clean (VST3+AU+Standalone); auval/pluginval@5 SUCCESS; render-harness ALL 9 PASS. All viz consumption hooks already exposed by the DSP layer. **No `Source/ui/` directory exists yet.**
**Approach (locked in CONTEXT):** Clone O-simpleGrain WebView, build direct (no separate mockup cycle). Checkpoint after Phase 3.1 for the deferred human DAW A/B + visual review.

> **TL;DR for the planner.** ~80% of Stage 3 is a verbatim/near-verbatim clone of the shipped **O-simpleGrain** WebView (editor C++ wiring, resource provider, relay/attachment binding, DPR-aware canvas, drop streaming, tooltips, preset-tour hook). The **genuinely net-new work** is (a) the **interactive waveform editor** — draggable start/end + loop handles over the thumbnail (grain's is read-only), and (b) a **batch of processor-side native-fn hooks** the grain processor has but the sampler does not yet (`getSourceThumbnail`, `dropSample*` commit path, file-picker, `wasLastLoadTruncated`, `applyFactoryPreset`, and — if we add an on-screen keyboard — `handleUiMidi`). Every DSP→UI viz atomic the editor needs (`getDisplayPlayhead`, `getDisplayCutoffHz`/`getDisplayK`, `getVizRing`, `getCurrentSampleRate`) **already exists**.

---

## 1. Clone source map (load-bearing reference files)

All paths under `plugins/O-simpleGrain/`:

| File | Role | Sampler reuse |
|------|------|---------------|
| `Source/PluginEditor.cpp` | WebView wiring: resource provider, relays→WebView→attachments, native fns, 30 Hz Timer | **Adapt** — swap the 19→21 param ID lists, swap viz Timer body |
| `Source/PluginEditor.h` | Member-order contract (relays / WebView / attachments) + `vizAnalyzer` + `fileChooser` | **Adapt** — same structure, sampler IDs |
| `Source/ui/public/index.html` | DOM contract: `knob-<id>` / `combo-<id>` / `toggle-<id>` / `<canvas>` ids / `data-tip` / `data-preset` | **Adapt** — sampler groups + the waveform-editor canvas |
| `Source/ui/public/css/styles.css` | Visual base (Ouaricon field-guide aesthetic) | **Clone**, retheme to sampler signal-path groups |
| `Source/ui/public/js/app.js` (1022 lines) | Knob/combo/toggle binding, drop wiring, canvas helpers, tooltips, preset tour, on-screen keyboard | **Adapt** — reuse binding/canvas/drop/tooltip helpers verbatim; rewrite the waveform draw as interactive |
| `Source/ui/public/js/juce/index.js` + `check_native_interop.js` | JUCE 8 ES-module interop | **Copy verbatim** |
| `Source/ui/public/modules/webview-drop-streaming.js` | macOS content-streaming drag-drop (`readFileEntryAsBase64`) | **Reuse via `ouaricon_add_module`** (do NOT hand-copy) |
| `CMakeLists.txt` lines 52–122 | `ouaricon_add_module` + dual-NAMESPACE `juce_add_binary_data` + link order | **Adapt** — `O-simpleSampler_UIResources` (NAMESPACE `UIBinaryData`) |

---

## 2. Phase 3.1 — Layout + Controls + Cross-Platform Wiring + Sample Loading

### 2.1 Editor member order (CRITICAL — release-build crash if wrong)

`PluginEditor.h` declares, in this order (C++ destroys in reverse):
1. **Relays** (declared first → destroyed last)
2. **WebView** (`std::unique_ptr<juce::WebBrowserComponent>`)
3. **Attachments** (declared last → destroyed first, while WebView still alive)

Wrong order = an attachment outlives the WebView and calls into a freed component on plugin reload. Grain's header (`PluginEditor.h:60–73`) is the exact template.

### 2.2 The 21-parameter binding map (sampler-specific)

Drives the relay/attachment ID lists in `PluginEditor.cpp` and the DOM `knob-`/`combo-`/`toggle-` ids in `index.html`. Source: `parameter-spec.md`.

**Sliders (16)** — `WebSliderRelay` + `WebSliderParameterAttachment`:
`start`, `end`, `loopStart`, `loopEnd`, `loopCrossfade`, `tune`, `fine`, `vintage`, `filterCutoff`, `filterResonance`, `ampAttack`, `ampDecay`, `ampSustain`, `ampRelease`, `velToAmp`, `outputLevel`

> NB `rootKey` (Int 0–127) and `tune` (Int −24..+24) are integer params but still bind as **sliders** via `WebSliderRelay` (the relay carries the param's range/step; JS reads `st.getScaledValue()` for the label). That makes **17 slider relays**, not 16 — recount below.

**Corrected slider list (17)** — all Float + Int continuous params:
`start`, `end`, `loopStart`, `loopEnd`, `loopCrossfade`, `rootKey`, `tune`, `fine`, `vintage`, `filterCutoff`, `filterResonance`, `ampAttack`, `ampDecay`, `ampSustain`, `ampRelease`, `velToAmp`, `outputLevel`

**Combos (3)** — `WebComboBoxRelay` + `WebComboBoxParameterAttachment` (`getComboBoxState` in JS):
`sourceSample` (built-in choices), `loopMode` (Off/Forward/Ping-Pong), `pitchMode` (Repitch/Stretch)

**Toggles (1)** — `WebToggleButtonRelay` + `WebToggleButtonParameterAttachment` (`getToggleState` in JS):
`reverse`

**Total: 17 + 3 + 1 = 21.** ✓ Matches the frozen APVTS contract.

> Plan must double-check `rootKey`/`tune` as slider-relays vs a stepped UI affordance. They are continuous-enough to use the knob widget (rootKey as a note-name readout; tune as a ±24 st readout). This is the O-simpleGrain `grainPitch`/integer-knob convention.

### 2.3 Resource provider (bare-path matching)

`getResource(const juce::String& url)` receives a **bare path** (`/`, `/index.html`, `/js/app.js`, …) — compare with **direct equality**, never strip scheme/host (project memory: `fromFirstOccurrenceOf("://")` on a bare path returns "" → all lookups fail → "Frame load interrupted"). Serve text with `charset=utf-8`. Grain's `getResource` (`PluginEditor.cpp:41–72`) is the exact template; sampler's URL set is identical minus `insects.png` (sampler chooses its own optional image) plus the drop module at `/js/modules/webview-drop-streaming.js`.

### 2.4 CMake (mostly already wired at Foundation)

Already set in `O-simpleSampler/CMakeLists.txt` (verified): `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, and the embedded-samples target `O-simpleSampler_Samples` (NAMESPACE `BinaryData`). **Net-new in 3.1** (placeholders already commented in the CMakeLists at lines 45–54):
- `ouaricon_add_module(O-simpleSampler webview-drop-streaming)` — copies the JS module into `Source/ui/public/modules/`.
- `juce_add_binary_data(O-simpleSampler_UIResources NAMESPACE UIBinaryData HEADER_NAME UIBinaryData.h SOURCES …)` — **distinct NAMESPACE is mandatory** (a distinct HEADER_NAME alone is insufficient — both targets otherwise redefine `getNamedResource`/`namedResourceListSize` in `BinaryData`; this is the O-simpleGrain Stage-3.1 collision in project memory). SOURCES = `index.html`, `css/styles.css`, `js/app.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`, `modules/webview-drop-streaming.js` (+ any image).
- `target_link_libraries(O-simpleSampler PRIVATE O-simpleSampler_UIResources …)`.
- Editor includes `UIBinaryData.h`; processor keeps `BinaryData.h` (the `.wav`s).

### 2.5 Two-way binding pattern (JS side — clone verbatim)

From `app.js`:
- **Knob** (`bindKnob`, lines 173–233): `Juce.getSliderState(id)` → `st.getNormalisedValue()`/`st.setNormalisedValue(n)` with `st.sliderDragStarted()`/`sliderDragEnded()` bracketing each gesture; relative vertical drag (`dy / DRAG_TRAVEL_PX`); keyboard + wheel fine-nudge; `valueChangedEvent`/`propertiesChangedEvent` listeners repaint. **This same API drives the waveform-editor handles in 3.2.**
- **Combo** (`bindCombo`, lines 235–265): `Juce.getComboBoxState(id)`; builds `<option>`s from `st.properties.choices`; `st.setChoiceIndex(sel.selectedIndex)`.
- **Toggle** (`bindToggle`, lines 273–293): `Juce.getToggleState(id)`; `st.setValue(!st.getValue())`.
- **MUST** `import * as Juce from "./juce/index.js"` and pass the **`Juce` namespace** (not `window.__JUCE__`) to any shared module that calls `getNativeFunction` — the silent-TypeError trap in project memory. The drop module takes the JUCE api as a param; pass `Juce`.

### 2.6 Sample loading (FUNC-03) — PROCESSOR-SIDE GAP

The macOS WKWebView drag-drop content-streaming pattern is shipped in `webview-drop-streaming.js` (`readFileEntryAsBase64`). The grain **editor** calls `dropSessionStart`/`dropSessionAddFile`/`dropSessionCommitFile`/`loadSourceFromFileChooser`/`wasLastLoadTruncated` native fns (`PluginEditor.cpp:118–150`); the grain **processor** implements them.

**The sampler processor does NOT yet have any of these handlers.** It DOES have the decode/resample/publish primitives (`loadBuiltInSource`, `resampleToEngineRate(src, srcRate, engineRate, truncated)`, `createReaderFor(MemoryInputStream)`, `currentSource` shared_ptr atomic-publish, `currentSourceIdentity` + `builtInIndexForIdentity`). So the net-new work is the **drop→temp-file→decode glue + a `wasLastLoadTruncated` flag**, not the decoder.

**Decode gotcha (project memory):** decode the streamed base64 with **`juce::Base64::convertFromBase64(OutputStream&, StringRef)`**, NOT `juce::MemoryBlock::fromBase64Encoding` (JUCE's own non-standard `<size>.<alt>` format silently rejects `btoa()` output).

**Recommended native-fn set for the sampler** (mirror grain; ARCHITECTURE.md §6 names them `dropSample*`):
- `dropSampleStart(sessionId, name)` → `processor.dropSampleStart(...)` (open session temp dir)
- `dropSampleChunk(sessionId, name, base64)` / `dropSampleCommit(sessionId, name, base64)` → decode → resample → publish via the existing `loadBuiltInSource`-style atomic swap; flag `truncated` if >30 s
- `loadSourceFromFileChooser()` → async `juce::FileChooser` (the editor holds the `std::unique_ptr<juce::FileChooser>` member, grain `PluginEditor.h:48`)
- `wasLastLoadTruncated()` → bool getter for the >30 s notice toast
- On commit, set `currentSourceIdentity` to the file path (the `SOURCE/identity` ValueTree child) so the session restores it; bump `pendingSnap` (already done on source change at `PluginProcessor.cpp:473`).

> **Open decision for the plan:** match grain's exact session-streaming fn names (`dropSession*`) for max copy-paste, or the ARCHITECTURE-named `dropSample*`. The JS module is name-agnostic (the editor maps names→processor methods), so either works; recommend `dropSample*` per ARCHITECTURE for sampler-semantic clarity, but the JS in `app.js` `bindWebViewFileDrop` calls specific names — keep JS + editor + processor names consistent (the project-memory "native-fn bridge gap fails silently" trap).

---

## 3. Phase 3.2 — Interactive Waveform Editor + Filter Curve + Amp-ADSR (the headline net-new)

### 3.1 Static waveform background (two ports available)

Two shipped peak-bin generators — pick per how much metadata the editor wants pushed:
- **Lighter (recommended baseline):** grain `getSourceThumbnail(numPairs)` (`O-simpleGrain/Source/PluginProcessor.cpp:411`) — snapshot `currentSource` (atomicLoad), walk `numPairs` buckets, return a **flat `[min,max,…]` array** in [−1,1]. The sampler has the identical `currentSource` shared_ptr + atomicLoad pattern → **port verbatim**. JS fetches once on load + at boot (`fetchSourceThumbnail`, `app.js:613`), fills a brown waveform (`drawSourceWaveform`, `app.js:518–610`). `numPairs = 512` for projector readability.
- **Richer (if the editor wants loop/root metadata alongside peaks):** O-MicrotonalSampler `snapshotWaveformPeaks(...)` (`O-MicrotonalSampler/Source/PluginProcessor.cpp:2142`, native fn `getWaveformPeaks` at `PluginEditor.cpp:1377`) returns a **JSON DynamicObject** carrying `peaks` (`[[min,max],…]`) **plus** `lengthSamples`, `sourceSampleRate`, `loopStart`, `loopEnd`, `loopMode`, `rootNote`, etc. — handy if the canvas wants sample-accurate `sampleToX`/`xToSample` mapping rather than pure-normalized.

**Recommendation:** start with grain's flat-array `getSourceThumbnail` (the sampler already drives loop/root via params/relays, so it doesn't *need* the JSON metadata) but adopt the MicrotonalSampler **canvas idiom** (`redrawLoopEditor`) for the draw + DPR (see §3.2).

### 3.2 Interactive handles (NET-NEW *combination* — primitives exist in O-MicrotonalSampler)

The Explore survey found the missing prior art: **O-MicrotonalSampler's loop editor is the near-exact template** for draggable markers over a waveform — `Resources/ui/js/sampler-app.js`:
- `redrawLoopEditor()` (line 2211) — DPR-aware backing store (`canvas.width = round(clientWidth*dpr)`, `ctx.setTransform(dpr,0,0,dpr,0,0)`) + min/max envelope filled path + centerline. **Use this as the canvas draw base** (richer than grain's, with the DPR transform inline).
- `sampleToX(sample, cssW)` / `xToSample(x, cssW)` (lines 2317–2328) — sample↔pixel mapping.
- `onCanvasPointerDown/Move/Up` (lines 2398–2461) — `getBoundingClientRect`, hit-test markers within `MARKER_HIT_PX` (8 px), `setPointerCapture`, **ordering/clamp enforcement** (`start` clamped to `end − MIN_GAP`, etc.), redraw on move.
- `drawMarker()` (lines 2296–2315) — vertical line + triangle glyph per marker.

**The one graft that is genuinely net-new:** MicrotonalSampler writes markers via a native fn (`overrideLoopPoints`), **not** an APVTS slider relay. The sampler must instead map drag→param through the **`getSliderState` relay** (same API the knobs use): on drag, `st.setNormalisedValue(xToNorm(x))` bracketed by `sliderDragStarted/Ended`; two-way via `valueChangedEvent` → repaint. The relay-drag primitive itself also exists in that same file (`bindKnobGlobalDrag` → `state.setNormalisedValue`, `sampler-app.js:315`) — so it's a **graft of two shipped pieces**, not new invention.

Handles to draw on **one canvas** (the net-new *combination* — no single existing file does all of this):
- **start/end trim handles** → `start`/`end` (% 0–100).
- **shaded loop region** + **loopStart/loopEnd handles** → `loopStart`/`loopEnd` (% **of region**: map display within `[start,end]`; the param is 0–100 of region). Region fill from grain's band (`app.js:558–570`); MicrotonalSampler draws loop markers but does **not** fill between them, so combine grain's `fillRect` band + MicrotonalSampler's drag/hit-test.
- **root-key indicator** → marker/label from `rootKey` (note name). **Fully net-new** — no plugin renders a root-key glyph on a canvas (Explore confirmed). Static draw; optional click-to-set.
- Loop handles + region **visible only when `loopMode ≠ Off`** (read the combo state JS-side).

ARCHITECTURE.md confirms this editor is the *"genuinely-new work."* It is **not hard** (hit-testing + the established `setNormalisedValue` binding, both shipped) but it is where design/feel iteration lives — exactly why CONTEXT batches the **3.1 checkpoint before** it.

### 3.3 Live playhead (hook EXISTS — `getDisplayPlayhead`; use the PUSH model)

`getDisplayPlayhead()` returns the lead (loudest-active) voice's normalized read axis over `[startSamp,endSamp)` — `readPos` for Repitch / `timePos` for Stretch — 0 when nothing sounds (`PluginProcessor.h:144`; published from `SampleVoice::getPlayheadPos()` at `SampleVoice.h:560`, processor `PluginProcessor.cpp:671`). **No DSP change needed.**

**Architecture (Explore correction):** the repo-wide idiom is a **C++ editor `Timer` → `emitEventIfBrowserIsVisible` push → JS `addEventListener`**, NOT a JS-side Timer polling a native fn. Follow it: in the editor's `startTimerHz(30)` `timerCallback`, `webView->emitEventIfBrowserIsVisible("playheadUpdate", processorRef.getDisplayPlayhead())`; JS subscribes and draws the vertical line (grain playhead draw, `app.js:582–596`). Simpler than grain (which packs the playhead into a `GrainCloudFrame`) — the sampler pushes one float. *(If smooth tweening between 30 Hz frames is wanted, O-simpleBeatmaker's `requestAnimationFrame` interpolation of a `"frame"` event — `O-simpleBeatmaker/Source/ui/public/js/app.js:432,472` — is the reference; optional polish.)*

### 3.4 Repitch-vs-Stretch visible indicator (UI-02)

CONTEXT open question (form): the playhead **motion itself** is the indicator — in Repitch it sweeps at the pitch-coupled rate (fast on high notes), in Stretch at ~1×. **Recommendation:** rely on playhead motion **plus** a small text/label readout that reads the `pitchMode` combo (`"Repitch — pitch & time linked"` / `"Stretch — time held, pitch independent"`). Low-cost, makes the A/B explicit for the classroom, and ROADMAP allows either. The render-harness already proved the behaviour (duration ratio 1.89 vs 0.93) so the visual just has to surface it.

### 3.5 Snap atomics — RESOLVED: apply silently (answers a CONTEXT open question)

The zero-cross snap atomics (`pendingSnap`, `snapRegionStart/End`, `snapLoopStart/End`) are **DSP-internal**, one-directional (message thread `computeZeroCrossSnaps` → audio thread reads once/block, `PluginProcessor.cpp:457,542–545,608–623`). They are **absolute source-frame markers in samples**, they snap the **audio playback bounds only**, and they **do NOT feed back to the `start`/`end` params or to any UI-facing getter** (the members are private; no getter exists).

→ **The handles reflect the raw % param (smooth dragging); the snap is an inaudible-seam guarantee underneath, NOT a handle quantization.** This is the correct, simplest design and matches the shipped DSP. **Recommendation: apply silently in 3.2** (no new hooks). *Optional polish:* to surface snap visibly would need NEW normalized getters (`getSnappedRegionStartNorm()` etc.) for a faint "snap tick" overlay — defer to Stage 4 if wanted; not required by any UI-0x requirement.

### 3.6 Filter response curve (hooks EXIST — `getDisplayCutoffHz`/`getDisplayK`)

`getDisplayCutoffHz()` + `getDisplayK()` (k = 1/Q = JUCE's R2) are published once/block from the lead voice and **mirror the running filter by construction** (`PluginProcessor.h:131–132, 274–275`). Draw the closed-form `|H_LP|` (the `SubFilterCurve::magnitudeDb` / `SubVizAnalyzer.h` helper already present in `Source/`) → emit `"filterCurveUpdate"`. Because the same g/k feed audio and curve, the curve **matches what is heard** (QUAL-02) for free. **No DSP change.**

### 3.7 Amp-ADSR animation (UI-03)

Push the live amp-env value (or reconstruct the ADSR shape from the 4 params) → `"envUpdate"`; JS animates the ADSR curve with a playhead. Grain has the analogous pattern. Reconstructing the shape from `ampAttack/Decay/Sustain/Release` JS-side (no per-frame push) is the cheapest correct approach; a live VCA-level dot needs a new lead-voice env atomic (optional — plan decides).

### 3.8 Optional output scope/spectrum (hook EXISTS — `getVizRing`)

`getVizRing()` is a fully-allocated lock-free overwrite ring; the audio thread copy-only writes post-gain output; **the editor runs the FFT on the Timer** (`PluginProcessor.h:143, 280–285`). Reuse `SamplerVizAnalyzer.h` (already in `Source/`, the grain `VizAnalyzer` port). **Copy the scope window BEFORE the in-place FFT** (project invariant). CONTEXT open question: ship in 3.2 or defer? **Recommendation: ship it in 3.2** — the analyzer + ring are already built and the grain Timer body is a near-verbatim clone (`app.js:626–700`, `drawScope`/`drawSpectrum`); marginal cost, high pedagogical value.

### 3.9 DPR-aware canvas (project memory — replaced-element gotcha)

`makeCanvas`/`resize` (`app.js:406–420`): `canvas.width = round(clientWidth*dpr)`, `canvas.height = round(clientHeight*dpr)`, `ctx.setTransform(dpr,0,0,dpr,0,0)`; CSS sizes the canvas via a positioned `.canvas-wrap` using `width:calc(...)`/`height:calc(...)` — **NOT** `right`/`bottom` (a `<canvas>` is a CSS replaced element and won't stretch via right/bottom; it stays 300×150). Re-fit on `window.resize`. Clone verbatim.

### 3.10 Waveform-editor reuse map (from the Explore survey)

| Need | Copy from | File : symbol |
|------|-----------|---------------|
| DPR canvas + min/max envelope draw | O-MicrotonalSampler | `Resources/ui/js/sampler-app.js` : `redrawLoopEditor` / `drawMarker` / `sampleToX` / `xToSample` (2211–2328) |
| Draggable handles + hit-test + ordering clamp | O-MicrotonalSampler | `sampler-app.js` : `onCanvasPointerDown/Move/Up` (2398–2461) |
| Handle → APVTS relay (graft onto above) | O-MicrotonalSampler | `sampler-app.js` : `bindKnobGlobalDrag` → `setNormalisedValue` (315) |
| C++ flat min/max thumbnail (baseline) | O-simpleGrain | `PluginProcessor.cpp` : `getSourceThumbnail` (411) |
| C++ peak-bin JSON + metadata (richer alt) | O-MicrotonalSampler | `PluginProcessor.cpp` : `snapshotWaveformPeaks` (2142) + native fn `PluginEditor.cpp:1377` |
| Playhead overlay + 30 Hz push | O-simpleGrain | `PluginEditor.cpp` : `startTimerHz(30)`/`timerCallback` (234/247) + `app.js` playhead (582) |
| Shaded loop region fill | O-simpleGrain | `app.js` : `drawSourceWaveform` band (558–570) |
| Already in target (no work) | O-simpleSampler | `PluginProcessor.h:getDisplayPlayhead` (144), `SampleVoice.h:getPlayheadPos` (560), `SamplerVizAnalyzer.h` |

---

## 4. Phase 3.3 — Pedagogical Layer

### 4.1 Tooltips (UI-04) — clone the pattern

JS const map keyed by `data-tip` attribute → floating tooltip on `pointerenter`/`pointermove`/`pointerleave` + Escape-to-hide (`app.js:839–880`). Every control's wrapper carries `data-tip="<key>"`. Clone the mechanism verbatim; author sampler-specific plain-language copy for all 21 controls + the waveform editor + each viz cell.

### 4.2 Preset-tour hook (FUNC-07) — hook only; content Stage 4

`applyFactoryPreset(label)` native fn → processor snapshots a full APVTS preset; the relays/attachments sync every knob/combo/toggle back to the page automatically (no DOM poking) (`PluginEditor.cpp:169–173`, `app.js:801–833`). **The sampler processor has no `applyFactoryPreset` yet — net-new method.** Wire the hook + named buttons (`data-preset`) in 3.3; the 7 concept presets (Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell, Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped) get their parameter values authored in **Stage 4**. CONTEXT open question (dropdown vs named buttons): **named buttons** per ROADMAP/grain — more legible for a tour, each its own tooltip.

---

## 5. Processor-side NET-NEW hooks (the gap list — give the plan a checklist)

Everything the **viz** needs already exists. These are the methods the sampler processor must **add** for Stage 3 (all small; grain has each as a reference impl):

| Hook | Purpose | Phase | Reference |
|------|---------|-------|-----------|
| `getSourceThumbnail(numPairs)` | min/max envelope for the waveform | 3.2 | grain `PluginProcessor.cpp:411` (port verbatim) |
| `dropSampleStart/Chunk/Commit` (names TBD) | drag-drop user-file decode glue → existing `resampleToEngineRate`/publish | 3.1 | grain `dropSession*` |
| `loadSourceFromFileChooser()` | file-picker fallback (async `FileChooser`) | 3.1 | grain `PluginEditor.cpp:143` + processor |
| `wasLastLoadTruncated()` | >30 s notice | 3.1 | grain `PluginEditor.cpp:148` |
| `applyFactoryPreset(label)` | preset-tour snapshot (content Stage 4) | 3.3 | grain `PluginEditor.cpp:169` |
| *(optional)* `handleUiMidi(note,on,vel)` + `MidiMessageCollector` | on-screen keyboard | 3.1/3.2 | grain `PluginEditor.cpp:177`, `uiMidi` |
| *(optional)* `getSnappedRegionStartNorm()` etc. | surface zero-cross snap as a visible tick | defer | net-new (see §3.5) |

**Hooks that ALREADY EXIST (no work):** `getDisplayPlayhead`, `getDisplayCutoffHz`, `getDisplayK`, `getVizRing`, `getCurrentSampleRate`, `getAPVTS`, the `currentSource` atomic-publish, `resampleToEngineRate`, `loadBuiltInSource`, `currentSourceIdentity`/`builtInIndexForIdentity`, the snap atomics (internal).

---

## 6. On-screen keyboard — recommendation

The grain UI ships a 1-octave on-screen keyboard (`app.js:913–967`) routed through `uiMidi`→`handleUiMidi`→`MidiMessageCollector`→processBlock. **The sampler processor has no MidiMessageCollector.** The ROADMAP 3.1 component list does **not** require an on-screen keyboard, and the sampler is host-MIDI-driven. **Recommendation:** include it (it's a near-verbatim clone and "play a note, hear your loaded sound" is high pedagogical value for the classroom), but treat it as **optional / lowest-priority in 3.1** — if it slips, host MIDI fully covers the test criteria. If included, gate `handleUiMidi` with an empty host MIDI buffer merge (project-memory bridge-gap trap: an unregistered/dead `uiMidi` passes build+auval+harness silently).

---

## 7. Pitfalls carried over (from project memory + CONTEXT)

1. **Member order** relays→WebView→attachments (§2.1) — release crash on reload otherwise.
2. **Resource provider receives bare paths** — direct `==`, never strip scheme/host (§2.3).
3. **Pass the `Juce` ES-module namespace** (not `window.__JUCE__`) to the drop module / any `getNativeFunction` caller (§2.5).
4. **`juce::Base64::convertFromBase64`**, NOT `MemoryBlock::fromBase64Encoding`, for streamed drag-drop (§2.6).
5. **Dual binary-data NAMESPACE** — `UIBinaryData` + `BinaryData`, distinct NAMESPACE *and* HEADER_NAME (§2.4).
6. **DPR-aware canvas** with `width:calc()` sizing, not `right/bottom` (§3.9).
7. **Windows:** `withUserDataFolder(tempDir)` + `withBuiltInErrorPageDisabled()` (grain `PluginEditor.cpp:184–195`) — DAW hosts deny the default WebView2 data dir → silent IE fallback → blank page. Both WebView2 flags already set in CMake.
8. **Native-fn bridge gap fails silently** — every `getNativeFunction(name)` in JS must have a matching `withNativeFunction(name)` in the editor AND a processor method; grep-diff the two lists (project memory). A dead control passes build/auval/harness.
9. **Re-run the render-harness at the START of Stage 4** — once the editor gains WebView types, the harness (which compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0`) silently breaks. Fix: guard `createEditor` with `#if JUCE_WEB_BROWSER` and drop `PluginEditor.cpp` from the harness sources (project memory; bit O-simpleBeatmaker).

---

## 8. CONTEXT open questions — resolved

| Open question | Resolution |
|---------------|------------|
| Waveform peak-bin gen: native-fn push vs editor-side; bin resolution | **Native-fn `getSourceThumbnail(512)` pushed once on load** (port grain verbatim). 512 pairs = projector-readable. (§3.1) |
| Start/end/loop drag→param precision; surface snap or apply silently | Drag→`st.setNormalisedValue` (same API as knobs); **apply zero-cross snap silently** (DSP-internal, no UI feedback exists or is needed). (§3.2, §3.5) |
| Repitch-vs-Stretch indicator form | **Playhead motion + a small `pitchMode` text readout.** (§3.4) |
| Ship scope/spectrum in 3.2 or defer | **Ship in 3.2** — analyzer + ring already built; grain Timer body is a near-verbatim clone. (§3.8) |
| Preset-tour affordance: dropdown vs buttons | **Named buttons** (each with its own tooltip); hook in 3.3, content Stage 4. (§4.2) |

## 9. Remaining decisions for the plan (not blocking)

- Peak-bin format: grain flat `[min,max,…]` array (recommended baseline) vs MicrotonalSampler JSON-with-metadata — §3.1.
- `rootKey`/`tune` widget form (note-name knob vs stepped readout) — §2.2.
- Exact drop native-fn names (`dropSample*` per ARCHITECTURE vs grain's `dropSession*`) — keep JS+editor+processor consistent — §2.6.
- Include the on-screen keyboard in 3.1? (recommend yes, optional) — §6.
- Amp-ADSR: shape-from-params (cheap) vs a live VCA-level atomic (new hook) — §3.7.
- Click-to-set root key on the waveform editor (nice-to-have) — §3.2.

---

*Research complete. Ready for plan phase — ROADMAP already specifies 3.1–3.3 in detail; this RESEARCH grounds the clone-vs-net-new split and the processor-side hook gap list.*
