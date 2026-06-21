# Changelog — O-simpleFM

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.0] — 2026-06-20

First release. A pedagogical 2-operator FM / phase-modulation synthesizer with a
field-guide "Naturalist" WebView UI, built to make *"oh, THAT's how FM works"* land
in about five minutes.

### Synth engine (DSP)
- 16-voice polyphonic 2-operator **phase-modulation** voice (radians convention,
  1:1 with Chowning/Bessel math). MIDI instrument, audio out.
- Modulation Index `I` 0–20 with a perceptual `I = 20·norm^1.7` taper; carrier null
  reachable at I ≈ 2.405 (Bessel J₀ zero) as a teaching marker.
- C:M **Ratio** with optional integer **Snap** (harmonic ⇄ inharmonic), plus a
  **Fixed-Hz** modulator mode for key-independent, formant-like colour.
- DX7-style modulator **self-feedback** (two-sample average / Tomisawa anti-hunting,
  history clamp + NaN scrub + note-on reset) — sine → saw → noise, stable at 100%.
- Independent **mod** and **amp** ADSR envelopes; mod-env → index (depth default 1.0),
  optional velocity → index; amp envelope governs voice lifetime.
- Anti-aliasing: sine LUT + key-tracked **Carson index ceiling** + 2× polyphase-IIR
  oversampling, always on (v1.0 sine-only chain).

### Interface (WebView)
- Single-page Ouaricon-Naturalist UI; all 17 parameters two-way bound (relative-drag
  knobs, wheel, and **keyboard** arrow-key control).
- Live **spectrum** (4096-pt FFT, Blackman-Harris) + **oscilloscope**, pushed at 30 Hz
  off a lock-free audio-thread tap (no audio-thread FFT/alloc).
- Live operator **routing diagram** (MOD → CAR + feedback loop, thickness ← index/feedback).
- Plain-language **tooltips** on every parameter, reachable by mouse *and* keyboard focus.
- **Lesson Presets** tour (E-Piano, Tubular Bell, Brass, Clarinet, Clang Bell) — each
  isolates one FM concept.

### Presets
- Suite-canonical **preset manager**: factory + user presets persisted as JSON under
  `~/Library/O-simpleFM/Presets/`, surviving DAW session reloads.
- In-UI **preset browser**: factory/user list, prev/next navigation, save (native
  dialog), and delete (factory presets protected — Delete disables on them).
- 6 factory presets shipped (Default + the five lessons).

### Validation
- `auval` SUCCEEDED (AU); `pluginval --strictness-level 10` SUCCESS (VST3).
- Offline render harness: 7/7 — makes-sound, pitch, index→sidebands, carrier-null@2.405,
  feedback-stable, plus high-pitch and fixed-Hz **aliasing-budget** audits.

### Platforms
- macOS: VST3 + AU + Standalone. Windows VST3 cross-platform flags in place
  (WebView2 static-link + user-data-folder); Windows build not produced this cycle.
