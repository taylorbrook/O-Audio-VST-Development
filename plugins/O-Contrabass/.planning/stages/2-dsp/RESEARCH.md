# Stage 2: DSP — Research (Phase 2.1)

**Date:** 2026-04-26
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP) — Phase 2.1 cycle (gate-first)
**Phase:** research
**Cycle Scope:** Phase 2.1 only (Phases 2.2–2.6 each get a fresh research pass)

---

## Purpose

This research pass does NOT re-litigate the locked Stage 0 contracts (`parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md`). Its job is to:

1. Pin down the exact extraction surface from O-Bowed and the bass-tuning delta (so the plan phase can list literal file moves and edits).
2. Resolve the 5 open questions handed in by `CONTEXT.md`.
3. Pattern-confirm the new module against existing shared modules.
4. Verify the JUCE 8.0.4 APIs the architecture relies on (`Oversampling`, `DelayLine<float, Lagrange3rd>`).
5. Surface risks and pitfalls the plan / execute phases must defend against.

All findings are sourced from real code in the repository and the local JUCE tree at `/Users/taylorbrook/JUCE` (8.0.4).

---

## 1. O-Bowed Extraction Surface — what we copy, what we adapt

### 1.1 Source-of-truth files in O-Bowed

| O-Bowed file | LOC | Phase 2.1 status | Notes |
|---|---|---|---|
| `Source/DSP/HyperbolicFriction.h` | 56 | **inline copy → module in 2.1b** | Header-only, no `.cpp`. Stateless except `mu_s/mu_d/v_0/R_s` |
| `Source/DSP/BowModel.{h,cpp}` | 52 + 98 | **inline copy → module in 2.1b** | One-pole envelope; `BowState` per CONTEXT |
| `Source/DSP/WaveguideString.{h,cpp}` | 79 + 240 | **bass-adapted reimplementation** (not verbatim) | Critical structural deltas — see §1.3 |
| `Source/BowedMPESynthesiser.h` | 53 | **pattern reference for `OContrabassMPESynthesiser`** | CC11 dispatch + voice loop |
| `Source/BowedStringVoice.{h,cpp}` | 142 + ~1000 | **pattern reference for `BowedContrabassVoice`** | Per-voice 2× oversampler, NoteExpression hookup, TuningEngine pointer |

Spike-validated reference (per memory file): O-Lyrica `BowedStringVoice` is the canonical Note-Expression-aware voice pattern; both consume `modules/tuning/note-expression`. We inherit that wiring at Phase 2.6 — Phase 2.1 does NOT touch tuning.

### 1.2 `HyperbolicFriction.h` — bass-tuning delta (verbatim port + 3 number changes)

Current O-Bowed defaults vs. `ARCHITECTURE.md §"Hyperbolic Friction Curve"` and `CONTEXT.md §"Cycle Scope 2.1a"` bass tuning:

| Field | O-Bowed default | O-Contrabass default | Source |
|---|---|---|---|
| `mu_s` | 0.8 | **0.85** | CONTEXT.md §2.1a |
| `mu_d` | 0.3 | **0.25** | CONTEXT.md §2.1a |
| `v_0`  | 0.05 | **0.05** | CONTEXT.md §2.1a (unchanged baseline; `setRosin()` modulates) |
| `R_s`  | 0.5  | 0.5 | unchanged |

`setRosin(rosinParam)` formula (`v_0 = 0.1 * exp(-4.6 * rosinParam)`) is preserved verbatim. The bass default of `ROSIN = 0.65` (per `parameter-spec.md`) gives `v_0 ≈ 0.005`, which is consistent with thick rosined bass hair.

**Plan-phase action:** Phase 2.1a copies the file with three numeric edits to the member initialisers; Phase 2.1b promotes the file (already-edited) to the new module under the bass-suitable defaults — O-Bowed will keep its existing defaults via either a constructor parameter or `setDefaults()` method (TBD in plan).

### 1.3 `WaveguideString` — structural deltas (NOT a verbatim port)

The CONTEXT.md says "port verbatim with bass-tuned defaults", but the architecture demands several structural changes from O-Bowed's existing waveguide. The plan phase MUST treat `WaveguideString.cpp` as a reference, not a copy. Concrete deltas:

| Aspect | O-Bowed (current) | O-Contrabass Phase 2.1a (per ARCHITECTURE.md) | Why |
|---|---|---|---|
| Interpolation | `Thiran` (1st-order IIR allpass) | **`Lagrange3rd`** | Thiran is stateful → clicks under detune/vibrato modulation. Architecture §"String Waveguide Bank" + research §3.3 explicit. |
| Topology | **Split delay** (bridge + neck rails, junction at bow contact) | **Single delay line, 8192 samples** (junction implicit; bow position ≠ split point in v1.0) | CONTEXT.md §2.1a explicit. Phase 2.1 does not yet model `BOW_POSITION` as a split point — it parameterises the friction junction via `beta`-derived impedance only. The split-rail model becomes meaningful when Phase 2.5 adds the body bank and `BOW_POSITION` starts shaping perceived timbre. |
| In-loop saturator | `4 · tanh(x/4)` | **algebraic `x / sqrt(1 + x²)`** | ARCHITECTURE.md §"String Waveguide Bank" — asymmetric onset at 0.6, asymptote at 1.0, prevents loop-gain runaway. tanh is fine but algebraic is cheaper and matches the architecture spec. |
| DC blocker | not present | **`H(z) = (1−z⁻¹)/(1−R·z⁻¹)`, R=0.999, in-loop after bridge filter** | ARCHITECTURE.md §"DC Blocker". O-Bowed gets away without one because it doesn't run drone mode at the same loop gains; O-Contrabass needs it. |
| Denormal leak | `if (\|out\| < 1e-15) out = 0` (ad-hoc flush at end) | **constant `−1e-20` added to bridge filter `y`** outside drone (`INFINITE_SUSTAIN < 0.95`) | Architecture §"Bridge Filter" + research §3.2. The architecture's choice is more robust because it stops subnormals from forming in the first place rather than flushing them after the fact. |
| Bridge LP `g` mapping | `g = 0.990 + 0.0095·sustain`, range 0.990→0.9995 | **0.997 → 0.99995, hard ceiling 0.9999999, quadratic skew** (`g = 0.997 + 0.00295·x²`) | ARCHITECTURE.md §"Bridge Filter (One-Pole Lowpass + Loop Gain)" + Open Question #5. Bass needs much higher loop gain ceiling for true drone, with the steep regime concentrated in the upper half of the knob. See §4 for skew choice. |
| Bridge LP `p` clamp | not clamped | **`p` clamped to [0.05, 0.95]** | Architecture §"Bridge Filter" — ensures HF damping is never zero. |
| Oversampling | per-voice 2× `filterHalfBandPolyphaseIIR` (already in O-Bowed) | **same** | No change. We adopt O-Bowed's pattern verbatim. |

**Decision:** the plan phase will list `BowedContrabassVoice.{h,cpp}` and `WaveguideString.h` as **new files modelled on O-Bowed**, not copies. They will reference O-Bowed by line number for the parts that ARE verbatim (the friction call, the Helmholtz mode-locking via `clampedRho` calculation, the `JunctionState` split-read pattern), and adapt the rest.

This is a slight scope expansion vs. CONTEXT.md's "port verbatim", but the architecture explicitly mandates the structural changes — there is no version of "verbatim" that satisfies the locked architecture. Flag as deviation in PLAN.md preamble.

### 1.4 `BowModel` — verbatim port

`BowModel.{h,cpp}` ports almost exactly. The only adaptations:

- Default `bowSpeedParam = 0.15` (bass) vs. O-Bowed's `0.2` (more general). Sourced from `parameter-spec.md` BOW_SPEED default.
- Default `bowPressureParam = 1.0` (bass) vs. O-Bowed's `0.5`. Sourced from `parameter-spec.md` BOW_PRESSURE default.

Both deltas are init-list edits. The envelope coefficients, attack/release semantics, retrigger reset behaviour, and `setBowSpeed()`/`setBowPressure()` ratio-preserving updates all carry over unchanged.

In Phase 2.1b extraction, `BowModel` ships unchanged in the module — it is bass-defaults-agnostic (defaults are init values, not behaviour). The "BowState envelope" reference in CONTEXT.md is `BowModel`'s internal smoothed-target envelope; no rename required for the extraction.

### 1.5 `SchellengGuard` — does not yet exist

CONTEXT.md §"Module extraction" lists `SchellengGuard` as one of the three classes to move into the module in Phase 2.1b. Survey of O-Bowed `Source/DSP/`: there is no file named `SchellengGuard.h` or class `SchellengGuard`. The Schelleng wedge logic in O-Bowed's `BowedStringVoice.cpp` is inlined within `renderNextBlock`.

**Implications:**
- Phase 2.1a does NOT need `SchellengGuard` (Schelleng clamping enters in Phase 2.3 with the slow-bow LFO depth-clamp; ARCHITECTURE.md §"Slow-Bow LFO" shows the clamp is wired around the LFO, not the standalone friction junction).
- Phase 2.1b SHOULD NOT extract `SchellengGuard` — it doesn't exist yet. The module should ship with `HyperbolicBowTable` + `BowState` (i.e. `HyperbolicFriction.h` + `BowModel.{h,cpp}`) only.
- Defer `SchellengGuard` extraction to Phase 2.3 of O-Contrabass, where the class will first be authored. At that point, the module gets a v1.1.0 entry adding the new file, and O-Bowed adopts it via a follow-up sweep.

This is a meaningful refinement of CONTEXT.md §"2.1b — Module extraction": the module surface is smaller than initially listed. PLAN.md will document this as a scope correction.

---

## 2. Existing Module Pattern Confirmation

### 2.1 Layout we will adopt for `bow-friction`

Surveyed two existing modules in different categories:

**`modules/tuning/note-expression/`** (the closest analog — header-only, single-domain library):
```
note-expression/
├── module.yaml          # name/version/description/category/provides/dependencies/requirements/sources/used_by/changelog
├── module.cmake         # JUCE-patch-marker verifier (custom hook called by ouaricon_add_module)
├── README.md            # public usage doc
└── cpp/
    └── NoteExpression.h # header-only public API in namespace Ouaricon::NoteExpression
```

**`modules/tuning/scala-tuning-engine/`** (multi-file C++ + JS):
```
scala-tuning-engine/
├── module.yaml
├── README.md
├── cpp/                 # TuningEngine.{h,cpp}, ScaleGenerator.{h,cpp}, EmbeddedTunings.{h,cpp}, TuningExporter.{h,cpp}
├── js/                  # tuning-panel.js
└── snippets/            # tuning-panel.css
```

**Recommended layout for `modules/synthesis/bow-friction/` (Phase 2.1b):**
```
modules/synthesis/bow-friction/
├── module.yaml          # name=bow-friction, category=synthesis, version=1.0.0
├── README.md            # short — friction model + bow envelope; usage example
└── cpp/
    ├── HyperbolicFriction.h   # ported from O-Bowed; default-template the bass vs treble defaults
    ├── BowModel.h             # ported from O-Bowed
    └── BowModel.cpp           # ported from O-Bowed
```

No `module.cmake` hook needed — the module has no JUCE patch dependency, no per-format routing, no JS. `ouaricon_add_module(<plugin> bow-friction)` will sweep `cpp/*.{h,cpp}` into SharedCode automatically (per `OuariconModules.cmake` line 57–67) and add `cpp/` as a PRIVATE include path.

### 2.2 CMake integration pattern

For both consumers, the call site in their `CMakeLists.txt` becomes:

```cmake
# Phase 2.1b — new shared module
ouaricon_add_module(O-Contrabass bow-friction)
ouaricon_add_module(O-Bowed     bow-friction)
```

This is the **Pattern A** (`ouaricon_add_module`) currently used by O-Bowed and O-Contrabass for `note-expression`. The function is in `modules/cmake/OuariconModules.cmake`; it is already `include()`d in both plugins' `CMakeLists.txt`, so there is zero CMake-system change required to add the module.

Removal of the inline copies from each plugin's `target_sources` and include path is the actual diff to track.

### 2.3 Registry entry for `bow-friction`

Append to `modules/registry.yaml` under the `synthesis` block (currently empty — first synthesis-category module):

```yaml
  # ============================================================================
  # SYNTHESIS MODULES
  # ============================================================================

  - name: bow-friction
    path: synthesis/bow-friction
    version: 1.0.0
    description: |
      STK-style memoryless hyperbolic bow-string friction model + click-free
      bow excitation envelope. Header-only (HyperbolicFriction) plus a small
      one-pole envelope class (BowModel) covering attack/release/retrigger
      semantics. Stateless friction curve — O(1) per sample, always stable.
      Defaults parameterised per-instrument (bass vs general).
    category: synthesis
    provides:
      - cpp-class: HyperbolicFriction
      - cpp-class: BowModel
    dependencies: []
    tags: [synthesis, friction, bow, waveguide, physical-modeling]
    reuse_score: 9
    used_by:
      - plugin: O-Bowed
        version: 1.3.x
      - plugin: O-Contrabass
        version: 1.0.0
```

`reuse_score: 9` because two consumers and bass/treble/cello use cases all map to the same surface; not 10 because the module is narrowly scoped to bowed-string PM and won't show up outside that family.

### 2.4 Why `synthesis/` is the right home (not `dsp/`)

`modules/registry.yaml` schema (lines 16–39) defines exactly 8 categories: core, persistence, metering, tuning, modulation, synthesis, effects, ui. **There is no `dsp` category.** ROADMAP §2 said "or appropriate path per registry"; CONTEXT.md `synthesis/` decision stands.

`synthesis` is described as "Sound generation (oscillators, modal resonators)" — friction-junction excitation generates sound (not modulation, not an effect, not a UI element), so the fit is correct semantically. `OuariconModules.cmake` line 34 lists `synthesis` in the search order, so `ouaricon_add_module()` will find it without any code change.

---

## 3. JUCE 8.0.4 API Confirmation

### 3.1 `juce::dsp::Oversampling<float>`

Confirmed against `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_Oversampling.h` (8.0.4 head):

```cpp
// Constructor signature (line 98–102):
Oversampling (size_t numChannels,
              size_t factor,                                         // 2^factor → 2× when factor=1
              FilterType type,
              bool isMaxQuality = true,
              bool useIntegerLatency = false);

// FilterType enum (line 66–71):
enum FilterType
{
    filterHalfBandFIREquiripple = 0,
    filterHalfBandPolyphaseIIR,    // ← O-Bowed uses this; we adopt it
    numFilterTypes
};

// Lifecycle:
void initProcessing (size_t maximumNumberOfSamplesBeforeOversampling);  // call from prepareToPlay
void reset() noexcept;                                                  // clear internal state
SampleType getLatencyInSamples() const noexcept;                        // returns float — fractional!

// Per-block:
AudioBlock<SampleType> processSamplesUp (const AudioBlock<const SampleType>& inputBlock) noexcept;
void processSamplesDown (AudioBlock<SampleType>& outputBlock) noexcept;
```

**Confirmed concrete construction (matches CONTEXT.md):**
```cpp
juce::dsp::Oversampling<float> oversampling {
    /*numChannels*/ 1,
    /*factor*/      1,                                                  // 2× = 2^1
    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
    /*isMaxQuality*/ true                                               // useIntegerLatency defaults false
};
```

This is **exactly** the pattern O-Bowed uses (`BowedStringVoice.h:95`). Phase 2.1a copies it.

**Latency reporting:** `getLatencyInSamples()` returns a `float`. Architecture §"Latency" mandates `setLatencySamples(static_cast<int>(std::ceil(oversampler.getLatencyInSamples())))` in `prepareToPlay`. The ceil is required because `setLatencySamples()` takes `int`. Verified on `Oversampling.h:123`.

**Voice-level vs processor-level:** O-Bowed's pattern (per-voice) is what we adopt. For a mono synth (CONTEXT.md §"MIDI trigger") this is moot from a CPU perspective — there is exactly one active voice. The pattern matters because (a) it matches O-Bowed's structure 1:1 (less cognitive overhead, easier code review), (b) keeps the door open for future multi-voice modes (e.g. ringing-after-release), and (c) `BowedStringVoice` already exposes `getOversamplingLatency()` as a method the processor reads in `prepareToPlay` to forward to `setLatencySamples()`. Adopt the same expose-via-method pattern.

### 3.2 `juce::dsp::DelayLine<float, Lagrange3rd>`

Confirmed against `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_DelayLine.h`:

```cpp
namespace DelayLineInterpolationTypes {
    struct Lagrange3rd {};   // line 72, doc on line 64–71:
    // "interpolated using a 3rd order Lagrange interpolator. This method incurs
    //  more computational overhead than linear interpolation but reduces the
    //  low-pass filtering effect whilst remaining amenable to real time delay
    //  modulation."
}

// Class API (relevant subset):
template <typename SampleType, typename InterpolationType = DelayLineInterpolationTypes::Linear>
class DelayLine
{
    explicit DelayLine (int maximumDelayInSamples);
    void prepare (const ProcessSpec& spec);
    void setDelay (SampleType newDelayInSamples);                 // OK from audio thread
    SampleType getDelay() const;
    void setMaximumDelayInSamples (int maxDelayInSamples);        // ALLOCATES — never call from audio thread
    void reset();
    void pushSample (int channel, SampleType sample);
    SampleType popSample (int channel,
                          SampleType delayInSamples = -1,         // -1 = use current setDelay value
                          bool updateReadPointer = true);
};
```

**Sub-sample modulation behaviour (Lagrange3rd):** the JUCE doc explicitly says it "reduces the low-pass filtering effect whilst remaining amenable to real time delay modulation". This matches the architecture's claim that "Lagrange3rd absorbs continuous changes cleanly" — confirmed.

**Allocation contract:** `setMaximumDelayInSamples()` allocates internally and **must not** be called from `processBlock`. We size to 8192 once in `prepareToPlay()` (Phase 2.1a; ARCHITECTURE.md §"String Waveguide Bank" — 8192 samples covers E1 −1200 cents at 88.2 kHz).

**Construction matching the architecture:**
```cpp
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 8192 };

// in prepareToPlay (post-oversampling-init):
juce::dsp::ProcessSpec spec { sampleRate * 2.0, blockSize * 2u, 1u };  // internal SR is 2×
delayLine.prepare(spec);
delayLine.setMaximumDelayInSamples(8192);                              // idempotent if matches ctor
```

