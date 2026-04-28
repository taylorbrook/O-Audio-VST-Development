---
title: "O-MicrotonalSampler Stage 3 (GUI) — Research"
created: 2026-04-27
stage: 3-gui
phase: research
status: complete
inputs:
  - .planning/stages/3-gui/CONTEXT.md
  - .planning/BRIEF.md
  - .planning/REQUIREMENTS.md
  - .planning/STATUS.md
references:
  - plugins/O-Bells/Source/PluginEditor.{h,cpp}
  - plugins/O-Bells/Resources/ui/{index.html, js/tuning-panel.js, css/tuning-panel.css}
  - plugins/O-Bells/CMakeLists.txt
  - plugins/O-TextureForge/Source/PluginEditor.{h,cpp}
  - JUCE/modules/juce_gui_extra/misc/juce_WebControlRelays.h
  - JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.{h,cpp}
  - .claude/projects/.../memory/MEMORY.md (WebView2 + resource provider critical patterns)
---

# Stage 3 (GUI) — Research

## 1. Suite Reference Survey (RQ3-7, RQ3-8)

### 1.1 Canonical WebView wiring (O-Bells)

`O-Bells` is the closest analog (tabs + tuning panel + bottom strip + suite aesthetic).
The editor implementation establishes the pattern we replicate verbatim:

**Member ordering (`PluginEditor.h`).** Three layers in this exact order, because
destructors run in reverse:

1. `std::unique_ptr<juce::WebSliderRelay>` / `WebComboBoxRelay` / `WebToggleButtonRelay` — one per APVTS parameter.
2. `std::unique_ptr<juce::WebBrowserComponent> webView` — built with `Options{}.withOptionsFrom(*relayN)…`.
3. `std::unique_ptr<juce::WebSliderParameterAttachment>` / `WebComboBoxParameterAttachment` — one per relay.

Reverse-destruction order means attachments die first (they call `evaluateJavascript`
during destruction and need a live `webView`), then the WebView dies, then relays.
**Stage 3 must follow this layout.** O-Bells' `PluginEditor.cpp` lines 18–828 are
the reference implementation.

**WebView construction (O-Bells `PluginEditor.cpp:96-105`).**

```cpp
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)))
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        .withOptionsFrom(*relay1)
        .withOptionsFrom(*relay2)
        // … one per relay …
        .withNativeFunction("name", [this](const juce::Array<juce::var>& args, auto complete) { … })
        // … one per native function …
);
```

**Resource provider (O-Bells `PluginEditor.cpp:941-998`).** Pattern #8: explicit
URL-to-BinaryData mapping, no generic loops. Each branch compares the bare path:

```cpp
if (url == "/" || url == "/index.html") {
    return juce::WebBrowserComponent::Resource {
        makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
        juce::String("text/html") };
}
if (url == "/js/juce/index.js") { … }
if (url == "/css/tuning-panel.css") { … }
// Resource not found
juce::Logger::writeToLog("O-Bells: Resource not found: " + url);
return std::nullopt;
```

**This matches the resource-provider memory pattern: the callback receives PATHS,
not full URLs. Direct equality, no `fromFirstOccurrenceOf("://")` stripping.**

**Initial navigation (O-Bells `PluginEditor.cpp:821`).**

```cpp
webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
```

### 1.2 CMake recipe (O-Bells `CMakeLists.txt`)

```cmake
juce_add_plugin(O-Bells
    …
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2    TRUE       # ← non-negotiable for Windows
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_compile_definitions(O-Bells
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1   # ← non-negotiable for Windows
        JUCE_USE_CURL=0)

juce_add_binary_data(O-Bells_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/snail.png
        Resources/ui/js/tuning-panel.js
        Resources/ui/css/tuning-panel.css
        Resources/ui/modules/instrument-footer-panel.js
        Resources/ui/css/instrument-footer-panel.css)

target_link_libraries(O-Bells PRIVATE O-Bells_UIResources)
```

**Stage 3 inherits this recipe verbatim.** Both `NEEDS_WEBVIEW2 TRUE` AND
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` are mandatory; from the global
memory's "WebView2 on Windows" entry, omitting either silently breaks Windows
(plugin tries to dynamic-load WebView2Loader.dll that we don't ship → blank UI
on Windows hosts; or falls through to IE backend which has no resource provider
→ blank page with no error). The `withUserDataFolder()` override is also required
for sandboxed DAW hosts that deny the WebView2 default user-data location.

### 1.3 Resource-bundling strategy (RQ3-8)

**Resolved.** O-Bells uses `juce_add_binary_data()` to bake every UI asset into
a static C++ blob exposed via `BinaryData::*`, served by the resource provider
on demand. No filesystem reads at runtime. We replicate this pattern.

The dev-time alternative (resource provider streaming from `Resources/ui/` on
disk) is brittle in installed plugin contexts and unnecessary — `juce_add_binary_data`
+ a CMake reconfigure on asset edit is the suite norm.

### 1.4 Aesthetic asset inventory (RQ3-7)

**Resolved.** Pull palette and typography from O-Bells' `Resources/ui/index.html`
inline `<style>` block (lines 30–500 ish). The canonical Ouaricon house values:

- **Background** `#F5E6D3` (cream/parchment); gradient pair `#EBD9C7` → `#F5E6D3`.
- **Body text** `#3C2F2F` (warm dark brown); secondary `#5C4033`; muted `#8B7355`.
- **Accent / highlight** `#B8860B` (antique gold) for active states.
- **Border / divider** `rgba(139, 115, 85, 0.35)` warm-brown alpha.
- **Active-state callout** `#C0392B` (rust red) — used for active scale degrees.
- **Container fill** `rgba(235, 217, 199, 0.9)` semi-opaque parchment for cards.
- **Font** `'Garamond', 'Times New Roman', serif` — editorial / botanical feel.
- **Drop shadow** `0 10px 40px rgba(0,0,0,0.5)` for the outermost card.

