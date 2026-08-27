---
task: 260826-ieq-multi-language-tooltips-across-all-vst-p
type: execute
mode: quick
status: incomplete
stages_complete: [A, B, C, D, E]
stages_remaining: [F, G, H, I, J, K, L, M]
stopped_at: "End of Stage E (T6-T9). Four repo-level tools shipped, NO plugin touched. Stages F, G, H are approved but are separate dispatches; do NOT stop here for long — Stage E leaves canon v2 in the tree with zero plugins on it."
plugins_shipped:
  - name: O-MultiBandCompressor
    version: 1.10.0
    commit: b98aa9ec
  - name: O-Octagon
    version: 1.6.0
    commit: 8dcb1317
  - name: O-ReverseDelay
    version: 1.9.0
    commit: 951dd584
  - name: O-Bitrot
    version: 1.14.0
    commit: 1f3a9faa
  - name: O-Tapestop
    version: 1.5.0
    commit: 547f9738
subsystem: webview-ui / i18n
tags: [i18n, tooltips, webview, juce, apvts-persistence]
key-files:
  created:
    - scripts/serve-ui.js
    - scripts/boot-all-uis.js
    - scripts/ui-stub/generic-juce-stub.js
    - scripts/ui-stub/stub-preamble.js
    - scripts/ui-stub/README.md
    - scripts/i18n-extract.js
    - scripts/i18n-extract-README.md
    - scripts/check-ui-labels.js
    - scripts/check-ui-labels-README.md
    - plugins/O-Prism/tests/ui-stub/generic-overrides.json
    - plugins/O-MultiBandCompressor/Source/ui/public/js/i18n.js
    - plugins/O-Octagon/Source/ui/public/js/i18n.js
    - plugins/O-ReverseDelay/Source/ui/public/js/i18n.js
    - plugins/O-Bitrot/Source/ui/public/js/i18n.js
    - plugins/O-Tapestop/Source/ui/public/js/i18n.js
  modified:
    - plugins/O-MultiBandCompressor/Source/ui/public/js/app.js
    - plugins/O-MultiBandCompressor/Source/ui/public/index.html
    - plugins/O-MultiBandCompressor/Source/ui/public/css/styles.css
    - plugins/O-MultiBandCompressor/Source/PluginProcessor.h
    - plugins/O-MultiBandCompressor/Source/PluginProcessor.cpp
    - plugins/O-MultiBandCompressor/Source/PluginEditor.cpp
    - plugins/O-MultiBandCompressor/CMakeLists.txt
    - plugins/O-MultiBandCompressor/CHANGELOG.md
    - plugins/O-Octagon/Source/ui/public/js/app.js
    - plugins/O-Octagon/Source/ui/public/index.html
    - plugins/O-Octagon/Source/ui/public/css/styles.css
    - plugins/O-Octagon/Source/PluginProcessor.h
    - plugins/O-Octagon/Source/PluginProcessor.cpp
    - plugins/O-Octagon/Source/PluginEditor.cpp
    - plugins/O-Octagon/CMakeLists.txt
    - plugins/O-Octagon/CHANGELOG.md
    - plugins/O-Octagon/tests/ui_frontend_check.js
    - plugins/O-Octagon/tests/ui-stub/juce-stub.js
    - plugins/O-ReverseDelay/Source/ui/public/js/app.js
    - plugins/O-ReverseDelay/Source/ui/public/index.html
    - plugins/O-ReverseDelay/Source/ui/public/css/styles.css
    - plugins/O-ReverseDelay/Source/Plugin{Processor.h,Processor.cpp,Editor.cpp}
    - plugins/O-ReverseDelay/CMakeLists.txt
    - plugins/O-ReverseDelay/CHANGELOG.md
    - plugins/O-ReverseDelay/tests/ui_frontend_check.js
    - plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js
    - plugins/O-ReverseDelay/tests/ui-stub/juce-stub.js
    - plugins/O-Bitrot/Source/ui/public/index.html
    - plugins/O-Bitrot/Source/Plugin{Processor.h,Processor.cpp,Editor.cpp}
    - plugins/O-Bitrot/CMakeLists.txt
    - plugins/O-Bitrot/CHANGELOG.md
    - plugins/O-Bitrot/tests/ui_tooltip_clamp_check.js
    - plugins/O-Bitrot/tests/ui-stub/juce-stub.js
    - plugins/O-Tapestop/Source/ui/public/js/app.js
    - plugins/O-Tapestop/Source/ui/public/index.html
    - plugins/O-Tapestop/Source/ui/public/css/styles.css
    - plugins/O-Tapestop/Source/Plugin{Processor.h,Processor.cpp,Editor.cpp}
    - plugins/O-Tapestop/CMakeLists.txt
    - plugins/O-Tapestop/CHANGELOG.md
    - plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js
    - plugins/O-Tapestop/tests/ui-stub/juce-stub.js
    - scripts/check-i18n.js
    - scripts/i18n-canon.js
    - PLUGINS.md
actuals:
  # Stage D dispatch only (three plugins, tooltip-only). Kept as the n=1
  # calibration sample the plan's SIZING section reasons from.
  tokens: 232000
  tasks: 3
  commits: 5
actuals_stage_e:
  tokens: 285000
  tasks: 4
  commits: 4
---

# Quick Task 260826-ieq — Stage B Summary

Stage B (task T3) is done: **O-MultiBandCompressor v1.10.0** is the first
localized plugin in the suite, shipped in one path-scoped commit `b98aa9ec`
containing only `plugins/O-MultiBandCompressor/**` (9 files).

Stages C–G remain. Stopped at **Checkpoint 1** as planned.

---

## Checkpoint 1 — what to look at

This shape gets hand-copied into 42 more plugins, and CONTEXT.md has accepted
that there is no shared module to fix it centrally afterwards. A change to the
table shape or the popover structure costs one plugin now and 43 later.

**Open the plugin.** `O-MultiBandCompressor-dev` is installed as VST3 and AU,
version 1.10.0. Note that `build-and-install.sh` does **not** rebuild the
Standalone `.app` — that binary is stale, so use a DAW or rebuild the Standalone
target explicitly.

### 1. The gear popover, top right

Click the ⚙ where the v1.4.1 `?` button used to be. Judge:

- **Geometry.** 190 px wide, drops 7 px below the gear, right-aligned to the
  header edge. Two rows: Language, Hover help.
- **Open/close interaction.** Click the gear to toggle; click anywhere else, or
  press Escape, to dismiss. There is no animation — is that right for this
  plugin's feel?
- **Aesthetic fit.** It reuses MBC's own tokens (parchment gradient, `#5C4033`
  border, olive `#8BA870` for the engaged state, Garamond, 3 px radius) and the
  gear keeps the old `?` button's exact circle geometry so the header silhouette
  is unchanged. **This is the widget structure the other 42 plugins inherit** —
  only the styling is meant to vary per plugin.
- **The Hover help toggle.** It MOVED here from the header; it is not
  duplicated. It reads On / Off with `aria-pressed`. `On`/`Off` stay English:
  CONTEXT.md puts non-tooltip chrome out of scope. **Confirm that is what you
  want** — if the caption should localize, say so now, not at plugin 30.

