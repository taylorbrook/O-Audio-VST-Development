---
task: 260826-ieq-multi-language-tooltips-across-all-vst-p
type: execute
mode: quick
autonomous: false          # two checkpoints: after Stage B (pattern review) and Stage D (French geometry)
staged: true
files_modified:
  - scripts/param-dump/{main.cpp,ParamDump.cmake}
  - scripts/i18n-canon.js
  - scripts/check-i18n.js
  - plugins/<Name>/Source/ui/public/js/i18n.js       # 33 plugins on this root
  - plugins/<Name>/Resources/ui/js/i18n.js           # 10 plugins on this root
  - plugins/<Name>/Source/ui/public/js/app.js  (or Resources/ui/js/app.js)
  - plugins/<Name>/Source/ui/public/index.html (or Resources/ui/index.html)
  - plugins/<Name>/Source/ui/public/css/styles.css   # gear popover only; NOT .tooltip
  - plugins/<Name>/Source/Plugin{Processor,Editor}.{h,cpp}
  - plugins/<Name>/CMakeLists.txt
  - plugins/<Name>/CHANGELOG.md
  - plugins/O-Octagon/tests/{ui_frontend_check.js,ui-stub/juce-stub.js}
  - plugins/O-ReverseDelay/tests/{ui_frontend_check.js,ui_tooltip_clamp_check.js,ui-stub/juce-stub.js}
  - plugins/O-Contrabass/tests/ui_frontend_check.js
  - plugins/O-Bitrot/tests/{ui_tooltip_clamp_check.js,ui-stub/juce-stub.js}
  - plugins/O-Tapestop/tests/{ui_tooltip_clamp_check.js,ui-stub/juce-stub.js}
  - PLUGINS.md

estimate:
  tokens: 900000           # across all 8 tasks; NOT a single-agent budget — see <staging>
  raw_tokens: 450000
  tasks: 8
  confidence: low          # no calibration samples for a 43-plugin rollout in this repo

must_haves:
  truths:
    - "A user opens any localized plugin, clicks the gear, picks Français, and every tooltip re-renders in French without a reload."
    - "Closing and reopening the DAW session restores the chosen language; loading a preset does NOT change it."
    - "A fresh install with no saved state shows English, and shows it synchronously — never a blank tooltip, never a flash of the wrong language."
    - "Tooltip copy exists in exactly one place per plugin (js/i18n.js) and nowhere in index.html."
    - "Every tooltip in a localized plugin is rendered by ONE renderer — the measure-then-pin data-tip runtime."
    - "French tooltips stay fully inside the shipping viewport on every anchor of every clamp-gated plugin."
  artifacts:
    - "scripts/param-dump/{main.cpp,ParamDump.cmake} — runtime getParameters() inventory tool"
    - "scripts/i18n-canon.js — the canonical applyI18n block, byte-compared against every plugin"
    - "scripts/check-i18n.js — repo-level i18n consistency + drift gate"
    - "plugins/<Name>/<uiroot>/js/i18n.js — per-plugin I18N + TIP_BINDINGS + tr()"
  key_links:
    - "i18n.js -> juce_add_binary_data SOURCES -> getResource() branch -> app.js import (all four, one commit)"
    - "gear <select#lang-select> -> applyI18n() -> setUiLanguage native fn -> std::atomic<int> uiLanguage -> state.setProperty(\"uiLanguage\")"
    - "getUiLanguage PULL at page init -> applyI18n() (English painted synchronously BEFORE the promise)"
    - "TIP_BINDINGS keys -> I18N keys -> data-tip/data-tip-title attributes -> the unchanged measure-then-pin renderer"
---

<objective>
Move every plugin's tooltip copy out of markup and into a per-plugin `js/i18n.js` key table,
render it through the single `data-tip` measure-then-pin runtime, and add a gear popover
carrying a Language selector (English / Français) whose choice persists on the APVTS state
tree.

Purpose: one tooltip convention repo-wide, and hover-help that a French-speaking user can
actually read.
Output: 43 localized plugin UIs, 2 repo-level tools, 6 rewritten static gates.
</objective>

<context>
@/Users/taylorbrook/Dev/VST-development/CLAUDE.md
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-CONTEXT.md
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-RESEARCH.md

Reference implementations (read the named line ranges, do not re-read whole files):
@plugins/O-MultiBandCompressor/Source/ui/public/js/app.js   # applyTooltip :1379-1391, tables :1173-1256, bridge :1315-1332, foot-of-file deferred init
@plugins/O-ReverseDelay/Source/ui/public/js/app.js          # the measure-then-pin renderer :913-1028
@plugins/O-Octagon/Source/PluginEditor.cpp                  # getResource :181-247, native-fn pair :1278-1300
@plugins/O-Octagon/Source/PluginProcessor.cpp               # XML-root-attribute persistence :867-901
@plugins/O-Bitrot/Source/PluginProcessor.cpp                # ValueTree-property persistence :1751-1795
@plugins/O-Bitrot/tests/render-harness/CMakeLists.txt       # the console-app-against-the-processor target pattern
</context>

<facts_resolved_during_planning>
Verified this session — do NOT re-derive:

- **43 plugins.** 33 serve `Source/ui/public/`, 10 serve `Resources/ui/`.
- **O-MicrotonalSampler serves `Resources/ui/`** (`CMakeLists.txt:80-99`). Its
  `Source/ui/public/` is a build-time JS-copy staging dir only (`CMakeLists.txt:66`).
- **O-Orbit serves `Resources/ui/`** (`CMakeLists.txt:54-62`). Same situation; its
  `Source/ui/public/` is not embedded. RESEARCH decision 14 is now CLOSED.
- **THREE clamp gates exist, not two:** `O-Bitrot`, `O-ReverseDelay`, **and `O-Tapestop`**
  (`plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js`, `SHIP_W/H = 860 x 580`,
  `NATURAL_MAX_W = 230`). RESEARCH §D3 undercounted. All three must be language-parameterised.
- **Three hard native-fn COUNT assertions** break the moment the two-function language pair
  is added:
  | Gate | Line | Current | After |
  |---|---|---|---|
  | `O-Octagon/tests/ui_frontend_check.js` | `:235` `registered.size === 23` | 23 | 25 |
  | `O-ReverseDelay/tests/ui_frontend_check.js` | `:190` `called.size === 13 && registered.size === 13` | 13 | 15 |
  | `O-Contrabass/tests/ui_frontend_check.js` | `:123` `called.size === 34 && registered.size === 34` | 34 | 36 |
- **O-Octagon also asserts `setsEqual(stubbed, registered)`** (`:239`) — the ui-stub whitelist
  must learn `getUiLanguage`/`setUiLanguage` in the SAME commit or the gate fails.
- **O-Octagon §2 (`:169-191`) forbids module-level declarations after `init();` and requires
  `init();` to be the literal last statement.** So on O-Octagon `applyI18n()` is called from
  INSIDE `init()`, and `import { I18N, TIP_BINDINGS, tr } from './i18n.js'` (hoisted) is the
  only new top-level form. `i18n.js` must never self-execute.
