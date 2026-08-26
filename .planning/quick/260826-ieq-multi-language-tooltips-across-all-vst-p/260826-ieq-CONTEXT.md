# Quick Task 260826-ieq: Multi-language tooltips across all VST plugin UIs - Context

**Gathered:** 2026-08-26
**Status:** Ready for research

<domain>
## Task Boundary

Add multi-language tooltip support to the WebView UI of every plugin in `plugins/`
(43 plugins). Each plugin gains an in-UI language selector; tooltip copy is served
from a per-plugin localized string table rather than inline markup.

**In scope:** tooltip hover-help copy (title + body), the language selector control,
per-plugin string tables, persistence of the language choice, English + French.

**Out of scope:** localizing non-tooltip UI chrome (parameter labels, section
headings, preset names, readout units), DAW-facing metadata, plugin descriptions,
the marketing site.

</domain>

<decisions>
## Implementation Decisions

### Plugin scope
- **All 43 plugins**, not just the 21 that currently have rich tooltips.
- The 22 plugins with no tooltip copy today (only stray native `title=` attributes)
  get English tooltip text **authored as part of this task**, then localized.
- The three existing tooltip conventions are unified into one before localization:
  | Convention | Plugins | Tips |
  |---|---|---|
  | `data-tip` + `data-tip-title` HTML attrs | 13 — O-Bitrot(53), O-Octagon(52), O-Contrabass(44), O-simpleGrain(34), O-simpleSampler(34), O-Tapestop(33), O-simpleSubtractive(33), O-Orbit(32), O-ReverseDelay(29), O-simpleAdditive(24), O-simpleFM(24), O-simplePhysicalModelSynth(21), O-simpleBeatmaker(17) | 430 |
  | `data-tooltip` HTML attr | 7 — O-Polystutter(102), O-Lyrica(43), O-IntonationPad(37), O-SpectralShaper(25), O-Gain(23), O-Marimba(15), O-FreqPulse(12) | 257 |
  | JS table (`GLOBAL_TOOLTIPS[]` / `BAND_TOOLTIPS[]`) | 1 — O-MultiBandCompressor | ~22 |
  | none (native `title=` only) | 22 | 0 |
- Existing English copy is preserved verbatim where it exists — this task moves it,
  it does not rewrite it.

### Language selection UX
- **Explicit dropdown only. English is the default.** No OS/host locale sniffing —
  a fresh install is English until the user chooses otherwise.
- The choice **persists per plugin instance** in the APVTS state tree as a
  non-parameter entry (never an `AudioParameterChoice` — it must not appear in
  DAW automation lanes or preset parameter diffs).
- Persistence caveat carried from prior work: a non-parameter value on the APVTS
  state tree round-trips through XML as a **string** `var`. Store the language as a
  language code string (`"en"`, `"fr"`) and compare as a string — do not rely on
  type-checking predicates on the restored value.

### Storage + lookup
- **Per-plugin JS, no shared module.** Each plugin owns its own i18n table and its
  own copy of the lookup runtime, matching this repo's existing hand-copy
  convention for UI code (there is no shared knob module either).
- No new entry in `modules/registry.yaml`; `modules/ui/` is not touched.
- Lookup happens in **JS at load time and on language change**. C++ only persists
  and restores the selected language code and hands it to the WebView.
- Two UI roots must both be handled: `Source/ui/public/` (33 plugins) and
  `Resources/ui/` (10 plugins — O-Bassoon, O-Bells, O-Bowed, O-FreqPulse, O-Lyrica,
  O-MicrotonalSampler, O-Orbit, O-Reed, O-SpectralShaper, O-Wind).

### Languages + translation source
- Ship **English + French** only. French is the pilot that proves the round-trip;
  further languages are added later by dropping in additional tables once the
  format is settled.
- French copy is **machine-drafted and marked unreviewed** so a native speaker can
  correct it without guessing which strings were vetted.

### Language selector placement
- A **gear / settings popover** in each plugin's UI, holding the language selector.
- Where a plugin already has a tooltips-on/off control, that control moves into the
  same popover rather than sitting beside it. **9** plugins have the
  `setTooltipsEnabled` native bridge: O-Bitrot, O-Contrabass, O-FreqPulse, O-Lyrica,
  O-MultiBandCompressor, O-Octagon, O-Orbit, O-SpectralShaper, O-Tapestop.
