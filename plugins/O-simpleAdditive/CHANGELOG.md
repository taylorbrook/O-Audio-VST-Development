# Changelog — O-simpleAdditive

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.3.0] — 2026-09-07

Simplified Chinese joins English and French. MINOR: 131 rows of new copy, a
third `LANGUAGES` member, a third `<option>` in the selector, a three-way
language codec in the C++, a CJK font tail and eleven geometry pins. No
parameter, range, type or state format changed, and no English or French
rendering moved.

This plugin is the TRACER for wave 4g, the seventh and last volume wave of the
rollout, and it was chosen for that because every structural question the other
five inherit is cheapest to answer here: 131 rows is the wave's smallest table,
two states its thinnest walk, `40 of 40` visible with zero never-visible the
only exact coverage fraction in the wave with no artefact to discount, and a
quoted `CMakeLists` literal the plainest of the wave's three version shapes.

**The version this bumps FROM is 1.2.1, not the 1.2.0 the registry says.** The
`PLUGINS.md` row is one patch stale — it never followed the 2026-09-03
suite-wide French hover-help rename that shipped 1.2.1. Every row in this wave
is stale in exactly that way and all six come from that one commit. The
registry is corrected at the end of the batch; the source of truth is and
remains `CMakeLists.txt`.

**This download ships two versions.** The last published tag was
`O-simpleAdditive-v1.2.0`; the binaries here carry **1.2.1 and 1.3.0** both.
1.2.1 (2026-09-03) is the suite-wide French hover-help rename — the caption
became `Infobulles`, every sentence was re-agreed from feminine singular to
feminine plural, and the bare back-references that carried no occurrence of the
old phrase were rewritten rather than regexed. Its own section is below, and the
release-notes extractor does not reach it.

### Added

- **Simplified Chinese (`zh-Hans`) across all 131 rows** — 43 tooltip titles,
  43 tooltip bodies and 45 captions — at `reviewed: 'bt'`.
- **The endonym in the language selector**, as the HTML entities the suite
  writes it with, copied byte-for-byte from the shipped precedent rather than
  retyped, and exempted with a reason like the other two.
- **A three-way `languageCode` / `languageIndex` codec** in
  `Source/PluginProcessor.h`. Pure ASCII on both lines: the persisted value is
  the BCP-47 code and nothing else, so no Chinese character reaches the header
  and nothing a text editor, a compiler or a session file could mangle is
  stored. Anything that is neither the French nor the Chinese code still
  degrades to English rather than being stored unvalidated.

### Changed

- **The language tooltip no longer ENUMERATES the languages on offer.** Through
  1.2.1 its English body ended *"English and French are available; value
  readouts and the two drop-down menus stay in English"*, and the French body
  said the same. That clause was true for exactly as long as the selector held
  two entries and became false the moment this version added a third. It is
  deleted rather than extended: the selector already lists the languages in
  their own endonyms, which is the one place the list cannot go stale. **The
  exception list stays, and its numeric claim was re-verified before it was
  kept** — `index.html` carries three `<select>` elements and subtracting the
  language selector leaves two, so *"the two drop-down menus"* is true today.
  The superseded phrasings are recorded here and deliberately not repeated in
  any source comment, so a repo grep for either stays at zero.
- **The CJK font tail on both declared stacks**, before the trailing generic —
  Chromium resolves a bare `serif` against the document's `lang`, so under
  `zh-Hans` the generic is already a Chinese face and a tail written after it is
  never consulted. The absent Garamond members are KEPT: they are the
  Windows/print intent and this machine is not the one the suite builds for.
  `--symbol-font` takes the tail too, and that is measured rather than assumed:
  its only consumer is the gear button, the gear button is the first tip anchor
  in the table, and a tip anchor carries Han in `data-tip` regardless of the
  glyph it paints.
- **Eleven geometry pins under a new `zh-Hans GEOMETRY PINS` block** in
  `css/styles.css`. Han faces carry taller metrics than Times New Roman, so
  every element inheriting the UA's `line-height: normal` grew 2-4 px the moment
  its caption became Chinese, and the header and group headers are in normal
  flow, so that growth pushed the page down. Eight leaf ratios, two row ratios
  and one width floor. **Every ratio is measured, not chosen**: the element's
  own English line box — rect height less padding less border — divided by its
  own font-size, unitless, grouped per measured box. The lesson buttons sit
  apart from the lesson label at half a pixel of font-size because a `<button>`
  is a form control whose UA `font` shorthand resets `line-height`, so its ratio
  comes from its own content box and not from the leaf table. **There is no
  global `line-height`**: a global rule would move English, which is the
  regression these gates exist to catch rather than a fix.
- **A width floor on the tour caption at 311.52 px**, its exact measured English
  box. Chinese is shorter, so the failure shape on this page is a SHRINK: the
  Chinese caption is 244.34 px, short enough to fit on the flex row's first line
  beside the six lesson buttons, which collapsed the section from two flex lines
  to one and took the button group from 679 px to 422.66 px with it. The floor
  restores the wrap. Not a fixed width — French is 395.83 px and must stay wider
  than the floor.

### Disclosed quality level

The Chinese ships at **`reviewed: 'bt'`** — back-translated, not natively
reviewed. This project has no native Simplified Chinese reader, so the French
lane, where `reviewed: true` means the developer read it, is closed. The
substitute is a blind reverse pass: every one of the 131 rows was emitted with
the English source and the key names WITHHELD and the row ids BLINDED behind a
per-batch salt, dispatched to a separate model in a fresh session with no tools
from a working directory outside this repository, and the returned English was
read against the original in every row.

`reviewed: 'native'` stays open and is not a blocker.

The batch was split into two chunks **on the concept, not on size**: every
caption went to one reader and its own tooltip title to the other, so a
divergence between the two halves of one control is visible to a reader rather
than to nobody. **That split paid for itself on this page.** The Chinese for
*Harmonic Drawbars* came back as *"Harmonic drawbar"* from the chunk that held
the tooltip titles and as *"Harmonic sliders"* from the chunk that held the
captions — one string, two independent readings, and only the split could
produce both. The pair is what shows the word is right and that the second
reader simply had no organ context around it.

