# Stage 3: GUI — Research

**Date:** 2026-05-01
**Plugin:** O-Bassoon
**Stage:** 3 of 4 (GUI)
**Phase:** research
**Predecessor:** Stage 3 / discuss-phase ✅ COMPLETE — `stages/3-gui/CONTEXT.md`
**Blocker:** UI mockup pass (`/ui-mockup O-Bassoon`) — runs in parallel with this research-phase; plan-phase blocks on mockup approval **and** this RESEARCH.md.

---

## 1. OQ Resolutions (10/10)

### OQ#1 — Tuning-panel module wiring pattern

**Two patterns exist in the family:**

| Pattern | Used by | CMake source spec | Resource handler URL |
|---|---|---|---|
| **A: Shared-module direct reference** | **O-Wind** (`CMakeLists.txt:101-102`) | `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js` + `snippets/tuning-panel.css` | `/js/tuning-panel.js`, `/css/tuning-panel.css` (`PluginEditor.cpp:638-646`) |
| **B: Per-plugin copy** | O-MicrotonalSampler (`Resources/ui/{js,css}/tuning-panel.{js,css}`) | Local source paths | Local paths |

**Recommendation:** **Pattern A (O-Wind shared-module)**. Keeps single source of truth in `modules/tuning/scala-tuning-engine/`, lets upstream fixes (e.g. the docstring update memory pinned for `window.__JUCE__` → `Juce` ES-module) propagate without per-plugin re-copy. O-Bassoon already references the C++ side this way (`CMakeLists.txt:44-48`). Lift O-Wind verbatim.

**Module source of truth:**
- `modules/tuning/scala-tuning-engine/js/tuning-panel.js` — `TuningPanel` class
- `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` — base styles
- `modules/tuning/scala-tuning-engine/snippets/INTEGRATION-CHECKLIST.md` — 11-step checklist (Step 7 says "Copy to Resources/ui/js/tuning-panel.js" — **stale, contradicts O-Wind**; Pattern A is current canonical)
- `modules/tuning/scala-tuning-engine/snippets/native-functions.cpp` — reference C++ native fn implementations
- `modules/tuning/scala-tuning-engine/snippets/parameters.cpp` — APVTS additions (already wired headless at Stage 1)
- `modules/tuning/scala-tuning-engine/snippets/persistence.cpp` — getStateInformation/setStateInformation hooks

---

### OQ#2 — `Juce` ES-module import path

**Family-canonical import (O-Wind `index.html:1109`):**
```js
import * as Juce from '/js/juce/index.js';
```

**Bundling:** `juce_add_binary_data` block adds two files from a per-plugin `Resources/ui/js/juce/` directory (NOT the shared module — these are JUCE-stock files committed per-plugin):
- `Resources/ui/js/juce/index.js` (577 lines, JUCE 8.0.4 stock)
- `Resources/ui/js/juce/check_native_interop.js` (146 lines, JUCE 8.0.4 stock)

**Resource handler entries (lift from O-Wind `PluginEditor.cpp:627-635`):**
```cpp
if (url == "/js/juce/index.js")
    return juce::WebBrowserComponent::Resource {
        makeVector(BinaryData::index_js, BinaryData::index_jsSize),
        juce::String("application/javascript") };

if (url == "/js/juce/check_native_interop.js")
    return juce::WebBrowserComponent::Resource {
        makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
        juce::String("application/javascript") };
```

**Source for the two stock JS files:** copy from `plugins/O-Wind/Resources/ui/js/juce/` (or any other family WebView plugin).

