# Changelog — O-simpleAdditive

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.2] — 2026-06-25

Per-voice CPU optimization for continuous wavetable motion. No change to static
patches (bit-identical) and no change at common large host block sizes.

### Changed
- **Refill-cadence cap (PERF).** `AdditiveVoice::refillTable()` rebuilds the whole
  2048-point single-cycle table — a fixed ~2048×16 sine-sum cost *per call,
  independent of the host block size*. During continuous motion (scan/LFO/mod-env/
  spectral-decay) the table is marked dirty every block, so on small host blocks
  the rebuild fired far more often per second than on large ones and the cost did
  not amortize. Motion-driven refills are now bounded to a control-rate interval
  (~5 ms, resolved from the sample rate in `prepareToPlay`): a minimum number of
  samples must elapse between rebuilds.

  **Root cause / profile (offline render-harness, 16-voice Morph-Pad chord, dense
  Saw spectrum):** MOVING-regime CPU climbed from 4.7 % of a core at 512-sample
  blocks to 36.7 % at 64-sample blocks (perfect 2× per block-size halving) while a
  STATIC patch stayed flat at ~0.2–0.3 % — i.e. the per-block full-table rebuild
  was the hot cost and did not amortize across small blocks.

  **After:** 64-sample MOVING dropped 36.7 % → 9.0 % (4.1×), 128-sample 18.6 % →
  9.0 % (2.1×); 256/512-sample blocks unchanged (the cap is a no-op once the host
  block already exceeds the interval). The capped small-block cadence (~256 samples
  ≈ 5.8 ms) matches the cadence a 256-sample host already used, so the 20 ms scan
  smoother's zipper-free guarantee is preserved by construction.

- **No-regression guarantees (verified by the render-harness golden battery):**
  static patches (decay 0, LFO depth 0) keep the once-per-note refill and are
  **bit-identical** (`maxAbsDiff = 0`); moving patches rendered at host blocks ≥ the
  cap interval (512) are **also bit-identical**, proving the fill math itself is
  untouched — only the small-block refill *cadence* changed.

### Added
- **`tests/render-harness/`** — offline Stage-2 correctness gate + refill profile
  for this plugin (mirrors the O-simpleFM/O-simpleGrain harnesses). `--profile`
  measures STATIC-vs-MOVING per-voice CPU across host block sizes; `--dump-golden`/
  `--check-golden` capture and bit-compare the static + large-block-moving battery.
  Off by default (`-DOUARICON_BUILD_TESTS=ON`).

## [1.0.1] — 2026-06-25

Code-quality cleanup bundle. No change to the synth's audio output; one
display-accuracy refinement to the live drawbar glow.

### Fixed
- **Live drawbar glow now band-limited.** `refillTable()` published the
  *pre*-band-limit partial amplitudes into `activeSpectrum[]`, so on high notes
  partials above Nyquist (`k > Kmax`) lit the green live-glow even though they are
  never written into the table and make no sound. The snapshot is now taken
  *after* the `nyquistGain(k+1, Kmax)` band-limit, so the glow shows only what is
  actually sounding — matching the drawbar tooltip's promise (QUAL-02). Audio
  output (the wavetable itself) is unchanged.
- **`frameBSource` choice resolved robustly.** `pushParamsToVoices()` cast the raw
  parameter value with a truncating `(int)` cast; it now uses
  `jlimit(0, 3, (int) std::round(...))`, matching how the bit-depth choice is
  resolved. Same result for the four valid choice indices, but no longer relies on
  the float landing exactly on an integer.

### Changed
- **Single source of truth for the 16 partial IDs.** The `partialIds[16]` string
  array was duplicated four times (createParameterLayout, pushParamsToVoices,
  applyFactoryPreset, and the editor's `sliderIds`). Hoisted one
  `constexpr std::array` into `OSimpleAdditive::ParamIDs` (PluginProcessor.h) and
  referenced everywhere.
- **Single source of truth for lesson captions (WebView).** The lesson copy lived
  twice in `app.js` — once in `TIPS.lesson*` and again in `LESSONS`. `LESSONS` is
  now canonical; the per-button hover tooltips are derived from it. (Hover-tooltip
  wording for the six lesson buttons changes slightly as a result; tour-caption
  text is unchanged.)
- `-Wfloat-equal` hygiene: the `band[k] != 0.0f` silent-partial skip in
  `refillTable()` now uses `juce::exactlyEqual`, matching the rest of the codebase.
- Removed stale stage-process comments (the "lifted verbatim from FmVizAnalyzer",
  "not yet constructed", and "Stage 2 (complete)" framing in the file headers).

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
