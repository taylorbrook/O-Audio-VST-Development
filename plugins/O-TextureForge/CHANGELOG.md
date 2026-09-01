# O-TextureForge Changelog

## [1.2.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **17 of 41 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint, which went from 19 findings to 0
  under `--strict`. Ten carry a terminology or idiom fix, thirteen a typography
  fix, seven a grammar or agreement fix and two a meaning fix. The most visible:
  the GRAIN SIZE caption is now **Taille de grain** (the glossary root, not the
  v1.2.0 abbreviation), CROSSFADE is **Fondu enchaîné** rather than *Fondu* —
  *fondu* alone is a fade, not a crossfade — a *bed* of grains is a **nappe**
  rather than a literal *lit*, and no-break spaces now sit before `%`, `;` and
  `:` and between every number and its unit, as French typography requires.
- **Two French sentences said something the English does not.** The TEXTURE tip
  listed two of the English's three qualities and has its third restored
  (*la rugosité*, for "grit"); the DENSITY tip said the high *values* overlap
  when it is the *grains* that overlap. Neither language's meaning was invented
  — the English was checked against `GrainScheduler.cpp` first.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.
- Four tip titles that are the same word in both languages — Texture, Variation,
  Position, Gain — now carry the explicit `sameAsEn: true` declaration, so an
  untranslated string cannot hide among them as a coincidence.

### Notes
- `reviewed: false` is unchanged on all 41 entries. That flag means *a native
  speaker read this*, and none has; this pass is a second machine reading
  against a glossary and a lint, recorded in the `i18n.js` header.
- Scatter X / Y keep **Disp. X** / **Disp. Y**. The glossary root *Dispersion X*
  measures 65.14 px and wraps to two lines inside the hard 72 px caption box,
  taking the label from 10 px tall to 20.

## [1.2.0] - 2026-08-30

### Added
- Hover-help, in both languages. 14 tooltips — one for each of the 12
  parameters, plus the settings gear and the language selector — each with a
  title and a two-or-three sentence body ending in the control's range and unit.
  Every French body is machine-drafted and flagged `reviewed: false`; no native
  speaker has read them.
- A tooltip RENDERER, because authoring the copy was not the whole job. Canon
  v2's `applyI18n()` writes `data-tip-title` and `data-tip` attributes onto the
  anchors and stops there; the code that reads them and paints a surface is
  per-plugin, and this page had none — `id="tooltip"` 0, `.tooltip {` 0,
  `closest("[data-tip]")` 0. Binding 14 entries without it would have shipped 14
  invisible strings past three green gates. Ported behaviourally from
  O-simpleFM: one delegated, cursor-following surface, clamped on all four edges
  with an 8 px margin, `pointer-events: none`, hidden until hovered.
- The renderer and its styling live in `Source/ui/public/js/i18n_init.js` and
  the page's own stylesheet, NOT in `Source/ui/src/app.js`. This is the suite's
  only webpack-bundled page, and putting them in the bundle input would mean a
  webpack rebuild inside every copy change.
- A last-input-device latch on the focus arm. A mouse click on a `<button>`
  focuses it, so an unconditional `focusin` rule parks a tip on screen after
  every click — measured here as **4648 px²** of the gear's own tip lying across
  the settings popover the click had just opened. `:focus-visible` is not the
  discriminator; Chromium reports it false for a programmatic `.focus()` after a
  click. Any keydown clears the latch, so the keyboard half of hover-help still
  works.
- A drag guard, which the reference renderer does not have and this page needs.
  Every knob starts its drag on `mousedown` and tracks `document.mousemove`, so
  a vertical drag straying into a neighbouring `.knob-row` would otherwise open
  that row's tip on top of the control being turned.
- `tests/ui_tip_render_check.js` — 287 assertions at the shipping 900 × 600.
  No existing gate can see a rendered tooltip: `check-i18n` reads the table
  statically, `check-ui-labels` has no tooltip awareness at all, and
  `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. With
  `setupTooltips()` commented out, this file reports 36 failures while the other
  two stay green — `check-ui-labels` byte-identically so.
- `.planning/params.tsv`, the runtime parameter inventory, and the
  `OUARICON_BUILD_TESTS` / `ouaricon_add_param_dump()` block that produces it.

### Changed
- `PluginProcessor.cpp` puts `#include "PluginEditor.h"` and all five uses of
  `TextureForgeEditor` behind `#if JUCE_WEB_BROWSER` — `createEditor()` plus the
  four `dynamic_cast<TextureForgeEditor*>(getActiveEditor())` sites in
  `setStateInformation()` and `loadCorpusFile()`. The param-dump console target
  compiles this TU with `JUCE_WEB_BROWSER=0` and no editor sources. Under a
  normal build `JUCE_WEB_BROWSER=1`, every guarded arm is the original code
  verbatim, and behaviour is byte-identical to v1.1.0.
- Two `I18N_EXEMPT` reasons corrected. Both said `50ms` and `0 dB` were "written
  by src/app.js"; they are not written at all. See below.

