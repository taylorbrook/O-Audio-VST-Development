# Stage 3: GUI — Plan

**Date:** 2026-05-01
**Plugin:** O-Bassoon
**Stage:** 3 of 4 (GUI)
**Phase:** plan
**Predecessors:**
- Stage 3 / discuss-phase ✅ COMPLETE — `stages/3-gui/CONTEXT.md`
- Stage 3 / research-phase ✅ COMPLETE — `stages/3-gui/RESEARCH.md`
**Blocker (Phase 3.1 execute-phase only):** UI mockup pass (`/ui-mockup O-Bassoon`) must land before Phase 3.1 task M (mockup conversion). Plan-phase itself does NOT block on mockup; only the execute-phase task that consumes it.

---

## Goal

Replace the Stage 1 `GenericAudioProcessorEditor` placeholder with a JUCE 8 WebView UI that exposes all 10 APVTS parameters in the Ouaricon-botanical aesthetic, embeds the shared `tuning-panel` module as a Tuning tab, and provides three live-feedback elements (active-voice dots, effective-breath meter, vibrato-envelope dot). Closes UI-01 + UI-02. 2-phase split per ROADMAP D8 / CONTEXT D8.

---

## Cycle Scope

**Phase 3.1 — Layout + Parameter Binding + Tuning Tab Embed (10 tasks)**
Convert finalized mockup → production HTML, wire 10 WebSliderRelay+Attachment pairs, embed shared `tuning-panel.{css,js}` via Pattern A (CMake direct reference, not per-plugin copy), wire ~25 tuning native functions, set up resource provider (bare-path equality, 6 paths), add `juce_add_binary_data` block, configure WebView2 user-data folder, add `getAPVTS()` accessor (already present at PluginProcessor.h:52 — confirmed). Backfill BRIEF D6 / ROADMAP Stage 3 D8 / REQUIREMENTS UI-01 amendment per CONTEXT D7.

**Phase 3.2 — Polish + 3 Feedback Elements + Final Verification (8 tasks)**
Add 3 push-channel atomics on processor (`std::atomic<int> currentActiveVoiceCount`, `std::atomic<float> currentEffectiveBreath`, `std::atomic<float> currentVibratoEnvelope`), snapshot in processBlock prologue (allocation-free), 30 Hz Timer in editor with diff-suppression, JS receivers for the three `emitEventIfBrowserIsVisible` events, layout polish, final Logic-AU + pluginval-10 verification, atomic commit.

**Out of scope (defer to v1.1+):** preset browser, aftertouch→vibrato UI, custom knob skins beyond family default, spectrum/scope viz, MIDI learn UI, Dorico playback-template UI exposure (Stage 4 owns Playback Template).

---

## Task Dependency Graph (informational)

```
Phase 3.1
  T0 doc backfill ─┐
                   ├─ T1 mockup convert (BLOCKED on /ui-mockup output)
  T2 dirs+assets ─┘    │
  T3 stock juce/* ─────┤
  T4 binary_data ──────┤
  T5 PluginEditor.h rewrite ─── T6 PluginEditor.cpp (relays+webview+attachments+resource)
                                  │
  T7 native fn chain (lifts O-Wind) ──┘
  T8 build/install/auval/pluginval-5
  T9 manual smoke (Logic-AU)

Phase 3.2
  T10 processor atomics + snapshots
  T11 Vibrato/BassoonVoice envelope accessors
  T12 Editor Timer + diff-suppression
  T13 JS receivers + DOM render (voice dots / breath meter / vibrato dot)
  T14 layout polish (spacing, hover, value tooltips)
  T15 build + pluginval-10 + auval
  T16 static-check battery #17–#28 (12 checks)
  T17 manual Gate 5 (Logic-AU full) + atomic commit
```

---

## Phase 3.1 — Tasks

