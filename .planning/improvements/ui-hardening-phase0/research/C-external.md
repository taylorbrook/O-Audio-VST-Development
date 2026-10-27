# C — External research: a shared, versioned UI kit for 46 JUCE 8 WebView plugins

Date: 2026-10-26 · Scope: how others build web UIs for plugins, platform facts, and fit with this repo. The repo was only read, never changed. · Confidence labels: **[verified]** = read from a source or from code; **[inferred]** = reasoned from verified facts, not tested on hardware.

---

## 0. The recommendation on one screen

1. **Kit shape.** Add a first-party `ui-kit` module at `modules/ui/kit/`. It holds **light-DOM autonomous custom elements** written as plain ES modules and CSS. No framework, no bundler. The code is typed with JSDoc and checked by `tsc --checkJs`. This has direct precedent in Cmajor's own control library (`cmaj-parameter-controls.js`), which uses light DOM, CSS strings, JSDoc and `checkJs: true, strict: true` [verified]. Output's ADC24 talk also teaches plain TypeScript with DOM plus Canvas and no framework [verified].
2. **One seam to JUCE.** Kit components call `bridge.js` and never touch `window.__JUCE__` or `Juce.getSliderState` directly. There are three backends:
   - JUCE 8 (today).
   - JUCE 9: JUCE 9.0.1 moved the interop package and republished it as `@juce-framework/webview`.
   - Stub, for the gallery and tests.

   This is "branch by abstraction". When JUCE upgrades, or a test fake is swapped in, only one file changes.
3. **One way to deliver files.** At build time, stage each plugin's UI root together with the kit. Embed the result as **one deterministic zip** in BinaryData and serve it through **one shared ZipResourceProvider** with a MIME table. This is the pattern JUCE's own WebViewPluginDemo uses [verified]. It removes three problems at once:
   - the hand-written `url == "..."` branches in every `PluginEditor::getResource()`;
   - the `juce_add_binary_data` traps (hyphens stripped from names, namespace collisions);
   - the symbol reverse-engineering in `serve-ui.js`.

   `registry.yaml` already lists a `resource-provider` 1.0.0 module, but `modules/core/resource-provider/` does not exist [verified]. This would fill that slot.
4. **The C++ half of the kit.** One shared `makeWebViewOptions()` sets up everything in one place:
   - backend, user-data-folder policy, error page off, background colour;
   - `withInitialisationData("ouaricon", manifest)` and `withUserScript(hardening)`;
   - the resource provider and the common native functions.

   The manifest is generated from the APVTS at runtime: IDs, defaults, units, choices, and sampled tables for lambda ranges. JS then stops hard-coding defaults. JUCE's SliderState carries no default value and only understands start/end/skew/interval [verified, `index.js:135-273`].
5. **Versioning.** Keep one version of the kit, at HEAD. Each plugin carries a small lockfile, `ui-kit.lock`, recording the kit version and tree hash last verified for that plugin. `/publish` refuses to ship when the lock is stale.

   A breaking change ships side by side as `kit/v2/`, a transitional copy that is deleted once no plugin uses it. A kit change therefore cannot reach a plugin's release until that plugin's UI suite has passed against it.
6. **Look.**
   - Tokens in three tiers as CSS custom properties.
   - Cascade layers `@layer legacy, kit, theme, plugin`, so legacy CSS can be wrapped and live alongside the kit during migration.
   - SVG knobs whose arc geometry is computed in JS, not drawn with dash tricks.
   - Canvas only for meters and spectra.
   - A fixed design stage scaled with `transform`.
   - Embedded fonts, optionally including a subset CJK serif.
7. **Tests.**
   - A **kit gallery page** showing every component × state × locale × theme × scale, captured with Playwright `toHaveScreenshot` in Chromium (close to WebView2) and WebKit (close to WKWebView).
   - Smoke screenshots per plugin.
   - axe, with violations stored as fingerprints, plus a custom contrast-over-texture check.
   - Behaviour tests that assert the gesture sequences recorded by a stub fed from `param-dump`.
   - Playwright and browser versions pinned through a lockfile.
8. **Migration, using the strangler pattern.**
   - **Phase 0:** characterization screenshots, plus build plumbing whose output is pixel-identical.
   - **Phase 1:** plugins adopt kit *behaviour* one component at a time while keeping their *legacy look* (pixel parity). The existing `scripts/ui-canon.js` report drives the burn-down and becomes a ratchet.
   - **Phase 2:** a deliberate visual refresh, plugin by plugin, through theme tokens.
   - **Phase 3:** scaling and resizing.
9. **Engine floor.** The UI floor is *already* Safari 15, because every embedded copy of JUCE's `index.js` uses a private class method [verified]. Declare **floor = Safari 15.6** and lint against it, or raise the floor to macOS 11 (Safari 16.6) to unlock the features that arrived in Safari 16.4. See §2.5.

---

## 1. How others ship web UIs in audio plugins

