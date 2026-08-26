# Quick Task 260826-ieq: Multi-language tooltips across all VST plugin UIs — Research

**Researched:** 2026-08-26
**Domain:** JUCE 8 WebView UI / in-page i18n / per-plugin JS
**Confidence:** HIGH (every claim below is from a file read this session; no web sources used)

---

## User Constraints (from CONTEXT.md — LOCKED, not revisited)

1. Scope = all 43 plugins; the 22 bare ones get English copy authored here.
2. Explicit dropdown, English default, **no** locale sniffing.
3. Per-plugin JS i18n table + per-plugin copy of the runtime. No shared module, `modules/registry.yaml` untouched.
4. English + French only; French machine-drafted and marked unreviewed.
5. Selector lives in a new gear/settings popover; existing tooltips-on/off control moves into it.
6. Language persists per instance on the APVTS state tree as a non-parameter entry.

Additionally binding from `CLAUDE.md`: path-scoped commits only (`git commit -- plugins/<Name>`), re-check `git branch --show-current` + `git status --short` immediately before every commit, trunk-based on `main`, full AU-cache-clear sequence after every build.

---

## Summary

There are **two** tooltip renderers in this repo, not three. O-MultiBandCompressor's `GLOBAL_TOOLTIPS`/`BAND_TOOLTIPS` arrays are not a third renderer — `applyTooltip()` *writes* `data-tip` / `data-tip-title` attributes onto elements resolved by selector, and the identical measure-then-pin renderer then reads them back. MBC is therefore already the key-indirection design this task wants, and it is the model to generalise. The genuine second renderer is the `data-tooltip` family (7 plugins), which is a simpler, single-string, **unmeasured** positioner with hard-coded pixel guesses.

The C++→JS bridge question has a clean answer already proven in-tree: a `getX`/`setX` native-function **pull at page init** (never a push from the constructor or a timer). The `withKeepPageLoadedWhenBrowserIsHidden` hidden-completion hazard does **not** apply here — the gating flag is the `WebBrowserComponent`'s own `visibleFlag`, and both plugins checked `addAndMakeVisible` the view once and never hide it. Revision-counter polling is also **not** needed: the language is not preset state and no preset path can change it.

The i18n table must be a plain `<script>` include, not a `fetch()`ed JSON. Nothing in this repo fetches anything at runtime; every resource is compiled into BinaryData and served by a resource provider that receives bare paths. A `<script>` include is the proven-on-both-platforms path.

The single largest engineering risk is **not** translation — it is that six existing static gates assert the current markup shape, and one of them (`O-ReverseDelay/tests/ui_frontend_check.js:759-761`) explicitly asserts that a tooltips-enable native function *does not exist*.

**Primary recommendation:** Converge everything on the existing `data-tip` / `data-tip-title` **attribute** contract (unchanged renderer, unchanged gates), and drive those attributes from a per-plugin `applyI18n()` pass modelled on MBC's `applyTooltip()`. Author copy as `{key: {en:{t,b}, fr:{t,b}}}` in a separate `js/i18n.js`, included with a `<script>` tag. Do **not** introduce a `data-tip-key` attribute the renderer has to understand.

---

## A. Unifying the three conventions

### A1. The `data-tip` family — 13 plugins, 430 tips — is the target contract

`plugins/O-ReverseDelay/Source/ui/public/js/app.js:913-1028` is the reference runtime. The only two lines that touch copy are:

```js
// app.js:941-942
const title = target.getAttribute("data-tip-title");
const body  = target.getAttribute("data-tip");
```

Everything else is geometry: content built with `createElement` + `textContent` (`:945-957`, so localized strings stay inert as required), width released → measured → **pinned** (`:968-973`), vertical flip (`:979-986`), horizontal clamp (`:988-989`), arrow-x custom property (`:998`). `.tooltip` is `position: fixed`.

**Migration shape:** none to the renderer. Move the copy out of the HTML attributes into `I18N`, keyed on the anchor's `id` (every anchor already carries one — the static gate at `ui_frontend_check.js:667-270` enumerates them by id and a separate assertion requires it). Then at init, before `initTooltips()`, run a loop that re-writes the two attributes from the table. Language change re-runs the same loop.

### A2. The MBC "JS table" form is **already** this design

`plugins/O-MultiBandCompressor/Source/ui/public/js/app.js:1379-1391`:

```js
function applyTooltip(selector, title, body, wrapper) {
    const element = document.querySelector(selector);
    if (!element) { console.warn(`Tooltip target not found: ${selector}`); return; }
    const target = wrapper ? (element.closest(wrapper) || element) : element;
    target.setAttribute('data-tip-title', title);
    target.setAttribute('data-tip', body);
}
```

`GLOBAL_TOOLTIPS` (`:1173-1213`) is `[selector, title, body, wrapper?]`; `BAND_TOOLTIPS` (`:1219-1256`) is `[controlSuffix, wrapper, title, body]` expanded four times by `applyBandTooltips()` (`:1364-1376`) against `BAND_LABELS = { low, lomid, himid, high }` (`:1215`). The renderer below it (`:1397-1490`) is byte-for-byte the same measure-then-pin logic as O-ReverseDelay's — O-ReverseDelay's header even says so (`app.js:913` "lifted from O-MultiBandCompressor v1.4.1").

**Answer to the posed question:** a `data-tip-key` attribute is *not* needed for MBC and would be a regression. MBC's tips attach to elements the HTML does not mark up (`.control-group` wrappers, `.spectrum-container`, `#range-low`), and the selector+wrapper tuple is exactly the machinery that resolves that. Keep the selector as the key's *binding site*; the key becomes the third element of the tuple.

**Migration shape for MBC:** replace the literal title/body strings in the two arrays with a key, and have `applyTooltip` look the key up:

```js
// tuple becomes [selector, key, wrapper?]  /  [controlSuffix, wrapper, key]
function applyTooltip(selector, key, wrapper) {
    const element = document.querySelector(selector);
    if (!element) { console.warn(`Tooltip target not found: ${selector}`); return; }
    const target = wrapper ? (element.closest(wrapper) || element) : element;
    const s = tr(key);                        // { t, b } for the current language
    target.setAttribute('data-tip-title', s.t);
    target.setAttribute('data-tip', s.b);
}
```