- **O-ReverseDelay `ui_frontend_check.js:691-696`** asserts all 14 `TIP_ANCHORS` carry
  `data-tip=` + `data-tip-title=` **in index.html**. Fails by construction once copy moves.
- **O-ReverseDelay `ui_frontend_check.js:760-761`** forbids `setTooltipsEnabled` by NAME.
  Per D13 it STAYS. O-ReverseDelay gets the language pair only, so the gate stays green.
- **Five ui-stub files exist:** O-Bitrot, O-Octagon, O-ReverseDelay, O-SpectralShaper,
  O-Tapestop. Each needs the two new names.
- **Version lives in different places per plugin** — `set(<PLUGIN>_VERSION "x.y.z")` then
  `VERSION "${...}"` (O-simpleFM), or a literal `VERSION x.y.z` in `juce_add_plugin`
  (O-Octagon `:15`, O-Bassoon `:14`). Read each plugin's CMakeLists; never assume.
</facts_resolved_during_planning>

<canonical_contract>
This block is the single source of truth replicated into all 43 plugins. Task 2 writes it to
`scripts/i18n-canon.js`; `scripts/check-i18n.js` byte-compares (whitespace-normalised) every
plugin's copy against it. Deviate only where a `[VARIES]` marker says so.

### 1. `<uiroot>/js/i18n.js` — data + lookup, zero side effects

```js
// js/i18n.js — an ES module that EXPORTS ONLY. It must never self-execute:
// a top-level statement here throws out of module evaluation and takes every
// later initializer with it (pattern_module_toplevel_init_tdz, MBC v1.4.0).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens, so a second file
// named i18n-fr.js would have to be reached as the symbol i18nfr_js. One
// combined file for both languages sidesteps the question entirely.

export const LANGUAGES = ['en', 'fr'];

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title, b = tooltip body.
//   fr.reviewed is REQUIRED and starts false: machine-drafted French that is
//   never marked degrades silently into "we shipped English in a French UI".
export const I18N = {
  'sync-segments': {
    en: { t: 'Segments', b: 'How many slices the buffer is cut into.' },
    fr: { t: 'Segments', b: 'Nombre de tranches découpées dans le tampon.', reviewed: false },
  },
  // Parameterised entries carry {token} placeholders, substituted by tr()'s
  // `vars` argument. NOT a template literal — the table is inert data.
  'band.gain': {
    en: { t: '{band} Gain', b: 'Output level of the {band} band.' },
    fr: { t: 'Gain {band}', b: 'Niveau de sortie de la bande {band}.', reviewed: false },
  },
};

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars]. The selector is the BINDING SITE —
// MBC proves tips must be able to attach to wrappers the HTML does not mark up
// (.control-group, .spectrum-container), which a key-on-the-element attribute
// cannot express. Id-anchored plugins simply use '#the-id'.
export const TIP_BINDINGS = [
  ['#sync-segments', 'sync-segments'],
];

export function tr(key, lang, vars) {
  const entry = I18N[key];
  if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
  const s = entry[lang] || entry.en;
  const sub = (v) => vars
    ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? vars[n] : m))
    : String(v);
  return { t: sub(s.t), b: sub(s.b) };
}
```

### 2. `app.js` — the replicated runtime block

Declared ABOVE every reader. Called from the plugin's existing deferred/foot-of-file init,
inside its own `try/catch`, never from eager module top level.

```js
import { LANGUAGES, I18N, TIP_BINDINGS, tr } from './i18n.js';

let uiLanguage = 'en';
let getUiLanguageNative = null;
let setUiLanguageNative = null;

function applyI18n(lang) {
    uiLanguage = LANGUAGES.includes(lang) ? lang : 'en';
    for (const [selector, key, wrapper, vars] of TIP_BINDINGS) {
        const el = document.querySelector(selector);
        if (!el) { console.warn(`i18n: tip target not found: ${selector}`); continue; }
        const target = wrapper ? (el.closest(wrapper) || el) : el;
        const s = tr(key, uiLanguage, vars);
        target.setAttribute('data-tip-title', s.t);
        target.setAttribute('data-tip', s.b);
    }
    const sel = document.getElementById('lang-select');
    if (sel && sel.value !== uiLanguage) sel.value = uiLanguage;
}

// Exposed so the clamp gate can drive the language without teaching the stub
// a promise contract: page.evaluate((l) => window.__setLanguage(l), 'fr').
window.__setLanguage = applyI18n;

function initI18n() {
    try {
        getUiLanguageNative = Juce.getNativeFunction('getUiLanguage');
        setUiLanguageNative = Juce.getNativeFunction('setUiLanguage');
    } catch (e) {
        console.warn('Language preference not available, session-only:', e);
    }

    // Paint the default SYNCHRONOUSLY first. Never blank, never a flash.
    try { applyI18n('en'); } catch (e) { console.error('i18n init failed:', e); }

    if (getUiLanguageNative) {
        getUiLanguageNative()
            .then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}
```

One PULL, no push, no timer, no `poll().then(poll)`, no revision counter. The language is not
preset content: `OuariconPresetManager::loadPreset` walks `preset["parameters"]` and never
touches state-tree properties, so no preset path can change it.

### 3. Gear popover markup

Structure identical everywhere; **styling is per-plugin** and must sit inside that plugin's own
visual system. Do not paste one widget in unchanged.

```html
<button id="gear-btn" class="gear-btn" type="button"
        aria-haspopup="dialog" aria-expanded="false" aria-label="Settings">⚙</button>
<div id="settings-popover" class="settings-popover" role="dialog" aria-label="Settings" hidden>
  <label class="settings-row" for="lang-select">
    <span data-label="Language">Language</span>
    <select id="lang-select">
      <option value="en">English</option>
      <option value="fr">Français</option>
    </select>
  </label>
  <!-- The hover-help row appears ONLY on the 9 plugins that already have the
       setTooltipsEnabled bridge. It MOVES here; it is not duplicated. -->
  <label class="settings-row" for="tips-toggle">
    <span data-label="Hover help">Hover help</span>
    <button id="tips-toggle" type="button" aria-pressed="true">On</button>
  </label>
</div>
```

The `<option>` captions are **endonyms** — `English` / `Français` — and are never localized and
never written by a state updater. Every label span carries `data-label`, so a shared updater
that writes `textContent` reads `el.dataset.label ?? fallback` instead of erasing the authored
caption. Add `aria-pressed` wherever a caption replaces On/Off text.

### 4. C++ — persistence + bridge

```cpp
// PluginProcessor.h
// 0 = en, 1 = fr. std::atomic<juce::String> does not compile (non-trivially
// copyable), so the audio-safe form is an int index behind a two-function codec
// while the PERSISTED form stays a string.
std::atomic<int> uiLanguage { 0 };
static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
static int         languageIndex  (const juce::String& s) { return s == "fr" ? 1 : 0; }
```

Persistence — **mirror whichever idiom the plugin already uses for `tooltipsEnabled`**:

```cpp
// Idiom 1 — ValueTree property (O-Bitrot, O-Orbit, O-Tapestop, most plugins)
auto state = apvts.copyState();
state.setProperty ("uiLanguage", languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);
// ...restore, after replaceState:
const juce::var lang = apvts.state.getProperty ("uiLanguage");
if (! lang.isVoid())
    uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

// Idiom 2 — XML root attribute (O-Octagon already uses this)
xml->setAttribute ("uiLanguage", languageCode (uiLanguage.load (std::memory_order_acquire)));
// ...restore:
if (xml->hasAttribute ("uiLanguage"))
    uiLanguage.store (languageIndex (xml->getStringAttribute ("uiLanguage")), std::memory_order_release);
```

`isVoid()` is the ONLY correct guard and `.toString()` the only correct read — the XML
round-trip rebuilds every property as a `var` over the attribute STRING, so a type predicate is
false for every saved session. **If `getStateInformation` delegates to
`presetManager.getStateAsXml()`, the `setProperty` must happen BEFORE that delegation**, or use
idiom 2 on the returned XML.

```cpp
// PluginEditor.cpp — a plain withNativeFunction pair, no relay
options = options.withNativeFunction ("getUiLanguage",
    [this] (auto&, auto complete)
    { complete (juce::var (Processor::languageCode (processorRef.uiLanguage.load (std::memory_order_acquire)))); });

options = options.withNativeFunction ("setUiLanguage",
    [this] (auto& args, auto complete)
    {
        if (args.size() > 0)
            processorRef.uiLanguage.store (Processor::languageIndex (args[0].toString()),
                                           std::memory_order_release);
        complete (juce::var (Processor::languageCode (processorRef.uiLanguage.load (std::memory_order_acquire))));
    });
```

### 5. The four-place rule

Every new `i18n.js` lands in **all four** places in ONE commit:
`juce_add_binary_data SOURCES` → a `getResource()` branch → the `import` in `app.js` → the file
itself. Miss any and O-Octagon §21 fails set-equality (by design), or the page 404s at runtime
on the other 42.
</canonical_contract>

<staging>
Seven stages. Every stage is independently shippable, because every commit is path-scoped to a
single plugin.

| Stage | Task | Scope | Rough size |
|---|---|---|---|
| A | T1, T2 | Two repo tools; no plugin touched | small |
| B | T3 | **Pattern-bearer: O-MultiBandCompressor** | medium |
| C | T4 | **Gate-hardening: O-Octagon** | medium |
| D | T5 | Clamp trio: O-ReverseDelay, O-Bitrot, O-Tapestop | medium-large |
| E | T6 | The remaining 9 `data-tip` plugins | large |
| F | T7 | Port the 7 `data-tooltip` plugins | large |
| G | T8 | The 22 bare plugins — English authoring + French | very large |

**Why O-MultiBandCompressor is the pattern-bearer, not O-Octagon:**

1. Its `applyTooltip()` (`app.js:1379-1391`) is *already* the key-indirection design. The change
   is literally "replace the two string arguments with one key" — the smallest possible diff
   that proves the whole contract.
2. It is the only plugin with **composed** tips (`applyBandTooltips` at `:1367-1368` builds
   `` `${bandName} — ${title}` `` across four bands). That is the hardest table shape and it
   gates O-FreqPulse in Stage F. Proving parameterised entries here retires the risk early.
3. It already has the `setTooltipsEnabled` bridge, so the "toggle moves into the popover"
   consolidation gets exercised on the first plugin, not the twentieth.
4. Its foot-of-file `initializeDeferredUI()` is where the TDZ lesson was learned (v1.4.0). The
   init-ordering precedent lands where the precedent came from.
5. **It has `tests/` but no tooltip gate.** So Stage B proves *the pattern* without
   simultaneously proving *a gate rewrite*. O-Octagon does the opposite in Stage C, under the
   strictest static gate in the repo (43 sections, a native-fn count, a stub whitelist
   set-equality and a CMake SOURCES set-equality). Decoupling those two proofs is the point of
   the ordering.

**Safe stop points, in order of attractiveness:**

- **End of Stage D — the recommended partial ship.** 5 plugins localized, the pattern proven
  under the strictest static gates, the ui-stub/CMake/getResource loop closed, and the French
  geometry claim *measured* rather than assumed. Everything after D is replication.
- **End of Stage F — the natural feature boundary.** ONE tooltip convention repo-wide, all ~690
  existing tips localized across 21 plugins. The 22 bare plugins keep their status quo, which is
  not a regression — they have no tooltip copy today either.
- **End of Stage B or C.** Fine, but the French geometry risk is still open.
- **Anywhere inside Stage E, F or G.** Each plugin is one path-scoped commit; stopping between
  plugins leaves the repo consistent.

Stage G is pure content work and may be stopped after any batch.
</staging>

<tasks>

<task type="tracer" tdd="false">
  <name>T1 (Stage A): Runtime parameter-dump tool</name>
  <files>scripts/param-dump/main.cpp, scripts/param-dump/ParamDump.cmake, scripts/param-dump/README.md</files>
  <read_first>plugins/O-Bitrot/tests/render-harness/CMakeLists.txt (the console-app-against-the-processor pattern), plugins/O-Bitrot/CMakeLists.txt:123-128 (the OUARICON_BUILD_TESTS opt-in)</read_first>
  <action>
Build the tool Stage G depends on. Static parsing of `createParameterLayout()` provably fails
in this repo — O-Emulator produces four of its six parameters from a local `percent()` factory
lambda (`PluginProcessor.cpp:63-74`) and O-Prism concatenates all 81 of its IDs
(`PluginProcessor.cpp:76-87`), so the literal ID never appears in source. Only a runtime walk
of `getParameters()` sees them.

`main.cpp` (~40 lines): construct the processor, iterate `getParameters()`, and emit one CSV or
TSV row per parameter with `getParameterID()`, `getName(128)`, `getLabel()`, `getNumSteps()`,
`getText(0.f, 64)`, `getText(1.f, 64)`, `getDefaultValue()`. `getLabel()` is what carries the
unit where one was given. Write to stdout; the caller redirects.

`ParamDump.cmake` factors the ~100 lines of boilerplate in the Bitrot render-harness
CMakeLists into one function, `ouaricon_add_param_dump(<PluginTarget> <PluginSourceDir>)`, so
the per-plugin cost is three lines. Every `JucePlugin_*` value must be DERIVED from the plugin
target's own properties that `juce_add_plugin` sets — `JUCE_VERSION`, `JUCE_PRODUCT_NAME`,
`JUCE_PLUGIN_CODE`, `JUCE_MANUFACTURER_CODE` — never mirrored as a literal, because a mirrored
fixture constant in this repo has drifted silently twice. Fail the configure with
`FATAL_ERROR` if any property reads empty rather than stamping a guess.

Compile with `JUCE_WEB_BROWSER=0` and do NOT list the editor TU, matching the render-harness
rule that keeps a WebView swap from breaking the target.