### 2. The `{token}` placeholder form — the thing that most needs your eye

Open `plugins/O-MultiBandCompressor/Source/ui/public/js/i18n.js`.

Per-band entries carry a `{band}` placeholder:

```js
'band.threshold': {
    en: { t: '{band} — Threshold', b: '…' },
    fr: { t: '{band} — Seuil',     b: '…', reviewed: false },
},
```

and the binding passes the band name **as a key, not as a string**:

```js
['#himid-threshold', 'band.threshold', '.knob-control', { band: 'band.himid' }]
```

`tr()` resolves a `vars` value that is itself an `I18N` key to that key's
localized title. **That indirection is the load-bearing decision.** The obvious
alternative — `vars: { band: tr('band.himid', lang).t }` as the plan sketched —
cannot work, because `TIP_BINDINGS` is static module data evaluated once at
load, so it would pin every band tip to whichever language was active then.
Hover any band knob and switch language: you should see `High-Mid — Threshold`
become `Haut-médium — Seuil`, not `High-Mid — Seuil`.

**This is the shape O-FreqPulse needs in Stage F.** If you want a different
placeholder syntax or a different resolution rule, now is the time.

### 3. `reviewed: false` placement

It sits **inside the `fr` object**, alongside `t` and `b`:

```js
fr: { t: 'Gain d’entrée', b: '…', reviewed: false },
```

`node scripts/check-i18n.js` prints the worklist:
`O-MultiBandCompressor  32 / 32 entries unreviewed`. All 32 French entries are
machine-drafted. Spot-check a few — particularly `band.release`
("Rétablissement"), `band.knee` ("Coude") and `band.peak-rms` ("Crête / RMS"),
which are the terms most likely to want a native speaker's judgement.

### 4. Language round-trip in the DAW

- Pick **Français** → every tooltip re-renders immediately, no reload.
- Save the session, close the DAW, reopen → still French.
- Load a factory preset → still French, and the preset's parameters load
  normally.
- Add a **fresh** instance → English immediately, no blank frame, no flash of
  French.

### 5. One thing I changed that you should sanity-check visually

`.tooltip` z-index went 1000 → **10001** (`max-width` untouched at 230 px, as
instructed). Reason in "Deviations" below. Hover a control anywhere on the page
and confirm nothing about the tip layer looks different from v1.9.0.

---

## What shipped

| Layer | Change |
|---|---|
| `js/i18n.js` (new, 372 lines) | 32 keys × {en, fr}, `TIP_BINDINGS` (70 entries), `tr()` with nested-key `vars` resolution |
| `js/app.js` | canonical `applyI18n`/`initI18n` block verbatim; `GLOBAL_TOOLTIPS`/`BAND_TOOLTIPS`/`applyTooltip`/`applyBandTooltips`/`BAND_LABELS` deleted; popover open/close; toggle moved |
| `index.html` | gear + popover markup replacing the `?` button; zero tooltip copy remains |
| `css/styles.css` | `.gear-btn`, `.settings-popover`, `.settings-row/-label/-toggle`; two stacking fixes |
| `PluginProcessor.h/.cpp` | `std::atomic<int> uiLanguage`, `languageCode`/`languageIndex` codec, state-tree persistence |
| `PluginEditor.cpp` | `getUiLanguage`/`setUiLanguage` native pair; `/js/i18n.js` `getResource()` branch |
| `CMakeLists.txt` | `i18n.js` in `SOURCES`; `OMBC_VERSION 1.9.0 → 1.10.0` |

**The four-place rule held in one commit:** the file, `juce_add_binary_data
SOURCES`, the `getResource()` branch and the `import` in `app.js`. Confirmed at
the binary level — `BinaryData::i18n_jsSize = 22309`, exactly `wc -c` of the
source file.

### Binding constraints, each confirmed

- **`grep -rn 'setVisible' plugins/O-MultiBandCompressor/Source/` → zero hits**
  (exit 1). The web view is `addAndMakeVisible`'d once at `PluginEditor.cpp:457`
  and never hidden, so the hidden-completion drop cannot fire. The one-shot pull
  is safe here.
- One convention: `data-tip` + `data-tip-title` attributes written at runtime.
  **No `data-tip-key` attribute anywhere.**
- Bridge is a plain `withNativeFunction` pair, **pulled once at page init**. No
  push from the constructor, no timer, no `poll().then(poll)`, no revision
  counter. JS `try/catch`es `getNativeFunction`, paints English synchronously
  first, `.catch`es the promise.
- Persistence: `parameters.state.setProperty("uiLanguage", "en"|"fr", nullptr)`
  written **before** the `presetManager.getStateAsXml()` delegation (that call
  takes its own `copyState()`, so a later `setProperty` would never reach the
  XML). Restored after `setStateFromXml` with `isVoid()` + `.toString()` — never
  `isBool()`/`isString()`/`isInt()`. Held as `std::atomic<int>`.
- `js/i18n.js` is a plain ES-module import. **No `fetch()`, no JSON file.** No
  hyphen in the filename.
- `applyI18n()` runs from the foot-of-file `initializeDeferredUI()` inside its
  own `try/catch`, with the table and language bindings declared **below** the
  eager `initializeUI()` call and **above** every reader. Verified by rendering:
  `initializeCrossoverDrag()` still binds all three crossover lines, which is
  precisely the code the v1.4.0 TDZ throw killed.
- Localized strings stay `textContent` on every path. No `innerHTML`, no `<` in
  any string literal (gate assertion 9).
- `.tooltip { max-width }` **unchanged** at 230 px.
- The `setTooltipsEnabled` toggle **moved** into the popover.
- Every French entry carries `reviewed: false`.
- Label-erasure trap: both popover captions carry `data-label`, the `<option>`
  endonyms are never written by a state updater, and the render harness asserts
  `Language|Hover help` survives page init.

---

## Deviations from plan

### 1. `Object.freeze()` on the two object-literal exports in `i18n.js` — affects the canon for all 43

**This is the one deviation that needs your decision at Checkpoint 1.**

The plan's `<canonical_contract>` §1 writes the table as
`export const I18N = { … };`. That form **fails `check-i18n.js` assertion 7**.
The gate's scanner segments top-level statements on `}` or `;` at depth zero, so
an export whose closing brace lands at depth zero splits the trailing `;` off as
an empty statement of its own — reported as a top-level statement outside an
export declaration. It fired exactly twice, on `BAND_GR_METERS` and `I18N`; the
array exports are unaffected because `]` is not a segmentation trigger.

I kept the change inside the plugin (my scope was MBC only) by wrapping both
object exports in `Object.freeze(…)`, which nests the brace and also states that
the table is inert data. It reads fine and I would keep it.

**The alternative** is a one-line tolerance for an empty statement in
`scripts/check-i18n.js` (Stage A), which would let the plan's original literal
form stand. Pick one before Stage C — whichever you choose is what 42 more
plugins get.

### 2. `[Rule 1 — Bug] The settings popover was rendered but unclickable`