**setDelay precision:** Lagrange3rd is stateless (FIR); `setDelay()` mid-stream produces no transient. Per-sample `setDelay()` calls are the documented pattern for vibrato/detune ramps (research §5; we don't need this in Phase 2.1a — it lands in Phase 2.2 with per-string detune and Phase 2.3 with vibrato).

### 3.3 `juce::ScopedNoDenormals` and other host requirements

No surprises. Already used in Stage 1 placeholder `processBlock`. Phase 2.1a keeps it.

---

## 4. Resolution of the 5 Open Questions

### Q1. Headless render harness selection

**Recommendation: thin custom CTest binary built on JUCE Standalone runner (`AudioProcessor` directly), no external deps.**

**Why:**
- We need 60-second WAV renders that drive a `juce::MidiBuffer` with a programmatically generated note-on at t=0 and note-off at t=59s, with a fixed APVTS preset, and inspect the output for: (a) no NaN, (b) `RMS(block)` not monotonically rising past a ceiling (runaway detection), (c) block-time CPU not exceeding 5× nominal (denormal-spike detection), (d) peak ≤ 0 dBFS.
- `juce::AudioProcessorPlayer` is heavier than needed (it bridges to `AudioDeviceManager`, which we don't want for offline). The right pattern is to instantiate the `OContrabassAudioProcessor` directly, call `prepareToPlay`, push synthetic MIDI through a `MidiBuffer`, accumulate output into `juce::AudioBuffer<float>`, write to WAV via `juce::WavAudioFormat::createWriterFor`. This is the canonical JUCE offline-render pattern.
- pluginval is for plugin-host correctness, not stability invariants. It will not fail on a 60s sustain test even if the engine drifts to NaN partway through.
- An external tool would couple us to non-JUCE code in the test path — unnecessary.

**Concrete shape (for PLAN.md):**
- New target `O-Contrabass-render-test` in `tests/O-Contrabass/render-harness/CMakeLists.txt`. Standalone executable, links the plugin's static library only (no host).
- One-file C++ entrypoint: `int main(int argc, char**)` reads CLI args (preset name, duration, output WAV path), constructs the processor, runs the loop, writes WAV + JSON summary.
- Exit code 0 = pass, non-zero = fail invariant. CTest picks this up; we get a unit-test-style PASS/FAIL line.
- Reusable for Phase 2.4's 108-combo matrix: a wrapper script iterates the cross-product, runs the harness 108×, aggregates JSON summaries.

**Out of scope for Phase 2.1:** we don't ship the matrix runner now — only the single-test harness that will be re-driven by Phase 2.4's runner script. CONTEXT.md §"Stability test harness" already approved this scaffolding-now approach.

**Plan phase delivers:** `tests/O-Contrabass/render-harness/{CMakeLists.txt,main.cpp}` + a CTest entry that runs one 60s E1 + max-INFINITE_SUSTAIN test.

### Q2. O-Bowed regression coverage bar

**Recommendation: bit-exact WAV diff for the friction-junction-only render path, plus a 1-preset A/B audition before module extraction is committed.**

**Why bit-exact, not spectral-feature:**
- The extraction is intended to be behaviour-preserving. Anything other than bit-exact says "we changed the math somewhere unintentionally."
- Friction-junction code is deterministic given a fixed input MIDI sequence and parameter set (no RNG in `HyperbolicFriction` or `BowModel`).
- O-Bowed has other randomness (`HumanizeEngine`, `BowNoiseGenerator`); the regression render must disable these (`HUMANIZE_*` and `BOW_NOISE` knobs at zero in the regression preset).

**Concrete bar (for PLAN.md):**
- **Pre-extraction (start of 2.1b):** check out O-Bowed at HEAD, render a 30-second WAV using a "regression preset" with HUMANIZE = 0, BOW_NOISE = 0, fixed seed, neutral expression. Store the WAV + sha256 in `tests/O-Bowed-regression/baseline.wav` (committed).
- **Post-extraction (mid-2.1b):** re-render the same 30s sequence against the now-module-consuming O-Bowed. Compare WAVs byte-for-byte. Bit-mismatch ⇒ extraction-introduced change ⇒ block.
- **Sonic A/B:** load both WAVs in DAW (Logic), null-test (sample-accurate phase invert + sum). Audible residue ⇒ block (catches anything the bit compare missed if floats happened to differ by sign-bit-only).

**Cost:** 1 day, matches CONTEXT.md §"Cycle Scope 2.1b" estimate.

**Fallback:** if bit-exact fails on any host-toolchain combination (e.g. linker re-orders init), accept ULP-level diff (max abs sample error < `1e-7` and RMS error < `−120 dBFS` against baseline). Document that bar in the harness output.

### Q3. MPESynthesiser vs. Synthesiser at this phase

**Recommendation: use `MPESynthesiser` from day one (Phase 2.1a).**

**Why:**
- O-Bowed and O-Lyrica both use `juce::MPESynthesiser`. CC11/MPE expression dispatch already lives in `BowedMPESynthesiser.h` (53 LOC) — directly cloneable for `OContrabassMPESynthesiser`.
- Migrating from `Synthesiser` to `MPESynthesiser` later would require rewriting the voice base class (`MPESynthesiserVoice` callbacks: `noteStarted`, `noteStopped(allowTailOff)`, `notePitchbendChanged`, `notePressureChanged`, `noteTimbreChanged`, `noteKeyStateChanged`) — **all** different signatures from `SynthesiserVoice::startNote/stopNote/pitchWheelMoved/controllerMoved`. That's a Phase 2.6 hazard if we picked Synthesiser now.
- Single-voice mode (Phase 2.1) is fully supported by `MPESynthesiser` — `setCurrentPlaybackSampleRate`, `addVoice`, `setVoiceStealingEnabled(false)`, `noteAdded/noteReleased` work with one voice perfectly.
- Phase 2.1a does NOT need MPE channel routing logic — but we get it for free when we wire it up properly later (no rewiring cost in Phase 2.6).

**Open consequence:** Phase 2.1a's MIDI handling needs an `MPEZoneLayout` set on the synth. Default lower-zone (master ch 1, member ch 2-15) is fine for a single voice — non-MPE DAWs send all events on channel 1 which falls through to the master zone, and the synth still triggers the voice. Confirmed by Stage 1 verification of similar plugins (see O-Bowed `PluginProcessor.cpp` MPE init).

### Q4. Voice-level oversampler placement

**Recommendation: voice-level (inside `BowedContrabassVoice`).**

**Why:**
- Mirrors O-Bowed exactly (`BowedStringVoice.h:95`). `juce::dsp::Oversampling<float>` per voice with `(1, 1, filterHalfBandPolyphaseIIR)` and a per-voice `juce::AudioBuffer<float> voiceBuffer` is the validated pattern.
- Per-voice cost is negligible for a mono synth (one instance).
- Forward compatibility: when Phase 2.2 adds per-string voicing (one voice per string with bow steering between them), each voice gets its own oversampler and its own buffer; no refactor needed.
- Latency reporting: the voice exposes `getOversamplingLatency()` (already a method on O-Bowed's voice); the processor reads it in `prepareToPlay` and passes to `setLatencySamples()`. This is the existing, validated chain.

**Voice contract (for PLAN.md):**
```cpp
class BowedContrabassVoice : public juce::MPESynthesiserVoice {
    // ... DSP members ...
    juce::dsp::Oversampling<float> oversampling {
        1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::AudioBuffer<float> voiceBuffer;
public:
    void prepareToPlay(double hostSampleRate, int maxBlockSize);
    float getOversamplingLatency() const noexcept { return oversampling.getLatencyInSamples(); }
    // ...
};
```

Processor in `prepareToPlay`:
```cpp
synth.setCurrentPlaybackSampleRate(sampleRate);
for (int i = 0; i < synth.getNumVoices(); ++i)
    if (auto* v = dynamic_cast<BowedContrabassVoice*>(synth.getVoice(i)))
        v->prepareToPlay(sampleRate, samplesPerBlock);

if (auto* v = dynamic_cast<BowedContrabassVoice*>(synth.getVoice(0)))
    setLatencySamples(static_cast<int>(std::ceil(v->getOversamplingLatency())));
else
    setLatencySamples(0);
```

### Q5. `INFINITE_SUSTAIN` mapping curve

**Recommendation: quadratic skew, `g = 0.997 + 0.00295 · x²`, hard-clamped to ≤ 0.9999999.**

**Why quadratic vs linear vs piecewise:**
- ARCHITECTURE.md §"Bridge Filter (One-Pole Lowpass + Loop Gain)" line 447 already specifies "quadratic" — this resolves the open question by inheriting the architecture's own choice. The CONTEXT.md framed the question as still-open; it isn't, on closer reading. We confirm and lock the architecture's choice.
- Audibility argument: the perceptual interesting region is `g > 0.999` (corresponds to T60 > 6 seconds). Linear skew puts that region in the top 0.7% of the knob (unusable). Quadratic puts the top half of the knob between `g = 0.997 + 0.00295·0.25 = 0.99774` and `g = 0.99995` — a perceptually rich span.
- A piecewise mapping (linear below 0.5, exponential above) gives finer control at the top but introduces a slope discontinuity at 0.5 — automation through that point produces a perceptible kink. Reject.
- A pure exponential mapping is over-aggressive: tiny knob movements at the top become huge gain changes. Quadratic is the sweet spot.

**Concrete formula (PLAN.md):**
```cpp
float computeLoopGain(float infSustainParam01) noexcept {
    constexpr float kFloor   = 0.997f;
    constexpr float kSpan    = 0.00295f;        // 0.99995 − 0.997 at param=1
    constexpr float kCeiling = 0.9999999f;       // architectural hard ceiling
    const float g = kFloor + kSpan * infSustainParam01 * infSustainParam01;
    return std::min(g, kCeiling);
}
```

**Drone-mode boundary:** ARCHITECTURE.md says the `−1e-20` constant leak is added "outside drone mode (`INFINITE_SUSTAIN < 0.95`)". Use the parameter value (not the computed `g`) for that branch — the user knob is the natural hysteresis-free indicator.

```cpp
const float kIsDroneMode = infSustainParam01 >= 0.95f;
const float leak = kIsDroneMode ? 0.0f : -1.0e-20f;
```

This ensures the leak doesn't fight a true drone (where energy must persist forever) and stays defensive in normal-bow mode.

---

## 5. Pitfalls and Risk Surfacing

Pulled from the auto-loaded `spike-findings-VST-development` skill, the global memory file, and the implementation-risks section of `ARCHITECTURE.md`. Phase 2.1a/b/c must defend against each.

| # | Pitfall | Manifestation if missed | Defence |
|---|---|---|---|
| 1 | Calling `setMaximumDelayInSamples` from `processBlock` | Silent allocation on audio thread → spike, possible heap thrash under DAW load | All sizing in `prepareToPlay` only. Plan task explicitly. |
| 2 | Forgetting `juce::ScopedNoDenormals` at `processBlock` entry | E1 sustain CPU spikes 30–100× when loop tail enters subnormal range; pluginval may still pass | Stage 1 already includes it; Phase 2.1a must NOT remove it when replacing the silent placeholder. |
| 3 | Lagrange3rd construction without correct `prepare(ProcessSpec)` call | Internal write pointers wrong → sample-rate-relative pitch error and overflow on first block | `prepare(spec)` then `setMaximumDelayInSamples(8192)` in voice `prepareToPlay`. |
| 4 | Bridge filter `g` computed at parameter-update rate but applied per-sample | If parameter snapshot races, `g` can briefly exceed clamp ceiling → divergence | `g` is computed once per block in `renderNextBlock` from the cached APVTS atomic, then passed to the per-sample inner loop as a constant. |
| 5 | DC blocker placed before bridge filter | DC blocker is a HP; placing it before the LP saturator wrecks the in-loop saturator's symmetry | ARCHITECTURE.md is explicit: dispersion → bridge LP → saturator → **DC blocker** → fractional delay (§"Processing Order Requirements" step 13.b). Plan must list this order verbatim. |
| 6 | Per-sample `setDelay()` not used during ramps | Zipper noise on parameter sweeps (PERF/QUAL-01 fail) | Phase 2.1 doesn't have `DETUNE` ramps — but `STRING_STIFFNESS` recomputes dispersion `a` and that should also be smoothed. Plan: compute `a` once per block from a `juce::SmoothedValue<float>` on stiffness (20 ms ramp). |
| 7 | Oversampler `processSamplesUp` returns a block whose lifetime ends with the next call | Holding the returned `AudioBlock` past `processSamplesDown` is undefined | Process synchronously: `auto up = os.processSamplesUp(host); engine.process(up); os.processSamplesDown(host);` no aliasing. |
| 8 | `MPESynthesiser` not given an `MPEZoneLayout` | Voices never trigger because the synth has no zone | Set lower-zone in processor constructor: `synth.enableLegacyMode(/*pitchbendRange*/24, juce::Range<int>(1,16))` for non-MPE DAWs, or `setZoneLayout(MPEZoneLayout::makeLowerZoneOnly(...))` for MPE. The existing O-Bowed processor is the reference. |
| 9 | Loop gain `g` evaluated *after* hardware floats round to non-finite | One subnormal slip and the ceiling-clamp doesn't help (clamp is `min`, not `isfinite` check) | Add `if (!std::isfinite(state)) state = 0.0f;` at the bridge-filter entry point. Cheap and one-shot recovery. |
| 10 | Pluginval failing at strictness 10 because new audio path introduces denormal-flush sensitivity | Stage 1 passed pluginval; Phase 2.1 must not regress | Run `pluginval --strictness-level 10 --validate-in-process` against the dev binary as part of the verify phase. |
| 11 | `getLatencySamples()` overridden anywhere in the new code | JUCE 8 critical: getter is non-virtual; an `override` is silently ignored, breaking host alignment | Stage 1 already audited this. Phase 2.1 keeps `setLatencySamples()` calls only. |
| 12 | Bass-tuned `mu_s/mu_d` not actually changing default behavior | If the friction model is constructed somewhere with no `setX` override, defaults still come from the header init list — bass voice ends up with treble friction | Phase 2.1a sets `HyperbolicFriction` defaults via member init list edits in the bass-side header copy. Phase 2.1b parameterises so each consumer gets the right defaults. |

---

## 6. Phase 2.1 Sequencing Refinement

Synthesising the answers above into the order PLAN.md should plan:

1. **2.1a — E1 + hyperbolic + bridge LP (inline copy):**
   - Author `Source/DSP/HyperbolicFriction.h` (bass-edited copy).
   - Author `Source/DSP/BowModel.{h,cpp}` (bass-default copy).
   - Author `Source/DSP/WaveguideString.{h,cpp}` (bass-adapted: single Lagrange3rd delay, algebraic in-loop saturator, DC blocker, denormal leak, quadratic `g` mapping).
   - Author `Source/BowedContrabassVoice.{h,cpp}` (per-voice 2× oversampler, `MPESynthesiserVoice`, single E1 string).
   - Author `Source/OContrabassMPESynthesiser.h` (CC11 dispatch, MPE zone init).
   - Update `Source/PluginProcessor.cpp`: replace silent placeholder with synth render path, wire APVTS to voice, set latency from voice oversampler.
   - Author `tests/O-Contrabass/render-harness/{CMakeLists.txt,main.cpp}` + a single CTest entry covering 60s E1 max-sustain stability.
   - Verify: build, AU `auval`, pluginval level 10, harness PASS.

2. **2.1b — Module extraction:**
   - Pre-bar: render `tests/O-Bowed-regression/baseline.wav` from current O-Bowed HEAD with humanise/noise OFF.
   - Create `modules/synthesis/bow-friction/` with `cpp/HyperbolicFriction.h`, `cpp/BowModel.{h,cpp}`, `module.yaml`, `README.md`.
   - Update `modules/registry.yaml` with the new entry.
   - Update `plugins/O-Bowed/CMakeLists.txt`: remove `Source/DSP/HyperbolicFriction.h` and `Source/DSP/BowModel.{h,cpp}` from `target_sources`, add `ouaricon_add_module(O-Bowed bow-friction)`. Add a config-time defaults selector if needed (e.g. compile-define `OBOWED_BOW_FRICTION_TREBLE_DEFAULTS`).
   - Update `plugins/O-Contrabass/CMakeLists.txt` similarly.
   - Delete `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h` and `BowModel.{h,cpp}` and the equivalents in `plugins/O-Contrabass/Source/DSP/`.
   - Re-render O-Bowed regression WAV; sha256 must match baseline (or fall to ULP-level fallback).
   - O-Bowed full pluginval + auval re-run.

3. **2.1c — Cascaded allpass dispersion:**
   - Author `Source/DSP/DispersionFilter.h` per ARCHITECTURE.md §"Cascaded Allpass Dispersion (Rauhala/Välimäki 2006)" — `template <int MaxSections=4>`, transposed-DF-II per-section, closed-form coefficient via `computeAllpassCoeff(f0, B, M)`.
   - Wire into `WaveguideString.cpp` immediately before bridge LP on the right-going wave (architecture §"Loop placement").
   - For E-string: `M=4`, `B = 1e-4 · STRING_STIFFNESS`. Coefficient recomputed per-block from a 20 ms `SmoothedValue` on `STRING_STIFFNESS`.
   - Verify: STRING_STIFFNESS sweep 0→100% produces continuous timbral change (no clicks). Test reuses the harness.

This matches CONTEXT.md §"Cycle Scope (Phase 2.1)" verbatim — the only refinement is making explicit that the WAV-baseline step in 2.1b runs **before** any module-extraction code change is committed.

---

## 7. Module Reuse Map (for PLAN.md cross-reference)

| Module | Status | Phase 2.1 use |
|---|---|---|
| `core/webview-relay-manager` | existing | not used in Phase 2.1 (Stage 3 territory) |
| `core/resource-provider` | existing | not used in Phase 2.1 |
| `tuning/scala-tuning-engine` v2.0 | existing, linked from Stage 1 | not exercised in Phase 2.1 (Phase 2.6 territory) |
| `tuning/note-expression` v1.0 | existing, linked from Stage 1 | not exercised in Phase 2.1 (Phase 2.6) |
| `synthesis/bow-friction` v1.0 | **NEW — created in Phase 2.1b** | Phase 2.1a uses inline copy; 2.1b extracts; 2.1c continues to consume |

---

## 8. Open Items for Plan Phase

These are decisions PLAN.md should make based on this research, but that don't block writing the plan itself:

1. **`BowedContrabassVoice` constructor injection** vs. setters — pick a style consistent with `BowedStringVoice` (latter uses `explicit BowedStringVoice(juce::AudioProcessorValueTreeState* apvts)` plus `setX()` setters for everything else). Follow that.
2. **Compile-time defaults selection in `bow-friction` module** — simplest is templated defaults (`HyperbolicFriction<BassDefaults>` vs `<TrebleDefaults>` traits class), but a runtime constructor parameter (`HyperbolicFriction(Defaults d = Defaults::Treble)`) is also fine. Plan picks one.
3. **Render-harness MIDI generator** — CLI args for note number, velocity, sustain duration, release duration; default to E1 (MIDI 28), velocity 0.7, 60s sustain, 5s release tail. Plan finalizes the CLI shape.
4. **Whether to commit the regression baseline WAV** — `tests/O-Bowed-regression/baseline.wav` is binary; size ~10 MB at 44.1 kHz mono float 30s. Acceptable to commit; alternative is a sha256-only check against a freshly-rendered baseline (slower, requires CI to render twice). Plan picks committed-WAV.

---

## 9. References

**Inside the project:**
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` — locked DSP contract (sha256:3cb26814)
- `plugins/O-Contrabass/.planning/parameter-spec.md` — locked APVTS contract (sha256:c47fe736)
- `plugins/O-Contrabass/.planning/ROADMAP.md` — locked phase plan (sha256:106639f6)
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` — discuss-phase output
- `plugins/O-Contrabass/.planning/stages/1-foundation/{SUMMARY,VERIFICATION}.md` — Stage 1 outputs
- `research/O-Contrabass-bass-waveguide-stability.md` — depth-research source for §3 of architecture

**O-Bowed reference code:**
- `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h` — friction model verbatim source
- `plugins/O-Bowed/Source/DSP/BowModel.{h,cpp}` — envelope verbatim source
- `plugins/O-Bowed/Source/DSP/WaveguideString.{h,cpp}` — structural reference (NOT a copy target)
- `plugins/O-Bowed/Source/BowedMPESynthesiser.h` — MPE dispatch pattern
- `plugins/O-Bowed/Source/BowedStringVoice.{h,cpp}` — voice structure pattern
- `plugins/O-Bowed/CMakeLists.txt` — existing module-consumer pattern
- `plugins/O-Lyrica/Source/BowedStringVoice.*` — Note-Expression-aware voice pattern (not relevant until Phase 2.6)

**Module-system reference:**
- `modules/registry.yaml` — category list, module entry schema
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module()` implementation
- `modules/tuning/note-expression/{module.yaml,module.cmake}` — header-only-module pattern reference
- `modules/tuning/scala-tuning-engine/module.yaml` — multi-file C++ module pattern reference

**JUCE 8.0.4 source (local fork at `/Users/taylorbrook/JUCE`):**
- `modules/juce_dsp/processors/juce_Oversampling.h` — `Oversampling<float>` class (lines 61–200)
- `modules/juce_dsp/processors/juce_DelayLine.h` — `DelayLine<float, Lagrange3rd>` (lines 64–168)

**External references** (architecture-level, no need to re-fetch):
- Rauhala & Välimäki 2006, "Tunable dispersion filter design for piano synthesis" — closed-form coefficient
- Smith, *Physical Audio Signal Processing* — DC blocker pattern, scattering junction
- JUCE forum, "State of the Art Denormal Prevention" — `ScopedNoDenormals` semantics

---

## Next Phase

Ready for: **plan** — `/plugin-plan O-Contrabass 2-dsp`

Plan focus:
- Sequence 2.1a → 2.1b (with WAV baseline pre-step) → 2.1c.
- Atomic-task breakdown each with explicit input artefacts, output artefacts, success criteria.
- Code-skeleton snippets for the new files (file/line counts; method signatures; member layouts) so execute can fill bodies without re-deriving structure.
- Pluginval / auval / harness verification checklist (Phase 2.1 exit gate).
- Deviation flagged: `WaveguideString.cpp` is a bass-adapted reimplementation, not a verbatim port (CONTEXT.md framing was approximate; ARCHITECTURE.md mandates the deltas). `SchellengGuard` is dropped from Phase 2.1b's module surface (defer to Phase 2.3).

---

# 10. Re-Research After Phase 2.1a Harness Failure (2026-04-26)

**Trigger:** `CHECKPOINT-2.1a.md` — render-harness `O-Contrabass-render-test --note 28 --sustain 60 --infinite-sustain 1.0` exits 1 with `pass_rms = false` (peak −39 dBFS, `rmsMid_s5_s6 = 0.0`, `rmsFinal_lastSecond = 0.0`, no NaN/Inf, no CPU spike). String is excited but **never reaches steady-state Helmholtz**; the rest of the gate is green.

This section re-researches the 5 hypotheses listed in `CHECKPOINT-2.1a.md §"What Needs Research"` in the order most likely to be load-bearing on the failure. It supersedes the Phase 2.1a coding decisions for `WaveguideString.cpp` only — `BowModel.{h,cpp}`, `HyperbolicFriction.h`, `BowedContrabassVoice.{h,cpp}` outside the friction-junction call, the render-harness, and the MPESynthesiser shell remain validated.

---

## 10.1 H1 — Single-rail vs split-rail energy budget

### 10.1.1 Smith's canonical formulation (cited)

Per Smith's *Physical Audio Signal Processing*, "Bow-String Scattering Junction" (CCRMA / dsprelated.com mirror), the canonical bow excitation injects friction-modulated velocity **symmetrically into both traveling-wave rails**:

```
v_sr⁻ = v_sl⁺ + ρ̂(v_Δ⁺) · v_Δ⁺      // outgoing right (toward bridge)
v_sl⁻ = v_sr⁺ + ρ̂(v_Δ⁺) · v_Δ⁺      // outgoing left  (toward nut)
```

Both right- and left-going outgoing waves carry the **same** injection term `ρ̂(v_Δ⁺) · v_Δ⁺`. Per round-trip across the string, the bow point therefore re-injects this term **twice** (once into each rail). This is the formulation O-Bowed implements verbatim at `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:131-133`:

```cpp
float toBridge = nutReflection + newVelocity;
float toNeck   = bridgeReflection + newVelocity;
```

The same `newVelocity` enters both delays simultaneously.

### 10.1.2 What single-rail actually does

A single-rail collapse merges the bridge and neck delays into one delay of length `T = sr / f0`. The bow point is implicit at one end of this delay (effectively "at the bridge"). At each round-trip, only **one** injection of `newVelocity` enters the loop:

`plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:144`:
```cpp
float x = incoming + newVelocity;        // single rail; one injection per round-trip
```

This is a **dimensionally inequivalent** simplification of the canonical scattering junction, not a numerically equivalent rewrite. Two consequences:

1. **Steady-state RMS is ~6 dB lower** for the same `Δv` and `ρ̂`. The string sees half the per-period excitation energy. If the loss filter `g` is unchanged, the equilibrium RMS at the friction junction equals `(injection × g) / (1 − g²)` instead of `(2 · injection × g) / (1 − g²)`. At `g = 0.99995` (max `INFINITE_SUSTAIN` in O-Contrabass) and an arbitrary friction injection of 0.05, split-rail equilibrium ≈ 1000; single-rail equilibrium ≈ 500. **Both should still produce non-zero RMS** — which means halved injection alone does not explain `rmsFinal = 0.0`.

2. **Schelleng F_min scales as 1/(injection efficiency)²** — see §10.4. Halving the per-round-trip injection roughly **doubles** the bow force needed to leave the surface-sound regime. Combined with the H4 finding (default `F_bow = 1.0` is already below F_min for bass parameters), single-rail pushes the operating point further below Helmholtz threshold and the string stays in surface-sound / no-Helmholtz mode (which manifests as effectively zero output through the bridge filter for low-amplitude excitation, since the LP boundary sees a non-Helmholtz random-walk signal that decays without locking).

### 10.1.3 Is "single-rail with 2× injection compensation literally equivalent to split-rail"?

**No, not at all frequencies.** The two topologies match only:
- For a **lossless** lattice (no bridge filter), a single-rail with delay `T = sr/f0` and 2× injection produces the same period-T fundamental amplitude as split-rail. Higher-mode behaviour matches if and only if the loss filter is applied symmetrically.
- For a **frequency-dependent loss filter** (one-pole bridge LP), single-rail places the LP once per round-trip; split-rail places it once per round-trip too (only on the bridge return). Per-round-trip total loss is identical in both cases. So the *steady-state* mode amplitudes are equivalent under 2× compensation **for the fundamental and its low harmonics** (where group-delay variation across the LP is small relative to f0).

**They are NOT equivalent for**:
- The **bow contact point in space**. Split-rail has a meaningful β = bridgeSamples / (bridgeSamples + neckSamples). Single-rail collapses β → 0 (bow at one end). This breaks the Schelleng wedge's `1/β²` dependence in F_min — see H5.
- **Helmholtz mode-locking timing**. The bow phase relative to bridge reflection and nut reflection differs in single-rail: the string sees one combined echo, not two distinct echoes. In practice the friction model still locks (period-T sticking) but the slip-stick transition timing is altered. For Phase 2.1's "is there ANY Helmholtz?" question this is acceptable; for Phase 2.5's body coupling and Phase 2.3's Slow-Bow LFO, the timing fidelity matters.

### 10.1.4 Recommendation

**The 2× injection compensation is necessary but not sufficient.** We must also fix H4 (Schelleng F_min — apply it via `F_bow` headroom) before single-rail can sustain Helmholtz at the bass operating point.

**Decision (locked here, will land in PLAN.md):**
- **R1: Promote `WaveguideString` to split-rail.** This is the cleanest fix and removes the 2× compensation question entirely. The split-rail topology is already validated in O-Bowed (cited above). Cost: ~70 LOC of new code in `WaveguideString.{h,cpp}`, an extra delay line of identical type/size, and `bowPosition` recovers its physical meaning (β-spatial-split lands now instead of Phase 2.5).
- **R1-alt (rejected): keep single-rail with `2.0f * newVelocity`.** Mathematically equivalent at the fundamental but inequivalent for β-physics. Would also require a follow-up restructure in Phase 2.5 (split-rail is required for the body bank's bridge-side coupling). Treats the symptom, not the cause.

**Why R1 is the right choice for Phase 2.1a:**
- Phase 2.1's gate is "stable Helmholtz at E1 with max INFINITE_SUSTAIN". Split-rail is the topology Smith and O-Bowed both use to *get* that gate green. Single-rail was a CONTEXT.md scope-reduction guess that ARCHITECTURE.md never explicitly mandates (architecture says "single delay line" but does not say "single rail"; the delay-line count is implementation, the topology is physics).
- The architecture's loop ordering (`dispersion → bridge LP → in-loop saturator → DC blocker → fractional delay`) maps cleanly onto split-rail: the right-going wave runs through this chain, the left-going wave runs through nut reflection only (`-1` boundary) — same as O-Bowed.
- ARCHITECTURE.md §"Processing Order Requirements" step 13.b uses "delay" (singular) in the description but is consistent with two delay lines (one per rail) — the order describes per-sample order, not delay count.
- We get bow position β as a real spatial split (not a fudge factor on R_s). H5 dissolves into "set delay lengths from β, same as O-Bowed".

**Sources:**
- Julius O. Smith III, *Physical Audio Signal Processing*, "Bow-String Scattering Junction" — formulas in §10.1.1.
- McIntyre, Schumacher, Woodhouse (1983), "On the oscillations of musical instruments", JASA 74(5) — original two-port formulation.
- `research/bow-string-friction-models.md` §3.2 (lines 482–495) — same equations transcribed in repo.
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` — validated split-rail reference (lines 100–155).

---

## 10.2 H2 — In-loop DC blocker effect on steady-state oscillation at f0 = 41.2 Hz

### 10.2.1 Frequency response of the architectural DC blocker

`H(z) = (1 − z⁻¹) / (1 − R·z⁻¹)`, with R = 0.999, applied at internal sample rate `sr_int = 88.2 kHz` (host 44.1 kHz × 2 oversampler).

**−3 dB cutoff:** approx. `f_c ≈ sr_int · (1 − R) / (2π) ≈ 88200 · 0.001 / 6.283 ≈ 14.04 Hz`.

**Magnitude at f0 = 41.2 Hz** (E1 fundamental):
```
ω = 2π · 41.2 / 88200 = 2.935e-3 rad/sample
|H(e^jω)| = |1 − e^{−jω}| / |1 − R·e^{−jω}|
         ≈ sqrt(2 − 2 cos ω) / sqrt(1 − 2R·cos ω + R²)
         = sqrt(8.61e-6) / sqrt(1.997e-6)
         ≈ 2.93e-3 / 1.41e-3
         ≈ 0.946  →  −0.48 dB
```

**Magnitude at higher harmonics:** essentially unity. The 2nd harmonic (82.4 Hz) sits at −0.12 dB; everything above 200 Hz is below −0.01 dB.

**Phase shift at f0:** `arg(H(e^jω)) ≈ +π/2 − ω · 0.5 ≈ +89.92°`. At 41.2 Hz this is roughly a quarter-period phase advance. Per-round-trip this is absorbed by the delay line (we recompensate via `setDelay` if it matters; in practice the system phase-locks regardless).

**Conclusion:** the DC blocker attenuates f0 by **0.48 dB per round-trip**. Over 60 seconds (≈ 2472 round-trips at f0 = 41.2 Hz), cumulative attenuation is **0.48 dB × loop attenuation per cycle**, which in steady state is balanced by injection. The DC blocker does **not** kill steady-state oscillation.

### 10.2.2 Could the DC blocker still be the failure?

Two ways the DC blocker could nuke a not-yet-locked transient:

1. **Phase-shift coupling with the fractional delay**. The DC blocker adds a tiny additional group delay (~0.5 sample at 41.2 Hz) which is not compensated in `updateDelayLength()` — `WaveguideString.cpp:65` only compensates the bridge LP. This tunes f0 by ~0.05 ¢ — completely inaudible; cannot be the failure cause.

2. **Pre-Helmholtz transient eaten by the HP**. Cold-start transients have substantial sub-50 Hz energy. The DC blocker's −3 dB at 14 Hz means a t=0 step injection loses energy in the HP-cutoff range that would otherwise feed the build-up of standing waves. Quantitatively: if 10% of the initial transient energy is below 41 Hz, it's attenuated by ~3 dB. **This slows the build-up but does NOT prevent Helmholtz from forming if injection is sufficient.**

### 10.2.3 Recommendation

**Not the primary cause; keep the in-loop DC blocker, but verify it is placed AFTER the saturator** (already correct in current code, line 169–173) and that `R = 0.999` is appropriate at internal SR.

- **R2: Keep `R = 0.999` at internal SR = 88.2 kHz.** Cutoff ≈ 14 Hz, well below E1 = 41.2 Hz. Confirmed by §10.2.1 numerics. Matches `ARCHITECTURE.md §"DC Blocker"`.
- **R2-aside:** if the harness still fails post-R1, a debug variant temporarily sets `R = 0.9995` (cutoff ≈ 7 Hz) to rule out cumulative HP attenuation. Not the default.

**Sources:**
- Smith, *Physical Audio Signal Processing*, "DC Blocker" chapter (CCRMA, online).
- Local code: `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:169-173`.
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` line 92–99.

---

## 10.3 H3 — Saturator placement: in-loop `x/√(1+x²)` vs O-Bowed external `4·tanh(x/4)`

### 10.3.1 Comparison at low signal level

Both saturators are odd-symmetric, asymptote-bounded soft-clippers. Small-signal gain (Taylor series at `x = 0`):

| Saturator | Small-signal expansion | Linear gain | Asymptote |
|---|---|---|---|
| `4·tanh(x/4)` | `x − x³/48 + …`  | `1.0` | ±4 |
| `x / √(1+x²)` | `x − x³/2 + …`  | `1.0` | ±1 |

At `|x| < 0.1` the algebraic form is functionally `y ≈ x · (1 − x²/2) ≈ x`. At `|x| < 0.01` (which is the regime the harness shows: peak −39 dBFS = 0.0112) the saturator passes the signal essentially unchanged. **It is not bounding the small-signal recursion.**

### 10.3.2 Comparison at large signal level

The asymptote difference matters once `|x| > 0.5`:
- `4·tanh(x/4)`: at `|x| = 1`, output ≈ 0.987; at `|x| = 4`, output ≈ 3.86; only ratchets toward 4 asymptotically. **Effective bound ≈ ±4.**
- `x / √(1+x²)`: at `|x| = 1`, output = 0.707; at `|x| = 5`, output ≈ 0.981. **Effective bound = ±1.**

The algebraic saturator is **4× more aggressive** in clamping large signals, which is why ARCHITECTURE.md prefers it (drone runaway is the primary risk in O-Contrabass; max sustain at all-knobs-up requires a hard ±1 ceiling). For Phase 2.1's failure mode (under-injection), the algebraic saturator's tighter ceiling **does not affect** the build-up — both saturators are essentially linear in this regime.

### 10.3.3 Placement difference: in-loop vs on the write path

The placements ARE meaningfully different even though both are "before the delay write":

**O-Bowed (split-rail), `WaveguideString.cpp:138-141`:**
```cpp
toBridge = sat * std::tanh (toBridge / sat);   // saturator on outgoing wave
toNeck   = sat * std::tanh (toNeck / sat);
bridgeDelay.pushSample (0, toBridge);
neckDelay.pushSample (0, toNeck);
```
Saturator sits **after injection, before the delay**. The bridge LP is a separate stage that ran on the popped sample at the *previous* round-trip. So per round-trip, the saturator is applied **after** the friction injection and **before** the next round-trip's filter — i.e. the LP filter sees post-saturated samples.

**O-Contrabass (single-rail), `WaveguideString.cpp:144-176`:**
```cpp
float x   = incoming + newVelocity;            // injection
float y   = bridgeG * (... + bridgeP * bridgeY) + denormalLeak;  // LP
float sat = y / std::sqrt (1.0f + y*y);        // saturator AFTER LP
float dc  = sat - dcX1 + R * dcY1;             // DC blocker
delayLine.pushSample (0, dc);
```
Saturator sits **after the LP**, before DC blocker, before delay write.

**Why this difference matters at small signal:**

In the bass register at low loop-gain, both placements are linear. At high loop-gain (max INFINITE_SUSTAIN, drone), the LP filter inside the recursion accumulates state that is itself bounded by `g · bridgeY + ...`. With `g → 0.99995`, `bridgeY` can in principle wander before the saturator clamps it on the *next* iteration. The architecture's choice (saturator AFTER LP) is the **defensive** placement: it clamps the LP output on the *current* iteration before it's written to the delay. This is consistent with "Smith's PASP places nonlinearity *after* the lossy filter to avoid amplifying numerical noise" (`ARCHITECTURE.md` line 267).

**At the failure operating point (`peak = −39 dBFS`)**, neither placement is bounding anything. The saturator is irrelevant to the missing Helmholtz oscillation.

### 10.3.4 Recommendation

**Not a primary cause of the harness failure.** The architecture's choice is correct; keep it.

- **R3: Keep `x / √(1+x²)` AFTER the bridge LP, BEFORE the DC blocker.** This matches `ARCHITECTURE.md §"In-loop saturator"` and §"Loop ordering" (line 267) verbatim.
- **R3-companion:** when split-rail lands per R1, the saturator is applied **per rail** on the outgoing wave, identical to O-Bowed's pattern but with the algebraic form. Both `toBridge` and `toNeck` get `y = x / sqrt(1+x²)` after the bridge LP processes the bridge-rail return.

**Sources:**
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:138-141` — tanh-on-write reference.
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` line 88, 267 — algebraic-after-LP locked.
- Smith, *Physical Audio Signal Processing*, "Nonlinearity in Lossy Loops" (CCRMA online) — supports post-LP placement.

---

## 10.4 H4 — Schelleng F_min at β = 0.10 with bass friction defaults

### 10.4.1 Numerical computation

Schelleng's classical formula (per Euphonics §9.3.1, eq. 9):

```
F_min ≈ Z_0² · v_b / (2 · R · β² · (μ_s − μ_d))
```

Plugging plausible double-bass E1 string values:

| Symbol | Value | Source |
|---|---|---|
| `T` (E1 tension)               | 350 N   | typical orchestral bass E (TalkBass tension chart, average wound bronze) |
| `m` (E1 mass / unit length)    | 0.022 kg/m | typical wound E core+winding |
| `Z_0 = √(T·m)`                 | 2.77 N·s/m | computed |
| `R` (bridge mech. resistance)  | 50 N·s/m | Bissinger / Askenfelt range for double bass |
| `v_b`                          | 0.15 m/s | bass-default `bowSpeedParam` |
| `β`                            | 0.10     | `BOW_POSITION` default |
| `μ_s`                          | 0.85     | bass-tuned default |
| `μ_d`                          | 0.25     | bass-tuned default |

```
F_min = (2.77² · 0.15) / (2 · 50 · 0.10² · (0.85 − 0.25))
      = (7.673 · 0.15) / (100 · 0.01 · 0.60)
      = 1.151 / 0.600
      ≈ 1.92 N
```

```
F_max = (2 · Z_0 · v_b) / (β · (μ_s − μ_d))
      = (2 · 2.77 · 0.15) / (0.10 · 0.60)
      = 0.831 / 0.06
      ≈ 13.85 N
```

**Playable wedge: 1.92 N ≤ F_bow ≤ 13.85 N.**

The default `BOW_PRESSURE = 1.0` (driving `F_bow_target = 1.0` in `BowModel.cpp:33`) is **below the Schelleng minimum** by roughly a factor of 2. Even with the friction model's linear scaling (`r = 0.25·μ·F_bow / R_s`), 1.0 maps to a "0.5×μ" injection magnitude that the canonical Schelleng analysis would call surface-sound territory.

### 10.4.2 Compounding with H1 single-rail injection

Single-rail injects `newVelocity` once per round-trip; canonical split-rail injects it twice. The effective Schelleng F_min scales with the **square** of the injection efficiency (because F_min ∝ 1/(injection)²), so:

`F_min,single-rail ≈ 4 × F_min,split-rail ≈ 7.7 N`

at the same numerical operating point. Default F_bow = 1.0 is now ~8× below F_min. **Surface-sound regime, not Helmholtz.** This matches the harness observation: the string is excited (peak −39 dBFS = 0.0112) but never locks into period-T sticking → low / zero RMS at the fundamental.

### 10.4.3 Caveats on the absolute Newton calculation

The friction model in code uses **dimensionless / arbitrary units** (`R_s = 0.5`, F_bow ∈ [0, ~something_unbounded]). It is not literally "Newtons". So the absolute Schelleng numbers are *indicative*, not contractual. What IS contractual:
- Relative shift of F_min between O-Bowed (β=0.13, Δμ=0.5) and O-Contrabass (β=0.10, Δμ=0.6): F_min ratio = (0.13/0.10)² · (0.5/0.6) ≈ **1.41×**.
- Relative shift of F_max between same: F_max ratio = (0.13/0.10) · (0.5/0.6) ≈ **1.08×**.
- O-Bowed sustains Helmholtz at F_bow=0.5 split-rail. O-Contrabass needs F_bow ≥ 0.5 × 1.41 × 2 (single-rail penalty) ≈ **1.41×O-Bowed = 0.71** to even *equal* O-Bowed's normalized F_min. Default is 1.0 — barely above. With any subtractive friction effect (μ < μ_s during slip), it falls below.

In other words: **the bass defaults are sitting on the F_min boundary**, and any inefficiency in injection (single-rail) tips them below it.

### 10.4.4 Recommendation

- **R4-1: Adopt R1 (split-rail) as the primary fix** — this immediately recovers the 2× injection deficit and pulls operating point back to roughly O-Bowed's normalized regime (with the β=0.10 vs 0.13 Δμ=0.6 vs 0.5 deltas accounted for, F_min × 1.41 vs O-Bowed). At F_bow=1.0 normalized, this is comfortably above F_min.
- **R4-2: Verify F_bow_target maps usefully**. The friction-model `r = 0.25 · μ · F_bow / R_s` formula with `R_s = 0.5` and `F_bow = 1.0` gives `r = 0.5 · μ` (roughly 0.13 to 0.43 across slip→stick). The reflection coefficient `ρ = r/(1+r)` is then 0.111 to 0.298. **Plenty of injection** if the topology is right.
- **R4-3 (optional, defensive):** add a Schelleng-aware UI-level clamp on `BOW_PRESSURE` such that the **automation envelope** can be biased into the Helmholtz wedge automatically. For Phase 2.1a, **NOT** needed if R1 lands. Defer to Phase 2.3 (Slow-Bow LFO) where Schelleng-clamping is already on the roadmap.
- **R4-4 (do NOT do):** do NOT raise `mu_d` to narrow Δμ. Bass character requires the wider 0.60 gap (matches Hanson period-doubling sub-harmonic regime that Phase 2.4 needs).
- **R4-5 (do NOT do):** do NOT change `BOW_POSITION` default. β=0.10 is the locked operating point per `parameter-spec.md` and ARCHITECTURE.md (sul-ponticello-leaning bass).

**Sources:**
- Schelleng, J.C. (1973). "The bowed string and the player." JASA 53(1).
- Euphonics §9.3.1 — F_min/F_max formulas: <https://euphonics.org/9-3-1-shellengs-bow-force-limits/>.
- `research/bow-string-friction-models.md` §4.4 (lines 674–714) — repo transcription of Schelleng formulas.
- `research/O-Contrabass-bass-waveguide-stability.md` — bass impedance ranges.
- Bissinger, G., bridge-impedance measurements on bowed-string family.

---

## 10.5 H5 — Single-rail bow position physics

### 10.5.1 What β means in each topology

**Split-rail (canonical):** β is a **spatial split** of the total delay:
- `bridgeSamples = β · totalDelay`
- `neckSamples   = (1 − β) · totalDelay`

The bow sees the **two rail impedances in parallel**. Effective impedance at the bow contact: `Z_eff = Z_0 · β · (1−β)` (max at β=0.5, drops near both ends). β shifts harmonic-emphasis (Schelleng's `1/β`-spaced "missing-harmonic" pattern) **and** shifts F_min via `1/β²`.

**Single-rail (current O-Contrabass):** β has **no spatial meaning**. The bow point is implicitly co-located with the bridge end of the delay. Current code (`BowedContrabassVoice.cpp:228-229`) bolts on a **fudge factor**:
```cpp
const float betaScale = juce::jlimit (0.4f, 1.5f, 0.5f / std::max (0.02f, effectivePosition) * 0.1f);
frictionModel.setStringImpedance (juce::jlimit (0.4f, 1.5f, betaScale));
```
At default β=0.10, `betaScale = 0.5/0.10 · 0.1 = 0.5` → no impedance change. At β=0.02 (extreme close-to-bridge), `betaScale = 1.5` (clamped) — *higher* impedance, which is **physically backwards**: at a real bridge-leaning β, the bow sees *lower* parallel impedance, not higher. The fudge factor only fires at extremes and does so in the wrong direction. **It cannot reproduce Schelleng's `1/β²` F_min dependence**.

### 10.5.2 Why single-rail bow-position physics is structurally inadequate

Three things split-rail does that single-rail cannot recover via any impedance-fudge:

1. **Missing-harmonic patterns.** A bow at β=1/3 cannot excite the 3rd, 6th, 9th… harmonics (node coincidence). Single-rail has no spatial node concept; harmonics are excited based purely on friction-junction state, regardless of β. **Tonal-character loss**.
2. **F_min(β) curve.** Schelleng's `1/β²` divergence near β=0 is what enforces "you cannot bow at the bridge with low force". Single-rail has no β² term in any equation (only the fudge factor's `1/β` linear scaling, and only in extremes). **Playability-physics loss**.
3. **Body bank coupling at the bridge.** Phase 2.5's body resonator reads the bridge-side outgoing wave. Single-rail's "bridge" is fictive (one end of one delay), so what the body sees is not actually a bridge-side wave — it's the union of bridge + neck contributions. **Body-coupling fidelity loss**.

### 10.5.3 Phase 2.1a does not need 1, 2, or 3 to pass the harness test, BUT…

The Phase 2.1a gate is "60s of stable Helmholtz at max INFINITE_SUSTAIN, no NaN, no runaway". It does NOT require correct β-physics. It DOES require Helmholtz to form, which requires sufficient injection per round-trip (H1) and/or operating above F_min (H4).

**However**, if we adopt R1 (split-rail) for H1, β becomes a real spatial split for free — H5 is dissolved as a separate question, not deferred. There is no extra cost, only benefit.

### 10.5.4 Recommendation

- **R5: Drop the `betaScale → setStringImpedance` fudge in `BowedContrabassVoice.cpp:225-229`.** Once R1 makes β a real spatial split, the friction-model impedance is fixed at `R_s = 0.5` and the spatial physics emerges from the rail split.
- **R5-companion:** keep `BOW_POSITION` parameter intact (locked contract). It now drives `setBowPosition(beta)` in `WaveguideString` which calls `updateDelayLengths()` — same pattern as O-Bowed.

**Sources:**
- Euphonics §9.3 — Schelleng diagram and missing-harmonic explanation: <https://euphonics.org/9-3-how-a-violinist-can-go-wrong-schellengs-diagram/>.
- Smith, *Physical Audio Signal Processing*, "Bow-String Scattering Junction" — β-spatial-split formulation (cited in §10.1.1).
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:71-79` — split implementation reference.

---

## 10.6 Recovery Decision Matrix

| ID | Decision | Action in 2.1a | Files affected |
|---|---|---|---|
| **R1** | **Promote `WaveguideString` to split-rail** | Two `juce::dsp::DelayLine<float, Lagrange3rd>` (bridgeDelay + neckDelay), each 8192 samples. Bow position β splits the total delay. Mirror O-Bowed's `processSample` structure with O-Contrabass's loop ordering (dispersion → bridge LP → in-loop saturator → DC blocker → delay write) on the bridge-rail return. Nut-rail uses `-1` reflection, no LP/saturator/DC blocker (matches O-Bowed). Both rails get `+ newVelocity` injection. | `WaveguideString.h` (add `neckDelay`, drop unused `bowPosition`-as-fudge), `WaveguideString.cpp` (rewrite `processSample`, `updateDelayLength` → `updateDelayLengths`, `reset`, `trigger`) |
| **R2** | Keep DC blocker R = 0.999 in-loop AFTER saturator | No code change | (none) |
| **R3** | Keep algebraic saturator `x / √(1+x²)` AFTER bridge LP | No code change to formula; per-rail application after R1 | `WaveguideString.cpp` (apply per rail in split-rail processSample) |
| **R4** | Keep `BOW_PRESSURE` default = 1.0; do not retune friction defaults | No code change | (none) |
| **R5** | Drop the `betaScale → setStringImpedance` fudge in voice | Remove `betaScale` lines; `frictionModel.setStringImpedance` retains its prepare-time default; `waveguideString.setBowPosition(effectivePosition)` becomes the real β source | `BowedContrabassVoice.cpp:225-229` (delete betaScale block) |

**Net effect:** ~70 LOC change in `WaveguideString.{h,cpp}` (split-rail implementation), ~5 LOC delete in `BowedContrabassVoice.cpp` (drop fudge factor). Everything else (BowModel, HyperbolicFriction, MPESynthesiser, oversampler, render-harness, CMake, processor wiring) is unchanged. The bass-tuned defaults survive intact. The 2× oversampling + Lagrange3rd interpolation + `setLatencySamples` chain survives intact.

---

## 10.7 Why this is NOT a CONTEXT.md / ARCHITECTURE.md violation

CONTEXT.md §"Cycle Scope 2.1a" line 30: *"Single `juce::dsp::DelayLine<float, Lagrange3rd>` (8192 samples) for E1 (41.2 Hz)."* — this is the implementation guess that is now invalidated by the harness FAIL.

ARCHITECTURE.md §"String Waveguide Bank" (line 78–88): describes "per-string dispersion", "bridge filter", "in-loop saturator", "constant leak", "per-string detune" — does **not** specify single-rail. The *number* of delay lines per string is not part of the locked architecture; the *structure* (Lagrange3rd, dispersion, bridge LP with quadratic g-skew, algebraic in-loop saturator, constant leak, DC blocker, 2× oversampling) is.

ROADMAP.md sha256:106639f6 — Phase 2.1 description does not specify rail count.

Promoting `WaveguideString` to split-rail therefore:
- **Does NOT** violate ARCHITECTURE.md (the architecture is silent on rail count; all named components survive).
- **Does NOT** violate ROADMAP.md (rail count not specified).
- **DOES** deviate from CONTEXT.md's specific implementation guess. Flag this as a CONTEXT-deviation in PLAN.md preamble; CONTEXT.md was written before the harness validated reality.
- **DOES NOT** require parameter-spec.md changes. The 29 APVTS IDs survive intact.

Per the GSD workflow, CONTEXT.md is a discuss-phase artifact — it is *advisory* relative to research/architecture. Re-research overrides discuss-phase guesses where reality demands.

---

## 10.8 Open Items for Plan Phase (post-recovery)

1. **Per-rail loop ordering**. ARCHITECTURE.md says `dispersion → bridge LP → saturator → DC blocker → fractional delay`. In split-rail, this chain runs on the **bridge-side outgoing wave** only. Nut-side gets a `-1` boundary plus the fractional delay (no dispersion, no LP, no saturator, no DC blocker on the nut return). Matches O-Bowed. **Plan must list this explicitly.**

2. **DC blocker per rail or per loop?** Recommendation: **per-loop, on bridge side only** (same as the bridge LP). Rationale: the DC accumulation is from the friction-junction nonlinearity, which feeds both rails equally. Once the bridge-side return is DC-blocked, the round-trip integral DC is pinned. Adding a second DC blocker on the nut side double-counts and adds a 1-sample group-delay imbalance between rails (pitch error ~0.05¢, inaudible but bookkeeping waste).

3. **Sign convention check**. O-Bowed uses `bridgeReflection = -bridgeLossFilter.process(bridgeDelay.popSample(0))` (sign flip after LP) and `nutReflection = -neckDelay.popSample(0)` (sign flip on raw nut return). The injection flows into both rails *positive*. Mirror this verbatim in O-Contrabass split-rail.

4. **Initial conditions**. `trigger(frequency)` in current code does `reset()` → both delay lines empty. With cold start, first round-trip at the bow is `v_string_incoming = 0`, so `Δv = v_bow ≠ 0` → friction injects → string excites. Same as O-Bowed. No additional initialization needed.

5. **Scope expansion vs CONTEXT.md.** The original CONTEXT.md envisioned single-rail to "simplify Phase 2.1a". After re-research, single-rail is not actually simpler in any meaningful way (same number of moving parts; missing the Schelleng/Helmholtz physics), and is an anti-deviation from O-Bowed's validated topology. **Net LOC delta from current 2.1a code is ~+30 LOC** (one extra delay line, two `popSample`/`pushSample` calls; total `WaveguideString.cpp` rises from 198 LOC to ~230 LOC). This is small. Phase 2.1a's day budget is unaffected.

6. **Pre-flight check**. Before R1 implementation, plan should add a 1-test step: render the harness against a `WaveguideString` variant where line 144 is changed from `float x = incoming + newVelocity;` to `float x = incoming + 2.0f * newVelocity;` (R1-alt). If this passes the harness, single-rail with 2× compensation is sufficient at the *fundamental* level (and the canonical fix per §10.1.4 is more conservative but still recommended for β-physics correctness). If it does NOT pass, more is broken (sign convention, initial-conditions, BowModel envelope, …) — investigate before R1. **30-minute test, low cost, high diagnostic value.**

---

## 10.9 Summary — Phase 2.1a Recovery Plan

Single sentence: **"Promote `WaveguideString` from single-rail to split-rail (R1), drop the `betaScale` fudge in the voice (R5), keep everything else."**

The five hypotheses in `CHECKPOINT-2.1a.md §"What Needs Research"` resolve as:

1. **H1 (single-rail energy budget)** — confirmed primary cause. Single-rail loses 50% of canonical injection per round-trip; Schelleng F_min scales 4× → operating point is below playable wedge. **Fix: R1 split-rail.**
2. **H2 (DC blocker)** — not the cause. Magnitude at 41.2 Hz is −0.48 dB; cumulative attenuation cannot suppress steady-state Helmholtz when injection is sufficient. **Keep as-is (R2).**
3. **H3 (saturator placement)** — not the cause. Both forms are linear at the failure operating point (peak −39 dBFS). The architecture's choice is correct for high-loop-gain behaviour. **Keep as-is (R3).**
4. **H4 (Schelleng F_min)** — confirmed secondary cause (compounds with H1). Bass operating point sits *near* F_min before single-rail penalty. After R1, bass defaults are comfortably inside the wedge. **Keep defaults; no friction retuning (R4).**
5. **H5 (single-rail bow position)** — structural inadequacy of single-rail topology. Dissolved by R1 (β recovers spatial meaning automatically). **Drop `betaScale` fudge (R5).**

Plan-phase deliverable: rewrite `WaveguideString.{h,cpp}` to split-rail (≈70 LOC delta), delete the `betaScale` fudge in `BowedContrabassVoice.cpp:225-229`, leave `BowModel`, `HyperbolicFriction`, `OContrabassMPESynthesiser`, oversampler wiring, `PluginProcessor`, render-harness, and CMake intact. Re-run the harness — expected PASS at all four invariants. Then proceed with Phase 2.1b module extraction and Phase 2.1c dispersion as previously planned.

---

## 10.10 References (recovery section)

**Primary citations:**
- Smith, J.O. (online). *Physical Audio Signal Processing*, "Bow-String Scattering Junction" — <https://www.dsprelated.com/freebooks/pasp/Bow_String_Scattering_Junction.html>. Canonical two-rail injection formulation.
- Smith, J.O. (online). *Physical Audio Signal Processing*, "Digital Waveguide Bowed-String" — <https://ccrma.stanford.edu/~jos/waveguide/Digital_Waveguide_Bowed_String.html>. Two delay-line pair structure.
- Schelleng, J.C. (1973). "The bowed string and the player." JASA 53(1).
- McIntyre, M.E., Schumacher, R.T., Woodhouse, J. (1983). "On the oscillations of musical instruments." JASA 74(5). Original two-port scattering formulation.
- Euphonics §9.3.1, "Schelleng's bow force limits" — <https://euphonics.org/9-3-1-shellengs-bow-force-limits/>. Closed-form F_min and F_max.
- Euphonics §9.3, "How a violinist can go wrong: Schelleng's diagram" — <https://euphonics.org/9-3-how-a-violinist-can-go-wrong-schellengs-diagram/>. β-physics and missing-harmonic pattern.

**Repo references:**
- `plugins/O-Contrabass/.planning/stages/2-dsp/CHECKPOINT-2.1a.md` — failure observation.
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` — locked architecture (synthesis line 78–99, 267).
- `research/bow-string-friction-models.md` §3.2, §4.4 — Smith/STK formulation transcribed in repo.
- `research/O-Contrabass-bass-waveguide-stability.md` — bass-impedance / dispersion / denormal context.
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:100-155` — validated split-rail reference implementation.
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:115-186` — current single-rail implementation (the suspect file).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp:225-229` — current `betaScale` fudge (R5 target).

---

# 11. Re-Research After R1 Pre-Flight FAIL (2026-04-26 second pass)

**Trigger:** PLAN rev-2 R1 pre-flight diagnostic — single-rail with `2.0f * newVelocity` injection — produced peak −32.6 dBFS, `rmsMid_s5_s6` = 2.23e-8, `rmsFinal_lastSecond` = 0.0. Statistically indistinguishable from rev-1 baseline; doubling injection lifted the cold-start transient by ~6 dB but did not establish steady-state Helmholtz. Per PLAN rev-2 R1 fail-action, R2–R5 (split-rail rewrite, betaScale removal, Gate 1 rerun, atomic commit) NOT executed. Diagnostic edit reverted. PLAN rev-2 superseded.

**Falsified hypothesis (from §10):** "Single-rail vs split-rail differs only in per-round-trip injection energy budget at the fundamental, so single-rail with 2× compensation is mathematically equivalent to split-rail at f0 = 41.2 Hz." This is **wrong**. Single-rail with 2× compensation matches split-rail in *steady-state mode amplitudes for a given driving signal*, but does NOT match split-rail's ability to bootstrap from cold-start sticking equilibrium because there is no spatial asymmetry to break the equilibrium. See §11.1.

This section overrides §10's R1 pre-flight gating (the gate was a flawed test) but PRESERVES §10's split-rail conclusion (now upgraded from "primary fix" to "primary structural fix, necessary but no longer claimed sufficient on its own"). It also identifies two additional bugs in the pre-rev-1 source tree that compound with single-rail and persist even if split-rail is adopted.

---

## 11.1 Root-Cause Analysis: Three Compounding Bugs

The harness failure is not a single bug. It is the superposition of three independent problems that each contribute to the observation "string excited briefly during attack but never reaches steady-state Helmholtz."

### B1 — Single-rail topology cannot bootstrap Helmholtz from sticking equilibrium

**Severity:** Primary. Structural. Cannot be patched within single-rail.

**Mechanism (sample-by-sample trace at default bass operating point, INFINITE_SUSTAIN=1.0, F_bow=1.0, μ_s=0.85, μ_d=0.25, R_s=0.5, β=0.10, v_bow envelope settled at 0.114):**

Compute the sticking equilibrium analytically. In sticking, `injection = v_delta` exactly (string follows bow). So the outgoing wave at the friction junction is:

```
x = incoming + injection
  = incoming + (v_bow − v_string_incoming)
  = incoming + v_bow − incoming        // since v_string_incoming = incoming in current code
  = v_bow                              // CONSTANT, independent of incoming
```

Through the bridge LP, saturator, DC blocker chain, the OUTPUT pushed to the delay line in steady state is:

- LP DC gain (with current buggy form, see B2): `−g·(1−p)/(1−g·p) ≈ −0.99990` at g=0.99995, p=0.5
- LP output `y → −0.99990 · v_bow ≈ −0.114` (constant)
- Saturator (linear at this magnitude): sat ≈ −0.114
- DC blocker, given constant input: `dcOut(n) = sat(n) − sat(n−1) + R · dcOut(n−1)` → with sat constant, the difference term is 0, and dcOut decays as `R^N` toward 0 with time constant ~1000 samples (~11 ms at 88.2 kHz)

After ~3 DCB time constants (~3000 samples ≈ 34 ms), pushed value → 0. Once the entire 2141-sample delay loop is filled with zeros (≈ 50 ms after onset), `incoming = 0` permanently. v_delta = v_bow = 0.114. injection = 0.114. x = 0 + 0.114 = 0.114 = v_bow. Output pushed = 0. The system has reached a **stable silent equilibrium**.

**Why split-rail breaks this equilibrium and single-rail cannot:** in split-rail, the bow point sees the SUM of two distinct returning waves (`v_string_incoming = bridgeReflection + nutReflection`). The two reflections have different round-trip periods (`bridgeSamples = β·T` vs `neckSamples = (1−β)·T`) and accumulate asymmetric energy from each round-trip's `toBridge = nutReflection + newVelocity` and `toNeck = bridgeReflection + newVelocity` push pattern. This asymmetric accumulation creates a **non-DC standing wave** in the delay lines — the Helmholtz corner — which modulates `v_string_incoming` periodically. Once the periodic modulation grows past the slip threshold `frictionVelocity = 0.5·μ·F_bow/R_s ≈ 0.30–0.85` (depending on instantaneous μ), the bow slips, releasing energy that grows the standing wave further, locking into period-T Helmholtz oscillation.

In single-rail, no such spatial-asymmetric mechanism exists. The "incoming" wave is a single delayed copy of the outgoing wave; sticking pins outgoing to v_bow and the loop converges to silence regardless of injection scaling. Doubling `newVelocity` (the rev-2 R1 pre-flight) merely raises the cold-start transient amplitude (which is what the +6 dB peak measurement showed) but does NOT change the equilibrium structure: x is still pinned to `incoming + (v_bow − v_string_incoming) + extra_injection_term`, and in steady-state sticking the extra term is also bounded by v_delta → no growth → silence.

**This is the dominant cause of the harness failure.** Hypothesis H6 (first-tick envelope timing, flagged as "strongest candidate" in `SUMMARY.md`) is dissolved — the observed transient peak IS the bow envelope's first 50–100 ms of injection before DC-blocker decay zeros the output; the issue is not envelope timing, it is that single-rail cannot CONVERT the injection into oscillation.

**Reference:** Smith, *Physical Audio Signal Processing*, "Bow-String Scattering Junction" (cited §10.1.1) — the canonical formulation explicitly requires two-port (split-rail) topology for Helmholtz bootstrapping. The cited equations show that `v_sr⁻ = v_sl⁺ + ρ·v_Δ` and `v_sl⁻ = v_sr⁺ + ρ·v_Δ` are NOT equivalent to a single recurrence `v⁻ = v⁺ + 2·ρ·v_Δ` because the spatial role of the two rails (one terminating at the bridge boundary, the other at the nut boundary) is what creates the period-T standing wave geometry.

### B2 — Bridge LP recursion erroneously multiplies `g` into the feedback term

**Severity:** Secondary. Compounds with B3. Independently breakable.

**Location:** `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:162`

```cpp
float y = bridgeG * (bridgeOneMinusP * lpInput + bridgeP * bridgeY) + denormalLeak;
```

This expands to: `y[n] = g·(1−p)·x[n] + g·p·y[n−1] + leak`

Transfer function (ignoring leak): `Y(z)/X(z) = g·(1−p) / (1 − g·p·z⁻¹)`

- **Pole:** `g·p` (NOT `p` as the canonical bridge LP intends)
- **DC gain:** `g·(1−p)/(1−g·p)`. At g=0.99995, p=0.5: `0.499975/0.500025 ≈ 0.99990` — **almost unity**.

**Compare to O-Bowed canonical** (`plugins/O-Bowed/Source/DSP/WaveguideString.cpp:94-95`):

```cpp
*bridgeLossFilter.coefficients = juce::dsp::IIR::Coefficients<float> (
    g * (1.0f - p), 0.0f, 1.0f, -p);
```

Coefficients `(b0, b1, a0, a1) = (g·(1−p), 0, 1, −p)` give: `H(z) = g·(1−p) / (1 − p·z⁻¹)`

- **Pole:** `p` (independent of g — the LP shape is invariant under loop-gain changes)
- **DC gain:** `g·(1−p)/(1−p) = g` — **exactly the loop gain**, which is what the architecture intends ("g — loop gain" per `WaveguideString.cpp:97` comment)

**Equivalent O-Bowed-form recurrence:** `y[n] = g·(1−p)·x[n] + p·y[n−1]` (note: `p`, not `g·p`, multiplying the feedback)

**Consequences of the bug:**

1. **Bridge LP fails to attenuate DC by `g` per round trip.** At drone-mode g (0.99995), DC gain is 0.99990 ≈ 1. So once a sticking-regime DC offset enters the loop, the bridge LP does NOT attenuate it. This is what motivated the in-loop DCB (B3) — but the DCB is the wrong fix; the right fix is to correct the LP transfer function.

2. **LP cutoff frequency drifts with INFINITE_SUSTAIN.** Pole at `g·p` shifts as g changes. At low g (0.997), pole = 0.4985; at high g (0.99995), pole = 0.49998. Audibly negligible (cutoff drift ~0.003 Hz at sr_int=88.2 kHz). Not user-visible, but conceptually wrong.

3. **The architecture's "g = 0.997 → 0.99995" mapping no longer corresponds to literal loop gain.** Loop gain = LP DC gain = `g·(1−p)/(1−g·p)`, not `g`. The `INFINITE_SUSTAIN` curve is therefore mis-calibrated by ~0.05 % at max sustain (effective gain 0.99990 vs intended 0.99995 → halving the steady-state amplitude per ~7000 round trips ≈ 170 s). For 60 s test this is invisible, but for true infinite-sustain drone the calibration matters.

**Fix:** rewrite the recurrence as `y = g·(1−p)·x + p·y_prev + leak` (drop the `g` from the feedback term). One-line change.

**Sources:**
- O-Bowed canonical: `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:82-98`
- Smith, *Physical Audio Signal Processing*, "Lossy Waveguide Filter Design" — DC gain of a one-pole loss filter equals the loop gain; the pole locates the cutoff independently.
- Karplus & Strong (1983), "Digital Synthesis of Plucked-String and Drum Timbres", CMJ 7(2): the lossy filter's DC magnitude IS the loop attenuation by definition.

### B3 — In-loop DC blocker actively suppresses cold-start sticking-regime injection

**Severity:** Secondary. Caused by B2's mis-calibration; obviated by B2 fix.

**Location:** `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:171-173`

```cpp
float dcOut = saturated - dcX1 + kDCBlockerR * dcY1;
dcX1 = saturated;
dcY1 = dcOut;
```

This is the architecture-mandated `H(z) = (1 − z⁻¹)/(1 − R·z⁻¹)` with R=0.999. Time constant ≈ 1/(1−R) = 1000 samples ≈ 11 ms at 88.2 kHz internal SR.

**Mechanism:** As shown in B1's sticking-equilibrium trace, the saturator output in sticking is approximately constant at `−v_bow ≈ −0.114`. The DCB converts this constant into an exponentially decaying transient. After ~3000 samples (34 ms), pushed value → 0. The delay line fills with zeros within one round-trip period (~50 ms), and the friction junction sees `incoming = 0` permanently → trapped in sticking → silence.

The DCB was added to ARCHITECTURE.md to handle DC accumulation that B2's broken bridge LP cannot prevent. With B2 fixed (LP DC gain = g, attenuating DC by `1−g` per round trip), the loop's natural DC handling is sufficient for the 60 s sustain test (cumulative DC attenuation = `g^N` round trips = `0.99995^2472` ≈ 0.884 over 60 s — slow but nonzero). For TRUE drone-mode (sustain hours, not seconds), additional DC handling may be needed — but it should be at the OUTPUT path post-waveguide, not in-loop, where it cannot interfere with bootstrapping.

**Fix:** REMOVE the in-loop DCB. If DC drift becomes an issue at long-form drone use, re-add a DCB at the per-voice output path (after the saturator final guard at `BowedContrabassVoice.cpp:181`), where it cannot interfere with the loop's bootstrapping dynamics. This is a deviation from `ARCHITECTURE.md §"DC Blocker"` line 92–99 — see §11.6 for justification.

**Sources:**
- Smith, *Physical Audio Signal Processing*, "DC Blocker" — explicitly notes that in-loop DC blockers in feedback systems with strong nonlinearities can suppress non-DC oscillation modes if they overlap the DCB's transition band; standard practice is to place the DCB at the system output.
- O-Bowed empirical: `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` has NO in-loop DCB and works at all gain levels up to its 0.9995 cap.

---

## 11.2 Hypothesis Triage (revisit per `SUMMARY.md` priority order)

| # | Hypothesis | Status | Verdict |
|---|---|---|---|
| H6 | First-tick envelope timing | **Dissolved** | Bow envelope at vel=0.7 reaches v_bow_target within ~100 ms (attackCoeff = 6.13e-4 at sr_int=88.2 kHz; 1−exp(−1) at sample N=1632; `BowedContrabassVoice.cpp:124-167`). Per-sample envelope is non-zero from sample 1 onward. The observed transient peak corresponds to envelope ramp-up + DCB transient response — not a "first-sample is zero" bug. **Not the cause.** |
| H1 | Sign-convention transcription error | **Disproven** | Round-trip sign closure is correct in current single-rail. O-Bowed applies the rigid-bridge `−1` sign flip on the INCOMING wave (after the LP, line 108: `bridgeReflection = -bridgeLossFilter.processSample(...)`). O-Contrabass applies `−1` on the OUTGOING wave (before the LP, line 161: `lpInput = −x`). Both yield the same per-round-trip transfer function `−g·(1−p)/(1−p·z⁻¹)·z⁻ᵀ` (correct form) or `−g·(1−p)/(1−g·p·z⁻¹)·z⁻ᵀ` (B2-buggy form). The difference is conventional, not behavioural. **Not the cause.** |
| H2 | DC blocker round-trip energy at f0=41.2 Hz | **Reframed → B3** | §10.2.1 computed −0.48 dB per round trip at 41.2 Hz. That number describes steady-state attenuation of an established AC oscillation, which in steady state is balanced by injection. The actual DCB pathology is **transient**: it zeros the constant sticking-regime output during cold-start, preventing Helmholtz bootstrapping. See B3. **Reframed: contributing cause via cold-start mechanism, not via steady-state cumulative attenuation.** |
| H3 | Saturator dynamics | **Disproven** | At peak −32.6 dBFS (= 0.0235), the algebraic saturator `x/√(1+x²)` is essentially linear (small-signal expansion `y ≈ x − x³/2 + …` gives gain = 0.9997 at \|x\|=0.0235). Cannot be bounding any signal at the failure operating point. **Not the cause.** Same conclusion as §10.3. |
| H4 | BowModel attack envelope at 2× sample rate | **Disproven** | `BowModel.cpp:20, 37` use `static_cast<float>(sampleRate)` directly; `BowedContrabassVoice.cpp:117` calls `bowModel.prepare(spec_at_2x.sampleRate)` = `hostSampleRate * 2.0` = 88200 Hz. attackCoeff and releaseCoeff are then computed against the internal SR — sample-rate-correct. **Not the cause.** |
| H5 | Friction defaults at bass operating point vs Schelleng F_min | **Reframed → B1** | §10.4 computed F_min ≈ 1.92 N at bass defaults vs F_bow=1.0 default — placing the operating point ~2× below Schelleng minimum in normalized units. §10.4 then claimed split-rail (which doubles effective injection) recovers this. The reframing in §11.1 is sharper: even if split-rail eliminated the 2× efficiency penalty, the default F_bow target is at the boundary of (or just inside) the Helmholtz wedge — Helmholtz bootstrap will be SLOW and possibly fragile at default knob position. With split-rail, the harness should still PASS at default settings within a few seconds, but Phase 2.4's 108-combo stability matrix may surface edge cases (especially low-Sustain × low-Pressure × high-Stiffness corners). **Reframed: latent issue inside split-rail's playable wedge, not the immediate cause of the rev-2 R1 fail.** |

**Net new finding (B2):** The bridge LP recursion is mis-implemented. This is a separate bug from B1 (topology) and B3 (DCB), discovered during the line-by-line comparison against O-Bowed canonical that §11.1 required. Was not surfaced in §10's hypothesis list because §10 focused on `WaveguideString.cpp:144` (the friction-injection line) and `WaveguideString.cpp:161` (the `lpInput = -x` sign-flip), not `WaveguideString.cpp:162` (the LP recurrence itself).

---

## 11.3 Recommended Fix (locked decision)

PLAN rev-3 must apply ALL THREE fixes simultaneously. Removing only one of the three (e.g. just split-rail, just LP fix, just drop DCB) will leave one or more of the three failure modes intact.

| ID | Fix | Action | LOC delta |
|---|---|---|---|
| **F1** | Split-rail topology (formerly §10 R1) | Two `juce::dsp::DelayLine<float, Lagrange3rd>` rails (bridgeDelay + neckDelay), each 8192 samples. β splits the total delay (`bridgeSamples = β·T`, `neckSamples = (1−β)·T`). Mirror O-Bowed's `processSample` structure. | ~+45 LOC `WaveguideString.{h,cpp}` |
| **F2** | Bridge LP recurrence fix (B2) | Drop `g` from the feedback term: `y = g·(1−p)·lpInput + p·bridgeY + leak`. Apply per rail in split-rail (bridge-rail only — nut-rail uses pure `−1` boundary, no LP). | 1-line change inside `processSample` |
| **F3** | Remove in-loop DC blocker (B3) | Drop the `dcX1 / dcY1 / kDCBlockerR` block from `WaveguideString.{h,cpp}`. If DC handling is needed at sustained drone, re-add a DCB at the per-voice output path (`BowedContrabassVoice::renderNextBlock` after `processSamplesDown`). For Phase 2.1, leave output-path DCB OUT and revisit if Phase 2.4's 108-combo matrix surfaces DC drift. | ~−6 LOC `WaveguideString.{h,cpp}`, ~−2 state members in `.h` |
| **F4** | Drop the `betaScale` fudge in voice (formerly §10 R5) | Remove `BowedContrabassVoice.cpp:228-229`. Once split-rail makes β a real spatial split, `frictionModel.setStringImpedance` retains its prepare-time default `R_s = 0.5`. | ~−2 LOC `BowedContrabassVoice.cpp` |

**Net code delta from current rev-1 working tree:** ~+38 LOC `WaveguideString.{h,cpp}`, ~−4 LOC `BowedContrabassVoice.cpp`. Total file size of `WaveguideString.cpp` expected to be ~190 LOC after fix (down from 232 today, because removing DCB net-deletes more than split-rail adds).

**Files NOT affected:** `BowModel.{h,cpp}`, `HyperbolicFriction.h`, `OContrabassMPESynthesiser.h`, `PluginProcessor.{h,cpp}`, `tests/render-harness/main.cpp`, `CMakeLists.txt`. The render harness re-runs unchanged; expected to PASS (peak ≤ 1.0, no NaN/Inf, `rmsRatio_final_over_mid` in [0.5, 2.0], `blockTime_max_over_median` ≤ 5×).

---

## 11.4 Code-Level Fix Sketch (per file)

### `Source/DSP/WaveguideString.h`

```cpp
// REPLACE the single delayLine member with two rails:
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> bridgeDelay { 8192 };
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> neckDelay   { 8192 };

// REMOVE these members entirely (DCB no longer in-loop):
//   float dcX1 = 0.0f;
//   float dcY1 = 0.0f;
//   static constexpr float kDCBlockerR = 0.999f;

// RENAME (for clarity, optional):
//   updateDelayLength()  →  updateDelayLengths()
//   setDelaySamples()    →  setDelaySamples (samples_bridge, samples_neck)
```

### `Source/DSP/WaveguideString.cpp::processSample` — split-rail rewrite

```cpp
float WaveguideString::processSample (float v_bow, float F_bow,
                                       const HyperbolicFriction& friction)
{
    if (filterDirty)
        updateBridgeFilterCoeffs();

    // Read both rails. Bridge-side returning wave goes through the bridge LP
    // and rigid-boundary -1 flip; nut-side just reflects with -1 (rigid nut).
    float bridgeRaw = bridgeDelay.popSample (0);
    float neckRaw   = neckDelay.popSample (0);

    // Bridge LP — F2 fixed form: y = g*(1-p)*x + p*y_prev + leak.
    // Note `p` (not `g*p`) on the feedback term — DC gain == g exactly.
    if (! std::isfinite (bridgeY))
        bridgeY = 0.0f;
    float bridgeFiltered = bridgeG * bridgeOneMinusP * bridgeRaw
                         + bridgeP * bridgeY
                         + denormalLeak;
    bridgeY = bridgeFiltered;

    float bridgeReflection = -bridgeFiltered;   // -1 boundary AFTER LP (matches O-Bowed)
    float nutReflection    = -neckRaw;          // -1 boundary, no LP (rigid nut)

    // Sum at bow point (split-rail v_string_incoming).
    float v_string_incoming = bridgeReflection + nutReflection;
    float v_delta = v_bow - v_string_incoming;

    // Friction (unchanged from current rev-1).
    float rho = friction.computeReflectionCoefficient (v_delta, F_bow);
    float clampedRho = std::min (rho, 0.85f);
    float frictionVelocity = 2.0f * clampedRho / (1.0f - clampedRho);
    float absVd = std::abs (v_delta);
    float injection = std::min (frictionVelocity, absVd);
    float newVelocity = (v_delta >= 0.0f) ? injection : -injection;

    // Symmetric injection into both rails (canonical Smith two-port pattern).
    // [Phase 2.1c: dispersion will be inserted on the BRIDGE rail's outgoing
    //  wave only, BEFORE the algebraic saturator — matches ARCHITECTURE
    //  §13.b loop ordering, applied per the bridge rail.]
    float toBridge = nutReflection + newVelocity;
    float toNeck   = bridgeReflection + newVelocity;

    // In-loop algebraic saturator on each rail (RESEARCH §1.3, ARCHITECTURE).
    // Matches O-Bowed pattern but with algebraic form instead of tanh.
    toBridge = toBridge / std::sqrt (1.0f + toBridge * toBridge);
    toNeck   = toNeck   / std::sqrt (1.0f + toNeck   * toNeck);

    // F3: NO DC blocker in the loop. (If long-form drone needs DC handling,
    // re-add one at BowedContrabassVoice::renderNextBlock post-down-sample.)

    bridgeDelay.pushSample (0, toBridge);
    neckDelay.pushSample   (0, toNeck);

    // Output from the bridge end (matches O-Bowed `output = toBridge`).
    float output = toBridge;
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);
    return output;
}
```

### `Source/DSP/WaveguideString.cpp::updateDelayLengths`

```cpp
void WaveguideString::updateDelayLengths()
{
    float totalDelay = static_cast<float> (sampleRate) / std::max (1.0f, currentFrequency);
    float pi = juce::MathConstants<float>::pi;
    float filterGroupDelay = static_cast<float> (sampleRate) / (2.0f * pi * std::max (1.0f, brightnessHz));
    float compensated = totalDelay - filterGroupDelay;

    float bridgeSamples = compensated * bowPosition;
    float neckSamples   = compensated * (1.0f - bowPosition);

    // Lagrange3rd minimum = 4 samples per rail.
    bridgeSamples = juce::jlimit (4.0f, 8190.0f, bridgeSamples);
    neckSamples   = juce::jlimit (4.0f, 8190.0f, neckSamples);

    bridgeDelay.setDelay (bridgeSamples);
    neckDelay.setDelay   (neckSamples);
}
```

`setBowPosition` now calls `updateDelayLengths()` (matches O-Bowed `setBowPosition` at `WaveguideString.cpp:157-164`).

### `Source/BowedContrabassVoice.cpp` — drop `betaScale` fudge

```cpp
// REMOVE lines 225-229 entirely (the betaScale block + setStringImpedance call).
// frictionModel.setStringImpedance retains its constructor-default R_s = 0.5.
// waveguideString.setBowPosition(effectivePosition) below already handles β
// as a real spatial split.
```

---

## 11.5 Verification Approach

### V1 — render-harness rerun (Gate 1)

After F1+F2+F3+F4 land, re-run `O-Contrabass-render-test --note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0 --out /tmp/r3.wav --json /tmp/r3.json`.

**Expected pass conditions** (per `tests/render-harness/main.cpp:217-232`):
- `pass_nan = true` (no NaN/Inf samples)
- `pass_peak = true` (peak ≤ 1.0)
- `pass_blockTime = true` (max-block-time ratio ≤ 5×)
- `pass_rms = true` (`rmsFinal/rmsMid` in [0.5, 2.0]) — **the gate that's been failing**
- Expected `rmsMid_s5_s6` ≈ 0.05–0.20 (steady-state Helmholtz at v_bow=0.114, output normalisation 0.35)
- Expected `peak` ≈ 0.20–0.40 (Helmholtz corner amplitude, before output saturator hard-clip kicks in)

### V2 — instrumented diagnostic mode (optional, defensive)

If V1 fails, add a TEMPORARY logging path inside `WaveguideString::processSample` (gated by a `#define DEBUG_WAVEGUIDE_FIRST_N 5000` macro) that writes `v_bow, F_bow, v_string_incoming, v_delta, rho, frictionVelocity, |v_delta|/frictionVelocity, newVelocity, toBridge, toNeck` to a CSV for the first 5000 samples. Plot the |v_delta|/frictionVelocity ratio — Helmholtz bootstrap is signalled by this ratio crossing 1.0 (slip event) within the first 1–3 round-trip periods (≈ 25–75 ms at f0=41.2 Hz).

This is an INSTRUMENTATION HOOK, NOT a runtime path — guarded by `#ifdef` and stripped from Release builds. Add it only if V1 fails. PLAN rev-3 should NOT include V2 unless V1 fails post-fix.

### V3 — auval + pluginval re-validation (Gate 2)

After V1 PASS, re-run `auval -v aumu OCbs OuDv` and `pluginval --strictness-level 10`. Both must remain PASS (they passed under rev-1 single-rail; the topology change should not introduce new validator regressions).

---

## 11.6 Why This Is NOT a CONTEXT.md / ARCHITECTURE.md Violation

`CONTEXT.md §"Cycle Scope 2.1a"` line 30: *"Single `juce::dsp::DelayLine<float, Lagrange3rd>` (8192 samples) for E1 (41.2 Hz)."* — same advisory implementation guess that was already overridden in §10.7. F1 (split-rail) is the same deviation §10 argued for; §11 reaffirms it.

`ARCHITECTURE.md §"DC Blocker"` lines 92–99: mandates an in-loop DCB. **F3 deviates from this contract.** Justification:

1. The architectural DCB requirement was motivated by B2 (broken bridge LP DC gain ≈ 1 instead of g). With B2 fixed (F2), the bridge LP correctly attenuates DC by `1−g` per round trip; a redundant in-loop DCB then has only DOWNSIDE (suppresses cold-start bootstrapping per B3 mechanism) with no upside.
2. ARCHITECTURE.md §"DC Blocker" cites no specific pathology that an in-loop DCB solves beyond "subharmonic accumulation under high feedback gain" — which is precisely what the bridge LP's `g`-DC-gain handles when implemented correctly.
3. Phase 2.1's 60 s sustain test cannot detect DC drift longer than 60 s. If Phase 2.4's 108-combo stability matrix or Phase 2.5's body-bank coupling later surfaces a real DC-drift pathology that a CORRECT bridge LP cannot handle, an OUTPUT-PATH DCB (post-waveguide, in `BowedContrabassVoice::renderNextBlock`) can be added then. That placement does not interfere with bootstrapping.

**ARCHITECTURE.md amendment recommendation:** PLAN rev-3 preamble flags this as a deviation. ARCHITECTURE.md should be updated post-Phase-2.1-verify to reflect the correct LP form (B2 fix) and the output-path DCB option. The architecture's intent (bounded loop gain, no DC drift in long-form drone) is preserved — only the implementation mechanism changes.

`ARCHITECTURE.md §"In-loop saturator"` (algebraic `x/√(1+x²)` after LP): preserved, applied per rail in F1's split-rail rewrite. No deviation.

`ARCHITECTURE.md §"Bridge Filter (One-Pole Lowpass + Loop Gain)"` (g range, p clamp, quadratic skew): preserved. F2 only fixes the recurrence implementation; the coefficient computation in `updateBridgeFilterCoeffs` is unchanged.

`ARCHITECTURE.md §"Constant denormal leak"`: preserved. The `denormalLeak` member and gating logic carry over unchanged into the split-rail bridge LP (applied to the bridge rail only; nut rail has no LP and no leak — it's a pure delay + sign flip).

`parameter-spec.md` (sha256:c47fe736…): unaffected. All 29 APVTS parameter IDs / ranges / defaults preserved.

`ROADMAP.md` (sha256:106639f6…): unaffected. Phase 2.1 description silent on rail count, DCB placement, and LP recurrence form.

---

## 11.7 Summary

**One-sentence root cause:** Single-rail topology (B1) cannot bootstrap Helmholtz from sticking equilibrium because the friction junction has no spatial-asymmetric returning wave to perturb v_delta past the slip threshold; the bridge LP's mis-implemented recurrence (B2) eliminates the natural DC attenuation that would normally compensate; and the in-loop DC blocker (B3), added to ARCHITECTURE.md as a workaround for B2, actively suppresses the cold-start sticking-regime injection that would otherwise charge the delay lines.

**One-sentence fix:** PLAN rev-3 adopts split-rail (F1, formerly §10 R1), corrects the bridge LP recurrence to `y = g·(1−p)·x + p·y_prev + leak` (F2, drop `g` from feedback), removes the in-loop DC blocker (F3, deviation from ARCHITECTURE.md justified by F2 fix), and drops the `betaScale` fudge in voice (F4, formerly §10 R5).

**Hypothesis-by-hypothesis disposition:**

| Hypothesis (per `SUMMARY.md`) | Status |
|---|---|
| H6 — First-tick envelope timing | **Dissolved.** Envelope behaves correctly; the observed transient peak is the cold-start friction injection + DCB transient response combination, not an envelope timing bug. |
| H1 — Sign-convention transcription | **Disproven.** Round-trip sign closure is correct in current code; sign-flip placement convention differs from O-Bowed but is mathematically equivalent. |
| H2 — DC blocker round-trip energy | **Reframed → B3.** §10.2.1's steady-state attenuation analysis was correct but irrelevant; the actual DCB pathology is its TRANSIENT response zeroing the cold-start sticking-regime DC injection. |
| H3 — Saturator dynamics | **Disproven.** Linear at the −32.6 dBFS failure operating point; cannot bound any signal. |
| H4 — BowModel sr-correctness | **Disproven.** `prepare(spec_at_2x.sampleRate)` is called correctly; envelope coefficients computed against internal SR. |
| H5 — Friction defaults vs Schelleng | **Reframed → B1 + latent.** Default F_bow=1.0 sits near the Schelleng F_min boundary in normalized units; split-rail (F1) is necessary but Phase 2.4's 108-combo matrix may surface edge cases at low-Pressure × low-Sustain corners. Latent — track in Phase 2.4 risks. |
| **B2 — Bridge LP feedback gain bug** | **NEW finding.** Discovered during line-by-line comparison against O-Bowed canonical at `WaveguideString.cpp:162`. Independent of B1 and B3 but motivates B3's existence. |

**PLAN rev-3 deliverable:** rewrite `WaveguideString.{h,cpp}` to (a) split-rail (~+45 LOC), (b) corrected bridge LP recurrence (1-line change), (c) drop in-loop DCB (~−6 LOC, ~−2 .h state members). Drop `betaScale` block in `BowedContrabassVoice.cpp` (~−2 LOC). Re-run render harness for Gate 1 PASS, auval + pluginval level-10 for Gate 2 PASS, atomic commit, hand off to Phase 2.1b (module extraction, unchanged from rev-1/rev-2 plan).

**Why this is NOT a third re-research after a third failure (i.e. why this should converge):**
- Rev-1 RESEARCH §10 hypothesised single-rail energy budget as primary cause; PLAN rev-2 R1 pre-flight tested only the energy-scaling sub-hypothesis (single-rail + 2× injection) and falsified it.
- Rev-2 R1 FAIL is consistent with the structural-asymmetry argument in §11.1: any per-sample scalar tweak to single-rail injection cannot create the spatial geometry Helmholtz requires.
- The B2 discovery in §11 is a NEW signal that wasn't visible during §10 analysis (which focused on `WaveguideString.cpp:144` injection line, not `WaveguideString.cpp:162` LP recurrence line). With B2 identified and fixed, the architectural justification for B3's in-loop DCB collapses, and B3 can be cleanly removed.
- All three fixes (F1, F2, F3) target distinct, independently-falsifiable mechanisms. F1 fixes the topology; the harness will detect topology-related Helmholtz failure (RMS=0). F2 fixes DC-gain calibration; the harness will detect long-form drone DC drift (sustain ratio < 0.5 over 60 s). F3 enables bootstrapping; the harness will detect bootstrapping failure (RMS=0 in seconds 5–6).
- If V1 (post-F1+F2+F3+F4 harness rerun) still fails, the V2 instrumentation hook will emit per-sample friction-junction state across the first 5000 samples — turning the failure mode from "silent steady-state" into a directly observable trace of v_delta vs frictionVelocity, which uniquely identifies the remaining bug class.

---

## 11.8 References (incremental, beyond §10.10)

**Primary citations (new in §11):**
- Smith, J.O. (online). *Physical Audio Signal Processing*, "Lossy Waveguide Filter Design" — DC gain of a one-pole loss filter equals the loop attenuation; supports B2 fix.
- Smith, J.O. (online). *Physical Audio Signal Processing*, "DC Blocker" — placement guidance for DCBs in feedback systems with strong nonlinearities; supports B3 fix.
- Karplus, K. & Strong, A. (1983). "Digital Synthesis of Plucked-String and Drum Timbres." Computer Music Journal 7(2). Original lossy-filter formulation showing DC magnitude = loop attenuation.

**Repo references (new in §11):**
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:94-95` — canonical bridge LP coefficient form `(g·(1−p), 0, 1, −p)` confirming F2 fix.
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:108-109` — sign-flip placement on incoming wave (after LP); informs F1 split-rail rewrite.
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:131-142` — symmetric injection pattern for split-rail; F1 mirrors this with algebraic saturator substituted for tanh per ARCHITECTURE.md §"In-loop saturator".
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:162` — current B2-buggy LP recurrence (the F2 target).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:171-173` — current B3 in-loop DCB (the F3 deletion target).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp:225-229` — `betaScale` fudge (F4 deletion target, same as §10 R5).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — rev-2 R1 pre-flight FAIL data and six-hypothesis priority order.
- `plugins/O-Contrabass/.planning/stages/2-dsp/CHECKPOINT-2.1a.md` — rev-1 baseline failure observation.

---

# 12. Phase 2.4 Follow-Up: In-Loop Saturator Tail Dissipation (rev-2 append)

**Date appended:** 2026-04-26 (research-phase rev-2)
**Cross-reference:** `CONTEXT.md` rev-2 §"Saturator-tail follow-up parking" + `SUMMARY.md` lines 17–21, 162–174.

This section captures the analytical envelope estimate for the in-loop algebraic saturator's free-decay rate, parked as a Phase 2.4 follow-up per the rev-2 Option-A decision. It is not a fix or a re-research — it documents the phenomenon precisely enough that Phase 2.4's 108-combo stability matrix can re-evaluate without re-deriving the math from scratch.

## 12.1 Phenomenon Recap

After the rev-3 fix (F1 split-rail + F2 bridge LP + F3 DCB removal + F4 betaScale removal), the standard rev-3 render-harness call (`--sustain 60 --release 5`) reports `pass_rms = false`:

| Window | RMS | Notes |
|---|---|---|
| seconds 5–6 (sustained, bow on) | 0.0353 | Above the rev-3 floor 1e-3, lower edge of expected 0.05–0.20 band |
| final 1 s (4–5 s after bow-off) | ≈ 0.0 (below FP precision) | Decayed substantially |
| `rmsRatio = rmsFinal / rmsMid` | < 0.5 | Below the rev-3 PLAN's `pass_rms` threshold |

The bow-on-only validation harness (`--sustain 65 --release 0`) reports 4/4 invariants TRUE with `rmsRatio = 1.04`, byte-identical reproducibility verified at verify-phase. Helmholtz bootstrapping is achieved; sustained-state energy is stable.

The standard `pass_rms` failure is therefore localized to the post-bow-off tail, not the sustained-state region the rev-3 fixes were designed to demonstrate.

## 12.2 Analytical Derivation

The in-loop algebraic saturator (`ARCHITECTURE.md §"String Waveguide Bank"`, RESEARCH §1.3) is `y = x / sqrt(1 + x²)`. Taylor expansion at x = 0:

```
y(x) = x − (1/2)·x³ + (3/8)·x⁵ − ...
```

For `|x| ≪ 1`, the relative attenuation per saturator pass is:

```
(x − y) / x = (1/2)·x² − O(x⁴) ≈ x² / 2
```

Per round-trip on the split-rail topology (PLAN rev-3 §"Locked decision"), the saturator is applied **per-rail on the WRITE path** — once on `toBridge`, once on `toNeck` — so each round-trip incurs **two saturator passes**. (See `SUMMARY.md` lines 56–59 for the canonical loop sequence.)

E1 fundamental: f₀ = 41.20 Hz → 41.20 round-trips per second on the waveguide.

Per-second amplitude attenuation envelope, treating the in-loop signal as low-amplitude oscillation with peak |x|:

```
attenuation_rate ≈ (x² / 2) × 2 rails × 41.2 RTs/s
                 = x² × 41.2 /s
```

Worked example matching the harness data (`SUMMARY.md` line 169):
- |x| ≈ 0.049 → x² / 2 ≈ 0.12 %
- 0.12 % × 2 rails × 41.2 RTs/s ≈ 9.9 %/s
- Over 4 s of bow-off: cumulative attenuation = 1 − (0.901)⁴ ≈ 33 %, RMS reduction ≈ 1 − 0.901⁴ ≈ 33 % per amplitude (or ~55 % per energy depending on which window the harness measures) — within the order of magnitude of the JSON-observed `rmsFinal_lastSecond ≈ 0`.

**Caveat:** the constant 41.2 is f₀-specific. At higher pitches the rate scales linearly with f₀ (more round-trips per second → more saturator passes per second → faster decay). Phase 2.4 must validate this scaling against the per-string fundamentals — the scaling alone could explain why `pass_rms` is more sensitive on high strings than on E1.

## 12.3 Why This Was Not Caught Earlier

The rev-3 PLAN's pass-bar (`pass_rms = rmsFinal / rmsMid ∈ [0.5, 2.0]`) was specified before the per-rail in-loop saturator's low-amplitude dissipation was characterised. The standard `--release 5` harness call measures decay during the bow-off tail, where the saturator's cubic-loss dominates. The pass-bar implicitly assumed the saturator is approximately linear at low amplitudes (i.e. that `pass_rms` measures only the bridge-LP loss), which is *quantitatively* incorrect for x ≈ 0.05.

This is consistent with §11's framing of B2/B3 being LP-and-DCB issues, not saturator issues — the saturator was correctly identified in §10.3 as the component that bounds steady-state amplitude, but its low-amplitude dissipation rate during free-decay was not derived analytically until SUMMARY's six-hypothesis priority pass.

## 12.4 Comparison Candidates for Phase 2.4

Phase 2.4 must decide whether the current `x / sqrt(1 + x²)` saturator stays as the architectural choice or whether ARCHITECTURE.md §"In-loop saturator" requires an amendment. Candidate comparisons:

| Candidate | Cubic loss factor | Notes |
|---|---|---|
| `x / sqrt(1 + x²)` (current) | `x² / 2` | Algebraic, cheapest. Leading order matches `tanh(x)` near zero. |
| `4 · tanh(x / 4)` (O-Bowed) | `x² / 48` | tanh(u) = u − u³/3 + ...; with u = x/4, cubic term is `−x³/192`, scaled back: cubic loss `≈ x² / 48`. ~24× weaker than algebraic. |
| `tanh(x)` | `x² / 3` | Stronger low-amplitude dissipation. Not used by either plugin. |
| `clip(x, ±1)` | 0 (linear in |x|<1) | No cubic loss; loop gain handled entirely by bridge LP. Asymmetric clipping artefacts at peaks. |

`SUMMARY.md` Option-C investigation suggests a direct A/B test: render O-Bowed at `INFINITE_SUSTAIN = 1.0` with the same harness profile and check whether O-Bowed exhibits a similar `rmsRatio < 0.5` at bow-off + 4 s. If O-Bowed shows `rmsRatio ≈ 1.0`, the algebraic-vs-tanh choice is the substantive question; if O-Bowed shows a similar ratio, the issue is the harness pass-bar specification, not the saturator.

This A/B comparison is **deferred to Phase 2.4** along with the 108-combo stability matrix work. Reason: O-Bowed does not currently expose `INFINITE_SUSTAIN`, so the comparison harness needs a small APVTS-side scaffold or a direct loop-gain override hook — outside the Phase 2.1 cycle scope.

## 12.5 Re-Evaluation Triggers

Phase 2.4 should escalate this back to architecture-level review if any of the following surface in the 108-combo stability matrix:

1. **Per-string asymmetry** — A1 / D2 / G2 strings show `rmsRatio` consistently below threshold while E1 sits at the threshold edge (would indicate the f₀-scaled decay is the dominant problem, not the saturator-form choice).
2. **Audible truncation** — listening tests reveal an unnaturally fast natural-decay envelope on bow-off compared to acoustic contrabass reference recordings (the human ear is more sensitive to envelope shape than to RMS ratios).
3. **Drone-mode interaction** — `INFINITE_SUSTAIN > 0.95` combined with low Bow Force shows energy dropout before the loop-gain sustain compensates (would indicate saturator dissipation is preventing drone bootstrapping at low excitation).
4. **Body-resonator coupling** — Phase 2.5's body bank gain might amplify the saturator's low-amplitude loss into an audible difference relative to acoustic reference.

If none of these surface during Phase 2.4's matrix sweep, the saturator-tail decay can be ratified as ARCHITECTURE-correct and `pass_rms` can be re-specified as a sustained-state-only invariant in Phase 2.4's harness extension. If any surface, the §12.4 comparison kicks off and ARCHITECTURE.md §"In-loop saturator" becomes a candidate for amendment alongside §"DC Blocker" (§11.6).

## 12.6 Architecture-Track Status

ARCHITECTURE.md §"DC Blocker" amendment is already deferred to end-of-Stage-2 verify (per CONTEXT.md rev-2 decision #3). §"In-loop saturator" amendment is now **conditionally tracked** as part of the same end-of-Stage-2 review:

- **If Phase 2.4's matrix triggers any §12.5 escalation:** §"In-loop saturator" gets an amendment proposal alongside §"DC Blocker".
- **If no triggers fire:** §"In-loop saturator" stands as currently specified; §"DC Blocker" amendment proceeds independently (output-path DCB option vs. removal).

No mid-cycle ARCHITECTURE edit is required for either deviation. The audit trail is sufficient: PLAN rev-3, SUMMARY.md, VERIFICATION.md, R7 commit-message body, and this RESEARCH §12 footnote together document the deviations and the deferred amendment path.

## 12.7 References

- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` lines 17–21 (phenomenon recap), 162–174 (analytical derivation), 230–235 (Option-C deferred A/B test), 267–280 (decision matrix).
- `e1-max-sustain.json` (project root, captured at 2026-04-26 verify-phase rerun) — concrete numerical evidence: `rmsMid_s5_s6 = 0.0353`, `rmsFinal_lastSecond ≈ 0`, `pass_rms = false`, `passNan = true`, `passPeak = true`, `passBlockTime = true`.
- RESEARCH §1.3 — In-loop saturator placement contract.
- RESEARCH §10.3.1 — algebraic-vs-tanh low-amplitude comparison (informal — only computed at large amplitude there; this section completes the low-amplitude analysis).
- RESEARCH §11.7 — H3 disposition table noting saturator was disproven as the *bootstrapping* failure mode but not analyzed for tail dissipation; §12 closes that gap.

---

# 13. Phase 2.1b Module-Extraction Research (rev-2 append)

**Date appended:** 2026-04-26 (research-phase rev-2)
**Cross-reference:** `CONTEXT.md` rev-2 §"Cycle Scope: Part B — Phase 2.1b opening" + §"Open Questions" + §"Files / Artefacts to Produce in Phase 2.1b".

This section refines the Phase 2.1b module-extraction plan in light of the rev-2 corrections (module surface = `HyperbolicFriction` + `BowModel` only) and resolves the five Open Questions handed from `CONTEXT.md` rev-2. It supersedes-by-refinement (not by replacement) §1 (extraction surface) and §2 (module pattern) — both remain authoritative for material not contradicted here.

## 13.1 Carry-Forward From §1, §2

The following decisions from §1, §2 stand verbatim:

- **Module home:** `modules/synthesis/bow-friction/` (§2.4).
- **Module name:** `bow-friction` → CMake target `ouaricon_bow_friction` (§2.1).
- **Layout:** `module.yaml`, `README.md`, `cpp/HyperbolicFriction.h`, `cpp/BowModel.h`, `cpp/BowModel.cpp` (§2.1, refined in §13.3 below).
- **CMake pattern:** Pattern A — `ouaricon_add_module(<plugin> bow-friction)` (§2.2; resolves Open Question #2 — see §13.3).
- **Registry entry shape:** appended under `synthesis` category, version 1.0.0, with O-Bowed and O-Contrabass listed in `used_by` (§2.3).
- **Friction-default deltas:** O-Bowed uses init defaults `mu_s = 0.8, mu_d = 0.3`; O-Contrabass uses `mu_s = 0.85, mu_d = 0.25` (§1.2). The module exposes the O-Bowed init defaults as the baseline; O-Contrabass injects its values via a setter API (resolves Open Question #5 — see §13.3).
- **WaveguideString stays per-plugin.** Saturator differs (O-Bowed `4·tanh(x/4)`; O-Contrabass `x/sqrt(1+x²)`) and topology variations exist (§1.3); promoting `WaveguideString` to the module would require saturator-template parameterisation. Deferred indefinitely.

## 13.2 Corrected Module Surface (rev-2)

`CONTEXT.md` rev-1 named three classes for extraction: `HyperbolicBowTable`, `BowState`, `SchellengGuard`. Survey of the O-Bowed source-of-truth (`grep -rn` on `plugins/O-Bowed/Source/DSP/`) confirms:

| rev-1 name | Actual O-Bowed class | Status |
|---|---|---|
| `HyperbolicBowTable` | `HyperbolicFriction` (`HyperbolicFriction.h`, 55 LOC, header-only) | Real — extract verbatim. |
| `BowState` | `BowModel` (`BowModel.h` 51 LOC + `BowModel.cpp` 97 LOC) | Real — extract verbatim. |
| `SchellengGuard` | (does not exist) | Aspirational. Schelleng wedge logic is currently inlined inside `BowedStringVoice.cpp::renderNextBlock`; not extracted in 2.1b. Phase 2.3 will author this class as the slow-bow LFO clamp lands; module gets a v1.1.0 entry adding it. |

**Locked module surface for Phase 2.1b v1.0.0:** `HyperbolicFriction` + `BowModel` only. Total source size: 55 + 51 + 97 = 203 LOC across three files.

## 13.3 Open Question Resolutions (rev-2 §"Open Questions")

### Q1 — RESEARCH §12 timing → Resolved by writing §12 in this same research-phase pass.

`CONTEXT.md` rev-2 recommended option (a): document saturator-tail in §12 during 2.1b research. Done. §12 above is the deliverable; no separate small-update commit is needed.

### Q2 — Module CMakeLists pattern → **Pattern A: `ouaricon_add_module(<plugin> bow-friction)`**.

Rationale (refines §2.2):

- The friction module has **no JUCE patch** (unlike `note-expression`, which requires `JUCE-NE-PATCH`). No `module.cmake` hook is needed.
- The module has **no per-format routing** (unlike `note-expression`, where VST3-vs-AU dispatch matters). Headers + sources go into `SharedCode` for all formats automatically.
- The module is **single-language** (C++ only — no JS, no resources). Pattern A's automatic `cpp/*.{h,cpp}` glob (per `OuariconModules.cmake` lines 57–67) handles the entire surface with one call.
- Both plugins (`O-Bowed`, `O-Contrabass`) already `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` for `note-expression`, so the helper is in scope without any CMake-system change.

Plugin CMakeLists call site (both plugins):
```cmake
# Phase 2.1b — extracted shared friction module
ouaricon_add_module(<TargetName> bow-friction)
```

### Q3 — Header layout / public-API surface → **Two direct headers, no umbrella**.

Module layout (locked):
```
modules/synthesis/bow-friction/
├── module.yaml
├── README.md
└── cpp/
    ├── HyperbolicFriction.h    (55 LOC, header-only, copy from O-Bowed verbatim + §13.3-Q5 setter additions)
    ├── BowModel.h              (51 LOC, copy from O-Bowed verbatim)
    └── BowModel.cpp            (97 LOC, copy from O-Bowed verbatim)
```

Consumer include lines:
```cpp
#include <HyperbolicFriction.h>
#include <BowModel.h>
```

(Or `"HyperbolicFriction.h"` / `"BowModel.h"` — both work because `ouaricon_add_module` adds `cpp/` to PRIVATE include path.)

**No umbrella `bow-friction.h`.** Rationale:
- The module's two public classes are independently useful: `BowModel` is bow-envelope state; `HyperbolicFriction` is the friction curve. Some future consumer might want only one (e.g. a bow-envelope-driven excitation that uses elastoplastic friction instead).
- Mirrors `scala-tuning-engine`'s convention: separate `TuningEngine.h`, `ScaleGenerator.h`, etc. — no umbrella.
- Two `#include` lines is not a meaningful ergonomics tax compared to the maintenance overhead of a third file.

### Q4 — Plugin-side include-switch mechanics → **Delete inline copies; update include lines**.

Concrete diff plan (to be encoded in PLAN rev-4):

**O-Bowed (existing source-of-truth — extract):**
- `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h` → DELETE
- `plugins/O-Bowed/Source/DSP/BowModel.h` → DELETE
- `plugins/O-Bowed/Source/DSP/BowModel.cpp` → DELETE
- `plugins/O-Bowed/Source/BowedStringVoice.h` lines 23–24:
  ```cpp
  // BEFORE
  #include "DSP/BowModel.h"
  #include "DSP/HyperbolicFriction.h"
  // AFTER
  #include "BowModel.h"
  #include "HyperbolicFriction.h"
  ```
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` line 13:
  ```cpp
  #include "HyperbolicFriction.h"   // unchanged — already a bare-name include
  ```
  Works as-is via the new include path.
- `plugins/O-Bowed/CMakeLists.txt`: drop `Source/DSP/HyperbolicFriction.h`, `Source/DSP/BowModel.h`, `Source/DSP/BowModel.cpp` from `target_sources`. Add `ouaricon_add_module(O-Bowed bow-friction)`.
- `plugins/O-Bowed/Source/DSP/ElastoPlasticFriction.h` and `ThermalFriction.h`: comment-only mentions of `HyperbolicFriction` (`// Match HyperbolicFriction API`). No code change needed.

**O-Contrabass (current consumer — switch from inline copy to module):**
- `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h` → DELETE
- `plugins/O-Contrabass/Source/DSP/BowModel.h` → DELETE
- `plugins/O-Contrabass/Source/DSP/BowModel.cpp` → DELETE
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (current Phase 2.1a version): grep-and-update any `#include "DSP/BowModel.h"` / `#include "DSP/HyperbolicFriction.h"` to `#include "BowModel.h"` / `#include "HyperbolicFriction.h"`.
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp`: grep-and-update `#include "HyperbolicFriction.h"` (already bare-name; works as-is).
- `plugins/O-Contrabass/CMakeLists.txt`: drop the three DSP friction files from `target_sources`. Add `ouaricon_add_module(O-Contrabass bow-friction)`.
- `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt`: drop `${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP/BowModel.cpp` from `target_sources`. Add `ouaricon_add_module(O-Contrabass-render-test bow-friction)` (or duplicate the include path manually if `ouaricon_add_module` does not work for non-plugin targets — PLAN-phase pattern-confirms this).

**No shim files.** No `Source/DSP/HyperbolicFriction.h` re-exporting the module header; no `// removed in 2.1b` placeholder comments. Clean delete.

### Q5 — Bass-default propagation API → **Setter API on `HyperbolicFriction`**.

Module `HyperbolicFriction.h` keeps O-Bowed's existing init defaults (`mu_s = 0.8`, `mu_d = 0.3`, `v_0 = 0.05`, `R_s = 0.5`) and gains two new setters:

```cpp
void setStaticFrictionCoefficient (float mu) noexcept    { mu_s = mu; }
void setDynamicFrictionCoefficient (float mu) noexcept   { mu_d = mu; }
```

(Existing `setRosin(float)` and `setStringImpedance(float)` are retained unchanged.)

Consumer call patterns:

- **O-Bowed `BowedStringVoice::prepareToPlay`:** no change. Defaults inherit from the module's init list.
- **O-Contrabass `BowedContrabassVoice::prepareToPlay`** (or wherever the Phase 2.1a Stage-1 wiring sets up the friction model): add two setter calls:
  ```cpp
  frictionModel.setStaticFrictionCoefficient (0.85f);
  frictionModel.setDynamicFrictionCoefficient (0.25f);
  ```

Rationale for setter-API over factory-or-ctor:

- **Smallest module-surface change:** two new setters (4 LOC additions to `HyperbolicFriction.h`); no factory function, no `Defaults` struct, no template parameter.
- **Matches existing pattern:** the class already exposes `setRosin()` and `setStringImpedance()` setters; the pattern is established and consistent. Rosin is bass-tuned via `parameter-spec.md ROSIN = 0.65 → setRosin(0.65)` per §1.2 — the same hook applies for `mu_s` / `mu_d` if the user later wants to expose them as APVTS parameters (deferred to v1.1).
- **No init-list churn:** we're not flipping the module's defaults to bass values then calling treble setters from O-Bowed; O-Bowed gets to keep "no setter calls = O-Bowed defaults" as the simplest possible behavior.
- **Trivial determinism:** setter calls happen in `prepareToPlay`, which runs before any audio block; same input parameters → same internal state → bit-exact reproducibility maintained for both plugins post-extraction.

The bass-default propagation is a per-`prepareToPlay` operation, not a per-block operation, so it does not violate PERF-01 (no allocations / locks / file I/O in `processBlock`).

## 13.4 Canonical Preset for O-Bowed Golden Render

The Gate 2.1 bit-exact regression bar requires a single canonical preset against which `o-bowed-pre-extraction-canonical.wav` (golden reference) and `o-bowed-post-extraction-canonical.wav` (post-switch render) are byte-compared via `cmp`.

**Preset spec (locked here, mirrors `CONTEXT.md` rev-2 §"O-Bowed regression bar"):**

| Setting | Value | Source |
|---|---|---|
| Plugin under test | O-Bowed (current main, pre-extraction) | rev-2 CONTEXT |
| Sample rate | 44 100 Hz | rev-2 CONTEXT (matches O-Contrabass harness) |
| Buffer size | 512 samples | mirrors O-Contrabass harness (deterministic block boundary) |
| MIDI note | A4 = note 69 | rev-2 CONTEXT "default A4 sustained" |
| MIDI velocity | 0.7 | matches O-Contrabass harness convention |
| Note-on at | t = 100 ms | mirrors O-Contrabass harness (warm-up tail) |
| Note-off at | t = 5.0 s (total render duration ~5.0 s) | rev-2 CONTEXT "~5 s" |
| Release tail | 0 s (render stops at note-off) | minimises decay-tail entropy |
| APVTS state | factory defaults (i.e. no `setStateInformation` call) | rev-2 CONTEXT "no detune/vibrato/sub-harmonics, INFINITE_SUSTAIN OFF" |
| Output channels | stereo (2 ch) | matches O-Bowed plugin output bus |
| WAV format | 32-bit float WAV | mirrors O-Contrabass render-harness `WavAudioFormat` writer (`createWriterFor` defaults) |

**Determinism checks (must hold for bit-exact `cmp` to be a valid regression bar):**

1. `HyperbolicFriction` is pure-value-class code: no static state, no random number sources, no time-dependent floats outside the float-math reproducibility envelope. Same inputs → same outputs bit-by-bit.
2. `BowModel` envelope coefficients are deterministic functions of `(sampleRate, velocity, bowSpeedParam, bowPressureParam)` only (`BowModel.cpp:15-21, 23-39`). Same inputs → same outputs.
3. `WaveguideString` (NOT in the module — stays per-plugin) is deterministic given same inputs.
4. JUCE `MPESynthesiser` voice allocation is deterministic in single-voice scenarios (one note-on, one voice — no contention).
5. CPU FP rounding mode is JUCE-default (round-to-nearest); `juce::ScopedNoDenormals` ensures denormals are flushed identically across runs.

**Caveat:** if the future v1.1 of the friction module changes the init defaults (e.g. moves to bass defaults), the canonical preset's output WILL diverge from the golden reference. Phase 2.1b is the **only** opportunity for bit-exact regression; v1.1 onwards will need a new regression strategy (RMS tolerance, spectrum tolerance, etc.). PLAN rev-4 must capture this constraint.

## 13.5 O-Bowed Render-Harness — Spec for Plan-Phase

O-Bowed currently has **no** render-harness (`plugins/O-Bowed/` has no `tests/` directory — confirmed by directory listing). The Gate 2.1 golden render requires building one. This is plan-phase R8 work, not research-phase work.

**Recommended approach: mirror O-Contrabass's harness exactly.**

PLAN rev-4 R8 should:

1. Create `plugins/O-Bowed/tests/render-harness/` directory.
2. Copy `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` → `plugins/O-Bowed/tests/render-harness/CMakeLists.txt`. Substitute:
   - Target name: `O-Contrabass-render-test` → `O-Bowed-render-test`.
   - Plugin name: `O-Contrabass` → `O-Bowed`.
   - PluginCode: `0x4f436273` (`OCbs`) → `0x4f426f77` (`OBow`) — confirm against O-Bowed's actual JucePlugin_PluginCode.
   - `target_sources` paths: `BowedContrabassVoice.cpp` → `BowedStringVoice.cpp`; drop `BowModel.cpp` from explicit list (will come in via `bow-friction` module — but at this stage the module doesn't yet exist, so keep `BowModel.cpp` from `Source/DSP/`).
3. Copy `plugins/O-Contrabass/tests/render-harness/main.cpp` → `plugins/O-Bowed/tests/render-harness/main.cpp`. Substitute:
   - Class name: `OContrabassAudioProcessor` → `OBowedAudioProcessor` (or whatever O-Bowed's class is named — `BowedAudioProcessor` likely).
   - Default note: 28 (E1) → 69 (A4).
   - Default sustain: 60 s → 5 s.
   - Default release: 5 s → 0 s.
   - Drop `--infinite-sustain` flag (O-Bowed doesn't expose this).
   - Output WAV path: `e1-max-sustain.wav` → `o-bowed-pre-extraction-canonical.wav`.
   - JSON path: `e1-max-sustain.json` → `o-bowed-pre-extraction-canonical.json`.
4. Add `add_subdirectory(tests/render-harness)` (gated by `OUARICON_BUILD_TESTS`) to `plugins/O-Bowed/CMakeLists.txt` if not already present.
5. Build + run the harness against pre-extraction O-Bowed. Save `o-bowed-pre-extraction-canonical.wav` + `.json` + `sha256sum` output.
6. Commit the new harness files in a **separate commit** ahead of the R15 atomic switch — call it R8a — so R15's diff is purely the module-extraction switch and the harness tooling is independently reviewable.

PASS conditions for the canonical preset (looser than the O-Contrabass stability harness because this is a 5-second render, not a 60-second drone test):

| Invariant | Threshold |
|---|---|
| `pass_nan` | no NaN / Inf samples |
| `pass_peak` | `\|sample\| ≤ 1.0f` |
| `pass_blockTime` | max-block / median-block ≤ 5.0× (denormal-spike sentinel) |
| (no `pass_rms` check) | bow-on / bow-off envelope is short — RMS sanity not meaningful |

The bit-exact `cmp` is the actual gate — the harness JSON is for traceability and to flag obvious instabilities.

## 13.6 Sequencing in PLAN rev-4

The plan-phase task breakdown (rev-4) is expected to look like (numbering carries forward from rev-3 R7):

| Task | Description |
|---|---|
| R8 — pre-flight harness | Build `plugins/O-Bowed/tests/render-harness/` (per §13.5). Render `o-bowed-pre-extraction-canonical.wav` + `.json` + sha256. Commit harness files (R8a) — separate commit, ahead of module work. |
| R9 | Create `modules/synthesis/bow-friction/` directory tree. Write `module.yaml` + `README.md`. |
| R10 | Copy `HyperbolicFriction.h` (with §13.3-Q5 setter additions) + `BowModel.{h,cpp}` from O-Bowed verbatim into `modules/synthesis/bow-friction/cpp/`. |
| R11 | Update `modules/registry.yaml` per §2.3 (append `bow-friction` entry under `synthesis`). |
| R12 | Update `plugins/O-Bowed/CMakeLists.txt`: add `ouaricon_add_module(O-Bowed bow-friction)`, drop `target_sources` for the three deleted DSP files. Update `BowedStringVoice.h` includes (per §13.3-Q4). Delete `Source/DSP/HyperbolicFriction.h`, `Source/DSP/BowModel.{h,cpp}`. |
| R13 | Update `plugins/O-Contrabass/CMakeLists.txt`: same shape. Update `BowedContrabassVoice.{h,cpp}` includes. Delete the three DSP files. Add §13.3-Q5 setter calls in `prepareToPlay`. Update `tests/render-harness/CMakeLists.txt` to drop `BowModel.cpp` and gain the module include path. |
| R14 | Build + auval + pluginval (both plugins). Re-render O-Bowed canonical → `o-bowed-post-extraction-canonical.wav`. Re-render O-Contrabass bow-on-only → match `/tmp/e1-bowon-only.json` byte-for-byte. |
| R15 | **Gate 2 atomic commit:** module + both plugin switches + registry update + O-Contrabass harness CMake update in one commit. Only on Gate 2 PASS. |

R8 / R8a (harness) is structurally a **prerequisite** to R9–R14 because the golden reference must exist before any module-extraction edits. R8 is also independently mergeable (no semantic risk to either plugin's behavior — purely additive tooling), which justifies the separate-commit treatment.

## 13.7 Open Items for Plan Phase

The following are deliberately deferred to PLAN rev-4 (i.e. not pinned by this research-phase pass):

1. **Confirm O-Bowed `JucePlugin_PluginCode`** for the harness CMake substitution (likely `0x4f426f77` = `OBow` but PLAN-phase verifies against `plugins/O-Bowed/CMakeLists.txt`).
2. **Confirm O-Bowed processor class name** for the harness `main.cpp` substitution (likely `OBowedAudioProcessor` but PLAN-phase verifies).
3. **Confirm `ouaricon_add_module` works for non-plugin targets** (the O-Contrabass render-harness target is a `juce_add_console_app`, not a `juce_add_plugin`; PLAN-phase pattern-confirms whether the module helper supports it or whether the harness needs explicit-include-path treatment).
4. **Pin the renderer's `WavAudioFormat` writer parameters** (bit depth, sample format) to ensure the pre-/post-extraction WAVs are byte-comparable. Plan-phase reads `O-Contrabass/tests/render-harness/main.cpp::renderToWav` for the existing parameter set and inherits.
5. **Decide whether to commit the golden WAV** (`o-bowed-pre-extraction-canonical.wav`) into git, or keep it in `/tmp/` and rely on regenerating via the harness. Recommendation: keep the SHA-256 hash + harness JSON committed for audit trail; do NOT commit the binary WAV (~885 KB at 32-bit float stereo / 5 s; `/tmp/` is sufficient for the live `cmp` invocation).

## 13.8 References (rev-2 append)

- `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h` lines 19–55 — extraction source, header-only.
- `plugins/O-Bowed/Source/DSP/BowModel.{h,cpp}` — extraction source.
- `plugins/O-Bowed/Source/BowedStringVoice.h` lines 23–24, 84–85 — current inline-include consumer pattern.
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` line 13 — already bare-name include (no edit needed at extraction).
- `modules/tuning/note-expression/module.yaml` — Pattern A reference for `module.yaml` schema.
- `modules/tuning/scala-tuning-engine/` — multi-header module convention reference.
- `modules/cmake/OuariconModules.cmake` lines 34, 57–67 — `ouaricon_add_module` category search + cpp/* glob behaviour.
- `modules/registry.yaml` lines 16–39 — category schema confirming `synthesis` exists.
- `plugins/O-Contrabass/tests/render-harness/{CMakeLists.txt,main.cpp}` — harness template for §13.5.
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-2 §"Open Questions" — the five questions resolved in §13.3.

---

# 14. Phase 2.1c Cascaded-Allpass Dispersion Research (rev-3 append)

**Date appended:** 2026-04-27 (research-phase rev-3)
**Cross-reference:** `CONTEXT.md` rev-3 §"Cycle Scope" + §"Open Questions" + §"Files / Artefacts to Produce in Phase 2.1c".

This section resolves the five Open Questions handed by `CONTEXT.md` rev-3 (closed-form constants, group-delay formula, setter API, template shape, harness output), pins the literal Rauhala/Välimäki 2006 coefficient values for `DispersionFilter.h`, and specifies the pre-flight bit-exact baseline-render procedure for the Gate 3 stiffness=0 regression bar. It supersedes-by-refinement the §"Cascaded Allpass Dispersion" structural sketch in `research/O-Contrabass-bass-waveguide-stability.md` §2.3 — that document remains authoritative for the high-level algorithm; this section pins the concrete plugin-side specifics.

## 14.1 Carry-Forward From Prior Research

The following decisions remain in effect verbatim:

- **Closed-form Rauhala/Välimäki 2006 algorithm** (`research/O-Contrabass-bass-waveguide-stability.md` §2.3 lines 145–161). Algorithm pinned; only the literal `m1..m4`, `k1..k3` constants and the implementation surface need finalisation here.
- **Loop placement on bridge rail, before bridge LP** (`CONTEXT.md` rev-3 Q1 lock): `pop → dispersion → bridge LP → −1 boundary → friction inject → in-loop saturator → push`. Mirrors `WaveguideString.h` line 38 contract; supersedes the stale `WaveguideString.cpp:170-171` placeholder comment (correction encoded in PLAN rev-5 R17).
- **Hardcoded M=4 for Phase 2.1c (E1 only)** (`CONTEXT.md` rev-3 Q2 lock). Per-string M=4/3/2/1 table is Phase 2.2 work.
- **`B = 1e-4 · STRING_STIFFNESS`** verbatim from `ARCHITECTURE.md` §"String Waveguide Bank" line 81 (`CONTEXT.md` rev-3 Q3 lock). Prefactor `1e-4` is the locked E1 value.
- **Per-block coefficient cadence** from the existing 20 ms `stiffnessSmoothed` in `WaveguideString.h:100`. Per-sample `a` interpolation is the click-fallback only (invoked if R18 sweep produces clicks).
- **Per-plugin location** at `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (`CONTEXT.md` rev-3 lock). O-Bowed has no dispersion filter (verified — `find plugins/O-Bowed/Source -name "Disper*"` returns empty); module-promotion deferred until a second consumer arrives.

## 14.2 Open Question #1 — Rauhala/Välimäki 2006 Closed-Form Constants (RESOLVED)

`research/O-Contrabass-bass-waveguide-stability.md` §2.4 lines 224–239 already contains a fully-worked C++ sketch of the closed-form `computeAllpassCoeff(f0, B, M)` with **literal constants extracted from Rauhala & Välimäki (2006), "Tunable dispersion filter design for piano synthesis", IEEE Signal Processing Letters Vol. 13 No. 5, Table 1** (paper URL: https://ieeexplore.ieee.org/document/1618690). The constants are:

| Constant | Value | Role |
|---|---|---|
| `k1` | `-0.0135f` | Constant term in `k(I) = k1 + k2·I + k3·I²` (key-number polynomial) |
| `k2` | ` 0.0058f` | Linear term — slope of dispersion target vs. virtual key number |
| `k3` | `-0.000004f` | Quadratic term — high-key dispersion curvature correction |
| `m1` | ` 0.0034f` | `log(B)` weight in `C(B,M) = m1·lB + m2·lM + m3·lB·lM + m4` |
| `m2` | ` 0.0179f` | `log(M)` weight |
| `m3` | `-0.0009f` | Cross term `log(B)·log(M)` |
| `m4` | `-0.4986f` | Bias term — sets the negative-`a` regime that produces rising-phase-delay |

**Closed form (lock for `DispersionFilter.h`):**

```cpp
// f0    : fundamental in Hz (per-note voice state)
// B     : inharmonicity coefficient = 1e-4f * STRING_STIFFNESS for E1
// M     : cascade depth (Phase 2.1c: hardcoded 4 for E1)
//
// Returns: allpass coefficient `a` in (-0.99, 0.99), already clamped.
//
// Citation: Rauhala & Välimäki (2006), IEEE Sig. Proc. Letters, Table 1.
//           See research/O-Contrabass-bass-waveguide-stability.md §2.3-2.4.

static float computeAllpassCoefficient (float f0Hz, float B, int M) noexcept
{
    constexpr float k1 = -0.0135f, k2 = 0.0058f, k3 = -0.000004f;
    constexpr float m1 =  0.0034f, m2 = 0.0179f, m3 = -0.0009f, m4 = -0.4986f;

    const float I  = std::log2 (juce::jmax (f0Hz, 1.0f) / 440.0f) * 12.0f + 49.0f;
    const float lB = std::log  (juce::jmax (B,    1e-9f));
    const float lM = std::log  (static_cast<float> (juce::jmax (M, 1)));

    const float C  = m1 * lB + m2 * lM + m3 * lB * lM + m4;
    const float k  = k1 + k2 * I + k3 * I * I;

    return juce::jlimit (-0.99f, 0.99f, -C / k);
}
```

**Numerical sanity check at the four corners of the Phase 2.1c parameter envelope (E1, M=4, sr=88200):**

| STRING_STIFFNESS | B | I (E1=41.2 Hz) | C | k | -C/k | clamped `a` |
|---|---|---|---|---|---|---|
| 0.00 | 1e-9 (clamp floor) | 8.00 | -0.5454 | 0.0327 | 16.68 | **+0.99** |
| 0.01 | 1e-6 | 8.00 | -0.5219 | 0.0327 | 15.96 | **+0.99** |
| 0.30 (factory default) | 3e-5 | 8.00 | -0.4869 | 0.0327 | 14.89 | **+0.99** |
| 1.00 | 1e-4 | 8.00 | -0.4936 | 0.0327 | 15.10 | **+0.99** |

**Anomaly flagged for plan-phase + execute-phase R18 sweep:** at E1 (I=8.0), the closed-form's `k` denominator is small (`0.0327`), driving `-C/k` to ~15 across the entire stiffness range — i.e. the formula clamps to `+0.99` regardless of B. This is consistent with Rauhala & Välimäki's published design target being **piano** (I roughly 1..88 over 88-key range, with the calibration tuned for the upper register), not a contrabass low E (I=8). At E1 the closed form sits at the **edge of its validity envelope**.

**This is NOT an implementation bug.** The closed form is reproduced verbatim from the paper. The behaviour is a known limitation of applying piano-tuned coefficients to contrabass register, and matches the literature's framing (`research/O-Contrabass-bass-waveguide-stability.md` §2.3 line 161 reports the paper's design range as `B ∈ [1e-6, 1e-3], M = 4 → a ∈ [-0.05, -0.5]` for *piano* registers; bass register sits outside that envelope).

**Implication for Phase 2.1c:** the audible character of `STRING_STIFFNESS` will manifest mostly as clamp-saturated dispersion at the bridge rail — i.e. dispersion is "on" for any STRING_STIFFNESS > 0 rather than smoothly proportional. The `--stiffness-sweep` harness (R18) will surface whether this is musically acceptable. If R18 reveals the sweep is musically uninteresting because the coefficient saturates immediately, the **Phase 2.4 follow-up** is to replace the closed form with a piecewise polynomial calibration for the bass register (analogous to the Phase 2.4 saturator-tail parking pattern in §12.5 — out of Phase 2.1c scope). PLAN rev-5 should annotate this as a known trade-off in R18's success criteria, **not** as a bug to fix mid-stage.

**Mitigation already locked in CONTEXT.md rev-3:** the bit-exact regression at STRING_STIFFNESS=0 is the strongest possible regression bar; combined with auval/pluginval-10/bow-on-only stability (4/4 TRUE), Gate 3 still has a meaningful pass/fail signal even if the audible sweep is duller than ideal.

## 14.3 Open Question #2 — Group-Delay Subtraction Formula (RESOLVED — option b)

For a single first-order allpass section `A(z) = (a + z^-1) / (1 + a·z^-1)` with `|a| < 1`:

**Phase response:**
```
φ_section(ω) = -ω - 2·arctan( a·sin(ω) / (1 + a·cos(ω)) )
```

**Group delay (closed form, derivative of φ):**
```
τ_section(ω) = -dφ/dω = (1 - a²) / (1 + 2a·cos(ω) + a²)
            = (1 - a²) / |1 + a·e^{-jω}|²
```

For a cascade of `M` identical sections, total group delay = `M · τ_section(ω)`.

**Two evaluation points debated:**

| Option | Formula | Cost | Accuracy at f0 |
|---|---|---|---|
| (a) DC | `τ_section(0) = (1-a)/(1+a)` (simplifies because cos(0)=1) | 1 div | Inexact for non-DC; OK for low-f0 |
| (b) at f0 | `τ_section(2π·f0/sr) = (1-a²) / (1 + 2a·cos(2π·f0/sr) + a²)` | 1 cos + 2 mul + 1 div | Exact |

**Numerical sanity check at E1 (f0=41.2 Hz, sr=88200, ω=0.002935 rad, cos(ω)≈0.9999957):**

| `a` | (a) DC formula | (b) at-f0 formula | Δ (samples) |
|---|---|---|---|
| 0.00 | 1.0000 | 1.0000 | 0.0000 |
| 0.50 | 0.3333 | 0.3333 | <0.0001 |
| 0.99 | 0.005025 | 0.005000 | 0.000025 |

At E1's ω ≈ 0.003 rad, options (a) and (b) agree to 4+ decimal places because the contrabass fundamental is ~0.05 % of Nyquist at 88.2 kHz internal SR. **Both formulas would pass the bit-exact regression at stiffness=0** (where `a=0` exactly); for non-zero `a`, the residual difference is below sub-sample accuracy.

**Decision: option (b) — at-f0 formula.** Rationale:

1. **Mathematical correctness over premature optimisation** — the cost is one trig + two multiplies, computed once per block (not per sample); not a measurable CPU cost.
2. **Future-proof for higher strings** (Phase 2.2) — A1/D2/G2 strings push f0 up to 98 Hz (still < 0.5 % of Nyquist) but G3 (Phase 2.2 stretch) is 196 Hz; option (b) stays accurate as f0 climbs.
3. **No reason to choose (a)** — it isn't simpler in code (same number of float ops), isn't faster in any meaningful sense, and is a strict subset of (b).

**Closed form for `DispersionFilter::getGroupDelaySamples()`:**

```cpp
// Computes total group delay (in samples) of an M-section cascade at frequency f0.
// Closed form: D = M · (1 - a²) / |1 + a·e^{-j·2π·f0/sr}|²
//
// Used by WaveguideString::updateDelayLengths() to compensate base round-trip.

float getGroupDelaySamples (float f0Hz, double sampleRateHz) const noexcept
{
    if (activeSections == 0)
        return 0.0f;

    const float a    = sections[0].a;          // all sections share the same coefficient
    const float w    = juce::MathConstants<float>::twoPi
                     * f0Hz / static_cast<float> (sampleRateHz);
    const float cosW = std::cos (w);
    const float oneMinusASq = 1.0f - a * a;
    const float denom       = 1.0f + 2.0f * a * cosW + a * a;
    const float perSection  = oneMinusASq / juce::jmax (denom, 1e-9f);

    return static_cast<float> (activeSections) * perSection;
}
```

**Identity-at-`a=0` check:** with `a=0`, numerator `=1`, denominator `=1`, per-section delay `=1`, total `= M·1 = M`. Mirrors the `M` unit-delay topology of the cascade exactly. The compensated subtraction `totalDelay - filterGroupDelay - dispersionGroupDelay` therefore subtracts exactly `M` samples when dispersion is identity, exactly matching the `M` z^-1 elements the cascade interposes — net round-trip delay is unchanged → bit-exact regression at stiffness=0 holds. ✓

## 14.4 Open Question #3 — Per-Block Setter API on WaveguideString (RESOLVED — option a)

**Decision: option (a) — `setDispersionCoefficient(float a)` (voice computes, waveguide consumes).**

```cpp
// In WaveguideString.h public API, alongside existing setStringStiffness:
void setDispersionCoefficient (float a) noexcept;
```

Rationale:

1. **Closed form depends on `f0` (per-voice state), not waveguide state.** `WaveguideString` does not currently know the per-note fundamental — `currentFrequency` is set via `trigger(frequency)` but the closed form also depends on `M` (cascade depth, plugin-policy state) and `B` (`= 1e-4·STRING_STIFFNESS`, which the waveguide already smooths internally). Putting the math in the voice keeps the `f0/B/M → a` policy decision out of the waveguide and on the voice's `renderNextBlock` boundary.
2. **Symmetric with existing `setBrightness(cutoffHz)` and `setInfiniteSustain(amount)` setters** — voice computes, waveguide consumes raw float. Consistent setter contract across all `WaveguideString` parameters.
3. **Simplifies the bit-exact regression bar** — at STRING_STIFFNESS=0, voice computes `a = 0.0f` once at note-on (or at the first block-boundary update) and passes 0.0f to the waveguide. The waveguide sees a deterministic stream of `setDispersionCoefficient(0.0f)` calls and the dispersion path is identity. No internal-state surprise.
4. **Cleaner per-block update path:** voice's `renderNextBlock` already advances `stiffnessSmoothed` per block (per `BowedContrabassVoice.cpp:234` `setStringStiffness` call routes the param into the smoother) — extending it to call `computeAllpassCoefficient(f0, B, M)` and forwarding to the waveguide is one extra line.

**Per-block update sequencing in `BowedContrabassVoice::renderNextBlock` (PLAN rev-5 R17 plumbing):**

```cpp
// Existing per-block update (Phase 2.1a-recovery):
// updateParametersFromAPVTS();   // sets brightness, infiniteSustain, stiffness, etc.

// Phase 2.1c addition (per-block, BEFORE the per-sample loop):
{
    // Advance the 20 ms stiffness smoother by numSamples (block-rate step).
    waveguideString.advanceStiffnessSmootherBy (numSamples);
    const float currentStiffness = waveguideString.getCurrentSmoothedStiffness();
    const float B = 1.0e-4f * currentStiffness;       // rev-3 Q3 lock
    constexpr int M = 4;                              // rev-3 Q2 lock
    const float f0 = currentFrequency;                // per-voice state
    const float a  = DispersionFilter<4>::computeAllpassCoefficient (f0, B, M);
    waveguideString.setDispersionCoefficient (a);
}

// Per-sample loop runs as before; dispersion processes via the cached `a`.
```

**Plan-phase patterning hint:** the "advance smoother by numSamples" + "read current smoothed value" pair may need two new accessor methods on `WaveguideString` (`advanceStiffnessSmootherBy(int)`, `getCurrentSmoothedStiffness() const`) since `stiffnessSmoothed` is a private member. PLAN rev-5 R17 specifies these accessors.

**Per-sample fallback (deferred — only invoked if R18 sweep clicks):** if the per-block cadence produces audible clicks during the STRING_STIFFNESS automation sweep, switch the `a` coefficient itself to a per-sample interpolation between block-boundary values. This is the same per-sample-vs-per-block trade-off resolved in O-Bells ramping; ~5 LOC change scoped inside `WaveguideString::processSample` and orthogonal to the rest of Phase 2.1c. Not invoked unless R18 fails — research-phase explicitly defers.

## 14.5 Open Question #4 — DispersionFilter.h Template/Class Shape (RESOLVED — option c)

**Decision: option (c) — `template <int MaxSections> class DispersionFilter` with a runtime `int activeSections ≤ MaxSections`.**

This is exactly the shape already sketched in `research/O-Contrabass-bass-waveguide-stability.md` §2.4 lines 199–241 (template+activeSections). The research-document sketch is the **direct ancestor** of the locked spec; Phase 2.1c brings it into `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` with one refinement noted below.

Rationale:

1. **Compile-time `MaxSections=4` covers Phase 2.1c (E1 only) with no waste** — exactly four `AllpassSection` state members; the array sizes statically.
2. **Runtime `activeSections` is the natural per-string M selector** for Phase 2.2 (E1=4, A1=4, D2=2, G2=1 per the per-string M table in `research/O-Contrabass-bass-waveguide-stability.md` §2.3 line 172) — Phase 2.2 builds the per-string bank, sets `activeSections` per voice/string, no template re-instantiation.
3. **No `AudioProcessor`-time allocation:** state is `AllpassSection sections[MaxSections]` (stack/in-place), no `prepare()` allocation; only `reset()` zeroes state.
4. **Clean RT-safe API:** `prepare(sampleRate)` is a one-shot init, `reset()` zeroes state, `setCoefficient(float a)` is per-block, `processSample(float x)` is per-sample, all noexcept.

**Refined header skeleton for PLAN rev-5 R16 (writes new file `Source/DSP/DispersionFilter.h`):**

```cpp
/*
  ==============================================================================

    DispersionFilter.h
    O-Contrabass — Cascaded First-Order Allpass Dispersion (Rauhala/Välimäki 2006)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1c. Lives on the bridge rail of the split-rail waveguide,
    between popSample and the bridge LP one-pole. Identity at a=0.

    Closed-form coefficient computation per Rauhala & Välimäki (2006),
    "Tunable dispersion filter design for piano synthesis", IEEE Sig.
    Proc. Letters Vol. 13 No. 5, Table 1. Constants and validity envelope
    documented in RESEARCH §14.2.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>

template <int MaxSections = 4>
class DispersionFilter
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : sections) s.z = 0.0f;
    }

    // Set the cascade depth at runtime (Phase 2.2 per-string M-table hook).
    // Phase 2.1c: voice constructs DispersionFilter<4> and calls setActiveSections(4).
    void setActiveSections (int M) noexcept
    {
        activeSections = juce::jlimit (0, MaxSections, M);
    }

    // Per-block setter — voice computes `a` from (f0, B, M) and pushes here.
    void setCoefficient (float a) noexcept
    {
        const float clamped = juce::jlimit (-0.99f, 0.99f, a);
        for (int i = 0; i < MaxSections; ++i)
            sections[i].a = clamped;
    }

    // Per-sample processing.
    inline float processSample (float x) noexcept
    {
        for (int i = 0; i < activeSections; ++i)
        {
            // Transposed direct form II — single state element per section.
            //   y[n] = a * x[n] + z[n-1]
            //   z[n] = x[n] - a * y[n]
            const float a = sections[i].a;
            const float y = a * x + sections[i].z;
            sections[i].z = x - a * y;
            x = y;
        }
        return x;
    }

    // Closed-form coefficient — voice calls this once per block.
    static float computeAllpassCoefficient (float f0Hz, float B, int M) noexcept
    {
        constexpr float k1 = -0.0135f, k2 = 0.0058f, k3 = -0.000004f;
        constexpr float m1 =  0.0034f, m2 = 0.0179f, m3 = -0.0009f, m4 = -0.4986f;

        const float I  = std::log2 (juce::jmax (f0Hz, 1.0f) / 440.0f) * 12.0f + 49.0f;
        const float lB = std::log  (juce::jmax (B,    1e-9f));
        const float lM = std::log  (static_cast<float> (juce::jmax (M, 1)));

        const float C  = m1 * lB + m2 * lM + m3 * lB * lM + m4;
        const float k  = k1 + k2 * I + k3 * I * I;

        return juce::jlimit (-0.99f, 0.99f, -C / k);
    }

    // Total group delay of the active cascade at frequency f0Hz, in samples.
    // Used by WaveguideString::updateDelayLengths() to compensate base round-trip.
    float getGroupDelaySamples (float f0Hz) const noexcept
    {
        if (activeSections == 0)
            return 0.0f;

        const float a    = sections[0].a;
        const float w    = juce::MathConstants<float>::twoPi
                         * f0Hz / static_cast<float> (sampleRate);
        const float cosW = std::cos (w);
        const float perSection = (1.0f - a * a)
                               / juce::jmax (1.0f + 2.0f * a * cosW + a * a, 1e-9f);
        return static_cast<float> (activeSections) * perSection;
    }

    int getActiveSections() const noexcept { return activeSections; }

private:
    struct AllpassSection
    {
        float a = 0.0f;     // coefficient, |a| < 0.99 (clamped at setCoefficient)
        float z = 0.0f;     // single state element
    };

    AllpassSection sections[MaxSections];
    int            activeSections = 0;
    double         sampleRate     = 88200.0;
};
```

**Refinements vs. `research/O-Contrabass-bass-waveguide-stability.md` §2.4 sketch:**

1. Splits `prepare(f0Hz, B, M)` (computes coefficient internally) into separate `prepare(sampleRate)` + `setCoefficient(a)` + `setActiveSections(M)` calls — keeps the math in the voice (Q3 resolution) and makes `prepare()` a pure DSP init.
2. Adds `getGroupDelaySamples(f0Hz)` for `updateDelayLengths()` consumption (Q2 resolution).
3. Adds `setActiveSections(int)` for Phase 2.2 per-string M-table hook (forward compatibility — Q2 lock for Phase 2.1c uses M=4).
4. Same `AllpassSection { a, z }` two-float state struct, same transposed-direct-form-II tick math, same `juce::jlimit` clamping.

**State size at runtime:** `4 sections × 8 bytes = 32 bytes per voice` plus `int activeSections` (4 bytes) + `double sampleRate` (8 bytes) = **44 bytes per voice**. Allocated alongside `WaveguideString` instance.

## 14.6 Open Question #5 — Harness Output Format (RESOLVED — option a)

**Decision: option (a) — single WAV `e1-stiffness-sweep.wav` (60 s mono float, MIDI E1, STRING_STIFFNESS ramps 0→1 linearly over duration), plus `.json` metadata with `sha256` field.**

Rationale (refines `CONTEXT.md` rev-3 Q5):

1. **Click-detection invariant requires continuous audio** — the Gate 3 invariant "STRING_STIFFNESS sweep produces no audible clicks" is fundamentally a *transition* invariant; the click happens *between* samples adjacent in time. Three discrete WAVs at 0/50/100 % cannot capture inter-sample clicks at other stiffness values.
2. **Single file is easier to A/B in Logic** — drag-and-drop one WAV to a track; scrub through the timeline; mark click events with timestamp markers. Three files is three tracks or three load operations.
3. **JSON includes sha256** — allows the auditioned WAV to be linked back to a specific harness invocation in the audit trail (mirrors `o-bowed-pre-extraction-canonical.json::sha256` from Phase 2.1b).

**Harness CLI flag spec for PLAN rev-5 R18 plumbing (in `tests/render-harness/main.cpp`):**

```
--stiffness-sweep                  Enable sweep mode (mutually exclusive with default sustained-note mode).
                                   When set, STRING_STIFFNESS ramps linearly 0→1 over the sustain duration.
                                   All other parameters at factory defaults.
--string-stiffness <float=apvts>   In default (non-sweep) mode, override the STRING_STIFFNESS APVTS value
                                   before prepareToPlay (mirrors --infinite-sustain pattern at main.cpp:105-109).
                                   Defaults to APVTS factory default (0.30).
                                   Used by the pre-flight bit-exact baseline (R16-pre, see §14.7).
```

**Sweep-mode behaviour spec:**

```cpp
// Pseudocode for the per-block parameter ramp (PLAN rev-5 R18):
const int totalBlocks = (totalSamples + blockSize - 1) / blockSize;
for (int b = 0; b < totalBlocks; ++b) {
    const float fraction = static_cast<float>(b) / static_cast<float>(juce::jmax(1, totalBlocks - 1));
    const float stiffnessNormalised = juce::jlimit(0.0f, 1.0f, fraction);
    if (auto* p = proc.parameters.getParameter("STRING_STIFFNESS"))
        p->setValueNotifyingHost(stiffnessNormalised);
    // ... existing block processing ...
}
```

**Output WAV spec (mirrors existing harness):**

| Field | Value |
|---|---|
| Filename | `e1-stiffness-sweep.wav` |
| Sample rate | 44100 Hz (host SR; matches existing harness) |
| Channels | 2 (stereo, matches plugin output bus) |
| Bit depth | 24 bit |
| Duration | sustain (default 60 s) + release (default 5 s) = 65 s total |
| MIDI note | 28 (E1) |
| Velocity | 0.7 (matches existing harness convention) |
| INFINITE_SUSTAIN | 1.0 (matches existing harness — lets bow stay engaged across the sweep) |
| All other params | APVTS factory defaults |

**Output JSON spec:**

```json
{
  "status": "PASS|FAIL",
  "mode": "stiffness-sweep",
  "midiNote": 28,
  "velocity": 0.7,
  "sustainSeconds": 60.0,
  "releaseSeconds": 5.0,
  "stiffnessRamp": { "start": 0.0, "end": 1.0, "shape": "linear" },
  "totalSamples": 2866500,
  "peak": <float>,
  "nanCount": <int>,
  "infCount": <int>,
  "rmsByDecade": [ /* RMS per 6s decile of the sweep — surfaces dropouts */ ],
  "blockMicros_median": <float>,
  "blockMicros_max": <float>,
  "sha256": "<64-hex>",
  "outputWav": "e1-stiffness-sweep.wav"
}
```

**Click-detection harness invariants (passive — for traceability, not gating):**

- `pass_nan` — no NaN/Inf samples (same as existing harness).
- `pass_peak` — `|sample| ≤ 1.0f` (same).
- `pass_blockTime` — max-block / median-block ≤ 5.0× (denormal-spike sentinel; same).
- (NO `pass_rms` check — RMS varies by design across the sweep.)

The actual "no clicks" judgement is a **manual** Logic-audition step (Gate 3 §"Test Criteria" item 1). The harness mechanically captures the audio for repeatability and traceability; the user listens to confirm.

## 14.7 Pre-Flight Bit-Exact Baseline Render — Strategy (NOT executed in research)

The Gate 3 bit-exact regression bar (`CONTEXT.md` rev-3 §"Test Criteria" item 7) requires:

1. A **`pre`** render at `STRING_STIFFNESS=0` with no dispersion code present.
2. A **`post`** render at `STRING_STIFFNESS=0` after dispersion code lands.
3. `cmp pre.wav post.wav` byte-equal.

The existing harness (post Phase 2.1b commit `bd5fae0` / `ef0604d`) does NOT expose a `--string-stiffness` CLI flag — only `--infinite-sustain`. Setting STRING_STIFFNESS=0 today requires either editing `main.cpp` to override the param or modifying the APVTS default (both touch source).

**Research-phase decision: defer the actual baseline capture to execute-phase R16-pre** — adding `--string-stiffness` CLI flag to the harness IS the first execute task, BEFORE any DSP source edits. This keeps research-phase invariants intact (no production or test-code edits) and avoids splitting the R20 atomic commit.

**Procedure spec for PLAN rev-5 R16-pre / R16a:**

| Step | Action | Owner |
|---|---|---|
| 1 | Add `--string-stiffness <float>` CLI flag to `tests/render-harness/main.cpp` (mirrors the `--infinite-sustain` override at lines 105–109). Builds + runs without behavioural change at default. | execute R16a |
| 2 | Build harness target: `ninja O-Contrabass-render-test`. | execute R16a |
| 3 | Render baseline: `./O-Contrabass-render-test --string-stiffness 0 --sustain 60 --release 5 --infinite-sustain 1.0 --out e1-bowon-only-stiffness-zero-pre.wav --json e1-bowon-only-stiffness-zero-pre.json`. | execute R16a |
| 4 | Compute sha256: `shasum -a 256 e1-bowon-only-stiffness-zero-pre.wav` → record in `e1-bowon-only-stiffness-zero-pre.json` (or in PLAN.md rev-5 R16a notes). | execute R16a |
| 5 | Stage harness file + golden WAV (or sha256 reference); does NOT commit yet — the R20 atomic commit absorbs all Phase 2.1c work. | execute R16a |
| 6 | (Later, R19) Render post-dispersion at STRING_STIFFNESS=0 with same CLI: `./O-Contrabass-render-test --string-stiffness 0 ... --out e1-bowon-only-stiffness-zero-post.wav`. | execute R19 |
| 7 | (Later, R19) `cmp e1-bowon-only-stiffness-zero-pre.wav e1-bowon-only-stiffness-zero-post.wav` → exit 0 (byte-equal) is Gate 3 PASS for invariant 7. | execute R19 |

**Why the baseline must come from a PRE-dispersion build:** the dispersion code path's identity-at-`a=0` is the *property under test*. If the post-render were generated by a build that had never seen dispersion, the test would pass trivially (no dispersion code = no dispersion side-effect). The pre-render must be from a build with dispersion CODE PRESENT but coefficient `a=0` driving identity behaviour. This is what makes the test meaningful: it confirms the dispersion path's identity branch.

Wait — that requires a re-think. The bit-exact regression at stiffness=0 is most cleanly stated as:

> **Pre-render** = build BEFORE any dispersion code (today's main).
> **Post-render** = build AFTER dispersion code lands, with `a=0` driving identity behaviour.
> **cmp must be byte-equal** because `a=0` makes the dispersion path equivalent to the no-dispersion code path.

So the procedure is correct as listed: the `pre` baseline is captured against PRE-dispersion code (today's working tree, post-Phase 2.1b), and the `post` is captured against POST-dispersion code (post-R19). The bit-exactness depends on:

1. The dispersion path at `a=0` being a pure pass-through (M unit delays in, M unit delays compensated out → net delay change = 0).
2. The compensated subtraction in `updateDelayLengths()` producing identical `bridgeSamples` and `neckSamples` values pre and post (with `dispersionGroupDelay=0` when `a=0`, the subtraction is identical to today's `totalDelay - filterGroupDelay`).
3. Float arithmetic determinism (already guaranteed by JUCE round-to-nearest + `ScopedNoDenormals`).

If `getGroupDelaySamples(f0)` at `a=0` returns *exactly* `M` (= 4 for E1), the compensated calculation in `updateDelayLengths()` becomes `compensated = totalDelay - filterGroupDelay - M`, and the cascade itself contributes `M` unit delays back into the path → net round-trip is preserved. **However**, the bridge rail's geometry changes: today's working tree assigns `bridgeSamples = compensated * bowPosition` where `compensated = totalDelay - filterGroupDelay`. Post-R17, `compensated = totalDelay - filterGroupDelay - dispersionGroupDelay`, and the bridge rail's assigned delay is shorter by `M·bowPosition` samples while the dispersion cascade adds `M` unit delays. Net bridge-rail delay = original `bridgeSamples - M·bowPosition + M = bridgeSamples + M·(1-bowPosition)`. **That is NOT bit-exact to the pre-dispersion working tree!**

**This is a real concern flagged for plan-phase to resolve.** Two options:

| Option | Approach | Bit-exact? |
|---|---|---|
| (i) | Subtract `M` (when `a=0`) from `bridgeSamples` only, not from `compensated` (split-aware compensation). | ✓ — bridge rail's delay-line gets `M` fewer samples; cascade adds `M` back; net unchanged. Neck rail untouched. |
| (ii) | Keep current "subtract from `compensated` then split" math; accept that bit-exact at stiffness=0 needs the dispersion subtraction to be bridge-rail-local. | (i) is the cleaner mental model. |

**Recommended for PLAN rev-5 R17:** option (i) — subtract dispersion group delay from `bridgeSamples` directly, not from `compensated`:

```cpp
// In updateDelayLengths() (post Phase 2.1c R17):
float totalDelay         = static_cast<float>(sampleRate) / std::max(1.0f, currentFrequency);
float filterGroupDelay   = static_cast<float>(sampleRate) / (2.0f * pi * std::max(1.0f, brightnessHz));
float compensated        = totalDelay - filterGroupDelay;
float bridgeSamples      = compensated * bowPosition;
float neckSamples        = compensated * (1.0f - bowPosition);

// Phase 2.1c addition: dispersion lives on bridge rail only → compensate bridge rail only.
float dispersionDelay    = bridgeDispersion.getGroupDelaySamples(currentFrequency);
bridgeSamples           -= dispersionDelay;

// Phase 2.1c R17 clamp guard (existing min):
bridgeSamples = juce::jlimit(4.0f, 8190.0f, bridgeSamples);
neckSamples   = juce::jlimit(4.0f, 8190.0f, neckSamples);
```

At `a=0`: `dispersionDelay = M = 4` samples. The bridge rail loses 4 samples; the M-section cascade adds 4 z^-1 unit delays back. Net bridge-rail delay = unchanged. Neck rail = unchanged. **Bit-exact regression at stiffness=0 holds.** ✓

**Edge case:** at low f0 + high stiffness, `dispersionDelay` may approach 0 (per the §14.3 sanity table at `a=0.99`, per-section delay ≈ 0.005 samples → total ≈ 0.02 samples). The bridge rail's delay-line therefore gets *longer* than today's no-dispersion case by ~M-0.02 = 3.98 samples → the round-trip pitch tracking has a ~3.98-sample shift at stiffness=100%. This is precisely the dispersion-induced pitch effect the literature cares about; it is NOT a bug. The Gate 3 invariant "100 %-stiffness affects attack but NOT steady-state pitch (mode-locking)" is what gates this — the bow's stick-slip nonlinearity should phase-lock the partials regardless of small delay shifts (`research/O-Contrabass-bass-waveguide-stability.md` §2.2 lines 117–122). If the audible test fails (steady-state pitch DOES drift at 100 % stiffness), that is a Phase 2.4 follow-up RESEARCH item, not a Phase 2.1c blocker.

**`bridgeSamples` clamp safety:** with `dispersionDelay` up to 4 samples, the lowest reachable `bridgeSamples` value in the working envelope is `(totalDelay - filterGroupDelay) * bowPosition - 4`. At E1 (totalDelay ≈ 1070) and bowPosition=0.10 (β floor, the most aggressive case), `(1070 - 13) * 0.10 - 4 ≈ 105.7 - 4 = 101.7` samples. Well above the Lagrange3rd 4-tap minimum. No clamp regression. At G3 (Phase 2.2, totalDelay ≈ 225) the calculation tightens but is still safe.

## 14.8 WaveguideString.cpp Stale Comment Update (R17 housekeeping)

`WaveguideString.cpp` lines 170–171 currently read:

```cpp
// Step 6: Symmetric injection into both rails (canonical Smith two-port).
// [Phase 2.1c placeholder] dispersion will run on the BRIDGE rail's
//  outgoing wave only, BEFORE the algebraic saturator below.
```

**This comment is stale.** `CONTEXT.md` rev-3 Q1 lock places dispersion **before bridge LP** (between Step 1 popSample and Step 2 bridge LP), NOT before the saturator (Step 7). The Step-6 comment was written before the Q1 decision was settled.

**Plan-phase R17 directive:** during the dispersion wiring edit, replace this comment with a forward-pointer:

```cpp
// Step 6: Symmetric injection into both rails (canonical Smith two-port).
//  (Dispersion already ran in Step 1.5, between bridgeRaw popSample and bridge LP —
//   bridge rail only, per ARCHITECTURE.md §"Cascaded Allpass Dispersion" and
//   §"Processing Order"; mirrors O-Bowed bridge-rail-only loop chain.)
```

(`Step 1.5` is colloquial — the actual code edit may renumber Step 1 or insert a Step 1b. PLAN-phase fixes the exact wording.)

The header at `WaveguideString.h` line 38 already documents the correct chain: `[Phase 2.1c: dispersion] → bridge LP → −1 boundary → ...`. No header edit needed beyond updating the "Phase 2.1a omits dispersion; placeholder lives at the friction-write boundary" sentence at line 40-41 to reflect that dispersion has now landed.

## 14.9 Coefficient Sanity Checks (extra plan-phase belt-and-braces)

Beyond the closed-form clamp at `[-0.99, 0.99]`, PLAN rev-5 R17 should add belt-and-braces guards against pathological inputs:

| Guard | Where | Why |
|---|---|---|
| `if (!std::isfinite(a)) a = 0.0f;` | Voice's per-block `a` computation, before push to `setDispersionCoefficient` | The closed form's `lB = std::log(B)` returns `-inf` for `B=0`; the clamp at `B → 1e-9f` already prevents this, but defensive `isfinite` mirrors `WaveguideString.cpp:144` for `bridgeY` recovery. Cheap insurance. |
| `juce::jlimit(20.0f, 5000.0f, f0)` before `computeAllpassCoefficient` | Voice (per-block) | E1 = 41.2 Hz, scordatura −1200 cents = 20.6 Hz; G3 = 196 Hz; future open strings stay < 250 Hz; clamp upper to 5 kHz as a paranoia bound. |
| `static_assert(MaxSections >= 1)` in template | `DispersionFilter.h` | Compile-time guard against zero-section instantiation. |

## 14.10 Risk-Surface Refinement for PLAN rev-5

The six risks listed in `CONTEXT.md` rev-3 §"Risks" stand. Two refinements based on this research pass:

**Refinement to Risk #2 (group-delay subtraction wrong → pitch drifts):** the §14.7 split-aware compensation choice (subtract from `bridgeSamples` only, not from `compensated`) is what makes the bit-exact regression at stiffness=0 work. PLAN rev-5 R17 must implement option (i) above; option (ii) (subtract from `compensated`) breaks bit-exactness.

**New Risk #7 (closed-form clamp saturation at E1):** §14.2 anomaly — at I=8.0 (E1), `-C/k ≈ 15` for all B, clamping to `+0.99`. The audible STRING_STIFFNESS sweep may be flatter than expected. **Mitigation:** R18's `--stiffness-sweep` audition is the surfacing mechanism; if R18 reveals the sweep is musically uninteresting, file as Phase 2.4 follow-up (calibration polynomial for bass register), do NOT block Phase 2.1c. Gate 3 stability and the bit-exact regression bar still exit cleanly.

## 14.11 Sequencing in PLAN rev-5

The plan-phase task breakdown (rev-5) is expected to look like:

| Task | Description | Source |
|---|---|---|
| **R16-pre / R16a** | Add `--string-stiffness <float>` CLI flag to `tests/render-harness/main.cpp` (mirrors `--infinite-sustain` at lines 105–109). Build harness. Render `e1-bowon-only-stiffness-zero-pre.wav` + `.json` + sha256. **No DSP source edits.** | §14.7 |
| R16 | Write new file `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` per §14.5 skeleton. Add file to `plugins/O-Contrabass/CMakeLists.txt` `target_sources`. | §14.5 |
| R17 | Edit `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}`: add `DispersionFilter<4> bridgeDispersion` member; add `setDispersionCoefficient(float a)` setter; add `advanceStiffnessSmootherBy(int)` + `getCurrentSmoothedStiffness()` accessors; insert dispersion processing between Step 1 popSample and Step 2 bridge LP on bridge rail; update `updateDelayLengths()` per §14.7 split-aware compensation; update Step-6 stale comment per §14.8. | §14.4, §14.7, §14.8 |
| R17b | Edit `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` `renderNextBlock`: add per-block `a`-computation block (advance smoother, compute `a` via `DispersionFilter<4>::computeAllpassCoefficient`, push to waveguide via `setDispersionCoefficient`). | §14.4 |
| R18 | Add `--stiffness-sweep` CLI flag to `tests/render-harness/main.cpp` per §14.6. Build. Render `e1-stiffness-sweep.wav` + `.json` + sha256. Audition in Logic for click-free continuous timbral change. | §14.6 |
| R19 | Re-render `e1-bowon-only-stiffness-zero-post.wav` with same CLI as R16-pre. `cmp` byte-equal vs. R16-pre golden. Re-run bow-on-only 65 s harness at INFINITE_SUSTAIN=1.0 (4/4 invariants TRUE). auval + pluginval-10 PASS. Logic AU smoke at STRING_STIFFNESS = 0 / 50 / 100 % E1 sustained tone. | `CONTEXT.md` Gate 3 §"Test Criteria" |
| R20 | **Gate 3 atomic commit** — `DispersionFilter.h` + `WaveguideString.{h,cpp}` + `BowedContrabassVoice.{h,cpp}` + `tests/render-harness/main.cpp` (`--string-stiffness` + `--stiffness-sweep` flags) + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION/STATUS updates) — all in one commit, only on Gate 3 PASS. | `CONTEXT.md` rev-3 §"Approach Decisions" → "Atomic commit unit" |

R16-pre / R16a (harness pre-flight) is structurally a **prerequisite** to R16 because the golden reference must be captured before any DSP source edits land. R16-pre is also independently mergeable (harness CLI extension only — no semantic risk to plugin behaviour), justifying the early position in the sequence.

## 14.12 Open Items for Plan Phase

Deliberately deferred to PLAN rev-5 (i.e. not pinned by this research-phase pass):

1. **Confirm `WaveguideString::stiffnessSmoothed` accessor names.** Recommended: `advanceStiffnessSmootherBy(int numSamples)` + `getCurrentSmoothedStiffness() const`. PLAN rev-5 R17 may bikeshed names; the contract is the per-block "advance + read" pair.
2. **Decide on per-sample-`a` interpolation fallback location.** §14.4 leaves it unimplemented; if R18 sweep produces clicks, decide whether the per-sample interpolation goes inside `WaveguideString::processSample` or as a separate `DispersionCoefficientRamp` helper. Bias: keep inside `WaveguideString` to avoid a fourth file.
3. **Confirm `tests/render-harness/main.cpp` block-rate parameter cadence is sufficient.** §14.6 sketches per-block `setValueNotifyingHost`; if APVTS in plugin-host context needs a parameter-change settle delay (it shouldn't — parameters are in-process), R18 surfaces it.
4. **Decide whether to commit `e1-bowon-only-stiffness-zero-pre.wav` into git** (mirrors the §13.7 item-5 question for O-Bowed canonical). Recommendation: **commit the sha256** in `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` (text file, ~75 bytes) — do NOT commit the binary WAV (~22 MB at 24-bit stereo / 65 s — too large for git; reproducible from harness on demand).
5. **Pin the `--stiffness-sweep` JSON `rmsByDecade` array semantic.** §14.6 sketches it but does not pin: 6 s windows × 10 deciles? Or 1 s windows × 60 deciles? Recommendation: **6 s × 10 deciles** — coarse enough to be readable in JSON, fine enough to surface mid-sweep dropouts. PLAN rev-5 R18 finalises.
6. **CMake dependency for sha256 computation in JSON output.** The existing harness (`main.cpp` lines 248–274) does NOT currently compute sha256 in the JSON. Adding sha256 emission requires `juce::SHA256` (in `juce_cryptography` module) — confirm the harness target's `target_link_libraries` includes `juce::juce_cryptography`, or compute sha256 externally via `shasum -a 256` and inject into JSON post-render. Recommendation: **external `shasum`** for the harness — avoids adding a JUCE module dependency for a single text-output feature.

## 14.13 Summary — Phase 2.1c Research Plan

This research-phase pass:

1. ✅ Pinned the literal `m1..m4`, `k1..k3` constants (Q1) from Rauhala/Välimäki 2006 IEEE SP Letters Table 1, with `B`/`M`/`f0` envelope and a flagged anomaly (E1 sits at the edge of paper validity, clamping to `a≈+0.99`).
2. ✅ Resolved Q2 (group-delay formula) with option (b) at-f0 closed form, and derived the identity-at-`a=0` proof.
3. ✅ Resolved Q3 (setter API) with option (a) `setDispersionCoefficient(float a)` — voice computes, waveguide consumes.
4. ✅ Resolved Q4 (template/class shape) with option (c) `template<int MaxSections> class DispersionFilter` + runtime `activeSections` — full skeleton spec for PLAN rev-5 R16.
5. ✅ Resolved Q5 (harness output) with option (a) single-WAV ramp + JSON metadata + `sha256` field.
6. ✅ Specified pre-flight bit-exact baseline procedure as R16-pre / R16a (deferred to execute) — keeps research read-only on production code.
7. ✅ Identified split-rail compensation subtlety in `updateDelayLengths()` (subtract from `bridgeSamples` directly, NOT from `compensated`) — required for bit-exact regression at stiffness=0.
8. ✅ Refined `CONTEXT.md` Risk #2 + added Risk #7 (closed-form clamp saturation at E1).
9. ✅ Listed PLAN rev-5 task sequencing R16-pre → R16 → R17 → R17b → R18 → R19 → R20.
10. ✅ Listed 6 plan-phase open items.

**No production or test-code edits in this research-phase pass.** All edits are spec-only in `RESEARCH.md` §14 (this section) + the eventual `CONTEXT.md` audit-trail update. Execute-phase R16–R20 implements; verify-phase R19 confirms Gate 3 invariants; R20 atomic commit lands.

## 14.14 References (§14 append)

**Papers (closed-form derivation):**

- Rauhala, J., & Välimäki, V. (2006). "Tunable dispersion filter design for piano synthesis." *IEEE Signal Processing Letters*, Vol. 13 No. 5, Table 1 — literal `m1..m4, k1..k3` constants. https://ieeexplore.ieee.org/document/1618690
- Rauhala, J., & Välimäki, V. (2006). "Dispersion modeling in waveguide piano synthesis using tunable allpass filters." *Proc. DAFx-06*, pp. 71–76 — companion derivation.
- Karjalainen, M., Välimäki, V., & Tolonen, T. (1998). "Plucked-string models: From the Karplus-Strong algorithm to digital waveguides and beyond." *Computer Music Journal*, 22(3), 17–32 — seminal cascaded-allpass dispersion treatment + group-delay formulas.
- Smith, J. O. (2010). *Physical Audio Signal Processing*. CCRMA. §"Allpass Filters" — at-f0 vs at-DC group-delay derivation.

**Local research (already on disk):**

- `research/O-Contrabass-bass-waveguide-stability.md` §2.3 lines 145–161 — Rauhala/Välimäki cascade design closed form.
- `research/O-Contrabass-bass-waveguide-stability.md` §2.4 lines 199–241 — C++ implementation pattern (template+activeSections); direct ancestor of the §14.5 spec.
- `research/O-Contrabass-bass-waveguide-stability.md` §2.3 line 172 — per-string M=4/3/2/1 table (Phase 2.2 hook).
- `research/O-Contrabass-bass-waveguide-stability.md` §2.3 line 161 — paper validity envelope `B ∈ [1e-6, 1e-3], M=4 → a ∈ [-0.05, -0.5]` (piano register).

**Source files inspected:**

- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` lines 37–51 (loop-chain contract, stiffnessSmoothed member, deferred-dispersion comments).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` lines 64–95 (`updateDelayLengths`, `setDelaySamples`); lines 129–191 (`processSample`, including the stale Step-6 comment at lines 170–171); lines 223–230 (`setStringStiffness` smoother drive).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 200–237 (`updateParametersFromAPVTS`, the per-block waveguide-setter wiring point).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` lines 56–62 (STRING_STIFFNESS APVTS factory default = 0.30).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` lines 50–88 (`Args` struct + `parseArgs`); lines 100–112 (param-override pattern at prepareToPlay); lines 138–179 (per-block render loop).
- `plugins/O-Contrabass/.planning/parameter-spec.md` STRING_STIFFNESS row (default 0.30, range 0.0–1.0).

**Planning artefacts cross-referenced:**

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-3 §"Open Questions" (Q1–Q5 — resolved here).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-3 §"Approach Decisions" Q1 (placement = before bridge LP).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-3 §"Test Criteria" item 7 (bit-exact regression at stiffness=0).
- `plugins/O-Contrabass/research/ARCHITECTURE.md` §"Cascaded Allpass Dispersion" (line 417 placement directive); §"Processing Order" (line 267 chain order); §"String Waveguide Bank" (line 81 `B = 1e-4·STRING_STIFFNESS`).
- `plugins/O-Contrabass/.planning/STATUS.md` `next_action` field 2026-04-27 (research-phase scope: closed-form constants + group-delay + Q3-Q5 + pre-flight baseline).
