# Changelog - O-simpleAmbisonics

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.2] — 2026-10-19

### Changed

- **Double-click a dial to type a value** — Enter or click away applies, Esc cancels. **Alt/Option-click resets** a control to its default (this was double-click). Output Level takes `-inf` / `off` for the floor. The sphere wells are unchanged.

## [1.0.1] - 2026-10-14

Review pass (`/improve`): bugs, simplification, UI idle cost. No parameter, range, state
or DSP-output change; the engine's audio path is untouched.

### Fixed
- **Save could overwrite a user preset without asking.** The page checked the typed
  name for the "Replace?" prompt, but the plugin strips `"#@,;:<>*^|?\/`, control
  characters and leading dots before saving — "My:Pad" silently replaced "MyPad". The
  page now cleans the name exactly as the plugin does before both checks (and "Init?"
  now shows the factory-name message, not the generic save error).
- **A user preset named like a copy key was translated in the dialogs** ("decoder"
  read « Décodeur » in the Delete / Replace prompts). Names are bidi-isolated (FSI …
  PDI), which also keeps right-to-left text in a name from reordering the sentence.
- **The azimuth automation gesture could end mid-drag.** Scrolling the knob and then
  dragging the sphere within 250 ms let the wheel timer end the gesture while the drag
  still wrote. One shared pointer-drag helper now owns one gesture per parameter: a
  sphere drag first closes the knob's wheel gesture, and a second pointer, the other
  front-end of the same parameter, the wheel or the keys are ignored while a drag runs.
- The "gain (pattern)" and "peak" captions under the channel bars sat ≈ 7 px right of
  their marks.
- The output meter drew past the top of its well above 0 dBFS. The bar now stops at 0
  and a clip lamp over each side lights for 1.5 s when the output reaches 0 dBFS.
- CHANGELOG 1.0.0: Distance never had a delay (gain + air low-pass only).

### Changed
- **The page idles when nothing moves.** The host-stopped frame is cached on its
  inputs instead of rebuilt every display frame; a meter frame redraws only the
  canvases it changes; the signal-path stations touch the DOM only when their light
  flips; the chevron colour is a constant (no `getComputedStyle` per redraw).
- An exception in one frame's drawing no longer stops the animation for the session
  (the frame callback is re-armed first).
- The Order / Decoder / Orbit segments are proper radio groups: one Tab stop, arrow
  keys move and select.
- Simplified: one 12-id parameter list (`ParamIDs::kAll`) for the preset targets and
  the editor's defaults native; one segment binder; dead code removed
  (`DecoderMixer::processSample`, `SpatialEngine::maxBlockSize` / `latencySamples`,
  the unused procedural SH icons, unused locals and CSS variables).

## [1.0.0] - 2026-10-14

First release. A pedagogical ambisonic spatializer: one mono source is encoded to
ambisonics, the scene is rotated, and the result is decoded to stereo, all inside one
stereo-out plugin with no multichannel bus. The "Field Guide" WebView shows every
stage of the chain as you hear it.

### Added
- **The full chain on a stereo track.** Mono or stereo input (Input Mode: Mono sum /
  Left only) → SN3D / ACN spherical-harmonic encoder → rotation in the ambisonic
  domain → decoder → L / R. Mono-in / stereo-out and stereo-in / stereo-out only
  (auval caps `[1, 2] [2, 2]`); no I/O configuration in the host.
- **Source position.** Azimuth −180 … +180° (AmbiX: +90° = left), Elevation −90 … +90°,
  Distance 0.2 – 20 m (gain and an air low-pass). Angle moves take the
  short way round (+170° → −170° passes through 180°) and sweep without zipper noise.
- **Orders 1 / 2 / 3** (4 / 9 / 16 channels), switchable while audio runs with a
  click-free crossfade.
- **Scene rotation.** Yaw, Pitch and Roll rotate the whole sound field after encoding
  (the scene turns, the source stays put), by the Ivanic–Ruedenberg recursion; zero
  rotation is bit-transparent.
- **Two decoders.** Binaural for headphones (SADIE II KU100 MagLS filters per order,
  embedded, uniformly partitioned convolution) and Stereo for speakers or a projector.
  Switching crossfades. Reported latency 64 samples in both modes at every rate.
- **Orbit.** Automatic azimuth motion, On / Off and 0.02 – 2 Hz, so movement is
  audible without DAW automation.
- **Field Guide UI** (1180 × 780 design frame, resizable 0.6× – 1.5× at a fixed aspect,
  fits the display it opens on): the signal path left to right (Source | Encoder |
  Rotation | Decoder | Output), the source on a sphere with a draggable puck and a
  rotating compass, live B-format meters labelled by spherical harmonic (4 / 9 / 16),
  a polar plot, the decoder view, an output meter, tooltips on every control, and
  English, French and Simplified Chinese. Keyboard and screen-reader support on every
  knob (arrows, Home / End, Page Up / Page Down).
