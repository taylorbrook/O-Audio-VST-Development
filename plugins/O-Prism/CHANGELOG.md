# O-Prism Changelog

## v1.20.0 (2026-08-20)

### Added
- **Double-click any control to type a value.** Double-clicking a knob, a mod-matrix amount slider, or the A4 Ref knob now opens an inline entry field on the value readout, pre-filled with the current reading and text-selected. Enter (or clicking away) commits, Esc cancels. Covers all 84 controls: 67 `bindKnob()` knobs, 16 mod-matrix amount sliders, and the bespoke A4 Ref knob.
  - Values are typed in the units shown. Explicit units are honoured — `1.2k`/`1.2kHz` on a frequency knob, `500ms` or `2s` on a time knob, `30L`/`C`/`30R` on a pan knob.
  - A **bare** number on a *time* knob inherits the unit currently displayed (`2` reads as 2 ms on a knob showing `30ms`, as 2 s on one showing `1.4s`), because those readouts flip between ms and s mid-range and bare seconds would make every sub-second value unenterable. A bare number on a *frequency* knob is always Hz — the full range is expressible in Hz, and on a knob reading `1.2kHz` someone typing `440` means 440 Hz.
  - Unparseable input is discarded without writing the parameter. Out-of-range input clamps to the parameter's limits.
  - Commits are wrapped in `sliderDragStarted()`/`sliderDragEnded()`, so a typed value records as one clean automation gesture in the host.

### Changed
- **Reset-to-default moved from double-click to Alt/Option-click** on all the same controls, freeing double-click for value entry. Matches the Serum/Vital/Ableton convention.

### Fixed
- **Reset-to-default went to the wrong value on 11 skewed knobs.** Long-standing, present since these knobs were introduced. `bindKnob()`'s third argument is a **normalised** [0,1] reset target, but 11 call sites passed the parameter's **scaled** default instead — which is only the same number when the range is 0–1 and unskewed. That is exactly why it survived ~20 versions: every linear parameter was correct, and only skewed ones were off. Now corrected and verified against `createParameterLayout()`:

| Parameter | Used to reset to | Now resets to |
|---|---|---|
| Amp / Filter Attack | 1 ms | 10 ms |
| Amp Decay | 1.4 ms | 300 ms |
| Amp / Filter Release | 1.1 ms | 500 ms |
| Filter Decay | 2.9 ms | 500 ms |
| Glide Time | 1.1 ms | 100 ms |
| Delay Time | 17.6 ms | 375 ms |
| Reverb Predelay | 2 ms | 20 ms |
| Chorus Rate | 0.125 Hz | 1 Hz |
| EQ Mid Freq | 211 Hz | 1 kHz |

- **A4 Ref knob displayed the wrong frequency.** Its readout used an inline `415 + norm * 50` mapping (415–465 Hz) while the `masterTune` parameter is 420–460 Hz — correct only at centre and 5 Hz out at both ends of the range. It now renders through `masterTuneFmt`, which uses the parameter's real range. Found while making the readout editable; the audible tuning was always correct, only the number shown was wrong.

### Technical Notes
- Value entry converts typed text to the parameter's **scaled** value, then normalises through `state.properties` — the live `start`/`end`/`skew`/`interval` the `WebSliderRelay` pushes up from C++ — rather than the range constants the JS format helpers mirror by hand. The entry path therefore cannot drift from C++ even if one of those mirrors does. (The A4 Ref bug above was exactly such a drift.)
- Each of the 21 format helpers gained a matching `.parse`, so all 70 `bindKnob()` call sites are unchanged. Display units are not always scaled units: `pct` knobs are 0–1 in C++ but read 0–100%, `delayFeedback` tops out at 0.95 while reading 95%, pan is −1…1 shown as `30L`/`C`/`30R`, and `reverbPredelay` is scaled in **milliseconds** (0–200) so it is a direct readout rather than a seconds param.
- `scaledToNorm()` clamps before its `Math.pow()`. This is load-bearing, not cosmetic: a scaled value below `start` gives a negative base, and a negative base raised to a fractional skew is `NaN` — which would pass straight through `setNormalisedValue()` into the parameter and stick.
- Text that has not been edited is never written back. Readouts are rounded (LFO Rate shows whole Hz above 10, so 19.43 reads `19 Hz`), so without that guard merely opening the field and pressing Enter would quantise the parameter to the rounded reading.
- Readout updates are suppressed while an entry field is open — the `valueChanged` listeners assign `textContent`, which would wipe out the `<input>` living inside that span.
- The Alt-click reset handlers are ordered deliberately. On the knobs and mod-matrix rows the reset binds to an **ancestor** in the capture phase so it lands ahead of the drag handler; on the A4 Ref knob both handlers sit on the *same* element, where at-target listeners fire in registration order regardless of the capture flag, so that one is registered first and uses `stopImmediatePropagation()`. Without this the control would reset and then immediately begin dragging from the same click.

### Testing
- Every one of the 68 `bindKnob()` reset targets is now checked against the C++ `NormalisableRange` by script (`defaultNorm` vs `((default−start)/(end−start))^skew`), which is how the 11 wrong ones surfaced. 58 match a parsed C++ declaration directly; the remaining 10 use helper-supplied defaults (`levelDefault`, the filter-prefix helper) and were confirmed by hand — both cutoffs default to 20 kHz → norm 1.0, the rest are linear 0–1.
- 354-check format→parse→normalise round-trip harness across all 21 formatters at 12 points each, run against code extracted from the live `index.html` rather than a retyped copy. All pass. Four negative controls (dropping the `/100` in `parsePercent`, removing the `scaledToNorm` clamp, testing `/s/` before `/ms/`, routing predelay through `parseSeconds`) each fail the harness, confirming it discriminates.
- 27 browser-driven DOM interaction checks against a faithful `SliderState` mock: entry opens pre-filled and selected, zero layout shift, input survives a `valueChanged` mid-edit, Esc cancels without writing, Enter commits wrapped in drag start/end, unedited Enter and garbage both write nothing, out-of-range clamps without `NaN`, Alt-click resets without starting a drag, plain mousedown still drags, and a second double-click while open is a no-op.

## v1.19.3 (2026-08-02)

### Fixed
- **Windows CI build failure (v1.19.2 tag never released).** Same latent MSVC break as O-Lyrica v2.3.3 / O-IntonationPad v2.8.3: MSVC rejects `SafePointer(this)` init-captures in nested lambdas (C2440/C2119 cascade). Hoisted the SafePointer to a local and captured by value in all six FileChooser `launchAsync` callbacks in `PluginEditor.cpp`. No behavior change.

### Technical Notes
- v1.19.2 binaries were never published — the Windows job failed at compile, so `create-release` never ran. This release ships the v1.19.2 content (license headers) plus the MSVC fix.

## v1.19.2 (2026-08-02)

### Changed
- Added AGPL-3.0 license notice headers to all Ouaricon-authored source files (repo relicensed to AGPL-3.0). No functional change.

### Technical Notes
- First tagged release since v1.18.1 — these binaries are the first to ship the full v1.18.2–v1.19.1 code-review remediation (47 findings, see entries below) and the dead licensing-integration removal.
- Version bump rationale: PATCH — license headers only; no parameter, preset, or state-format changes.

## v1.19.1 (2026-07-02)

Code-review batch 3 (final) — the 17 Info findings from `.planning/CODE-REVIEW.md` (2026-07-02 deep review). Re-verified against the post-v1.19.0 tree first: IN-08 (Zarlino/JI "byte-identical") turned out to be a reviewer error — the arrays already differ at degree 10 (9/5 vs 16/9 minor seventh), no change made.

### Fixed — DSP / RT
- **IN-01 (completion): last uncached per-block APVTS lookups.** `resolveActiveTable` string-looked-up `oscATable`/`oscBTable` every block via `updateWavetableAssignments`; both pointers now cached in the constructor with the rest of the block.
- **IN-03: delay time changes zippered.** `delaySamples` is now a `SmoothedValue` (50 ms ramp) read per-sample — automation glides the read position (tape-style pitch bend through the Lagrange interpolator) instead of stepping.
- **IN-04: latency was reported even with distortion (the only latency source) bypassed** — hosts over-compensated the default configuration. Reported latency now follows `distBypass` (0 when bypassed), updated from the message-thread timer.
- **IN-06: sub-osc/noise reset unconditionally in startNote** — clicked on legato/glide retrigger. Now gated by the same `(!wasActive || glideMode == 0)` condition as the main oscillators.

### Fixed — tuning correctness
- **IN-07: built-in Bohlen-Pierce preset omitted the 13th degree** (1755.6¢) — the top step double-jumped to the tritave and diverged from the embedded `nonoctave/bohlenpierceET`. All 13 degrees present now.
- **IN-10: `generateRank2` clamped the generator against the unclamped period.** Period is clamped first.
- **IN-11: KBM formal octave degree now used in frequency math** (Scala spec compliance — the per-pattern-repetition pitch jump honors a non-default formal octave instead of always using the scale period); the exporter pitch circle no longer double-draws the unison when the interval list includes the closing period (ET spokes now match the scale size too).
- **IN-09: `isNoteMapped` read KBM state without `intervalMutex`** (latent — no callers yet). Locks now.

### Fixed — UI / bridge
- **IN-14: `applyGeneratedScale` ignored the scale-name argument** — generator names reverted to "Generated" on reopen. `args[1]` is used when present.
- **IN-16 (completion): `toJsonFloatArray` clamps non-finite floats to 0** (a NaN froze the waveform display via a JS `JSON.parse` throw); the `getActiveOscInfo` factory branch routes the table name through `juce::JSON::toString` like the user branch.
- **IN-17: tonic arrows updated UI state even when the bridge call failed** (silent UI/DSP desync). Failures now log to console and revert `currentTonic`; the `getTonicNote` fetch failure logs too.
- **IN-12: `smoothFrames` strength semantics un-inverted** (1.0 = max smoothing). Latent — the only caller passes the symmetric 0.5, so no behavior change today.