**Critical (per project memory, juce8-critical-patterns.md #21):** the `<script>` tag MUST have `type="module"`:
```html
<script type="module" src="/js/app.js"></script>
```
Or use inline-module pattern:
```html
<script type="module">
    import * as Juce from '/js/juce/index.js';
    // …
</script>
```

---

### OQ#3 — C++→JS push-channel pattern

**Family-canonical channel:** `juce::WebBrowserComponent::emitEventIfBrowserIsVisible(eventName, juce::var(payload))` — confirmed in O-MicrotonalSampler `PluginEditor.cpp:1314, 1328, 1356, 1431, 1463, 1610, 1693`.

**JS-side subscription:**
```js
window.__JUCE__.backend.addEventListener('eventName', (payload) => {
    // payload is the juce::var contents (string, number, etc.)
});
```

**Why `emitEventIfBrowserIsVisible` and NOT `evaluateJavascript` string-interpolation:**
- O-Bells uses `evaluateJavascript` with formatted JS strings (`PluginEditor.cpp:855-859`) — older pattern, requires browser-side global function table (`window.updateMeterLevels`, `window.tuningNoteOn`).
- O-MicrotonalSampler/O-Lyrica use `emitEventIfBrowserIsVisible` — cleaner, type-safe via `juce::var`, decoupled from JS function naming, suppresses emit when WebView not visible (zero overhead in collapsed/hidden DAW state).
- **Recommendation:** lift O-MicrotonalSampler `emitEventIfBrowserIsVisible` pattern for the 3 feedback channels.

**Three feedback channels for Stage 3:**
1. `activeVoiceCount` — `juce::var(int)` payload
2. `effectiveBreath` — `juce::var(float)` payload (0.0..1.0)
3. `vibratoEnvelope` — `juce::var(float)` payload (0.0..1.0; product of onset progress × |sin(phase)| or similar)

---

### OQ#4 — Audio→message-thread bridge

**Family-canonical pattern (lifted from O-MicrotonalSampler/O-Bells/O-Lyrica):**

**Processor side (audio thread, RT-safe):**
```cpp
// PluginProcessor.h
std::atomic<int>   currentActiveVoiceCount { 0 };
std::atomic<float> currentEffectiveBreath  { 0.0f };
std::atomic<float> currentVibratoEnvelope  { 0.0f };
```

```cpp
// PluginProcessor.cpp inside processBlock — relaxed memory order is sufficient
// (single producer, single consumer, no causal dependency on other atomics).
int active = 0;
const int n = synthesiser.getNumVoices();
for (int v = 0; v < n; ++v)
    if (synthesiser.getVoice (v)->isVoiceActive())
        ++active;
currentActiveVoiceCount.store (active, std::memory_order_relaxed);

// effective breath / vibrato envelope: sample once at processBlock end
// from any active voice (or use processor-side smoother snapshot).
currentEffectiveBreath.store (uiBreathSmoother.getCurrentValue() * cc2Normalised,
                              std::memory_order_relaxed);
```

**Editor side (message thread, polled at 30 Hz):**
```cpp
// PluginEditor.h — inherit privately from juce::Timer
class OBassoonAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer { … };

// PluginEditor.cpp — ctor
startTimerHz (30);

// PluginEditor.cpp — dtor (BEFORE webView destruction)
stopTimer();

// PluginEditor.cpp — timerCallback
void OBassoonAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr) return;

    const int   active   = processorRef.currentActiveVoiceCount.load (std::memory_order_relaxed);
    const float breath   = processorRef.currentEffectiveBreath.load  (std::memory_order_relaxed);
    const float vibEnv   = processorRef.currentVibratoEnvelope.load  (std::memory_order_relaxed);

    // Diff-suppression: only emit if value changed beyond epsilon.
    if (active != lastEmittedActive)
    {
        webView->emitEventIfBrowserIsVisible ("activeVoiceCount", juce::var (active));
        lastEmittedActive = active;
    }
    if (std::abs (breath - lastEmittedBreath) > 0.005f)
    {
        webView->emitEventIfBrowserIsVisible ("effectiveBreath", juce::var (breath));
        lastEmittedBreath = breath;
    }
    if (std::abs (vibEnv - lastEmittedVibEnv) > 0.005f)
    {
        webView->emitEventIfBrowserIsVisible ("vibratoEnvelope", juce::var (vibEnv));
        lastEmittedVibEnv = vibEnv;
    }
}
```

**Why diff-suppression:** O-MicrotonalSampler `PluginEditor.cpp:1418` (`if (low == prevActiveNotesLow && high == prevActiveNotesHigh) return;`) — avoids per-tick WebView IPC chatter when no state changed. Documented family pattern.

**Why 30 Hz:** O-Bells `PluginEditor.cpp:827` (`startTimerHz(30)`), O-MicrotonalSampler `:1375` (same). Smooth-enough animation, 33 ms tick, well below browser RAF rate but matches risk #8 mitigation in CONTEXT.md.

---

### OQ#5 — Active-voice count source: cap vs live

**Recommendation: live count, NOT the cap.** D5 already commits to "active-voice dots (1..N)" — interpretation is the live count, which is the more interesting feedback element ("how many notes am I holding right now?"). The cap is already shown by the `voice_count` knob value.

**Source of truth for live count:** the same loop already in `BassoonSynthesiser::findFreeVoice` (`Source/BassoonSynthesiser.h:45-49`) — count voices where `getVoice(i)->isVoiceActive()` returns true. Run this loop once per processBlock at the **prologue** (after `voice_count` snapshot, before tone-dispatch) so the snapshot stays consistent with the cap-update site. Store via `std::atomic<int> currentActiveVoiceCount`.

**Allocation-free + RT-safe:** the loop is a fixed `getNumVoices()` (16) read of voice flags; no allocation, no locks. Identical cost to the cap-enforcement loop already on the hot path.

**Thread safety:** `juce::SynthesiserVoice::isVoiceActive()` reads a single `int currentlyPlayingNote` member with no synchronisation guarantees, but the audio thread is the only writer/reader — safe by single-thread invariant.

---

### OQ#6 — Botanical overlay asset

**Family asset audit:**
- O-Wind: `fern.png` (Resources/ui/img/fern.png)
- O-Lyrica: `fern_naturalistsmisc1Geor_0089.png` (Georg Ehret 19th-c naturalist plate, public domain)
- O-Bowed: `botanical.png`
- O-Bells: `snail.png` (non-botanical; bell pattern)
- O-Orbit: `shell.png` (non-botanical; orbital pattern)

**Recommendation:** **reuse `fern.png` from O-Wind** (or copy a Georg Ehret plate from O-Lyrica). Bassoon has no plant counterpart (the etymology is from Italian *bassone*, "low one"), and the wind-family-default fern carries the wind-instrument cohesion better than a forced reed/cane illustration. Mockup pass may override; finalise asset there.

**Licensing:** all current family assets are public-domain (Georg Ehret plates pre-1900) or Ouaricon-original. Safe to reuse. If a unique illustration is needed, source from:
- Wikimedia Commons "Botanical illustrations" category
- Biodiversity Heritage Library (BHL) public-domain scans
- USDA PLANTS Database line drawings

**Path convention:** `Resources/ui/img/<name>.png` → CMake `juce_add_binary_data` SOURCES → resource handler `if (url == "/img/<name>.png")`.

---

### OQ#7 — About tab content scope

**Family-canonical (O-MicrotonalSampler `index.html:258-281`):**
```html
<section id="tab-about" class="tab-body">
  <div class="about-card">
    <h2 class="about-title">O-Bassoon</h2>
    <div class="about-version" id="about-version"></div>     <!-- populated from JUCE_AppVersion or hardcoded -->
    <p class="about-tagline">
      Modal-synthesis bassoon for sustained microtonal long tones.
    </p>
    <p class="about-blurb">
      Polyphonic 1–16 voices, VST3 Note Expression + MPE for Dorico
      microtonal playback, breath/CC2 expression, vibrato, and the
      Ouaricon tuning-system family. Built on JUCE 8.
    </p>
    <div class="about-meta">
      <span>Made by</span>
      <a class="about-link" href="https://ouaricon.com" target="_blank"
         rel="noopener noreferrer">Ouaricon</a>
    </div>
  </div>
</section>
```

**Version source:** hardcode `1.0.0` in HTML for v1.0; future enhancement is to expose via a `getPluginVersion` native function reading `JucePlugin_VersionString`.

---

### OQ#8 — Windows WebView2 redistributable

**Audit result: O-Bassoon already has BOTH required flags.** ✅
- `CMakeLists.txt:18` — `NEEDS_WEBVIEW2 TRUE` (links `WebView2LoaderStatic.lib` via JUCE)
- `CMakeLists.txt:96` — `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (compile-define for static-link path)

**O-Bassoon is NOT in the 34/35-plugins-missing-flag audit.** No remediation required at Stage 3.

**Stage 3 plan-phase task addition:** add `withUserDataFolder` runtime call (per project memory; without it, WebView2 may be denied default user data location in DAW plugin hosts and silently fall back to IE backend → blank page):

```cpp
.withWinWebView2Options(
    juce::WebBrowserComponent::Options::WinWebView2{}
        .withUserDataFolder(juce::File::getSpecialLocation(
            juce::File::SpecialLocationType::tempDirectory)
                .getChildFile("OBassoon_WebView"))
        .withStatusBarDisabled()
        .withBuiltInErrorPageDisabled())
```

Lifted verbatim from O-Wind `PluginEditor.cpp:74-80`. Folder name `OBassoon_WebView` (no underscore-prefix; matches O-Wind `OWind_WebView`).

---

### OQ#9 — Mockup blocker resolution timing

**Confirmed:** `/ui-mockup O-Bassoon` runs **in parallel** with this research-phase. Plan-phase (`/plugin-plan O-Bassoon 3-gui`) blocks on **BOTH**:
1. User approval of finalised mockup (mockup-orchestrator output)
2. This RESEARCH.md merged

**Sequencing rationale:**
- This research-phase produces the *technical* brief (CMake patterns, native fn list, push-channel API, file layout).
- Mockup-phase produces the *visual* contract (HTML structure, exact CSS, layout grid, knob skins).
- Plan-phase consumes both and emits PLAN.md with concrete tasks (Phase 3.1: convert mockup → production HTML; Phase 3.2: feedback elements + verification).

**No blocking dependency between research-phase and mockup-phase content** — research-phase resolves "how do we wire X" while mockup-phase resolves "what does X look like". The two artefacts intersect only at execute-phase.

---

### OQ#10 — ROADMAP / BRIEF amendment phase

**Recommendation: backfill at plan-phase** as a planning task (NOT this research-phase, NOT discuss-phase, NOT Stage 3 close).

**Rationale:**
- Lowest cost: a single `Edit` to `BRIEF.md` D6 row + `ROADMAP.md` Stage 3 D8 row + `REQUIREMENTS.md` UI-01 acceptance criteria.
- Highest traceability: the amendment lands in the same atomic commit as the executable plan that absorbs it (`feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`).
- Avoids stale-doc drift between discuss/research artefacts and the locked plan.

**Concrete amendment text (PLAN.md Phase 3.1 task #1 — "Doc backfill"):**
1. **BRIEF.md D6:** strikethrough "v1.0 headless tuning, UI exposure deferred to v1.1" and append: "**SUPERSEDED 2026-05-01:** tuning panel exposed as a tab at v1.0 (CONTEXT D7) per user authority."
2. **ROADMAP.md Stage 3 D8:** add row noting "tuning-tab embed added to Phase 3.1 scope (was: 10-param-only) — see CONTEXT D7."
3. **REQUIREMENTS.md UI-01:** acceptance criterion adds "Tuning panel accessible via Tuning tab (Sound/Tuning/About tab structure, Sound default)."

---

## 2. JUCE 8 API Mappings

### 2.1 WebView setup chain (lift from O-Wind `PluginEditor.cpp:71-119`)

```cpp
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)   // Windows: webview2 backend
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)
                        .getChildFile("OBassoon_WebView"))
                .withStatusBarDisabled()
                .withBuiltInErrorPageDisabled())
        .withNativeIntegrationEnabled()                  // enables window.__JUCE__
        .withKeepPageLoadedWhenBrowserIsHidden()         // page survives DAW collapse
        .withResourceProvider([this](const juce::String& url) {
            return getResource(url);                     // bare-path equality lambda
        })
        // ── attach 10 WebSliderRelay options ──
        .withOptionsFrom(*vibratoRateRelay)
        .withOptionsFrom(*vibratoDepthRelay)
        .withOptionsFrom(*vibratoOnsetRelay)
        .withOptionsFrom(*breathRelay)
        .withOptionsFrom(*toneRelay)
        .withOptionsFrom(*attackCharacterRelay)
        .withOptionsFrom(*attackTimeRelay)
        .withOptionsFrom(*releaseTimeRelay)
        .withOptionsFrom(*voiceCountRelay)               // AudioParameterInt → still WebSliderRelay (#19)
        .withOptionsFrom(*outputGainRelay)
        // ── attach tuning native functions (~25 entries; see §2.4) ──
        .withNativeFunction("getTuningIntervals", …)
        // …
        // ── about-tab static, no native fn needed ──
);

webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());   // cross-platform (juce:// vs https://juce.backend/)
```

### 2.2 WebSliderRelay + WebSliderParameterAttachment (10×)

**Pattern (per `juce8-critical-patterns.md #12` — 3-arg ctor, `nullptr` undoManager):**
```cpp
// Header: 10 unique_ptr<juce::WebSliderRelay> + 10 unique_ptr<juce::WebSliderParameterAttachment>
vibratoRateRelay = std::make_unique<juce::WebSliderRelay>("vibrato_rate");
// ... 9 more relays with parameter-id strings exactly matching APVTS ParameterID

vibratoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("vibrato_rate"), *vibratoRateRelay, nullptr);
// ... 9 more attachments
```

**JS-side (per `juce8-critical-patterns.md #15`, #19, #21):**
```js
const vibratoRateState = Juce.getSliderState("vibrato_rate");
vibratoRateState.valueChangedEvent.addListener(() => {
    const v = vibratoRateState.getNormalisedValue();   // 0..1
    knobEl.setValue(v);                                // re-render visual
});
knobEl.addEventListener('input', () => {
    vibratoRateState.setNormalisedValue(knobEl.value);
});
```

**`voice_count` is `AudioParameterInt`** — still uses `Juce.getSliderState("voice_count")`. Per `#19`, `getToggleState` is for `AudioParameterBool` only; integer parameters use `getSliderState`. Confirmed against PluginProcessor.cpp:92 (`AudioParameterInt`).

### 2.3 Resource provider (bare-path equality)

**Lift O-Wind `PluginEditor.cpp:611-656` verbatim, adapt to O-Bassoon BinaryData symbols:**

```cpp
std::optional<juce::WebBrowserComponent::Resource>
OBassoonAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    if (url == "/" || url == "/index.html")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html") };

    if (url == "/js/juce/index.js")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript") };

    if (url == "/js/juce/check_native_interop.js")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript") };

    if (url == "/js/tuning-panel.js")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String("application/javascript") };

    if (url == "/css/tuning-panel.css")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String("text/css") };

    if (url == "/img/fern.png")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::fern_png, BinaryData::fern_pngSize),
            juce::String("image/png") };

    DBG("Resource not found: " + url);
    return std::nullopt;
}
```

**CRITICAL — DO NOT use `fromFirstOccurrenceOf("://")` on the URL** — per project memory, the URL parameter is *already* a bare path. Stripping a non-existent scheme returns empty string and triggers "Frame load interrupted" / blank page. Bare-path equality only.

### 2.4 Tuning native function set (lift O-Wind `PluginEditor.cpp:237-475`)

**~25 native functions — full list per `INTEGRATION-CHECKLIST.md` Step 5:**

Tuning data:
- `getTuningIntervals` → `processorRef.getTuningEngine()->getIntervals()`
- `setTuningIntervals` → `processorRef.getTuningEngine()->setIntervals(arr)`
- `getTuningName` → `processorRef.getTuningEngine()->getName()`
- `setSingleInterval` → `processorRef.getTuningEngine()->setSingleInterval(index, cents)`
- `setSingleIntervalEncoded` → encoded variant (legacy)

Tonic:
- `setTonicNote` / `getTonicNote`

Octave stretch:
- `setOctaveStretch` / `getOctaveStretch`

Master tune:
- `setMasterTune` / `getMasterTune`

Presets:
- `setTemperamentPreset` / `getTemperamentPreset`

File I/O:
- `loadScalaFile` / `saveScalaFile` / `loadKBMFile` / `saveKBMFile`

Scale generator:
- `generateEDO` / `generateHarmonicSeries` / `generateRank2` / `applyGeneratedScale`

Embedded library:
- `getEmbeddedTuningList` / `getEmbeddedTuningCategories` / `loadEmbeddedTuning`

Export:
- `exportTuningHTML`

**Implementation pattern (per fn):**
```cpp
.withNativeFunction("getTuningIntervals",
    [this](const juce::Array<juce::var>&, auto complete) {
        auto intervals = processorRef.getTuningEngine()->getIntervals();
        juce::Array<juce::var> arr;
        for (auto cents : intervals)
            arr.add(cents);
        complete(juce::var(arr));
    })
```

**Plan-phase decision:** lift the entire `withNativeFunction(...)` chain from O-Wind verbatim, swap `processorRef` accessor to O-Bassoon's `getTuningEngine()` accessor (already exposed at Stage 1 / `PluginProcessor.h:?`). Audit at execute-phase to confirm no orphan refs.

### 2.5 TuningPanel JS instantiation (per CONTEXT.md constraint + project memory)

**CORRECT (lift O-MicrotonalSampler `sampler-app.js:343-356`):**
```js
const mod = await import('/js/tuning-panel.js');
const TuningPanel = mod.TuningPanel || mod.default;
// CRITICAL: pass `Juce` ES-module namespace, NOT window.__JUCE__
//   tuning-panel.js calls juceApi.getNativeFunction(name) — that method
//   lives on the ES-module namespace `Juce`, NOT on window.__JUCE__.
const tuningPanelInstance = new TuningPanel(container, Juce);
```

**WRONG (silent failure — per memory):**
```js
const tuningPanelInstance = new TuningPanel(container, window.__JUCE__);   // ❌ getNativeFunction undefined
```

**Lazy mount pattern (recommended):** mount on first Tuning-tab activation, NOT page load — saves boot cost when user never opens the Tuning tab. Pattern in O-MicrotonalSampler `sampler-app.js:332-356` (`ensureTuningPanelMounted()`).

### 2.6 Push-channel JS receivers

```js
// Active-voice dots row
window.__JUCE__.backend.addEventListener('activeVoiceCount', (count) => {
    renderVoiceDots(count);   // 0..16 filled dots
});

// Breath meter
window.__JUCE__.backend.addEventListener('effectiveBreath', (level) => {
    breathMeterFill.style.width = (level * 100) + '%';
});

// Vibrato pulsing dot
window.__JUCE__.backend.addEventListener('vibratoEnvelope', (env) => {
    vibratoDot.style.opacity = env;   // 0=invisible, 1=full
});
```

### 2.7 Family-canonical CSS variables (lift O-Wind palette)

```css
:root {
    --bg-paper:      #F5E6D3;
    --green-mid:     #6B8E4E;
    --brown-frame:   #5C4033;
    --brown-text:    #3C2F2F;
    --brown-muted:   #8B7355;
    --white-overlay: rgba(255, 255, 255, 0.4);
    --tuning-accent: #6B8E4E;
}

body {
    font-family: 'EB Garamond', 'Garamond', serif;
    background: var(--bg-paper);
    color: var(--brown-text);
}
```

---

## 3. CMakeLists patches (Stage 3 additions)

**Add to `plugins/O-Bassoon/CMakeLists.txt` (after the existing licensing block, before `target_compile_definitions`):**

```cmake
# WebView UI Resources (Stage 3)
juce_add_binary_data(O-Bassoon_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/fern.png
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Bassoon
    PRIVATE
        O-Bassoon_UIResources
)
```

**Existing `target_compile_definitions` already correct** — both `JUCE_WEB_BROWSER=1` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` present at lines 95-96.

**Existing `juce_add_plugin` already correct** — `NEEDS_WEB_BROWSER TRUE` (line 17), `NEEDS_WEBVIEW2 TRUE` (line 18).

---

## 4. File scaffold (Stage 3 net-new)

```
plugins/O-Bassoon/
├── CMakeLists.txt                          # MOD: add juce_add_binary_data block
├── Resources/                              # NEW
│   └── ui/
│       ├── index.html                      # NEW: from /ui-mockup output
│       ├── img/
│       │   └── fern.png                    # NEW: copy from O-Wind/Resources/ui/img/fern.png
│       └── js/
│           └── juce/
│               ├── index.js                # NEW: copy from O-Wind/Resources/ui/js/juce/index.js
│               └── check_native_interop.js # NEW: copy from same source
└── Source/
    ├── PluginEditor.h                      # REWRITE: replace GenericAudioProcessorEditor stub with WebView class
    └── PluginEditor.cpp                    # NEW: full WebView setup (relays + attachments + native fns + resource provider + Timer)
```

**Note:** `tuning-panel.{js,css}` are NOT copied per-plugin — referenced from `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/` directly in the `juce_add_binary_data` block (Pattern A per OQ#1).

---

## 5. PluginEditor skeleton

### 5.1 PluginEditor.h (REWRITE)

```cpp
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OBassoonAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit OBassoonAudioProcessorEditor(OBassoonAudioProcessor& p);
    ~OBassoonAudioProcessorEditor() override;

    void paint(juce::Graphics&) override {}
    void resized() override;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    OBassoonAudioProcessor& processorRef;

    // 10× WebSliderRelay (param-id keyed)
    std::unique_ptr<juce::WebSliderRelay> vibratoRateRelay, vibratoDepthRelay, vibratoOnsetRelay;
    std::unique_ptr<juce::WebSliderRelay> breathRelay, toneRelay, attackCharacterRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay, releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> voiceCountRelay, outputGainRelay;

    // WebView (constructed AFTER relays so .withOptionsFrom() works)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 10× WebSliderParameterAttachment (constructed AFTER WebView per family pattern)
    std::unique_ptr<juce::WebSliderParameterAttachment>
        vibratoRateAttachment, vibratoDepthAttachment, vibratoOnsetAttachment,
        breathAttachment, toneAttachment, attackCharacterAttachment,
        attackTimeAttachment, releaseTimeAttachment,
        voiceCountAttachment, outputGainAttachment;

    // Push-channel diff suppression
    int   lastEmittedActive  { -1 };       // sentinel for first-tick force-emit
    float lastEmittedBreath  { -1.0f };
    float lastEmittedVibEnv  { -1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassoonAudioProcessorEditor)
};
```

### 5.2 PluginEditor.cpp (NEW)

```cpp
#include "PluginEditor.h"
#include "BinaryData.h"