| Stack | JS ↔ native binding | State sync | How assets are served |
|---|---|---|---|
| **JUCE 8 WebBrowserComponent** | `WebSliderRelay` / `WebToggleButtonRelay` / `WebComboBoxRelay` plus the matching ParameterAttachments. In JS: `getSliderState/getToggleState/getComboBoxState`. Native functions return Promises. Events go both ways. `ControlParameterIndexUpdater` reports the parameter under the mouse. | The APVTS is authoritative. Each relay pushes value and properties, and JS sends `requestInitialUpdate`. Formatting happens in JS ("calculating and displaying values and labels happens in JS"). | Resource provider on a custom scheme: `juce://juce.backend/` on macOS, `https://juce.backend/` on Windows. **The demo ships its built frontend as a versioned zip (`webviewplugin-gui_1.0.0.zip`)** read by `ZipFile`, with a MIME map. A dev server is available for hot reload. |
| **JUCE 9.0.1** | Same API, translated to TypeScript and published on npm as `@juce-framework/webview` (1.0.0, 2026-07-23). The C++ side asserts a matching frontend version and JS logs a warning on mismatch. | — | The transpiled JS now lives at `modules/juce_gui_extra/native/typescript/webview-interop/dist/index.js`, so the old path breaks (BREAKING_CHANGES 9.0.1). |
| **choc::ui::WebView** (Jules Storer / Tracktion) | `bind(name, fn)` makes the function available to JS as a Promise. Also `addInitScript` and `evaluateJavascript`. | App-defined. | A `fetchResource(path)` callback serves files from memory over a custom scheme. Options include `acceptsFirstMouseClick`, `transparentBackground` (avoids a white flash), and Safari clipboard-shortcut interception. |
| **iPlug2 WebView** | JS calls `SPVFUI` (set value), `BPCFUI`/`EPCFUI` (begin/end gesture), `SAMFUI`, `SMMFUI`. Native calls `SPVFD`, `SCVFD`, `SCMFD`, `SAMFD`. Values are normalized 0–1. | Native code pushes values to the delegate. | `resources/web/` inside the bundle. Debug builds load from the source tree; a dev-server URL is optional. **The example uses a `knob-control` custom element (shadow DOM).** |
| **Cmajor patch GUIs** | `PatchConnection`: `sendEventOrValue`, `sendParameterGestureStart/End`, `addParameterListener`, `requestParameterValue`, plus a stored-state API for UI state. | The patch is authoritative. | `view.src` is a JS module whose default export returns an `HTMLElement` ("We expect a patch to provide a web component"). The `cmaj_api` controls are **light-DOM** custom elements (`cmaj-knob-control`, `-switch-`, `-options-`) using CSS custom-property tokens, `setValueAsGesture` (begin + set + end for single changes), double-click reset, and an idempotent `customElements.get()` guard before `define()`. |
| **NIH-plug webview / NovoNotes "WRAC"** (Rust, wry) | JSON IPC via serde; "adaptive IPC" that switches method by payload size. | Rust side. | `with_serve_zip(...)` serves from a zip. Meters stream at about 30 Hz without blocking the host main thread. NovoNotes reports about 15k users of WRAC-based plugins. |
| **Elementary SRVB** | Native messages; Vite + React + Tailwind. | — | Dev builds load from the Vite server (HMR works inside the host). Release bundles are packaged into the plugin. |
| **Spitfire LABS** (commercial, JUCE WebView) | TS glue that is "pretty framework agnostic". The ValueTree is recreated in TS. | One-way C++ → UI. The WebView is not kept alive while closed. | — Own glue code for DAW keypress behaviour. Jest unit tests for TS logic; styling is not tested. |
| **Output** (ADC24, Arthur Carabott) | — | — | "High Performance WebView User Interfaces": DOM vs 2D Canvas, a 60 fps zoomable waveform, **plain TypeScript, no frameworks**. |

**Patterns that recur across these stacks:**
- Native state is authoritative and the UI is a projection of it.
- Every change is bracketed by begin/end gesture calls; even single clicks get a begin + set + end sandwich.
- Assets are served from memory or a zip over a custom scheme.
- A dev server is used for iteration.
- High-rate data (meters, spectra) is pushed at 20–30 Hz with `emitEventIfBrowserIsVisible`, or pulled through the resource provider.
- Each control is its own element. Custom elements are the common denominator: iPlug2, Cmajor, g200kg `webaudio-controls`, and Travis Skare's ADC24 open-wc rotary demo all use them.

JUCE's own position: "JUCE supplies no UI controls … choose a frontend framework and component library", and "The JS library runs without errors when the backend is absent, which helps with visual testing" (JUCE 8 WebView feature overview).

---

## 2. Component strategy

### 2.1 Options

| | Vanilla light-DOM custom elements (recommended) | Lit (LitElement, shadow DOM) | Keep loose ES-module functions (today) |
|---|---|---|---|
| Dependencies / build | none; works with no build step | ~5–6 kB gz. Bare specifiers need a bundler, or the vendored single-file `lit-core.min.js` | none |
| Engine floor | Custom Elements v1 runs on any floor that matters | Lit 3 is published as ES2021, so Safari ≥14.1 (fine) | any |
| Theming of 46 different looks | full: plugin CSS reaches inside | only through custom properties and `::part`; global CSS cannot reach internals | full |
| i18n walker (`[data-i18n]`) | works unchanged | **blind**: `querySelectorAll` does not pierce shadow roots | works |
| Existing DOM-walking gates (check-ui-labels, measure-ui, censuses) | keep working | go blind unless rewritten | work |
| Playwright locators | work | work (open shadow is pierced, but XPath is not) | work |
| Cross-component ARIA (`aria-labelledby` to a label outside) | works | **broken in WebKit**: Reference Target shipped only in Chrome/Edge 152 (Sept 2026); WebKit has stated no position | works |
| Lifecycle / cleanup discipline | provided by the element lifecycle plus a tiny base class | provided | ad hoc (the source of the drift) |
| Fit with Cmajor / iPlug2 precedent | Cmajor = exact match | open-wc/Lit = ADC24 demo and the ADC25 Cmajor + Lit workshop | — |

**Recommendation:** use vanilla light-DOM custom elements with a base class of about 150 lines (`OElement`) providing:
- attribute → property reflection;
- `render()` batched with rAF;
- an `AbortController` per connection, so every listener is removed in `disconnectedCallback`;
- bridge subscription handling.

Nolan Lawson's analysis applies here: shadow DOM's value is encapsulation *between different authors*. Here the component author and the page author are the same person, and the cost lands on theming and tooling. Lit's own docs warn that light-DOM rendering in Lit is "generally not recommended", so using Lit with `createRenderRoot(){return this}` gets the worst of both. If list-heavy views later need templating (preset browser, tuning tables), import only **lit-html's `html` + `render(container)`**, which renders into light DOM natively.

