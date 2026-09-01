---
task: 260826-ieq-multi-language-tooltips-across-all-vst-p
type: execute
mode: quick
status: incomplete
stages_complete: [A, B, C, D, E, F, G, H, I, J, K, L, M, N]
stages_remaining: []
decision_items_closed: [1, 4, 17, 18, 26, 27, 29, 68]
checkpoint_5_french_review: DONE — Stage N second reading on 43/43, then read by the developer (reads French); 3702 flags reviewed: true
i18n_exempt_contract: RESOLVED — scoped entries landed between K1 and K2
stopped_at: "STAGE N COMPLETE 2026-08-31 — Checkpoint 5 as a QA pass, 43 of 43 plugins patch-bumped: every French entry read against its English, scripts/i18n-fr-glossary.js (~260 terms) and scripts/i18n-fr-lint.js. Repo-wide lint --strict 2145 -> 0 on 43/43; boot-all-uis 43/43 clean, 0 DEAD, 19 late (by design), title= 0; check-i18n ALL PASS, 3751 entries; 0 non-label elements moved on every page; auval PASS x43; installed Info.plist audited. 12 lint defects and 1 check-ui-labels gate defect (8b painted rects) found by executors and fixed; 2 glossary terms wrong of ~260; 20 of 43 header width defences proven backwards. STILL OPEN: item 27 (reviewed: false on all 3751 — no native speaker), item 29 (--strict is green and unwired), item 31 (17 plugins without a committed render gate), items 19/22/30/34-68 per-plugin English/CSS/DSP defects. Status stays incomplete only because Checkpoint 5 as WRITTEN (a native speaker) has not happened."

plugins_shipped:
  - name: O-Gain
    version: 1.3.0
    commit: f571a78e      # v1.3.0, Stage J — LAST of the stage. NARROWEST frame in the repo
                          # (350x500). The plan's structural claim was FALSE: O-Gain had no
                          # tooltip JS at all — a pure-CSS [data-tooltip]::after with three
                          # hand-picked direction classes. 23 of its tips were IN-FLOW boxes
                          # inflating the doc's own scroll extent to 435x540 inside a 350x500
                          # window, in ENGLISH, at rest. Port fixes that: now exactly 350x500.
                          # NO D-04 stop was needed — 41 French labels DO fit.
                          # 23 of 23 tooltips hand-split (zero clean).
  - name: O-Marimba
    version: 1.13.0
    commit: 5406f419      # v1.13.0, Stage J. Produced the READOUT THIRD ARM to the D-01 test:
                          # six timbre words backed by AudioParameterFloat still exempt, because
                          # they are written into the KNOB READOUT node (contract 5). Also proved
                          # the byte-identity discriminator against Choice option strings.
                          # Plan claimed it had the tooltips bridge; it did NOT (window.JuceAPI
                          # has never existed there). Preceded by gate fix 91d81cf7.
  - name: O-IntonationPad
    version: 2.9.0
    commit: bb275ae1      # v2.9.0, Stage J. FIRST plugin whose C++ round-trip was MEASURED, six
                          # ways, incl. a real Standalone launch-and-quit and three negative
                          # controls. Built the language bridge FROM SCRATCH. 77 live anchors /
                          # 74 unique vs the plan's "~37". Hand-splits 63 of 77. Found a DEAD
                          # tooltip (TOOLTIPS.voicingMode, applied to a knob that never existed).
                          # Preceded by gate fixes 1151de07 and 435957a7.
  - name: O-SpectralShaper
    version: 1.7.0
    commit: 5b509e8c      # v1.7.0, Stage J. The #app-relative positioner replaced, not adapted:
                          # #app's padding:12px made the old rails 24px NARROWER than the window
                          # and the vertical rail read containerRect.height. Zero visible English
                          # elements moved v1.6.2 -> v1.7.0. Its committed tests/ui-stub taught
                          # both language names. Caught a hard-coded v1.6.1 header on a v1.6.2 build.
  - name: O-Lyrica
    version: 2.4.0
    commit: 09bdbb61      # v2.4.0, Stage J. Heaviest text load in the stage (181 nodes / 700x450).
                          # 56 I18N_EXEMPT entries, 46 of them AudioParameterChoice options.
                          # 12-TET Standard stays English as a tuning IDENTIFIER; Factory -> Usine
                          # as a preset-group heading; Custom exempt TWICE OVER. i18n.js joined the
                          # EXISTING no-NAMESPACE binary-data target. Preceded by gate fix b0ce697e,
                          # which its own sweep then proved LATENT across all 16 prior plugins.
  - name: O-Polystutter
    version: 1.14.0
    commit: 66456f1e      # v1.14.0, Stage J. THE SOURCE of the four hard-coded literals, all now
                          # deleted: tooltipHeight=60, tooltipWidth=220, >660, >1000. Inline
                          # renderer extracted from index.html to js/app.js (596 lines). Housed a
                          # homeless composed string (the delete-preset confirm, {name} token) in
                          # I18N with an EMPTY BODY — carried item 10, but not where Stage I predicted.
                          # Tightest boxes on the page are ENGLISH (SUBDIV 3.69px), not French.
  - name: O-FreqPulse
    version: 1.18.0
    commit: 37602894      # v1.18.0, Stage J — FIRST of the stage. FOUND THE VERTICAL CLAMP GAP:
                          # the O-ReverseDelay renderer prefers above, flips below, and STOPS,
                          # which strands a tip off-window on a tall anchor (376px #grid-area put
                          # a French tip 15px below the frame). Clamp added and proven by reverting
                          # it alone; then ported into all six remaining Stage-J plugins.
                          # 36 of its 56 tip anchors GENERATED from BAND_IDS so bands cannot drift.
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

---

# Stage J — T14: the seven second-renderer plugins — STAGE J COMPLETE

**ONE TOOLTIP RENDERER REPO-WIDE.** The third stop point named in the plan is reached.

| # | Plugin | Version | Commit | Root | Frame |
|---|---|---|---|---|---|
| 1 | O-FreqPulse | 1.18.0 | `37602894` | `Resources/ui` | 850x550 |
| 2 | O-Polystutter | 1.14.0 | `66456f1e` | `Source/ui/public` | 1000x690 |
| 3 | O-Lyrica | 2.4.0 | `09bdbb61` | `Resources/ui` | 700x450 |
| 4 | O-SpectralShaper | 1.7.0 | `5b509e8c` | `Resources/ui` | 700x500 |
| 5 | O-IntonationPad | 2.9.0 | `bb275ae1` | `Source/ui/public` | 800x500 |
| 6 | O-Marimba | 1.13.0 | `5406f419` | `Source/ui/public` | 600x400 |
| 7 | O-Gain | 1.3.0 | `f571a78e` | `Source/ui/public` | **350x500** |

Plus four gate fixes, each committed ALONE ahead of its plugin: `b0ce697e`, `1151de07`,
`435957a7`, `91d81cf7`.

## Done-criteria, verified by the ORCHESTRATOR independently of the executors

- `grep -rn 'tooltipHeight'` over every served UI root → **0**.
- Every surviving `data-tooltip` occurrence is a **comment** documenting the deleted design.
- `check-i18n.js --strict-v2` → **21 canon v2, 0 canon v1, 0 none**.

## THE HEADLINE: the plan's model of these seven plugins was wrong in four ways

1. **O-Gain had no tooltip JavaScript AT ALL.** The plan says all seven carry the
   never-measures positioner. O-Gain's help was a pure-CSS `[data-tooltip]::after` with
   `content: attr(data-tooltip)` and three hand-picked direction-override classes an author
   chose per anchor — unmeasurable, unflippable, unclampable. Worse than the positioner, not
   better. And because 23 of those boxes were **in-flow**, they inflated the document's own
   scroll extent to **435x540 inside a 350x500 window, in ENGLISH, at rest**. The port fixed
   a pre-existing English defect nobody had measured.
2. **O-Marimba does NOT have the tooltips bridge** the plan credits it with. v1.12.1 called
   `window.JuceAPI.getNativeFunction('setTooltipsEnabled')` inside a try/catch; `window.JuceAPI`
   has never existed there and no such native function is registered. A dead call, removed
   rather than repaired.
3. **Every tip count in the plan is a grep artifact, and they are wrong in BOTH directions.**
   O-Lyrica's "48" was 43 (the token also matches a CSS selector and four JS references);
   O-IntonationPad's "~37" was **77 live anchors / 74 unique** once `makeKnob`-applied and
   injected-template tips were counted. Only O-Marimba's and O-Gain's survived parsing.
   Text-node counts fared worse — O-Marimba's "~40" was 61, the 20-node gap being two shared
   FX modules.
4. **The `"Label: sentence."` shape mostly does NOT hold.** The plan expects "a handful" of
   hand-splits. Actual: O-Lyrica 0 of 43, O-SpectralShaper 0 of 21, O-Marimba 0 of 15 — but
   O-IntonationPad **63 of 77** and O-Gain **23 of 23**. The rule that worked, and should be
   the default in Stage K: **the title is the control's own existing English caption — reuse
   it, never author new prose** — with every body verified byte-identical against `HEAD`.

## The vertical clamp — found on plugin 1, latent in all 21

The renderer as written in O-ReverseDelay prefers *above*, flips *below*, and **stops there**,
because every anchor on that page is knob-sized. Given a tall anchor NEITHER placement fits
and the tip lands off-window. O-FreqPulse's 376px `#grid-area` put a 97px French tip 15px
below the bottom.

Independently reproducible on **O-FreqPulse** and **O-IntonationPad** (which stranded two
French tuning-tab tips by 53px and 67px). NOT reproducible on O-Polystutter, O-Lyrica,
O-SpectralShaper, O-Marimba or O-Gain — measured each time by deleting the two lines and
re-sweeping, not assumed. Ported into all seven regardless, because one runtime repo-wide is
the point of the stage.

**Every "not reproducible" verdict is backed by a harness-blindness check**: delete the
HORIZONTAL clamp instead and confirm the sweep still reports failures. It always did —
O-SpectralShaper 14 off-frame tips, O-IntonationPad 26, O-Marimba 10, O-Gain 26. Without that
control, "no failures" would be indistinguishable from a sweep that cannot see failures.

## The D-01 test needs a THIRD ARM — the most reusable finding of the stage

The plan's discriminator is binary: is the string an `AudioParameterChoice` option? If yes,
exempt (page and host automation lane must agree); if no, localize.

**O-Marimba proved that incomplete.** Its six timbre words — `Edge`, `Center`, `Shimmer`,
`Focused`, `Warm`, `Bright` — are backed by `AudioParameterFloat`, so the binary test says
localize. But they are written into `#strike-value` / `#damping-value` / `#tone-value`: the
knob's **readout**, wearing a word instead of a number at the ends of its travel. Contract §5
is explicit that a readout is never a `[data-i18n]` element. Keying one makes the element
enter and leave the sweep as the knob turns, so a later language change repaints `Chaud` over
`62%`; and its French face exists only at a knob extreme, which no committed gate state can
reach, making D-04 undischargeable.

**The third arm: what ELEMENT receives the string.** Readout node → exempt regardless of
parameter type.

**And the clean discriminator for the choice arm is BYTE-IDENTITY against the option strings.**
`12-TET` and `MTS-ESP` match `TUNING_MODE`'s options verbatim → exempt. `CUSTOM` does not (the
option is `Scala`) → a plain caption, localizes to `PERSO`. O-Gain ran the same test on one
page and landed on both sides: `Peak`/`RMS`/`VU`/`LUFS` match verbatim → exempt;
`M/S OFF`/`ENC`/`DEC` match none of `Off`/`Encode`/`Decode` → page-invented, they localize.

O-Gain then **overruled the third arm with reasons**, correctly: `LOW`/`MED`/`HIGH` in
`#learn-confidence` is copy, because that node never holds a number (the `0` branch writes
language-neutral `--`), `learnConfidence` is not a parameter at all, and a committed gate
state drives all three French faces so D-04 IS dischargeable.

## Checkpoint 4 — MEASURED at last, on two of twenty-one

The C++ language round-trip had been reasoned from the `isVoid()` guard on all nineteen prior
plugins and never executed. Two runs in this stage measured it:

**O-IntonationPad**, six ways: symmetric `comm -3` grep-diff of 31 `getNativeFunction` names
against 31 `withNativeFunction` registrations, empty on both sides; both symbols plus the
French table present in the shipped VST3 binary; a real Standalone launch-and-quit producing
`<Parameters uiLanguage="en">` in the decoded settings blob; a stored `fr` surviving
quit→relaunch→quit; three negative controls (`"zz"` → `en`, a pre-v2.9.0 session with no
attribute → `en` without crashing, then `fr` again to prove the first two were not the default
sticking); and the JS half driven separately with the backend holding `fr` before page load.

**O-Marimba**, both halves: a compiled JUCE probe over the three added code paths (confirming
the attribute comes back a **STRING** `var`, per the known ValueTree XML round-trip type loss),
`auval` VERIFYING CLASS INFO, and a four-state JS drive through the real gear popover — fresh
install → picked Français → session reopened → English reopened, with the native-fn call log
at each step.

**A trap worth carrying:** `build-and-install.sh` builds VST3+AU only. O-IntonationPad's first
round-trip read `uiLanguage` as ABSENT because it was testing a **stale July Standalone**. It
checked the binary with `strings` rather than trusting the run, then built
`O-IntonationPad_Standalone` explicitly.

## The seventeenth gate-shape finding — deliberately NOT fixed

`check-i18n` assertion 12 reads the **whole RHS** of a `.textContent =` and flags every
two-letter literal. Right for `on ? "On" : "Off"`; wrong for a literal in **call-argument
position** — `formatParam('trim', norm)` renders `"+0.0 dB"` and `'trim'` is a parameter ID
that never reaches the screen.

Not fixed, by choice. The shape occurs at exactly two sites repo-wide and both were these.
Narrowing a **coverage** assertion on n=2 buys nothing, and the error is asymmetric: an
over-report costs one glance, an under-report ships English. The two call sites now pass the
branch's own `paramId` variable instead of re-typing its value, and the gate's shape is named
in `O-Gain/js/app.js:591-608`.

The O-Marimba run made the same call in the mirror image: broadening `scanJsSource()` to match
its `markupKeyRefs()` fix would have reported five violations on shipped **O-Lyrica v2.4.0**,
which keys those nodes by id AFTER injection via `window.__setLabel(document.getElementById(…))`
— legal canon. Tried, reverted, named in the code.

## Pre-existing ENGLISH defects the French sweep exposed

- **O-Gain**: the whole `[data-tooltip]::after` layer overflowed the window at rest (above).
  Separately, `.learn-section` was an intrinsic-width button + `flex: 1` group, so the Target
  readout slid up to 40px sideways whenever Learn ran, in English.
- **O-Marimba**: the tuning-mode row overhung its 200px panel by 10.83px and the Scala row by
  12.78px — both now fit.
- **O-SpectralShaper**: the header read `v1.6.1` on a v1.6.2 build, hard-coded and never bumped.
- **O-Lyrica**: `.truekeys-view` was `display: flex` unconditionally, painting "Hold 2+ notes
  to see intervals" beside the pitch circle in EVERY visualization mode; and `technique`
  option 4 lacked the accent `createParameterLayout` has always carried.
- **O-IntonationPad**: `TOOLTIPS.voicingMode` was applied by `makeKnob()` to a voicingMode knob
  that has never existed.

## Carried into Stage K

1. **`git commit -- <paths>` TAKES ONLY TRACKED FILES.** O-Marimba's first commit silently
   omitted three new untracked files and left HEAD with a gutted `index.html` and no
   controller. It was caught only by reading `git show --stat` and amending. **`git add` the
   exact new paths, commit, then `git show --stat` and confirm.** Every plugin in Stage K adds
   new files too.
2. **Do not trust the plan's structural claims about a plugin** — bridges, tip counts, text
   counts and positioner shape were each wrong at least once in this stage. Verify against the
   code before planning work around a number.
3. **The title of a split tooltip is the control's own English caption.** Reuse, never author.
4. **`flex: 1 1 0` WITH `min-width: 0`.** A width pin on one flex button just redistributes to
   the same row total — flex items are floored by min-content and carry `min-width: auto`.
   Verify any timing-sensitive measurement at two settle times (180ms and 1.7s) to rule out a
   transition mid-flight.
5. **Try `width: 100%` on a wrapper first, and probe it moves zero children.** It freed the
   natural French on O-SpectralShaper — where "Sensibilité" is NARROWER than "Sensitivity" —
   but does nothing for a shrink-wrapping box (O-IntonationPad). Pin **per-element**, never
   uniformly, and pin to the **English** box rounded up so English barely moves.
6. **Strings owned by shared registry modules under `${CMAKE_SOURCE_DIR}/modules/` are exempt** —
   localizing them is cross-plugin and a local edit is reverted by `/module-upgrade`. O-Marimba's
   Effects tab therefore stays English for a French user (`ANALOG`, `Thresh`, `Attack`, `Release`).
7. **A `screencapture` is never the verification.** One run grabbed the user's entire desktop,
   including an unrelated private document, while trying to see a reopened plugin. Deleted
   immediately; verified by the orchestrator as not retained. Use the headless harness and DOM
   reads.
8. **`serve-ui.js` picks port 0 deliberately** — a server on a taken port silently serves
   another session's files.

## NEEDS A HUMAN DECISION — none blocking Stage K

1. **`PLUGIN_VERSION` is not a JUCE keyword.** JUCE reads `VERSION`; the unrecognised keyword
   is silently ignored, so the plugin ships reporting **1.0.0** to the host whatever the line
   says. Affects **seven** plugins: O-Contrabass, O-Marimba, O-Octagon, O-Reed, O-ReverseDelay,
   O-MicrotonalSampler, O-Tapestop. Three are already-shipped canon-v2 plugins. Correcting it
   is a host-visible behavioural change across all seven at once — decide before the next
   `/publish`.
2. **The vertical clamp now ships in all 21 renderer copies** though it was independently
   reproducible on only two. Uniformity was chosen over minimal change; worth confirming.
3. **`O-IntonationPad/js/tuning-panel.js` is a DIVERGED local copy** — header says module
   v2.0.0, `modules/tuning/scala-tuning-engine` is at v3.0.1, and `dependencies.json` lists
   `ouariconModules: []`. v3.0.1 is the release that fixes the scrambled tuning tab.
4. **O-Gain and O-Lyrica still ship the watermarked "Adobe Stock" `paper1.jpg`**, and both
   shipped a new version in this stage.
5. **O-IntonationPad's tuning panel already overflows its 800px frame in ENGLISH** — the 220px
   controls column sits partly outside. The French was sized to the English budget rather than
   re-laying out the panel; the layout itself is a design decision.
6. **O-Gain's seven-button utility row is 344.8px of min-content in a 334px row in ENGLISH** —
   PH L, PH R and M/S OFF each render on two lines at v1.2.1 and still do. Every French caption
   was chosen so its widest word matches its English counterpart's, so the row keeps two lines
   and its 32px height.

## Not verified

- **Checkpoint 5 is outstanding on all 21 plugins.** No human has seen any French UI.
- **Checkpoint 4 is measured on two of 21** (O-IntonationPad, O-Marimba). The other nineteen
  are reasoned from the guard, not executed — and the DAW reopen, as opposed to the Standalone
  one, is unexecuted everywhere.
- **All French strings repo-wide are machine drafts**, every one `reviewed: false`. No native
  speaker has read them.
- **Windows/WebView2 font metrics** remain the named hardware-blocked deferral. The tightest
  French margins measured in this stage: O-Marimba **1.69px** (CUSTOM in a 48px box),
  O-Polystutter 4.2px (DÉCLIN), O-FreqPulse 5.2px (MÉDIUM).


---
---

# Stage K (batch K1) — T15: the seven cheap plugins — BATCH K1 COMPLETE

**Fourteen commits: seven plugins, SIX repo-level gate fixes, and four docs.**
Batches K2, K3 and K4 remain: **14 of 21 plugins still to run.**

| # | Plugin | Version | Commit | Root | Frame | LABEL |
|---|---|---|---|---|---|---|
| 1 | O-AnalogSaturation | 1.2.0 | `7e8cd024` | `Source/ui/public` | 600x450 | 14 |
| 2 | O-Texture | 0.2.0 | `3b307140` | `Source/ui/public` | 800x600 | 21 |
| 3 | O-Freeze | 2.1.0 | `e235e33c` | `Source/ui/public` | 550x530 | 18 |
| 4 | O-Detune | 1.6.0 | `6437d0de` | `Source/ui/public` | 600x480 | 30 |
| 5 | O-Bassoon | 1.1.0 | `b8c1d9a1` | **`Resources/ui`** | 900x600 | 29 |
| 6 | O-Emulator | 1.1.0 | `fab53677` | `Source/ui/public` | 620x430 | 19 |
| 7 | O-TextureForge | 1.1.0 | `6c595b70` | `Source/ui/public` | 900x600 | 21 |

`check-i18n --strict-v2`: **28 canon v2, 0 canon v1.** `PLUGINS.md` rows landed
in ONE commit (`54c1a818`), not seven — see the parallel protocol.

## THE HEADLINE: five of the seven ran CONCURRENTLY, and the protocol is the deliverable

K1 plugins 3-7 ran as five simultaneous executors in one checkout. It worked —
no lost commit, no build collision, nothing foreign in any commit — but only
because four shared resources were taken away from the executors first
(`260826-ieq-STAGE-K-BRIEF.md`, PARALLEL DISPATCH PROTOCOL section):

1. **`build/`** — ONE shared ninja dir for all 43 plugins (`build-and-install.sh:308,443`),
   and ninja does not lock. Plus the AU cache wipe is global, so a second
   install pulls the cache out from under the first plugin's `auval`. Both go
   inside a `mkdir` mutex with a 25-minute stale-lock break.
2. **`PLUGINS.md`** — one row per plugin, no merge help in a single working
   tree; seven concurrent read-modify-writes lose rows silently.
3. **`scripts/`** — gate defects run at 4-6 per stage here, so two executors
   editing `check-i18n.js` at once is live, not theoretical. An executor that
   finds one now STOPS and reports; the orchestrator lands it alone.
4. **THE SCRATCHPAD** — found the hard way, mid-batch. See below.

## THE SCRATCHPAD IS SHARED, AND IT SILENTLY SWAPS PLUGINS

The scratchpad path carries ONE session id for every executor. O-Freeze's
`measure.js` was overwritten mid-run by O-Bassoon's script and handed O-Freeze
a page of **O-BASSOON's knob widths in reply to its own command**; its baseline
JSON was clobbered the same way. O-Detune's `base-en.json` came back holding
**O-Emulator's** rects. O-Emulator's first-round numbers came from bare-named
root files too.

**The danger is not a crash. It is a geometry diff between a clobbered baseline
and an intact after-pass — a comparison between two different plugins that
reads exactly like a real result.** All three detected it independently (from
the JSON shape, or from data that made no sense for their page), re-took every
number in a per-plugin subdir against a `git show HEAD:` / `git archive`
baseline, and reported only the re-taken figures. O-Detune added an
`assert plugin === 'O-Detune'` to its differ.

Rule now in the protocol: **write everything to `scratchpad/<yourplugin>/`**,
and re-run rather than reason about whether a touched file mattered.

## SIX gate fixes — the eighteenth through twenty-first wrong-shaped assumptions

Every one was found by a control that FAILED TO FIRE, or by holding a variable
constant that should have produced no result. None was found by reading code.

| Commit | Defect |
|---|---|
| `e80288eb` | **Assertions 1 and 2 hard-failed on a plugin with NO hover-help** — the shape every Stage K plugin has. `I18N has entries` is false on a page whose copy is entirely LABELS; `TIP_BINDINGS has entries` treated 0 as a defect when it is the correct state. |
| `fbdb6930` | **The label gate measured only WIDTH.** A French caption that WRAPS out of a fixed-height control spilled with every assertion green — 44px of text across four lines in a 28px button. |
| `e6c159e0` | **Assertion 6 could not find a controller loaded by `src`.** It reported "a controller module exists" FALSE on a correct plugin, and since 6 gates the rest, 10-13 and 15 never ran — the plugin came out "canon none" with the whole v2 half silently skipped. |
| `64b3d53d` | **A minified bundle is unparseable, and the gate scanned it anyway.** `stripJsComments` blanked 17.3% of a 220KB bundle across 711 runs, so all EIGHT live keys read as DEAD while assertion 12 went vacuous — planted English prose PASSED. |
| `3be873eb` | **The geometry diff keyed on a path truncated at 6 segments**, collapsing distinct elements onto one key. Reported 12 elements moved WITH THE LANGUAGE HELD CONSTANT. |
| `a32039c7` | **`I18N_EXEMPT` matches by TEXT, so it silences every node with that string** — a missed label hides as a deliberate one. |

### Two of those fixes had a FIRST DRAFT THAT WAS WRONG, and the sweep caught it

Both times the repo-wide sweep before committing is what caught it, not review.

- **`fbdb6930`**: comparing text height to content height put **O-simpleBeatmaker
  red IN ENGLISH**, on four-letter words like "Kick", by exactly 1.0px — a
  line-height tighter than the font's natural line box. Wrapping IS more than
  one line box, so the condition became `textLines > 1 && textHeight >
  contentHeight`, and the single-line overhang prints as a NOTE.
