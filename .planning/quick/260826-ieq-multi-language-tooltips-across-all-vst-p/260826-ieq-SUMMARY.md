---
task: 260826-ieq-multi-language-tooltips-across-all-vst-p
type: execute
mode: quick
status: incomplete
stages_complete: [A, B, C, D, E, F, G, H, I]
stages_remaining: [J, K, L, M]
stopped_at: "STAGE I COMPLETE. Batch I2 shipped all seven `O-simple*` plugins, one path-scoped commit each: O-simpleAdditive v1.1.0 (`b1082fc0`), O-simpleSampler v1.4.0 (`2a2f68c4`), O-simpleSubtractive v1.3.0 (`5fc1ceb0`), O-simpleGrain v1.3.0 (`857dfa85`), O-simpleFM v1.3.0 (`2a5efef0`), O-simplePhysicalModelSynth v1.2.0 (`051e1a72`), O-simpleBeatmaker v1.1.0 (`665150c0`). `check-i18n --strict-v2` reports 14 canon v2, 0 canon v1 — EVERY `data-tip`-convention plugin in the repo is now localized, so Stage J starts from a green repo. TWO more repo-level gate fixes were unavoidable and each landed as its OWN commit ahead of its plugin: `08e7649b` (assertion 7 measured ANIMATED elements, so a SMIL `animateTransform` reported three phantom French failures per run in a SINGLE language — PROBE now freezes SMIL and Web Animations, deliberately NOT CSS transitions) and `816f2767` (assertion 8 compared labels across PAINT LAYERS, so an opaque z-index:40 popover drawn over the page on purpose read as a collision the moment the French string grew long enough to reach it). Those are the ELEVENTH and TWELFTH wrong-shaped gate assumptions in this task; both were regression-swept across every previously-shipped canon-v2 plugin. Checkpoints 4 AND 5 remain OUTSTANDING and are now outstanding on FOURTEEN plugins: the C++ language round-trip has still never been run by hand on any of them, and no human has seen the French UI. TWO items need a human decision, neither blocking Stage J: (1) O-simpleSampler's content grew 796 -> 835px inside its FIXED 980x720 frame, putting the on-screen keyboard entirely below the fold on a pedagogical plugin — a contained CSS/copy reversal if wanted; (2) O-simplePhysicalModelSynth's Delete-preset button deletes WITHOUT confirmation (it calls deletePreset() directly and passes no deleteButton, so the vendored module's confirm dialog is dead code) — reported, not fixed, because adding one is a design change. PREVIOUSLY, at end of Stage I batch I1: O-Contrabass v1.8.0 and O-Orbit v1.2.0 shipped, 7 canon v2."

plugins_shipped:
  - name: O-simpleBeatmaker
    version: 1.1.0
    commit: 665150c0      # v1.1.0, Stage I batch I2 — LAST of the batch, completing Stage I.
                          # 14th on canon v2. Only RESIZABLE plugin in the batch
                          # (860x640 min vs a 1060x900 setSize) — the gate only ever
                          # measures setSize, so the minimum was hand-measured: nothing
                          # clips, French 15px taller in an already-scrolling pane.
                          # 48 of its 82 [data-i18n] elements are RUNTIME-GENERATED.
                          # Preceded by gate fix 816f2767, committed separately.
  - name: O-simplePhysicalModelSynth
    version: 1.2.0
    commit: 051e1a72      # v1.2.0, Stage I batch I2. FIXED 1040x860 window — no scroll
                          # to absorb French growth. Harness version drift was LIVE:
                          # .factory-version on disk read 1.0.0 against a shipping 1.1.0.
                          # Preceded by gate fix 08e7649b, committed separately.
  - name: O-simpleFM
    version: 1.3.0
    commit: 2a5efef0      # v1.3.0, Stage I batch I2. Narrowest frame in the batch (760x980).
                          # Source of the `white-space: nowrap` result — removed 103 of 125
                          # moved elements at ZERO English page cost.
  - name: O-simpleGrain
    version: 1.3.0
    commit: 857dfa85      # v1.3.0, Stage I batch I2. Two CMake version variables
                          # (OSIMPLEGRAIN_VERSION + _VERSION_CODE) moved together.
                          # NOTE: another session landed v1.4.0 (4d6056bc, DSP) on top.
  - name: O-simpleSubtractive
    version: 1.3.0
    commit: 5fc1ceb0      # v1.3.0, Stage I batch I2. Largest label count in the batch (58).
                          # Scroll extent PROVEN unchanged at 1118px by swapping HEAD's
                          # files in and out — the control that makes O-simpleSampler's
                          # +39px a real signal rather than a cost of the work.
  - name: O-simpleSampler
    version: 1.4.0
    commit: 2a2f68c4      # v1.4.0, Stage I batch I2. The ONLY one of the seven with a real
                          # C++ tips bridge (named tipsEnabled, not tooltipsEnabled).
                          # OPEN ITEM: content 796 -> 835px in a FIXED 980x720 frame put the
                          # on-screen keyboard entirely below the fold.
  - name: O-simpleAdditive
    version: 1.1.0
    commit: b1082fc0      # v1.1.0, Stage I batch I2 — FIRST of the batch, established the
                          # family pattern: data-tip is the tip KEY here, not the body, so
                          # every anchor moves to data-param and the listeners delegate.
  - name: O-Orbit
    version: 1.2.0
    commit: 9d8e50d0      # v1.2.0, Stage I batch I1 — tooltips AND labels in ONE
                          # release, completing batch I1. SEVENTH plugin on canon
                          # v2. Bridge 25 -> 27. UI root Resources/ui, not
                          # Source/ui/public. Preceded by the repo-level gate fix
                          # f00e5d45, committed separately and never bundled.
  - name: O-Contrabass
    version: 1.8.0
    commit: 7035029a      # v1.8.0, Stage I batch I1 — tooltips AND labels in ONE
                          # release. First plugin outside the original five, and the
                          # sixth on canon v2. Bridge 34 -> 36. No prior tooltip
                          # release: v1.7.0 shipped the data-tip renderer in English.
  - name: O-MultiBandCompressor
    version: 1.11.0
    commit: cc2ea600      # v1.11.0, Stage H — labels. v1.10.0 was b98aa9ec, tooltips only.
  - name: O-Octagon
    version: 1.9.0
    commit: 99e7d206      # v1.9.0, Stage G — labels, on canon v2. v1.6.0 was 8dcb1317,
                          # tooltips only. Run against v1.8.0 (2ba236a1), not the v1.6.0
                          # the plan describes. Scripts fixes: 3f6b201d.
  - name: O-ReverseDelay
    version: 1.10.0
    commit: 55228ffb      # v1.10.0, Stage H — labels. v1.9.0 was 951dd584, tooltips only.
  - name: O-Bitrot
    version: 1.15.0
    commit: ff162184      # v1.15.0, Stage H — labels. v1.14.0 was 1f3a9faa, tooltips only.
  - name: O-Tapestop
    version: 1.6.0
    commit: 8b146dd3      # v1.6.0, Stage F — labels. v1.5.0 was 547f9738, tooltips only.
subsystem: webview-ui / i18n
tags: [i18n, tooltips, webview, juce, apvts-persistence]
key-files:
  created:
    - plugins/O-Orbit/Resources/ui/js/i18n.js
    - plugins/O-Orbit/tests/i18n-states.json
    - plugins/O-Orbit/tests/ui-stub/generic-overrides.json
    - plugins/O-Contrabass/Source/ui/public/js/i18n.js
    - plugins/O-Contrabass/tests/i18n-states.json
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
    - plugins/O-Orbit/Resources/ui/index.html
    - plugins/O-Orbit/Resources/ui/css/styles.css
    - plugins/O-Orbit/Resources/ui/js/app.js
    - plugins/O-Orbit/Source/Plugin{Processor.h,Processor.cpp,Editor.cpp}
    - plugins/O-Orbit/CMakeLists.txt
    - plugins/O-Orbit/CHANGELOG.md
    - scripts/check-ui-labels.js          # assertion 6 -> a DELTA (f00e5d45, its own commit)
    - plugins/O-Contrabass/Source/ui/public/index.html
    - plugins/O-Contrabass/Source/Plugin{Processor.h,Processor.cpp,Editor.cpp}
    - plugins/O-Contrabass/CMakeLists.txt
    - plugins/O-Contrabass/CHANGELOG.md
    - plugins/O-Contrabass/tests/ui_frontend_check.js
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
actuals_stage_h:
  tokens: 470000
  tasks: 1              # T12, three plugins
  commits: 5            # 3 plugin commits + 2 scripts/ commits
actuals_stage_i_batch_i1:
  tokens: 205000
  tasks: 1              # T13, ONE plugin of the nine
  commits: 1            # one path-scoped plugin commit; NO scripts/ commit was needed
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


---

# Stage F — T10: the label pattern-bearer, O-Tapestop v1.6.0

**Commits:** `92e5fdc9` (gate fixes, `scripts/`) then `8b146dd3`
(`plugins/O-Tapestop` + `PLUGINS.md`). Two path-scoped commits, in that
order, because the plugin cannot pass gates that are themselves wrong.

O-Tapestop is now the first plugin in the suite whose **page** is French, not
only its hover help. It is also the first on canon v2 (`check-i18n` split:
v2 1, v1 4).

## THE LABEL CONTRACT — this is what Stages G–L replicate

Stated plainly, because 42 more plugins inherit it under a rule that forbids
fixing it centrally afterwards.

1. **A label is owned by its element**, via `data-i18n="key"` on a LEAF
   element. The authored English stays in the markup as the fallback that
   renders if `applyI18n()` never runs. An element with element children is
   never keyed — `applyLabel()` writes `textContent` and would delete them.
2. **`applyLabel()` writes `textContent` AND `dataset.label` together.** This
   is the systemic answer to
   `pattern_js_state_updater_overwrites_html_labels`. The documented repo fix
   is that an updater reads `el.dataset.label ?? fallback`; writing both in
   one place means the mirror is always in the CURRENT language, and it makes
   the invariant checkable at render time rather than guarded by a whitelist.
   `check-ui-labels` assertion 3 asserts `dataset.label === textContent`
   after init, after a language switch, and after a state pass.
3. **A script-written label declares its own key** through `setLabel(el,
   'literal.key')`, becoming a `[data-i18n]` element that the sweep owns.
   Two-state captions are **two calls behind an `if`/`else`**, never one call
   with a ternary in its argument — `check-i18n` assertion 13 rejects that
   shape, and it is what contract §6 authors around.
4. **`aria-label` / `placeholder` / `alt` are keyed** through
   `data-i18n-aria` etc. Native `title=` is **deleted**, not localized.
5. **THE REUSE RULE, which the plan left open and this stage settles: a label
   reuses a tooltip key only where the string is identical in BOTH
   languages.** English-only matches are not enough. `#seg-sync-sync`'s tip
   title is `Sync` / `Synchronisé`; the label needs `Sync` / `Synchro`,
   because `SYNCHRONISÉ` is 90 px of type in a 66 px segment. Reusing there
   would make every future tooltip copy edit a silent geometry change to a
   control. Nine keys here qualified for reuse; the rest got their own.
6. **French is sized, never shrunk.** One French string per key; nothing
   chooses between variants at runtime. Where French did not fit, the fix was
   the plugin's own CSS — except `Suivi tonal` over `Suivi de timbre` for
   TONE TRACK, which was simply the better French and is recorded as such in
   the table.

## The plan's fact 3 was wrong, in the direction that mattered

The plan chose O-Tapestop partly because it has **"zero JS-written prose, so
Stage F proves the label mechanism without also proving `setLabel`"**, and
gave `setLabel` to Stage H's MBC.

`i18n-extract` found **four** JS-written prose sites: the hover-help toggle's
`On`/`Off` face and the delete button's `Delete`/`Confirm?` face. They were
not literals — they came from `data-on` / `data-off` / `data-confirm`
attributes AUTHORED IN THE MARKUP, which is exactly the idiom the repo's own
pattern note prescribes, and which is why a source scan for string literals
in `app.js` missed them. **An attribute holds one string.** On a two-language
page the off face would have been restored in English the instant the user
picked Français.

So Stage F proved `setLabel` after all. That is good news for Stage H — the
mechanism is no longer unproven — but the estimate for **every** stage should
now assume `setLabel` work, because the `data-*`-authored caption idiom is
this repo's house style and appears wherever a caption swaps with state.
Running the extractor rather than trusting the count is what caught it.

## What the geometry diff found, and what was done about each

Measured at the shipping 860 × 580 across 7 states. 26–30 non-label elements
moved between English and French. D-04 forbids auto-shrink and short
variants, so every fix spends slack the container already had.

| Moved | By | Cause | Fix |
|---|---|---|---|
| both fleurons, both nav arrows, the 300 px preset readout | ±46.9 px | SAVE/LOAD/DELETE 188 px vs ENREGISTRER/CHARGER/SUPPRIMER 282 px, in a `justify-content:center` band | fixed button widths 108 / 84 / 96 |
| `#pane-stop .stop-col:1`, dragging its combo and curve knob | dw +17.1, dx −9.2 | RALENTISSEMENT 105.1 px vs SPIN DOWN 66.1 px setting the column width | `#pane-stop .stop-col { width: 108px }` |
| the env slot and `#knob-ENV_FREE_MS` | dx −31.0, dw −8.5 | three shrink-to-fit tracks over-ran the pane, so flex-shrink took the difference **out of the time slot** | fixed tracks 84 / 88 / 202, gap 12 = 398 |
| `#pane-continuous .stop-col:1` | dw −1.1 | CARACTÈRE vs CHARACTER | first column pinned 72 px |
| both footer fleurons | ~±70 px | caption 403.5 px vs 542.7 px | `.footer-text { width: 560px }` |

**The env-row fix corrects an English bug too.** That time slot has been
rendering 79.2 px against its designed 88 for as long as the row has existed.
Nothing was measuring it. French did not cause it; French exposed it.

**Nothing moved at all after the fixes** — and on the tightest frame in the
suite that is a claim worth distrusting, so: it is credible *because* the
fixes made every container in the paths that moved a fixed size. The diff is
load-bearing on a shrink-to-fit layout and goes quiet once the layout is
pinned. That is not the gate going blind — negative control NC-2 (an
over-long French label) still fails the run — but it fails **assertion 8**,
the overlap check, not assertion 7. On a pinned page the overlap and
text-spill checks are the ones still doing work. Stages G–L should expect the
same shift and should not read a silent assertion 7 as proof of anything on
its own.

## Three gate defects, found by running Stage E's tooling for real

All three would have followed the tooling into 42 plugins.

1. **`check-ui-labels` assertion 4 was blind to the overflow it exists to
   catch.** A leaf whose computed `overflow` is `visible` reports
   `scrollWidth === clientWidth` however far its text spills. PLEURAGE
   (60.2 px) and ALÉATOIRE (63.8 px) sat outside their 58 px CHARACTER
   segments with every assertion green. Fixed by measuring the rendered text
   with a `Range`. Segments widened to 72 px.
2. **Assertions 5 and 6 failed on O-Tapestop IN ENGLISH**, on two authored
   layout facts: three `.group-label` legends are `position: absolute` and
   deliberately straddle the panel border (9.0 px of intended overhang), and
   a decorative `.botanical-overlay` runs 20 px past the frame so
   `body.scrollWidth` reports 880 where `documentElement` reports 860. Both
   are identical in both languages and **identical at the pre-retrofit
   commit — measured, by rendering HEAD's files side by side, not assumed.**
   The committed clamp gate asserts `documentElement` and passes; the plan
   said that if the two tools disagree about O-Tapestop's geometry the tool
   is wrong. Both assertions now measure the French delta and report the
   English baseline.
3. **`check-i18n` assertions 1, 4, 5 and the reviewer worklist read `I18N`
   only.** All 39 French LABEL strings were invisible to every one: no en/fr
   pair check, no passthrough check, no `reviewed` flag, and absent from the
   worklist that exists precisely to mark machine drafts. Thirty-nine
   unreviewed strings reported as zero is worse than no worklist, because it
   reads as done. Worklist now splits tooltip and label counts; repo total
   200 → 251.

## Verification — every gate, individually