**Two rows were re-authored and a second round confirmed both**, against a
fresh reader on a third model with a fresh salt. The discriminator is collision
on the page, never drift distance:

- the oscilloscope hint came back as *"Sum of single-cycle waveformS"*, plural,
  while the caption on its own row reads *Waveform*, singular, and the scope
  draws one trace. Re-authored so the summing is the operation and the waveform
  is the result — the direction the English states it in. It now returns as
  *"post-summation single-period waveform"*.
- the **Organ** lesson came back as *"Pipe organ"* while that entry's own body
  says a Hammond-style drawbar registration. A Hammond is an electric organ; a
  pipe organ is not a Hammond and has no drawbars at all — a title and its own
  body naming two different instruments, on one control. The glossary root is
  the pipe-organ word, and the corpus site count for it was checked before
  anything was written: *Organ* is a caption on exactly ONE plugin — this one,
  in three entries — and that root is shipped nowhere else, so it was derived
  from this caption alone and never had a second site to check it against.
  O-Bells independently confirms what the root means: its own copy uses that
  word to name the PIPES of one. Corrected to the electric-organ word at the
  three places it exists, each with its own entry-scoped note, and the root is
  flagged for the glossary rather than forked silently. It now returns as
  *"electronic organ"* on all three.

Round two corrected nothing further, so there is no round three.

**Six glossary exemptions in total, all reasoned and all entry-scoped**, three
for **Organ** above and three for **Morph Pad**, whose root renders as a morph
PANEL — a user-interface surface — where the English names a synth PAD. That
root has the same single-site provenance and the same disposition.

**Expect Chinese line boxes about 30% taller than Latin at the same font-size.**
That is what the pin block above exists for, and it is why a Chinese caption
that measures narrower can still move a page.

### Verified

- `check-i18n` exit 0: `LANGUAGES` lists three, the key counts are unchanged at
  43 + 45, and all 43 tips stay bound.
- `i18n-zh-lint` 0 findings, `--self-test` 10/10, 0 entries below the ship bar.
- `i18n-fr-lint` 0 findings — no French rendering changed.
- `check-ui-labels` exit 0, `ALL CHECKS PASSED`: **no non-label element moved on
  either non-English arm**, across both states, and coverage is unchanged at
  `40 of 40` visible with zero never-visible.
- `measure-ui`: `undeclared-font` 0 against a NON-EMPTY input of 111 visible
  Han-bearing nodes — a measurement rather than the vacuum the same screen
  reported before the table landed. `wrap-count` 0 against a baseline of 0.
  One `line-height: normal` residual survives and is named with its
  measurement: the hover-help switch, font-size 10 px, English and Chinese
  heights both 24 px. It is a fixed-height form control and cannot move.
- The state-effect assertion fired on **both** click states: the hit test at
  each target's centre returns the button itself, and each state measurably
  took effect — the popover raised the visible label count from 37 to 40, and
  the switch's face went from its on-word to its off-word.
- Zero Chinese characters anywhere under `Source/`, proved on a comment-stripped
  copy with a positive control fired on the same run.
- `boot-all-uis --strict-tips`: clean, 0 dead bindings and 0 late.


## [1.2.1] — 2026-09-03

The French rendering of the hover-help surface changes suite-wide (task
260903-ukp; O-Gain 1.3.3 was the tracer). PATCH: French strings and source
comments only — no parameter, range, type or state format changed.

### Changed

- **The French caption is now `Infobulles`** (feminine plural). The superseded
  rendering named the ACTION — help on hover; *infobulle* is the noun French
  DAW and OS interfaces use for the surface itself. The glossary root moved
  with it, ROOT-ONLY: `scripts/i18n-fr-glossary.js` now reads
  `'hover help': ['infobulles']` and
  `'toggle hover help': ['activer ou désactiver les infobulles']`, with the old
  rendering REMOVED rather than kept as an accepted alternate — so a plugin
  drifting back is a red G1 gate, not a silent pass.
- **Every sentence re-agreed from feminine singular to feminine plural**, not
  substituted: `cette …` → `ces infobulles`, `l’…` → `les infobulles`,
  `de l’…` → `des infobulles`, `toute l’…` → `toutes les infobulles`,
  `Une fois désactivée` → `Une fois désactivées`; the distributive `chaque …`
  → `chaque infobulle` is the one place the new term stays singular.
  Bare back-references that carried no occurrence of the old phrase — clauses
  reading *le réglage de l’aide*, *l’état de l’aide*, *son affichage ou non*,
  and the pronouns in *Lorsqu’elle est désactivée … la réactiver* — were
  rewritten too. A regex pass would have left every one of them pointing at an
  antecedent that no longer exists.
- Every changed body was read by the developer at a blocking checkpoint
  *before* it was written, so each ships `reviewed: true` legitimately and the
  repo-wide unreviewed-French TOTAL stays at 0.


## [1.2.0] — 2026-09-01

### Added
- **The hover-help switch, in the settings gear beside the language selector.**
  Through v1.1.2 the gear panel held the language row alone — its comment said
  a toggle would be a control for a preference that does not exist, because the
  tooltip engine had no enabled state at all. The suite's other settings panels
  (O-simpleGrain, O-simpleSampler) offer the switch, and a panel that looks
  like theirs but lacks it reads as a missing control. Same port as O-simpleFM
  v1.4.0, same commit: a `.settings-toggle` button (`#help-toggle`,
  `aria-pressed`) whose face is written through `setLabel` (`ui.on` /
  `ui.off`, two calls behind an if/else — check-i18n assertion 13);
  `applyTipsEnabled()` / `setupTipsToggle()` in `app.js`; `show()` returns
  early when `tipsEnabled` is false, and `hideTooltip` is published so
  switching off dismisses a tip already showing. Browser-side preference under
  localStorage `osadd.tipsEnabled` (default on) — no C++ state, no bridge, no parameter.
  A `<div class="settings-row">`, not a `<label for>`: a button is labelable,
  and a label wrapping one re-dispatches the click and toggles twice.
- **Copy.** `help-toggle` (tip), `aria.helpToggle`, `ui.on`, `ui.off` — the
  French is O-simpleGrain's, byte-identical to entries already `reviewed: true`
  there, and carried as reviewed here on that basis. The `gear-btn` tip body
  now names both controls in the panel.
