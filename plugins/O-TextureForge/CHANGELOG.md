# O-TextureForge Changelog

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