OBassoonAudioProcessorEditor::OBassoonAudioProcessorEditor(OBassoonAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays (param-id-keyed)
    vibratoRateRelay     = std::make_unique<juce::WebSliderRelay>("vibrato_rate");
    vibratoDepthRelay    = std::make_unique<juce::WebSliderRelay>("vibrato_depth");
    vibratoOnsetRelay    = std::make_unique<juce::WebSliderRelay>("vibrato_onset");
    breathRelay          = std::make_unique<juce::WebSliderRelay>("breath");
    toneRelay            = std::make_unique<juce::WebSliderRelay>("tone");
    attackCharacterRelay = std::make_unique<juce::WebSliderRelay>("attack_character");
    attackTimeRelay      = std::make_unique<juce::WebSliderRelay>("attack_time");
    releaseTimeRelay     = std::make_unique<juce::WebSliderRelay>("release_time");
    voiceCountRelay      = std::make_unique<juce::WebSliderRelay>("voice_count");      // AudioParameterInt → still slider
    outputGainRelay      = std::make_unique<juce::WebSliderRelay>("output_gain");

    // 2. Create WebView
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OBassoon_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // Relays
            .withOptionsFrom(*vibratoRateRelay)
            .withOptionsFrom(*vibratoDepthRelay)
            .withOptionsFrom(*vibratoOnsetRelay)
            .withOptionsFrom(*breathRelay)
            .withOptionsFrom(*toneRelay)
            .withOptionsFrom(*attackCharacterRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*releaseTimeRelay)
            .withOptionsFrom(*voiceCountRelay)
            .withOptionsFrom(*outputGainRelay)
            // Tuning native functions (~25; see RESEARCH §2.4 — lift from O-Wind verbatim)
            .withNativeFunction("getTuningIntervals", /* … */)
            // … (24 more — see §2.4)
    );

    // 3. Create attachments (3-arg ctor; nullptr undoManager per #12)
    vibratoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("vibrato_rate"), *vibratoRateRelay, nullptr);
    // … 9 more attachments

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // 4. Editor size + Timer
    setSize(900, 600);
    startTimerHz(30);
}

