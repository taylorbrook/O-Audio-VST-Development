# Changelog — O-simpleAdditive

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.0] — 2026-06-22

First complete release. A pedagogical 16-partial **additive** synth with a light
**wavetable** dimension — the additive sibling to O-simpleFM. The 16 drawbars *are* the
spectrum: you build a tone harmonic-by-harmonic and watch it on the live display.

### Synthesis engine (Stage 2)
- **16-partial additive voice**, 16-voice polyphonic (`AdditiveVoice : juce::SynthesiserVoice`).
  Each note is summed into a band-limited single-cycle wavetable (2048-pt) and read by phase —
  not a per-sample sum-of-sines — so CPU stays flat and there is **zero added latency**
  (`setLatencySamples(0)`).
- **Exact per-note anti-aliasing** — partials above Nyquist (`k > Kmax = floor(0.5·fs/f0)`) are
  never written; a raised-cosine taper on the top 2 surviving harmonics avoids boundary clicks.
  No oversampling. High notes with all drawbars up stay clean.
- **Wavetable scan/morph** — per-partial linear *spectral* morph from Frame A (the drawbars)
  toward a Frame B preset (Sine / Saw / Square / Odd), driven by a manual knob, a global sine
  LFO, and a mod-envelope. Zipper-free via a 20 ms smoother + control-rate table refill.
- **Spectral-decay macro** — per-partial exponential tilt over the note (`D_k = exp(−rate·k·τ)`),
  with optional velocity routing (`velToDecay`).
- **Bit-depth quantizer** — discrete `{Off, 12, 10, 8, 6, 4, 2}` mid-tread crusher for lo-fi grit.
- **Dual ADSR** — amp envelope (voice lifetime) + an independent mod-envelope routed to scan.
- Headroom-normalized table sum (÷ max(1, Σ amplitudes)) so 16 maxed drawbars don't clip.
- `ScopedNoDenormals` + block-level `isfinite` scrub; floor-modulo phase wrap.

### Interface (Stage 3) — "Additive Field Guide" WebView
- Single-page classroom/projector-readable layout (sibling aesthetic to O-simpleFM).
- **16 drawbars double as the live spectrum** — brass set-level + green live-glow showing the
  exact *morphed + decayed* active-spectrum snapshot (not an FFT estimate).
- Live **oscilloscope** of the summed waveform (30 Hz message-thread Timer over a lock-free
  `VizRing`; analyzer copies the window before its in-place FFT — no corruption, no audio-thread
  allocation).
- 33 parameters two-way bound (31 `WebSliderRelay` + 2 `WebComboBoxRelay`).
- Plain-language **hover tooltips on every control** (overtone series, why odd-only is hollow,
  what scan/morph/bit-depth do).
- **6 lesson presets** — Pure Sine, Sawtooth, Square (hollow), Organ, Morph Pad, Lo-Fi Bells —
  each isolates one concept via a full APVTS snapshot.
- On-screen keyboard MIDI (`MidiMessageCollector` + drain into the synth).
- Cross-platform WebView wired: `NEEDS_WEBVIEW2` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING` +
  `withUserDataFolder` for Windows; custom-scheme resource provider for macOS.

### Validation (Stage 4)
- **pluginval strictness 8 — VST3: SUCCESS, AU: SUCCESS** (render, automation, parameter fuzz,
  state restore, threading, bus layouts).
- **`auval -v aumu OSiA OuDv` — AU VALIDATION SUCCEEDED** (incl. Test MIDI render).
- Factory-preset sweep: all 6 lessons apply finite, in-range snapshots (reset-to-default first).
- Aliasing audit: band-limit + taper + headroom + finite-phase guard confirmed exact; fuzz/render
  at the top of the keyboard surfaced no NaN/Inf/denormal.
- Default patch (H1=100%, rest 0) is a pure sine — unregressed from the Stage 2 DSP.

### Deferred to v1.1
- Persistent user preset save/load bar (`OuariconPresetManager`).
- Per-partial mod-env decay routing.