Found during Stage B, by rendering the page rather than reading it. Playwright
reported `<section class="spectrum-section"> intercepts pointer events` on
`#tips-toggle`: `.plugin-header` and `.spectrum-section` both sit at `z-index: 1`
and the spectrum comes later in the DOM, so it won the tie. Because the header is
itself a stacking context, no `z-index` on the popover could lift it out.

Fixed with the idiom this plugin **already documents** for its preset dropdown
(`styles.css:190-194`): lift the whole header while the panel is open, via
`.plugin-header:has(.settings-popover:not([hidden]))`.

I probed the preset dropdown before changing anything to see whether it shared
the bug — it does not; it already carries `z-index: 9999` plus its own `:has()`
rule, and all ten probe items down to y=239 were hit-testable. So this is a new
bug introduced by the new panel, not a pre-existing one.

### 3. `[Rule 1 — Bug] .tooltip z-index 1000 → 10001`

Consequence of fix 2. The tip layer is a sibling of `.plugin-header` inside
`.plugin-container`; once the header is lifted to 9999 while a panel is open, a
tip raised over a control **inside** that panel — the language selector and the
hover-help toggle both have one — painted behind it. Verified with
`elementsFromPoint`: the stack is now `tooltip@10001` above `spectrum-section@1`.
`max-width` deliberately untouched.

### 4. The hover-help toggle's own tip is ONE key, not a state-swapped pair

v1.9.0's `setTooltipsEnabled()` wrote two hard-coded English sentences onto the
button, swapped on click. That cannot survive the canon: `applyI18n()` re-renders
every tip straight from the table on a language change, so a state-dependent
string written outside it is stranded in the previous language the moment the
selector fires — and `window.__setLanguage()`, which the Stage D clamp gates use,
does not fire a `change` event at all, so no listener workaround would cover both
paths.

So the copy is one `tips-toggle` entry covering both states, and the caption plus
`aria-pressed` carry the state. This matters beyond MBC: **O-Bitrot, O-Tapestop,
O-Octagon, O-Contrabass, O-Orbit, O-FreqPulse, O-Lyrica, O-SpectralShaper and
O-Marimba all have this same toggle** and will hit the same problem.

### 5. `PLUGINS.md` row updated on disk but NOT committed

The row now reads `1.10.0 | … | 2026-08-26`, but `PLUGINS.md` already carried
**unrelated modifications from another session** when I started. `git commit --
<path>` commits a whole file, and there is no line-level path scoping without
interactive staging, so committing it would have swept another session's work
into my commit. Per the commit discipline in my brief, the commit contains only
`plugins/O-MultiBandCompressor/**`.

**Action needed:** whoever owns the other `PLUGINS.md` change should commit it,
or the row can be committed separately once the file is otherwise clean.

---

## Verification — what was run

| Check | Result |
|---|---|
| `node scripts/check-i18n.js` (repo-wide) | **exit 0** — 19/19 assertions, 1 localized plugin, 32/32 French unreviewed |
| `node scripts/check-i18n.js --plugin O-MultiBandCompressor` | **exit 0** |
| `./scripts/build-and-install.sh O-MultiBandCompressor` | **clean**, 50 s. VST3 + AU installed, AU cache cleared, dual-variant sweep ran |
| `auval -v aufx OMbc OuDv` | **AU VALIDATION SUCCEEDED** |
| `plugins/O-MultiBandCompressor/tests/render-harness` (`O-MultiBandCompressor-preset-test`) | **47 presets active, 0 failures**, order-independence clean over all 50 |
| `node --check` on `app.js` and `i18n.js` as ES modules | clean |
| Scratchpad Playwright harness against the real page | **40/40, exit 0** |
| Binary embedding | `BinaryData::i18n_jsSize = 22309` == `wc -c i18n.js` |
| `git show --stat b98aa9ec` | 9 files, all under `plugins/O-MultiBandCompressor/` |

MBC has **no** UI gate in `tests/` — only the preset render harness. That is why
the plan made it the pattern-bearer: Stage B proves the pattern without also
proving a gate rewrite. Stage C does the opposite under O-Octagon's gates.

### The render harness (composed band tips, both languages)

The one thing I could not verify from static checks is that the composed band
tooltips actually **render** correctly in both languages, so I drove the real
page in Chromium at the shipping viewport 900 × 640, serving
`Source/ui/public/` byte-identical except `js/juce/index.js` swapped for a
minimal bridge stub — the same technique the repo's own clamp gates use.

Measured, not inspected:

- module evaluation survived; **zero** page errors or `console.error`s; all
  three crossover lines still bound
- English default painted synchronously: `High-Mid — Threshold`,
  `Low — Gain Reduction`, `Low-Mid — Frequency Range`
- French: `Haut-médium — Seuil`, `Grave — Réduction de gain`, `Gain d’entrée`
- **all 70** bound elements carry a non-empty title and body with **no
  unsubstituted `{token}`** in either language
- the **renderer** — not just the attribute — shows `Aigu — Taux` with the
  French body, and that tip fits the 900 × 640 viewport
  (l=662 r=892 t=171.5 b=246)
- popover opens/closes/Escapes, fits the viewport, both controls are the
  hit-test target at their own centre, endonyms `English` / `Français` intact,
  authored `.settings-label` captions not erased
- choosing a language writes through `setUiLanguage`, and the tips follow
- `applyI18n('fr')` syncs the `<select>` without a `change` event (the path the
  Stage D clamp gates will drive)
- the old `#help-toggle` is **gone** (moved, not duplicated), `#tips-toggle` is
  inside the popover, and its tip stays French across a state change

This harness lives in the session scratchpad, **not** in the repo. Stage B's plan
does not call for a committed MBC UI gate, and MBC has no `tests/ui-stub/`. If
you want one, that is a scope addition worth deciding at this checkpoint.

---

## Not verified

Be aware of these before approving the pattern.

1. **Nothing has been checked in a real DAW.** Everything above is a headless
   Chromium render, `auval`, and an offline C++ harness. The DAW round-trip in
   "Checkpoint 1 — what to look at" §4 is exactly what I could not do.
2. **The C++ persistence path was never executed.** `uiLanguage` compiles, and
   `auval` exercises `get/setStateInformation` for parameter state, but nothing I
   ran wrote `"fr"` to a session and read it back. The claim "close the DAW,
   reopen, still French" is **reasoned, not measured** — it rests on
   `getStateAsXml()` taking its own `copyState()` (read, confirmed at
   `OuariconPresetManager.h:579-583`) and `setStateFromXml()` doing
   `replaceState(ValueTree::fromXml(...))` (read, confirmed at `:604-613`), which
   restores a root XML attribute as a tree property. **Please verify this one by
   hand at the checkpoint** — it is the highest-value manual check on the list.
3. **The native bridge was exercised only against a stub.** The real
   `Juce.getNativeFunction('getUiLanguage')` round-trip through the WebView has
   not run.
4. **The Standalone `.app` is stale.** `build-and-install.sh` builds VST3 + AU
   only. Do not judge the UI there without rebuilding that target.
