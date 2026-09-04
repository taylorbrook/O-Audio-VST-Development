# Changelog

All notable changes to O-Texture will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `otex.tipsEnabled`.
- **`data-tip-always` on `#gear-btn` and on `#tips-toggle`, and on nothing
  else.** Those two controls are the ones that REACH and RESTORE the help
  layer, so they keep explaining themselves while it is off. `#lang-select`
  deliberately does not carry it: it is only reachable through the gear, which
  already explained itself on the way in.
- **Five i18n keys, four of them settled roots copied rather than authored.**
  `label.hoverHelp`, `ui.on`, `ui.off` and `aria.helpToggle` take the French
  glossary roots verbatim from `scripts/i18n-fr-glossary.js` — *Aide au survol
  / Marche / Arrêt / Activer ou désactiver l'aide au survol*. The fifth,
  `tip.tipsToggle`, is the tooltip's own title and body.

### Measured

- **The second row costs nothing.** With the popover forced open it occupies **y 51.5..115.69, 196 x 64.19 px — byte-identical in English and French** — inside an 800 x 600 frame. The switch face grows 42.00 -> 46.97 px for *Marche*, leftward into the panel's own slack; `check-ui-labels` [7] reports 0 non-label elements displaced.
- **The switch's face is a `min-width: 42px` floor, not a pinned width**, so a
  longer French face grows LEFTWARD into slack the popover already has. The row
  is `space-between` and the button is a `[data-i18n]` node, so nothing the
  geometry gate measures moves. `check-ui-labels` [7] reports **0 non-label
  elements displaced** between English and French, and the visible element set
  identical in both.
- Every declaration in `.settings-toggle` above the four switch-specific ones
  is **copied from this page's own language `<select>`** — its font stack, ink,
  plate, hairline and radius — so the two controls in the popover match by
  construction rather than by a second designer re-deciding them.

### Decided

- **Default is ON.** The previous version showed hover help unconditionally, so
  ON is the setting that leaves an existing user's plugin behaving exactly as
  it did. Default OFF would additionally have made `boot-all-uis --strict-tips`
  measure an empty tip surface and call it correct.

### Also driven

- `tests/ui_tip_render_check.js` [0] asserted the init line **byte-for-byte**
  (`try { initI18n(); setupTooltips(); } catch`), which goes red the moment a
  second guarded call is added. It now asserts the ORDER — `initI18n()` first,
  then the renderer — and separately that `initializeTipsToggle()` is inside
  that same guarded block, which is the property that actually matters: the
  toggle reads `setLabel`, and a top-level call reaching a lower `let`/`const`
  is the TDZ throw that takes the whole module with it.


## [0.3.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **10 of 26 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and lint (`scripts/i18n-fr-lint.js`), which
  went 13 findings to 0 with `--strict` exiting clean. 19 edits: 6 terminology,
  9 typographic, 2 grammar/register, 2 meaning. The visible ones:
  - **Mélange → Mix**, on the Mix caption, on its tooltip title and inside the
    Freeze tooltip that named the control. *Mix* is what a French DAW shows;
    *Mixage* is the mixing process and *Mélange* is a blend. It also retires the
    only French caption on this page that was wider than its English — 20.6 px
    against 45.5 — so nothing here grows in French any more.
  - **The Evolve tooltip is now titled *Évolution*** where the caption is still
    the width-pinned *Évol.*, matching what Character A and Character B already
    did and what the Freeze tooltip's own sentence already called the control.
    The English title is unchanged.
  - **Nine no-break spaces** — before every colon and semicolon and between
    *800* and *Hz*, as French typography requires and as no browser inserts for
    you.
  - **Two sentences that had drifted from the English**: the Character B tooltip
    had lost its closing range and now ends *De 0,00 à 1,00.* like every other
    body on the page, and the Freeze tooltip said *pendant ce temps* (meanwhile)
    where the English states a condition — *tant que le gel est actif*.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

No English copy, key, binding, selector, exemption or stylesheet rule changed,
and `reviewed: false` still stands on all 26 entries — this was a second machine
reading against a glossary, not a native-speaker review.

