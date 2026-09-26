# Changelog — O-Orbit

## [1.4.0] - 2026-09-26

This is the UI pass from the 260924-nho design review (Phase C). It adds a CSS custom-property palette, AA text colours from `ouaricon-naturalist-001` (R4), the 9px text floor, the bundled EB Garamond face (R5), and `:focus-visible` rings. It is MINOR because the change is visible and bundles a font. **There is no DSP, parameter, range, type or state-format change.** No processor file was touched. The only C++ change is four `getResource()` branches for the font. Knob keyboard/ARIA (R7) was left out on purpose, because no knob code was touched.

### Changed

- **The palette is custom properties.** A `:root` block of 21 tokens (20 colours and the font) replaces all 159 hex literals in `css/styles.css`. There were no custom properties before this release, and there are now 0 literals outside `:root`. Canvas colours in `js/app.js` stay literal, because a 2D context does not resolve `var()`. The `rgba()` washes are unchanged.
- **AA text colours.** `#8B7355` becomes `--rule-muted` and is kept for borders, rules and fills only. Each text failure takes its variant from the aesthetic's Text Contrast table:
  - Preset-menu category headers: `--text-muted` `#6A5641`, 3.22 → 4.98.
  - The active preset-menu item: the fill is `--walnut-fill` `#6A5641`, 3.66 → 5.68.
  - The lit Elevation pill and the lit hover-help switch: the fill is `--leaf-fill` `#4E6839`, 2.16 → 5.10. `--sage` stays on borders and on the knob pointer.
  - The ⚙ glyph: `--gear-ink` `#3F5530`, 3.90 → 5.74. On the pressed 0.65 wash it is `--gear-ink-open` `#2C3E10` (5.90).
  - The tooltip title: `--leaf-text` `#4E6839`. It was 4.07 on the tooltip gradient's `#EBD9C7` stop and is now 4.54.
  - The layout-name placeholder: `--text-muted`. The UA grey was 4.20 on its cream field and it is now 6.33.
- **9px floor.** The preset caret goes from 8px to 9px. The canvas text goes from 8px to 9px: speaker labels, elevation badges, the ELEV caption and the L/R source letters.
- **Canvas text inks.** The census cannot see these, so they were computed against the canvas plate `#E7D7C2`.
  - FRONT / REAR / ELEV and the editor hint line were `rgba(60,47,47,0.3–0.4)` (1.70–2.09). They are now `#6A5641` (4.93).
  - The L/R letters take a darker ink of their dot's hue: `#3F5530` at 5.83 and `#6F5228` at 5.11, where they were 1.88 / 1.67. The dots and glows keep their colours.
  - Elevation badges go from `#5C6E3E` / `#8A5A2B` to `#3F5530` / `#7A4A1E`. They were 3.97 / 4.16 and are now 5.83 / 5.27.
  - A speaker label on the sage drag fill uses `#3C2F2F`, which reads 4.84. `#5C4033` read 3.55 there.
- **One text face, bundled.** EB Garamond comes from `modules/ui/eb-garamond` by direct embed:
  - 4 `SOURCES` lines, 4 `getResource()` branches, and `css/eb-garamond.css` linked ahead of `css/styles.css`.
  - The 14 repeated font stacks become `var(--font-serif)`, and the canvas uses one `CANVAS_FONT` constant.
  - The bare `Garamond` and `Times` entries are dropped. A CDP probe shows EB Garamond resolving on every Latin node in en/fr/zh-Hans, with the Han runs on PingFang SC.
  - The four branches test the raw `url`, the module snippet's own shape, not the normalized `path`. `scripts/serve-ui.js` only places out-of-root embeds from `url == "…"` literals. The first cut used `path ==`, which made the served test tree 404 the font, so every gate measured the Georgia fallback.