- **Corrected 2026-08-26 (RESEARCH.md F1a):** O-ReverseDelay was listed here as the
  10th. It has NO such bridge — and `tests/ui_frontend_check.js:759-761` actively
  asserts one does not exist, tracing to a recorded stage-4 decision "D13"
  (`plugins/O-ReverseDelay/.planning/stages/4-polish/PLAN.md:356`). Giving
  O-ReverseDelay a popover means reversing D13 explicitly.
- The popover is a **new component in all 43 UIs** and must respect each plugin's
  existing visual system — it is not a single uniform widget pasted in unchanged.

### Post-research decisions (2026-08-26, after RESEARCH.md)

**Second renderer — PORT, do not localize in place.**
The 7 `data-tooltip` plugins (O-Polystutter, O-Lyrica, O-IntonationPad,
O-SpectralShaper, O-Gain, O-Marimba, O-FreqPulse) are moved onto the `data-tip`
measure-then-pin renderer rather than having i18n bolted onto their existing
positioner. Their current runtime never measures — `tooltipHeight = 60 // Approximate`,
`tooltipWidth = 220`, viewport literals `660`/`1000`
(`plugins/O-Polystutter/Source/ui/public/index.html:1717-1733`) — which is already
wrong before French makes strings taller. After the port there is ONE renderer and
ONE convention repo-wide.
- Existing single-string copy splits on the first `": "` to recover title/body
  (it is already authored `"Label: sentence."`). Expect a handful of hand-splits.
- O-FreqPulse synthesises `data-tooltip` with interpolated band names at
  `Resources/ui/js/app.js:335-531`; it needs parameterised i18n entries, not flat
  ones. Same for O-MultiBandCompressor's band-name composition (`app.js:1367-1368`).

**O-ReverseDelay — D13 STANDS. Language selector only, no tooltips toggle.**
`tests/ui_frontend_check.js:759-761` asserts no `setTooltipsEnabled` exists, tracing
to stage-4 decision D13 ("tooltips only, the bridge stays at 11"). That gate is NOT
retired. O-ReverseDelay gets `getUiLanguage`/`setUiLanguage` only — the gate forbids
`setTooltipsEnabled` by name, so it stays green — and its popover carries the
language selector alone. Its bridge goes 11 → 13, which D13 did not speak to.

### Claude's Discretion
- The exact unified tooltip attribute/key convention (`data-tip` vs `data-tooltip`
  vs a key-based `data-tip-key`) and the shape of the per-plugin string table.
- The i18n file format and where it sits inside each UI root.
- Whether the language code reaches JS via an existing native-function bridge, a
  fresh one, or an injected global — subject to the constraints in RESEARCH.md.
- Gear icon design, popover geometry, and open/close interaction, within each
  plugin's own aesthetic.
- Batching/sequencing across the 43 plugins.

</decisions>

<specifics>
## Specific Ideas

- Reference implementations already in-tree:
  - `plugins/O-ReverseDelay/Source/ui/public/js/app.js` — the `data-tip` runtime
    (measure-then-pin geometry, `TOOLTIP_DELAY_MS`, edge clamp).
  - `plugins/O-MultiBandCompressor/Source/ui/public/js/app.js` — the JS table form
    (`GLOBAL_TOOLTIPS`, `BAND_TOOLTIPS`, `applyTooltip`, `setTooltipsEnabledNative`).
- Tooltip copy is `textContent`, never `innerHTML` — localized strings must stay
  inert. Preserve that on every path.
- Existing gates that must keep passing: `tests/ui_tooltip_clamp_check.js`
  (O-Bitrot, O-ReverseDelay), `tests/ui_frontend_check.js`,
  `tests/ui_layout_check.js` (O-Octagon).

</specifics>

<risks>
## Risks Flagged (accepted by user)

- **Hand-copy drift.** Per-plugin JS with no shared module means 43 independent
  copies of the i18n runtime and 43 copies of the gear popover. A later fix to the
  runtime has to be re-applied 43 times, and prior module-extraction work in this
  repo shows copies diverge silently. Accepted deliberately to match existing
  convention.
- **Content volume.** Authoring English tooltip copy for the 22 bare plugins is the
  largest single chunk of this task and is content work, not engineering.
- **Tooltip geometry.** French strings run longer than English; the width/clamp
  gates were verified against English copy at a fixed viewport. Re-verification is
  required, not optional.

</risks>

<canonical_refs>
## Canonical References

- `CLAUDE.md` — build/cache-clear protocol, trunk-based commit discipline
  (path-scoped commits, re-check staging immediately before every commit).
- `modules/registry.yaml` — module system, deliberately not extended by this task.

</canonical_refs>