5. **All 32 French strings are unreviewed machine drafts.** No native speaker has
   seen them. `check-i18n.js` prints the worklist.
6. **French geometry is proven at MBC's viewport only, and only for the anchors
   the harness hovered** (two of 70). The full per-anchor sweep in both languages
   is Stage D's job, on the three clamp-gated plugins. MBC has no clamp gate.
7. **No cross-platform check.** Windows/WebView2 has not been exercised; the
   `getResource()` scheme differs per platform, though the branch follows the
   existing pattern exactly.
8. **`PLUGINS.md` is uncommitted** (deviation 5).

---

## Next (as of the end of Stage B)

Checkpoint 1 was answered: `Object.freeze()` stays AND the gate's assertion-7
scanner bug was fixed independently (`cceebfa4`); the `{token}` nested-key
resolution and the one-key-per-toggle rule are settled contract. Stage C
proceeded on that basis.

---
---

# Stage C — O-Octagon v1.6.0 (task T4)

Stage C is done: **O-Octagon v1.6.0** ships in one path-scoped commit
`8dcb1317` — 12 files, all under `plugins/O-Octagon/` plus the one PLUGINS.md
row. Stages D–G remain.

O-Octagon is the gate-hardening plugin, and the point of the ordering was to
prove the Stage B contract survives the strictest static gate in the repo
**while rewriting that gate in the same commit**. It did.

## What shipped

| Layer | Change |
|---|---|
| `js/i18n.js` (new, 517 lines) | 45 keys × {en, fr}; 55 `TIP_BINDINGS`; `tr()` with nested-key `vars` |
| `js/app.js` | canonical `applyI18n`/`initI18n` block **verbatim**; `initSettingsPopover()`; the hover-help toggle rewired to a segmented pair |
| `index.html` | all 53 `data-tip`/`data-tip-title` literals removed; gear + popover replace the `?` button; two footer readout labels gained ids |
| `css/styles.css` | `.help-toggle` → `.settings-cluster` / `.gear-btn` / `.settings-popover` / `.settings-row` / `.settings-select` / `.settings-unit-group` / `.settings-unit` |
| `PluginProcessor.h/.cpp` | `std::atomic<int> uiLanguage`, the `languageCode`/`languageIndex` codec, **root XML attribute** persistence |
| `PluginEditor.cpp` | `getUiLanguage`/`setUiLanguage` pair; `/js/i18n.js` `getResource()` branch |
| `CMakeLists.txt` | `i18n.js` in `SOURCES`; `VERSION 1.5.0` → `1.6.0` |
| `tests/ui_frontend_check.js` | §3 count 23 → 25; §9 embedded 11 → 12; §9 import scan widened to both quote styles |
| `tests/ui-stub/juce-stub.js` | the two new names + a mirrored `uiLanguage` |

The four-place rule held in one commit, confirmed at the binary level:
`UIBinaryData::i18n_jsSize = 30719`, exactly `wc -c` of the source file.

## The gates that break by construction — and what happened to each

| Gate | Expected | Result |
|---|---|---|
| §21 derived module registry | `js/i18n.js` on disk must appear in `SOURCES` | passes; registry now derives **8** modules and set-equality holds |
| §9 three-way closure | embedded count is a moving literal | 11 → 12, **plus an unplanned second edit** — see below |
| §3 native-fn census | count moves, stub whitelist must match | 23 → 25, `setsEqual(stubbed, registered)` green |
| §2 init ordering | `init();` must stay the literal last statement | passes; `initI18n()` is called from **inside** `init()` and the hoisted import is the only new top-level form |
| §6 HTML-authored labels | no new `textContent` receiver | passes **untouched** — the toggle became a two-button pair rather than one that relabels itself, so the whitelist did not have to grow |
| §1 / §7 / §12 / §14 / §19 | all iterate the derived module list, so they now scan `i18n.js` too | all pass; `i18n.js` balances at 0/0 gestures and defines no projection |

**§9 needed an edit the plan did not predict.** Its reference scan was
`/from "\.\/([^"]+)"/g` — **double quotes only**. Every import on this page was
double-quoted until now, but the canonical i18n import line is single-quoted
repo-wide and `check-i18n.js` assertion 6 requires it **verbatim**. Left alone,
§9 reported `js/i18n.js` as embedded-but-never-referenced: a false failure
describing a page that does import it. The regex now accepts both styles, with
the reason recorded in the section's own comment. **This will recur on every
plugin whose gates scan imports** — O-ReverseDelay and O-Contrabass in Stages D
and E should be checked for the same pattern before their counts are bumped.

## Verification — what was run

| Check | Result |
|---|---|
| `node plugins/O-Octagon/tests/ui_frontend_check.js` | **exit 0** — 43/43 sections, **377** assertions |
| `node plugins/O-Octagon/tests/ui_layout_check.js` | **exit 0** — 31/31 sections, rendered at 1100 × 720. **Not 77**: Playwright resolved and the page really was measured |
| `node scripts/check-i18n.js` (repo-wide) | **exit 0** — **2 localized plugins**, all assertions passing |
| `node scripts/check-i18n.js --plugin O-Octagon` | **exit 0** |
| `./scripts/build-and-install.sh O-Octagon` | clean, 49 s; VST3 + AU installed, AU cache cleared, dual-variant sweep ran |
| `O-Octagon-geometry-test` (CI job `octagon-probes-macos`) | **exit 0** — 49 probes, 0 failures |
| `O-Octagon-render-test` (same CI job) | **exit 0** — 57 probes, 0 failures |
| `auval -v aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED** |
| Shipped bundle version | `CFBundleShortVersionString = 1.6.0` |
| Binary embedding | `i18n_jsSize = 30719 == wc -c js/i18n.js` |
| `git show --stat 8dcb1317` | 12 files: 11 under `plugins/O-Octagon/`, plus the PLUGINS.md row |

`plugins/O-Octagon/tests/` holds exactly four gates —
`ui_frontend_check.js`, `ui_layout_check.js`, `tests/unit` (geometry) and
`tests/render-harness` (render). All four ran; all four passed. `tests/tools/`
is study and fixture-generation scripts, not gates. **The two C++ targets are
the ones `ci-tests.yml` actually runs per-commit**, which is why they were built
from a fresh `-DOUARICON_BUILD_TESTS=ON` configure rather than assumed.

### The popover is CLICKABLE, not merely present

Stage B found a panel that rendered and was pointer-dead behind a z-index tie no
static check could see, so this was measured rather than inspected. A scratchpad
Playwright harness served `Source/ui/public/` byte-identical except
`js/juce/index.js` → the repo's own ui-stub, at the shipping 1100 × 720.
**40 probes, all pass:**

- zero console errors, zero `i18n:` warnings, all 10 column readouts bound — a
  TDZ throw would have left every one at the authored em-dash
- **55** elements carry a `data-tip`; none blank, none with an unsubstituted
  `{token}`, in either language
- `elementFromPoint` at the centre of `#gear-btn`, `#lang-select`,
  `#btn-tips-on` and `#btn-tips-off` returns **each control itself** — this is
  the assertion Stage B's bug would have failed