OBassoonAudioProcessorEditor::~OBassoonAudioProcessorEditor()
{
    stopTimer();   // BEFORE webView destruction
    // Destruction order (reverse of construction): attachments → webView → relays
}

void OBassoonAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());
}

void OBassoonAudioProcessorEditor::timerCallback()
{
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

std::optional<juce::WebBrowserComponent::Resource>
OBassoonAudioProcessorEditor::getResource(const juce::String& url)
{
    // Lift O-Wind PluginEditor.cpp:611-656 verbatim — see RESEARCH §2.3
    // … (full lambda body)
}
```

### 5.3 PluginProcessor patches

**Add to `PluginProcessor.h` (public section):**
```cpp
// Stage 3 push-channel atomics (audio-thread writer, message-thread reader)
std::atomic<int>   currentActiveVoiceCount { 0 };
std::atomic<float> currentEffectiveBreath  { 0.0f };
std::atomic<float> currentVibratoEnvelope  { 0.0f };

juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return parameters; }   // if not already exposed
```

**Add to `PluginProcessor.cpp` `processBlock`** (insert after the existing voice_count snapshot block at line 197-205, BEFORE tone-dispatch):
```cpp
// Stage 3 — live active-voice count snapshot (allocation-free, RT-safe)
{
    int active = 0;
    const int n = synthesiser.getNumVoices();
    for (int v = 0; v < n; ++v)
        if (synthesiser.getVoice(v)->isVoiceActive())
            ++active;
    currentActiveVoiceCount.store(active, std::memory_order_relaxed);
}

