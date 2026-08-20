# O-SpectralShaper Changelog

## [1.6.2] - 2026-08-20

### Fixed
- **The editor no longer freezes when audio is processed faster than realtime.** Opening the UI during an offline bounce — or any render that is not rate-limited to the clock — could hang the whole message thread indefinitely: the window never finished appearing and the plugin stopped responding until the host was killed. Realtime playback was never affected, which is why this survived to release.
- **Root cause:** the editor's 60 Hz visualization drain was `while (fifo.getNumReady() > 0)`, re-asking the audio thread how many frames were pending on *every* iteration. The producer is one frame per FFT hop (`HOP_SIZE` 256 → 187 frames/sec at 48 kHz), so at realtime the loop drains ~3 frames and the count reaches zero. Under a faster-than-realtime render the audio thread refills the FIFO faster than the message thread can emit, the count never reaches zero, and `timerCallback()` never returns — starving the message loop that WebView2 needs to finish opening. **Fix:** snapshot `getNumReady()` once before the loop and read exactly that many frames, so frames arriving mid-drain wait for the next tick instead of extending the current one. Emission is additionally capped at the 16 newest frames of the snapshot, with the entire snapshot retired via `finishedRead()` whether or not it was emitted (leaving skipped frames ready would re-create the same stall on the following tick).
- **Caught by:** Windows `pluginval` strictness 10, which hung in *"Open editor whilst processing"* until its 10-minute timeout and failed the v1.6.1 release build. The plain *"Editor"* test — which opens the editor with no concurrent `processBlock` — passed in 1.0 s, isolating the fault to the concurrent path.

### Changed
- **Visualization frames are no longer serialized when the browser is hidden.** `emitVisualizationFrame` now returns before building its ~4 kB JSON payload (289 float→string conversions) if the WebView is not visible. The gate is `isVisible()` — deliberately the exact condition JUCE tests inside `emitEventIfBrowserIsVisible()`, not the stricter `isShowing()` — so it can only skip payloads that were already going to be discarded, and cannot stall the spectrogram during window reparenting.

### Notes
- No DSP, parameter, preset-format or state-format changes. Presets and sessions from 1.6.x load unchanged; the audio path is untouched.
- The 16-frame cap is clear of every realtime hop rate — ~3 frames/tick at 48 kHz, ~13 at 192 kHz — so no frame is dropped during normal playback at any supported sample rate. It engages only when a render outruns the clock, where the extra columns are stale before they could be drawn.
- This is the first O-SpectralShaper release to pass through the Windows `pluginval` gate: `ci-tests.yml` runs `pluginval` on O-Octagon only, so the strictness-10 editor tests are first reached at release-tag time by `build-and-release.yml`. The defect dates from the Phase 3.3 visualization work, not from 1.6.1.

## [1.6.1] - 2026-08-19