- the panel fits the viewport (l=872 t=47.5 r=1082 b=126.5)
- `{n}` substitutes per cell: `Weight 1` … `Weight 8`, `User scene 3`
- `'.vunit-group'` still resolves to the **Venue** delay toggle, not the new
  popover group — the collision the distinct class exists to prevent
- Français: `Poids 5`, `Scène utilisateur 2`, `Aide au survol`; and
  `getUiLanguage()` afterwards returns `"fr"`, so the write reached the
  (stubbed) processor
- `window.__setLanguage('en')` syncs the `<select>` and fires **0** change
  events — the path Stage D's clamp gates will drive
- a tip raised over a control **inside** the panel renders in French, fits the
  viewport, respects the untouched 240 px cap, and paints on top
  (tooltip z=20 vs popover z=15)
- Escape and an outside press both dismiss; the endonyms `English|Français` and
  the authored captions `Language|Hover help` survive; `#help-toggle` is **gone**
  (moved, not duplicated); the `⚙` glyph is never written by JS

### French geometry, measured — O-Octagon has no clamp gate, so this stood in for one

Every anchor on **both screens**, hovered and measured, in **both languages**:

| | anchors | horizontally clamped | flipped below | widest |
|---|---|---|---|---|
| `en` | 57 | 18 | 12 | 240.0 px |
| `fr` | 57 | 18 | **13** | 240.0 px |

Every tip in both languages sits fully inside 1100 × 720 with an 8 px margin.
The clamp engaged 18 times per language, so the sweep is not vacuous about it.
**French costs exactly one extra vertical flip and no extra clamp.** Nothing
exceeded the 240 px cap, which was not touched.

This is an early, favourable data point for Checkpoint 2 — but it is **not** the
Stage D verdict. O-Octagon's frame is 1100 × 720; the clamp trio ships at
860 × 580 and 840 × 620, where the same French strings have far less room.

### Three negative controls, each confirmed to FAIL

A gate that has never been seen to fail proves nothing.

| Mutation | Expected failure | Observed |
|---|---|---|
| delete `getUiLanguage` from `juce-stub.js` | §3 stub/C++ set-equality | `FAIL: [3] … cpp-only: [getUiLanguage]` |
| revert §9's import regex to double-quote-only | §9 referenced == embedded | `FAIL: [9] … embedded-only: [/js/i18n.js]` |
| change one line of the canon block in `app.js` (`includes` → `indexOf`) | check-i18n assertion 6 drift gate | `FAIL: [6] the applyI18n/initI18n region matches scripts/i18n-canon.js` |

Each was applied to a byte-exact backup and restored from that backup, **never**
`git checkout --`, which would have wiped the uncommitted work alongside the
mutation.

## Decisions taken inside Stage C

### 1. The hover-help toggle became a segmented On/Off PAIR, not a relabelling button

MBC writes `helpToggleEl.textContent = 'On' | 'Off'`. Doing that here would have
added a `textContent` receiver and forced §6's explicit whitelist to grow — a
list whose comment says it is *reviewed when it grows*. O-Octagon already has
the right idiom one screen away: the Venue delay unit toggle, whose comment
reads "TWO BUTTONS, NOT ONE THAT RELABELS ITSELF". Both captions stay authored,
the state is a class plus `aria-pressed`, and **§6 did not have to change at
all**. The toggle's own tip is still one key covering both states, per the
settled contract.

### 2. A distinct class for the popover's toggle group — found by rendering

The first draft reused `.vunit-group`. `TIP_BINDINGS` binds the delay-unit tip
through `document.querySelector('.vunit-group')`, which returns the **first**
match in document order — and the header now comes before the Venue screen. The
delay-unit tip would have silently landed on the hover-help toggle. Caught
before it shipped, but caught by reasoning about the selector, and then
**confirmed by a render probe** that asserts `.venue-delay .vunit-group` still
carries `"Delay unit"`.

### 3. Two footer `<span class="readout-label">` elements gained ids

They were the only tooltip anchors with neither an id nor a unique class or a
containing element to reach them through. `:nth-of-type` would have worked and
would have broken the first time a footer child moved.

### 4. Popover z-index 15, deliberately between the plan stack (3) and `.tooltip` (20)

`.tooltip` and `max-width: 240px` are **both unchanged**. O-Octagon's header is a
plain flex row that opens no stacking context, so unlike MBC there was no
z-index tie to break and no header-lifting `:has()` rule was needed — verified
by hit-test, not assumed.

## Deviations from plan

### 1. `[Rule 3 — Blocking]` §9's import scan had to accept single quotes

Unplanned, in scope, and described above. Without it §9 fails on correct code.
Recorded in the section's own comment in the file's style.

### 2. `[Rule 2 — Missing critical functionality]` §9's embedded-count literal

The plan named §3's count (23 → 25) and §21, but not §9's `embedded.size === 11`.
It moves for exactly the same reason and was bumped to 12 with its running
comment extended.

### 3. The `preset-list` tooltip's stale "17 parameters" was moved VERBATIM

v1.5.0 added an 18th parameter (`decorr`) and did not update that sentence. The
plan is explicit that this task *moves* English rather than rewriting it, so the
stale count came across unchanged rather than being silently corrected inside an
i18n commit. **Flagged in the CHANGELOG under "Known, and left alone"** — it is
a one-word fix for whoever next touches that string.

### 4. Pre-existing uncommitted O-Octagon work was NOT swept in

The brief warned that `plugins/O-Octagon/` carried uncommitted modifications
from other work. By the time Stage C started, another session had committed them
as `0826c0d4` (**O-Octagon v1.5.0** — the mono decorrelator), and `be94e414` had
committed the MBC PLUGINS.md row. Both paths were verified clean by
`git status --short` immediately before staging, and again immediately before
committing. `git show --stat 8dcb1317` confirms the commit contains only
`plugins/O-Octagon/**` and PLUGINS.md — the five modified `.claude/*` files
belonging to other work stayed out.

### 5. PLUGINS.md IS committed this time

Unlike Stage B. The file was clean when Stage C started, so the row could go in
with the plugin. The union-merge duplicate check
(`grep "^| O-" … | sort | uniq -d`) returns empty.

## Not verified — read this before Stage D

1. **Nothing has been checked in a real DAW.** Everything above is headless
   Chromium, `auval`, and two offline C++ harnesses.
2. **The C++ persistence path was never executed.** `uiLanguage` compiles and
   `auval` exercises `get/setStateInformation`, but nothing wrote `"fr"` into a
   session and read it back. The claim "close the DAW, reopen, still French" is
   **reasoned, not measured**. It is a stronger inference here than on MBC —
   O-Octagon writes a root XML attribute on the very XML it serialises, next to
   the `tooltipsEnabled` attribute that has shipped and worked since v1.2.0 —
   but it is still an inference. **This is the highest-value manual check
   outstanding**, and it is outstanding on both localized plugins.
3. **The native bridge was exercised only against the ui-stub.** The real
   `Juce.getNativeFunction('getUiLanguage')` round trip through WKWebView has
   not run.