### Removed — dead code (IN-02, IN-05, IN-13, IN-15)
- **IN-02:** `WavetableOscillator::getNextSample()` (mono path, zero callers) deleted — it duplicated the sync/warp logic and had to be kept in sync with every CR-01-class fix.
- **IN-05:** the duplicated `kDivBeats` tables (processor + voice) consolidated into `Source/NoteDivisions.h` — divergence would have silently desynced global vs per-voice LFO rates.
- **IN-13:** 11 registered-but-never-called native functions removed (`setTuningIntervals`, `getMasterTune`, `setMasterTune`, `setOctaveStretch`, `setTemperamentPreset`, `getTemperamentPreset`, `getEmbeddedTuningCategories`, `getWavetableFrame`, `getWavetableInfo`, `getWavetableFrameForPosition`, `deletePreset`). Relays and the remaining fns are the live paths; grep-verified zero JS references.
- **IN-15:** unused `webview-relay-manager` module link removed from CMakeLists (no source ever included it; the editor uses its own relay helpers). Registry never tracked the dependency, so no registry change.

### Technical Notes
- **Version bump rationale:** PATCH (1.19.0 → 1.19.1) — bug fixes and dead-code removal only; no parameter, preset, or state-format changes.
- **Root cause source:** `.planning/CODE-REVIEW.md` (2026-07-02 deep review), Info batch. All 17 findings now dispositioned; review complete.
- **Behavior note:** hosts now see latency change when toggling distortion bypass; delay-time automation sounds like tape pitch-glide instead of zipper.
- **Files changed:** `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `Source/PrismVoice.cpp`, `Source/TuningEngine.cpp`, `Source/ScaleGenerator.cpp`, `Source/TuningExporter.cpp`, `Source/NoteDivisions.h` (new), `Source/dsp/{WavetableOscillator.{h,cpp},DelayProcessor.{h,cpp},WavetableEditor.cpp}`, `Source/ui/public/index.html`, `CMakeLists.txt`.

## v1.19.0 (2026-07-02)

Code-review batch 2 — all remaining Critical and Warning findings from `.planning/CODE-REVIEW.md` (2026-07-02 deep review). 25 findings: CR-04..CR-07, CR-10, WR-01..WR-20.

### Fixed — RT safety (CR-04, CR-05, CR-06, WR-01)
- **CR-04: EQProcessor heap-allocated IIR coefficients on the audio thread every block.** `Coefficients::makeLowShelf/makePeakFilter/makeHighShelf` each `new` a ref-counted object, four setters ran unconditionally per callback. Ported the O-Formant v1.25.1 pattern verbatim: `ArrayCoefficients<float>::makeXXX` assigned in place into pre-grown `*state` storage, atomics for target values, change-detection to skip the no-change case.
- **CR-05: TuningEngine was mutated from processBlock — mutex (×128 per rebuild) + heap allocation on the audio thread; message-thread getters raced shared Strings.** processBlock now only change-detects the five tuning params (cached pointers) and defers all engine mutation to the message thread via `AsyncUpdater`. The preset is applied only when the `tuningPreset` param actually changed (a scalar-only update must not clobber a user-loaded .scl). `pitchBendRange` is now atomic (the one engine scalar read per-note on the audio thread); `rebuildFrequencyTable` locks `intervalMutex` once instead of per MIDI note. With mutation now message-thread-only, `getActiveTuningName`/`getMasterTune`/`getBuiltInPreset` no longer race.
- **CR-06: `advanceGlobalLfoPhases` built ~28 `juce::String`s + 12 APVTS map lookups per block.** All lfoN Sync/Rate/Division/Shape pointers cached in the constructor alongside the FX block. Also cached: masterTune, octaveStretch, pitchBendRange, tuningPreset, tonic, stereoWidth, masterVol (IN-01 partial).
- **WR-01: SVF cutoff not clamped below Nyquist — unstable filter and sticky NaN at fs < 40 kHz.** Cutoff now clamps to `min(20 kHz, 0.49·fs)`; added a non-finite output guard that flushes integrator state to silence instead of recirculating NaN into the delay/reverb tanks forever.

### Fixed — tuning & file-format correctness (CR-07, WR-11..WR-15, WR-17)
- **CR-07: `.kbm` reference frequency was clamped through the 400–480 Hz A4 master-tune clamp** — a standard middle-C-referenced KBM (261.63 Hz) mistuned the whole instrument ~7 semitones sharp, silently. The KBM ref freq now lives in its own member (validated 8–20000 Hz) and drives the KBM branch of `calculateCustomFrequency` directly.
- **WR-11: `.scl` parser broke on blank description lines and silently dropped negative-cents pitches.** The description is now consumed positionally (spec allows it to be blank); `parseScalaPitch` returns NaN on error instead of −1 so legal negative cents parse; loads now fail loudly on malformed pitch lines, invalid degree counts (≤ 0), and count mismatches instead of "succeeding" truncated.
- **WR-12: `loadKBMFile` trusted the header-claimed map size** (a hostile file claiming 2×10⁹ attempted an ~8 GB vector). Sizes outside 0–128 are rejected.
- **WR-13: `WavetableImporter` trusted audio headers** — int overflow in the 30 s cap and unclamped channel counts allowed multi-GB allocations. Cap computed in int64 with an absolute 8M-sample ceiling, channels clamped to 2, non-finite/≤0 sample rates rejected. Applied to both file and memory import paths.
- **WR-14: `setSingleInterval` wiped any legitimate 11-degree scale (12 interval entries) to 12-TET.** Removed the `size() == 12` clause — only genuinely empty interval sets initialize to 12-TET.
- **WR-15: `TuningExporter::toHTML` ignored periods ≤ 1200¢** — Carlos Gamma (737.1¢) exported with a 1200¢ period and wrong deviations/pitch circle. The last interval is now the period whenever positive.
- **WR-17: session state omitted KBM mapping, engine mode, and current preset** — `.kbm` silently lost on reload, `getTemperamentPreset` reported 12-TET after every reload. New `TuningEngine::writeStateTo/restoreStateFrom` persist intervals, name, tonic, mode, preset, and the full KBM block (incl. ref freq); legacy session trees still restore.

### Fixed — preset & file safety (CR-10, WR-08, WR-09, WR-10, WR-16)
- **CR-10: Preset Save button was dead on macOS** — `window.prompt()` always returns null under JUCE's WKWebView (no text-input panel delegate). Replaced with an in-DOM save modal (same pattern as the wavetable editor's), with Enter/Escape and click-outside handling.
- **WR-08: `applyPresetJson` didn't reset params to defaults first, and on-disk factory JSON never regenerated.** All non-excluded params now reset to defaults before a preset applies (partial/old presets no longer inherit stale state — known suite bug, O-Polystutter v1.12.3). Factory presets are version-stamped (`.factory-version`) and regenerate whenever the plugin version changes.
- **WR-09: preset names were used verbatim as file paths** — "/" silently broke save/list, "../" escaped the preset dir. Names are sanitized via `File::createLegalFileName` (+ explicit separator strip, leading-dot strip) in save and delete.
- **WR-10: user wavetable names from the WebView were unsanitized file paths** — `saveEditedWavetable("../../Desktop/x")` wrote to the Desktop; delete was a relative-path deletion primitive. Names sanitized at both entry points; deletion additionally verifies `isAChildOf(wavetableDir)`.
- **WR-16: `setMasterTune`/`setOctaveStretch` native fns were silently reverted within one block** by the APVTS→engine sync. They now write the APVTS parameter (`setValueNotifyingHost`), mirroring `setTemperamentPreset`.

### Added — dead features now implemented (WR-02, WR-03, WR-04, WR-06)
- **WR-03: delay tempo-sync is live.** New `delayDivision` parameter (18 divisions, same table as the LFOs) + Division dropdown in the Effects tab; when Sync is on the delay time follows host BPM. **The ~20 factory presets authored with `delaySync`/`delayDivision` now sound as designed** (factory JSON regenerates via the WR-08 version stamp, so the new parameter reaches existing installs). Dead `setSync`/`setPlayHead`/`tempoSync` plumbing removed from DelayProcessor.
- **WR-02: all nine previously-silent mod-matrix destinations now work.** LFO1–4 Rate (±2 octaves at full offset, per-sample in the voice); OscA/B Detune (block-rate; unison/detune/width are now also live under automation instead of note-start-only); Reverb/Delay/Chorus/Dist Mix and Master Vol via a new processor-level matrix evaluated once per block, driven by the global LFOs, mod wheel, and aftertouch (per-voice sources read as 0 for global destinations).
- **WR-04: Phase knobs are live.** Phase = 0 keeps the classic random-phase start; Phase > 0 gives a deterministic oscillator start phase via the previously-dead `resetWithPhase()`.
- **WR-06: glide modes actually differ, and glide is audible polyphonically.** "Legato" glides only while another note is held (or on a stolen voice); "Always" glides on every note, seeding fresh voices from a processor-level last-played-frequency atomic. GlideProcessor's `setTarget` now takes an explicit glide-in flag (the old `wasActive` heuristic snapped on almost every fresh poly voice).

### Fixed — FX & UI (WR-05, WR-07, WR-18, WR-19, WR-20)
- **WR-05: hard-coded delay-line capacities clamped at high sample rates** (2 s delay needs 384k samples at 192 kHz vs 192k capacity; 200 ms predelay needs 38.4k vs 19.2k). Both now size from `spec.sampleRate` in `prepare()`, with input clamps in `setTime`/`setPredelay`.
- **WR-07: FX mix≈0/bypass gate left stale delay/reverb/chorus buffers that replayed as a "ghost echo" when the mix rose.** Each effect now resets once on its active→inactive transition (covers both the mix gate and bypass).
- **WR-18: Redo (Ctrl/Cmd+Shift+Z) could never fire** — `e.key` is `'Z'` when Shift is held. Compared case-insensitively; Ctrl/Cmd+Y also redoes.
- **WR-19: preset loads never refreshed the native-fn-backed tuning UI.** `window.__refreshAllControls` is now defined (calls `refreshTuningState`), and the `tuningPreset`/`tonic` relay listeners trigger a debounced refresh on host automation.
- **WR-20: mod-matrix source/dest names were hard-coded in JS** while the registered `getModSourceNames`/`getModDestNames` native fns went uncalled — any future C++ enum change would silently misroute modulation. The dropdowns now populate from the native fns at startup (hard-coded lists remain only as a fallback).

### Technical Notes
- **Version bump rationale:** MINOR (1.18.2 → 1.19.0) — new `delayDivision` parameter and newly functional features (mod destinations, Phase knobs, glide modes, tempo-synced delay). No parameter IDs changed or removed; v1.18.x sessions and presets load identically (delayDivision defaults to 1/4).
- **Behavior notes:** unison count/detune/width now respond to automation at block rate (previously note-start-only). Factory preset JSON on disk regenerates once on first load of v1.19.0 (user presets untouched).
- **Files changed:** `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `Source/TuningEngine.{h,cpp}`, `Source/TuningExporter.cpp`, `Source/OuariconPresetManager.h`, `Source/PrismVoice.{h,cpp}`, `Source/dsp/{EQProcessor,SVFFilter,DelayProcessor,ReverbProcessor,GlideProcessor,UserWavetableManager,WavetableEditor,WavetableImporter}.{h,cpp}`, `Source/ui/public/index.html`, `Source/ui/public/js/wavetable-editor.js`, `CMakeLists.txt`.
- **Deferred:** the 17 Info findings (IN-01 partially done via pointer caching) remain in `.planning/CODE-REVIEW.md` for a later cleanup pass.