Stage 3 carries these into `Resources/ui/css/sampler-shell.css` (new) plus the
verbatim `tuning-panel.css` for the Tuning tab. No need to lift botanical
overlay assets (snail.png) — Stage 3.5 polish can decide whether to add a
botanical motif unique to the sampler (research scope: not now).

## 2. JUCE WebView API surface (relevant subset)

### 2.1 Relays (`juce_WebControlRelays.h`)

`WebSliderRelay`, `WebComboBoxRelay`, `WebToggleButtonRelay` are **final**
classes (`JUCE_DECLARE_NON_COPYABLE` + `JUCE_DECLARE_NON_MOVEABLE`); each is
constructed with a string identifier (`WebSliderRelay { "outputGain" }`),
threaded into the WebView via `withOptionsFrom(*relay)`, and surfaced to JS as
`Juce.getSliderState("outputGain")` / `Juce.getComboBoxState(…)` /
`Juce.getToggleState(…)`.

A `WebSliderParameterAttachment` (constructed with `RangedAudioParameter& +
WebSliderRelay&`) bridges the relay to an APVTS parameter.

**Important:** there is **no public `WebControlRelay` base class**. CONTEXT.md
D3-11 ("custom `WebControlRelay` named `sampleMap`") is incorrect terminology.
The right primitive for arbitrary JSON broadcast is documented in §2.2.

### 2.2 Arbitrary JSON broadcast — `emitEventIfBrowserIsVisible`

`WebBrowserComponent` exposes a public method:

```cpp
// juce_WebBrowserComponent.h:587 / 331
void emitEventIfBrowserIsVisible (const Identifier& eventId, const var& object);
```

The JS side receives this via `window.__JUCE__.backend.addEventListener(eventId, cb)`.
Inside the framework JS (`juce_WebBrowserComponent.cpp:155`), the payload is
`JSON.parse`'d before delivery, so a JSON-string payload yields a parsed object.

**This is the right primitive for the `sampleMap` snapshot.** Every relay
internally calls it (see `juce_WebControlRelays.cpp:81-84`), so it's a stable,
public API.

### 2.3 Native functions (C++ → JS-callable)

`Options{}.withNativeFunction("name", [this](Array<var> args, std::function<void(var)> complete){…})`.
JS calls `Juce.getNativeFunction("name")(arg0, arg1, …)` and awaits a Promise.
O-Bells uses ~30 of these for tuning + presets + GUI keyboard. Stage 3 needs
~10 (described in §4 below).

### 2.4 JS → C++ pull (snapshot fetch)

Native functions are bidirectional: JS can `await` the result. We use this for
*pull* (e.g. JS calls `getSampleMap()` on editor open to fetch initial snapshot)
and `emitEventIfBrowserIsVisible` for *push* (C++ broadcasts new snapshot after
load).

## 3. RQ Resolutions

### RQ3-1 — TuningPanel display-only mode

**Resolved.** Carry suite `tuning-panel.{js,css}` verbatim into our
`Resources/ui/{js,css}/`. **Do not modify the carried JS.** Make the panel
read-only via two non-invasive levers:

1. **Selectively register only the read-side native functions** —
   `getTuningIntervals`, `getTuningName`, `getTonicNote`, `getOctaveStretch`,
   `getEmbeddedTuningList`, `getEmbeddedTuningCategories` (the last two only
   if we want the library section to render its catalog as a browse-only list).
   *Do not* register `setSingleInterval`, `setTonicNote`, `loadEmbeddedTuning`,
   `setOctaveStretch`, `setMasterTune`, `loadScalaFile`, `saveScalaFile`,
   `loadKBMFile`, `saveKBMFile`, `exportTuningHTML`, `generateEDO`,
   `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`. The panel's
   write paths fail silently — they `await this.juce.getNativeFunction('setX')(…)`
   inside try/catch (e.g. `tuning-panel.js:317-323`).
2. **Read-only CSS overlay** — new file `Resources/ui/css/tuning-panel-readonly.css`
   loaded after `tuning-panel.css` via the resource provider. Contents:

   ```css
   /* Hide write affordances entirely (display-only mode for sampler) */
   .tuning-panel .interval-input,
   .tuning-panel .tonic-selector,
   .tuning-panel .octave-stretch-section,
   .tuning-panel .ref-knob-container,
   .tuning-panel .tuning-file-section,
   .tuning-panel .generator-section,
   .tuning-panel .library-list .library-item-apply { display: none !important; }

   /* Restyle interval rows as read-only displays */
   .tuning-panel .interval-display { /* renders cents text only */ }
   ```

   Plus a small JS shim mounted *before* the panel that swaps each
   `.interval-input` with a `<span>.interval-display` after panel render —
   only ~10 lines of init code in our `index.html`.