Gate the target behind the existing `OUARICON_BUILD_TESTS` cache option so a normal build is
unaffected.
  </action>
  <verify>
    <automated>cmake --build build --target O-Emulator-param-dump O-Prism-param-dump 2>&amp;1 | tail -3; ./build/.../O-Emulator-param-dump | grep -vc '^#' ; ./build/.../O-Prism-param-dump | grep -vc '^#'</automated>
  </verify>
  <done>
O-Emulator dumps **6** parameter rows (a regex over its source finds 2 — this is the negative
control that proves the runtime walk beats static parsing). O-Prism dumps **81** rows with
literal IDs that appear nowhere in `PluginProcessor.cpp`. A build with `OUARICON_BUILD_TESTS`
unset produces no param-dump target.
  </done>
  <precondition>CMake configure with `-DOUARICON_BUILD_TESTS=ON` succeeds for O-Emulator and O-Prism before the dump targets can be built.</precondition>
</task>

<task type="auto" tdd="false">
  <name>T2 (Stage A): The i18n canon + repo-level consistency gate</name>
  <files>scripts/i18n-canon.js, scripts/check-i18n.js</files>
  <action>
CONTEXT accepts 43 hand-copies of the runtime as a deliberate cost. Make that cost
*detectable* — this script is the only mitigation available under the no-shared-module rule.

`scripts/i18n-canon.js` holds §2 of `<canonical_contract>` verbatim as the reference text,
exported as a string. It is data, not an import target.

`scripts/check-i18n.js` walks every plugin that has an `i18n.js` under either UI root and
asserts, per plugin (`check(cond, msg)` style, exit code = failure count, matching the existing
gates):

1. `en` and `fr` key sets in `I18N` are identical.
2. Every key referenced in `TIP_BINDINGS` exists in `I18N`.
3. The plugin's `index.html`, with comments stripped, contains **zero** `data-tip=`,
   `data-tip-title=` or `data-tooltip=` literals — copy has fully left the markup.
4. No French entry is a straight passthrough: `fr.t === en.t && fr.b === en.b` fails unless the
   entry carries an explicit `sameAsEn: true` (some terms genuinely do not translate).
5. Every `fr` entry carries an explicit boolean `reviewed`.
6. **Drift gate.** The `applyI18n` + `initI18n` region of `app.js`, whitespace-normalised and
   comment-stripped, equals the canon string. This is the assertion that makes 43 copies safe.
7. `i18n.js` has no top-level statement outside `export` declarations.
8. `i18n.js` appears in the plugin's `juce_add_binary_data SOURCES` **and** in a `getResource()`
   branch. This generalises O-Octagon's §21 to all 43 and catches the highest-frequency mistake
   in this task.
9. Strip comments, then assert `i18n.js` contains no `innerHTML` reference and no `<` inside any
   string literal — machine-drafted French must not open a markup path.

Report unreviewed-French counts per plugin as a summary line so a native speaker has a worklist.
Add a `--plugin <Name>` filter so per-plugin commits can run it scoped.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js; echo "exit=$?"</automated>
  </verify>
  <done>
On a repo with no `i18n.js` anywhere the script exits 0 with "0 plugins localized". Deliberately
corrupting a fixture — deleting one `fr` key, flipping one `applyI18n` line, removing `i18n.js`
from a SOURCES block — makes it exit non-zero with the specific failing assertion named. A gate
that has never been seen to fail proves nothing, so run all three negative controls.
  </done>
</task>

<task type="auto" tdd="false">
  <name>T3 (Stage B): Pattern-bearer — O-MultiBandCompressor</name>
  <files>plugins/O-MultiBandCompressor/Source/ui/public/js/i18n.js, .../js/app.js, .../index.html, .../css/styles.css, plugins/O-MultiBandCompressor/Source/PluginProcessor.{h,cpp}, .../PluginEditor.cpp, plugins/O-MultiBandCompressor/CMakeLists.txt, plugins/O-MultiBandCompressor/CHANGELOG.md</files>
  <read_first>plugins/O-MultiBandCompressor/Source/ui/public/js/app.js:1165-1500 (tables, applyTooltip, applyBandTooltips, the renderer) and its foot-of-file initializeDeferredUI()</read_first>
  <action>
Implement `<canonical_contract>` end to end on one plugin, wired through every layer: table →
`applyI18n` → renderer → gear popover → native pair → APVTS persistence.

1. **Pre-flight:** `grep -rn 'setVisible' plugins/O-MultiBandCompressor/Source/`. The
   hidden-completion drop fires only if the `WebBrowserComponent` itself is hidden. If any hit
   targets the web view, stop and report rather than adding the pull.
2. **Table.** Create `js/i18n.js`. Move `GLOBAL_TOOLTIPS` (`:1173-1213`) and `BAND_TOOLTIPS`
   (`:1219-1256`) copy into `I18N` **verbatim** — this task moves English, it does not rewrite
   it. `GLOBAL_TOOLTIPS` tuples become `TIP_BINDINGS` entries `[selector, key, wrapper?]`.
3. **Band composition.** `applyBandTooltips` composes `` `${bandName} — ${title}` `` against
   `BAND_LABELS`. Preserve that: the four band display names become their own `I18N` keys, the
   per-band tip keys become parameterised entries carrying a `{band}` placeholder, and the
   expansion passes `vars: { band: tr(bandKey, lang).t }`. The joining em-dash stays a literal.
   This is the shape O-FreqPulse needs in Stage F — get it right here.
4. **Runtime.** Replace `applyTooltip`'s two string parameters with one key. Paste the canonical
   `applyI18n`/`initI18n` block, declared above every reader. Call `initI18n()` from
   `initializeDeferredUI()` inside its own `try/catch` — a translation-table typo must not take
   `initializeCrossoverDrag()` down, which is exactly what happened in v1.4.0.
5. **Popover.** Add the gear markup. Move the existing hover-help toggle into it — moved, not
   duplicated. Style it inside MBC's own visual system.
6. **C++.** Add `uiLanguage` + the codec pair to the processor, the persistence lines to
   `get/setStateInformation` mirroring the idiom already used for `tooltipsEnabled`, and the
   `getUiLanguage`/`setUiLanguage` pair to the editor.
7. **CMake + getResource.** Add `js/i18n.js` to `SOURCES` and a `getResource()` branch, in this
   same commit.
8. Bump the version (read where MBC declares it) and add a CHANGELOG entry in this repo's
   format. Update the PLUGINS.md row.
9. Build with `./scripts/build-and-install.sh O-MultiBandCompressor`, which performs the
   AU-cache clear and the `-dev` ↔ unsuffixed dual-variant sweep. Note the script does not
   rebuild the Standalone `.app`, so do not verify a UI change in a stale Standalone.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --plugin O-MultiBandCompressor &amp;&amp; ./scripts/build-and-install.sh O-MultiBandCompressor</automated>
    <human-check>In the DAW: hover a band control (English body), open the gear, pick Français — the same tooltip re-renders in French with the band name composed correctly, with no reload. Save the session, close the DAW, reopen: still French. Load a factory preset: still French.</human-check>
  </verify>
  <done>