## v1.18.2 (2026-07-02)

### Fixed
- **CR-01: Out-of-bounds wavetable read at high pitch (host crash).** `phaseIncrement` had no upper bound and the render wrap subtracted 1.0 exactly once, so any frequency above the sample rate (e.g. MIDI 127 + `oscACoarse +24` at 44.1 kHz) grew the phase accumulator without bound and `readSample()` walked off the end of the wavetable buffer. Fixed with floor-based wrapping in all four advance paths (sync + non-sync, mono + stereo), a wrap after the hard-sync re-seed (which could land ≥ 1.0 at `syncRatio` up to 4), a defensive wrap + non-finite guard at the top of `readSample()`, and a Nyquist clamp in `setFrequency()`.
- **CR-02: Use-after-free on wavetable editor close / user-table delete.** Voices hold raw `const WavetableData*` refreshed only at the top of the next `processBlock`, but `stopEditing()` freed the working table immediately and `deleteWavetable()` freed the entry in place. Closing the editor or deleting a table while a chord sustained dereferenced freed heap on the audio thread. Fixed with a retired-table reaper: tables are parked with the current block-generation stamp and freed on a 500 ms message-thread timer only after the generation has advanced ≥ 2 (same class of fix as O-MicrotonalSampler v1.23.2).
- **CR-03: Saving an edited wavetable dangled the other oscillator permanently.** `saveAsUserWavetable()` ended with `manager.loadFromDisk()`, which destroyed **every** user `WavetableData` and re-imported fresh objects with nothing re-syncing the osc pointers. Now only the saved entry is inserted/replaced (`replaceOrInsertFromFile`), the replaced table is retired via the reaper, and both osc pointers are re-resolved by name before retirement (the editing osc keeps its working-table preview).
- **CR-09: All 6 `FileChooser::launchAsync` completions captured raw `this` — UAF on editor teardown.** Known suite bug (O-MicrotonalSampler v1.23.5 W12). All six sites (loadScalaFile, loadKBMFile, saveScalaFile, saveKBMFile, exportTuningHTML, importUserWavetable) now capture `juce::Component::SafePointer` and bail with a bare `return` when the editor is gone — never calling `complete()` on the dead path, since that callback is owned by the destroyed WebView.
- **CR-08 (partial, pulled forward): WAV overwrite appended instead of replacing.** `FileOutputStream` positions at end-of-file, so re-saving under an existing name appended a second WAV and the next import read the stale original header. Both save paths now `deleteFile()` first. Pulled into this batch because the CR-03 fix re-imports the just-written file and would otherwise import stale data.

