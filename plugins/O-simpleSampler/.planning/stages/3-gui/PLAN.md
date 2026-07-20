# Stage 3: GUI — Execution Plan

**Date:** 2026-06-26
**Plugin:** O-simpleSampler
**Inputs:** CONTEXT.md (3 decisions locked) · RESEARCH.md (clone-vs-net-new split + hook-gap list) · ROADMAP Phases 3.1–3.3
**Verified against live code:** all viz hooks present (`getDisplayPlayhead`, `getDisplayCutoffHz`/`getDisplayK`, `getVizRing`, `getCurrentSampleRate`, `getAPVTS`); processor primitives present (`loadBuiltInSource`, `resampleToEngineRate`, `currentSource` atomic-publish, `currentSourceIdentity`/`builtInIndexForIdentity`); `SubFilterCurve::magnitudeDb` + `SubVizAnalyzer`/`VizRing` in `Source/SubVizAnalyzer.h`; `SamplerVizAnalyzer.h` present. **No `Source/ui/` exists; net-new processor hooks absent (confirmed).** CMake placeholders + both WebView2 flags + `JUCE_WEB_BROWSER=1` already in place.

---

## Goal

Build the O-simpleSampler WebView GUI by cloning the shipped **O-simpleGrain** UI and grafting an **interactive waveform editor** (the headline net-new). Deliver a single-page, projector-readable signal-path layout with all **21 controls two-way bound**, cross-platform (macOS VST3+AU + Windows VST3), load-your-own (drag-drop + file picker), the interactive waveform editor (draggable start/end + shaded loop region + root-key indicator + live playhead), filter-response curve, amp-ADSR animation, output scope, on-hover pedagogical tooltips, and the preset-tour UI hook. ~80% verbatim/near-verbatim clone; the genuinely net-new work is the waveform editor canvas + a batch of processor-side native-fn hooks.

**Execution batching (CONTEXT-locked):** **Build Phase 3.1 → STOP at the checkpoint** for the deferred Stage-2 human DAW A/B (loop @ 0/10/100 ms by ear, Repitch↔Stretch, Vintage/filter feel) + a layout/feel review, **before** the expensive waveform-editor canvas work in 3.2.

---

## Decisions resolved (RESEARCH §9 — locked for execute)

| Decision | Resolution |
|----------|-----------|
| Peak-bin format | Grain's **flat `[min,max,…]` array**, `numPairs = 512` (projector-readable). Sampler drives loop/root via params, so it does NOT need MicrotonalSampler's JSON-with-metadata. |
| `rootKey`/`tune` widget | **Slider relays** (`WebSliderRelay`) → knob widget; `rootKey` shows a **note-name** readout, `tune` a **±st** readout. (O-simpleGrain integer-knob convention.) **17 slider relays total.** |
| Drop native-fn names | **`dropSampleStart` / `dropSampleChunk` / `dropSampleCommit`** per ARCHITECTURE §6. Keep JS (`app.js`) + editor (`withNativeFunction`) + processor method names **identical** (bridge-gap trap). |
| On-screen keyboard | **Include, but lowest priority in 3.1.** Near-verbatim clone; "play a note, hear your sound" is high classroom value. If it slips, host MIDI covers all test criteria. Gate `handleUiMidi` with an empty host-MIDI-buffer merge. |
| Amp-ADSR | **Reconstruct shape from the 4 params JS-side** (cheap, no new hook) for 3.2. A live VCA-level dot (new atomic) is optional polish — not planned. |
| Click-to-set root key on canvas | Nice-to-have; **deferred** (draw the static indicator only in 3.2). |
| Scope/spectrum | **Ship in 3.2** — `SamplerVizAnalyzer` + `VizRing` already built; grain Timer body is a near-verbatim clone. |

---

## The 21-parameter binding map (frozen APVTS contract)

C++ identifiers live in `OSimpleSampler::ParamIDs` (`PluginProcessor.h:40–79`). **Note:** string IDs `"start"`/`"end"` map to C++ identifiers `regionStart`/`regionEnd` (the `juce::end` shadow fix) — the **DOM/relay/JS use the string IDs** (`knob-start`, `knob-end`).