`grep -v '^\s*//' index.html | grep -c 'data-tip'` returns 0. `check-i18n.js --plugin
O-MultiBandCompressor` exits 0. Both band-composed and flat tips localize. The language survives
a session round-trip and is unchanged by a preset load. A fresh instance with no saved state
shows English immediately, with no blank frame and no flash of French.
  </done>
</task>

<task type="checkpoint:decision">
  <name>Checkpoint 1: Review the pattern before it is replicated 42 times</name>
  <action>
Stop. Show the developer the O-MultiBandCompressor diff — specifically `js/i18n.js`, the
`applyI18n` block, and the gear popover markup and styling.

This shape gets hand-copied into 42 more plugins and CONTEXT has accepted that there is no
shared module to fix it centrally afterwards. A change to the table shape or the popover
structure costs one plugin now and 43 later.

Ask specifically about: the `{token}` placeholder form for parameterised entries, the
`reviewed: false` flag placement, and whether the gear popover's geometry and open/close
interaction read correctly inside MBC's aesthetic.
  </action>
  <done>Developer has approved the contract, or named the changes to make before Stage C.</done>
</task>

<task type="auto" tdd="false">
  <name>T4 (Stage C): Gate-hardening — O-Octagon</name>
  <files>plugins/O-Octagon/Source/ui/public/js/{i18n.js,app.js}, .../index.html, .../css/styles.css, plugins/O-Octagon/Source/PluginProcessor.cpp, .../PluginEditor.cpp, plugins/O-Octagon/CMakeLists.txt, plugins/O-Octagon/tests/ui_frontend_check.js, plugins/O-Octagon/tests/ui-stub/juce-stub.js, plugins/O-Octagon/CHANGELOG.md</files>
  <read_first>plugins/O-Octagon/tests/ui_frontend_check.js:169-191 (§2 init ordering), :195-245 (§3 native-fn census), :861-890 (§21 module/SOURCES set-equality); plugins/O-Octagon/Source/PluginProcessor.cpp:867-901 (the XML-root idiom this plugin already uses)</read_first>
  <action>
Same contract as T3, under the strictest static gate in the repo. Everything here lands in ONE
commit — source, CMake, `getResource()`, gate rewrites and stub, together — because §21 and the
native-fn census will otherwise fail on a partial commit, which is the behaviour they exist for.

1. `grep -rn 'setVisible' plugins/O-Octagon/Source/` — verified absent this session for the web
   view, re-confirm before wiring the pull.
2. **§2 constraint is binding.** `init();` must remain the literal last statement of `app.js`
   with no module-level declaration after it. So `initI18n()` is called from **inside**
   `init()`, and the `import` from `./i18n.js` (hoisted) is the only new top-level form.
3. **Persistence uses idiom 2** — the XML root attribute — because `PluginProcessor.cpp:867-901`
   already carries `tooltipsEnabled` that way. Do not introduce the ValueTree-property form
   alongside it.
4. **Gate §3 (`:235`):** the count moves `23` → `25`. Update the literal AND extend the running
   comment block above it with the same one-line justification style it already uses for every
   prior bump — a count assertion is only worth anything because it moves and fails loudly.
5. **Gate §3 (`:239`) `setsEqual(stubbed, registered)`:** teach `tests/ui-stub/juce-stub.js` the
   two new names. `getUiLanguage` resolves `'en'`; `setUiLanguage` echoes its argument. Leave the
   unknown-name rejection path intact.
6. **Gate §21 / §9:** adding `js/i18n.js` to `Source/ui/public/js/` is exactly what those
   sections derive from. Add it to `juce_add_binary_data(OuariconOctagon_UIResources SOURCES ...)`
   and a `getResource()` branch so set-equality holds. Note the SOURCES list has a `NAMESPACE`
   and it is not optional.
7. **Gate §6** asserts HTML-authored labels are never written via `textContent`. The popover's
   `<option>` captions and the `data-label` spans must satisfy it — this is the same class of bug
   that erased authored captions before.
8. Do **not** touch `.tooltip { max-width }` — O-Octagon's cap is 240px
   (`css/styles.css:1302`), differing from the 230 the clamp gates mirror.
9. Version bump, CHANGELOG, PLUGINS.md, then `./scripts/build-and-install.sh O-Octagon`.
  </action>
  <verify>
    <automated>node plugins/O-Octagon/tests/ui_frontend_check.js &amp;&amp; node plugins/O-Octagon/tests/ui_layout_check.js &amp;&amp; node scripts/check-i18n.js --plugin O-Octagon &amp;&amp; ./scripts/build-and-install.sh O-Octagon</automated>
  </verify>
  <done>
All 43 sections of `ui_frontend_check.js` pass, including §2 (init is still the last statement),
§3 at 25 registered functions with the stub whitelist equal to the C++ surface, §9 and §21 with
`js/i18n.js` in the derived set and in SOURCES, and §6. `ui_layout_check.js` passes — the gear
button did not disturb the layout it asserts. Language switches and persists in the DAW.
  </done>
</task>

<task type="auto" tdd="false">
  <name>T5 (Stage D): Clamp trio + gate rewrites — O-ReverseDelay, O-Bitrot, O-Tapestop</name>
  <files>plugins/O-{ReverseDelay,Bitrot,Tapestop}/Source/ui/public/js/{i18n.js,app.js}, .../index.html, .../css/styles.css, plugins/O-{ReverseDelay,Bitrot,Tapestop}/Source/Plugin{Processor,Editor}.{h,cpp}, plugins/O-{ReverseDelay,Bitrot,Tapestop}/CMakeLists.txt, plugins/O-{ReverseDelay,Bitrot,Tapestop}/tests/ui_tooltip_clamp_check.js, plugins/O-{ReverseDelay,Bitrot,Tapestop}/tests/ui-stub/juce-stub.js, plugins/O-ReverseDelay/tests/ui_frontend_check.js, plugins/O-{ReverseDelay,Bitrot,Tapestop}/CHANGELOG.md</files>
  <read_first>plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js:100-190 (mirrored constants + the whole-run assertions) and :320-385 (the four per-anchor assertions); plugins/O-ReverseDelay/tests/ui_frontend_check.js:661-700 and :755-765</read_first>
  <action>
This is where the French geometry claim gets measured. Do the three plugins one at a time, one
path-scoped commit each, in the order O-ReverseDelay → O-Bitrot → O-Tapestop.

**Per plugin, the contract from T3/T4 plus:**

**(a) Rewrite the clamp gate to sweep both languages in one process.** Do not duplicate the
file. Wrap the existing anchor sweep in `for (const lang of ['en', 'fr'])`, setting the language
before each sweep with `await page.evaluate((l) => window.__setLanguage(l), lang)`. Assertion
counts double. Label each failure with its language or a French-only failure reads as a
mysterious regression.

Keep every whole-run assertion: the viewport really is `SHIP_W x SHIP_H`, every anchor was
measured, and **the clamp engaged at least once** — now required per language.