- **Styling** stays in this plugin's own vocabulary: the selector's box beside
  it at rest, the gear's `--btn-active` / `--btn-border-active` lit state when
  pressed.
- `tests/i18n-states.json` gains the "hover help switched OFF" state so the
  `ui.off` face is measured by check-ui-labels in both languages.

### Validation
- check-i18n: 15/15 PASS, 0 / 88 entries unreviewed (43 tooltip, 45 label).
- i18n-fr-lint: CLEAN, exit 0.
- boot-all-uis `--strict-tips`: clean, 0 dead / 0 late tip bindings.
- check-ui-labels: ALL CHECKS PASSED; 40 of 40 `[data-i18n]` elements visible.
- Built + installed (VST3 + AU). WebView-only change — no DSP, parameter, or
  state-format change.

## [1.1.2] — 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide
i18n rollout.

### Fixed
- **item 58 — hover help opened on a pointer click:** `setupTooltips()`
  (`js/app.js`) opened the tip on *any* `focusin`, so clicking the gear, the
  language selector, either combo or a lesson button — the ten anchors the
  browser focuses on click — showed the help the same press had just hidden.
  The Stage M focus latch is ported from O-Comp v1.7.0: `pointerdown` latches,
  any `keydown` releases, `focusin` opens only while released, `focusout` and
  Escape hide. Keyboard focus (Tab / Shift+Tab) still opens the tip, in both
  languages, with the anchor's own body. The one programmatic `.focus()` on
  the page (popover Escape → gear) is covered by the same rule.
- **item 58, second mechanism — the sixteen drawbars re-opened their tip on a
  press:** the press hid the tip, then `setFromY` grew `#fill-partialN` under
  the pointer and the child boundary's `pointerover` re-opened it through the
  hover path. The pressed anchor is now remembered and not re-opened until the
  pointer has left it — the behaviour the knobs already had (their stem is
  `pointer-events: none`, so nothing changed under the pointer).
  Measured with a scratchpad Playwright probe over every focusable `[data-tip]`
  anchor: click opens a tip on 26 of 41 before (10 focus, 16 hover) → 0 after;
  Tab into Scan LFO Rate opens its tip inside the frame with the matching body
  before and after; Escape hides. No copy, CSS or DSP change (render-harness
  golden byte-identical).

## [1.1.1] — 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **52 of the 84 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint: 58 findings → 0.
  - **Typography** throughout — straight apostrophes became typographic ones
    (`l'interface` → `l’interface`), and a no-break space now sits before every
    `%`, `:` and `;` and inside the guillemets around « Off », so a French
    sentence can no longer break in front of its own punctuation.
  - **Terminology**, where the suite had settled on another word: the two Scan
    LFO captions are now VITESSE DU LFO and PROFONDEUR DU LFO (was AMPLEUR
    LFO — *ampleur* was the minority reading of "depth"), BIT DEPTH is
    RÉSOLUTION, RELEASE is RELÂCHEMENT in full, the lesson row reads LEÇONS,
    and the four amplitude-envelope tooltips are titled "Attaque d’amplitude"
    rather than "Attaque ampl.". Scan is named *Balayage* in every caption,
    title and heading on the page.
  - **Five sentences whose meaning had drifted from the English**: both Scan
    LFO tips had dropped the control they drive ("qui balaie automatiquement"
    said nothing about Scan), the Sawtooth lesson said "des harmoniques" where
    the English says *all* overtones, and the oscilloscope hint had lost
    "single-cycle".
  - Root terms fit here: VITESSE DU LFO wraps to two lines inside a caption box
    that already reserves two, PROFONDEUR DU LFO (69.36 px) and RELÂCHEMENT
    (77.33 px) clear their neighbours in a 56–58 px cell, and no non-label
    element moves between English and French — unchanged from v1.1.0. The one
    abbreviation kept for width is DÉCROISS. SPECTRALE, which the glossary
    accepts.
  - Four captions are now substrings of the accessible name they label
    (WCAG 2.5.3 label-in-name): LFO Rate, LFO Depth, Bit Depth and Release.
  - **All French is still machine-drafted and flagged `reviewed: false`.** This
    was a second reading against a glossary and a lint, not a native review.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

## [1.1.0] — 2026-08-27

### Added
- **The interface speaks French.** A gear in the header opens a settings panel
  holding a language selector (English / Français). Every control caption,
  section heading, button label, hint and hover tooltip switches with it, with
  no reload. The choice is remembered with the session.
  - The panel holds the selector **alone**. This plugin has no hover-help toggle
    and never had one — its help layer is always on — so a toggle row here would
    be a control for a preference that does not exist.
  - Value readouts stay English (`250 ms`, `0.50 Hz`, `0.0 dB`), and so do the
    Frame B and Bit Depth menu entries: those come from the C++
    `AudioParameterChoice` and are the host's automation-lane contract.
  - The language rides the APVTS tree as a plain property (`uiLanguage`, stored
    as the string `"en"`/`"fr"`), not as a parameter — it must not appear in a
    DAW automation lane, and a lesson preset must not be able to change which
    language somebody reads their interface in. A pre-1.1.0 session has no
    property and defaults to English; the restore is gated on `isVoid()`,
    because the ValueTree→XML round-trip rebuilds every property as a string var
    and a type predicate would never fire.
  - **All French is machine-drafted and flagged `reviewed: false`.** 84 entries
    (42 tooltip, 42 label). No native speaker has read it;
    `node scripts/check-i18n.js` prints the worklist.
- `Source/ui/public/js/i18n.js` — the copy table for both languages, embedded as
  BinaryData and served from the editor's resource provider.
- `tests/i18n-states.json` — drives the settings popover open so the render gate
  measures the two labels inside it. Without it 37 of 38 labels were measured
  and the 38th was reported as an unmeasured coverage hole.

### Fixed
- **"The 2th harmonic" / "The 3th harmonic"** in the drawbar tooltips. The
  per-partial tips were generated by a helper that built its ordinal as
  `${k}th` with no special-casing, so partials 2 and 3 read wrong in every
  build since the tooltips shipped. They now read "2nd" and "3rd". The sixteen
  tips are written out in `js/i18n.js` rather than generated: a generated string
  is invisible to the translator reviewing the file. The other fourteen were
  compared back to the generator's output byte-for-byte and are unchanged.