**Sliders (17)** — `WebSliderRelay` + `WebSliderParameterAttachment` (3-arg, `nullptr` undoManager); JS `Juce.getSliderState(id)`:
`start`, `end`, `loopStart`, `loopEnd`, `loopCrossfade`, `rootKey`, `tune`, `fine`, `vintage`, `filterCutoff`, `filterResonance`, `ampAttack`, `ampDecay`, `ampSustain`, `ampRelease`, `velToAmp`, `outputLevel`

**Combos (3)** — `WebComboBoxRelay` + `WebComboBoxParameterAttachment`; JS `Juce.getComboBoxState(id)`:
`sourceSample` (piano/vocal/flute/vinyl — piano-only live, rest fall back), `loopMode` (Off/Forward/Ping-Pong), `pitchMode` (Repitch/Stretch)

**Toggle (1)** — `WebToggleButtonRelay` + `WebToggleButtonParameterAttachment`; JS `Juce.getToggleState(id)`:
`reverse`

Groups left→right (signal path): **SOURCE** (sourceSample / Load…) | **REGION** (start/end/loopMode/loopStart/loopEnd/loopCrossfade/reverse) | **PITCH** (rootKey/pitchMode/tune/fine) | **VINTAGE** (vintage) | **FILTER** (filterCutoff/filterResonance) | **AMP** (ampAttack/Decay/Sustain/Release/velToAmp) | **OUTPUT** (outputLevel).

---

## Tasks

### ━━━ Phase 3.1 — Layout + Controls + Cross-Platform Wiring + Sample Loading ━━━
*(FUNC-03, UI-05, COMPAT-02) — ends at the human-DAW + visual CHECKPOINT.*

**1. [ ] Scaffold `Source/ui/public/` tree (clone from O-simpleGrain)**
- Create: `Source/ui/public/index.html`, `css/styles.css`, `js/app.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`.
- `js/juce/index.js` + `check_native_interop.js` → **copy verbatim** from grain.
- `modules/webview-drop-streaming.js` → do NOT hand-copy; pulled by `ouaricon_add_module` in Task 2.
- Depends on: none.
- Ref: `O-simpleGrain/Source/ui/public/`.

**2. [ ] CMake — UI binary-data target + drop module + link**
- `ouaricon_add_module(O-simpleSampler webview-drop-streaming)` (copies the JS module into `Source/ui/public/modules/`).
- `juce_add_binary_data(O-simpleSampler_UIResources NAMESPACE UIBinaryData HEADER_NAME UIBinaryData.h SOURCES index.html css/styles.css js/app.js js/juce/index.js js/juce/check_native_interop.js modules/webview-drop-streaming.js)`.
- `target_link_libraries(O-simpleSampler PRIVATE O-simpleSampler_UIResources)`.
- **CRITICAL:** distinct `NAMESPACE UIBinaryData` *and* `HEADER_NAME UIBinaryData.h` — distinct header alone is insufficient (O-simpleGrain duplicate-symbol collision, MEMORY.md). Embedded samples target keeps `NAMESPACE BinaryData`.
- Replace the placeholder comment block at `CMakeLists.txt:44–58`.
- Depends on: Task 1.

**3. [ ] `PluginEditor.h` — member-order contract + relays/attachments + members**
- Declare in this order (C++ destroys in reverse): **(1) all 17 slider + 3 combo + 1 toggle relays → (2) `std::unique_ptr<juce::WebBrowserComponent> webView` → (3) all 21 attachments.** Wrong order = release crash on plugin reload.
- Add `std::unique_ptr<juce::FileChooser> fileChooser`, the `SamplerVizAnalyzer` member, and `juce::Timer`/`startTimerHz(30)` plumbing (timer body filled in 3.2).
- Include `UIBinaryData.h` (not `BinaryData.h`).
- Depends on: Task 2.
- Ref: `O-simpleGrain/Source/PluginEditor.h:48,60–73`.