## [0.3.0] - 2026-08-30

Hover-help, in both languages — and the renderer that makes it visible, because
v0.2.0 had no way to paint one.

### Added
- **Eleven tooltips, English and French**, covering all ten APVTS parameters plus
  `#gear-btn` and `#lang-select`. Authored into `I18N` in
  `Source/ui/public/js/i18n.js` and bound in `TIP_BINDINGS`.
- **`setupTooltips()` in `Source/ui/public/js/main.js`, and its surface and
  styling.** Canon v2's `applyI18n()` writes `data-tip-title` and `data-tip` onto
  the bound anchors and stops there; the code that reads those attributes and
  paints a surface is per-plugin, and this page had none of it — no tooltip
  element, no `.tooltip` rule, no hover handler. **Copy alone would have shipped
  eleven invisible strings past three green gates:** `check-i18n` assertion 2 only
  counts bindings, `check-ui-labels` has no tooltip awareness whatsoever (its
  output is byte-identical before and after this whole change), and
  `boot-all-uis` counts `aria-label` and `title` and never `data-tip`.
  Ported from O-simpleFM's delegated, cursor-following renderer — delegated on
  `document` because no anchor carries `data-tip` until `applyI18n()` has run;
  `pointerover`/`pointerout`/`focusin`/`focusout` because those bubble;
  `createElement` + `textContent`, never `innerHTML`; flip to the other side of
  the cursor and then clamp on all four edges at 8 px. Styled in this page's own
  aged-paper vocabulary: the same ground, rule, shadow and Garamond
  `.settings-popover` already wears, with the title line in `--botanical-green-dark`.
- **`tests/ui_tip_render_check.js`** — 208 assertions, the gate no existing gate
  could stand in for. Drives the real page at the shipping 800 x 600 read out of
  `PluginEditor.cpp`, hovers all eleven anchors in `en` then `fr` then `en` again,
  asserts the rendered title and body are **byte-equal** to the table (not
  "contains" — a `.tip-title` that silently kept the previous anchor's text passes
  a contains check, and a deliberately broken title path produced 22 failures
  here), and asserts the rect is inside the frame on all four edges.
  `TIP_BINDINGS`, the `max-width` cap, the clamp margin and `setSize` are all
  PARSED from their sources, never retyped.
- **`.planning/params.tsv`, and the param-dump wiring that produced it** —
  `option(OUARICON_BUILD_TESTS)` plus `ouaricon_add_param_dump()` in
  `CMakeLists.txt`, and `PluginEditor.h` moved behind an `#if JUCE_WEB_BROWSER`
  guard in `PluginProcessor.cpp` with a `GenericAudioProcessorEditor` fallback, so
  the console dump target links without the editor TU. Under a normal build
  `JUCE_WEB_BROWSER=1` and behaviour is byte-identical.

### Fixed
- **A click would have parked a tip across the popover it just opened.** The
  reference renderer opens a tip on any `focusin`, and a mouse click on a
  `<button>` focuses it — so the tip `pointerdown` had just hidden reopens
  immediately, pointer still on the anchor and no further `pointerover` coming.
  Measured here with the latch removed: the gear's own tip covered the settings
  popover by **5130 px²**. A `lastInputWasPointer` latch, cleared by any keydown,
  gates the focus arm. `:focus-visible` is deliberately NOT the discriminator —
  Chromium reports it false for a programmatic `.focus()` after a click, so a gate
  driving focus directly would measure "no tip" and record that as correct.

### Notes
- **Ten parameters, nine parameter tips — arithmetic, not a gap.** `X` and `Y`
  share one control, the XY pad canvas, and `applyI18n()` writes the tip
  attributes onto the element a selector resolves to, so two bindings on
  `#xy-pad` would have the second overwrite the first and leave one entry
  permanently unrenderable. `tip.xyPad` names both axes instead. The gate asserts
  that every binding lands on a **distinct** element, so this cannot regress
  silently.