Band tips are *composed* (`applyBandTooltips` at `:1367-1368` does `` `${bandName} — ${title}` ``). That composition must survive localisation, so the band display names (`Low`, `Low-Mid`, `High-Mid`, `High`) become i18n keys too, and the joining string `" — "` stays a literal.

### A3. The `data-tooltip` family — 7 plugins, 257 tips — is a genuinely different renderer

Read both. They are not the same.

| | `data-tip` family | `data-tooltip` family |
|---|---|---|
| copy shape | two attrs (title + body) | one attr, single string; convention is `"Label: sentence."` |
| `.tooltip` position | `fixed` (`ReverseDelay ui_frontend_check.js:700-701`) | `absolute` inside `.plugin-frame` (`O-Polystutter/Source/ui/public/index.html:845-861`) |
| width | released → measured → **pinned** | never measured; `const tooltipWidth = 220` hard-coded (`Polystutter index.html:1728`) |
| height | measured (`getBoundingClientRect().height`) | `const tooltipHeight = 60; // Approximate max height` (`index.html:1718`) |
| bounds | `window.innerWidth` / `innerHeight` | literals `660` and `1000` (`:1719, :1729-1733`) |
| arrow | `--arrow-x` recomputed after clamp | fixed `left:50%` CSS pseudo (`:868-878`) |
| dwell | `TOOLTIP_DELAY_MS` timer | none — immediate on `mouseover`, 100 ms hide delay |
| gating | `tipAllowed()` + `data-tip-always` | `if (!tooltipsEnabled) return` |

O-SpectralShaper (`Resources/ui/js/app.js:586-660`) is a third variant *within* the `data-tooltip` family — it does measure-then-pin (`:634-640`) but positions against `#app` rather than the viewport. O-FreqPulse (`Resources/ui/js/app.js:335-531`) is a fourth: it **synthesises** `data-tooltip` at runtime with interpolated band names, e.g. `` label.setAttribute('data-tooltip', `${band.name} band: …`) `` (`:335`). Those interpolated strings need parameterised i18n entries, not flat ones.

**Recommendation:** for the 7 `data-tooltip` plugins, **port them onto the `data-tip` renderer** rather than localising the weaker one. The port is worth doing on its own merits — the hard-coded `60`/`220`/`660`/`1000` guesses are already wrong before French makes strings longer. The existing single-string copy splits cleanly at the first `": "` because it is already written `"Label: body."` (see `Polystutter index.html:1022-1111`, `Lyrica Resources/ui/index.html`), so the English migration is mechanical and lossless.

**Verdict:** one convention, two attributes, one renderer, table-driven — `data-tip` / `data-tip-title` written by an `applyI18n()` pass. `[VERIFIED: files cited above, read this session]`

---

## B. Getting the language code from C++ into JS

### B1. The bridge shape already in use (10 plugins)

C++ side, `plugins/O-Octagon/Source/PluginEditor.cpp:1278-1300`:

```cpp
options = options.withNativeFunction ("setTooltipsEnabled",
    [this] (auto& args, auto complete)
    {
        if (args.size() > 0)
            processorRef.tooltipsEnabled.store ((bool) args[0], std::memory_order_release);
        complete (juce::var (processorRef.tooltipsEnabled.load (std::memory_order_acquire)));
    });

// PULLED by the page at init, never pushed — a push from the constructor or a poll tick
// fires before the page module has evaluated, so the preference would silently never arrive
options = options.withNativeFunction ("getTooltipsEnabled",
    [this] (auto&, auto complete)
    { complete (juce::var (processorRef.tooltipsEnabled.load (std::memory_order_acquire))); });
```

JS side, `plugins/O-MultiBandCompressor/Source/ui/public/js/app.js:1315-1332`:

```js
let getTooltipsEnabledNative = null;
try {
    getTooltipsEnabledNative = Juce.getNativeFunction('getTooltipsEnabled');
    setTooltipsEnabledNative = Juce.getNativeFunction('setTooltipsEnabled');
} catch (e) {
    console.warn('Tooltip preference not available, using session-only state:', e);
}
setTooltipsEnabled(tooltipsEnabled, false);   // paint the default FIRST, never blank
if (getTooltipsEnabledNative) {
    getTooltipsEnabledNative()
        .then((stored) => setTooltipsEnabled(stored !== false, false))
        .catch((e) => console.warn('Could not read tooltip preference:', e));
}
```

Three load-bearing details in that JS: the `try/catch` around `getNativeFunction` (the same page is opened in a plain browser by the test gates, where `Juce` has no such function), painting the default synchronously before the promise resolves, and the `.catch`.

O-Contrabass (`PluginEditor.cpp:155-171`), O-FreqPulse (`:122-136`), O-SpectralShaper (`:84-92`), O-Bitrot (`:145-170`), O-Orbit (`:262-274`), O-Tapestop (`:225-248`), O-MultiBandCompressor (`:142-155`), O-Lyrica (`:780-790`) are the same shape modulo brace style. **There is no relay involvement** — these are plain `withNativeFunction` pairs, and the language pair should be identical.

### B2. The hidden-completion trap — verified NOT applicable

The trap is real but narrower than the phrasing in the task suggests. `juce_WebBrowserComponent.cpp:336-344` gates `emitCompletionEvent` on `owner.isVisible()`, which is `Component::flags.visibleFlag` — that component's **own** flag, not the recursive `isShowing()`. Hiding the editor, minimising the plugin window, collapsing the inspector, or switching tracks all leave a child web view's `visibleFlag` **true**. The drop fires only if something calls `setVisible(false)` on the `WebBrowserComponent` itself, or it is never `addAndMakeVisible`'d.

Verified for the two plugins named in the task:

```
plugins/O-Octagon/Source/PluginEditor.cpp:1406:      addAndMakeVisible (*webView);
plugins/O-ReverseDelay/Source/PluginEditor.cpp:524:  addAndMakeVisible (*webView);
```

No `setVisible(false)` on `webView` in either file. **So a one-shot `getUiLanguage()` pull at page init is safe here**, and the existing tooltips-enabled path needs no guard because it never faced the hazard. `[VERIFIED: grep of both editors, this session]`