### Technical Notes
- **Version bump rationale:** PATCH (1.18.1 → 1.18.2) — crash/UAF bug fixes only; no parameter, preset, or UI changes.
- **Root cause source:** `.planning/CODE-REVIEW.md` (2026-07-02 deep review), criticals batch 1.
- **Files changed:** `Source/dsp/WavetableOscillator.cpp`, `Source/dsp/WavetableEditor.{h,cpp}`, `Source/dsp/UserWavetableManager.{h,cpp}`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`.
- **New mechanism:** `OPrismAudioProcessor` now inherits `juce::Timer`; `processBlock` publishes a `blockGeneration` counter; retired tables freed only after two generations pass. If the host stops calling `processBlock`, retired tables are held (never freed unsafely).
- **API changes (internal):** `UserWavetableManager::deleteWavetable` → `removeWavetable` (returns the removed table for retirement); `saveAsUserWavetable` gains a `replacedOut` out-param; new processor entry points `deleteUserWavetable()` / `saveEditedWavetable()` used by the WebView native fns.

## v1.18.1 (2026-05-06)

### Added
- **Ouaricon licensing overlay (compile-flag gated, OFF by default in shipped builds).** Wires `OuariconLicense` + `OuariconLicenseOverlay` into the editor; license manager owned by the processor (persists across editor open/close). When licensing is enabled at build time, the overlay hides the WebView until the plugin is activated. Released builds ship with `OUARICON_LICENSING=OFF` — **no user-facing change vs v1.18.0**.

### Technical Notes
- **Version bump rationale:** PATCH (1.18.0 → 1.18.1) — integration plumbing only, no behavior change in shipped artifacts.
- **Files changed:** `CMakeLists.txt` (licensing block + cryptography link), `Source/PluginEditor.{cpp,h}`, `Source/PluginProcessor.{cpp,h}`.
- **No DSP, UI, parameter, or preset changes.**

## v1.18.0 (2026-05-06)

### Added
- **Factory preset library doubled — 96 → 192 presets.** All 96 new presets are hand-authored, each a deliberate parameter snapshot exploring an idea distinct from the existing library. New per-category counts:
  - Pads 18 → 26 (+8): Glass Curtain, Stratosphere, Bone Cathedral, Lichen Drift, Hollow Star, Salt Air, Lithium Glow, Snowmelt
  - Drone 12 → 22 (+10): Continental Plate, Glacier Breath, Stone Choir, Salt Ocean, Bell Forest, Iron Hum, Cathedral Furnace, Coral Tide, Volcanic Throat, Fossil Bell
  - Lead 15 → 24 (+9): Glass Throat, Phantom Choir, Ember Tongue, Static Halo, Iron Howl, Mantis Cry, Velvet Razor, Solder Burn, Magma Wire
  - Bass 10 → 20 (+10): Iron Wolf, Tar Pit, Magnet Bass, Bone Saw, Black Tide, Stutter Gut, Velvet Floor, Plasma Trench, Coral Reese, Glass Hammer
  - Pluck 10 → 20 (+10): Brass Bell, Wire Fence, Gut String, Comb Drop, Solder Drop, Vellum Harp, Shell Click, Frost Bell, Wax Pluck, Spectral Spark
  - Keys 8 → 20 (+12): Cathedral Pipe, Reed Organ, Vibraphone Cold, Mbira, Music Box, Harmonium, Wood Rhodes, Vox Continental, Toy Piano, Gamelan, Glass Marimba, Choir Organ
  - Sequence 12 → 22 (+10): Crystal Gate, Mantra Arp, Beam Walk, Marble Step, Solder Loop, Vowel Drift, Strobe Forest, Liquid Counter, Glass Telegraph, Steam Cycle
  - FX 8 → 20 (+12): Reverse Surge, Solar Flare, Atom Split, Comet Tail, Storm Approach, Bell Rain Backwards, Vapor Trail, Mech Bloom, Pressure Vent, Spectral Fold, Subspace Echo, Static Bloom
  - Percussion 3 → 18 (+15): Sub Boom, Glass Hat, Wood Knock, FM Tom, Click Snap, Ride Cymbal, Bell Tap, Snare Shimmer, Bongo, Tribal Drum, Synthetic Conga, Glitch Hit, Wind Burst, Tom Floor, Synth Clave

### Wavetable Coverage Improved
- Filled CRITICAL gaps in the previous library:
  - `WT_VocalLead`: 1 → 3 osc-A primary uses (Glass Throat, Phantom Choir + existing Phantom Vocal)
  - `WT_SpectralTilt`: 1 → 3 osc-A primary uses (Stratosphere, Glacier Breath + existing Frozen Drift)
  - `WT_FormantFilter`: 0 → 8 osc-A primary uses (was osc-B-only)
  - `WT_OddHarmonics`: 0 → 5 osc-A primary uses (was osc-B-only)
  - `WT_HarmonicStretch`: 0 → 6 osc-A primary uses (was osc-B-only)
- All 27 wavetables now have ≥2 osc-A uses across the full 192-preset library; 26/27 have ≥3.

### Tuning Persistence (Hard Constraint)
- Stepping through any of the 192 presets does **not** modify the user's active tuning. The 7 tuning-related parameters (`tuningPreset`, `tonic`, `masterTune`, `octaveStretch`, `pitchBendRange`, `glideMode`, `glideTime`) remain in `excludedParameterIds` (`PluginProcessor.cpp:474-477`) and are skipped on save / load / applyFactoryDefinition. **No new code touches tuning state** — verified by `grep -cE` against the 7 IDs in `FactoryPresets.cpp` returning 0.

### Technical Notes
- **Version bump rationale:** MINOR (1.17.4 → 1.18.0) — new feature content, no parameter schema changes, no breaking changes. User-saved presets from v1.17.x load identically.
- **Files changed:** `Source/FactoryPresets.cpp` (1225 → 2335 lines, +1110), `CMakeLists.txt` (version), `CHANGELOG.md`, `PLUGINS.md`.
- **No DSP, UI, or parameter changes.** Only preset content was added.
- **Build:** Clean Release build (macOS VST3 + AU). No new warnings.

## v1.17.4 (2026-05-06)

### Fixed
- **AU registry stuck on v1.17.0 in Logic Pro:** A stale non-suffixed `O-Prism.component` (dev build, v1.17.0, installed Apr 26) was shadowing the current `O-Prism-dev.component` (v1.17.3) in `~/Library/Audio/Plug-Ins/Components/`. Both bundles registered with the same AU triple `aumu OuPr OuDv`, so Logic's plugin scanner pinned the slot to whichever was installed first — the older v1.17.0. Symptom: Logic showed v1.17.0 in the plugin registry; "O-Prism-dev" never appeared as a separate entry. Same shadowing on VST3 (`O-Prism.vst3` v1.17.0 vs `O-Prism-dev.vst3` v1.17.3).

### Changed (project-wide)
- **`scripts/build-and-install.sh` — Phase 4 hardening (variant-suffix sweep):** The install pipeline now sweeps both the current `PRODUCT_NAME` *and* its dev/release counterpart bundle (`-dev` ↔ unsuffixed). Prevents legacy bundles from a prior branding configuration from pinning the AU registry slot. Emits a `⚠ Sweeping ALTERNATE-variant` warning when an alternate-variant orphan is found, so the cleanup is visible in build logs. Applies to all 35 plugins, not just O-Prism.
- **`scripts/build-and-install.sh` — Phase 3 fix (un-expanded CMake variables in PRODUCT_NAME):** The previous Phase 3 grepped `PRODUCT_NAME` from CMakeLists.txt as a literal string, which returned `O-Prism${OUARICON_DEV_SUFFIX}` un-expanded for any plugin using the suffix system. Phases 4–7 then operated on a non-existent path and silently no-op'd (Phase 4 found no old AU/VST3 to remove → orphan accumulation; Phase 5 fell through to a hard error on a literal-`${...}` artefact path). Phase 3 now reads the bare `PRODUCT_NAME` from the actual `*.component` / `*.vst3` filename in `build/plugins/<P>/<P>_artefacts/Release/{AU,VST3}/`, with a CMakeLists.txt fallback that errors out cleanly if it sees an un-expanded variable. The build artefact is the authoritative source of truth. This is the *primary* mechanism that allowed the legacy `O-Prism.component` orphan to survive — Phase 4 was looking for the wrong filename for months.

### Root Cause
- The `OUARICON_DEV_SUFFIX` system (root `CMakeLists.txt:30-40`) appends `-dev` to `PRODUCT_NAME` only when `OUARICON_RELEASE=OFF` (the local default). Bundles built **before** this suffix existed were installed without the suffix and persisted across the dev-branding transition. The previous Phase 4 (`build-and-install.sh:253-292`) only removed the bundle matching the *current* `PRODUCT_NAME`, never the alternate-variant counterpart. Once the suffix activated, every dev rebuild went to `<Name>-dev.component` while the legacy `<Name>.component` sat undisturbed and continued to claim the `aumu/OuPr/OuDv` registry slot.

### Verification
- Pre-fix state: `auval -a | grep -i prism` showed two entries with the same `aumu OuPr OuDv` triple, both labeled "O-Prism-dev" — confirming registry collision.
- Pre-fix `Info.plist` versions: `O-Prism.component` = 1.17.0 (Apr 26), `O-Prism-dev.component` = 1.17.3 (May 6). Identical CFBundleIdentifier `com.Ouaricon Audio Development.O-Prism`, identical factory function `O_Prism_devAUFactory` — i.e. both were dev builds, just from different points in time.
- Post-fix: `./scripts/build-and-install.sh O-Prism` rebuilds at v1.17.4, sweeps the legacy `O-Prism.{vst3,component}` orphan via the new alternate-variant pass, installs `O-Prism-dev.{vst3,component}` v1.17.4, clears AU cache + kills `AudioComponentRegistrar`. Single clean registry entry remains.
- No plugin source changes — DSP, parameter schema, preset format, and UI are byte-identical to v1.17.3.

### Technical Notes
- **Version bump rationale:** PATCH (1.17.3 → 1.17.4) — distribution/install hardening only. Plugin code unchanged. The bump exists to give the install pipeline change a verifiable through-line and a fresh CHANGELOG anchor; existing v1.17.3 sessions and presets load identically.
- **Manual cleanup template** in root `CLAUDE.md` updated to mirror the dual-removal pattern (sweeps both `<Name>.component` and `<Name>-dev.component` before install), so manual installs follow the same hardened path as the script.
- **Generalizable to all plugins:** any plugin built locally before the `OUARICON_DEV_SUFFIX` system landed may have an orphan non-suffixed bundle in `~/Library/Audio/Plug-Ins/{VST3,Components}/`. Running `./scripts/build-and-install.sh <PluginName>` once will sweep them as a side effect.

## v1.17.3 (2026-05-06)

### Changed
- Phase 3 sweep — 5 MEDIUM/LOW simplification candidates from `plugins/O-Prism/.planning/SIMPLIFICATION-AUDIT.md` applied:
  - **MEDIUM-01:** Replaced 4 ad-hoc JSON-array build loops in `Source/PluginEditor.cpp` with the existing `toJsonArray` helper (covers MEDIUM-04 nested 2D case). Sites: `startWavetableEditor` harmonics, `getFrameHarmonics`, `getAllEditorFrameWaveforms` (composed nested call), and the inner names array of `getPresetListWithCategories`. The outer object structure of `getPresetListWithCategories` (per-category `firstCat` flag) is preserved since it emits a JSON object, not array. ~25 LOC saved.
  - **MEDIUM-02:** `getEmbeddedTuningList` now emits the `period` field; non-octave tunings (Bohlen-Pierce, Carlos α/β/γ) now correctly display "(NNNN¢ period)" in the library list. The dead JS branch at `Source/ui/public/index.html:3304` (`tuning.period && tuning.period !== 1200 ? ...`) is now reachable.
  - **MEDIUM-07:** Extracted `OPrismAudioProcessor::resolveActiveTable (int oscIndex) const` private helper. `updateWavetableAssignments` (audio-thread voice-assignment path) and `getActiveOscTable` (public accessor) now share a single source of truth for the "user pointer takes priority over factory index" lookup. `std::memory_order_relaxed` and `juce::jlimit (0, factoryTables.size() - 1, ...)` clamping preserved verbatim per audit's "Skipped — factoryTables atomic ownership is intentional" caveat.
  - **LOW-01:** Corrected stale "60 Hz is plenty" comment to match the actual `startTimerHz (30)` call in `PluginEditor.cpp`.
  - **LOW-02:** Removed dead `currentPitchWheel` member from `PrismVoice` (covers LOW-05). Member was set in `startNote` and `pitchWheelMoved` but never read. The JUCE-mandated `currentPitchWheelPosition` parameter on the `startNote` override stays in the signature (now `/*currentPitchWheelPosition*/` to silence unused-param warning).

### Skipped (audit candidate not applied)
- **LOW-04 — inline `canPlaySound` into header:** Audit underestimated this one. `PrismSound` is forward-declared in `PrismVoice.h` (line 23); inlining `canPlaySound` requires the full type for `dynamic_cast`. Adding `#include "PrismSound.h"` to `PrismVoice.h` would tighten compile coupling without a clear win. Left as-is in `PrismVoice.cpp:142-145`.

### Verification
- Clean Release build (macOS VST3 + AU) — no new warnings introduced (pre-existing `[this]`-capture and unused-include hints unchanged).
- AU validation passed (`auval -v aumu OuPr OuDv`).
- AU cache cleared and fresh binaries installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` per project CLAUDE.md.
- Spot-check greps:
  - `grep -rn "currentPitchWheel\b" plugins/O-Prism/Source/` — only the JUCE override-signature occurrences remain (zero bare-member references).
  - `grep -n "60 Hz is plenty" plugins/O-Prism/Source/` — zero matches.
  - 4 ad-hoc `if (i > 0) json` / `if (f > 0) json` / `if (s > 0) json` / `if (! firstCat) ... [...]` patterns reduced to just the helper-internal occurrences in `toJsonArray` / `toJsonFloatArray` plus the outer object iteration in `getPresetListWithCategories`.
- Visual smoke: tuning library now shows period for Bohlen-Pierce; wavetable editor harmonic bars + frame waveforms render correctly; A/B oscillator user-vs-factory swap works in both directions; LFO sync/free-run toggles unchanged.
- No APVTS schema, preset, or persistence-format changes.

### Technical Notes
- Phase 3 of the `/simplify` workflow audit (see `plugins/O-Prism/.planning/SIMPLIFICATION-AUDIT.md`). Phase 1 (HIGH-01..03) shipped in v1.17.1; Phase 2 (HIGH-04..07) shipped in v1.17.2. The audit's MEDIUM-03 (per-FX param atomic caching) was already resolved as a side effect of HIGH-05 in Phase 2. No-op MEDIUM-05 / MEDIUM-06 / LOW-03 entries are explicit "keep" notes from the audit.
- Version bump rationale: PATCH (1.17.2 → 1.17.3) — internal refactor + 1 visible fix (period emission for non-octave tunings); no parameter, preset, or feature changes.

## v1.17.2 (2026-05-06)

### Changed
- **HIGH-04:** Replaced 64 inline SVG knob HTML scaffolds in `Source/ui/public/index.html` with `data-knob` placeholders + a single `expandKnobMarkup()` JS pass at script start. ~30 KB reduction in the index.html binary blob and ~15× DOM verbosity reduction. The large refPitch knob (l1351) and the 2 small footer knobs use bespoke markup and remain untouched. The `vine-<paramId>` and `val-<paramId>` IDs emitted by the expander match the originals exactly so existing `bindKnob` and `valueChangedEvent` handlers continue to work unchanged.
- **HIGH-05:** Cached the 5 FX bypass + per-FX config param atomics (25 `std::atomic<float>*` total) as members in `OPrismAudioProcessor`. Extracted a `runEffect()` template helper for the bypass-and-process pattern across Distortion, Chorus, Delay, Reverb, EQ. Eliminates ~30 hash-map lookups per audio block. The per-FX `mix > 0.001f` short-circuit is preserved inside each configure callback (load-bearing — not every FX `process()` is RT-safe at mix=0).
- **HIGH-06:** Extracted a `buildTable()` template helper across 17 of 20 `WavetableFactory::generate*` functions (the static `generateFormantTable` helper plus 16 public generators). Each generator collapses to its per-frame body. The 3 generators with non-standard skeletons keep their current shape: `generateBitcrush` (pre-loop sawBuf setup), `generateFM` (writes every sample with `=` and takes `cmRatio/minIndex/maxIndex` params), `generateChurchBell` (audit caveat — keep as-is for safety). For the `generateBreath`, `generateWind`, `generateFilteredNoise` generators, `std::mt19937 rng` and `phaseDist` are captured by reference into the per-frame lambda so the cross-frame draw sequence stays deterministic (seeds 42, 99, 77).
- **HIGH-07:** Consolidated 4 LFO sync + free-run toggle-relay/attachment loops into 3 file-static helpers: `createToggleRelays`, `addRelayOptions`, `attachToggleRelays`. The `lfoSyncRelays` and `lfoFreeRunRelays` vectors stay separate (preserves member-declaration destruction order from `PluginEditor.h:30-35`: relays last, WebView middle, attachments first). The `bypassRelays` / `modSlotToggleRelays` / `delaySyncRelay` groups follow the same shape and could fold in here in a future pass — out of scope for this commit.

### Verification
- Release build: `ninja O-Prism_VST3 O-Prism_AU` clean.
- AU validation: `auval -v aumu OuPr OuDv` — PASSED.
- AU cache cleared and fresh binaries installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` per project CLAUDE.md.
- No APVTS schema, preset, or persistence-format changes — existing sessions/presets load unchanged.
- Render-harness identity: WavetableFactory output is bit-identical for the 17 extracted generators (no-op `std::fill` for `generateWavefold` does not alter output because every sample is `=`-assigned). The 3 deferred generators (Bitcrush, FM, ChurchBell) are byte-identical to v1.17.1 because their bodies were not touched.
- Visual smoke: 64 knobs render with correct labels and initial values; refPitch large knob unchanged; 5 FX bypass + process correctly with mix=0 short-circuit; 4 LFO sync + free-run toggles bidirectional with APVTS.