4. **The Standalone `.app` is stale.** `build-and-install.sh` builds VST3 + AU
   only.
5. **All 45 French entries are unreviewed machine drafts.** The terms most
   likely to want a native speaker: `rake` ("Inclinaison"), `hullAtten`
   ("Atténuation hors enveloppe"), `rolloff` ("Atténuation"), `decorr`
   ("Décorréler"), `output-set` ("Jeu de sorties"). Repo total is now **77**
   unreviewed entries across two plugins.
6. **No cross-platform check.** Windows/WebView2 has not been exercised. The
   `getResource()` branch follows the existing pattern exactly, and
   `ci-tests.yml`'s `octagon-windows-vst3` job will build and pluginval it — but
   that has not run here.
7. **The two render probes live in the session scratchpad, not in the repo.**
   The plan does not call for a committed O-Octagon tooltip gate, and adding one
   would be a scope addition. So the popover-clickability assertion and the
   both-language anchor sweep are **evidence for this change only** — they will
   not catch a regression tomorrow. Worth deciding whether the both-language
   sweep should be promoted into `tests/` when Stage D rewrites the three clamp
   gates into that exact shape.
8. **French geometry is proven at 1100 × 720 only.** The Stage D trio is
   smaller; that is where the real risk sits.

## Next

**Stage D** — the clamp trio, O-ReverseDelay → O-Bitrot → O-Tapestop, one
path-scoped commit each. Carry forward:

- **Check each plugin's gates for a double-quote-only import scan** before
  bumping its counts. O-ReverseDelay and O-Contrabass are the candidates.
- O-ReverseDelay's bridge count 13 → 15, its `:691-696` anchor assertion
  rewritten, and `:760-761` (the D13 `setTooltipsEnabled` prohibition) left
  **untouched and green**.
- All three clamp gates parameterised over `['en','fr']`, reading `max-width`
  from each plugin's own CSS rather than the literal 230.
- The sweep re-run **after** the gear button lands, not before.
- If a plugin's toggle can be expressed as a two-button pair, prefer it — it
  keeps the "no textContent write" property that O-Octagon's §6 protects.

---
---

# Stage D — the clamp trio (task T5)

Stage D is done. **O-ReverseDelay v1.9.0** (`951dd584`), **O-Bitrot v1.14.0**
(`1f3a9faa`) and **O-Tapestop v1.5.0** (`547f9738`) ship in three path-scoped
commits, in that order. `node scripts/check-i18n.js` ends at **5 localized
plugins**, all assertions passing.

Stages E, F and G remain and are **not approved**.

This stage existed to MEASURE the French geometry claim that Stages B and C only
sampled. These are the only three plugins in the repo with a
`tests/ui_tooltip_clamp_check.js`, and all three are SMALLER frames than the
1100 × 720 where French was first measured.

---

## THE ANSWER: the French geometry claim holds, MEASURED, on all three frames

Every anchor of every plugin hovered and measured, in both languages, at the
shipping viewport:

| Plugin | frame | anchors | clamped en → fr | flipped below en → fr | tallest en → fr |
|---|---|---|---|---|---|
| O-ReverseDelay | 940 × 768 | 31 | 8 → 8 | 2 → 2 | 133.9 → 148.8 px |
| O-Bitrot | 900 × 740 | 55 | 14 → 14 | 10 → **13** | 119.1 → 133.9 px |
| O-Tapestop | 860 × 580 | 35 | 12 → 12 | 4 → 4 | 104.2 → 119.1 px |

**Every tip in both languages sits fully inside its viewport with an 8 px
margin, on all three frames.** No assertion failed in French anywhere, so the
Checkpoint-2 branch about shortening copy or adjusting a flip threshold does not
arise.

Three things worth reading out of that table:

1. **French costs three extra vertical flips on O-Bitrot and none on the other
   two.** This is the first place in the whole rollout where French changes flip
   behaviour at all — O-Octagon at 1100 × 720 cost one, which the Stage C
   summary flagged as "an early, favourable data point, not the Stage D
   verdict". All three of O-Bitrot's extra flips were caught by the existing
   flip logic, which is exactly what assertion 3 exists to prove.

2. **The clamp count did not move on any of the three.** That is the expected
   result and it is worth stating explicitly: assertion 2 is capped by
   `max-width`, so a longer string cannot get wider — it wraps. The horizontal
   risk was never the language-sensitive one, and now that is measured rather
   than argued.

3. **French is 14.8 px taller at its tallest on all three plugins** — one extra
   wrapped line at this font size. The consistency is a property of the cap, not
   a coincidence: 230 px is the same on all three.

`.tooltip { max-width }` was **not touched** on any plugin.

### Why the counts are not vacuous

Two guards, both required PER LANGUAGE in each of the three gates:

- **The clamp must engage at least once.** It engaged 8 / 14 / 12 times per
  language. A run where it never fires proves nothing about the clamp.
- **Every anchor's copy must actually DIFFER between `en` and `fr`.** Without
  this a sweep in which `window.__setLanguage` silently did nothing would
  measure English twice and report a confident, meaningless pass. Verified by
  negative control on two of the three plugins: stubbing `__setLanguage` to a
  no-op fires it on all 31 / all 55 anchors respectively.

---

## The both-language sweep is now a COMMITTED gate, not a scratchpad probe

Stage C's summary flagged that its render probes lived in the session scratchpad
and "will not catch a regression tomorrow". Stage D closes that: the
parameterised clamp gate IS the permanent home, in all three `tests/`
directories, and it runs on `node plugins/<Name>/tests/ui_tooltip_clamp_check.js`
like every other gate in the repo.

**Parameterised, not duplicated.** One process, one page load, every existing
sweep pass run once for `en` and once for `fr`. Every failure is labelled with
its language, because a French-only failure in an unlabelled run reads as a
mysterious regression in a file that never mentions French.

Four changes shared by all three gates:

| Change | Why |
|---|---|
| `max-width` PARSED from the plugin's own CSS | The cap differs per plugin — 230 on all three here, 240 on O-Octagon, 220 on O-Polystutter. A mirrored literal mis-asserts the moment the file is pointed elsewhere. The literal survives as the drift guard. |
| A final sweep pass with the settings popover OPEN | `#lang-select` (and the moved toggle) are real anchors inside a panel that ships hidden. Excluding them would have skipped the two controls sitting in the top-right corner — where the clamp and the flip bite hardest. |
| The anchor count DERIVED from `TIP_BINDINGS` | O-Bitrot and O-Tapestop counted `data-tip=` literals in `index.html`. That count is **zero** now, so the old assertion would have passed vacuously against nothing. |
| A `withinCap` assertion (5th per anchor) | A tip wider than `max-width` means the CSS is not doing the wrapping the whole language sweep depends on. |

---

## Per-plugin, what shipped

### O-ReverseDelay v1.9.0 — `951dd584`, 13 files

29 tooltips moved; 31 keys. **D13 respected in substance: no hover-help toggle,
no `setTooltipsEnabled`, ever.** The popover carries the language selector
alone. Bridge 13 → 15.