- **Every one of the ten parameters has an empty `label` in the dump — there is
  not one unit anywhere in the set.** Six are latent-space coordinates, one a
  drift rate, one a normalised level, one a bipolar tilt, two are choices. No unit
  was invented: each range is quoted the way the page renders that control
  (`main.js:361` and `main.js:466`, both `scaledValue.toFixed(2)`). The XY pad has
  **no readout node at all**, so `tip.xyPad` is the only range on this page taken
  from the dump's own `textAtMin`/`textAtMax` rather than from a formatter, and it
  is spelled with three decimals for that reason.
- **`Mix` is a level, not a blend, and the tooltip says so.** `processBlock` ends
  with a plain `applyGain(mix)`; Generate mode is a source, the sidechain input bus
  ships disabled and nothing reads it, so there is no dry path to balance against.
- **`Character A` and `Character B` are titled in full**, against the rule that the
  page's caption wins. "Char A" is an abbreviation forced by a hard 50 px column
  ("Caractère A" measures 51.5 px and wraps); the tooltip is the one surface here
  with room for the parameter's real display name, which is also the name the host
  shows.
- **Binding the ROW, not the buttons, keeps hover-help alive over the disabled
  ones.** Five of the six source buttons and one of the two mode buttons ship
  `disabled`, which is exactly where a user asks why nothing happens. Chromium
  retargets a pointer event over a disabled form control to the nearest enabled
  ancestor, so a tip on `.source-selector` / `.mode-toggle` opens over all of them.
  Measured and asserted, not assumed.
- **`z-index: 1200` on the surface is load-bearing.** `body::after` paints a
  180 x 180 fern at `z-index: 1000` over the bottom-right corner — directly on top
  of `.freeze-toggle` — so a surface at the popover's `z-index: 61` would be
  printed under it. A hover check reading only `visibility` cannot see that, so the
  gate asserts the computed `z-index` against the fern's.
- **No `tabindex` was added.** The XY pad, the three sliders and the two knobs are
  pointer-drag only and have never been keyboard-operable; a tab stop there would
  add noise for a control the keyboard still could not move and would pop a tip
  open mid click-drag. `#gear-btn`, `#lang-select` and the two enabled buttons
  carry the keyboard half.
- **Zero geometry movement.** `check-ui-labels --plugin O-Texture` is
  **byte-identical** to the pre-change baseline in all driven states, `moved=0`
  before and after, still 7 `[data-i18n]` elements measured, and no inert-element
  note appeared. The idle surface is `position: fixed` + `visibility: hidden` +
  `opacity: 0`, which that gate's visibility predicate rejects on all three counts.
  No geometry pin was added, so none is claimed and none is owed a negative control.
- **No hover-help on/off toggle.** The gear popover keeps exactly the language
  selector it had, and `tip.gearBtn` says so in as many words rather than promising
  O-Tapestop's toggle. Two of the suite's 43 plugins have one; making them agree is
  a decision across all 43.
- All eleven French bodies are machine drafts, `reviewed: false`. The
  native-speaker worklist for this plugin is now 26 entries (11 tooltip, 15 label).

## [0.2.0] - 2026-08-28

The PAGE speaks French, not only a tooltip — because this plugin never had a
tooltip. And the page now FITS ITS OWN FRAME, which it did not.

### Fixed
- **The layout grew without bound and pushed most of the UI off the bottom of the
  800 x 600 frame.** `#xy-pad` is a canvas, a replaced element whose intrinsic size
  is its `width`/`height` ATTRIBUTES, which `resizeCanvas()` writes from
  `clientWidth`/`clientHeight`. `.main-area` is a flex item and therefore carried
  `min-height: auto`, whose content-based minimum was floored by that intrinsic
  height — so every `ResizeObserver` delivery wrote an attribute that raised
  `.main-area`'s own floor, which raised the canvas's client box by the 2 px borders
  on `.xy-pad`, which the next delivery wrote back 4 px larger. A self-feeding loop,
  not a settling one. Measured at the shipping frame: `.main-area` 362 px at 100 ms,
  894 at 1.2 s, **5414 at 6 s**, still climbing. The XY pad, the source row and the
  whole bottom strip left the frame within the first second, and
  `html { overflow: hidden }` meant they were simply not reachable. `min-height: 0`
  on `.main-area` removes the floor; the area now resolves to the 306 px the design
  intends and holds it at every settle time from 100 ms to 6 s. Introduced in v0.1.2
  by the IN-10 `ResizeObserver` change.