### Technical Notes
- Phase 2 of the `/simplify` workflow audit (see `plugins/O-Prism/.planning/SIMPLIFICATION-AUDIT.md`). Phase 3 (MEDIUM-01..07 + LOW-01..05) remains; run `/simplify-phase3 O-Prism` for the deferred low-risk sweep.
- Version bump rationale: PATCH (1.17.1 → 1.17.2) — internal refactor only, no parameter, preset, or feature changes.

## v1.17.1 (2026-05-05)

### Changed
- **HIGH-01:** Removed three completely-unused WebView resource files (`Source/ui/public/js/tuning-panel.js`, `Source/ui/public/css/tuning-panel.css`, `Source/ui/public/modules/preset-manager.js` — ~1918 LOC) plus their `juce_add_binary_data` SOURCES entries in `CMakeLists.txt` and their `getResource` URL handlers in `PluginEditor.cpp`. The runtime tuning UI is implemented inline in `index.html`; the bundled files were never `<link>`-ed or `<script>`-ed by any HTML or imported by any JS module. Removes ~30 KB from the plugin binary and eliminates a recurring footgun where editors of `tuning-panel.js` would silently modify dead code.
- **HIGH-02:** Added a `syncTuningPresetToCustom (juce::AudioProcessorValueTreeState&)` helper near the JSON array helpers and replaced 5 verbatim copies of the `setValueNotifyingHost(... kCustomTuningPresetIndex ...)` block in `PluginEditor.cpp` (`setTuningIntervals`, `setSingleInterval`, `loadScalaFile` success branch, `loadEmbeddedTuning`, `applyGeneratedScale`). One source of truth for the "force `tuningPreset` to Custom for persistence" idiom.
- **HIGH-03:** Replaced 5 ad-hoc `name.replace("\"", "\\\"")` JSON-string-escapes in `PluginEditor.cpp` (`getUserWavetableList`, `importUserWavetable`, `importUserWavetableData`, `getActiveOscInfo`, `saveEditedWavetable`) with `juce::JSON::toString` calls. Also fixes a latent bug where wavetable names containing backslashes, tabs, or control characters would produce malformed JSON.

### Verification
- Release build: `ninja O-Prism_VST3 O-Prism_AU` clean.
- AU validation: `auval -v aumu OuPr OuDv` — PASSED.
- AU cache cleared and fresh binaries installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` per project CLAUDE.md.
- Visual smoke: tuning tab renders, library list, generators, custom-cents editor, Scala load — all functional after dead-WebView purge. Wavetable name display unchanged in dropdown.
- No APVTS schema changes — existing sessions/presets load unchanged.

### Technical Notes
- Phase 1 of `/simplify` workflow audit (see `plugins/O-Prism/.planning/SIMPLIFICATION-AUDIT.md`). Phase 2 (HIGH-04..07, MEDIUM-risk) and Phase 3 (MEDIUM/LOW) candidates remain; run `/simplify-phase2 O-Prism` and `/simplify-phase3 O-Prism` for the deferred sweeps.
- Version bump rationale: PATCH (1.17.0 → 1.17.1) — internal refactor + dead-code purge, no parameter or feature changes.

## v1.17.0 (2026-04-26)

### Added
- **adds VST3 Note Expression microtonal support for Dorico** (per O-Lyrica 2.3.0 reference shape). O-Prism now responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling correct microtonal playback of quarter-tones, third-tones, and arbitrary tuning deltas authored in Dorico's tonality system. End users must set Microtonality to "VST3 Note Expression" on the assigned Dorico expression map.
- **Shared `note-expression` module adoption** (`modules/tuning/note-expression` v1.0.0).

### Technical Notes
- **Composition with TuningEngine:** `PrismVoice::startNote` queries `TuningEngine::getFrequency(midi)`, then composes Dorico's NE delta via `applyPendingTuning(table, midi, freq)` before `glide.setTarget()` and per-oscillator `setFrequency()` calls. `currentFrequency` is the multiplicative root for `freqA = currentFrequency * pow(2, ...)`, `freqB = currentFrequency * pow(2, ...)`, and `subOsc.setFrequency(currentFrequency)` — applying NE before these multiplications is mathematically correct (D-10).
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/PrismVoice.{h,cpp}`, `CMakeLists.txt`.
- **Version bump rationale:** MINOR (1.16.1 → 1.17.0) — new user-visible feature, backward compatible, no preset impact.

## v1.16.0 (2026-04-11)

### Added
- **Factory preset library: 96 presets across 10 categories** (`OuariconPresetManager.h`, `FactoryPresets.h/cpp`, `PluginProcessor.h/cpp`, `PluginEditor.cpp`, `index.html`). Full preset system with persistent in-plugin browser. Categories: Pads (18), Drone (12), Lead (12), Bass (10), Pluck (10), Harmonic (10), Keys (8), Sequence (8), FX (5), Percussion (3). Presets stored as JSON under `~/Library/O-Prism/Presets/Factory/{Category}/` on first run; user presets go in `User/`. Preset browser lives in the header bar (centered between the title and subtitle) so it's accessible from every tab — click the preset display to open a 2-column categorized picker; prev/next arrows step through the flat list; ★ saves a user preset.
- **Tuning preservation across preset switches**. New `excludedParameterIds` list on `OuariconPresetManager` — parameters in the list are never written to preset JSON and never overwritten on load. O-Prism excludes all 7 tuning parameters (`tuningPreset`, `tonic`, `masterTune`, `octaveStretch`, `pitchBendRange`, `glideMode`, `glideTime`), so switching presets leaves the active tuning/tonic/scale intact. This matches the requirement that tuning is a global setting, independent from sound design.

### Technical Notes
- Domain: Persistence + UI (new subsystem)
- Preset format: JSON with `parameters` (normalized APVTS values), `category`, `name`, `plugin`, `factory`, `version`. Factory presets are written once on first construction (`factoryPresetsExist()` guards re-initialization).
- Preset completeness: every factory preset writes every non-excluded parameter (built via `completeBase()` → category archetype → per-preset overrides), so switching presets is fully deterministic — no leftover state from the previous patch.
- Mod matrix authoring: each preset uses 2–3 mod slots minimum so nothing is a static snapshot. Common routings: velocity→filter cutoff, LFO1→OscA position (pads), AmpEnv→pitch (FX), mod wheel→LFO1 rate (leads).
- Archetypes per category share a parameter signature but each preset overrides ~10–15 distinctive params (wavetable pick, filter shape, envelope timing, FX mix). The archetype+override pattern keeps the 96 definitions maintainable in ~1500 lines of data.
- Header-bar UI: `position: absolute` preset menu floats over tab content at `top: 34px` with a fixed 540px width — survives tab switches without re-rendering. Uses the same `Juce.getNativeFunction` plumbing as the tuning panel.
- Category taxonomy and preset count sourced from cross-synth research (Serum, Vital, Pigments, Surge XT, Ableton Wavetable). 96 sits in the boutique-synth sweet spot — more than Vital's free tier (~75) and Ableton Wavetable (~130), less than Serum/Pigments (~500+).

## v1.15.0 (2026-04-11)

### Changed
- **Independent per-filter envelope depth** (`PluginProcessor.cpp`, `PrismVoice.cpp/h`, `PrismParamIds.h`, `index.html`). Split the shared `filtEnvDepth` parameter into two independent parameters, `filtAEnvDepth` and `filtBEnvDepth`, so Filter A and Filter B can be modulated by the filter envelope with their own depth/polarity. Both parameters keep the original range (-1..1, default 0). UI replaces the single "Depth" knob in the Filter Envelope section with two knobs labelled "Dep A" and "Dep B".