3. **Stub setter native functions** as no-ops returning `true` — preserves the
   panel's UX (no console error spam) without mutating tuning. Optional;
   include only if console errors during a setter call are visually distracting.

**Why not augment `tuning-panel.js`?** D3-7 commits to "carry verbatim" so the
in-flight `generalize-microtones` skill can extract the shared module without
fighting our local fork. CSS+native-function-fence is non-invasive.

**Net new files:** `Resources/ui/js/tuning-panel.js` (verbatim copy),
`Resources/ui/css/tuning-panel.css` (verbatim copy), and
`Resources/ui/css/tuning-panel-readonly.css` (new, ~30 lines).

### RQ3-2 — SampleMap JSON snapshot schema

**Resolved.** Snapshot includes per-slot metadata sufficient for: grid render,
loop-editor render, skipped-files disclosure, tuning-state readout (deferred to
TuningPanel), version-stamping for diff detection.

```json
{
  "version": 17,
  "lowestNote": 36,
  "highestNote": 96,
  "numVelocityLayers": 4,
  "slots": [
    {
      "midiNote": 60,
      "velocityLayer": 0,
      "filename": "C4_v1.wav",
      "lengthSamples": 96000,
      "sourceSampleRate": 48000.0,
      "loopStart": 24000,
      "loopEnd": 89000,
      "loopMode": "auto"
    }
  ],
  "skippedFiles": ["something.txt: unparseable filename"]
}
```

- `version` — monotonic counter; increments on every map replace (folder load
  OR per-cell replace OR loop-override). JS uses it to detect stale data after
  async cell-replace queueing (EC3-5).
- `lowestNote` / `highestNote` / `numVelocityLayers` — drive grid range
  computation in JS. Equal to existing `SampleMap` fields.
- `slots` — flat array; not all (pitch, layer) combinations are present (only
  loaded ones). Empty cells are inferred by the grid renderer.
- `filename` — basename only (`File::getFileName()`), not full path. Used for
  the cell tooltip. Not for re-loading.
- `lengthSamples` / `sourceSampleRate` — needed by the loop-editor side panel
  to display original-sample duration and SR.
- `loopStart` / `loopEnd` — sample indices into the **converted** buffer
  (post-SR conversion). `loopEnd == 0` means one-shot.
- `loopMode` ∈ `{"one-shot", "auto", "manual"}` — drives the loop-editor's
  "Reset to auto-detect" affordance (EC3-7: disabled when `one-shot`).
- `skippedFiles` — entries from `getLastSkippedFiles()`, formatted as
  `"<filename>: <reason>"`. May be empty.

**Stage 2 surface additions required to populate this:**

- `SampleSlot`: add `juce::String filename;` and an enum
  `LoopMode { OneShot, Auto, Manual }` field. The loader populates
  `filename` from `File::getFileName()`; `LoopDetector` (Phase 2.5) sets
  `loopMode = Auto` on success and `OneShot` on fallback. A user override
  flips it to `Manual`.
- `SampleMap`: add `int version = 0;`. Incremented by the processor on every
  store.
- `OMicrotonalSamplerAudioProcessor`: add `juce::String snapshotSampleMapJson() const;`
  helper that walks `currentSampleMap` and the skipped-file list and returns
  the JSON string. Called from a native function (`getSampleMap`) and from
  message-thread broadcast paths. **Read-only** — no mutation, can run on
  the message thread without locks (the shared_ptr atomic-load handles
  thread safety).

### RQ3-3 — Per-cell loader API

**Resolved.** Add a new public method on the processor:

```cpp
// PluginProcessor.h
void loadSingleSample (int midiPitch, int velocityLayer, const juce::File& file);
```

Implementation pattern (extends the existing Phase 2.2 path):

1. Validate args (`midiPitch ∈ [0,127]`, `velocityLayer ∈ [0, currentMap->numVelocityLayers - 1]`,
   `file.existsAsFile()` and `file.hasFileExtension(".wav;.aif;.aiff")`).
2. Spawn a `SampleLoader::loadSingleSlot(file, midiPitch, velocityLayer, sourceSR,
   completion)` job (extension to `SampleLoader`). The worker:
   - opens via `AudioFormatManager::createReaderFor`,
   - SR-converts via `juce::LagrangeInterpolator` per channel (D2-9),
   - mono → stereo duplicate (D2-10),
   - runs `LoopDetector::detect(buffer)` → `(loopStart, loopEnd, mode)`,
   - assembles a single `SampleSlot` POD,
   - dispatches the completion callback on the message thread with
     `(SampleSlot newSlot, juce::String skippedReason /* empty on success */)`.
