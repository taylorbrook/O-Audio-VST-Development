# Changelog — O-simpleSubtractive

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.0] — 2026-06-25

First release. A teaching subtractive synth for the Ouaricon pedagogical suite:
the canonical **oscillator → filter → VCA** voice with two independent ADSR
envelopes, built around the O-simpleFM / O-simpleAdditive north star — a tight
loop between gesture and *visible* consequence. Lower the cutoff and watch the
upper harmonics fall away under the live filter curve; push resonance to
self-oscillation and hear the filter whistle on its own.

### Synth engine
- **Source:** one waveform-selectable oscillator — Saw / Square / Triangle / Sine
  — band-limited (PolyBLEP saw & square, polyBLAMP triangle, LUT sine) so it never
  buzzes at high notes. Plus a sub-oscillator (square, −1 octave) and a white-noise
  source, each with its own level.
- **Filter:** a zero-delay-feedback state-variable filter — **LP / HP / BP / Notch**
  at **6 / 12 / 24 dB/oct** — with cutoff, resonance, bipolar filter-envelope amount
  (octaves) and cutoff key-tracking. Pushed to max resonance it **self-oscillates**
  into a clean, bounded sine that plays in tune with key-track (soft-knee limiter +
  resonance-dependent make-up). Zero added latency (no oversampling).
- **Envelopes:** two independent `juce::ADSR` envelopes — one sweeps the cutoff, one
  shapes the VCA and voice lifetime — so brightness and loudness move separately.
- **Voicing:** 16-voice **Poly**, plus **Mono** and **Legato** modes with **glide**
  (portamento) — the same voice serves the classic monosynth and the polysynth.

### Live teaching UI (JUCE 8 WebView)
- Headline **filter-response-over-spectrum** visual: the closed-form filter magnitude
  curve overlaid on the live output spectrum on one shared log-frequency axis — you
  watch the curve subtract harmonics in real time (matches the audio filter to 0.00 dB
  by construction).
- Output **scope**, a **dual-ADSR** display with live envelope markers, an
  **oscillator → filter → amplifier** signal-path diagram that highlights the active
  stage, **30 hover tooltips**, and an on-screen QWERTY/click keyboard.

### Concept-preset tour (FUNC-06)
- Eight named factory patches, each isolating ONE idea, loadable from the UI:
  **Saw Sweep**, **Pluck**, **Brass Stab**, **Sweep Pad**, **Acid Bass** (303),
  **Square Bass** (hollow), **Noise Wind** (filtered noise), **Self-Oscillation**
  (the in-tune whistle). Selecting a preset writes the whole APVTS through the host
  API, so every on-screen control and visual snaps to the new patch automatically.
- Playable enough (FUNC-07) to double as a simple subtractive instrument — bass,
  lead, pluck and pad starting points are one click away.

### Platform / packaging
- VST3 + AU (macOS); Windows WebView2 flags set (`NEEDS_WEBVIEW2 TRUE` +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`).
- Full APVTS state persistence (20 parameters round-trip).

### Validation
- **auval: AU VALIDATION SUCCEEDED.**
- **pluginval strictness-level 10: ALL TESTS PASSED (SUCCESS).**
- Stage-2 offline render-harness gate green (band-limiting / self-oscillation-in-tune /
  closed-form-curve-vs-measured-response).
