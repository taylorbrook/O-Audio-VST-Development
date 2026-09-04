---
quick_id: 260903-rjm
type: execute
mode: quick
autonomous: false
files_modified:
  - plugins/O-Chorus/Source/ui/public/index.html
  - plugins/O-Chorus/Source/ui/public/js/i18n.js
  - plugins/O-Chorus/CMakeLists.txt
  - plugins/O-Chorus/CHANGELOG.md
  - plugins/{22 others}/… (see the scan table — one index.html or js/app.js, one js/i18n.js, CMakeLists.txt, CHANGELOG.md each)
  - scripts/check-i18n.js

estimate:
  tokens: 260000
  raw_tokens: 130000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "Opening the gear popover in O-Chorus shows two rows: Language and Hover help, and the Hover help button turns the tooltips off and on."
    - "With hover help OFF, hovering a knob shows no tip; the gear and the Hover help button still show theirs."
    - "The Hover help caption and the On/Off face are localized in all three O-Chorus languages (en, fr, zh-Hans)."
    - "The toggle state survives closing and reopening the plugin editor (localStorage)."
    - "Every one of the 43 plugins that has #lang-select also has a hover-help toggle; check-i18n fails if one does not."
  artifacts:
    - "plugins/O-Chorus/Source/ui/public/index.html — .settings-toggle CSS, the second .settings-row, data-tip-always on #gear-btn and #tips-toggle, the tipsEnabled gate + toggle init"
    - "plugins/O-Chorus/Source/ui/public/js/i18n.js — label.hoverHelp, ui.on, ui.off, aria.helpToggle, I18N 'tips-toggle', TIP_BINDINGS row"
    - "scripts/check-i18n.js — assertion [16]"
  key_links:
    - "TIP_BINDINGS ['#tips-toggle','tips-toggle'] must resolve — a binding with no element is a DEAD binding that boot-all-uis --strict-tips reports."
    - "The toggle code must live OUTSIDE the applyI18n/initI18n region — that region is byte-compared against scripts/i18n-canon.js by check-i18n assertion [6]."
    - "Default is ON everywhere. Default OFF would be a behavior regression and would make ui_tip_render_check.js and boot-all-uis measure an empty tip surface."
---

<objective>
O-Chorus (and 22 other plugins) ship a gear popover with a language selector but
no way to turn the hover help off. 20 of the 43 plugins already have that toggle;
23 do not. Close the whole class, and add the lint that keeps it closed.

Purpose: hover help that cannot be switched off is hover help in the way. The
suite already settled the control, the copy and the glossary renderings — this is
a port, not a design.

Output: a hover-help toggle in all 23 missing plugins, matching the sibling
pattern; the toggle's copy in every language each plugin speaks; check-i18n
assertion [16] making "language selector without a hover-help toggle" a red gate.
</objective>

<context>
@/Users/taylorbrook/Dev/VST-development/CLAUDE.md
@/Users/taylorbrook/Dev/VST-development/.planning/STATE.md