Avoid the following:
- Customized built-ins (`is="..."`): WebKit "won't fix".
- `ElementInternals` and `adoptedStyleSheets`: both need Safari 16.4, see §2.5.
- `@font-face` inside shadow roots.

### 2.2 Authoring style: enhance markup that already exists

Write `<o-knob param="drive"><label data-i18n="drive">Drive</label></o-knob>`. The element adopts and wires its existing children (id, `aria-labelledby`) and renders only the dial and readout. This is Jeremy Keith's "HTML web components" approach, in which the element wraps markup that already works. It keeps `i18n-extract.js`, `check-i18n`, `check-ui-labels` and static scans working, because the strings stay in the HTML. Generated-only labels would hide them from the static scanners.

### 2.3 Contract every control must satisfy

These are the "break less" rules. Several of them are bugs this repo has already hit.
- **Gestures are always balanced.** Every path that changes a value calls `begin` then `end`:
  - drag: `begin` on pointerdown, `end` on pointerup, `pointercancel` or `lostpointercapture`;
  - wheel: `end` after about 250 ms idle;
  - keys: `begin` on the first key, `end` on keyup, blur, or after an idle timeout;
  - double-click reset and value-entry commit: begin + set + end.

  A watchdog also calls `end` on `blur`, `visibilitychange` and disconnect. A stuck "touch" leaves hosts in latch/touch automation state.
- **During your own gesture the local value is authoritative.** Incoming echoes are ignored, and the control resyncs to the backend value once the gesture ends.
- **Repaints are coalesced.** The bridge stores the latest value per parameter and schedules a single rAF that renders every changed control. There are no layout reads in the hot path; cache the control's rect once at pointerdown.
- **Defaults and formatting come from the manifest** (initialisationData). There are no literals in plugin JS.
- **Host parameter-under-mouse works.** The kit stamps the annotation attribute from `props.parameterIndex` and installs a single `ControlParameterIndexUpdater` mousemove listener, so this works uniformly in all 46 plugins.

### 2.4 Accessibility

The WAI-ARIA APG slider pattern applies to knobs:
- `role="slider"`, `aria-valuemin`, `aria-valuemax`, `aria-valuenow`;
- **`aria-valuetext` carrying the formatted, localized value** (for example "−6,0 dB" in French);
- the accessible name via `aria-labelledby` pointing at the visible label;
- `aria-orientation` for vertical sliders.

Keyboard behaviour:
- Arrows: one step, finer with Shift.
- PageUp / PageDown: a large step.
- Home / End: min / max.
- Enter: open value entry.
- Delete or Backspace (or double-click): reset to the default.

`Intl.NumberFormat` has no `decibel` or `hertz` unit [verified, MDN `supportedValuesOf`]. The kit's formatter therefore formats the number with `Intl` and appends the unit using locale spacing rules (a narrow no-break space before units in French). Value entry should accept both `,` and `.` as the decimal separator.

**Focus policy versus DAW shortcuts.** Focusable controls fight the host for the spacebar. The JUCE forum has no official fix; there are open threads on WebView keys not reaching Ableton or Logic, the spacebar being swallowed, and Windows being "not really solvable inside JUCE". Spitfire wrote its own keypress glue. Recommended policy:
- Tab reaches controls for keyboard users.
- Pointer interactions do not leave DOM focus behind.
- Esc blurs.
- A **native `forwardKeyToHost(code)` function** sends an allow-listed set of keys (Space and transport keys) back to the host window. This needs a spike on both OSes [inferred].

### 2.5 Engine floor: what you can actually use