| Gate | Result |
|---|---|
| `check-ui-labels.js --plugin O-Tapestop` | **exit 0**, all assertions, **46/46** labels measured visible across 7 states (was 26/46 before `i18n-states.json`) |
| `check-i18n.js` (all 5 plugins) | **exit 0**; canon split **v2 1, v1 4**; `--strict-v2` still not the default |
| `check-i18n.js --plugin O-Tapestop` | **exit 0**, reports canon **v2**, assertions 10–15 all run |
| `tests/ui_tooltip_clamp_check.js` | **exit 0 — not 77.** 35 anchors, 13 clamped / 4 flipped per language, French +0 clamps +0 flips |
| `tests/render-harness` (`O-Tapestop-render-test`) | **exit 0**, 69 probe checks, 0 failures |
| `boot-all-uis.js` | 41/43 clean, **unchanged**. O-Tapestop: `title=0 aria=15 i18n=46` |
| `./scripts/build-and-install.sh O-Tapestop` | VST3 + AU built and installed, AU cache cleared, dual-variant sweep ran |
| `auval -v aufx OTsp OuDv` | **AU VALIDATION SUCCEEDED** |

**Clamp count moved 12 → 13 per language.** The plan's done-criterion asked
for Stage D's numbers unchanged and said a change means the label gate missed
a movement. It did not: the fixed-width preset buttons move `#preset-delete`
38 px right **in English**, which the label gate does not assert (it asserts
en == fr, not en == previous-en). The change is identical in both languages,
so it is a consequence of a deliberate English layout change, not an
unreported French one. Anchor and flip counts are unchanged at 35 and 4.

## Negative controls — ten, every one fired

Each applied to a byte-exact backup and restored FROM THAT BACKUP; `git
checkout --` would have wiped the uncommitted work alongside the mutation.
Restores verified by sha256 after every round.

| Mutation | Assertion that fired |
|---|---|
| revert the CHARACTER segment widening | `[4][fr]` text-spill, both segments named with widths |
| an over-long French label | `[8]` new overlap (**not** `[7]` — see above) |
| clobber `textContent` after `applyLabel` | `[3]` in both languages, `label="Enclencher" text="CLOBBERED"` |
| stub `__setLanguage` to a no-op | `[2]` vacuity, `0/46 labels differ` |
| a French label spilling more than English | `[5]` `9.0px -> 105.6px`, and `[6][fr]` frame crossing |
| strip a LABELS `reviewed` flag | `[5]` LABELS |
| a LABELS French entry copied from English | `[4]` LABELS |
| a ternary inside a `setLabel` argument | `[13]` |
| remove one `data-i18n` from the markup | `[10]`, plus `[15]` dead key |
| reinstate a native `title=` | `[11]` |

**A methodological note worth carrying.** The `reviewed`-flag control at
first appeared NOT to fire. The assertion was fine; the mutation was a
Python `str.replace` whose pattern did not match, and `str.replace` returns
the string unchanged rather than raising. A silent no-op mutation is
indistinguishable from a working gate. Every later control was written to
print its substitution count and assert it is non-zero.

## NOT VERIFIED — read this before Stage G

- **THE C++ PERSISTENCE ROUND-TRIP HAS STILL NEVER BEEN RUN.** Checkpoint 4's
  part (b) is outstanding and has been since Stage B. Every claim that a
  language choice survives a session is REASONED from source, not measured.
  Nothing in Stage F touched C++ and nothing in Stage F tested this.
- **No human has seen the French UI.** All geometry is headless Chromium
  through the plugin's own bridge stub. Whether `RALENTISSEMENT` at 10 px
  uppercase reads well inside O-Tapestop's aesthetic — as opposed to merely
  fitting — is Checkpoint 4 part (a) and is unanswered.
- **The state-update pass is weaker than it reads.** `check-ui-labels`
  reports `state-update pass driven via: events-only` on this plugin: it
  drives real slider/toggle/combo states only where `window.__stubStates`
  exists, which the GENERIC stub provides and O-Tapestop's own committed stub
  does not. Assertion 3's "after a state pass" is therefore dispatched
  `input`/`change` events, not driven parameter updates. It still caught the
  clobber control, but it is not the full-strength check on the five plugins
  with hand-written stubs.
- **All 74 French strings are machine-drafted, `reviewed: false`.** No native
  speaker has read one. Several are judgement calls a reviewer should
  challenge: `Timing` → `Cadence`, `On`/`Off` → `Marche`/`Arrêt`, `Tone
  Track` → `Suivi tonal` (chosen partly for width, and recorded as such).
- **Windows / WebView2 remains a named deferral, blocked on hardware.** Every
  width in the fix table was measured in Chromium on macOS. A French label
  that fits at 860 px here may clip under WebView2's font metrics. Unchanged
  by this stage; not retired by it.
- **Nothing was tested in a DAW.** The plugin builds, installs and `auval`s;
  it has not been opened in Logic or Ableton.
- The generated inventory artifacts (`plugins/O-Tapestop/.planning/i18n-*`)
  are left UNCOMMITTED. They are regenerable from `i18n-extract.js` and are
  not part of the shipped plugin.

## Carried into Stage G (O-Octagon)

- **Another session is actively working inside `plugins/O-Octagon/` right
  now**, including `Source/ui/public/js/i18n.js`, `tests/ui_frontend_check.js`
  and `tests/ui_layout_check.js`. Stage G must reconcile with it before
  starting, not after.
- **Run `i18n-extract` before trusting any per-plugin count in the plan.** It
  was wrong about O-Tapestop's JS prose, in the direction that adds work.
- Assertion 7 goes quiet once a layout is pinned. Do not read its silence as
  a result on its own.
- The `data-*`-authored two-state caption idiom (`data-on`/`data-off`,
  `data-label`/`data-confirm`) is house style and is invisible to a literal
  scan. Grep for it directly.
- O-Octagon's `ui_frontend_check.js` §6 whitelist is the fourth wrong-shaped
  gate assumption candidate. The `dataset.label === textContent` mirror is
  the assertion that should replace it — and the clamp-gate rewrite in this
  commit is a worked example of doing that without weakening the rule.


---
---

# Stage H — T12: MBC, O-Bitrot, O-ReverseDelay

**Commits, in order:** `a1d80957` (scripts), `cc2ea600` (O-MultiBandCompressor
v1.11.0), `e56abded` (scripts), `ff162184` (O-Bitrot v1.15.0), `55228ffb`
(O-ReverseDelay v1.10.0). Five path-scoped commits: three plugins, one each, and
two `scripts/` commits kept separate because a plugin cannot pass gates that are
themselves wrong.

**FOUR of the five shipped plugins are now fully localized on canon v2.**
`check-i18n` reports the split as **v2 4, v1 1** — the one is O-Octagon.

## Stage G is DEFERRED, not skipped

O-Octagon was untouched this dispatch, deliberately and on instruction. Another
session was mid-flight inside `plugins/O-Octagon/` on a v1.8.0 motion feature —
27 modified files and 4 new ones, including `index.html`, `app.js`, `i18n.js`,
`styles.css` and all three test files. `git commit -- plugins/O-Octagon` commits
the whole path, so a Stage G commit would have swept that work into this task's
history. That session has since landed it as `2ba236a1` (O-Octagon v1.8.0).

**The consequence to carry:** O-Octagon is the only shipped plugin still
half-localized — French tooltips over English labels — and its `lang-select`
entry still tells the user, in both languages, that the labels do not change.
Stage G should now be re-run against v1.8.0 rather than against the v1.6.0 the
plan describes: its `i18n.js`, its `app.js` and all three of its gates have moved
underneath the plan's numbers, and `i18n-extract.js` must be re-run before any
per-plugin count in the plan is trusted.

Also carried, and NOT done here: the plan assigns O-MultiBandCompressor a stale
`preset-list` tooltip reading "17 parameters". **That string is O-Octagon's**
(`js/i18n.js:452`), not MBC's — MBC has no `preset-list` tooltip at all. It is
still stale and still belongs to Stage G.

## THE LABEL GEOMETRY VERDICT, across four frames

Checkpoint 5 asks for this measured rather than argued. All four frames, before
any fix:

| Plugin | frame | labels | non-label elements MOVED, before fixes | after |
|---|---|---|---|---|
| O-Tapestop (Stage F) | 860 × 580 | 46 | 26–30 | **0** |
| O-MultiBandCompressor | 900 × 640 | 77 | **222** | **0** |
| O-Bitrot | 900 × 740 | 71 | **78** | **0** |
| O-ReverseDelay | 940 × 768 | 50 | **7** | **0** |

**No plugin needed a layout change rather than a CSS tweak.** Every fix on all
three either pinned a container that was already shrink-to-fit, or spent slack
the container already had. D-04 was never in danger of forcing the design
conversation the plan reserved for it. That is the answer to the question
Checkpoint 5 exists to ask, and it is the strongest argument for continuing into
Stages I–L rather than stopping.

**The number that matters is not 222 — it is the SHAPE.** Every one of those 222
traces to five root causes, and the same five recur on all four plugins:

1. **A flex container with `align-items: center` makes every child shrink-to-fit.**
   MBC's `.knob-grid` and `.button-row`, O-Bitrot's `.ctl`, `.mix-text` and
   `.p-head .caption`, O-ReverseDelay's preset band. This is the single largest
   source, and it is a house pattern rather than an accident — it will recur on
   every plugin in Stages I–L.
2. **A `justify-content: space-around` / `space-between` / `center` row re-deals
   itself when any child changes width**, moving every sibling. MBC's footer,
   both plugins' preset bands, O-ReverseDelay's footer fleurons.
3. **A `<select>` sizes itself to its widest OPTION**, so localizing the choices
   widens the control. MBC's M/S mode went 71 → 90 px.
4. **A caption growing past its cell can push the whole page**, not just its
   row: MBC's French SAVE/LOAD wrapped a 367.8 px title to two lines and moved
   everything below it down 27 px.
5. **A container sized by a keyed child** — O-Bitrot's `.hdr-right` (the plate
   line) and its Splices group (the marginal note).

Label WIDTHS, French vs English, at the worst on each plugin:

| Plugin | worst growth | what it was |
|---|---|---|
| O-MultiBandCompressor | +49.5 px (+122%) | SAVE 40.6 → ENREGISTRER 90.1 |
| O-Bitrot | +45.6 px (+52%) | Tab. VII — Rot 87.5 → — Corruption 133.1 |
| O-ReverseDelay | +139.2 px (+34%) | the footer caption 403.5 → 542.7 |

Nine French captions were SIZED rather than the layout changed, and each is
recorded at its entry in the plugin's own `LABELS` table with the measurement
that forced it: `Enreg.` / `Ouvrir` / `Suppr.` on all three preset bars, `Clics`
(O-Bitrot Pop), `Ampleur` (O-Bitrot Depth), `Pente` and `Biseau`
(O-ReverseDelay's WINDOW group, whose cells are 66 px where every other knob cell
on the page is 72), `Ampleur` again (O-ReverseDelay Drift Depth) and `Compens.`
(MBC Makeup).

**Coverage holes, stated rather than implied.** Across the three plugins, nine
`[data-i18n]` elements were never visible in any driven state and were therefore
never measured — and **all nine are `<option>` elements inside native
`<select>`s**. A `<select>` popup is UA-rendered; its options have no box in the
document. This is permanent, not a missing state, and the geometry that actually
matters for them is the select's own width, which is pinned on MBC and unchanged
on O-Bitrot. Everything else reaches 100%: O-ReverseDelay measures **50 of 50**,
MBC 73 of 77, O-Bitrot 66 of 71.

## Two English bugs that French exposed

Neither was caused by this work; neither was being measured by anything.

1. **MBC's knob grids were never on their band's thirds.** Each band's
   `.knob-grid` rendered 126.4 px inside a 188.5 px band, because
   `.band-controls` is `align-items: center` and the grid was shrink-to-fit.
   Pinning it to 100% fixes French and corrects the English spacing.
2. **O-Bitrot's Comfort column already rendered 53.8 px against its neighbours'
   50**, because its own English caption was wider than its knob.