Two gate rewrites in `ui_frontend_check.js`:

- **`:190` bridge count 13 → 15**, with the running justification comment
  extended in the file's own style.
- **`:661-696` anchor coverage**, which failed by construction once the copy
  left the markup. Rewritten into the stronger form the brief asked for: every
  anchor is bound in `TIP_BINDINGS`, resolves to a key that exists, and carries
  BOTH an `en` and an `fr` entry. The old form could only say "some string is
  present"; this one fails on a missing translation. **It immediately found the
  inventory had drifted** — v1.8.0's COLOUR panel shipped without adding
  `knob-diffusion` and `knob-drive`, the second such drift in that file.
  Backfilled.

### O-Bitrot v1.14.0 — `1f3a9faa`, 11 files

53 tooltips moved; 55 keys. **41 of the 53 anchors carry no id** — they are
`.ctl` and `.g-group` wrappers around a `[data-param]` knob — so binding by id
was never available. This is the first plugin in the rollout to need the
canonical `[selector, key, wrapper]` triple in earnest, and it is the shape
MBC's `.control-group` case predicted.

### O-Tapestop v1.5.0 — `547f9738`, 12 files

33 tooltips moved; 35 keys. 17 anchors are idless wrappers, addressed through
their first **id'd descendant** rather than their class — because the sync/free
swap slots put a `.select-cell` and a `.knob-cell` back to back under the SAME
tip title ("Spin-Down Time" appears twice, once as a note division and once in
milliseconds), so a class-based key would have collided. Caught by the
generator's own uniqueness check before it could ship.

The gear takes the `?` button's **exact absolute box** — the 860 × 580 frame is
a PLAN Locked Decision, so nothing was allowed to move. Asserted by measurement.

---

## Deviations from plan

### 1. `[Rule 3 — Blocking]` D13's assertion was wrong-shaped and had to be tightened

**D13 itself is untouched and still holds.** O-ReverseDelay has no hover-help
toggle and no `setTooltipsEnabled`, and nothing was added.

The assertion guarding it, though, was a bare substring search for the name
anywhere in `app.js` or `PluginEditor.cpp`. That cannot distinguish REGISTERING
the function from writing a comment saying the function is deliberately absent —
and the comments this release adds explaining the absence contain the literal
name. The gate therefore failed on source that honours D13 exactly: it reported
a violation of a rule the code was obeying.

Rather than delete the assertion or strip the comments (the comments are the
useful part, and the next plugin would hit this again), it was **tightened** to
match the two forms that would be a real violation — a C++ `withNativeFunction`
registration and a JS `getNativeFunction` fetch — plus a new assertion that the
processor holds no `tooltipsEnabled` state at all. Confirmed by negative
control: injecting a real registration fires all three of the D13 assertion, the
dead-registration check and the bridge census; the comments fire none of them.

### 2. `[Rule 3 — Blocking]` `scripts/check-i18n.js` assumed every plugin has a `js/app.js`

O-Bitrot's controller is one inline `<script type="module">` in `index.html`.
The drift gate read `p.appJs` unconditionally and reported `[6] app.js exists`
FALSE on a plugin that is entirely correct — the same wrong-shaped assertion as
O-Octagon's double-quote-only import scan in Stage C, and the second instance of
that class in this task.

It now reads the module from wherever it lives, and accepts the depth-adjusted
import specifier (`'./js/i18n.js'` from the UI root vs `'./i18n.js'` from
`js/`). **Only the specifier differs** — the `applyI18n`/`initI18n` BODY it
byte-compares is identical either way, and that body is the part 43 hand-copies
can actually drift in. Confirmed by negative control: changing one line of the
canon inside the inline module fires assertion 6.

This is the one file outside the three plugins that this stage touched. It is a
Stage A tool, and it was blocking a correct plugin.

### 3. `[Rule 1 — Bug]` the `?`-glyph assertion on O-Bitrot and O-Tapestop

Both clamp gates pinned the literal `?` as proof that a state updater had not
erased an HTML-authored label. The toggle now reads On/Off, so its caption is
written from script for the first time — and a check for `?` would fail on a
deliberate change while saying nothing about the rule it was protecting.

Rewritten to compare the RENDERED caption against the `data-on` / `data-off`
attributes authored on the element. If either is stripped, the plugin's own
fallback literal supplies the caption and the assertion fails — which is exactly
the case that matters. Confirmed by negative control on both plugins.

### 4. The quote-style trap did NOT recur

The brief flagged O-Octagon's `/from "\.\/([^"]+)"/g` double-quote-only scan.
Checked all three plugins before concluding anything: **O-ReverseDelay's §9
already uses `["']`** and accepts both styles, and O-Bitrot and O-Tapestop have
no import-reference scan at all. No change was needed.

### 5. Another session's `PLUGINS.md` row was swept into the O-Bitrot commit, and removed

`git commit -- PLUGINS.md` commits the whole file. Between the O-ReverseDelay
and O-Bitrot commits, a concurrent session edited the O-Octagon row
(`1.6.0-dev` → `1.7.0-dev`), and it landed in my commit.

Caught by the post-commit `git show --stat` check (2 rows changed, not 1),
amended out, and the working-tree edit restored so it is uncommitted again and
still belongs to that session. The O-Tapestop commit held their row back the
same way, deliberately, before committing. **Their O-Octagon row is still
uncommitted on disk** — that is correct and intentional; it belongs with their
source changes, which are also still uncommitted.

### 6. `i18n.js` was untracked and nearly missed the first commit

`git commit -- <path>` does **not** stage untracked files. The O-ReverseDelay
commit went in without `js/i18n.js` — the single most important file in it.
Caught by the post-commit `git show --stat`, `git add`ed and amended. The other
two were staged explicitly before committing.

Worth recording as a repo hazard: the four-place rule says the file, SOURCES,
`getResource()` and the import all land in ONE commit, and path-scoped
committing silently violates it for the new file every time.

---

## Verification — every gate, individually

