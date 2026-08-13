# O-SpectralShaper Changelog

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