### Changed
- **Tooltip copy moved out of `js/app.js`** into `js/i18n.js`, and the renderer
  now reads the anchor's own `data-tip-title` / `data-tip` attributes, which
  `applyI18n` rewrites in the current language. Through v1.0.7 the copy lived in
  a `TIPS` object and each anchor carried the tip KEY in its own `data-tip`;
  that key and the localized copy would have fought over one attribute, so the
  anchors moved to `data-param` (naming the APVTS parameter each drives), an id,
  or the `data-preset` the lesson buttons already carried.
- The tooltip is built with `createElement` + `textContent` instead of
  `innerHTML`. The tip text is table-sourced now rather than a fixed literal.
- The tooltip listeners are **delegated on the document** rather than attached
  per element. The sixteen drawbar cells do not exist until `buildDrawbars()`
  runs and no anchor carries `data-tip` until `applyI18n` has run, so a
  `querySelectorAll` at setup time would bind whatever happened to exist at that
  instant.
- The six lesson tooltips and the six tour captions are now twelve authored
  table entries. v1.0.7 derived each tooltip from its caption by splitting on
  the first " — " at load time; a derived string cannot be reviewed by a
  translator, and once the page has two languages the table is the single source
  of truth rather than the caption.
- The tour caption is written through `setLabel()`, so the element becomes a
  `[data-i18n]` element and the language sweep owns it. It is the one string on
  this page chosen by a click, and as a raw `textContent` write it would have
  been stranded in whichever language was active when the lesson was picked.
- Knob and combo accessible names come from `data-i18n-aria` and resolve to the
  control's tooltip title, so they switch language with everything else. The two
  combo boxes read "Frame B Source" and "Bit Depth" where they read "Frame B
  source" and "Bit depth" before — one string for the control's name instead of
  two spellings of it. The sixteen drawbars read their partial's tip title
  ("Partial 3 · odd harmonic") where they read "Partial 3 level".

### Layout — four language-geometry fixes, each with its measured before/after
`node scripts/check-ui-labels.js --plugin O-simpleAdditive` renders the page at
the shipping 860×930 frame in both languages and reports every element that is
not a label but moved. It named 34. The frame is a locked decision and did not
move; D-04 forbids auto-shrink fonts and short-variant fallbacks, so each fix is
a container pin or a reserved line. Every one was reverted alone afterwards and
the gate re-broke — a fix that passes both ways is decoration.

1. **`.title-block { flex: 1 1 auto; min-width: 0 }`.** The block shrink-wrapped
   its widest child, the subtitle, so French made it **131.6px wider (397.1 →
   528.7)** and dragged the `<h1>` box with it. It now takes the header's slack
   and measures 792 in both. *Negative control: removing it reports the same
   `dw=131.6` on both elements.*
2. **`.knob-label { min-height: 2.2em }`** — two lines reserved in both
   languages. "LFO Rate" (48.9px) and "LFO Depth" (56.3px) fit the 58px cell on
   one line; "Vitesse LFO" (64.9px) and "Ampleur LFO" (72.3px) do not, so those
   two cells grew **10.4px** in French and pushed `#val-scanLfoRate` and
   `#val-scanLfoDepth` down by exactly one line-height. Reserving the line costs
   group-row 1 **nothing** — it is already 129.9px tall in both languages
   because "Spectral Decay" wraps — and group-row 2 **10.5px** (111.4 → 121.9).
   The keyboard's bottom edge moves 879.3 → 889.8 in a 924px usable frame, so
   22px of slack remain and the page still does not scroll.
   *Negative control: removing it reports `dh=10.4` on both cells and
   `dy=10.4` on both readouts.*
3. **`.group-row > .group-env { flex: 1 1 0 }`** — flex-basis 0, not auto. Each
   group's share of row 2 was seeded by its own max-content width, which its
   title sets: "Modulation Envelope · → Scan" grows more in French than
   "Amplitude Envelope" does, so the amplitude group **lost 12.5px** to its
   neighbour (338.7 → 326.1 against 338.7 → 363.8) and dragged four knobs
   6.3px left. Both are 338.5 in both languages now.
   *Negative control: reverting the basis to auto reports `dw=-18.8` and 36
   moved elements.*
   The companion `.group-row > .group-output { flex: 0 0 123px }` is a
   **design pin, not a geometry fix, and is recorded as such**: the gate passes
   without it. What the gate needed was the child-combinator selector, because
   the authored `.group-output { flex: 0 0 auto }` had been losing on
   specificity to `.group-row > .group { flex: 1 1 auto }` since v1.0.6 and the
   group had silently been growing. Restoring `0 0 auto` alone collapses the
   group to its 82px contents and wraps "Output" onto two lines; the 123px pin
   holds its shipped English box (measured 122.7).
4. **`.tour-label { flex: 0 0 99px }` + `.tour-buttons { flex: 1 1 auto }`.**
   The label is the only item before the button group in that flex row, so its
   width alone sets where the buttons start, and the group then shrink-wrapped
   six captions that are 386.6px of text in English and 428.3px in French —
   `dx=80.3 dw=41.7`. 99px is the English caption's own measured width (98.8)
   rounded up, so the English row is unchanged to within 0.2px.
   *Negative controls: removing the label pin reports `dx=-15.0 dw=15.0`;
   removing the button-group grow reports `dw=41.7`.*
   The French label was authored to fit inside that box. "Préréglages
   pédagogiques" is the faithful rendering and measures **179.1px** against the
   English 98.8px; pinning to it instead would leave an 81px hole beside the
   English caption, changing a shipping layout for a language nobody has
   selected. "Préréglages" is 83.8px and the dropped "lesson" survives in every
   button's own hover help. *Negative control: restoring the longer string
   reports 42 moved elements and pushes the on-screen keyboard down 1px.*