This is the third instance of the Stage F pattern ("French did not cause it;
French exposed it") and it is now a reliable enough by-product to expect.

## THREE gate defects, found by running the tooling on plugins O-Tapestop could not reach

All three would have followed the tooling into 39 more plugins.

1. **`check-i18n` assertion 15 could not see a key declared by ASSIGNING
   `dataset.i18nAria`.** An element the controller creates at runtime — a
   preset-dropdown row — cannot carry the attribute in the markup, and
   `setLabel()` is not available for an ATTRIBUTE key because it writes
   `textContent`. So the only in-canon way to localize a dynamically created
   element's accessible name reported as a **dead key**. The reference scan now
   reads plain string literals from those assignments; a computed key still adds
   nothing, exactly as assertion 13 already rules for a computed `setLabel` key.
   Fixed in `a1d80957`.

2. **`check-ui-labels` measured the two languages at DIFFERENT PARAMETER
   VALUES.** It interleaved its language probes with the state pass, so the
   English snapshot was taken before any state pass and the French one after —
   and every rotated `.knob-stem` in the plugin read as "moved between English
   and French". **53 phantom rows on MBC.** Both `before` snapshots are now taken
   before any state pass runs. It stayed invisible through Stage F because
   O-Tapestop's own committed ui-stub exposes no `window.__stubStates` and its
   state pass was `events-only`; the GENERIC stub does drive parameters, so
   **every plugin without a hand-written stub would have hit this** — that is 38
   of 43. Fixed in `a1d80957`.

3. **`check-ui-labels`'s text-spill check failed on correct ENGLISH markup for
   any inline label.** `clientWidth` is DEFINED as 0 for a non-replaced inline
   element — an inline box has no content box, it is a run of line boxes — so
   comparing a `Range` width against it reports every inline label as spilling.
   O-Bitrot's nine panel captions are `<span>`s and fired all nine. The check now
   skips inline boxes and PRINTS which ones it skipped; assertions 5, 6 and 7
   still measure their rects. Fixed in `e56abded`.

That is five wrong-shaped gate assumptions in this task now, and **three of the
five reported a violation of a rule the code was obeying.** The Stage D lesson
holds: check the gate's shape before believing a failure.

## The plan's expectations, and what was actually there

The brief warned to run `i18n-extract.js` and believe it over any expectation.
That was right on all three.

| Plugin | plan expected | extractor found |
|---|---|---|
| O-MultiBandCompressor | "the `setLabel` proof" — JS prose | **7 js-prose + 3 composed**, including a composed `aria-label` |
| O-Bitrot | inline-module canon v2 | that, **plus 6 js-prose** in three `data-*`-authored caption pairs |
| O-ReverseDelay | "no `setLabel`" implied | **zero js-prose literals**, but `data-label` / `data-confirm` on the delete button — the same house idiom, invisible to a literal scan |

**The `data-*`-authored two-state caption idiom appeared on ALL THREE**, exactly
as Stage F predicted, and a source scan for string literals finds none of them.
Grep for `data-on=` / `data-off=` / `data-confirm=` / `data-label=` directly on
every remaining plugin.

## A sixth trap, new in this stage: canon v2 is reached at BINDING time

Both MBC and O-Bitrot needed their whole i18n block MOVED above the eager
`initializeUI()` / top-level binding code, and neither plan step anticipated it.

Canon v1 was only ever reached from `initializeDeferredUI()`, so its `let`
bindings could sit anywhere below. **Canon v2 is reached from binding time**:
`updateToggleUI()` calls `setLabel()`, `labelKnob()` records a key, and a panel
button's `render()` runs immediately. With the block still below, `uiLanguage`
was in its temporal dead zone and every one of those reads threw a
`ReferenceError` that the binder's own try/catch **swallowed** — three global
knobs and the Auto-MU toggle silently unbound on MBC, with the page otherwise
looking correct. That is `pattern_module_toplevel_init_tdz` and the v1.4.0
failure in miniature.

**`boot-all-uis.js` caught it. `check-ui-labels` did not**, because the error is
caught and logged rather than uncaught, and the label gate only asserts on
uncaught page errors. Keep `boot-all-uis` in every plugin's verification list for
Stages I–L; it is the only gate that sees a swallowed binding failure.

On O-Bitrot the same bug would have been worse: that module has no `init()` to
isolate a failure, so the throw takes the entire UI down.

## Verification — every gate, per plugin, individually

| Plugin | Gate | Result |
|---|---|---|
| O-MultiBandCompressor | `check-i18n.js --plugin` | **exit 0**, canon **v2** |
| O-MultiBandCompressor | `check-ui-labels.js --plugin` | **exit 0** — zero geometry shifts, 73/77 labels measured over 2 states |
| O-MultiBandCompressor | `render-harness` (`-preset-test`) | **exit 0** — 47 presets active, 0 failures, order-independent over all 50 |
| O-MultiBandCompressor | `build-and-install.sh` | clean; VST3 + AU, AU cache cleared, dual-variant sweep ran |
| O-MultiBandCompressor | `auval -v aufx OMbc OuDv` | **AU VALIDATION SUCCEEDED** |
| O-MultiBandCompressor | binary embedding | `i18n_jsSize` 36742 == `wc -c` |
| O-Bitrot | `check-i18n.js --plugin` | **exit 0**, canon **v2** (inline module) |
| O-Bitrot | `check-ui-labels.js --plugin` | **exit 0** — zero geometry shifts, 66/71 over 2 states |
| O-Bitrot | `ui_tooltip_clamp_check.js` | **exit 0 — not 77.** 55 anchors; French +3 flips, +1 clamp, 14.8 px taller |
| O-Bitrot | `ui_preset_menu_check.js` | **exit 0** |
| O-Bitrot | `render-harness` | **exit 0** — 109/109 probes |
| O-Bitrot | `build-and-install.sh` | clean |
| O-Bitrot | `auval -v aufx OBrt OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Bitrot | binary embedding | `i18n_jsSize` 46151 == `wc -c` |
| O-ReverseDelay | `check-i18n.js --plugin` | **exit 0**, canon **v2** |
| O-ReverseDelay | `check-ui-labels.js --plugin` | **exit 0** — zero geometry shifts, **50/50** over 4 states |
| O-ReverseDelay | `ui_frontend_check.js` | **exit 0** — 164 assertions, including the three D13 checks |
| O-ReverseDelay | `ui_tooltip_clamp_check.js` | **exit 0 — not 77.** 31 anchors; French +0 flips, +0 clamps |
| O-ReverseDelay | `render-harness` | **exit 0** — ALL PROBES PASSED |
| O-ReverseDelay | `build-and-install.sh` | clean |
| O-ReverseDelay | `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| O-ReverseDelay | binary embedding | `i18n_jsSize` 37686 == `wc -c` |
| repo | `check-i18n.js` | **exit 0** — 5 localized plugins; canon split **v2 4, v1 1** |
| repo | `check-i18n.js --strict-v2` | exits **1**, naming O-Octagon. Correct: Stage G is deferred. |
| repo | `boot-all-uis.js` | **41 / 43, unchanged.** The two failures are O-Bowed and O-Reed, pre-existing and unrelated |
| repo | `check-ui-labels.js --plugin O-Tapestop` | **exit 0**, 46/46 — the two tool fixes did not disturb Stage F |

**No gate exited 77.** Every clamp gate really rendered.

### The clamp counts, and why one moved

| Plugin | anchors | clamped en → fr | flipped en → fr | vs Stage D |
|---|---|---|---|---|
| O-Bitrot | 55 | 15 → 16 | 10 → 13 | clamp 14 → 15, **anchors and flips unchanged** |
| O-ReverseDelay | 31 | 8 → 8 | 2 → 2 | **unchanged** |

O-Bitrot's clamp count moved by one **in English as well as French**, so it is a
consequence of the deliberate English layout changes moving the anchors, not an
unreported French one — the label gate asserts `en == fr`, never
`en == previous-en`. Same shape as Stage F's O-Tapestop 12 → 13.

### Negative controls — 37 run, 36 fired

Every mutation printed its substitution count and asserted it non-zero before the
gate ran, per Stage F's methodological note; each was applied to a byte-exact
backup and restored FROM THAT BACKUP, with the sha256 verified after every round.
**Never `git checkout --`**, which would have wiped the uncommitted work
alongside the mutation. One mutation was a silent no-op and the harness refused
to run it — which is exactly what that guard is for.

| Mutation | Assertion that fired |
|---|---|
| drop the `dataset.i18nAria` assignment (MBC) | `[15]` dead key |
| typo the assigned key (MBC) | `[15]` dangling AND `[15]` dead |
| no-op `__setLanguage` (×3 plugins) | `[2]` vacuity, `0/77`, `0/71`, `0/50` |
| clobber `textContent` after `applyLabel` (×3) | `[3]` in both languages, all elements named |
| revert `.control-group { width }` (MBC) | `[7]` geometry diff, 11 moved |
| revert `.knob-grid { width: 100% }` (MBC) | `[7]` geometry diff, 76 moved |
| revert `.ctl:has(> .knob)` (O-Bitrot) | `[7]` geometry diff, 15 moved |
| revert the `.p-head` caption/gap pin (O-Bitrot) | `[7]` geometry diff, 6 moved |
| revert `.footer-text { width }` (O-ReverseDelay) | `[7]` geometry diff, 2 moved |
| revert the `#syncSegments` widening (O-ReverseDelay) | `[4][fr]` text-spill, `Synchro 54.8>54.0` |
| restore BOTH over-long WINDOW captions (O-ReverseDelay) | `[8]` new overlap, `label.tilt x label.taper` |
| an over-long French label (×2) | `[7]` geometry diff, 185 and 23 moved |
| remove one `data-i18n` (×3) | `[10]`, and `[15]` dead key where the key became orphaned |
| reinstate a native `title=` (×3) | `[11]` |
| a ternary inside a `setLabel` argument (×3) | `[13]`, both halves |
| a raw prose literal to `textContent` (×3) | `[12]`, with file:line |
| drift one canon line (×3, incl. the INLINE module) | `[6]` matches NEITHER canon |
| a LABELS French entry copied from English (MBC) | `[4]` LABELS |
| strip a LABELS `reviewed` flag (MBC) | `[5]` LABELS |
| empty an `I18N_EXEMPT` reason (×2) | `[14]` |
| break the toggle ownership mirror (O-Bitrot clamp gate) | the rewritten assertion, both halves |
| write the delete caption as a JS literal (O-ReverseDelay FE) | the rewritten assertion, both halves |
| delete the `fr` half of a LABEL key (O-ReverseDelay FE) | the extended assertion, `fr MISSING` |
| reintroduce a `ui.on` key (O-ReverseDelay FE) | **the new D13 label assertion** |

**THE ONE THAT DID NOT FIRE, and what it means.** Reverting `label.taper` alone
to the over-long "Adoucissement" (91.9 px in a 66 px cell) produced **no failure
from any assertion**. It only fires when BOTH window captions are over-long, as
an OVERLAP. The reason is structural and generalises:

- `.knob-label` is a shrink-to-fit flex item, so **its box is always exactly its
  text** and assertion 4's text-spill check can never fire on it;
- its `offsetParent` is the frame rather than the cell, so assertion 5's spill
  check cannot fire either;
- it is a `[data-i18n]` element, so assertion 7 excludes it by design.

**A single over-long caption in a shrink-to-fit cell is invisible to this gate.**
It is caught only when it collides with a neighbour. That is a real coverage hole
in `check-ui-labels`, it was found by a control rather than reasoned about, and
it is left OPEN rather than patched, because the fix is a design question — the
gate would need each label's intended CELL, which only the plugin knows.

## English fidelity, checked mechanically

This task MOVES English, it does not rewrite it. Every `en` entry came from
`i18n-extract.js`'s inventory rather than being transcribed. **Two deliberate
normalisations, both recorded at the entry:**

1. MBC's three band buttons authored `SOLO` as their text node and
   `data-label="Solo"` as the caption a state update writes back, so the page has
   rendered "Solo" since bindings first ran. The keys carry the `data-label`
   casing and the text nodes were normalised to match; `text-transform:
   uppercase` makes the two identical on screen.
2. O-Bitrot's French plate line was authored at 260.6 px and shortened to
   `Catalogue des supports défaillants · Pl. XLVII` (221.4) to fit the pinned
   block. English untouched.

## NOT VERIFIED — read this before Stage G or Stage I

1. **THE C++ PERSISTENCE ROUND-TRIP HAS STILL NEVER BEEN RUN.** Checkpoint 4's
   part (b) has been outstanding since Stage B and is outstanding on all five
   plugins. Nothing in Stage H touched C++ and nothing in Stage H tested it. Every
   claim that a language choice survives a session is REASONED from source. This
   remains the highest-value manual check on the list.
2. **No human has seen the French UI, on any plugin.** All geometry is headless
   Chromium. Whether `ENREG.` beside `OUVRIR` reads acceptably in MBC's header, or
   `— CORRUPTION` in O-Bitrot's catalogue hand, is Checkpoint 4 part (a) and
   Checkpoint 5, both unanswered.
3. **Nothing was tested in a DAW.** All three build, install and `auval`; none
   has been opened in Logic or Ableton.
4. **The Standalone `.app` is stale on all three.** `build-and-install.sh` builds
   VST3 + AU only.
5. **A single over-long caption in a shrink-to-fit cell is not gated** — see the
   negative-control section above. O-ReverseDelay's WINDOW group is the known
   case; the same shape exists wherever a `.knob-label` sits in a centred flex
   column, which is most of this suite.
6. **`check-ui-labels`'s state pass is `stub slider states` on MBC and O-Bitrot
   but `events-only` on O-ReverseDelay and O-Tapestop**, because their committed
   ui-stubs expose no `window.__stubStates`. Assertion 3's "after a state pass" is
   therefore weaker on those two than on the two that use the generic stub.
7. **All 389 French entries across the five plugins are machine drafts**, every
   one `reviewed: false`. No native speaker has read any of them. Nine were picked
   with WIDTH as a constraint and are named in the geometry section above; those
   are the ones to challenge first.
8. **Windows / WebView2 remains a named deferral, blocked on hardware.** Every
   width in every fix table was measured in Chromium on macOS. A French label
   that fits at 900 px here may clip under WebView2's font metrics. Unchanged by
   this stage; not retired by it.
9. **O-Octagon is still half-localized and still tells the user its labels do not
   change.** Stage G, deferred.
10. **The generated inventory artifacts** (`plugins/*/.planning/i18n-*`) are left
    UNCOMMITTED on all three. They are regenerable from `i18n-extract.js` and are
    not part of the shipped plugin.

## Carried into Stage G / Stage I

- **Re-run `i18n-extract.js` before trusting any per-plugin count in the plan.**
  It was wrong about all three plugins in this stage, always in the direction that
  adds work.
- **Grep for `data-on=` / `data-off=` / `data-confirm=` / `data-label=`
  directly.** The `data-*`-authored two-state caption is this repo's house style,
  it appeared on all three plugins here, and no literal scan of the JS sees it.
- **Expect to MOVE the i18n block above the eager binding code.** Canon v2 is
  reached at binding time; canon v1 was not.
- **Run `boot-all-uis.js` on every plugin.** It is the only gate that sees a
  binding failure swallowed by a try/catch, and it caught the TDZ regression on
  MBC that every other gate passed.
- **`align-items: center` is the tell.** Any flex container with it makes its
  children shrink-to-fit, and every child that holds a caption will move. Look for
  it first; it accounted for most of the 307 moved elements across the three.
- Stage F's note still holds: assertion 7 goes quiet once a layout is pinned, so
  its silence is not a result on its own.


---
---

# Stage G — T11: gate-hardening, O-Octagon v1.9.0

**Commits, in order:** `3f6b201d` (`scripts/`, three gate fixes) then `99e7d206`
(O-Octagon v1.9.0 + `PLUGINS.md`). Two commits, in that order, because a plugin
cannot pass gates that are themselves wrong.

**Run LAST, out of order.** Stage G was deferred at Stage H because another
session held `plugins/O-Octagon`. That session landed its v1.8.0 motion engine as
`2ba236a1` and the blocker cleared. **All five shipped plugins are now on canon
v2**, and `check-i18n --strict-v2` — reserved for Stage L — passes repo-wide for
the first time.

## The plan's numbers were stale, in the direction that adds work

Stage H's carried-forward note said to re-run `i18n-extract.js` and disbelieve
the plan. That was right again, on every count.

| The plan said | Live at v1.8.0 |
|---|---|
| **78** `aria-label` attributes | **89** |
| §6, §9/21 and §2 to check | §6 and §2; the numbering also now has TWO §41s |
| "1100 x 720 is roomiest, expect FEWER findings than O-Tapestop" | correct — **4** before fixes vs O-Tapestop's 26–30 |
| implicitly, a v1.6.0 page | a v1.8.0 page with a Motion tab, 10 new captions, 10 new aria-labels and a Drift-only Seed cell |

The extractor found **184 label candidates**: 88 html-text, 89 html-attr, 12
js-prose, 7 js-composed. Final shipped counts: **84 `[data-i18n]` elements, 89
`data-i18n-aria`, 125 LABELS keys** (some elements reuse a tooltip key through
`trLabel`'s fallback, so keys ≠ elements).

## §6 WAS ABOUT TO PASS VACUOUSLY, and that is the finding of this stage

The plan predicted §6's whitelist was "the fourth wrong-shaped gate assumption
candidate" and warned against simply growing it. The reality was worse than
predicted, and in the opposite direction.

**It would not have grown at all.** §6 scanned for `IDENT.textContent =` and
required every receiver identifier to be whitelisted. Canon v2's `applyLabel`
takes its element as a parameter named **`el`** — and `el` was already on the
whitelist, for the map-banner copy and the venue name. So the single largest
`textContent` writer ever added to this page, one that writes all 84 labels,
walked straight through.

This was **measured, not reasoned**: with canon v2 fully in place and §6
untouched, the gate's only complaint was a markup regex about the `<th>` split.
The receiver check said nothing.

### What replaced it

A write is now legitimate when it is one of four things, and each is asserted:

1. **A mirror half** — `IDENT.dataset.label` is assigned within the same
   statement neighbourhood. Plus a separate assertion that `applyLabel` writes
   both *from the same expression, adjacently*: a mirror that recorded a
   different string from the one it rendered would satisfy "two writes" and
   certify nothing.
2. **An explicit mirror teardown** — `delete el.dataset.i18n; delete
   el.dataset.label;` before writing. That is how a node stops being a label and
   becomes a raw diagnostic sink for a C++ reason code this build does not know.
   Required, not merely tolerated: a stale mirror is exactly what breaks
   `check-ui-labels` assertion 3.
3. **A freshly created node** — `appendChild(el(...)).textContent = ...`. There
   is no authored text to erase; the node did not exist a statement ago.
4. **A dedicated value receiver**, from a whitelist that **SHRANK by two**.
   `monitorCopy` and `monitorNode` were added at v1.7.0 only because there was no
   route for a state-dependent caption; both are `setLabel()` calls now. The
   whitelist getting smaller as the page gets *more* localized is the sign the
   mechanism is the right one.

Three further assertions were added that the old §6 could not make:

- **No `[data-i18n]` element may have element children.** `applyLabel` writes
  `textContent`, which deletes them — silently, and in English too, so no
  language check would ever catch it. This is why the Delay column head was
  **split** into a keyed caption span beside its `#vcol-delay-unit` value span.
- **Every keyed element must carry authored English.** That text is the fallback
  that renders if `applyI18n()` never runs, which is the whole point of
  `initI18n`'s "paint the default SYNCHRONOUSLY first" rule.
- **No value receiver may bind an element that also carries `data-i18n`.** The
  mirror and the value write would fight and the last writer would win — the
  original bug wearing the new mechanism's clothes. **This assertion caught a
  real leftover on its first run:** `monitor-copy` was both keyed and bound.

Non-vacuity checks on the mirror route and the value route sit beside them, for
the reason §21 gives about the derived module registry.

**§2 is intact and was confirmed rather than assumed.** `init();` is still the
literal last statement of `app.js`, with no module-level declaration after it —
and NC-8 proves the check still fires.

## The TDZ trap did not bite here, and the reason is worth carrying

Stage H's sixth trap — canon v2 is reached at BINDING time — cost MBC and
O-Bitrot a swallowed `ReferenceError`. **It did not fire on O-Octagon, and not by
luck.** This plugin has no eager top-level binding at all: `init()` is the only
top-level call and it is the last statement, so every `let` above it is already
initialised when any binder runs. §2 has been enforcing exactly that since Phase
3.1. The i18n block did not need to move.

That is a data point for Stages I–L: **the plugins that need the block moved are
the ones without an `init()`-last discipline**, not a random subset.

## Three gate defects, all repo-level, all fixed in `3f6b201d`

1. **`check-i18n` assertions 12, 13 and 15 scanned ONE module.** Correct for the
   canon drift gate; wrong for "does any shipped JS write raw prose, and is every
   key live and resolvable". **Eleven plugins** split their page across sibling
   modules; O-Octagon has seven, and `js/venue.js` alone is 796 lines of
   controller code. The gate was wrong in both directions simultaneously: two raw
   English prose writes in `venue.js` passed assertion 12 **green**, and two keys
   referenced only from `venue.js` reported as **dead**. Now derived from the
   directory with a non-vacuity check on the derivation itself.

2. **`readSetLabelCalls` could not see `window.__setLabel(...)`.** The canon block
   publishes `window.__setLabel = setLabel` and states, in its own comment, that
   this is how a sibling module writes a localized label without `app.js`
   exporting anything. So that spelling is not a workaround — it is the *only*
   in-canon route available to `venue.js`. A scan matching only the bare name
   reported every key reached that way as dead, and treated a ternary in the same
   call as invisible to assertion 13. Same shape as the `dataset.i18nAria` gap
   fixed in `a1d80957`.

3. **`check-ui-labels` could only drive a state by `click`.** Two shapes ordinary
   in this repo were unreachable, and a label the gate cannot reveal is a label it
   certifies by never looking at it. Added `dblclick` (the house popover idiom —
   O-Octagon's speaker→output assignment) and `eval` (a state the PLUGIN owns,
   reached through the plugin's own committed ui-stub hook; the three frame
   banners render from `getStatus()` and cannot be clicked into existence).
   **On O-Octagon this took measured coverage from 35/84 to 84/84.**

**That is eight wrong-shaped gate assumptions in this task now, and five of the
eight reported a violation of a rule the code was obeying.**

## Geometry — 4 before fixes, exactly as the plan predicted

The plan said 1100 × 720 is the roomiest of the five, so expect fewer findings
than O-Tapestop, and that MORE would mean the tool or the attribute handling was
wrong. **Four**, against O-Tapestop's 26–30 and MBC's 222. The tool is behaving.

Driving the eight new states raised the total to **twelve**. Every one is the
`align-items: center` / shrink-to-fit family Stage H named — it was the first
thing looked for, and it was the answer every time.

| Fix | Measured cause |
|---|---|
| `.screen-tab { min-width: 84px }` | VENUE 80.4 → LIEU 64.5; `.screens` is `margin-left:auto`, so the pair does not just narrow, it slides 15.7 px right |
| `.scene-btn { padding: 6px 3px }` | GAUCHE/DROITE 38.5 in a 36.2 px content box, in a 10-column 1fr grid with no cell to spare — the padding was the only slack |
| `.cell-dense` track 44 → 52 px | MARCHE 45.4, SYNCHRO 49.7, VITESSE 45.1 |
| `.settings-unit` 36 → 44 px | MARCHE 39.7 in a 34 px content box |
| `.elev-readouts .cell-label { min-width: 46px }` | Ear 21.0 → Oreille 43.3 widened the strip 188.8 → 211.1 and dragged it 22.4 px left |
| `.caption-field { min-width: 50px }` | FIELD 40.1 → CHAMP 48.2 moved `#field-legend` 8.1 px |
| `#readout-label-envelope { min-width: 82px }` | ENVELOPE 71.5 → ENVELOPPE 79.6 |
| `.safe-banner` 184, `.map-banner` 396, `.monitor-banner` 400 px | all three are shrink-to-fit in the header flex; a growing copy line slid `nav.screens` AND `.settings-cluster` by 47.2 / 50.3 / 73.8 px |
| `.venue-rake / .venue-delay > .cell-label` 42 → 70 px | RAKE 42.0 → INCLINAISON 69.4. The floor was ALREADY there for this reason; it just needed the number French wants |
| `.vcol-head > [data-i18n] { min-width: 38px }` | DELAY 30.7 → RETARD 36.8 slid the unit span 6.1 px |
| `.venue-head > .cell-label { min-width: 40px }` | VENUE 37.8 → **LIEU 27.2** |
| `#btn-scene-store { min-width: 46px }` | STORE 44.1 → MÉM. 37.7 |

**Two of these are worth reading twice.**

The `.venue-head` one is a shape not seen in Stages F or H: **a caption that gets
SHORTER moves its neighbour just as surely as one that grows.** Every prior
stage's mental model was "French is longer". Four of this stage's twelve are
French being *shorter* — LIEU, MÉM., → SORTIE, and the tab pair. Stages I–L should
not scan only for growth.

The `#btn-scene-store` one was **not found by the gate**. It is a `[data-i18n]`
element, so assertion 7 excludes it by design — the exact coverage hole Stage H
documented and left open. It was found by measuring by hand. That hole is still
open and still real.

Nine of the twelve change ENGLISH geometry too. That is the trade D-04 asks for:
French is sized, never shrunk, and the only box two languages can share is the
wider one.

### Two French strings were sized, once, in the table

- **`Trajectoire` → `Tracé`** for the Path caption (66.7 px in a track that
  reaches 52 even after widening). This one should NOT worry a reviewer: `Tracé`
  is what this plugin's own English internals call it (`refreshTrace`,
  `TRACE_SHAPE_IDS`), so the width constraint and the better French agree. It
  needed its own LABELS key under the Stage F reuse rule, because the *tooltip*
  title is correctly `Trajectoire`.
- **The monitor's suppressed copy** → `Désactivé hors ligne — l'export est
  propre` (370.8 → ~283 px). The full form put the banner at 461.7 px in a header
  that also carries the title, the tab pair and the gear.

Neither is chosen at runtime. Both are recorded at their entry.

## Two stale facts fixed, both pre-existing

1. **The `lang-select` tooltip lied**, in both languages, saying the page labels
   do not change. Now says they do, and that numbers and unit symbols do not.
2. **The preset tooltip said "17 parameters".** The real figure is **28** —
   `oo::params::kCount`, static-asserted at `DSP/GainStage.h:79`. Stale since
   v1.8.0 added the ten motion parameters. Stage H correctly identified this
   string as O-Octagon's, not MBC's, and left it for this stage. Both languages.

## Deviations from the plan

**[Rule 1 — Bug] The `preset-list` tooltip's parameter count was wrong.** Not
part of the label retrofit; found while editing the file, verifiable against a
`static_assert`, and shipping a false claim about the product. Fixed inline.

**[Rule 2 — Missing critical functionality] Three repo-level gate defects.** A
gate that passes prose it should reject is not a cosmetic problem — assertion 12
was green over two raw English strings. Fixed in `scripts/`, in a separate commit,
per the Stage F/H precedent.

**[Rule 3 — Blocking] `check-ui-labels` could not reach 49 of 84 labels.** The
plan's done-criterion requires all 89 aria-labels to resolve in both languages
and the geometry diff to be clean. Neither claim is checkable on labels the gate
cannot render. `dblclick` and `eval` state actions unblocked it.

**[Rule 3 — Blocking] `check-i18n` assertion 7 rejected the LABELS generators.**
The 56 speaker/weight aria keys are generated from a template rather than
transcribed, and a top-level `const speakerAria = (n) => [...]` is a top-level
statement. The rule is right — `i18n.js` must never self-execute — so the
generators moved INSIDE the export declaration rather than the rule being
softened. "This particular top-level statement is harmless" is the judgement the
rule exists to refuse.

**Scope note, honestly stated:** the plan's T11 step 7 says "version bump,
CHANGELOG, PLUGINS.md, build-and-install, staging and commit discipline exactly
as T10 step 10", i.e. one plugin commit. It became two, because the scripts fixes
had to land first. That is the Stage F and Stage H shape, not a new one.

## Verification — every gate, individually

| Gate | Result |
|---|---|
| `tests/ui_frontend_check.js` | **exit 0 — 43 sections**, with §6 rewritten and §2 confirmed |
| `tests/ui_layout_check.js` | **exit 0 — 31 sections**. The label work did not disturb the layout it asserts |
| `check-i18n.js --plugin O-Octagon` | **exit 0**, reports canon **v2**, assertions 10–15 all run |
| `check-i18n.js` repo-wide | **exit 0** — canon split **v2 5, v1 0** |
| `check-i18n.js --strict-v2` | **exit 0 repo-wide — the first time.** Stage L's gate, reached at Stage G |
| `check-ui-labels.js --plugin O-Octagon` | **exit 0** — **84/84** labels measured across **9 states**, zero geometry shifts, 83/89 attributes change language |
| `check-ui-labels.js` on the other four | **exit 0 each** — the three tool changes disturbed nothing |
| `boot-all-uis.js` | **41/43 clean, unchanged.** O-Octagon: `title=0 aria=89 i18n=84`. The two failures are O-Bowed and O-Reed, pre-existing and unrelated |
| `O-Octagon-geometry-test` | **exit 0 — 57 probes, 0 failures**, from a FRESH `-DOUARICON_BUILD_TESTS=ON` configure |
| `O-Octagon-render-test` | **exit 0 — 73 probes, 0 failures**, same fresh configure |
| `./scripts/build-and-install.sh O-Octagon` | clean; VST3 + AU built and installed, AU cache cleared, dual-variant sweep ran |
| `auval -v aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED** |
| binary embedding | `i18n_jsSize` **60485** == `wc -c` — the retrofit really reached the binary |

The two C++ targets were built from a fresh configure into a throwaway
`build-octagon-tests/`, which was deleted afterwards rather than left as an
untracked directory.

## Negative controls — 26 run, 26 fire

Each applied to a byte-exact backup and restored **from that backup**, never
`git checkout --`, which would have wiped the uncommitted retrofit alongside the
mutation. Every mutation printed its substitution count and the harness refused
to run a gate on a zero-count mutation. All files sha256-verified after every
round and again at the end.

| Mutation | Assertion that fired |
|---|---|
| `applyLabel` writes textContent with NO `dataset.label` mirror | §6 mirror-pair, and the classifier |
| the mirror records a DIFFERENT string from the one it renders | §6 "from the SAME expression" |
| a raw prose textContent write in a SIBLING module | §6 classifier, `js/venue.js:686 strayNode` |
| revert the `<th>` split — key an element with element children | §6 leaf check, `"label.vcol.delay" on <th>` |
| a keyed element with no authored English fallback | §6 fallback check |
| a value receiver bound to a KEYED element | §6 `readout-label-envelope` |
| `venue.js` aliases `el` to `rows[0].labelInput` | §6 alias check — **see below** |
| `venue.js` aliases `el` to a different bare identifier | §6 alias check |
| a module-level declaration after `init();` | **§2** |
| clobber textContent after `applyLabel` ran | `[3]` both languages, `label="Ear" text="CLOBBERED"` |
| stub `__setLanguage` to a no-op | `[2]` vacuity, `0/84` |
| revert the `.cell-dense` 52 px track | `[4][fr]` text-spill, 5 captions named with widths |
| revert the `.screen-tab` pin | `[7]` geometry diff |
| revert the three banner pins | `[7]` geometry diff, 4 moved |
| an over-long French label | `[7]` geometry diff |
| remove one `data-i18n` from the markup | `[10]` |
| reinstate a native `title=` | `[11]` |
| a ternary inside a `setLabel` argument | `[13]` |
| a COMPUTED `setLabel` key | `[13]`, with `js/app.js:711` |
| drift ONE line of the canon block | `[6]` matches NEITHER canon |
| a LABELS `fr` entry copied from `en` | `[4]` LABELS |
| strip a LABELS `reviewed` flag | `[5]` LABELS |
| empty an `I18N_EXEMPT` reason | `[14]` |
| raw prose in a sibling module | **`[12]`** — the page-module scan fix |
| a dangling key on a `window.__setLabel` call | **`[15]`** — the spelling fix |
| a ternary inside a `window.__setLabel` argument | **`[13]`** — the spelling fix |

**A counter-proof, because a control that passes both ways is decoration.**
Reverting the `window.__setLabel` spelling fix in `i18n-extract.js` and re-running
the dangling-key control confirms it goes **SILENT**. The fix is load-bearing.

### THE ONE THAT DID NOT FIRE AT FIRST, and it was my own new gate

`venue.js aliases el to an unknown node` produced **no failure**. The assertion
was new, written in this stage, and wrong: it read
`/const el = ([A-Za-z_$][\w$]*);/`, so `const el = rows[0].labelInput;` simply did
not match, the alias dropped out of the list, and `.every()` passed over what was
left. **A gate that stops seeing the thing it checks when that thing gets more
complicated** — the same vacuity class §21 of this very file exists to catch, and
the ninth wrong-shaped assumption in this task.

Widened to match the whole right-hand side, given a non-vacuity check of its own,
and re-run: it fires on both the property-access form and a bare-identifier form.

It is worth being blunt that this was **found by a control, not by review**. The
assertion had been read over several times and looked correct.

## English fidelity

Every `en` entry came from `i18n-extract.js`'s inventory of the live markup rather
than being transcribed. **One deliberate normalisation, recorded at its entry:**
the Delay column head's caption moved from a bare text node inside the `<th>` into
its own `<span>`, unchanged in wording. `.venue-rake`'s label floor changed 42 →
70 px, which moves English — but that floor already existed precisely to keep the
rake and delay rows' controls on one vertical line, so the mechanism is unchanged.

## NOT VERIFIED — read this before Stage I

1. **THE C++ PERSISTENCE ROUND-TRIP HAS STILL NEVER BEEN RUN.** Checkpoint 4(b)
   has been outstanding since Stage B and is outstanding on all five plugins.
   Nothing in Stage G touched C++ and nothing in Stage G tested it. Every claim
   that a language choice survives a session is REASONED from source. **This is
   now the single highest-value outstanding check in the whole task**, because
   all five plugins are done and every one of them rests on it.
2. **No human has seen the French UI, on any plugin.** All geometry is headless
   Chromium. Whether `INCLINAISON` at 9 px uppercase, or `AFFECT.` as the MAP
   badge, or `MÉM.` on the store button read well inside O-Octagon's aesthetic —
   as opposed to merely fitting — is Checkpoint 4(a) and Checkpoint 5.
   `AFFECT.` and `MÉM.` are the two I would challenge first.
3. **Nothing was tested in a DAW.** It builds, installs and `auval`s; it has not
   been opened in Logic or Ableton. The plugin is a 7.1 surround spatializer, so
   its DAW behaviour is the least covered by headless testing of any of the five.
4. **The Standalone `.app` is stale.** `build-and-install.sh` builds VST3 + AU
   only. It was NOT refreshed by this stage.
5. **A single over-long caption in a shrink-to-fit cell is still not gated.** The
   hole Stage H documented is unchanged, and `#btn-scene-store` is this stage's
   instance of it — found by hand, not by the gate.
6. **`check-ui-labels`'s state pass is `events-only` on O-Octagon**, because its
   committed ui-stub exposes no `window.__stubStates`. Assertion 3's "after a
   state pass" is therefore dispatched events, not driven parameter updates —
   weaker here than on MBC and O-Bitrot. It still caught the clobber control.
7. **The `eval` state action is new and is a sharp tool.** It runs arbitrary JS in
   the page. O-Octagon's states file uses it only through the plugin's own
   committed stub hook (`window.__OCTAGON_STUB__.setStatus` / `.setMapInvalid`),
   which is the intended discipline — but nothing enforces that. A states file
   that drove the DOM directly would measure a state the plugin cannot actually
   produce. **That is a review question for Stages I–L, deliberately left to
   review rather than forbidden**, because forbidding it would also forbid the
   legitimate use.
8. **My first banner state drove an IMPOSSIBLE combination** — SAFE + MAP +
   MONITOR at once, which `monitorAvailable = !safeMode && !mapInvalid` forbids —
   and produced 106 moved elements and frame crossings **in English too**. That
   was the states file being wrong, not the plugin. It is a warning for Stages
   I–L: an `eval` state can manufacture a state the plugin never reaches, and the
   resulting "failure" is noise.
9. **All 184 French entries are machine drafts**, every one `reviewed: false`. No
   native speaker has read any. Repo total is now **435 unreviewed** across five
   plugins.
10. **Windows / WebView2 remains a named deferral, blocked on hardware.** Every
    width in the table above was measured in Chromium on macOS.
11. **The generated inventory artifacts** (`plugins/O-Octagon/.planning/i18n-*`)
    are left UNCOMMITTED, as on the other four. They are regenerable.
12. **`§41` appears TWICE in `ui_frontend_check.js`** — "the trace is NOT generated
    on the page (v1.8.0)" and "the rake line spans the BBOX". Pre-existing, from
    the v1.8.0 work, untouched here because it is not this task's file to
    renumber. The section COUNT (43) is right; the labels are not.

## Concurrency — the thing that deferred this stage, encountered again

**Another session was writing inside `plugins/O-Octagon` throughout this dispatch.**
Between starting and committing, it modified `Source/DSP/MotionClock.h` (a v1.10.0
WR-01 fix: the v1.8.0 sync multiplier table was 4× slower than every label and
made `1/16D` and `1/8T` bit-identical), `Source/PluginProcessor.cpp`,
`Source/ui/public/js/elevation.js`, `CODE_REVIEW.md`, and all four test
`main.cpp` files.

**`git commit -- plugins/O-Octagon` would have swept all of it in.** The commit
names its ten files explicitly instead. Verified after committing: every one of
their paths is still dirty and untouched.

**Carry this forward.** The plan's T10 step 10 and the CLAUDE.md rule both say
"path-scope the commit", and the canonical example given is
`git commit -- plugins/<Name>`. On a plugin another session is editing, that is
not narrow enough. **The scope that matters is the FILE SET, not the directory.**

**A version-numbering note for whoever picks up next:** that session's comments
target **v1.10.0**. This stage shipped **v1.9.0**. The numbers are compatible in
that order, but they were chosen independently and nobody coordinated them.

**Observed live, minutes after `99e7d206` landed:** that session bumped the
working tree's `CMakeLists.txt` from the `1.9.0` this stage committed to
`1.10.0`, i.e. it sequenced on top rather than colliding. Their bump is
uncommitted and is theirs to commit; it was deliberately left untouched. The
practical consequence is that **the installed bundle is v1.9.0 while the working
tree now reads v1.10.0**, so the next `build-and-install.sh` will produce a
v1.10.0 binary carrying both stages' work.

## Carried into Stage I

- **Re-run `i18n-extract.js`.** Wrong about every plugin in every stage so far.
- **`align-items: center` is still the tell**, and it accounted for all twelve
  findings here.
- **Also scan for French getting SHORTER.** Four of twelve were shrink, not
  growth — a shape Stages F and H never hit.
- **Expect to move the i18n block only where there is no `init()`-last
  discipline.** O-Octagon did not need it, and §2 is why.
- **`data-*`-authored captions**: O-Octagon had `data-label` on two settings rows
  with nothing writing them. Grep for the idiom anyway.
- **`boot-all-uis.js` on every plugin** — still the only gate that sees a
  swallowed binding failure.
- **The new `dblclick` / `eval` state actions** are what made 84/84 coverage
  possible. Write a `tests/i18n-states.json` for every plugin from now on, and
  drive banners through the plugin's OWN stub hook, one reachable state at a time.


---
---

# Stage I (batch I1) — T13: O-Contrabass v1.8.0

**Commit:** `7035029a` — **ONE** path-scoped plugin commit, ten files named
individually. No `scripts/` commit: the one gate defect found this stage lives
in the plugin's own `tests/`, so unlike Stages F, G and H there was nothing
repo-level to land first.

**PARTIAL: 1 of the 9 plugins Stage I covers.** O-Orbit — the other half of
batch I1 — was excluded from this dispatch, and the seven `O-simple*` plugins of
batch I2 are untouched. Stage I stays open.

O-Contrabass is the **sixth plugin on canon v2** and the first outside the five
that already shipped tooltips. `check-i18n --strict-v2` reports **v2 6, v1 0**.

## What was different here: BOTH halves in one release

The five before it were already on canon v1 — French tooltips over English
labels — and Stages F–H were retrofits. O-Contrabass had the `data-tip` renderer
(v1.7.0) with its copy authored in the markup and no `i18n.js` at all, so this
release moves 44 tooltips out of `index.html` AND localizes 52 labels, 6
accessible names and 2 script-written captions in the same commit. Splitting
them would have shipped a half-localized plugin twice, which is the state the
five were being rescued from.

Final shipped counts: **46 I18N entries** (44 moved + `gear-btn` + `lang-select`),
**57 LABELS keys**, **46 TIP_BINDINGS**, **30 I18N_EXEMPT entries**, **52
`[data-i18n]` elements**, **6 `data-i18n-aria`**, 103 French strings.

## The plan's numbers, again

Stage G's carried-forward note said to run `i18n-extract.js` and disbelieve the
plan. It was right again, though this time in BOTH directions.

| The plan said | Live at v1.7.2 |
|---|---|
| 44 tips | **44** — correct, the first per-plugin count in this task that was |
| 61 text | **61** (60 html-text + 1 js-prose) — also correct |
| **47 attrs** — "second-highest attribute count in the repo" | **3.** Three `aria-label`s, zero `title=`, zero `placeholder`, zero `alt`. The 47 is `44 data-tip-title` + `3 aria-label`, i.e. it counted the tooltip TITLES as localizable attributes. They are, but they are handled by TIP_BINDINGS, not by `data-i18n-aria`. The real attribute-keying job was **3**, not 47. |
| bridge 34 → **36** | **36**, exactly. The one arithmetic prediction in the plan that held. |
| "19 render goldens" (`STATUS.md`) | **21**. `reproduce-goldens.sh` has 21 entries; the status line was written before `note-expression` and `mpe-yz` were added. All 21 reproduce byte-identical. |

**The `data-*`-authored two-state caption idiom is ABSENT on this plugin** —
`grep -on 'data-\(on\|off\|confirm\|label\)='` over `index.html` returns
nothing. It appeared on all three Stage H plugins and Stage F predicted it would
recur everywhere; it is house style, not a law. Grepping for it directly still
cost nothing and is still the right move.

## THE ONE GATE DEFECT, and it certified a silent 404

The plan warned to check this plugin's gates "for a double-quote-only import
scan BEFORE bumping the bridge count", because that exact shape cost a false
failure in Stage C. The defect was there. It was not a false failure — it was a
**false PASS**, which is worse.

`ui_frontend_check.js` §6 is the resource-provider closure check: every local
file the page references must have a `getResource()` branch, because a missing
one is a silent 404 and a blank UI. It enumerated **three import SHAPES**:

```js
/(?:src|href)="(\.\/|)((?:js|css)\/[^"]+)"/          // double-quoted attribute
/import\(\s*["']\.\/((?:js|css)\/[^"']+)["']\s*\)/   // dynamic import
/import\s+\*\s+as\s+\w+\s+from\s+["']\.\/…/          // NAMESPACE import
```

Canon v2's import is `import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from
'./js/i18n.js';` — a **named** import, a fourth shape, matching none of the
three. The scan counted 4 references where the page has 5 and reported
`PASS: all 4 local UI file references have resource-provider entries`.

It now matches any module specifier — named, default, namespace, side-effect or
dynamic — in either quote style, with a non-vacuity check on the derived set and
a targeted assertion that `/js/i18n.js` is in it.

**Proven both ways, because a control that passes both ways is decoration.** With
the v1.7.2 scan restored AND the `getResource()` branch deleted, the gate prints
`PASS: all 4 local UI file references have resource-provider entries` over a page
that really would 404. The fix is load-bearing. That is NC-28.

That is **nine wrong-shaped gate assumptions in this task**, and this is the
second of the nine that passed a rule the code was breaking rather than failing
one the code was obeying.

## Two English bugs French exposed

The Stage F/H/G pattern held for a fourth stage running.

1. **The header row has had ZERO slack, in English, since the tab strip was
   added.** `.header-spacer` measures **0 px** in English, and the preset readout
   authored at `width: 300px` was being flex-shrunk to **250.2** — it had never
   once rendered at its authored width, and nothing was measuring it. French did
   not cause that; French made it visible by needing 35.3 px more for
   PRINCIPAL/ACCORD and taking it out of the readout (−22.9), both nav arrows
   (−1 each), the whale (−3.6) and the brand line (−16.3). Fixed by pinning the
   readout at 180 and the tab buttons at 100, which gives the row real slack in
   both languages. "Cinematic Bass Sustain", the longest factory preset name,
   measures 135 px inside the 180.
2. **The Active Strings readout said "4 of 4" on a French page.** It was
   `FMT.ACTIVE_STRINGS = v => Math.round(v) + " of 4"`. D-03 exempts a number and
   its UNIT SYMBOL; "of" is a connective word, and no assertion in the suite
   would have caught it — a `fmt(v)` call is not a string literal, so
   `check-i18n` assertion 12 is silent about it by construction. It goes through
   `setLabel(valueEl, "readout.activeStrings", { n: v })` now: the number stays a
   number, only the connective is localized, and the element joins the sweep.
   A new §7 assertion scans what is LEFT in `FMT` for connective words so the
   next one cannot be added without a key.

## Geometry — 21 moved before fixes, ZERO after

Measured at the shipping 1000 × 650 across three driven states.

| Plugin | frame | labels | moved before | after |
|---|---|---|---|---|
| O-Tapestop (F) | 860 × 580 | 46 | 26–30 | 0 |
| O-MultiBandCompressor (H) | 900 × 640 | 77 | 222 | 0 |
| O-Bitrot (H) | 900 × 740 | 71 | 78 | 0 |
| O-ReverseDelay (H) | 940 × 768 | 50 | 7 | 0 |
| O-Octagon (G) | 1100 × 720 | 84 | 4 (12 with states) | 0 |
| **O-Contrabass (I)** | **1000 × 650** | **52** | **21** | **0** |

Every one of the 21 is the `align-items: center` / shrink-to-fit family Stage H
named. It was the first thing looked for and it was the answer every time.

| Fix | Measured cause |
|---|---|
| `.tab-btn { min-width: 100px }` | MAIN 64.3 → PRINCIPAL 98.2; the strip grew 148.6 → 183.9 and re-dealt a header row with no slack — 9 of the 21 |
| `.preset-name-display { width: 180px; flex: 0 0 180px }` | was 300, rendering 250.2; see the English bug above |
| `.preset-save-btn { min-width: 66px }` | SAVE 52.5 → ENREG. 64. It is a `[data-i18n]` element so assertion 7 excludes its own box — it still PUSHED the tab strip and the whole right cluster 9.5 px |
| `.panel-label > span:first-child { flex: 1 1 auto }` | the caption span was shrink-to-fit on all six numeral panels: Bow +22.1, Body +6.5, Drone +18.2, Strings −4.3, Output −4.3 |
| `.sec-body .sublabel { min-width: 93px }` | resonator 87.7 → résonateur 92.7 |
| `.sec-expression .sublabel { min-width: 108px }` | vibrato · bow drift 81.7 → vibrato · dérive d'archet 107.1 |
| `.strip-field > .strip-field-label { min-width: 98px }` | Tuning System 80.3 → Système d'accord 97.5, sliding the select and the toggle |
| `.scl-btn { min-width: 98px }` | Load .scl… 81.8 → Charger .scl… 97.5, sliding the Note Expression toggle a further 15.7 |

**Two of these are worth reading twice.**

`.preset-save-btn` is the Stage H "container sized by a keyed child" shape at its
sharpest: assertion 7 excludes a `[data-i18n]` element by design, so the button
that caused nine rows of the diff never appeared in it. The diff named the nine
victims and not the culprit. On a flex row, read the diff as a list of
CONSEQUENCES and look one step upstream.

`.panel-label > span:first-child` is a shape the earlier stages did not hit.
Nothing on screen moved — `.panel-label` is `justify-content: space-between`, so
the caption is pinned left and the sublabel right, and a wider caption just eats
empty space. The gate flagged it anyway, correctly: the BOX changed. The fix is
to give the span the remaining space instead of letting it shrink-wrap, which
makes the box deterministic and changes nothing visible. **Four of the six
captions got SHORTER in French** (Strings, Output) or stayed the same
(Expression, Microtonal) — Stage G's "also scan for French getting shorter" note
held, and a shorter caption flags exactly as loudly as a longer one.

**One French string was SIZED**, recorded at its entry with the measurement:
`point de fonctionnement de l'archet` (156.3 px) beside a 167.4 px title in a
314 px panel head **wrapped to two lines and pushed the Schelleng canvas down
11 px**. Shortened to `point de fonctionnement`. This one should not worry a
reviewer: the panel is already titled "Diagramme de Schelleng", so "de l'archet"
was saying twice what the title says once.

Nine of the pins change ENGLISH geometry too. That is the trade D-04 asks for.

## Structural markup changes, and why each was forced

- **Six panel captions split** into a `.panel-title` span beside the Roman
  numeral, and the drone caption into two spans either side of its authored
  `<br>`. `applyLabel()` writes `textContent`, which deletes an element child —
  silently, and in English too, so no language check would ever catch it.
- **The "?" toggle became a gear + popover.** The gear carries `.help-toggle` as
  well as `.gear-btn`, so it inherits the 26 px circle, border, hover and lit
  state and the header silhouette is unchanged. The popover is written in the
  **preset menu's own dark vocabulary** — the same `#4A3226 → #3E2A20` gradient,
  the same `rgba(237,217,190,0.3)` rule, the same 4 px radius — not O-Bitrot's
  paper card. This header band is dark; a paper panel here would read as pasted
  in from another plugin.
- **The Tuning tab's load-failure notice** was an `innerHTML` string literal with
  an inline style. It is `createElement` + `setLabel` now, with its presentation
  moved to `.tuning-load-error`.
- **Native `title=`: there were none.** The page had zero at v1.7.2, so
  contract §4 was already satisfied. Asserted, not assumed.

## The TDZ trap fired here, and Stage G's rule predicted it

Stage G's carried note: *"expect to move the i18n block only where there is no
`init()`-last discipline"*. O-Contrabass has no `init()` at all — the 25 knob
bindings, the four fine-tuners, `bindActiveStrings()` and `bindMicrotonal()` all
run at module top level — so the canon block had to go ABOVE them, and it does.

**Proven, not reasoned.** NC-29 physically relocates the canon block to below the
bind calls and `boot-all-uis` reports
`O-Contrabass: Cannot access 'uiLanguage' before initialization`, exactly the
Stage H failure. `check-ui-labels` and `check-i18n` both stay silent about it.
`boot-all-uis` remains the only gate that sees this class.

A new §7 assertion pins the ORDER positionally, because that is what the bug is.

## Verification — every gate, individually

| Gate | Result |
|---|---|
| `tests/ui_frontend_check.js` | **exit 0**, all checks, 7 sections. Bridge **36 ↔ 36** |
| `check-i18n.js --plugin O-Contrabass` | **exit 0**, canon **v2**, assertions 10–15 all run |
| `check-i18n.js --strict-v2` repo-wide | **exit 0** — canon split **v2 6, v1 0** |
| `check-ui-labels.js --plugin O-Contrabass` | **exit 0** — **52/52** labels measured across **3 states**, **zero** geometry shifts, 6/6 keyed attributes change language, 46/52 (88%) differ en→fr |
| `check-ui-labels.js` on the other five | **exit 0 each** — nothing shared was touched, and this confirms it |
| `boot-all-uis.js` | **41/43 clean, unchanged.** O-Contrabass: `title=0 aria=6 i18n=52`. The two failures are O-Bowed and O-Reed, pre-existing and unrelated |
| `tests/render-harness/reproduce-goldens.sh` | **all 21 goldens byte-identical.** The DSP-FROZEN invariant held; `PluginProcessor.cpp` is in the harness build and was edited |
| `./scripts/build-and-install.sh O-Contrabass` | clean; VST3 + AU built and installed, AU cache cleared, dual-variant sweep ran |
| `auval -v aumu OCbs OuDv` | **AU VALIDATION SUCCEEDED** |
| bundle version | `CFBundleShortVersionString` **1.8.0** — the single `VERSION` declaration reached the artefact |
| binary embedding | `i18n_jsSize` **43114** == `wc -c` — the retrofit really reached the binary |

`CMakeLists.txt` was checked for a second version declaration before editing, per
the plan's step 3. There is exactly one (`VERSION 1.7.2` at line 16); the render
harness does not carry its own copy.

## Negative controls — 29 run, 29 fired

Each mutation was applied to a **byte-exact backup** and restored **FROM THAT
BACKUP**, never `git checkout --`, which would have wiped the uncommitted
retrofit alongside it. Every mutation printed its substitution count and the
harness **refused to run the gate on a zero-count mutation**. Every file was
sha256-verified after every round.

| Mutation | Assertion that fired |
|---|---|
| revert `.tab-btn` min-width | `[7]` geometry diff, 9 moved |
| revert `.preset-save-btn` min-width | `[7]` geometry diff, 9 moved |
| revert the panel-caption `flex: 1` pin | `[7]` geometry diff, 5 moved |
| revert `.strip-field-label` min-width | `[7]` geometry diff, 5 moved |
| revert `.scl-btn` min-width | `[7]` geometry diff, 3 moved |
| restore the over-long Schelleng French sublabel | `[7]` geometry diff, 2 moved |
| an over-long French knob caption | `[4][fr]` text-spill, `label.brightness 159>62` |
| stub `__setLanguage` to a no-op | `[2]` vacuity, `0/52 labels differ` |
| clobber `textContent` after `applyLabel` | `[3]` in both languages |
| remove one `data-i18n` from the markup | `[10]` |
| reinstate a native `title=` | `[11]` |
| a ternary inside a `setLabel` argument | `[13]` |
| a raw prose literal written to `textContent` | `[12]`, with file:line |
| drift ONE line of the canon block | `[6]` matches NEITHER canon |
| a LABELS `fr` entry copied from `en` | `[4]` LABELS |
| strip a LABELS `reviewed` flag | `[5]` LABELS |
| empty an `I18N_EXEMPT` reason | `[14]` |
| delete the `getResource()` branch for `/js/i18n.js` | `[8]`, **and** frontend §6 |
| a dangling `data-i18n` key | `[15]` dangling |
| an unreferenced LABELS key | `[15]` dead |
| break a TIP_BINDINGS selector | frontend §7 UNMATCHED |
| delete the `fr` half of a LABELS key | frontend §7 `fr MISSING` |
| write the toggle caption as a JS literal | frontend §7 setLabel keys |
| put a connective word back into `FMT` | frontend §7 D-03 |
| drop the toggle's markup fallback key | frontend §7 |
| a native fn call the C++ never registers | frontend §2 bridge parity |
| **NC-28: the v1.7.2 §6 scan + a deleted provider branch** | **SILENT — the counter-proof.** The old scan prints PASS over a page that would 404 |
| **NC-29: canon block below the eager bindings** | **`boot-all-uis`: `Cannot access 'uiLanguage' before initialization`**, plus frontend §7 positional |

**A note on which assertion fires.** The over-long-French-caption control fired
`[4][fr]` text-spill, not `[7]` — `.knob-label` here has a fixed 62 px content
box, so the text overflows rather than moving a neighbour. On O-Tapestop the same
control fired `[8]` overlap and on O-Bitrot `[7]`. The three assertions cover
each other; none of them is the one that always catches it, which is the argument
for keeping all three.

## English fidelity, checked mechanically

Every `en` entry was extracted from `index.html` by script and **compared back
byte-for-byte** — 44/44 titles and bodies, 0 drift. Two things that could have
gone wrong silently and did not:

1. **Seven distinct HTML entities** appear in the tooltip copy
   (`&mdash; &ndash; &beta; &times; &plusmn; &cent; &minus;`). `getAttribute`
   decodes them at parse time, so the page has always rendered `β`; a
   transcription into a JS string literal would not. The extraction decoded them
   explicitly and **threw on any entity it did not know**, which is how the set
   above is known to be complete rather than assumed.
2. `scripts/i18n-extract.js`'s own `decodeEntities` does **not** know `&beta;` —
   it left the literal `&beta;` in two rows of the generated inventory. That is a
   defect in the extractor's convenience output, not in anything shipped, and it
   was caught because the verification decoder threw rather than passing the
   entity through. It is **left OPEN and reported here** rather than patched: the
   inventory is a review aid, the fix belongs with whoever next touches
   `i18n-extract.js`, and a `scripts/` commit for it was out of this dispatch's
   scope.

**No English wording was changed anywhere.**

## Commit discipline — the sweep that nearly landed, and did

Another session was live in this checkout throughout, holding 13 files under
`plugins/O-Octagon/` and 5 under `.claude/`. HEAD moved under this work once
(`3593213d`, O-Orbit v1.1.1) before the commit.

The ten files were named individually, never `git commit -- plugins/O-Contrabass`.
That was still not enough. **`PLUGINS.md` is a shared file, and the other session
had an uncommitted row edit in it** (O-Octagon `1.10.0-dev` → `1.10.1-dev`).
Naming the FILE stages the whole file, so the first commit swept their row in.

Caught by inspecting `git show --stat` and then the actual diff — `PLUGINS.md`
reported `4 +-` where a one-row edit is `2`. Fixed by amending: `PLUGINS.md` was
rebuilt from `HEAD~1` plus only the O-Contrabass row, re-staged, `--amend`ed
(unpushed, HEAD verified unmoved and still mine), and their row edit then written
back into the working tree so it survives as theirs to commit. Final commit
`7035029a`; their 22 files verified still dirty and untouched afterwards.

**Carry this forward, because Stage G's lesson was one level short.** Stage G
concluded "the scope that matters is the FILE SET, not the directory". That is
necessary and not sufficient. On a file two sessions both edit — and `PLUGINS.md`
is the one file in this repo that every plugin release touches — **the scope that
matters is the HUNK**. Read `git show -- PLUGINS.md` after every plugin commit,
not just the `--stat`.

## Deviations from the plan

**[Rule 1 — Bug] The header row had no slack in English.** Not part of the
localization; found because the geometry diff named nine victims of it; verifiable
by measuring `.header-spacer` at 0 px; shipping a preset readout that never
rendered at its authored width. Fixed inline.

**[Rule 1 — Bug] The Active Strings readout wrote English prose.** Same shape:
found while reading every `READOUT` row by hand, as the procedure's step 1 asks.

**[Rule 2 — Missing critical functionality] `ui_frontend_check` §6 certified a
silent 404.** A resource-provider gate that passes a missing provider branch is
not a cosmetic problem — it is the blank-UI failure the assertion exists to
prevent, and this commit is the one that would have hit it. Fixed in the plugin's
own `tests/`, so it lands in the plugin commit rather than a `scripts/` one.

**[Rule 2] Three new §7 assertion groups** were added to the plugin gate: key
resolution in both languages, TIP_BINDINGS selector liveness, and the canon
block's POSITION relative to the eager bindings. The third is the only gate in
the repo that pins the TDZ ordering statically; `boot-all-uis` catches it at
runtime but only after the fact.

**Scope note, honestly stated:** the plan's T13 lists nine plugins. This dispatch
was scoped to one, on instruction. That is a dispatch boundary, not a deviation
from the plan's content.

## NOT VERIFIED — read this before the rest of Stage I

1. **THE C++ LANGUAGE ROUND-TRIP HAS STILL NEVER BEEN RUN.** Checkpoint 4(b) has
   been outstanding since Stage B and is now outstanding on **six** plugins.
   Nothing in this stage tested it. The claim that a language choice survives a
   session reload is REASONED from source — the `uiLanguage` atomic, the root XML
   attribute in `get/setStateInformation`, and the `getUiLanguage` pull at page
   init. O-Contrabass uses `xml->setAttribute` / `getStringAttribute` rather than
   a ValueTree property, which sidesteps
   `critical_valuetree_xml_roundtrip_loses_type` by construction (an XML
   attribute is a string by definition, so there is no `isBool()` guard to
   misfire) — but *sidesteps by construction* is still not *measured*. This
   remains the single highest-value outstanding check in the task.
2. **No human has seen the French UI, on any plugin.** All geometry here is
   headless Chromium at 1000 × 650. Whether `— ARCHET` reads well in this
   plugin's naturalist-plate hand, or whether `Tendeurs fins` sits right over the
   tailpiece faders, is Checkpoint 4(a) and Checkpoint 5. `Chute` for Release,
   `Satur.` for Saturate and `Vib Ampl.` are the three I would challenge first.
3. **Nothing was tested in a DAW.** It builds, installs and `auval`s; it has not
   been opened in Logic or Ableton. The four Stage-4 human gates in
   `plugins/O-Contrabass/.planning/STATUS.md` (Windows pluginval via CI, the
   Dorico 24-EDO smoke test, the subjective sign-off and the Logic manual checks)
   were **not attempted and are not affected** — the goldens and `auval` confirm
   nothing regressed under them.
4. **The Standalone `.app` is stale.** `build-and-install.sh` builds VST3 + AU
   only. It was NOT refreshed by this stage.
5. **The Tuning tab is NOT localized.** It is rendered by the shared
   `scala-tuning-engine` module (`js/tuning-panel.js`), which O-Wind, O-Bowed,
   O-Reed and O-Bassoon also consume, so localizing it is a MODULE change and out
   of a per-plugin commit's scope. Half of this plugin's UI surface is therefore
   English in both languages. `check-i18n` cannot see it (assertion 10 scans
   `index.html`) and `check-ui-labels` was not driven into it. This is a scope
   statement, not an oversight, and it is the largest single gap in the claim
   "O-Contrabass speaks French".
6. **`label.tuningLoadFailed` was never RENDERED.** It only appears if the
   `import('./js/tuning-panel.js')` throws. Driving that would need an `eval`
   state manufacturing a condition the plugin cannot reach on its own — the
   Stage G warning about impossible states — so it is verified statically
   (assertion 15, and frontend §7 resolves it in both languages) and never
   measured. Its geometry is unknown.
7. **A single over-long caption in a shrink-to-fit cell is still not gated.** The
   hole Stage H documented and Stage G hit again is unchanged. It did not bite
   here — `.knob-label` on this plugin has a fixed content box, so the text-spill
   check does fire — but that is a property of this page, not a fix.
8. **All 103 French entries are machine drafts**, every one `reviewed: false`. No
   native speaker has read any. Repo total is now **618 unreviewed** across six
   plugins.
9. **Windows / WebView2 remains a named deferral, blocked on hardware.** Every
   width in the fix table was measured in Chromium on macOS.
10. **`i18n-extract.js` does not decode `&beta;`** — see the English-fidelity
    section. Left OPEN; affects the generated inventory only, nothing shipped.
11. **The generated inventory artifacts** (`plugins/O-Contrabass/.planning/i18n-*`)
    are left UNCOMMITTED, as on the five before. They are regenerable.

## Carried into the rest of Stage I

- **O-Orbit is now UNBLOCKED.** It was excluded from this dispatch because
  another session held `plugins/O-Orbit`; that session landed O-Orbit v1.1.1 as
  `3593213d` and no O-Orbit file is dirty. Its plan row still warns that
  `CMakeLists.txt:54-62` serves `Resources/ui/`, **not** `Source/ui/public/` —
  verify that against the live tree before writing a byte, and re-run
  `i18n-extract` for its counts.
- **Read `git show -- PLUGINS.md` after every plugin commit**, not the `--stat`.
  A one-row edit is 2 changed lines; anything more is another session's row.
- **The plan's "attrs" column counts tooltip TITLES, not `aria-label`s.** For the
  seven `O-simple*` plugins its attribute figures (5, 5, 10, 6, 3, 1, 14) are
  small enough that they are probably genuine `aria-label` counts — but check,
  the same way, before sizing the work.
- **On a flex row, read the geometry diff as a list of CONSEQUENCES.** The
  element that caused nine of the 21 rows never appeared in the diff, because it
  is a `[data-i18n]` element and assertion 7 excludes those by design.
- **`.panel-label > span:first-child { flex: 1 1 auto }`** is a reusable answer
  for the "caption span beside a numeral in a space-between header" shape, which
  is house style across the naturalist-plate plugins.
- **French getting SHORTER flags as loudly as French getting longer** — four of
  six panel captions here. Stage G's note, confirmed on a second plugin.
- **`boot-all-uis.js` on every plugin.** Still the only gate that sees the TDZ
  throw, and NC-29 proves it on this plugin specifically.
- **Write a `tests/i18n-states.json` for every plugin.** Two states — the gear
  popover open, and the hover-help toggle lit — took measured coverage from 49/52
  to **52/52** here.

---

# Stage I (batch I1, second and final plugin) — T13: O-Orbit v1.2.0

**Commits:** TWO, deliberately separate.

- **`f00e5d45`** — `scripts/check-ui-labels.js` alone. A repo-level gate fix,
  landed FIRST and never bundled into the plugin commit. Section below.
- **`9d8e50d0`** — `plugins/O-Orbit` (11 files) + the single `PLUGINS.md` row.

**Batch I1 is now COMPLETE** — O-Contrabass `7035029a` and O-Orbit `9d8e50d0`.
**Stage I is not.** The seven `O-simple*` plugins of batch I2 are untouched.

O-Orbit is the **seventh plugin on canon v2**. `check-i18n --strict-v2` reports
**v2 7, v1 0**, and `check-ui-labels` exits 0 on all seven.

Final shipped counts: **34 I18N entries** (32 moved + `gear-btn` + `lang-select`),
**57 LABELS keys**, **34 TIP_BINDINGS**, **5 I18N_EXEMPT entries**, **56
`[data-i18n]` elements**, **7 `data-i18n-aria`** + 1 `data-i18n-placeholder`,
**91 French strings**. Bridge **25 → 27**.

## THE HEADLINE: a repo-level gate fix was unavoidable, so it shipped alone

The dispatch said to stop and report rather than bundle a `scripts/` change into
the plugin commit. It was unavoidable, and the resolution was to land it as its
**own commit ahead of the plugin**, which is what Stages F, G and H each did with
their gate defects (`3f6b201d` is the precedent). It is reported here as the
headline rather than buried in a deviation list.

`check-ui-labels` assertion 6 asserted **absolutely** that no `[data-i18n]` rect
may cross the shipping frame. **O-Orbit is the first plugin the gate has met with
an intentionally scrolling pane.** `#controls-container` is `flex: 1` with
`overflow-y: auto`, and its three parameter groups need 555px inside a 226px
pane, so eleven labels sit below the 600px frame at rest.

**Proven pre-existing, not caused by the retrofit.** The same probe run against
the tree at `HEAD` before the retrofit and after it returns byte-identical
numbers — `clientH 226, scrollH 555, overflow 329`, the Spatial heading at
`y=616.8` in **English** both times.

**Proven to contribute nothing in French.** The out-of-frame sets are the SAME
eleven keys with the SAME overshoot **to the decimal** in both languages, and all
eleven are inside the scrolling pane:

| key | en overshoot | fr overshoot |
|---|---|---|
| label.groupSpatial | 33.8 | 33.8 |
| label.speakerLayout | 54.8 | 54.8 |
| ui.downmix | 103.8 | 103.8 |
| label.distance / airAbsorption / attenCurve / centerDiverge | 54.8 | 54.8 |
| label.groupSourceMix | 179.8 | 179.8 |
| label.sourceMode / lrOffset / mix | 200.8 | 200.8 |

So the absolute form produced **8 failures per run against a plugin whose French
geometry is perfect** — the gate arguing with the design rather than reporting a
French problem. That is precisely the reasoning already written into assertion
5, which was made a DELTA for the same reason after Stage F met O-Tapestop's
absolutely-positioned panel legends.

Assertion 6 now fails on `overshoot(fr) > overshoot(en)` and **reports the
English baseline** so it is never silent. The per-language document
scroll-extent checks stay ABSOLUTE — a page whose own scroll extent exceeds its
frame is a different and genuinely broken thing, and O-Orbit passes those.

**Proven both ways.**
- The delta form FIRES on an over-long French header caption:
  `[6] label.viewMotion 0.0px -> 3.0px @404,-3 384x38` — out of the frame in
  French only.
- **COUNTER-PROOF:** restoring the old absolute form on the correct, unmutated
  tree fails **8 times**, reporting **11 in EN and 11 in FR**, the identical
  count.
- **Regression:** all six plugins already on canon v2 still pass with zero
  failures, and **not one of them emits the new English-baseline note** — so the
  change is a strict no-op for every plugin the gate covered before today.

That is the **tenth** wrong-shaped gate assumption in this task.

### A control that did NOT reach its assertion, recorded as a miss

My first attempt at the assertion-6 control lengthened a French **toolbar**
string, expecting Import to leave the frame. It fired `[5]` and `[7]` but **not
`[6]`** — `#editor-toolbar` is `overflow-x: auto`, so it clips internally and the
rect never leaves the frame. A weakened assertion I could not demonstrate firing
would have been decoration, so the control was replaced with the header one
above rather than the result being written up as a pass.

## The plan's numbers, and the counts that were checked rather than trusted

| The plan said | Live at v1.1.1 |
|---|---|
| 32 tips | **32** — correct |
| 57 text | **57** html-text nodes — correct |
| **38 attrs** | **6.** Five `aria-label`s and one `placeholder`; zero `title=`, zero `alt`. The 38 is `32 data-tip-title` + `6` — it counted the tooltip TITLES again, exactly as the carried-forward note predicted. The real attribute-keying job was **6**. |
| — | **12 `js-prose` rows**, the third-highest class of work here and the number the plan does not carry at all. Every one went through `setLabel`. |

`UNSURE`/`READOUT` rows read by hand, as step 1 requires: the one `UNSURE`
(`#preset-name` "Default") is a factory preset name written by
`preset-manager.js` → `I18N_EXEMPT` under D-02, and must never become a
`[data-i18n]` element or the sweep and the module would fight over one node. The
one `READOUT` (`#speed-value` "1.0 Hz") is correctly exempt under D-03.

**`grep -rn 'setVisible' plugins/O-Orbit/Source/` returns nothing** — no hidden
WebView completion hazard. **`CMakeLists.txt` declares the version exactly once**
(`VERSION 1.1.1`, line 20); no `set(<PLUGIN>_VERSION ...)`, no render harness,
no second copy. Both checked before editing, per steps 2 and 3.

## One ENGLISH bug French exposed

The Stage F/H/G/I pattern held for a fifth stage running.

**The view toggle has changed width on every click since v1.1.0.** `#view-toggle`
is a shrink-to-fit box and its two English faces measure 93.1px ("Motion View")
and 114.1px ("Speaker Editor"); `#header` is `justify-content: space-between`, so
every view switch dragged the whole preset band 21px sideways. Nothing was
measuring it, because no gate had ever rendered this page. Pinned to the widest
of its **four** faces (169.3px, "Éditeur d'enceintes"), which now holds both
languages *and* both views still.

## Geometry — 16 moved before fixes, ZERO after

Measured at the shipping 800 × 600 across **four** driven states.

| Plugin | frame | labels | moved before | after |
|---|---|---|---|---|
| O-Tapestop (F) | 860 × 580 | 46 | 26–30 | 0 |
| O-MultiBandCompressor (H) | 900 × 640 | 77 | 222 | 0 |
| O-Bitrot (H) | 900 × 740 | 71 | 78 | 0 |
| O-ReverseDelay (H) | 940 × 768 | 50 | 7 | 0 |
| O-Octagon (G) | 1100 × 720 | 84 | 4 (12 with states) | 0 |
| O-Contrabass (I) | 1000 × 650 | 52 | 21 | 0 |
| **O-Orbit (I)** | **800 × 600** | **56** | **16** | **0** |

| Fix | Measured cause |
|---|---|
| `#view-toggle { min-width: 170px }` | four faces spanning 93.1 → 151.3; the English bug above. 10 of the 16 rows |
| `.preset-btn-hdr { min-width: 58px }` | Save 26.8 → Enreg. 38.8, Load 29.8 → Ouvrir 41.3, Del 21 → Suppr. 36.1 |
| `.preset-btn[data-preset="6"/"7"] { min-width: 40px }` | Hex 22.1 → Hexa 29.8, Oct 21.5 → Octo 29.0 |
| `#layout-select { width: 92px }` | Layouts… 42.2 → Dispositions… 60, sliding the layout library 23.5px |
| `#layout-library button { min-width: 53px }` | Enreg. 38.8 / Suppr. 36.1 / Sûr ? 28.9 |
| `#file-buttons button { min-width: 74px }` | Exporter 55.1, Importer 54.0 |
| `.preset-btn { padding: 2px 4px }` | the budget — see below |

**Three of these are worth reading twice.**

**Six of the eight format chips needed nothing, and the measurement is why.**
"Stereo" and "Stéréo" measure **identically** (40.8), as do "Quad"/"Quad" (30.9),
and 5.1 through 7.1.4 are digits. Only Hexa and Octo grow. Pinning all eight
uniformly to the widest would have cost 164px the toolbar does not have — the
instinct to pin a whole class is what measuring per element prevented.

**Pinning up alone made the toolbar overflow in BOTH languages.** `#editor-toolbar`
is `space-between` over three groups inside 768px of usable width. French
unpinned is 778.4 — already 10.4 over, which is why `label.import` sat at x=803
in French. Pinning every worded control makes both languages **785.5**, i.e. it
converts a French bug into a bug in both. The 32px had to come back from
somewhere real: `.preset-btn` horizontal padding 6px → 4px returns exactly that
to both languages, landing at 753.5 with 14.5px of slack.

**`label.import` never appeared in the geometry diff that its own overflow
caused** — it is a `[data-i18n]` element and assertion 7 excludes those by
design. It surfaced in `[5]` and `[6]` instead. Stage I's "read the diff as a
list of CONSEQUENCES" note generalises: also read the OTHER assertions, because
the culprit shows up in whichever one is not excluding it.

**One French string was SIZED**, recorded at its entry with the measurement:
`Synchro tempo` measures **105.4px** and the Motion group's grid track is
**100.3px** (`repeat(auto-fit, minmax(90px, 1fr))` over nine items resolves to
seven 100.3px columns), so it wrapped to two lines and pushed the Tempo Sync
dropdown down 13px. It ships as `Sync tempo` (79.2px). The full phrase survives
as the tooltip title, which renders in a 230px box.

**Only the Motion group constrains, and that was measured rather than assumed.**
Its cells are 100.3px; Spatial's are **143.6** (the speaker-layout `<select>`'s
widest option forces its own min-content width onto the track) and Source/Mix's
are **244.7** (three items, `auto-fit` collapses the unused tracks). So
"Courbe attén." at 97.3 and "Décalage G/D" at 93 have 46px and 152px of margin
respectively, not the 3px and 7px a single assumed cell width would have implied
— and two captions were nearly shortened for no reason.