### T0 — Doc backfill (CONTEXT D7 / RESEARCH OQ#10)
- [ ] **T0** Backfill BRIEF D6 + ROADMAP Stage 3 D8 + REQUIREMENTS UI-01 with the tuning-tab-at-v1.0 amendment row
  - Files: `plugins/O-Bassoon/.planning/BRIEF.md`, `plugins/O-Bassoon/.planning/ROADMAP.md`, `plugins/O-Bassoon/.planning/REQUIREMENTS.md`
  - **BRIEF.md D6 row:** strikethrough "v1.0 headless tuning, UI exposure deferred to v1.1"; append "**SUPERSEDED 2026-05-01:** tuning panel exposed as a tab at v1.0 (CONTEXT D7 / Stage 3 plan-phase) per user authority."
  - **ROADMAP.md Stage 3 D8 row:** add note "Tuning-tab embed added to Phase 3.1 scope (was: 10-param-only) — see Stage 3 CONTEXT D7."
  - **REQUIREMENTS.md UI-01 acceptance:** append "Tuning panel accessible via Tuning tab (Sound/Tuning/About tab structure, Sound default)."
  - Depends on: none

### T1 — Mockup conversion (BLOCKED on `/ui-mockup O-Bassoon` output)
- [ ] **T1** Convert finalized UI mockup HTML/CSS to `Resources/ui/index.html`
  - Files: `plugins/O-Bassoon/Resources/ui/index.html` (NEW)
  - Source: mockup orchestrator output (path TBD by `/ui-mockup` skill — typically `<plugin>/.planning/mockup/index.html` or similar)
  - Must contain: tab bar (Sound/Tuning/About, Sound active by default), 4-section knob layout for 10 params, `<img class="botanical-overlay" src="/img/fern.png">`, `<link rel="stylesheet" href="/css/tuning-panel.css">`, `<script type="module">` block importing `'/js/juce/index.js'`, `<div id="tuning-container">` for lazy-mounted TuningPanel, container divs for `voice-dots`, `breath-meter`, `vibrato-dot`.
  - **Block:** Phase 3.1 execute-phase MUST NOT begin task T1 until mockup is finalised AND user has approved. STATUS.md `next_action` flips to `stage_3_execute_phase` only after mockup approval lands. Per CONTEXT risk #10.
  - Depends on: mockup pass complete (external)

### T2 — Resource directory tree + asset copy
- [ ] **T2** Create `Resources/ui/` subtree and copy `fern.png` from O-Wind
  - Files (NEW):
    - `plugins/O-Bassoon/Resources/ui/img/fern.png` (copy from `plugins/O-Wind/Resources/ui/img/fern.png`)
  - Per RESEARCH OQ#6 — public-domain Georg Ehret botanical, family-canonical; mockup pass may override at T1.
  - Depends on: none

### T3 — JUCE stock JS files (per-plugin copy)
- [ ] **T3** Copy JUCE 8.0.4 stock interop JS into `Resources/ui/js/juce/`
  - Files (NEW):
    - `plugins/O-Bassoon/Resources/ui/js/juce/index.js` (copy from `plugins/O-Wind/Resources/ui/js/juce/index.js`, 577 lines stock)
    - `plugins/O-Bassoon/Resources/ui/js/juce/check_native_interop.js` (copy from same source, 146 lines stock)
  - Per RESEARCH OQ#2 — these are JUCE-stock files committed per-plugin; the shared tuning-panel module is referenced by CMake path (Pattern A), but the JUCE interop layer is per-plugin.
  - Depends on: none

### T4 — `juce_add_binary_data` block
- [ ] **T4** Add `juce_add_binary_data(O-Bassoon_UIResources …)` and `target_link_libraries(O-Bassoon PRIVATE O-Bassoon_UIResources)` to CMakeLists
  - Files (MOD): `plugins/O-Bassoon/CMakeLists.txt`
  - Insert location: after existing licensing block, before `target_compile_definitions`
  - SOURCES list (6 entries — must match the 6 resource-handler bare-path equality checks in T6):
    1. `Resources/ui/index.html`
    2. `Resources/ui/js/juce/index.js`
    3. `Resources/ui/js/juce/check_native_interop.js`
    4. `Resources/ui/img/fern.png`
    5. `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js` (Pattern A — shared module direct reference)
    6. `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` (Pattern A)
  - **DO NOT** copy `tuning-panel.{js,css}` per-plugin — CMake direct reference per RESEARCH OQ#1 (lifts upstream fixes without re-copy; the `INTEGRATION-CHECKLIST.md` Step 7 is stale per RESEARCH §1.OQ#1).
  - **Existing `NEEDS_WEBVIEW2 TRUE` (line 18) and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (line 96) already correct** — confirmed at RESEARCH §3 / OQ#8. NO changes to those.
  - Depends on: T2, T3 (asset paths must exist)