**(b) Read `.tooltip { max-width }`, do not assume it.** Replace the literal `NATURAL_MAX_W =
230` with a value parsed from that plugin's own CSS, keeping the existing regex cross-check as
the drift guard. All three happen to be 230 today; O-Octagon is 240, and the gate must not
mis-assert if it is ever pointed elsewhere.

**(c) Do NOT change `.tooltip { max-width }`.** Raising the cap makes tips wider, which pushes
the horizontal assertion toward failure while barely helping the vertical one — and removing a
width cap in this repo has previously removed the alignment the cap was silently providing. Let
French wrap taller inside the existing cap; the flip logic already handles taller tips and the
vertical assertion is what proves it.

**(d) Re-run the sweep AFTER the gear button lands, not before.** Widening a control moved the
right-most tip toward the edge and was the exact trigger for the shrink-to-fit bug. The gear
button is a new control near an edge in all three.

**(e) Teach each `tests/ui-stub/juce-stub.js` the two new names** (all three plugins have one).

**(f) O-ReverseDelay only — two `ui_frontend_check.js` edits:**
   - `:190` bridge count `13` → `15`, with the comment extended in the file's own style.
   - `:691-696` currently requires all 14 `TIP_ANCHORS` to carry the copy **in index.html**,
     which fails by construction. Rewrite it into the stronger form: every anchor id is a key in
     `i18n.js` **and** has both an `en` and an `fr` entry. That catches a missing translation,
     which the current assertion cannot. Record the rewrite in the section's comment.
   - `:760-761` **stays untouched.** D13 forbids `setTooltipsEnabled` by name; O-ReverseDelay
     gets the language pair only and its popover carries the language selector alone, so the
     assertion stays green. Its bridge goes 13 → 15, which D13 did not speak to. Do not add a
     hover-help row to this plugin's popover.

**(g) O-Bitrot and O-Tapestop** both already have the `setTooltipsEnabled` bridge, so their
existing toggle moves into the popover.
  </action>
  <verify>
    <automated>for P in O-ReverseDelay O-Bitrot O-Tapestop; do node plugins/$P/tests/ui_tooltip_clamp_check.js || echo "FAIL $P"; done; node plugins/O-ReverseDelay/tests/ui_frontend_check.js; node scripts/check-i18n.js</automated>
  </verify>
  <done>
All three clamp gates exit **0** — not 77, which means Playwright was unresolvable and nothing
was verified — with the sweep run for both `en` and `fr`, and with the clamp proven to engage at
least once in each language. O-ReverseDelay's frontend gate passes with the bridge at 15, the
rewritten anchor-coverage assertion, and the D13 assertion still green. Every anchor of all three
plugins renders fully inside the shipping viewport in French.
  </done>
  <reversibility rating="costly">Rewriting the three clamp gates and O-ReverseDelay's anchor assertion is the point of no return for the old markup convention — after this the gates no longer describe a shape any plugin has.</reversibility>
</task>

<task type="checkpoint:human-verify">
  <name>Checkpoint 2: French geometry verdict</name>
  <action>
Stop and report the measured result before replicating across 40 plugins.

Report: how many anchors changed clamp behaviour between `en` and `fr`, how many tips gained a
line, and whether any anchor needed the vertical flip in French that did not need it in English.

If any assertion failed in French, present the options rather than picking one — shortening the
French copy for that anchor, or adjusting that plugin's flip threshold — because shortening copy
is a content decision and adjusting geometry changes a shipped visual.

If all three passed clean, say so plainly and note that **this is the recommended partial-ship
point**: 5 plugins localized, the pattern proven under the strictest gates, and the geometry risk
retired.
  </action>
  <done>Developer has seen the measured French geometry result and either confirmed continuation to Stage E or chosen to ship here.</done>
</task>