3. On completion (message thread):
   - Take `currentMap = std::atomic_load(&currentSampleMap)`.
   - Build a new `SampleMap` (`auto next = std::make_shared<SampleMap>();`).
   - Copy header fields and `version + 1`.
   - Copy slots from `currentMap` minus the (midiPitch, velocityLayer) entry.
   - Push the new slot.
   - Update `lowestNote` / `highestNote` / `numVelocityLayers` if the new slot
     extends the range.
   - `std::atomic_store(&currentSampleMap, next);`
   - Append skip reason to `lastSkippedFiles` if non-empty.
   - Broadcast the new snapshot via `webView->emitEventIfBrowserIsVisible(
     "sampleMapUpdated", snapshotSampleMapJson())`. **Editor reaches into
     processor** for this — see §5 for the wiring.

**RT-safety:** Active voices read via `std::atomic_load(&currentSampleMap)` at
`startNote()`. The new map's slot vector contains the new slot; the old slot
is held alive by the active voice's shared_ptr snapshot until the note ends
(Stage 2 EC-3 — voices keep their snapshot for note duration).

**Cost analysis — Stage 2 invariant addition required:**

- Currently `SampleSlot` owns its `juce::AudioBuffer<float> audio` inline
  (deep-copy semantics). Per-cell replace would deep-copy every slot's audio
  to build the new map → ~700 MB copy on a full orchestral library →
  unacceptable on the message thread.
- **Fix:** Change `SampleSlot::audio` to `std::shared_ptr<juce::AudioBuffer<float>>`.
  Voices access via `slotLow->audio->getReadPointer(…)` (one extra indirection).
  Map deep-copy now copies a vector of shared_ptrs + POD fields — a few KB.
  Voice snapshot at `startNote()` keeps the slot's audio alive via
  `currentMap` shared_ptr's transitive ref. RT-safe.
- Files affected: `Source/SampleMap.h`, `Source/SampleLoader.cpp`,
  `Source/MicrotonalSamplerVoice.cpp` (read-pointer dereference path),
  `Source/PluginProcessor.cpp` (test fixture and any direct audio access).
  Render harness (`tests/render-harness/`) may also need a touch.