// Stage 3 — effective breath snapshot (UI breath × CC2 normalised)
// Pull from existing per-voice breathSmoother shadow state (Phase 2.3 wire).
const float uiBreath = parameters.getRawParameterValue("breath")->load();
const float cc2Norm  = currentCC2 / 127.0f;   // currentCC2 already maintained by Phase 2.3 takeover state machine
currentEffectiveBreath.store(uiBreath * cc2Norm, std::memory_order_relaxed);

// Stage 3 — vibrato envelope snapshot (sample first active voice's onset progress)
// Pull from BassoonVoice::getVibratoEnvelope() (NEW accessor, Phase 3.2 task).
float vibEnv = 0.0f;
for (int v = 0; v < synthesiser.getNumVoices(); ++v)
    if (auto* bv = dynamic_cast<BassoonVoice*>(synthesiser.getVoice(v)))
        if (bv->isVoiceActive()) {
            vibEnv = bv->getVibratoEnvelope();   // 0..1
            break;
        }
currentVibratoEnvelope.store(vibEnv, std::memory_order_relaxed);
```

**Add to `BassoonVoice.h` (public, Phase 3.2):**
```cpp
float getVibratoEnvelope() const noexcept { return vibrato.getEnvelope(); }   // proxy to Vibrato::getEnvelope
```

**Add to `Vibrato.h` (public, Phase 3.2):**
```cpp
float getEnvelope() const noexcept { return onsetEnvelope.getCurrentValue(); }
```

---

## 6. HTML structure preview (mockup-driven, schematic only)

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <link rel="stylesheet" href="/css/tuning-panel.css">
    <style>/* family palette + section grid */</style>
</head>
<body>
    <img class="botanical-overlay" src="/img/fern.png" alt="">

    <!-- Tab bar -->
    <nav class="tab-bar">
        <button class="tab-btn active" data-tab="sound">Sound</button>
        <button class="tab-btn" data-tab="tuning">Tuning</button>
        <button class="tab-btn" data-tab="about">About</button>
    </nav>

    <!-- Sound tab (4 sections × knob clusters) -->
    <section id="tab-sound" class="tab-panel active">
        <div class="section vibrato">
            <h3>Vibrato</h3>
            <!-- 3 knobs: vibrato_rate, vibrato_depth, vibrato_onset -->
            <div class="vibrato-dot" id="vibrato-dot"></div>   <!-- pulsing feedback -->
        </div>
        <div class="section expression">
            <h3>Expression</h3>
            <!-- 3 knobs: breath, tone, attack_character (Soft↔Tongued labels) -->
            <div class="breath-meter" id="breath-meter">
                <div class="breath-meter-fill"></div>          <!-- live feedback -->
            </div>
        </div>
        <div class="section envelope">
            <h3>Envelope</h3>
            <!-- 2 knobs: attack_time, release_time -->
        </div>
        <div class="section voicing">
            <h3>Voicing &amp; Output</h3>
            <!-- 2 knobs: voice_count, output_gain -->
            <div class="voice-dots" id="voice-dots"></div>     <!-- live feedback -->
        </div>
    </section>

    <!-- Tuning tab (lazy-mounted on first activation) -->
    <section id="tab-tuning" class="tab-panel">
        <div id="tuning-container"></div>
    </section>

    <!-- About tab (static) -->
    <section id="tab-about" class="tab-panel">
        <!-- title + version + tagline + blurb + Ouaricon link (per OQ#7) -->
    </section>

    <script type="module">
        import * as Juce from '/js/juce/index.js';
        // Bind 10 sliders, wire tab switcher, lazy-mount TuningPanel,
        // subscribe to 3 push channels (activeVoiceCount, effectiveBreath, vibratoEnvelope)
    </script>
</body>
</html>
```