### Technical Notes
- Domain: DSP + UI (parameter split)
- Motivation: previous behavior forced both filters to track the filter envelope with the same depth and sign, preventing common patches like envelope-opening LP on Filter A while Filter B stays static (or moves inversely). Two depths give the standard Serum/Vital dual-filter modulation flexibility.
- DSP change at `PrismVoice.cpp` cutoff computation: `modulatedCutoffA = baseCutoffA * pow(2, filtEnvVal * filtAEnvDepth * 4)` and `modulatedCutoffB = baseCutoffB * pow(2, filtEnvVal * filtBEnvDepth * 4)`. Previously both used the single `filtEnvDepth`.
- Breaking for existing sessions: the `filtEnvDepth` parameter ID has been removed. Sessions/presets that stored a non-zero value will reset both new params to their default 0 on load (APVTS silently ignores the unknown key). To preserve the old patch, set `filtAEnvDepth` and `filtBEnvDepth` to the previous depth value.
- `allSliderIds` in `PrismParamIds.h` now lists 6 filter-envelope params instead of 5 (auto-attach stays correct — no editor code changes needed).

## v1.14.0 (2026-04-11)

### Added
- **Per-LFO free-running mode** (`PluginProcessor.cpp`, `PrismVoice.cpp`, `index.html`). New `lfo1FreeRun`..`lfo4FreeRun` bool parameters (default `false`). When enabled, an LFO's phase continues across note boundaries instead of resetting on note-on. Each LFO gains a "Retrig / Free Run" toggle button in its section header (adjacent to the existing Free/Sync rate-mode toggle).

### Technical Notes
- Domain: DSP + UI (feature addition)
- Architecture: 4 shared phase accumulators (`OPrismAudioProcessor::globalLfoPhase`) advance once per block in `processBlock` after `renderNextBlock`, using the same sync-aware rate calculation as voices. Each voice queries `getGlobalLfoPhase(i)` at the start of its sample loop and, when that LFO's `FreeRun` is enabled, copies the global phase into its local `LFO` via the new `setPhase()` accessor. All 16 voices read the same global phase → phase-locked across the voice pool, which means free-running is coherent under polyphony (not just for held monophonic notes). Without this, newly-allocated or stolen voices would start at phase 0 even when other voices are mid-cycle.
- Voice also skips `lfo[N].reset()` in `startNote` when `lfoNFreeRun` is on, so retriggered voices preserve their local state between the block-start sync writes.
- Default `false` = zero behavior change for existing presets/sessions.
- Tempo-sync compatible: the global phase advance uses the same `kDivBeats` table and `Sync`/`Division` params as `PrismVoice::renderNextBlock`, so switching between Free/Sync rate modes while Free Run is active doesn't break phase continuity.
- UI labels deliberately chosen to avoid collision with the pre-existing "Free"/"Sync" rate-mode toggle: "Retrig" (default, phase resets on note-on) vs "Free Run" (active, phase continues).

## v1.13.5 (2026-04-11)

### Changed
- **Code quality: extracted custom tuning preset index magic number** (`PrismParamIds.h`, `PluginEditor.cpp`). Introduced `PrismParamIds::kCustomTuningPresetIndex = 10` to replace the hardcoded `10.0f` literal used in 5 `setValueNotifyingHost` call sites that sync APVTS to the Custom tuning slot (setCustomTuning, setSingleInterval, loadScalaFile, applyTuningByName, applyGeneratedScale). The constant documents its coupling to the `tuningPreset` choice StringArray in `PluginProcessor::createParameterLayout()`, reducing the risk of silent drift if preset ordering ever changes.

### Technical Notes
- Domain: Code quality (refactoring)
- Root cause: Magic number repeated across 5 call sites with no named reference to the `tuningPreset` choice array. If a new built-in tuning were inserted before "Custom" in the StringArray, every site would need manual updating — the constant centralizes that coupling.
- Zero behavior change. `static_cast<float>(10)` is bit-identical to `10.0f`.
- Note: user request specified "3 locations" but code inspection found 5 — all 5 were updated for consistency.

## v1.13.4 (2026-04-11)

### Changed
- **DSP perf: hoisted key tracking `std::pow` out of sample loop** (`PrismVoice.cpp`). Filter A and B key-tracking were calling `std::pow(2.0, (filtKeyTrack * (currentMidiNote - 60)) / 12.0)` on every sample, even though `currentMidiNote`, `filtAKeyTrack`, and `filtBKeyTrack` are all constant within a render block. Replaced with two block-scoped `const double` multipliers (`keytrackMultiplierA`/`keytrackMultiplierB`) computed once before the sample loop, then applied per-sample as simple multiplications. Gated on `> 0.001f` (skips `pow` entirely when key tracking is disabled, resolving to multiply-by-1.0).

### Technical Notes
- Domain: DSP (performance)
- Root cause: Per-sample `std::pow` call on values that never change within a block. At typical buffer sizes (128–512 samples) this is 2 redundant transcendentals × buffer_size × active_voices every processBlock.
- Zero audible change — algebraic identity. Output is bit-identical to v1.13.3 (multiplication is commutative/associative for the same operand).
- Parallels the same-block hoisting pattern used for `filtEnvVal * filtEnvDepth` cutoff modulation, which remains per-sample (correctly — filter envelope is sample-varying).

## v1.13.3 (2026-04-10)

### Fixed
- **WebView event listener leaks**: Document- and window-level listeners were being attached without cleanup paths, so they persisted for the lifetime of the page and could accumulate if their host scopes ever re-ran.
  - **`wavetable-editor.js`**: `document.keydown`, `window.resize`, and `window.mouseup` (harmonic mouse-up) were bound inside `bindEvents()` as anonymous handlers and never removed. Moved them into new `bindGlobalListeners()` / `unbindGlobalListeners()` functions that run on `onTabActivated()` / `onTabDeactivated()`, guarded by a `globalListenersBound` flag. Handlers are now stored as module-level refs so they can be removed by identity.
  - **`tuning-panel.js`**: `setupRefPitchKnob()` attached `document.mousemove` and `document.mouseup` as inline anonymous arrows. Converted to instance-stored refs (`_refPitchMouseMove`, `_refPitchMouseUp`) and added a `destroy()` method that detaches them for clean teardown when the panel is replaced.
  - **`index.html` (inline tuning IIFE, ~line 2823)**: `document.mousemove` and `document.mouseup` on the A4 reference-pitch knob were anonymous. Converted to named handlers (`refPitchMouseMoveHandler`, `refPitchMouseUpHandler`) and registered a `window.__prismTuningCleanup()` hook that detaches them, giving any future teardown path a way to prevent accumulation across re-inits.