**4. [ ] `PluginEditor.cpp` — resource provider + relay/WebView/attachment construction + native-fn registration**
- `getResource(const juce::String& url)`: **bare-path direct equality** (`url == "/"`, `url == "/index.html"`, …) — NEVER strip scheme/host. Serve text `charset=utf-8`. URL set = grain's minus image, plus `/js/modules/webview-drop-streaming.js`.
- WebView options: `withResourceProvider`, all relays via `.withOptionsFrom(relay)`, **Windows** `withUserDataFolder(File::tempDirectory.getChildFile("O-simpleSampler_WebView"))` + `withBuiltInErrorPageDisabled()`.
- Construct attachments AFTER WebView (`getSliderState`/`getComboBoxState`/`getToggleState`, 3-arg, `nullptr` undoManager).
- Register `withNativeFunction(...)` for every native fn used in JS (Task 8/9 set; thumbnail/preset land in later phases). Guard `createEditor` with `#if JUCE_WEB_BROWSER`.
- Depends on: Task 3.
- Ref: `O-simpleGrain/Source/PluginEditor.cpp:41–72` (resource provider), `184–195` (Windows opts).

**5. [ ] `index.html` — DOM contract (groups + control ids + canvas placeholders + data-tip stubs)**
- 7 signal-path group columns (above). Each control wrapper: `knob-<id>` / `combo-<id>` / `toggle-<id>` matching the binding map; `data-tip="<key>"` stub on each (copy authored in 3.3).
- Reserve canvas containers for 3.2: `.canvas-wrap` for the **waveform editor**, **filter curve**, **amp-ADSR**, **scope** — sized via `width:calc(...)`/`height:calc(...)`, NOT `right`/`bottom` (replaced-element gotcha).
- SOURCE group: `combo-sourceSample` + a **Load…** button (file picker) + drop affordance.
- `<script type="module" src="js/app.js">`.
- Depends on: Task 1.

**6. [ ] `css/styles.css` — clone grain, retheme to sampler groups**
- Clone grain's Ouaricon field-guide aesthetic; relabel/regroup to the 7 sampler groups. Projector-readable contrast/sizing (UI-05).
- `.canvas-wrap` positioned sizing via `calc()` (DPR setup is JS-side, 3.2).
- Depends on: Task 5.

**7. [ ] `js/app.js` — two-way binding for all 21 controls**
- `import * as Juce from "./juce/index.js"`.
- `bindKnob` (×17), `bindCombo` (×3), `bindToggle` (×1) — clone grain verbatim (`getSliderState`/`getComboBoxState`/`getToggleState`; `sliderDragStarted/Ended` bracketing; relative vertical drag; wheel/keyboard nudge; `valueChangedEvent`/`propertiesChangedEvent` repaint).
- `rootKey` label → note-name; `tune` label → ±st.
- **Pass the `Juce` namespace** (not `window.__JUCE__`) to the drop module (Task 9) and any `getNativeFunction` caller (silent-TypeError trap).
- Depends on: Tasks 4, 5.
- Ref: `O-simpleGrain/Source/ui/public/js/app.js:173–293`.

**8. [ ] Processor hooks — sample loading (FUNC-03)**
- Add to `PluginProcessor`: `dropSampleStart(sessionId, name)` (open session temp dir) · `dropSampleChunk(sessionId, name, base64)` · `dropSampleCommit(sessionId, name, base64)` → **`juce::Base64::convertFromBase64(OutputStream&, StringRef)`** (NOT `MemoryBlock::fromBase64Encoding`) → write temp file → `createReaderFor` → `resampleToEngineRate(src, srcRate, engineRate, truncated)` → atomic-publish via the existing `currentSource` swap.
- `loadSourceFromFileChooser()` (editor holds the async `FileChooser`; processor decode path shared with drop).
- `wasLastLoadTruncated()` bool getter; set `truncated` when source >30 s.
- On commit: set `currentSourceIdentity` to the file path (SOURCE/identity ValueTree child for session restore) + bump `pendingSnap` (already done on source change at `PluginProcessor.cpp:473`).
- Reap stale session temp dirs on next session start (grain pattern).
- Depends on: none (parallel with 3–7).
- Ref: grain `dropSession*` + `PluginProcessor.cpp` decode/resample primitives.

**9. [ ] Wire drag-drop + file-picker + truncation toast (JS)**
- `bindWebViewFileDrop` (reuse `webview-drop-streaming.js` `readFileEntryAsBase64` via the `Juce` api) → `dropSampleStart`/`Chunk`/`Commit`.
- **Load…** button → `loadSourceFromFileChooser`.
- Toast progress (`Loading X of N: filename`) + a ">30 s — truncated" notice driven by `wasLastLoadTruncated`.
- Depends on: Tasks 7, 8.