**Mockup pass owns the exact HTML/CSS — this is informational only.**

---

## 7. Risks Updated (rev-2 from CONTEXT)

| # | Risk | Severity | Status | Mitigation |
|---|---|---|---|---|
| 1 | Tuning-tab embed underestimated | Medium | **Mitigated** by OQ#1 + §2.4 + §2.5 | Lift O-Wind native fn chain verbatim; lazy-mount per O-MicrotonalSampler |
| 2 | C++→JS push channels break PERF-01 | Medium | **Mitigated** by OQ#3 + OQ#4 + §2.6 | `std::atomic<T>` snapshot + 30 Hz Timer + diff suppression; pattern proven in O-Bells/O-MicrotonalSampler |
| 3 | Botanical overlay licensing | Low | **Resolved** by OQ#6 | Reuse O-Wind `fern.png` (public domain); mockup may override |
| 4 | Resource-provider regression | High → Low | **Mitigated** by §2.3 + grep gate | Lift O-Wind lambda verbatim; pre-commit `grep -rn fromFirstOccurrenceOf plugins/O-Bassoon/Source/` MUST return zero |
| 5 | Windows WebView2 silent IE fallback | High → Low | **Mitigated** by OQ#8 + CMake audit | Both flags already present (CMakeLists:18, 96); add `withUserDataFolder` to WebView Options |
| 6 | 3-arg `WebSliderParameterAttachment` footgun | Low | **Documented** by `juce8-critical-patterns.md #12` | `nullptr` undoManager required; lifted in §5.2 |
| 7 | Stale CONTEXT divergence at mockup | Medium | **Process control** | Amendments land as `(rev-2)` addendum at plan-phase, NOT discuss-rewrite |
| 8 | Live feedback DAW-side CPU overhead | Low | **Mitigated** by 30 Hz throttle + diff suppression | Family precedent (O-MicrotonalSampler emits at 30 Hz); diff-suppression skips emit when value within 0.005 |
| 9 | Tab switch state persistence | Low | **Defer** to v1.1 | Default "Sound tab on open"; document as known limitation |
| 10 | Mockup landing stalls Stage 3 | Medium | **Process control** | Mockup-orchestrator owns its own iteration loop; STATUS.md `next_action` flips only after mockup approval |
| **NEW 11** | `juce_add_binary_data` symbol naming mismatch | Low | **Mitigated** by O-Wind precedent | BinaryData symbol names are `tuningpanel_js`, `tuningpanel_css`, `fern_png`, `index_html`, `index_js`, `check_native_interop_js` — confirmed against O-Wind PluginEditor.cpp:622-651 |
| **NEW 12** | Active-voice count loop double-runs (cap-enforce + Stage 3 snapshot) | Low | **Acceptable** | The loop already runs in `BassoonSynthesiser::findFreeVoice` per note-on AND in processBlock prologue per Stage 3 snapshot; both are 16-iter integer reads. Total cost <1 µs/block at 256 buffer / 48 kHz. Optimisation deferred to v1.1 if profiling flags it. |
| **NEW 13** | `vst3Extensions.getPendingTable()` access from message-thread native fns may race with audio-thread NE drain | Low | **Mitigated** by NE module's lock-free design | `Ouaricon::NoteExpression::PendingTable` is documented lock-free (see module impl); native fn writes use the same atomic API the audio thread reads. No new synchronisation required. |

---

## 8. Pre-execute static-check grep battery (Stage 3 additions to existing 16)