| Plugin | Gate | Result |
|---|---|---|
| O-ReverseDelay | `ui_frontend_check.js` | **exit 0** — 159 assertions, 0 failures |
| O-ReverseDelay | `ui_tooltip_clamp_check.js` | **exit 0** — 89 assertions. **Not 77**: Playwright resolved and the page really was measured |
| O-ReverseDelay | `O-ReverseDelay-render-test` | **exit 0** — ALL PROBES PASSED (0 failures) |
| O-ReverseDelay | `build-and-install.sh` | clean, 51 s; VST3 + AU, AU cache cleared, dual-variant sweep ran |
| O-ReverseDelay | `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Bitrot | `ui_tooltip_clamp_check.js` | **exit 0**, both languages. Not 77 |
| O-Bitrot | `ui_preset_menu_check.js` | **exit 0** |
| O-Bitrot | `O-Bitrot-render-test` | **exit 0** — 109/109 probes |
| O-Bitrot | `build-and-install.sh` | clean, 33 s |
| O-Bitrot | `auval -v aufx OBrt OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Bitrot | binary embedding | `BinaryData::i18n_jsSize` 29845 == `wc -c` |
| O-Tapestop | `ui_tooltip_clamp_check.js` | **exit 0**, both languages. Not 77 |
| O-Tapestop | `O-Tapestop-render-test` | **exit 0** — 69 probe checks, 0 failures |
| O-Tapestop | `build-and-install.sh` | clean, 49 s |
| O-Tapestop | `auval -v aufx OTsp OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Tapestop | binary embedding | `UIBinaryData::i18n_jsSize` 23319 == `wc -c` |
| repo | `scripts/check-i18n.js` | **exit 0** — **5 localized plugins**, all 19 assertions each |

That is every gate in all three `tests/` directories. O-ReverseDelay has
`ui_frontend_check.js`, `ui_tooltip_clamp_check.js` and `render-harness`;
O-Bitrot has `ui_preset_menu_check.js`, `ui_tooltip_clamp_check.js` and
`render-harness`; O-Tapestop has `ui_tooltip_clamp_check.js` and
`render-harness`. All ran. All passed. **No gate exited 77.**

### English fidelity, checked mechanically rather than claimed

This task MOVES English, it does not rewrite it. On all three plugins the copy
was extracted from the markup by script and the generated table compared back
against it: **29/29, 53/53 and 33/33 entries byte-identical**, HTML entities
decoded. Nothing was hand-transcribed.

### Negative controls — every rewritten gate confirmed to FAIL

A gate that has never been seen to fail proves nothing.

| Mutation | Expected failure | Observed |
|---|---|---|
| delete one `fr` entry (O-ReverseDelay) | section 14 completeness | `FAIL: … knob-jitter -> knob-jitter (fr MISSING)` |
| drop a `TIP_BINDINGS` entry (O-ReverseDelay) | section 14 coverage | `FAIL: all 29 controls are bound … UNBOUND: knob-drive` |
| register a REAL `setTooltipsEnabled` (O-ReverseDelay) | D13 + census + dead-registration | all three fired |
| no-op `window.__setLanguage` (O-ReverseDelay) | fr-differs vacuity guard | `FAIL: … 0/31 differ` |
| no-op `window.__setLanguage` (O-Bitrot) | fr-differs vacuity guard | `FAIL: … 0/55 differ` |
| no-op `window.__setLanguage` (O-Tapestop) | fr-differs vacuity guard | `FAIL: … 0/35 differ` |
| strip `data-off` (O-Bitrot) | authored-caption assertion | both halves fired |
| strip `data-off` (O-Tapestop) | authored-caption assertion | both halves fired |
| drift one canon line in the INLINE module (O-Bitrot) | check-i18n `[6]` | `FAIL: … matches scripts/i18n-canon.js` |
| drift one canon line (O-Tapestop) | check-i18n `[6]` | `FAIL: … matches scripts/i18n-canon.js` |
| remove `i18n.js` from CMake SOURCES (O-Bitrot) | check-i18n `[8]` | `FAIL: … SOURCES block` |

Each mutation was applied to a byte-exact backup and restored **from that
backup**, never `git checkout --`, which would have wiped the uncommitted work
alongside the mutation.

---

## NOT VERIFIED — read this before deciding on Stage E

1. **Nothing has been checked in a real DAW.** Everything above is headless
   Chromium against each plugin's own committed ui-stub, `auval`, and three
   offline C++ harnesses.

2. **The C++ persistence path was never executed, on any of the three.**
   `uiLanguage` compiles and `auval` exercises `get/setStateInformation`, but
   nothing wrote `"fr"` into a session and read it back. The claim "close the
   DAW, reopen, still French" is **reasoned, not measured** — and it is now
   outstanding on all five localized plugins. On O-Bitrot and O-Tapestop the
   inference is stronger than elsewhere: the property sits beside
   `tooltipsEnabled`, which has shipped and worked since v1.12.0 / v1.4.0 and
   has its OWN round-trip probe in each render harness (both passed). But the
   language property itself was not round-tripped. **This is the
   highest-value manual check outstanding.**

3. **The native bridge was exercised only against the ui-stubs.** The real
   `Juce.getNativeFunction('getUiLanguage')` round trip through WKWebView has
   not run on any plugin.

4. **The Standalone `.app` is stale on all three.** `build-and-install.sh`
   builds VST3 + AU only.

5. **All 200 French entries across the five plugins are unreviewed machine
   drafts** — 31 + 55 + 35 added here. No native speaker has read any of them.
   `node scripts/check-i18n.js` prints the worklist. Terms most likely to want
   judgement: O-ReverseDelay `knob-duck` ("Atténuation dynamique"),
   `knob-regenMakeup` ("Regain"); O-Bitrot `ROT_ENABLE` ("Corruption"),
   `CRUSH_ENABLE` ("Écrasement"), `seedRo` ("Germe"); O-Tapestop
   `knob-TONE_TRACK` ("Suivi de timbre"), `engage-btn` ("Enclencher").

6. **No cross-platform check.** Windows/WebView2 has not been exercised on any
   of the three. The `getResource()` branches follow each plugin's existing
   pattern exactly, but that has not run.

7. **French geometry is proven at these three frames and O-Octagon's, and
   nowhere else.** Stages E–G cover 38 more plugins, none of which has a clamp
   gate. The three that do now sweep both languages permanently; the rest would
   not.

8. **The popover was proven clickable by render probe on all three**
   (`elementFromPoint` / real Playwright clicks, not markup inspection) — Stage
   B shipped one that was correct in markup and CSS and completely pointer-dead.
   But it was proven only at each shipping viewport in Chromium, not in a
   WebView.

9. **`PLUGINS.md` carries another session's uncommitted O-Octagon row**
   (`1.7.0-dev`). Deliberate — see deviation 5.

---

## Next

**This is the recommended partial-ship point**, and it is where this dispatch
stops. Stages E, F and G are not approved and were not started.

Five plugins localized. The pattern is proven under the strictest static gates
in the repo, the ui-stub / CMake / `getResource()` loop is closed on all five,
and **the French geometry risk is retired with measurements rather than
assumptions** on every frame that has a gate to measure it with.

Everything after Stage D is replication:

- **Stage E** — the remaining 9 `data-tip` plugins. Mechanical.
- **Stage F** — port the 7 `data-tooltip` plugins onto the measure-then-pin
  renderer. The natural feature boundary: ONE tooltip convention repo-wide.
- **Stage G** — author English for the 22 bare plugins, then localize.

Carry forward into Stage E:

- **Check every plugin's gates for wrong-shaped assumptions before assuming a
  failure is real.** Three instances now: O-Octagon's double-quote-only import
  scan (Stage C), `check-i18n`'s `js/app.js` assumption and O-ReverseDelay's
  D13 substring search (both Stage D). All three reported violations of rules
  the code was obeying.
- **`git commit -- <path>` does not stage untracked files.** Stage `i18n.js`
  explicitly, every time.
- **Check `PLUGINS.md` for other sessions' rows immediately before committing**,
  and hold them back rather than sweeping them in.
- O-Contrabass's bridge count `:123` goes 34 → 36, and it should be checked for
  the quote-style trap before that count is bumped.