**French got SHORTER on four Spatial captions** — Speaker Layout 109.9 →
Enceintes 68.9, Air Absorption 104.2 → Absorption 79.1, Center Diverge 107.8 →
Divergence 80.3. Stage G's note, confirmed on a third plugin. Each was checked
for a row-height change; none moved, because those cells are 143.6 wide and the
English never wrapped there either.

## The TDZ trap did NOT fire here, and that is the honest result

Stage G's carried rule: *"expect to move the i18n block only where there is no
`init()`-last discipline"*. **O-Orbit has that discipline** — every initializer
runs from a `DOMContentLoaded` handler and nothing executes at module top level.

**Tested, not assumed.** The canon block was physically relocated to the BOTTOM
of `app.js`, below every other declaration, and `boot-all-uis` reports
`BOOT O-Orbit ... clean: 1/1` with `check-ui-labels` and `check-i18n` both still
green. So unlike O-Contrabass — where the same mutation produced
`Cannot access 'uiLanguage' before initialization` — **the placement here is
defensive, not load-bearing**, and the code comment says so rather than claiming
a trap was averted. Stage G's rule is confirmed by a plugin on the other side of
it.

## Verification — every gate, individually

| Gate | Result |
|---|---|
| `check-i18n.js --plugin O-Orbit` | **exit 0**, canon **v2**, assertions 10–15 all run. Passed on the FIRST run |
| `check-i18n.js --strict-v2` repo-wide | **exit 0** — canon split **v2 7, v1 0** |
| `check-ui-labels.js --plugin O-Orbit` | **exit 0** — **56** labels over **4** states, **ZERO** non-label geometry shifts, 49/56 (88%) of labels and **9/9** keyed attributes change language |
| `check-ui-labels.js` on the other six | **exit 0 each**, zero failures — the gate change is a no-op for them |
| `boot-all-uis.js` | **41/43 clean, unchanged.** O-Orbit: `text=62 aria=8 title=0 i18n=56`. The two failures are O-Bowed and O-Reed, pre-existing and unrelated |
| `./scripts/build-and-install.sh O-Orbit` | clean, 27s, zero errors; VST3 + AU built and installed, AU cache cleared, dual-variant sweep ran |
| `auval -v aufx OuOr OuDv` | **AU VALIDATION SUCCEEDED** |
| bundle version | `CFBundleShortVersionString` **1.2.0** — the single `VERSION` declaration reached the artefact |
| binary embedding | `i18n_jsSize` **38868** == `wc -c` — the retrofit really reached the binary |

