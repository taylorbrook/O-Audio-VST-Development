# Stage 2 (DSP) — VERIFICATION

**Plugin:** O-simpleGrain · **Stage:** 2 of 4 (DSP) · **Phase:** verify · **Date:** 2026-06-24
**Verdict:** ✅ **PASS** — the granular audio engine specified in ARCHITECTURE.md is implemented, builds clean across all three formats, passes auval, and passes an 8-check offline DSP correctness harness. Stage goal achieved.

---

## Stage goal (from PLAN.md)

> Fill the silent `processBlock` with the full granular engine: an 8-voice
> `juce::Synthesiser` of `GrainVoice`s, each owning a preallocated bounded grain
> pool scheduled at the density-derived period, reading windowed grains from a
> global-playhead read head over an embedded/loaded source, overlap-added,
> anti-aliased on up-transposition, shaped by amp ADSR, with scan/freeze, per-grain
> spray/scatter, source loading (embed 4 + drag-drop + hot-swap), and the three
> lock-free audio→UI taps. Real-time-safe, zero added latency.

**Achieved.** All three sub-phases (2.1 core engine, 2.2 read head + spray + AA,
2.3 loading + viz taps) implemented, each an independently green build + commit.

---

## Automated evidence

### Build (all three formats, every sub-phase)
`ninja O-simpleGrain_VST3 O-simpleGrain_AU O-simpleGrain_Standalone` → clean link,
no errors or O-simpleGrain warnings (only pre-existing JUCE-internal `wstring_convert`
/ NSEvent-switch notes). Bundle grew 4.7M → 6.1M after embedding the 4 built-in WAVs.

### auval (AU registration + render validation)
`auval -v aumu OsGr OuDv` → **AU VALIDATION SUCCEEDED** at every sub-phase. Render
tests pass at 22050/44100/48000/96000/192000 Hz × frame sizes 64–4096; 1-channel;
bad-max-frames; parameter set/ramp; **Test MIDI PASS**. No NaN/crash across rates.

### Offline DSP correctness harness (`tests/render-harness/`, `-DOUARICON_BUILD_TESTS=ON`)
A standalone console app instantiates `OSimpleGrainAudioProcessor`, drives MIDI,
renders offline, and asserts the granular acceptance criteria — converting the
ROADMAP "manual-listen" checks into automated gates. **8/8 PASS:**

| Check | Maps to | Result |
|-------|---------|--------|
| `makes-sound` | engine renders | rms=0.013, peakGrains=7, finite |
| `density->continuity` | FUNC-01 / DSP-02 | envelope continuity 0.16 (sparse) → 0.93 (dense) |
| `pitch-tracks-MIDI` | FUNC-02 | C2/C3/C4 f0 = 65.6 / 130.9 / 260.9 Hz — each within **<1%** of `130.81·2^((N−60)/12)` (perfect octaves; root C3 = source at recorded pitch) |
| `window-rect-clicks` | DSP-03 | rectangular window injects **905×** more top-octave energy than Hann |
| `freeze-sustains` | FUNC-03 / QUAL-01 | freeze pinned → finite, bounded, sustaining pad (rms=0.015) |
| `scatter-async-flatter` | DSP-05 | spectral flatness 0.003 (sync/peaky) → 0.39 (async/noisy) |
| `stress-bounded` | PERF-01/02 | 5-note chord, max density×size×spray×scatter → finite, peak 0.40, **159 ≤ 192** grain cap |
| `uptranspose-stable` | DSP-08 (stability half) | grainPitch +24 st + high pitch spray → finite, bounded |

The pitch check was hardened through several metric iterations (it initially
mis-measured the *grain-rate comb* rather than the source pitch); a single-grain
autocorrelation probe gave the definitive, comb-free result above and confirmed
key-tracked resampling is exactly correct — a point where a real transposition bug
could not otherwise be distinguished from a probe artifact.

### Real-time safety (code + pluginval-class checks)
`processBlock`/`renderNextBlock` allocation/lock/IO-free: preallocated
`std::array<Grain,24>`/voice + steal-oldest (never resized), window LUTs built at
construction, source snapshot via lock-free atomic `shared_ptr` held for the block,
`ScopedNoDenormals` + block-level `isfinite` scrub, per-voice `juce::Random`, all
decode/resample off-thread. `setLatencySamples(0)` retained; `getLatencySamples()`
never overridden. The stress check's bounded 159-grain ceiling demonstrates the
pool + steal-oldest prevent xrun.

---

## Success criteria (ROADMAP) — disposition

**Automated (this verify):** build VST3+AU+Standalone ✓ · AU registers/validates ✓ ·
allocation-free / no-xrun under max load ✓ · `setLatencySamples(0)` ✓ ·
density→continuity ✓ · MIDI transpose (FUNC-02) ✓ · rectangular window clicks (DSP-03) ✓ ·
freeze sustains click-bounded (FUNC-03/QUAL-01) ✓ · scatter sync↔async (DSP-05) ✓ ·
up-transposition stability (DSP-08) ✓ · viz taps lock-free / FFT off audio thread ✓.

**Manual-listen (DAW pass — recommended before release, not blocking Stage 3):** the
*subjective* grain character — separated→fused grains by ear, few-ms buzz vs
tens-of-ms fragments (DSP-01), the *audible* freeze click-freeness, the *audible*
"no buzz" cleanliness of high pitch-spray grains (DSP-08), drag-drop + picker loading
a user `.wav`, and glitch-free source swap while holding a note. The decode/transpose/
freeze/scatter mechanisms underlying all of these are automated-verified above; only
the subjective listen remains.

**Stage-3-dependent:** the four visual renderers + the drag-drop/picker/source-select
UI consume the taps + handlers laid here (`getVizRing()`, `getGrainCloudBuffer()`,
`getActiveGrainCount()`, the `dropSession*`/`loadSourceFromFileChooser` bridge).

---

## Risks carried into later stages (none blocking)

- **Built-in sources are mono** (the read is mono per ARCHITECTURE's lesson framing);
  a stereo grain read is a documented v1.x refinement.
- **`std::atomic_load/store` on `shared_ptr`** are C++20-deprecated (compile clean;
  established O-MicrotonalSampler pattern). Consider `std::atomic<std::shared_ptr>` in a
  future cleanup.
- **Drag-drop is C++-only** this stage; the WebView JS + WebBrowserComponent that call
  the registered native functions are wired in Stage 3.1 (handler names are fixed by the
  shared module — do not rename).
- **Default patch loudness:** `fire` at the default position can be quiet (sparse
  crackle) — a preset-tuning note for Stage 4 (the preset tour), not a DSP defect.

**Verdict: Stage 2 (DSP) goal achieved. Ready for Stage 3 (GUI).**