### T5 — `PluginEditor.h` rewrite
- [ ] **T5** Rewrite `PluginEditor.h`: replace `GenericAudioProcessorEditor` with WebView class skeleton
  - Files (REWRITE): `plugins/O-Bassoon/Source/PluginEditor.h`
  - Inherit from `juce::AudioProcessorEditor, private juce::Timer`
  - Members per RESEARCH §5.1:
    - 10× `std::unique_ptr<juce::WebSliderRelay>` (param-id-keyed)
    - 1× `std::unique_ptr<juce::WebBrowserComponent> webView`
    - 10× `std::unique_ptr<juce::WebSliderParameterAttachment>`
    - 3× diff-suppression sentinels: `int lastEmittedActive { -1 }; float lastEmittedBreath { -1.0f }; float lastEmittedVibEnv { -1.0f };` (sentinel forces first-tick emit)
  - Public: ctor, dtor, `paint`, `resized`
  - Private: `timerCallback`, `getResource(const juce::String&)` returning `std::optional<juce::WebBrowserComponent::Resource>`
  - Reverse destruction order documented in dtor: attachments → webView → relays
  - Depends on: none

### T6 — `PluginEditor.cpp` (relays + WebView + attachments + resource provider + Timer ctor)
- [ ] **T6** Write full editor implementation
  - Files (NEW): `plugins/O-Bassoon/Source/PluginEditor.cpp`
  - Construction order (per RESEARCH §5.2 + family precedent): relays first → WebView with `.withOptionsFrom(*relay)` and tuning native fn chain (T7) → attachments (3-arg ctor, `nullptr` undoManager) → `addAndMakeVisible(*webView)` → `webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot())` → `setSize(900, 600)` → `startTimerHz(30)`
  - **WebView Options chain (lift from O-Wind `PluginEditor.cpp:71-119`):**
    - `.withBackend(juce::WebBrowserComponent::Options::Backend::webview2)`
    - `.withWinWebView2Options(...)` with `.withUserDataFolder(File::tempDirectory.getChildFile("OBassoon_WebView"))` + `.withStatusBarDisabled()` + `.withBuiltInErrorPageDisabled()`
    - `.withNativeIntegrationEnabled()`
    - `.withKeepPageLoadedWhenBrowserIsHidden()`
    - `.withResourceProvider([this](const juce::String& url) { return getResource(url); })`
    - 10× `.withOptionsFrom(*relay)` — order matches APVTS layout (vibrato_rate, vibrato_depth, vibrato_onset, breath, tone, attack_character, attack_time, release_time, voice_count, output_gain)
    - tuning native fn chain (T7)
  - **Resource provider lambda — bare-path equality, 6 paths (per RESEARCH §2.3 / risk #4):**
    - `if (url == "/" || url == "/index.html")` → `BinaryData::index_html`, `text/html`
    - `if (url == "/js/juce/index.js")` → `BinaryData::index_js`, `application/javascript`
    - `if (url == "/js/juce/check_native_interop.js")` → `BinaryData::check_native_interop_js`, `application/javascript`
    - `if (url == "/js/tuning-panel.js")` → `BinaryData::tuningpanel_js`, `application/javascript`
    - `if (url == "/css/tuning-panel.css")` → `BinaryData::tuningpanel_css`, `text/css`
    - `if (url == "/img/fern.png")` → `BinaryData::fern_png`, `image/png`
    - Default: `DBG("Resource not found: " + url); return std::nullopt;`
    - **CRITICAL** — DO NOT use `fromFirstOccurrenceOf("://")` on `url` (memory-pinned regression — bare path; stripping non-existent scheme returns empty string → "Frame load interrupted" / blank page)
  - **Attachments (10×, 3-arg ctor per `juce8-critical-patterns.md` #12):**
    - `*processorRef.getAPVTS().getParameter("<param_id>")`, `*<param_id>Relay`, `nullptr` (undoManager)
  - **Stub `timerCallback` and processor atomic accesses** — full 3-channel emit logic lives in T12; for T6, `timerCallback()` may be empty stub or already-correct skeleton (will be filled at T12)
  - Depends on: T4 (BinaryData symbols), T5 (header)

### T7 — Tuning native function chain (~25 entries, lift O-Wind verbatim)
- [ ] **T7** Lift the tuning native function chain into the WebView Options builder in `PluginEditor.cpp`
  - Files (MOD): `plugins/O-Bassoon/Source/PluginEditor.cpp`
  - Source of truth: O-Wind `PluginEditor.cpp:237-475` (per RESEARCH §2.4)
  - Function set (~25 — verify exact count against O-Wind at execute-phase):
    - **Tuning data:** `getTuningIntervals`, `setTuningIntervals`, `getTuningName`, `setSingleInterval`, `setSingleIntervalEncoded`
    - **Tonic:** `setTonicNote`, `getTonicNote`
    - **Octave stretch:** `setOctaveStretch`, `getOctaveStretch`
    - **Master tune:** `setMasterTune`, `getMasterTune`
    - **Presets:** `setTemperamentPreset`, `getTemperamentPreset`
    - **File I/O:** `loadScalaFile`, `saveScalaFile`, `loadKBMFile`, `saveKBMFile`
    - **Scale generator:** `generateEDO`, `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`
    - **Embedded library:** `getEmbeddedTuningList`, `getEmbeddedTuningCategories`, `loadEmbeddedTuning`
    - **Export:** `exportTuningHTML`
  - Each fn lambda accesses `processorRef.getTuningEngine()` (already exposed; verify accessor name at execute-phase)
  - **JS-side TuningPanel instantiation pattern (in `index.html` <script type="module"> block from T1):**
    ```js
    const mod = await import('/js/tuning-panel.js');
    const TuningPanel = mod.TuningPanel || mod.default;
    // CRITICAL: pass `Juce` ES-module namespace, NOT window.__JUCE__
    const tuningPanelInstance = new TuningPanel(container, Juce);
    ```
    Lazy-mount on first Tuning-tab activation per O-MicrotonalSampler `sampler-app.js:332-356` precedent (`ensureTuningPanelMounted()`).
  - Depends on: T6 (WebView builder skeleton)

### T8 — Build + install + auval + pluginval-5
- [ ] **T8** First Phase 3.1 build verification gate
  - Commands:
    - `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone --parallel`
    - Per CLAUDE.md cache-clearing protocol: `killall -9 AudioComponentRegistrar 2>/dev/null || true; rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache; rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3 ~/Library/Audio/Plug-Ins/Components/O-Bassoon.component`
    - Install: `cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/VST3/O-Bassoon.vst3 ~/Library/Audio/Plug-Ins/VST3/; cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/AU/O-Bassoon.component ~/Library/Audio/Plug-Ins/Components/`
    - `auval -v aufx OBsn Ouar` → VALIDATION SUCCEEDED expected
    - `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3` → exit 0 expected
  - Depends on: T1 (mockup landed), T2, T3, T4, T6, T7

### T9 — Manual Logic-AU smoke (Phase 3.1 close-out checklist)
- [ ] **T9** Manual smoke test in Logic-AU
  - Checklist:
    1. Plugin loads without crash; window appears at 900×600
    2. All 10 knobs visible in Sound tab; correct group labels (Vibrato/Expression/Envelope/Voicing & Output)
    3. Tab switch Sound ↔ Tuning ↔ About works; Sound is default tab on first open
    4. Tuning tab: intervals table renders (lazy-mount succeeds); Generate EDO button populates intervals (memory-known regression sentinel — risk #1, OQ#1)
    5. About tab: title "O-Bassoon", version "1.0.0", tagline, blurb, ouaricon.com link visible
    6. All 10 knobs respond to drag (relative-drag pattern, no jump-to-cursor); audio responds in real time
    7. DAW automation round-trip: automate one parameter from DAW → knob moves
    8. Botanical overlay (`fern.png`) visible behind UI
  - **Open Phase 3.1 close-out:** if any item fails, iterate (rev-2/rev-3 max per CONTEXT inline-iteration ceiling).
  - Depends on: T8

---

## Phase 3.2 — Tasks

### T10 — Processor atomics + processBlock snapshots
- [ ] **T10** Add 3 push-channel atomics + processBlock snapshot block
  - Files (MOD): `plugins/O-Bassoon/Source/PluginProcessor.h`, `PluginProcessor.cpp`
  - **Header additions (public section):**
    ```cpp
    std::atomic<int>   currentActiveVoiceCount { 0 };
    std::atomic<float> currentEffectiveBreath  { 0.0f };
    std::atomic<float> currentVibratoEnvelope  { 0.0f };
    ```
  - **processBlock additions (insert AFTER existing voice_count snapshot block at PluginProcessor.cpp:197-205, BEFORE tone-dispatch at :211):**
    - Live active-voice count: 16-iter loop over `synthesiser.getVoice(v)->isVoiceActive()`, store relaxed
    - Effective breath snapshot: read first active voice's `breathSmoother.getCurrentValue()` (already composed `ui_breath × cc2_normalised` per BassoonVoice.cpp:149); 0 if no active voice
    - Vibrato envelope snapshot: read first active voice's `getVibratoEnvelope()` (T11 accessor); 0 if no active voice
  - **Why per-voice for breath/vibEnv (vs. processor-level CC2 + UI breath multiply):** per RESEARCH §10 item #6 — currentCC2 is NOT maintained at processor level (lives per-voice in `BassoonVoice::cc2EverActive` + `lastCC2SampleCount`). Sampling first-active-voice is the cheapest single-site read; consistent with the breath value the audio thread is actually applying to that voice's render output (no skew between displayed and audible).
  - All stores `std::memory_order_relaxed` (single-producer single-consumer, no causal dependency)
  - Depends on: T11 (vibrato envelope accessor); but T10 can land first with a temporary `vibEnv = 0.0f` stub, then T11 wires the real call
  - **RT-safety:** zero allocation, zero locks; loop costs <1 µs/block at 256 buf / 48 kHz (per risk NEW 12 in RESEARCH §7)

### T11 — Vibrato envelope accessors
- [ ] **T11** Add `getEnvelope()` proxy chain through Vibrato → BassoonVoice
  - Files (MOD): `plugins/O-Bassoon/Source/Vibrato.h`, `BassoonVoice.h`
  - **Vibrato.h public:**
    ```cpp
    float getEnvelope() const noexcept { return onsetEnvelope.getCurrentValue(); }
    ```
  - **BassoonVoice.h public:**
    ```cpp
    float getVibratoEnvelope() const noexcept { return vibrato.getEnvelope(); }
    ```
  - 5-line additions total. Member `onsetEnvelope` already exists from Phase 2.3.
  - Depends on: none

### T12 — Editor Timer + diff-suppression
- [ ] **T12** Implement `timerCallback()` with diff-suppression and 3-channel emit
  - Files (MOD): `plugins/O-Bassoon/Source/PluginEditor.cpp`
  - Pattern (per RESEARCH §2.6 + OQ#4):
    ```cpp
    void OBassoonAudioProcessorEditor::timerCallback() {
        if (webView == nullptr) return;
        const int   active = processorRef.currentActiveVoiceCount.load(std::memory_order_relaxed);
        const float breath = processorRef.currentEffectiveBreath.load(std::memory_order_relaxed);
        const float vibEnv = processorRef.currentVibratoEnvelope.load(std::memory_order_relaxed);
        if (active != lastEmittedActive) {
            webView->emitEventIfBrowserIsVisible("activeVoiceCount", juce::var(active));
            lastEmittedActive = active;
        }
        if (std::abs(breath - lastEmittedBreath) > 0.005f) {
            webView->emitEventIfBrowserIsVisible("effectiveBreath", juce::var(breath));
            lastEmittedBreath = breath;
        }
        if (std::abs(vibEnv - lastEmittedVibEnv) > 0.005f) {
            webView->emitEventIfBrowserIsVisible("vibratoEnvelope", juce::var(vibEnv));
            lastEmittedVibEnv = vibEnv;
        }
    }
    ```
  - Dtor: `stopTimer()` BEFORE webView destruction
  - Sentinels (`-1`, `-1.0f`) force first-tick emit so initial state propagates without waiting for change
  - Depends on: T6 (Timer skeleton), T10 (atomics exist)

### T13 — JS receivers + DOM render
- [ ] **T13** Add 3 `window.__JUCE__.backend.addEventListener` subscribers to `Resources/ui/index.html` <script type="module"> block
  - Files (MOD): `plugins/O-Bassoon/Resources/ui/index.html`
  - **Active-voice dots (`activeVoiceCount` event, `juce::var(int)` payload):**
    - `renderVoiceDots(count)` — render N filled dots out of `voice_count` (the cap) total dots; 0..16
    - Container: `#voice-dots`
  - **Effective-breath meter (`effectiveBreath` event, `juce::var(float)` 0..1):**
    - `breathMeterFill.style.width = (level * 100) + '%'`
    - Container: `#breath-meter`, fill: `#breath-meter .breath-meter-fill`
  - **Vibrato envelope dot (`vibratoEnvelope` event, `juce::var(float)` 0..1):**
    - `vibratoDot.style.opacity = env`
    - Container: `#vibrato-dot`
    - Pulse driven by env value (which is already `onsetProgress × |sin(phase)|` shape from Vibrato class) — no client-side animation needed
  - **Why `window.__JUCE__.backend.addEventListener` (NOT `Juce.backend...`):** event subscription is on the low-level postMessage handler `window.__JUCE__`; only `getNativeFunction`/`getSliderState`/`getToggleState` live on the ES-module `Juce` namespace. Per project memory.
  - Depends on: T1 (HTML exists), T12 (events emitted from C++)

### T14 — Layout polish
- [ ] **T14** Layout polish pass
  - Files (MOD): `plugins/O-Bassoon/Resources/ui/index.html` (CSS section)
  - Items:
    - Section padding/spacing tuning (4-section grid breathing room)
    - Knob hover state (subtle scale or opacity per O-Wind precedent)
    - Knob value tooltip on drag (text overlay showing current value with units, per family precedent)
    - Tab-bar active-state indicator (paper underline / sage-green tint)
    - Botanical overlay opacity tune (typically 0.15..0.25 per family)
    - Garamond font fallback chain
    - Mobile/small-window guard (clamp at 900×600 minimum)
  - Depends on: T13 (final UI structure stable)

### T15 — Build + pluginval-10 + auval (Phase 3.2 final build)
- [ ] **T15** Full Stage 3 build verification
  - Commands (per CLAUDE.md cache-clearing protocol; same sequence as T8 but stricter pluginval level):
    - `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone --parallel`
    - Cache clear + reinstall (CLAUDE.md sequence)
    - `auval -v aufx OBsn Ouar` → VALIDATION SUCCEEDED expected
    - `pluginval --strictness 10 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3` → exit 0 expected
  - Depends on: T14

### T16 — Static-check grep battery (12 checks: #17–#28)
- [ ] **T16** Run all 12 Stage 3 static-check grep gates per RESEARCH §8
  - 17: `grep -rn 'fromFirstOccurrenceOf' plugins/O-Bassoon/Source/` → **0 matches**
  - 18: `grep -rn 'WebSliderParameterAttachment' plugins/O-Bassoon/Source/PluginEditor.cpp` → all matches end with `, nullptr);`
  - 19: `grep -n 'withUserDataFolder' plugins/O-Bassoon/Source/PluginEditor.cpp` → ≥1 match, contains `OBassoon_WebView`
  - 20: `grep -n 'withResourceProvider' plugins/O-Bassoon/Source/PluginEditor.cpp` → ≥1 match
  - 21: `grep -n 'new TuningPanel' plugins/O-Bassoon/Resources/ui/index.html` → contains `, Juce)`, NOT `, window.__JUCE__)`
  - 22: `grep -n 'type="module"' plugins/O-Bassoon/Resources/ui/index.html` → ≥1 match
  - 23: 6 resource-provider bare-path equality checks present in PluginEditor.cpp
  - 24: `grep -rn 'O-Reed\|OReed\|o-reed' plugins/O-Bassoon/Resources/ plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` → **0 matches** (DSP-07 regression)
  - 25: `grep -nE 'NEEDS_WEBVIEW2 TRUE|JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING' plugins/O-Bassoon/CMakeLists.txt` → **2 matches**
  - 26: `grep -n 'juce_add_binary_data' plugins/O-Bassoon/CMakeLists.txt` → **1 match**
  - 27: `grep -nE 'currentActiveVoiceCount|currentEffectiveBreath|currentVibratoEnvelope' plugins/O-Bassoon/Source/PluginProcessor.h` → **3 matches**
  - 28: `grep -n 'private juce::Timer' plugins/O-Bassoon/Source/PluginEditor.h` → **1 match**
  - All 12 must PASS before T17.
  - Depends on: T15

### T17 — Manual Gate 5 (Logic-AU full) + atomic commit
- [ ] **T17** Final Gate 5 manual verification + atomic commit
  - **Gate 5 checklist (Logic-AU):**
    1. All T9 items still pass after Phase 3.2 changes (regression check)
    2. Active-voice dots: play 1, 2, 4, 8 simultaneous notes — dots row reflects live count; cap is shown by `voice_count` knob (NOT confused with live count)
    3. Breath meter: move CC2 (Mod Wheel rerouted in DAW) — meter responds; UI breath knob alone also responds; CC2 takeover gate (500 ms idle window) works
    4. Vibrato dot: enable vibrato (vibrato_depth > 0) + hold note — dot pulses at `vibrato_rate`; `vibrato_onset` gates pulse onset (silent for first N ms then fades in)
    5. 60 s long-tone sustain: no UI freeze, no audio drop, no visible memory growth in DAW process inspector (Activity Monitor / Logic CPU panel)
    6. Tab switch under load (notes held, vibrato active): no audio glitch, no UI hitch
    7. Knob automation under load: no audio glitch
    8. WebView hide/show (DAW collapse plugin window): `withKeepPageLoadedWhenBrowserIsHidden` keeps state; `emitEventIfBrowserIsVisible` correctly suppresses emits when hidden
  - **Doc updates pre-commit:**
    - `STATUS.md` — flip stage 3 to closed, append Phase 3.1 + 3.2 verify-phase summary
    - `REQUIREMENTS.md` — UI-01 + UI-02 pending → complete
    - `SUMMARY.md` (Stage 3) — write
    - `VERIFICATION.md` (Stage 3) — write
  - **Atomic commit (per CLAUDE.md commit protocol — orchestrator does NOT auto-commit; user trigger required):**
    - Subject: `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`
    - Scope: PluginEditor.{h,cpp} (rewrite + new), PluginProcessor.{h,cpp} (atomics + snapshots), Vibrato.h + BassoonVoice.h (envelope accessors), CMakeLists.txt (binary_data block), Resources/ui/* (mockup-converted HTML + CSS + JUCE stock JS + fern.png), planning artefacts (CONTEXT-rev-1 + RESEARCH-rev-1 + PLAN-rev-1 + SUMMARY + VERIFICATION + STATUS + REQUIREMENTS + BRIEF + ROADMAP amendments)
  - Depends on: T16

---

## Files to Create / Modify (consolidated)

### NEW
- `plugins/O-Bassoon/Source/PluginEditor.cpp` (T6)
- `plugins/O-Bassoon/Resources/ui/index.html` (T1, T13, T14)
- `plugins/O-Bassoon/Resources/ui/img/fern.png` (T2)
- `plugins/O-Bassoon/Resources/ui/js/juce/index.js` (T3)
- `plugins/O-Bassoon/Resources/ui/js/juce/check_native_interop.js` (T3)
- `plugins/O-Bassoon/.planning/stages/3-gui/SUMMARY.md` (T17)
- `plugins/O-Bassoon/.planning/stages/3-gui/VERIFICATION.md` (T17)

### MOD
- `plugins/O-Bassoon/Source/PluginEditor.h` (T5 — full rewrite)
- `plugins/O-Bassoon/Source/PluginProcessor.h` (T10 — 3 atomics added)
- `plugins/O-Bassoon/Source/PluginProcessor.cpp` (T10 — 3 snapshot blocks in processBlock)
- `plugins/O-Bassoon/Source/BassoonVoice.h` (T11 — `getVibratoEnvelope()` accessor)
- `plugins/O-Bassoon/Source/Vibrato.h` (T11 — `getEnvelope()` accessor)
- `plugins/O-Bassoon/CMakeLists.txt` (T4 — `juce_add_binary_data` block + `target_link_libraries` add)
- `plugins/O-Bassoon/.planning/BRIEF.md` (T0)
- `plugins/O-Bassoon/.planning/ROADMAP.md` (T0)
- `plugins/O-Bassoon/.planning/REQUIREMENTS.md` (T0, T17)
- `plugins/O-Bassoon/.planning/STATUS.md` (T17)

### Files explicitly NOT touched
- DSP layer: `BassoonVoice.cpp`, `Exciter.{h,cpp}`, `ModeBank.{h,cpp}`, `NoiseExciter.{h,cpp}`, `Vibrato.cpp`, `BassoonSynthesiser.{h,cpp}`, `BassoonSound.h` — Stage 2 closed; Stage 3 is UI-only
- `Vibrato.cpp` is NOT modified (only the `.h` for the `getEnvelope()` const accessor — inline header-only definition)

---

## Success Criteria

### Phase 3.1
- [ ] T0–T9 all complete
- [ ] Build clean (12/12 targets), VST3 + AU + Standalone produced
- [ ] `auval -v aufx OBsn Ouar` → VALIDATION SUCCEEDED
- [ ] `pluginval --strictness 5` → exit 0
- [ ] Logic-AU smoke test passes 8/8 items (T9 checklist)
- [ ] Tuning-tab regression sentinel: intervals table renders + Generate EDO populates table (memory-known fail mode caught)
- [ ] BRIEF / ROADMAP / REQUIREMENTS amendment landed (T0)

### Phase 3.2 (Stage 3 close)
- [ ] T10–T17 all complete
- [ ] All 12 static-check grep gates (#17–#28) PASS
- [ ] Build clean (12/12 targets) post-feedback-elements
- [ ] `auval` → SUCCEEDED; `pluginval --strictness 10` → exit 0
- [ ] Logic-AU Gate 5 passes 8/8 items (T17 checklist)
- [ ] 60 s long-tone with feedback elements running: no UI freeze, no audio drop, no memory growth
- [ ] All 3 push channels confirmed live in DAW (voice dots, breath meter, vibrato dot)
- [ ] UI-01 + UI-02 both flipped to complete in REQUIREMENTS.md
- [ ] Atomic commit subject `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS` ready for user trigger
- [ ] Stage 3 closed in STATUS.md; Stage 4 (Polish/Validation) becomes next-action

### Hard fail (block atomic commit)
- Any of static checks #17, #21, #22, #24, #25 fail (memory-pinned regression sentinels — bare-path, Juce-namespace, type=module, DSP-07, Windows WebView2 flags)
- `auval` failure or `pluginval --strictness 10` non-zero exit
- Any audio glitch / UI freeze in 60 s sustain test

---

## Inline Iteration Ceiling

**rev-3** (family precedent — Phase 2.x research/plan/verify-phases). If iteration exceeds rev-3 within Phase 3.1 OR Phase 3.2, escalate to discuss-phase rewrite (CONTEXT rev-2 addendum, NOT full rewrite per CONTEXT risk #7).

---

## Atomic Commit (locked subject)

`feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`

Phase 3.1 and Phase 3.2 may EITHER land as one combined atomic commit (preferred — closes Stage 3 cleanly per family precedent) OR as two phase-scoped commits if Phase 3.2 is significantly delayed. Decision deferred to Phase 3.2 close.

Per CLAUDE.md commit protocol: orchestrator does NOT auto-commit. User trigger required (`commit it` / `land it` / `ship it`).

---

## Open Items (handed forward)

1. **Mockup landing** — T1 blocks; orchestrator must wait for `/ui-mockup O-Bassoon` to complete before invoking execute-phase. Run mockup pass NOW, in parallel with discuss/research/plan completion if not already triggered.
2. **`getTuningEngine()` accessor name** — confirm at execute-phase against PluginProcessor.h Stage 1 contract (should already exist; if named differently, T7 rename).
3. **Phase 3.2 vibrato envelope source-of-truth** — first-active-voice sampling is the fastest single-site read; if multiple voices have wildly different vibrato envelopes (only when vibrato_onset is mid-fade-in across stagger-onset notes), feedback may visually "jump" when one voice releases. Acceptable v1.0 trade-off; v1.1 candidate is per-voice rotation or max-envelope sampling.
4. **Plugin-version display** — hardcode `1.0.0` in About-tab HTML at T1; defer `getPluginVersion()` native fn to v1.1.
5. **Stage 4 prep (out-of-scope for this plan)** — Stage 3 close hands forward to Stage 4 (Polish/Validation): pluginval --strictness 10 on Windows VST3 build, Dorico Playback Template + microtonal score parity test (per spike-findings), CHANGELOG, presets, finalises COMPAT-01 + COMPAT-02 + DSP-06 end-to-end DAW per OQ#10-rev-4 fallback from Stage 2.

---

**Next phase:** execute-phase (`/plugin-execute O-Bassoon 3-gui`) — blocks on `/ui-mockup O-Bassoon` landing AND user approval of mockup output.