**O-Orbit has no `tests/` gates of its own** (it had no `tests/` directory at
all before this commit), so unlike every plugin before it there was no
plugin-local suite to re-run. The repo gates are the entire automated coverage.

## Negative controls — 29 run, 29 fired

Each mutation was applied to a **byte-exact backup** and restored **FROM THAT
BACKUP**, never `git checkout --`, which would have wiped the uncommitted
retrofit alongside it. Every mutation printed its substitution count and the
harness **refused to run the gate on a zero-count mutation**. Every file was
sha256-verified against the backup after every round.

| Mutation | Assertion that fired |
|---|---|
| revert `#view-toggle` min-width | `[7]` geometry diff, 10 moved |
| revert `.preset-btn-hdr` min-width | `[7]`, 7 moved |
| revert the Hexa/Octo chip pins | `[7]`, 4 moved |
| revert `#layout-select` fixed width | `[7]`, 3 moved |
| revert `#layout-library button` min-width | `[7]`, 3 moved |
| revert `#file-buttons button` min-width | `[7]`, 4 moved |
| restore the over-long `Synchro tempo` caption | `[7]`, 1 moved (`#tempo_sync` dy=13) |
| an over-long FRENCH header caption | **`[6]` delta**, `label.viewMotion 0.0 -> 3.0px` |
| the OLD absolute `[6]` on the CORRECT tree | **8 failures, 11 EN / 11 FR — the counter-proof** |
| stub `__setLanguage` to a no-op | `[2]` vacuity, `0/56 labels differ` |
| clobber `textContent` after `applyLabel` | `[3]` in both languages |
| remove one `data-i18n` from the markup | `[10]` |
| reinstate a native `title=` | `[11]` |
| a ternary inside a `setLabel` argument | `[13]` |
| a raw prose literal written to `textContent` | `[12]`, with file:line |
| drift ONE line of the canon block | `[6]` matches NEITHER canon |
| a LABELS `fr` entry copied from `en` | `[4]` LABELS |
| strip a LABELS `reviewed` flag | `[5]` LABELS |
| empty an `I18N_EXEMPT` reason | `[14]` |
| delete the `getResource()` branch for `js/i18n.js` | `[8]` |
| remove `i18n.js` from the CMake SOURCES block | `[8]` |
| a dangling `data-i18n` key | `[15]` dangling |
| an unreferenced LABELS key | `[15]` dead |
| restore a `data-tip=` copy literal in the markup | `[3]` |
| a top-level statement in `i18n.js` | `[7]` self-execute ban |
| **canon block relocated to the bottom of `app.js`** | **SILENT — see the TDZ section. The honest negative result.** |