### Testing
- `node scripts/check-i18n.js --plugin O-simpleAdditive` — exit 0, canon v2.
- `node scripts/check-i18n.js --strict-v2` — exit 0; 8 plugins on canon v2, 0 on v1.
- `node scripts/check-ui-labels.js --plugin O-simpleAdditive` — ALL CHECKS PASSED
  in both states, **zero** non-label elements moved between English and French,
  the vacuity guard confirming French rendered (38/38 labels and 37/37 keyed
  attributes differ), `dataset.label === textContent` holding after init, after
  the switch and after a state-update pass driven through the stub's slider
  states, and 38/38 labels measured.
- `node scripts/boot-all-uis.js --plugin O-simpleAdditive` — boots clean, no
  page error, 38 `[data-i18n]` elements, 37 aria-labels, 0 native `title=`.
- Render harness: ALL PASS, with v1.0.7's numbers unchanged (makes-sound
  rms=0.4454, pitch a_f0=0.6299, morph H2 0.00000→0.09588, static-deterministic
  n=308700, motion-finite peak=3.0038, noteoff-click preRms=0.4144 /
  tailRms=0.2814 / maxJump=0.0354).
- Six negative controls run against a byte-exact backup and restored from that
  backup — never `git checkout --`, which would have wiped the uncommitted work
  alongside the mutation. Five fired; the sixth (`.group-output` width) did not,
  and is recorded above as a design pin rather than claimed as a geometry fix.
- `auval -a` lists `aumu OSiA OuDv`.

### Not verified
- **No native French speaker has read any of the 84 entries.** Every one carries
  `reviewed: false`.
- **Windows.** WebView2 font metrics differ from WebKit's, so a French caption
  that fits on macOS may wrap or clip there. Every measurement above is from
  headless Chromium on macOS. Carried as the same named hardware deferral the
  repo already records for `octagon-windows-vst3`.
- The DAW session round-trip (pick Français, close, reopen) is written and its
  gate is the C++ read/write pair; it has not been exercised by hand in a host.

## [1.0.7] — 2026-08-25

### Fixed
- **Clicks on note-off, at any settings** (ported from O-simpleFM v1.2.5; found
  by a suite-wide sweep of the per-block ADSR push pattern). Root cause: the
  processor pushed ADSR parameters into the live `juce::ADSR` amp + mod envelopes every
  block via `setParameters()`, whose `recalculateRates()` recomputes the release
  slope from the SUSTAIN level — clobbering the envelope-value-based rate that
  `noteOff()` had just computed. With sustain = 0 the recomputed rate is 0, and
  `recalculateRates()` treats a zero-rate release as finished: it hard-resets the
  envelope one block after every note-off, truncating the ringing tail to
  silence instantly — the click. (JUCE's ADSR docs explicitly forbid changing
  parameters during playback.)
  Fix in `AdditiveVoice.h`: envelope params are cached each block but only pushed to
  the live envelope(s) when their values actually change AND the voice is not in
  its release phase; changes made mid-release apply at the next note-on. The
  release therefore always completes at the rate captured at note-off.
- Render-harness: new `noteoff-click` probe (sustain 0, slow decay, note-off
  mid-decay) asserting the release tail still rings after note-off.

### Testing
- Render harness: ALL PASS including the new probe (preRms 0.4144 / tailRms 0.2814 / maxJump 0.0354).
- Negative control: probe re-run against v1.0.6 voice code fails as expected
  (preRms 0.4144 / tailRms 0.0000 / maxJump 0.0380 — the tail is truncated to exact silence one block after note-off).

## [1.0.6] — 2026-08-17

> **Supersedes v1.0.5, which was never released publicly.** This is the first
> public release since v1.0.4, so it carries both versions' work.
>
> **From v1.0.5** — the whole interface, including the on-screen keyboard, now
> fits the 860×980 editor without scrolling. Previously the frame needed 1251 px
> of content in 974 px of usable height, a 277 px overflow that pushed the
> lesson-preset row and the entire keyboard panel below the fold and clipped the
> Output group mid-knob. The four control groups moved from vertical stacks into
> two horizontal rows, reclaiming 295 px; nothing that carries the design got
> smaller (drawbars keep their full 168 px travel, knobs their 56/48 px
> diameters, and the keyboard is *taller* at 96 px). Full detail in the v1.0.5
> entry of `plugins/O-simpleAdditive/CHANGELOG.md`.

Follow-up to v1.0.5: the oscilloscope trace was drawing off-centre, and the scope
box was taller than a single-cycle waveform needs. No parameter, preset, state, or
DSP changes.