<task type="auto" tdd="false">
  <name>T6 (Stage E): Remaining 9 data-tip plugins</name>
  <files>plugins/O-{Contrabass,simpleGrain,simpleSampler,simpleSubtractive,Orbit,simpleAdditive,simpleFM,simplePhysicalModelSynth,simpleBeatmaker}/** (per-plugin, one commit each)</files>
  <action>
Mechanical replication. The English copy already exists in these plugins' markup and the renderer
already reads `data-tip`/`data-tip-title` — this stage moves copy into `i18n.js`, adds the gear
popover, the bridge pair and the persistence, and drafts French.

**Enumerated batches. One path-scoped commit per plugin.**

*Batch E1 — the one with a gate (do first, alone):*
| Plugin | UI root | Tips | Note |
|---|---|---|---|
| O-Contrabass | `Source/ui/public` | 44 | `tests/ui_frontend_check.js:123` asserts `called.size === 34 && registered.size === 34` → **36**. Has `setTooltipsEnabled`; the toggle moves into the popover. |

*Batch E2 — the `Resources/ui` one:*
| Plugin | UI root | Tips | Note |
|---|---|---|---|
| O-Orbit | **`Resources/ui`** | 32 | Serves `Resources/ui/` per `CMakeLists.txt:54-62` — its `Source/ui/public/` is NOT embedded. Has `setTooltipsEnabled`. Note the `configure_file` vendored preset-manager copy already writes into `Resources/ui/js/modules/`. |

*Batch E3 — the whole `simple*` family, seven plugins, no tooltip gates:*
| Plugin | UI root | Tips |
|---|---|---|
| O-simpleGrain | `Source/ui/public` | 34 |
| O-simpleSampler | `Source/ui/public` | 34 |
| O-simpleSubtractive | `Source/ui/public` | 33 |
| O-simpleFM | `Source/ui/public` | 25 |
| O-simpleAdditive | `Source/ui/public` | 24 |
| O-simplePhysicalModelSynth | `Source/ui/public` | 24 |
| O-simpleBeatmaker | `Source/ui/public` | 20 |

None of the seven has `setTooltipsEnabled`, so none gets a hover-help row — language selector
only. All seven have a `tests/render-harness/` target; none has a tooltip gate.

**Per plugin, in this order:**
1. `grep -rn 'setVisible' plugins/<Name>/Source/` — abort and report if it targets the web view.
2. `grep -n -i version plugins/<Name>/CMakeLists.txt` — several of these declare the version via
   `set(<PLUGIN>_VERSION "x.y.z")` consumed by both the plugin and its render harness. Bump the
   variable, not a second copy.
3. Move every `data-tip`/`data-tip-title` pair into `I18N` **verbatim**, keyed on the anchor id;
   `TIP_BINDINGS` entries are `['#<id>', '<id>']`.
4. Paste the canonical `applyI18n`/`initI18n` block; call it from the plugin's existing deferred
   init inside `try/catch`. If the plugin uses O-ReverseDelay's single-bottom-`init()` idiom, call
   from inside `init()`.
5. Gear popover, styled inside that plugin's own visual system.
6. C++ pair + persistence, mirroring the plugin's existing `tooltipsEnabled` idiom where it has
   one, otherwise the ValueTree-property idiom.
7. `i18n.js` into `SOURCES` + `getResource()` + the `import`, same commit.
8. Draft French with `reviewed: false`.
9. Version bump, CHANGELOG, PLUGINS.md row.
10. `./scripts/build-and-install.sh <Name>`, then `node scripts/check-i18n.js --plugin <Name>`.
11. Re-check `git branch --show-current` and `git status --short` **immediately** before
    committing — another session shares this index — then `git commit -- plugins/<Name> PLUGINS.md`.
    Never `git add -A`, never `git commit -a`.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js &amp;&amp; node plugins/O-Contrabass/tests/ui_frontend_check.js</automated>
    <human-check>Spot-check three plugins across the three batches in the DAW: switch to Français, confirm every hovered tip is French, reopen the session and confirm the choice held.</human-check>
  </verify>
  <done>
All 9 plugins pass `check-i18n.js`. O-Contrabass's frontend gate passes at a bridge surface of 36.
No plugin's `index.html` retains a `data-tip` literal. All 14 `data-tip`-convention plugins
(these 9 plus MBC, O-Octagon and the Stage D trio) are localized.
  </done>
</task>

<task type="auto" tdd="false">
  <name>T7 (Stage F): Port the 7 data-tooltip plugins onto the data-tip renderer</name>
  <files>plugins/O-{Polystutter,Lyrica,IntonationPad,SpectralShaper,Gain,Marimba,FreqPulse}/** (per-plugin, one commit each)</files>
  <read_first>plugins/O-ReverseDelay/Source/ui/public/js/app.js:913-1028 (the renderer being ported IN); plugins/O-Polystutter/Source/ui/public/index.html:808-895 and :1670-1757 (the renderer being ported OUT)</read_first>
  <action>
These 7 are **ported**, not localized in place. Their current positioner never measures —
`const tooltipHeight = 60; // Approximate`, `const tooltipWidth = 220`, viewport literals `660`
and `1000` (`O-Polystutter/index.html:1717-1733`) — so it is already wrong before French makes
strings taller. After this stage there is ONE renderer and ONE convention repo-wide.

**Enumerated, one path-scoped commit per plugin, hardest first so the shape is proven early:**

| # | Plugin | UI root | Tips | Port note |
|---|---|---|---|---|
| 1 | O-FreqPulse | **`Resources/ui`** | 12 | **Do first.** Synthesises `data-tooltip` at runtime with interpolated band names (`js/app.js:335-531`), so it needs the parameterised `{token}` entries proven on MBC in Stage B. Has `setTooltipsEnabled`. |
| 2 | O-Polystutter | `Source/ui/public` | 102 | The largest port and the source of the hard-coded literals. Renderer is inline in `index.html`; extract it to `js/app.js` alongside the port. `.tooltip max-width` here is **220**, not 230 — read it. |
| 3 | O-SpectralShaper | **`Resources/ui`** | 25 | Already measures (`js/app.js:634-640`) but positions against `#app`, not the viewport — the port changes the reference frame. Has `setTooltipsEnabled` and a `tests/ui-stub/juce-stub.js` that needs the two new names. |
| 4 | O-Lyrica | **`Resources/ui`** | 43 | Has `setTooltipsEnabled`. **`CMakeLists.txt:88-96` has NO `NAMESPACE`** on its binary-data target — it relies on being the only one. Add `i18n.js` to the existing target; do not add a second target. |
| 5 | O-IntonationPad | `Source/ui/public` | 37 | No toggle bridge; language selector only. |
| 6 | O-Gain | `Source/ui/public` | 23 | No toggle bridge. |
| 7 | O-Marimba | `Source/ui/public` | 15 | Has `setTooltipsEnabled`. |

**Per plugin:**
1. **Split the existing copy.** These strings are authored `"Label: sentence."`, so split on the
   **first** `": "` to recover `{t, b}`. Expect a handful that do not split cleanly — hand-split
   those and list them in the commit message; do not silently drop the colon into the body.
2. **Port the renderer.** Copy the measure-then-pin runtime from
   `O-ReverseDelay/js/app.js:913-1028`: `position: fixed`, width released → measured → **pinned**
   before `left` is applied, vertical flip, horizontal clamp, `--arrow-x` recomputed after the
   clamp, `TOOLTIP_DELAY_MS` dwell, `tipAllowed()` gating. Delete the old positioner and its
   `60`/`220`/`660`/`1000` literals entirely — leaving both is how two renderers came to exist.
3. **Port the CSS.** `.tooltip` becomes `position: fixed`. Carry over each plugin's own
   `max-width` value unchanged; do not normalise them to one number.
4. Then the standard contract: `i18n.js`, `applyI18n`, gear popover, C++ pair, persistence,
   SOURCES + `getResource()` + `import` in one commit, French draft, version bump, CHANGELOG,
   PLUGINS.md, build-and-install, `check-i18n.js --plugin`, re-check branch/status, path-scoped
   commit.
5. **Verify the port by rendering**, not by inspection — serve the page against the ui-stub and
   hover. A stub server on an already-taken port will silently serve another session's files, so
   pick an unused port and confirm the served page is this plugin's.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js &amp;&amp; grep -rn 'tooltipHeight\s*=\s*60\|tooltipWidth\s*=\s*220' plugins/O-*/Source/ui/public plugins/O-*/Resources/ui | grep -v '^\s*//' | wc -l</automated>
    <human-check>For each of the 7: hover a long tip near the right edge and near the top edge, in both English and French, and confirm the tip stays fully on screen and the arrow still points at its anchor.</human-check>
  </verify>
  <done>
The hard-coded-literal grep returns **0**. `grep -rl 'data-tooltip' plugins/*/Source/ui
plugins/*/Resources/ui` returns nothing. All 7 render through the measure-then-pin runtime.
`check-i18n.js` passes for all 21 localized plugins. **One tooltip convention repo-wide — this is
the natural feature boundary and the second recommended stop point.**
  </done>
  <reversibility rating="costly">Deleting the second renderer is deliberate and one-directional — the 7 plugins' old positioners are gone, not disabled.</reversibility>
</task>