### Found and NOT fixed
- **Three `.knob-value` readouts render BLANK, and have since v1.0.0.**
  `setupKnob` is passed `(n) => ''` as the formatter for `grainSize`,
  `grainDensity` and `outputGain` (`Source/ui/src/app.js:705`, `:706`, `:708`),
  so the first `updateDisplay()` erases the authored markup fallbacks `50ms`,
  `8` and `0 dB` and nothing replaces them. Measured in the headless harness,
  not reasoned. Repairing it means editing the webpack INPUT and rebuilding
  `app.bundle.js`, which is out of scope for a hover-help commit and a 220 KB
  unreviewable diff. The three ranges in the new tooltip bodies therefore come
  from the parameter dump rather than from the page's formatter, and say so.
- **A fresh CMake configure of this repo cannot build this plugin.** The root
  `CMakeLists.txt` caches `CMAKE_OSX_DEPLOYMENT_TARGET` at 10.13, and
  `_deps/knncolle-src` needs `std::filesystem` (macOS 10.15+). The committed
  `build/` works only because its cache says 11.0. Root-CMakeLists-owned, one
  plugin, latent fresh-clone / CI hazard.
- `Source/ui/package.json` still declares `"version": "1.0.0"`. It is private,
  not host-visible, and was already stale at v1.1.0.

### Not changed
- No parameter IDs, ranges, types, defaults or DSP behaviour.
- No geometry. `check-ui-labels` output is byte-identical to the v1.1.0
  baseline — 0 non-label elements moved in either direction, and the tooltip
  node does not enter the label sweep. **No pin was added, so none is owed a
  negative control.**
- `js/app.bundle.js` is untouched — verified by checksum.
- No hover-help on/off toggle. Two plugins in the suite have one and forty-one
  do not; making this the forty-second is a uniformity decision, not a side
  effect of a copy commit.
- Readouts stay English in both languages, per D-03.

## [1.1.0] - 2026-08-28

### Added
- The PAGE speaks French. 21 HTML captions and 8 JavaScript-written messages are
  localized through a new `Source/ui/public/js/i18n.js` label table (en + fr),
  plus 3 keyed accessible names. Every French string is machine-drafted and
  flagged `reviewed: false` — no native speaker has read them.
- A settings popover beside the plugin name, carrying the language selector,
  styled in this plugin's own naturalist vocabulary.
- `getUiLanguage` / `setUiLanguage` native functions and session persistence.
  The language rides the saved state as a `UILANG` XML child beside `CORPUS` —
  this processor's own idiom for non-parameter state — and is deliberately NOT
  an `AudioParameterChoice`: it must not appear in a DAW automation lane, and a
  preset must not decide which language somebody reads their plugin in.
- `Source/ui/public/js/i18n_init.js`, a served ES module carrying the canonical
  i18n runtime. This is the first webpack-BUNDLED page in the suite: the canon
  block cannot live in `src/app.js`, because webpack would resolve its
  `import './i18n.js'` at build time and inline the label table into
  `app.bundle.js`, leaving the embedded, served `js/i18n.js` read by nobody.
- `tests/i18n-states.json`, so the label gate can open the popover and measure
  the captions inside it.

### Changed
- `.bottom-knobs` is a five-column grid rather than `flex` + `space-around`.
  Under space-around the five shrink-wrapped knob columns took their positions
  from every caption's rendered width, in both directions, so a French caption
  moved the whole row. A 1fr grid makes each column 173.6 px whatever it holds.
  The English knobs move by at most 3.12 px and the row is now evenly spaced.
- `.scatter-placeholder` has a fixed 320 px width. It is absolutely positioned
  and centred with `translate(-50%)`, so it shrink-wrapped its message and moved
  its own box as the text changed. Its visible centre is unchanged to 0.00 px.
- The scatter placeholder's text moved into its own span. Through 1.0.2 the
  placeholder was rewritten with `innerHTML` in three places, re-authoring the
  fleuron glyph each time; the glyph is now authored once and only the text is
  written, so no localized string passes through a markup path.
- No native `title=` attribute was removed, because 1.0.2 carried none.

### Not changed
- No parameter IDs, ranges, types, defaults or DSP behaviour.
- The three MIDI-mode option captions stay English in French: they are the
  `MIDI_MODE` `AudioParameterChoice` option strings byte for byte, and the page
  and the host automation lane must agree about the same setting.
- Readouts stay English in both languages — `50 ms`, `0 dB`, `50%`.

## [1.0.1] - 2026-02-15

### Fixed
- File loading was non-functional: WebView intercepted all drag-and-drop events before they reached the JUCE FileDragAndDropTarget
- Added `browseForFile` native function using juce::FileChooser for reliable file selection
- Wired click handlers on drop zone and scatter placeholder to open file browser dialog
- Large file warning (>100MB) now also works through file browser path

## [1.0.0] - 2026-02-15

### Added
- Initial release: concatenative texture synthesis engine
- 19D MFCC descriptor extraction with PCA and UMAP scatter visualization
- Real-time granular playback with KD-tree nearest-neighbor search
- Timbral macro controls (Energy, Brightness, Texture)
- Scatter position controls with variation radius
- Three MIDI modes: Pitch-Mapped, Trigger + Modulate, Generative Drone