### Fixed
- **Curve editors now update when a preset is loaded.** Every preset already carried full 32-band attack/sustain curve data (all 29 factory presets since 1.6.0 via `makeCurveState`, user presets via the preset manager's `customState`), and loading one did change the DSP — but the WebView curve editors received curve data exactly once, 100 ms after the editor first attached (`parentHierarchyChanged`). No load path (menu, ◀ ▶ arrows, `loadPreset`, load-from-file, session restore with the editor open) ever re-sent the curves, so the editors kept drawing the previous preset's shapes and the bank *looked* like it shipped without curve settings.
- **Root cause:** one-shot C++→JS curve push with no notification on state-driven curve replacement. **Fix:** the processor keeps a `curvesRevision` atomic bumped *only* in the preset manager's `customLoad` callback — deliberately not in `setAttackCurve`/`setSustainCurve`, so the UI's own drag-edits never echo back into an in-progress drag. The editor's existing 60 fps timer polls the revision and re-sends both curves through the existing `sendAttackCurveToJS`/`sendSustainCurveToJS` path when it changes. This covers every load path, including session restore, with no new bridge functions.

### Notes
- No DSP, parameter, preset-format or state-format changes. Presets and sessions from 1.6.0 load unchanged.
- The timer send is gated on `hasNavigated`; before the page exists the revision simply stays pending and is delivered on the first tick after navigation. `sendCurveToJS` already guards with `if (window.fn)` on the JS side.

## [1.6.0] - 2026-08-19

### Added
- **The preset readout is now a click-to-open menu, grouped by category.** Clicking the preset name in the header drops a scrollable menu of the whole bank under seven narrative headings — Essentials, Drums & Percussion, Cymbals & Air, Vocals & Speech, Instruments, Mix & Master, Creative — with sticky category headers, the loaded preset highlighted and scrolled into view, and a `▾` affordance on the readout. Selection goes through the same `loadPreset()` path the ◀ ▶ arrows use, so the menu and the arrows cannot disagree about what is loaded. Escape or a click anywhere outside closes it.
- **Factory bank grown 9 → 29 presets.** Twenty new presets, each with purpose-authored 32-band attack/sustain curves (band frequencies taken from the analyzer's log spacing, values in the ±12 dB curve range) and parameter values in engineering units converted through each parameter's own `NormalisableRange` — the CR-02 skew lesson applied from the start. New: Extra Snap; Kick Tightener, Snare Crack, Tom Focus, Room Tamer, Percussion Sparkle; Hat De-Harsh, Shimmer Sustain; Plosive Guard, Vocal Presence, Breath & Air; Strum Snap, Piano Hammer, Bass Definition, String Swell, Pick Bite; Low-End Tightener, Master Polish; Attack Eraser, Infinite Bloom. All nine pre-existing presets keep their exact values and names.
- **`getPresetListGrouped` native function (ported from O-Bitrot v1.13.0).** Categories are expressed as index *spans* over the factory vector's declaration order — never a second list of names, which would go stale silently on the first rename — and the constructor asserts the spans tile the bank exactly. The function returns an ordered array of `{category, presets}` sections cross-checked against the live preset list, then a "User" section holding everything else on disk (omitted when empty).

### Fixed
- **The ◀ ▶ arrows now step through the menu's grouped order.** The preset-manager module's native prev/next walk the C++ flat *alphabetical* list, which matched the old flat display only by coincidence; against a grouped menu, ▶ from the last drum preset would land mid-way through another category (`pattern_grouping_preset_dropdown_breaks_prev_next`). The arrows are now bound in `app.js` to the flattened menu order (`presetWalkOrder`), wrapping at the ends; a preset loaded from a file enters the walk at the top going forward, the bottom going back.

### Notes
- No DSP and no parameter changed: same 7 parameter IDs, same ranges, same state format. Existing sessions and user presets load unchanged; the 20 new factory presets appear on first launch (factory files are refreshed at startup).
- Verified by a new headless gate, `tests/ui_preset_menu_check.js` (ported from O-Bitrot): it derives the expected grouping from `PluginProcessor.cpp` itself, holds the browser stub and the rendered DOM to it, and drives the real page at the shipping 700×500 — 32/32 checks. The walk-order probe was negative-controlled: rebinding the arrows to the module's alphabetical navigation makes exactly that check fail (▶ from "Cymbal Control" lands on "De-Esser" instead of "Hat De-Harsh").
- The category header background is opaque (`--paper-accent`) because the headers are `position: sticky` — a translucent tint lets rows show through once a header pins. The `▾` affordance is a CSS `::after`, not a child node, because the module's `_updateDisplay()` writes `textContent` to the readout and would erase any real child on the first preset change.

## [1.5.0] - 2026-08-13

### Added
- **Hover tooltips across the whole interface, gated by a "?" toggle in the header.** Every control now carries a `data-tooltip` describing what it does and its real range — 25 in total: the seven knobs/toggles, both curve editors and their ten buttons, the spectrogram, and the five preset-bar controls. Tooltips are off by default and only arm when the "?" beside the version string is lit, so the field-guide layout stays uncluttered for users who don't need them. Ranges quoted in the text are taken from the actual `NormalisableRange` definitions (Attack 0.1–50 ms, Sustain 10–500 ms, LA Time 0.1–10 ms, Output −12 to +12 dB) and the curve range from `STFTProcessor::MAX_SHAPE_DB` (±12 dB), rather than being written from the UI labels.
- **Tooltip preference persists with the session.** The toggle state round-trips through two new native functions (`setTooltipsEnabled` / `getTooltipsEnabled`) into a `tooltipsEnabled` attribute on the session XML. The WebView *pulls* the stored value during its own init rather than having the editor push it on open, which would race the WebView load. The attribute is stamped on after `presetManager.getStateAsXml()` has built the tree, so a UI preference never leaks into a saved preset file — loading a preset cannot change whether your tooltips are on.

### Changed
- **The nine existing `title=` attributes became `data-tooltip`.** They previously produced the OS's own native tooltip; leaving them in place alongside the new system would have shown two overlapping tooltips on the same control. They are now part of the toggle-gated system and share its styling.

### Fixed
- **Tooltip measurement is done at a neutral origin before placing.** An absolutely positioned element's shrink-to-fit width is computed against (containing-block width − `left`), so measuring the surface while it still sits at its previous position reports a wrapped, narrow box near the right edge and the edge-clamp then mispositions it. Measured against the Lookahead tooltip at `left: 600px`, the naive ordering collapses the surface to **100 × 269 px** instead of the correct **240 × 110 px**. The surface is now reset to `0,0` with `width: auto`, measured, and its width pinned in px before the final placement is applied. (The reference implementation in O-FreqPulse v1.5.0 measures in the naive order and carries this latent bug — flagged for a separate pass.)

### Notes
- UI-only release. No DSP, parameter, preset-format or state-format changes: parameter IDs, ranges and factory presets are untouched, and the added session attribute is optional on read (absent in pre-1.5.0 sessions ⇒ defaults to off), so existing sessions and presets load unchanged.
- The Lookahead tooltips state plainly that the control is inert rather than describing behaviour it does not have, matching the Known Limitation carried since 1.3.2.
- Verified in a headless WebView harness against a stubbed JUCE bridge: all 25 tooltips resolve their text, become visible only while armed, and stay inside the 700×500 window (0 overflowing, 0 collapsed, widths 206–240 px). Toggle off/on, hover-while-disabled, and mouseout were each asserted, as was the C++ persistence round-trip.
- The comment at `STFTProcessor.cpp:320` describes the curve range as ±18 dB while `MAX_SHAPE_DB` is `12.0f`. The tooltips follow the constant. The stale comment is left as-is here — flagged, not fixed, to keep this release UI-only.

## [1.4.0] - 2026-08-12

### Changed
- **UI reskinned to the Ouaricon Naturalist brand aesthetic.** The interface was a generic dark charcoal theme (`#1A1A1A`) with modern blue/orange accents, Georgia type and plain dark-disc knobs — it carried none of the house style. It is now a field-guide page: aged-paper ground, Garamond typography, warm earth palette (walnut `#8B7355` / oak `#5C4033` / `#3C2F2F` text), botanical seed cross-section knobs, green botanical toggle and buttons, and fleuron ornaments.
- **Analysis displays kept as dark specimen plates.** The spectrogram and both curve editors retain dark grounds, now set in 3px walnut frames with inset shadow so they read as photographic plates mounted on the paper page. The WebGL inferno colormap is unchanged — spectral legibility was the reason to keep these areas dark rather than invert them to ink-on-cream.
- **Curve accents moved from modern blue/orange to earth tones** that stay legible on the dark plate: attack moss `#9BB877`, sustain ochre `#D4A257`. These were duplicated as string literals in three places in `app.js`; they are now a single `ACCENT_COLORS` constant mirroring the `--accent-attack` / `--accent-sustain` CSS custom properties.
- **Botanical specimen made visible.** The plugin already shipped a nudibranch illustration (after Trinchese, lith. Armanino, *Atti della R. Università di Genova*, Vol. II, Tav. VI — public domain) but rendered it at 0.08 opacity where it was effectively invisible, and as a dark-plate image that could not sit on a light ground. It is now converted to sepia ink on a transparent ground and placed right-side per the house spec at 0.42 opacity, bleeding off the edge behind the control column.

### Fixed
- **Three controls were unreachable.** The knob sidebar laid seven controls out in a single flex column whose content ran 638px tall inside a 418px container — a 220px overflow with `overflow` unset, so **Lookahead, LA Time and Output Gain were all clipped off the bottom of the window with no way to scroll to them**. Output Gain in particular has been inaccessible from the UI since the sidebar was introduced. The sidebar is now a two-column grid (Mix/Attack, Sustain/Sensitivity, Output/LA Time, with the Lookahead toggle spanning both columns); all seven controls fit with 60px of vertical slack, verified stable across the Garamond, Times New Roman and Georgia font fallbacks.
- **Header version string was stale:** `index.html` hard-coded `v1.3.0` while the plugin shipped as 1.3.2. Now reads v1.4.0.

### Removed
- **Watermarked stock background texture.** `Resources/ui/images/paper-bg.webp` was a tiled-"Adobe Stock"-watermarked image (visible when brightened; it went unnoticed because it rendered at 0.1 opacity over a dark background). It was also a *dark navy* grunge texture, unusable for the aged-paper ground. Replaced with the clean aged-paper texture already used by O-Tremolo, re-encoded to WebP at 700×500. Filename is unchanged, so the `juce_add_binary_data` list needed no edit.

### Notes
- Visual restyle only — no DSP, parameter, preset-format or state changes. Parameter IDs, ranges, factory presets and the saved-state format are untouched, so existing sessions and presets load unchanged.
- The Lookahead control remains inert (see the 1.3.2 Known Limitations); this release only makes it reachable, it does not change its behaviour.
- Two sibling plugins still ship the same watermarked texture — `O-Lyrica/Resources/ui/images/paper1.jpg` and `O-Gain/Source/ui/public/images/paper1.jpg` are byte-identical (md5 `b7c865c45f2fb95a7a8651071da186e6`). Out of scope here; flagged for a separate pass.

## [1.3.2] - 2026-07-07

### Fixed
- **CR-01 — Preset file-dialog use-after-free:** the `savePresetWithDialog` and `loadPresetFromFile` `FileChooser::launchAsync` completions captured raw `this` + the WebView-owned `complete` callback. If the plugin window closed while a dialog was open, the completion dereferenced the freed editor and invoked a dead callback. Now capture a `juce::Component::SafePointer` and bail with a bare `return` on teardown (matching the correct pattern already used in `parentHierarchyChanged`).
- **CR-02 — Factory presets ignored the ATTACK_TIME/SUSTAIN_TIME skew:** preset values were authored as linear fractions that ignored the 0.3 skew, so every preset recalled attack/sustain times ~10–30× too short and the "Default" preset did not match the plugin's power-on state. Factory preset values are now authored in engineering units (ms/dB/fractions) and converted through each parameter's own `NormalisableRange` via `convertTo0to1()` at init, so the skew is applied correctly. "Default" now reproduces the APVTS defaults (ATTACK_TIME 10 ms, SUSTAIN_TIME 100 ms).
- **WR-01 — Attack/Sustain knob readouts showed wrong numbers:** the JS `formatValue` callbacks re-derived the skewed range with an incorrect exponential formula (attack displayed ~0.3 ms at the 10 ms default). They now read the real engineering value from JUCE via `Juce.getSliderState(id).getScaledValue()`.
- **WR-02 — Curve-less presets left stale curves applied:** loading a preset without `customState` (Default, Gentle Shaping, Aggressive Bite, Sustain Lift) left the previously-loaded preset's attack/sustain curves in effect. These four presets now carry an explicit flat (all-0.0) curve `customState`, so loading them resets both curves to neutral.
- **WR-03 — Latency re-reported from the audio thread every block:** `processBlock` called `setLatencySamples()` on every block, continuously signalling a (potentially changed) latency from the audio thread — which many hosts glitch on or ignore. Latency is now cached in `lastReportedLatency` and re-signalled only when it actually changes (Lookahead toggled or its time changed). Only the latency-thrash bug is fixed here; the Lookahead control's underlying no-op behaviour is a known limitation deferred to a dedicated DSP pass.

### Known Limitations
- **Lookahead is currently inert:** enabling Lookahead delays both the transient detection and the shaped signal by the same amount, so gain and signal stay time-aligned and nothing audible changes (it only adds reported latency). True lookahead requires splitting detection from application in `STFTProcessor` and is deferred to a future release.

## [1.3.1] - 2026-07-01

### Fixed
- Preset-manager module sync (`preset-manager` v1.0.2) — fixes from the O-DigiDelay code review:
  - **WR-04:** preset names are sanitized before use as filenames (`/\\:` → `_`) in save/load/delete/isFactory, so a name containing `/` no longer silently drops the file.
  - **IN-02:** preset JSON records the real plugin version (`JucePlugin_VersionString`) instead of a hard-coded `"1.0.0"`.
  - **IN-03:** prev/next resume from the last in-list position instead of snapping to index 0 after loading an out-of-list preset from file.
  - **IN-01:** corrected the preset-path docstring.

## [1.3.0] - 2026-03-08

### Added

- **Real-time spectrum overlay on curve editors:** Toggle "Spectrum" button on each curve editor to display the live input FFT magnitude as a semi-transparent filled shape behind the editable curve. Maps 257 FFT bins to the logarithmic X-axis with 60dB dynamic range. Uses accent-colored fill (blue for attack, orange for sustain) at ~15% opacity so the curve remains clearly visible. Spectrum state persists across freehand/node mode switches.
- **Spectrum toggle buttons:** Each curve editor (attack, sustain) has a "Spectrum" button in the control bar. Active state shows accent-colored highlight matching the curve type.

## [1.2.0] - 2026-03-08

### Added

- **Undo/redo for curve editors:** Both freehand and node curve modes now support undo (Ctrl/Cmd+Z) and redo (Ctrl/Cmd+Shift+Z) with a 30-step snapshot stack per editor. Snapshots capture at action boundaries (stroke start, node add/move/delete, reset) so undo reverts entire gestures, not intermediate frames. NodeCurve snapshots include node positions for full state restoration.
- **Undo/redo UI buttons:** Small arrow buttons added to each curve editor's control bar with automatic enable/disable state tracking.
- **Focus-aware keyboard routing:** Keyboard shortcuts route to whichever curve editor (attack or sustain) was last clicked, with proper cleanup on editor destroy/mode switch.

## [1.1.5] - 2026-03-08

### Fixed

- **Add synthesis window to STFT overlap-add reconstruction:** Previously only the analysis Hann window was applied before the forward FFT. Now a Hann synthesis window with per-sample WOLA normalization is applied after the inverse FFT, smoothing frame-boundary discontinuities caused by spectral modification. Reduces metallic ringing and "musical noise" artifacts during aggressive shaping. Root cause: Hann² is not COLA at 50% overlap (sum varies 0.5–1.0), so a precomputed normalized synthesis window `w[i] / (w²[i] + w²[i+H])` is used for correct reconstruction. Zero additional runtime cost (single multiply, precomputed table).

## [1.1.4] - 2026-03-08

### Changed

- **Remove dead code:** `hopTime` member in STFTProcessor (set but never read), empty `loadCurvesFromProcessor()` in app.js, redundant `#include <juce_dsp/juce_dsp.h>` in PluginProcessor.h (already included via STFTProcessor.h).
- **Encapsulation:** `handleAttackCurveUpdate`, `handleSustainCurveUpdate`, `sendAttackCurveToJS`, `sendSustainCurveToJS` moved from public to private in PluginEditor.h.
- **Deduplicate curve handlers:** Identical `handleAttackCurveUpdate`/`handleSustainCurveUpdate` merged into single `handleCurveUpdate(args, setter)` using member function pointer.
- **Deduplicate dialog result creation:** Extracted `makeDialogResult` helper lambda for preset save/load file dialog callbacks.
- **Remove FreehandCurve.drawCurve() indirection:** Eliminated wrapper that just called `drawDataCurve()`; renamed `drawDataCurve` to `drawCurve` directly.
- **Fix JS event listener leaks:** Store bound references for CurveEditor resize handler, Spectrogram WebGL context lost/restored handlers, and NodeCurve keydown listener; add `destroy()` methods for proper cleanup.

## [1.1.3] - 2026-03-08

### Changed

- **Cache APVTS parameter pointers:** `getRawParameterValue()` pointers now cached as class members in constructor instead of 7 string-hash lookups per `processBlock` call.
- **Replace magic number 512:** Dry delay buffer size and modulo operations now use `STFTProcessor::FFT_SIZE` constant.
- **Reuse lastMagnitudes[] in detectTransients():** Band magnitude loop reuses pre-computed magnitudes instead of recomputing `sqrt(r²+i²)` for ~257 bins per frame.
- **Pre-allocate visualization JSON:** `emitVisualizationFrame()` uses `preallocateBytes(4096)` and `<<` operator instead of repeated `+=` at 60fps.
- **Cache getBandFrequencies() and log constants:** Band frequencies computed once in constructor. `logMinFreq`/`logMaxFreq`/`logFreqRange` cached for `freqToX()`/`xToFreq()` (called hundreds of times per render frame).

## [1.1.2] - 2026-03-08

### Fixed

- **Dangling pointer crash in parentHierarchyChanged:** `callAfterDelay(100, [this]{...})` could fire on a destroyed editor if the DAW closed/recreated the UI within 100ms. Now guarded with `juce::Component::SafePointer`.
- **Unclamped/NaN curve values from WebView:** `handleAttackCurveUpdate` and `handleSustainCurveUpdate` now clamp values to [-1.0, 1.0] and replace NaN with 0.0 before passing to DSP.
- **Division by zero in NodeCurve.interpolateAndSample():** When two nodes share the same frequency, the denominator `(rightNode.freq - leftNode.freq)` was zero, producing NaN that propagated through curve data. Now guards the denominator and falls back to `t = 0.0` (left node's gain).

## [1.1.1] - 2026-03-08

### Fixed

- Attack/sustain time knobs not responding
- Curve data race condition
- Lookahead latency reporting

## [1.1.0] - 2026-02-07

### Added

- Initial release with WebView UI

## [1.0.0] - 2026-02-03

### Added

- Per-frequency transient shaping with 32 logarithmic bands
- Freehand and node-based curve editing
- Real-time spectrogram with transient heat overlay