<task type="auto" tdd="false">
  <name>T8 (Stage G): The 22 bare plugins — author English, then French</name>
  <files>plugins/O-{AnalogEQ,AnalogSaturation,Bass,Bassoon,Bells,Bowed,Chorus,Comp,Detune,DigiDelay,Emulator,Formant,Freeze,GrainScatter,MicrotonalSampler,Prism,Reed,SimpleReverb,Texture,TextureForge,Tremolo,Wind}/** (per-plugin, one commit each)</files>
  <action>
Content work, not engineering. These plugins have no tooltip copy today — only stray native
`title=` attributes — so this stage **authors** English hover-help for ~520 parameters, then
localizes it. Nothing here can regress an existing tooltip, because there are none.

**Per plugin, the skeleton is machine-generated and the prose is hand-written:**
1. Build and run the T1 param-dump: `ouaricon_add_param_dump(<Target> <SourceDir>)`, build,
   redirect to `plugins/<Name>/.planning/params.tsv`. This gives id, display name, unit, step
   count, min/max display text and default — authoritative even where the IDs are concatenated
   or the parameters come from a factory lambda.
2. Generate one `I18N` entry per parameter from the dump: `t` = the display name, `b` =
   hand-written prose saying what the control does and when to reach for it, ending with the
   range and unit from the dump. Where `getLabel()` is empty the range must be phrased from the
   UI's own formatter, not invented — read how `app.js` renders that readout.
3. Bind `TIP_BINDINGS` to the DOM ids the UI already uses for those parameters. Delete the stray
   native `title=` attributes they replace, so there is no double tooltip.
4. Then the standard contract, identical to Stage E step 4 onward.

**Enumerated batches, ordered so the cheap ones land first and the volume is isolated:**

*Batch G1 — small, `Source/ui/public`, ~7-18 params each (11 plugins):*
O-AnalogSaturation (4), O-Bass (5), O-Comp (7), O-Tremolo (7), O-Chorus (8), O-DigiDelay (8),
O-SimpleReverb (8), O-Texture (10), O-Freeze (12), O-TextureForge (12), O-AnalogEQ (16).
Note O-Chorus has **zero** unit strings on all 8 parameters — its ranges must be phrased from
the UI formatter.

*Batch G2 — small, `Resources/ui` (2 plugins):*
O-Bassoon (10), O-MicrotonalSampler (19). **O-MicrotonalSampler serves `Resources/ui/`** per
`CMakeLists.txt:80-99`; its `Source/ui/public/` is a build-time JS-copy staging dir and is NOT
embedded. Write only into `Resources/ui/`.

*Batch G3 — medium (4 plugins):*
O-Detune (18), O-Bowed (23), O-GrainScatter (36), O-Reed (35). O-Bowed has a `tests/` dir —
re-run its gates.

*Batch G4 — the volume, 51% of the total in four plugins (do one per session):*
O-Wind (56), O-Formant (64), O-Bells (65), O-Prism (81). O-Prism's 81 IDs are string-concatenated
and appear nowhere in source — the param dump is the only way to enumerate them.

*Batch G5 — the factory-lambda case:*
O-Emulator (6 real, 2 by regex). Has a `tests/` dir. The param dump is the only correct
inventory here.

**Discipline, every plugin, every commit:** `grep -rn 'setVisible' plugins/<Name>/Source/` before
wiring the pull; build via `./scripts/build-and-install.sh <Name>`; re-check
`git branch --show-current` and `git status --short` immediately before committing; then
`git commit -- plugins/<Name> PLUGINS.md`.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js; echo "--- localized plugin count ---"; ls plugins/*/Source/ui/public/js/i18n.js plugins/*/Resources/ui/js/i18n.js 2>/dev/null | wc -l</automated>
    <human-check>Per batch, open one plugin in the DAW: every knob and control has a tooltip, the copy is accurate about what the control does, the range matches what the readout shows, and Français switches all of it.</human-check>
  </verify>
  <done>
All 43 plugins have an `i18n.js`. `check-i18n.js` exits 0 across the repo and reports the
unreviewed-French count per plugin as a native-speaker worklist. No plugin retains a stray
`title=` attribute on a control that now has a `data-tip`.
  </done>
</task>

</tasks>

<threat_model>
No new external dependencies are introduced — no npm, pip or cargo install — so the package
legitimacy gate does not apply. Playwright is a pre-existing tool dependency and the gates
already resolve it themselves, exiting 77 when they cannot.

| Boundary | Description |
|---|---|
| i18n table → DOM | Machine-drafted French strings are written into the live page |
| WebView JS → C++ | `setUiLanguage` accepts an arbitrary string from the page |
| Session XML → C++ | `uiLanguage` is read back from a user-editable saved-session file |

| ID | Category | Component | Severity | Disposition | Mitigation |
|---|---|---|---|---|---|
| T-ieq-01 | Tampering | localized string → tooltip node | medium | mitigate | Copy stays `textContent` on both title and body; the renderer builds nodes with `createElement`. `check-i18n.js` assertion 9 rejects any `innerHTML` reference in `i18n.js` and any `<` inside a string literal. A line break uses `\n` + CSS `white-space: pre-line`, never a markup tag. |
| T-ieq-02 | Tampering | `setUiLanguage` argument | low | mitigate | The C++ side runs `languageIndex()`, which maps anything that is not `"fr"` to `0`; JS runs `LANGUAGES.includes(lang)` and falls back to `'en'`. No unvalidated string reaches storage. |
| T-ieq-03 | Tampering | `uiLanguage` restored from session XML | low | mitigate | Restore is guarded by `isVoid()`, read via `.toString()`, and passed through the same `languageIndex()` clamp. A hand-edited or corrupt value degrades to English. |
| T-ieq-04 | Denial of Service | module evaluation | high | mitigate | A malformed table entry must not take the UI down. `applyI18n` is called only from deferred init inside `try/catch`, `tr()` returns a fallback for a missing key rather than throwing, and `i18n.js` has no top-level statement — enforced by `check-i18n.js` assertion 7 and, on O-Octagon, by gate §2. |
</threat_model>

<verification>
Repo-wide, after any stage:

```bash
node scripts/check-i18n.js
node plugins/O-Octagon/tests/ui_frontend_check.js
node plugins/O-Octagon/tests/ui_layout_check.js
node plugins/O-ReverseDelay/tests/ui_frontend_check.js
node plugins/O-Contrabass/tests/ui_frontend_check.js
for P in O-ReverseDelay O-Bitrot O-Tapestop; do node plugins/$P/tests/ui_tooltip_clamp_check.js; done
```

Exit **77** from a clamp gate means Playwright was unresolvable and **nothing was verified** —
it is deliberately distinct from 0. Treat it as a failure to verify, not a pass.

After any merge that touched `PLUGINS.md`, run the union-merge duplicate check:
`grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` — empty output means clean.
</verification>

<success_criteria>
- All 43 plugins carry a `js/i18n.js` under their **actually served** UI root, present in
  `juce_add_binary_data SOURCES`, in a `getResource()` branch, and imported by `app.js`.
- Zero `data-tip=`, `data-tip-title=` or `data-tooltip=` literals remain in any `index.html`.
- Zero occurrences of the old positioner's `tooltipHeight = 60` / `tooltipWidth = 220` literals.
- `scripts/check-i18n.js` exits 0, including the byte-comparison drift gate against
  `scripts/i18n-canon.js`.
- The three clamp gates pass for **both** `en` and `fr`, with the clamp proven to engage at least
  once per language.
- O-Octagon (25), O-ReverseDelay (15) and O-Contrabass (36) frontend gates pass at their new
  bridge counts, with each ui-stub whitelist equal to its C++ surface.
- O-ReverseDelay's D13 assertion at `ui_frontend_check.js:760-761` is still green and untouched.
- Language persists across a session round-trip and is unchanged by a preset load, on every
  plugin.
- Every plugin touched has a version bump, a CHANGELOG entry and an updated PLUGINS.md row, in a
  single path-scoped commit.
</success_criteria>

<output>
Per-plugin commits only. After each stage, report which plugins shipped, at what versions, and
which gates were run with what exit codes.
</output>