Reference implementations (read the one that matches the plugin's family):
@/Users/taylorbrook/Dev/VST-development/plugins/O-Polystutter/Source/ui/public/index.html
@/Users/taylorbrook/Dev/VST-development/plugins/O-Gain/Source/ui/public/js/i18n.js
@/Users/taylorbrook/Dev/VST-development/plugins/O-simpleFM/Source/ui/public/js/app.js

Target:
@/Users/taylorbrook/Dev/VST-development/plugins/O-Chorus/Source/ui/public/index.html
@/Users/taylorbrook/Dev/VST-development/plugins/O-Chorus/Source/ui/public/js/i18n.js
</context>

---

## THE SCAN (measured 2026-09-03, live grep over all 43 plugins)

All 43 plugins have `#lang-select`. **20 have a hover-help toggle. 23 do not.**
Two id conventions already coexist and both are live; neither is being retired.

### Already has a toggle — DO NOT TOUCH (20)

| Family | id | Plugins |
|---|---|---|
| older | `#help-toggle`, label key `help-toggle` | O-Bitrot, O-Contrabass, O-Orbit, O-simpleAdditive, O-simpleBeatmaker, O-simpleFM, O-simpleGrain, O-simplePhysicalModelSynth, O-simpleSampler, O-simpleSubtractive, O-Tapestop (11) |
| newer | `#tips-toggle`, label key `label.hoverHelp` | O-FreqPulse, O-Gain, O-IntonationPad, O-Lyrica, O-Marimba, O-MultiBandCompressor, O-Polystutter, O-SpectralShaper (8) |
| newer, segmented | `#tips-toggle` as a two-button `role="group"` (`#btn-tips-on` / `#btn-tips-off`) | O-Octagon (1) |

### MISSING the toggle — the work (23)

`ui root` is relative to `plugins/<Name>/`. `renderer` is the file holding
`setupTooltips`/`initializeTooltips` — that is where the `tipsEnabled` gate goes.
`ver` is the current `VERSION` in that plugin's `CMakeLists.txt`.

| # | Plugin | ui root | renderer | ver | lang label key | note |
|---|--------|---------|----------|-----|----------------|------|
| 1 | O-Chorus | Source/ui/public | index.html | 1.5.0 | `label.language` | **TRACER.** 3 languages (en/fr/zh-Hans). 125 px frame, popover opens UPWARD |
| 2 | O-AnalogEQ | Source/ui/public | index.html | 1.3.1 | `label.language` | |
| 3 | O-AnalogSaturation | Source/ui/public | index.html | 1.3.1 | `label.language` | |
| 4 | O-Bass | Source/ui/public | index.html | 1.5.1 | `label.language` | popover height already documented in a width table — update it |
| 5 | O-Bassoon | Resources/ui | index.html | 1.2.2 | `label.language` | |
| 6 | O-Bells | Resources/ui | index.html | 4.3.2 | `label.language` | `ui.on`/`ui.off` may already exist — reuse, do not duplicate |
| 7 | O-Bowed | Resources/ui | index.html | 1.6.2 | `label.language` | |
| 8 | O-Comp | Source/ui/public | index.html | 1.7.1 | `label.language` | header notes a `#lang-select` tip paint-order rule — keep it |
| 9 | O-Detune | Source/ui/public | index.html | 1.7.1 | `label.language` | |
| 10 | O-DigiDelay | Source/ui/public | index.html | 1.4.1 | `label.language` | |
| 11 | O-Emulator | Source/ui/public | index.html | 1.2.1 | `label.language` | |
| 12 | O-Formant | Source/ui/public | index.html | 1.27.2 | `label.language` | |
| 13 | O-Freeze | Source/ui/public | index.html | 2.3.0 | `label.language` | |
| 14 | O-GrainScatter | Source/ui/public | **js/app.js** | 2.6.1 | `label.language` | only one of the 23 with an app.js renderer — follow O-simpleFM, not O-Chorus |
| 15 | O-MicrotonalSampler | Resources/ui | index.html | 1.25.2 | `label.language` | largest UI in the suite; has its own checkbox controls — do not reuse their class |
| 16 | O-Prism | Source/ui/public | index.html | 1.22.1 | `label.language` | **OUTLIER:** popover has NO `.settings-row` wrapper — label + select are direct children. Add the row wrapper for the new control only, or match the flat shape; measure either way |
| 17 | O-Reed | Resources/ui | index.html | 1.3.1 | `label.language` | tuning tab is a known open defect — do not touch it |
| 18 | O-ReverseDelay | Source/ui/public | index.html | 1.10.1 | **`lang-select`** | only one of the 23 on the older label-key convention; still use `#tips-toggle` + `label.hoverHelp` for the new control |
| 19 | O-SimpleReverb | Source/ui/public | index.html | 1.7.2 | `label.language` | `ui.on`/`ui.off` may already exist — reuse |
| 20 | O-Texture | Source/ui/public | index.html | 1.19.2 | `label.language` | |
| 21 | O-TextureForge | Source/ui/public | index.html | 1.2.1 | `label.language` | |
| 22 | O-Tremolo | Source/ui/public | index.html | 1.8.2 | `label.language` | |
| 23 | O-Wind | Resources/ui | index.html | 1.18.2 | `label.language` | |

**O-Chorus is the only one of the 23 that speaks zh-Hans.** The other 22 are
en/fr only, so their `LANGUAGES` array stays two long and no Chinese is authored
for them.

---

## THE RECIPE (settled — copy it, do not redesign it)

Five edits per plugin. New work uses the **newer** convention (`#tips-toggle`,
`label.hoverHelp`) because 21 of the 23 already use `label.language`, its sibling.

**1. CSS** — `.settings-toggle`, ported from `plugins/O-Polystutter/Source/ui/public/index.html:949-972`,
recolored to the target plugin's own palette:

```
.settings-toggle { min-width: 42px; padding: 2px 8px; border: 1px solid <plugin border>;
  border-radius: 3px; background: <plugin chip bg>; color: <plugin fg>;
  font-family: inherit; font-size: 11px; line-height: 1.2; cursor: pointer;
  transition: all 150ms ease; }
.settings-toggle[aria-pressed="true"] { background: <plugin accent>; color: <plugin accent fg>;
  border-color: <plugin accent>; }
.settings-toggle:focus-visible { outline: 2px solid <plugin accent>; outline-offset: 2px; }
```

**2. Markup** — a second row inside `#settings-popover`, immediately after the
language `<label class="settings-row">`. A `<div>`, **never** a
`<label for="tips-toggle">`: a button is a labelable element, so a label wrapping
one re-dispatches the click and toggles twice.

```
<div class="settings-row">
  <span class="settings-label" data-i18n="label.hoverHelp" data-label="Hover help">Hover help</span>
  <button type="button" class="settings-toggle" id="tips-toggle"
          aria-pressed="true" data-i18n-aria="aria.helpToggle"
          aria-label="Toggle hover help" data-i18n="ui.on" data-tip-always>On</button>
</div>
```

`data-label` is required — a shared state updater writing `textContent` erases an
HTML label without it (`pattern_js_state_updater_overwrites_html_labels`).

Also add `data-tip-always` to `#gear-btn`. Those two controls are the ones that
reach and restore the help layer, so they keep explaining themselves when it is
off. Leave `#lang-select` without it (it is only reachable through the gear).

**3. i18n keys** — four LABELS/aria keys plus one I18N tip entry. English and
French are settled suite-wide and are copied **verbatim**; they are the glossary
roots in `scripts/i18n-fr-glossary.js:79,80,101,102`:

```
'label.hoverHelp': { en: { t: 'Hover help' },        fr: { t: 'Aide au survol', reviewed: true } },
'ui.on':           { en: { t: 'On' },                fr: { t: 'Marche', reviewed: true } },
'ui.off':          { en: { t: 'Off' },               fr: { t: 'Arrêt',  reviewed: true } },
'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
```

and in `I18N`:

```
'tips-toggle': {
    en: { t: 'Hover Help',
          b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
    fr: { t: 'Aide au survol',
          b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
          reviewed: true },
},
```

plus `['#tips-toggle', 'tips-toggle'],` appended to `TIP_BINDINGS` (no wrapper —
the button is its own hover target).

`reviewed: true` is legitimate here: these exact French strings were read by the
developer in the Stage-N QA pass and are the glossary's roots. They are not new
machine output.

**4. JS** — the gate and the switch. Both live **outside** the
`applyI18n`/`initI18n` region, which check-i18n assertion [6] byte-compares
against `scripts/i18n-canon.js`; a character inside it fails the drift gate.

```
let tipsEnabled = true;          // shipped default; localStorage wins at boot

function applyTipsEnabled(on) {
  tipsEnabled = !!on;
  if (!tipsEnabled) hide();      // <- the renderer's own hide, by whatever name it has
  const btn = document.getElementById('tips-toggle');
  if (!btn) return;
  btn.setAttribute('aria-pressed', tipsEnabled ? 'true' : 'false');
  if (tipsEnabled) setLabel(btn, 'ui.on');
  else             setLabel(btn, 'ui.off');
}

function initializeTipsToggle() {
  const btn = document.getElementById('tips-toggle');
  if (!btn) { console.error('Missing tips-toggle element'); return; }
  let stored = null;
  try { stored = localStorage.getItem('<abbrev>.tipsEnabled'); } catch (e) { stored = null; }
  applyTipsEnabled(stored !== 'false');
  btn.addEventListener('click', () => {
    applyTipsEnabled(!tipsEnabled);
    try { localStorage.setItem('<abbrev>.tipsEnabled', String(tipsEnabled)); } catch (e) { /* private mode */ }
  });
}
```

`<abbrev>` follows the existing convention (`osfm`, `osg`, `opms`, `osadd`,
`osbm`, `ossub`) — pick the plugin's initials, e.g. `ochor` for O-Chorus.

Two calls behind an `if/else`, **never a ternary inside the `setLabel` argument**
— check-i18n assertion [13] rejects a conditional anywhere in that call.

The gate goes at the top of the renderer's `show()`:

```
if (!tipsEnabled && !el.hasAttribute('data-tip-always')) return;
```

`initializeTipsToggle()` is called at the very bottom, after `initI18n()`, inside
the same `try/catch` — it needs `setLabel`, and a top-level call reaching a lower
`let`/`const` is the TDZ throw that takes the whole inline module with it
(`pattern_module_toplevel_init_tdz`).

**5. Version + CHANGELOG.** New user-facing control = **minor** bump, matching the
prior art for exactly this control (O-Bitrot v1.12.0, "? toggle + hover help").
Bump `VERSION` in `plugins/<Name>/CMakeLists.txt` and add a dated `## [X.Y+1.0]`
section to `plugins/<Name>/CHANGELOG.md`.

**The UI is `juce_add_binary_data`, so nothing appears in the plugin until it is
rebuilt.** Every plugin touched needs `./scripts/build-and-install.sh <Name>`.

---

## GATES (all five run per plugin; exit 0 required)

```
node scripts/check-i18n.js      --plugin <Name>
node scripts/check-ui-labels.js --plugin <Name>       # exit 77 = NOTHING verified, never a pass
node scripts/boot-all-uis.js    --plugin <Name> --strict-tips
node scripts/i18n-fr-lint.js    --plugin <Name>
node plugins/<Name>/tests/ui_tip_render_check.js      # only where that file exists
```

`check-ui-labels` opens the gear popover (every plugin has
`plugins/<Name>/tests/i18n-states.json` with `{"click": "#gear-btn"}`), so the new
row **is** measured in every language. Its assertion 7 fails any element whose
rectangle differs between English and another language. If the French
`Aide au survol` pushes the row, use the glossary's shorter accepted forms —
`ui.on` → `Act.` / `ui.off` → `Dés.` (O-Lyrica) or `Activé`/`Désactivé`
(O-Polystutter) — and record the measurement in the CHANGELOG. **Do not widen the
popover to fit French without measuring English at the same width**: several
popover widths are hard-pinned precisely so French cannot move them.

Playwright resolves from the npx cache (`~/.npm/_npx/*/node_modules/playwright`)
via `scripts/serve-ui.js` — verified present.

---

<tasks>

<task type="tracer" tdd="false">
  <name>Task 1: O-Chorus end to end — the toggle, in three languages, built and installed</name>
  <files>
    plugins/O-Chorus/Source/ui/public/index.html
    plugins/O-Chorus/Source/ui/public/js/i18n.js
    plugins/O-Chorus/CMakeLists.txt
    plugins/O-Chorus/CHANGELOG.md
  </files>
  <read_first>
    plugins/O-Polystutter/Source/ui/public/index.html (lines 949-972 CSS, 1200-1210 markup)
    plugins/O-Gain/Source/ui/public/js/i18n.js (lines 236-246, 539-551, 655-665)
    plugins/O-simpleFM/Source/ui/public/js/app.js (lines 384-420)
    scripts/i18n-zh-glossary.js (lines 116-120, 157-158)
  </read_first>
  <action>
    Apply the five-step recipe above to O-Chorus, with the zh-Hans arm it alone needs.

    index.html — the file is 1423 lines and every insertion point is known:
      - CSS: add `.settings-toggle` (+ `[aria-pressed="true"]` + `:focus-visible`)
        after the `.settings-select:focus-visible` block near line 530. Palette:
        border `rgba(139, 115, 85, 0.4)`, accent `#3a5c4a`, fg `#3C2F2F`, chip
        `rgba(245, 230, 211, 0.97)` — the values the popover and gear already use.
      - Markup: insert the new `.settings-row` div immediately after the language
        `</label>` (currently line 834), before the `</div>` closing
        `#settings-popover`. Add `data-tip-always` to `#gear-btn` (line 804-806)
        and to the new `#tips-toggle`.
      - The popover pins `width: 170px` DELIBERATELY (comment at line 434-444):
        that pin is what makes the panel language-invariant. Do not change it.
        The panel opens UPWARD from a gear 5 px off the bottom of a 125 px frame
        and currently occupies y 59..99; a second row grows it by roughly one row
        plus the 6 px flex gap. MEASURE the new top edge against y=0 and record
        the number in the CHANGELOG. If it clears the frame, nothing else changes;
        if it does not, reduce the popover's `gap`/`padding` before touching the
        frame, and say so.
      - JS: add `tipsEnabled`, `applyTipsEnabled`, `initializeTipsToggle` after
        `setupTooltips()` ends (line ~1408) and before the bottom init block.
        localStorage key `ochor.tipsEnabled`. Gate `setupTooltips`'s
        `const show = (el, x, y) => {` (line 1293) with the
        `!tipsEnabled && !el.hasAttribute('data-tip-always')` early return.
        Call `initializeTipsToggle()` inside the existing bottom
        `try { initI18n(); setupTooltips(); }` — extend that one line; do not add
        a second try/catch, and do not move `initializeSettingsPopover()`.
      - Update the two stale comments that now assert the opposite of the code:
        line ~799 ("ONE row, because this plugin has no hover-help to toggle")
        and line ~1205 ("ONE row. This plugin has no hover-help to switch on or
        off"). Leaving them is a canon line three authors will reach around.

    js/i18n.js — add the four LABELS/aria keys and the `tips-toggle` I18N entry,
    each with a THIRD `'zh-Hans'` arm, since LANGUAGES here is
    `['en', 'fr', 'zh-Hans']` and check-i18n assertion [1] requires identical key
    sets across all three. The four short strings take their glossary ROOTS
    verbatim from scripts/i18n-zh-glossary.js — 悬停帮助 (hover help), 开 (on),
    关 (off), 开关悬停帮助 (toggle hover help) — at `reviewed: 'bt'`, because a
    root rendering settled in Stage 1 across 552 shared strings is exactly what
    the back-translation already covered. The tip BODY is new prose and is the
    only real authoring here: mirror the English at two sentences, obey Z1 (full
    width ，。 never ASCII), Z2 (NO U+00A0 anywhere), Z4 (one plain U+0020
    between every Latin/digit run and Han run) and Z7 (no full-width Latin or
    digits), and take 设置 for the gear. Then run
    `node scripts/i18n-zh-backtranslate.js` over the new entry BLIND (pass
    `--manifest` explicitly — the ingest path defect is a known open item and
    without it the provenance refusal runs inert) and mark it `reviewed: 'bt'`
    only if the reverse pass comes back meaning what the English says. If the
    back-translate tool cannot be driven cleanly, ship the entry at
    `reviewed: 'mt'` — assertion [5] accepts it — and note the gap in the
    CHANGELOG rather than claiming a review that did not happen.
    Append `['#tips-toggle', 'tips-toggle'],` to TIP_BINDINGS after the
    `#lang-select` row.

    CMakeLists.txt: VERSION 1.5.0 -> 1.6.0.
    CHANGELOG.md: new `## [1.6.0] - 2026-09-03` section above 1.5.0, recording the
    measured popover top edge, the French and Chinese renderings used, and the
    default-ON decision.

    Then rebuild and install per CLAUDE.md: `./scripts/build-and-install.sh O-Chorus`
    (its Phase 4 does the dev/release dual-variant sweep — do not hand-copy).

    DO NOT commit anything outside plugins/O-Chorus.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js --plugin O-Chorus && node scripts/i18n-fr-lint.js --plugin O-Chorus && node scripts/i18n-zh-lint.js --plugin O-Chorus && node scripts/boot-all-uis.js --plugin O-Chorus --strict-tips && node scripts/check-ui-labels.js --plugin O-Chorus && node plugins/O-Chorus/tests/ui_tip_render_check.js</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && grep -c 'id="tips-toggle"' plugins/O-Chorus/Source/ui/public/index.html | grep -qx 1 && grep -q "data-tip-always" plugins/O-Chorus/Source/ui/public/index.html && grep -q "'#tips-toggle'" plugins/O-Chorus/Source/ui/public/js/i18n.js && grep -q "VERSION 1.6.0" plugins/O-Chorus/CMakeLists.txt && echo WIRED</automated>
    <automated>auval -v aufx OuCh OuDv 2>&1 | tail -5 | grep -q "PASS" && echo AUVAL-PASS</automated>
    <human-check>Open O-Chorus in Logic. Gear popover shows two rows. Click Hover help -> it reads Off and knob tips stop appearing; the gear and the toggle still show theirs. Click again -> On, tips return. Switch to Français and to 简体中文 with help both On and Off: the caption and the button face are translated, the panel does not clip, and nothing on the page moves. Close and reopen the editor -> the toggle is where you left it.</human-check>
  </verify>
  <done>
    O-Chorus v1.6.0 is installed, auval-clean, and all six gates exit 0. The gear
    popover has a working, localized, persistent hover-help switch in en, fr and
    zh-Hans. Committed with `git commit -- plugins/O-Chorus`.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: port the toggle to the other 22 plugins — 22 edits, 22 builds</name>
  <files>
    plugins/{O-AnalogEQ,O-AnalogSaturation,O-Bass,O-Bassoon,O-Bells,O-Bowed,O-Comp,O-Detune,O-DigiDelay,O-Emulator,O-Formant,O-Freeze,O-GrainScatter,O-MicrotonalSampler,O-Prism,O-Reed,O-ReverseDelay,O-SimpleReverb,O-Texture,O-TextureForge,O-Tremolo,O-Wind}/{CMakeLists.txt,CHANGELOG.md}
    plugins/{…}/Source/ui/public/index.html or plugins/{…}/Resources/ui/index.html
    plugins/{…}/Source/ui/public/js/i18n.js or plugins/{…}/Resources/ui/js/i18n.js
    plugins/O-GrainScatter/Source/ui/public/js/app.js
  </files>
  <precondition>Task 1 is committed and its gates are green — the recipe is only proven once O-Chorus has shipped it.</precondition>
  <action>
    Apply the same five-step recipe to each of the 22 remaining rows of the scan
    table, en/fr only (none of them speaks zh-Hans — leave LANGUAGES two long).

    Work in batches of 4-5 plugins. After each batch: run the four per-plugin
    gates on each plugin in the batch, build and install each with
    `./scripts/build-and-install.sh <Name>`, `auval -v aufx <CODE> OuDv` each, and
    commit the batch path-scoped:
      `git commit -- plugins/<A> plugins/<B> plugins/<C> plugins/<D>`
    Never `git add -A`, and re-run `git branch --show-current` + `git status --short`
    immediately before each commit — another session shares this index and HEAD.

    Per-plugin specifics that are NOT mechanical, all measured at planning time:
      - O-GrainScatter: the renderer is js/app.js, not index.html. Follow
        O-simpleFM's app.js shape (the gate lives in its `show`/`showTooltip`),
        not O-Chorus's inline shape.
      - O-Prism: #settings-popover has NO .settings-row children — the label and
        select are direct children. Give the new control a `.settings-row` div and
        check the popover still lays out, or match the flat shape; either way run
        check-ui-labels before moving on.
      - O-ReverseDelay: its language caption key is `lang-select`, not
        `label.language`. Leave that key alone; the NEW keys still use the newer
        `label.hoverHelp` spelling.
      - O-Bells and O-SimpleReverb already define `ui.on`/`ui.off`. REUSE them.
        A duplicate key is a silent last-wins overwrite, and assertion [15] will
        not see it.
      - O-MicrotonalSampler has its own `<input type="checkbox">` controls. Do not
        reuse their classes; the toggle is a `<button class="settings-toggle">`.
      - O-Reed's tuning tab is a standing open defect. Touch only the settings
        popover and the tooltip renderer.
      - Every plugin's `.settings-toggle` colors come from that plugin's own
        palette. Do not paste O-Polystutter's greens into a plugin that has none.

    Default is ON in all 22, with localStorage persistence under
    `<abbrev>.tipsEnabled`. ON preserves each plugin's current observable
    behavior exactly; OFF would be a regression AND would make the render gates
    measure an empty tip surface.

    If any plugin's French pushes geometry, use the glossary's shorter accepted
    forms and record the measurement in that plugin's CHANGELOG. Do not silently
    widen a pinned popover.

    THIS IS 22 BUILDS. Each `build-and-install.sh` run is minutes, not seconds.
    Report progress after each batch and stop if a batch fails rather than
    carrying a broken recipe into the next four.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js && node scripts/i18n-fr-lint.js && node scripts/boot-all-uis.js --strict-tips</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && for p in $(ls plugins); do h=$(ls plugins/$p/Source/ui/public/index.html plugins/$p/Resources/ui/index.html 2>/dev/null | head -1); [ -z "$h" ] && continue; grep -q 'id="lang-select"' "$h" || continue; grep -qE 'id="(help-toggle|tips-toggle)"' "$h" || echo "MISSING TOGGLE: $p"; done; echo "scan done"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && for p in O-AnalogEQ O-AnalogSaturation O-Bass O-Bassoon O-Bells O-Bowed O-Chorus O-Comp O-Detune O-DigiDelay O-Emulator O-Formant O-Freeze O-GrainScatter O-MicrotonalSampler O-Prism O-Reed O-ReverseDelay O-SimpleReverb O-Texture O-TextureForge O-Tremolo O-Wind; do node scripts/check-ui-labels.js --plugin $p >/dev/null 2>&1 || echo "UI-LABELS FAIL: $p"; done; echo "ui-labels sweep done"</automated>
    <human-check>Spot-check three installed plugins from different families in Logic — O-GrainScatter (app.js renderer), O-Prism (flat popover) and one Resources/ui plugin such as O-Wind. In each: the toggle turns tips off, the gear still explains itself, French renders without clipping, and the state survives an editor reopen.</human-check>
  </verify>
  <done>
    The second verify command prints only "scan done" with no MISSING lines: all
    43 plugins with a language selector have a hover-help toggle. All 22 are
    version-bumped, CHANGELOG'd, built, installed and auval-clean, committed in
    path-scoped batches.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: check-i18n assertion [16] — a language selector implies a hover-help toggle</name>
  <files>scripts/check-i18n.js</files>
  <precondition>Task 2 is complete — adding this assertion before the ports would turn the repo-wide gate red for 22 plugins at once, which teaches the team to ignore it.</precondition>
  <action>
    Add assertion [16] to scripts/check-i18n.js using the existing
    `check(cond, desc)` helper (defined at line 139; assertions [14] and [15] near
    lines 912 and 1193 are the closest models).

    The rule, stated so it cannot go vacuous: for each plugin whose index.html
    contains `id="lang-select"`, that index.html must ALSO contain exactly one of
    `id="help-toggle"` or `id="tips-toggle"`, that element must be referenced by a
    TIP_BINDINGS selector, and the key it binds must exist in I18N. Three
    separate `check()` calls, not one — a single conjunction reports "failed"
    without saying which half.

    Both id spellings are accepted. The suite has 11 plugins on `#help-toggle`
    and 12 on `#tips-toggle`; a gate that demands one spelling would be a rename
    of 11 shipped plugins disguised as a lint, and this task is not that.

    Zero plugins with a language selector is not a legitimate zero here — every
    one of the 43 has one — so make the assertion report the COUNT it examined, in
    the same style as assertion [2]'s bound-count, and fail if that count is zero.
    A gate that passes because it found nothing to check is the failure mode this
    whole script exists to prevent.

    Document [16] in the numbered assertion list in the file header (currently
    ends at 15, around line 79) and update the "Fifteen assertions per localized
    plugin" line near line 30.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js; test $? -eq 0 && echo "43/43 PASS"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && cp plugins/O-Chorus/Source/ui/public/index.html /tmp/rjm-oc-backup.html && perl -pi -e 's/id="tips-toggle"/id="tips-toggle-BROKEN"/' plugins/O-Chorus/Source/ui/public/index.html && node scripts/check-i18n.js --plugin O-Chorus; RC=$?; cp /tmp/rjm-oc-backup.html plugins/O-Chorus/Source/ui/public/index.html; test $RC -ne 0 && echo "POSITIVE CONTROL FIRED (rc=$RC)" || echo "GATE IS VACUOUS — assertion 16 did not fire"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && git status --short plugins/O-Chorus | grep -q . && echo "DIRTY — restore failed" || echo "restored clean"</automated>
  </verify>
  <done>
    `node scripts/check-i18n.js` exits 0 across all 43 plugins with assertion [16]
    live, and the negative control proves [16] actually fires when the toggle is
    removed. O-Chorus's index.html is byte-identical to its committed state after
    the control. Committed with `git commit -- scripts/check-i18n.js`.
  </done>
</task>

</tasks>

<verification>
1. `node scripts/check-i18n.js` — 43/43, assertion [16] live, exit 0.
2. `node scripts/boot-all-uis.js --strict-tips` — 43/43, 0 DEAD bindings.
3. `node scripts/i18n-fr-lint.js` — exit 0.
4. `node scripts/i18n-zh-lint.js --plugin O-Chorus` — exit 0.
5. `check-ui-labels` exit 0 on all 23 touched plugins (exit 77 is NOT a pass).
6. The scan loop in Task 2's verify prints no MISSING lines.
7. Every touched plugin is version-bumped, CHANGELOG'd, rebuilt, reinstalled and
   auval-clean.
8. `git log --oneline` shows only path-scoped commits under `plugins/<Name>` and
   `scripts/check-i18n.js`. No `git add -A` anywhere.
</verification>

<success_criteria>
- O-Chorus has a working, localized, persistent hover-help toggle in three languages.
- All 43 plugins with a language selector have a hover-help toggle.
- check-i18n assertion [16] fails if that stops being true, and its positive control fired.
- No plugin's tooltip behavior changed for a user who never opens the gear: default is ON everywhere.
</success_criteria>

<risks>
- **Scope.** 23 plugins, 23 builds. This is not a five-minute task. Task 1 alone
  is the developer's stated ask; Tasks 2 and 3 close the class. If time runs out,
  the scan table above survives in this file and the work resumes from it.
- **Geometry.** Adding a row grows every settings popover. Most panels have room;
  the ones with hard-pinned widths and small frames (O-Chorus at 125 px, and any
  plugin whose CHANGELOG carries a popover width table) need the measurement
  recorded, not assumed.
- **The zh-Hans body** for O-Chorus's `tips-toggle` tip is the only genuinely new
  translation in the whole task. Everything else is a settled glossary root.
- **Concurrent sessions** share this checkout's index and HEAD. Re-check branch and
  staging immediately before every commit, not once at the start.
</risks>

<output>
Write `.planning/quick/260903-rjm-o-chorus-has-language-options-but-no-but/260903-rjm-SUMMARY.md` when done.
</output>