The plan must still run `grep -rn 'setVisible' plugins/<Name>/Source/` per plugin before adding the pull — the answer is per-plugin, not global. The hazard that genuinely bites is a `poll().then(poll)` recursion, which the language path must **not** use.

### B3. Revision-counter polling — NOT needed for language

The revision-counter pattern exists in one place: O-SpectralShaper's 32-band curves. `PluginProcessor.cpp:167` bumps `curvesRevision` **only** inside the `customLoad` callback; `PluginEditor.cpp:525-533` polls it from the existing timer gated on `hasNavigated`:

```cpp
// (curvesRevision is bumped only by the customLoad state callback, never …)
if (const auto revision = processorRef.getCurvesRevision();
    hasNavigated && revision != lastSentCurvesRevision)
{ lastSentCurvesRevision = revision; /* re-send */ }
```

That machinery exists because curve data **is** preset content and a preset load changes it behind the UI's back. The language is not preset content and no path can change it:

- JSON preset load (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:353-373`) walks `preset["parameters"]` and calls `customLoad` — it **never** touches the state tree's own properties.
- Session restore (`:604-613`) does `parameters.replaceState(...)`, but that is the same path that *carries* the language in, so it restores it rather than clobbering it.

**Recommendation:** no revision counter, no polling. Pull once at page init; the UI is the only writer thereafter. `[VERIFIED: OuariconPresetManager.h:295-373, 579-631]`

### B4. Persistence — the XML round-trip type loss, and two valid idioms

Both idioms exist in-tree and both are correct. Mirror whichever the plugin already uses for `tooltipsEnabled`.

**Idiom 1 — ValueTree property + `isVoid()` guard** (O-Bitrot, O-Orbit, O-Tapestop). `plugins/O-Bitrot/Source/PluginProcessor.cpp:1751-1795`:

```cpp
// getStateInformation
auto state = apvts.copyState();
state.setProperty("tooltipsEnabled", tooltipsEnabled.load(std::memory_order_acquire), nullptr);
if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);

// setStateInformation, after apvts.replaceState(...)
// isVoid() is the ONLY correct test … NamedValueSet::setFromXmlAttributes rebuilds every
// property as `var (value)` over the attribute STRING, so what comes back is a var holding
// "1" or "0" and a type test on bool or int is false for every saved session
const juce::var tips = apvts.state.getProperty("tooltipsEnabled");
if (! tips.isVoid())
    tooltipsEnabled.store((bool) tips, std::memory_order_release);
```

**Idiom 2 — XML root attribute** (O-Octagon). `PluginProcessor.cpp:867-882` / `:887-901`:

```cpp
// v1.2.0 — hover-help preference rides the session as a root XML attribute, NOT a
// ValueTree property: the ValueTree XML round-trip rebuilds properties as strings, so an
// isBool() guard on restore would never fire (critical_valuetree_xml_roundtrip_loses_type).
xml->setAttribute ("tooltipsEnabled", tooltipsEnabled.load (std::memory_order_acquire));
…
if (xml->hasAttribute ("tooltipsEnabled"))
    tooltipsEnabled.store (xml->getBoolAttribute ("tooltipsEnabled"), std::memory_order_release);
```

**For a language code the type loss is a non-issue** — the value is *already* a string and the round-trip preserves it exactly. Use `.toString()`, never a type predicate. Recommended concrete shape (idiom 1, since CONTEXT says "on the APVTS state tree"):

```cpp
// PluginProcessor.h
std::atomic<int> uiLanguage { 0 };          // 0 = en, 1 = fr — atomics can't hold juce::String
static juce::String languageCode (int i) { return i == 1 ? "fr" : "en"; }
static int languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

// getStateInformation
state.setProperty ("uiLanguage", languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);

// setStateInformation, after replaceState
const juce::var lang = apvts.state.getProperty ("uiLanguage");
if (! lang.isVoid())
    uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);
```

`std::atomic<juce::String>` is not viable (non-trivially-copyable). An `int` index behind a two-function codec keeps the audio-thread-safe atomic while the persisted form stays a string. The property name `uiLanguage` is chosen not to collide with any parameter ID.

Editor side:

```cpp
options = options.withNativeFunction ("setUiLanguage",
    [this] (auto& args, auto complete)
    {
        if (args.size() > 0)
            processorRef.uiLanguage.store (Processor::languageIndex (args[0].toString()),
                                           std::memory_order_release);
        complete (juce::var (Processor::languageCode (processorRef.uiLanguage.load (std::memory_order_acquire))));
    });

options = options.withNativeFunction ("getUiLanguage",
    [this] (auto&, auto complete)
    { complete (juce::var (Processor::languageCode (processorRef.uiLanguage.load (std::memory_order_acquire)))); });
```

JS side (mirrors MBC exactly — default painted first, `try/catch`, `.catch`):

```js
let setUiLanguageNative = null, getUiLanguageNative = null;
try {
    getUiLanguageNative = Juce.getNativeFunction('getUiLanguage');
    setUiLanguageNative = Juce.getNativeFunction('setUiLanguage');
} catch (e) { console.warn('Language preference not available, session-only:', e); }

