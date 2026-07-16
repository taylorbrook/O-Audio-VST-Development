# Stage 3: GUI — Research

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 3 (GUI) — research phase
**Date:** 2026-06-27
**Inputs:** CONTEXT.md (7 decisions D1–D7), parameter-spec.md (17 locked IDs), VizTap.h (Stage-2 data contract), ROADMAP.md (3.1/3.2/3.3), live source (PluginProcessor, PhysicalModelVoice). Three sibling deep-dives: **O-simpleFM** (full WebView template + spectrum/scope + keyboard + preset bar), **preset-manager** shared module, **O-simpleAdditive** (naturalist palette + group/knob markup) + **O-simpleGrain** (conditional grey-out).

---

## 1. Summary

Stage 3 is a **near-verbatim port of the O-simpleFM WebView template** plus **one genuinely new widget** (the animated loop/flow diagram, UI-02). Everything load-bearing — CMake WebView config, editor relay/attachment wiring, resource provider, the 30 Hz message-thread FFT spectrum + scope, the DPR-aware canvas, the tooltip system, the on-screen keyboard, the preset bar — already exists in shipping siblings and ports with mechanical edits. The risk surface is small and concentrated in: (a) the SVG loop diagram driven by the real `loopEnergy` scalar, (b) the resonator-/exciter-aware grey-out (no sibling drives this off a *choice* param yet — but the toggle-driven pattern is a one-line swap), and (c) two seams that fail **silently** if done wrong (the harness `#include` guard, and the WebView native-fn bridge).

**Bottom line:** Low-to-moderate risk. No new external research needed — this is an integration/assembly stage against proven in-house patterns.

---

## 2. Reuse map (port verbatim vs adapt vs new)

| Area | Source | Disposition |
|------|--------|-------------|
| CMake WebView flags + binary-data target | O-simpleFM `CMakeLists.txt` | **Port** (flags already set in Stage 1; add the one `juce_add_binary_data` target) |
| Editor relay→WebView→attachment wiring + member order | O-simpleFM `PluginEditor.{h,cpp}` | **Port** (15 sliders + 2 toggles → **14 sliders + 3 combos, 0 toggles** here) |
| Resource provider (bare-path `==` matcher) | O-simpleFM `getResource()` | **Port** verbatim (update file list + BinaryData symbol names) |
| Message-thread FFT spectrum + scope | O-simpleFM `FmVizAnalyzer.h::process()` | **Port** the analyzer half; reuse our existing `VizTap::VizRing` (do NOT re-paste VizRing) |
| DPR-aware canvas (`makeCanvas`) | O-simpleFM / O-simpleAdditive `app.js` | **Port** verbatim |
| Tooltip system (`#tooltip` div + `data-tip` + `TIPS` map) | O-simpleAdditive `app.js` + `styles.css` | **Port** verbatim; author new `TIPS` content from BRIEF |
| Naturalist palette + `.frame/.group/.knob-row/.knob` markup + CSS | O-simpleAdditive `styles.css` + `index.html` | **Port** the building blocks; **re-author** top-level layout (vertical → left→right) |
| Knob component (CSS disc + JS vertical drag) | O-simpleAdditive `app.js` | **Port** verbatim |
| Combo (`getComboBoxState`/`getChoiceIndex`) | O-simpleAdditive `app.js:246-275` | **Port** (3 choice params) |
| Preset bar (C++ `OuariconPresetManager` + JS `PresetManager` + 10 native fns) | preset-manager module + O-simpleFM | **Port** (D4 — shell in S3, factory set in S4) |
| On-screen keyboard (`uiMidi` → `handleUiMidi` → `MidiMessageCollector`) | O-simpleFM | **Port** + **add processor plumbing** (not present in our Stage 1) |
| Resonator-/exciter-aware grey-out | O-simpleGrain `.env-bypassed` pattern | **Adapt** (swap toggle read for `getChoiceIndex()`) |
| **Animated loop/flow diagram (UI-02) + modal stems (UI-05)** | — | **NEW** (inline SVG, driven by `loopEnergy` + 8 stems) |