**This invariant change lands in sub-stage 3.1.** Document explicitly in
the 3.1 SUMMARY (per CONTEXT.md "No method-signature churn… except
*additions* needed for per-cell loader and loop-override relays. Document
any additions in the 3.1 SUMMARY").

### RQ3-4 — Loop-override writeback path

**Resolved.** Same atomic-shared_ptr-replace mental model as per-cell replace,
without re-decoding the audio (just a metadata flip).

```cpp
// PluginProcessor.h
void overrideLoopPoints (int midiPitch, int velocityLayer,
                         int loopStart, int loopEnd, int crossfadeLen,
                         bool resetToAutoDetect = false);
```

Implementation (synchronous; runs on the message thread — JS calls a native
function which routes here):

1. Take `currentMap = std::atomic_load(&currentSampleMap)`.
2. Locate the matching slot. If absent, return (no-op + log).
3. Build a new `SampleMap` (deep-copy header + slots — cheap once
   §RQ3-3's shared_ptr buffer change lands).
4. In the new map's matching slot, set `loopStart` / `loopEnd` /
   `loopMode = LoopMode::Manual`. (`crossfadeLen` is consumed by the voice's
   loop-boundary 8-sample crossfade in Phase 2.5; if Phase 2.5's value is
   currently a constant, add a per-slot field; if not, parameter is recorded
   for v1.0 future use.)
5. If `resetToAutoDetect`, run `LoopDetector::detect(slot.audio)` and write
   the detected values; set `loopMode = LoopMode::Auto` (or `OneShot` on
   fallback).
6. Bump `version`, `std::atomic_store(&currentSampleMap, next)`.
7. Broadcast snapshot via `emitEventIfBrowserIsVisible("sampleMapUpdated", …)`.

**Voices reading mid-note keep their old snapshot** (Stage 2 EC-3); new
note-ons see the new loop fields. Editor's Apply UX surfaces a toast: "New
loop points apply to next note-on" (EC3-6).

**No per-block work**: this whole operation is metadata churn on the message
thread, atomic store, broadcast. Audio thread sees nothing until the next
`startNote()` snapshots the new map.

### RQ3-5 — Waveform render strategy

**Resolved.** Pre-render peak summary on the message thread when a cell is
selected, broadcast to JS once via `emitEvent`. JS draws on `<canvas>`.

```cpp
// PluginProcessor.h
juce::String snapshotWaveformPeaks (int midiPitch, int velocityLayer,
                                    int targetBins = 512) const;
```

Returns a JSON string:

```json
{
  "midiNote": 60,
  "velocityLayer": 0,
  "lengthSamples": 96000,
  "sourceSampleRate": 48000.0,
  "loopStart": 24000,
  "loopEnd": 89000,
  "loopMode": "auto",
  "peaks": [[-0.42, 0.51], [-0.38, 0.47], …]
}
```

Each `[min, max]` pair is the per-channel-summed peak across one bin
(`numFrames / targetBins` samples per bin). 512 bins ≈ 4 KB JSON. Computed
in O(N) over the audio buffer — typical 5-second 48 kHz sample = 240k
multiplies, single-pass; ~1 ms on Apple Silicon. Acceptable on message
thread for a user-initiated cell-click.

JS:

```js
window.__JUCE__.backend.addEventListener("waveformPeaks", (data) => {
    drawWaveform(canvas, data);
    drawLoopMarkers(canvas, data.loopStart, data.loopEnd, data.lengthSamples);
});
```

When the user drags loop markers, JS computes the new sample positions
locally (no round-trip), shows a preview, and on mouseup calls
`overrideLoopPoints` via a native function. Snapshot rebroadcast updates
the persisted state.

**Why not stream raw PCM?** A 5-second stereo float buffer is ~2 MB.
JSON-serialized array of floats blows up further. Peak summary is the
right granularity for a UI loop editor.

### RQ3-6 — Cell DnD path

**Resolved.** Use `juce::FileDragAndDropTarget` on the host editor (proven
pattern from O-TextureForge `PluginEditor.cpp:359-389`). Resolve drop target
(folder zone vs cell vs out-of-bounds) via a **C++-side cell-layout shadow**
that JS publishes whenever the grid lays out.

**C++ side** (`PluginEditor` mixes in `juce::FileDragAndDropTarget`):

```cpp
struct CellRect { int midiNote, velocityLayer; int x, y, w, h; };
juce::Array<CellRect> cellLayout;
juce::Rectangle<int>  folderZoneRect;

bool isInterestedInFileDrag(const juce::StringArray& files) override {
    return ! files.isEmpty();
}

void fileDragEnter(const StringArray&, int x, int y) override {
    webView->emitEventIfBrowserIsVisible("hostFileDragMove",
        juce::var(juce::String("{\"x\":") + juce::String(x)
                  + ",\"y\":" + juce::String(y) + "}"));
}
void fileDragMove (const StringArray&, int x, int y) override { /* same */ }
void fileDragExit (const StringArray&)            override {
    webView->emitEventIfBrowserIsVisible("hostFileDragExit", juce::var());
}

void filesDropped(const juce::StringArray& files, int x, int y) override {
    if (files.isEmpty()) return;
    juce::File f (files[0]);

    // 1) Cell hit?
    for (auto& c : cellLayout)
        if (juce::Rectangle<int>(c.x, c.y, c.w, c.h).contains(x, y)) {
            if (f.hasFileExtension(".wav;.aif;.aiff"))
                processorRef.loadSingleSample(c.midiNote, c.velocityLayer, f);
            else
                webView->emitEventIfBrowserIsVisible("toast",
                    juce::var("Drop a .wav/.aif on a cell"));
            return;
        }

    // 2) Folder zone?
    if (folderZoneRect.contains(x, y)) {
        if (f.isDirectory())
            processorRef.loadSampleFolder(f);
        else
            webView->emitEventIfBrowserIsVisible("toast",
                juce::var("Drop a folder, not a file"));
        return;
    }

    // 3) Out-of-bounds — silent reject.
}
```

**JS side** publishes the cell layout to C++ via a native function
whenever `resize` fires or `sampleMapUpdated` triggers a re-layout:

```js
const reportLayout = Juce.getNativeFunction('reportCellLayout');
function publishCellLayout() {
    const cells = [...document.querySelectorAll('.grid-cell')].map(el => {
        const r = el.getBoundingClientRect();
        return { midiNote: +el.dataset.note, velocityLayer: +el.dataset.layer,
                 x: r.left|0, y: r.top|0, w: r.width|0, h: r.height|0 };
    });
    const drop = document.querySelector('.folder-drop-zone').getBoundingClientRect();
    reportLayout(JSON.stringify({
        cells,
        folderZone: { x: drop.left|0, y: drop.top|0, w: drop.width|0, h: drop.height|0 }
    }));
}
new ResizeObserver(publishCellLayout).observe(document.body);
window.addEventListener('sampleMapUpdated', publishCellLayout);
```

**Visual hover feedback** (drop zone glow, cell highlight) is driven by the
C++ → JS `hostFileDragMove`/`hostFileDragExit` events, NOT by HTML5 DnD
events. JS resolves which cell/zone is under the cursor using the same
layout shadow.

**Why this approach.** HTML5 `dataTransfer.files` is unreliable across
sandboxed AU/VST3 hosts on macOS — paths may be missing or stripped. JUCE's
`FileDragAndDropTarget` receives real `juce::File` paths from the native
drag layer. The host editor receives drops because JUCE's
`WebBrowserComponent` does not consume native file drops by default (the
native WebView2 / WKWebView only intercept HTML5-registered drop handlers,
which we deliberately don't register on `body`).

**Tradeoff vs O-Bells/O-TextureForge:** O-TextureForge does the same
thing minus the cell-layout shadow (it accepts any drop globally). We
need the layout shadow because EC3-2 / EC3-3 require coordinate-targeted
behavior.

### RQ3-7 / RQ3-8

Resolved in §1.3 / §1.4 above.

## 4. Native function inventory (Stage 3)

Required `withNativeFunction(...)` registrations on the WebView:

| Name | Args | Returns | Purpose | Sub-stage |
|---|---|---|---|---|
| `getSampleMap` | () | JSON string | Pull initial snapshot on editor open | 3.1 |
| `getSkippedFiles` | () | JSON string-array | Disclosure render | 3.3 |
| `loadSampleFolderDialog` | () | bool | Spawn native file chooser → loadSampleFolder | 3.3 |
| `loadSingleSampleDialog` | (midiNote, velLayer) | bool | Spawn FileChooser → loadSingleSample | 3.2 |
| `overrideLoopPoints` | (midi, vel, start, end, xfade) | bool | Apply manual loop | 3.4 |
| `resetLoopToAutoDetect` | (midi, vel) | bool | Reset loop fields | 3.4 |
| `getWaveformPeaks` | (midi, vel, bins) | JSON string | Pre-render peak summary for editor | 3.4 |
| `reportCellLayout` | (jsonString) | void | Layout shadow publish (§RQ3-6) | 3.1 |
| **TuningPanel reads (§RQ3-1):** | | | | |
| `getTuningIntervals` | () | JSON array | Reflect TuningEngine state | 3.1 |
| `getTuningName` | () | string | Header readout + panel | 3.1 |
| `getTonicNote` | () | int | Panel tonic display | 3.1 |
| `getOctaveStretch` | () | float | Panel display | 3.1 |
| `getEmbeddedTuningList` | () | JSON | Library section browse | 3.1 (optional) |

Total ≈ 13 native functions; matches Stage 2 surface (no DSP-side additions).

## 5. C++ ↔ JS event inventory

Push events emitted via `webView->emitEventIfBrowserIsVisible(eventId, var)`:

| Event ID | Payload | Trigger | JS consumer |
|---|---|---|---|
| `sampleMapUpdated` | JSON string (full snapshot) | After `loadSampleFolder` completion, `loadSingleSample` completion, `overrideLoopPoints`, `resetLoopToAutoDetect` | Grid re-render + Issues disclosure |
| `waveformPeaks` | JSON string | When user clicks a loaded cell to open editor (driven by `getWaveformPeaks` native function path) | Loop editor canvas |
| `hostFileDragMove` | `{x,y}` | JUCE host-level fileDragMove | Drop hover feedback |
| `hostFileDragExit` | `{}` | JUCE host-level fileDragExit | Hover off |
| `toast` | string | C++ rejects a drop | Toast component |

Pull (JS → C++ via native function): see §4.

## 6. Memory pitfalls (auto-loaded)

The following critical patterns from
`.claude/projects/.../memory/MEMORY.md` apply to Stage 3 verbatim:

1. **WebView2 dynamic-vs-static linking.** `NEEDS_WEBVIEW2 TRUE` in
   `juce_add_plugin()` requires `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
   in compile defs. Setting one without the other breaks Windows silently.
2. **Cross-platform WebView URL schemes.** macOS uses `juce://juce.backend/`;
   Windows uses `https://juce.backend/`. Never hard-code; use
   `getResourceProviderRoot()` in C++ and the framework's
   `getBackendResourceAddress()` in JS if needed.
3. **Resource provider receives PATHS.** Compare directly:
   `if (url == "/" || url == "/index.html")`. **Do not** strip schemes via
   `fromFirstOccurrenceOf("://")` — the path is already bare.
4. **Windows WebView2 user-data folder.** Always set
   `withUserDataFolder(tempDirectory.getChildFile("OMicrotonalSampler_WebView"))`
   on `JUCE_WINDOWS`. Default location may be denied in DAW plugin hosts,
   forcing silent fallback to the IE backend (no resource provider → blank).
5. **`AudioProcessor::getLatencySamples()` is non-virtual.** We never call
   `setLatencySamples` (sampler is feed-forward). Editor never touches
   latency state.
6. **Canvas replaced-element gotcha.** Stage 3.4 waveform `<canvas>` MUST
   use `width: calc(100% - Npx)` + DPR-aware backing store
   (`canvas.width = clientWidth * dpr; ctx.setTransform(dpr, 0, 0, dpr, 0, 0);`).
   Naive `position: absolute; left:0; right:0;` does NOT stretch a
   replaced-element canvas.

## 7. Sub-stage research — execution prerequisites

### 3.1 — WebView shell + tabs + TuningPanel mount + relays + JSON broadcast scaffold

**Prerequisites:**
- Stage 2 invariant addition: `SampleSlot.audio` → `std::shared_ptr<juce::AudioBuffer<float>>`,
  `SampleSlot.filename`, `SampleSlot.loopMode`, `SampleMap.version`
  (per RQ3-2 / RQ3-3).
- `OMicrotonalSamplerAudioProcessor::snapshotSampleMapJson()` helper.
- `OMicrotonalSamplerAudioProcessor::loadSingleSample(...)` skeleton (logic in 3.2).
- `OMicrotonalSamplerAudioProcessor::overrideLoopPoints(...)` skeleton (logic in 3.4).
- `OMicrotonalSamplerAudioProcessor::onSampleMapChanged` callback hook (so the
  editor can subscribe and call `webView->emitEvent("sampleMapUpdated", ...)`).
  Implementation: add a `std::function<void()>` member on the processor with
  a setter; `loadSampleFolder`, `loadSingleSample`, `overrideLoopPoints`
  invoke it on the message thread after atomic store.

**JUCE work:**
- New `PluginEditor.{h,cpp}` replacing the Phase 2.2 placeholder wholesale.
- 7 relays (one per APVTS param) + 7 attachments.
- `juce_add_binary_data` target with HTML/JS/CSS sources.
- Resource provider explicit URL map.
- WebView constructor with `NEEDS_WEBVIEW2 TRUE` in CMake + the four critical
  options (`backend(webview2)`, `withWinWebView2Options(...withUserDataFolder)`,
  `withNativeIntegrationEnabled`, `withResourceProvider`).

**HTML/CSS/JS work:**
- `Resources/ui/index.html` — tabs (Sample Map / Tuning / About), bottom
  control strip with 7 sliders bound to relays, fallback empty grid div.
- `Resources/ui/css/sampler-shell.css` — palette + typography from §1.4.
- `Resources/ui/js/sampler-app.js` — entry point. Imports `tuning-panel.js`
  on Tuning-tab activation, applies the readonly CSS, mounts panel via
  `new TuningPanel(container, window.__JUCE__).init()` plus the
  interval-input → span swap shim (§RQ3-1).

**Gate:** Plugin opens in DAW; 7 sliders move; Tuning tab renders TuningPanel
in read-only mode; pluginval --strictness 5 SUCCESS.

### 3.2 — Sample-mapping grid (FUNC-06 + UI-01)

**Prerequisites:**
- 3.1 complete; `loadSingleSample` / `getSampleMap` working.
- `reportCellLayout` native function callable.

**Work:**
- Grid layout: 88 keys × 4 vel rows. Use CSS grid with
  `grid-template-columns: repeat(88, 1fr); grid-template-rows: repeat(4, …);`.
  Cell width drives octave compression; min cell width 8 px (clamp).
- Cell states: empty / loaded / active (currently sounding) / loading / stolen.
  States as CSS classes, applied from JS in response to `sampleMapUpdated`
  and (optional) MIDI activity events.
- Cell click → `loadSingleSampleDialog(midiNote, velLayer)` native function
  spawns native FileChooser → on selection, `loadSingleSample` → atomic
  swap → broadcast → grid re-render.
- Layout shadow publish on `sampleMapUpdated` and on `ResizeObserver`.

**Gate:** Replacing one cell mid-session retunes only that note; grid reflects
load state in <100 ms (measure: clock between FileChooser-result and grid
re-render).

### 3.3 — Folder drop-zone (FUNC-05) + skipped-files surfacing

**Prerequisites:** 3.1 complete; FileDragAndDropTarget wiring (§RQ3-6) in
place; `getSkippedFiles` native function.

**Work:**
- Drop zone div above grid; HTML5 dragenter/dragover for visual hover (cursor
  feedback) but NOT a drop handler — we let the host editor receive the drop.
  *Note:* HTML5 `dragover` is needed to PREVENT browser default behavior; if
  the WebView2 default eats the drop, fall back to host-zone-only without
  HTML5 hover. Smoke test in 3.3.
- Toast component triggered by `toast` event for skipped-file count.
- Disclosure section bound to `getSkippedFiles` data, rendered into a
  collapsible `<details>` block.

**Gate:** Drag a folder onto the zone → loads identically to the button path;
skipped files appear in disclosure.

### 3.4 — Loop-point editor side panel (DSP-06 + UI-02)

**Prerequisites:** 3.2 complete; `getWaveformPeaks` / `overrideLoopPoints` /
`resetLoopToAutoDetect` native functions; `waveformPeaks` event broadcast
on cell selection.

**Work:**
- Side-panel layout: when a loaded cell is clicked (instead of right-clicked
  for replace — D3-12 must clarify; recommend **double-click for replace,
  single-click for editor open** to disambiguate). Pending decision in
  PLAN.md.
- Canvas waveform render with DPR-aware backing store (memory pitfall #6).
- Two draggable markers (loopStart, loopEnd) — pointer events, snap-to-zero-
  crossing (optional v1.1; v1.0 = free-drag).
- Crossfade-length slider (8–256 samples, default 8 to match Phase 2.5).
- "Reset to auto-detect" button — disabled when `loopMode == "one-shot"`
  (EC3-7).
- Apply / Cancel / Close.

**Gate:** Editing loop points produces audibly different sustain on next
note-on; reset restores the auto-detected values.

### 3.5 — Bottom control strip + aesthetic polish + tuning-state readout

**Prerequisites:** 3.1–3.4 complete.

**Work:**
- Bottom band: 7 controls (ADSR×4, Polyphony, Vel-XF, Out Gain). Knob style
  inherited from O-Bells (`.ouaricon-knob` pattern in
  `O-Bells/Resources/ui/index.html`; lift verbatim to
  `Resources/ui/css/sampler-shell.css`).
- Tuning-state readout in chrome: small badge near tabs showing
  `getTuningName()`. Updated when Tuning tab fires a refresh (or polls every
  500 ms; recommend polling for v1.0 simplicity since tuning rarely changes).
- Aesthetic pass: spacing, typography rhythm, container shadows, hover
  states.

**Gate:** Visual review against O-Bells aesthetic; window resize behaves;
pluginval --strictness 5 SUCCESS.

## 8. Open questions deferred to PLAN

| # | Question | Why-defer |
|---|---|---|
| RP3-1 | Single-click cell behavior — open loop editor or open FileChooser? | Affects 3.2 and 3.4 UX. Recommend: single-click loaded cell → open editor; double-click cell → replace via FileChooser; right-click → context menu (replace, clear, open editor). PLAN.md final call. |
| RP3-2 | Crossfade-length per slot vs global APVTS param? | Stage 2 Phase 2.5 sets a global value; per-slot override is a v1.1 candidate. Recommend v1.0: keep global, expose only loopStart/loopEnd in the UI editor. |
| RP3-3 | Tuning-state readout polling vs event-driven? | TuningEngine exposes no change-listener on this plugin. v1.0 = poll on Tuning-tab activation + once on editor open. PLAN.md final. |
| RP3-4 | About tab content — version, license link, credits? | Can be empty in 3.1; populate in 3.5. |
| RP3-5 | Cell width clamp at narrow window — show octave-only / grouped cells? | If min cell width drops below ~8 px, scroll horizontally instead. Alternatively, group into octaves and show range labels. PLAN.md decides. |

## 9. Risk register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Stage 2 surface change (`SampleSlot::audio` shared_ptr) regresses Stage 2 audio | LOW | HIGH | Re-run full Stage 2 verify gate after the 3.1 commit; render-harness tests, pluginval --strictness 5, auval. Verified-from-scratch loop is one block of work, not a Stage 2 reopen. |
| WebView2 silent fallback to IE on Windows | MEDIUM | HIGH | Both flags in CMake + manual smoke test on Windows DAW (Reaper) before 3.5 sign-off. |
| Macsandboxed AU drops fail | LOW | MEDIUM | FileDragAndDropTarget approach (§RQ3-6) sidesteps HTML5 path issues. Smoke-test in Logic Pro AU. |
| TuningPanel readonly CSS leaks across plugins | LOW | LOW | The overlay is in our `Resources/ui/css/`, loaded only by our resource provider. Other plugins are unaffected. |
| `ResizeObserver` cell-layout publish thrash | LOW | LOW | Throttle via `requestAnimationFrame` in JS. |
| Per-cell replace stalls UI when audio is large | LOW | MEDIUM | Background-thread decode + async completion. UI shows "loading…" state on the cell during the gap (EC3-5). |
| Editor close mid-load orphans the snapshot broadcast | LOW | LOW | `emitEventIfBrowserIsVisible` no-ops if browser is gone. The atomic-store is independent of the editor. |
| Latency contract violation (PERF-04) | LOW | HIGH | Stage 3 never calls `setLatencySamples`; sampler stays feed-forward. Verify in 3.5 gate. |

## 10. Files to create / modify (Stage 3 plan preview)

### New
- `plugins/O-MicrotonalSampler/Resources/ui/index.html`
- `plugins/O-MicrotonalSampler/Resources/ui/css/sampler-shell.css`
- `plugins/O-MicrotonalSampler/Resources/ui/css/tuning-panel.css` (verbatim copy)
- `plugins/O-MicrotonalSampler/Resources/ui/css/tuning-panel-readonly.css`
- `plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js`
- `plugins/O-MicrotonalSampler/Resources/ui/js/tuning-panel.js` (verbatim copy)
- `plugins/O-MicrotonalSampler/Resources/ui/js/juce/index.js` (from JUCE)
- `plugins/O-MicrotonalSampler/Resources/ui/js/juce/check_native_interop.js` (from JUCE)

### Modified
- `plugins/O-MicrotonalSampler/CMakeLists.txt` — `NEEDS_WEB_BROWSER`,
  `NEEDS_WEBVIEW2`, compile defs, `juce_add_binary_data` target.
- `plugins/O-MicrotonalSampler/Source/PluginEditor.{h,cpp}` — wholesale
  replacement.
- `plugins/O-MicrotonalSampler/Source/PluginProcessor.{h,cpp}` — additions:
  `loadSingleSample`, `overrideLoopPoints`, `snapshotSampleMapJson`,
  `snapshotWaveformPeaks`, `setSampleMapChangedCallback`. No method-signature
  removals.
- `plugins/O-MicrotonalSampler/Source/SampleMap.h` — `SampleSlot::audio` →
  `std::shared_ptr<juce::AudioBuffer<float>>`, add `filename` + `loopMode` +
  `version`.
- `plugins/O-MicrotonalSampler/Source/SampleLoader.{h,cpp}` — extend with
  `loadSingleSlot(...)`. Update buffer ownership.
- `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.{h,cpp}` —
  read-pointer dereference path; access via `slot->audio->getReadPointer(…)`.
- `plugins/O-MicrotonalSampler/Source/LoopDetector.{h,cpp}` — accept the new
  buffer ownership (or keep current if it operates on `const float*`).

### Test / fixture (verify regression-free)
- `plugins/O-MicrotonalSampler/tests/render-harness/main.cpp` — update to new
  buffer ownership.
- Re-run Stage 2 verify gate (pluginval, auval, render harness) after 3.1.

## 11. Next phase

Ready for: **plan** phase

`/plugin-plan O-MicrotonalSampler 3-gui`