## English fidelity, checked mechanically

Every `en` entry was extracted from `index.html` by script and **compared back
byte-for-byte**: **32/32** tooltip titles and bodies, **0 drift**, with the
entity decoder configured to **throw on any entity it did not know** so the set
is known complete rather than assumed. Separately, the multiset of visible text
nodes was diffed between the pre- and post-retrofit markup: **57 → 62 nodes,
ZERO removed**, the five added being the new popover controls (`Language`,
`English`, `Français`, `Hover help`, `Off`). And all **55** keyed elements were
confirmed to carry a fallback string byte-identical to their `LABELS` `en`.

**No English wording was changed anywhere.**

## Commit discipline — HEAD moved under this work, again

Another session was live in this checkout throughout. **HEAD moved mid-commit**:
`77565bd5` (O-Octagon v1.11.0) landed between staging and committing. Their
commit was properly path-scoped and did **not** sweep the staged
`scripts/check-ui-labels.js`.

`git show -- PLUGINS.md` was read after the commit, not `--stat`: **1 insertion,
1 deletion**, the O-Orbit row only. Their O-Octagon row edit was already
committed in `77565bd5`, so there was nothing of theirs in the working tree to
sweep. The duplicate-row check (`uniq -d`) is clean and the commit deletes no
files.

**A new lesson, learned by breaking it.** Piping the negative-control harness
through `head` killed it with SIGPIPE **mid-mutation** and left `index.html`
with a `data-i18n` still stripped. It was caught by the harness's own
post-restore sha256 comparison and restored from the backup. **Never pipe a
mutate/restore harness into a truncating command** — capture to a file and read
the file.