WKWebView uses the system WebKit. On older macOS versions, Safari updates ship WebKit out of band through StagedFrameworks, so the engine follows the installed Safari version [inferred from WebKit bug 195543 and Apple's Safari updates for older macOS]. WebView2 Evergreen is always a recent Chromium, so **WKWebView sets the floor.**

| macOS | Latest Safari | Notes |
|---|---|---|
| 10.13 High Sierra | 13.1.2 | **JUCE's `index.js` cannot parse:** it uses the private method `#getControlParameterIndex`, which needs Safari ≥15 |
| 10.14 Mojave | 14.1.2 | same |
| 10.15 Catalina | 15.6.1 | the de facto floor today |
| 11 Big Sur | 16.6.1 | unlocks 16.x features |
| 12 Monterey | ~17.6 | |
| 13 Ventura | ≥18.6 | |

The repo builds with `CMAKE_OSX_DEPLOYMENT_TARGET 10.13` (`CMakeLists.txt:5`). All **46** plugins embed JUCE's `index.js`. The copies come in 3 byte-variants, but they differ only in whitespace (CRLF and one blank line), and every one contains the private method [verified]. **The installers promise 10.13, but the UI cannot load below Safari 15** [inferred; not run on real hardware].

Feature availability, desktop Safari, from caniuse [verified]:

| Feature | Safari version |
|---|---|
| `@layer` | 15.4 ✓ |
| `:focus-visible`, `:has()`, `dialog` | 15.4 ✓ |
| container queries | 16.0 |
| `@property`, import maps, `adoptedStyleSheets`, ElementInternals | 16.4 |
| CSS nesting | 16.5 partial / 17.2 full |
| Popover API, `modulepreload` | 17 |
| anchor positioning | 26 |

**Decision needed:**
- **(a)** Floor = Safari 15.6. Raise the deployment target to 10.15 so the installer is honest. The kit avoids 16.x features.
- **(b)** Floor = macOS 11 (Safari 16.6). This adds container queries, `@property` and import maps.

Either way, put the floor in one `browserslist` entry and enforce it with `eslint-plugin-compat` (JS) and `stylelint-no-unsupported-browser-features` (CSS).

If 10.13/10.14 support is genuinely required, run JUCE's `index.js` and the kit through `esbuild --target=safari13` (or `safari14`). That lowers syntax: private methods become WeakSet helpers and CSS nesting is flattened. It cannot polyfill CSS features (flex `gap` needs Safari 14.1, `@layer` needs 15.4), so this path is not recommended.

---

## 3. Build, bundling and versioning

### 3.1 Bundle or not

The V8 team says unbundled modules are fine for apps with fewer than 100 modules and a dependency depth under 5. The kit should be about 20–30 modules, imported from one `kit.js` entry, at depth ≤3. Over a local custom scheme there is no network RTT. Every request still goes through the host's message thread in both backends (WKURLSchemeHandler; WebView2 `WebResourceRequested`), so keep the module graph shallow [inferred].

**Do not add Vite.** If startup measurements in the Standalone later show module waterfalls, or if you keep a Safari floor below what your syntax needs, add **esbuild** in one place: either a kit-release step that emits a committed `kit.bundle.js`, or a CMake custom command. No per-plugin `node_modules` either way.

One exception already exists: O-TextureForge has its own webpack build (`Source/ui/package.json`, for regl-scatterplot). The kit must stay plain ES modules so bundled UIs like this one can import it too.

### 3.2 Delivery: one zip per plugin

The staging step was tested in the scratchpad: `cmake -E tar cf out.zip --format=zip --mtime=... -- .` produces clean relative entries [verified]. The overall CMake wiring is a sketch to prove in a spike.
```cmake
# ouaricon_add_webui(<target> ROOT Source/ui/public NAMESPACE OGainWebUI)
add_custom_command(OUTPUT ${zip}
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${stage}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ROOT} ${stage}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/modules/ui/kit/runtime ${stage}/kit
  COMMAND ${CMAKE_COMMAND} -E chdir ${stage} ${CMAKE_COMMAND} -E tar cf ${zip} --format=zip --mtime=2000-01-01 -- .
  DEPENDS ${ui_files} ${kit_files} VERBATIM)
juce_add_binary_data(${target}_WebUI NAMESPACE ${NAMESPACE} SOURCES ${zip})   # one file: no hyphen/collision traps
```
- **C++ side.** One `ZipResourceProvider`:
  - holds a static `ZipFile` over a `MemoryInputStream`;
  - maps `/` to `index.html`;
  - looks up MIME types by extension: html, js/mjs (`text/javascript`, which module scripts require), css, json, svg, png, jpg, webp, woff2, wasm, map;
  - in Debug only, can serve straight from a source directory named by an environment variable, giving an edit-and-reload loop in the real WebView.

  JUCE's `withResourceProvider(..., allowedOrigin)` can also allow `serve-ui.js` as a dev server in Debug. Never ship that origin.
- **The test tree equals the shipped tree.** `serve-ui.js` serves the staged directory or the zip itself, so there is no more symbol reverse-engineering.
- **Zero network, enforced in tests.** Fail on any request that is not to the resource root, any console error, and any 404. A CSP `<meta>` could add a runtime guard, but spike it in WKWebView first [inferred]:
  - `'self'` has to match the custom `juce://` origin;
  - about half the pages carry inline code (26 `index.html` files with inline `<script>`, 29 with `<style>` [verified grep]), which a strict policy would block.

### 3.3 Versioning a kit across 46 consumers on trunk

| Option | Pros | Cons |
|---|---|---|
| One version at HEAD (Google's "One-Version Rule") | no version sprawl; a fix reaches everyone | a change ships into plugin X at X's next release, possibly without X having been re-verified |
| Vendored copy per plugin (close to today) | reproducible | copies drift; this is exactly the 13–22 variants per component measured in R3 (`ui-canon.js` header) |
| Directory per major (`kit/v1`, `kit/v2`) | breaking changes without a big-bang rewrite | more than two live majors becomes a maintenance burden |

**Recommended:** one version at HEAD, with semver in `kit/VERSION` and a `CHANGELOG.md`.
- Each plugin has a `ui-kit.lock` holding `version + sha256(kit/runtime tree)`, which **attests** that the plugin's UI suite passed against that kit. It is not a copy.
- A gate reports plugins whose lock is stale. `/publish` blocks a stale lock.
- Every kit change runs the whole kit gallery suite plus every adopting plugin's smoke suite (`boot-all-uis` already exists).
- A breaking change goes into `kit/v2/` as transitional architecture. Plugins move one `/improve` at a time, and `v1` is deleted when no plugin uses it.
- Stamp `KIT_VERSION` into the page (`<html data-kit="1.4.2">`) and into `boot-all-uis` output for bug reports and a burn-down view.

Register the kit in `modules/registry.yaml` as `ui-kit` so `/module-*` works. Derive `used_by` from the lockfiles, because `registry.yaml` is already noted as stale for `scala-tuning-engine`.

### 3.4 Per-plugin customization

Customization goes through **tokens and layers, not forks**:
- **Semantic tokens** (`--o-surface`, `--o-ink`, `--o-accent`, `--o-control-track`, `--o-focus-ring`, and so on) are the theming API.
- **Component tokens** (`--o-knob-size`, `--o-knob-arc-width`, …) are added only where looks really differ. Nathan Curtis: many systems never need component tokens; theming is when they appear.
- **Variants as attributes** (`variant="large|mini"`) cover the two existing knob families.
- **Cascade layers.** Declare `@layer legacy, kit.reset, kit.tokens, kit.components, theme, plugin;`. During migration, wrap each plugin's existing CSS in `@layer legacy { … }` or `@import url(...) layer(legacy)`. Toyota's design system does exactly this ("Current website styles are wrapped in `@layer legacy {}` … replaced gradually").
- **Two gotchas with layers:**
  - Unlayered CSS beats every layer, so wrap it.
  - `!important` in an *earlier* layer beats `!important` in a later one, so the kit never uses `!important`.
- **Format.** Keep tokens as a CSS file. Move to DTCG JSON (first stable spec 2025.10) plus Style Dictionary only if the tokens must also feed C++ LookAndFeel or the website.

### 3.5 Types and lint without a build step

`// @ts-check` with a `jsconfig.json` (`checkJs`, `strict`), the same setup Cmajor ships. Types for the bridge come from a small `.d.ts`, or from `@juce-framework/webview`'s typings after moving to JUCE 9.

Add one tooling `package.json` with a lockfile pinning `@playwright/test`, the Playwright browsers, `@axe-core/playwright`, `typescript`, eslint + compat and stylelint + plugin. Today `boot-all-uis.js` resolves *whatever* Playwright is installed (chromium only), and that cannot hold screenshot references stable.

---

## 4. Visual quality

- **Tokens.** Use three tiers (primitive → semantic → component; Rangle and Curtis):
  - a spacing scale on a 4 px base;
  - a type scale of 3–4 sizes;
  - radius, shadow and motion tokens (100–150 ms ease-out on hover/press, **no animation on automation-driven value changes**);
  - focus-ring tokens;
  - colours in `oklch()` (Safari 15.4 ✓). Avoid `color-mix()` (Safari 16.2) unless the floor is raised; precompute shades as tokens instead.

  The repo's "aesthetic templates" (ui-template-library) could be saved as token files, which would make them executable.
- **Knobs.**
  - Use **SVG in light DOM**: track plus value arc plus pointer.
  - Compute the arc's `d` (an `A` command) from the value in JS. Do not rely on `pathLength` or dash-offset tricks: WebKit has open dash/pathLength rendering bugs (bug 258897, moved to CoreGraphics). Cmajor also computes dash lengths explicitly.
  - Rotate the pointer with a `transform` write. Use `vector-effect: non-scaling-stroke` so strokes stay constant when scaled.
  - Use Pointer Events with `setPointerCapture` and `touch-action: none`.
  - Measure drag sensitivity in px per full range, with Shift for fine control.
  - Normalize `deltaMode` for the wheel and accumulate fractional steps (trackpads send many small deltas).
  - `preventDefault` ctrl+wheel (pinch and zoom).
  - Avoid filmstrip PNGs: they are heavy, not themeable, and blur when scaled.
- **Meters, spectra, waveforms.**
  - Use `<canvas>` with a backing store of CSS size × `devicePixelRatio` × UI scale. Re-run sizing when `matchMedia('(resolution: Xdppx)')` changes (MDN).
  - Run the rAF loop only while the canvas is visible.
  - Receive data at 20–30 Hz with `emitEventIfBrowserIsVisible` and interpolate per frame; Output's ADC talk shows 60 fps waveforms with this split.
  - Batch DOM reads before writes; layout thrashing is the classic killer (web.dev).
- **Typography on paper textures.**
  - Use tabular figures and fixed-width readout boxes (already the house style).
  - Embedded fonts load before first paint: `document.fonts.load()` for each face/weight in use, then the "ready" flag.
  - Keep critical background CSS inline in `<head>` to avoid a white flash. On WebView2, also use `withBackgroundColour`; JUCE has no transparent-background option for WKWebView [verified in `juce_WebBrowserComponent_mac.mm`].
  - Watch thin Garamond strokes under DirectWrite at small sizes. Use a minimum size token and test at 125% and 150% Windows scaling.
  - **CJK.** The UIs fall back to system PingFang SC (macOS) and Microsoft YaHei (Windows) [verified: 111/94 references]. These are sans faces with different metrics; the repo's notes already record line-box differences. A recommended option: **subset-embed a serif CJK face** (Noto Serif SC or Source Han Serif, SIL OFL) restricted to glyphs in the zh-Hans string tables, through `unicode-range`. Metrics then match on both OSes, the look matches EB Garamond, and screenshots become deterministic. The cost is a subsetting step whenever strings change.
- **Resizing and scaling.**
  - **C++:** `setResizable`, `setResizeLimits`, `getConstrainer()->setFixedAspectRatio()`, and persist the editor size in state.
  - **Web:** a fixed design stage (for example 900×560 CSS px) with `transform: scale(s)`, `transform-origin: 0 0`, where `s = min(innerWidth/W, innerHeight/H)`, recomputed by a ResizeObserver throttled to rAF.
  - **No `will-change: transform` on the stage**: it rasterizes at 1× and then blurs [inferred from Chromium compositing behaviour].
  - Canvases re-render at DPR × s.
  - Avoid CSS `zoom`: it is standardized in Chromium/Firefox, but Safari has historically returned un-zoomed `getBoundingClientRect` values.
  - Use container queries only if the floor is macOS 11 or later.
- **Textures and icons.** Ship 2× textures with `background-size`. Use one SVG sprite with `currentColor`. Set `user-select: none` and `-webkit-user-drag: none` on control chrome.

---

## 5. Visual regression and behaviour testing

1. **Pure unit tests (node).** Cover value mapping (skew, interval, sampled lambda tables), formatters checked against `param-dump` `getText()` at N points (this makes `param-dump` the contract source), i18n interpolation, and wheel/drag math.
2. **Kit gallery** (`modules/ui/kit/dev/gallery.html`). A plain page; Storybook is optional. Each `<section data-story="knob/bipolar/hover/fr/scale-1.5">` renders against the **bridge-level stub**. Playwright enumerates the stories, then for each one:
   - takes `locator.toHaveScreenshot()`;
   - asserts the ARIA attributes;
   - asserts **the event log**: a drag must emit `begin, value…, end`; double-click must emit begin + default + end; Esc during value entry must emit nothing.

   Run it in the **chromium** (close to WebView2) and **webkit** (close to WKWebView; Playwright's WebKit is built from WebKit main and "closest to Safari on macOS") projects. Snapshot names already include the browser and platform.
3. **Per-plugin page smoke.** Use the **wire-level stub** (`window.__JUCE__`, installed with `addInitScript` or the existing preamble) fed by that plugin's `param-dump` TSV. Each plugin page must have:
   - no console errors, 404s or CSP violations;
   - every `param=` bound to a known ID;
   - existing label-fit and i18n gates passing;
   - **one screenshot per locale** at the default size.
4. **Determinism.**
   - Use a fixed viewport and `deviceScaleFactor`, with `scale:'device'` on a few tests to catch HiDPI and canvas bugs.
   - Load fonts explicitly before the shot.
   - Leave `animations:'disabled'` and `caret:'hide'` at their defaults.
   - `mask` meters and live canvases.
   - Freeze timers with `page.clock` and use `emulateMedia({reducedMotion:'reduce'})`.
   - For Chromium, add `--font-render-hinting=none --disable-lcd-text` (Argos).
   - Generate and compare references in **one environment** (your Mac, or a pinned Playwright Docker image in CI), and regenerate them on every Playwright upgrade.
   - Leave the per-pixel `threshold` at 0.2. Use small `maxDiffPixels` for component shots and `maxDiffPixelRatio` of about 0.001 for page shots.
5. **Accessibility.**
   - Run `@axe-core/playwright` with `withTags(['wcag2a','wcag2aa','wcag21aa'])` on the gallery and every plugin page.
   - Snapshot **fingerprints** (rule ID + target), not the raw violations array (Playwright's own advice).
   - axe cannot judge contrast over `background-image`; it leaves these unflagged or "needs review". Add a custom check: screenshot with text hidden through `stylePath`, sample the luminance under each text box, compare with the text colour, and require ≥4.5:1.
6. **Real-engine coverage** [inferred, cheap]. A Standalone `--ui-selftest` mode loads the gallery in the *actual* WKWebView or WebView2, runs the kit's assertions in-page, and reports through a native function before exiting. This is the only way to test the Safari floor (for example on a Catalina or Big Sur VM) and Windows DPI or accelerator behaviour.

---

## 6. Migration with the strangler pattern ("gradual … transitional architecture … earlier return", Fowler)

**Phase 0: safety net and plumbing (no visible change)**
- Freeze current behaviour as characterization tests: per-plugin screenshots in EN/FR/zh plus gesture probes on the param-dump stub.
- Ship `ouaricon_add_webui()`, the ZipResourceProvider and `makeWebViewOptions()`. Convert plugins one by one; **acceptance = pixel-identical screenshots** and an identical event log.
- Add the bridge (JUCE 8 backend), hardening, the manifest, and a `KIT_VERSION` stamp.

**Phase 1: unify behaviour component by component, keeping the legacy look**
- Order components by the ui-canon drift report: tooltip renderer → hover-help → knob binding → knob visual → value entry → toggle/select → preset bar → gear popover.
- Pilot every component on **two extremes** before freezing its API: O-Gain (small) and O-Prism (173 parameters, IDs built at runtime) or O-MicrotonalSampler.
- A component reaches the kit only after three plugins need it (rule of three).
- Per plugin: wrap legacy CSS in `@layer legacy`, swap markup to `<o-*>`, delete the old JS path, and expect screenshots to show **parity**. Turn `check-ui-canon.js` from report-only into a **ratchet**: the non-kit count may only go down.

**Phase 2: deliberate visual refresh**
- Per plugin, inside its own `/improve`: switch the theme tokens. Screenshot diffs are the review artefact (the Playwright HTML report has an expected/actual/diff slider). One plugin per version bump.

**Phase 3: resizing and scaling**, plus the optional CJK subset font.

**Done when** no `legacy` layer remains, every lock is current, `kit/v1` has no adopters left (if `v2` exists), and ui-canon reports 0 non-kit implementations.

---

## 7. Pitfalls specific to WKWebView / WebView2 (with this repo's exposure)

1. **Engine floor mismatch.** The deployment target is 10.13, but JUCE `index.js` needs Safari 15 (§2.5). Lint the kit against the floor and test on a floor VM.
2. **JUCE 9.0.1 moved the interop JS** (`native/javascript` → `native/typescript/webview-interop/dist`), and a version-mismatch assert was added. Keep the path behind `bridge.js` and the copy step.
3. **WebView2 browser accelerators are live.** JUCE 8.0.15 touches only DevTools (off in Release), status bar, error page, user agent and background colour [verified, `juce_WebBrowserComponent_windows.cpp:1005-1035`]. The following all still work unless disabled:
   - Ctrl+R/F5 reload, which resets page state;
   - Ctrl+F, Ctrl+P;
   - Ctrl+± and Ctrl+wheel zoom, which breaks fixed layouts;
   - the default context menu.

   The reliable fix is native: `put_AreBrowserAcceleratorKeysEnabled(FALSE)`, `put_IsZoomControlEnabled(FALSE)` and `put_AreDefaultContextMenusEnabled(FALSE)`. These apply after the next navigation, so set them before `goToURL`. Do it through your `vendored/JUCE-overrides` mechanism, and keep a JS fallback in kit hardening (`contextmenu`, ctrl+wheel, F5/Ctrl+R `keydown`). Only 20 of 46 UIs mention `contextmenu` at all [verified by grep; behaviour not tested].
4. **WebView2 user-data folder.** **16 plugins pass the bare `%TEMP%` directory** (O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bells, O-Chorus, O-Comp, O-Detune, O-Freeze, O-IntonationPad, O-Lyrica, O-Marimba, O-MultiBandCompressor, O-Polystutter, O-Reed, O-SpectralShaper, O-Tremolo) [verified]. That is the JUCE demo's choice, so the folder is shared with any other plugin that copied the demo. Microsoft: a second environment on the same folder with *different options* "will fail to create WebView2 objects". Use a suite folder such as `%LOCALAPPDATA%/Ouaricon/WebView2` with identical options built by the shared helper, so one browser process serves all Ouaricon plugins, or per-plugin folders, which cost more memory.
5. **Keyboard focus versus the DAW** (Space, transport keys). There is no JUCE fix; see §2.4. On macOS, JUCE already forwards Cmd-X/C/V/A through `performKeyEquivalent` [verified]. On WebView2, clipboard keys are always enabled.
6. **Hidden editors.** WebView2 `IsVisible=false` suspends work and `emitEventIfBrowserIsVisible` drops events, so resync on visibility (already in memory notes). rAF is throttled when the view is occluded, and to about 30 fps in Low Power Mode on iOS WebKit (unconfirmed on macOS). Never tie value logic to rAF cadence.
7. **Processes and memory.**
   - WKWebView gives each view its own WebContent process until a limit, then shares within a `WKProcessPool` (Apple).
   - WebView2 shares one browser process per user-data folder.
   - Many instances means many renderers. Spitfire did not keep the WebView alive while the editor is closed.
8. **MIME strictness.** Module scripts fail without a JS MIME type. Fonts, wasm and json need correct types too. The JUCE demo `jassert`s on unknown extensions.
9. **Requests run on the host's message thread** in both backends. A busy host slows UI load, so keep the module graph shallow and measure time-to-interactive in the Standalone.
10. **Inspecting the real WebView.** JUCE sets `developerExtrasEnabled` in Debug only, and never sets WKWebView `inspectable`. For apps linked against SDK ≥ macOS 13.3, `isInspectable` defaults to false (WebKit blog), so add it to a Debug override. WebView2 DevTools are off in Release.
11. **First paint.** WKWebView shows a white flash; inline the background colour in `<head>` and use `withBackgroundColour` on WebView2. With `acceptsFirstMouse`, JUCE defaults to click-through; keep it.
12. **Scrollbars.** Windows uses classic, space-consuming scrollbars; macOS uses overlay scrollbars. Layouts measured on a Mac do not show Windows overflow, so style scrollbars explicitly or avoid scrolling regions.
13. **Text rendering differs** (CoreText vs DirectWrite; system CJK fallbacks; fractional DPI on Windows). Keep separate screenshot references per engine and platform; never compare across them.
14. **Web Components gaps in WebKit:** no customized built-ins, no cross-root ARIA, and ElementInternals/`adoptedStyleSheets` only from 16.4.
15. **Linux** (if ever): there is a forum report of `juce://` resource-provider requests not reaching the provider and crashing on WebKitGTK.

---

## 8. Decisions for Taylor, each with a recommended default

1. Engine floor: **Safari 15.6 (raise the deployment target to 10.15)**, or macOS 11.
2. Kit versioning: **HEAD plus a per-plugin lock gated in `/publish`**, or pinned copies.
3. Spike first: **zip provider + `ouaricon_add_webui()` on O-Gain**, with pixel-identical acceptance.
4. Spike: native WebView2 settings override (accelerators, zoom, context menu) **plus a suite-wide user-data folder**.
5. Spike: `forwardKeyToHost` for Space on macOS and Windows.
6. Optional: embed a subset CJK serif font.

---

## 9. Sources

JUCE
- JUCE 8 WebView feature overview — https://juce.com/blog/juce-8-feature-overview-webview-uis/
- WebViewPluginDemo (zip, MIME map, ControlParameterIndexReceiver, 20 Hz spectrum) — https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/WebViewPluginDemo.h
- BREAKING_CHANGES (9.0.1 package move; 8.0.2 relay ctor change) — https://github.com/juce-framework/JUCE/blob/master/BREAKING_CHANGES.md
- JUCE 9.0.1 announcement (TypeScript npm package) — https://forum.juce.com/t/uce-9-0-1-is-out/69269 · npm metadata — https://registry.npmjs.org/@juce-framework/webview
- JUCE 8 WebView video tutorials (official channel) — https://forum.juce.com/t/juce-8-tutorial-videos/62554
- Keyboard focus threads — https://forum.juce.com/t/fixing-webview-keyboard-focus-issue/63987 · https://forum.juce.com/t/can-webbrowsercomponent-forward-key-presses-to-the-daw/50729
- Linux provider issue — https://forum.juce.com/t/webbrowsercomponent-very-poor-linux-experience/66681
- Spitfire LABS interview — https://juce.com/made-with-juce/jake-from-spitfire/
- Faceplate (visual designer for JUCE WebView2) — https://forum.juce.com/t/faceplate-a-visual-ui-designer-for-juce-webview2-plugins/68135
- Local, read-only: `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/index.js` (8.0.15), `.../native/juce_WebBrowserComponent_{mac.mm,windows.cpp}`, `.../misc/juce_WebBrowserComponent.h`

Other plugin stacks
- choc WebView — https://github.com/Tracktion/choc/blob/main/choc/gui/choc_WebView.h
- iPlug2 WebView docs — https://mintlify.wiki/iPlug2/iPlug2/ui-frameworks/webview
- Cmajor patch format — https://cmajor.dev/docs/PatchFormat · cmaj_api controls — https://github.com/cmajor-lang/cmajor/tree/main/javascript/cmaj_api
- nih-plug-webview — https://github.com/maxjvh/nih-plug-webview · NovoNotes WRAC talk — https://speakerdeck.com/novonotes/write-0-lines-of-c-plus-plus-building-commercial-audio-plugins-with-rust-and-web-gui
- Elementary SRVB — https://github.com/elemaudio/srvb · Sound-Field (React + JUCE 8 relays) — https://github.com/mbarzach/Sound-Field
- ADC24 "High Performance WebView User Interfaces" (Output) — https://conference.audio.dev/session/2024/high-performance-webview-user-interfaces/
- ADC24 "Frontend Development Strategies For WebUI" — https://conference.audio.dev/session/2024/frontend-development-strategies-for-webui/ · demos (open-wc rotary, Storybook, WTR) — https://github.com/tskare/adc24-demos
- ADC25 workshop "Web UIs for Music Apps" (Cmajor + Lit) — https://conference.audio.dev/session/2025/workshop-web-uis-for-music-apps/
- g200kg webaudio-controls — https://www.g200kg.com/webaudio-controls/test.html

Web components and accessibility
- Lit shadow DOM / createRenderRoot — https://lit.dev/docs/components/shadow-dom/ · Lit 3 (ES2021) — https://lit.dev/blog/2023-10-10-lit-3.0/
- Nolan Lawson, Shadow DOM and encapsulation — https://nolanlawson.com/2023/12/30/shadow-dom-and-the-problem-of-encapsulation/
- Jeremy Keith, HTML web components — https://dev.to/adactio/my-approach-to-html-web-components-5h9 · LWC light DOM — https://developer.salesforce.com/docs/platform/lwc/guide/create-light-dom.html
- Reference Target (Chrome 152) — https://developer.chrome.com/release-notes/152 · explainer — https://github.com/WICG/webcomponents/blob/gh-pages/proposals/reference-target-explainer.md
- WebKit and customized built-ins — https://webkit.org/b/182671 · https://github.com/w3c/webcomponents-cg/discussions/71
- APG slider pattern — https://www.w3.org/WAI/ARIA/apg/patterns/slider/
- Intl unit identifiers — https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl/supportedValuesOf

Engines and platforms
- caniuse: https://caniuse.com/mdn-javascript_classes_private_class_methods · https://caniuse.com/css-cascade-layers · https://caniuse.com/import-maps · https://caniuse.com/css-nesting · https://caniuse.com/css-container-queries · https://caniuse.com/mdn-api_elementinternals · https://caniuse.com/mdn-api_document_adoptedstylesheets · https://caniuse.com/mdn-css_at-rules_property
- Safari 15.6.1 for Catalina/Big Sur — https://9to5mac.com/2022/08/18/safari-15-6-1-security-fixes-macos/ · StagedFrameworks — https://bugs.webkit.org/show_bug.cgi?id=195543 · Safari 18.6 on Ventura/Sonoma — https://webkit.org/?p=17330
- WKWebView isInspectable — https://webkit.org/blog/13936/enabling-the-inspection-of-web-content-in-apps/ · WKProcessPool — https://developer.apple.com/documentation/webkit/wkprocesspool
- WebView2 process model / user-data folder — https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/process-model
- WebView2 AreBrowserAcceleratorKeysEnabled — https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2settings3 · settings (zoom, context menus) — https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2settings
- WebResourceRequested vs virtual host — https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/webresourcerequested
- WebKit rAF Low Power Mode (iOS) — https://bugs.webkit.org/show_bug.cgi?id=215745 · WebKit dash/pathLength bug — https://bugs.webkit.org/show_bug.cgi?id=258897
- CSS zoom standardization — https://groups.google.com/a/chromium.org/g/blink-dev/c/W8j6RKDeRoM/m/FyCKOAG9AAAJ · zoom vs transform — https://blog.openreplay.com/css-zoom-scale-ui-elements/
- devicePixelRatio and canvas — https://developer.mozilla.org/en-US/docs/Web/API/Window/devicePixelRatio · layout thrashing — https://web.dev/articles/avoid-large-complex-layouts-and-layout-thrashing

Build and versioning
- V8 on modules vs bundling — https://v8.dev/features/modules
- esbuild target lowering — https://esbuild.github.io/api/#target · https://github.com/evanw/esbuild/blob/main/CHANGELOG-2023.md
- CMake ARCHIVE_CREATE / `-E tar --format=zip` — https://cmake.org/cmake/help/latest/command/file.html#archive-create
- Strangler fig — https://martinfowler.com/bliki/StranglerFigApplication.html · Branch by abstraction — https://martinfowler.com/bliki/BranchByAbstraction.html
- One-Version Rule — https://opensource.google/docs/thirdparty/oneversion/ · https://buttondown.com/j2kun/archive/monorepos-and-forced-migrations

Design tokens and CSS architecture
- DTCG first stable spec 2025.10 — https://www.w3.org/community/design-tokens/2025/10/28/design-tokens-specification-reaches-first-stable-version/
- Token tiers — https://rangle.io/blog/developing-your-token-structure
- Cascade layers, wrapping legacy CSS — https://dx-playbook.toyota-europe.com/cds/architecture/layers

Testing
- Playwright visual comparisons — https://playwright.dev/docs/test-snapshots · toHaveScreenshot options — https://playwright.dev/docs/api/class-pageassertions#page-assertions-to-have-screenshot-1
- Playwright browsers (WebKit vs Safari) — https://playwright.dev/docs/browsers · addInitScript — https://playwright.dev/docs/api/class-browsercontext#browser-context-add-init-script · locators and shadow DOM — https://playwright.dev/docs/locators
- Accessibility testing with axe — https://playwright.dev/docs/accessibility-testing
- Stable text rendering flags — https://argos-ci.com/docs/learn/reliability-and-flakiness/flaky-tests/stabilize-text-rendering
- axe color-contrast and background images — https://rocketvalidator.com/accessibility-validation/axe/4.12/color-contrast · https://www.deque.com/blog/upgrade-accessibility-testing-axe-extension-3-0/
- Optional heavier tooling: Storybook web-components-vite — https://storybook.js.org/docs/get-started/frameworks/web-components-vite · Web Test Runner — https://modern-web.dev/docs/test-runner/overview/