- **`64b3d53d`**: borrowing `i18n-extract`'s whole `JS_SKIP` — on the strength
  of check-i18n's own comment claiming the two scanners see the same set —
  reported **37 live O-IntonationPad keys as DEAD**, because its diverged local
  `js/tuning-panel.js` is where they are referenced. `JS_SKIP` answers "whose
  code may this tool propose edits to" (a WORKLIST concern) and correctly drops
  vendored modules; assertion 15 asks "is every key live". Only unparseability
  is borrowed now, and the wrong comment is corrected in place.

### And one PROPOSED fix was a NO-OP

Two executors independently proposed fixing the `I18N_EXEMPT` hole by testing
`keyedAncestor` before `exemptSet` in assertion 10. **Both branches are
`continue`** — the node is skipped either way and O-Detune's control still
passes. Checked in the code before writing anything. A real fix scopes each
exemption to the element that earned it, changing the `I18N_EXEMPT` contract
across all 28 localized plugins: **a decision, not a repair, and NOT taken.**
`a32039c7` makes the hazard visible instead, at zero pass/fail change.

## FOUR controller shapes in seven plugins — the plan describes none of them

The plan silently assumes the O-Tapestop `js/app.js` shape. Actual:

| Shape | Plugins |
|---|---|
| Controller AND stylesheet **inline in `index.html`** | O-AnalogSaturation, O-Freeze, O-Detune, O-Bassoon, O-Emulator |
| `js/main.js`, loaded by `src` | O-Texture |
| **webpack bundle** — `Source/ui/src/app.js` → `public/js/app.bundle.js`, a CLASSIC script | O-TextureForge |

The inline shape re-roots the canon import to `'./js/i18n.js'` and makes a TDZ
throw fatal to the WHOLE UI, not one panel. The bundle shape cannot host the
canon at all — webpack would inline the table at build time, leaving the
embedded, served `js/i18n.js` read by nobody — so O-TextureForge got a separate
served ES module, `js/i18n_init.js` (underscore: `i18n-init.js` would embed as
`i18ninit_js`).

## Pre-existing ENGLISH defects the French sweep exposed

- **O-Texture: a self-feeding layout runaway, in English, at rest.** `#xy-pad`
  is a canvas whose intrinsic size is written from its own client box, and
  `.main-area` carried `min-height: auto`, so each `ResizeObserver` delivery
  raised the floor, which raised the canvas, which fed the next delivery:
  800x644 at 100ms, 800x1176 at 1.2s, **800x5696 at 6s and still climbing**.
  The XY pad and the whole bottom strip left the 600px frame inside the first
  second, `html{overflow:hidden}` making them unreachable. Introduced by
  v0.1.2's IN-10 change. **Fixed** — `min-height: 0` — because assertion 7
  compares an EN sweep to an FR sweep ~180ms later and at 0.86px/ms every
  element differs: the gate can say nothing about French on a page that will
  not hold still.
- **O-Emulator: `.hdr` is 570px of content holding 732.28px of max-content** —
  162px over-full in English at v1.0.1. `.wordmark` renders on two lines and
  `.plate` on three, both overflowing the 48px header upward. **Reported, not
  fixed** — a layout decision, not a localization one.
- **O-Texture: six `title="Coming soon"`** deleted; text moved into the
  accessible NAME as `Metal — coming soon`, assembled only from strings already
  on the page. A shared `aria.comingSoon` key would have made `aria-label`
  REPLACE "Metal" as the accessible name, breaking label-in-name.

## Geometry — every pin negative-controlled, and one removed as DECORATION

Zero non-label elements moved between languages on all seven, after pins.
**French shrinks at least as often as it grows**: O-Texture 6 of 7 strings
shrank, O-Bassoon 6 of 12, O-TextureForge 4 of 12, O-AnalogSaturation 2 of 5.
A clip-only check would have certified every one of those pages.

- **O-Texture tried `.xy-pad-container { min-height: 0 }`, its negative control
  PASSED, and it was REMOVED.** No decorative pin ships in this batch.
- O-Detune folded `.preset-action-btn` side padding 8→6px — border-box with a
  pinned width, so no outer rectangle moves — taking its tightest French margin
  from **2.91px to 6.91px**.
- O-TextureForge rejected "Dispersion X" on measurement: 65.14px WRAPPED to two
  lines in a 72px box because the space before the axis letter is a break
  opportunity. `Disp. X` is 38.78.
- O-Emulator caught a wrong number of its own: an armed face measured 17.69 was
  really **26.23** — 17.69 was the widest LINE of a wrapped string in the
  unpinned box.
- O-Bassoon's first French About blurb was **too SHORT**: two line boxes against
  English's three shrank the card 19.4px and pulled the byline up. Re-authored
  to the English line count.

## Carried into K2, K3 and K4

1. **`git commit -- <paths>` takes only TRACKED files.** Still true, still the
   trap. `git add` new paths, commit, `git show --stat`, confirm the count.
2. **Do not trust the plan's structural claims.** Frames and served roots were
   right on all 21; text counts are wrong on 20 of 21 and JS prose is
   undercounted almost everywhere — see `260826-ieq-STAGE-K-MEASURED-INVENTORY.md`.
3. **Check whether the page HOLDS STILL before trusting any geometry number.**
   Measure at 180ms and 1.7s. O-Texture's runaway made assertion 7 meaningless
   until fixed, and it was invisible to every other assertion.
4. **`scratchpad/<yourplugin>/`, always.**
5. **An executor's own probe can carry the gate's bug.** O-Emulator's differ had
   the same 6-segment `pathOf` truncation, copied from the gate, and had to be
   corrected and both diffs re-taken.
6. **The "28 of 26 elements visible" coverage line is NOT the path cap** — it is
   the class list inside the path changing when `active` toggles on a tab
   button. A report line, not an assertion, and it errs safe: it can invent a
   surplus but cannot conceal a hole. **It will recur on every tabbed plugin in
   K3/K4.** Not fixed.

## NEEDS A HUMAN DECISION — none blocking K2

1. ~~**The `I18N_EXEMPT` contract.**~~ **RESOLVED — see the section below.**
2. **O-Bassoon's registry row now reads `🚧 Stage 0 | 1.1.0`**, which is
   internally odd. Built, installed, `auval`-clean — but its executor declined
   to assert the `🚧 → 📦` flip as beyond a localization dispatch, and that
   restraint was kept. The flip is the developer's.
3. **Assertion 6's scroll-extent check is blind to a page that has left its
   frame.** It reads `documentElement.scrollWidth/Height`, which
   `html{overflow:hidden}` clamps — it reported a comfortable 800x600 while
   O-Texture's `body.scrollHeight` was 1704. Such a page is visible only
   INDIRECTLY, via assertion 7 mistaking growth for French. Promoting the
   existing NOTE to an assertion is repo-wide and could turn shipped plugins red
   mid-rollout.
4. **`O-Freeze` ships `assets/paper1.jpg`** — same filename as the watermarked
   "Adobe Stock" texture already flagged on O-Gain and O-Lyrica. Bytes not
   compared. **Three plugins to check before the next `/publish`.**
5. **O-TextureForge's `package-lock.json` is UNTRACKED**, so its webpack bundle
   reproduced byte-identically on this machine — which is what made the diff
   attributable — but a fresh clone resolves its own tree with no guarantee.
6. **O-Emulator's `GB` caption is not byte-identical to its `Game Boy` option**
   — a pre-existing page/automation divergence. Exempted, because keying one
   segment of five leaves it switching while four arm-1 siblings stay pinned.
7. `PLUGIN_VERSION`, the silently-ignored non-keyword, still affects seven
   plugins. **O-Reed and O-MicrotonalSampler are both still ahead in K4.**

## Not verified

- **Checkpoint 5 outstanding on all 28.** No human has seen any French UI.
- **Checkpoint 4 measured on two of 28** (O-IntonationPad, O-Marimba, both in
  Stage J). All seven K1 plugins are reasoned from the `isVoid()` guard and the
  build, not run through a host session reopen.
- **All French repo-wide is machine drafts**, every entry `reviewed: false`.
- **Windows/WebView2 font metrics** — the standing hardware-blocked deferral.
  Tightest K1 margins: O-Emulator **0.83px** (`Broyage`), O-Bassoon 1.8px
  (rejected `Car. attaque`, shipped `Caractère` at 5.2px), O-Detune 6.91px.
- **The Standalone `.app` is stale on every K1 plugin** — `build-and-install.sh`
  builds VST3+AU only.


---

# Stage K — the `I18N_EXEMPT` contract, RESOLVED

Decided and landed between batches K1 and K2, before the remaining 14 plugins
could add more entries under the old rule.

## The contract

An entry is **`[text, reason]`** or **`[text, reason, scope]`**. A scope is a
comma-separated list of **`tag`**, **`.class`** or **`#id`**, matched against
the text node's own parent and its ancestors.

**Unscoped entries stay legal, deliberately.** Most exemptions are not
ambiguous — a product name or `Hz` says the same thing everywhere it appears —
and demanding a scope there is noise that teaches people to write one without
thinking.

**A scope is REQUIRED in exactly one situation: a string that is exempt AND
keyed on the same page.** That is the one state in which the gate cannot tell a
deliberate skip from a label somebody forgot, because both look identical to a
text match. `check-i18n` assertion 14 enforces that and nothing more.

## Why three forms and no more

The scope language was chosen **after** looking at the data, not before. The
five ambiguous strings in the repo needed exactly three shapes between them:
`option`, `.knob-value`, `.title-accent`. `scanHtml` already gives every node
its parent chain, `id`, `tag` and `classes`, so those three cost nothing. A
full CSS engine would be a second parser to keep in step with the first.

## What was scoped, so the repo never went red

| Plugin | String | Scope |
|---|---|---|
| O-Detune | `Random` | `option` |
| O-Lyrica | `Off` | `option` |
| O-MultiBandCompressor | `Off` | `.knob-value` |
| O-ReverseDelay | `Delay` | `.title-accent` |
| O-simpleGrain | `Grain` | `.title-accent` |

All five landed in the SAME commit as the assertion, so contract §8 — a gate is
never red for a whole rollout — holds. `--strict-v2`: 28 plugins, ALL CHECKS
PASS. `check-ui-labels` exits 0 on all five.

**Two of those five reasons already carried a hand-written WARNING** saying the
entry also silences some other element. Those warnings are now ENFORCED rather
than advisory. The brief tells the remaining dispatches: **if you catch
yourself writing that warning, write a scope instead.**

## No version bumps, and why

`I18N_EXEMPT` has **no runtime consumer**. The canon imports `LANGUAGES`,
`I18N`, `LABELS`, `TIP_BINDINGS` and `tr`; the three controllers that mention
`I18N_EXEMPT` mention it only in comments. The rendered UI is unchanged, so
bumping five shipped plugins would be a user-visible version change for no
user-visible reason. The embedded copies differ from source by an unused array
field until each plugin's next build.

## The controls that make this real

1. O-Detune with `label.random`'s key stripped and bare English left on the
   page — **the exact case that PASSED before** — now FAILS
   `[10] 1 uncovered: Random @span`.
2. A scope pointed at a class that exists nowhere stops exempting: the two
   legitimate `<option>` nodes go uncovered. **The scope is really evaluated,
   not decoration.**
3. Removing the scope from an ambiguous entry FAILS assertion 14 by name.
4. The same missed-label case on a different scope shape — O-ReverseDelay,
   `.title-accent` — FAILS `[10] Delay @div`.

All restores checksum-verified.

## The one thing this does NOT close

**A scope cannot be evaluated for a JS-written string**, because a row from
`extractJsRows` has no element to match against. Assertion 12 therefore still
matches exemptions by text alone. No plugin needs it today; it is named here so
it is not rediscovered as a surprise.


---

# Stage K (batch K2) — T15: the five TIGHT FRAMES — BATCH K2 COMPLETE

**Ran SERIAL by plan instruction**, because the stop condition depends on seeing
each geometry diff before the next plugin starts. Seven commits: five plugins and
**two repo-level gate fixes**, each landed alone ahead of the plugin that found it.

| # | Plugin | Version | Commit | Frame | LABEL | Layout change? |
|---|---|---|---|---|---|---|
| 1 | O-Chorus | 1.3.0 | `ff9c8616` | **700x125** | 16 | no |
| 2 | O-DigiDelay | 1.3.0 | `934f3eeb` | **700x196** | 21 | yes, one — NOT French-caused |
| 3 | O-AnalogEQ | 1.2.0 | `fcfcc5d9` | **920x220** | 20 | no |
| 4 | O-Bass | 1.4.0 | `f3631041` | 420x320 | 16 | no |
| 5 | O-SimpleReverb | 1.6.0 | `0f8abb9d` | 500x350 | 26 | no |

## THE HEADLINE: the tight frames were not the risk the plan expected

The plan isolated these five because "a label that gains a line has nowhere to
go", and set a stop condition: **if two of the five need layout changes, stop —
that is a pattern, not an incident.** It was not met, and not narrowly: **five of five tight frames absorbed French
with ZERO French-caused layout changes.**

The one layout change in the batch was **O-DigiDelay's, and French did not cause
it**: `.led-meter-label` was `width: 18px` while the ENGLISH word `OUT` renders
20.91px. Keying the node exposed it; reverting the change alone fails
`[4][en] label.out "OUT" 20.9>18.0` **before any French exists**, and no French
string could have fitted either (SORTIE 35.77, SORT. 28.05, even SOR 19.92).

**What the tight frames actually cost was ABBREVIATION, not geometry.** Every
plugin paid in rejected fuller forms, each recorded with its measured width:
RÉINJECTION 66.30 and MODULATION 68.00 past O-DigiDelay's cliff; PROFONDEUR 68.02
and SATURATION 63.52 past O-Chorus's wrap cliff; ANALOGIQUE 66.70 in a 57.00px
box; CHARGER + SAUVER = +37.71px on a zero-slack row.

## Keying a node EXPOSES a pre-existing ENGLISH defect — 2 of 5

Both were invisible until the node was keyed, and neither is a French problem:

- **O-DigiDelay** `.led-meter-label` — assertion 4 HARD-FAILS in English. Fixed
  in the same commit, negative-controlled before any French existed.
- **O-Chorus** `label.lfo` overhangs its offsetParent by 2.0px in English —
  assertion 5 reports it as an authored layout, so it was REPORTED, not fixed.

**That split is the rule for K3/K4**: a hard fail is yours to fix and prove; an
authored-layout NOTE is a design decision and is not.

## THE CLIFFS ARE PER-PAGE, AND THEY COME IN PAIRS OR TRIPLES

No two plugins in this batch had the same cliff structure, and each mechanism is
invisible to the assertion that catches the other:

| Plugin | Cliffs | What separates them |
|---|---|---|
| O-Chorus | **50.00** / **62.00** | 50 widens the label rect (gate-visible, invisible to a user); 62 WRAPS and pushes the readout down 10px |
| O-DigiDelay | **60.00** / **60.00** | same number, different mechanism: a single word SPILLS (`[4][fr]` sees it, `[7]` is blind); two words WRAP AND PUSH (`[7]` sees it, `[4]` is blind) |
| O-AnalogEQ | 57.00 / — / **67.00** | the third is invisible to BOTH — see the gate fix below |
| O-Bass | 65.00 / zero-slack / no threshold | **no fixed-width text box anywhere**: every caption's width fed straight into where its neighbours painted |
| O-SimpleReverb | 52.00 / zero-slack / **97.50** | the third is a GRID-TRACK spill — a `1fr` track is `minmax(auto, 1fr)`, so a caption whose longest word passes the track width raises min-content and the column steals from its three siblings |

**One plant certifies one mechanism.** The batch's practice, now standing: plant
a harness-blindness probe for EACH cliff, both arms where a pin is involved
(pin present → `[4]` fails and nothing moves; pin removed → `[4]` green and
`[7]` fails).

## `dx` ALONE MISLABELS A PIN AS DECORATION — O-Bass

A pin expected to be decoration moved its neighbours only **0.33px**, under the
0.5px tolerance — and **failed its negative control anyway**. Assertion 7
tolerances `dw` as well as `dx`, and the **0.68px WIDTH** change on the row the
caption owned was itself past 0.5.

**A caption can be load-bearing through the box it OWNS while moving nothing far
enough to notice.** Judge every pin on the full delta, never on `dx`.

**O-SimpleReverb confirmed it independently, on a different page and a sharper
number.** Its CHARACTER pin moves the knob by `dx=0.50` — *exactly* the
tolerance, not past it — and all four of its children by 0.50 too, so a
`dx`-only differ reports **nothing at all**. It fails on `dw=-1.00`. Two
independent confirmations in one batch makes this a rule, not an anecdote.

## TWO GATE FIXES — the twenty-second and twenty-third wrong-shaped assumptions

Both were found by a gate MISBEHAVING on a real plugin, not by reading code.

### `67bdf3a4` — the gate could not tell an ANIMATION from a French push

Assertion 7's premise is that the page holds still. The probe already froze
DECLARATIVE animation — SMIL via `pauseAnimations()`, WAAPI via
`getAnimations()`, both added in Stage H for O-simplePhysicalModelSynth — but
**neither API can reach a `requestAnimationFrame` loop, because that is the
page's own code and not an `Animation` object.**

O-Chorus's `#lfo-dot` free-runs off the wall clock: `dy=-24.0` on one run,
`dy=+0.4` on the next, every French string identical. Held at ENGLISH on both
sides it reported the same element moving `dx=4.9 dy=19.6` — the `3be873eb`
signature. **A failure that reproduces with the language held constant is never
a French failure.**

The animated set is now MEASURED, not declared: English is probed twice more and
anything unstable across the three samples is excluded and printed as a NOTE
with its EN→EN spread. Two extra samples, because a periodic animation can land
on the same phase twice. The hole it leaves is named: an element that BOTH
animates and is pushed by French is excluded, so a second NOTE prints its EN→FR
delta beside the spread — a real push shows a delta well OUTSIDE it.

The repo-wide sweep also found **O-simpleSubtractive's `#tooltip`** unstable
(`dy=11.5 dh=16.2`) in one committed state — a **CSS transition mid-flight**, a
second mechanism with the same symptom, which was passing on tolerance luck.

### `4fa586e8` — THE THIRD CLIFF: a caption can grow ON TOP of its neighbour

Assertions 4/5 catch a caption that SPILLS its box; assertion 7 catches one that
PUSHES a sibling. A caption in a box that is `position: absolute`, width-pinned
and height-free does **neither**: it wraps, grows downward inside its own box,
exceeds no width, exceeds no content height (the box grew with it), and pushes
nothing because absolute positioning takes it out of flow. It lands ON TOP of
what is beneath it.

O-AnalogEQ's `.band-label` is exactly that shape. Planting `PLATEAU BF` with the
caption's `nowrap` removed grows it `dh=+13.00px`, reaching y=86 into a knob ring
that begins at y=75 — **and the whole gate printed ALL CHECKS PASSED.** Assertion
8 was blind too: a knob ring is not a label, and 8 compares labels to labels.

New assertion **8b**: a label that GREW must not intersect a non-label element it
cleared in English.

**THREE SHIPPED PLUGINS WENT RED ON THE NAIVE FORM, and every red was a rule
rather than a bug** — which is the reusable part:

1. **O-Contrabass** exposed a defect in the EXISTING assertion 8. `overlayOf`
   tested only `backgroundColor`, so a panel painted with a `linear-gradient`
   reads `rgba(0,0,0,0)` and **was never recognised as a paint layer at all** —
   its own popover captions were being compared against the page beneath the
   panel. Fixing it strengthens assertion 8 as well as 8b.
2. **O-Bassoon** hit `.botanical-overlay`: a full-bleed `<img>` at `opacity: 0.18`
   with `pointer-events: none`. Decoration a caption cannot collide with. Skipped
   — and every skip is PRINTED.
3. **O-IntonationPad** is the subtle one. `Tuning` becomes `Gamme` — **French
   SHORTER by a character** — so the tab row re-centres, the button MOVES, and it
   grazes two panel edges it had cleared. Nothing grew. **The rule is GROWTH, not
   intersection**; a label that merely moved is a different phenomenon, and
   movement of non-labels is already assertion 7's job.

Controls: the plant fails; the decoration skip did NOT make it vacuous (it still
fires by naming the knob WRAPPER divs, which geometrically contain the skipped
SVG art, so the skip costs no coverage); the growth gate did not make it vacuous
either; and **all 31 localized plugins were swept under HEAD's gate and the new
one with 8b clean on every one.** Contract §8 holds.

The residual hole, named: a wrapper that is itself `pointer-events: none` would
be skipped entirely.

## O-SimpleReverb: the density was cheap, and the reason generalises

26 labels in a 500x350 frame was the highest text-per-pixel density of the
batch, and it needed **three pins for twenty-six labels** where O-Bass needed
four for sixteen. Two reasons, both reusable:

- **Twelve of the 26 never needed a key.** Six `<option>` texts are
  byte-identical to `AudioParameterChoice { "Booth", "Room", "Hall", "Spring",
  "Plate", "Ambient" }` — D-01 arm 1, the purest case in the stage, because a
  `WebComboBoxRelay` drives the combo by INDEX and the visible strings are the
  only place the page and the automation lane can diverge.
- **The other fourteen sit in CSS grid tracks**, and `repeat(4, 1fr)` /
  `repeat(3, 1fr)` hand out **seven language-invariant rectangles** for free.
  O-Bass had none of those and paid for it in pins.

**A grid is a localization asset.** Where K3/K4 meet one, expect the pin count
to fall; where they meet centred shrink-to-fit rows, expect it to rise.

## Carried into K3 and K4

1. **Measure your page's cliffs; expect two or three, each invisible to the
   assertion that catches the others. Plant for EACH.**
2. **Judge a pin on its FULL delta, never on `dx`** — `dw` past tolerance on the
   box the caption owns is enough to make it load-bearing.
3. **Expect keying to expose a pre-existing ENGLISH overflow.** Hard fail in
   English → fix and negative-control it before any French exists. Authored-layout
   NOTE → report, do not fix.
4. **`[7]` animation NOTEs and `[8b]` DECORATION / PAINT LAYER NOTEs are
   machinery, not findings.** Read the second `[7]` NOTE: a real French push
   shows an EN→FR delta well outside the EN→EN spread.
5. **A CSS transition mid-flight looks exactly like an animation.** Confirm the
   page holds still at 180ms and 1.7s before trusting any number.
6. `git commit -- <paths>` still takes only TRACKED files. Still the trap.
7. **The plan's text counts remain wrong** and the measured inventory remains
   right — **it matched in every column on all five K2 plugins**.
8. **NEVER copy another plugin's measured width.** O-SimpleReverb re-measured
   `LANGUAGE -> LANGUE` in its own `.settings-label`: 63.55 -> 47.11, against
   O-Bass's 55.31 -> 38.87 for the same two words at the same declared font-size
   and letter-spacing. The delta is identical (-16.44) but **the absolutes differ
   by 8.24px**. Borrowing the number would have produced a wrong number that
   reads exactly like a right one.
9. **`grep` without `-a` on a built binary reports EVERY string missing**,
   including ones that are certainly there. O-SimpleReverb's first verification
   pass hit this and read as a total failure. `grep -a` finds them. It is the
   same trap `strings(1)` sets from the other direction by splitting a UTF-8
   multibyte — **verify the probe against a string you know is present before
   believing a miss.**
10. **The canon block does not always go at the bottom.** O-SimpleReverb's
    low-cut IIFE calls `trLabel()` during module evaluation, so the block had to
    go at the TOP or the read is a TDZ `ReferenceError` that takes the whole UI.
    `initI18n()` is still called last. Place it by where the page first READS it,
    not by convention.

## NEEDS A HUMAN DECISION

1. **`paper1.jpg` is byte-identical across all TWELVE copies in the repo**,
   sha256 `1619a20c6e7b4155b4a2feb533101f8c4fa6e0f70ccdc290e11c0a698280db49`.
   The watermarked "Adobe Stock" texture flagged on O-Gain and O-Lyrica is the
   same file served by **TEN** plugins — O-AnalogEQ, O-AnalogSaturation,
   O-Chorus, O-Detune, O-DigiDelay, O-Freeze, O-Gain, O-Lyrica, O-Marimba,
   O-MicrotonalSampler — plus two `.planning/mockups` copies. The standing note
   said "three plugins to check"; it is ten, measured. **A licensing question,
   before the next `/publish`.**
2. **O-AnalogEQ's two three-way Q toggles clip their own `TIGHT` option in
   ENGLISH.** `.three-way-option` is `flex: 1` without `min-width: 0`, so the
   three sit at min-content — 109.87px inside a 108px content box — and
   `overflow: hidden` clips **1.87px** off TIGHT. Pre-existing, invisible to both
   gates because the nodes are exempt and never keyed.