**10. [ ] (OPTIONAL, lowest priority) On-screen keyboard**
- 1-octave keyboard (clone grain `app.js:913–967`) → `uiMidi` → processor `handleUiMidi(note,on,vel)` → `MidiMessageCollector` (net-new on the sampler) → merge into `processBlock`.
- **Gate** `handleUiMidi` with an empty host-MIDI-buffer merge (bridge-gap trap: a dead `uiMidi` passes build/auval silently).
- Skip if 3.1 is running long — host MIDI covers all test criteria.
- Depends on: Tasks 4, 8.

**11. [ ] Build + validate + install (3.1 gate)**
- Build VST3+AU+Standalone; clear AU cache (CLAUDE.md sequence / `build-and-install.sh`); `auval`; `pluginval --strictness 5`; install (dual-variant sweep).
- Verify: WebView opens (no blank), all 21 controls two-way (drag→DSP + host-automation→UI), drag-`.wav` loads+plays, file-picker works, >30 s truncates with notice.
- **Grep-diff** `getNativeFunction` (JS) vs `withNativeFunction` (editor) vs processor methods — no orphans.
- Depends on: Tasks 1–9 (10 optional).

> ### ⛔ CHECKPOINT — STOP after Task 11
> Present the playable shell for the **deferred Stage-2 human DAW A/B** (loop @ 0/10/100 ms by ear · Repitch↔Stretch · Vintage/filter feel) **+ layout/feel/projector-readability review**. Do NOT start 3.2 until the human sign-off lands. (Mirrors the Stage-2 2.1 checkpoint; catches layout/feel issues before the canvas work builds on top.)

---

### ━━━ Phase 3.2 — Interactive Waveform Editor + Filter Curve + Amp-ADSR ━━━
*(UI-01/02/03, QUAL-02) — the headline net-new. Begins only after the 3.1 checkpoint sign-off.*

**12. [ ] Processor hook — `getSourceThumbnail(numPairs)`**
- Port grain verbatim (`O-simpleGrain/Source/PluginProcessor.cpp:411`): snapshot `currentSource` (atomicLoad), walk `numPairs` buckets, return flat `[min,max,…]` in [−1,1]. `numPairs = 512`.
- Register `withNativeFunction("getSourceThumbnail", …)`; JS fetches once on load + at boot.
- Depends on: Task 4 (editor exists).

**13. [ ] Waveform editor canvas — DPR-aware static draw**
- New `.canvas-wrap` + `<canvas>`; JS `makeCanvas`/resize: `canvas.width = round(clientWidth*dpr)`, `ctx.setTransform(dpr,0,0,dpr,0,0)`; re-fit on `window.resize`.
- Draw min/max envelope filled path + centerline (port MicrotonalSampler `redrawLoopEditor` idiom, `sampler-app.js:2211`) from the `getSourceThumbnail` array.
- `sampleToX`/`xToSample` mapping helpers (normalized; `sampler-app.js:2317–2328`).
- Depends on: Task 12.

**14. [ ] Draggable start/end + shaded loop region + handles → relays**
- Hit-test markers within `MARKER_HIT_PX` (8 px), `setPointerCapture`, ordering/clamp (`start` ≤ `end − MIN_GAP`, etc.) — port `onCanvasPointerDown/Move/Up` (`sampler-app.js:2398–2461`).
- **Graft:** drive params through the **relay** (`st.setNormalisedValue(xToNorm(x))` bracketed by `sliderDragStarted/Ended`) — same API as knobs (`bindKnobGlobalDrag`, `sampler-app.js:315`), NOT MicrotonalSampler's `overrideLoopPoints` native fn. Two-way via `valueChangedEvent` → repaint.
- `start`/`end` map 0–100 % of source; `loopStart`/`loopEnd` map 0–100 % **of region** (display within `[start,end]`).
- **Shaded loop region** fill (grain `drawSourceWaveform` band, `app.js:558–570`) — visible only when `loopMode ≠ Off` (read combo state JS-side).
- Depends on: Task 13.

**15. [ ] Root-key indicator (net-new, static draw)**
- Marker/label from `rootKey` (note name) on the canvas. Static; click-to-set deferred.
- Depends on: Task 13.