### Fixed
- **The waveform drew in the top quarter of the scope box instead of centred.**
  `makeCanvas` sized the canvas backing store once at boot and `rewireResize`
  refreshed it only on a **window** resize — but the canvas box also changes when
  the layout settles or the frame reflows, neither of which fires that event. The
  store got pinned to a transient boot height of 373 px while the box settled to
  178 px; the browser then squashed that store into the box, so a trace drawn at
  `h / 2` landed at **23.9%** of the box. Measured before and after: 23.9% → 49.7%
  (the residual 0.5% is the 2 px stroke's half-pixel on a 96 px canvas).

  The v1.0.5 elastic scope is what made the boot-vs-settled gap large enough to
  see, but the boot-once sizing was the underlying defect and was always fragile.

  Fixed by observing the **canvas element** with a `ResizeObserver` rather than the
  window, so the backing store and the drawing transform are always recomputed
  together against the box's real size. `resize()` now returns whether anything
  actually changed, because assigning `canvas.width` clears the canvas even when
  the value is identical — a no-op resize must not repaint. The window listener is
  kept for the one case the observer cannot see: a `devicePixelRatio` change, which
  alters the required store without changing the element's CSS size. A stale DPR is
  self-consistent rather than broken — the store and transform are set atomically,
  so the trace stays correctly placed and only render resolution is affected.

### Changed
- **The scope is a fixed 116 px again, not elastic.** Letting it absorb the frame's
  leftover slack (v1.0.5) stretched it to 198 px, which read as a thin trace
  stranded in a tall empty box. A single-cycle waveform needs far less height than
  the drawbars do.
- **Editor height 980 → 930.** With the scope fixed and shorter, content measures
  892 px in 924 px of usable frame height, so the window gives back 50 px of screen
  rather than holding empty paper. Headroom is 32 px — still an order of magnitude
  more than the ≤2 px the layout shifts across serif fallbacks.

### Testing
- Trace centring measured by reading canvas pixels, not by re-deriving the maths:
  the green stroke's vertical midpoint sits at 49.7% of the backing store. The same
  probe reported 23.9% against the pre-fix build, so it discriminates.
- Backing store verified to refit after a live viewport change, with the trace
  repainted and still centred.
- Layout at 860×930: 892 px content in 924 px usable, **0 px overflow**, 32 px
  headroom, keyboard fully visible, no knob-row wrapping.

## [1.0.5] — 2026-08-17

UI layout pass: the whole interface — including the on-screen keyboard — now fits
the 860×980 editor without scrolling. No parameter, preset, state, or DSP changes;
this is presentation only.

### Fixed
- **The on-screen keyboard was entirely below the fold and required scrolling to
  reach.** Measured at the shipped 860×980 editor size, the `.frame` scroll
  container needed 1251 px of content in 974 px of usable height — a 277 px
  overflow that pushed the lesson-preset row and the whole keyboard panel out of
  view, and clipped the Output group mid-knob.

  Root cause: the four control groups (Morph · Wavetable, Spectral Shaping, the
  envelope pair, Output) were stacked vertically, so `.controls` alone consumed
  542 px — more than half the window — while each group left most of its width
  empty. The single-knob Output group cost ~130 px of height to show one control.

  Fixed by laying the groups out in two horizontal rows rather than four stacks,
  which reclaimed 295 px of the 277 px needed. The reclaimed space was then spent
  back on the elements that carry the design, so nothing that matters got smaller:
  the drawbars keep their full 168 px travel, the knobs their original 56/48 px
  diameters, and the keyboard is now *taller* than before (96 px, was 92 px).

### Changed
- **Control groups now sit in two rows** (`.group-row`): Morph · Wavetable beside
  Spectral Shaping, then Amplitude Envelope · Modulation Envelope · Output. The
  former `.env-pair` rule is generalised into `.group-row` and the standalone
  Output section is folded into the second row as a narrow third panel. `.controls`
  drops from 542 px to 247 px.
- **The oscilloscope is now the single elastic section** (`flex: 1 0 136px`). It
  absorbs whatever vertical slack the frame has left after every other section
  takes its natural size, so the keyboard lands on the bottom edge instead of
  floating above dead paper — 136 px at minimum, 198 px at the 980 px editor
  height. `flex-shrink` stays 0, so 136 px is a hard floor: a host that renders
  text taller makes the scope grow less rather than clipping anything.
- **Padding and gaps tightened** across the frame, panels, group headers, and the
  preset-tour and keyboard footers (roughly 2–4 px each). The lesson-preset
  caption's `max-width` went 46% → 54% so it stops wrapping onto a second line.
- Header title 26 px → 25 px; combo boxes 40 px → 36 px tall.

### Testing
- Layout measured in headless Chromium against the real `index.html` / `styles.css`
  / `app.js`, with only the JUCE ES-module namespace stubbed. At an 860×980
  viewport: content height 912 px in 974 px usable, **0 px overflow**, 62 px of
  headroom, no knob-row wrapping, lesson-preset buttons on one line, keyboard
  fully within the viewport. The same probe reported the 277 px overflow before
  the change, so it discriminates.
- Font-fallback sweep (Garamond stack, Times New Roman, Georgia, generic serif,
  sans-serif) moves total height by ≤ 2 px — the layout is dominated by fixed-px
  elements — so headroom holds at ≥ 59 px whichever serif the host's WebView
  resolves.
- Short-window degradation checked at 860×720: the scope floors at exactly 136 px,
  the frame falls back to scrolling, and the keyboard stays reachable. Nothing
  clips.

## [1.0.4] — 2026-07-15

Code-review resolution pass, part 2: the six Info findings (IN-01..IN-06) deferred
from the v1.0.3 pass (CODE_REVIEW.md 2026-07-15). All are hardening/cleanup — no
audible change at standard sample rates, no parameter or state changes.

### Fixed
- **IN-03 — Nyquist edge: a fundamental at/above Nyquist now renders silence, not
  aliasing.** `computeKmax` clamped the harmonic count to a minimum of 1, so when
  `f0 ≥ 0.5·fs` (only reachable at sample rates below ~25 kHz) the fundamental was
  written above Nyquist and aliased. Root cause: `jlimit (1, …)` forbade the
  legitimate `Kmax = 0` case. Now clamped to `[0, 16]`; `nyquistGain`'s `k > Kmax`
  check already zeroes every partial at `Kmax = 0`, so `refillTable` naturally
  produces a silent table. Unreachable at 44.1 kHz+; closes the one hole in the
  otherwise-exact band-limit.
- **IN-04 — `uiMidi` native boundary now validates its arguments.** `handleUiMidi`
  built MIDI messages from the raw bridge int (out-of-range → JUCE `jassert` /
  malformed message) and a NaN velocity passed through `jlimit` (NaN comparisons
  are false). Note number is clamped to 0–127 and non-finite velocity falls back
  to 0.8 before the message is queued.
- **IN-05 — `midiCollector` given a valid timestamp base at construction.** The
  collector was only `reset()` in `prepareToPlay`; on-screen-keyboard messages
  queued before the host's first prepare (Standalone startup, editor on a
  suspended plugin) hit an unreset collector (debug assertion, undefined
  timestamp base). Constructor now seeds `reset (44100.0)`; `prepareToPlay`
  re-resets with the real rate.
- **IN-06 — knobs and drawbars now publish ARIA value attributes.** `role="slider"`
  elements had no `aria-valuenow/-min/-max`, so screen readers announced valueless
  sliders. Knobs set min/max from the pushed C++ range properties (skew-safe, no
  hardcoded JS ranges), `aria-valuenow` from `getScaledValue()`, `aria-valuetext`
  via the existing FORMAT map, and an `aria-label` from the tooltip title;
  drawbars publish 0–100 (%) matching their readout.

### Removed
- **IN-01 — dead `getSampleRate` bridge endpoint.** Registered "for future
  frequency-axis labels" but never called from JS; removed until a caller exists
  so it cannot drift into an assumed-working API.
