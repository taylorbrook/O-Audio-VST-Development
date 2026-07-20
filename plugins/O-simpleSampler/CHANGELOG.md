# Changelog — O-simpleSampler

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.0] — 2026-06-26

First release. O-simpleSampler is the sampler sibling to O-simpleFM / O-simpleAdditive /
O-simpleGrain / O-simpleSubtractive — a deliberately simple, pedagogical keyboard
sampler that strips the software sampler down to its spine: **a recording, a region of
it, a root key, a loop, and an envelope**, each with a visible consequence on the
waveform. Built for the MUSC319 wk05 sampling session. Cross-platform WebView UI
(macOS AU + VST3, Windows VST3 via WebView2).

The 21-parameter APVTS contract is frozen for v1.0.

### Added — instrument (Stages 1–3)
- **Polyphonic keyboard sampler** (16-voice) — the active source read through a
  fractional-read varispeed head, region-isolated, anti-aliased, shaped by a per-voice
  amp ADSR + VCA + velocity sensitivity, tuned to the live **Root Key**.
- **Curated built-in source set** (FUNC-02): **piano** (root 48), **cello** (root 69),
  **pizz** (pizzicato strings, root 69), and **hit** (percussive one-shot, neutral root
  60). Selecting a source seeds its recorded-pitch root so it plays in tune. Roots were
  probed via YIN f0 estimation. Embedded as a second `juce_add_binary_data` target with
  a distinct `BinaryData` namespace (UI resources use `UIBinaryData`).
- **Load your own sound** (FUNC-03) — drag-and-drop (macOS WKWebView content-streaming
  bridge) **and** a file-picker fallback; WAV / AIFF / FLAC, resampled to the engine
  rate, capped at 30 s.
- **Region** — Start/End to isolate the useful region; **Loop** (Off / Forward /
  Ping-Pong) with equal-power crossfade + zero-cross marker snapping; **Reverse**.
- **Pitch** — **Repitch** (honest varispeed) ↔ **Stretch** (SOLA pitch/time
  independence), plus Root Key, semitone Tune, and fine cents.
- **Vintage** — SP-1200-style sample-rate decimation + bit-crush (clean at 0).
- **Filter** — resonant TPT low-pass with a live-matching display curve.
- **Interactive waveform editor** — draggable start/end + loop handles (two-way via
  relays), shaded loop band, live playhead, root-key indicator, filter curve, amp-ADSR
  scope. 34/34 controls tooltipped.
- **Concept preset tour** (FUNC-07) — 7 named factory presets, each isolating one
  sampler concept: Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell,
  Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped. Each resets to defaults,
  re-seeds the active source's recorded root (so nothing plays octave-flat), then sets
  only the parameters that isolate its concept; the WebView relays resync every control.

### Changed / hardened (Stage 4 — Polish)
- **RT-safe source-swap handoff.** The audio thread now reads the live source through a
  raw `std::atomic<AudioBuffer*>` (acquire) and never holds a `shared_ptr`, so a swap can
  never free a buffer in the render path. The previous buffer is held one generation
  (`retiredSource`) and freed off the audio thread; the owning shared_ptrs are guarded by
  a `CriticalSection` taken only off the audio thread (replaces the deprecated
  C++20 `std::atomic_load/store(shared_ptr)` helpers).
- **Deferred fresh-instance root seed.** The one-time per-source root seed no longer calls
  `setValueNotifyingHost` inside `prepareToPlay` (discouraged — prepare can run off the
  message thread / during scans); it is deferred to the existing AsyncUpdater
  (guaranteed message thread).
- **Render-harness re-armed.** Dropped `PluginEditor.cpp` from the harness target so the
  offline DSP correctness gate stays buildable now that the editor uses WebView types
  under `JUCE_WEB_BROWSER=0` (`createEditor()` falls back to `GenericAudioProcessorEditor`).

### Validation
- Offline render-harness: **9/9 PASS** (makes-sound, Repitch tuning, Stretch
  pitch-tracks-key, Stretch time-independence, loop-seam continuity fwd + ping-pong,
  region-end declick, Vintage clean-at-zero, anti-alias up-transpose, stress bound).
- `auval` SUCCEEDED — **21** Global Scope Parameters.
- pluginval strictness 5 — exit 0 on **both** VST3 and AU.
- Native-fn WebView bridge: 8 JS ≡ 8 editor ≡ 8 processor, 0 orphans.

### Notes
- **Windows (COMPAT-02):** WebView2 wiring is in place (`NEEDS_WEBVIEW2` +
  static-linking flag, dual `BinaryData`/`UIBinaryData` namespaces). Runtime
  verification on a Windows host/DAW is pending user testing — not a CI gate for v1.0.