applyLanguage('en', false);                 // synchronous paint; never blank, never stale
if (getUiLanguageNative) {
    getUiLanguageNative()
        .then((code) => applyLanguage(code === 'fr' ? 'fr' : 'en', false))
        .catch((e) => console.warn('Could not read language preference:', e));
}
```

---

## C. Where the localized strings physically live

### C1. There is no `fetch()` anywhere in this repo's UIs

```
grep -rn "fetch(" plugins/*/Source/ui/public/js/*.js plugins/*/Resources/ui/js/*.js …  → 0 hits
grep -rn "XMLHttpRequest" …                                                            → 0 hits
```

Every resource is compiled into BinaryData and returned by a `withResourceProvider` callback. `plugins/O-Octagon/Source/PluginEditor.cpp:181-247` is the reference; the file's own header comment states the constraint:

> The WKWebView / WebView2 resource callback receives a **BARE PATH** (`"/"`, `"/js/app.js"`, …) — there is no scheme or host to strip, and stripping one would collapse every lookup to an empty string. Match by direct equality and never hard-code `juce://` vs `https://juce.backend`.

The scheme really does differ per platform (`juce://juce.backend/` on macOS/iOS/Linux, `https://juce.backend/` on Windows/Android). Pages reference resources **relatively** — `O-Octagon/Source/ui/public/index.html:63` is `<link rel="stylesheet" href="css/styles.css" />` and `:726` is `<script type="module" src="js/app.js"></script>`. Relative references are scheme-agnostic by construction; an absolute `fetch()` URL is not, and there is zero in-tree precedent for CORS/scheme behaviour of `fetch()` against a custom scheme in WKWebView.

**Definitive answer: use a `<script>` include, not a `fetch()`ed JSON.** Evidence: (a) zero `fetch()` precedent across 43 plugins; (b) the resource-provider contract is path-equality only, so a JSON path would work but is untested on both platforms; (c) `<script src="js/i18n.js">` is byte-for-byte the mechanism every existing module already uses and is proven on macOS + Windows in shipped plugins. Introducing the first-ever runtime `fetch()` across 43 plugins is a cross-platform risk with no upside — the table is static content, so there is nothing to gain from loading it asynchronously.

Layout: `Source/ui/public/js/i18n.js` (or `Resources/ui/js/i18n.js` for the 10 plugins on that root), an ES module exporting one object, imported by `app.js`. Both roots already carry `js/` subdirectories.

### C2. `juce_add_binary_data` hyphen stripping — affects naming, plan around it

`plugins/O-Octagon/CMakeLists.txt:63-102` states both traps in one block:

> **NAMESPACE is NOT optional.** A second `juce_add_binary_data` target in the same build collides with the default `BinaryData` namespace and the link fails — or worse, resolves to the wrong table.
> **NO FILENAME HERE CONTAINS A HYPHEN.** `juce_add_binary_data` **STRIPS** hyphens rather than converting them to underscores, so `room-plan.js` would have to be reached as the symbol `roomplan_js`.

`PluginEditor.cpp:200-204` repeats it at the serve site.

**Consequences for this task:**
- `i18n.js` → symbol `i18n_js`. Safe (no hyphen).
- `i18n-fr.js` → symbol **`i18nfr_js`**, not `i18n_fr_js`. Avoid. If per-language files are ever wanted, name them `i18nfr.js` / `i18nen.js` on disk so no transform has to be remembered — but a **single** `i18n.js` holding both languages is simpler and avoids the question entirely (only two languages, and the whole point of the dropdown is instant switching with no second load).
- `en.json` / `fr.json` → moot; the recommendation is not to use JSON at all.
- Every new file must be added to **three** places: the `juce_add_binary_data SOURCES` list, a `getResource()` branch, and the HTML/JS that asks for it. O-Octagon's `tests/ui_frontend_check.js` §9 and §21 close that loop by deriving the module list from `Source/ui/public/js/*.js` and asserting set-equality against the CMake SOURCES block — **adding `i18n.js` to O-Octagon without updating CMakeLists will fail §21**, which is the desired behaviour but must be sequenced.

Note O-Lyrica's `CMakeLists.txt:88-96` has **no `NAMESPACE`** — it relies on being the only binary-data target in that plugin. Adding a second there would collide.

---

## D. Tooltip geometry under longer French strings

### D1. What the two clamp gates assert

Both `plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js` (508 lines) and `plugins/O-Bitrot/tests/ui_tooltip_clamp_check.js` (548 lines) drive the **real page** in Playwright/Chromium at the **shipping viewport**, hover every `[data-tip]` anchor, await the `.visible` class, and measure the rendered rect. Per anchor (`O-ReverseDelay:328-345`):

1. **Not shrink-wrapped** — `m.w > NATURAL_MAX_W * 0.5 || m.text < 40`
2. **Both X edges inside** — `left >= MARGIN - 0.5 && right <= SHIP_W - MARGIN + 0.5`
3. **Both Y edges inside** — `top >= -0.5 && bottom <= SHIP_H + 0.5`
4. **Arrow inside the tip** — `0 <= --arrow-x <= width`

Plus three whole-run assertions: viewport really is `SHIP_W x SHIP_H` (`:223-224` in the Bitrot copy), every anchor was actually measured (`:371-375`), and **the clamp actually engaged at least once** (`:378-381` — "a run where it never fires proves nothing about the clamp").

Mirrored constants, cross-checked against source so the file cannot drift (`O-ReverseDelay:104-109, 165-170`):

| | O-ReverseDelay | O-Bitrot |
|---|---|---|
| `SHIP_W` × `SHIP_H` | 940 × 768 | 900 × 740 |
| `TOOLTIP_MARGIN` | 8 (regex-checked against `app.js`) | 8 |
| `NATURAL_MAX_W` | 230 (regex-checked against `styles.css`) | 230 (checked against inline HTML CSS) |

O-Octagon's `.tooltip` uses `max-width: 240px` (`css/styles.css:1302`) — the cap is **per plugin**, so any generalised gate must read it rather than assume 230.

### D2. Are they viewport-sensitive? Are they language-sensitive?

**Viewport-sensitive: yes, by design** — that is the whole reason the file exists separately from the static gate (`O-ReverseDelay:26-34`). But the viewport is not changing here, so no re-anchoring is needed on that axis.

**Language-sensitive: yes, and unevenly.** The distinction that matters:

- **Assertion 2 (horizontal)** is language-*independent for any tip already at `max-width`* — CSS caps the width, so a longer string cannot push `right` further. It **is** language-dependent for tips *below* the cap: a 180 px English tip that becomes 215 px in French moves its own right edge by ~17 px and can newly cross the clamp threshold. Note the clamp is what *fixes* that, so this is a coverage change (which anchors clamp) more than a failure risk.
- **Assertion 3 (vertical)** is fully language-dependent. Longer strings at a fixed `max-width` wrap to **more lines**, so `height` grows; `top = anchor.top - height - MARGIN` moves up and can go under `MARGIN`, triggering the flip at `app.js:982-986` to below the anchor, where `bottom` must stay inside `SHIP_H`. Both directions are live. The gate's own header (`:41-53`) documents exactly this class of failure from three past editor resizes.
- **Assertion 1** uses `m.text < 40` as an escape hatch for genuinely short tips; French copy is *longer*, so this only gets safer.
- **Assertion 4** recomputes `--arrow-x` after clamping, so it follows whatever the clamp did — no separate risk.

### D3. Are the gates run by CI?

**No.** `.github/workflows/ci-tests.yml` has exactly two jobs — `octagon-probes-macos` (`:51`) and `octagon-windows-vst3` (`:147`) — both of which build and run **C++** test targets. There is no `node` step, no Playwright step, and no reference to any `ui_*.js` file in either workflow. `.github/workflows/build-and-release.yml` carries the eight Apple signing secrets and is explicitly forbidden from adding test targets (stated at `ci-tests.yml:6-11`).

The JS gates are **manual**: `node plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js`, exit code = number of failed assertions, and exit **77** = "playwright not resolvable, NOT verified" (`:186-190` — deliberately distinct from 0). Only 17 of 43 plugins have a `tests/` directory at all: O-Bitrot, O-Bowed, O-Contrabass, O-Emulator, O-MicrotonalSampler, O-MultiBandCompressor, O-Octagon, O-ReverseDelay, O-simpleAdditive, O-simpleBeatmaker, O-simpleFM, O-simpleGrain, O-simplePhysicalModelSynth, O-simpleSampler, O-simpleSubtractive, O-SpectralShaper, O-Tapestop. Only **two** have a tooltip clamp gate.

### D4. Recommended minimal re-verification strategy

1. **Parameterise the two existing clamp gates by language**, don't duplicate them. Add an env/argv switch that sets the language before the sweep (`await page.evaluate((l) => window.__setLanguage(l), 'fr')` — exposing `applyLanguage` on `window` in the UI is a two-line change and makes the gate drivable without touching the bridge stub). Run the sweep for `en` then `fr` in one process. Assertion counts double; the mirrored-constant guards are unaffected.
2. **Assert the max-width cap is read, not assumed.** Replace the literal `NATURAL_MAX_W = 230` with a value parsed from the plugin's own CSS, keeping the existing regex cross-check as the drift guard. O-Octagon is 240 and would otherwise be mis-gated if the gate is ever generalised.
3. **No `max-width` change.** Raising the cap makes tips *wider*, which pushes assertion 2 toward failure while barely helping assertion 3 (fewer lines but a wider box near the edges), and there is a standing repo lesson that removing a `max-width` cap also removes the alignment it was silently providing. Let French wrap taller inside the existing cap — the flip logic already handles taller tips, and assertion 3 is what proves it.
4. **Extend the two gates before extending coverage.** Getting `en` + `fr` green on O-ReverseDelay and O-Bitrot proves the geometry claim for the whole `data-tip` family, since the renderer is a hand-copy of one implementation. Adding a clamp gate to the other 41 plugins is not proportionate for this task.
5. **The 7 ported `data-tooltip` plugins get their geometry from the port, not from a gate.** Moving them onto the measured renderer removes the `tooltipHeight = 60` / `tooltipWidth = 220` guesses, which is a strictly larger improvement than any gate could deliver against the current code. Verify by rendering, per `pattern_module_toplevel_init_tdz`'s stub-server recipe.

---

## E. The 22 plugins with no tooltip copy

All 22 are WebView plugins (`WebBrowserComponent` found in `Source/`), so all 22 can take the same treatment. Approximate parameter counts are `grep -c "make_unique<juce::AudioParameter"` on `PluginProcessor.cpp` — a **lower bound** where factory lambdas are used (see E2).

| Plugin | UI root | native `title=` today | params (≥) | APVTS layout at |
|---|---|---|---|---|
| O-AnalogEQ | `Source/ui/public` | 9 | 16 | `PluginProcessor.cpp:33` |
| O-AnalogSaturation | `Source/ui/public` | 0 | 4 | `PluginProcessor.cpp` |
| O-Bass | `Source/ui/public` | 9 | 5 | `PluginProcessor.cpp:35` |
| O-Bassoon | `Resources/ui` | 3 | 10 | `PluginProcessor.cpp` |
| O-Bells | `Resources/ui` | 0 | 65 | `PluginProcessor.cpp` |
| O-Bowed | `Resources/ui` | 3 | 23 | `PluginProcessor.cpp` |
| O-Chorus | `Source/ui/public` | 8 | 8 | `PluginProcessor.cpp:33` |
| O-Comp | `Source/ui/public` | 9 | 7 | `PluginProcessor.cpp` |
| O-Detune | `Source/ui/public` | 9 | 18 | `PluginProcessor.cpp` |
| O-DigiDelay | `Source/ui/public` | 9 | 8 | `PluginProcessor.cpp` |
| O-Emulator | `Source/ui/public` | 4 | **2 (undercount — 6 real)** | `PluginProcessor.cpp:52-74` |
| O-Formant | `Source/ui/public` | 4 | 64 | `PluginProcessor.cpp` |
| O-Freeze | `Source/ui/public` | 0 | 12 | `PluginProcessor.cpp` |
| O-GrainScatter | `Source/ui/public` | 0 | 36 | `PluginProcessor.cpp` |
| O-MicrotonalSampler | **both roots** | 7 | 19 | `PluginProcessor.cpp` |
| O-Prism | `Source/ui/public` | 8 | 81 | `PluginProcessor.cpp:461` |
| O-Reed | `Resources/ui` | 0 | 35 | `PluginProcessor.cpp` |
| O-SimpleReverb | `Source/ui/public` | 8 | 8 | `PluginProcessor.cpp` |
| O-Texture | `Source/ui/public` | 6 | 10 | `PluginProcessor.cpp` |
| O-TextureForge | `Source/ui/public` | 0 | 12 | `PluginProcessor.cpp:62` |
| O-Tremolo | `Source/ui/public` | 9 | 7 | `PluginProcessor.cpp` |
| O-Wind | `Resources/ui` | 3 | 56 | `PluginProcessor.cpp:35` |

Total ≥ **~520 parameters** to author English copy for, then translate. O-Prism (81), O-Bells (65), O-Formant (64) and O-Wind (56) are 51% of the volume in four plugins. **O-MicrotonalSampler has both `Source/ui/public/` and `Resources/ui/`** and needs its actual served root determined from its `CMakeLists.txt` before anything is written; O-Orbit is in the same position (but already has 32 `data-tip` tips).

### E1. Are ranges/units machine-extractable from the APVTS layout?

**Partly — and static source parsing is not reliable enough to build on.**

Clean case (`plugins/O-AnalogEQ/Source/PluginProcessor.cpp:38-43`) — id, display name, min, max, step, skew, default, **and unit** all present:

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "lf_freq", 1 }, "LF Frequency",
    juce::NormalisableRange<float>(30.0f, 500.0f, 0.1f, 0.3f), 100.0f, "Hz"));
```

Failure case 1 — **no unit at all** (`plugins/O-Chorus/Source/PluginProcessor.cpp:37-62`): eight parameters, zero unit strings. `Depth`, `Spread`, `Width`, `Mix`, `Drive` are all bare `0.0f–1.0f`. Copy has to say "0–100%" or "0–1" from *knowledge of the UI's formatter*, not from the layout.

Failure case 2 — **factory lambda** (`plugins/O-Emulator/Source/PluginProcessor.cpp:63-74`): four parameters are produced by a local `percent()` helper, so a regex counts 2 where 6 exist and finds no ranges for four of them.

Failure case 3 — **string-concatenated IDs and names** (`plugins/O-Prism/Source/PluginProcessor.cpp:76-87`): `juce::ParameterID { prefix + "Table", 1 }, label + " Wavetable"`. The literal ID never appears in the source; only the runtime value is real. This is the largest plugin in the set.

**Recommendation: extract at runtime, not from source.** The repo already builds standalone C++ harness binaries against the processor (15 `tests/render-harness/` targets exist, pattern at `plugins/O-Bitrot/tests/render-harness/{CMakeLists.txt,main.cpp}`). A ~40-line variant that constructs the processor and walks `getParameters()` emitting `getParameterID()`, `getName(128)`, `getLabel()`, `getNumSteps()`, `getText(0.f, 64)`, `getText(1.f, 64)`, `getDefaultValue()` produces the authoritative, complete inventory for any plugin including O-Prism and O-Emulator, and *includes the unit* wherever `getLabel()` is set. That is a one-time tool, reusable across all 22, and it is the only method that survives the three failure cases above.

**Verdict: copy authoring is semi-automatable** — the *skeleton* (one entry per parameter, with correct id, display name, range and default) is fully machine-generated from a runtime dump; the *prose* (what the control does and when to reach for it) is hand-written. The prose is the irreducible content work CONTEXT already flags as the largest chunk. Do **not** attempt to generate it from static source parsing.

---

## F. Pitfalls specific to this repo

### F1. Six existing static gates assert the current markup and bridge shape

This is the highest-probability way this task breaks something.

**F1a — `O-ReverseDelay/tests/ui_frontend_check.js:759-761` forbids the very function this task adds:**

```js
// D13 scoped this to hover help only: no toggle, no persisted state, and so
// no 12th native function.
check(!/setTooltipsEnabled/.test(appJs) && !/setTooltipsEnabled/.test(editorCpp),
    'no tooltip-enable native fn (D13: tooltips only, the bridge stays at 11)');
```

Adding the gear popover to O-ReverseDelay (which per CONTEXT is one of the 10 with `setTooltipsEnabled` — **it is not**; this gate proves O-ReverseDelay has *no* tooltips toggle) will fail this assertion. The plan must retire or invert it as a deliberate, recorded decision, not silently delete it. **Correction to CONTEXT's census:** `grep -rn setTooltipsEnabled plugins/*/Source/*.cpp` returns O-Bitrot, O-Contrabass, O-FreqPulse, O-Lyrica, O-MultiBandCompressor, O-Octagon, O-Orbit, O-SpectralShaper, O-Tapestop — **9**, not 10. O-ReverseDelay is listed in CONTEXT but has no such bridge.

**F1b — `O-ReverseDelay/tests/ui_frontend_check.js:661-696` asserts the copy is in the HTML:**

```js
const missingTips = TIP_ANCHORS.filter(id => {
    const m = html.match(new RegExp(`id="${id}"[\\s\\S]{0,400}?>`));
    return !m || !/data-tip=/.test(m[0]) || !/data-tip-title=/.test(m[0]);
});
```

Moving copy out of `index.html` into `i18n.js` will fail this for all 28 anchors. The assertion must become "every anchor id appears as a key in `i18n.js`, in both `en` and `fr`" — which is a *stronger* gate (it catches a missing translation, which the current one cannot). Same rewrite applies to any equivalent in the other 12 `data-tip` plugins.

**F1c — `O-Octagon/tests/ui_frontend_check.js` §9 and §21** derive the JS-module list from `Source/ui/public/js/*.js` and assert set-equality against the `juce_add_binary_data SOURCES` block. Adding `js/i18n.js` to O-Octagon and forgetting either CMakeLists or `getResource()` fails §21 rather than 404-ing at runtime. This is working as designed but must be sequenced: CMake + `getResource()` + file, all in one commit.

**F1d — the clamp gates' mirrored constants** (`TOOLTIP_MARGIN`, `.tooltip max-width`, `setSize(W,H)`) are regex-checked against source. Any CSS change to `.tooltip` breaks the mirror check loudly. Good — but budget for it.

**F1e — `tests/ui-stub/juce-stub.js`** hand-implements `getNativeFunction(name)` per name (`O-ReverseDelay/tests/ui-stub/juce-stub.js:289-320`). Adding `getUiLanguage`/`setUiLanguage` without adding stub branches means the gate's page gets an unresolved promise. The MBC-style `try/catch` + synchronous default paint (§B4) makes this non-fatal, but the stub should still learn the two names so the gate can *exercise* language switching.

### F2. TDZ ordering — the exact failure this task can reproduce

`plugins/O-MultiBandCompressor/Source/ui/public/js/app.js` (foot of file):

```js
// Initialize tooltips once their state above has been evaluated. This must stay at
// the foot of the file: initializeUI() runs at module top level, where these
// `let`/`const` bindings are still in the temporal dead zone, and the ReferenceError
// would escape module evaluation and take initializeCrossoverDrag() down with it.
function initializeDeferredUI() {
    try { initializeTooltips(); } catch (e) { console.error('Tooltip initialization failed:', e); }
    initializePresets().catch((e) => console.error('Preset initialization failed:', e));
}
```

MBC v1.4.0 shipped with `initializeTooltips()` inside the eager `initializeUI()`; the TDZ throw killed `initializeCrossoverDrag()` — untouched, working code — and the C++ build, `auval`, and a static selector check all passed. **The i18n table and the language state are exactly the same kind of late-declared `const`/`let`.** They must be declared **above** everything that reads them, and `applyI18n()` must be called from the foot-of-file deferred init, inside its own `try/catch`, so a translation-table typo cannot take the knobs down.

O-ReverseDelay's `app.js` uses the opposite-but-equivalent idiom: a single `init()` call at the very bottom (`:1063`), with `initTooltips()` at `:1054`. Both are fine; the eager-top-level pattern is not.

### F3. A JS state updater that writes `textContent` erases HTML-authored labels

Documented in-repo. The relevant instance here: the gear popover's language `<option>` labels, and any tooltips-toggle button whose caption is written by a shared updater. Fix shape (from the memory): let the element own its label via `data-label` and have the updater preserve it — `el.textContent = el.dataset.label ?? (v ? 'On' : 'Off')`. Add `aria-pressed` when a label replaces On/Off text so state stays non-visually exposed. And: **re-check tooltip geometry after any control resizes** — widening a button moved the right-most one toward the edge and was the exact trigger for the shrink-to-fit bug.

### F4. Tooltip copy must stay `textContent`, never `innerHTML`

Already true on both renderers — `O-ReverseDelay/app.js:945-957` builds title/body with `createElement` + `textContent`; `O-Polystutter/index.html:1707` and `O-SpectralShaper/app.js:626` use `textContent`. `O-Octagon/tests/ui_frontend_check.js:337` asserts "t / b are createElement'd tooltip children, not authored nodes". Machine-drafted French **must not** be allowed to introduce an `innerHTML` path for entities or `<br>`; use `\n` + CSS `white-space: pre-line` if a line break is ever needed.

### F5. Preset interaction — safe, but for a specific reason

Worth stating so nobody "fixes" it later: `OuariconPresetManager::loadPreset` (`cpp/OuariconPresetManager.h:353-373`) iterates `preset["parameters"]` and calls `customLoad(preset["customState"])`. It **never** touches state-tree properties, so `uiLanguage` cannot ride into a preset file and a preset load cannot change the user's language. `getStateAsXml`/`setStateFromXml` (`:579-631`) are the *session* path and do `parameters.replaceState(...)` — which is where `uiLanguage` is carried and restored. If a plugin's `getStateInformation` delegates to `presetManager.getStateAsXml()`, the `state.setProperty("uiLanguage", …)` call must happen **before** that delegation, or use O-Octagon's root-attribute idiom on the returned XML.

### F6. Hand-copy drift is accepted, but make it detectable

CONTEXT accepts 43 independent copies. The one cheap mitigation that matches repo convention: a single repo-level script that reads every plugin's `i18n.js` and asserts (a) `en` and `fr` key sets are identical, (b) every `data-tip` anchor id / MBC selector key has an entry, (c) no entry is an untranslated `fr` string identical to its `en` (which is how "machine-drafted, marked unreviewed" degrades into "silently English"). Store the reviewed/unreviewed flag *in the table* (`fr: { t, b, reviewed: false }`) so it is machine-checkable rather than a comment.

### F7. Build/install discipline

Per `CLAUDE.md`: every VST3/AU build must be followed by `killall -9 AudioComponentRegistrar`, removal of `~/Library/Caches/AudioUnitCache/` and `com.apple.audiounits.cache`, and removal of **both** the `-dev` and unsuffixed bundle variants before installing — the alternate variant pins Logic's registry slot. Prefer `./scripts/build-and-install.sh [PluginName]` (its Phase 4 does the dual sweep). Note `build-and-install.sh` does **not** rebuild the Standalone `.app`, so a UI change verified only in Standalone may be looking at a stale binary.

---

## Decisions this forces on the plan

1. **One convention: `data-tip` + `data-tip-title` attributes, written by JS from a key table.** No new `data-tip-key` attribute; no renderer change for the 13-plugin `data-tip` family; MBC's `applyTooltip()` (`app.js:1379-1391`) is the generalisation template.
2. **The 7 `data-tooltip` plugins are PORTED to the `data-tip` renderer, not localised in place.** Their current positioner uses hard-coded `60`/`220`/`660`/`1000` literals (`O-Polystutter/index.html:1717-1733`) and is already wrong before French. Split existing copy on the first `": "` to recover title/body.
3. **O-FreqPulse needs parameterised i18n entries**, not flat ones — it synthesises `data-tooltip` with interpolated band names at `Resources/ui/js/app.js:335-531`. Same for MBC's band-name composition (`app.js:1367-1368`).
4. **Bridge = a `getUiLanguage`/`setUiLanguage` `withNativeFunction` pair, PULLED once at page init.** No push from the editor constructor, no timer push, **no `poll().then(poll)` recursion**, no revision counter. JS must `try/catch` `getNativeFunction`, paint the English default synchronously first, and `.catch` the promise.
5. **Persist as a string.** `state.setProperty("uiLanguage", "en"|"fr", nullptr)` guarded on restore by `isVoid()` and read with `.toString()` — never `isBool()`/`isString()`/`isInt()`. Hold it in C++ as `std::atomic<int>` behind a two-function codec (`std::atomic<juce::String>` does not compile). Mirror O-Octagon's XML-root-attribute idiom instead where that plugin already uses it.
6. **Per-plugin, before adding the pull: `grep -rn 'setVisible' plugins/<Name>/Source/`.** The hidden-completion drop fires only if the `WebBrowserComponent` itself is hidden. Verified absent in O-Octagon and O-ReverseDelay; unverified elsewhere.
7. **`js/i18n.js` as a `<script>`/ES-module include. No runtime `fetch()`, no JSON file.** Zero `fetch()` precedent across 43 plugins; the resource provider takes bare paths and the scheme differs per platform.
8. **No hyphen in any new filename.** `juce_add_binary_data` strips hyphens (`i18n-fr.js` → `i18nfr_js`). One combined `i18n.js` avoids the question. Every new file goes into CMake SOURCES + `getResource()` + the page in **one commit**, or O-Octagon §21 fails.
9. **Six gates must be rewritten as part of the work, not after it:** `O-ReverseDelay/tests/ui_frontend_check.js:759-761` (forbids the new native fn) and `:661-696` (asserts copy is in the HTML) both fail by construction. Rewrite `:661-696` into the stronger form — "every anchor id has an `en` and an `fr` entry."
10. **CONTEXT's `setTooltipsEnabled` census is off by one.** Nine plugins have it, not ten: O-Bitrot, O-Contrabass, O-FreqPulse, O-Lyrica, O-MultiBandCompressor, O-Octagon, O-Orbit, O-SpectralShaper, O-Tapestop. **O-ReverseDelay does not** — it gets a fresh toggle, and the gate that forbids one has to be retired first.
11. **Clamp gates: parameterise by language, do not duplicate; run `en` then `fr` in one process.** They are manual (`node …`, exit 77 = "not verified"), not CI — `.github/workflows/ci-tests.yml` runs only two C++ Octagon jobs. Assertion 3 (vertical) is the language-sensitive one; assertion 2 is capped by `max-width`.
12. **Do not change `.tooltip { max-width }`.** Let French wrap taller inside the existing cap. Note the cap differs per plugin (230 in O-ReverseDelay/O-Bitrot, 240 in O-Octagon, 220 in O-Polystutter) — read it, never assume it.
13. **Build the runtime parameter-dump tool first** (a ~40-line variant of `tests/render-harness/main.cpp` walking `getParameters()`). Static source parsing of `createParameterLayout()` fails on factory lambdas (O-Emulator) and concatenated IDs (O-Prism, 81 params). The dump gives the machine-generated skeleton; the prose is hand-written.
14. **Determine O-MicrotonalSampler's and O-Orbit's actual served UI root** from their `CMakeLists.txt` before touching either — both have `Source/ui/public/` *and* `Resources/ui/`.
15. **`applyI18n()` is called from the foot-of-file deferred init, inside `try/catch`.** Table and language `const`/`let` declared above every reader. A TDZ throw out of module evaluation kills unrelated working controls silently (MBC v1.4.0 precedent).
16. **Localized strings stay `textContent`.** No `innerHTML` path for French entities or `<br>`.
17. **Store the unreviewed flag in the table** (`fr: { t, b, reviewed: false }`) and add a repo-level checker asserting `en`/`fr` key-set equality, full anchor coverage, and no `fr === en` passthroughs.
18. **Path-scoped commits per plugin** (`git commit -- plugins/<Name>`), with `git branch --show-current` + `git status --short` re-checked immediately before each. Full AU-cache-clear + dual-variant sweep after every build.

---

## Sources

All findings are from files read in this session. No web or package-registry lookups were performed (no external dependencies are introduced by this task, so the Package Legitimacy Audit and Environment Availability sections do not apply — Playwright is the only tool dependency and the gates already resolve it themselves, exiting 77 rather than installing).

**Primary (HIGH — read this session):**
- `plugins/O-ReverseDelay/Source/ui/public/js/app.js` (tooltip runtime `:913-1028`, init `:1030-1063`)
- `plugins/O-MultiBandCompressor/Source/ui/public/js/app.js` (`:1165-1490`, foot-of-file init)
- `plugins/O-Polystutter/Source/ui/public/index.html` (`:808-895` CSS, `:1670-1757` runtime)
- `plugins/O-SpectralShaper/Resources/ui/js/app.js` (`:586-660`); `plugins/O-FreqPulse/Resources/ui/js/app.js` (`:335-531`, `:1068-1100`)
- `plugins/O-Octagon/Source/PluginEditor.cpp` (`:181-247`, `:268-271`, `:1278-1310`, `:1406-1407`); `PluginProcessor.cpp` (`:867-901`)
- `plugins/O-Bitrot/Source/PluginProcessor.cpp` (`:1751-1795`); O-Orbit (`:749-751`, `:817-819`); O-Tapestop (`:936-987`)
- `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` (`:295-373`, `:579-631`)
- `plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js`; `plugins/O-Bitrot/tests/ui_tooltip_clamp_check.js`; `plugins/O-ReverseDelay/tests/ui_frontend_check.js`; `plugins/O-ReverseDelay/tests/ui-stub/juce-stub.js`
- `plugins/O-Octagon/CMakeLists.txt` (`:63-102`); `plugins/O-Lyrica/CMakeLists.txt` (`:88-111`)
- `.github/workflows/ci-tests.yml`
- `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:33-60`; `O-Chorus:33-63`; `O-Emulator:52-74`; `O-Prism:461`, `:76-87`
- `CLAUDE.md`, `.planning/STATE.md`, `.planning/PROJECT.md`, task `CONTEXT.md`

**Secondary (project memory, corroborated against code this session):**
- `critical_webview_resource_provider_and_schemes` — corroborated by `O-Octagon/Source/PluginEditor.cpp:169-180`
- `critical_webview_completion_gated_on_isvisible` (corrected 2026-08-25) — corroborated by the `addAndMakeVisible` greps
- `critical_valuetree_xml_roundtrip_loses_type` — corroborated by `O-Bitrot/Source/PluginProcessor.cpp:1774-1793`
- `pattern_webview_one_shot_state_push_stale_on_preset_load` — corroborated by `O-SpectralShaper/Source/PluginEditor.cpp:525-533`
- `critical_binary_data_strips_hyphens`, `critical_dual_binary_data_namespace_collision` — corroborated by `O-Octagon/CMakeLists.txt:63-72`
- `pattern_module_toplevel_init_tdz`, `pattern_js_state_updater_overwrites_html_labels`, `pattern_fixed_tooltip_shrink_to_fit_edge`, `pattern_tooltip_clamp_gate_viewport_sensitive`

**Assumptions Log**

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | French copy averages ~15–20% longer than English | D | Carried from the task brief, not measured here. Affects sizing of the geometry re-verification, not its design — assertion 3 is re-run either way. |
| A2 | Splitting existing `data-tooltip` strings on the first `": "` recovers title/body losslessly | A3, Decision 2 | Sampled across O-Polystutter and O-Lyrica and consistent; not exhaustively checked across all 257 tips. A handful may need hand-splitting. |
| A3 | Parameter counts in §E are lower bounds from a regex | E | Undercounts factory-lambda plugins (proven for O-Emulator). The runtime dump tool in Decision 13 is what makes the real number knowable — that is why it is Decision 13. |