- **Lessons.** Eight lesson buttons, each isolating one idea, with a caption that
  says what to listen for: Point Source (FOA), Order 1 → 2 → 3 (repeated clicks walk
  the order 1 → 2 → 3 → 1), Behind You, Overhead, Rotate the Room, Far Away / Up Close,
  Slow Orbit, Speakers vs Headphones.
- **Preset manager.** A header preset bar (◀ name ▶, Save, Delete) with 9 factory
  presets (Init + the 8 lessons, from one recipe table). User presets save, replace
  (with a confirm) and delete in the page; factory names are protected. The bar shows
  the current name and a dot when it has been edited, and lights the matching lesson.
  **Presets and lessons never change Output Level or Input Mode.** Presets live in
  `~/Library/O-simpleAmbisonics/Presets`; the session remembers the preset name,
  language and window size.
- **Measured quality and performance** (render harness, 289 gates; installed VST3 via
  pedalboard).
  - Encoder vs an independent double table: max error 2.07e-07 over a 13 × 7 grid.
  - Rotation: rotate(encode) == encode(rotated) within 7.2e-07 at order 3.
  - Binaural at +90°: ILD +12.3 / +13.8 / +13.9 dB (left louder) and ITD +36 / +38 / +37
    samples at orders 1 / 2 / 3, mirrored at −90°.
  - CPU: order 3 Binaural at 64-sample blocks renders ≈ 195× faster than real time at
    48 kHz (≈ 72× at 96 kHz), Release, Apple Silicon; 0 allocations on the audio thread.

### Fixed in Stage 4
- **Windows: the interface would have shown "page not found".** The editor now passes
  `.withBackend (Backend::webview2)` (WR-01; Windows CI build + pluginval s10 pass).
- **One NaN / Inf input sample silenced the plugin for the session** (the air
  low-pass latched it). Non-finite input samples are now zeroed (WR-02; harness gate +
  negative control).
- **A host reset left the previous tail playing** (up to ≈ 6.7 ms). `reset()` now
  clears the engine (WR-03; tone after reset bit-equal to a fresh instance).
- **Input Mode switched in one sample (click).** It now ramps over 20 ms (WR-04;
  click ratio 1.003 vs the same-motion reference, splice control 579).
- **Tail length was reported as 0.** It is now 64 samples + the decoder filter length
  at the running rate (WR-05; measured 317 ≤ reported 320 samples at 48 kHz).
- **Lessons reset Input Mode.** Input Mode is now user-owned like Output Level (WR-06).
- **Lesson ids lived in four hand-kept copies; a misspelled id looked applied.** One
  recipe table; the lit lesson comes from the plugin's own state (WR-07).
- Processing before the host prepared the plugin is now safely silent (IN-01).
- A Debug assertion at 352.8 / 384 kHz (IN-04); blurry canvases after moving between
  displays (IN-07); canvas labels below 4.5:1 contrast (IN-08); right-click on a knob
  opened a host gesture (IN-11); a stale lesson light after edits and reopen (IN-13);
  a restored session without a window size kept the previous instance's (IN-19);
  VST3 category is now Fx | Spatial (IN-20).
- The order-3 encoder readout "(3+1)² = 16 ch" was cut off in English; the front
  chevron overlapped the puck when azimuth ≈ yaw (carried from Stage 3).
- The latency chip in the header was retired for the preset bar; the latency sentence
  moved into the Decoder tooltip.

### Known limitations
- No output safety stage: at 0.2 m and order 3 a 0 dBFS source can peak near
  +15 dBFS (a float output; no factory preset reaches this corner).
- The Binaural filters' onsets (50 – 84 samples) are longer than the decoder
  crossfade's guard, so a decoder switch can comb briefly during its 20 ms fade.
- A session restored into an already-open editor keeps the page language until the
  editor is reopened.
- At the 0.6× minimum window scale the smallest captions render at about 6 px.
- French copy awaits a native review; Simplified Chinese is machine-assisted.
- v1.0 is stereo-out only: no B-format output bus, no head tracking, no SOFA import.

### Development history
- Stage 1 (2026-10-12): JUCE / WebView shell, 12 parameters, mono + stereo input
  buses, 64-sample reported latency from the start; pluginval s10 VST3 + AU.
- Stage 2 (2026-10-12 – 13): encoder + Stereo decoder (2.1), SH rotation (2.2),
  MagLS Binaural with partitioned convolution, crossfades and CPU / latency gates (2.3).
- Stage 3 (2026-10-13 – 14): the Field Guide WebView, live meters and sphere view,
  polar plot, decoder view, tooltips, lessons, en / fr / zh.
- Stage 4 (2026-10-14): four-critic code review and fixes, the preset manager and
  header bar, validation sweep (pluginval s10 ×2 VST3 + AU, auval, pedalboard on the
  installed VST3), Windows CI build + pluginval s10 (validate-only, no release).