**16. [ ] Live playhead (push model)**
- Editor `timerCallback` (30 Hz): `webView->emitEventIfBrowserIsVisible("playheadUpdate", processorRef.getDisplayPlayhead())`. JS `addEventListener("playheadUpdate")` → draw vertical line (grain `app.js:582–596`). No DSP change.
- Depends on: Task 13, Task 3 (Timer plumbing).

**17. [ ] Repitch-vs-Stretch visible indicator (UI-02)**
- Playhead motion is the primary cue (pitch-coupled rate in Repitch, ~1× in Stretch) **plus** a small text readout bound to the `pitchMode` combo (`"Repitch — pitch & time linked"` / `"Stretch — time held, pitch independent"`).
- Depends on: Tasks 16, 7.

**18. [ ] Filter response curve (QUAL-02 by construction)**
- Editor Timer: emit `"filterCurveUpdate"` from `getDisplayCutoffHz()`/`getDisplayK()`; JS draws closed-form `|H_LP|` via `SubFilterCurve::magnitudeDb` semantics (reuse `SubVizAnalyzer`/`SamplerVizAnalyzer` curve helper already in `Source/`). Same g/k feed audio + curve → matches what is heard. No DSP change.
- Depends on: Task 3.

**19. [ ] Amp-ADSR animation (UI-03)**
- Reconstruct the ADSR shape JS-side from `ampAttack/Decay/Sustain/Release` (no per-frame push); animate a playhead dot along it (optionally gated by the `"playheadUpdate"` event for note-active state).
- Depends on: Tasks 7, 16.

**20. [ ] Output scope/spectrum (UI polish, hook exists)**
- Editor Timer runs the FFT via `SamplerVizAnalyzer` over `getVizRing()`; **copy the scope window BEFORE the in-place FFT** (project invariant). Emit `"scopeUpdate"`/`"spectrumUpdate"`; JS draws (grain `drawScope`/`drawSpectrum`, `app.js:626–700`). No audio-thread FFT/alloc.
- Depends on: Task 3.

**21. [ ] Build + validate (3.2 gate)**
- Build + auval + pluginval@5 + install. Verify: handles drag both ways; loop region shaded inside region; playhead tracks live read pos (UI-01); Repitch↔Stretch visibly differs (UI-02); filter curve matches heard (QUAL-02); amp-ADSR animates; canvas crisp on Retina; **no audio-thread FFT/alloc**; smooth at 30 Hz.
- Depends on: Tasks 12–20.

---

### ━━━ Phase 3.3 — Pedagogical Layer ━━━
*(UI-04, FUNC-07)*

**22. [ ] Tooltips on every control (UI-04)**
- Clone grain mechanism verbatim (`app.js:839–880`): JS const map keyed by `data-tip` → floating tooltip on `pointerenter`/`pointermove`/`pointerleave` + Escape-to-hide.
- Author sampler-specific plain-language copy for **all 21 controls + the waveform editor + each viz cell**.
- Depends on: Tasks 5, 7 (3.2 canvases for editor/viz tips).

**23. [ ] Preset-tour hook (FUNC-07 — hook only; content Stage 4)**
- Processor: net-new `applyFactoryPreset(label)` → snapshot a full APVTS preset; relays/attachments resync every knob/combo/toggle automatically (no DOM poking).
- UI: **named buttons** (`data-preset`, each its own tooltip) for the 7 concept presets (Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell, Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped). **Parameter values authored in Stage 4** — wire the hook + buttons only.
- Register `withNativeFunction("applyFactoryPreset", …)`.
- Depends on: Tasks 4, 7.
- Ref: grain `PluginEditor.cpp:169–173`, `app.js:801–833`.

**24. [ ] Signal-path readability polish + final 3.3 build**
- Final pass on layout legibility/grouping/labels (UI-05). Build + auval + pluginval@5 + install; grep-diff native-fn lists once more.
- Depends on: Tasks 22, 23.

---

## Pitfalls checklist (carry into every task — project memory + RESEARCH §7)