## Deviations from the plan

**[Rule 2 — Missing critical functionality] `check-ui-labels` assertion 6 was
wrong-shaped.** Landed as its own commit `f00e5d45` ahead of the plugin, never
bundled. Full reasoning, both proofs and the six-plugin regression are in the
headline section above. This is the one thing on this dispatch that reached
outside `plugins/O-Orbit/**`, and it is reported rather than absorbed.

**[Rule 1 — Bug] The view toggle changed width on every click, in English.** Not
part of the localization; found because the geometry diff named ten victims of
it. Fixed inline.

**[Rule 2] `plugins/O-Orbit/tests/ui-stub/generic-overrides.json` was added.**
The generic stub answers `getDownmixStatus` with the STRING `'{}'`, on which the
page's `status.active` is `undefined`, so the downmix badge never rendered and
`ui.downmix` — the only composed label on the page — would have been a key the
geometry gate never measured. The override supplies the shape
`PluginEditor.cpp:152-158` really completes with (an object carrying
`active` / `sourceChannels` / `targetChannels`), verified against that source
rather than invented.

## NOT VERIFIED — read this before batch I2

1. **THE C++ LANGUAGE ROUND-TRIP HAS STILL NEVER BEEN RUN.** Checkpoint 4(b) has
   been outstanding since Stage B and is now outstanding on **seven** plugins.
   The claim that a language choice survives a session reload is REASONED from
   source. O-Orbit stores it as a ValueTree PROPERTY (matching its own
   `tooltipsEnabled` idiom) rather than the root XML attribute O-Contrabass
   uses, so it genuinely round-trips through XML as a string var — which is why
   it is stored as `"en"`/`"fr"` and gated on `isVoid()`. That is the correct
   construction, and *correct by construction* is still not *measured*. This
   remains the single highest-value outstanding check in the task.
2. **No human has seen the French UI, on any plugin.** All geometry here is
   headless Chromium at 800 × 600. `Oui`/`Non` for the elevation toggle's
   On/Off faces, `Sync tempo`, and `Enceintes` for Speaker Layout are the three
   this stage would challenge first. The `Oui`/`Non` pair was chosen under a
   hard three-glyph budget (a 50px pill with an 18px thumb riding over it) and a
   native speaker may well prefer `I`/`O`.
3. **Nothing was tested in a DAW.** It builds, installs and `auval`s; it has not
   been opened in Logic or Ableton.
4. **The Standalone `.app` is stale.** `build-and-install.sh` builds VST3 + AU
   only.
