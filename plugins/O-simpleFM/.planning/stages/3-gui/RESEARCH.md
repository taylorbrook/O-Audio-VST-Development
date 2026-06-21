# Stage 3 (GUI) — RESEARCH

> Reference patterns verified against the live codebase (2026-06-20). Exact files/line cues for the
> gui-agent. No external research needed — all patterns exist in the suite.

## Reference plugins (read these for exact code)

| Concern | Reference | What to copy |
|---------|-----------|--------------|
| Botanical theme + relay/attachment + resource provider + member order | **O-Prism** | `Source/PluginEditor.h` (member-order block), `Source/PluginEditor.cpp` (`getResource`, `addNativeFunctions`, relay creation `~1015`, `withResourceProvider`/`withNativeIntegrationEnabled` `~1038`, `withUserDataFolder` `~1066`, `goToURL(getResourceProviderRoot())` `~1149`), `CMakeLists.txt` (`juce_add_binary_data` block `~94`) |
| 30 Hz viz emit + canvas drawing | **O-Marimba** | `Source/PluginEditor.cpp` `timerCallback()` (`~272`): `webView->emitEventIfBrowserIsVisible("name", value)`; inline JS in `index.html` for `addEventListener` + canvas |
| Botanical theme spec | **`.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`** | color vars, Garamond, seed-knob conic-gradient, toggle styling, botanical overlay CSS, spacing |

## WebView wiring (JUCE 8) — canonical sequence

1. **CMake** — add to `plugins/O-simpleFM/CMakeLists.txt`:
   ```cmake
   juce_add_binary_data(O-simpleFM_UIResources
       SOURCES
           Source/ui/public/index.html
           Source/ui/public/js/juce/index.js
           Source/ui/public/js/juce/check_native_interop.js
           Source/ui/public/js/app.js
           Source/ui/public/css/styles.css
           Source/ui/public/img/<botanical>.png)
   target_link_libraries(O-simpleFM PRIVATE O-simpleFM_UIResources)
   ```
   WebView2 flags + `JUCE_WEB_BROWSER=1` are ALREADY present (Foundation). Keep
   `Source/*.h` in `target_sources` (Operator.h/FMVoice.h/FmVizAnalyzer.h already listed).

2. **Editor member order** (PluginEditor.h) — relays (vector<unique_ptr<WebSliderRelay>> +
   WebToggleButtonRelay) → `unique_ptr<WebBrowserComponent> webView` → attachments. Keep the
   existing `FmVizAnalyzer vizAnalyzer;` and `private juce::Timer`. Remove `genericEditor`.

3. **Relays/attachments** (PluginEditor.cpp ctor):
   - One `WebSliderRelay(id)` per float param (15), one `WebToggleButtonRelay(id)` per bool (2).
   - `Options{}.withNativeIntegrationEnabled().withResourceProvider(getResource)` then
     `.withOptionsFrom(*relay)` for EVERY relay (order doesn't matter across relays, but each
     must be registered before the WebView is constructed).
   - `#if JUCE_WINDOWS` → `.withWinWebView2Options(... .withUserDataFolder(tempDir/"OsimpleFM_WebView"))`.
   - Construct `webView`. Then create `WebSliderParameterAttachment(*apvts.getParameter(id), *relay, nullptr)`
     and `WebToggleButtonParameterAttachment(...)` — 3-arg form, `nullptr` undoManager.
   - `webView->goToURL(WebBrowserComponent::getResourceProviderRoot());`

4. **Resource provider** — `getResource(const String& url)` compares BARE PATHS by equality:
   `if (url == "/" || url == "/index.html") → index.html`; map `/js/app.js`, `/js/juce/index.js`,
   `/js/juce/check_native_interop.js`, `/css/styles.css`, `/img/<botanical>.png`. MIME types:
   text/html, text/javascript, text/css, image/png. Build `Resource{ {data...}, mime }` from
   `BinaryData::...`.

5. **Viz emit** (timerCallback, AFTER the existing `vizAnalyzer.process(...)`):
   ```cpp
   if (webView != nullptr) {
       const auto& spec  = vizAnalyzer.getSpectrum(); // 256 dB bins
       const auto& scope = vizAnalyzer.getScope();    // 128 pts
       juce::Array<juce::var> sa, sc;
       for (float v : spec)  sa.add (v);
       for (float v : scope) sc.add (v);
       webView->emitEventIfBrowserIsVisible ("spectrumUpdate", juce::var (std::move (sa)));
       webView->emitEventIfBrowserIsVisible ("scopeUpdate",    juce::var (std::move (sc)));
   }
   ```
   (If passing a large array as `var` is awkward, the O-Marimba alternative is a JS-side
   `getNativeFunction` poll; emit is preferred and simpler here.)

## JS side (app.js, type="module")

- `import * as Juce from './js/juce/index.js';`
- Sliders: `Juce.getSliderState("ratio")` → `.valueChangedEvent`/`.setNormalisedValue`; or use
  the relay convention the suite uses (match O-Prism's `wavetable-editor.js`).
- Toggles: `Juce.getToggleState("ratioSnap")`.
- Viz: `window.__JUCE__.backend.addEventListener("spectrumUpdate", arr => drawSpectrum(arr));`
  and `"scopeUpdate"`. Canvas DPR-aware backing store.
- Spectrum bins are dB in ≈[-100,0]; map to bar height. Scope pts in [-1,1]; map to y.
- Routing diagram: SVG/CSS, MOD→CAR + feedback arc; opacity/width reflects `feedback`/`modIndex`.
- Tooltips: JS const map keyed by param id; show on hover.
- Preset tour: buttons that set parameter values via slider/toggle states (APVTS snapshot per patch).

## Theme application (ouaricon-naturalist-001)

- CSS vars from `aesthetic.md` "Example Color Codes" (paper `#F5E6D3`, brown `#8B7355`,
  green `#6B8E4E`, text `#3C2F2F`). Garamond stack.
- Seed-knob: conic-gradient 10-segment + radial ring + inner core + inset shadows (spec §Controls).
- Toggles: green tint default / deeper-green active, fleuron corner, uppercase labels.
- Botanical overlay: pick a **synth-appropriate** specimen — an **insect** (buzzing/sideband
  character) or **seed/fruit** (FM = "spectrum grows from a seed ratio"). Copy to
  `Source/ui/public/img/`, `.botanical-overlay` right-side ~0.35 opacity, `pointer-events:none`,
  left 60-70% clear for controls + viz. 17 params → compact 55px knobs, grouped sections.

## Pitfalls to avoid (project memory)

- `window.__JUCE__` has NO `getNativeFunction` — use `Juce` namespace for state/native fns.
- Resource provider gets bare paths — don't `fromFirstOccurrenceOf("://")`.
- Static-link WebView2 flag already set (good) — don't switch to dynamic.
- Verify every JS helper reference after writing — a single ReferenceError silently blanks the UI
  (passes auval/build). Open Standalone to confirm.
- Canvas: explicit width/height + DPR backing store, or it renders 300×150 / blurry on Retina.