### Technical Notes
- Domain: UI (WebView)
- Root cause: Global-scope (`document`/`window`) listeners attached as anonymous functions with no removal path. While the JUCE WebView reloads fresh on each editor open (so leaks don't currently persist across plugin instances), the pattern was fragile — any future re-activation logic or UI rebuild would have caused real accumulation. This change makes every such listener removable by identity.
- No parameter changes, no DSP changes — behavior is byte-identical. Element-scoped listeners (on knob/canvas nodes inside the container) are left untouched since they die with their DOM nodes when the container is cleared.
- Note: `tuning-panel.js` is not currently imported by `index.html` (the live tuning UI is inlined), but was fixed preemptively so the pattern is correct if the module is wired in later.

## v1.13.2 (2026-04-10)

### Fixed
- **Reference pitch (A4) knob sync**: The tuning panel's reference pitch knob was desynced from the backend master tune parameter in two ways:
  1. On init, the knob hardcoded 440 Hz instead of fetching the actual `masterTune` value from the backend — the UI showed 440 even when a saved session had a different value.
  2. On mousedown, the drag baseline (`startValue`) was captured once at setup and never refreshed, so after any external parameter change (preset load, automation, undo) subsequent drags jumped relative to a stale baseline.

  Fix: `loadInitialState()` now calls `getMasterTune` and updates the knob UI via a new `updateRefPitchKnobUI()` method. The current value is mirrored on `this.masterTune`, which `mousedown` reads to set a fresh `startValue` on every drag (`tuning-panel.js`).

### Technical Notes
- Domain: UI (WebView)
- Root cause: Drag-baseline closure captured once at `setupRefPitchKnob()` time and UI init didn't query backend state
- No parameter changes — full backward compatibility

## v1.13.1 (2026-04-10)

### Fixed
- **Parallel filter routing +6dB gain inflation**: Parallel mode (Filter A + B) summed both filters at unity, producing ~2x the level of serial mode (A → B). Added 0.5x scaling to the parallel sum so both routing modes output at matched levels.

### Changed
- **Pitch mod cleanup**: Removed redundant `* 12.0 / 12.0` no-op from `pitchModRatio` calculation and replaced `std::pow(2.0, x)` with `std::exp2(x)` for clarity (`PrismVoice.cpp:429`). Zero behavior change — algebraic identity.

### Technical Notes
- Domain: DSP
- Root cause: Uncompensated additive sum in parallel filter path (`PrismVoice.cpp:601-602`)
- No parameter changes — full backward compatibility

## v1.13.0 (2026-04-09)

### Changed
- **Reorder FX chain**: Moved reverb from last to second-last position in the effects chain. New order: Distortion → Chorus → Delay → Reverb → EQ. EQ is now the final stage for post-reverb tonal shaping.
- **Effects tab UI**: Reordered to match signal chain — reverb now appears directly above EQ at bottom of effects tab.

### Technical Notes
- Domain: DSP + GUI
- No parameter changes — full backward compatibility
- Presets load identically; only processing order changed

## v1.12.1 (2026-04-09)

### Fixed
- **Reverb buzzing**: `tankFeedbackA`/`tankFeedbackB` were local variables in `process()`, reset to zero every buffer call. The Dattorro figure-8 cross-feedback was broken at buffer boundaries (~86 Hz discontinuity at 512-sample buffers). Moved to member variables for correct inter-buffer persistence.
- **Incorrect output tap formula**: Only 5 taps per channel with wrong delay line sources and reused tap positions. Replaced with full Dattorro Table 1 output: 7 taps per channel from correct delay lines and allpass diffuser nodes. Added `read()` method to `Allpass` struct for diffuser taps.
- **Static decay diffusion 2 coefficient**: Second decay diffuser used fixed `0.7f` instead of the Dattorro decay-dependent formula `decay² × 0.5 + 0.15`. Now updates dynamically with the Size parameter for natural decay character.
- **Per-sample `scaledDelay()` in audio loop**: `tankDelayA2` and `tankDelayB2` read positions were recomputed every sample via float division. Precomputed as `delayA2len`/`delayB2len` in `prepare()`.

### Technical Notes
- Domain: DSP
- Root cause: v1.12.0 Dattorro implementation had local feedback state and incomplete output tap network
- No parameter changes — full backward compatibility

## v1.12.0 (2026-04-08)

### Changed
- **Dattorro plate reverb**: Replaced stock `juce::dsp::Reverb` (Freeverb/Schroeder-Moorer) with a full Dattorro plate reverb implementation. Figure-8 tank topology with 4-stage input diffusion, cross-fed parallel decay paths, one-pole damping filters, and multi-tap stereo output. All delay lengths scaled from the original 29761 Hz reference rate.

### Added
- **Reverb modulation controls**: Two new parameters — `reverbModDepth` (0-100%, default 30%) and `reverbModRate` (0.1-5.0 Hz, default 1.0 Hz). LFO modulates tank delay lines with 90-degree phase offset between left and right paths for lush stereo movement characteristic of plate reverbs.

### Technical Notes
- Domain: DSP + GUI
- Dattorro reference: "Effect Design Part 1: Reverberator and Other Filters", J. Audio Eng. Soc., 1997
- Custom allpass, delay line, and one-pole filter structs (no heap allocation in audio thread)
- Existing parameters (Size, Damp, Pre-Dly, Mix) preserved with same IDs — full backward compatibility
- Size maps to tank decay coefficient (0.0-0.98), Damp maps to one-pole LPF coefficient
- No breaking parameter changes — existing presets load without issue

## v1.11.0 (2026-03-09)

### Added
- **Oscillator warp modes**: 4 post-wavetable-lookup warp algorithms applied per-unison-voice for maximum richness:
  - **Sync** — Hard self-sync with dual phase accumulators. Slave runs at up to 4x master frequency, hard-resets on master wrap. Creates classic formant-shifting buzz.
  - **Bend** — Asymmetric phase distortion via `pow(phase, exponent)` where exponent ranges 1-4. Shifts harmonics through nonlinear phase remapping (Casio CZ-style).
  - **FM** — Phase modulation from the other oscillator's previous sample output. Safe cross-routing allows mutual FM without ordering dependency.
  - **Window** — Windowed sync: same as Sync but with `sin(pi * masterPhase)` amplitude envelope per cycle, smoothing reset discontinuities for cleaner formant character.
- **New parameters**: `oscAWarpType` / `oscBWarpType` (Choice: Off, Sync, Bend, FM, Window) and `oscAWarpAmt` / `oscBWarpAmt` (Float 0-1) per oscillator.
- **Mod matrix destinations**: `OscA Warp` and `OscB Warp` added as modulation targets, enabling LFO/envelope-driven warp amount sweeps.

### Technical Notes
- Domain: DSP + GUI
- Per-voice FM cross-routing uses 1-sample delay (mono sum of other osc's stereo output) for stable mutual modulation
- Sync ratio range: 1x-4x (warp amount 0-100%)
- Bend exponent range: 1.0-4.0
- No breaking parameter changes — full backward compatibility with existing presets

## v1.10.0 (2026-03-08)

### Added
- **Wavetable Editor** (5th tab): Per-frame harmonic bar editing with real-time iFFT preview. Canvas-based frame strip with click/shift+click/ctrl+click multi-selection. Osc A/B toggle to edit either oscillator's table. Configurable bin count (32/64/128/256). Frame operations: Normalize (per-frame/global), Fade Edges, Reverse Audio, Reverse Order, Smooth (6dB/oct spectral rolloff). Save edited tables as new user wavetables. Undo/redo support (Ctrl+Z / Ctrl+Shift+Z, max 50 entries). DPR-aware canvas rendering for Retina displays.
- **Per-frame mipmap regeneration**: `WavetableGenerator::generateMipmapsForFrame()` regenerates all 10 mipmap levels for a single frame (~0.05ms vs ~12ms for full table), enabling real-time harmonic editing without audio glitches.
- **WavetableEditor C++ class**: Deep-copy working table management, FFT-based harmonic analysis with phase preservation, and 5 frame operations. Editor points oscillator at working copy via atomic pointer for live preview.
- 8 new native functions for WebView ↔ C++ communication: `startWavetableEditor`, `stopWavetableEditor`, `getEditorFrameWaveform`, `getFrameHarmonics`, `setFrameHarmonics`, `applyFrameOperation`, `saveEditedWavetable`, `getAllEditorFrameWaveforms`.

### Technical Notes
- Domain: Mixed (DSP + GUI)
- Milestone: add-wavetable-editor
- No new APVTS parameters — editor uses native functions for all state
- Full backward compatibility — no preset or parameter changes

## v1.9.0 (2026-03-08)

### Added
- **Custom wavetable import from .wav files**: FFT-based analysis slices audio into 2048-sample frames (up to 256 frames), builds band-limited mipmap hierarchy, and registers as a selectable user wavetable. Follows Serum's FFT 2048 import standard — short files produce fewer frames, long files truncate at 256 frames. Supports WAV, AIFF, FLAC via JUCE AudioFormatManager.
- **Drag-and-drop .wav import**: Drop audio files directly onto oscillator A or B canvas in the WebView UI. Files are read via HTML5 FileReader, base64-encoded, and decoded in C++ for FFT processing.
- **Persistent user wavetable storage**: Imported wavetables saved as 32-bit float WAV files in `~/.ouaricon/wavetables/` — survive sessions and plugin restarts. Loaded on plugin construction.
- **User wavetable management modal**: View and delete imported wavetables from the UI. Deletions auto-clear any active oscillator overrides.
- **User wavetable state persistence**: Active user table selections saved/restored in plugin state via `getStateInformation`/`setStateInformation` (backward compatible — old presets load without user tables).
- **New C++ classes**: `WavetableImporter` (FFT import pipeline), `UserWavetableManager` (persistent storage + registry). Non-APVTS override architecture preserves factory parameter range (0-27) for full backward compatibility.

## v1.8.1 (2026-03-06)

### Fixed
- **Broken oscillator & tuning visualizations**: `bindLfoSync()` used the old JUCE API `syncState.addListener({handleToggleStateChange})` instead of JUCE 8's `syncState.valueChangedEvent.addListener()`. The `TypeError` halted the ES module, preventing all subsequent code (WavetableDisplay, tuning system) from initializing. Introduced in v1.4.0 when tempo-synced LFO rates were added. Also fixes LFO sync toggle not reflecting state changes from DAW automation.

## v1.8.0 (2026-03-05)

### Added
- **Master stereo width control**: New `stereoWidth` parameter (0.0–2.0, default 1.0) applies mid-side processing after the effects chain and before master volume. 0.0 = mono, 1.0 = normal stereo, 2.0 = extra wide. Formula: `mid = (L+R)*0.5, side = (L-R)*0.5, L = mid + side*width, R = mid - side*width`. Uses per-sample smoothing to prevent zipper noise. Mono buffer fallback for single-channel hosts.

## v1.7.0 (2026-03-05)

### Changed
- **3-voice ensemble chorus**: Replaced `juce::dsp::Chorus` (single-voice) with custom `EnsembleChorus` engine. Three independent delay lines with staggered center delays (5ms, 7ms, 9ms), each modulated by sine LFOs at slightly different rates (1.0x, 0.93x, 1.07x) with 120-degree phase offsets. Equal-power stereo panning spreads voices across the stereo field (L/C/R at -0.6/0.0/+0.6). Max LFO modulation depth of 2ms. Wet gain normalized by 1/sqrt(3) for consistent output level. Same `chorusRate`, `chorusDepth`, `chorusMix` parameters — no preset breakage.

## v1.6.0 (2026-03-05)

### Added
- **Velocity curve parameter**: New `velocityCurve` choice parameter with 4 modes — Linear (default, unchanged behavior), Soft (sqrt curve, more dynamic range at low velocities), Hard (squared curve, requires harder hits), and Fixed (always full velocity regardless of key strike). Curve transformation applied in `startNote()` so `noteVelocity` is already curved before use in `renderNextBlock()`. Fully DAW-automatable.

## v1.5.0 (2026-03-05)

### Changed
- **Stereo noise generator**: `NoiseGenerator` now produces independent noise per channel via `getNextSampleStereo()`. White and Digital types use separate PRNG instances (randomL/randomR). Pink noise has independent Paul Kellet filter states per channel (b0L/b1L/b2L, b0R/b1R/b2R). Brown noise has independent integrator states. Vinyl has independent bandpass filters and crackle events per channel. Wind shares the LFO (coherent spectral sweep) but uses independent brown noise sources and lowpass filter states per channel. Previously a single mono sample was added identically to both L and R — now decorrelated noise provides true stereo width.

## v1.4.0 (2026-03-05)

### Added
- **Tempo-synced LFO rates**: Each of the 4 LFOs now has a Sync toggle and note Division selector. When Sync is enabled, LFO rate is calculated from host BPM instead of the free-running Hz knob. 18 note divisions available: straight (1/1 through 1/32), dotted (1/1D through 1/32D), and triplet (1/1T through 1/32T). BPM is read from the DAW transport via `getPlayHead()->getPosition()->getBpm()`. 8 new APVTS parameters: `lfo1Sync`, `lfo1Division`, `lfo2Sync`, `lfo2Division`, `lfo3Sync`, `lfo3Division`, `lfo4Sync`, `lfo4Division`. UI shows Free/Sync toggle per LFO — when synced, the rate knob hides and division dropdown appears.

## v1.3.0 (2026-03-05)

### Added
- **Pitch modulation destination**: Added "Pitch" as the 23rd mod destination in the modulation matrix. Routes any source (LFO, envelope, velocity, mod wheel, etc.) to pitch for vibrato, pitch envelopes, and velocity-to-pitch effects. Applied as a semitone offset (±12 semitones at full modulation) multiplied into oscillator and sub-oscillator frequency calculations.

## v1.2.2 (2026-03-05)

### Changed
- **Cached PrismVoice APVTS pointers**: Cache all 50 `std::atomic<float>*` parameter pointers once in `setAPVTS()` instead of performing string-based hash map lookups via `getRawParameterValue()` every audio block. `renderNextBlock()` (44 reads) and `startNote()` (24 reads) now do direct atomic loads. At 8 voices, eliminates ~352 hash map lookups per block.

## v1.2.1 (2026-03-05)

### Fixed
- **Report correct latency from distortion oversampling**: `DistortionProcessor` uses 2x oversampling which introduces latency, but `prepareToPlay()` called `setLatencySamples(0)`. Now reads `oversampling.getLatencyInSamples()` after preparing the distortion processor and reports it to the host so DAWs can apply proper delay compensation.

## v1.2.0 (2026-03-05)

### Changed
- **Cached ModulationMatrix APVTS pointers**: Cache all 64 `std::atomic<float>*` parameter pointers once in `setAPVTS()` instead of constructing 16 prefix strings and performing 64 hash map lookups every `processBlock` call. `updateFromAPVTS()` now does 64 direct atomic loads with zero string allocation or map traversal.

## v1.1.9 (2026-03-04)

### Changed
- **Per-block oscillator tuning reads**: Moved `oscACoarse`, `oscAFine`, `oscBCoarse`, `oscBFine` APVTS reads from per-sample to per-block in `PrismVoice::renderNextBlock`. Precompute pitch ratios (`std::pow`) once per block instead of every sample — eliminates 4 atomic loads and 2 `std::pow` calls per sample per voice.

## v1.1.8 (2026-03-04)

### Changed
- **SVFFilter coefficient caching**: Added dirty-flag to `SVFFilter` so `updateCoefficients()` (which computes `std::tan()`) only runs when cutoff or resonance actually change. Previously `setCutoff()` and `setResonance()` each triggered a full recompute — 8 `std::tan()` calls per sample per voice. Now deferred to `processSample()` with value-change detection: 2x reduction when modulated, zero cost when static.

## v1.1.7 (2026-03-04)

### Removed
- **Deprecated compatibility stubs**: Removed `connectMTSClient()` (always returned false with a DBG message) and dual-arg `loadScalaFile(File&, File&)` (ignored second argument, delegated to single-arg overload). Neither had any callers.

## v1.1.6 (2026-03-04)

### Changed
- **JSON array helpers**: Extracted `toJsonArray` (template with lambda) and `toJsonFloatArray` (strided raw pointer) helpers in PluginEditor.cpp — replaced 11 instances of manual `"[" + for-loop + "]"` JSON string building across `addNativeFunctions()` and `timerCallback()`

## v1.1.5 (2026-03-04)

### Changed
- **Shared math constants**: Consolidated `kPi`, `kTwoPi`, `kHalfPi` definitions from 8 source files into a single `dsp/MathConstants.h` header. Removed 10 duplicate `static constexpr` locals across WavetableOscillator, SubOscillator, NoiseGenerator, DistortionProcessor, PrismVoice, SVFFilter, WavetableFactory, and WavetableGenerator.

## v1.1.4 (2026-03-04)

### Removed
- **Dead parameter**: Removed unused `polyphony` APVTS parameter — was defined in `createGlobalParameters()` and bound in UI footer but never read by processBlock or voice management. Synth always uses 16 voices. Removed from PluginProcessor.cpp, PrismParamIds.h, and WebView UI footer.

## v1.1.3 (2026-03-04)

### Removed
- **Dead code**: Removed unused `prevPhase` variable in LFO.cpp — was assigned from `phase` but never read

## v1.1.2 (2026-03-04)

### Removed
- **Dead code**: Removed unused `activeNotesMutex` from PluginProcessor — note tracking already uses lock-free `std::atomic<bool>` array, the mutex was declared but never locked anywhere

## v1.1.1 (2026-03-04)

### Changed
- **Knob visual overhaul**: Replaced all 63 knobs from CSS conic-gradient rotary style to SVG vine-arc style (matching O-Detune). Green vine stroke (#5a7a6a) animates around a tan track with smooth requestAnimationFrame interpolation. Three sizes: standard (52px), small (44px, footer), large (64px, A4 ref pitch). Added mouse wheel support and double-click reset to all knobs.

## v1.0.1 (2026-03-03)

### Fixed
- **Sticky unison knobs**: Osc A/B Unison knobs required ~25px of drag to change by one step, making them feel stuck. Added adaptive drag sensitivity — discrete parameters (≤16 steps) now require ~8px per step instead. Continuous knobs are unaffected.

## v1.0.0 (2026-02-23)

### Breaking Changes
- Removed `lfo1Depth`, `lfo2Depth`, `lfo1Dest`, `lfo2Dest` APVTS parameters (replaced by modulation matrix)
- Presets saved with v0.12.0 will lose LFO depth/destination settings on load; re-create them as mod matrix routes

### Added
- **16-slot modulation matrix** with per-sample evaluation in each voice
- 9 modulation sources: None, LFO1, LFO2, AmpEnv, FilterEnv, Velocity, NoteNum, ModWheel, Aftertouch
- 21 modulation destinations: None, OscA/B Position, FiltA/B Cutoff, FiltA/B Resonance, Osc Mix, Sub Level, Noise Level, LFO1/2 Rate, OscA/B Detune, OscA/B Pan, Reverb/Delay/Chorus/Dist Mix, Master Vol
- Each slot has: source selector, destination selector, bipolar amount (-100% to +100%), on/off toggle
- 64 new APVTS parameters (4 per slot x 16 slots), all fully DAW-automatable
- MIDI ModWheel (CC1) and Channel Aftertouch captured as global mod sources
- New "Mod" tab in WebView UI with interactive routing list (dropdowns + sliders)
- `ModulationMatrix` DSP class (`Source/dsp/ModulationMatrix.h/.cpp`) with fixed-size arrays for zero-allocation audio-thread operation

### Changed
- LFO sections in Synth tab now show Rate + Shape only (routing moved to Mod tab)
- Filter resonance now modulatable per-sample via mod matrix (previously static per-block)
- Pan modulation now computed per-sample when mod routes target OscA/B Pan

### Technical Notes
- Mod matrix routes evaluated per-sample inside `PrismVoice::renderNextBlock` for click-free modulation
- Source values computed once per sample, then all 16 slots iterated (early-out for disabled/None slots)
- Cutoff modulation uses multiplicative octave-scaling: `cutoff * pow(2, modOffset * 4)` matching the filter envelope pattern
- Additive modulation for position/level/pan destinations, clamped to valid ranges
- Processor stores ModWheel/Aftertouch as `std::atomic<float>`, read by voices each sample

## v0.12.0 (2026-02-24)

### Added
- **LFO system** with 2 independent per-voice LFOs for smooth per-sample modulation
- LFO1 hardcoded to modulate Osc A wavetable position, LFO2 hardcoded to modulate Filter A cutoff
- Each LFO has Rate (0.01–20 Hz, skewed), Shape (Sine/Triangle/Saw/Square/S&H), Depth (0–100%), and Dest selector
- 8 new APVTS parameters: `lfo1Rate`, `lfo1Shape`, `lfo1Depth`, `lfo1Dest`, `lfo2Rate`, `lfo2Shape`, `lfo2Depth`, `lfo2Dest`
- Generic reusable `LFO` DSP class (`Source/dsp/LFO.h/.cpp`) with phase accumulator design — ready for future modulation matrix
- Dest parameters included as Choice params (Osc A Pos / Osc B Pos / Filt A Cut / Filt B Cut / Pitch) for future routing
- WebView UI: LFO 1 and LFO 2 sections in Synth tab with rate knobs, shape/dest dropdowns, and depth knobs

### Technical Notes
- LFO modulation applied per-sample inside `PrismVoice::renderNextBlock` for click-free smooth modulation
- LFO1 applies additive modulation to wavetable position: `pos + lfoVal * depth`, clamped [0,1]
- LFO2 applies multiplicative modulation to filter cutoff: `cutoff * pow(2, lfoVal * depth * 4)` — same pattern as filter envelope
- LFOs reset phase on note-on for consistent attack character
- S&H shape triggers new random value on phase wrap

## v0.11.0 (2026-02-23)

### Added
- Expanded factory wavetable library from 4 single-frame tables to 28 multi-frame wavetables across 5 categories
- **Analog** (3 new): PWM Sweep, Supersaw, Sync Sweep (32 frames each)
- **Digital** (5 new): FM E.Piano, FM Bell, FM Metallic, Wavefold, Bitcrush (32 frames each)
- **Formant** (4 new): Vowel Morph (64 frames), Choir Pad (48 frames), Vocal Lead (32 frames), Formant Filter (32 frames)
- **Spectral** (6 new): Harmonic Series, Spectral Tilt, Odd Harmonics, Harmonic Stretch, Comb Sweep, Prism Spectrum (32 frames each)
- **Organic** (6 new): Breath, Plucked String, Church Bell, Organ Sweep, Wind, Filtered Noise (16-32 frames each)
- Categorized wavetable dropdown menus with optgroup sections
- WavetableFactory class for procedural multi-frame table generation
- All tables generated procedurally with deterministic RNG seeds

### Changed
- Wavetable selector range expanded from 0-3 to 0-27
- Position knob now sweeps through multiple frames per table for musically useful morphing
- Original 4 tables (Saw, Square, Triangle, Sine) preserved at indices 0-3 for preset compatibility

## v0.9.2 (2026-02-18)

### Fixed
- **Stereo filter distortion**: Mono filter + stereo balance reconstruction caused full-wave rectification on left channel and 3x amplification on right channel during negative signal excursions. All waveforms were severely distorted (sine sounded like square). Replaced with true stereo filter processing using independent L/R filter instances.
- **Wavetable selection mapping**: oscATable/oscBTable parameter range was [0, 15] but only 4 factory tables exist. UI dropdown normalized values mapped incorrectly — selecting Square or Triangle both loaded the Sine table. Fixed parameter range to [0, 3] matching the 4 factory waveforms.

## v0.9.1 (2026-02-18)

- Initial release with tuning panel v2.0.0
