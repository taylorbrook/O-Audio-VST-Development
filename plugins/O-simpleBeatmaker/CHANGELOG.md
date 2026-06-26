# Changelog — O-simpleBeatmaker

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.0] — 2026-06-25

First release. A pedagogical TR-808/909-lineage step-sequencer drum machine built
for the MUSC319 wk09 MIDI & beatmaking session: program a beat on a 16-step grid,
then **watch and hear** velocity, swing, quantize, and humanize reshape it in real
time. The step grid and the piano roll are literally two views of one MIDI stream —
the internal sequencer emits GM-mapped note-ons at sample-accurate offsets into the
same `MidiBuffer` as incoming host MIDI, so the voices and the visualiser see one
merged stream. Playable live over MIDI as a real 808/909-style instrument too.

Built in four staged passes (Foundation → DSP → GUI → Polish), each gated by an
offline render-harness and pluginval/auval.

### Added

- **Six synthesized drum voices** (no samples): Kick, Snare, Clap, Closed Hat,
  Open Hat, Tom — 808/909 flavour per voice, with the closed hat choking the open
  hat. Per-voice **tune / decay / tone / level** plus **mute / solo**. GM drum map
  36/38/39/42/46/45 — every voice is MIDI-playable from the DAW piano roll or a pad.
- **Host-synced step sequencer** with a **sample-accurate sub-step Δt** scheduler.
  Reads the host transport (tempo / ppq / play state), enumerates firing steps per
  block, and emits each hit at its exact sample offset (no block-boundary snapping).
  Free-runs at the `tempo` knob when the host is stopped / in Standalone.
- **Timing-feel engine** — the pedagogical heart:
  - **Swing** (0–75%) delays the off-beat 16ths into long-short pairs.
  - **Humanize** (0–100%) adds small pre-seeded random per-hit timing + velocity
    offsets so repeats aren't identical.
  - **Quantize Strength** (0–100%) pulls the *humanized* deviation back toward the
    grid — and, critically, **leaves intentional swing untouched** (the DSP-04
    invariant: `Δt = Δswing + Δhuman·(1−q)`).
- **Per-step velocity** (0–127) with ghost / normal / accent quick-states; velocity
  drives loudness and a little timbre (harder = brighter / snappier).
- **Selectable pattern length** (8 / 16 / 32 steps); custom 6×32 grid persisted in a
  `PATTERN` ValueTree child (lock-free `std::atomic<uint8_t>` grid, not 384 params).
- **WebView teaching UI** (single projector-readable page):
  - 6×16 step grid with click-to-toggle, click-again velocity cycle, and a live
    amber **playhead** sweeping in sync with the transport.
  - **Timing / groove lane** drawing each hit's **applied Δt** (the exact offset
    baked into the audio, not a UI recompute) — swing pushes off-beats late,
    humanize scatters, quantize pulls back, all visible in real time.
  - **Live MIDI readout** printing note-on (note#, velocity) events from both the
    internal sequencer and incoming MIDI, with a source flag.
  - Plain-language **tooltips** on every control (pointer + keyboard focus), grid
    keyboard operability, ARIA labelling, and a **Clear all** affordance.
- **Six concept-isolating factory presets** (the lesson tour) — each isolates one
  idea so a student can reverse-engineer the move:
  **Straight** (flat / no-feel baseline) · **Backbeat + Accents** (velocity alone) ·
  **Ghost Notes** (quiet snares that make it breathe) · **Triplet Swing** (clean
  shuffle, no scatter) · **Humanized** (loosened off the grid) · **Quantize Demo**
  (sweep quantize to pull the scatter back while the swing stays). Loading a preset
  sets the timing-feel knobs (host-notifying, so the UI updates) and stamps the grid.
- **Offline render-harness** (`tests/render-harness`, `-DOUARICON_BUILD_TESTS=ON`) —
  the DSP correctness gate: a headless console app injects a synthetic transport and
  asserts the six probes (grid accuracy ±0 samples, swing offset, humanize+quantize
  bounds, block-boundary independence, MIDI-playable voices + hat choke + aliasing
  budget, and viz-truth = the lane offset equals the applied audio Δt).

### Technical

- Real-time safe `processBlock`: no allocations / locks / file I/O on the audio
  thread; humanize RNG pre-seeded in `prepareToPlay`; `fastSine` LUT warmed there
  too. Audio→UI handoff via a lock-free `AbstractFifo` + atomics. Zero added latency
  (`setLatencySamples(0)`; the scheduling lookahead is bookkeeping, not a delay line).
- Cross-platform WebView: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`,
  Windows `withUserDataFolder(tempDir)`, bare-path resource provider, single
  `O-simpleBeatmaker_UIResources` binary-data target (default BinaryData namespace).
- Validation: clean VST3 + AU + Standalone build; **auval `aumu OSiB OuDv` SUCCEEDED**
  (render / 1-channel / bad-max-frames / parameter set + ramp / MIDI);
  **pluginval `--strictness-level 10` SUCCESS** for both VST3 and AU; render-harness
  6/6 probes green.