---

## 3. Open questions from CONTEXT — resolved

### Q1 — Window dimensions + on-screen keyboard?
**Recommendation: ~1040 × 860, fixed (not resizable); include the on-screen keyboard.**
Siblings: O-simpleFM 760×980, O-simpleAdditive 860×980, O-simpleGrain 900×760 — all *taller than wide* because their layouts stack vertically. Our layout is **left→right signal-flow** (4 stages) with a **prominent central viz**, so it wants a **wider** canvas. Proposed vertical budget: header+preset bar (~64px) → viz row (~300px: loop diagram center + spectrum/scope) → 4 control columns (~300px) → keyboard (~120px) → padding. Keep the keyboard (every pedagogical sibling has one; the BRIEF's "play harder → brighter" story needs live playing). Exact px are tunable during execute — fix them once the panels are laid out.

### Q2 — Loop-diagram render tech: SVG vs canvas?
**Recommendation: inline SVG for the loop/flow diagram; keep `<canvas>` for spectrum + scope.**
The diagram is a labelled **block diagram** (EXCITATION→RESONATOR→MATERIAL→OUT boxes + a circulating loop arrow) — declarative geometry + text, which SVG/DOM does far more cleanly than canvas, and it sidesteps the canvas replaced-element sizing gotcha entirely. Animate by updating SVG element attributes/CSS (opacity, transform, `<rect>` height) each 30 Hz tick from `loopEnergy`/stems. This mirrors O-simpleFM, which already uses **inline SVG for its routing diagram** and **canvas for spectrum/scope**. The two canvases reuse the proven DPR-aware `makeCanvas` verbatim.

### Q3 — Spectrum analyzer liftability + modal inharmonic display?
**Liftable almost verbatim.** `FmVizAnalyzer::process(const VizRing&, double sr)` ports directly because its signature already takes a `VizRing&` — and our `VizTap` exposes exactly that (`viz.waveform`). **Caveat:** `FmVizAnalyzer.h` *defines* its own `VizRing`, but our `VizTap.h` already ported `VizRing` verbatim. Do **not** paste FmVizAnalyzer wholesale (duplicate `VizRing` definition). Instead create a slim **`PmVizAnalyzer.h`** holding only the analyzer state (FFT order 12 / 4096, Blackman-Harris window, 256 log-spaced dB bins, 128-pt max-abs scope, asymmetric smoothing 0.5↑/0.1↓) and `process(processor.getVizTap().waveform, sr)`. The **log-frequency axis already renders inharmonic spacing correctly** — modal peaks at `f_k = f0·k·√(1+B·k²)` appear unevenly spaced vs the String harmonic comb. No special modal mode needed in the spectrum; the *exact* mode freqs/amps are shown by the stem widget (Q4), not the FFT.

### Q4 — Modal stem display: separate panel vs Modal skin of the diagram?
**Recommendation: fold the stems INTO the diagram's Modal skin — one widget, not two.** D2 already says the diagram "re-skins itself to modal stems in Modal mode," and CONTEXT flags avoiding duplicate data widgets. So: the loop/flow diagram's **resonator stage morphs** — String mode shows the circulating delay-loop (pulse around the loop arrow); **Modal mode replaces that interior with the 8 stems** (vertical lines at `freq→x`, `amp→height`, decaying as the note rings). This satisfies UI-02 and UI-05 with a single SVG panel. The spectrum canvas (live FFT) and the stem skin (exact bank values) are **complementary, not duplicative** — FFT magnitude of the sounding signal vs the bank's actual `(f_k, amp_k)`.

### Q5 — Timer rate + scope downsample?
**Port O-simpleFM exactly: 30 Hz `startTimerHz(30)`.** Scope window 1024 → 128 display points (8:1 max-abs, sign-preserving). Spectrum 4096-pt FFT → 256 log bins. The same 30 Hz tick reads `loopEnergy` + the 8 stems and drives the SVG diagram (30 Hz is smooth enough for the visible per-pass dampening; matches the sibling and the envelope-follower's time constant).

---

## 4. WebView foundation (Phase 3.1)

### 4.1 CMakeLists.txt
WebView flags were set in **Stage 1** (`NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`). Stage 3 adds **one** binary-data target (Stage 1 deliberately omitted it):

```cmake
# BEFORE juce_add_binary_data (the module helper copies preset-manager.js into the embed dir):
ouaricon_add_module(O-simplePhysicalModelSynth preset-manager)   # see §8

juce_add_binary_data(O-simplePhysicalModelSynth_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/modules/preset-manager.js)
# link FIRST in the PRIVATE list:
target_link_libraries(O-simplePhysicalModelSynth PRIVATE O-simplePhysicalModelSynth_UIResources ...)
# juce_generate_juce_header(...) stays AFTER target_link_libraries (JUCE-8 requirement).
```
- **One** target → default `BinaryData` namespace is fine (we embed **no** audio samples, so the dual-binary-data namespace collision does NOT apply here — but if that ever changes, give the 2nd target a distinct `NAMESPACE`).
- BinaryData symbol mangling: `preset-manager.js` → `BinaryData::presetmanager_js`, `js/juce/index.js` → `BinaryData::index_js`.

### 4.2 Editor wiring (PluginEditor.h/.cpp — NEW files)
**Member declaration order is load-bearing** (reverse-destruction safety — wrong order = release-build crash on reload):
```cpp
// 1) RELAYS   (declared first → destroyed last)
std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;
std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;     // 3 choice params
// 2) WEBVIEW
std::unique_ptr<juce::WebBrowserComponent>              webView;
// 3) ATTACHMENTS (declared last → destroyed first, WebView still alive)
std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>   sliderAttachments;
std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;
PmVizAnalyzer vizAnalyzer;                  // message-thread FFT (§6)
std::unique_ptr<juce::FileChooser> fileChooser;   // held while async preset dialog open (§8)
```
Construction order in the ctor body: **(1)** build all relays → **(2)** `Options{}.withNativeIntegrationEnabled().withResourceProvider(...)` then `for (relay) options = options.withOptionsFrom(*relay)` → **(3)** register native functions (preset ×10, `uiMidi`, `getSampleRate`) → **(4)** `#if JUCE_WINDOWS` `withWinWebView2Options().withUserDataFolder(tempDir/"OsimplePMS_WebView")` → **(5)** `webView = make_unique<WebBrowserComponent>(options)` → **(6)** build attachments (3-arg `(*param, *relay, nullptr)`, `jassert(param)` each) → **(7)** `addAndMakeVisible(*webView); webView->goToURL(getResourceProviderRoot()); setSize(1040,860); startTimerHz(30);`.

### 4.3 Resource provider (port verbatim)
Callback receives a **bare path** — compare with `==`, never strip scheme/host:
```cpp
if (url == "/" || url == "/index.html") return makeBinaryResource(BinaryData::index_html, ..., "text/html; charset=utf-8");
if (url == "/css/styles.css")           return ... "text/css; charset=utf-8";
if (url == "/js/app.js")                return ... "application/javascript; charset=utf-8";
if (url == "/js/juce/index.js")         return ... ; // BinaryData::index_js
if (url == "/js/juce/check_native_interop.js") return ...;
if (url == "/modules/preset-manager.js") return ... ; // BinaryData::presetmanager_js
return std::nullopt;
```
Keep `charset=utf-8` on every text resource (the naturalist UI uses ♪ ❦ – entities that mojibake otherwise).

---

## 5. Parameter binding (Phase 3.1) — all 17, resonator-aware

### 5.1 Control-type split (17 = 3 combos + 14 sliders, 0 toggles)
| Type | Relay / Attachment | Param IDs |
|------|--------------------|-----------|
| **Choice** (3) | `WebComboBoxRelay` + `WebComboBoxParameterAttachment` | `excitationType` (Pluck/Strike/Bow), `resonatorType` (String/Modal), `stringModel` (Karplus-Strong/Waveguide) |
| **Slider** (14) | `WebSliderRelay` + `WebSliderParameterAttachment` | `excitationPosition`, `excitationColor`, `bowForce`, `inharmonicity`, `modeBrightness`, `damping`, `decay`, `material`, `coarseTune`*, `fineTune`, `ampAttack`, `ampRelease`, `velToBrightness`, `outputLevel` |

*`coarseTune` is `AudioParameterInt` (−24…+24) — it is still a `RangedAudioParameter`, so a `WebSliderRelay`/`WebSliderParameterAttachment` binds it fine; it steps by 1. JS reads/writes via `Juce.getSliderState`. No toggles in this plugin.

The JS binds choices with `Juce.getComboBoxState(id)` (`.getChoiceIndex()/.setChoiceIndex()`) and sliders with `Juce.getSliderState(id)` (`.getNormalisedValue()/.setNormalisedValue()` + `sliderDragStarted/Ended`). **Relay/attachment ID strings must equal the APVTS IDs and the JS `KNOB_IDS`/`COMBO_IDS` exactly** — a drift = a silently dead control (`jassert(param)` catches it in Debug only).

### 5.2 Material macro — co-move is automatic
`parameterChanged()` already writes `damping` + `decay` via `setValueNotifyingHost()` on the message thread (PluginProcessor.cpp:202-220). Because those writes notify the host, the **WebSliderParameterAttachments for `damping` and `decay` update on their own** — moving Material visibly moves both knobs with **no extra JS**. Just bind all three as ordinary sliders. (Verify in execute: drag Material, watch Damping+Decay knobs track.)

### 5.3 Resonator-/exciter-aware grey-out (D5)
No sibling drives a grey-out off a **choice** param yet — but O-simpleGrain does exactly this off a **toggle**, and the swap is mechanical (`getValue()` → `getChoiceIndex()`).

**Pattern (adapt O-simpleGrain `app.js:280-291` + `.env-bypassed` CSS):**
```js
// CSS (port O-simpleGrain styles.css:381-382, optionally + Polystutter grayscale):
// .knob-cell.pm-disabled { opacity:0.38; pointer-events:none; transition:opacity .2s ease; filter:grayscale(30%); }

function applyEngineGating() {
  const res = Juce.getComboBoxState("resonatorType").getChoiceIndex();   // 0=String,1=Modal
  const exc = Juce.getComboBoxState("excitationType").getChoiceIndex();  // 0=Pluck,1=Strike,2=Bow
  setDisabled("stringModel",   res !== 0);          // String-only
  setDisabled("inharmonicity", res !== 1);          // Modal-only
  setDisabled("modeBrightness",res !== 1);          // Modal-only
  setDisabled("bowForce",      exc !== 2);          // Bow-only
}
// wire once, plus on every change:
Juce.getComboBoxState("resonatorType").valueChangedEvent.addListener(applyEngineGating);
Juce.getComboBoxState("excitationType").valueChangedEvent.addListener(applyEngineGating);
applyEngineGating();   // initial state at boot
```
`setDisabled(id, bool)` toggles `.pm-disabled` on that control's `.knob-cell`/`.combo-cell`. **The selectors (`resonatorType`, `excitationType`) live in their own panels — outside any dimmed group — so `pointer-events:none` never traps the user** (O-Polystutter v1.0.2 regression lesson). Grey-out **disables but keeps visible** (D5 — pedagogical: students see the full surface).

> **`stringModel` / Waveguide note:** Stage 2 shipped **KS-only** (Waveguide deferred to v1.1). In String mode `stringModel` is enabled, but selecting "Waveguide" is currently a no-op. Plan decision: either disable the Waveguide *option*, or leave it selectable with a tooltip "Waveguide — coming in v1.1." Recommend the tooltip (honest, low-effort, no contract change).

---

## 6. Visualization — spectrum + scope (Phase 3.2)

### 6.1 New `PmVizAnalyzer.h` (message-thread only)
Lift the analyzer half of `FmVizAnalyzer.h` — **omit its `VizRing`** (reuse `VizTap`'s). Constants: `kFftOrder=12` (4096), Blackman-Harris window, `kSpectrumBins=256` (log 20 Hz→Nyquist), `kScopeWindow=1024`, `kScopePoints=128`. `process(const VizRing& ring, double sr)`:
1. **Scope FIRST** (the FFT clobbers its work buffer): `ring.readLatest(scopeRaw,1024)` → 128 max-abs (sign-kept) points, clamped ±1.
2. **Spectrum:** `ring.readLatest(work,4096)` → window → `fft.performFrequencyOnlyForwardTransform` → 256 log-spaced bins, `mag/4096`, `gainToDecibels(mag+1e-9,-100)`, per-bin asymmetric smoothing (coeff 0.5 rising, 0.1 falling).

### 6.2 Editor `timerCallback()` (30 Hz)
```cpp
vizAnalyzer.process(processorRef.getVizTap().waveform, processorRef.getCurrentSampleRate());
if (webView == nullptr) return;
// pack spectrum (256) + scope (128) into juce::Array<juce::var>, then:
webView->emitEventIfBrowserIsVisible("spectrumUpdate", std::move(specArr));
webView->emitEventIfBrowserIsVisible("scopeUpdate",    std::move(scopeArr));
// + loopUpdate (§7)
```
JS receives via `window.__JUCE__.backend.addEventListener("spectrumUpdate"|"scopeUpdate", ...)` and draws onto the two DPR-aware canvases (`drawSpectrum` = 256 bars log axis; `drawScope` = 128-pt polyline). **`getCurrentSampleRate()` exists on `AudioProcessor`** — no new processor method needed for the analyzer (siblings expose `getSampleRate` as a native fn only for the JS log-axis labels; optional).

---

## 7. Loop/flow diagram + modal stems (Phase 3.3) — the NEW widget (UI-02 + UI-05)

**Render:** inline `<svg>` in `index.html`; animate by attribute/CSS updates each 30 Hz tick. **Two skins**, toggled on `resonatorType`:

- **String skin (KS loop):** the BRIEF's own block diagram — `[EXCITATION] → [RESONATOR ⟲] → [MATERIAL] → [OUT]`. A pulse element travels the loop arrow; its **opacity/glow = `loopEnergy`**, so it **visibly dims on each pass** as the note decays. (Waveguide rails skin deferred with the Waveguide engine — v1.1.)
- **Modal skin (stems):** the RESONATOR box interior becomes **8 vertical stems** — `x = log-map(stemFreq[k])`, `height = stemAmp[k]` — ringing/decaying live. This **is** the UI-05 stem display (one widget, no duplication — Q4).

**Data path:** editor reads `viz.getLoopEnergy()` and `viz.getStemFreq(k)/getStemAmp(k)` (k=0…7) each tick and emits one `loopUpdate` event:
```cpp
auto* o = new juce::DynamicObject();
o->setProperty("energy", processorRef.getVizTap().getLoopEnergy());
juce::Array<juce::var> sf, sa;
for (int k=0;k<VizTap::kStems;++k){ sf.add(viz.getStemFreq(k)); sa.add(viz.getStemAmp(k)); }
o->setProperty("stemFreqs", sf); o->setProperty("stemAmps", sa);
webView->emitEventIfBrowserIsVisible("loopUpdate", juce::var(o));
```
JS knows the current resonator from `getComboBoxState("resonatorType")` — no need to send it. **`loopEnergy` is a 0…~1 envelope-follower** (`loopEnergyEst += 0.001·(|dry|−loopEnergyEst)`, PhysicalModelVoice.h:208) — it rises on attack and decays with the note, so the dampening animation is driven by *real* circulating energy (BRIEF's "what students see is what they hear"). Stems are `(Hz, linear-amp)` from the live modal bank.

> Tooltips for the diagram boxes (UI-06) carry the teaching text: pitch = SR ÷ delay length; feedback near one sustains; higher modes decay faster; what inharmonicity does.

---

## 8. Preset bar (D4) — Phase 3.1 (shell), populated Phase 4

D4 puts the preset bar in Stage 3 (it is **not** in the original ROADMAP 3.1–3.3 component lists — it's a discuss-phase addition; wire it in 3.1 alongside the header/layout). The `simple*` family normally defers `ouaricon_add_module(... preset-manager)` to Stage 4 — but D4 supersedes that: wire the **shell** (module + C++ backend + 10 native fns + JS bar) now; seed the **factory preset set** in Stage 4.

- **CMake:** `ouaricon_add_module(O-simplePhysicalModelSynth preset-manager)` **before** `juce_add_binary_data`, and list `Source/ui/public/modules/preset-manager.js` as a binary-data source (the helper copies it there).
- **Processor:** add `OuariconPresetManager presetManager;` member, init `presetManager(parameters, "O-simplePhysicalModelSynth")` in the ctor list, and **switch state I/O** from plain APVTS XML to `presetManager.getStateAsXml()/setStateFromXml()` (replaces the current `getStateInformation/setStateInformation` bodies). `initializeFactoryPresets({...})` stays empty/Default-only until Stage 4.
- **Editor:** register all **10** native fns — `savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile, getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset, deletePreset, isFactoryPreset`. `savePresetWithDialog`/`loadPresetFromFile` are **not** class methods — implement them in the editor with the held `juce::FileChooser` returning `{success, name}` (copy O-simpleFM verbatim). `selectNextPreset`/`selectPreviousPreset` map to C++ `getNextPreset()`/`getPreviousPreset()` (return name, JS then `loadPreset`s it).
- **JS:** `new PresetManager({ ..., getNativeFunction: Juce.getNativeFunction })` — **must** pass `Juce.getNativeFunction`, **NOT** `window.__JUCE__` (the namespace gotcha — see §10).
- Storage: `~/Library/O-simplePhysicalModelSynth/Presets/{Factory,User}/`.

---

## 9. Tooltips (UI-06) — Phase 3.3

Port O-simpleAdditive's system verbatim: a single floating `#tooltip` div + `data-tip="key"` on every control cell + a `TIPS` map (`key → [title, body]`). `setupTooltips()` wires `pointerenter/move/leave/down` + `focusin/out` (keyboard a11y) + Escape, with viewport-edge flipping. CSS: `.tooltip { position:fixed; z-index:50; opacity:0; visibility:hidden; pointer-events:none } .tooltip.show{opacity:1;visibility:visible}`. **Avoid native `title=`** (slow, unstyled OS tooltips). Author `TIPS` content from the BRIEF examples (one plain-language line per control).

---

## 10. On-screen keyboard + processor plumbing (Phase 3.1/3.2)

Siblings include a keyboard; keep it (the "play harder → brighter" story needs live input). Port O-simpleFM's keyboard JS (C3–C5, QWERTY map, mouse + computer-key). **New processor plumbing required** (our Stage 1 has none):
- Editor: `uiMidi` native fn → `processorRef.handleUiMidi((int)args[0],(bool)args[1], args.size()>=3?(float)args[2]:0.8f)`.
- Processor: add `void handleUiMidi(int note,bool on,float vel)` building a `MidiMessage::noteOn/Off`, **timestamp in seconds** (`Time::getMillisecondCounterHiRes()*0.001`), into a `juce::MidiMessageCollector midiCollector;` member; `midiCollector.reset(sampleRate)` in `prepareToPlay`; drain at the **top** of `processBlock`: `midiCollector.removeNextBlockOfMessages(midiMessages, numSamples);` before `readParams()/renderNextBlock`.
- `MidiMessageCollector` is **not** a WebView type → adding it to PluginProcessor.cpp keeps the harness clean.

---

## 11. Render-harness seam — the SILENT footgun (do this right)

Our `tests/render-harness/CMakeLists.txt` compiles **`PluginProcessor.cpp`** under **`JUCE_WEB_BROWSER=0`** and deliberately does **not** compile `PluginEditor.cpp`. Today `createEditor()` is **inlined in `PluginProcessor.h`** returning `GenericAudioProcessorEditor` for both branches, and the header includes **no** WebView/editor types — that is why the harness builds.

**Stage 3 must preserve this.** ⚠ **Diverge from O-simpleFM**, whose `PluginProcessor.cpp` includes `"PluginEditor.h"` **unconditionally** (O-simpleFM:13) — copying that verbatim drags WebView types into our harness TU under `JUCE_WEB_BROWSER=0` → un-buildable (the O-simpleBeatmaker lesson). Correct seam:

```cpp
// PluginProcessor.h — replace the inlined createEditor with a declaration:
juce::AudioProcessorEditor* createEditor() override;

// PluginProcessor.cpp — GUARD the include and the WebView branch:
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif
juce::AudioProcessorEditor* OSimplePhysicalModelSynthAudioProcessor::createEditor()
{
   #if JUCE_WEB_BROWSER
    return new OSimplePhysicalModelSynthAudioProcessorEditor (*this);
   #else
    return new juce::GenericAudioProcessorEditor (*this);   // harness build
   #endif
}
```
Under `JUCE_WEB_BROWSER=0` the include + WebView branch vanish → harness compiles `PluginProcessor.cpp` with zero WebView types. Keep `PluginEditor.cpp` out of the harness `SOURCES`. **Re-run the harness at the START of Stage 4** to catch any regression (per ROADMAP).

---

## 12. Pitfalls / gotchas checklist (all carry to plan)

1. **`Juce` vs `window.__JUCE__`** — pass `Juce.getNativeFunction` (ES-module namespace) into `PresetManager`; `window.__JUCE__` has no `getNativeFunction` → every preset call silently throws inside try/catch (dead bar, no error). `window.__JUCE__.backend` is correct **only** for incoming `addEventListener` viz events. (`critical_juce_webview_namespace_vs_postmessage`.)
2. **WebView native-fn bridge gaps fail silently** — grep-diff JS `getNativeFunction(...)` names vs C++ `withNativeFunction(...)` names; all **10** preset fns + `uiMidi` (+ optional `getSampleRate`) must match exactly. (`pattern_webview_native_fn_bridge_gap`.)
3. **Harness `#include` guard** — §11; the single most likely Stage-3→Stage-4 regression.
4. **Editor member order** — relays → webView → attachments (reverse-destruction crash otherwise).
5. **Param-ID drift** — relay/attachment strings == APVTS IDs == JS `KNOB_IDS`/`COMBO_IDS`; `jassert(param)` per attachment.
6. **Resource provider = bare paths**, `==` match, `charset=utf-8` on text; BinaryData symbol mangling (`presetmanager_js`, `index_js`).
7. **Canvas replaced-element gotcha** — DPR backing store (`canvas.width = clientWidth*dpr; ctx.setTransform(dpr,...)`) + CSS `width/height:100%` inside a positioned `overflow:hidden` wrap (NOT `right/bottom`). (Diagram is SVG → unaffected; spectrum/scope canvases need this.)
8. **Windows WebView2** — flags already in Stage 1; still set `withUserDataFolder(tempDir)` (DAW sandbox denial → blank page / silent IE fallback).
9. **Grey-out re-enable escape hatch** — keep the choice selectors outside any `pointer-events:none` group.
10. **Module helper ordering** — `ouaricon_add_module` before `juce_add_binary_data`.
11. **Single binary-data target** → `BinaryData` namespace OK (no samples embedded). Distinct `NAMESPACE` only if a 2nd target ever appears.

---

## 13. Phase mapping + suggested files

| Phase | Deliverable | New / edited files |
|-------|-------------|--------------------|
| **3.1 Layout + binding + preset shell** | Single-page left→right WebView; 17 params bound; resonator-aware grey-out; preset bar shell; keyboard | NEW `Source/ui/public/{index.html, css/styles.css, js/app.js, js/juce/index.js, js/juce/check_native_interop.js}`; NEW `Source/ui/public/modules/preset-manager.js` (helper-copied); NEW `Source/PluginEditor.{h,cpp}`; EDIT `PluginProcessor.{h,cpp}` (createEditor seam §11, presetManager member + state I/O §8, handleUiMidi + midiCollector §10); EDIT `CMakeLists.txt` (binary-data target + module). |
| **3.2 Scope + spectrum** | Live waveform/decay scope + log spectrum (inharmonic spacing in Modal) | NEW `Source/PmVizAnalyzer.h`; EDIT `PluginEditor.cpp` (timerCallback emits spectrum/scope); EDIT `app.js` (`drawSpectrum`/`drawScope`, `makeCanvas`). |
| **3.3 Loop diagram + stems + tooltips** | SVG loop/flow diagram (String skin) + Modal stem skin, both driven by `loopEnergy`/stems; tooltips on every control | EDIT `index.html` (inline SVG diagram + `#tooltip` + `data-tip`), `app.js` (`loopUpdate` handler, skin morph, `setupTooltips`, `TIPS`), `styles.css` (diagram + tooltip CSS); EDIT `PluginEditor.cpp` (emit `loopUpdate`). |

**Verifies:** UI-01 (3.1), UI-03/UI-04 (3.2), UI-02/UI-05/UI-06 (3.3).

---

## 14. References

**In-house (port targets):**
- `plugins/O-simpleFM/` — `CMakeLists.txt`, `Source/PluginEditor.{h,cpp}`, `Source/FmVizAnalyzer.h`, `Source/PluginProcessor.{h,cpp}` (createEditor + keyboard `handleUiMidi`/`MidiMessageCollector`), `Source/FactoryPresets.{h,cpp}`, `Source/ui/public/{index.html,css/styles.css,js/app.js}`. **The primary template.**
- `plugins/O-simpleAdditive/` — naturalist `:root` palette, `.frame/.group/.knob-row/.knob` markup + CSS, knob JS, combo binding, tooltip system (`app.js` + `styles.css:480-526`).
- `plugins/O-simpleGrain/` — conditional grey-out (`app.js:280-291`, `styles.css:381-382 .env-bypassed`).
- `plugins/O-Polystutter/` — multi-section dimming + grayscale + re-enable escape hatch (`parameter-bindings.js:632-751`, `index.html:584-621`).
- `plugins/O-Formant/` — the only existing `getChoiceIndex()`-driven visual gate (`main.js:831-845`).
- `modules/persistence/preset-manager/` — `cpp/OuariconPresetManager.h`, `js/preset-manager.js`; `modules/cmake/OuariconModules.cmake` (`ouaricon_add_module`).

**Local Stage-2 contract:**
- `Source/VizTap.h` — `VizRing waveform` (8192), `getLoopEnergy()`, `getStemFreq/Amp(k)` (k=0…7).
- `Source/PhysicalModelVoice.h:79-84,208` — `publishViz()`, `loopEnergyEst` envelope follower.
- `Source/PluginProcessor.cpp:202-220,302-320` — Material macro write-back, `publishViz` lead-voice tap.

**Memory (gotchas):** `critical_juce_webview_namespace_vs_postmessage`, `pattern_webview_native_fn_bridge_gap`, `pattern_render_harness_breaks_on_webview_editor`, `critical_dual_binary_data_namespace_collision`, `feedback_module_extraction_regression_check`, Canvas-replaced-element note (MEMORY.md).