- **Keyboard focus.** There was no `:focus-visible` rule before. Now a 2px `--text-body` ring (7.67 on paper) covers all 12 Tab-reachable control families. It also replaces the bare border-colour change that three `:focus { outline: none }` rules left behind. The knobs, `#view-toggle` and the preset-menu items are divs and cannot be reached by Tab. The Elevation checkbox is `display: none`. None of these is listed.
- **Metric re-pins for the new face.** EB Garamond carries Times New Roman's line box. This page previously resolved to macOS *Times*, which has a taller line box, so the English and French rows lose 1–2px each: the controls pane scrolls 11–12px less at 800×600. Widths were re-measured with each pin lifted:
  - `.preset-btn-hdr` goes from 58 to 59, for Ouvrir at 58.39.
  - The Hex/Oct chips go from 40 to 41, for Octo at 40.88.
  - `#view-toggle` stays at 170 (Éditeur d'enceintes is 169.00). The layout buttons stay at 53 and the file buttons at 74.
  - The ten Chinese-scoped line-height ratios were re-derived from the new English content boxes, the same method v1.3.0 used. The form controls go from 1.3 to 1.1, the 11px leaves to 1.0909091, `.group-label` to 1.1428571 and `#view-toggle` to 1.25.
  - The four Han format chips are re-pinned to the new English row: 50.39 / 41.48 / 41 / 41.

### Tests

- `tests/i18n-states.json` gains four states, so that before and after are measured on the same fuller page:
  - Elevation on.
  - The preset menu open.
  - A keyboard-focused tooltip.
  - A no-op settle step after the hover-help click. The switch fades over 0.2s and the census samples 140ms after the click, so v1.3.0's "1.72" was the mid-fade pair. The settled pair was 2.16.
- `tests/ui-stub/generic-overrides.json` gains `getPresetListGrouped` / `getCurrentPreset`, so the menu renders under the stub.

### Measured (v1.3.0 → v1.4.0, served page, 800×600, all states in `tests/i18n-states.json`)

| Gate | Before | After |
|---|---|---|
| `measure-ui.js --contrast`, per language (en = fr = zh-Hans) | 6 / 67 below AA, 1 under 9px, min 1.72 (mid-fade; settled 2.16) | **0 / 67**, **0 under 9px**, min 4.98 |
| `check-ui-labels.js` (en / fr / zh-Hans) | ALL CHECKS PASSED | ALL CHECKS PASSED (340 PASS lines) |
| `check-i18n.js` | ALL PASS | ALL PASS |
| Resize probe at 600×450 / 800×600 / 1600×1200 (en / fr / zh-Hans, rest + speaker editor) | no horizontal overflow; 600px toolbar scrolls 187px (D4, by design) | no horizontal overflow; toolbar scrolls 186px; 0 page errors, 0 404s |

### Known, not addressed here

- **The botanical overlay sits above the text.** The overlay is `#botanical-overlay`: z-index 1000 at 0.35 opacity. Pixel-sampled at 800×600, PHASE reads about 3.98 and EXPORT about 3.50 through it. The census cannot see an `<img>` ground. This was unchanged by this release and needs a design decision: it could move below the panels.
- **The Elevation pill's thumb covers the first glyph of OFF** (unchanged).
- **R7 keyboard/ARIA knobs** remain for a separate pass: the O-ReverseDelay port, and making `#view-toggle` / the Elevation checkbox focusable.
- **The canvas captions are still English-only**: FRONT, REAR, ELEV and the editor hint line.

## [1.3.0] - 2026-09-07

Simplified Chinese joins English and French. MINOR: 125 new interface strings,
one whole-page font-stack repair and thirteen Chinese-scoped geometry rules —
no parameter, range, type or state format changed, and no English or French
rendering moved.

### Added

- **Simplified Chinese (`zh-Hans`) across the whole interface** — 125 rows over
  34 tooltip entries and 57 captions, plus the `简体中文` endonym in the language
  selector and its `I18N_EXEMPT` entry. The `languageCode` / `languageIndex`
  codec in `PluginProcessor.h` is three-way and stays pure ASCII; persistence is
  unchanged (the ValueTree `uiLanguage` property).
- **Disclosed quality level: `reviewed: 'bt'`** — machine-drafted, then read
  BACK from Chinese into English by a separate agent with no access to this
  repository and no sight of the original English, in two concept-split batches
  with a fresh blinding salt each, and every one of the 125 triples read. It is
  NOT `'native'`: no native Simplified Chinese speaker has read these strings,
  and `scripts/i18n-zh-lint.js` prints that on every run.

### Changed

- **The single house font stack now names an installed serif face.** All
  fourteen declaration sites read `Garamond, 'EB Garamond', serif` with no
  `font-family: inherit` anywhere, and neither Garamond family is present on the
  build machine — so the only surviving member was the bare generic, which
  Chromium resolves against the document language. Probed through the DevTools
  protocol, the ASCII wordmark resolved to Times on the English and French arms
  and to a Chinese face on the Chinese arm, at a different width. The stack now
  reads `Garamond, 'EB Garamond', Times, 'Times New Roman', 'PingFang SC',
  'Microsoft YaHei', serif`: an installed Latin face first so neither Latin arm
  can move, the CJK tail before the generic, and the two Garamond members kept
  ahead of everything as the Windows and print intent. Re-probed after the
  change, all three arms resolve the same face at the same width, and every
  Han-bearing node resolves through PingFang SC.
- **Ten Chinese-scoped line-box ratios.** A Han line box runs about 30% taller
  than a Latin one at the same size, so every leaf whose `line-height` resolved
  to `normal` grew 1px to 3px. Each ratio is derived from that element's own
  measured English content box rather than from a shared table, and all ten sit
  under `html[lang="zh-Hans"]` so the English and French arms keep the `normal`
  they shipped with.
- **The four worded speaker-format chips are pinned to the English row.** Their
  Chinese renderings are all three glyphs where the English ranges from three to
  six characters, so one chip shrank 8.61px and three grew — a net 2.98px that
  slid the four unkeyed numeric chips 7.33px left. Latin letter-spacing is
  trimmed on those four under Chinese, where it buys nothing.
- **The language tooltip no longer enumerates the available languages.** It said
  which two there were; there are three, and a body that counts them is wrong
  again at the next one. The exception list is unchanged and re-verified — value
  readouts and preset names still stay in English, the second of those being the
  `#preset-name` node the markup has always declared off-limits to the sweep.

### Notes

- `Resources/ui/js/modules/preset-manager.js` is unchanged. It carries seven
  English strings; five live in a `createPresetBar()` block this page never
  calls, and the two that do reach the page write the preset NAME, which is the
  preset's filename and is exempt by design. No gate in this repo opens the file
  — reported here rather than fixed inside a copy-only release.
- `plugins/O-Orbit/libs/SAF` is untouched at `b6fe1882` (v1.3.4).

## [1.2.3] - 2026-09-03

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


## [1.2.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed
- **item 58 — hover help, keyboard half:** the page opened tips on `mouseover` only; there was
  no `focusin` handler at all, so Tab into a control opened nothing and the hover help had no
  keyboard half. `app.js` `initializeHoverHelp()` now carries the Stage M focus latch
  (`lastInputWasPointer`, O-Comp v1.7.0): `pointerdown` latches, any `keydown` releases,
  `focusin` opens the focused anchor's tip only while released, `focusout` hides, Escape hides.
  A mouse click on the gear, a select or a button still opens no tip (it focuses the control,
  and an unconditional `focusin` rule would have re-opened the tip pointerdown had just hidden,
  over the popover the click opened). The popover's own Escape handler refocuses the gear by
  script; it registers first and reads the latch as the click left it, so no tip lands on the
  gear as the popover closes. Probe at the 800×600 frame, both languages: click #gear-btn → no
  tip before and after; Tab → the Path cell's tip (91.4 px en / 106.2 px fr), Tab → Tempo Sync
  (121.1 / 135.9 px), Shift+Tab ×2 → the gear (76.5 px), every tip inside the frame — 8 of 23
  assertions failed before, 0 after.
- **item 44 — `.toggle-label { font-size: 9px }`:** dead since v1.0.0 — `.param-container
  label { font-size: 11px }` (0,1,1) beats it (0,1,0), `getComputedStyle` reads 11px. Deleted
  rather than promoted: promoting would have shrunk the elevation pill's face on a shipped
  control (Non 26.84 → 22.50 px, Off 23.19 → 19.52 px; the 50×24 pill itself unchanged), and
  every width in the `i18n.js` header was measured at the winning 11px, so nothing on screen
  moves. The Oui/Non exemption on that pill is therefore permanent at 11px (MARCHE 53.06 px in
  a 46 px content box; it would have fit at the 9px that never rendered, 44.52 px). The
  `styles.css` note and the `i18n.js` comments that said "until the specificity is settled"
  are corrected in place. `check-ui-labels`: 0 non-label elements moved, both languages, before
  and after.

No English or French copy changed.

## [1.2.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **23 French entries revised** against the suite glossary and lint: 6 terminology, 15
  typography, 1 agreement, 1 meaning. The visible ones — **Mixage → Mix** on the Source / Mix
  heading, the Mix caption and the Mix tooltip title (*mixage* is the mixing process; *Mix* is
  what French DAWs print for a dry/wet control); **Absorption → Absorption air**, which
  measures 104.17 px — exactly the width of the English "Air Absorption" it replaces — and
  restores the half of the name that says what is absorbing; **Désact. → Désactivé** in the
  Tempo Sync list, which had 40.90 px of room to spare; straight apostrophes → typographic
  ones; and no-break spaces before `% : ; ?` and between a number and its unit, so a French
  line never breaks between "0" and "%" or between "1" and "mesure".
- **A gender error in the Tempo Sync tooltip** — *toutes les quatre temps* → *tous les quatre
  temps*; *temps* is masculine.
- **The Speed tooltip** now says *Vitesse* rather than *Fréquence*, matching its own caption
  and title, and names the control it cross-refers to by the caption on screen (*Sync tempo*).
- **`<html lang>` now follows the language selector** (canon change, all plugins), so
  assistive technology reads the page in the language it is displayed in.

Three entries carry a reasoned glossary exemption rather than a change: the **downmix badge**
keeps *Mixage réducteur* — a channel fold-down is not the dry/wet control — and the elevation
toggle keeps **Oui / Non**, because no settled form fits the 46 px pill it shares with the
hover-help button (MARCHE 53.06 px, ACTIVÉ 46.33 px, in a 46.00 px box). All 91 entries remain
`reviewed: false`: that flag records a native-speaker reading, and none has happened.

## v1.2.0 — 2026-08-28

Feature release: **the page speaks French, not only the hover help.** Quick task 260826-ieq
Stage I, batch I1. O-Orbit is the **seventh plugin on canon v2** and the second outside the
five that shipped tooltips first, so it gains BOTH halves in one release — 32 tooltips moved
out of the markup AND 57 labels, 8 accessible names and 6 script-written captions localized —
rather than being half-localized twice.

### Added
- **Interface language, English + French.** A settings gear in the header opens a popover
  holding a language selector and the hover-help toggle. Every control caption, group heading,
  dropdown option, button face and accessible name follows the selection, with no reload.
  `Resources/ui/js/i18n.js` carries 34 tooltip entries, 57 label keys, 34 tip bindings and a
  5-entry reasoned `I18N_EXEMPT`. All 91 French entries are machine drafts flagged
  `reviewed: false` — no native speaker has read them.
- **`getUiLanguage` / `setUiLanguage`** native functions. The choice rides the session as a
  plain APVTS tree property beside `tooltipsEnabled`, never as a parameter: it must not appear
  in a DAW automation lane, and a preset must not be able to change which language you read
  your interface in. Stored as the string `"en"` / `"fr"`, restored behind an `isVoid()` gate —
  the XML round-trip rebuilds every property as a string var, so a type predicate would never
  fire (`critical_valuetree_xml_roundtrip_loses_type`).
- **Settings popover**, written in this plugin's own paper vocabulary: the `#F5E6D3` plate and
  `#8B7355` rule the parameter panels use, with sage `#8BA870` reserved for the lit state. The
  gear replaces the v1.1.0 "?" in the same header slot and wears the same 20px circle, so the
  header silhouette is unchanged; the "?" toggle MOVED inside the panel rather than being
  duplicated.

### Changed
- **Value readouts, preset names and channel-format designations stay English** (D-03, D-02).
  `1.0 Hz`, `180°`, `Default`, `5.1.4` read identically in both languages.
- **Two-click delete faces are now KEYS, not `data-label` / `data-confirm` attributes**, on both
  the preset button and the layout-library button. An attribute holds one string, so switching
  language while a button was armed would have restored the English armed face.
- **Tooltip copy no longer lives in `index.html`.** `applyI18n()` writes `data-tip` and
  `data-tip-title` at runtime from `TIP_BINDINGS`. The eighteen parameter cells gained a
  `data-param` attribute as their tip anchor — a bare `.param-container` selector would have
  matched the first of eighteen.

### Fixed
- **The view toggle changed width on every click, in English, since v1.1.0.** `#view-toggle`
  is a shrink-to-fit box whose two faces measure 93.1px ("Motion View") and 114.1px ("Speaker
  Editor"), and `#header` is `justify-content: space-between`, so switching views dragged the
  whole preset band 21px sideways. Nothing was measuring it. The button is now pinned to the
  widest of its four faces (169.3px, "Éditeur d'enceintes"), which holds both languages and
  both views still.

### Layout — seven containers pinned, every number measured
Measured in headless Chromium at the shipping 800 × 600 across four driven states. Each pin was
reverted on its own and confirmed to re-break the geometry gate.

| Fix | Measured cause |
|---|---|
| `#view-toggle { min-width: 170px }` | four faces spanning 93.1 → 151.3; see Fixed above |
| `.preset-btn-hdr { min-width: 58px }` | Save 26.8 → Enreg. 38.8, Load 29.8 → Ouvrir 41.3, Del 21 → Suppr. 36.1 |
| `.preset-btn[data-preset="6"/"7"] { min-width: 40px }` | Hex 22.1 → Hexa 29.8, Oct 21.5 → Octo 29.0. Stereo/Stéréo and Quad/Quad measure IDENTICALLY, so six of the eight chips needed nothing |
| `#layout-select { width: 92px }` | Layouts… 42.2 → Dispositions… 60, sliding the whole layout library 23.5px |
| `#layout-library button { min-width: 53px }` | Enreg. 38.8 / Suppr. 36.1 / Sûr ? 28.9 |
| `#file-buttons button { min-width: 74px }` | Exporter 55.1, Importer 54.0 |
| `.preset-btn { padding: 2px 4px }` | pinning every worded control in the editor toolbar took the French row to 785.5px inside 768px of usable width and pushed **Import past the 800px frame**. Trimming 2px a side off the eight format chips returns 32px to BOTH languages, which is where the budget came from |

**One French string was SIZED**, recorded at its entry with the measurement: `Synchro tempo`
measures 105.4px and the Motion group's grid track is 100.3px, so it wrapped to two lines and
pushed the Tempo Sync dropdown down 13px. It is `Sync tempo` (79.2px) — the full phrase survives
as the tooltip title, which renders in a 230px box.

Four of the pins change ENGLISH geometry too. That is the trade D-04 asks for.

### Testing
- `check-i18n.js --strict-v2` repo-wide: **exit 0**, canon split **v2 7, v1 0**.
- `check-ui-labels.js --plugin O-Orbit`: **exit 0** — 56 labels over 4 driven states, **zero**
  non-label elements moved between English and French, 49/56 (88%) of labels and 9/9 keyed
  attributes change language.
- `boot-all-uis.js`: **41/43 clean, unchanged.** O-Orbit reports `title=0 aria=8 i18n=56`.
- **29 negative controls, 29 fired.** Each mutation was applied to a byte-exact backup and
  restored from that backup, never `git checkout --`, which would have wiped the uncommitted
  retrofit alongside it.
- `./scripts/build-and-install.sh O-Orbit` → `auval -v aufx OuOr OuDv` PASSED.

### Known limitations
- **All 91 French entries are unreviewed machine drafts.** `Oui` / `Non` for the elevation
  toggle's On/Off faces and `Sync tempo` are the three this release would challenge first.
- **No human has seen the French UI**, and nothing was tested in a DAW. The language
  round-trip through `get/setStateInformation` is reasoned from source, not measured.
- **19 of the 56 keyed elements were never measured** — every one is an `<option>` inside a
  closed `<select>`, which has no box until the OS renders the popup. No states file can reach
  them; this is a property of native select menus, not a coverage gap a test could close.
- Windows / WebView2 font metrics are a named deferral, blocked on hardware. Every width above
  was measured in Chromium on macOS.

## v1.1.1 — 2026-08-27

Patch: the tempo-sync table mirrored from O-Octagon's v1.10.0 WR-01 fix (the table originated here
and was byte-copied there).

### Fixed
- **Tempo Sync ran 4× slower than its labels, and two menu pairs were identical.** `tempoMultipliers`
  (`MotionEngine.h`) is cycles per beat — `getEffectiveSpeed()` returns `bpm/60 · mult` and the C1
  PPQ lock uses `ppq · mult` directly, no hidden factor. The table had `1/4 = 0.25` (one cycle per
  four beats), `1 Bar = 0.0625` (one cycle per **sixteen** beats) and `4 Bars = 1/64`; triplets used
  4/3 instead of 3/2, so `1/16D ≡ 1/8T` and `1/8D ≡ 1/4T`. The table is now written from the
  musical definitions: `1/4` = 1.0 cycle per beat, `1 Bar` = 0.25, triplet = 3/2 × parent, dotted =
  2/3 × parent; no duplicates. The Tempo Sync tooltip now states the convention ("1/4 is one cycle
  per beat, 1 Bar one cycle per four beats (4/4 assumed)").
- **Saved state:** a `tempo_sync` index keeps its label and gains its label's meaning — every synced
  session moves 4× faster than before, which is what the menu always said. The *Tempo Quarter*
  factory preset (`1/4`) now completes one orbit per beat rather than one per bar.

### Compatibility
- No parameter IDs, ranges, defaults or choice lists changed. Sessions restore identically as data;
  synced motion RATE changes as described above.

### Testing
- `./scripts/build-and-install.sh O-Orbit` → `auval -v aufx OuOr OuDv` PASSED; installed
  `O-Orbit-dev.component` reports 1.1.1. The table's musical semantics are held by O-Octagon's
  MP9 probe against the identical numbers (O-Orbit has no render harness — see Known Issues).

## v1.1.0 — 2026-08-19

Feature release: Parts B–D of the v1.1 review-findings brief (suite parity + motion and speaker-editor upgrades). C2 Doppler and C4 custom path remain deferred per the brief.

### Added
- **Preset manager (B1)** — migrated to the shared preset-manager module (v1.0.6). Categorized preset menu (Stereo / Surround / Creative / User), prev/next stepping that walks the menu order (not the alphabetical list), user preset save/load/delete with native file dialogs, two-click armed delete. The 12 factory presets keep their exact v1.0.0 values (converted skew-aware through each parameter's own range at registration). The legacy Programs API is collapsed to a single program like the rest of the suite.
- **Hover help (B2)** — "?" toggle in the header + hover tooltips on all 17 parameters, the view toggle, preset band, editor toolbar, and downmix badge. Measure-then-pin fixed-position tooltips (viewport-clamped, arrow tracks the anchor). Preference persists with the session (plain tree property, `isVoid()`-gated restore).
- **PPQ-locked tempo sync (C1)** — when synced and the transport is playing, motion phase derives directly from the host beat position (`phase = fmod(ppq × cyclesPerBeat, 1) × 2π`, computed in double). Offline bounces are deterministic and downbeat-aligned; Drift's noise time locks too. Free-run remains the fallback when stopped or without a playhead (Standalone still moves).
- **Ping-Pong path (C3)** — 5th path choice (appended, never reordered — APVTS sessions store the index). Triangle-wave azimuth: sweeps −w/2 → +w/2 and folds back with no positional discontinuity, unlike Linear's saw-wrap.
- **Speaker editor: elevation + distance editing (D1)** — shift+vertical-drag edits elevation (±90°), alt-drag or scroll edits distance (0.1–30 m), plain drag still edits azimuth. Speakers draw at a distance-scaled radius (same mapping as the hit-test) with an elevation badge, and a hover/drag readout shows az/el/dist.
- **Named layout library (D2)** — save/load/delete named custom layouts (`~/Library/Ouaricon Orbit/Layouts/*.json`, same schema as export/import) from a dropdown in the editor toolbar. File export/import unchanged.
- **Height visualization (D3)** — source dot scales and brightens with elevation, side elevation gauge in motion view, height-layer speakers drawn with a dashed second ring in both views.
- **Resizable editor (D4)** — 600×450 to 1600×1200 at a fixed 4:3 aspect (default 800×600). Canvas re-rasterizes on resize (`setTransform`, not cumulative `scale`); visualizer height is now vh-based.

### Changed
- `moveSpeakerInLayout()` takes distance and re-derives the layout's `is3D` flag; the `moveSpeaker` native fn accepts an optional 4th argument (older callers keep the speaker's current distance).
- Session state now stores `currentPreset` and `tooltipsEnabled` as plain tree properties (absent in older sessions → defaults stand; pre-1.1.0 sessions unaffected).

### Compatibility
- All parameter IDs, ranges, and defaults unchanged; the only layout change is the appended 5th "path" choice. Saved sessions restore identically (APVTS stores the choice index). Normalized "path" *automation lanes* recorded against the 4-choice range decode against 5 — same accepted trade-off as prior suite choice-append releases.
- Factory presets are regenerated under the v1.1.0 sentinel; values match v1.0.0's Programs byte-for-byte in effect.

### Testing
- pluginval strictness 10 (VST3), auval (AU) — see build log
- Factory presets A/B'd against v1.0.1 Programs values (skew-aware conversion verified)
- PPQ sync: phase derived from beat position — two offline bounces produce identical motion

## v1.0.1 — 2026-08-19

Defect-fix release (Part A of the v1.1 review-findings brief; features B/C/D deferred to a later milestone).

### Fixed
- **Depth parameter was dead** — `MotionEngine` computed a Depth-driven distance but `processBlock` never read it; the distance model and visualizer only ever saw the static Distance param. Effective distance is now `Distance × motion distance` (floor-clamped at 0.1 m), fed to both L/R distance models and the visualizer dot.
  - *Behavior note:* factory presets with Depth > 0 (Fast Spiral, Ambient Drift, Deep Space, …) gain audible near/far motion for the first time — intended.
- **Audio-thread allocation every block** — `DistanceModel::updateDistance()` heap-allocated IIR coefficients (`Coefficients::makeLowPass`) per block for both channels. Now assigns `ArrayCoefficients::makeLowPass` (stack array) into the existing storage, and skips recomputation entirely when (distance, air absorption, curve) are unchanged.
- **Dead parameter smoothers / mix zipper** — five smoothers were reset in `prepareToPlay` but never advanced or read. Mix is now genuinely smoothed per-sample (20 ms ramp, snapped on prepare to avoid a fade-in); the four redundant motion smoothers were deleted (motion is phase-integrated and VBAP gains already interpolate per block). Distance gain now ramps per-sample inside `DistanceModel` — necessary since the Depth fix makes distance move every block.
- **Dry/wet mix tilted surround output frontward** — the mix loop only blended the first `min(in, out)` channels, so on 5.1/7.1.4 outputs at mix < 100% front L/R got dry-blended while all other channels stayed full wet. Now wet scales on ALL output channels and dry blends only into its native input channels (constant spatial balance).
- **Double-click knob reset landed off-default on skewed knobs** — reset went to normalized 0.5 instead of the parameter default (e.g. Speed, skew 0.5, default 1.0 Hz). Now resets to each knob's `data-default` via a skew-aware inverse of the parameter range.

### Root cause
v1.0.0 shipped with the Depth→distance wiring dropped between MotionEngine and processBlock, and per-block parameter application throughout the mix/distance path. Found by full code review 2026-08-19 (`.planning/improvements/v1.1-review-findings.md`).

### Testing
- pluginval strictness 10 (VST3), auval (AU) — see build log
- Depth sweep: audible level/HF motion + visualizer dot radius follows
- Mix automation sweep: no zipper (per-sample ramp)