- **IN-02 — dead `currentNote` voice member.** Written in `startNote`, never read
  (the base class already tracks `getCurrentlyPlayingNote()`).

## [1.0.3] — 2026-07-15

Code-review resolution pass (CODE_REVIEW.md 2026-07-15, WR-01..WR-06). Also
**recovers the lost v1.0.1/v1.0.2 source**: those releases were built, installed,
and recorded in PLUGINS.md but never committed — the working tree had silently
reverted to v1.0.0. The complete v1.0.2 source was restored from
`backups/O-simpleAdditive/v1.0.2/` before applying the fixes below (WR-06).

### Fixed
- **WR-01 — sine LUT no longer constructed on the audio thread.** `fastSine`'s
  function-local `static SineTable` was first touched via `startNote →
  refillTable`, i.e. inside the first note-on's render call: a magic-static guard
  (potential mutex), a `LookupTableTransform` heap allocation, and 1024 `std::sin`
  calls on the RT thread — at the most audible moment. Root cause: lazy
  initialization with an audio-thread-only call site. The LUT is now touched once
  in `AdditiveVoice::prepareToPlay` (message/host thread), so the static is fully
  constructed before any `renderNextBlock`.
- **WR-02 — Spectral Decay / Vel→Decay knobs no longer go dead after 2 s.** The
  only re-dirty path for a decay-rate change was the `rate > 0 && tau < 1` render
  branch; once `tau` saturated at 1 (≈2 s into a note) rate changes were silently
  ignored, and turning the knob to 0 mid-note left the table frozen at its darkest
  state instead of restoring the undecayed spectrum. Root cause: the dirty check
  tracked the ramp, not the rate. `renderNextBlock` now compares the effective
  rate against `lastRenderedDecayRate` and re-dirties the table on any change
  while `tau > 0` — covering both the saturated-tau sweep and the rate→0 restore.
- **WR-03 — `applyFactoryPreset` parameter writes now gestured.** All 33
  `setValueNotifyingHost` calls (reset loop + `setReal`/`setChoice`) are bracketed
  by `beginChangeGesture`/`endChangeGesture`. Un-gestured edits map to a
  `performEdit` without `beginEdit` in the VST3 wrapper; hosts that gate
  automation recording on gestures (Logic touch/latch, Cubase) could drop or
  mis-record lesson-preset moves.
- **WR-04 — stuck knob/drawbar drags eliminated.** Drag end depended on a
  `pointerup` reaching `window`; releasing the mouse outside the plugin window (or
  a `pointercancel`) left the drag active — knob glued to the cursor and the host
  automation gesture open indefinitely. Knobs and drawbar tracks now
  `setPointerCapture` on `pointerdown` (guaranteeing up/cancel delivery) and treat
  `pointercancel` as `pointerup`.
- **WR-05 — stuck on-screen-keyboard notes eliminated.** Note-off depended on
  `pointerup`/`keyup` reaching the WebView; releasing outside the window, a
  `pointercancel`, or clicking away to the DAW mid-keypress left `heldNotes`
  populated and the note droning indefinitely. Added a panic path: `releaseAll()`
  on window `blur` and `visibilitychange` (hidden), plus a `pointercancel`
  handler for pointer-driven notes. (No pointer capture on the keyboard — key
  glissando relies on `pointerover` retargeting.)
- **WR-06 — version drift resolved by restoring the lost releases.** Source said
  1.0.0 while PLUGINS.md and the installed binaries said 1.0.2. Investigation
  showed v1.0.1/v1.0.2 were real (built, installed, backed up) but never
  committed, and the tree had reverted. Restored the v1.0.2 source (including
  `tests/render-harness/`) and released this pass as 1.0.3 across CMakeLists,
  CHANGELOG, STATUS.md, and PLUGINS.md.

### Notes
- Info findings IN-01..IN-06 from the same review are deferred (opt-in); see
  NOTES.md Known Limitations.

## [1.0.2] — 2026-06-25

Per-voice CPU optimization for continuous wavetable motion. No change to static
patches (bit-identical) and no change at common large host block sizes.

### Changed
- **Refill-cadence cap (PERF).** `AdditiveVoice::refillTable()` rebuilds the whole
  2048-point single-cycle table — a fixed ~2048×16 sine-sum cost *per call,
  independent of the host block size*. During continuous motion (scan/LFO/mod-env/
  spectral-decay) the table is marked dirty every block, so on small host blocks
  the rebuild fired far more often per second than on large ones and the cost did
  not amortize. Motion-driven refills are now bounded to a control-rate interval
  (~5 ms, resolved from the sample rate in `prepareToPlay`): a minimum number of
  samples must elapse between rebuilds.

  **Root cause / profile (offline render-harness, 16-voice Morph-Pad chord, dense
  Saw spectrum):** MOVING-regime CPU climbed from 4.7 % of a core at 512-sample
  blocks to 36.7 % at 64-sample blocks (perfect 2× per block-size halving) while a
  STATIC patch stayed flat at ~0.2–0.3 % — i.e. the per-block full-table rebuild
  was the hot cost and did not amortize across small blocks.

  **After:** 64-sample MOVING dropped 36.7 % → 9.0 % (4.1×), 128-sample 18.6 % →
  9.0 % (2.1×); 256/512-sample blocks unchanged (the cap is a no-op once the host
  block already exceeds the interval). The capped small-block cadence (~256 samples
  ≈ 5.8 ms) matches the cadence a 256-sample host already used, so the 20 ms scan
  smoother's zipper-free guarantee is preserved by construction.

- **No-regression guarantees (verified by the render-harness golden battery):**
  static patches (decay 0, LFO depth 0) keep the once-per-note refill and are
  **bit-identical** (`maxAbsDiff = 0`); moving patches rendered at host blocks ≥ the
  cap interval (512) are **also bit-identical**, proving the fill math itself is
  untouched — only the small-block refill *cadence* changed.

### Added
- **`tests/render-harness/`** — offline Stage-2 correctness gate + refill profile
  for this plugin (mirrors the O-simpleFM/O-simpleGrain harnesses). `--profile`
  measures STATIC-vs-MOVING per-voice CPU across host block sizes; `--dump-golden`/
  `--check-golden` capture and bit-compare the static + large-block-moving battery.
  Off by default (`-DOUARICON_BUILD_TESTS=ON`).