| # | Check | Command | Pass criterion |
|---|---|---|---|
| 17 | Bare-path equality (no `fromFirstOccurrenceOf` regression) | `grep -rn 'fromFirstOccurrenceOf' plugins/O-Bassoon/Source/` | **0 matches** |
| 18 | 3-arg `WebSliderParameterAttachment` (per `#12`) | `grep -rn 'WebSliderParameterAttachment' plugins/O-Bassoon/Source/PluginEditor.cpp` | All matches end with `, nullptr);` |
| 19 | `withUserDataFolder` present (per OQ#8) | `grep -n 'withUserDataFolder' plugins/O-Bassoon/Source/PluginEditor.cpp` | **≥1 match**, value contains `OBassoon_WebView` |
| 20 | `withResourceProvider` present | `grep -n 'withResourceProvider' plugins/O-Bassoon/Source/PluginEditor.cpp` | **≥1 match** |
| 21 | `Juce` ES-module passed to TuningPanel (NOT `window.__JUCE__`) | `grep -n 'new TuningPanel' plugins/O-Bassoon/Resources/ui/index.html` (or app.js) | Match contains `, Juce)`, NOT `, window.__JUCE__)` |
| 22 | `type="module"` on Juce-importing script (per `#21`) | `grep -n 'type="module"' plugins/O-Bassoon/Resources/ui/index.html` | **≥1 match** |
| 23 | Resource provider serves all 6 expected paths | `grep -nE '"/" \|\| url == "/index.html"\|/js/juce/index.js\|/js/juce/check_native_interop.js\|/js/tuning-panel.js\|/css/tuning-panel.css\|/img/fern.png' plugins/O-Bassoon/Source/PluginEditor.cpp` | **6 matches** (or count match) |
| 24 | DSP-07 (no O-Reed refs in UI sources) | `grep -rn 'O-Reed\|OReed\|o-reed' plugins/O-Bassoon/Resources/ plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` | **0 matches** |
| 25 | `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` both present | `grep -nE 'NEEDS_WEBVIEW2 TRUE\|JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING' plugins/O-Bassoon/CMakeLists.txt` | **2 matches** |
| 26 | `juce_add_binary_data(O-Bassoon_UIResources` block present | `grep -n 'juce_add_binary_data' plugins/O-Bassoon/CMakeLists.txt` | **1 match** |
| 27 | Push-channel atomics declared | `grep -nE 'currentActiveVoiceCount\|currentEffectiveBreath\|currentVibratoEnvelope' plugins/O-Bassoon/Source/PluginProcessor.h` | **3 matches** |
| 28 | Editor inherits from `juce::Timer` | `grep -n 'private juce::Timer' plugins/O-Bassoon/Source/PluginEditor.h` | **1 match** |

---

## 9. Verification Gate Preview (Stage 3 / Gate 5)

**Phase 3.1 (layout + binding + tuning-tab embed) gate:**
1. Build clean (`cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone --parallel`) — 12/12 targets, zero warnings on PluginEditor.cpp
2. `auval -v aufx OBsn Ouar` (or whatever component code) — VALIDATION SUCCEEDED
3. `pluginval --strictness 5` — exit 0
4. Static-check battery #17–#26 (10 checks) — all PASS
5. Logic-AU smoke test:
   - All 10 knobs respond to drag (relative-drag pattern, no jump-to-cursor)
   - All 10 knobs round-trip parameter changes (move knob → audio responds; automate parameter → knob moves)
   - Tab switch Sound ↔ Tuning ↔ About works
   - Tuning tab: intervals table renders, Generate EDO button works (memory-known regression sentinel — OQ#1 / risk #1)
   - About tab: title/version/blurb/link visible

**Phase 3.2 (polish + 3 feedback elements + final verify) gate:**
1. Build clean + `pluginval --strictness 10` exit 0
2. Static-check battery #17–#28 (12 checks, including #27/#28 push-channel additions) — all PASS
3. Logic-AU full Gate 5:
   - Active-voice dots: play 1, 2, 4, 8 simultaneous notes — dots row reflects live count (NOT cap)
   - Breath meter: move CC2 (Mod Wheel rerouted via DAW) — meter responds; UI breath knob also responds
   - Vibrato dot: enable vibrato + hold note — dot pulses at vibrato_rate; vibrato_onset gates the pulse onset
   - 60 s long-tone sustain: no UI freeze, no audio drop, no visible memory growth in DAW process inspector
   - Tab switch under load: no audio glitch
4. Atomic commit: `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`

---

## 10. Open Items handed to plan-phase

1. **Mockup integration:** PLAN.md Phase 3.1 task #1 — convert mockup HTML/CSS to `Resources/ui/index.html` once `/ui-mockup O-Bassoon` lands.
2. **Doc backfill:** PLAN.md Phase 3.1 task — BRIEF D6 + ROADMAP Stage 3 D8 + REQUIREMENTS UI-01 amendment per OQ#10.
3. **`getAPVTS()` accessor:** confirm whether PluginProcessor already exposes `parameters` publicly or via accessor; if not, add public `getAPVTS()` (one-line) at PLAN.md Phase 3.1.
4. **`vst3Extensions` accessor:** confirm whether tuning native functions also need access to NE module for any purpose (probably not — they go to `tuningEngine` only).
5. **Vibrato envelope accessor chain:** PLAN.md Phase 3.2 task — add `Vibrato::getEnvelope()` + `BassoonVoice::getVibratoEnvelope()` proxies (5-line additions).
6. **Effective-breath snapshot site:** PLAN.md Phase 3.2 task — confirm whether to snapshot in `processBlock` or per-voice. Recommendation: processor-level `currentCC2` member (already maintained by Phase 2.3 takeover state machine) × `breath` parameter — single-site, no per-voice loop.
7. **Plugin version display:** hardcode `1.0.0` in About tab HTML for v1.0; defer `getPluginVersion` native fn to v1.1.

---

**Locked atomic commit subject (Stage 3 close):** `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`
**Inline iteration ceiling:** rev-3 (family precedent — Phase 2.x research-phases)
**Next phase:** plan-phase (`/plugin-plan O-Bassoon 3-gui`) — blocks on mockup approval AND this RESEARCH.md merged.
