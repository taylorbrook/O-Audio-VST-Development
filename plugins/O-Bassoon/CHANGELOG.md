# Changelog

All notable changes to O-Bassoon will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-08-28

The PAGE speaks French, not only a tooltip — because this plugin never had a
tooltip.

### Added

- **31 label keys and 5 accessible-name keys in a new `Resources/ui/js/i18n.js`**,
  covering the header strip, the three tabs, the four section headings, all ten
  knob captions, the Soft/Tongued end-label pair, the whole About card, and the
  one JS-written string on the page.
- **A settings popover in the header strip**, carrying the language selector.
  One row: this plugin has no hover-help to switch on or off. It opens
  downwards, dismisses on an outside `mousedown` and on Escape, and its width is
  pinned so its own rectangle is language-invariant.
- **A `getUiLanguage` / `setUiLanguage` native-function pair and session
  persistence.** `uiLanguage` is an `std::atomic<int>` behind a two-function
  string codec, saved as a readable `"en"` / `"fr"` property on the APVTS state
  tree — deliberately NOT an `AudioParameterChoice`, so it never appears in a
  DAW automation lane and no preset can change which language somebody reads
  their plugin in.
- **`plugins/O-Bassoon/tests/i18n-states.json`**, driving the About tab and the
  settings popover so their labels are measured rather than skipped.

### Changed

- **Every native `title=` is DELETED** (contract §4). All three were the only
  help their element had, so each text moved to `data-i18n-aria` unchanged —
  the vibrato dot, the breath meter and the voice-dot row. **No hover-help prose
  was invented**; authoring that copy is a later stage. `TIP_BINDINGS` and
  `I18N` are both empty, which `check-i18n` assertion 2 reports as
  "0 tip(s) bound" rather than passing silently.
- **The tuning-panel load-failure notice is built with `createElement` +
  `setLabel`** instead of an `innerHTML` string. It now declares its own key and
  becomes a `[data-i18n]` element, so a language switch after the failure
  re-renders it instead of stranding it in the previous language.
- **Two nodes were SPLIT** (contract §5). `Version 1.0.0` became a keyed caption
  beside an unkeyed number, so the shipping version never sits inside a
  translated string; `Made by <link>` had its caption keyed in its own span.
  Four section headings gained an inner `<span>` so `applyLabel`'s `textContent`
  write cannot delete the sibling it shares its flex row with.

### Notes

- **The `.knob-label` cap is the only tight box on this page** —
  `max-width: 78px` with `nowrap` + `ellipsis`, which clips SILENTLY. Every
  caption was measured as rendered at 900 × 600. The widest French is
  `PROFONDEUR` at 72.8px, 5.2px inside the cap. `Attack Char` (73.0px) became
  `Caractère` (62.0px) rather than the literal `Car. attaque` (76.2px): 1.8px of
  margin is inside the range where a Windows/WebView2 font metric decides
  whether a caption ellipsises, and Windows metrics are hardware-blocked.
- **Six of the visible labels SHRANK in French** rather than growing.
- **Geometry.** English v1.0.0 → v1.1.0: 1 of 121 elements moved — the header
  subtitle slid 32px left for the gear cluster — and the document scroll extent
  is unchanged at 900 × 600. English → French at a fixed frame: **0 non-label
  elements moved**, measured with a full, untruncated element path.
- **The About blurb is authored to the English LINE COUNT**, not merely
  translated. A shorter first draft shrank the card by one 19.4px line and
  pulled the byline up; the geometry diff caught it.
- **The Tuning tab stays English for a French user.** Every caption in it
  belongs to the shared `scala-tuning-engine` module, referenced by path from
  `CMakeLists.txt` rather than copied, so localizing it is a cross-plugin change
  and a local edit would be reverted by `/module-upgrade`.
- All French is machine-drafted and flagged `reviewed: false`. No native speaker
  has read it.
- No parameter IDs, ranges, types, DSP behaviour or preset content changed.
