# Changelog — O-Orbit

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