### Added
- **Language selector, in a gear popover beside the title.** Styled in this page's own
  vocabulary — aged-paper fills, the single 1 px `--ink-brown` rule `.mode-toggle`
  already uses, the `--botanical-green` active state, Garamond — rather than pasted in
  from another plugin. Opens downwards; the gear sits 21 px from the top of a 600 px
  frame. One row, because there is no hover-help to switch on or off.
- **`Source/ui/public/js/i18n.js`** — the label table, English + French, on canon v2.
  Embedded in `juce_add_binary_data` SOURCES *and* served from a `getResource()` branch
  in the same commit: a file embedded but not served is a 404 that presents as a page
  stuck in English and nothing else.
- **The UI language persists with the session.** A non-parameter `uiLanguage` property
  on the APVTS state tree beside `evolve_seed` and `evolve_cursors`, saved as
  `"en"`/`"fr"` and read back with `hasProperty()` + `toString()` — the XML round-trip
  rebuilds every property as a var over the attribute STRING, so an `isBool()`/`isInt()`
  test would be false for every saved session. Deliberately not an
  `AudioParameterChoice`: the language must not appear in a DAW automation lane and a
  preset must not be able to change which language somebody reads their plugin in.

### Changed
- Seven visible strings localize: Char A, Char B, Evolve, Brightness, Mix, Freeze and
  the popover's own Language caption. **Nine do not, and each says why in
  `I18N_EXEMPT`:** the six source captions (Rain, Metal, Wind, Crowd, Synth, Organic)
  and the two mode captions (Generate, Transform) are the `SOURCE` and `MODE`
  `AudioParameterChoice` option strings byte for byte, so translating the caption alone
  would make the page and the host automation lane disagree about the same setting. The
  h1 is a product name.
- **All six native `title="Coming soon"` attributes are deleted.** Each one's text moves
  into that control's accessible NAME, keeping the visible caption in it —
  `aria-label="Coming soon"` alone would have REPLACED "Metal" as the accessible name of
  the button whose visible caption is Metal, breaking the label-in-name match. No new
  prose was invented: every name is the control's own caption plus the status text the
  title already carried.

### Notes
- All French is a machine draft, every entry flagged `reviewed: false`. No native speaker
  has read it.
- No hover-help copy was authored: `TIP_BINDINGS` and `I18N` are both empty, which is this
  plugin's correct state rather than a gap. Authoring that prose is a later stage's job.
- Zero geometry movement from the localization. **0 of 75 English elements moved**, 9
  added (the gear cluster and its popover); the header's `space-between` still puts
  `.mode-toggle` at exactly `[590.44, 20, 185.56, 25]`. No non-label element moves
  between English and French. **Six of the seven page labels get SHORTER in French**, not
  longer — the half a clip check is blind to.
- No parameter IDs, ranges, types or DSP behaviour changed.

## [0.1.2] - 2026-07-15

Resolves CODE_REVIEW.md info findings IN-01 and IN-03–IN-10 (IN-02 was already
resolved incidentally by CR-01 in v0.1.1).

### Fixed

- **IN-04: CMake hardcoded the macOS-arm64 ONNX Runtime path.** Root cause: the
  post-build embed step baked in `onnxruntime-1.19.2-macOS-arm64`, which breaks
  x86_64/universal builds. The dylib path is now derived from ANIRA's exported
  `ANIRA_ONNXRUNTIME_SHARED_LIB_PATH` (arch/OS-correct), with per-platform lib
  names for future Windows/Linux builds.