3. **O-DigiDelay ships a label-in-name defect**: `label.load` `CHARGER` against
   `aria.loadPreset` "Ouvrir un préréglage…" — WCAG 2.5.3 does not hold in French
   on that button, and its own `i18n.js` comment claims a string it does not
   ship. Found by O-AnalogEQ, reported not edited (another plugin's file).
4. **O-Bass ships `OUT` as `sameAsEn` while `OUTPUT` becomes `SORTIE`** — the
   same English word gets two answers on one page, decided by geometry (14px of
   slack vs none). Keyed rather than exempted so it stays on the reviewer's list.
5. **O-SimpleReverb's footer wordmark is a hard-coded `Ouaricon Audio v1.5.5`**,
   already two versions stale before this commit and now three. The right fix is
   the runtime-filled `id="versionLabel"` span O-DigiDelay and O-Tremolo already
   use, not another literal — a user-visible change unrelated to localization,
   so it was recorded in the `I18N_EXEMPT` reason rather than made.
6. Items 2-7 of K1's human-decision list still stand, including `PLUGIN_VERSION`
   on **O-Reed and O-MicrotonalSampler, both still ahead in K4**.

## Not verified

- **Checkpoint 5 outstanding on all 32.** No human has seen any French UI.
- **All French repo-wide is machine drafts**, every entry `reviewed: false`.
- **Checkpoint 4** is reasoned from the `isVoid()` guard and the build on every
  K2 plugin — no host session reopen was executed for any of the five.
- **Windows/WebView2 font metrics** — the standing hardware-blocked deferral, and
  K2 is where it bites hardest. Tightest margins shipped: O-AnalogEQ **1.20px**
  (`CHARGER`), O-SimpleReverb **1.28px** (`CARACTÈRE`), O-DigiDelay **1.39px**
  (`SYNCHRO`), O-Bass **1.66px** (`FRÉQUENCE`), O-Chorus **1.89px** (`LARGEUR`).
- **The Standalone `.app` is stale on every K2 plugin** — `build-and-install.sh`
  builds VST3+AU only.
- O-FreqPulse returned one `rc=1` during a repo-wide sweep with **zero** `[8b]`
  failures — a different assertion, and it did not reproduce in three consecutive
  re-runs. Recorded, not chased.

---

# STAGE K — BATCH K3 (the five medium plugins, CONCURRENT)

Five executors at once in one checkout. **Zero repo-level gate fixes were
needed** — the first batch in this task with none, after six in K1 and two in
K2. No executor was blocked, and none edited `scripts/`.

| Plugin | Version | Commit | Files | LABEL: plan / inventory / measured |
|---|---|---|---|---|
| O-Comp | 1.6.0 | `ec8a4c88` | 8 | 19 / 22 / **22** |
| O-Tremolo | 1.7.0 | `af3610dd` | 8 | 19 / 23 / **23** |
| O-GrainScatter | 2.5.0 | `1791397b` | 9 | 73 / 70 / **70** |
| O-Bowed | 1.5.0 | `9ff19bf4` | 8 | 50 / 48 / **48** |
| O-Reed | 1.2.0 | `819a6113` | 8 | 58 / 52 / **52** |

Rows in `8ba0b8b8`, duplicate check clean. `check-i18n --strict-v2`: **38 canon
v2, 0 canon v1**. `boot-all-uis`: **43/43 clean, 0 warn, 0 failed**.

**The measured inventory matched in every column on all five, and the plan was
wrong on all five.** That is 15 of 15 across K2 and K3. The inventory is the
authority; the plan's text counts are not.

## The finding that decides how K4 runs: canvas `ctx.fillText`

**Three of the five executors hit this independently, and the batch shipped TWO
DIFFERENT ANSWERS.**

- **O-Comp** localized its three strings (`Envelope`, `Gain Reduction`, the live
  `GR:` prefix), housing them in **`I18N` with empty bodies** rather than
  `LABELS`, and verified them with a `fillText`-recording probe, en→fr→en, plus
  its own negative control.
- **O-GrainScatter** exempted its one (`"<n> grains"`) with a reason, citing the
  suite's existing position — O-Orbit, O-MultiBandCompressor and O-simpleSampler
  already ship canvas strings in English — and carried it to Stage M.
- **O-Bowed** found **15** of them across three visualisation canvases, reported
  them, and ran the control that settles the mechanism.

**The mechanism, proven rather than reasoned.** Adding a key to `LABELS` and
calling `trLabel()` from inside a `fillText` **fails assertion 15 as a dead
key**: 15's `referenced` set collects markup attributes, literal `setLabel`
keys, literal `.dataset.i18n* =` writes and innerHTML-injected keys, and a
`trLabel()` call is in none of them. O-Bowed read that as needing an
orchestrator-owned change. It does not — **O-Comp's `I18N`-with-empty-bodies
shape already works inside the contract as written**, and the two findings
together prove it from both directions.

Neither gate can see any of this: assertion 10 walks TEXT NODES, assertion 12
scans `textContent`/`innerText` writes, and `fillText` is neither. **Leaving
every one of these strings in English passes green.**

**16 files repo-wide paint text to a canvas**, and
`plugins/O-Formant/Source/ui/public/js/main.js` is one of them — so **K4 meets
this whichever way it is decided.**

## A second indirection shape, equally invisible

O-Tremolo's dropdown section headers are written as
`header.textContent = headerText`, with the English literals `'Factory'` and
`'User'` sitting **one frame away at the `addSection()` call site**. The
extractor sees a variable; **assertion 12 is blind to the identical shape for
the identical reason.** Both would have shipped English inside a French UI with
every gate green. All three strings now route through `setLabel()`.

**Two distinct indirection blind spots in one batch.** The reusable check for
K4 is mechanical: grep for `textContent =` with a non-literal right-hand side,
and grep for canvas text — do not wait for a gate to raise either.

## A pre-existing shipped `SyntaxError`, in TWO plugins

O-Bowed and O-Reed each loaded `js/juce/index.js` as a classic `<script src>`
**and** as an ES module import, throwing `SyntaxError: Unexpected token
'export'` on **every page load, in the shipping plugin, at HEAD**. Found
independently by both executors, fixed by both, negative-controlled by O-Bowed
(restore the tag alone → the plugin goes red by name). Zero survivors repo-wide.

**An orchestrator lesson worth more than the bug.** The first executor to
finish reported `boot-all-uis` at 41/43 with these two failing, and the
orchestrator called it expected transient state from batch-mates mid-retrofit —
without checking. It was a real defect that had been shipping. **A red sweep
during a concurrent batch is not automatically batch-mate noise. Check the
pre-image before explaining it away.**

## `PLUGIN_VERSION` — the standing human-decision item, CORRECTED by measurement

Seven plugins mention `PLUGIN_VERSION`. Only **two** lack a real `VERSION`
keyword beside it and therefore actually ship `1.0.0` to the host, confirmed
with `PlistBuddy` against the installed bundles rather than reasoned:

| Plugin | Registry row | Bundle reports |
|---|---|---|
| **O-Reed** | 1.2.0 | **1.0.0** |
| **O-Marimba** | 1.13.0 | **1.0.0** (shipped in Stage J) |

**O-MicrotonalSampler was on the standing list and is NOT affected** — it
carries a real `VERSION` keyword and its bundle reports 1.23.9. The other four
(O-Contrabass, O-Octagon, O-ReverseDelay, O-Tapestop) carry a dead
`PLUGIN_VERSION` line beside a working one: harmless clutter, not a defect.

The item is therefore **two plugins, not seven**, and one of them is already
shipped.

## Geometry — the plan's model of where French pushes is wrong, in two reusable ways

**Every plugin: zero non-label elements moved EN→FR.** Every page held still
between 180ms and 1.7s, so no transition contaminated any number.

**French SHRANK on a large fraction everywhere** — O-Comp 3 of 7, O-Bowed 12 of
26, O-Reed 12 of 27, O-GrainScatter 13 of 32. **A clip-only check certifies all
four pages.**

Two mechanisms that make a caption harmless, both measured after a plant
disproved the executor's model:

1. **An explicit `width` caps a flex item's automatic minimum** (Flexbox §4.5).
   O-GrainScatter planned around a push cliff at its 62px `.knob-container` —
   made worse by a spatial row summing to *exactly* 846.00px in an 846.00px
   container — and the first plant disproved it: a 66.53px caption overflows
   symmetrically and pushes nothing. **The whole 900x800 page has only two
   shrink-wrapping push sites.**
2. **`repeat(4, 1fr)` is `minmax(auto, 1fr)`.** O-Bowed's tracks were
   content-sized 38/44/41/38 summing to 164 in a 162px box, and `width: 100%` on
   the column **alone changed nothing** — 100% of a content-sized track *is* the
   content size. The first draft did exactly that and the gate still reported 45
   moved. The fix is `minmax(0, 1fr)` on the grid.

**Executors' own predictions were wrong three times; the negative control caught
all three.** O-Tremolo twice — the knob it predicted would shift 2.03px does not
move at all (two centrings cancel; the pin is load-bearing on `dw`, not `dx`),
and a popover delta it predicted at 1.55px is really **16.44px**, because a
`width: 100%` select contributes nothing to a shrink-to-fit parent's intrinsic
size. O-GrainScatter once, above. **`dx` alone mislabels a pin as decoration —
now confirmed four times across two batches.**

Two pins were shipped honestly labelled as **design guards** rather than fixes,
because their negative control passes: O-Tremolo's and O-GrainScatter's
`.settings-label { white-space: nowrap }`.

## Three more pre-existing ENGLISH defects, exposed by keying and fixed

| Plugin | Defect | Since |
|---|---|---|
| O-Comp | `.preset-action-btn` carried the UA default `1px 6px`, leaving an **18px content box holding an 18.5px "Load"** — clearing assertion 4 by exactly the 0.5px tolerance | pre-v1.5.0 |
| O-Bowed | `Rev. Friction` **silently ellipsised** — 63.0px caption in a 62px cap | v1.4.1 |
| O-Reed | `Embouchure` (61.44) and `Double Reed` (60.58) **silently ellipsised** in a 60px `text-overflow: ellipsis` box | v1.0.0 |

All three are invisible to every layout check, because **a clip pushes
nothing**. Each was negative-controlled to re-fail in English before any French
existed. This is the fifth batch running in which keying exposed a pre-existing
English defect — treat it as expected, not as a surprise.

## Method traps found this batch

1. **`HEAD~1` is NOT your commit's parent in a shared checkout.** Another
   executor committed on top of O-Tremolo between its commit and its
   verification, so `git show HEAD~1:file` read **its own commit** as the
   pre-image and hid a whole-file CRLF→LF conversion. Use `<sha>^`, never
   `HEAD~1`, while other executors are committing.
2. **`git checkout -- <file>` to revert a plant wipes the UNCOMMITTED fix too.**
   O-GrainScatter lost its entire v2.5.0 edit to `PluginEditor.cpp` this way and
   caught it by re-grepping. **Restore plants from a namespaced scratchpad copy
   while the fix is uncommitted.**
3. **`nth-child` path keys turn a pure INSERTION into phantom MOVED rows.**
   O-Comp's first geometry diff reported 26 movers that read exactly like a
   regression; inserting `.settings-cluster` had renumbered every following
   sibling. Re-keyed on class-ordinal paths and re-taken. Same *silent wrong
   number* hazard as the shared scratchpad, from a different direction.
4. **A `grep -a` probe can miss on its own punctuation.** O-Tremolo's binary
   probe returned 0 for `Langue de l'interface` — a straight apostrophe against
   the source's U+2019. Caught only because a known-present control was checked
   first.

## Gate observations — no defect, two blindnesses worth recording

- **`boot-all-uis` is blind to the C++ half of assertion 8**, by design: delete
  the `getResource()` branch and it still reports clean, because it serves from
  the copied file tree rather than through `getResource()`. `check-i18n [8]`
  catches it statically, and both halves were controlled independently. Nothing
  to fix; stated so a later dispatch does not read `boot-all-uis` as covering it.
- **`[8b]` compares rectangles without accounting for an ancestor's
  `overflow: hidden` clip.** `.section-content { max-height: 0; overflow:
  hidden }` does not remove a collapsed section's children from layout — they
  keep full rectangles and are clipped only from *painting* — so `[8b]` reports
  collisions a user can never see. **Over-strict, not a hole**: it produces
  false failures, never false passes, so nothing is at risk. It did cost real
  translation quality on three O-Reed captions (`Visualisation de la perce` →
  `Coupe de la perce`, `Conception sonore` → `Design sonore`, `Ouverture` →
  `Ouvert.`), each verified to pass in the all-expanded state and fail only in
  the default one.

## Controller shape, again

**Four of the five are an inline `<script type="module">` in `index.html`**, not
the O-Tapestop `js/app.js` shape the plan assumes; only O-GrainScatter has an
`app.js`. The plan has now been wrong about controller shape in the large
majority of plugins in this stage. **Verify it; never assume it.**

## `I18N_EXEMPT` and the D-01 arms

- **O-Comp: D-01 arm 1 exempts NOTHING** — the plugin has no
  `AudioParameterChoice` at all.
- **O-Bowed: arm 1 exempts nothing either.** The dispatch predicted
  technique/articulation options; O-Bowed has exactly one Choice param
  (`tuningSystem`) and none of its options appears in the served markup.
- **O-Reed carries the one arm-1 collision the brief predicted**: `"Breath"` is
  a `vibratoSource` option **and** the `breathPressure` knob caption. Caption
  keyed → `Souffle`; option carried as a **SCOPED** exempt entry, because an
  unscoped one would silence the caption and leave the gate green over bare
  English. That is O-Detune's hole, avoided by the §7 third field.
- **O-GrainScatter has the other scoped entry**: `Trajectory` is a
  `spatial_mode` option and the trajectory dropdown's caption.
- **Arm 3 was never overruled anywhere in this batch.**

## NEEDS A HUMAN DECISION

1. **Canvas `ctx.fillText` policy** — the batch shipped two answers. Recommended:
   adopt O-Comp's `I18N`-with-empty-bodies shape as the standard, and treat the
   standing English canvas strings (O-Bowed's 15, O-GrainScatter's 1, plus
   O-Orbit, O-MultiBandCompressor, O-simpleSampler) as a named Stage-M backlog.
   **K4's O-Formant needs this decided.**
2. **O-Reed and O-Marimba ship `1.0.0` to the host.** Correcting
   `PLUGIN_VERSION` → `VERSION` is a host-visible change. Two plugins, not seven.
3. **O-Reed's fifteen XY-pad instrument markers stay English.** Three are
   byte-identical Choice options and the set is one Choice's abbreviations
   sitting 30px from a host-owned dropdown naming the same fifteen in English —
   localizing the other twelve would put two languages in one 15-item set. **A
   French user reads `Oboe` and `E.Hrn`.**
4. **`plugins/O-Bowed/tests/render-harness/CMakeLists.txt` hard-codes
   `JucePlugin_VersionString="1.3.0"`** — a second version copy, already drifted
   before this batch and now two behind. There is no variable to bump, so which
   copy is canonical is a human call.
5. **O-Reed's `.effects-placeholder` fix moves ENGLISH visibly** — "Coming Soon"
   goes from x=0 to the middle of the FX tab (h2 85.3 → 378.2). Correct, but a
   visible English change.
6. **O-Comp's `Relâche`** was chosen over the fuller `Relâchement`, which widens
   a shrink-wrapped knob column by 10.92px and slides the knob. If a native
   speaker rejects the abbreviation, the fix is the row's layout, not a longer
   string.
7. **O-Tremolo's `index.html` was the LAST CRLF `index.html` in the repo** and
   this commit converted it to LF (verified: all 43 are now LF). Review its real
   change with `git diff -w af3610dd^ af3610dd` — 986/19, not the 2251/1284 the
   stat shows.
8. Items from K1 and K2 still stand.

## Not verified

- **Checkpoint 5 outstanding on all 38.** No human has seen any French UI.
- **All French repo-wide is machine drafts**, every entry `reviewed: false`.
- **Checkpoint 4 is reasoned on all five** — from the `isVoid()` guard and the
  build. No host session was saved and reopened for any K3 plugin.
- **The Standalone `.app` is stale on every K3 plugin** —
  `build-and-install.sh` builds VST3+AU only.
- **Windows/WebView2 font metrics** — the standing hardware-blocked deferral.
  K3's margins are far roomier than K2's 1.2–1.9px band. Tightest shipped:
  O-Bowed **ACCORD 2.87px** and **ENR. 2.91px**; O-GrainScatter
  **PROBABILITÉ 2.98px**, where exceeding the box overflows harmlessly rather
  than clipping because the container cannot grow; O-Comp **Sauver 5.0px**;
  O-Reed **Ouvert. 5.30px**. O-Tremolo's tightest margins are **English**, not
  French — `LOAD` 0.22px and `SAVE` 0.17px inside their own pins.
- O-Reed's Tuning tab was not driven as a state, so `label.tuningLoadFailed` is
  referenced but never rendered by the gate.
- O-GrainScatter's `#pitch-hint` is measured only via a states-file `eval` that
  adds the `.visible` class its controller sets; the real reveal path was not
  driven.

# STAGE K — BATCH K4 (the volume) — STAGE K COMPLETE, 21 of 21

Three concurrent (O-Wind, O-Bells, O-Formant), then O-MicrotonalSampler alone.

| Plugin | Version | Commit | Files | LABEL: plan / inventory / measured |
|---|---|---|---|---|
| O-Wind | 1.17.0 | `a2e60212` | 8 | 65 / 61 / **61** |
| O-Formant | 1.26.0 | `314bd710` | 10 | 109 / 94 / **94** |
| O-Bells | 4.2.0 | `d9ba51ca` | 12 | 84 / 79 / **79** |
| O-MicrotonalSampler | 1.24.0 | `783ebb1d` | 12 | 126 / 146 / **146** |

Rows in `66f7a70d` and `046c568a`, duplicate check clean both times.
`check-i18n --strict-v2`: **42 localized plugins, ALL PASS**. `boot-all-uis`:
**43/43 clean, 0 warn, 0 failed.**

**The measured inventory matched on all four, and the plan was wrong on all
four. That is 19 of 19 across K2, K3 and K4.** The inventory is the authority;
the plan's text counts never were.

## THE HEADLINE: a gate can certify the absence of a thing it cannot see

O-Wind reported that `check-i18n` assertion 11 — "zero native `title=` remain" —
walked only `scanHtml()`'s parse of `index.html`. **`el.title = '...'` in a
controller was invisible to it.** O-Wind's page rendered **19** native titles
against the **3** its markup declared, and the gate counted 3.

It had already shipped that way: **O-Lyrica rendered 16 of them under a passing
claim**, from a single write in a per-knob setup path.

Two things generalise, and the second is the more useful:

1. **A source grep counts WRITES; the page renders INSTANCES.** One write,
   sixteen tooltips. A grep would have called O-Lyrica a one-site defect.
   `boot-all-uis`' per-plugin `title=` column was the only thing that ever saw
   them — and it is a REPORT, not a gate, so it printed 16 and exited 0.
2. **The half-enforced contract.** §4 has a markup half and a JS half; the
   markup scan enforced one and nothing enforced the other. Assertion 11 now
   feeds both sources into ONE check, because splitting them lets a plugin pass
   the half it happens not to trip.

Fixed in `5e9813c3` with both controls (O-Lyrica fails by name; a plant in clean
O-Tapestop goes red then green on restore). Repo swept: 41 localized plugins,
exactly one failure. O-Lyrica fixed in `bd89aa11` (v2.4.1). **O-Bells then hit
the identical shape and caught it because the gate had learned to see it** —
16 knob readouts, same `valueDisplay.title = 'Double-click to edit'` line.
O-MicrotonalSampler had 8 more, on grid-render paths.

**Four plugins carried this. Three would have shipped it green.**

## The tuning panel: the extractor's skip list is not an ownership test

`i18n-extract.js:442` drops `tuning-panel.js` **by filename**, with no ownership
test, so ~34 visible strings per plugin sat in no count. Ownership is not
uniform, and the brief's blanket module exemption was wrong for three of four:

| Plugin | What it consumes | Verdict |
|---|---|---|
| O-Wind | the MODULE file, by reference | exempt — its Tuning tab is English in both languages, about a third of its navigable surface |
| O-Bells | plugin-owned, **279 lines diverged** | localized |
| O-Formant | plugin-owned, **45 lines diverged** | localized |
| O-MicrotonalSampler | plugin-owned, **317 lines diverged** | localized |

None of the three lists the module in a `dependencies.json`, so
`/module-upgrade` will not revert the edits. **`check-i18n` DOES scan these
files** — `pageModules` deliberately does not apply `JS_SKIP` — so 12/13/15
cover the keys; only the worklist was missing. The orchestrator got this
backwards in the first dispatch and corrected both running executors mid-flight.

## The canvas decision, applied once and controlled

O-Formant was the only K4 plugin with `fillText` (5 sites; the other three had
0). Two localized via `I18N` with empty bodies, three exempt under D-01 arm 2
(IPA glyphs, F1–F5 markers). **Its negative control is the finding**: restoring
a hardcoded English `ctx.fillText('LYRICS', …)` still gave **ALL CHECKS PASS**.
The probe — recording `fillText`, en→fr→en, at three sibilance values so all
three manner branches paint — is the only thing that sees this class.

## Geometry — and the margin nobody had a number for

**Every plugin: 0 non-label elements moved EN→FR**, at 180ms and 1.7s.
O-Wind 8/10/99 → 0/0/0. O-Bells 11/47/134 → 0 across 14 states.
O-Formant 9 → 0. **O-MicrotonalSampler 116 → 0 across 22 states.**

**O-Formant quantified the standing shrink-to-fit hole**: a **20-character**
French caption in a **55px** `.knob-label` passes assertions 7, 8 and 8b
**green** — ~22px of overflow per side into empty space — and it takes ~**38
characters** before assertion 8 names it. Knob captions in this batch were sized
by hand against that margin, not by the gate.

The orchestrator wrote a **caption-fit probe** (painted text width vs the
element's own content box, per language, per state) and proved it by planting a
38-char caption on O-Bells' `label.fxReverb`: **+47.38px, 109.38 in a 62px
`.fx-title`**, clean on restore. **All four K4 plugins measure 0 in both
languages.** The hand-sizing held. The probe is in the session scratchpad, not
promoted to `scripts/` — promoting it changes what every future dispatch must
pass, which is a decision, not a cleanup.

**`dx` alone mislabels a pin as decoration — now five times.** O-MicrotonalSampler's
tonic row is the cleanest instance yet: a fixed 142px CENTRED row where the
caption's 12.84px growth produced `dx=6.42, dw=0` on all three siblings. Nothing
widened; everything moved.

Two pins shipped honestly labelled **design guards** because their negative
control passes: O-Bells' `.tonic-selector` gap (it prevents a 7.82px overflow
identical in both languages, which an en-vs-fr diff cannot see by construction).

## A harness that cannot reach a state certifies nothing about it

O-MicrotonalSampler's stub reported an empty `slots[]`, which leaves
`#clear-samples-btn` and `#batch-loop-btn` disabled — so **six dialog states
were unreachable and `check-ui-labels` died on a hidden button** rather than
reporting them. This was present from the executor's first run and read as a
harness crash. Giving the stub the `slots` a real loaded state carries changed
**no rendering** (the legacy flat list is only read when `cells` is empty) and
immediately surfaced **38 real failures**.

The same stub fired a 3.3s toast at load — alive for the English snapshot, gone
by the French one, reported as "2 vanished in fr". A transient is not a
regression; the Issues panel is now driven through the real `setLabel` path.

**Both are the same lesson from opposite ends: the gate was green (or crashed)
for reasons that had nothing to do with the plugin.**

## A THIRD gate fix — the modal shape the paint-layer rule could not see

`check-ui-labels`' `overlayOf` required `position: absolute|fixed` **on the
painting element**. The most common modal there is — a full-viewport `fixed`
root that paints nothing, flex-centring an opaque card that is only `position:
relative` — matched neither node, so **every caption inside every dialog was
compared against the page underneath it**. O-MicrotonalSampler writes all six of
its dialogs that way: **13 `[8b]` failures and 1 `[8]`, all in `*-open` states,
naming collisions no user can see.**

Same class as the gradient bug already recorded two comments above it: *the test
described one way of building the thing rather than the thing.*

Fixed in `7c51c4eb`. It only ever RELAXES (pairs move from a failure to a
printed NOTE), so it cannot turn a green plugin red. Controls:
- 13 → 3 with 11 skips reported. **The three survivors are the proof it is not
  blinded**: one is `label.rrBodyAfter` against `<code>` elements INSIDE the
  same dialog — same layer, still failing.
- A 94-char French caption planted inside the batch-loop dialog is still caught,
  by **`[7]`** with 3 new failures naming `blUnits` six times. In a flex dialog a
  taller caption PUSHES rather than overlaps, so `[7]` is the correct catcher
  there; `[8b]` remains the catcher for the absolute-positioned case.
- O-Contrabass (the plugin the original rule was written for), O-Tapestop and
  O-Bells: exit 0, zero failures each.

**Three gate fixes this batch, all from executor reports or executor-found
shapes.** Total for the task: nine.

## Pluralization — the one inline English inflection in the repo

O-MicrotonalSampler's `${n} file${n === 1 ? '' : 's'}` and five siblings are
**gone, not ported**: French pluralizes 0 as singular where English does not, so
a mechanical port is wrong at n=0 before it is wrong anywhere else. The count
now sits after a colon beside an invariant plural noun phrase. The same
reasoning rejected a one-line interval header that fits (`Intervalles · {n}
notes`, 138.44px in a 142px column) in favour of `Interv. · notes : {n}`
(114.45px), because the former reads "1 notes".

## Pre-existing ENGLISH defects, exposed by keying — the sixth batch running

| Plugin | Defect | Since |
|---|---|---|
| O-Formant | `#octave-stretch-value` rendered **9px past the 800px frame**, clipped, in every build | v1.25.4 |
| O-Bells | `"True Keys"` wrapped to two lines inside its own `.viz-btn`, making the viz row 10px taller | pre-v4.1.5 |
| O-Bells | the header's version label read **`v4.0.0` at v4.1.5** | v4.1.5 |
| O-Wind | 16 JS-written native titles (above) | — |

Each negative-controlled to re-fail in English before any French existed. Both
O-Formant's and O-Bells' root cause is the same one K3 named: **`flex: 1` leaves
`min-width: auto`**, so the item is floored by its longest word or its input's
intrinsic minimum.

## FOUND AND NOT FIXED — a functional defect wider than this task

**`window.confirm` and `window.prompt` do not work in JUCE WebViews.** Three
independent prior in-repo findings say so outright — O-MicrotonalSampler v1.0.2
("JUCE does not wire JS `confirm()` through WKWebView's UIDelegate"), the same
plugin's v1.23.7 ("returns `undefined` silently"), and O-simpleFM ("unreliable
inside JUCE WebViews") — and both of those plugins already route around it with
in-DOM dialogs.

The vendored `modules/persistence/preset-manager/js/preset-manager.js` still
does `if (confirm(...)) { this.deletePreset(...) }`, whose own docstring hedges
"confirm() may not work in all JUCE WebView contexts" and then does it anyway.
**`undefined` is falsy, so Delete Preset is a silent no-op** in every plugin
still on that path: **O-Bass, O-Comp, O-DigiDelay, O-Polystutter,
O-SpectralShaper**. O-Formant's Save uses `prompt()` and may never have worked
outside a browser. O-IntonationPad uses both directly.

Out of scope for an i18n task and deliberately untouched. It is a
**cross-plugin functional defect**, not a translation one, and it wants its own
task.

## NEEDS A HUMAN DECISION

1. **The `confirm`/`prompt` defect above** — ~7 plugins, Delete Preset and Save
   likely dead. The fix pattern already exists twice in the repo.
2. **O-Wind's Tuning tab is English in both languages.** Localizing the shared
   `scala-tuning-engine` panel is a module change touching O-Bowed, O-Reed and
   O-Bassoon as well. The three plugin-owned copies are now localized and
   therefore diverge further from the module — reconverging them is a separate
   piece of work.
3. **The caption-fit probe is not a gate.** It found nothing on four plugins
   that had been hand-sized against the documented margin, but the hole it
   covers is real and standing since Stage H. Promoting it to `scripts/`
   changes what every future dispatch must pass.
4. **English moves visibly on three plugins** where a pin had to be
   French-sized: O-Formant's preset cluster −9.1px and lyrics controls −28px;
   O-Wind's Effects tab (147 elements, all horizontal, no rewraps); O-Bells' 22
   / 62 / 144 elements per tab, all inside the frame. Correct, and visible.
5. **O-MicrotonalSampler's `Empl.` pin adds ~7.5px of space before the slot
   number in ENGLISH prose** — the visible cost of holding an inline element
   still. A shorter French word would remove it.
6. **`Interv. · notes : {n}`** is an abbreviation chosen over the fuller
   `Intervalles` on a measured 142px column. A native speaker may prefer the
   layout change instead.
7. Items from K1, K2 and K3 still stand — including the two O-Bowed render-harness
   version copies, and O-Reed's fifteen English XY-pad instrument markers.

## Not verified

- **Checkpoint 5 outstanding on all 42.** No human has seen any French UI.
- **All French repo-wide is machine drafts**, every entry `reviewed: false`.
- **Checkpoint 4 is reasoned on all four** — from the `isVoid()` guard, the
  native fns present in each built binary, and `auval`. **No host session was
  saved and reopened for any K4 plugin.**
- **No DAW test on any of the four.** `auval` and the headless harness only.
- **The Standalone `.app` is stale on all four** — `build-and-install.sh` builds
  VST3+AU only.
- **Windows/WebView2 font metrics** — the standing hardware-blocked deferral.
  K4's tightest margins are roomier than K2's 1.2–1.9px band, but
  O-MicrotonalSampler's three English-width pins (125/489/53px) sit 3.03,
  9.94 and 10.69px above their French, and a wider Windows face eats that first.
- **O-Bells: nine `<option>` captions and one `:hover` note were never
  MEASURED** — a closed native select's options have no box. Their French is
  confirmed rendered by assertion 2's text sweep; only their geometry is unknown.
- **O-Formant's tuning library list was populated by injection**, not by the
  real reveal path (the stub returns `[]`).
- **O-MicrotonalSampler's `.SCL`/`.KBM` file dialogs and `Generate`** were not
  driven — they call into C++ FileChoosers.


---
---

# STAGE L — T16: O-Prism, alone — STAGE L COMPLETE, 43 of 43

The largest single item in the project, batched with nothing. One executor, one
gate fix, one plugin commit.

| | |
|---|---|
| Version | **1.21.0**, commit `e4796486`, **10 files** (+1257 / −206) |
| Parent | `f70ea7a0` — the gate fix, landed ahead of it per the standing precedent |
| LABEL: plan / measured | 247 / **272** |
| js-prose: plan / measured | 9 / **24** |
| Parameters: plan / param-dump | 173 / **173** |
| Rendered text-bearing elements | **925** — the largest in the repo |
| Registry row | `66f7a70d`-style docs commit, duplicate check clean |

**`check-i18n` now passes across all 43 plugins with no flag. Canon v2: 43.
Canon v1: deleted.** `boot-all-uis`: 43/43 clean, 0 warn, 0 failed, and
**repo-wide native `title=` is 0.**

**Twenty for twenty.** The measured inventory matched the executor's own
re-measurement on every plugin of K2, K3, K4 and L, and the plan's text count
was wrong on every one. The inventory was always the authority.

## THE HEADLINE: 64 knob captions no gate could see, and the fix the gate forbade

`expandKnobMarkup()` walks 64 `div.knob-container[data-knob]` placeholders and
injects `<span class="knob-label">` from each container's **`data-label`
attribute**. Every detector was blind to them at once:

| Detector | Why |
|---|---|
| `i18n-extract` | does not scan `data-label`. The inventory does not contain the string once. |
| assertion 10 | reads the **static parse** of `index.html`. The caption is never a text node there. |
| assertion 11 | walks only `aria-label` / `placeholder` / `alt`. |
| assertion 12 | sees a concatenation whose caption half is a **variable**. |
| `check-ui-labels` | collects `[data-i18n]`. An unkeyed span is not one. |

**A quarter of the page could have shipped English with ALL CHECKS PASS on both
gates.** Sixth instance in this task of a gate certifying the absence of a thing
it cannot see, and the largest.

**The obvious fix is blocked by the gate's own rules**, which is what makes this
worth recording rather than just fixing. A concatenated
`data-i18n="knob.' + paramId + '"` makes `markupKeyRefs` record a dangling
`knob.` and reports all 64 real keys DEAD (assertion 15); `setLabel(el, 'knob.'
+ id)` is a computed key (assertion 13). Both rules are correct. The shape that
satisfies them: **key the STATIC container, and move the attribute onto the
generated span at expansion time.** It registers in assertion 15's `markupKeys`,
needs no exemption, and puts `check-ui-labels` — not `check-i18n` — in the
evidence seat.

64 keyed, **35 distinct keys**. Post-init: `containersKeyed=0`, `labelsKeyed=65`,
`vinesAlive=64`, `valuesAlive=64`, unchanged after `en→fr→en→fr`, 0 page errors.
The `containersKeyed=0` assertion is load-bearing: leaving the key on the
container makes `applyLabel`'s `textContent =` **delete the entire expanded
knob** — svg, vine and readout — on the first language switch.

**The negative control is the finding.** Un-keying one container leaves
`check-i18n` **byte-identical** when the key is shared (29 of 35) and trips only
the dead-key rule when unique (6). `check-ui-labels`' `[2]` count drops 189 → 188
either way.

## THE TENTH GATE FIX — a nested template desynchronizes the comment stripper

`stripJsComments()` walked a backtick with the same flat "copy to the next
matching quote" loop it uses for `'` and `"`. An interpolation can hold a nested
template, a quote, a regex and a comment; reading its contents as string content
leaves the parse **one quote out of phase for the rest of the file**.
`readLiteralAt()` in the same file already recursed correctly — `stripJsComments`
never called into that knowledge.

Found by the executor, who **stopped and reported rather than working around
it**, which is the protocol working as designed. Fixed in `f70ea7a0`.

| | before → after |
|---|---|
| O-Lyrica comments swallowed | 38 → 0 |
| O-Prism comments swallowed | 12 → 0 |
| `check-i18n` repo-wide diff | **exactly one line** — O-Prism `[15]` FAIL → PASS |

The four keys it falsely reported dead — `label.genStartHarm`, `label.genEndHarm`,
`label.genGenerator`, `label.genNotes` — are each declared in a `data-i18n`
attribute inside a scale-generator `innerHTML` template and read on every
language change. Driven in the harness afterwards in both languages and all
three generator variants, so they are **live, not merely unreported**.

**It fails SAFE, and that is measured, not reasoned.** Five plants of raw
unkeyed English — four by the executor on O-Prism, one by the orchestrator
inside the exact region the old scanner swallowed in O-Lyrica's inline module —
were caught by BOTH scanners, because assertion 12's discovery runs through
`readLiteralAt`, which was never broken. **A desynchronized scan invents a false
failure and does not open a false pass.** So the accurate statement about
O-Lyrica v2.4.1 is narrower than it first read: it shipped under a partially
corrupted comment strip, **with no demonstrated hole**. The executor's first
draft called it a shipped defect; that was stronger than the evidence.

**Two regressions in the fix itself, both caught by the before/after control
rather than by reading the diff:**

1. **Position preservation** — the invariant the whole function rests on, so a
   line number reported against the stripped source still points at the right
   line. A first draft broke it (+1 char on O-simpleBeatmaker) because the new
   interpolation scanner inherited `n` as a regex-preceder — it is in the set to
   cover `return` — so `${Math.floor(n / 12) - 1}` read its **division** as a
   regex literal and ran to end of line. An interpolation holds an expression,
   never a statement, so `return` cannot appear inside one. Caught by an
   invariant sweep over 270 script blocks; back to 0 violations.
2. **The first metric was measuring the wrong thing.** "A `//` survived
   stripping" is not a defect — a `//` inside a string is *supposed* to survive
   — and counting them made O-Octagon look like a 3 → 8 regression when the new
   scanner was right and the old one had been wrongly stripping five lines of a
   template that holds an HTML page. **The gate-outcome diff is the control that
   means something; the comment count only pointed at where to look.**

## A rotated SVG's bounding box is 41% wider than the knob — general, not French

The most reusable geometry finding of the stage, and it holds for every plugin
drawing this knob, in both languages, before any localization exists.

```
.knob-visual                52 × 52 px
.knob-visual svg            transform: rotate(-135deg)
getBoundingClientRect(svg)  73.54 × 73.54 px  (52 × √2)  →  +10.77 px per side
```

`getBoundingClientRect` returns the **axis-aligned box of the transformed
shape**. The visible artwork has not moved a pixel; only the measured rectangle
has. Columns sit 4 px apart, so each neighbour reaches **6.77 px into yours** and
a centred `.knob-label` has **38.45 px, not 52**. Against the neighbour's
`.knob-track` circle the cap is **49.8 px**.

Three consequences: `[8b]` reports a caption past 38.45 px as intersecting the
neighbour's svg — **true of the boxes, false of the pixels**, over-strict and
never a false pass, and why 13 knobs went red before sizing; **`dx` alone is
useless on this family**, because a caption growing inside a centred column
moves siblings without widening anything; and the honest fix is at source —
rotating an inner `<g>` instead of the `<svg>` is rendering-identical, restores
every knob caption in the suite to its full column, and retires the `[8b]` false
positives. **Not made** — it is working visual code across three viewBox variants
plus three hand-written knobs, changed for a measurement artefact. Decision item.

## The caption-fit probe was VACUOUS as specified, and the hole it covers is not here

The brief told the executor to point O-Formant's caption-fit probe at the knob
captions. Pointed at `.knob-container` it reports 0 for a planted 38-char
caption, because that box **shrink-wraps**. Re-pointed at the design column
(`.knob-visual`, 52 px) it works.

And then it found nothing — because planting the same 38-char caption in the
**source** makes the real gate fail with 2 `[7]`s. **The shrink-to-fit hole does
not exist on this page**: an oversized caption here PUSHES rather than silently
overflowing. That is a property of this layout, not a clean bill of health for
the probe, and the standing Stage-H hole is still standing elsewhere.

## Geometry

**EN→FR non-label movers: 24 `[7]` failures → 0**, with `[4]`, `[5]`, `[6]`,
`[8]` and `[8b]` all 0 across 23 states. Verified at 180 ms **and** 1.7 s, with
`en@180 vs en@1700`, `fr@180 vs fr@1700` and `HEAD en@180 vs en@1700` all
**moved=0** — the page is pixel-identical at both, so no transition sits behind
any number.

**English moves exactly 8 elements**, identically at both settle times in all 16
states: the preset browser and six children at `dx=-77.5`, the subtitle at
`dx=-155.0`. One cause — the gear is a fourth `space-between` child. Nothing
outside the header, no `dw`, no `dh`.

**Nine pins ship, each reverted alone** with its own failure count (1, 7, 13, 2,
7, 2, 2, 6, 6). **A tenth was measured as DECORATION and removed.** `dx` alone
would have mislabelled two more as decoration — `delayFeedback` is
`dx=0.6 dw=-1.2`, `oscAWarpAmt` is `dx=1.6 dw=-3.2`. **That is the sixth and
seventh time `dw` has been the deciding number.**

**French SHRANK on the dominant failures** — `Réinj.` −19.75, `Étir.` −16.83,
`Langue` −14.97, `Bibliothèque` −9.34. **Five of the nine pins exist to stop
shrinkage, not growth.**

**A pin trap worth carrying:** the first ops-bar pins were sized to painted text,
and under global `box-sizing: border-box` that gave a 69 px content box which
**wrapped `Normalize Global` onto two lines in ENGLISH**. Border-box sizing
(`text + 26`) is correct.

## Native titles: the source grep counts writes, the page renders instances — again

Assertion 11's markup scan sees **6**, not the 8 a grep finds, because
`scanHtml` does not parse inside `<script>`. And "6 rendered" was badly low:
**6 at load, 127 after Matrix, 248 after Rotation.** Now **0 in every state**,
repo-wide 0.

This is the O-Wind/O-Lyrica lesson recurring with a different multiplier: one
per-knob setup path gave O-Lyrica 16, and O-Prism's grid-render paths gave 248.

## Corrections to the orchestrator's own brief

- **The 65th knob container does not exist.** The brief asserted 65 and asked
  which carried no `data-label`. `grep -o` counts 65 because one match is the
  literal inside the **source comment at `index.html:1658`**, which a browser
  does not render. Runtime is 64 in every state. Nothing was missed.
- The caption-fit probe instruction was vacuous as written (above).

## `I18N_EXEMPT` — 105 entries, 101 scoped

33 parameter options (arm 1, scoped `.param-select`); **28 wavetable catalogue
names that are NOT arm 1** — `oscATable` is an `AudioParameterInt`, so they are
exempt as a C++-owned mirrored catalogue, and saying so is the point of a
reasoned exemption; 36 mod-matrix names (arm 1, scoped `#mod-matrix-rows`
because `Osc Mix` is also a live key); `— Init —` (D-02); `12-TET Standard`
(arm 3); 4 unscoped.

**Two arm-1 overrules, decided by geometry and stated plainly.** The five bypass
buttons are **keyed** — upper-case `ON`/`OFF` is byte-identical to nothing. The
delay Sync button is **exempt**: it was keyed first, then taken back to arm 1
when `Arrêt`/`Marche` pushed the delay row 16 px. So `label.sync` ships
`sameAsEn` while the LFO button keeps `Synchro` — **the same English word, two
answers, decided by measurement.**

## Not verified

- **Checkpoint 5 outstanding on all 43.** No human has seen any French UI.
- All 3202 French entries repo-wide are machine drafts, every one
  `reviewed: false`.
- **Checkpoint 4 reasoned, not executed**, on O-Prism — from the `isVoid()`
  guard, the native fns present in the binary, and `auval`. No host session was
  saved and reopened.
- No DAW test. `auval` and the headless harness only. Standalone `.app` stale.
- **Four O-Prism states not driven:** the two drag overlays, closed-`<select>`
  option *geometry* (text confirmed), the `.SCL`/`.KBM`/`Export HTML`
  FileChoosers, and the **empty** user-wavetable state.
- **Windows/WebView2 font metrics**, the standing hardware-blocked deferral, and
  O-Prism's margins are the tightest yet measured: `Satur.` **0.71 px**,
  `Prof A` 0.78, `Prof B` 0.86, `Taille` 0.90.
- Generated scale names stay English — `Rank-2 (696.6¢, 12 notes)` is
  **persisted through C++ as the stored identifier**, so localizing it would
  change saved state.
- Six `<optgroup label>` strings: no gate scans them and canon has no attribute
  for them.
- `<html lang>` does not follow the language selector. Canon-owned, all 43.

# STAGE M — T17, BATCH M1: the ten cheapest of the 22 bare plugins — M1 COMPLETE, 10 of 10

The first batch of the last stage. Ten plugins, one commit each, three
orchestrator commits ahead of or behind them, and six brief amendments — every
one of them written because a measurement contradicted the brief.

| | |
|---|---|
| Plugins | **10 of 10** |
| Parameters, dumped | **72** — matching `auval` and `params2.tsv` exactly, a third independent confirmation of the 607/22 scope |
| Tips authored and bound | **87** = 67 parameter + 20 chrome |
| Unreviewed French | 3202 → **3289**, exactly +87 |
| Zero-tip plugins | 22 → **12**, the exact M2/M3 remainder |
| `check-i18n` | **43/43 ALL PASS**, all canon v2 |
| `boot-all-uis` | **43/43 clean**, 0 warn, 0 failed, native `title=` **0** repo-wide, 3789 text-bearing elements unchanged |
| New per-plugin gates | 10 × `tests/ui_tip_render_check.js`, **all passing** |

| Plugin | Version | Commit | Params → tips | Gate | Latch control |
|---|---|---|---|---|---|
| O-AnalogSaturation | 1.3.0 | `18f914b2` | 4 → 4+2 | pass | 4669 px² |
| O-Bass | 1.5.0 | `a983fddd` | 5 → **3**+2 | 125 | 5110 px² |
| O-Emulator | 1.2.0 | `5cf6bba1` | 5 → 5+2 | 123 | shipped WITH the latch |
| O-Comp | 1.7.0 | `40a156b4` | 7 → 7+2 | 156 | 3800 px² |
| O-Tremolo | 1.8.0 | `85e94b5f` | 7 → **6**+2 | 186 | 4600 px² |
| O-Chorus | 1.4.0 | `8fc4a4e6` | 8 → 8+2 | 240 | 4672 px² |
| O-DigiDelay | 1.4.0 | `c1878590` | 8 → **7**+2 | 216 | 4964 px² |
| O-SimpleReverb | 1.7.0 | `a1fce025` | 8 → 8+2 | 169 | 5110 px² |
| O-Bassoon | 1.2.0 | `493bbad9` | 10 → 10+2 | 198 | 5280 px² |
| O-Texture | 0.3.0 | `d6473d63` | 10 → **9**+2 | 208 | 5130 px² |

## THE HEADLINE: T17 said "content work, not engineering," and all three
## detectors said zero

Measured on all 22 bare plugins before the first dispatch: `id="tooltip"` **0**,
`.tooltip {` **0**, `closest("[data-tip]")` **0**. Canon v2's `applyI18n()`
writes `data-tip-title` and `data-tip` **attributes**; the code that reads them
and paints a surface is per-plugin and did not exist on any of them.

Authoring 72 bodies and binding them — exactly what T17 specifies — would have
shipped **72 invisible strings past three green gates**. `check-i18n` reads the
table statically; `check-ui-labels` has no tooltip awareness whatsoever;
`boot-all-uis` counts `aria-label` and `title` and never `data-tip`. **Seventh
instance in this task of a gate certifying the absence of a thing it cannot
see**, and the first where the stage's whole deliverable sat inside the hole.

**Measured on three separate plugins, not asserted:** with `setupTooltips()`
disabled, `check-i18n --plugin` and `check-ui-labels --plugin` both print ALL
CHECKS PASS while the new render gate fails 8 (O-AnalogSaturation), 150
(O-Tremolo) and 158 (O-Chorus) assertions.

## THE DEFECT IN THE RENDERER THE ORCHESTRATOR SPECIFIED

The brief told ten executors to port O-simpleFM's `setupTooltips`. **It opens a
tip on any `focusin`, and a mouse click on a `<button>` focuses it** — so the tip
`pointerdown` had just hidden reopened immediately, pointer still on the anchor,
nothing left to dismiss it, sitting on top of whatever the click had opened.

O-Emulator's executor found it on its own page, stopped, and shipped a
last-input-device latch. Measured overlap with the settings popover, per plugin,
by intersecting the two rects: **3800 to 5280 px²** on all nine that lacked it.

**Both gates were green the whole time.** `check-ui-labels` classes the surface
as `pointer-events: none` decoration and has nothing to say about it. What
exposed it was the `[8b]` inert-element count moving **7 → 9** *inside a passing
run*. **Read the counts in a green gate, not only its verdict.**

`:focus-visible` is not the discriminator: Chromium reports it false for a
programmatic `.focus()` after a click, so a gate driving focus directly measures
"no tip" and records that as correct — a false pass built into the fix.

## AND THE ASSERTION FOR IT WAS DECORATION, twice

The orchestrator wrote the focus assertion for the two pilots that had landed
without the latch. **It passed 125/125 with the latch deleted.** An earlier
section of each gate leaves focus on `#gear-btn`, and **clicking an
already-focused element fires no `focusin` at all** — precisely the trap
O-Emulator's executor had reported one message earlier.

With `activeElement.blur()` first, the control fires. Four executors then ran the
2×2 independently and O-Texture tabulated it:

| latch | blur | click assertion | keyboard assertion |
|---|---|---|---|
| yes | yes | PASS | PASS |
| yes | no | PASS | PASS |
| **no** | yes | **FAIL 5130 px²** | PASS |
| **no** | **no** | **PASS — decoration** | PASS |

The keyboard half stayed green in all four, which is what proves the two
assertions are independent rather than one counted twice.

## THE SHIPPED DEFECT NO GATE COULD SEE: an unclickable language selector

O-Bassoon's executor found that its settings popover had been **painted over
since v1.1.0**. `body` is `display: flex`, so the header bar and tab bar are flex
items — and `z-index` applies to a flex item at `position: static`, so each opens
its own stacking context. Both were `z-index: 10`, a tie broken by document
order. `#settings-popover`'s own `z-index: 21` is scoped **inside** the header's
context and cannot climb out. `elementFromPoint` at `#lang-select`'s centre
returned `.tab-btn`.

**The only control the entire i18n feature adds could not be clicked.** No gate
saw it: `check-ui-labels` compares rectangles, and **a rect is unchanged by paint
order**. It surfaced only when a *new* render gate tried to HOVER `#lang-select`.

That generalised into a repo-wide `elementFromPoint` hit-test over all 43
plugins — blindness-checked first by reverting O-Bassoon's committed fix, which
returned FULLY BLOCKED, covered by `BUTTON.tab-btn`.

**Two more plugins were affected, both shipped that way since Stage K:**

| Plugin | Blocker | Cause | Fix |
|---|---|---|---|
| O-Wind | `BUTTON.tab-btn` | `.preset-bar` / `.tab-bar` both `z-index: 10`, flex items of a flex body | `.preset-bar` → 30 |
| O-MicrotonalSampler | `#tab-samplemap` | `#header` / `#tab-bodies` / `#control-strip` all `z-index: 1` | `#header` → 2 |

Fixed in `9ade62fe`, each negative-controlled alone, both rebuilt and reinstalled,
both AU-validating. **41 of 43 were reachable at all three probe points**; those
two were the entire population. No version bump — O-Wind's latest tag is v1.9.0
against a source 1.17.0, O-MicrotonalSampler's v1.9.1 against 1.24.0, so neither
current version has been released.

## A dumped parameter is not necessarily a control — 4 of 10

72 parameters produced **67** parameter tips, and every gap is a finding rather
than an omission. An authored body with no binding is an ORPHAN that assertion 2
fails, so none of these was papered over and **no control was added to satisfy a
count** — that is a feature change with a geometry cost.

| Plugin | Parameter | Why it has no tip |
|---|---|---|
| O-Bass | `latency_mode`, `bypass` | **No control in the WebView in any version.** Automatable and host-reachable, page-unreachable. |
| O-Tremolo | `SYNC_DIVISION_PARAM` | The Speed knob *becomes* its detented stepper when sync is on. Described inside `tip.speed` / `tip.tempoSync`. |
| O-DigiDelay | `division` | Page-reachable, but only through `#time-knob`, which another parameter already anchors. A second `TIP_BINDINGS` row on the same node would silently overwrite the first. |
| O-Texture | `X` / `Y` | Share one control, the `#xy-pad` canvas. One entry names both axes. |

**`applyI18n` writes onto the element a selector resolves to, so two bindings on
one node mean the second overwrites the first — while `check-i18n` cheerfully
reports two bound tips.** Three plugins' gates now assert that all bindings land
on **distinct** nodes by identity.

## "Bind to the ids the UI already uses" — false on 9 of 10, for six reasons

T17's one-line instruction failed on every plugin but O-DigiDelay, and the
selector half and the target half fail **independently**:

- **O-Chorus** — no knob carries an id at all. The only id inside a knob is the
  SVG arc, and `.knob-vine` is `fill:none; stroke-width:3`: walked with
  `elementFromPoint`, **147 of 4526 points (3.2%)** of the cell hit it, and that
  target's size *moves with the parameter* because the painted length is
  `stroke-dashoffset`.
- **O-Emulator** — 5 of 7 anchors are CSS selectors.
- **O-Comp, O-SimpleReverb, O-Tremolo** — every selector is an id; the target is
  a wrapper anyway, because the id'd node is a 52 px vine face.
- **O-Texture** — 5 of 11 are selectors, two with no per-parameter element at all.
- **O-Bassoon** — selectors are not ids, and no wrapper is needed either: the
  `.knob-control` IS the hover cell.
- **O-DigiDelay** — **true**, and structurally so: its positioning block places
  every control by id.

Two further traps: **a wrapper class can match twice** (O-Tremolo's
`.waveform-section`, the second wrapping the canvas — `closest()` reaches the
right one, a bare `querySelector` would be right by luck); and **the chrome must
bind BARE** wherever the gear and the selector share an ancestor, or hovering
`#lang-select` resolves to the gear's tip (O-Comp, O-Chorus).

## Probe artefacts that argue for deleting the fix

1. **A keyboard-tab probe sampling DURING the 120 ms fade reports a false
   "never opens" — and the obvious response is to delete the latch.** O-Comp's
   first control slept 80 ms into the transition while treating anything under
   opacity 0.99 as hidden, and reported "none in 20 tabs" for a path that works
   at tab #5. O-SimpleReverb hit the identical artefact. O-Chorus settled 150 ms
   past the transition and tested `visibility` instead.
2. **A STATIC regex for `lastInputWasPointer` stays green with the guard clause
   deleted** — the declaration, the write and the clear all survive. Only the
   behavioural control discriminates.
3. **A plant sized by habit is a control that cannot fail.** O-Tremolo's 40×
   plant (880 chars, ≈390 px) *fit and reported nothing* on a 400 px frame with
   384 px of clamp room.
4. **`page.evaluate` given a function-source STRING** returns the function
   object, is unserialisable, resolves to `undefined`, and sails through a
   truthiness assertion over a surface nobody read (O-Bass).

## Geometry — and the claim in the brief that was false

**moved-before 0, moved-after 0 on all ten.** `check-ui-labels` output is
byte-identical to each plugin's pre-change baseline. **No pin was added on any
plugin**, so none was claimed and none was owed a negative control.

The brief asserted that an un-hidden tooltip surface "would enter
`check-ui-labels`' text sweep and every geometry diff." **O-Bass's executor
negative-controlled it and it is false**: un-hiding the surface left that gate
byte-identically green, because a fixed box at 0,0 has the same rect in both
languages — it neither moves nor changes the visible set. What catches it is
`check-i18n` assertion 10 and `boot-all-uis`' text count (14 → 16).

**The clamp is the normal path, not an edge case.** O-DigiDelay measured 27 of
27 hovers flipping above the cursor, 9 flipping left, and **19 landing on the
8 px top rail**; a flip-only renderer puts 19 of 27 above the top edge, and a
renderer that clamps *before* the flip passes every containment row while
failing only an explicit second-clamp assertion. O-Chorus: **17 of 20**
placements outside on both sides of the flip. O-Bass at 420×320: every anchor in
both languages placed by flipping, two on the 8 px floor.

## Units: the brief's tendency was wrong in both directions

"O-Comp is the one M1 plugin with real units" was false at the second plugin.
Measured: O-Comp (6 of 7), O-Bass (3 of 3), O-Emulator (4 of 5), O-Tremolo (3 of
7) and O-DigiDelay (6 of 8) carry real `label` values. **O-Chorus, O-SimpleReverb
and O-Texture carry none at all** and every range was recovered from the page's
own formatter with a file:line citation.

O-Chorus is the sharpest case and T17 understated it: **seven of its eight dumped
ranges are wrong for a user**, because the formatter rescales them —
`0.00 .. 1.00` renders as `50%`, and `tone`'s formatter *adds a sign*. Only
`rate` is quotable as dumped.

Two dump-vs-page disagreements reported and not fixed: O-Bassoon's
`vibrato_depth` declares `" cents"` and renders `" c"`; O-SimpleReverb's
`LP Filter Freq` / `LP Filter On` are backed by `makeHighPass`, and the page's
`LOW CUT` caption is the correct one. Renaming an `AudioParameterFloat` is
host-visible.

## Other findings worth carrying

- **A disabled control does not swallow pointer events** — Chromium retargets to
  the nearest enabled ancestor, so a row-bound tip opens over all five disabled
  source buttons (O-Texture). The strongest argument for the wrapper form.
- **A decorative overlay can paint over the tip.** O-Texture's `body::after`
  fern sits at `z-index: 1000` directly on `.freeze-toggle`; at the popover's
  `z-index: 61` the tip would paint *under* it — invisible to any hover check
  reading only `visibility`. Its gate asserts computed z-index against the fern's.
- **An open tip does not re-render on a language change** — canon behaviour,
  shared by all 21 shipped tooltip plugins. On O-AnalogSaturation
  `check-ui-labels` assertion 7 is green **partly because of it**: the tip's rect
  is identical in `en` and `fr` only because it never re-rendered. If the canon
  is ever taught to refresh an open tip, assertion 7 begins comparing tip
  rectangles across languages on every Stage M plugin, and French wraps taller.
- **A pre-existing `SyntaxError` window is real during a concurrent batch.**
  Two executors reported `boot-all-uis` failing on O-Chorus mid-run; it was that
  executor's uncommitted mid-edit state and resolved when it landed. Both flagged
  it rather than explaining it away, which is the K3 lesson working.
- O-Chorus added `drag.active` beyond the reference family: this page starts a
  knob drag on mousedown, and a drag crossing into a neighbouring container would
  otherwise open that neighbour's tip mid-gesture. `pointerdown` alone cannot
  cover it, because `pointerover` arrives after it.

## NEEDS A HUMAN DECISION

1. ~~**French decimal separator, and M1 split on it.**~~ **RESOLVED by the
   developer, 2026-08-30: the COMMA.** It is correct French and it is what all
   21 already-shipped tooltip plugins write. The readout keeps its point — D-03
   exempts the readout NODE — so a body and its readout spell the number
   differently on purpose: one is prose, the other a machine-formatted value.

   The split was **wider than reported**. O-Chorus flagged it and O-Comp
   documented the opposite choice, but a scan of all ten plugins' French entries
   for `\d+\.\d+` found **O-SimpleReverb had shipped the point too, silently** —
   and its own header comment claimed the comma while the entry below it used a
   point. Two entries changed (`tip.attack`, `tip.decay`), three comments were
   rewritten, three plugins rebuilt. Fixed in `f0eb50c8`. **A convention that
   two executors argue about is one a third gets wrong without noticing.**
2. **No hover-help on/off toggle.** M1 ships tips always-on. O-Tapestop and
   O-Bitrot have a toggle; the other 41 do not. Uniformity is a separate pass.
3. **`#lang-select` is 59.0 × 16.0 px on O-DigiDelay**, under WCAG 2.5.8's
   24×24. Pre-existing at v1.3.0.
4. **Keyboard reach is partial by design.** Knobs on most of these pages are
   pointer-drag `div`s with no `tabindex` and never were keyboard-operable, so
   the keyboard half of hover-help reaches only the natively focusable controls.
   Adding `tabindex` is a feature change with focus-ring geometry cost.
5. **O-Bassoon's PLUGINS.md state cell reads "Stage 0" against a STATUS.md of
   stage 4.** Pre-existing; the M1 registry commit changed versions and dates only.

## Not verified

- **Checkpoint 5 outstanding on all 43.** No human has seen any French UI. All
  **3289** French entries repo-wide are machine drafts, every one `reviewed:
  false` — M1 added 87 to that worklist.
- **No DAW test on any of the twelve plugins touched.** `auval` and the headless
  harness only.
- **The Standalone `.app` is stale everywhere** — `build-and-install.sh` builds
  VST3 + AU only.
- **Rendering was verified in headless Chromium**, against the tree `serve-ui.js`
  assembles from the same `juce_add_binary_data` SOURCES list the build embeds —
  **not inside a real WKWebView**.
- **Windows/WebView2 font metrics** — the standing hardware-blocked deferral.
  Nothing in this batch retires it, and tooltips are new surface area for it.
- O-Texture's ONNX/inference path was neither touched nor exercised.


---

# STAGE M — T17, BATCH M2: eight of the remaining twelve — M2 COMPLETE, 8 of 8

Eight plugins, one commit each (plus one comment-only follow-up), two
orchestrator commits around them. The param-dumps were not wired for any of the
eight at dispatch; wiring and running them was the orchestrator's first job.

| | |
|---|---|
| Plugins | **8 of 8** |
| Parameters, dumped | **177** — every count matching the plan exactly |
| Parameters with a control | **170**; seven are host-reachable and page-unreachable |
| Tips authored and bound | **183** = 166 parameter + 16 chrome + 1 double-binding |
| Unreviewed French | 3289 → **3471** |
| Zero-tip plugins | 12 → **4** — O-Bells, O-Formant, O-Prism, O-Wind, the exact M3 set |
| `check-i18n` | **43/43 ALL PASS**, all canon v2 |
| `boot-all-uis` | **43/43 clean**, 0 warn, 0 failed, native `title=` **0** repo-wide |
| New per-plugin gates | 8 × `tests/ui_tip_render_check.js`, **3424 assertions, 0 failures** |

| Plugin | Version | Commit | Params → tips | Gate | Latch control |
|---|---|---|---|---|---|
| O-Detune | 1.7.0 | `a4c243af` | 18 → **16**+2 | 393 | 5624 px² |
| O-Freeze | 2.2.0 | `4090e6ac` | 12 → 12+2 | 311 | 4293 px² |
| O-TextureForge | 1.2.0 | `56be79ca` | 12 → 12+2 | 287 | 4648 px² |
| O-AnalogEQ | 1.3.0 | `308b3360` | 16 → **11**+2 | 308 | 4998 px² |
| O-Bowed | 1.6.0 | `eb62babf` | 29 → **28**+2 | 457 | 5504 px² |
| O-Reed | 1.3.0 | `fadd9b7e` | 35 → **33**+2 | 523 | 5280 px² |
| O-MicrotonalSampler | 1.25.0 | `adb7fc4f` | 19 → **18**+2+1 | 349 | 6969 px² |
| O-GrainScatter | 2.6.0 | `5398b5a2` | 36 → 36+2 | 796 | 3650 px² |

O-AnalogEQ's 16 → 11 is not four missing tips: **four dual knobs carry two
parameters each on one hover target**, plus `output_gain` with no control.

## THE HEADLINE: seven parameters a DAW can automate and a user cannot see

M1 found one (O-Bass's `latency_mode` and `bypass`). M2 found **seven across five
plugins**, and the reason each exists is different — a UI simplification that
kept the parameter, a relay built for a selector nobody added, a control that
lives in a lazy-mounted shared module, a helper function never called.

Not one executor added a control to satisfy the count, which was the instruction
and the right call: a control is a feature change with a geometry cost, and a
body with no binding is an ORPHAN that assertion 2 fails by design. Three
plugins — O-Freeze, O-TextureForge, O-GrainScatter — reconciled exactly in both
directions.

**O-Detune's pair is the sharpest.** `focus_low` (20–500 Hz) and `focus_high`
(1–20 kHz) are not leftovers: the DSP writes their coefficients every block
(`PluginProcessor.cpp:602-603`), `PluginEditor.cpp` relays them, and the page
even builds slider states and formatters for them. Everything exists except the
element.

## TWO SHIPPED DEFECTS FOUND BECAUSE SOMEBODY HAD TO DESCRIBE THE CONTROL

Neither is a tooltip bug. Both surfaced because authoring a body means reading
what the control actually does.

**O-TextureForge has had three blank readouts since v1.0.0.** `setupKnob` is
passed `(n) => ''` as the formatter for `grainSize`, `grainDensity` and
`outputGain` (`src/app.js:705, :706, :708`), so the first `updateDisplay()`
erases the authored markup fallbacks `50ms`, `8`, `0 dB` and nothing replaces
them. Measured in the harness — `{grainSize:"", grainDensity:"", outputGain:""}`
while the other eight read `50%`. The same executor corrected two v1.1.0
`I18N_EXEMPT` reasons that claimed those nodes were "written by src/app.js"; they
are not written at all.

**O-Reed's `instrumentPreset` is a dead parameter.** `pInstrumentPreset` is
fetched at `ReedWindVoice.cpp:50` and never `load()`ed anywhere in `Source/`. The
dropdown moves the automation lane and nothing else; the source comment
describing "macro crossfading full parameter sets" describes something never
built. The 21 factory presets carrying those voicings are unreachable from the UI
(no preset bar) and from the host (`getNumPrograms()` returns 1). The same
plugin's reported latency does not follow Oversampling — `setLatencySamples()` is
called once in `prepareToPlay()` from the default 2× path.

Both plugins put the truth in the tooltip body rather than describing the feature
as intended.

## THE GATE FINDING THAT REACHES BACK ACROSS ALL ELEVEN PORTS

O-AnalogEQ measured that the **post-flip re-clamp is unreachable by
construction**. `position()` re-clamps with `ny = innerHeight - M - r.height`,
then floors with `Math.max(M, ny)`. After a flip `ny = y - h - 12`, so the test
`ny + r.height > innerHeight - M` collapses to `y - 12 > innerHeight - M` — **it
stops mentioning the tip's size** — and fires only for a cursor outside the
viewport. Same collapse on x.

O-Chorus's copy credits that line with *"every vertical placement on this page
lands on the line below, not on the flip above it."* The behaviour is real; the
credited line is not producing it. Deleting the `Math.max` floor puts a tip at
`top −95.1` — 103 px off the page — while all 13 shipped tips stay green.

**Eighth instance in this task of an assertion credited with work it cannot do.**

## THE SECOND ONE: `applyI18n` FALLS BACK, SO THE HOVER SWEEP CANNOT SEE A BROKEN WRAPPER

O-Reed broke one binding's wrapper deliberately. Assertion [1] failed;
**assertions [2], [3] and [4] all passed, in both languages** — because
`applyI18n` resolves `el.closest(w) || el` and hands back the bare element. The
tip still opens, on the wrong-sized cell.

A gate asserting only "the tip appeared, with the right text, inside the
viewport" is structurally blind to it. That is why [1] must be a hard FAIL.

## AN ANCHOR CAN BE UNOPENABLE WHILE PASSING EVERY STATIC CHECK

Three shapes, three plugins, and each would have shipped a tip nobody could open:

- **O-MicrotonalSampler** — `#ctrl-attack` is a **1×1 px, opacity-0,
  `pointer-events: none`** range input. It is an id, it resolves, `check-i18n` is
  satisfied. The addressable node is `[data-knob-id="ctrl-attack"]`.
- **O-AnalogEQ** — `.knob-outer` and `.knob-inner` are **both**
  `pointer-events: none`; the parent resolves outer-vs-inner by the cursor's
  distance from centre, not a child boundary.
- **O-GrainScatter** — **14 of 36 controls are `pointer-events: none` in the
  DEFAULT state**, from two feature gates. Found by the gate's first run, 168
  failures.

All three drove out of the state through the page's own `valueChangedEvent` path
rather than stripping the class, and O-GrainScatter then pinned the rest state as
its own assertion. Also recorded: **`pointer-events: none` does not remove an
element from the tab ring** — three gated `<select>`s open their tip from the
keyboard while unreachable by mouse.

## THE BLUR IS LOAD-BEARING ON SEVEN OF EIGHT, AND THE EIGHTH IS THE USEFUL ONE

O-TextureForge ran the full 2×2 and found the blur made no difference: latch
removed fails at 4648 px² either way, because the section preceding that control
is pure mouse work and the French sweep had already blurred the gear. **It kept
the blur and recorded in the gate that this is an accident of section order, one
edit from decoration.**

That is the correct handling of a carried trap. O-Tremolo's finding is about a
class of gate, not a law.

The static-grep half is now confirmed on six plugins: deleting only
`if (lastInputWasPointer) return;` leaves every `grep -c lastInputWasPointer`
green.

## THE ONE MEASURED PROOF THAT THE PROCESSOR EDIT IS DSP-NEUTRAL

Every M1 and M2 plugin took a `#include "PluginEditor.h"` behind an
`#if JUCE_WEB_BROWSER` guard so the param-dump console target links. Everywhere
that neutrality is argued from the preprocessor.

**O-Bowed has a render harness, rebuilt and re-run it, and its canonical preset
renders bit-identical to the committed golden** —
`93124fb8dd8223caafac5948c988a226230363d79a17323d386e9a1db34c8891`. That is the
only measurement in the stage.

## "BIND TO THE IDS THE UI ALREADY USES" — ONE PLUGIN IN FOURTEEN

O-Freeze is the first and only plugin where the naive reading of T17 is correct
on **both** halves: `.knob` is a flex column holding visual, caption and readout,
so the id already *is* the hover cell. Every binding bare, no `closest()`
anywhere.

The other seven fail on the selector half, the target half, or both — each for a
different reason. O-GrainScatter is the extreme: **3 of 38 anchors use an id, and
the page has 4 ids in total.** Chrome binds bare on all eight, because
`.settings-cluster` holds the gear and the popover on every page in the batch;
O-TextureForge found the mirror image, where `#midi-mode` shares
`.bottom-controls` with `#drop-zone`.

## ORCHESTRATOR WORK, AND TWO DEPENDENCY HAZARDS

The param-dumps were wired and run once, serially, in an isolated build directory
(`build/` untouched). Both arms compile-verified per plugin. O-Bowed's existing
`OUARICON_BUILD_TESTS` block was extended rather than a second option declared;
O-TextureForge needed **six** guard sites rather than one, because four
`dynamic_cast<TextureForgeEditor*>` call sites sit above `createEditor()`.

**Two latent FetchContent hazards, both in O-TextureForge's chain, neither
fixed:**

1. The root `CMakeLists.txt:5` caches `CMAKE_OSX_DEPLOYMENT_TARGET` at **10.13**,
   and O-TextureForge cannot compile at that target — `_deps/knncolle-src` needs
   `std::filesystem` (10.15+). The committed `build/` only works because its
   cache says 11.0, set at some point and never written down. A fresh clone or a
   CI runner hits this.
2. `build/_deps/subpar-src/extern/CMakeLists.txt:4-9` declares **`sanisizer` with
   `GIT_TAG master`** — a moving branch. Upstream rewrote a commit; local and
   `origin/master` diverged 1↔1, so every CMake *regenerate* runs
   `git pull --rebase`, replays the local copy onto the upstream rewrite of
   itself, and conflicts. Any `CMakeLists.txt` change triggers a regenerate —
   which was all eight executors. Resolved in the **derived** directory only; the
   entire delta between the two SHAs is one added comment line.

## Decision items added

6. `sanisizer` pinned to a moving branch (above).
7. O-TextureForge's three blank readouts.
8. O-Reed's dead `instrumentPreset` and oversampling-blind latency.
9. The unreachable post-flip re-clamp across eleven plugins — per-plugin page
   code, so an eleven-file sweep.
10. O-AnalogEQ's two Q toggles clip their own `TIGHT` option **in English**;
    `.three-way-option` is `flex: 1` without `min-width: 0`, so three items sit
    at min-content (109.87 px in a 108 px box). Carried from v1.2.0, invisible to
    both gates.
11. **Keyboard reach is partial on most of this batch by design** — 11 of 38 on
    O-GrainScatter, 3 of 14 on O-TextureForge, 2 of 13 on O-AnalogEQ. Knob cells
    are pointer-drag `<div>`s with no `tabindex`. The largest open accessibility
    question the stage has surfaced.
12. **Checkpoint 5 is further out, not closer.** M2 adds **183** machine-drafted
    French entries to a worklist no native speaker has begun.

## Not verified

- **Checkpoint 5 outstanding on all 43.** No human has seen any French UI; all
  **3471** French entries repo-wide are machine drafts, every one
  `reviewed: false`.
- **No DAW test on any of the eight.** `auval` and the headless harness only.
- **The Standalone `.app` is stale everywhere** — `build-and-install.sh` builds
  VST3 + AU only.
- **Rendering was verified in headless Chromium**, never inside a real WKWebView.
- **Checkpoint 4** (host session save/reopen) reasoned, not executed.
- **Windows/WebView2 font metrics** — the standing hardware-blocked deferral, and
  **tooltips are new surface for it**: French wraps taller inside a fixed
  `max-width`, so a wider face shows there first.
- O-TextureForge's ONNX/UMAP/corpus path was neither touched nor exercised.

## What M3 is

**Four plugins: O-Bells, O-Formant, O-Prism, O-Wind.** O-Prism is 173 parameters
on its own — larger than M1 and M2 combined — and O-Formant carries 5
empty-body `I18N` entries under the same K4 rule that governed
O-MicrotonalSampler's 51. Param-dumps are wired for none of the four.

---

# STAGE M — T17, BATCH M3: the last four — M3 COMPLETE, 4 of 4, and STAGE M IS DONE

Four plugins, one commit each, two orchestrator commits around them. **Stage M is
finished: all 22 bare plugins now have hover-help in both languages.**

| | |
|---|---|
| Plugins | **4 of 4** |
| Parameters, dumped | **358** — more than M1 and M2 combined (249) |
| Parameters with a control | **275**; 17 are host-reachable and page-unreachable, 65 arrive too late to bind |
| Tips authored and bound | **281** = 274 parameter + 8 chrome, less double-bindings |
| Unreviewed French | 3471 → **3751** (+749 entries across the four) |
| Zero-tip plugins | 4 → **0** |
| `check-i18n` | **43/43 ALL PASS**, all canon v2 |
| `boot-all-uis` | **43/43 clean**, 0 warn, 0 failed, native `title=` **0** repo-wide |
| New per-plugin gates | 4 × `tests/ui_tip_render_check.js`, **5338 assertions, 0 failures** |

| Plugin | Version | Commit | Params → tips | Gate | Latch control |
|---|---|---|---|---|---|
| O-Wind | 1.18.0 | `8c6e1c97` | 56 → **50**+2 | 774 | 5280 px² |
| O-Bells | 4.3.0 | `31fcd0cf` | 65 → **63**+2 | 1024 | 5394 px² |
| O-Formant | 1.27.0 | `6c898178` | 64 → **55**+2 | 1360 | 7114 px² |
| O-Prism | 1.22.0 | `db939f7f` | 173 → **105**+2 | 2180 | 3319 px² |

**Stage M total, M1+M2+M3: 22 plugins, 607 parameters, 556 tips, 8762 render-gate
assertions, 0 failures.**

## THE HEADLINE: the gate that was supposed to catch a dead binding never could

The M brief told 22 executors that `boot-all-uis` *"is the only gate that sees a
swallowed binding failure"* and that *"a console warning there is a real failure,
not noise."* **Both halves are false.**

- `scripts/i18n-canon.js:165` warns with `console.warn`
- `scripts/boot-all-uis.js:141` — `if (m.type() !== 'error') return;`

The warning is dropped before it is examined. So a `TIP_BINDINGS` row resolving to
nothing is invisible to `check-i18n` (static), invisible to `check-ui-labels` (no
tooltip awareness), and invisible to `boot-all-uis`. **No gate in this repo sees a
dangling tip binding at runtime.**

Found by O-Bells' executor and verified at both line numbers. It means the
per-plugin render gate's assertion [1] — *every `TIP_BINDINGS` selector resolves,
as a hard FAIL* — is the only thing standing between this stage and 556 silently
dead bindings across 22 plugins. **That assertion is now load-bearing for the
entire T17 feature.** Recorded as decision item 17; `scripts/` is the
orchestrator's.

This is the eighth instance in this task of a gate certifying the absence of a
thing it cannot see, and the first one that was written into the standing brief
itself.

## THE SECOND HEADLINE: a repo-wide gate count is not a batch verdict

O-Wind finished first, ran `grep -cE '\[2\] 0 tip\(s\) bound'`, got **0**, and
reported *"M3's exit condition is met — all four landed."* **Only O-Wind had
committed.** The other three had authored `i18n.js` into the shared working tree
and had not yet reached their commit step.

Every gate in `scripts/` reads the filesystem, so in a trunk-based checkout with
concurrent executors **a repo-wide result describes the union of everybody's
uncommitted work.** No executor can separate its own contribution from its
neighbours'. Only `git log` can close a batch. First cross-session instance of
this failure mode in the task.

## The orchestrator's pre-dispatch measurements were wrong in five places

All five were caught because every dispatch said *"confirm each, do not take them
on faith"* and every executor did.

**A grep of parameter IDs over a served root cannot establish reachability, and it
errs in both directions.** False negatives where a control is reached through a
native-function **alias** and the ID never appears — O-Wind's `referencePitch`,
O-Bells' `tuning_masterTune` and `tuning_octaveStretch`, all three routed through
`get/setMasterTune`-style bridges. False positives where a matched string is a CSS
class family or a `getSliderState` argument rather than an anchor — O-Prism's
`tonic` and `tuningPreset`.

The claimed 16 page-unreachable became **14**; O-Prism's claimed 107/64/2 split
became **105/65/3**. **The running 26 figure across M1–M3 was produced by this same
scan and has never been audited by its own critics** — treat it as an upper bound
(decision item 20).

Also mismeasured: O-Wind has **3** tab panels, not 1 (21 of its 52 anchors sit on
a hidden Effects tab); O-Formant's page has **1** inline module, not 3 (the 3 is
`check-i18n`'s module count); O-Prism has **64** JS-expanded knobs, not 61.

## The mechanism for a late-mounting anchor already exists, on 1 plugin in 43

O-Bells bound two anchors inside a lazily-`import()`ed panel that O-Reed and
O-Formant could not. The entire difference is one line, declared outside the
byte-compared canon region:

```js
window.__reapplyI18n = () => applyI18n(uiLanguage);   // index.html:1992
```

called by the panel's own init after it mounts (`:3066`). `grep -rl` across all 43
served roots returns **O-Bells and nothing else**. It is not `localizeSubtree`,
which loops labels and aria attributes and **writes no tip attributes**.

So the open canon question from M2 is closed: the thing to decide is only whether
to roll this out, not what to build (decision item 18).

**O-Prism is the exception that needs more.** Its 64 mod-matrix rows arrive after
`applyI18n()` *and* carry no per-parameter id even once they arrive — the columns
are `.mod-col-src` / `.mod-col-dst` / `.mod-col-amt` / `.mod-col-on` inside
`#mod-row-<i>`. `__reapplyI18n` alone would have nothing to name. Its executor
shipped the 105 statically-anchored tips, reported the 65, and did not invent a
mechanism — and its gate asserts the evidence rather than asserting about it:
`[8]` counts `[id^="modSlot"]` and requires 0, so the page cannot quietly grow
them.

## Five shipped defects found while writing copy — the stage's highest yield

M1 found none, M2 found three, M3 found five plus one that was fixed.

- **O-Wind `toneHoleToggle` is dead** — the DSP was never implemented
  (`PluginProcessor.cpp:316-319`). The switch and its automation lane move;
  nothing is heard.
- **Inverted bypass buttons on two plugins** — O-Wind's four and O-Prism's five
  read the inverse of their parameter, found independently.
- **O-Formant: three knobs wired to relays that do not exist.** `js/main.js:288-290`
  asks for `consonantAttackSlider`/`Hold`/`Decay`; the editor declares 54 relays and
  none of those. The parameters are live and all 16 consonant presets set them; the
  controls are dead.
- **O-Formant: a session saved at A4 = 442 Hz reopens at 440.** The panel's knobs
  write the `TuningEngine` directly and never the parameter, and
  `getStateInformation()` does not save them.
- **O-Prism: two dead `bindKnob()` calls** looking for elements that do not exist.

**FIXED, because it was this feature's own foundation: O-Formant's language
persistence had been dead since v1.26.0.** `js/main.js` imported named bindings
and no namespace, so `Juce` was unbound, `initI18n()` threw `ReferenceError`, and
its own `try/catch` degraded the feature to session-only behind a `console.warn`
**that no gate fails on**. One added `import * as Juce`
(`critical_juce_webview_namespace_vs_postmessage`) — the third
gate-blind-to-a-warning finding in this batch alone.

Every one went into a tooltip body rather than being described as intended.

## Three sharper rules for a negative control

- **A control that samples STATE cannot see a transition that begins and ends
  inside one task.** O-Wind's boundary guard passed 774/774 with the guard deleted
  under a post-hoc DOM read *and* under a per-frame opacity sampler; only a
  `MutationObserver` on the class attribute saw it (10 mutations, 5 hides).
- **The release must be asserted, and now it is proven.** O-Prism deleted the
  `pointerup`/`pointercancel` arm of its drag guard: **1290 FAIL / 890 pass** — the
  flag latches on the first click and no tip opens again.
- **A positive control on the clamp has a narrow window.** O-Prism's first draft
  landed an 8481 px tip, "section 6's probe wearing 6b's name"; the real one is
  1625 chars / 585.7 px.

Every executor ran the blur 2×2 rather than assuming: **load-bearing on O-Wind,
O-Bells and O-Prism; decoration on O-Formant** (an accident of section order,
kept and labelled). The static-grep half is confirmed on ten plugins now —
deleting only `if (lastInputWasPointer) return;` leaves every
`grep -c lastInputWasPointer` green.

## Carried traps

- **A hover harness that scrolls on viewport visibility is blind to a
  scroll-container clip.** O-Formant: a previous anchor's scroll left `.right-col`
  mid-way, the pointer landed on the tab bar, and the surface was measured still
  carrying the previous anchor's text — **84 failures that look exactly like a copy
  bug**. Scroll unconditionally and assert `elementFromPoint` before every hover.
- **A second hover surface can already be on the page.** O-Bells' `.hi-fi-toggle`
  carried its own `:hover` `.toggle-tooltip` at `z-index: 100`. Deleted, its
  sentence moved verbatim into the new tip body. Contract §4 deletes native
  `title=` and says nothing about a hand-rolled CSS tooltip.
- **The plant must be searched for on a large frame.** The habitual 40× plant fit
  and reported nothing on O-Formant (640 chars → 248 px in 600) and O-Wind.
- **`--amend` was unsafe again** — O-Bells' parent is O-Wind's M3 commit, which
  landed between its status check and its commit. Third batch, third time.

## "Bind to the ids the UI already uses" — final score, 2 of 18

O-Prism joins O-Freeze as the only plugins where the naive reading of T17 holds on
both halves: 107 of 107 anchors are ids (the page has no `data-param` at all), and
for 81 of 107 the id *is* the cell. They sit at opposite ends of the size range,
which is the point — it is a property of how the page was built, not of how big it
is.

## Decision items added

17. No gate sees a dangling tip binding — the T17 feature rests on it.
18. Generalise `__reapplyI18n` (supersedes M2's item 14 as an open question).
19. O-Prism's mod matrix needs addressable row nodes as well.
20. The 26 host-reachable / page-unreachable figure is unaudited.
21. Two plugins ship inverted bypass buttons.
22. O-Formant's A4 tuning is lost on session reopen.
23. `scripts/serve-ui.js:320` seeds the stub on the wrong key for O-Formant.
24. `check-ui-labels.js:1022` prints a union count against a single-state snapshot.
25. Checkpoint 5 is now 3751 entries; M3 added 749.

## Not verified

- **Checkpoint 5 outstanding on all 43.** No human has seen any French UI; all
  **3751** French entries repo-wide are machine drafts, every one
  `reviewed: false`. O-Prism's 262 is the largest single block in the repo.
- **No DAW test on any of the four.** `auval` and the headless harness only.
- **The Standalone `.app` is stale everywhere** — `build-and-install.sh` builds
  VST3 + AU only.
- **Rendering was verified in headless Chromium**, never inside a real WKWebView.
- **Checkpoint 4** (host session save/reopen) reasoned, not executed — and
  O-Formant's A4 defect is now a live reason to run it.
- **Windows/WebView2 font metrics** — the standing hardware-blocked deferral, and
  tooltips are new surface for it: French wraps taller inside the fixed cap on all
  four (O-Prism 123.9 → 139.3 px), so a wider face shows there first.
- **O-Prism's 65 mod-matrix parameters have no hover-help** and will not until
  decision items 18 and 19 are answered.

---

# DECISION ITEMS 17 AND 18 — the two `scripts/`-owned fixes M3 surfaced, both closed

Taken up directly after Stage M closed, as orchestrator work. Two commits, no
plugin version bumps, no DAW test needed — neither change is user-observable.

| | |
|---|---|
| Item 17 | `56cdbb37` — `scripts/boot-all-uis.js`, 1 file |
| Item 18 | `cec3f857` — `scripts/i18n-canon.js` + 43 plugin files, +57 / −28 |
| `check-i18n --strict-v2` | **43/43 canon v2, ALL CHECKS PASS** |
| `boot-all-uis` | **43/43 clean**, census byte-identical across the change |
| New signal | **1 DEAD tip binding repo-wide, 19 late** — the first time either number has existed |

## ITEM 17 — the gate can see a dangling binding now, and it found one on the first run

The M brief's claim that `boot-all-uis` sees a swallowed binding failure was
false in both halves, and the fix is one ordering change: read the canon's WARN
diagnostics **before** the `type() !== 'error'` filter rather than after it.

**The part that makes the census worth reading is the second step, not the
first.** A captured warning alone would have reported O-Bells' two
lazily-mounted tuning-panel anchors as defects, and O-IntonationPad's
seventeen — nineteen false positives, on a page where every one of them binds
correctly a moment later. So each missed selector is **re-queried after the
settle**: resolves now → LATE, still missing → DEAD.

Reported as its own verdict block, deliberately not folded into `clean: 43/43`.
The boot verdict answers *does the page load*, and a dead binding loads fine;
one number meaning both is the conflation that hid this for the whole stage.

`--strict-tips` opts into exit 2 on a DEAD binding. The default stays exit-0 —
this file reports, and the repo already knows what a permanently-red gate does
to the habit of reading gates.

### The census, first run, 43 plugins

**1 DEAD.** `O-simpleSampler/js/i18n.js:748` binds
`.tour-btn[data-preset="Filtered &amp; Enveloped"]`. `index.html:51` writes that
entity, so the HTML parser decodes it and the **DOM attribute value is
`Filtered & Enveloped`** — the JS selector carries `&amp;` literally and matches
nothing. Six of the seven tour buttons have hover-help; the seventh never has,
and nothing in the repo could say so. **Not fixed here — it is plugin-owned and
does not belong in a `scripts/` commit.** New decision item 26.

**19 late**, all correct: O-Bells 2, O-IntonationPad 17.

### The control that caught the fix being decoration

The first draft of the capture regex was `/^i18n: (tip target not found|missing
label key) /` — a space where the canon writes a colon. It matched nothing and
reported **0 for O-Bells**, a page documented to warn exactly twice. Running the
known-positive case first is the only reason that was caught rather than
committed as a green vacuous census.

Four controls in total: O-Bells → 2 late / 0 dead; a planted
`#planted-selector-that-does-not-exist` on that same page → 1 DEAD **and still 2
late**, which is the discriminator working on one page; `--strict-tips` → exit 2
on the dead plugin, exit 0 on late-only O-Bells.

## ITEM 18 — three authors reached around the same gate, so the canon took it

`grep -rl __reapplyI18n` returns **three** plugins, not the one M3 reported:
O-Bells (`index.html:1992`), **O-Marimba** and **O-IntonationPad**
(`js/app.js`), the latter two from Stage J. M3's "1 plugin in 43" was measured
over a narrower set of roots.

That correction is the whole argument for the item. Each of the three declared
the line **outside** the byte-compared region, and each left a comment saying
the region *"may not gain a line."* Three independent authors hitting the same
wall and working around it the same way is the canon's job, not theirs.

So the line moved into the canon beside the `window.__setLanguage` /
`window.__setLabel` it belongs with, all 43 copies synced, and the three
out-of-canon declarations deleted along with their now-false comments. Call
sites untouched — they keep their plugin-specific rationale.

**No call sites were invented.** Which panels should re-sweep is per-plugin
work, and O-Prism needs more than this line regardless: its 64 mod-matrix rows
carry no per-parameter id even once they mount (item 19, still open).

### The vacuous probe, and the rerun that wasn't

`__reapplyI18n` present and callable on 43/43 — necessary, not sufficient. The
second arm asked whether a re-sweep **preserves the language**, which is the
property the naive `applyI18n('en')` implementation fails, and the first draft
sampled an **English** page, where `before === after` is true no matter what the
function does.

Rerun on a page switched to French: **43/43 held French.** Substituting the
naive `window.__setLanguage('en')` into the same probe: **0/43**, reporting each
reset by name (`O-AnalogEQ: "SAUVER" → "SAVE"`). The probe can fail, so its pass
means something.

### Why the 43-file sync is not 43 version bumps

Nothing user-observable changed on any plugin: the boot census is byte-identical
across the change, the French totals are unchanged at 3751, and 43/43 pages
render the same. The sync is *forced by the drift gate* rather than chosen per
plugin — leaving 42 plugins off-canon was never an option. Precedent is thin:
`96b5f2eb` (retire canon v1) is the only prior canon change and it was
gate-side only, touching no plugin file.

### The control that proves 43/43 is not vacuous

Deleting the line from **O-Chorus alone** turns assertion 6 red by name —
`"[6] the applyI18n/initI18n region matches scripts/i18n-canon.js — does NOT
match"`, `OFF CANON 1 O-Chorus`. Restored, tree clean, 43/43 green again.

The runtime control is the sharper one: because the three old declarations were
**deleted**, a broken sync would have turned O-Bells' 2 and O-IntonationPad's 17
late bindings into DEAD. They are still late. That is what proves the canon copy
is the one now in use, rather than a leftover.

## Decision items

- **17 — CLOSED** (`56cdbb37`).
- **18 — CLOSED** as a mechanism (`cec3f857`). The *rollout* question it
  superseded from item 14 is now genuinely open and per-plugin: O-Reed's
  `referencePitch`, O-Formant's tuning panel and O-Wind's panel each need a call
  site, and each is an `/improve` with a version bump.
- **19 — unchanged.** O-Prism's mod matrix still needs addressable row nodes.
- **26 — NEW.** O-simpleSampler's seventh tour button has never had hover-help;
  an HTML entity leaked into a CSS attribute selector. One-line fix, plugin-owned.

## Not verified

- **Checkpoint 5 still outstanding on all 43** — 3751 French entries, unchanged
  by either commit, none read by a native speaker.
- **No DAW test**, and none warranted: neither change is user-observable.
- **Headless Chromium only**, never a real WKWebView — so "43/43 present and
  callable" is a Chromium claim.
- **The 21 plugins with no `ui_tip_render_check.js`** are now covered for the
  dangling-binding question by `boot-all-uis`, and by nothing else.
- **`--strict-tips` is not wired into any CI or script.** It exists; nothing
  calls it. It will report red on O-simpleSampler until item 26 is fixed.

# ITEM 26 — CLOSED, and the post-M baseline (2026-08-31)

**26 — CLOSED** (`242983a4`, O-simpleSampler v1.4.1). One character class in one
selector: `js/i18n.js:748` bound `.tour-btn[data-preset="Filtered &amp; Enveloped"]`,
the entity copied verbatim out of the markup; the DOM attribute is the DECODED
`Filtered & Enveloped`. The fix is the decoded string. `boot-all-uis` now reports
**0 DEAD** across 43, so `--strict-tips` would exit 0 repo-wide for the first time.

The extractor artifacts (`i18n-inventory.tsv`, `i18n-index-draft.html`,
`i18n-labels-skeleton.js`) for the 36 plugins left untracked through Stages J–M
were committed in `aaab5318`, so every shipped `i18n.js` now has its input in the
tree.

## Baseline, re-run from a clean tree

- `check-i18n --strict-v2`: **ALL CHECKS PASS — 43 localized plugins**, canon v2 43 / v1 0.
- `boot-all-uis`: **clean 43/43**, warn 0, failed 0; **DEAD 0**; late 19 (O-Bells 2,
  O-IntonationPad 17 — all bind after the settle); rendered text-bearing elements
  3787, aria-label 771, **native `title=` 0**.
- French entries: **3751 / 3751 `reviewed: false`** — Checkpoint 5 has not begun.

## What is still open, sorted by who owns it

| Owner | Item |
|---|---|
| a French reader | **Checkpoint 5** — 3751 machine-drafted entries, none vetted |
| canon (`scripts/i18n-canon.js`, 43-file sync) | `<html lang="en">` is hard-coded on all 43 and never follows `#lang-select` |
| shared module `scala-tuning-engine` | item-18 rollout — `referencePitch` on O-Reed / O-Wind is `#ref-pitch-knob` inside the module's `tuning-panel.js`, which has no `__reapplyI18n` call site (O-Bells and O-IntonationPad carry their own copies with one) |
| O-Formant | item-18 rollout — its tuning panel; also item 22 (A4 lost on reopen) |
| O-Prism | item 19 — 64 mod-matrix params need addressable row nodes before they can carry tips |
| suite-wide, separate pass | item 2 (hover-help toggle on 2 of 43), item 11 (keyboard reach on pointer-drag knobs) |
| not i18n | items 5–10, 20–24 |

# STAGE N — Checkpoint 5 as a QA pass — N0 COMPLETE (repo level), N1 pilot dispatched

**The developer chose the direction on 2026-08-31:** with all thirteen stages done and
no native speaker scheduled, Checkpoint 5 becomes a second reading of all 3751 French
entries by executors, against a glossary and a lint, one plugin per executor, patch bump
each. `reviewed: false` stays `false` — the flag means a native speaker, and the header of
each file records this pass instead.

## THE HEADLINE: 43 authors, 267 terms, up to nine French words each

Before anyone was dispatched, every plugin's `i18n.js` was imported (5078 rows, 43 plugins)
and the French grouped by its English source. **267 English label strings had more than
one French rendering across the suite.** "Off" had nine (Arrêt, ARRÊT, Non, NON, Aucun,
désactivé, Désact., DÉS., Désactivée); "Save" seven; "Release" seven; "Feedback" seven;
"Load" six; "Mix" five (Dosage, Mixage, Mix, DOSAGE, Mélange). Typography split the same
way: 1559 typographic apostrophes against 550 straight (eleven plugins straight, thirty-two
typographic); 6 no-break spaces before a colon against 332 plain ones; 15 before `%`
against 233.

Each rendering was one author's defensible choice on one page. **Dispatching 43 reviewers
without a shared list would have widened it** — so the list came first.
`scripts/i18n-fr-glossary.js` settles ~230 terms (root first, width-pinned abbreviations
after; the prose companion `260826-ieq-FR-GLOSSARY.md` gives the reasoning), and
`scripts/i18n-fr-lint.js` enforces them alongside seven typography rules, casing parity
and forbidden words. Report by default, `--strict` exits 2 — the `--strict-tips` shape,
because on the day it was written 43 of 43 failed it.

## The lint's first zero was a lie, and the corpus scan is what caught it

The first baseline reported **T4 (colon) = 0**. The corpus scan, written independently an
hour earlier, had counted 332 plain spaces before a colon. The T4 and T5 regexes matched a
non-space character immediately before the mark — the *no space at all* case — and let
`Plage : ` with an ordinary space through, which is the case French typography is about.
Fixed; baseline **1517 → 2145** (T4 311, T5 325). The scan that disagreed with the gate is
the only reason the gate was not committed reporting a clean column.

## `<html lang>` follows the selector — decision item 4 of the M brief, closed

`applyI18n()` now sets `document.documentElement.lang = uiLanguage` (`ee912c59`). Same
shape as `cec3f857`: one canon line, synced verbatim into all 43 canon-bearing files, no
version bumps — the user-visible half ships with each plugin's Stage N release and its
CHANGELOG. Controls: `check-i18n` 43/43 canon v2 (the sync is byte-equivalent),
`boot-all-uis` 43/43 clean / 0 DEAD / 19 late (unchanged), a runtime probe reading `lang`
on every page — `en` after init, `fr` after `__setLanguage('fr')`, still `fr` after
`__reapplyI18n()`, `en` after switching back, **43/43**, zero page errors — and a negative
control (a setter that leaves `lang` alone) that fails the same probe on O-Comp and O-Bells.

## Baseline, per plugin (lint findings)

O-AnalogSaturation 7 · O-Bass 13 · O-Texture 13 · O-Polystutter 15 · O-Tremolo 16 ·
O-AnalogEQ 20 · O-Tapestop 20 · O-FreqPulse 25 · O-Gain 25 · O-SpectralShaper 25 ·
O-Bassoon 26 · O-TextureForge 27 · O-Bowed 28 · O-Emulator 29 · O-ReverseDelay 29 ·
O-Octagon 30 · O-MultiBandCompressor 31 · O-Orbit 32 · O-Marimba 33 · O-Comp 39 ·
O-Freeze 41 · O-SimpleReverb 42 · O-Chorus 43 · O-DigiDelay 43 · O-simpleBeatmaker 44 ·
O-Bitrot 45 · O-Lyrica 49 · O-simplePhysicalModelSynth 50 · O-Reed 54 ·
O-simpleSubtractive 56 · O-simpleAdditive 62 · O-simpleFM 63 · O-Contrabass 67 ·
O-GrainScatter 68 · O-Detune 70 · O-simpleSampler 71 · O-IntonationPad 76 · O-Wind 76 ·
O-Formant 77 · O-MicrotonalSampler 77 · O-simpleGrain 92 · O-Bells 134 · O-Prism 262.
**Total 2145.** By code: T7 unit 427, G1 glossary 347, T5 `;!?` 325, T1 apostrophe 311,
T4 colon 311, F1 forbidden 203, T3 `%` 177, C1 casing 26, T6 minus 10, T2 decimal 8.
218 `sameAsEn` entries listed as INFO for the reviewers to confirm or translate.

## Commits

- `47e8163f` docs — the record reconciled (A–M complete, 17/18/26 closed).
- `ee912c59` feat(i18n) — `<html lang>` canon line, 44 files.
- `75ef8254` feat(i18n) — glossary + lint.
- `f1927cd4` docs — Stage N brief + glossary companion; decision items 27–29 added.

## Decision items added

27. `reviewed: false` after Stage N — dead weight or exactly right, the developer's call.
28. Pages that uppercase with CSS — the table's casing is invisible on screen, visible to
    assistive technology and the lint.
29. The lint is a report; `--strict` exists and nothing calls it.

## N1 pilot — dispatched

O-Comp (39), O-Chorus (43, the casing case), O-simpleFM (63, the O-simple* family with
26 straight apostrophes and Stage K width pins), three executors concurrently, builds on
the Stage K mutex. Results appended below as they land.

## Not verified

- No French entry has yet been changed by this stage; the N0 numbers are a baseline.
- The glossary's choices are the orchestrator's, from the corpus and standard French DAW
  vocabulary — not a native speaker's. The pilots are the first test of whether the
  settled terms fit the frames they were pinned on.
- `<html lang>` verified headless in Chromium only, never in a WKWebView.

# STAGE N — BATCH N1 COMPLETE, 3 of 3 — and three lint defects the pilots found

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-Comp | 1.7.1 | `55a81c59` (+`c080de9c` comment) | 39 → 0 | 16 of 34 fr entries | moved 0 → 0 | 156/156 |
| O-Chorus | 1.4.1 | `cbbda46b` (+`bba4626b` comment) | 43 → 0 | 19 of 28 | 0 → 0 | 240/240 |
| O-simpleFM | 1.3.1 | `6bb4cf32` (+`fbffc088` fixture, `f8494bae`) | 63 → 0* | 41 of 107 rows | 0 → 0 | scratchpad probe, 336 |

\* 63 → 2 at commit; → 0 once `6eb042c8` landed, with no plugin change.

## THE HEADLINE: the pilots found three defects in the lint, and none in the glossary

- **`termNote` guarded G1 only** — an exempted entry was printed EXEMPT and counted as F1
  in the same run (O-simpleFM, `label.knobFixedHz`). Fixed `6eb042c8`.
- **A glossary-accepted rendering could draw F1** — `écart` matched inside *Écart total*,
  the glossary's own settled term for *Total span*; O-Chorus found it by reading the two
  tables against each other, on a plugin that could not hit it, before N8 could. Fixed
  `daed4a2e`; 2145 → 1991, ~25 of the delta this false positive suite-wide.
- **T7 caught the wrong space and not the missing one** (`440Hz` passed clean), and `%`
  reported twice. O-Comp proved it with a five-string control. Fixed `8a387f1c`;
  1991 → 1837, the whole delta the `%` double-count.

The glossary itself needed one addition: `porteuse nulle` as the short form of *carrier
null* (160.7 px vs a 102 px badge). **No settled term was wrong.**

## Two of three header geometry defences were false, measured

O-Comp's v1.7.0 header defended `Sauver` on width against a 30 px content box; `Enreg.`
is **23.75 px against Sauver's 25.00** — narrower. O-Chorus's `Relâch.` is **36.80 against
Relâche 38.94 and English Release 37.70**. The glossary's abbreviation fit where the
header said the calque was the tight one. **`Relâchement` had two verdicts on two pages**:
62.92 px widens O-Comp's 52 px `.control-group`, and fits O-simpleFM's 56 px envelope cell
at 77.3 px nowrap with 15.5 px clearance — one term, measured twice, two right answers.

## Defects found by reading French, in the English

- **O-simpleFM's `Ratio C:M` is inverted.** `FMVoice.h:210` computes `fm = carrierHz *
  ratio` and the harness puts the carrier at the played note (`a440 = 0.6299`), so the knob
  is modulator-to-carrier: at 2.00 the modulator runs at twice the carrier, C:M = 1:2. The
  caption, tip title, automation-lane name and `FactoryPresets.cpp` comments are backwards;
  the tip *body* and the Clarinet lesson are correct. Not fixed — English, and host-visible.
  **Decision item 30.**
- **O-Comp's gear button had two French names** — *Réglages* in the tip, *Paramètres* in
  `aria-label`. Fixed (French only).
- O-simpleFM's render-harness `CMakeLists.txt` hard-coded `VersionCode=0x10202` (1.2.2,
  three releases stale) beside a correctly interpolated `VersionString`. Fixed as a version
  site, DSP-neutral by measurement.

## Meaning changes — the category the lint cannot see

O-Comp 3 (*la transitoire* → *le transitoire*; a release time does not pump, the compressor
does; *boutons* → *boutons rotatifs* on a page with both). O-simpleFM 6 (*accordé* "in
tune" → *à hauteur définie* "pitched", four bodies; the Tubular tip named a control the page
does not show; the Clang tip said the sidebands were non-integer when it is their multiples
that are). O-Chorus 0 — the smallest table, and every claim held.

## Casing under CSS — item 28 has its numbers

O-Chorus's captions all carry `text-transform: uppercase`; `Vitesse` and `VITESSE` measure
41.61 px each, eight for eight identical to the hundredth. Lower-cased anyway: the lint and
the accessible name read the table, and a screen reader handed `VITESSE` may spell it.

## Decision items added

30. O-simpleFM `Ratio C:M` inverted in English (caption, title, automation lane, presets).
31. **21 plugins have no committed hover-help render gate** (Stage I/J predate
    `ui_tip_render_check.js`). O-simpleFM proved the gap: a planted wrong French body passed
    281/281 of its probe until a language-difference arm was added. Scratchpad probes carry
    Stage N; nothing committed catches a French tip regression on those 21.
32. French label-in-name (WCAG 2.5.3) holds only by stem where the caption is an
    abbreviation (`Enreg.` ⊂ `Enregistrer le préréglage`) — O-Comp Save, and every plugin
    that abbreviates.
33. Stale width tables in `index.html`/`styles.css` comments after a caption change — two
    comment-only follow-up commits in N1; expect one per plugin whose header duplicates its
    measurements into CSS.

## Not verified

- **No native speaker.** 169 entries stay `reviewed: false` across the three.
- Headless Chromium at the shipping frame; `auval`; no DAW, no WKWebView, Standalone stale.
- O-simpleFM's canvas-free page had a scratchpad probe only — nothing committed (item 31).
- O-Comp's three `fillText` strings are seen by no gate; the U+00A0 was verified by
  `measureText` (2.75 px, identical to the ASCII space; a U+FFFF probe at 7.944 proves tofu
  would have been visible) and by grep in the binary. Nobody has seen it rendered.
- Repo lint total after N1 and the three fixes: **1835** (from 2145).

# STAGE N — BATCH N2 COMPLETE, 6 of 6

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-AnalogSaturation | 1.3.1 | `d793390e` | 6 → 0 | 5 of 15 | 0 → 0 | 31/31 |
| O-Bass | 1.5.1 | `506e6541` (+`86dae51d`) | 12 → 0 | 9 of 24 | 0 → 0 | 125/125 |
| O-Texture | 0.3.1 | `8fad691a` | 13 → 0 | 10 of 26 | 0 → 0 | 208/208 |
| O-Polystutter | 1.14.1 | `911a516e` (+`638c9a79`) | 15 → 0 | 15 of 133 rows | 0 → 0 | probe 34/34, 105 anchors |
| O-Tremolo | 1.8.1 | `d0bc5840` (+`5fe568b6`) | 14 → 0 | 11 of 29 | 0 → 0 | 186/186 |
| O-AnalogEQ | 1.3.1 | `04ac4aaf` (+`32759a48`) | 20 → 0 | 14 of 43 | 0 → 0 | 308/308 |

Nine plugins done. Repo lint total **1835 → 1755 → (after N2) see below**; `boot-all-uis`
43/43 / 0 DEAD on every run; `auval` PASS on all six.

## The brief was wrong about `sameAsEn`

It said `termNote` was the only key an executor would add. O-Texture applied the glossary's
*Mix* to a caption that had read *Mélange*, the French became a straight copy of the
English, and `check-i18n` assertion 4 hard-failed until `sameAsEn: true` was added. That
key is the existing, correct declaration — the brief now says so (correction 15), and the
lint's straight-copy census counts the CONDITION (`fr === en`: 327 suite-wide, 219
flagged) instead of the flag, which had printed 0 on a page that had one (`a43233dd`,
`b43f763f`).

## Glossary growth, all from measurements

`enr` for *save* (O-Bass, a 28 px content box); `sync tempo` and a `pan sync` row
(O-Tremolo — the lint had flagged exactly half of a matched pair). No settled term was
wrong; two were declined in BODIES for context and recorded in headers rather than
exempted (*gain d'entrée* on a plugin named for saturation; *filtre en bascule* for a tilt
filter that is described, not captioned).

## Third header defence proven backwards; two held

O-AnalogEQ's ENREG. is 3.75 px narrower than the SAUVER its header defended. O-Bass's and
O-Tremolo's headers re-measured to the hundredth. Running score: 3 of 9 headers wrong about
the string they defended, 0 of 9 wrong about the numbers they recorded.

## Meaning defects the lint cannot see — the category that justifies the stage

Omissions: O-Texture restored a closing range the draft had dropped; O-AnalogSaturation
restored DIODE's "harder". Wrong claims: *un clic à côté* (next to) for "a click
elsewhere"; *pendant ce temps* (meanwhile) for a condition; *replier du repliement*
(folding the folding); *discret* (unobtrusive) for "quiet"; *séparer … entre* (calque);
*sous les boutons* on pages that have both buttons and knobs. **Twelve meaning fixes across
six plugins**, zero of them visible to any gate.

## English defects found by reading French

- **O-Polystutter `midi` body**: says C1–B1 trigger lanes 1–4 and any other note triggers
  all; `TriggerRouter.cpp:76-85` routes notes 60–63 and only 67 triggers all. The source
  comment is wrong too. Item 34.
- **O-Tremolo `tip.panSync`**: says a stereo *signal* is needed; the gate is on the bus
  (`PluginProcessor.cpp:345` duplicates mono into channel 1 on a 1→2 bus). Item 35.
- O-Bass's `.meter-label` 24 px guard, declared decorative at v1.4.0, is load-bearing now
  (planting its removal fails assertion 7). Comment corrected.

## Decision items added

34. O-Polystutter MIDI tooltip states the wrong notes in both languages.
35. O-Tremolo Pan Sync tooltip states a signal condition where the code has a bus condition.

## Not verified

No native speaker; no DAW; Chromium only; the Standalone stale on all six. O-Polystutter
has no committed render gate (item 31 — now 2 of 9 in this stage).

# STAGE N — BATCH N3 COMPLETE, 6 of 6 — 15 of 43

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-Tapestop | 1.6.1 | `a51c7468` | 20 → 0 | 21 of 109 rows | 0 → 0 | clamp gate 35/13/4 both languages, unchanged |
| O-FreqPulse | 1.18.1 | `1d16dd0d` (+`6a6d6894`) | 21 → 0 | 19 of 67 | 0 → 0 | probe 1766 assertions, 56 bindings |
| O-Gain | 1.3.1 | `efd92f51` (+`e30017c1`, `6f81d0ea`) | 25 → 0 | 19 of 63 | 0 → 0 | probe 52 tips, heights unchanged |
| O-SpectralShaper | 1.7.1 | `3e7631aa` (+`4da9bd6a`) | 24 → 0 | 21 of 71 | 0 → 0 | probe 288, 28 anchors |
| O-Bassoon | 1.2.1 | `e76224af` | 26 → 0 | 16 of 44 | 0 → 0 | 198/198 |
| O-TextureForge | 1.2.1 | `6de042ff` (+`18aca4ee`) | 19 → 0 | 17 of 41 | 0 → 0 | 287/287 |

`boot-all-uis` 43/43 / 0 DEAD on every run; `auval` PASS ×6; the sanisizer hazard did not
fire on O-TextureForge, whose `i18n.js` is its own binary-data resource and not a webpack
input.

## THE HEADLINE: a 0 G1 on a page the glossary barely covers is coverage, not a verdict

O-TextureForge measured that `TERMS` matched **3 of its 12 parameter captions**; two of the
nine unmatched ones were wrong by reading (*Fondu* for a crossfade, *Taille grain*). The
glossary grew `crossfade`, `scatter x/y`, `timing → cadence` (heading sense), `disp`. The
lint's straight-copy INFO is now scoped the way `check-i18n`'s flag is (entry, not row) —
a French title over a translated body needs no flag and must not get one.

## *ce plugin*, settled

The suite was split 12 plugins *plugin* / 3 *plugiciel* / 0 *extension* (O-Tapestop asked
for the tie-break). Settled on *plugin*, masculine; *plugiciel* forbidden in prose;
O-Tapestop's one occurrence fixed by the orchestrator copy-only.

## Header width defences: 5 of 15 backwards, 0 of 15 wrong about a recorded number

O-Gain's popover note ("Hover help is the widest row, 12 px to spare" — it was Language,
29.2) and O-Tapestop's *Suivi tonal* ("97 px in an 88 px cell" — 91.97 px, and the cell was
never the constraint) join O-Comp, O-Chorus, O-AnalogEQ. Five headers held to the
hundredth. O-Gain's own first-draft header quoted a number under the wrong label
(*Confiance* 44.8 was *Confidence*) and caught itself — corrected in `6f81d0ea`.

## What the lint cannot see — the meaning column, N3

Restored omissions: *grit* (O-TextureForge), "bends the pitch" (O-Bassoon), *whether*
(O-FreqPulse), a closing range (O-Texture, N2). Wrong claims: *accelerating a tail*
(O-Bassoon), *the values overlap* for *the grains* (O-TextureForge), *intonné*, *lit* for a
bed, *corpulent* for body-heavy. Three bodies on O-SpectralShaper named a control by its
ENGLISH caption (*Save*, *Freehand*, *Lookahead*) where the French page reads *Enr.*,
*Libre*, *Anticipation* — correction 23's shape, three times on one page.

## Renderer finding — the floor absorbs growth

O-Gain's +440-char plant grew a tip 54 → 203 px and it **stayed in frame**, parked over
the controls above by the `Math.max(M, ny)` floor; `inFrame` cannot see a body that grows
on that page. Heights are the discriminator. O-FreqPulse's negative control (remove the
v1.18.0 bottom clamp) reproduced Stage J's finding exactly: one French tip, 15.00 px off
the bottom, 0 px of slack at the shipped face.

## English defects found by reading French (items 36–40)

36. O-Bassoon `tip.breath` says CC2 "takes over"; the code multiplies UI breath × CC2, so a
    breath controller does nothing with the knob at 0. `aria.breathMeter` on the same
    feature says it right.
37. O-Gain `tip.info-confidence` omits the 50-block minimum that also forces LOW
    (`kConfidenceMinBlocks`); a user running Learn 8 s on a sparse signal sees LOW unexplained.
38. O-Tapestop: three division selects carry `aria-label` "Stop Time" / "Start Time" /
    "Env Length" against tip titles *Spin-Down Time* / *Spin-Up Time* / *Pass Length* and a
    visible caption *Division* — three names per control, WCAG 2.5.3, both languages.
39. O-Tapestop `PRESET_THEMES` (`app.js:597-605`) is unlocalized English prose written via
    `textContent` — *Tape Stops / Scratch / Wobble & Warp / Glitch & Chaos* render on the
    French page. The draft's translated headings in a body were describing headings that do
    not exist in French; renamed back to match.
40. `.settings-toggle` `min-width: 40px` no longer covers *Marche* (36.97 px in a 22 px
    content box) or *Désactivée* (61.88) — the button resizes between its own French faces on
    O-FreqPulse and O-SpectralShaper, inside popover slack, both gates green. A ~55–62 px
    pin is a CSS decision Stage N does not take. Also: O-TextureForge `tip.midiMode` implies
    CC control distinguishes Trigger + Modulate (Drone takes CC too; T+M is monophonic).

## Not verified

No native speaker; no DAW; Chromium only; Standalone stale; 3 more plugins with no
committed render gate (O-FreqPulse, O-Gain, O-SpectralShaper — item 31 now 5 of 15).

# STAGE N — BATCH N4 COMPLETE, 6 of 6 — 21 of 43

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-Bowed | 1.6.1 | `a7ff4f35` (+`2988870d`) | 27 → 0 | 24 of 71 | 0 → 0 | 456/456; DSP golden bit-identical |
| O-Emulator | 1.2.1 | `8d2bfcba` (+`907a191e`) | 26 → 0* | 9 of 22 | 0 → 0 | 123/123 |
| O-ReverseDelay | 1.10.1 | `33feb09c` | 29 → 0 | 24 of 69 | 0 → 0 | clamp gate 8/2 both languages, 62 rows byte-identical |
| O-Octagon | 1.11.1 | `af42a44e` | 30 → 0* | 40 of 187 | 0 → 0 | probe 67 anchors ×2; 6 committed gates green |
| O-MultiBandCompressor | 1.11.1 | `5247ce85` (+`e88b5e87`) | 30 → 0* | 31 of 98 rows | 0 → 0 | probe 70/70; preset harness 47 active, 0 fail |
| O-Orbit | 1.2.1 | `79b4cc93` (+`ee1e3d44`) | 29 → 0 | 23 of 91 | 0 → 0 | probe 34 anchors ×2, heights identical |

\* closed to 0 by a glossary/lint change with no plugin change (`def4f1bb`, `0b5f22df`,
`77a11cbd`). `boot-all-uis` 43/43 / 0 DEAD on every run; `auval` PASS ×6.

## THE HEADLINE: a bundle can carry new strings and an old version, and every grep passes

O-ReverseDelay's first build shipped 1.10.1 source in a 1.10.0 bundle: a sibling's CMake
regenerate had stamped `build.ninja` six seconds NEWER than its `CMakeLists.txt` edit, so
ninja skipped the re-run; the binary-data `i18n.js` re-embedded and every string control
passed while `Info.plist` kept 1.10.0. The executor read the installed plist and rebuilt.
**All 25 bundles shipped in Stage N so far were then audited with PlistBuddy: none stale.**
Correction 31: `touch` the CMakeLists before the build; verify the installed plist.

## The lint's last two holes, found by reading two tables against each other

- A forbidden key ending in a period (`dériv.`, `fréq.`, `flatt.`) could never match a
  caption whose period is trailing — `norm()` had stripped it. MBC saw *Dériv.* draw G1
  and not F1 while *Relâche* drew both. Fixed `0b5f22df`; the stem match then fired
  correctly on nine not-yet-reviewed plugins and on none of the shipped ones.
- T2 read Logic's `7.1` as a decimal (O-Octagon, quoting the DAW's own menu). Fixed
  `77a11cbd` with a format-name skip; a real decimal beside one still fires.

Glossary growth, all measured: *écras./broyage*, *sûr ?*, *compens.*, *act./dés.*, `m/s`.
**Still no settled term wrong** — six contextual exemptions this batch, every one on
meaning, none on width (correction 33).

## Nine of twenty-one header width defences proven backwards

O-Bowed's "62 px hard cap" was raised to 64 at v1.5.0 and the header never followed;
O-Octagon's *Décroissance* fits 81.59 in 88 where the header said 72 holds 9; O-ReverseDelay's
*Profondeur* "0.4 px over — a clip" was shrink-to-fit with `overflow: visible`; O-Orbit's
"9 px pill" has rendered at 11 px since v1.0.0 because `.toggle-label { font-size: 9px }`
loses on specificity — so MARCHE/ARRÊT would fit at the size the stylesheet asks for.
Nine wrong about the string; still zero wrong about a number they actually recorded.

## Parameter faces are English in both languages — and the French bodies named them in French

O-Octagon's six motion choices and fifteen sync divisions are `juce::StringArray` literals,
exempt by design, rendered in English on the French page. The drafts told the user to find
*Orbite / Huit / Balayage / Dérive / Pendule / Spirale* — words no dropdown shows. MBC's M/S
tip named *Off / Mid / Side / Both* where the French select reads *Aucun / Mid / Side / Les
deux*. Correction 34: a capitalised face is named as the user sees it.

## Defects found by reading French (items 41–46)

41. **O-Bowed's 15 `fillText` strings are English literals in `index.html`**, never keyed —
    K3 reported them, the K4 addendum called them a Stage M backlog, M2 did not take them.
    A `fillText`-recording probe painted 41 identical strings in both languages
    (*Bridge*, *Nut*, *Speed: 0.20 m/s*, *Bow Pressure (N)*, *Helmholtz*…). Invisible to
    all three gates.
42. **O-Bowed: Sympathetic Decay stays on screen at Count 0**, exactly as inert as
    Sympathetic Amount, which `updateSympVisibility()` hides.
43. **O-ReverseDelay `knob-drive` body says "Regen Makeup sets how long the tail lasts"** —
    that control's caption and title are *Regen*; the body names a control the page does
    not show by that name.
44. **O-Orbit `.toggle-label { font-size: 9px }` is dead CSS** (specificity 0,1,0 vs 0,1,1);
    and **the page has no focus latch** — hover-help opens on `mouseover` only, so it has no
    keyboard half at all (a wider case of item 11).
45. **MBC's three-button row now has 5.31 px of French slack** in 188.50 (`Contour.` 64.50
    against a root *Contournement* that grows the band to 221.94) — the suite's most exposed
    Windows/WebView2 metric.
46. **O-Emulator `.hdr` is 162 px over-full in English** (pre-existing, v1.0.1) and its
    segment caption reads `GB` against a Choice option and `#consoleInfo` reading *Game Boy*.

## Not verified

No native speaker; no DAW; Chromium only; Standalone stale; 3 more without a committed
render gate (O-Octagon, MBC, O-Orbit — item 31 now 8 of 21). Repo lint total **1447**.

# STAGE N — BATCH N5 COMPLETE, 6 of 6 — 27 of 43

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-Marimba | 1.13.1 | `2240a80f` (+`2cbd1879`) | 31 → 0 | 22 of 69 | 0 → 0, byte-identical | probe 18/18 ×2, heights identical |
| O-Freeze | 2.2.1 | `3bd6b5e8` (+`f67268b1`) | 38 → 0 | 23 of 31 | 0 → 0 | 311/311 |
| O-SimpleReverb | 1.7.1 | `dd1b58e6` (+`db78b6b6`) | 35 → 0 | 14 of 29 | 0 → 0, byte-identical | 169/169, all ten heights identical |
| O-DigiDelay | 1.4.1 | `c4a7eac9` (+`65f0c40f`) | 40 → 0 | 20 of 31 | 0 → 0 | 216/216 |
| O-Bitrot | 1.15.1 | `143f75c7` (+`9ba02746`) | 44 → 0* | 49 of 117 | 1 → 1 (standing baseline) | clamp 55/16/13 fr, unchanged |
| O-Lyrica | 2.4.2 | `d80b0bed` (+`2a66842c`) | 42 → 0 | 37 of 167 | 0 → 0 | probe 46/46 ×2 |

\* closed by `dfdc9732` with no plugin change. Installed plists verified on all six
(correction 31); `boot-all-uis` 43/43 / 0 DEAD; `auval` PASS ×5 — O-Lyrica's `auval` fails
on a documented pre-existing "Free Glissando" mutual-exclusion check.

## THE HEADLINE: a rule that zeroes a column has not fixed the column

O-Bitrot's version identifier (`pre-1.10`) drew T2 and it proposed the elegant rule — a
digit.digit token that appears verbatim in the English is an identifier. Applied, the T2
column went to **0** across 43 plugins. That was the tell: the English writes real decimals
with a point too, so the rule exempted the defect T2 exists for. Reverted within the same
hour for an explicit identifier set (format names, multi-dot tokens, version words); the
genuine decimal on O-Prism fires again. [[pattern_new_lint_zero_must_agree_with_independent_scan]],
second instance in one stage.

## Two glossary terms were wrong, both caught by a reader who knew the instrument

*mailloche* is a bass-drum beater; a marimba's is a *maillet* — O-Marimba's own bodies had
said so since v1.12.1. And `load .scl` had no abbreviation where `save .scl` did, on the same
7 px button row. Both fixed `be3ce2c7`. Running score: **2 settled terms wrong out of ~240,
after 27 plugins.**

## Header defences: 12 of 27 wrong about the string, still 0 wrong about a number

O-Marimba's header put 118.9 px on AMORTISSEMENT (97.41), credited 108.6 to the long
*Intervalles de la gamme* and shipped the short one (107.88 — the long form is 121.61 and
fits). O-DigiDelay's TEMPS defence was on *meaning* ("a duration rather than a position") and
the control is a duration. O-SimpleReverb's twelve strings all held to the hundredth.

## Meaning column, N5

*chœur* (a choir) for a chorus effect; *nappe* covering both *pad* and *bed*; *Seuil* over a
knob captioned THRESHOLD; a tuning called a *tempérament* on a page where that word is
spoken for; *En mode CUSTOM* on a page whose button reads PERSO; *matière* for a material;
*lit de bruit* (a calque) four times; a dropped *at all*; a dropped *individual*.

## Defects found by reading French (items 47–52)

47. **O-SimpleReverb hard-codes `v1.5.5`** in the footer wordmark and console banner — four
    versions stale, inside a text-matched `I18N_EXEMPT` entry.
48. **O-Freeze DETUNE's "cents" are ~1.44× off in the DSP**: `playbackRate = 1 + r·(detune/1200)`
    where cents are `2^(c/1200)`. Knob 50 → ±70.67 ct actual. Host-visible unit.
49. **O-Bitrot `settings` tip says the gear chooses "the language of this hover help"** — it
    sets the whole page's language since v1.15.0; stale in both languages.
50. **O-Lyrica's footer (`position: absolute; z-index: 10`) paints over `#sympatheticAmount`
    and `#sympatheticQ`** — only a 6 px caption strip is reachable; the sliders lose their
    drag surface and their hover-help. Every rect-comparing assertion is green on it.
51. O-Lyrica `technique` body says *Harmonics*; the Choice face is *Harmonic*. O-Freeze's
    *Alé → Random* is a wider gap than English's *Rnd → Random* (faces are exempt by design).
52. C1's two arms are asymmetric: a 2–3 letter French all-caps over a mixed-case English
    (`Mix` → `MIX`) is invisible to the shouting arm. Observed, not changed.

## Not verified

No native speaker; no DAW; Chromium only; Standalone stale; O-Marimba and O-Lyrica have no
committed render gate (item 31: 10 of 27). Repo lint total **1215** (from 2145).

# STAGE N — BATCH N6 COMPLETE, the O-simple* family, 6 of 6 — 33 of 43

| Plugin | Version | Commit | Lint | Changed | Geometry | Probe |
|---|---|---|---|---|---|---|
| O-simpleBeatmaker | 1.1.1 | `768b07c7` (+`63935f5e`) | 42 → 0 | 32 of 81 | 0 → 0 at setSize; 128 → 128 at the 860×640 min (pre-existing) | 1352/1352, both frames |
| O-simplePhysicalModelSynth | 1.2.1 | `efadae9c` | 49 → 0 | 33 of 77 | 0 → 0 | 305/305; render-harness ALL PASS |
| O-simpleSubtractive | 1.3.1 | `2d7ee7f9` | 55 → 0 | 44 of 93 | 0 → 0; `.frame` 1118 held | 479/479 |
| O-simpleAdditive | 1.1.1 | `3cc2e392` (+`3cc703c8`) | 58 → 0 | 52 of 84 | 0 → 0 | 630/630 |
| O-simpleSampler | 1.4.2 | `e1684b33` (+`cff2d42a`) | 71 → 0 | 68 of 145 rows | 0 → 0; content 821.83 both | 37/37 ×2, seventh tour button binds |
| O-simpleGrain | 1.4.1 | `dd69886e` | 92 → 0 | 59 of 115 | 0 → 0 after a caption rewrite | 372/372; harness 15/15 |

Installed plists verified on all six; `boot-all-uis` 43/43 / 0 DEAD; `auval` PASS ×6. **Every
straight apostrophe in the suite's straight-apostrophe family is gone** (T1: 246 → 46 — the
rest are on N7–N9 plugins).

## THE HEADLINE: the family's captions live 1–5 px from a wrap, and a wrap moves a rack

O-simpleGrain's tour caption had 5.28 px of slack; the typographic apostrophe alone took
it to 1.34, the terminology fix wrapped it, and assertion 7 reported **159 non-label
elements moved** — the whole rack dropped 13.19 px. Reworded shorter with the same claim
(805 px, 40.92 slack). O-simpleBeatmaker spent 10.39 of its 17.56 px of banked margin on
the same kind of hint. These pages were designed to the pixel in English; French copy edits
on them are geometry edits.

## Same term, three verdicts

*Relâchement* (77.33 px, measured identically on all three pages) fits O-simpleFM and
O-simpleSubtractive with room and crosses O-simpleSampler's neighbour by 2.14 px. Same
cell width class; different gaps. Correction 42: measure the gap, not the cell.

## Meaning column, N6

*chœur*-class errors continued: *fût* (a drum shell) for "the instrument" (O-simpleBeatmaker);
*temps forts* (beats 1 and 3) for "the backbeat"; "the SAME buffer" and "single-cycle" and
"individual grains" dropped; *soutenue* for *tenue* on a pad; both Scan-LFO bodies on
O-simpleAdditive had lost the control they drive. Zero grammar defects on two pages —
the drafts were sound where they were plain.

## Defects found by reading French (items 53–58)

53. **O-simplePhysicalModelSynth `stringModel` is a dead parameter** — never `load()`ed;
    its tooltip still says the Waveguide "arrives in v1.1" on a 1.2.1 plugin.
54. **`aria.helpToggle` says "Toggle tooltips" against a tip title "Hover help"** on
    O-simplePhysicalModelSynth and O-simpleGrain — two English names for one control.
55. O-simpleSampler: `tip.pitchMode` and `tip.lessonRepitchStretch` open with the identical
    "The headline A/B."; `#pitchModeReadout` wraps to two lines in French in the Stretch
    state, which no committed gate drives.
56. **O-simpleBeatmaker at its 860×640 resize minimum moves 128 non-label elements between
    languages** — pre-existing at v1.1.0 (two hints wrap), invisible to every gate because
    the gates measure `setSize` only.
57. O-simpleGrain `tests/i18n-states.json` names the wrong caption as the longest; the
    actual longest French caption already wraps at v1.4.0 and shifts the rack 13.19 px in
    a state no gate drives.
58. **No focus latch on five pages** (O-Lyrica, O-Orbit, O-simpleSampler, O-simpleGrain,
    O-simpleAdditive): `focusin` opens a tip unconditionally, so a click parks one. Item 44
    widened.

## Not verified

No native speaker; no DAW; Chromium only; Standalone stale; none of the six has a
committed render gate (item 31: 16 of 33). Repo lint total **833** (from 2145).

# STAGE N — BATCH N7 COMPLETE, 5 of 5 — 38 of 43

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-Reed | 1.3.1 | `a382dae4` | 53 → 0 | 42 of 90 | 0 → 0, byte-identical | 523/523 |
| O-Contrabass | 1.8.1 | `33cecfe5` (+`50048559`) | 70 → 0 | 47 of 103 | 0 → 0 | probe 46/46 ×2; `ui_frontend_check` byte-identical |
| O-GrainScatter | 2.6.1 | `f438d23e` | 52 → 0 | 43 of 87 | 0 → 0 | 796/796 |
| O-Detune | 1.7.1 | `29152c60` (+`c81504c7`) | 65 → 0* | 26 of 47 | 0 → 0 | 393/393 |
| O-MicrotonalSampler | 1.25.1 | `2dadaeeb` | 74 → 0* | 73 of 290 values | 0 → 0 | 349/349, heights identical |

\* closed by a lint/glossary change with no plugin change. Installed plists verified on all
five; `boot-all-uis` 43/43 / 0 DEAD; `auval` PASS ×5.

## THE HEADLINE: the lint's prose scan missed prose that lives in LABELS

Four *plugiciel* on O-MicrotonalSampler — two multi-sentence dialog messages and two
preset accessible names, all in `LABELS`/`aria.*` — drew zero F1 because
`FORBIDDEN_IN_PROSE` ran on `body` rows only. Correction 27 had predicted "will meet it as
F1"; the executor met it by grep. Fixed: a prose-forbidden word is forbidden on every row.
The second lint hole this batch: the one TERMS key ending in a period was unreachable from
a lookup that strips trailing periods (O-Contrabass scanned all ~240 keys to find it).

## The glossary at 38 plugins

Rows added from measurements this batch: `az/el spread`, `anche dble`, `prof. vib`,
`écart tot`/`étendue`, `body` (Corps vs Caisse — the two bowed-string plugins disagree
and both are right; item 60). T7 learned decade names; T2's identifier rule stands.
**Settled terms wrong so far: 2 of ~250** (*mailloche*, and `load .scl` lacking its twin's
abbreviation). Contextual exemptions on meaning this batch: 13, none on width.

## Header defences: three more backwards on O-Detune, all of O-Reed's held

O-Detune's header defended SAUVER (ENREG. is 2.15 px narrower), measured the wrong box for
AMPLEUR (the caption is shrink-to-fit in a 91 px cell, not a 52 px face — PROFONDEUR fits
with 19.75 to spare), and put 61 px on PRÉ-DÉLAI (55.86). O-Reed's held to the hundredth,
and O-Contrabass PROVED *Enreg.* by planting the root (12 elements move). Score: 15 of 38
headers wrong about the string they defended; still 0 wrong about a number they recorded —
though O-MicrotonalSampler found one stale number under a sound conclusion.

## Meaning column, N7

"Fond enchaîné" (a *background*) for a crossfade — a one-letter typo that changed the noun;
*le défilement* (scrolling) for the host transport; *plus de poitrine* for "more chest";
a dangling *Trop légère, le son glisse*; `tip.scan` had dropped the control's own name;
O-GrainScatter's Doppler body was a garden path (*qui passe monte*).

## Defects found by reading French (items 59–61)

59. **Guillemet spacing has no rule and no lint code**: 43 plugins ship `« … »` with plain
    spaces (O-Reed counted 21 pairs on one page); French wants U+00A0 inside both. Adding it
    mid-stage would red every shipped plugin and add unbreakable runs to the tightest tips.
60. **Two bowed-string plugins name the soundbox differently** — O-Bowed *Caisse*, O-Contrabass
    *Corps*, five places each. Both correct; a reader's call, not a lint's.
61. O-Contrabass's `note-expression-toggle` tip title is sentence-cased "Note expression"
    against a caption and a VST3 feature named "Note Expression"; two stale AU registrations
    (`OCb5`, `OCbP`) still sit on this machine beside the live `OCbs`.

## Not verified

No native speaker; no DAW; Chromium only; Standalone stale; O-Contrabass has no committed
render gate (item 31: 17 of 38). Repo lint total **after N7: see the N8 log** — the remaining
five plugins carry it all.

# STAGE N — BATCH N8 COMPLETE, the tuning-panel family, 3 of 3 — 41 of 43

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-IntonationPad | 2.9.1 | `ac2a7323` (+`31b9ff1a`) | 67 → 0 | 52 of 199 | 0 → 0; 8b red on 4/20 states until the gate fix | probe 79/80 ×2; 17 late / 0 dead held |
| O-Wind | 1.18.1 | `33adb342` | 65 → 0 | 59 of 119 | 0 → 0 | gate 0 failures, every tab |
| O-Formant | 1.27.1 | `5c91939b` (+`9f48b062`) | 85 → 0 | 60 of 182 | 0 → 0 | 1360 byte-identical; canvas strings recorded en→fr→en |

Installed plists verified ×3; `boot-all-uis` 43/43 / 0 DEAD; `auval` PASS ×3.

## THE HEADLINE: a gate went red on the settled term, and the gate was wrong

O-IntonationPad's *Accord* (51.84 px in a 52 px box) nudged its tab button 0.62 px into a
`.tuning-controls-panel` rect that begins at x=542 — a panel scrolled inside an
`overflow: auto` tab that clips it at y=96, forty-four pixels above the tab row. Assertion
8b compares `getBoundingClientRect` boxes, which are unclipped; `elementFromPoint` at every
point of the 1.09 × 29 px overlap returns the button. English itself sat 0.03 px from the
same false verdict. The executor shipped the settled term, proved the overlap had no
pixels, and stopped. **Fixed in the gate** (`vrect`, clipped by overflow ancestors; 8b uses
it): O-IntonationPad 20/20 green, five other plugins byte-identical. Third gate defect this
stage found by a plugin the gate had never been run against in that state.

## O-Wind's Effects tab has never been measured — and the fix would be a states-file edit

`tests/i18n-states.json` opens the settings popover first; it renders at 698,39 190×40 over
the right 190 px of the 300 px Effects tab button, so the tab click lands in the popover
and Playwright times out. 25 of 65 `[data-i18n]` elements on that page have no geometry
history. The executor ran assertions 4/5/7/8 on the open tab by hand (0 moved, nothing
clipped, four captions shrank) and left the states file alone — it is a gate. Item 62.

## Header defences: O-Formant three backwards, O-Wind two held, O-IntonationPad one held

O-Formant's *Sauver* was 64.02 px in a `min-width: 65px` button — the abbreviation was
free; *Vib Vitesse* / *Vib Ampleur* / *Rétroaction* were each WIDER than the settled term
that replaced them. *Formant du chanteur* held to 0.01 px. O-IntonationPad's
*Bibliothèque* defence reproduced its own numbers to the tenth — and the executor's first
measurement (56.50 px clear) was taken in the collapsed-library state and was wrong; the
gate's state is the one that counts. Score: 18 of 41 headers wrong about the string.

## Meaning column, N8

*un plaqué naturel* (a block chord) for a timing offset; *La3* against a caption reading
RÉF. A4; "affiche On" for a button that reads MARCHE; *en chantant* for humming; a word
that is not French (*rugosifie*); *Mod Rd* (a caption) as a tip title that had lost
"Depth"; *affichage d'angle* for a corner readout; *démarre juste* for "starts on pitch".

## Defects found by reading French (items 62–65)

62. **O-Wind's Effects tab has never been geometry-measured** (states-file ordering; above).
63. **O-Formant `tip.formantSpread` misnames the pivot** — "around the first one"; the code
    scales around the centre of mass of all five formants (`FormantFilterBank.h:100-107`).
64. **O-Formant is the only plugin whose tuning-panel strings are keyed `tuning.*`** — the
    other seven key them differently; *Touches réelles* fits (62.55 in 66.80 px) but was not
    applied unilaterally. The eight copies converge on text, not on keys.
65. O-IntonationPad has no committed render gate (its tips landed at v2.9.0 outside M1–M3);
    O-Wind's `[Effects] Panel initialized (v1.14.0)` banner is four versions stale.

## Not verified

No native speaker; no DAW; Chromium only; Standalone stale; O-Formant's installed bundle is
one CSS comment behind HEAD. Repo lint total after N8: **the two N9 plugins carry it all.**

# STAGE N — BATCH N9 COMPLETE, 2 of 2 — STAGE N IS DONE, 43 of 43

| Plugin | Version | Commit | Lint | Changed | Geometry | Render |
|---|---|---|---|---|---|---|
| O-Bells | 4.3.1 | `33a24c58` (+`e0962b99`) | 115 → 0 | 72 of 186 | 0 → 0 | 1024/1024, every tab; 2 late / 0 dead held |
| O-Prism | 1.22.1 | `c704cabb` (+ orchestrator copy-only fix) | 235 → 19 → 0 | 125 of 262 | 0 → 0 | 2180/2180, every tab and modal |

O-Prism's last 19 were tooling: `glide` and `master` had no abbreviation and their roots do
not fit (Portamento 71.52 px in 52; Général 46.61 in 44.01 — measured, and *Glissé* /
*Maître* were the forbidden words the page had kept), plus two readouts a sentence quotes
verbatim. The glossary grew *porta / gén / fq. méd / amor*; the lint learned that text inside
« » is quoted screen text; the two forbidden captions were fixed by the orchestrator
copy-only (*Porta*, *Mode porta*, *Gén.*), `check-ui-labels` ALL PASSED, rebuilt.

## STAGE N, closed — the numbers

- **43 of 43 plugins shipped a patch bump** with every French entry read against its
  English, the glossary and the lint. Every one: `--strict` 0, **0 non-label elements
  moved** between languages (O-Bitrot's standing 1 unchanged), its render gate or a
  scratchpad probe green in both languages with tip heights read, `boot-all-uis` 43/43,
  `auval` PASS, installed `Info.plist` at the new version.
- **Repo-wide French lint: 2145 → 0** (`node scripts/i18n-fr-lint.js --strict` exits 0 on
  43/43). Final census: `boot-all-uis` clean 43/43, **0 DEAD**, 19 late (O-Bells 2,
  O-IntonationPad 17 — by design), native `title=` 0; `check-i18n` ALL CHECKS PASS, 3751
  entries, all `reviewed: false` — **no native speaker has read one** (item 27 stands).
- **Glossary:** ~260 settled terms; **2 were wrong** (*mailloche*; `load .scl` lacking its
  twin's abbreviation) and ~30 rows were added or extended from executors' measurements.
  60 `termNote` exemptions across the suite, every one on meaning, none on width.
- **Lint defects found by the executors and fixed: 12** (termNote scope, glossary-accepted F1,
  T7 missing-space, `%` double-count, straight-copy condition and entry scope, trailing-period
  keys twice, T2 identifiers (and one wrong rule reverted), decade names, prose scan on every
  row, quoted screen text). **One gate defect fixed:** `check-ui-labels` 8b now compares
  painted (clipped) rects.
- **Header width defences: 20 of 43 wrong about the string they defended, 0 wrong about a
  number they recorded** — "re-measure, never inherit" held for the whole stage.
- **Meaning defects the lint cannot see:** ~120 sentences across 43 plugins — dropped clauses,
  calques (*lit* for bed, *chœur* for chorus, *fût* for instrument), faces named in the wrong
  language, wrong antecedents, one typo that changed the noun (*Fond enchaîné*).

## Decision items added, N9

66. O-Bells `tip.partialTuning` says "the upper partials" — the code moves the tierce only;
    `tip.damping` overstates its scope; `ModalPartial::decayRate` is a dead write with the
    damping mapping inverted relative to the two live paths.
67. O-Prism's `glide` and `master` captions could not carry any listed root; *porta* / *gén.*
    are now settled abbreviations — a native reader may prefer others.
68. `--strict` is green on 43/43 for the first time and **still nothing calls it** (item 29).
    Wiring `node scripts/i18n-fr-lint.js --strict` beside `check-i18n` is one line.

## Not verified — the standing list

No native speaker (item 27); no DAW test on any of the 43 (`auval` + headless Chromium only,
never WKWebView); Standalone `.app` stale everywhere; Windows/WebView2 metrics untested and
now carrying ~1500 no-break spaces of new unbreakable runs; 17 plugins still have no
committed render gate (item 31); guillemet spacing has no rule (item 59).

## Post-stage, developer decisions (2026-08-31)

- **Item 27 — CLOSED.** The developer reads French and checked the copy: all 3702 flags are
  `reviewed: true` (`13fc8dd0`, metadata only, no bumps). `check-i18n`'s worklist reads 0.
- **Items 29 / 68 — CLOSED.** `--strict` is gone; `scripts/i18n-fr-lint.js` exits 2 on any
  finding by default. Nothing in CI runs it (no CI test target exists) — run it by hand after
  any French edit.

---

# STAGE O — BATCH O1 COMPLETE, 5 of 5 — 5 of 21

Stage O fixes the English / DSP / CSS defects Stage N found by reading the French against the
code (brief: `260826-ieq-STAGE-O-BRIEF.md`). One executor per plugin, one bump each, builds on
the Stage K mutex, path-scoped commits, no push. PLUGINS.md rows: `68b52758`.

| Plugin | Version | Commit | Item | Proof the probe moved |
|---|---|---|---|---|
| O-Freeze | **2.3.0** (minor) | `89f82a6a` | 48 DSP | knob 50: +63.1/−69.0 ct → +45.6/−47.0 (44 grains); 209 grains +69.6/−72.5 → +49.5/−50.0 |
| O-simpleFM | 1.3.2 | `aa76ef71` | 30 labels | auval `Name: Ratio (M:C)`; binary `Ratio (C:M)` ×0; render harness byte-identical |
| O-Polystutter | 1.14.2 | `c5f42cf7` | 34 tip | probe 6 assertions FAIL before → 19/19; tip fr 86.6 → 102.0 px, inside frame |
| O-Tremolo | 1.8.2 | `00b98b5e` | 35 tip | render gate 186/186; pan tip fr 114.5 → 130.6 px |
| O-Bassoon | 1.2.2 | `97db3b04` | 36 tip | render gate 198/198; breath tip fr 136.5 → 219.9 px, 204 px clearance |

Every one: check-ui-labels 0 → 0 moved, check-i18n PASS, i18n-fr-lint exit 0, boot-all-uis
43/43 / 0 DEAD / late unchanged, `auval` PASS, installed plist at the new version (AU + VST3),
new French VALUE grepped in the installed binary with `LC_ALL=C grep -a`.

## THE HEADLINE: the knob-5 measurement could not show ±5 ct until a second bug was fixed

O-Freeze's detune was linear (`1 + c/1200`), as the brief said — but the scratchpad harness
found the pre-fix grains at knob 5 sitting at exactly 0 / ±3.38 / ±6.75 ct and nowhere else.
`Grain::fractionalPosition` was a `float` accumulator: at a 1000 ms grain its ulp is 2⁻⁸, so
every per-sample rate rounded to a multiple of 1/512 — **every grain's pitch was snapped to a
3.38 ct grid** (1.69 ct at the default 400 ms). Fixed in the same commit (`double`). After the
fix each grain's knob-25 pitch is 5.000× its knob-5 pitch and knob-50 is 9.999× — it was not
before. The same `float` read-position pattern is a candidate in every granular plugin at long
grains (O-simpleGrain, O-GrainScatter, O-TextureForge — item 69, not checked).

## The brief was wrong twice, both caught by reading the source

- O-simpleFM's French target in the brief (`Ratio M:C`) ignored Stage K: *Rapport* (the
  glossary reserves *ratio* for dynamics) and **P** for *porteuse*. Shipped `Rapport M:P` /
  `Rapport (M : P)`. **A brief's French is a suggestion; the glossary and the Stage K header
  are the spec.**
- "The Clarinet lesson is right" — its odd-harmonic claim was; its opening "Carrier:modulator
  2:1" was backwards for a preset at ratio 2.0. Fixed.
- O-Polystutter: the brief's "C4–D#4 in JUCE/Yamaha C3=60 numbering" contradicts itself; the
  plugin's own docs and JUCE's `octaveNumForMiddleC = 3` settle C3 = 60, and the body carries
  the note numbers so a C4 = 60 host still agrees.

## Defects found, not fixed (items 69–70)

69. Float fractional read positions in other granular plugins (above).
70. **O-Bassoon's Breath knob is dead for a fresh note until an expression parameter moves.**
    `BassoonVoice.cpp:107-108` seeds `breathSmoother` / `lastUiBreath` from note VELOCITY at
    note-on; `PluginProcessor.cpp:311-321` dispatches `setExpression()` only when an
    expression param changed by > 0.001 since the last dispatch. With a static knob a new
    note's breath is its velocity, and CC2 multiplies velocity, not the knob — the new tip
    ("knob sets the ceiling") is exact only after a knob has moved since the note started.
    Comment cites CONTEXT-rev-3 as a design decision. The breath meter shows velocity for an
    untouched knob.
- O-Polystutter's `midi` entry (and ~17 others in that file) carries `sameAsEn: true` over a
  translated body with an English-equal title — correction 26 says no flag; both gates pass.
- O-Bassoon's PLUGINS.md status `🚧 Stage 0` is stale (STATUS.md: stage 4 in progress).
- O-Freeze `README.md:76` says `V1.0.0`.

## Not verified

No DAW listen on any of the five; O-Freeze not measured at 48/96/192 kHz (the `double` fix
covers it by construction); the scratchpad pitch harness uses `#define private public` to
seed the grain RNG and is not promotable as-is (item 31 still stands for O-Freeze).

---

# STAGE O — BATCH O2 COMPLETE, 5 of 5 — 10 of 21

PLUGINS.md rows: `95c1118a`. Orchestrator commit `47c7be7d` (O-Bells render gate, see below).

| Plugin | Version | Commit | Items | Proof the probe moved |
|---|---|---|---|---|
| O-Gain | 1.3.2 | `f5ed6f9a` | 37 | 26-anchor height probe at 350×500: tip 67.6 → 81.1 en / 94.6 fr, top edge 334 px from frame top, none off-frame |
| O-Formant | 1.27.2 | `420cfe49` | 63, **22** | save/reopen probe: A4 442 / stretch 1.05 → 440 / 1.00 BEFORE, 442 / 1.05 AFTER, note 69 → 442.000 Hz; spread tip 94.6 → 94.6 px |
| O-Bells | 4.3.2 | `19c3a9e2` | 66 | 75-case render (notes × damping × tierce), sha256 identical before/after the dead-write removal; seed 999 differs (probe can fail) |
| O-Bowed | 1.6.2 | `3302286f` | 41, 42 | `fillText` probe: 13 English strings under fr → 0; 12 French; repaint on switch; widest *Pression d’archet (N)* 93.69 px in 338; Decay 62×80.6 → 0×0 at Count 0 |
| O-Lyrica | 2.4.3 | `fc159b51` | 50, 51, 58 | `elementFromPoint` at both slider centres: `div.footer` → `input#sympatheticAmount/Q`; 81 elements moved, all SOUND tab, all −6/−12/−18/−24 in y; Tab → tip opens (failed before) |

Every one: check-ui-labels 0 → 0 moved, check-i18n PASS, i18n-fr-lint exit 0, boot-all-uis
43/43 / 0 DEAD / late unchanged (O-Bells 2), installed plist at the new version (AU + VST3),
new VALUE grepped in the installed binary. `auval` PASS on four; **O-Lyrica `auval` FAILS,
pre-existing and unchanged** (below).

## THE HEADLINE: item 22 was two bugs, and the brief's "optional" was the cheaper one

A4 was lost on reopen because `setMasterTune` wrote the engine directly and
`getStateInformation` never saved it; `setStateInformation` then pushed the never-touched
`tuning_masterTune` parameter (440) back over the engine. 23 lines in `PluginProcessor.cpp`
(save + restore on the existing `tuningEngine` child, `isVoid()` guard, string-var read).
But the panel had **no A4 read path**: a restored 442 would have sat under a *440.0 Hz*
readout and every drag started from a literal 440 — the vendored `tuning-panel.js` predated
the shared module's `getMasterTune` (`modules/tuning/scala-tuning-engine/js/tuning-panel.js:943-986`).
Ported into the vendored copy; `modules/` untouched. **Item 71:** the other tuning-panel
plugins (O-Bells, O-IntonationPad, O-Lyrica, O-MicrotonalSampler, O-Prism, O-Reed, O-Wind)
may carry the same pre-`getMasterTune` vendored copy and the same state gap — not checked.

## A gate went red the day the suite was read

`plugins/O-Bells/tests/ui_tip_render_check.js` asserted **every** French entry is
`reviewed: false` — written when no native speaker existed, backwards since `13fc8dd0`
flipped the suite to `true`. It failed at baseline and after; the executor reported it rather
than editing a gate outside its item. Fixed by the orchestrator (`47c7be7d`): the flag must be
a BOOLEAN, the `false` count is printed as the developer's worklist (2 of 65). Only O-Bells'
gate had the assertion (grep of every `ui_tip_render_check.js` / `ui_tooltip_clamp_check.js`).

## The brief was wrong three more times

- O-Gain's "frame trap" described the v1.2 pure-CSS `::after` tooltips; Stage J replaced them
  with the shared JS renderer. Measured through the renderer.
- O-Bowed: "15 canvas strings" is 17 at 13 `fillText` sites.
- O-Lyrica item 58: "a pointer click opens a tip via focusin" — there was NO `focusin` handler;
  the defect was the other half (Tab opened nothing). The latch port fixed the real one.
- (O-Gain also removed "High = over 15 s *with stable signal*" — nothing in the code reads
  signal stability into `confidence`.)

## Defects found, not fixed (items 71–75)

71. Tuning-panel family: vendored `tuning-panel.js` without `getMasterTune` + the same
    `getStateInformation` gap (above).
72. **O-Lyrica `auval` FAIL, pre-existing, stable 4/4**: ParameterID 1275870432 "Free
    Glissando" saved 0.338 vs current 0 — `freeToggle` / `scaleToggle` mutual exclusion
    (`PluginProcessor.cpp:1042/1047`) rewrites the restored value. Stage N recorded it as
    benign since v1.30.0; an `auval` FAIL is not benign for a release build — decide.
73. O-Formant `tuning_masterTune` / `tuning_octaveStretch` are host-visible parameters with
    no listener — automating them does nothing; the parameter view still shows 440 after a
    reopen at 442.
74. O-Lyrica TUNING tab `#generator-section` header (y 384–412) rests across the footer top —
    the item-50 shape on the other tab. `app.js:852` banner `v1.32.0` is a stale literal.
75. O-Bells `noteVariationDecay` is drawn and now feeds nothing (kept so the RNG sequence is
    unchanged). O-Bowed `tests/render-harness/CMakeLists.txt` pins `1.3.0`. O-Formant
    `TuningEngine::setBuiltInPreset()` prints to stdout on every call.

## Not verified

No DAW on any of the five; O-Formant's panel-readout half of item 22 verified by
syntax + boot-all-uis only (the ui-stub has no real `getMasterTune`); O-Bowed's material names
(*Membrane / Métal / Verre*) verified by width + table, not by runtime paint.