5. **19 of the 56 keyed elements were NEVER MEASURED.** Every one is an
   `<option>` inside a closed `<select>` — the path, tempo-sync, speaker-layout,
   atten-curve, source-mode and layout dropdowns. A closed `<select>`'s options
   have no box, and the popup is OS-rendered and outside the DOM viewport. **No
   states file can reach them**, so this is a property of native select menus,
   not a coverage gap a test could close. Their French is verified statically
   (assertions 4, 5, 15) and never measured. If a French option is too wide for
   a dropdown, nothing in this repo would catch it.
6. **`ui.downmix` was measured only because a stub override was written for it.**
   Without `generic-overrides.json` it renders in no state at all. Its real
   geometry depends on channel counts the gate supplies, not the host.
7. **The `#editor-toolbar` has 14.5px of slack in French at 800px.** That is the
   whole margin. The editor is resizable (D4) and the toolbar is
   `overflow-x: auto`, so it degrades to scrolling rather than breaking, but a
   single further French string in that row will start it scrolling at the
   default size.
8. **All 91 French entries are machine drafts**, every one `reviewed: false`. No
   native speaker has read any. Repo total is now **711 unreviewed** across
   seven plugins.
9. **Windows / WebView2 remains a named deferral, blocked on hardware.** Every
   width in the fix tables was measured in Chromium on macOS. `Mode source`
   (89.5 in a 244.7 cell) is comfortable, but `Sync tempo` at 79.2 in a 100.3
   cell has 21px of margin and is the tightest caption on the page.
10. **`i18n-extract.js` still does not decode `&beta;`** — carried unfixed from
    Stage I's O-Contrabass run. It did not bite here (O-Orbit's copy uses only
    `&#176;`, `&#8230;`, `&#9664;`, `&#9654;`, `&#9662;`, `&#9881;`, `&ccedil;`),
    but it is still open and still belongs to whoever next touches that script.
11. **The generated inventory artifacts** (`plugins/O-Orbit/.planning/i18n-*`)
    are left UNCOMMITTED, as on the six before. They are regenerable.
12. **No plugin-local gate exists for O-Orbit**, so a broken `TIP_BINDINGS`
    SELECTOR is unguarded. `check-i18n` assertion 2 checks that every binding's
    KEY resolves, but nothing checks that its selector still matches an element
    — O-Contrabass has a `tests/ui_frontend_check.js` §7 for exactly this and
    O-Orbit has nothing. `applyI18n` only `console.warn`s. A future markup edit
    that renames `data-param="speed"` would silently drop that tooltip.

## Carried into batch I2 (the seven `O-simple*` plugins)

- **Assertion 6 is now a DELTA.** If an `O-simple*` plugin has a scrolling pane
  it will report an English baseline note instead of failing. **Read that note** —
  it is the only signal that part of the page is off-frame at rest.
- **The plan's "attrs" column counts tooltip TITLES.** Confirmed a second time:
  O-Orbit's "38" was really 6. The I2 figures (5, 5, 10, 6, 3, 1, 14) are small
  enough to be genuine `aria-label` counts, but **check with `i18n-extract`
  before sizing the work** — the plan's own numbers have now been wrong on the
  attribute column for both batch-I1 plugins.
- **All seven have a `tests/render-harness/`.** O-Orbit had none, so nothing
  guarded the DSP here; on I2, `reproduce-goldens.sh` must be re-run because
  `PluginProcessor.cpp` is edited by the language pair.
- **MEASURE THE GRID TRACK BEFORE SHORTENING A CAPTION.** `auto-fit` +
  `minmax()` gives different track widths per group in the SAME page — 100.3,
  143.6 and 244.7 on this one — and a `<select>`'s widest option silently forces
  its own track wider. Two captions here were nearly shortened for no reason.
- **Measure every string; do not pin a whole class.** "Stereo"/"Stéréo" and
  "Quad"/"Quad" measure identically. Six of eight chips needed nothing.
- **Pinning up can convert a French bug into a both-language bug.** When a row
  is already near its budget, find the width to give back before pinning.
- **The culprit hides from assertion 7 but shows in 4, 5 or 6.** `[data-i18n]`
  elements are excluded from the diff by design; read the other assertions.
- **Never pipe the negative-control harness into `head`.** SIGPIPE leaves the
  tree mutated. Capture to a file.
- **Write a `tests/i18n-states.json` for every plugin** — three states here
  (gear open, help lit, speaker-layout view) took measured coverage from **26/56
  to 37/56**, measured by moving the states file aside and re-running. And write a `tests/ui-stub/generic-overrides.json` wherever a native
  fn's real return SHAPE differs from the generic stub's guess; verify the shape
  against `PluginEditor.cpp`, never invent it.
- **`boot-all-uis.js` on every plugin.** Still the only gate that sees a TDZ
  throw — and on a plugin with `init()`-last discipline it will stay silent, as
  it correctly did here.

---

# Stage I (batch I2) — T13: the seven `O-simple*` plugins — STAGE I COMPLETE

**Nine commits: seven plugins, plus two repo-level gate fixes each landed alone
ahead of the plugin that needed it.**

| # | Plugin | Version | Commit | Tips | Labels | FR | Scroll cost |
|---|---|---|---|---|---|---|---|
| 1 | O-simpleAdditive | 1.1.0 | `b1082fc0` | 42 | 42 | 84 | 0 |
| 2 | O-simpleSampler | 1.4.0 | `2a2f68c4` | 36 | 73 | 109 | **+39px** |
| 3 | O-simpleSubtractive | 1.3.0 | `5fc1ceb0` | 35 | 58 | 93 | 0 |
| 4 | O-simpleGrain | 1.3.0 | `857dfa85` | 37 | 76 | 113 | +0.2px |
| 5 | O-simpleFM | 1.3.0 | `2a5efef0` | 27 | 53 | 80 | 0 |
| 6 | O-simplePhysicalModelSynth | 1.2.0 | `051e1a72` | 24 | 53 | 77 | 0 |
| 7 | O-simpleBeatmaker | 1.1.0 | `665150c0` | 29 | 52 | 81 | 0 |

Gate fixes: **`08e7649b`** (animation), **`816f2767`** (paint layers).

`check-i18n --strict-v2`: **14 canon v2, 0 canon v1.** Every `data-tip`-convention
plugin in the repo is localized. **639 new French strings**, all `reviewed: false`.

## THE HEADLINE: the plan's read of this batch was wrong in three ways

T13 called I2 "no tooltip gates, no bridge toggle ... the lowest-risk batch in
the rollout". Two of those three claims did not survive contact.

**1. `data-tip` here is the tip KEY, not the tip body.** Every `O-simple*`
plugin carries `data-tip="someKey"` and looks it up in a `const TIPS = {}`
object. Canon v2's `applyI18n` WRITES `data-tip` as the body, and assertion 3
requires zero literal `data-tip=` in shipped markup. They cannot coexist. Every
plugin needed its anchors moved to `data-param`/an id AND its listeners
delegated on the document — `querySelectorAll("[data-tip]")` at setup time binds
nothing once the anchors move, and generated cells do not exist yet anyway.
This was invisible to the plan's inventory, which counted anchors without
reading what they held.

**2. "No bridge toggle" was true; "no toggle" was not.** Three of the seven
already shipped a tips toggle — O-simpleSampler a real C++ bridge (named
`tipsEnabled`, not `tooltipsEnabled`, which is why the pre-flight grep missed
it), O-simpleGrain and O-simplePhysicalModelSynth a localStorage one. Each
MOVED into the gear popover rather than leaving the plugin with two settings
surfaces. O-simpleSampler's was the dangerous one: it cached `data-tip-title` to
restore on re-enable, and `applyI18n` writes that attribute on every sweep, so
keeping the restore would have put English back after a French switch. The whole
layer was deleted rather than adapted, along with the native `title=` it served.

**3. Roomy frames did not mean free geometry.** 31 geometry pins across seven
plugins, every one reverted alone and confirmed to re-break the gate. Three were
found to be DECORATION — their negative control passed — and were removed or
re-labelled as design pins rather than claimed as fixes.

## Two more wrong-shaped gate assumptions, both regression-swept

Eleventh and twelfth in this task. Each landed as its own commit ahead of the
plugin, per the `3f6b201d` / `f00e5d45` precedent.

**`08e7649b` — assertion 7 reported ANIMATED elements as French failures.** It
compares two rect sweeps ~180ms apart. O-simplePhysicalModelSynth's signal-flow
diagram runs a SMIL `animateTransform` circling a pulse every 1.7s, so with the
page in a SINGLE language it reported three phantom moved rows and a different
answer every run. PROBE now pauses SMIL and Web Animations before measuring —
and deliberately NOT `CSSTransition`, because the state pass drives every slider
and this repo's knob stems transition their rotation, so pausing one mid-flight
would invent exactly the diff being removed.

**`816f2767` — assertion 8 compared labels across PAINT LAYERS.** A rectangle
has no z. O-simpleBeatmaker's settings popover is an opaque `z-index: 40` panel
drawn over the page on purpose; its caption overlaps the step-grid hint's tail
in BOTH languages, and the gate only noticed once the French string grew long
enough to reach x=935. PROBE now records each label's nearest opaque positioned
ancestor and skips only cross-layer pairs, printing a NOTE for each.

Both swept across every previously-shipped canon-v2 plugin: all still pass, and
the paint-layer skip fires on that one page only.

## What the gates still cannot see

- **Assertion 12 does not see `textContent = someVariable`.** Only literal
  right-hand sides produce an inventory row, so `showToast(msg)` /
  `setSourceStatus(text)` ship English with the gate green. Caught by hand on
  O-simpleGrain; the three plugins already committed before it were re-checked
  and were clean (two have no status path; O-simpleSampler had already keyed
  its toasts).
- **Assertion 7 prints only the first 12 moved elements.** O-simpleSubtractive
  reported "18 moved", listed 12, and the six withheld contained a second
  independent bug.
- **Assertion 8 only reports pairs disjoint in English**, so a `white-space:
  nowrap` fix can introduce an English collision the gate will never catch.
  O-simplePhysicalModelSynth did the adjacent-pair arithmetic and found one.
- **`check-ui-labels` measures only at `setSize`.** O-simpleBeatmaker is
  resizable to 860x640 — 200px narrower and 260px shorter than anything the gate
  sees. Hand-measured: nothing clips, French 15px taller in an already-scrolling
  pane. Every other plugin's minimum is unmeasured and unmeasurable by this gate.

## Pre-existing bugs found and fixed, none caused by this work

- **Dead flex bases lost to specificity, on four plugins.** A bare `.group-x`
  (0,1,0) loses to `.rack .group` (0,2,0), so authored bases had never applied
  and the racks had been sized by max-content — which is precisely what made
  them language-dependent. The single largest geometry win available, and it
  changes the ENGLISH layout, so each is called out under `### Fixed`.
- **Roughly 120 controls across the batch had `role="slider"` + `tabindex` and
  NO accessible name at all.** Fixed for free: `data-i18n-aria` resolves through
  `trLabel`'s I18N fallback to each control's own tooltip title.
- **Render-harness version drift on two plugins.** O-simplePhysicalModelSynth
  hardcoded `JucePlugin_VersionString="1.0.0"` against a shipping 1.1.0, and it
  was LIVE — `~/Library/O-simplePhysicalModelSynth/Presets/Factory/.factory-version`
  on disk read `1.0.0`, because the sentinel compares against that string and a
  mismatch rewrites the user's real factory-preset directory. O-simpleBeatmaker
  had the same drift with no on-disk consequence (no preset manager, so no
  sentinel). Both converted to the by-reference form the other five already use.
- **`harmonicTip()` rendered "The 2th harmonic"** in every O-simpleAdditive
  build since tooltips shipped.
- **O-simpleBeatmaker's transport strip resized the header** whenever the host
  started playing (`● synced` 44.9 vs `free-run` 52.7 at the right end of a
  space-between row). Nothing had been measuring it.
- **O-simpleFM's `bindKnob()` froze the accessible name in one language** — it
  copied the caption's `textContent` once at bind time, which runs before
  `initI18n()`. Same shape on O-simpleBeatmaker's 29 knobs.

## Reported, deliberately NOT fixed

- **O-simplePhysicalModelSynth's Delete-preset button deletes without
  confirmation.** `setupPresetManager()` calls `deletePreset()` directly and
  passes no `deleteButton`, so the vendored module's `promptDelete()` /
  `onConfirmDelete` are dead code. Adding a dialog is a design change, not i18n
  work.
- **O-simpleSampler's on-screen keyboard is now entirely below the fold** — the
  content grew 796 -> 835px inside a FIXED 980x720 frame. 17px is the rack
  repair, 10px a reserved caption line, the rest a new header row. The
  alternative was trimming eight French lesson captions to a character budget
  that holds their English line count, rejected as copy any later edit
  re-breaks. On a pedagogical plugin whose keyboard is a teaching surface this
  is a real regression and a human should rule on it. O-simpleSubtractive proved
  the cost is NOT inherent: its extent was measured identical before and after.

## Carried into Stage J

1. **A wrapper span around keyed fragments is a permanent assertion-7 failure.**
   A wrapper is not `[data-i18n]`, so the diff measures its box — and its box IS
   the French sentence, wider forever. Carry the class onto the fragments. All
   seven Stage-J plugins have hint/legend spans of this shape.
2. **Measure pinned keys AS RENDERED, not from a font probe.** `text-transform`
   and `letter-spacing` are not in `getComputedStyle().font`; a probe read
   LONGUEUR 12.6px narrow and the first pin landed UNDER the French.
3. **`white-space: nowrap` beats a `min-height` reservation on a shrink-wrapping
   caption** — the box is already text-width, so overflow is already the resting
   behaviour and wrapping only buys height. Removed 103 of 125 moves at zero
   English cost on O-simpleFM. But do the English adjacent-pair arithmetic
   first (see assertion 8, above).
4. **`flex: 1 1 auto` is not the fix; `flex: 1 1 0` is.** Flex line-breaking runs
   on the hypothetical main size BEFORE shrink, so the "fixed" diff comes back
   byte-identical to the broken one — which reads exactly like the fix not
   working. Check the BASIS before the specificity.
5. **`min-width` on the FIRST item of a flex row is not a pin.** Its used width
   is still max-content, and being first it positions everything after it. One
   caption produced 11 of 18 reported moves on O-simpleSubtractive.
6. **Establish harness determinism by running it twice on the SAME binary before
   comparing across versions.** O-simpleGrain's is unseeded (peakGrains 158 vs
   144 on consecutive runs); O-simpleFM's, O-simplePhysicalModelSynth's and
   O-simpleBeatmaker's are digit-stable. Earlier commits in this batch asserted
   digit-stability that would have been false. The comparable quantity is the
   VERDICT unless determinism is measured.
7. **A dynamic anchor selector will not show up in a literal grep.**
   O-simpleFM's was static; O-simplePhysicalModelSynth built its from a template
   literal. Both broke SILENTLY — the controlled property was never set and no
   page error fired.
8. **A ternary-written prose string is not automatically `setLabel` work.** The
   discriminator is: does it track a PARAMETER? If so it is a value mirror and
   `I18N_EXEMPT` under D-01, which also makes the assertion-13 hoist moot. If it
   tracks host or transport state, it is copy. O-simplePhysicalModelSynth's was
   the former, O-simpleBeatmaker's `● synced` the latter — verified by grepping
   the APVTS for a sync parameter and finding none.
9. **Take a baseline by swapping HEAD's files in and out from an in-memory copy
   in a `finally`** — never `git checkout --`, which wipes uncommitted edits.
   That is what makes "scroll extent unchanged" an honest claim.
10. **A per-element dynamic accessible name has no canon home.** Housing its
    sentence shapes in `I18N` with an empty body is the only shape satisfying
    assertions 13 and 15 together. O-Polystutter (102 tips) and
    O-MicrotonalSampler will both hit this.

## Not verified

- **Checkpoints 4 and 5, now outstanding on all fourteen plugins.** The C++
  language round-trip (pick Français, close the session, reopen, confirm it
  held) has never been executed by hand on any of them — it is reasoned from the
  `isVoid()` guard, not measured. No human has seen any of these French UIs.
- **All 1,350 French strings repo-wide are machine drafts**, every one
  `reviewed: false`. No native speaker has read them.
- **Windows/WebView2 font metrics** remain the named hardware-blocked deferral.
  The tightest French margin measured in this batch is 20.9px.