## [1.0.1] — 2026-06-25

Code-quality cleanup bundle. No change to the synth's audio output; one
display-accuracy refinement to the live drawbar glow.

### Fixed
- **Live drawbar glow now band-limited.** `refillTable()` published the
  *pre*-band-limit partial amplitudes into `activeSpectrum[]`, so on high notes
  partials above Nyquist (`k > Kmax`) lit the green live-glow even though they are
  never written into the table and make no sound. The snapshot is now taken
  *after* the `nyquistGain(k+1, Kmax)` band-limit, so the glow shows only what is
  actually sounding — matching the drawbar tooltip's promise (QUAL-02). Audio
  output (the wavetable itself) is unchanged.
- **`frameBSource` choice resolved robustly.** `pushParamsToVoices()` cast the raw
  parameter value with a truncating `(int)` cast; it now uses
  `jlimit(0, 3, (int) std::round(...))`, matching how the bit-depth choice is
  resolved. Same result for the four valid choice indices, but no longer relies on
  the float landing exactly on an integer.

### Changed
- **Single source of truth for the 16 partial IDs.** The `partialIds[16]` string
  array was duplicated four times (createParameterLayout, pushParamsToVoices,
  applyFactoryPreset, and the editor's `sliderIds`). Hoisted one
  `constexpr std::array` into `OSimpleAdditive::ParamIDs` (PluginProcessor.h) and
  referenced everywhere.
- **Single source of truth for lesson captions (WebView).** The lesson copy lived
  twice in `app.js` — once in `TIPS.lesson*` and again in `LESSONS`. `LESSONS` is
  now canonical; the per-button hover tooltips are derived from it. (Hover-tooltip
  wording for the six lesson buttons changes slightly as a result; tour-caption
  text is unchanged.)
- `-Wfloat-equal` hygiene: the `band[k] != 0.0f` silent-partial skip in
  `refillTable()` now uses `juce::exactlyEqual`, matching the rest of the codebase.
- Removed stale stage-process comments (the "lifted verbatim from FmVizAnalyzer",
  "not yet constructed", and "Stage 2 (complete)" framing in the file headers).

## [1.0.0] — 2026-06-22

First complete release. A pedagogical 16-partial **additive** synth with a light
**wavetable** dimension — the additive sibling to O-simpleFM. The 16 drawbars *are* the
spectrum: you build a tone harmonic-by-harmonic and watch it on the live display.

### Synthesis engine (Stage 2)
- **16-partial additive voice**, 16-voice polyphonic (`AdditiveVoice : juce::SynthesiserVoice`).
  Each note is summed into a band-limited single-cycle wavetable (2048-pt) and read by phase —
  not a per-sample sum-of-sines — so CPU stays flat and there is **zero added latency**
  (`setLatencySamples(0)`).
- **Exact per-note anti-aliasing** — partials above Nyquist (`k > Kmax = floor(0.5·fs/f0)`) are
  never written; a raised-cosine taper on the top 2 surviving harmonics avoids boundary clicks.
  No oversampling. High notes with all drawbars up stay clean.
- **Wavetable scan/morph** — per-partial linear *spectral* morph from Frame A (the drawbars)
  toward a Frame B preset (Sine / Saw / Square / Odd), driven by a manual knob, a global sine
  LFO, and a mod-envelope. Zipper-free via a 20 ms smoother + control-rate table refill.
- **Spectral-decay macro** — per-partial exponential tilt over the note (`D_k = exp(−rate·k·τ)`),
  with optional velocity routing (`velToDecay`).
- **Bit-depth quantizer** — discrete `{Off, 12, 10, 8, 6, 4, 2}` mid-tread crusher for lo-fi grit.
- **Dual ADSR** — amp envelope (voice lifetime) + an independent mod-envelope routed to scan.
- Headroom-normalized table sum (÷ max(1, Σ amplitudes)) so 16 maxed drawbars don't clip.
- `ScopedNoDenormals` + block-level `isfinite` scrub; floor-modulo phase wrap.

### Interface (Stage 3) — "Additive Field Guide" WebView
- Single-page classroom/projector-readable layout (sibling aesthetic to O-simpleFM).
- **16 drawbars double as the live spectrum** — brass set-level + green live-glow showing the
  exact *morphed + decayed* active-spectrum snapshot (not an FFT estimate).
- Live **oscilloscope** of the summed waveform (30 Hz message-thread Timer over a lock-free
  `VizRing`; analyzer copies the window before its in-place FFT — no corruption, no audio-thread
  allocation).
- 33 parameters two-way bound (31 `WebSliderRelay` + 2 `WebComboBoxRelay`).
- Plain-language **hover tooltips on every control** (overtone series, why odd-only is hollow,
  what scan/morph/bit-depth do).
- **6 lesson presets** — Pure Sine, Sawtooth, Square (hollow), Organ, Morph Pad, Lo-Fi Bells —
  each isolates one concept via a full APVTS snapshot.
- On-screen keyboard MIDI (`MidiMessageCollector` + drain into the synth).
- Cross-platform WebView wired: `NEEDS_WEBVIEW2` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING` +
  `withUserDataFolder` for Windows; custom-scheme resource provider for macOS.

### Validation (Stage 4)
- **pluginval strictness 8 — VST3: SUCCESS, AU: SUCCESS** (render, automation, parameter fuzz,
  state restore, threading, bus layouts).
- **`auval -v aumu OSiA OuDv` — AU VALIDATION SUCCEEDED** (incl. Test MIDI render).
- Factory-preset sweep: all 6 lessons apply finite, in-range snapshots (reset-to-default first).
- Aliasing audit: band-limit + taper + headroom + finite-phase guard confirmed exact; fuzz/render
  at the top of the keyboard surfaced no NaN/Inf/denormal.
- Default patch (H1=100%, rest 0) is a pure sine — unregressed from the Stage 2 DSP.

### Deferred to v1.1
- Persistent user preset save/load bar (`OuariconPresetManager`).
- Per-partial mod-env decay routing.