- **IN-09: Perlin cursors grew unboundedly.** Root cause: `cursors[ch] += step`
  forever — after days of playback float ULP exceeds small steps and the evolve
  modulation quantizes, then stalls. The noise is periodic in 256 (`hashAt` masks
  with `& 0xFF`), so cursors now wrap losslessly at 256 in `advance()`.
- **IN-10: Canvas backing store sized once at DOMContentLoaded.** Root cause: a
  cold WebView can fire DOMContentLoaded before the stylesheet settles, mis-sizing
  the XY pad. The canvas is now sized by a `ResizeObserver` with a DPR-aware
  backing store (`clientWidth × devicePixelRatio` + `setTransform`) for crisp
  Retina rendering.
- **IN-08: Dead fallback in the processBlock read loop.** The unreachable
  `samplesToRead <= 0` branch (which would have read across a hop boundary) is
  replaced with a `jassert` plus a defensive `break` that prevents an infinite
  loop if the OLA invariant is ever broken.

### Changed

- **IN-05: ANIRA unlinked from the plugin.** Its inference engine was never used
  (v0.1.1's CR-01 fix runs raw ONNX Runtime on a dedicated thread). The ANIRA
  fetch is kept solely as the cross-platform ONNX Runtime downloader;
  `libonnxruntime` is linked directly and `libanira.dylib` (384 KB × 3 formats)
  is no longer embedded in the bundles.
- **IN-06: encoder.onnx / prior.onnx removed from BinaryData** (~140 KB). They
  stay on disk under `Resources/models/placeholder/` and will be re-embedded when
  Transform mode lands.
- **IN-01: Dead `decoderReady` flag removed** (written in three places, never read).
- **IN-03: Unused JS removed** — `getBackendResourceAddress` import and the
  unused `thumb` lookup in `bindVerticalSlider`.

### Not Changed (investigated)

- **IN-07: MIDI input is kept, now documented as load-bearing.** Removing
  `NEEDS_MIDI_INPUT`/`acceptsMidi` was attempted and reverted: an `aumu`
  MusicDevice must accept MIDI — auval fails `MusicDeviceMIDIEventList` with
  error -4 without it. Comments at both declaration sites now explain why the
  (currently unused) MIDI input must stay.

### Testing

- Built VST3 + AU, installed with cache clear + dual-variant sweep.
- `auval -v aumu OuTx OuDv` PASS; pluginval strictness 5 PASS.
- `main.js` module parse check clean (no load-time errors that would blank the UI).

## [0.1.1] - 2026-07-15

Resolves CODE_REVIEW.md findings CR-01, CR-02, WR-01–WR-07 (IN-02 incidentally).

### Fixed

- **CR-01: Neural inference moved off the audio thread.** Root cause: `processBlock`
  ran `Ort::Session::Run` synchronously per hop, heap-allocated `Ort::MemoryInfo`/tensor
  wrappers per call, and logged from the audio-thread catch path. Now a dedicated
  inference thread (`juce::Thread`, 1 ms poll, high priority) decodes; the audio thread
  publishes latent vectors and consumes finished blocks through a seq-counter handoff
  (`reqSeq`/`doneSeq`, one request in flight, no locks). A missed hop plays the OLA
  Hann tail and retries — never blocks. Offline renders (`isNonRealtime()`) decode
  synchronously so bounces stay deterministic. `Ort::MemoryInfo` is created once in
  `initDecoderSession()`; the catch path sets an atomic `decodeError` flag instead of
  logging. Output tensors now bind directly to caller buffers, removing the shared
  intermediate buffer and its 16 KB copy per hop (also resolves IN-02).
- **CR-02: Out-of-bounds writes for >stereo layouts.** Root cause: no
  `isBusesLayoutSupported` override, so hosts could open the output bus as 5.1/7.1
  while `TiltFilter` was prepared for 2 channels — `lpState[ch]` indexed past the
  vector. Added the override (stereo out; sidechain disabled-or-stereo), clamped
  `TiltFilter::processBlock` to the prepared channel count, and `prepareToPlay` now
  passes the real output channel count.
- **WR-01: Data races on Perlin noise state.** Root cause: `setStateInformation`
  (host thread) rebuilt the permutation table and cursors while the audio thread read
  them. Restored seed/cursors are now staged under a `SpinLock` and applied by the
  audio thread at the top of `processBlock` (try-lock, never blocks); the audio thread
  publishes a per-hop seed/cursor snapshot that `getStateInformation` reads. If a
  restore hasn't been applied yet (no audio running), saving passes the staged values
  through verbatim so load→save round-trips.
- **WR-02: Restored evolve state wiped by `prepareToPlay`.** Root cause: unconditional
  `evolveNoise.reset()` in `prepareToPlay` discarded the restored evolution position
  in the standard host call order (setState → prepare). The reset is removed — Perlin
  state is hop-domain and sample-rate independent.
- **WR-03: FREEZE didn't freeze; sound not reproducible.** Root cause: inactive latent
  dims were re-randomized every hop from a wall-clock-seeded RNG. Inactive-dim values
  are now drawn once per seed (`regenerateInactiveValues`), so FREEZE holds still,
  EVOLVE=0 is static, and a saved project reproduces its texture from the persisted seed.
- **WR-04: Phantom 6144-sample latency (~128 ms PDC).** Root cause: latency reported
  as `ACCUM_SIZE` although the generator outputs from sample 0 with no input-to-output
  delay. Now reports 0.
- **WR-05: `M_PI` broke MSVC builds.** Replaced with a literal `twoPi` constant in
  `HannWindow.h` (header is deliberately JUCE-free).
- **WR-06: Double-click reset used 0.5 for every control.** Root cause: hardcoded
  normalized reset in JS drifted from C++ defaults (MIX=1.0, EVOLVE=0.3). Reset now
  takes a per-control default matching `createParameterLayout()`.
- **WR-07: Dead SOURCE/MODE controls presented as live.** Only the Rain model exists
  in v0.1.x, and Transform mode is unimplemented. The five other source buttons and
  the Transform toggle are now disabled with "Coming soon" tooltips. The SOURCE/MODE
  parameters (and sidechain bus) are intentionally kept so the parameter contract is
  stable when real models land.

### Testing

- Built VST3 + AU, installed with dual-variant sweep, `auval` validation.

## [0.1.0] - 2026-02-15

### Added

- Neural texture synthesis engine (1D CNN VAE, 32-dimensional latent space)
- 10 real-time parameters: Source, Mode, X, Y, Character A/B, Evolve, Freeze, Brightness, Mix
- 6 source categories: Rain, Metal, Wind, Crowd, Synth, Organic
- Generate and Transform modes (IS_SYNTH instrument plugin)
- XY pad with orbital trail animation (Canvas 2D, 30fps throttled)
- Evolve modulation via 28-channel Perlin noise with quintic interpolation
- Freeze mode (halts latent evolution, pauses trail animation)
- Brightness tilt filter (1-pole, 800 Hz pivot, SmoothedValue for zipper-free)
- Stereo decorrelation via latent offset (0.1 on X/Y dimensions)
- Overlap-add crossfading (4096-sample blocks, 2048-sample hop, Hann window)
- ONNX Runtime decoder inference (direct C++ API, synchronous)
- Ouaricon Naturalist WebView UI (aged paper, botanical motifs, serif typography)
- 3 vertical sliders (Character A, B, Evolve) with naturalist styling
- 6 source icon buttons with inline SVG line art
- 2 rotary knobs (Brightness, Mix) with seed cross-section visuals
- Fern botanical overlay (bottom-right, low opacity)
- State serialization including evolve noise seed and cursor positions
- Full JUCE 8 WebView relay/attachment parameter binding system
- ANIRA v2.0.3 + ONNX Runtime 1.19.2 embedded in plugin bundles (macOS)
- Ad-hoc code signing for local testing

### Technical Notes

- Uses placeholder ONNX models; real trained models will be integrated in a future version
- Plugin registers as AU instrument (aumu OuTx OuDv)
- JUCE 8.0.4, C++20
- Latent space mapping loaded from dim_map_rain.json (BinaryData)
- Pre-allocated decoder output buffer for real-time safety