1. **Member order** relays→WebView→attachments (Task 3) — release crash on reload otherwise.
2. **Resource provider receives bare paths** — direct `==`, never strip scheme/host (Task 4).
3. **Pass the `Juce` ES-module namespace** (not `window.__JUCE__`) to the drop module / any `getNativeFunction` caller (Tasks 7, 9).
4. **`juce::Base64::convertFromBase64`**, NOT `MemoryBlock::fromBase64Encoding`, for streamed drag-drop (Task 8).
5. **Dual binary-data NAMESPACE** — `UIBinaryData` + `BinaryData`, distinct NAMESPACE *and* HEADER_NAME (Task 2).
6. **DPR-aware canvas** with `width:calc()` sizing, not `right/bottom` (Tasks 5, 13).
7. **Windows:** `withUserDataFolder(tempDir)` + `withBuiltInErrorPageDisabled()` — DAW hosts deny default WebView2 dir → silent IE fallback → blank page (Task 4).
8. **Native-fn bridge gap fails silently** — grep-diff `getNativeFunction` (JS) ↔ `withNativeFunction` (editor) ↔ processor methods every build (Tasks 11, 24).
9. **Guard `createEditor` with `#if JUCE_WEB_BROWSER`** now (Task 4); the render-harness re-run happens at the **START of Stage 4** (harness compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0` and breaks once the editor gains WebView types — drop `PluginEditor.cpp` from harness sources then).
10. Build hygiene: AU cache clear + dual-variant sweep on every install (CLAUDE.md / `build-and-install.sh`).

---

## Success Criteria

**Phase 3.1 (checkpoint gate):**
- [ ] WebView opens; single-page signal-path layout renders, projector-readable (UI-05).
- [ ] All 21 controls two-way bound (drag→DSP; host automation→UI).
- [ ] Drag a `.wav` loads+plays; file-picker fallback works; >30 s truncates with a notice (FUNC-03).
- [ ] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI) (COMPAT-02).
- [ ] Human DAW A/B (deferred Stage-2 gate) + layout/feel review signed off.

**Phase 3.2:**
- [ ] Dragging start/end/loop handles updates params and vice-versa; loop region shaded inside the region.
- [ ] Playhead tracks the live read position during playback (UI-01).
- [ ] Toggling Repitch↔Stretch visibly changes playhead behaviour (UI-02).
- [ ] Filter curve matches what is heard (QUAL-02); amp-ADSR animates with the note.
- [ ] Output scope renders; no audio-thread FFT/alloc; UI smooth at 30 Hz; canvas crisp on Retina.

**Phase 3.3:**
- [ ] On-hover plain-language tooltip on every control + the waveform editor + viz cells (UI-04).
- [ ] 7 named preset buttons present and wired to `applyFactoryPreset` (FUNC-07 hook; content Stage 4).

**Stage exit:**
- [ ] auval SUCCEEDED + pluginval@5 SUCCESS, installed.
- [ ] Native-fn JS↔editor↔processor lists grep-clean (no orphans).
- [ ] 21 APVTS params unchanged (frozen contract).

---

## Net-new processor hooks (the gap checklist)

| Hook | Phase | Task | Reference |
|------|-------|------|-----------|
| `dropSampleStart/Chunk/Commit` | 3.1 | 8 | grain `dropSession*` + existing `resampleToEngineRate`/publish |
| `loadSourceFromFileChooser()` | 3.1 | 8 | grain `PluginEditor.cpp:143` |
| `wasLastLoadTruncated()` | 3.1 | 8 | grain `PluginEditor.cpp:148` |
| `handleUiMidi` + `MidiMessageCollector` *(optional)* | 3.1 | 10 | grain `PluginEditor.cpp:177` |
| `getSourceThumbnail(numPairs)` | 3.2 | 12 | grain `PluginProcessor.cpp:411` (port verbatim) |
| `applyFactoryPreset(label)` | 3.3 | 23 | grain `PluginEditor.cpp:169` |

**Already exist (no work):** `getDisplayPlayhead`, `getDisplayCutoffHz`, `getDisplayK`, `getVizRing`, `getCurrentSampleRate`, `getAPVTS`, `currentSource` atomic-publish, `resampleToEngineRate`, `loadBuiltInSource`, `currentSourceIdentity`/`builtInIndexForIdentity`, snap atomics (internal).

---

*Plan complete. 24 tasks across 3 phases; hard checkpoint after Task 11 (Phase 3.1). Next: `/plugin-execute O-simpleSampler 3-gui` (executes Phase 3.1 → STOP at checkpoint).*
