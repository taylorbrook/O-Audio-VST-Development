# Stage 3 (GUI) — SUMMARY

**Outcome:** The Stage-1 generic editor is replaced by the single-page Ouaricon
"simple-family" WebView teaching UI. All three ROADMAP GUI phases (3.1 grid +
playhead + controls + cross-platform wiring, 3.2 timing lane + live MIDI readout,
3.3 tooltips + single-page scaffolding + preset hook) landed together in one
express-mode execute. Build clean (VST3+AU+Standalone), auval SUCCEEDED,
pluginval strictness-10 SUCCESS, UI renders fully (verified by screenshot).

## What was built

### WebView UI (`Source/ui/public/`)
- **`index.html`** — single page, top-to-bottom per the BRIEF: header + live
  transport readout → **step grid** → **timing/groove lane** → **live MIDI
  readout** → Timing-Feel knobs + Pattern-Length selector → 6 per-voice strips →
  Master + preset tour → floating tooltip. Loads `check_native_interop.js` then
  `app.js` as `type="module"`.
- **`css/styles.css`** — field-guide aesthetic (parchment, Garamond, amber accent,
  seed-cross-section knobs) consistent with O-simpleFM/Subtractive; per-voice hue
  coding; DPR-safe lane canvas; projector-legible type.
- **`js/app.js`** — the controller:
  - Binds all 42 params: 29 knobs (relative vertical drag), 1 selector
    (`patternLength`), 12 mute/solo toggles — two-way via the Juce state objects.
  - **Step grid** built in JS (rows = voices in GM order, cols = active
    `patternLength`). Left-click cycles `off → normal(100) → accent(127) →
    ghost(40) → off`; right-click erases; cell fill height + brightness + glyph
    encode velocity. Each edit pushes through the `setStep` native fn.
  - `getGrid()` paints the authoritative C++ grid on load and re-polls ~4×/s so a
    host preset/state restore shows up live.
  - **Playhead**: the `frame` event's fractional step phase highlights the current
    column in the grid and draws a continuous sweep line in the lane.
  - **Timing/groove lane** (QUAL-02): each hit is plotted at its grid line offset by
    the *applied* Δt (the `d = applied − nominal` field straight off the C++ event),
    converted to a fraction of a 16th via sample-rate + live BPM — never a feel
    recompute. Dots fade over ~1.6 s; connector lines show early/late; voice-hued.
  - **Live MIDI readout**: every drained note-on printed with SEQ/MIDI source, note
    number, voice, velocity — the sequencer and played MIDI in one stream.
  - **Tooltips** on every control + the grid + lane + MIDI panel (plain-language).
  - **Preset tour** hook (6 concept buttons; content lands in Stage 4).
- Copied `js/juce/{index.js, check_native_interop.js}` from O-simpleSubtractive.

### C++ integration (`Source/PluginEditor.{h,cpp}`)
- Member order **relays → WebView → attachments** (destroy-in-reverse safe).
- 29 `WebSliderRelay` + 1 `WebComboBoxRelay` + 12 `WebToggleButtonRelay`, built from
  the `BeatmakerIDs::ParamIDs` single source of truth; 3-arg JUCE-8 attachments
  (`nullptr` undoManager); `jassert(param)` on each (catches ID drift in debug).
- Bare-path resource provider (`/`, `/index.html`, `/css/styles.css`, `/js/app.js`,
  `/js/juce/...`), `charset=utf-8` on text resources.
- Native fns `setStep` / `getGrid` / `clearGrid` / `getSampleRate`.
- `#if JUCE_WINDOWS` `withUserDataFolder(tempDir)` (avoids the silent IE-fallback
  blank page in DAW hosts).
- 60 Hz `Timer` drains `VizAnalyzer` into one `"frame"` event
  (`{ph, bpm, sync, hits[]}`) — one push feeds grid flash + lane + MIDI readout.

### Processor taps (`Source/PluginProcessor.{h,cpp}`)
- Added read-only, advisory: `getCurrentSampleRate()`, `getLastKnownBpm()` (relaxed
  atomic, stored once/block), `isHostSynced()` (relaxed atomic). **No DSP behaviour
  change** — purely the UI's lane scale + transport readout.

### Build (`CMakeLists.txt`)
- `juce_add_binary_data(O-simpleBeatmaker_UIResources …)` — the **only** binary-data
  target (synth has no embedded samples), so default `BinaryData` namespace is
  correct; the O-simpleGrain dual-namespace collision does not apply. Linked PRIVATE.
  WebView2 cross-platform flags were already present from Foundation.

## Files
- **Created:** `Source/ui/public/index.html`, `Source/ui/public/css/styles.css`,
  `Source/ui/public/js/app.js`, `Source/ui/public/js/juce/index.js`,
  `Source/ui/public/js/juce/check_native_interop.js`.
- **Modified:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`,
  `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `CMakeLists.txt`.

## Verification done
- `ninja` VST3 + AU + Standalone: clean (only benign JUCE switch-enum/deprecation warnings).
- `auval -v aumu OSiB OuDv`: **AU VALIDATION SUCCEEDED** (render, 1-channel, bad-max-frames, param-set/ramp, **MIDI** all PASS).
- `pluginval --strictness-level 10`: **SUCCESS** (buses 0-in/2-out, fuzz params).
- **Visual:** Standalone screenshot — full UI renders (grid + playhead + lane + MIDI readout + all knobs/strips + master). Not blank.
- **Native-fn parity:** JS `getNativeFunction` set == C++ `withNativeFunction` set.
- Cache cleared + dual-variant swept + dev bundles installed to system folders.

## Deviations / notes
- Combined the 3 ROADMAP GUI sub-phases into one execute (express mode) — coherent
  single page, built and gated as a whole.
- The timing lane is **tempo-normalised** (Δt as a fraction of a 16th) — required two
  tiny read-only processor taps (sample-rate + BPM); documented as a contract-safe,
  non-DSP addition.
- Preset content is intentionally deferred to Stage 4 (FUNC-05) — Stage 3 ships the
  selector hook only, per CONTEXT non-goals.

## Residual for Stage 4
- Concept-isolating factory presets (Straight / Backbeat+Accents / Ghost / Triplet
  Swing / Humanized / Quantize demo), playability tuning, final validation sweep,
  QUAL-02 audible-vs-visible audit, CHANGELOG v1.0.0.
