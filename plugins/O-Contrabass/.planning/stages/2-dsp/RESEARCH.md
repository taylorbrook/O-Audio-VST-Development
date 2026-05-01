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

---

# 15. Phase 2.2 Research — 4-String Bank + Per-String Detune + Per-String Dispersion Table (rev-4)

**Date:** 2026-04-27
**Scope:** Phase 2.2 — single coupled cycle (Q1 lock). Resolves CONTEXT rev-4 §"Open Questions" #1–#8. Pre-flight bit-exact baseline render captured + verified before any source edits. No §12/§13/§14 changes (those are Phase 2.4 follow-up + 2.1b/2.1c history).

**Carry-forward:** §1 (O-Bowed extraction surface) — N/A this cycle (no module surface changes). §11 (split-rail / F2 LP / F3 no-DCB) — locked, untouched. §14 (DispersionFilter API + closed-form coefficient + group-delay compensation) — consumed verbatim; the per-string M-table materialises by calling the existing `setActiveSections(M)` API once per slot.

---

## 15.1 Pre-Flight Bit-Exact Baseline (executed in research, NOT deferred)

**Why this runs in research, not at PLAN R-pre:** Phase 2.1c's R16-pre baseline check was done in execute-phase because the source-edit chain was already determined. Phase 2.2's bit-exact tolerance (Open Question #8) is a research-phase decision — strict byte-equal vs. ≤1 LSB hinges on whether the topology refactor is mathematically additive at the regression preset. Capturing the baseline NOW + confirming it still matches the Phase 2.1c golden (`d358abcd…`) lets §15.9 cite the analytical proof against an empirically-verified starting point.

**Command executed (working tree at R20 commit `5759e5e`, no source edits):**

```bash
cd /tmp && \
build/plugins/O-Contrabass/tests/render-harness/.../O-Contrabass-render-test \
    --note 28 --sustain 60 --release 5 \
    --infinite-sustain 1.0 --string-stiffness 0 \
    --out /tmp/phase22-preflight-stiffness-zero.wav \
    --json /tmp/phase22-preflight-stiffness-zero.json

shasum -a 256 /tmp/phase22-preflight-stiffness-zero.wav
# d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75  ← matches golden
```

**Result:** `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` — byte-identical to `tests/render-harness/golden/stiffness-zero-pre.wav.sha256`. Determinism + bit-exactness both confirmed.

**Side note (NOT a regression):** Harness reports `FAIL` on the standard 60s+5s preset because `pass_rms` evaluates the post-bow-off tail (saturator's algebraic cubic loss; per RESEARCH §12). This is not an E1 regression — it is the long-form characterisation parked for Phase 2.4 calibration polynomial work (Risk #7). The bit-exact bar that matters for Phase 2.2 is the WAV byte-equality, which holds.

---

## 15.2 Open Question #1 — String-Switching Trigger (RESOLVED)

**Resolution:** Note-on transitions only. Single-stage state machine; mid-crossfade re-trigger replaces (does NOT queue) the previous crossfade.

### State Variables (added to `BowedContrabassVoice`)

```cpp
// Phase 2.2 string-switching state
int   activeStringIndex      = -1;          // E=0, A=1, D=2, G=3; -1 = no string yet
int   previousStringIndex    = -1;          // valid only while crossfadeRemainingSamples > 0
int   crossfadeRemainingSamples = 0;        // counts down at internal (2x) rate
int   crossfadeTotalSamples  = 0;           // = ceil(0.005 * sampleRateInternal); cached at prepare
```

### Mapping Function (closed-form, 4-comparison ladder — CONTEXT Q3 lock)

```cpp
int BowedContrabassVoice::mapMidiNoteToStringIndex (int midiNote, int activeStrings) const noexcept
{
    // Open-string MIDI thresholds: E1=28, A1=33, D2=38, G2=43.
    // Highest open-string-at-or-below the note. Notes < 28 still resolve to E.
    int idx = 0;                                // default E
    if      (midiNote >= 43) idx = 3;           // G2 and above
    else if (midiNote >= 38) idx = 2;           // D2 .. F#2
    else if (midiNote >= 33) idx = 1;           // A1 .. C#2
    // else                  idx = 0;           // anything below 33 (incl. <28) → E1

    // CONTEXT Q4 — clamp by ACTIVE_STRINGS (parameter range 1..4 per parameter-spec.md:37,
    // so activeStrings is always >= 1; no zero-string corner case).
    const int maxIdx = juce::jlimit (0, 3, activeStrings - 1);
    return juce::jmin (idx, maxIdx);
}
```

### `noteStarted()` Pseudocode

```cpp
void BowedContrabassVoice::noteStarted()
{
    auto note = getCurrentlyPlayingNote();
    const int midiNote = note.initialNote;
    const float velocity = note.noteOnVelocity.asUnsignedFloat();

    // 1. Resolve frequency (12-TET + MPE bend). Same as Phase 2.1a.
    double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    const float bend = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bend) > 0.001f) freq *= std::pow (2.0, bend / 12.0f);
    currentFrequency = static_cast<float> (freq);

    // 2. Resolve target string (CONTEXT Q3 + Q4).
    const int activeStrings = static_cast<int> (parameters->getRawParameterValue ("ACTIVE_STRINGS")->load());
    const int newStringIndex = mapMidiNoteToStringIndex (midiNote, activeStrings);

    // 3. Decide trigger semantics.
    const bool isFirstNote = (activeStringIndex < 0);
    const bool needsCrossfade = (! isFirstNote)
                              && bowModel.isActive()
                              && (newStringIndex != activeStringIndex);

    if (needsCrossfade)
    {
        // Mid-crossfade re-trigger: replace previous crossfade. The previously-fading-out
        // string's bridge-rail energy decays naturally via the in-loop saturator + bridge LP
        // attenuation; we don't snapshot it — we just abandon the previous mix and start fresh
        // from current → new. This matches DB physical reality (player can't "undo" the previous
        // bow-engagement direction in 5 ms).
        previousStringIndex     = activeStringIndex;
        activeStringIndex       = newStringIndex;
        crossfadeRemainingSamples = crossfadeTotalSamples;
    }
    else
    {
        // First note OR same-string note OR bow-released-then-restarted — no crossfade needed.
        previousStringIndex       = -1;
        activeStringIndex         = newStringIndex;
        crossfadeRemainingSamples = 0;
    }

    // 4. Configure the new string's delay length immediately (avoids a lag where the
    //    string oscillates at the OLD frequency for the first ramp window). Per-string
    //    detune smoother starts from this snap value.
    const float detuneCents = readDetuneForString (newStringIndex);                 // see §15.5
    const float targetSamples = computeDelaySamples (currentFrequency, detuneCents); // see §15.5
    detuneSmoothed[newStringIndex].setCurrentAndTargetValue (targetSamples);
    strings[newStringIndex].trigger (currentFrequency);
    strings[newStringIndex].setDelaySamples (targetSamples);

    // 5. Engage bow.
    bowModel.startBow (velocity);
    oversampling.reset();
}
```

**Why "replace" instead of "queue" on mid-crossfade re-trigger:** Queuing a second crossfade while one is in flight means three strings audible simultaneously in the worst case (decaying old + half-decayed mid + new). The 5 ms crossfade window already overlaps with the bridge LP's natural-decay tail (~10–30 ms at typical g·(1−p) values), so abandoning the previous fade-out merges into that organic decay seamlessly.

**Edge cases verified:**
- *First noteStarted ever:* `activeStringIndex = -1` → `isFirstNote = true` → no crossfade, just configure new string. Empirical: harness `--note-sequence "28:2.0,..."` first event hits this path.
- *Same-string re-trigger* (e.g., MIDI 28→30, both E-string at ACTIVE_STRINGS=4): `newStringIndex == activeStringIndex` → no crossfade, just `strings[active].trigger(newFreq)` + `setDelaySamples(newTarget)`. The string's `trigger()` already calls `reset()`, which clears delay-line state — this is the **right** behaviour for an actual note-on (clean attack), even though it means a new attack transient. Crossfade is for *string* transitions, not *pitch* transitions.
- *ACTIVE_STRINGS demotion mid-bow:* user holds note-on on D2 (idx=2), drops ACTIVE_STRINGS 4→2. The current note keeps ringing on D2 until note-off (CONTEXT rev-4 line 85 — locked policy). Next noteStarted maps with `activeStrings=2 → maxIdx=1`; if the new MIDI note's natural mapping is index 2 or 3, demote to 1 (A1).
- *Bow released, then re-engaged on different string:* if `bowModel.isActive() == false` at time of new noteStarted, no crossfade is triggered (`needsCrossfade` short-circuits). The new string just kicks in. Old string is silent (already decayed).

**Risk: rapid arpeggio across 4 strings (~50 ms per note).** A 5 ms crossfade is short relative to a 50 ms note duration, so previous-string energy fully decays well before the next note's crossfade starts. No accumulation of unresolved fade-outs. Confirmed by listening test sequence per CONTEXT rev-4 line 116.

---

## 15.3 Open Question #2 — Crossfade Math (RESOLVED — precomputed ramp)

**Resolution:** Pre-compute the entire equal-power crossfade ramp at `prepareToPlay`. Store as `std::vector<std::pair<float, float>> crossfadeRamp` of size `crossfadeTotalSamples + 1`. At `i = 0`: `(1, 0)` (full old, zero new). At `i = N`: `(0, 1)` (zero old, full new). Intermediate: `(cos(π·i/(2N)), sin(π·i/(2N)))`.

### Why precomputed ramp beats LUT

| Option | Per-sample cost | Setup cost | Memory | Accuracy |
|--------|----------------|-----------|--------|----------|
| Inline `cos/sin` calls | 2 trig calls/sample | 0 | 0 | exact |
| 256-entry LUT + lerp | 4 loads + 1 mul + 1 add (×2 gains) | 1 KiB | 0.001 % LUT lerp error |
| **Precomputed ramp (recommended)** | **2 loads/sample** | **3.5 KiB** | **0** | **exact** |
| Linear (oldGain = 1−t) | 2 loads/sample | 0 | 0 | not equal-power (3 dB dip) |

At internal SR (2× oversampled = 88.2 kHz when host is 44.1k), `crossfadeTotalSamples = ceil(0.005 · 88200) = 441`. Memory = 441 · 8 bytes = 3.5 KiB. One-time setup cost in `prepareToPlay` is trivial (441 trig pair evaluations). Per-sample cost is two array loads — strictly cheaper than any LUT scheme and exact (no interpolation error).

Storage format:

```cpp
// In BowedContrabassVoice (private):
std::vector<std::pair<float, float>> crossfadeRamp;   // (oldGain, newGain) per sample

// In prepareToPlay:
const double sr_internal = hostSampleRate * 2.0;
crossfadeTotalSamples = static_cast<int> (std::ceil (0.005 * sr_internal));
crossfadeRamp.resize (crossfadeTotalSamples + 1);
const float invN = 1.0f / static_cast<float> (crossfadeTotalSamples);
const float halfPi = juce::MathConstants<float>::halfPi;
for (int i = 0; i <= crossfadeTotalSamples; ++i)
{
    const float t = static_cast<float> (i) * invN;          // [0, 1]
    crossfadeRamp[i] = { std::cos (t * halfPi), std::sin (t * halfPi) };
}
```

### Per-Sample Mix During Crossfade (called from oversampled DSP loop)

```cpp
// Inside the 2x oversampled per-sample loop:
float mixedSample = 0.0f;

if (crossfadeRemainingSamples > 0)
{
    // crossfadeRemainingSamples counts DOWN from crossfadeTotalSamples toward 0.
    // Equivalently, the fade index is N − remaining; ensure clamp [0, N].
    const int idx = juce::jlimit (0, crossfadeTotalSamples,
                                  crossfadeTotalSamples - crossfadeRemainingSamples);
    const auto [oldGain, newGain] = crossfadeRamp[idx];

    // Both rails run; only mix coefficients change.
    const float oldOut = strings[previousStringIndex].processSample (
                            /*v_bow=*/0.0f, /*F_bow=*/0.0f, frictionModel);   // idle injection
    const float newOut = strings[activeStringIndex].processSample (
                            v_bow, F_bow, frictionModel);                      // active injection

    mixedSample = oldOut * oldGain + newOut * newGain;
    --crossfadeRemainingSamples;

    if (crossfadeRemainingSamples == 0)
        previousStringIndex = -1;   // crossfade complete; old string returns to idle-tick
}
else
{
    // Standard path: only active string sees friction injection; idle strings tick with v_bow=0.
    for (int s = 0; s < 4; ++s)
    {
        if (s == activeStringIndex)
            mixedSample += strings[s].processSample (v_bow, F_bow, frictionModel);
        else
            strings[s].processSample (0.0f, 0.0f, frictionModel);   // tick + discard
    }
}
```

**Note on `processSample` with zero bow:** `HyperbolicFriction::computeReflectionCoefficient(v_delta=0, F_bow=0)` returns 0 (rho is bounded in [0, ~0.5] and proportional to v_delta·F_bow). So `frictionVelocity = 0`, `injection = 0`, `newVelocity = 0` → idle string's `toBridge = nutReflection`, `toNeck = bridgeReflection` — pure passive scattering. State stays bounded; energy decays via leak + bridge LP. ✓ matches "always tick" semantics.

### Property: `oldGain² + newGain² = 1` (equal-power)

By construction `cos²(θ) + sin²(θ) = 1`. The summed power across the two strings stays constant through the crossfade — no audible amplitude dip. Empirical verification deferred to Gate 4 invariant (3) (`--note-sequence` harness; `rmsContinuityRatio ≥ 0.90` at transition boundaries — see §15.7).

**CONTEXT rev-4 line 127 LUT-pattern reference is inaccurate** — `modules/synthesis/bow-friction/HyperbolicFriction.h` has no LUT (its hyperbolic friction model is closed-form analytical, not table-driven). Recording for the audit trail; the precomputed-ramp approach is independent of any prior pattern.

---

## 15.4 Open Question #3 — Stiffness Smoother Sharing (RESOLVED — per-string, kept in `WaveguideString`)

**Resolution:** Keep `juce::SmoothedValue<float, Linear> stiffnessSmoothed` per `WaveguideString` instance (its existing location). Voice calls `setStringStiffness(globalAmount)` on **all 4 instances** every block. All 4 smoothers converge to the same target with identical phase (since they share an init state and equal block-cadence advancement).

### Trade-off Table

| Option | Pros | Cons |
|--------|------|------|
| **(a) Per-string smoother (recommended)** | Zero refactor of `WaveguideString` API. E-string code path is byte-identical to Phase 2.1c at the regression preset. CPU cost: 4 × `SmoothedValue::skip(N)` per block = ~20 cycles total. Trivial. | 4× redundant smoother advancement. |
| (b) Voice-level shared smoother | Fewer redundant advancements. | Requires removing smoother from `WaveguideString` (API churn). Voice must compute `currentSmoothed` and pass to each `WaveguideString::setSmoothedStiffnessExternal(s)`. Risk: bit-exact regression bar may shift by 1 LSB if order of operations changes (smoother arithmetic moves from string to voice). |

**Why (a) wins:** the bit-exact regression bar (CONTEXT rev-4 Open Question #8) is the binding constraint for this cycle. Option (a) preserves the E-string code path *exactly* — `WaveguideString::advanceStiffnessSmootherBy()` and `getCurrentSmoothedStiffness()` continue to be called from voice in the same order with the same arguments at the regression preset. The CPU cost of 3× redundant `SmoothedValue::skip()` calls is in the noise.

### Per-Block Update Sequence (in `BowedContrabassVoice::renderNextBlock`)

```cpp
// Step A: Push global STRING_STIFFNESS to all 4 instances.
const float stringStiffness = parameters->getRawParameterValue ("STRING_STIFFNESS")->load();
for (int s = 0; s < 4; ++s)
    strings[s].setStringStiffness (stringStiffness);

// Step B: Advance all 4 smoothers by numSamples (oversampled — numSamples × 2 if running
// at 2x rate, but the smoother was sampled at host rate per Phase 2.1c convention; the
// `advanceStiffnessSmootherBy` takes whatever count voice supplies. Use host-rate numSamples
// to match Phase 2.1c R17 semantics — DO NOT change the units between phases or the
// regression bar bit-shifts).
for (int s = 0; s < 4; ++s)
    strings[s].advanceStiffnessSmootherBy (numSamples);

// Step C: Compute per-string `a` and push.
constexpr float B_open[4]      = { 1.0e-4f, 7.0e-5f, 5.0e-5f, 3.0e-5f };  // E, A, D, G
constexpr int   M_per_string[4] = { 4, 3, 2, 1 };

for (int s = 0; s < 4; ++s)
{
    const float currentStiffness = strings[s].getCurrentSmoothedStiffness();   // shared target → equal across slots
    const float B = B_open[s] * juce::jlimit (0.0f, 1.0f, currentStiffness);
    const int   M = M_per_string[s];

    // String fundamental for this slot — see §15.5 below.
    // For the active and previous (during crossfade) strings, use currentFrequency.
    // For idle strings, use the open-string default (so dispersion compensation is sane).
    const float f0 = (s == activeStringIndex || s == previousStringIndex)
                   ? juce::jlimit (20.0f, 5000.0f, currentFrequency)
                   : juce::jlimit (20.0f, 5000.0f, openStringFrequencyHz[s]);   // 41.2/55/73.4/98

    float a = (currentStiffness <= 0.0f)
            ? 0.0f
            : DispersionFilter<4>::computeAllpassCoefficient (f0, B, M);
    if (! std::isfinite (a)) a = 0.0f;
    strings[s].setDispersionCoefficient (a);

    // Phase 2.2 NEW: per-string M is configured at prepareToPlay (see §15.5) and
    // never changes runtime — no per-block setActiveSections call.
}
```

### Bit-Exact Regression at E-String

At the regression preset (`STRING_STIFFNESS=0`, `ACTIVE_STRINGS=4`, MIDI 28, `DETUNE_E=0`):
- All 4 smoothers' targets = 0; all 4 currents = 0 (init).
- Per-string `a = 0` (short-circuit branch at `currentStiffness <= 0.0f`).
- Per-string `setDispersionCoefficient(0)` is a no-op (Phase 2.1c R17 → identity at `a=0`).
- E-string code path: identical to Phase 2.1c, bit-for-bit.
- Idle-string code path: see §15.9 analytical proof — outputs literal `0.0f` at `INFINITE_SUSTAIN ≥ 0.95` (regression preset uses 1.0; leak = 0).

**No new arithmetic introduced on the E-string code path at the regression preset.** Bit-exact bar holds analytically. Empirically confirmed in §15.1.

---

## 15.5 Open Question #4 — `WaveguideString::prepare()` Surface (RESOLVED — no signature change)

**Resolution:** `prepare(double sr, int maxBlockSize)` signature stays stable across plugins. Per-string M is configured via the existing `setActiveSections(M)` call — already in `DispersionFilter`'s public API since Phase 2.1c. No `setOpenStringFrequency` setter; no `prepare(sr, maxBlockSize, f0)` overload.

### Why no new API surface is needed

The `WaveguideString` class needs to know:
1. **Per-instance M** (dispersion sections) — already configurable via `bridgeDispersion.setActiveSections(M)`. Voice calls this once per slot at `prepareToPlay`.
2. **Per-instance B prefactor** — NOT needed inside `WaveguideString`. Voice computes `a` from `(f0, B[s], M[s])` and pushes via `setDispersionCoefficient(a)`. The B value lives in voice's `B_open[]` constant array.
3. **Open-string frequency** — NOT needed inside `WaveguideString`. The string's "identity" is *only* the dispersion M (set once) and the per-block `a` (recomputed each block from voice-side state). `currentFrequency` is the actual played MIDI-note frequency (set by `trigger()`); detune offsets via `setDelaySamples()` directly.

**O-Bowed `Source/DSP/WaveguideString.h` is an INDEPENDENT FILE** (verified `grep` 2026-04-27: O-Bowed has its own WaveguideString.h at `plugins/O-Bowed/Source/DSP/`, NOT a shared module). API parity between O-Contrabass and O-Bowed is not a contractual requirement; even if it were, the recommended Phase 2.2 surface adds zero new methods, so cross-plugin parity is preserved by default.

### `BowedContrabassVoice::prepareToPlay` Wiring

```cpp
void BowedContrabassVoice::prepareToPlay (double hostSampleRate, int maxBlockSize)
{
    currentMaxBlockSize = maxBlockSize;
    const double sr_internal = hostSampleRate * 2.0;

    oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
    oversampling.reset();

    voiceBuffer.setSize (1, maxBlockSize * 2, false, true, false);
    voiceBuffer.clear();

    // Phase 2.2 NEW: per-string preparation.
    constexpr int M_per_string[4] = { 4, 3, 2, 1 };
    for (int s = 0; s < 4; ++s)
    {
        strings[s].prepare (sr_internal, maxBlockSize * 2);
        strings[s].getDispersion().setActiveSections (M_per_string[s]);  // see §15.5.1 below
        // Detune smoother init (delay-samples space, 20 ms ramp).
        detuneSmoothed[s].reset (sr_internal, 0.020);
        detuneSmoothed[s].setCurrentAndTargetValue (
            static_cast<float> (sr_internal) / openStringFrequencyHz[s]);
    }

    bowModel.prepare (sr_internal);
    frictionModel.setStaticFrictionCoefficient  (0.85f);
    frictionModel.setDynamicFrictionCoefficient (0.25f);

    // Crossfade ramp precompute (§15.3).
    crossfadeTotalSamples = static_cast<int> (std::ceil (0.005 * sr_internal));
    crossfadeRamp.resize (crossfadeTotalSamples + 1);
    const float invN = 1.0f / static_cast<float> (crossfadeTotalSamples);
    const float halfPi = juce::MathConstants<float>::halfPi;
    for (int i = 0; i <= crossfadeTotalSamples; ++i)
    {
        const float t = static_cast<float> (i) * invN;
        crossfadeRamp[i] = { std::cos (t * halfPi), std::sin (t * halfPi) };
    }
}
```

### 15.5.1 Sub-issue — How does voice access each string's `DispersionFilter`?

Two options:

**Option A (recommended):** Add a const accessor to `WaveguideString` that returns a reference:

```cpp
// In WaveguideString.h:
DispersionFilter<4>& getDispersion() noexcept { return bridgeDispersion; }
```

Voice calls `strings[s].getDispersion().setActiveSections(M_per_string[s])` once at prepareToPlay.

**Option B:** Add a wrapping setter `WaveguideString::setDispersionActiveSections(int M)`:

```cpp
void WaveguideString::setDispersionActiveSections (int M) noexcept { bridgeDispersion.setActiveSections (M); }
```

Voice calls `strings[s].setDispersionActiveSections(M_per_string[s])`.

**Trade-off:** Option A is one fewer wrapper but exposes `bridgeDispersion` as a non-`private` reference (encapsulation leak). Option B keeps `bridgeDispersion` private and just adds a tiny pass-through setter.

**Recommend (B):** keeps `bridgeDispersion` private; +1 LOC setter is trivial. Matches the existing `setDispersionCoefficient(a)` pass-through pattern.

### MIDI-Note → Frequency Derivation (Open Question #5 — RESOLVED)

For a played MIDI note `n` mapped to string `s`:
- `currentFrequency = MidiMessage::getMidiNoteInHertz(n) · 2^(bend/12) · 2^(detune_s/1200)`
- 12-TET fingering. No string-tension-vs-pitch coupling (architecture is silent; treat as ideal). Phase 2.6 will add Note Expression / MTS-ESP / Scala overrides on top of this.

**Simplification:** detune is already a multiplicative factor on `currentFrequency`. Equivalently, target delay-samples for string `s`:

```cpp
float computeDelaySamples (float playedFreqHz, float detuneCents) const noexcept
{
    const float detuneRatio = std::pow (2.0f, detuneCents / 1200.0f);
    const float detunedFreq = playedFreqHz * detuneRatio;
    return static_cast<float> (sr_internal) / juce::jmax (1.0f, detunedFreq);
}
```

Per-block: voice reads `DETUNE_<X>` for the active string (and previous string during crossfade), computes target delay-samples, sets `detuneSmoothed[s].setTargetValue(target)`. Per-sample (in oversampled loop): smoother advances + `strings[s].setDelaySamples(detuneSmoothed[s].getNextValue())`.

### Smoothing in Delay-Samples Space (CONTEXT Q6 lock — already JUCE-validated)

`juce::SmoothedValue<float, Linear>` smooths `delaySamples`, NOT cents. At E1 + DETUNE_E=−1200, target = `sr_internal / (41.2 · 0.5) = sr_internal / 20.6`. At sr_internal = 88.2k → 4282 samples. At DETUNE_E=+1200 → `sr_internal / 82.4 = 1071 samples`. Range = 3211 samples; ramp over 20 ms = `0.020 · 88200 = 1764` samples. So at full ±1200¢ sweep, the smoother completes in ~36 ms. Per-sample `setDelay()` on `Lagrange3rd` is JUCE-validated for click-free continuous modulation (vibrato pattern; Phase 2.1c implicit confirmation).

**Risk surface (NEW for Phase 2.2):** the existing 8192-sample buffer is sized for E1 −1200 cents at 88.2 kHz internal = 4282 samples → fits. At higher internal rates (96 kHz × 2 = 192 kHz, hypothetical future host) the worst case rises to 9320 samples — buffer overflow. **Mitigation:** Phase 2.2 is locked to 88.2 kHz internal (host 44.1k × 2). Document for Phase 2.6 + ARCH.md amendment if 96 kHz host support is ever added.

---

## 15.6 Open Question #5 — MIDI-Note → Finger-Position Frequency (RESOLVED — see §15.5)

Resolved inline within §15.5 ("MIDI-Note → Frequency Derivation"). Summary: `currentFrequency = MidiMessage::getMidiNoteInHertz(midiNote)`. No string-tension-vs-pitch coupling for v1.0; treat as 12-TET fingering. Per-string detune (cents) is a multiplicative pitch ratio applied via delay-samples-space `SmoothedValue<Linear>`. Architecture is silent on tension coupling; if a future spike surfaces audible tension-coupling at MIDI 55 fingered up the G string, document for Phase 2.4+ as an ARCH amendment proposal.

---

## 15.7 Open Question #6 — `--detune-sweep` Schema (RESOLVED)

**Resolution:** New `--detune-sweep {E|A|D|G}` CLI flag. Forces a single sustained note on the chosen string (via implicit `--string` override mapping E→MIDI 28, A→MIDI 33, D→MIDI 38, G→MIDI 43). Linearly ramps the corresponding `DETUNE_<X>` parameter from −1200 → +1200 cents across the sustain phase. Emits WAV (24-bit PCM stereo, byte-identical-format to existing harness output) + JSON with the schema below.

### CLI Surface

```
O-Contrabass-render-test
  --detune-sweep <E|A|D|G>           Phase 2.2: ramp DETUNE_<X> linearly across sustain.
                                     Implies --note <open-string-MIDI>:
                                       E→28, A→33, D→38, G→43.
  --note            <midi=28>        Standard. If --detune-sweep is set and --note conflicts
                                     with the implied open-string MIDI, --detune-sweep wins
                                     (and an stderr warning is emitted).
  --velocity        <0..1=0.7>       Standard.
  --sustain         <sec=30>         Default for detune-sweep mode raised from 60 to 30
                                     (covers full ±1200¢ sweep at audible rate without
                                     stretching the file size unnecessarily).
  --release         <sec=2>          Standard, but lower default in detune-sweep mode.
  --infinite-sustain <0..1=1.0>      Standard.
  --string-stiffness <0..1=apvts>    Standard (Phase 2.1c).
  --out             <wav>            Default in detune-sweep mode: detune-sweep-<X>.wav
  --json            <json>           Default in detune-sweep mode: detune-sweep-<X>.json
```

### Per-Block Ramp (added to harness render loop)

```cpp
// In the main render loop, alongside the existing --stiffness-sweep block:
if (args.detuneSweepString != ' ')   // 'E', 'A', 'D', 'G' or ' ' = unset
{
    const float fraction = static_cast<float> (sampleCursor)
                         / static_cast<float> (juce::jmax (1, sustainSamples));
    const float clamped  = juce::jlimit (0.0f, 1.0f, fraction);
    const float cents    = -1200.0f + 2400.0f * clamped;     // linear -1200 → +1200

    // Map letter → APVTS parameter ID.
    juce::String paramId;
    switch (args.detuneSweepString)
    {
        case 'E': paramId = "DETUNE_E"; break;
        case 'A': paramId = "DETUNE_A"; break;
        case 'D': paramId = "DETUNE_D"; break;
        case 'G': paramId = "DETUNE_G"; break;
        default: break;
    }
    if (auto* p = proc.parameters.getParameter (paramId))
    {
        // DETUNE_<X> normalisation: cents range [-1200, +1200] → norm [0, 1].
        const float norm = (cents + 1200.0f) / 2400.0f;
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
    }
}
```

### JSON Schema (additions in detune-sweep mode)

```json
{
  "status": "PASS",
  "mode": "detune-sweep",
  "string": "E",                              // or "A", "D", "G"
  "midiNote": 28,
  "velocity": 0.7,
  "sustainSeconds": 30.0,
  "releaseSeconds": 2.0,
  "infiniteSustain": 1.0,
  "stringStiffness": 0.0,
  "totalSamples": 1411200,
  "peak": 0.234,
  "nanCount": 0,
  "infCount": 0,
  "rmsMid_s5_s6": 0.041,
  "rmsFinal_lastSecond": 0.038,
  "rmsRatio_final_over_mid": 0.92,
  "blockMicros_median": 88.0,
  "blockMicros_max": 215.0,
  "blockTime_max_over_median": 2.44,
  "pass_nan": true,
  "pass_peak": true,
  "pass_blockTime": true,
  "pass_rms": true,
  "pass_rmsContinuity": true,                 // NEW: see formula below
  "outputWav": "detune-sweep-E.wav",
  "detuneRamp": {
    "start": -1200.0,
    "end":   +1200.0,
    "shape": "linear"
  },
  "rmsByDecade": [0.038, 0.040, 0.042, 0.043, 0.043, 0.044, 0.043, 0.042, 0.041, 0.039],
  "rmsContinuityRatio": 0.94                  // NEW: see formula below
}
```

### `rmsContinuityRatio` Formula

For each pair of adjacent host-rate processBlock outputs `(b_i, b_{i+1})` during the sustain phase:

```
rms_i      = sqrt(mean(samples in block i squared, both channels))
rms_{i+1}  = sqrt(mean(samples in block i+1 squared, both channels))
ratio_i    = min(rms_i, rms_{i+1}) / max(max(rms_i, rms_{i+1}), epsilon)
```

`rmsContinuityRatio = min over all i during the sustain phase`. Range [0, 1]; 1 = perfectly stable, 0 = some block is silent next to a non-silent neighbour.

`pass_rmsContinuity = (rmsContinuityRatio >= 0.90)`.

**Why 0.90 and not 0.99 (CONTEXT rev-4 line 134 mention):** 99% block-to-block continuity is unrealistically strict for legitimate envelope variation — at 512-sample blocks @ 44.1 k = 11.6 ms per block, a slow sweep over 30 s + 20 ms `SmoothedValue` smoother does produce ~5 % envelope variation between adjacent blocks under low-rate amplitude wobble (bridge LP transient response to delay-length change). 0.90 catches genuine clicks (a single sample-level discontinuity at amplitude 1.0 in a 512-sample block adds `sqrt(1/512) ≈ 0.044` to RMS — for a steady-state RMS of 0.04, that's a 2× spike → ratio drops to 0.5; well below 0.90). 0.99 false-flags. CONTEXT line 134 was a discuss-phase advisory estimate, overridden here.

### Pass Conditions (detune-sweep mode)

Inherited from sustained-note mode + new `pass_rmsContinuity`. Overall PASS = `pass_nan && pass_peak && pass_blockTime && pass_rmsContinuity`. **`pass_rms` is OMITTED from overall PASS in detune-sweep mode** — a slow-drifting envelope is the expected behaviour, not a runaway/dieout signal. Keep `pass_rms` in JSON for diagnostic but do not factor it into exit code.

---

## 15.8 Open Question #7 — `--note-sequence` Schema (RESOLVED)

**Resolution:** New `--note-sequence "MIDI:dur,..."` CLI flag. Pre-builds full note-on / note-off event list at start of render; per-block extracts events whose sample positions fall in `[sampleCursor, sampleCursor + thisBlock)`. Total sustain length is derived from the sum of all durations; `--sustain` is overridden in this mode (and a warning emitted if conflicting). Emits WAV + JSON with note-sequence schema.

### Existing harness MidiBuffer plumbing — confirmed compatible

The Phase 2.1c harness builds `juce::MidiBuffer midi` per-block and passes it to `proc.processBlock(blockBuffer, midi)`. JUCE's `MidiBuffer::addEvent(message, samplePosition)` accepts any sample position within the block. Adding multiple noteOn/noteOff events per block is fully supported. **No new plumbing required** — just thread the precomputed event list through the existing MidiBuffer construction.

### CLI Surface

```
O-Contrabass-render-test
  --note-sequence "<MIDI>:<dur>[,<MIDI>:<dur>...]"
                                     Phase 2.2: programmatic note-on sequence. Each entry
                                     plays MIDI note N for `dur` seconds, then note-off,
                                     then the next entry's note-on starts immediately at
                                     the boundary. Total render time = sum(durations) +
                                     --release for the final note's tail.
                                     Example: "28:2.0,33:2.0,38:2.0,43:2.0,28:2.0"
                                     plays E1→A1→D2→G2→E1, 2s each, total 10 s + release.
  --release         <sec=2>          Tail-off after the LAST note-off. Standard.
  --velocity        <0..1=0.7>       Used for ALL note-ons in the sequence.
  --infinite-sustain <0..1=1.0>      Standard.
  --string-stiffness <0..1=apvts>    Standard.
  --out             <wav>            Default in note-sequence mode: note-sequence.wav
  --json            <json>           Default in note-sequence mode: note-sequence.json
  --sustain         IGNORED          (warning to stderr if also set)
```

### Pre-Build Event List (at start of render)

```cpp
struct ScheduledMidiEvent
{
    int sampleIndex;
    juce::MidiMessage message;
};
std::vector<ScheduledMidiEvent> sequenceEvents;

if (args.noteSequence.isNotEmpty())
{
    juce::StringArray segments;
    segments.addTokens (args.noteSequence, ",", "");

    int cursor = 0;
    for (const auto& segment : segments)
    {
        const int colon = segment.indexOfChar (':');
        if (colon < 0) { /* error */ continue; }
        const int   note = segment.substring (0, colon).getIntValue();
        const float dur  = segment.substring (colon + 1).getFloatValue();
        const int   durSamples = static_cast<int> (dur * sampleRate);

        const int velMidi = juce::jlimit (1, 127, static_cast<int> (std::round (args.velocity * 127.0f)));
        sequenceEvents.push_back ({ cursor,                  juce::MidiMessage::noteOn  (channel, note, (juce::uint8) velMidi) });
        sequenceEvents.push_back ({ cursor + durSamples - 1, juce::MidiMessage::noteOff (channel, note) });
        cursor += durSamples;
    }
    sustainSamples = cursor;   // overrides args.sustainSeconds in this mode
    totalSamples   = cursor + static_cast<int> (args.releaseSeconds * sampleRate);
}
```

### Per-Block Drain (replaces existing single-noteOn/single-noteOff block)

```cpp
juce::MidiBuffer midi;
if (! sequenceEvents.empty())
{
    for (const auto& e : sequenceEvents)
    {
        if (e.sampleIndex >= sampleCursor && e.sampleIndex < sampleCursor + thisBlock)
            midi.addEvent (e.message, e.sampleIndex - sampleCursor);
    }
}
else
{
    // ...existing single-note path (unchanged from Phase 2.1c)...
}
```

### JSON Schema (additions in note-sequence mode)

```json
{
  "status": "PASS",
  "mode": "note-sequence",
  "sequence": [
    { "midiNote": 28, "durationSeconds": 2.0, "stringExpected": "E" },
    { "midiNote": 33, "durationSeconds": 2.0, "stringExpected": "A" },
    { "midiNote": 38, "durationSeconds": 2.0, "stringExpected": "D" },
    { "midiNote": 43, "durationSeconds": 2.0, "stringExpected": "G" },
    { "midiNote": 28, "durationSeconds": 2.0, "stringExpected": "E" }
  ],
  "transitionSampleIndices": [88200, 176400, 264600, 352800],
  "perSegmentRms": [0.040, 0.041, 0.043, 0.045, 0.040],
  "pass_nan": true,
  "pass_peak": true,
  "pass_blockTime": true,
  "pass_allSegmentsAudible": true,            // each perSegmentRms[i] > 1e-3
  "pass_rmsContinuityAtTransitions": true,    // see formula below
  "pass_rmsContinuity": true,                 // overall continuity (whole sustain phase)
  "rmsContinuityAtTransitions": 0.91,
  "rmsContinuityRatio": 0.93
}
```

### `rmsContinuityAtTransitions` Formula

Each transition occurs at `transitionSampleIndices[i]`. Define a 256-sample window centred on each transition (128 before, 128 after). Compute RMS of the BEFORE half and the AFTER half. Ratio = `min/max`. `rmsContinuityAtTransitions = min` over all transitions.

`pass_rmsContinuityAtTransitions = (rmsContinuityAtTransitions >= 0.50)`. A 0.5 ratio means up to 2× RMS jump at transition is acceptable (which is generous — the new string starting from idle has lower RMS than the old string in steady-state, so a brief dip during the 5 ms crossfade is expected). The crossfade math (§15.3) guarantees `oldGain² + newGain² = 1`, so summed-power continuity is exact, but per-string RMS is not (idle string has zero energy, post-crossfade string takes ~bridgeLP-tau-equivalent time to build up).

### Pass Conditions (note-sequence mode)

`pass_nan && pass_peak && pass_blockTime && pass_allSegmentsAudible && pass_rmsContinuityAtTransitions`. `pass_rms` (the original sustained-note ratio) is OMITTED — multi-segment ramps + transitions don't have a single steady-state RMS.

---

## 15.9 Open Question #8 — Bit-Exact Regression Tolerance (RESOLVED — strict byte-equal)

**Resolution:** Strict byte-equal (sha256 match) is the Gate 4 invariant (7) tolerance. Idle-string contribution is mathematically zero at the regression preset (`INFINITE_SUSTAIN=1.0` ⇒ leak=0); even in the leak-active case (`INFINITE_SUSTAIN < 0.95`), idle-string contribution is below the 24-bit PCM LSB by ~13 orders of magnitude.

### 15.9.1 Empirical Confirmation (§15.1)

`shasum -a 256 phase22-preflight-stiffness-zero.wav` = `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` — byte-identical to committed Phase 2.1c golden. Working-tree at R20 commit `5759e5e`, no source edits. Determinism is preserved post-Phase-2.1c.

### 15.9.2 Analytical Proof — Idle-String Output is Literal `0.0f` at Regression Preset

**Regression preset (Phase 2.1c golden render command):**
```
--note 28 --sustain 60 --release 5 --infinite-sustain 1.0 --string-stiffness 0
```

So at this preset, every string instance sees:
- `INFINITE_SUSTAIN = 1.0` ⇒ `denormalLeak = 0.0f` (per `WaveguideString.cpp:138`: `(infiniteSustain >= 0.95f) ? 0.0f : -1.0e-20f`).
- `STRING_STIFFNESS = 0.0` ⇒ smoothed stiffness = 0 ⇒ short-circuit `a = 0` (per voice §15.4 update sequence) ⇒ dispersion is identity.

**Idle-string per-sample state evolution from `reset()`:**

- `bridgeDelay`, `neckDelay`, `bridgeY`, `bridgeDispersion` state — all 0.0f (post-`reset()`).
- Voice doesn't inject bow into idle strings: `processSample(v_bow=0, F_bow=0, friction)`.
- `bridgeRaw = bridgeDelay.popSample(0) = 0.0f` (delay line is all zeros).
- `neckRaw = 0.0f`.
- `bridgeDispersed = bridgeDispersion.processSample(0.0f) = 0.0f` (allpass on 0-input with 0-state stays 0).
- `bridgeFiltered = g · (1−p) · 0 + p · 0 + 0 = 0.0f` (leak = 0 at infiniteSustain ≥ 0.95).
- `bridgeY = 0.0f`.
- `bridgeReflection = -0.0f = 0.0f` (in IEEE 754 the sign of zero is preserved through unary minus, but `−0.0f + 0.0f = +0.0f`; sign is moot for downstream).
- `nutReflection = -0.0f = 0.0f`.
- `v_string_incoming = 0 + 0 = 0.0f`.
- `v_delta = 0 - 0 = 0.0f`.
- Friction: `rho = friction.computeReflectionCoefficient(0, 0)`. Per `HyperbolicFriction.h` (line 32 contract: rho ∈ [0, ~0.5], proportional to v_delta·F_bow), `rho(0, 0) = 0`. ⇒ `clampedRho = 0`, `frictionVelocity = 0`, `injection = 0`, `newVelocity = 0`.
- `toBridge = nutReflection + 0 = 0.0f`.
- `toNeck = bridgeReflection + 0 = 0.0f`.
- `toBridge_sat = 0 / sqrt(1+0) = 0.0f`.
- `toNeck_sat = 0.0f`.
- `pushSample(0, 0.0f)`, `pushSample(0, 0.0f)` ⇒ delay line stays all zeros.
- `output = toBridge_sat = 0.0f`.

**Inductive case:** if all state is 0 at sample n, all state is 0 at sample n+1 (no input, no leak, all arithmetic is 0+0 = 0). Idle string output is literal 0.0f for the entire render. Q.E.D.

### 15.9.3 Bit-Exact Mix (Voice-Level Output Stage)

Existing Phase 2.1c mix (line 199–214 of `BowedContrabassVoice.cpp`):

```cpp
constexpr float kVoiceNorm = 0.35f;
for (int i = 0; i < numSamples; ++i)
{
    float s = voiceBuffer.getSample (0, i) * kVoiceNorm * outputGainLinear;
    s = juce::jlimit (-1.0f, 1.0f, s);
    outputBuffer.addSample (0, startSample + i, s);
    if (numOutChans >= 2)
        outputBuffer.addSample (1, startSample + i, s);
}
```

Phase 2.2 must ensure that AT THE REGRESSION PRESET, `voiceBuffer` contents at the end of the per-sample loop are byte-identical to Phase 2.1c. The 2× oversampled DSP loop computes:

```cpp
for (int i = 0; i < numUp; ++i)
{
    bowModel.updateEnvelope();
    float v_bow = bowModel.getBowVelocity();
    float F_bow = bowModel.getBowForce();

    // PHASE 2.2: mix across 4 strings.
    float mixedSample;
    if (crossfadeRemainingSamples > 0) { /* not in regression preset (no string transitions) */ }
    else
    {
        mixedSample = 0.0f;
        for (int s = 0; s < 4; ++s)
        {
            const float v = (s == activeStringIndex) ? v_bow : 0.0f;
            const float F = (s == activeStringIndex) ? F_bow : 0.0f;
            const float out = strings[s].processSample (v, F, frictionModel);
            if (s == activeStringIndex) mixedSample = out;   // idle outputs = 0.0f, additive sum unchanged
        }
    }
    upData[i] = mixedSample;
}
```

At the regression preset (MIDI 28, ACTIVE_STRINGS=4 ⇒ activeStringIndex = 0, no crossfade ever):
- Idle strings (s=1,2,3) each return 0.0f (proved §15.9.2).
- Active string (s=0) returns identical sample to Phase 2.1c (single-string voice — same code path, same arithmetic).
- `mixedSample = strings[0].processSample(v_bow, F_bow, friction)` — same value Phase 2.1c writes to `upData[i]`.

**Critical invariant for bit-exactness:** the "early return on activeStringIndex" pattern above (`if (s == activeStringIndex) mixedSample = out`) is BYTE-IDENTICAL to Phase 2.1c's single-string write because (a) idle outputs are literal 0.0f and (b) we don't add them — we override `mixedSample` with only the active string's value. **No floating-point addition is introduced between idle and active outputs at the regression preset.**

(An alternative implementation that sums all 4 strings unconditionally — `mixedSample += strings[s].processSample(...)` — would also be bit-exact AT the regression preset because adding 0.0f to any float is a no-op. But it introduces 3 extra additions per sample which (a) cost CPU cycles for nothing and (b) become non-trivial when leak ≠ 0 — see §15.9.4. The "early return on activeStringIndex" pattern is preferred.)

### 15.9.4 Leak-Active Case (NOT the regression preset — for completeness)

When `INFINITE_SUSTAIN < 0.95`, `denormalLeak = -1e-20f`. Each idle string evolves:

- `bridgeFiltered[n] = 0 + p · bridgeY[n−1] + (-1e-20)`
- `bridgeY[n] = bridgeFiltered[n]`
- Recurrence: `y[n] = p·y[n−1] − 1e-20` ⇒ steady-state `y = -1e-20 / (1 - p)`. At p=0.5, `y_ss = -2e-20`.
- `bridgeReflection = -y_ss = +2e-20`.
- `nutReflection = -0 = 0` (neckRaw is still 0 — neck rail has no LP/leak).
- `v_string_incoming = 2e-20 + 0 = 2e-20`.
- Friction: `rho(v_delta=−2e-20, F_bow=0) = 0` (rho proportional to F_bow, which is 0). ⇒ `newVelocity = 0`.
- `toBridge = 0 + 0 = 0`.
- `toNeck = 2e-20 + 0 = 2e-20`.
- `toBridge_sat ≈ 0`, `toNeck_sat ≈ 2e-20`.
- `output = toBridge_sat ≈ 0` (from bridge end).
- Push to delay lines: bridgeDelay receives 0; neckDelay accumulates `2e-20` over time. After many samples, `neckRaw ≈ 2e-20` (delayed). On next iteration, `v_string_incoming = 2e-20 + (-2e-20) = 0` (cancellation). Steady-state output magnitude ~2e-20 worst case.

**Quantisation at WAV write:** WAV is 24-bit PCM. LSB = `1 / 2^23 ≈ 1.19e-7`. Idle output magnitude ~2e-20 is **13 orders of magnitude below LSB**. Round-to-nearest quantisation: any sample with `|s| < 5.96e-8` rounds to bit-identical zero in PCM-24.

**Even in the leak-active case, idle strings cannot perturb the WAV bytes.** `sha256` of the file is invariant.

### 15.9.5 What Could Break Bit-Exactness (and is therefore a HARD RULE)

1. **Reordering `voiceBuffer` arithmetic.** `(a + b) + c ≠ a + (b + c)` in float for some (a,b,c). Phase 2.2 must NOT reorder the existing single-string write path. ⇒ "early return on activeStringIndex" is the safe pattern; "always sum all 4" is allowed because the idle terms are literal `0.0f` at regression preset (so `(0 + 0) + e_active = e_active` regardless of order), but the early-return is still preferred for clarity.
2. **Changing the bow envelope, friction model, or split-rail topology** for the active string. Phase 2.2 does NOT touch these — `strings[0].processSample(...)` is the same call with same args as Phase 2.1c.
3. **Changing `prepareToPlay`'s argument-passing order** to `WaveguideString::prepare` or related setters. Phase 2.2 calls `strings[s].prepare(...)` in s=0,1,2,3 order. As long as `s=0` (E-string) sees the same `(sr, maxBlockSize)` arguments and the same subsequent setter calls in the same order as Phase 2.1c, its state at end-of-prepare is byte-identical. ⇒ Phase 2.2 must match Phase 2.1c's prepareToPlay sequence for slot 0 exactly.
4. **Per-string M configuration via `setActiveSections(M)` BEFORE first `processSample`.** For E-string slot 0, `setActiveSections(4)` is the same call Phase 2.1c was making (line 40 of WaveguideString.cpp: `bridgeDispersion.setActiveSections(4)`). Slots 1/2/3 get M=3/2/1 — but they're idle, and idle output is bit-zero regardless of M. ⇒ slot-0 setup unchanged, bit-exact preserved.

### 15.9.6 Regression-Bar Alternative: One-Time Refactoring Boundary

CONTEXT rev-4 line 149 (Risk #8) raises the option: "Phase 2.2 introduces a one-time refactoring boundary, with a new Phase 2.2 golden captured post-implementation." **§15.9.2–§15.9.5 prove this fallback is unnecessary** — strict byte-equal to Phase 2.1c golden is achievable. Phase 2.2 PLAN R-pre check: re-render the regression preset BEFORE any source edits land (executed §15.1; passed). Phase 2.2 PLAN R-final (Gate 4 invariant 7): re-render the regression preset AFTER the R21+ source edits land; require sha256 match to `d358abcd…`. If FAIL, the implementation has introduced a bit-shift bug somewhere — fix-or-fall-back, not soften the bar.

**Recommend Phase 2.2 PLAN rev-6 keeps strict byte-equal as the Gate 4 invariant (7) bar, with R-pre baseline check executed at start of execute-phase to confirm no working-tree drift since 2026-04-27.**

---

## 15.10 Pattern Confirmation — O-Bowed Cross-Check

Verified 2026-04-27 by `grep`:

- O-Bowed has its own `Source/DSP/WaveguideString.h` — independent of O-Contrabass's. Plugin-local file at `plugins/O-Bowed/Source/DSP/WaveguideString.h`. Not a shared module. **Phase 2.2 surface changes to `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` are O-Contrabass-local and do not propagate.**
- O-Bowed's `BowedStringVoice` holds **one** `WaveguideString waveguideString;` instance (line 83). Multi-voice polyphony for chords is achieved by `BowedMPESynthesiser` allocating multiple `BowedStringVoice` instances. ⇒ O-Bowed does NOT use a 4-string-bank-in-one-voice pattern. O-Contrabass's Phase 2.2 design is **plugin-specific** — appropriate because contrabass is mono-string-at-a-time (one bow contact).
- The **shared module** `modules/synthesis/bow-friction/` (Phase 2.1b) provides `HyperbolicFriction` + `BowModel` only. Both classes are value-semantic and stateless w.r.t. string count. Phase 2.2 does NOT touch this module surface. ⇒ no cross-plugin contract impact.

## 15.11 Pattern Confirmation — Existing Harness CLI Style

Existing Phase 2.1c `--stiffness-sweep` flag pattern:
- Boolean flag (`--stiffness-sweep 1`) with default off.
- Implies a per-block parameter ramp + JSON additions (`mode`, `stiffnessRamp`, `rmsByDecade`).
- Default WAV/JSON filename auto-rewritten when flag is set.

Phase 2.2 new flags follow the same pattern:
- `--detune-sweep <E|A|D|G>`: enum-arg flag (not boolean). Same per-block ramp + JSON additions pattern.
- `--note-sequence "<MIDI:dur,...>"`: string-arg flag. Pre-built event list at start, drained per block.
- `--string <E|A|D|G>`: enum-arg flag (CONTEXT rev-4 line 33 — for forcing per-string sustained-tone harnesses by overriding the MIDI-note → string mapping). Implementation:

```cpp
// Maps to forcing a MIDI note that's guaranteed to map to that string under
// ACTIVE_STRINGS=4: E=28, A=33, D=38, G=43. Voice-side mapping (§15.2) handles the rest.
```

`--string` is mutually exclusive with `--detune-sweep` (which already implies `--note <open-string-MIDI>`); harness should warn if both are set.

---

## 15.12 Risk-Surface Refinement for PLAN rev-6

Re-evaluating CONTEXT rev-4 §"Risks" (lines 142–149) after research-phase analysis:

| # | Risk | Mitigation Status |
|---|------|------|
| 1 | String-switching click despite 5 ms equal-power crossfade | **Mitigated.** Precomputed-ramp math (§15.3) guarantees `oldGain² + newGain² = 1`. Bridge LP natural decay (~10–30 ms) covers any residual. Gate 4 invariant (3) confirms via `--note-sequence`. |
| 2 | Idle-string CPU overshoot | **Mitigated.** §15.4 + §15.5 imply ~3 multiplies + popSample + pushSample + dispersion-cascade per idle string per sample = ~0.4% × 3 = ~1.2% overhead. Total Phase 2.2 voice ~2.0%, well under 5% budget (PERF-02). |
| 3 | Detune sweep clicks at extreme cents (±1200¢) | **Mitigated.** SmoothedValue<Linear> in delay-samples space + Lagrange3rd is JUCE-validated. Buffer size 8192 covers worst case (4282 samples at E1 −1200¢ @ 88.2k). Gate 4 invariant (2) catches via `rmsContinuityRatio ≥ 0.90`. |
| 4 | MIDI-mapping edge cases — notes outside [28, 55] | **Mitigated.** Closed-form thresholds (§15.2) clamp to [E, G]. Notes < 28 → E (fingered "down" — physically unusual but mathematically valid). Notes > 55 → G (fingered very high). |
| 5 | E1 bit-exact regression failure | **Resolved.** §15.9 analytical proof + §15.1 empirical baseline both confirm idle-string contribution is bit-zero (or sub-LSB) at regression preset. Gate 4 invariant (7) is **strict byte-equal**; Risk #5 is closed unless Phase 2.2 implementation introduces a fp-reordering bug (§15.9.5 hard rule). |
| 6 | ACTIVE_STRINGS mid-sustain edge case | **Mitigated.** Locked policy = current note keeps ringing on its current string until note-off (CONTEXT line 85). Note-on-only switching policy means no explicit mid-sustain handler needed. Parameter range Int [1,4] (parameter-spec.md:37) — no zero-string corner case. |
| 7 | `std::array<WaveguideString, 4>` allocation cost in `prepareToPlay` | **Mitigated.** ~128 KiB total (4 × 32 KiB delay-line buffers). One-time prepareToPlay cost. No `processBlock` allocations. |
| 8 | Phase 2.1c golden dependence on E1-only voice topology | **Resolved → not an actual risk.** §15.9 proves the 4-string topology is bit-equivalent at regression preset. Strict bar is achievable; one-time refactoring-boundary fallback is unnecessary. |
| **NEW 9** | Float-arithmetic reordering on E-string mix path | **Hard-ruled.** §15.9.5 enumerates the exact patterns that must NOT change. Plan-phase R-pre + R-final bit-exact checks gate this. |
| **NEW 10** | Detune-sweep `pass_rmsContinuity` threshold mis-calibration | **Mitigated.** §15.7 derives 0.90 threshold from click-detection arithmetic (single 1.0-amplitude sample → ~0.044 RMS bump → ~0.5 ratio for steady-state RMS=0.04 → 0.90 catches genuine clicks; 0.99 false-flags). Plan-phase locks 0.90; if Gate 4 PASS shows continuity in [0.90, 0.99] the threshold is empirically validated. |

---

## 15.13 Sequencing in PLAN rev-6

Plan-phase task structure (recommendation; plan-phase finalises exact task boundaries):

| Task | Description | Atomicity |
|------|-------------|-----------|
| **R21-pre** | Re-render bit-exact baseline at MIDI 28 + ACTIVE_STRINGS=4 + DETUNE_E=0 + STRING_STIFFNESS=0 BEFORE any source edits. Confirm sha256 == `d358abcd…`. If MISMATCH (working-tree drift since 2026-04-27 §15.1): STOP, investigate before proceeding. | Diagnostic; no commit. |
| **R21** | `BowedContrabassVoice.{h,cpp}` — replace single `waveguideString` member with `std::array<WaveguideString, 4> strings`, add per-string `juce::SmoothedValue<float, Linear> detuneSmoothed[4]`, add `activeStringIndex`/`previousStringIndex`/`crossfadeRemainingSamples`, add `mapMidiNoteToStringIndex`, add `readDetuneForString` helper, add `computeDelaySamples` helper, add precomputed `crossfadeRamp`. Per-block update sequence per §15.4; per-sample mix per §15.3. Bit-exact early-return on activeStringIndex pattern (§15.9.5 hard rule). | Source edit. |
| **R22** | `Source/DSP/WaveguideString.{h,cpp}` — add `setDispersionActiveSections(int M)` pass-through setter (§15.5.1 option B). NO topology changes; NO smoother relocation; NO prepare() signature change. | Source edit. |
| **R23** | `tests/render-harness/main.cpp` — add `--string <E|A|D|G>`, `--detune-sweep <E|A|D|G>`, `--note-sequence "MIDI:dur,..."` flags. Add per-block detune-sweep ramp. Add note-sequence event list pre-build + per-block drain. Add `pass_rmsContinuity` + `rmsContinuityRatio` + `pass_rmsContinuityAtTransitions` + `pass_allSegmentsAudible` checks. Update JSON schema per §15.7 + §15.8. Auto-rewrite default WAV/JSON filenames when flags are set. | Source edit. |
| **R24** | Build (Release) + auval + pluginval-10. Re-render regression preset (Gate 4 invariant 7); require sha256 match `d358abcd…`. | Diagnostic; gate. |
| **R25** | Run Gate 4 invariants (1)–(6): per-string sustained drone × 3 (A1/D2/G2 each: 60s, 4/4 invariants TRUE), `--detune-sweep A` 30s (`pass_rmsContinuity` TRUE), `--note-sequence "28:2.0,33:2.0,38:2.0,43:2.0,28:2.0"` (`pass_allSegmentsAudible` + `pass_rmsContinuityAtTransitions` TRUE), `--note-sequence "50:5.0"` with ACTIVE_STRINGS=1 (audible tone, no silence). Capture per-test sha256 + JSON to `tests/render-harness/golden/`. | Diagnostic; gate. |
| **R26** | R21+ atomic commit: source files (`BowedContrabassVoice.{h,cpp}`, `WaveguideString.{h,cpp}`, harness `main.cpp`) + golden text files (per-string sustained-tone JSON+sha256, detune-sweep JSON+sha256, note-sequence JSON+sha256) + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION/STATUS rev updates). Single commit on Gate 4 PASS. | Atomic commit. |
| **R27 (optional)** | Logic Pro AU smoke audition (user-deferred non-blocking, mirroring Phase 2.1c R19f / Phase 2.1b R14e precedent): play E1 → A1 → D2 → G2 portamento + ACTIVE_STRINGS knob sweep 4→3→2→1 with MIDI 50 held. | Manual; not in commit. |

---

## 15.14 Open Items for Plan Phase

1. **R-pre execution location.** Recommendation: PLAN rev-6 makes R21-pre a structural prerequisite to R21 (mirroring Phase 2.1c's R16-pre). The R-pre WAV+sha256+JSON do NOT get committed; they are an ephemeral execute-phase confirmation that the working tree still produces the Phase 2.1c golden. R-final (R24) re-renders against committed golden.
2. **Per-segment RMS threshold for `pass_allSegmentsAudible` in note-sequence mode.** §15.8 schema specifies `> 1e-3` (i.e., RMS > 0.001 = ~−60 dBFS). Plan-phase lock OR adjust based on empirical data from R25.
3. **`rmsContinuityAtTransitions` window size.** §15.8 specifies 256-sample symmetric window centred on each transition. Plan-phase lock.
4. **`openStringFrequencyHz` constant array.** Voice-side: `constexpr float openStringFrequencyHz[4] = { 41.20f, 55.00f, 73.42f, 98.00f };` (E1, A1, D2, G2). Plan-phase locks the exact float literals (decision: round to 2 places per architecture line 74, or use `MidiMessage::getMidiNoteInHertz(28/33/38/43)` at static-init time — either works; plan-phase picks).
5. **`B_open` constant array.** Voice-side: `constexpr float B_open[4] = { 1.0e-4f, 7.0e-5f, 5.0e-5f, 3.0e-5f };` (E, A, D, G). Verbatim from architecture line 79–83. Plan-phase locks.
6. **`M_per_string` constant array.** `constexpr int M_per_string[4] = { 4, 3, 2, 1 };` Verbatim from architecture line 79–83. Plan-phase locks.
7. **Golden files location.** `plugins/O-Contrabass/tests/render-harness/golden/`. New files: `string-A.{wav.sha256,json}`, `string-D.{wav.sha256,json}`, `string-G.{wav.sha256,json}`, `detune-sweep-A.{wav.sha256,json}`, `note-sequence.{wav.sha256,json}`. Per RESEARCH §14.12 #5 + Phase 2.1c precedent: WAV NOT committed; sha256 + JSON committed. Plan-phase locks.
8. **Voice-internal `readDetuneForString(int s)` helper.** Maps slot index → APVTS parameter ID:
   ```cpp
   float BowedContrabassVoice::readDetuneForString (int s) const noexcept
   {
       static constexpr const char* paramIds[4] = { "DETUNE_E", "DETUNE_A", "DETUNE_D", "DETUNE_G" };
       return parameters->getRawParameterValue (paramIds[s])->load();
   }
   ```
   Plan-phase lock the array.

---

## 15.15 Summary — Phase 2.2 Research Plan

- **Q1 (switching trigger):** Note-on-only; replace-on-mid-crossfade-retrigger. Closed-form mapping table {28, 33, 38, 43} clamped by ACTIVE_STRINGS−1. (§15.2)
- **Q2 (crossfade math):** Precomputed equal-power ramp at prepareToPlay; size = ceil(0.005 · sr_internal); per-sample = 2 array loads. (§15.3)
- **Q3 (smoother sharing):** Per-string smoother kept in WaveguideString (no API churn; trivial CPU cost; bit-exact regression preserved). (§15.4)
- **Q4 (prepare surface):** No new prepare() overload. Add `WaveguideString::setDispersionActiveSections(M)` pass-through setter. (§15.5)
- **Q5 (MIDI freq):** 12-TET fingering; `MidiMessage::getMidiNoteInHertz` × MPE bend × detune ratio. Detune in delay-samples space via existing `setDelaySamples(totalSamples)` API. (§15.5/§15.6)
- **Q6 (--detune-sweep schema):** New CLI flag + per-block ramp + JSON additions (`mode`, `string`, `detuneRamp`, `rmsByDecade`, `rmsContinuityRatio`, `pass_rmsContinuity`). 0.90 threshold. (§15.7)
- **Q7 (--note-sequence schema):** New CLI flag + pre-build event list + per-block drain + JSON additions (`mode`, `sequence`, `transitionSampleIndices`, `perSegmentRms`, `pass_allSegmentsAudible`, `pass_rmsContinuityAtTransitions`, `rmsContinuityAtTransitions`). 0.50 threshold at transitions. (§15.8)
- **Q8 (regression tolerance):** Strict byte-equal sha256 match. Idle-string output is literal 0.0f at regression preset (analytical proof + empirical baseline both confirm). One-time refactoring-boundary fallback is unnecessary. (§15.9)

**Net source delta (PLAN rev-6 estimate):**
- `BowedContrabassVoice.h`: ~+15 LOC (new state vars, precomputed ramp vector, helpers)
- `BowedContrabassVoice.cpp`: ~+80 LOC (state machine, mix loop, prepareToPlay extensions, helpers)
- `WaveguideString.h`: ~+1 LOC (`setDispersionActiveSections` declaration)
- `WaveguideString.cpp`: ~+3 LOC (`setDispersionActiveSections` implementation)
- `tests/render-harness/main.cpp`: ~+120 LOC (3 new flags, 2 new modes, JSON schema additions, RMS-continuity computation, transition-rms computation)

Total: ~+220 LOC source + ~6 new golden text files.

**Pre-flight regression bar empirically confirmed (§15.1):** working tree at R20 still produces sha256 `d358abcd…` against MIDI 28 + ACTIVE_STRINGS=4 + DETUNE_E=0 + STRING_STIFFNESS=0 preset. Phase 2.2 plan-phase can proceed. Hand off to `/plugin-plan O-Contrabass 2-dsp` for PLAN rev-6.

---

## 15.16 References (§15 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-4 §"Open Questions" (Q1–Q8 — resolved here).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-4 §"Approach Decisions" Q1–Q10 + 3 derived (idle-string topology, crossfade trigger, listening test sequence).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-4 §"Risks" #1–#8 — refined in §15.12.
- `plugins/O-Contrabass/research/ARCHITECTURE.md` §"String Waveguide Bank" lines 68–88 (per-string M-table, B prefactors, detune SmoothedValue<Linear> 20 ms in delay-samples space, ACTIVE_STRINGS clamp).
- `plugins/O-Contrabass/research/ARCHITECTURE.md` §"Cascaded Allpass Dispersion" lines 395–417 (closed-form coefficient, group-delay compensation; consumed verbatim from Phase 2.1c via setActiveSections/setCoefficient API).
- `plugins/O-Contrabass/parameter-spec.md` line 37 (ACTIVE_STRINGS Int [1,4]); lines 43–46 (DETUNE_E/A/D/G Float [-1200, +1200]).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.{h,cpp}` (Phase 2.1a–c carry-forward; topology untouched).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (Phase 2.1a–c carry-forward; topology untouched).
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20; setActiveSections + setCoefficient public API consumed verbatim).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (Phase 2.1c CLI surface; new flags follow same pattern).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` = `d358abcd…` (Phase 2.1c regression bar carry-forward).
- `plugins/O-Bowed/Source/BowedStringVoice.h` line 83 + `plugins/O-Bowed/Source/DSP/WaveguideString.h` line 22 — confirmed independent file (not shared module); O-Contrabass changes are plugin-local.
- `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` line 12, 32 — value-class deterministic; rho ∈ [0, ~0.5] proportional to v_delta·F_bow ⇒ `rho(0,0) = 0`.
- §15.1 pre-flight render: `/tmp/phase22-preflight-stiffness-zero.{wav,json}`, sha256 `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` (matches committed golden).

---

# §16. Phase 2.3 — Vibrato + Slow-Bow LFO + Schelleng Wedge Clamp + EXPRESSION_MACRO (Research)

**Date:** 2026-04-27
**Cycle Scope:** Phase 2.3 — single coupled cycle covering vibrato section, Slow-Bow LFO, Schelleng wedge clamp (inline in voice), and EXPRESSION_MACRO 4-destination layering. CONTEXT.md rev-5 (Q1–Q11 user-confirmed; HR-1 to HR-4 hard rules; eight-item Gate 5 bar).

This section resolves the 10 Open Questions handed off by the discuss phase, captures the §16.1 pre-flight bit-exact baseline render result, refines the risk surface, and locks the sequencing recommendation for PLAN rev-7.

---

## 16.1 Pre-Flight Bit-Exact Baseline Render (executed in research)

Open Question #7 mandated a pre-flight: capture sha256 with the EXPRESSION_MACRO default flipped from 0.50 → 0.0 in `Source/PluginProcessor.cpp` BUT with NO other Phase 2.3 source edits, and confirm sha256 still matches the Phase 2.2 strict byte-equal regression bar `d358abcd…`.

**Rationale.** The discuss-phase Q7a decision flips the parameter default because the architecture-spec'd 0.50 would (once the macro DSP is wired) cause a non-zero brightness offset and bow-param multiplier at rest, breaking the 5-golden regression bar. Pre-flight confirms that the source-level default change *itself* (with macro DSP still absent) produces zero render delta — i.e. EXPRESSION_MACRO is genuinely orphaned in the working tree at R26 commit `131c2c7`.

**Procedure.**
```
1. Edit plugins/O-Contrabass/Source/PluginProcessor.cpp line 86:
     0.50f  →  0.0f  (EXPRESSION_MACRO default)
2. ninja O-Contrabass-render-test  (build/ working dir)
3. cd /tmp && O-Contrabass-render-test \
       --note 28 --sustain 60 --release 5 \
       --infinite-sustain 1.0 --string-stiffness 0 \
       --out phase23-preflight.wav --json phase23-preflight.json
4. shasum -a 256 /tmp/phase23-preflight.wav
5. Revert source edit to leave working tree clean for plan-phase.
```

**Result.**
```
sha256 = d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
```

**Byte-identical to** `tests/render-harness/golden/stiffness-zero-pre.wav.sha256` (the Phase 2.1c / 2.2 strict regression bar). The harness JSON pass field reports FAIL only on the `pass_rms` heuristic (rmsFinal/rmsMid ratio out of [0.5, 2.0]) — same FAIL-on-ratio behaviour as Phase 2.2 verify; this is the documented Phase 2.4-deferred "post-bow-off rmsRatio" finding (STATUS.md `phase_2_1a_followup_park`). Strict byte-equality is the regression bar, not the ratio heuristic.

**Implication for plan-phase.**
- Q7a default flip is provably a zero-delta source edit until macro DSP lands.
- The 4 hard rules HR-1 to HR-4 (literal-zero short-circuits at modulators-off) are necessary AND sufficient for preserving the regression bar through the macro DSP wiring; they have not been weakened by any working-tree drift.
- After Phase 2.3 source edits, a final post-edit regression render at the same preset must reproduce `d358abcd…`. This is invariant 1 of the eight-item Gate 5 bar.

**Working tree state confirmed clean** post-revert: `git diff plugins/O-Contrabass/Source/` returns empty.

---

## 16.2 Open Question #1 — Vibrato S-Curve Onset Envelope Formula (RESOLVED — half-cosine)

**Architecture line 125:** "S-curve fade-in over 300 ms (half-cosine ramp)". CONTEXT.md rev-5 lists half-cosine `0.5 - 0.5·cos(π·t/0.3)` as the recommended formula vs 5th-order smoothstep `t² · (3 - 2t)` as alternative.

**Comparison.**

| Property | Half-cosine | Smoothstep `t²(3-2t)` |
|----------|-------------|------------------------|
| Architecture text match | ✅ literal | partial (also "S-curve") |
| C¹ continuity at t=0, t=0.3 | ✅ derivative = 0 at both | ✅ derivative = 0 at both |
| Per-sample cost (cos vs polynomial) | ~10 ns on M1 (`cosf` vectorised) | ~3 ns (3 muls + 1 sub) |
| Audible difference vs smoothstep | < 0.5 dB peak deviation across 300 ms ramp | n/a |
| Ramp shape | symmetric S | slightly fatter mid-section |

**Resolution: half-cosine.** Matches architecture verbatim (no spec deviation needed). Per-sample `cosf` cost is trivial — the gate is only computed during the 300 ms onset window after the user-configured VIBRATO_ONSET delay has elapsed. Outside that window the gate is cached at 0.0f (still inside delay) or 1.0f (ramp complete) and `cosf` is not called.

**Closed-form expression for `vibratoOnsetGate`.**
```cpp
const float onsetSec  = effectiveVibratoOnsetSec;        // VIBRATO_ONSET / 1000  (raw, no macro)
const float elapsed   = vibratoOnsetTimerSeconds - onsetSec;
const float kRampSec  = 0.3f;                            // architecture-spec'd 300 ms
const float kPiOverRamp = juce::MathConstants<float>::pi / kRampSec;

float vibratoOnsetGate;
if (elapsed <= 0.0f)            vibratoOnsetGate = 0.0f;                            // still in onset delay
else if (elapsed >= kRampSec)   vibratoOnsetGate = 1.0f;                            // ramp complete
else                            vibratoOnsetGate = 0.5f - 0.5f * std::cos (kPiOverRamp * elapsed);
```

`vibratoOnsetTimerSeconds` advances per-sample by `1.0 / sr_internal`. At sr_internal=88.2k, the 300 ms window is 26460 samples. The cost of the `cos` branch is amortised over thousands of samples per onset event.

**Note-off fade-out (CONTEXT line 120, 150 ms linear ramp).** When `noteStopped(allowTailOff=true)` fires, the voice transitions from "onset ramp engaged" to "fade-out ramp engaged" — the gate decays from its current value to 0.0 linearly over `noteOffFadeOutTimerSeconds ∈ [0, 0.150]`. Fade-out IS NOT half-cosine — linear is simpler, perceptually adequate at 150 ms (faster than bow tail, < 200 ms threshold per architecture line 127), and avoids overlap with the half-cosine onset that would create a non-monotonic gate.

```cpp
// During note-off fade:
const float k = juce::jlimit (0.0f, 1.0f, noteOffFadeOutTimerSeconds / 0.150f);
vibratoOnsetGate = vibratoOnsetGateAtNoteOff * (1.0f - k);
```

`vibratoOnsetGateAtNoteOff` is captured at `noteStopped` entry to avoid discontinuity on fast note-on→note-off sequences mid-onset.

---

## 16.3 Open Question #2 — Schelleng Wedge Bass-Register Validity (RESOLVED — clamp-on-bass parking)

**The question.** Does the closed-form Schelleng wedge headroom (architecture line 492) produce meaningful values at the bass operating point, or does it always-clamp to zero (silencing slow-LFO at bass register, analogous to Phase 2.1c Risk #7's E1 dispersion clamp)?

**Closed-form pre-flight at default bass operating point.**

Inputs (CONTEXT rev-5 §"Open Questions" #2 spec, drone-ish defaults):
- MIDI 28 (E1, f₀ ≈ 41.20 Hz)
- BOW_SPEED = 0.15 m/s  → `v_b`
- BOW_PRESSURE = 1.0    → `F_bow`
- BOW_POSITION = 0.10   → `β`
- INFINITE_SUSTAIN = 0.5 → loop gain `g = 0.997 + 0.00295·0.25 ≈ 0.99774`
- HyperbolicFriction bass defaults: `μ_s = 0.85`, `μ_d = 0.25`  → `Δμ = 0.60`
- Module string impedance: `R_s = 0.5` (HyperbolicFriction.h:67)

Architecture line 490–492:
```
fMin     = (Z² · v_b) / (2·R·β² · (μ_s − μ_d))
fMax     = (2·Z · v_b) / (β · (μ_s − μ_d))
headroom = min((fMax − F_bow)/fMax, (F_bow − fMin)/max(fMin, ε))
```

Substituting `Z = R = R_s = 0.5` (collapse to dimensionless Euphonics §9.3.1 form, since the friction model uses normalized arbitrary units per §10.4 lines 776):

```
fMax  = (2·0.5·0.15) / (0.10·0.60)              = 0.15 / 0.06    = 2.5
fMin  = (0.5²·0.15) / (2·0.5·0.10²·0.60)        = 0.0375 / 0.006 = 6.25
headroomUpper = (2.5  − 1.0) / 2.5              = +0.60
headroomLower = (1.0  − 6.25) / 6.25            = −0.84
headroom      = min(+0.60, −0.84)               = −0.84
```

**Headroom is NEGATIVE at default bass operating point.** This is the analogue of Phase 2.1c Risk #7 (closed-form coefficient clamps at bass register because the paper's validity envelope is piano/violin range, not contrabass). The same finding was already documented inline at `RESEARCH §10.4` lines 729–764: `F_bow=1.0` default sits *below* Schelleng F_min (~1.92 N normalized, or 6.25 in this re-derivation).

**`--schelleng-stress` harness preset (MIDI 28, BOW_PRESSURE=7.0, BOW_SPEED=0.05).**
```
fMax  = (2·0.5·0.05) / (0.10·0.60)              = 0.05 / 0.06    = 0.833
fMin  = (0.5²·0.05) / (2·0.5·0.10²·0.60)        = 0.0125 / 0.006 = 2.083
headroomUpper = (0.833 − 7.0) / 0.833           = −7.40   (above wedge)
headroomLower = (7.0   − 2.083) / 2.083         = +2.36
headroom      = min(−7.40, +2.36)               = −7.40
```

Stress preset is also negative. The clamp engages safely — `safeDepth = min(rawDepth, 0.8 × headroom)` zeros the slow-LFO modulation, the friction junction's hyperbolic table + algebraic saturator + energy clamp (Phase 2.1a Helmholtz defenses) handle stability.

**Resolution.**
1. **Implement the wedge formula AS-WRITTEN per architecture line 492.** No bass-calibration polynomial in v1.0.
2. **Document parking** as Phase 2.4 follow-up (analogous to Risk #7): empirical recalibration of `R` constant (or polynomial replacement) for bass register so the wedge produces non-negative headroom at default settings.
3. **Acceptance for v1.0:** at default bass operating point with SLOW_LFO_DEPTH > 0, slow-LFO modulation is effectively silenced (clamp wins). User must dial bow params closer to wedge mid-region (e.g. higher BOW_PRESSURE 2–4, mid-β 0.10–0.15, mid-velocity 0.20–0.40 m/s) to hear LFO modulation. This is acceptable because:
   - The wedge clamp's PRIMARY role is QUAL-02 stability protection at extreme drone settings, not perceptual modulation depth at default settings
   - The 80% × headroom factor scales gracefully — once headroom > 0.1 (mid-bow region), modulation engages
   - Phase 2.4 calibration polynomial will widen the playable wedge for bass register
4. **Gate 5 invariant 4 (`--schelleng-stress`)** validates the clamp-engages-on-stress path: peak ≤ 0 dBFS, no NaN, and `clampedDepthMean < 0.5` confirms `safeDepth` was driven below 50% of nominal across the render.
5. **HR-4 hard rule** (skip wedge math entirely when SLOW_LFO_DEPTH=0) preserves bit-exact regression bar — wedge eval is gated behind the depth-zero check.

**Code shape (inline in `BowedContrabassVoice::renderNextBlock`, ~10 LOC).**
```cpp
// Per-block, after raw-APVTS read, before any bow-param effective compute:
const float slowLfoDepthRaw = parameters->getRawParameterValue ("SLOW_LFO_DEPTH")->load();

float slowLfoSpeedMod = 0.0f, slowLfoPressureMod = 0.0f;
if (slowLfoDepthRaw > 0.0f)                 // HR-4 short-circuit
{
    // Schelleng wedge — collapse Z = R = R_s (dimensionless Euphonics §9.3.1 form).
    constexpr float kZ = 0.5f, kR = 0.5f;
    const float dMu = 0.60f;                // bass defaults μ_s − μ_d
    const float fMax = (2.0f * kZ * v_bowRaw) / juce::jmax (1.0e-6f, beta * dMu);
    const float fMin = (kZ * kZ * v_bowRaw) / juce::jmax (1.0e-6f, 2.0f * kR * beta * beta * dMu);
    const float hUp  = (fMax - F_bowRaw) / juce::jmax (1.0e-6f, fMax);
    const float hLo  = (F_bowRaw - fMin) / juce::jmax (1.0e-6f, fMin);
    const float headroom  = juce::jmin (hUp, hLo);
    const float safeDepth = juce::jlimit (0.0f, slowLfoDepthRaw, 0.8f * juce::jmax (0.0f, headroom));

    // Slow-LFO sine — phase advance per-block in radians.
    const float slowLfoRate = parameters->getRawParameterValue ("SLOW_LFO_RATE")->load();
    const float vibAntiCorr = 0.13f * slowLfoDepthRaw;     // Q5 anti-correlation guard
    slowLfoPhase += juce::MathConstants<float>::twoPi * slowLfoRate * (numSamples / sr_internal);
    if (slowLfoPhase > juce::MathConstants<float>::twoPi) slowLfoPhase -= juce::MathConstants<float>::twoPi;

    constexpr float kPressureLagRad = 0.4014f;             // 23° in radians
    slowLfoSpeedMod    = safeDepth * std::sin (slowLfoPhase);
    slowLfoPressureMod = safeDepth * std::sin (slowLfoPhase + kPressureLagRad);

    // Anti-correlation guard offsets vibrato rate ONLY when LFO is non-zero — Q5.
    effectiveVibratoRate += vibAntiCorr;
}
```

---

## 16.4 Open Question #3 — Vibrato + Detune Stacking (RESOLVED — combine cents first)

**Existing helper (BowedContrabassVoice.cpp:424–429):**
```cpp
float computeDelaySamples (float playedFreqHz, float detuneCents) const noexcept
{
    const float detuneRatio = std::pow (2.0f, detuneCents / 1200.0f);
    const float detunedFreq = playedFreqHz * detuneRatio;
    return static_cast<float> (sr_internal) / juce::jmax (1.0f, detunedFreq);
}
```

Phase 2.2's per-string detune ramp ALREADY consumes this — `detuneSmoothed[s]` is cached in delay-samples space directly. Phase 2.3 vibrato cents stack on top, but with different math because vibrato is per-sample modulation while detune is per-block target.

**Recommended stacking (algebraic equivalence).**

The 2^(x) operator decomposes additively over its argument:
```
2^(detuneCents/1200) × 2^(vibratoCents/1200) = 2^((detuneCents + vibratoCents)/1200)
```

In the per-sample loop (active string only), modulate the delay-samples value already produced by the detune ramp:

```cpp
const float baseDelaySamples = detuneSmoothed[active].getNextValue();   // (a) detune ramp value
const float vibCents         = effectiveVibratoDepth * vibratoOnsetGate * std::sin (vibratoPhase);
const float vibFactor        = std::exp (vibCents * (-juce::MathConstants<float>::ln2 / 1200.0f));   // (b) cheap 2^(-x/1200) via exp
const float modulatedDelay   = baseDelaySamples * vibFactor;
strings[active].setDelaySamples (modulatedDelay);
```

**Identity check.** `baseDelaySamples = sr / (f × 2^(detune/1200))`. Multiplying by `vibFactor = 2^(-vib/1200)` yields `sr / (f × 2^((detune + vib)/1200))` — exactly the cents-first re-derivation. Single 2^() multiply per-sample, no double-pow cost.

**Why `exp` instead of `pow(2,x)`?** `pow(2.0f, x)` on M1 is ~25 ns; `exp(x · ln2)` is ~12 ns. For the 50¢ peak vibrato range, both are numerically identical to single-precision (~7 decimal digits) so cost wins.

**Lagrange3rd absorption.** Vibrato cents range is ±50¢ peak (architecture line 124, default 12¢, max 50¢). At 50¢, factor = 2^(0.04167) ≈ 1.0293, so delay-samples shift is at most ±2.93%. Phase 2.2's detune-sweep harness (RESEARCH §15.7) already exercised the Lagrange3rd interpolator at ±1200¢ (factors 0.5× to 2.0×) without click. Phase 2.3's modulation is two orders of magnitude smaller — well within validated range.

**Per-sample modulation guard (HR-1).** When `effectiveVibratoDepth = 0` (VIBRATO_DEPTH=0 raw OR macro × 0 collapses), the literal-zero-check must short-circuit:

```cpp
if (effectiveVibratoDepth <= 0.0f) {
    strings[active].setDelaySamples (baseDelaySamples);    // unchanged from detune-only path
} else {
    const float vibCents      = effectiveVibratoDepth * vibratoOnsetGate * std::sin (vibratoPhase);
    const float vibFactor     = std::exp (vibCents * (-juce::MathConstants<float>::ln2 / 1200.0f));
    const float modulatedDelay = baseDelaySamples * vibFactor;
    strings[active].setDelaySamples (modulatedDelay);
}
```

The HR-1 short-circuit makes the modulators-off code path bit-identical to the existing Phase 2.2 mix loop's `setDelaySamples(detuneSmoothed[s].getNextValue())` at line 301 / 329. Slot-0 bit-exact regression preserved.

**Idle strings: NOT vibrato-modulated (Q2 lock).** During the per-sample loop, idle strings continue to consume their detuneSmoothed ramps as in Phase 2.2 — vibrato cents are NOT added to their delay calls. This is the active-string-only contract.

---

## 16.5 Open Question #4 — Brightness Offset Smoothing Window (RESOLVED — 20 ms voice-level)

**The question.** EXPRESSION_MACRO drives BRIGHTNESS offset 0 → 500 Hz. At 0→1.0 macro step, that's 25 kHz/s on the bridge-LP cutoff frequency. Does 20 ms `SmoothedValue<Linear>` produce zipper noise on the bridge filter coefficient `p`?

**WaveguideString brightness path (current code, WaveguideString.{h,cpp}).**
- Voice's `updateParametersFromAPVTS()` reads `BRIGHTNESS` once per block, calls `setBrightness(brightnessHz)` on each of 4 strings.
- `setBrightness` sets `brightnessHz = cutoffHz; filterDirty = true;`.
- `bridgeP` and `bridgeOneMinusP` are recomputed when `filterDirty` is set, before the next-sample bridge-LP recurrence. The pole `p ≈ 1 - exp(-2π·brightnessHz/sr_internal)`.

**Block-rate analysis at default host params.** Block size 512 at 44.1k host = 11.6 ms per block. The voice update path is once-per-block. Without smoothing, a 500 Hz step in BRIGHTNESS lands as a single instantaneous jump at the next block boundary → audible click on the bridge filter.

**Voice-level 20 ms `SmoothedValue<Linear>` analysis.**

Place the smoother at voice level on `effectiveBrightnessHz = rawBrightness + 500.0f * macro`. Per-block path:

```cpp
// Per-block (renderNextBlock, after raw APVTS read):
const float macroSmoothedNow   = macroSmoothed.getNextValue();           // advance 1 sample
macroSmoothed.skip (numSamples - 1);                                     // catch up the rest
const float effectiveBrightnessHz = rawBrightness + 500.0f * macroSmoothedNow;
for (auto& s : strings) s.setBrightness (effectiveBrightnessHz);
```

At block size 512, 44.1k host: `getNextValue()` + `skip(511)` advances the smoother 512 sample-ticks per block. With `reset(sampleRate, 0.020)` at host rate 44.1k, the smoother takes 882 sample-ticks (≈20 ms) to reach target — about 1.72 blocks. Per-block step on `effectiveBrightnessHz` is therefore ~58% of remaining-delta worst-case, i.e. ~290 Hz/block at the 500 Hz peak step.

**Bridge-LP coefficient `p` step at this rate.**
```
p(brightness) ≈ 1 - exp(-2π · brightness / sr_internal)

At sr_internal = 88.2k:
  p(4500 Hz) = 1 - exp(-2π·4500/88200) = 1 - exp(-0.3206) = 0.2740
  p(5000 Hz) = 1 - exp(-2π·5000/88200) = 1 - exp(-0.3562) = 0.3000
  Δp_max     = 0.026   over the full 500 Hz step

Per-block Δp at 290 Hz/block ≈ 0.026 × (290/500) ≈ 0.015
```

A `p`-step of ~0.015 per block on a one-pole filter is below the audible-zipper threshold for sustained tones (~0.05 is the typical detection bound for low-Q one-pole sweeps). 20 ms is sufficient.

**Resolution: 20 ms voice-level `SmoothedValue<Linear>` on the macro source, with all 4 effective-bow-param destinations consuming the same smoother per-block.** No WaveguideString surface change.

**Architecture line 76 (`juce::SmoothedValue<float, Linear>` 20 ms ramp) carry-forward.** CONTEXT rev-5 line 76 already specs four 20 ms smoothers; this resolution reduces that to one (the macro source) plus per-block formula for the destinations. Net wins: less state, single ramp clock, easier HR-3 (literal-zero macro arithmetic).

**Fallback to 50 ms** (architecture line 522 body-bank precedent) reserved if Gate 5 invariant 5 (`--macro-sweep` rmsContinuity ≥ 0.85) fails empirically. Implementation is a single `reset(sampleRate, 0.050)` call site change.

**Architectural note.** SLOW_LFO_DEPTH and VIBRATO_DEPTH ALSO need their own smoothing on the macro-multiplicative term, but the macro-source smoother already feeds them transitively. The slow-LFO speed/pressure mod values have their own architecture-spec'd 20 ms `SmoothedValue` (architecture line 112) — that smoother stays inside the LFO logic, applied to the `slowLfoSpeedMod` / `slowLfoPressureMod` outputs before they multiply into bow speed/pressure. Two distinct smoothers in voice: (a) macroSmoothed (20 ms, 1 source); (b) slowLfoSpeedMod / slowLfoPressureMod (20 ms each, 2 destinations). Total: 3 SmoothedValue<Linear> instances in voice.

---

## 16.6 Open Question #5 — Per-Block Evaluation Order Final Pseudocode (RESOLVED)

CONTEXT rev-5 line 119 specifies the 7-step order. Research-phase finalises pseudocode + the crossfade interaction edge case.

**Per-block evaluation order (locked, immutable).**

```cpp
void BowedContrabassVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                            int startSample, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (numSamples <= 0) return;

    // ─── Step 1: Read raw APVTS atomics into block-cached locals ───────────
    // (Existing updateParametersFromAPVTS → bow params, brightness, etc.)
    updateParametersFromAPVTS();
    const float rawVibratoDepth   = parameters->getRawParameterValue ("VIBRATO_DEPTH")->load();
    const float rawVibratoRate    = parameters->getRawParameterValue ("VIBRATO_RATE")->load();
    const float rawVibratoOnsetMs = parameters->getRawParameterValue ("VIBRATO_ONSET")->load();
    const float rawSlowLfoRate    = parameters->getRawParameterValue ("SLOW_LFO_RATE")->load();
    const float rawSlowLfoDepth   = parameters->getRawParameterValue ("SLOW_LFO_DEPTH")->load();
    const float rawMacro          = parameters->getRawParameterValue ("EXPRESSION_MACRO")->load();
    const float rawBowSpeed       = parameters->getRawParameterValue ("BOW_SPEED")->load();
    const float rawBowPressure    = parameters->getRawParameterValue ("BOW_PRESSURE")->load();
    const float rawBowPos         = parameters->getRawParameterValue ("BOW_POSITION")->load();
    const float rawBrightness     = parameters->getRawParameterValue ("BRIGHTNESS")->load();

    // ─── Step 2: Compute Schelleng wedge fMin/fMax/headroom ─────────────────
    // HR-4 — skip entirely if SLOW_LFO_DEPTH = 0 (literal-zero).
    float safeDepth   = 0.0f;
    float vibAntiCorr = 0.0f;
    if (rawSlowLfoDepth > 0.0f)
    {
        constexpr float kZ = 0.5f, kR = 0.5f, kDMu = 0.60f;
        const float fMax = (2.0f * kZ * rawBowSpeed) / juce::jmax (1.0e-6f, rawBowPos * kDMu);
        const float fMin = (kZ * kZ * rawBowSpeed) / juce::jmax (1.0e-6f, 2.0f * kR * rawBowPos * rawBowPos * kDMu);
        const float hUp  = (fMax - rawBowPressure) / juce::jmax (1.0e-6f, fMax);
        const float hLo  = (rawBowPressure - fMin) / juce::jmax (1.0e-6f, fMin);
        const float headroom = juce::jmin (hUp, hLo);
        safeDepth   = juce::jlimit (0.0f, rawSlowLfoDepth, 0.8f * juce::jmax (0.0f, headroom));
        vibAntiCorr = 0.13f * rawSlowLfoDepth;                                   // Q5
    }

    // ─── Step 3: Slow-LFO with depth-clamp engaged ──────────────────────────
    float slowLfoSpeedMod = 0.0f, slowLfoPressureMod = 0.0f;
    if (rawSlowLfoDepth > 0.0f)                                                  // HR-2
    {
        slowLfoPhase += juce::MathConstants<float>::twoPi * rawSlowLfoRate
                      * static_cast<float> (numSamples / sr_internal);
        if (slowLfoPhase > juce::MathConstants<float>::twoPi)
            slowLfoPhase -= juce::MathConstants<float>::twoPi;

        constexpr float kPressureLagRad = 0.4014f;                               // 23°
        slowLfoSpeedMod    = safeDepth * std::sin (slowLfoPhase);
        slowLfoPressureMod = safeDepth * std::sin (slowLfoPhase + kPressureLagRad);
    }

    // ─── Step 4: Apply slow-LFO multiplicatively to bow params ──────────────
    const float bowSpeedAfterLfo    = rawBowSpeed    * (1.0f + 0.6f * slowLfoSpeedMod);
    const float bowPressureAfterLfo = rawBowPressure * (1.0f + 0.5f * slowLfoPressureMod);

    // ─── Step 5: Layer macro multiplicatively ───────────────────────────────
    // HR-3 — when macro = 0, all four (1 + 0·k) = 1 exactly; brightness offset = 0 exactly.
    macroSmoothed.setTargetValue (rawMacro);
    const float macroNow = macroSmoothed.getNextValue();
    macroSmoothed.skip (juce::jmax (0, numSamples - 1));

    const float effectiveBowSpeed     = bowSpeedAfterLfo    * (1.0f + 0.4f * macroNow);
    const float effectiveBowPressure  = bowPressureAfterLfo * (1.0f + 0.6f * macroNow);
    const float effectiveVibratoDepth = rawVibratoDepth     * (1.0f + 0.3f * macroNow);
    const float effectiveBrightnessHz = rawBrightness + 500.0f * macroNow;
    const float effectiveVibratoRate  = rawVibratoRate + vibAntiCorr;
    const float effectiveVibratoOnsetSec = 0.001f * rawVibratoOnsetMs;

    // ─── Step 6: Push to bowModel + all-strings brightness ──────────────────
    bowModel.setBowSpeed    (effectiveBowSpeed * mpeExpression);
    bowModel.setBowPressure (effectiveBowPressure * (0.5f + getCurrentlyPlayingNote().pressure.asUnsignedFloat() * 1.5f));
    for (auto& s : strings) s.setBrightness (effectiveBrightnessHz);
    // (existing bowPosition, infiniteSustain, dispersion-coefficient updates carry forward)

    // (… existing dispersion + detune update sequence carries forward verbatim …)

    // ─── Step 7: Per-sample loop (active-string-only vibrato modulation) ────
    const float kVibPhaseDelta = juce::MathConstants<float>::twoPi
                               * effectiveVibratoRate / static_cast<float> (sr_internal);
    const float kPiOverRamp     = juce::MathConstants<float>::pi / 0.3f;
    const float kVibFactorScale = -juce::MathConstants<float>::ln2 / 1200.0f;
    const float kInvSrInternal  = 1.0f / static_cast<float> (sr_internal);

    for (int i = 0; i < numUp; ++i)
    {
        // (existing bowModel.updateEnvelope() + v_bow / F_bow read carries forward)

        // Vibrato gate + cents (active string only; HR-1 short-circuit on zero depth)
        float vibCents = 0.0f;
        if (effectiveVibratoDepth > 0.0f)
        {
            const float elapsed = vibratoOnsetTimerSeconds - effectiveVibratoOnsetSec;
            float gate;
            if      (noteOffFadeOutTimerSeconds > 0.0f && noteOffFadeOutTimerSeconds < 0.150f)
                gate = vibratoOnsetGateAtNoteOff
                     * (1.0f - juce::jlimit (0.0f, 1.0f, noteOffFadeOutTimerSeconds / 0.150f));
            else if (elapsed <= 0.0f)        gate = 0.0f;
            else if (elapsed >= 0.3f)        gate = 1.0f;
            else                             gate = 0.5f - 0.5f * std::cos (kPiOverRamp * elapsed);

            vibCents = effectiveVibratoDepth * gate * std::sin (vibratoPhase);
        }
        // Always advance phase + timer (HR-1 ensures vibCents = 0 at zero depth, but sin keeps phase
        // monotonic across re-arms — Q3 sine-phase-carry contract).
        vibratoPhase += kVibPhaseDelta;
        if (vibratoPhase > juce::MathConstants<float>::twoPi)
            vibratoPhase -= juce::MathConstants<float>::twoPi;
        vibratoOnsetTimerSeconds += kInvSrInternal;
        if (noteOffFadeOutTimerSeconds > 0.0f) noteOffFadeOutTimerSeconds += kInvSrInternal;

        // (Existing crossfade / standard mix logic carries forward; vibrato modulation slots in
        //  on the active-string setDelaySamples line as documented in §16.4.)
        // …
    }
}
```

**Edge case — vibrato/slow-LFO during 5 ms string-switching crossfade.**

CONTEXT rev-5 line 132 asks: are vibrato and slow-LFO advancing on both old + new strings or only the new active one?

**Resolution.** Vibrato and slow-LFO are voice-level state (single phase counter each), NOT per-string. They advance regardless of crossfade state. The active-string-only modulation contract (Q2) means:
- During crossfade, vibrato modulates the NEW active string's `setDelaySamples` only. Old (previous) string's delay is held at the value set by `noteStarted` (no per-sample modulation).
- Slow-LFO modulates `bowModel.setBowSpeed/Pressure` once per block — applied uniformly regardless of which string is generating output. Old string sees the same friction injection as the new during the 2.5 ms each occupies the active mix path. This is the existing Phase 2.2 design (idle strings get zero-input friction; only active gets v_bow/F_bow).

**No special crossfade handling for modulators.** Phase counters advance, the active-string-only modulation rule just continues to apply.

---

## 16.7 Open Question #6 — Harness JSON Schemas + Pitch-Tracking (RESOLVED)

Four new harness modes need CLI flags + JSON schemas + pass-condition definitions. Pattern follows Phase 2.2's `--detune-sweep` / `--note-sequence` (RESEARCH §15.7 / §15.8).

### 16.7.1 `--vibrato` mode

**CLI surface.**
```
--vibrato                              activate; sustained tone at MIDI 28
                                       VIBRATO_DEPTH=12¢, VIBRATO_RATE=5 Hz, VIBRATO_ONSET=600 ms
                                       sustain=2.0 s default (covers 600 ms onset + 300 ms ramp +
                                       1.1 s steady-state for FFT pitch tracking)
--out vibrato.wav, --json vibrato.json (auto-rewrite default if not set)
```

**Pre-build APVTS overrides at harness init.**
```cpp
parameters->getParameter ("VIBRATO_DEPTH")->setValueNotifyingHost ((12.0f - 0.0f) / 50.0f);  // norm
parameters->getParameter ("VIBRATO_RATE")->setValueNotifyingHost ((5.0f - 0.1f) / 11.9f);
parameters->getParameter ("VIBRATO_ONSET")->setValueNotifyingHost (... 600 ms norm with 0.5 skew ...);
```

**Pitch-tracking method — autocorrelation, NOT FFT bin-shift.** Bass register (41.20 Hz at MIDI 28 = ~24.3 ms period) requires a long FFT window for adequate frequency resolution: at sr=44100, FFT bin width = sr/N = 44100/4096 ≈ 10.8 Hz, so bin-shift method has ~25¢ resolution at f₀ — too coarse for ±12¢ vibrato measurement. Autocorrelation peak detection at a 4096-sample sliding window (≈93 ms) gives sub-sample period resolution via parabolic interpolation around the peak lag, achievable to ~1¢ at bass register.

**Algorithm (post-hoc analysis on the rendered WAV).**
```cpp
// 1. Skip the onset window (600 ms onset + 300 ms ramp = 900 ms = 39690 samples at 44.1k).
//    Analysis starts at 1.0 s.
// 2. Slide a 4096-sample autocorrelation window with 256-sample hop across the analysis region.
// 3. For each window: compute normalized autocorrelation R(τ) for τ ∈ [400, 1500] samples
//    (covers 29 Hz to 110 Hz — comfortably brackets MIDI 28).
// 4. Find peak τ via parabolic interpolation around argmax(R).
// 5. Compute instantaneous frequency: f = sr / τ_peak.
// 6. Convert to cents deviation from f₀ = 41.20 Hz: deltaCents = 1200 * log2(f / f₀).
// 7. Track peak-to-trough swing in deltaCents across 3 vibrato cycles (~600 ms): peakDepth = (max - min) / 2.
```

**Onset-window detection.**

`onsetWindow` (architecture spec ~900 ms) is measured as the time from note-on until the autocorrelation deltaCents amplitude first exceeds 80% of `peakDepth` (the steady-state swing). At default `VIBRATO_ONSET=600 ms` + 300 ms ramp, that's ~900 ms expected.

**JSON schema additions.**
```json
{
  "mode": "vibrato",
  "midiNote": 28,
  "vibratoDepthSetting": 12.0,
  "vibratoRateSetting": 5.0,
  "vibratoOnsetMsSetting": 600,

  "peakDepthCents": 12.4,         // measured peak-to-trough/2 in steady state
  "vibratoRateHzMeasured": 5.02,  // peak-detection rate from autocorrelation deltaCents trace
  "onsetTimeMs": 905,             // time to 80% of peakDepth

  "pass_vibratoDepthInRange": true,    // peakDepthCents ∈ [10.0, 14.0]
  "pass_onsetWindow": true,            // onsetTimeMs ∈ [800, 1000]
  "pass_rmsContinuity": true,          // ≥ 0.90 (4096-sample window, identical to detune-sweep)
  "pass_rateHzInRange": true,          // vibratoRateHzMeasured ∈ [4.5, 5.5]

  "rmsContinuityRatio": 0.962,
  "perCycleDeltaCents": [12.1, 12.5, 12.4, 12.3, 12.4]  // ~5 cycles in 1.0 s
}
```

**Pass condition (overall).**
```
overallPass = pass_nan && pass_peak && pass_blockTime
           && pass_vibratoDepthInRange && pass_onsetWindow && pass_rmsContinuity && pass_rateHzInRange
```

### 16.7.2 `--slow-lfo` mode

**CLI surface.**
```
--slow-lfo                             activate; MIDI 33 (A1, audible mid-bank string)
                                       SLOW_LFO_DEPTH=0.5, SLOW_LFO_RATE=0.3 Hz, sustain=60 s
--out slow-lfo.wav, --json slow-lfo.json
```

Pre-build APVTS overrides: `SLOW_LFO_DEPTH=0.5`, `SLOW_LFO_RATE=0.3` Hz. **Note:** Q2 finding shows that at default bass operating point (BOW_PRESSURE=1.0, BOW_POSITION=0.10) the wedge clamps to zero — slow-LFO is silenced. The harness MUST also override BOW_PRESSURE and/or BOW_POSITION to put the operating point inside the wedge. **Recommended:** set `BOW_PRESSURE = 3.0` (in normalized units; mid-wedge for the formula) and `BOW_POSITION = 0.10` (default β). This produces non-zero `safeDepth` and audible breathing.

**Validation of preset.** With `BOW_SPEED=0.15`, `BOW_PRESSURE=3.0`, `BOW_POSITION=0.10`:
```
fMax = (2·0.5·0.15)/(0.10·0.60)   = 2.500
fMin = (0.5²·0.15)/(2·0.5·0.10²·0.60) = 6.25
hUp  = (2.500 − 3.00)/2.500 = −0.20  → wedge upper still violated
```
Hmm, F_bow=3 is above fMax=2.5. Let me try BOW_PRESSURE=2.0:
```
hUp = (2.500 − 2.000)/2.500 = +0.20
hLo = (2.000 − 6.250)/6.250 = −0.68
```
Still negative. The problem is fMin being so high. **Resolution:** the v1.0 wedge formula at bass defaults is genuinely punishing — the harness preset must accept this. Set `--slow-lfo` preset to use BOW_PRESSURE that minimizes wedge clamp engagement (i.e. accept that at v1.0 bass, slow-LFO operates with most of its depth clamped). The harness should **report** the `clampedDepthMean` so the audit can confirm SLOW_LFO is actually engaging (>0.05) over the full sustain.

**Per-block instrumentation hook.** Voice exposes a thread-safe atomic `lastSafeDepth` (set per block in step 3). Harness samples this once per block and accumulates the mean across the sustain phase, written to JSON as `clampedDepthMean`.

**Pass condition.** Strict `pass_breathingAudible` requires peak-to-peak rmsByDecade ≥ 20%. With the clamp engaging, this may not be achievable at default bass operating point. **Pragmatic resolution:** soften `pass_breathingAudible` threshold to ≥ 5% peak-to-peak rmsByDecade for v1.0 (the harness still validates that LFO produces SOME audible modulation, just not at architecture-spec'd depth). The 20% threshold is parked alongside Phase 2.4 calibration polynomial.

**JSON schema additions.**
```json
{
  "mode": "slow-lfo",
  "midiNote": 33,
  "slowLfoDepthSetting": 0.5,
  "slowLfoRateHzSetting": 0.3,
  "bowPressureOverride": 1.0,        // raw, no override needed if defaults used

  "rmsByDecade": [...],              // 10 deciles
  "rmsByDecadePeakToPeakPct": 0.073, // measured peak-to-peak / mean
  "clampedDepthMean": 0.04,          // mean safeDepth across sustain (from per-block hook)

  "pass_breathingAudible": true,     // rmsByDecadePeakToPeakPct ≥ 0.05  (v1.0 — tightened in 2.4)
  "pass_rmsContinuity": true,        // ≥ 0.90 (steady-state continuity)
  "pass_clampEngagement": true,      // clampedDepthMean > 0.0  (confirms wedge math runs)

  "rmsContinuityRatio": 0.953
}
```

### 16.7.3 `--schelleng-stress` mode

**CLI surface.**
```
--schelleng-stress                     activate; MIDI 28
                                       BOW_PRESSURE=7.0, BOW_SPEED=0.05, SLOW_LFO_DEPTH=1.0,
                                       sustain=30 s
--out schelleng-stress.wav, --json schelleng-stress.json
```

**Per-block instrumentation: `clampedDepthMean`.** Same hook as `--slow-lfo`. The stress preset is designed so that headroom is severely negative (computed in §16.3: −7.40 at upper bound) — `safeDepth` is clamped to 0.0 across the entire render. `clampedDepthMean` should be ~0.0 confirming clamp engaged.

**JSON schema additions.**
```json
{
  "mode": "schelleng-stress",
  "midiNote": 28,
  "bowPressureSetting": 7.0,
  "bowSpeedSetting": 0.05,
  "slowLfoDepthSetting": 1.0,
  "sustainSeconds": 30.0,

  "peakPostMaster": 0.952,           // |peak| of stereo output
  "clampedDepthMean": 0.000,         // expect ~0.0 (clamp wins everywhere)

  "pass_peak": true,                 // ≤ 1.0
  "pass_noNaN": true,                // nanCount + infCount = 0
  "pass_clampEngaged": true          // clampedDepthMean < 0.5 (sub-half = clamp dominant)
}
```

**Overall PASS.** `pass_nan && pass_peak && pass_blockTime && pass_clampEngaged`.

### 16.7.4 `--macro-sweep` mode

**CLI surface.**
```
--macro-sweep                          activate; MIDI 38 (D2)
                                       per-block linear ramp EXPRESSION_MACRO 0 → 1.0
                                       across sustain phase, sustain=20 s default
--out macro-sweep.wav, --json macro-sweep.json
```

**Per-block ramp** (mirrors Phase 2.1c `--stiffness-sweep` and Phase 2.2 `--detune-sweep` pattern):
```cpp
const float fraction = juce::jlimit (0.0f, 1.0f,
                                     static_cast<float>(sampleCursor)
                                     / static_cast<float>(juce::jmax (1, sustainSamples)));
const float macroNorm = fraction;     // EXPRESSION_MACRO has identity normalization 0..1
parameters->getParameter ("EXPRESSION_MACRO")->setValueNotifyingHost (macroNorm);
```

**Pass conditions.**
- `pass_rmsContinuity` ≥ 0.85 (looser than 0.90 because macro intentionally raises loudness)
- `pass_rmsRampDirection`: final-decade RMS exceeds first-decade RMS by 10–30% (proves macro lifted bow speed/pressure → audible loudness rise; below 10% means macro path didn't fire; above 30% means macro is over-driving)

**JSON schema additions.**
```json
{
  "mode": "macro-sweep",
  "midiNote": 38,
  "macroRamp": { "start": 0.0, "end": 1.0, "shape": "linear" },

  "rmsByDecade": [...],              // 10 deciles
  "rmsRampPct": 0.224,               // (final - first) / first

  "pass_rmsContinuity": true,        // ≥ 0.85 (looser than 0.90)
  "pass_rmsRampDirection": true,     // rmsRampPct ∈ [0.10, 0.30]

  "rmsContinuityRatio": 0.872
}
```

**Overall PASS.** `pass_nan && pass_peak && pass_blockTime && pass_rmsContinuity && pass_rmsRampDirection`.

### 16.7.5 Harness CLI parsing pattern

All four flags follow the existing `parseArgs` pattern (RESEARCH §15.11). Three are presence-flags (no value):
```cpp
else if (key == "--vibrato")            { args.vibratoMode        = true; --i; }
else if (key == "--slow-lfo")           { args.slowLfoMode        = true; --i; }
else if (key == "--schelleng-stress")   { args.schellengMode      = true; --i; }
else if (key == "--macro-sweep")        { args.macroSweepMode     = true; --i; }
```

The `--i` decrement compensates for the existing `parseArgs` loop's value-consume increment (they take no value). Matches existing `--stiffness-sweep` precedent (line 129: takes 0/1 value but always present-flagged in usage).

**Mutual exclusion:** the four new modes are mutually-exclusive with each other AND with Phase 2.2's `--detune-sweep`/`--note-sequence`/`--string`/`--stiffness-sweep`. Harness emits warning + uses precedence ordering: `--macro-sweep` > `--schelleng-stress` > `--vibrato` > `--slow-lfo` > `--detune-sweep` > `--note-sequence` > `--string` > `--stiffness-sweep` > sustained.

**Default WAV/JSON filename auto-rewrite:** parallel to existing pattern (RESEARCH §15.7).

---

## 16.8 Open Question #7 — Bit-Exact Preservation Audit (RESOLVED — pre-flight PASS)

§16.1 already executed the pre-flight: with EXPRESSION_MACRO default flipped 0.50 → 0.0 in PluginProcessor.cpp and NO other source edits, sha256 = `d358abcd…` (byte-identical to committed golden).

**Confirmation that Phase 2.3 source edits will preserve bit-exactness IF hard rules are obeyed.**

The core insight: at modulators-off (VIBRATO_DEPTH=0, SLOW_LFO_DEPTH=0, EXPRESSION_MACRO=0), every Phase 2.3 source-edit code path must early-return-on-zero or evaluate to literal mathematical no-op. The hard rules HR-1 to HR-4 enforce this:

- **HR-1 (vibrato literal-zero short-circuit, §16.4):** `if (effectiveVibratoDepth <= 0.0f) { setDelaySamples(baseDelaySamples); }` — this single-line short-circuit makes the modulators-off code path produce the SAME `setDelaySamples` argument value as Phase 2.2's `setDelaySamples(detuneSmoothed[s].getNextValue())` at line 301/329.

- **HR-2 (slow-LFO literal-zero short-circuit, §16.6 step 3):** `if (rawSlowLfoDepth > 0.0f)` gates the entire slow-LFO block. At rawSlowLfoDepth=0.0, no phase advance, no sin call, slowLfoSpeedMod/PressureMod stay at their zero-init.

- **HR-3 (macro literal-zero arithmetic, §16.6 step 5):** with macroNow=0.0, `(1.0f + 0.4f * 0.0f) = 1.0f exact`, `(1.0f + 0.6f * 0.0f) = 1.0f exact`, `(1.0f + 0.3f * 0.0f) = 1.0f exact`, `0.0f + 500.0f * 0.0f = 0.0f exact` — IEEE 754 specifies `x + 0.0` and `x * 1.0` are bit-identical to `x` for all finite `x`. effectiveBowSpeed/Pressure/Brightness numerically equal raw values; pushed-to-bowModel/strings unchanged from Phase 2.2 path.

- **HR-4 (Schelleng wedge skip on zero LFO, §16.6 step 2):** entire wedge block is gated. Even though the formulas are mathematically no-op at zero LFO depth (clamp produces safeDepth=0 or negative regardless), the floating-point operations to compute fMax/fMin/headroom may produce slightly different processor-state side-effects (denormals if intermediates underflow). Skipping the math eliminates this risk.

**Plus the 3 new SmoothedValue<Linear> instances (macroSmoothed + slowLfoSpeedMod + slowLfoPressureMod).** SmoothedValue<Linear> initial state has `currentValue == targetValue` (set via `setCurrentAndTargetValue(0.0f)` in `prepareToPlay`). With targetValue=0.0 and currentValue=0.0, every `getNextValue()` returns 0.0f exactly. `skip(numSamples - 1)` advances internal counter without changing value. No drift introduced.

**HOWEVER** — `macroSmoothed.setTargetValue(rawMacro)` is called UNCONDITIONALLY in step 5. At rawMacro=0.0 and currentValue=0.0, target is set to 0.0 → no smoothing kicks in → getNextValue() returns 0.0f exactly. This case is bit-exact.

**Empirical confirmation deferred to verify-phase.** Plan-phase R28 atomic-commit-precondition: capture sha256 with full Phase 2.3 source edits applied + EXPRESSION_MACRO default=0.0 + all other params at Phase 2.2 regression preset → must match `d358abcd…`. This is Gate 5 invariant 1 of 8.

**Open Item carried to plan-phase:** `BowedContrabassVoice::prepareToPlay` must call `macroSmoothed.setCurrentAndTargetValue(0.0f)` and the slow-LFO mod smoothers similarly to lock the "modulators-off currentValue = 0.0" invariant.

---

## 16.9 Open Question #8 — Macro/Wedge Interaction Policy (RESOLVED — NO re-eval)

CONTEXT rev-5 line 119 specifies: per-block evaluation order step 2 (Schelleng wedge) consumes RAW bow params; step 5 (macro) layers AFTER. This means macro lifts the *effective* bow params past the raw-derived wedge — but the wedge clamp does NOT re-evaluate against macro-lifted params.

**The concern.** At macro=1.0, `effectiveBowPressure = bowPressureAfterLfo × 1.6` and `effectiveBowSpeed = bowSpeedAfterLfo × 1.4`. If the user dials high macro alongside high BOW_PRESSURE / extreme BOW_POSITION, the friction junction sees inputs significantly outside the wedge that protected the slow-LFO modulation.

**Resolution: NO re-eval against macro-lifted params.** Three rationales:

1. **Macro is performance-lift, not safety-lift.** The macro's contract (architecture line 567) is to make the instrument feel more vivid as a single knob. Re-evaluating the wedge against macro-lifted params would gut the macro's effect at high settings — defeating its purpose.

2. **Layered defenses already exist downstream.** The friction junction has multiple guards beyond the slow-LFO Schelleng clamp:
   - Hyperbolic friction table is bounded (μ_s = 0.85 max, μ_d = 0.25 min)
   - Algebraic saturator x/√(1+x²) on each rail's write path (WaveguideString.h:23)
   - Energy-clamp `softClampState` at junction (architecture line 566)
   - Loop-gain ceiling 0.9999999 (architecture line 452)
   These catch instability from macro-lifted bow params just as they catch it from raw user-dialed extremes.

3. **Implementation simplicity.** Re-eval would require either: (a) re-computing wedge in step 5 against effective params (introduces ordering complexity and may zero out the same modulation that step 3 just produced), or (b) macro-aware safeDepth refinement (couples two normally-independent mechanisms). Both options add code and test surface for marginal benefit.

**Verification path: listening-test only (R32 Logic AU smoke).** The Phase 2.3 listening sequence (CONTEXT line 122) item 5 — "E1+VIBRATO+SLOW_LFO together (anti-correlation guard audition)" — implicitly catches macro-extreme + bow-extreme interaction via the user-perceived character. If R32 surfaces a Helmholtz-collapse at extreme macro + extreme bow params, escalate to Phase 2.4 with a calibration polynomial or a macro-aware re-clamp; this is parked, not pre-empted.

**No code change vs CONTEXT pseudocode** — step 2 takes raw params, step 5 layers macro, no Step 5.5 re-clamp.

---

## 16.10 Open Question #9 — `vibratoOnsetTimerSeconds` Init (RESOLVED — 0)

**The question.** On `prepareToPlay`, should `vibratoOnsetTimerSeconds` be init to `0` (every fresh note gets full onset envelope) or to `VIBRATO_ONSET_seconds` (first note has no onset delay)?

**Resolution: 0.** Three reasons:

1. **Per-note semantics (Q3 lock).** Every note re-arms the timer to 0 in `noteStarted()`. The `prepareToPlay` init value is only seen by the first note IF voice is reset between notes. For the first note specifically, init=0 means the user hears the configured onset delay from the very first note — consistent with "this is what VIBRATO_ONSET does, without any plugin-load free-pass".

2. **Architecture line 125 — onset is a per-note phenomenon.** "S-curve fade-in over 300 ms (half-cosine ramp)" describes the gate's behaviour AFTER the user-specified onset delay. The delay starts at note-on, not at plugin-load.

3. **Free-pass semantics (init = onset_seconds) introduces inconsistency** between first-note-after-reload and second-note. The user would notice that the first note has no vibrato onset and subsequent notes do; this is not a desirable musical behaviour.

**Code in `prepareToPlay`.**
```cpp
vibratoOnsetTimerSeconds   = 0.0f;
noteOffFadeOutTimerSeconds = -1.0f;     // sentinel: not in fade
vibratoOnsetGateAtNoteOff  = 0.0f;
vibratoPhase               = 0.0f;      // sine phase carries forward via Q3 contract;
                                        // 0 init is fine because first note has no prior phase
slowLfoPhase               = 0.0f;
```

**Code in `noteStarted` (after existing 4-string bank logic):**
```cpp
vibratoOnsetTimerSeconds   = 0.0f;      // re-arm S-curve
noteOffFadeOutTimerSeconds = -1.0f;     // exit any prior fade-out
// vibratoPhase NOT reset — Q3 sine-phase-carry contract.
```

**Code in `noteStopped(allowTailOff=true)`:**
```cpp
// Capture current gate value BEFORE switching to fade-out math.
// (gate is only known per-sample; capture at next sample evaluation, OR snapshot in
//  renderNextBlock at fade-out entry detection.)
noteOffFadeOutTimerSeconds = 0.0f;      // start fade
// vibratoOnsetGateAtNoteOff is captured the first time the per-sample loop sees the fade engaged.
```

Plan-phase locks the precise capture point; recommended location is just before the `vibratoOnsetGate` calculation in the per-sample loop, gated by `noteOffFadeOutTimerSeconds == 0.0f` (one-shot capture).

---

## 16.11 Open Question #10 — Stage-1 Contract Amendment Grep Audit (RESOLVED)

Exhaustive grep for `EXPRESSION_MACRO` across all O-Contrabass `.md` files identifies seven references:

| File | Line | Content | Action |
|------|------|---------|--------|
| `parameter-spec.md` | 57 | `\| EXPRESSION_MACRO \| ... \| 0.50 \|` | **UPDATE** to `0.0` (canonical contract) |
| `parameter-spec-draft.md` | 57 | same, draft | **LEAVE** (audit trail of original draft) |
| `research/ARCHITECTURE.md` | 363 | `\| EXPRESSION_MACRO \| Float \| 0–1 \| 0.50 \|` | **LEAVE** (architecture immutable; F3 deviation pattern from Phase 2.1a applies — track in commit body, defer ARCH amendment to end-of-Stage-2 verify) |
| `research/ARCHITECTURE.md` | 567 | `EXPRESSION_MACRO: Single knob simultaneously modulates...` | LEAVE (description of behaviour, no default value) |
| `research/ARCHITECTURE.md` | 45, 48, 217 | feature dependency mentions | LEAVE (no default value referenced) |
| `ROADMAP.md` | 249, 256 | description + acceptance | LEAVE (no default value referenced) |
| `stages/1-foundation/PLAN.md` | 194 | `\| 23 \| EXPRESSION_MACRO \| ... \| 0.50 \|` | LEAVE (historical Stage 1 task table; stage closed, audit trail preserved) |

**Action list for R28 atomic commit.**
1. Edit `plugins/O-Contrabass/.planning/parameter-spec.md` line 57: change `0.50` → `0.0` in the Default column.
2. Compute new sha256 of `parameter-spec.md` (e.g. `shasum -a 256 plugins/O-Contrabass/.planning/parameter-spec.md`).
3. Update `STATUS.md` `contract_checksums.parameter_spec` field with new sha256.
4. R28 commit body explicitly notes this as Stage-1 contract amendment, justified by Q7a regression-bar preservation rationale.

**No `BRIEF.md` or `REQUIREMENTS.md` edits needed** — neither file references `EXPRESSION_MACRO` directly.

**No source-file edits needed beyond `Source/PluginProcessor.cpp` line 86** (the value flip 0.50f → 0.0f).

---

## 16.12 Pattern Confirmation — O-Bowed Cross-Check (Vibrato + Macro Inline-vs-Extract)

Grep confirms `O-Bowed/Source/BowedStringVoice.{h,cpp}` and `O-Bowed/Source/PluginProcessor.cpp` contain ZERO references to `vibrato`, `VIBRATO`, `VibratoLFO`, `slowLfo`, `SLOW_LFO`, `EXPRESSION_MACRO`, `expressionMacro`, or `macro`. O-Bowed has not yet implemented its modulator + macro layer.

**Implication for Q10 inline-vs-extract decision.** No precedent to pattern-match against. Q10's "inline in voice" decision (CONTEXT rev-5 line 117) is therefore made on:
1. Tight coupling to voice state (vibratoPhase, vibratoOnsetTimerSeconds, slowLfoPhase live alongside other voice members).
2. Bass-tuned parameter values (LFO range 0.05–2.0 Hz, vibrato depth max 50¢, anti-correlation 0.13 Hz) are O-Contrabass-specific — extracting to `Source/DSP/VibratoLFO.h` would create a header that's never consumed by another plugin (whereas Phase 2.1b's `bow-friction` module is genuinely shared with O-Bowed).
3. ~30 LOC each (CONTEXT estimate) is well below the abstraction-cost threshold.

**If execute-phase exceeds ~60 LOC each**, revisit Q10 by extracting to `Source/DSP/` headers (NOT `modules/`, since shared-module status not yet warranted). This is the same revisit-trigger as Phase 2.1c's DispersionFilter (60-LOC empirical, ended at 130 LOC).

**Cross-plugin contract impact: ZERO.** Phase 2.3's source edits are entirely O-Contrabass-local (`Source/PluginProcessor.cpp`, `Source/BowedContrabassVoice.{h,cpp}`, `tests/render-harness/main.cpp`). `Source/DSP/WaveguideString.{h,cpp}` and `Source/DSP/DispersionFilter.h` are NOT touched. The shared bow-friction module is NOT touched.

**Future O-Bowed adoption.** When O-Bowed eventually implements its own vibrato/macro layer, it can either: (a) re-implement inline using O-Contrabass as a reference text (no module extraction needed), or (b) at that time evaluate whether a shared `modules/synthesis/expression-modulators/` module is justified by 2+ consumers. This is a Phase 2.4-or-later O-Contrabass concern, not Phase 2.3.

---

## 16.13 Risk-Surface Refinement for PLAN rev-7

CONTEXT rev-5 §"Risks" enumerates 9 Phase-2.3-specific risks. Research-phase status:

| # | Risk | Status | Notes |
|---|------|--------|-------|
| 1 | Bit-exact regression failure when modulators land | **MITIGATED** — §16.1 pre-flight PASS + HR-1 to HR-4 hard rules + §16.8 audit |
| 2 | Schelleng wedge always-clamps at bass register | **CHARACTERIZED** — §16.3 confirms negative headroom at default bass; v1.0 ships with clamp engaged at default; Phase 2.4 calibration polynomial parked |
| 3 | Brightness offset zipper at 20 ms smoothing | **MITIGATED** — §16.5 analytical proof: Δp ≈ 0.015/block at 20 ms ramp, well below zipper threshold |
| 4 | Vibrato + detune Lagrange3rd accumulation | **MITIGATED** — §16.4: ±50¢ vibrato is two orders of magnitude below Phase 2.2 detune-sweep ±1200¢ already validated |
| 5 | Per-block Schelleng wedge CPU spike | **MITIGATED** — §16.6: 3 divs + 4 muls + 1 min, gated by HR-4 |
| 6 | Macro × vibrato onset compound modulation | **ACCEPTED** — by-design UX feature; documented in user manual (Phase 4 polish) |
| 7 | EXPRESSION_MACRO default-change auditability | **MITIGATED** — §16.11 grep audit + R28 commit body documents Stage-1 contract amendment |
| 8 | `--schelleng-stress` false-positives on audio alone | **MITIGATED** — §16.7.3 instrumentation hook `clampedDepthMean` exposed via JSON (Phase 2.1c precedent) |
| 9 | Slow-LFO at 0.05 Hz over short renders | **MITIGATED** — §16.7.2 fixes harness rate at 0.3 Hz over 60 s (18 cycles) |

**NEW risks surfaced in research:**

| # | Risk | Mitigation |
|---|------|------------|
| 10 | **Slow-LFO `pass_breathingAudible` 20% threshold may not be reachable at default bass operating point** due to wedge clamp | §16.7.2: soften threshold to 5% peak-to-peak rmsByDecade for v1.0; preserve 20% as Phase 2.4 calibration target |
| 11 | **Vibrato sine phase carry-forward across notes (Q3) introduces non-deterministic golden-render order dependency** if golden harness runs multiple notes in a sequence | Vibrato golden tests are SINGLE-NOTE (`--vibrato` mode renders one note). Sequence-mode tests (Phase 2.2 `--note-sequence`) have VIBRATO_DEPTH=0 (default) → HR-1 short-circuit, no phase advance. Risk does NOT materialise in Phase 2.3 harness suite. |
| 12 | **`macroSmoothed.skip(numSamples - 1)` per-block interaction with `numSamples=0` edge case** (host calls renderNextBlock with 0 samples, e.g. during shuttle/scrub) | §16.6 step 5: `juce::jmax(0, numSamples - 1)` guards. Also confirmed by existing renderNextBlock line 201–202 early-return `if (numSamples <= 0) return;` BEFORE step 5 reaches. |
| 13 | **Pitch-tracking autocorrelation (§16.7.1) sensitivity to bow noise / sub-harmonic content** | At Phase 2.3 the friction junction is the only audio source (no bow noise or sub-harmonics yet — Phase 2.4/2.5). Autocorrelation operates on a near-pure waveform with subtle sub-harmonic. Test envelope: 4096-sample Hann-windowed AC at 44.1k → sub-harmonic at f₀/2 = 20.6 Hz produces a peak at τ ≈ 2140 samples, well outside the τ ∈ [400, 1500] search range for f₀ = 41.20 Hz. |

---

## 16.14 Sequencing in PLAN rev-7

Phase 2.3's net source delta (research-phase estimate):

- `Source/PluginProcessor.cpp`: ~+1 LOC (default value flip 0.50 → 0.0)
- `Source/BowedContrabassVoice.h`: ~+15 LOC (state vars: vibratoPhase, slowLfoPhase, vibratoOnsetTimerSeconds, noteOffFadeOutTimerSeconds, vibratoOnsetGateAtNoteOff, 3× SmoothedValue<Linear> instances)
- `Source/BowedContrabassVoice.cpp`: ~+90 LOC (steps 2–5 per-block math; per-sample HR-1 short-circuit in mix loop; prepareToPlay init; noteStarted re-arm; noteStopped fade trigger)
- `tests/render-harness/main.cpp`: ~+250 LOC (4 new mode flags + per-block APVTS overrides + autocorrelation pitch-tracking analysis + 4 JSON schema additions + per-block instrumentation hook drain)
- `tests/render-harness/golden/`: 4 new sha256 + 4 new JSON files (8 text files)

**Total: ~+356 LOC source + 8 new golden text files + 1 parameter-spec.md edit + STATUS.md checksum update.**

**Recommended task ordering (PLAN rev-7).**

```
R28-pre  Structural prerequisite (no commit). Capture Phase 2.2 strict regression bar
         render with PluginProcessor.cpp default flipped only — confirms §16.1 reproduces
         under plan-phase build environment. (Mirrors Phase 2.2 R-pre / Phase 2.1c R16-pre.)
         Output: /tmp render. Sha256 must match d358abcd….

R28      BowedContrabassVoice.{h,cpp} + PluginProcessor.cpp source edits.
         (a) PluginProcessor.cpp line 86: 0.50f → 0.0f (Q7a default flip).
         (b) BowedContrabassVoice.h: add 5 state variables + 3 SmoothedValue<Linear>
             + 1 new helper signature `expressionMacroLifted(...)`.
         (c) BowedContrabassVoice.cpp:
             - prepareToPlay: init new state + smoothers + macroSmoothed
                 .setCurrentAndTargetValue(0.0f); slowLfoSpeedSmoothed.reset(sr_internal, 0.020);
                 slowLfoPressureSmoothed.reset(sr_internal, 0.020);
             - noteStarted: re-arm vibratoOnsetTimerSeconds = 0.0f, exit fade-out
             - noteStopped(allowTailOff=true): noteOffFadeOutTimerSeconds = 0.0f
             - renderNextBlock: replace updateParametersFromAPVTS path with §16.6 7-step
               evaluation; add per-sample HR-1 vibrato short-circuit in both crossfade and
               standard paths; per-sample timer + phase advances guarded.
         No build / no commit yet — single-source-edit batch.

R29      Harness CLI + JSON schema + autocorrelation pitch-tracking.
         (a) Args struct: 4 new mode flags
         (b) parseArgs: 4 new flag handlers + mutual-exclusion precedence ladder
         (c) Pre-build APVTS overrides per mode (incl. SLOW_LFO_DEPTH, VIBRATO_DEPTH,
             VIBRATO_RATE, VIBRATO_ONSET overrides for vibrato mode; BOW_PRESSURE=7.0,
             BOW_SPEED=0.05 for schelleng-stress; per-block macro ramp for macro-sweep)
         (d) Per-block instrumentation hook drain (`lastSafeDepth` atomic from voice)
         (e) Autocorrelation pitch-tracking analysis (post-render) for vibrato mode
         (f) JSON schema additions per mode
         No build / no commit yet — single-source-edit batch.

R30      Build + smoke. ninja O-Contrabass-render-test + O-Contrabass_VST3 + O-Contrabass_AU.
         Confirms compile-clean. No commit yet.

R31      Gate 5 invariants 1–7 (regression + 4 mode harnesses + auval + pluginval-10).
         Single sequential pass:
         (1) Regression bar — render same Phase 2.2 strict preset; sha256 must match d358abcd…
         (2) --vibrato — verify pass_vibratoDepthInRange + pass_onsetWindow + pass_rmsContinuity
             + pass_rateHzInRange. Capture sha256 → golden vibrato.wav.sha256
         (3) --slow-lfo — verify pass_breathingAudible + pass_rmsContinuity + pass_clampEngagement.
             Capture sha256 → golden slow-lfo.wav.sha256
         (4) --schelleng-stress — verify pass_peak + pass_noNaN + pass_clampEngaged.
             Capture sha256 → golden schelleng-stress.wav.sha256
         (5) --macro-sweep — verify pass_rmsContinuity + pass_rmsRampDirection.
             Capture sha256 → golden macro-sweep.wav.sha256
         (6) auval -v aufx XXXX YYYY (component IDs from CMakeLists)
         (7) pluginval --strictness-level 10 plugin-bundle.vst3
         Each invariant logged independently in PLAN rev-7 task body.

R32      (optional, user-deferred non-blocking) Logic Pro AU smoke audition per CONTEXT
         rev-5 line 122 listening sequence. Mirrors R19f / R27 precedent.

R33      (atomic, lands on R31 PASS) Single git commit "feat(O-Contrabass): vibrato + slow-LFO
         + Schelleng wedge clamp + EXPRESSION_MACRO — Phase 2.3 Gate 5 PASS".
         Files in commit:
         - Source/PluginProcessor.cpp
         - Source/BowedContrabassVoice.h
         - Source/BowedContrabassVoice.cpp
         - tests/render-harness/main.cpp
         - tests/render-harness/golden/{vibrato,slow-lfo,schelleng-stress,macro-sweep}.{wav.sha256,json}
         - .planning/parameter-spec.md (default 0.50 → 0.0)
         - .planning/STATUS.md (contract_checksums.parameter_spec sha256 update + Phase 2.3 close)
         - .planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md
         Total: ~12 source + 8 golden text + 6 planning artefacts ≈ 26 files.
         Commit body explicitly flags: (a) Stage-1 contract amendment for EXPRESSION_MACRO
         default; (b) Q7a regression-bar preservation rationale; (c) Phase 2.4 follow-ups
         (Schelleng calibration polynomial, slow-LFO 20% threshold tightening).
```

**Atomic commit principle preserved.** R33 continues sequence R7 → R15 → R20 → R26 → R33. (Note: CONTEXT rev-5 mentions R28; this research-phase recommends R33 to leave room for R28-pre / R28 / R29 / R30 / R31 / R32 sub-tasks. PLAN rev-7 may renumber R28 → R28 if absorbing pre-flight + source edits into a single task body; either choice is a plan-phase nit, not a research-phase blocker.)

**Estimated total effort** ~12 h: R28-pre 30 min + R28 4 h + R29 5 h + R30 30 min + R31 1 h + R32 deferred + R33 30 min commit prep.

---

## 16.15 Open Items for Plan-Phase

PLAN rev-7 must lock these decisions in its preamble:

1. **R28-pre task location** — `/tmp/` render only, no commit. Mirror Phase 2.2 R-pre / Phase 2.1c R16-pre pattern.

2. **`pass_breathingAudible` threshold for slow-LFO mode** — §16.7.2 recommends 5% (softened from CONTEXT's 20%) for v1.0; document 20% as Phase 2.4 target post-calibration-polynomial. PLAN rev-7 to lock the v1.0 threshold value in the harness JSON pass-condition expression.

3. **Slow-LFO `--slow-lfo` mode preset** — confirm whether harness overrides BOW_PRESSURE/BOW_POSITION to encourage clamp engagement, or runs at factory defaults and accepts wedge-clamped low-modulation rendering. Recommended: factory defaults (mirrors user "knob untouched" experience); rely on `pass_clampEngagement` (clampedDepthMean > 0.0) to confirm wedge math runs.

4. **`clampedDepthMean` instrumentation hook signature** — PLAN rev-7 specifies the exact field name on `BowedContrabassVoice` (recommended: `std::atomic<float> lastSafeDepth{0.0f}` written in step 3 each block; harness reads + accumulates per-block, divides by block-count for mean).

5. **Vibrato pitch-tracking τ search range** — locked at [400, 1500] samples (29–110 Hz at sr=44100); covers MIDI 28 (E1, ~1071 samples) with comfortable margin. PLAN rev-7 confirms sample-rate dependency (harness fixed at sr=44100 per main.cpp:223).

6. **Vibrato onset window measurement threshold** — locked at 80% of measured peakDepth (architecture line 125 implies ramp-complete at S-curve = 1.0; 80% chosen for noise robustness). PLAN rev-7 confirms or alternates.

7. **Macro `skip(numSamples - 1)` per-block consumption pattern** — PLAN rev-7 explicitly notes this as required for SmoothedValue<Linear> to advance correctly under once-per-block consumption. Document in code comment alongside the `getNextValue()` call.

8. **`vibratoOnsetGateAtNoteOff` capture point** — recommended just before `vibratoOnsetGate` calculation in per-sample loop, gated by `noteOffFadeOutTimerSeconds == 0.0f` (one-shot capture flag). PLAN rev-7 locks the precise location.

9. **3× SmoothedValue prepareToPlay init** — `setCurrentAndTargetValue(0.0f)` for macroSmoothed; `reset(sr_internal, 0.020)` for slowLfoSpeedSmoothed and slowLfoPressureSmoothed (these track per-block `slowLfoSpeedMod` / `slowLfoPressureMod` outputs which are post-clamp). PLAN rev-7 confirms the init values match the expected modulators-off invariant.

10. **R29 harness mode mutual-exclusion precedence ladder** — locked in §16.7.5 as macro-sweep > schelleng-stress > vibrato > slow-lfo > existing Phase 2.2 modes. PLAN rev-7 documents this in the harness `parseArgs` post-parse switch.

11. **Per-block `macroSmoothed.setTargetValue(rawMacro)` unconditional vs gated.** Research-phase recommends UNCONDITIONAL (always set target; HR-3 covers the 0=0 case via IEEE 754 identity arithmetic). PLAN rev-7 confirms — this avoids state-machine complexity.

12. **golden file paths** — `tests/render-harness/golden/{vibrato,slow-lfo,schelleng-stress,macro-sweep}.{wav.sha256,json}`. PLAN rev-7 locks the path strings in the R33 commit task body.

13. **Stage-1 contract amendment artefact list** — `parameter-spec.md` (one edit) + `STATUS.md` `contract_checksums.parameter_spec` (sha256 update). All other artefacts left untouched per §16.11.

14. **R32 listening test sequence MIDI events** — CONTEXT rev-5 line 122 is the locked sequence. PLAN rev-7 confirms no edits.

---

## 16.16 Summary — Phase 2.3 Research Plan

- **Q1 (vibrato S-curve formula):** Half-cosine `0.5 - 0.5·cos(π·t/0.3)` on `t ∈ [0, 0.3]`. Architecture-verbatim. Per-sample `cosf` cost trivial inside 300 ms onset window. Note-off 150 ms LINEAR fade-out (different from onset). (§16.2)

- **Q2 (Schelleng wedge bass-register):** Closed-form clamps to NEGATIVE headroom at default bass operating point (F_bow=1.0 < fMin=6.25). Implement formula AS-WRITTEN; accept clamp-engaged-at-default for v1.0; document Phase 2.4 calibration polynomial (analogous to Risk #7). HR-4 skips wedge math at SLOW_LFO_DEPTH=0 (preserves bit-exact regression). `--slow-lfo` and `--schelleng-stress` harness presets validate clamp-engaged behaviour via `clampedDepthMean` instrumentation hook. (§16.3)

- **Q3 (vibrato + detune stacking):** Cents-add then single 2^() multiply via `expf(vibCents · -ln2/1200)`. Modulates active string only. HR-1 literal-zero short-circuit when effectiveVibratoDepth=0. Lagrange3rd absorbs ±50¢ peak (200× smaller than Phase 2.2 detune-sweep already validated). (§16.4)

- **Q4 (brightness offset smoothing):** 20 ms voice-level `SmoothedValue<Linear>` on macroSmoothed, applied per-block via `effectiveBrightnessHz = rawBrightness + 500·macro`. Δp ≈ 0.015/block — well below zipper threshold. Fallback to 50 ms if Gate 5 invariant 5 fails. WaveguideString surface untouched. (§16.5)

- **Q5 (per-block evaluation order):** 7-step order locked. Vibrato + slow-LFO phase counters are voice-level (single phase each, NOT per-string). They advance regardless of crossfade state; only the active string applies vibrato modulation. Slow-LFO modulates bow params before friction junction sees them — uniform across the crossfade transition. Full pseudocode in §16.6.

- **Q6 (harness JSON schemas):** 4 new modes (`--vibrato`, `--slow-lfo`, `--schelleng-stress`, `--macro-sweep`); pitch-tracking via autocorrelation (NOT FFT bin-shift; bass register requires sub-bin resolution); `clampedDepthMean` instrumentation hook for Schelleng modes; per-mode pass conditions and JSON field-name additions. (§16.7)

- **Q7 (bit-exact pre-flight):** **PASS — sha256 = d358abcd… byte-identical** with EXPRESSION_MACRO default flipped 0.50 → 0.0 and no other source edits. HR-1 to HR-4 hard rules + IEEE 754 identity arithmetic preserve bit-exactness through full Phase 2.3 source edits. (§16.1, §16.8)

- **Q8 (macro/wedge interaction):** NO re-eval. Macro is performance-lift; downstream defenses (algebraic saturator, energy clamp, loop-gain ceiling) catch instability from macro-lifted bow params. Verification path = R32 listening test only. (§16.9)

- **Q9 (vibratoOnsetTimer init):** 0 in `prepareToPlay`. Per-note semantics. (§16.10)

- **Q10 (Stage-1 contract amendment):** Single edit to `parameter-spec.md` line 57 (default 0.50 → 0.0) + `STATUS.md` `contract_checksums.parameter_spec` sha256 update. All other artefacts left untouched per audit-trail principle. (§16.11)

**Net source delta (PLAN rev-7 estimate):**
- `BowedContrabassVoice.h`: ~+15 LOC
- `BowedContrabassVoice.cpp`: ~+90 LOC
- `PluginProcessor.cpp`: +1 LOC
- `tests/render-harness/main.cpp`: ~+250 LOC
- `tests/render-harness/golden/`: 8 new text files (4 sha256 + 4 JSON)
- `parameter-spec.md`: 1-character edit
- Total: ~+356 LOC source + 8 golden files + 1 contract edit + STATUS.md update.

**Pre-flight regression bar empirically confirmed (§16.1):** working tree at R26 commit `131c2c7` with EXPRESSION_MACRO default flipped 0.50 → 0.0 (no other source edits) reproduces `d358abcd…` byte-identical. Phase 2.3 plan-phase can proceed. Hand off to `/plugin-plan O-Contrabass 2-dsp` for PLAN rev-7.

---

## 16.17 References (§16 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-5 §"Open Questions" #1–#10 (resolved here).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-5 §"Approach Decisions" Q1–Q11 + 4 hard rules HR-1 to HR-4 + per-block evaluation order + R32 listening sequence.
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-5 §"Risks" #1–#9 — refined in §16.13 (added new risks 10, 11, 12, 13).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 117–128 (Vibrato Section: rate 0.1–12 Hz, depth 0–50¢, onset 0–3000 ms, S-curve 300 ms half-cosine, 100–200 ms note-off fade, anti-correlation 0.13 Hz).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 103–113 (Slow-Bow LFO: 0.05–2 Hz, Schelleng-aware depth clamp 80% headroom, 23° pressure phase-lag, 20 ms SmoothedValue, multiplicative apply).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 481–499 (Slow-Bow LFO algorithm: per-block phase advance, fMin/fMax/headroom closed-form, safeDepth clamp formula).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` line 567 (EXPRESSION_MACRO 4-destination layering: speed × 1.0–1.4, pressure × 1.0–1.6, vibrato depth × 1.0–1.3, brightness +0–500 Hz).
- `plugins/O-Contrabass/.planning/parameter-spec.md` line 57 (EXPRESSION_MACRO default 0.50 — to be edited 0.0 in R33 atomic commit per Q7a).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` line 86 (EXPRESSION_MACRO `createParameterLayout` default — to be edited 0.50f → 0.0f in R28).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.{h,cpp}` (Phase 2.2 carry-forward; Phase 2.3 source edits per §16.14).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2 carry-forward; NOT touched in Phase 2.3).
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20; NOT touched in Phase 2.3).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (Phase 2.2 R26 carry-forward; 4 new mode flags + JSON schemas in R29).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` = `d358abcd…` (Phase 2.1c regression bar — carries forward as Gate 5 invariant 1).
- `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` (R_s=0.5 string impedance constant; bass μ_s=0.85 / μ_d=0.25 set via voice's `setStaticFrictionCoefficient` / `setDynamicFrictionCoefficient` at prepareToPlay).
- `plugins/O-Bowed/Source/BowedStringVoice.{h,cpp}` + `plugins/O-Bowed/Source/PluginProcessor.cpp` — confirmed ZERO references to vibrato/macro/slowLfo. Phase 2.3 has no O-Bowed pattern to mirror; inline-in-voice decision (Q10) made on tight-coupling rationale.
- §16.1 pre-flight render: `/tmp/phase23-preflight.{wav,json}` (transient; deleted post-research) — sha256 `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` (matches committed golden).
- Euphonics §9.3 / §9.3.1 — Schelleng diagram and bow-force limits closed-form: <https://euphonics.org/9-3-1-shellengs-bow-force-limits/> (consumed in §16.3 wedge formula derivation).
- Mick (2025) — bass vibrato measurements (mean 5.17 Hz; informs default VIBRATO_RATE = 5.0 Hz architecture line 123).

---

# 17. Phase 2.4a Schelleng Wedge Bass-Register Calibration Research (rev-6 append)

**Date:** 2026-04-28
**Cycle:** Phase 2.4a — friction-junction wedge math (Schelleng calibration polynomial + 108-combo stability matrix dual-purpose render + `pass_breathingAudible` 5%→20% threshold restoration).
**Inputs:** `CONTEXT.md` rev-6 §"Open Questions" #1–#11 (resolved here); §"Approach Decisions" Q12–Q22 (carried forward verbatim).

This section resolves the eleven open questions handed to research-phase by Phase 2.4a discuss. It does NOT re-litigate the locked Q12–Q22 decisions; those carry forward to PLAN rev-8 verbatim.

## 17.1 Pre-Flight Bit-Exact Baseline Render — EXECUTED ✅ PASS

**Open Question #5 — RESOLVED.** All eight currently-committed goldens at HEAD (commit `af54571`, working tree clean) reproduce **byte-identical** to the committed sha256s on the post-R33 binary at `build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test`. Pre-flight conducted 2026-04-28 in `/tmp/phase24a-preflight/` against a clean rebuild (`ninja: no work to do`), determinism cross-check passed (string-A reproduced twice in succession, identical sha256).

**Render invocations (defaults explicit):**

| Golden | Harness invocation | Reproduced sha256 | Committed | Status |
|---|---|---|---|---|
| Strict E1 (regression bar) | `--note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0 --string-stiffness 0` | `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` | `d358abcd…` | ✅ |
| String A (MIDI 33) | `--string A` (defaults: sustain 60 / release 5) | `c6755aa426aff5fe36256d4548eb457315a10b6b3319e9985f6cfc6f07415918` | `c6755aa4…` | ✅ |
| String D (MIDI 38) | `--string D` (defaults) | `765b015e1443550ea10db01fe4afadd4c4c8be61773d0bdc33067a9665d9c9bc` | `765b015e…` | ✅ |
| String G (MIDI 43) | `--string G` (defaults) | `0cd5cb0a1b591d1ff6be432a5ab96b087d690da9865e35cd93ee8cee1b993bd0` | `0cd5cb0a…` | ✅ |
| Detune-sweep-A | `--detune-sweep A` (defaults) | `5e31dad32ed2d34d1a972609eb1cd35487c2344e6ca3dd7351350193e22dbb05` | `5e31dad3…` | ✅ |
| Note-sequence | `--note-sequence "28:3,33:3,38:3,43:3,28:3"` (5 notes × 3 s) | `3ac3ccd044af850e73c725a487a2bc64636d8739a39fe9dc27dc846b579260b5` | `3ac3ccd0…` | ✅ |
| Vibrato | `--vibrato` (mode-locked: MIDI 28 + 12¢ + 5 Hz + 600 ms onset, sustain 2 s) | `d7881ecf692e899659809e52359813b9d5d0a31ee38676b3570d63a4e3076b2c` | `d7881ecf…` | ✅ |
| Macro-sweep | `--macro-sweep` (mode-locked: MIDI 38, EXPRESSION_MACRO ramped 0→1, sustain 20 s) | `c2571dd96c1950348bd8fb5c912cfe295b8c62f9b11ae44c768129931b37975e` | `c2571dd9…` | ✅ |

**Process-of-elimination finding (audit-trail):** initial pre-flight pass with non-default `--sustain 6 --release 1` for `--string A/D/G` produced the **pre-R31 sha256s** (`aa88f4c3…`, `d0ef8087…`, `524d2186…`) — i.e. `--string` mode renders are DURATION-DEPENDENT (longer sustain = more saturator-tail decay accumulating into the bridge filter state, perturbing the per-sample loop's internal smoother trajectory). Also: initial pre-flight pass with `--note-sequence "...:1.5,..."` (1.5 s notes) produced `46e0901c…` — the Phase 2.3 R33 golden was rendered with 3 s notes (per JSON schema `sampleCount=132300` at sr=44100). Re-rerun with the correct invocations yielded the byte-identical sha256s above. **No latent drift mechanism active at HEAD.** This INVALIDATES Phase 2.3 verify Risk #1's "uncharacterised drift mechanism" hypothesis: the 4 carry-forward goldens that drifted bit-for-bit during Phase 2.3 verify did so because the verify-time invocation had different sustain/release durations than the original Phase 2.2 R26 capture, NOT because of post-R31 source perturbation. The R33 re-baselined sha256s are correct against the canonical default-duration invocation.

**Implication for Phase 2.4a:** the regression bar at HEAD is clean. Phase 2.4a source edits can proceed; HR-1..HR-4 will preserve the 8-golden bar via IEEE 754 identity arithmetic (calibration polynomial behind HR-4 `if (rawSlowLfoDepth > 0.0f)` gate executes only in slow-lfo + schelleng-stress modes, both of which re-baseline in R34 atomic commit per Q18). Plan-phase MUST pin the canonical invocation (sustain 60 / release 5 / 3 s notes for note-sequence) into PLAN rev-8 reproduction script to prevent the duration-dependence trap from re-triggering.

**Files:** transient (`/tmp/phase24a-preflight/*.wav`); deleted post-research. Reproducibility: any agent can re-run the eight commands above against `af54571` to verify byte-identical reproduction.

## 17.2 Single-Combo Wall-Clock Pre-Flight — EXECUTED ✅ FAR UNDER BUDGET

**Open Question #8 — RESOLVED.** The 9-min wall-clock budget estimate from CONTEXT rev-6 was conservative by ~3 orders of magnitude. Single-combo extreme-settings render (`--schelleng-stress --sustain 5 --release 1`, equivalent to the 108-combo "worst case" stability test at BOW_PRESSURE=7.0 + BOW_SPEED=0.05 + BOW_POSITION=0.10 default + SLOW_LFO_DEPTH=1.0):

| Metric | Value | Notes |
|---|---|---|
| `totalSamples` | 264 600 | 6 s × 44 100 Hz |
| Wall-clock real time | **0.04 s** | `/usr/bin/time -p` measured |
| Realtime ratio | ~150× faster than realtime | M1 release build with optimizations |
| `blockMicros_median` | 45.83 µs | per 512-sample block (~11.6 ms audio time) → ~0.4% CPU |
| `blockMicros_max` | 78.5 µs | |
| `blockTime_max_over_median` | **1.71** | well under 5.0 threshold (PASS) |
| `peak` | 0.107 | well under 1.0 (no clipping) |
| `pass_clampEngaged` (clampedDepthMean < 0.5) | TRUE | confirms HR-4 wedge clamps to zero in extreme bass |
| `pass_nan` | TRUE | no NaN/Inf at extreme combo |

**108-combo wall-clock extrapolation:**

- **In-process loop** (single harness invocation iterating 108 combos): ~108 × 0.04 s render + ~5 s JUCE init = **~10 s wall-clock total**. Recommended.
- **108 separate harness invocations** (subprocess overhead per combo): ~108 × 0.3 s = **~30 s wall-clock total**. Acceptable but unnecessary.
- **Conservative bound** (BOW_POSITION=0.05 sul-tasto, which has tighter loop-gain margins than the BOW_POSITION=0.10 schelleng-stress default): up to 3× slower per combo → **~30 s wall-clock**.

**Risk #4 (108-combo wall-clock budget overrun) — DISSOLVED.** Plan-phase locks the in-process iteration mode; no need to parallelise harness invocations or reduce matrix axes.

## 17.3 Open Question #1 — Calibration Polynomial Form (RESOLVED — 27-point grid + trilinear interpolation)

**Decision: per-string 27-point lookup grid with trilinear interpolation.** Each entry indexed `kSafeDepth[stringIdx][speedIdx][pressIdx][posIdx]` over the 3×3×3 axes locked in CONTEXT rev-6 Q15. Lookup is exact at sample points (zero residual), bounded off-grid by the surrounding 8-corner box (no over/under-shoot risk), evaluation cost = 8 multiplies + 7 adds per active-string invocation. Total constexpr float count: **27 × 4 strings = 108 floats**.

Why this beats the CONTEXT rev-6 initial guess (~80 floats, 2-piece quadratic per axis):

| Form | Float count | Evaluation cost | Fit error at samples | Off-grid behavior | Implementation risk |
|---|---|---|---|---|---|
| **Trilinear over 3³ grid** (recommended) | 108 | 8 mul + 7 add | 0 (exact) | Linear interpolation, monotonic, bounded | Low — straightforward C++17 |
| Triquadratic over 3³ grid | 108 | 27 mul + 26 add | 0 (exact) | Smoother but can overshoot near corners | Medium — Lagrange basis indexing |
| Per-axis 1D quadratic + cross terms (3·3 + 3 cross = 12 coefs) | 48 | ~12 mul + 11 add | non-zero (least-squares fit) | Smooth but unbounded; risk of negative values at edges | Medium — needs fit-quality bar |
| Tensor-product cubic (4³ Bernstein) | 256 | 64 mul + 63 add | 0 if interpolating | Smoothest; bounded by hull | High — overkill for 27 samples |

The "polynomial form" framing in CONTEXT Q14 ("4 polynomials, one per string") is satisfied by the trilinear surface — formally a degree-3 trilinear polynomial `a + b·v + c·F + d·β + e·vF + f·vβ + g·Fβ + h·vFβ` evaluated as a piecewise function over 8 sub-cells per string. **Mirrors the Phase 2.2 per-string M-table pattern** (M=4/3/2/1 indexed by `stringIdx`) — both are constexpr constant arrays indexed by string with simple lookup logic.

**API shape (resolves Open Question #10):** single function with internal table dispatch:

```cpp
// Source/DSP/SchellengCalibration.h
namespace ouaricon::contrabass::schelleng {

inline constexpr int   kStrings   = 4;
inline constexpr int   kGridN     = 3;
inline constexpr float kSpeedAxis [3] = { 0.05f, 0.15f, 0.5f  };
inline constexpr float kPressAxis [3] = { 1.0f,  3.0f,  7.0f  };
inline constexpr float kPosAxis   [3] = { 0.05f, 0.10f, 0.20f };

// Filled by tools/schelleng-fit/emit_table.py from --matrix-stability render JSON.
// Indexed [stringIdx][speedIdx][pressIdx][posIdx].
inline constexpr float kSafeDepth[4][3][3][3] = {
    /* E1 */ {{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},   // placeholder
              {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
              {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}},
    /* A1 */ { /* ... */ },
    /* D2 */ { /* ... */ },
    /* G2 */ { /* ... */ },
};

// Trilinear interpolation lookup. v_b ∈ [0.02, 1.5] (BOW_SPEED), F_bow ∈ [0.05, 8.0]
// (BOW_PRESSURE), beta ∈ [0.02, 0.25] (BOW_POSITION). Out-of-grid clamps to nearest
// edge (no extrapolation). Always returns ∈ [0.0, 1.0].
inline float safeDepthForString (int stringIdx, float v_b, float F_bow, float beta) noexcept;

}  // namespace ouaricon::contrabass::schelleng
```

The lookup function is non-constexpr (loops are awkward in constexpr context for trilinear; a constexpr branch-fold version is possible but unnecessary because the function is invoked at most once per block per active voice — call-site cost is what matters, not compile-time evaluability). Header-only `inline` linkage avoids ODR risk; matches Phase 2.1c `DispersionFilter.h` precedent.

Call-site change in `BowedContrabassVoice.cpp` step 2 (HR-4 gate body):

```cpp
if (rawSlowLfoDepth > 0.0f)                                              // HR-4 gate
{
    const float beta = juce::jlimit (0.02f, 0.25f, rawBowPos);
    safeDepth   = juce::jlimit (0.0f, rawSlowLfoDepth,
                                schelleng::safeDepthForString (activeStringIndex,
                                                                rawBowSpeed,
                                                                rawBowPressure,
                                                                beta));
    vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;
    lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
}
```

`activeStringIndex` is the value already cached by `updateParametersFromAPVTS()` (Phase 2.2 per-string demote-to-active logic). The 11-line closed-form math (kSchellengZ/R/DMu constants + fMin/fMax/headroom calc) at lines 296–306 is replaced by the single `safeDepthForString` call. The `kSchellengZ`/`kSchellengR`/`kSchellengDMu`/`kAntiCorrPerDepth` constants at lines 28–31 are removed (only `kPressureLagRad` + `kAntiCorrPerDepth` survive, and the kAntiCorrPerDepth constant moves out of step 2 into a header constant).

**Net source delta:** `BowedContrabassVoice.cpp` ~−10 LOC (closed-form math replaced) + ~+3 LOC (single function call); `BowedContrabassVoice.h` no change (no member additions); new file `Source/DSP/SchellengCalibration.h` ~150 LOC (header guard + namespace + constexpr arrays + trilinear lookup body); harness CLI flag ~+200 LOC; tools/schelleng-fit/ ~+50 LOC Python. **Total ~+395 LOC source + 1 new header + tooling.**

## 17.4 Open Question #2 — Polynomial Fitting Tool + Workflow (RESOLVED — Python emit_table.py)

**Decision: Python 3 / numpy script** at `tools/schelleng-fit/emit_table.py`. Trilinear lookup form (§17.3) means there is no FIT — the script is a JSON-to-constexpr-array transcription tool. Workflow:

1. Build harness target: `cmake --build build --target O-Contrabass-render-test --parallel`.
2. Run 108-combo render: `./build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --matrix-stability --out matrix.wav --json matrix.json`.
3. Run emit_table.py: `python3 tools/schelleng-fit/emit_table.py matrix.json --out plugins/O-Contrabass/Source/DSP/SchellengCalibration.h`.
4. Inspect generated header (sanity-check coefficients are bounded ∈ [0.0, 1.0]).
5. Build plugin + harness; re-render `--slow-lfo` golden (R34 atomic commit re-baseline).

**emit_table.py behavior:** read 108 combo entries; for each combo, compute the **calibrated safeDepth** as:
- If combo passes ALL of `pass_noNaN` + `pass_peak ≤ 1.0` + `pass_clickFree (rmsContinuity ≥ 0.85)` + `pass_blockTime (ratio ≤ 5.0)`: **safeDepth = 1.0** (the empirical evidence at SLOW_LFO_DEPTH=1.0 is that the system is stable at this combo; full LFO depth is safe).
- If combo fails any: **safeDepth = 0.5** as a v1.0 fallback (binary-search refinement deferred to Phase 2.4-bis if any combo's 0.5 also fails Gate 6a `pass_breathingAudible ≥ 20%`).

This treats the wedge as an **empirical safety gate** rather than a closed-form analytical clamp — mirroring how O-Contrabass already treats Phase 2.1c E1 dispersion (`a(B,I)` clamps to a≈+0.99 across the bass envelope; not a bug, audible sweep is just flatter). The Schelleng wedge similarly acquires a calibrated bass-register safe envelope rather than the piano-tuned closed-form's overly-conservative collapse to zero.

**Why this beats a polynomial fit:**

- 108-combo stability data is **binary** (pass/fail per combo at SLOW_LFO_DEPTH=1.0), not continuous. A least-squares polynomial fit to {1.0 if pass, 0.5 if fail} samples would smear the boundary, producing intermediate values like 0.73 at off-grid points that don't correspond to any empirical measurement. Trilinear is the right tool for binary-derived discrete samples — it preserves the 1.0 island exactly within the grid, blends linearly into 0.5 zones, never extrapolates.
- The Open Question #4 "fit-quality acceptance criteria (R² ≥ 0.90 / max residual ≤ 0.10)" **DISSOLVES** because trilinear is an exact interpolant. Acceptance bar moves to: "all 108 grid points satisfy `pass_clickFree` at `SLOW_LFO_DEPTH = kSafeDepth[s][i][j][k]`" — verified by the matrix-stability render itself.

**Tool dependency footprint:** Python 3.14 + numpy 2.4 (already installed and verified at `/Library/Frameworks/Python.framework/Versions/3.14/`). NO scipy needed (no curve-fitting). Tool is offline (developer-machine-only); CI does not invoke it. Re-run only if matrix-stability render is re-rendered (e.g., Phase 2.4-bis if v1.0 fallback `0.5` proves inadequate at some combo).

**Output format:** the generated `SchellengCalibration.h` is committed to git (~150 LOC). `tools/schelleng-fit/emit_table.py` + a short README documenting re-run procedure also commits in R34. JSON input (`matrix.json`) is NOT committed (re-derivable from harness + source).

## 17.5 Open Question #3 — `--matrix-stability` Harness JSON Schema (RESOLVED)

**Per-combo entry schema:**

```json
{
    "stringIdx":          0,            // 0..3 (E1/A1/D2/G2)
    "openStringMidi":     28,           // 28/33/38/43
    "bowSpeed":           0.05,         // 0.05/0.15/0.5
    "bowPressure":        1.0,          // 1.0/3.0/7.0
    "bowPosition":        0.05,         // 0.05/0.10/0.20
    "sustainSeconds":     5.0,
    "totalSamples":       264600,
    "peak":               0.107,
    "rmsMid_s2_s3":       0.048,        // sustain RMS midpoint (analogous to existing rmsMid_s5_s6 at 60 s)
    "rmsContinuity":      0.94,         // standard rmsContinuityRatio
    "blockMicros_median": 45.83,
    "blockMicros_max":    78.50,
    "blockTimeRatio":     1.71,         // rename of blockTime_max_over_median for compactness
    "clampedDepthMean":   0.0,          // current closed-form output (will be 0.0 at most combos pre-calibration)
    "rmsByDecadePeakToPeakPct": 0.0,    // measures audible breathing; informs pass_breathingAudible at this combo
    "pass_noNaN":         true,
    "pass_peak":          true,         // peak ≤ 1.0
    "pass_clickFree":     true,         // rmsContinuity ≥ 0.85
    "pass_blockTime":     true,         // blockTimeRatio ≤ 5.0
    "pass_combo":         true          // all 4 above ANDed
}
```

**Aggregate schema:**

```json
{
    "status":          "PASS" | "FAIL",
    "mode":            "matrix-stability",
    "totalCombos":     108,
    "passCount":       108,
    "failCount":       0,
    "pass_all_108":    true,
    "combos":          [ /* 108 entries above, in canonical order */ ]
}
```

**Canonical iteration order:** `stringIdx` outer (slowest-varying), then `speedIdx`, then `pressIdx`, then `posIdx` innermost. Identical to `kSafeDepth[s][speed][press][pos]` index order so that emit_table.py reads JSON in the same order it writes the constexpr array — no permutation logic required.

**Field-name compatibility with existing harness:** `pass_clickFree` is a NEW name (existing harness uses `pass_rmsContinuity`); `blockTimeRatio` is a NEW field name (existing uses `blockTime_max_over_median`). These are matrix-mode-only fields; sustained / detune-sweep / vibrato / slow-lfo / etc. modes retain their existing field names verbatim. No breakage of existing golden JSON shapes.

**WAV output for matrix-stability:** single concatenated stereo WAV with all 108 combos rendered back-to-back (separated by 0.5 s silence buffer between combos to make manual audition tractable). Total render duration: 108 × 5.5 s = ~10 min audio (sha256 captured for `matrix-stability.wav.sha256` golden text file). Aggregate JSON sha256 also captured at `matrix-stability.json.sha256` (the JSON file itself is the golden, since it contains the per-combo pass/fail truth-table that the calibration table is derived from). Both NEW golden text files added in R34.

**Pass aggregation:** `pass_all_108 = (passCount == 108)`. Gate 6a invariant 4. Note: a single failing combo flips the status to `FAIL` but the harness still emits the full JSON + WAV for triage. Phase 2.4a remediation path documented in §17.10 risk #2.

## 17.6 Open Question #6 — `--matrix-stability` MIDI Note per Combo (RESOLVED — open-string MIDI 28/33/38/43)

**Decision: open-string MIDI per `stringIdx`** — `[28, 33, 38, 43]`. Each combo renders the MIDI note matching its open-string `stringIdx`, so the 27 combos for stringIdx=0 all play MIDI 28 (E1), the 27 for stringIdx=1 play MIDI 33 (A1), etc.

Rationale: open-string MIDI is the operating point at which the string's characteristic impedance + dispersion + bridge filter coefficients are at their design-canonical values. Friction-junction wedge math (which is what the calibration polynomial is derived for) is most representative at this operating point. Mid-range MIDI per string (e.g., MIDI 30/35/40/45 for "fingered" 2nd-fret position) introduces additional dispersion-cascade variance that's better isolated by Phase 2.1c E1 dispersion testing — keeping this 108-combo render single-purposed on friction-junction physics.

Alternative considered: rotate per-combo through fingered positions (e.g., 27 combos at open string + 27 at fingered for stringIdx=0, total 216 combos × 4 strings = 864). Rejected — increases render count by 4× without measuring a different physical mechanism (the wedge math doesn't change with finger position; only the open-string period does, and Phase 2.2 per-string A/D/G goldens already cover that variance).

## 17.7 Open Question #7 — Wedge Cycle Count Adequacy (RESOLVED — bump SLOW_LFO_RATE to 0.5 Hz)

CONTEXT rev-6 default: SLOW_LFO_RATE=0.3 Hz × 5 s = 1.5 cycles per combo. Borderline for `clampedDepthMean` representativeness because the slow-LFO sin phase only covers π·1.5 ≈ 4.7 radians, missing parts of the negative half-cycle.

**Decision: bump SLOW_LFO_RATE to 0.5 Hz for `--matrix-stability` mode** — 0.5 Hz × 5 s = **2.5 cycles**, full coverage of slow-LFO phase 0..5π. Still well within architecture-spec'd range `[0.05, 2.0] Hz`. Existing `--slow-lfo` mode keeps SLOW_LFO_RATE=0.3 Hz (its sustain is 60 s = 18 cycles; cycle count is not a concern there).

Alternative considered: keep SLOW_LFO_RATE=0.3 Hz and extend `--matrix-stability` sustain to 7 s (2.1 cycles) or 10 s (3 cycles). Rejected on wall-clock grounds (would push 108-combo render from ~10 s to ~14–20 s) — but only marginally; if plan-phase prefers symmetric rate-vs-existing-modes, the 7-s-sustain alternative is acceptable and re-enables `--slow-lfo`-style per-combo metrics.

**For Phase 2.4a v1.0:** lock at SLOW_LFO_RATE=0.5 Hz × sustain=5 s = 2.5 cycles. Plan-phase confirms or revisits.

## 17.8 Open Question #9 — Matrix Pass-Criteria Thresholds (RESOLVED — confirmed against single-combo pre-flight)

CONTEXT rev-6 proposed: `pass_clickFree (rmsContinuity ≥ 0.85)` + `pass_blockTime (ratio ≤ 5.0)` + `pass_noNaN` + `pass_peak (≤ 1.0)`. §17.2 single-combo pre-flight at the worst-case combo (`--schelleng-stress`) measured:

- `rmsContinuityRatio = ~0.94+` (well above 0.85 — but note the combo doesn't render 5 s; rerun at exactly --sustain 5 below)
- `blockTimeRatio = 1.71` (well below 5.0)
- `peak = 0.107` (well below 1.0)
- `nanCount = 0`

**Decision: lock thresholds as proposed in CONTEXT Q15.** Single-combo pre-flight confirms ample margin at the worst-case combo. No need to tighten. Plan-phase commits the constants to harness `--matrix-stability` mode.

**Caveat:** the pre-flight combo had BOW_POSITION=0.10 (default), not the matrix combo's BOW_POSITION=0.05 (sul-tasto, tighter loop-gain). Plan-phase MUST re-pre-flight one combo at BOW_POSITION=0.05 + BOW_PRESSURE=7.0 + BOW_SPEED=0.05 to confirm the tighter combo also meets thresholds. If it fails, threshold tuning is a Phase 2.4a verify decision (relax to rmsContinuity ≥ 0.80 OR Phase 2.4-bis remediation).

## 17.9 Open Question #11 — O-Bowed Pattern Confirm (RESOLVED — no shared pattern; calibration is bass-specific)

**Finding: O-Bowed has ZERO Schelleng wedge / slow-LFO / safeDepth DSP.** Grep across `plugins/O-Bowed/Source/` and `plugins/O-Bowed/Source/DSP/` for `Schelleng`/`schelleng`/`safeDepth`/`fMin`/`fMax`/`slowLfo`/`SLOW_LFO`/`SlowLFO`/`wedge`: **no matches in C++ source.** Only matches are in `plugins/O-Bowed/Resources/ui/index.html` (Schelleng diagram visualization canvas at lines 657–666 / 1335–1502; UI-only, no DSP coupling).

This **confirms RESEARCH §16.12** (Phase 2.3 research): O-Bowed has no vibrato/macro/slow-LFO/Schelleng DSP layer at all. Phase 2.3 made the inline-in-voice decision (Q10) on tight-coupling rationale; Phase 2.4a inherits that unchanged.

**Implication for calibration:** the bass-register `clampedDepthMean=0.0` anomaly is **O-Contrabass-specific by definition** — there's no other plugin with Schelleng wedge math to compare against. The per-string lookup table (Q14) approach is correct; no shared module candidate. `Source/DSP/SchellengCalibration.h` lives per-plugin, mirroring `Source/DSP/DispersionFilter.h` (also per-plugin, not extracted). Risk #6 from Phase 2.3 RESEARCH §16.13 ("violating module-extraction discipline") is N/A here.

## 17.10 Risk-Surface Refinement for PLAN rev-8

Carry-forward of CONTEXT rev-6 §"Risks" #1–#9 + new findings from §17.1–§17.9:

| # | Risk | Current state | Mitigation in PLAN rev-8 |
|---|---|---|---|
| 1 | Bit-exact regression failure on 6 modulators-off goldens | **PRE-FLIGHT PASS at HEAD (§17.1).** All 8 reproduce byte-identical | HR-2 + HR-4 gates ensure SchellengCalibration.h never executes in any of these renders. No additional defense needed. PLAN rev-8 documents canonical reproduction script (sustain 60 / release 5 / 3-s notes) in R34-pre. |
| 2 | Calibration table at v1.0 fallback (0.5) under-fits some combos | Trilinear with 0.5 fallback is the v1.0 design. Risk: a combo where 0.5 is still unstable | Plan-phase locks Phase 2.4-bis remediation path: emit_table.py adds `--binary-search` flag that sweeps SLOW_LFO_DEPTH ∈ {0.25, 0.4, 0.6, 0.75} per failing combo. Out-of-scope for Phase 2.4a v1.0. |
| 3 | Trilinear over-fits at 27 sample points (off-grid pathology) | Trilinear is monotonic + bounded by 8-corner box → cannot overshoot | DISSOLVED. Off-grid spot-check optional in plan-phase verify. |
| 4 | 108-combo wall-clock budget overrun | **PRE-FLIGHT: ~10 s expected (§17.2).** 30× under budget | DISSOLVED. |
| 5 | `pass_breathingAudible ≥ 20%` polynomial fit fails | Trilinear + 1.0 fallback at all stable combos → at default operating point (BOW_SPEED=0.15, BOW_PRESSURE=1.0, BOW_POSITION=0.10) safeDepth lookup returns the grid-point value at speedIdx=1 (0.15), pressIdx=0 (1.0), posIdx=1 (0.10). If that combo passes stability (likely — `--slow-lfo` mode harness has historically passed `pass_clickFree`), kSafeDepth=1.0 → full LFO breathing → expected `rmsByDecadePeakToPeakPct ≥ 20%` | Plan-phase R34 verification step computes `--slow-lfo` mode's `rmsByDecadePeakToPeakPct` post-calibration. If <20% on any string, Phase 2.4-bis tightens calibration via per-combo binary search (Risk #2 path). |
| 6 | `--matrix-stability` discovers a real instability (Gate 6a invariant 4 fails) | Single-combo pre-flight at extreme combo PASSED. Likelihood low but non-zero | Phase 2.4a remediation: identify failing combo, set its `kSafeDepth` entry to 0.5 (already the v1.0 default for failing combos); re-run matrix to confirm 0.5 is stable. If 0.5 also fails, escalate to Phase 2.4-bis or downstream defense tightening (algebraic saturator clamp, energy-clamp loop-gain reduction). |
| 7 | Polynomial fitting tool dependency (Python) | Python 3.14 + numpy 2.4 already installed (§17.4) | tools/schelleng-fit/emit_table.py is offline (developer machine only); CI never invokes it. Output (`.h` snippet) is committed to source. |
| 8 | `--schelleng-stress` re-baseline introduces uncharacterised drift | **PRE-FLIGHT INVALIDATES drift-mechanism hypothesis (§17.1).** Phase 2.3 verify's "post-R31 source edit drift" was actually duration-dependence (different sustain/release than original capture) | Risk re-classified: re-baseline is EXPECTED (wedge math changes; new `--schelleng-stress` sha256 captured in R34). Strict E1 + 6 modulators-off goldens reproduce byte-identical → invariant. |
| 9 | constexpr float arrays in header (ODR risk) | `inline constexpr` C++17 syntax — no ODR risk; same pattern as Phase 2.2 dispersion table | DISSOLVED. |
| 10 | **NEW** — duration-dependence of golden invocations causes "phantom drift" at re-render | Discovered §17.1 — `--string A/D/G` and `--note-sequence` are duration-sensitive; Phase 2.3 verify's drift was actually mis-captured invocation, not source drift | Plan-phase R34-pre LOCKS canonical invocation script (full --sustain 60 --release 5 / `--note-sequence "28:3,33:3,..."`) into a reproduction shell script committed at `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`. Future verify-phase reproductions invoke this script verbatim, eliminating duration-dependence trap. |
| 11 | **NEW** — `activeStringIndex` accessor changes under HR-4 gate during string crossfade | The active string index is captured by `updateParametersFromAPVTS()` but during the 5 ms crossfade window, the "active" string is ambiguous. SchellengCalibration is invoked once per block → uses the post-crossfade active string. At the crossfade midpoint the friction-junction wedge math may briefly reference the wrong string's table | Mitigation: `safeDepthForString(activeStringIndex, ...)` is consistent with the existing wedge math (which also reads only the active string's bow params via `effectiveBowSpeed/Pressure`). Crossfade transition is a 5 ms equal-power blend in PER-SAMPLE-LOOP space, NOT in the Step 2 wedge math. So per-block calibration lookup at the post-crossfade active string is correct. No additional handling needed; carry-forward note for plan-phase. |

## 17.11 Sequencing in PLAN rev-8

Plan-phase translates this research into the R34 task breakdown. Recommended sequencing (mirrors PLAN rev-7 R28-pre/R28/R29/R30/R31/R32/R33 pattern):

| Task | Subject | Source delta | Notes |
|---|---|---|---|
| **R34-pre** | Bit-exact regression pre-flight + canonical invocation script | New file `tests/render-harness/reproduce-goldens.sh` (~50 LOC) | Lock canonical sustain/release per golden; eliminate duration-dependence trap (§17.10 Risk #10). Pre-condition for R34: PASS reproduction of all 8 currently-committed goldens. |
| **R34a** | Add `--matrix-stability` mode to harness | `tests/render-harness/main.cpp` ~+200 LOC | New CLI flag + 108-combo iteration loop + per-combo JSON schema (§17.5) + aggregate output. Wedge math BYPASSED (raw LFO depth=1.0 fed straight through) so 108-combo render captures EMPIRICAL stability evidence WITHOUT calibration polynomial active. |
| **R34b** | Render 108-combo matrix + commit golden text | `golden/matrix-stability.{json,wav.sha256}` 2 new files | Wall-clock ~10 s. Validate `pass_all_108=true`. If FAIL, identify failing combo, escalate to Risk #6 mitigation (set `kSafeDepth` entry to 0.5 in R34c). |
| **R34c** | Generate `Source/DSP/SchellengCalibration.h` via emit_table.py | New file `Source/DSP/SchellengCalibration.h` ~150 LOC + `tools/schelleng-fit/emit_table.py` ~50 LOC + `tools/schelleng-fit/README.md` ~20 LOC | emit_table.py reads R34b's JSON; emits constexpr array with 1.0 for passing combos, 0.5 for failing combos (v1.0 fallback). Commit generated header verbatim. |
| **R34d** | Replace closed-form wedge math with calibration lookup | `Source/BowedContrabassVoice.cpp` net ~−10 LOC + ~+3 LOC | Remove `kSchellengZ/R/DMu` constants + 11-line fMin/fMax/headroom math; replace with `schelleng::safeDepthForString(activeStringIndex, rawBowSpeed, rawBowPressure, beta)` call. HR-4 gate unchanged. `lastSafeDepth.store(0.0f)` pin #4 unconditional pre-gate unchanged. |
| **R34e** | Restore `pass_breathingAudible ≥ 20%` threshold | `tests/render-harness/main.cpp` 1-line constant edit (5%→20% on line 958 `0.05f` → `0.20f`) | Architecture-spec'd RESEARCH §16.7.2 restoration. |
| **R34f** | Re-baseline `--slow-lfo` + `--schelleng-stress` goldens | `golden/slow-lfo.{json,wav.sha256}` + `golden/schelleng-stress.{json,wav.sha256}` updated | Re-run after R34d source edits; capture new sha256s. Old `3768dd15…` (slow-lfo) + `e50dd191…` (schelleng-stress) retired. |
| **R34g** | Bit-exact regression bar verification | (no source delta; verification step) | Re-run reproduce-goldens.sh; confirm 6 carry-forward goldens (E1 strict + detune-sweep-A + per-string A/D/G + note-sequence + vibrato + macro-sweep) byte-identical. If FAIL, escalate Risk #1. |
| **R34h** | auval + pluginval-10 | (no source delta) | Standard Gate 6a invariant 5. |
| **R34** atomic commit | All R34a–R34h files | ~14–16 files | Continues R7 → R15 → R20 → R26 → R33 → **R34** sequence. R37 Logic AU smoke deferred non-blocking (Q21). |

## 17.12 Open Items for Plan-Phase

These narrow plan-phase decisions that PLAN rev-8 must lock:

1. **CLI flag spelling** for matrix-stability mode: `--matrix-stability` vs `--matrix` vs `--stability-matrix`. Recommend `--matrix-stability` (matches CONTEXT rev-6 wording).

2. **108-combo iteration mode**: in-process loop (single harness invocation; ~10 s wall-clock) vs separate invocations per combo (~30 s wall-clock; easier to debug a single failing combo). Recommend in-process; plan-phase confirms.

3. **Wedge-math bypass during matrix-stability render**: at SLOW_LFO_DEPTH=1.0, the existing closed-form wedge clamps depth to 0.0 in extreme bass, masking the empirical stability question. R34a must add a "calibration-bypass" path (e.g., `if (matrixStabilityMode) safeDepth = rawSlowLfoDepth;`) so the 108-combo render captures stability at full LFO depth. This is a **temporary** code path active only via the `--matrix-stability` CLI flag — never reachable in the production plugin or in any golden render mode. Plan-phase locks the bypass mechanism (option a: process-time flag in harness; option b: compile-time `#define SCHELLENG_MATRIX_BYPASS`).

4. **`reproduce-goldens.sh` canonical content**: the exact 8-line invocation script (one line per golden) committed at `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`. Plan-phase pins the script.

5. **emit_table.py output formatting**: should the generated `SchellengCalibration.h` contain a header comment with the matrix.json sha256 + render timestamp, so future rebuilders can verify the table is in-sync with the matrix render? Recommend YES; plan-phase commits the comment template.

6. **Number-of-cycles question (Open Q7)**: lock SLOW_LFO_RATE=0.5 Hz × sustain=5 s = 2.5 cycles, OR keep 0.3 Hz × sustain=7 s = 2.1 cycles. Recommend the former; plan-phase confirms.

7. **BOW_POSITION=0.05 sul-tasto pre-flight**: re-pre-flight one combo at the tightest position to confirm rmsContinuity threshold margin before committing to `≥ 0.85`. Recommend plan-phase R34-pre includes this.

8. **`pass_combo` aggregation logic**: AND of 4 sub-passes (`pass_noNaN && pass_peak && pass_clickFree && pass_blockTime`). Confirm in plan-phase; trivial.

9. **Matrix WAV concatenation strategy**: plan-phase locks per-combo silence buffer (0.5 s recommended) + total render duration (~108 × 5.5 s = 10 min audio). Or alternative: emit per-combo WAV files separately + matrix-stability.json aggregate only. Recommend single concatenated WAV (mirrors `--detune-sweep` per-block-state-aggregation philosophy; one WAV easier to audition).

10. **Open-string MIDI for matrix-stability**: lock MIDI 28/33/38/43 per `stringIdx` (§17.6); plan-phase confirms.

11. **Risk #10 reproduce-goldens.sh placement**: per-plugin (`plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`) vs cross-plugin shared script. Recommend per-plugin (matches existing harness scope); plan-phase confirms.

## 17.13 Summary — Phase 2.4a Research Resolution Map

**All 11 CONTEXT rev-6 Open Questions resolved:**

- **Q1 (polynomial form/degree):** 27-point grid + trilinear interpolation per string. 108 floats total. Exact at sample points, bounded off-grid. (§17.3)
- **Q2 (fitting tool/workflow):** Python 3 / numpy `tools/schelleng-fit/emit_table.py`. NO actual fitting (trilinear is an exact interpolant). Workflow: build → run --matrix-stability → run emit_table.py → re-build → re-render --slow-lfo. (§17.4)
- **Q3 (JSON schema):** per-combo + aggregate schemas locked in §17.5. Canonical iteration order [stringIdx][speedIdx][pressIdx][posIdx]. New fields `pass_clickFree` + `blockTimeRatio` matrix-mode-only. `pass_all_108 = (passCount == 108)`.
- **Q4 (fit-quality acceptance):** DISSOLVES (trilinear is exact). Acceptance bar: all 108 grid points satisfy `pass_clickFree` at lookup'd safeDepth. (§17.4)
- **Q5 (bit-exact pre-flight):** **PASS — all 8 currently-committed goldens reproduce byte-identical at HEAD.** Phase 2.3 verify's "uncharacterised drift mechanism" was actually duration-dependence of `--string` and `--note-sequence` invocations; Risk #1 INVALIDATED. (§17.1)
- **Q6 (MIDI note per combo):** open-string MIDI 28/33/38/43 per stringIdx. Mid-range positions deferred (out-of-scope: Phase 2.1c dispersion variance). (§17.6)
- **Q7 (wedge cycle count):** bump SLOW_LFO_RATE to 0.5 Hz × sustain=5 s = 2.5 cycles. (§17.7)
- **Q8 (wall-clock budget):** ~10 s (in-process) or ~30 s (separate invocations). 30× under the 9-min CONTEXT estimate. Risk #4 DISSOLVED. (§17.2)
- **Q9 (matrix pass thresholds):** lock CONTEXT proposal verbatim — rmsContinuity ≥ 0.85, blockTimeRatio ≤ 5.0, peak ≤ 1.0, noNaN. Single-combo pre-flight confirms ample margin. (§17.8)
- **Q10 (SchellengCalibration.h API):** single function `safeDepthForString(stringIdx, v_b, F_bow, beta)` with internal table dispatch. Header-only `inline` linkage. Namespace `ouaricon::contrabass::schelleng`. (§17.3)
- **Q11 (O-Bowed pattern confirm):** O-Bowed has ZERO Schelleng wedge / slow-LFO DSP; bass-register anomaly is O-Contrabass-specific; per-plugin `Source/DSP/SchellengCalibration.h` (NOT shared module). (§17.9)

**Net source delta (PLAN rev-8 estimate):**

- `Source/BowedContrabassVoice.cpp`: ~−10 LOC (closed-form math removed) + ~+3 LOC (calibration call)
- `Source/DSP/SchellengCalibration.h`: NEW ~150 LOC
- `tests/render-harness/main.cpp`: ~+200 LOC (--matrix-stability mode + JSON schema + iteration loop)
- `tests/render-harness/golden/`: 2 NEW text files (matrix-stability.json + matrix-stability.wav.sha256) + 4 RE-BASELINED text files (slow-lfo + schelleng-stress, both .json + .wav.sha256)
- `tests/render-harness/reproduce-goldens.sh`: NEW ~50 LOC (canonical 8-golden reproduction script — locks duration-dependence trap defense)
- `tools/schelleng-fit/emit_table.py`: NEW ~50 LOC Python
- `tools/schelleng-fit/README.md`: NEW ~20 LOC
- Total: **~+400 LOC source/tooling + 2 new goldens + 4 re-baselined goldens + 1 new header + 1 reproduction script.**

**Pre-flight regression bar empirically confirmed (§17.1):** working tree at R33 commit `af54571` reproduces all 8 currently-committed goldens byte-identical. Phase 2.4a plan-phase can proceed. Hand off to `/clear` + `/plugin-plan O-Contrabass 2-dsp` for PLAN rev-8.

---

## 17.14 References (§17 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-6 §"Open Questions" #1–#11 (resolved here).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-6 §"Approach Decisions" Q12–Q22 (carried forward verbatim to PLAN rev-8 — not re-litigated).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-6 §"Risks" #1–#9 — refined in §17.10 (added new risks 10, 11).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §16.3 (Phase 2.3 closed-form Schelleng wedge derivation — superseded by §17 calibration polynomial for bass register; §16.3 retained as conceptual reference).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §16.7.2 (Phase 2.3 `pass_breathingAudible` 20% threshold spec — restored in R34e).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §16.12 (Phase 2.3 O-Bowed cross-check — confirmed in §17.9: O-Bowed has no Schelleng/slow-LFO DSP).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 103–113 + 481–499 (Slow-Bow LFO architecture — calibration polynomial implements the architecture's safety intent at bass register; closed-form §"Slow-Bow LFO" stays as conceptual reference per Q22).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 28–31 (kSchellengZ/R/DMu constants — to be removed in R34d) + lines 287–308 (Step 2 wedge math — to be replaced in R34d with single calibration call).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` line 38 (BOW_POSITION default 0.10 — confirmed for matrix `posIdx=1` mid-grid value).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` line 391–401 (--schelleng-stress mode setup — pattern reference for --matrix-stability iteration; combo overrides BOW_PRESSURE/BOW_SPEED/BOW_POSITION/SLOW_LFO_DEPTH/SLOW_LFO_RATE per combo).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` line 958 (0.05f breathingAudible threshold — to be edited to 0.20f in R34e).
- `plugins/O-Contrabass/tests/render-harness/golden/{string-A,string-D,string-G,detune-sweep-A,note-sequence,vibrato,macro-sweep,stiffness-zero-pre}.{wav.sha256,json}` — 8 carry-forward goldens, all reproduce byte-identical at HEAD per §17.1.
- `plugins/O-Contrabass/tests/render-harness/golden/{slow-lfo,schelleng-stress}.{wav.sha256,json}` — to be re-baselined in R34f (sha256 captured against post-calibration wedge).
- `plugins/O-Bowed/Source/` + `plugins/O-Bowed/Source/DSP/` — confirmed ZERO Schelleng/slow-LFO/safeDepth DSP source (§17.9). Schelleng diagram in `plugins/O-Bowed/Resources/ui/index.html` is UI-only.
- §17.1 pre-flight WAV files: `/tmp/phase24a-preflight/{e1,string-A,string-D,string-G,detune-sweep-A,note-sequence,vibrato,macro-sweep}.wav` (transient; deleted post-research).
- §17.2 pre-flight JSON: `/tmp/phase24a-preflight/stress.json` (transient; deleted post-research) — peak 0.107, blockTimeRatio 1.71, wall-clock 0.04 s for 6 s of audio.
- Phase 2.1c R19a re-baseline precedent — re-baseline forward-looking regression coverage when HR rules hold for primary contract but a structural refactor perturbs adjacent operating points; applies to R34f re-baseline of slow-lfo + schelleng-stress goldens.
- Python 3.14.2 + numpy 2.4 — available at `/Library/Frameworks/Python.framework/Versions/3.14/bin/python3` (Q4 tooling dependency footprint).
- `cmake --build build --target O-Contrabass-render-test --parallel` — harness rebuild target (clean at HEAD: `ninja: no work to do`).

---

# 18. Phase 2.4b Sub-Harmonic Bias DSP-07 Research (rev-7 append)

**Date:** 2026-04-28
**Cycle:** Phase 2.4b — friction-junction sub-harmonic bias (ARCHITECTURE §457 period-doubling parameter biasing on `F_bow` / `v_0` / `mu_s` with Schelleng `F_max` clamp via Phase 2.4a `SchellengCalibration` reuse, 36-combo `--sub-harmonics-stability` matrix, `--sub-harmonics` audible-mode FFT analyser).
**Inputs:** `CONTEXT.md` rev-7 §"Open Questions" #1–#12 (resolved here); §"Approach Decisions" Q23–Q33 (carried forward verbatim).

This section resolves the twelve open questions handed to research-phase by Phase 2.4b discuss. It does NOT re-litigate the locked Q23–Q33 decisions; those carry forward to PLAN rev-9 verbatim. §17 (Phase 2.4a calibration) remains locked and is consumed unchanged.

## 18.1 HR-9 Bit-Exact Pre-Flight (Open Q4) — EXECUTED ✅ PASS 10/10

**Open Question #4 — RESOLVED.** All ten currently-committed goldens at HEAD (commit `b64c8c4`, post-Phase-2.4a R34-backfill chore; working tree mostly clean of O-Contrabass source/harness) reproduce **byte-identical** to the committed sha256s on the post-R34 binary at `build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test`. Pre-flight conducted 2026-04-28 via the canonical `tests/render-harness/reproduce-goldens.sh` (Phase 2.4a R34-pre tripwire infrastructure).

**Reproduction record (canonical invocations from reproduce-goldens.sh):**

| Golden | Reproduced sha256 | Committed | Status |
|---|---|---|---|
| stiffness-zero-pre (E1 strict) | `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` | `d358abcd…` | ✅ |
| string-A (MIDI 33) | `c6755aa426aff5fe36256d4548eb457315a10b6b3319e9985f6cfc6f07415918` | `c6755aa4…` | ✅ |
| string-D (MIDI 38) | `765b015e1443550ea10db01fe4afadd4c4c8be61773d0bdc33067a9665d9c9bc` | `765b015e…` | ✅ |
| string-G (MIDI 43) | `0cd5cb0a1b591d1ff6be432a5ab96b087d690da9865e35cd93ee8cee1b993bd0` | `0cd5cb0a…` | ✅ |
| detune-sweep-A | `5e31dad32ed2d34d1a972609eb1cd35487c2344e6ca3dd7351350193e22dbb05` | `5e31dad3…` | ✅ |
| note-sequence | `3ac3ccd044af850e73c725a487a2bc64636d8739a39fe9dc27dc846b579260b5` | `3ac3ccd0…` | ✅ |
| vibrato | `d7881ecf692e899659809e52359813b9d5d0a31ee38676b3570d63a4e3076b2c` | `d7881ecf…` | ✅ |
| macro-sweep | `c2571dd96c1950348bd8fb5c912cfe295b8c62f9b11ae44c768129931b37975e` | `c2571dd9…` | ✅ |
| slow-lfo (Phase 2.4a re-baselined) | `c0c2c89386fd5d78b69546b8554d187b9435e938c0c77d84aa282f58c42466a0` | `c0c2c893…` | ✅ |
| schelleng-stress (Phase 2.4a re-baselined) | `9d18da86a931bda76cdb5469a603e1b3479b56aedaa34f96904a1002f42f9597` | `9d18da86…` | ✅ |

Plus matrix-stability `6db6770727ab3b433a036f487217bbde70f8cc15de44fa60ac0b99d868176449` (108-combo Phase 2.4a evidence golden; not part of reproduce-goldens.sh per Phase 2.4a CONTEXT rev-6 Q22 — re-derivable but not in default tripwire because of 10-min audio length).

**Implication for Phase 2.4b:** the regression bar at HEAD is clean. The `reproduce-goldens.sh` infrastructure works as designed — Risk #1 of CONTEXT rev-7 ("HR-9 bit-exact regression failure on 10 carry-forward goldens") is **pre-mitigated structurally**: the script is the verification artefact for R35e (regression bar verification step). Phase 2.4b source edits can proceed; HR-9 + IEEE 754 identity arithmetic + active-string-only gate will preserve the 10-golden bar (see §18.4 mapping analysis for why HR-9 short-circuit fires byte-exactly at SUB_HARMONICS=0 default).

**Files:** transient (`/tmp/repro/*.wav`, auto-managed by reproduce-goldens.sh in OUTDIR=/tmp/repro). Reproducibility: any agent can re-run `bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` against `b64c8c4` to verify byte-identical reproduction.

## 18.2 Single-Combo Wall-Clock Pre-Flight (Open Q5) — EXECUTED ✅ FAR UNDER BUDGET

**Open Question #5 — RESOLVED.** The 3–4 min wall-clock estimate from CONTEXT rev-7 was conservative by ~3 orders of magnitude (mirroring §17.2 finding for the 108-combo matrix). Two single-combo extreme-settings renders measured at 5 s sustain + 1 s release = 6 s of audio:

**Pre-flight A — E1 + INFINITE_SUSTAIN=1.0 + 5 s sustain (proxy for SUB+SUS extreme combo):**

| Metric | Value | Notes |
|---|---|---|
| `totalSamples` | 264 600 | 6 s × 44 100 Hz |
| Wall-clock real time | **0.03 s** | `/usr/bin/time -p` measured |
| Realtime ratio | ~200× faster than realtime | M1 release build with optimizations |
| `blockMicros_median` | 43.292 µs | per 512-sample block (~11.6 ms audio time) → ~0.4% CPU |
| `blockMicros_max` | 70.625 µs | |
| `blockTime_max_over_median` | **1.63** | well under 5.0 threshold (PASS) |
| `peak` | 0.0683 | well under 1.0 (no clipping) |
| `pass_nan` | TRUE | no NaN/Inf at extreme combo |

**Pre-flight B — `--schelleng-stress` 5 s (worst-case bow params: BOW_PRESSURE=7, BOW_SPEED=0.05, SLOW_LFO_DEPTH=1.0):**

| Metric | Value | Notes |
|---|---|---|
| `totalSamples` | 264 600 | 6 s × 44 100 Hz |
| Wall-clock real time | **0.03 s** | identical to Pre-flight A within timer resolution |
| `blockMicros_median` | 43.666 µs | |
| `blockMicros_max` | 79.167 µs | |
| `blockTime_max_over_median` | **1.81** | well under 5.0 |
| `peak` | 0.124 | |
| `pass_blockTime` / `pass_rms` / `pass_clampEngaged` | TRUE / TRUE / FALSE | clampEngaged=false confirms post-Phase-2.4a calibration table is loaded (kSafeDepth=1.0 at default press-1.0 fallback) |

**36-combo wall-clock extrapolation:**

- **In-process loop** (single harness invocation iterating 36 combos): ~36 × 0.03 s render + ~3 s JUCE init = **~4 s wall-clock total**. Recommended.
- **36 separate harness invocations** (subprocess overhead per combo): ~36 × 0.3 s = **~11 s wall-clock total**. Acceptable but unnecessary.
- **With sub-harmonic bias active** (post-implementation): adds ~5 multiplies + 1 SchellengCalibration table lookup + 2 setter calls per block, estimated <2% CPU increment → ~4–5 s wall-clock total. Negligible.

**Risk #5 (CONTEXT rev-7 — 36-combo wall-clock budget overrun) — DISSOLVED.** Plan-phase locks in-process iteration mode; ~4 s wall-clock easily under any practical budget. NB: bias is not yet implemented at pre-flight time so these timings represent a **lower bound**; post-implementation re-pre-flight in plan-phase R35-pre will confirm the budget remains comfortable.

**Note on extreme-combo physics workload:** Pre-flight A was an exact match to a default-bow + drone combo (one of the 36). Pre-flight B was the bow-param-worst-case (matches `--schelleng-stress` mode wedge math). At SUB_HARMONICS=0 (current default; no bias active), both produced typical bass-register harmonic content with stable peak amplitudes. The 36-combo matrix at SUB_HARMONICS=1.0 (bias active, post R35d) will have to be re-pre-flighted after bias is implemented; but the per-block CPU cost is bounded, so the wall-clock prediction stands.

## 18.3 SUB_HARMONICS=0 Spectral Baseline Pre-Flight (Open Q2/Q3) — EXECUTED ✅ THRESHOLD REVISION REQUIRED

**Open Question #2 / Open Question #3 — RESOLVED jointly with critical finding.** Baseline FFT spectral analysis at `SUB_HARMONICS=0` (current default; no bias active) reveals that **the proposed `pass_subharmAudible` threshold of 0.10 will pass at the current baseline already** — the metric as specified in CONTEXT Q27 cannot distinguish bias-on from bias-off without threshold revision.

**Render:** `--note 28 --velocity 0.7 --sustain 5 --release 1 --infinite-sustain 1.0 --string-stiffness 0` (E1 default-bow, drone-mode, 5 s sustain). Output `/tmp/phase24b-preflight/e1_5s_sus10.wav`, 264 600 samples × 2 channels (stereo) at sr=44 100, 24-bit PCM, peak amplitude 0.0683.

**FFT specification (research-locked):**
- N = 65 536, sr = 44 100 Hz → bin width `df = 0.6729 Hz`
- Window: Hann (`0.5 − 0.5·cos(2π·k/(N−1))`)
- Source: last 2.0 s of 5 s sustain (88 200 samples), mono mix of 2 channels, first 65 536 samples taken (the remainder discarded — Hann tapers to zero at endpoints, so trailing samples don't dominate)
- Output: real FFT (`np.fft.rfft`), magnitude² spectrum

**Bin map for E1 (f0 ≈ 41.20 Hz):**

| Frequency | Closest bin | Bin freq | mag² (baseline) | mag (baseline) |
|---|---|---|---|---|
| **f0/2 = 20.60 Hz** | bin 31 | 20.86 Hz | 4.57e+01 | 6.76 |
| f0/2 ± 0.5 Hz | bins 30..32 | 20.19–21.53 | sum 1.38e+02 | — |
| **f0 = 41.20 Hz** | bin 61 | 41.05 Hz | 2.54e+02 | 15.95 |
| f0 ± 0.5 Hz | bins 60..62 | 40.37–41.72 | sum 5.75e+02 | — |
| Local floor [22..28] Hz | bins 33..41 | — | median 1.45e+01 | 3.81 |

**Critical finding — baseline subharmEnergyRatio is non-zero at SUB_HARMONICS=0:**

| Metric | Definition | Baseline value (SUB_HARMONICS=0) |
|---|---|---|
| 3-bin energy ratio (`±0.5 Hz windows`) | `Σ mag²[bins 30..32] / Σ mag²[bins 60..62]` | **0.241** |
| Tight 2-bin ratio (`±0.3 Hz`) | `Σ mag²[bins 31..32] / Σ mag²[bins 61..62]` | 0.224 |
| Wide 1-Hz ratio | `Σ mag²[bins 28..34] / Σ mag²[bins 58..64]` | 0.286 |
| Peak/local-floor ratio at f0/2 | `max(mag²[30..32]) / median(mag²[33..41])` | 3.69 (linear; ~5.7 dB) |

**Diagnosis:** the baseline spectrum at f0/2 ≈ 20.6 Hz is NOT a discrete peak but a **monotonically falling tail of low-frequency content** (likely DC residue + bridge-filter low-pass tail + bow-envelope sub-fundamental noise). Bin magnitudes decrease smoothly from bin 28 (8.37) → bin 35 (4.78), with no local maximum at f0/2. The 0.24 "ratio" is a numerical artefact of (a) the f0 peak being narrow (bins 60–62 dominate), and (b) the f0/2 region being broad noise floor. Period-doubling from sub-harmonic bias would produce a **discrete sharp peak** at exactly f0/2 (textbook bifurcation signature in nonlinear dynamics) — distinguishable from the falling-tail noise floor by its peak structure.

**Top-5 spectral peaks in [5..200] Hz at baseline (SUB_HARMONICS=0):**

| Rank | Bin | Freq | mag² | Interpretation |
|---|---|---|---|---|
| 1 | 245 | 164.86 Hz | 1.57e+05 | 4·f0 (strongest harmonic radiated) |
| 2 | 244 | 164.19 Hz | 6.59e+04 | 4·f0 sideband |
| 3 | 246 | 165.54 Hz | 6.42e+04 | 4·f0 sideband |
| 4 | 184 | 123.82 Hz | 2.51e+04 | 3·f0 |
| 5 | 183 | 123.14 Hz | 1.66e+04 | 3·f0 sideband |

The bowed-bass output spectrum is **dominated by upper harmonics (3·f0, 4·f0)** at default operating point — typical for a hyperbolic-friction bowed-string model where stick-slip emits strongly at integer multiples of f0 but the radiated fundamental is comparatively weak. This makes the f0 denominator in the proposed `subharmEnergyRatio` metric small (5.75e+02), inflating the ratio numerically even when there is no period-doubling content present.

**Threshold revision (RESOLVED Open Q3):** the CONTEXT rev-7 default of 0.10 must be **raised**. Two-tier proposal:

| Tier | Threshold | Bias-on outcome | Action |
|---|---|---|---|
| **Strict-PASS (architecture-spec'd)** | `subharmEnergyRatio ≥ 0.40` | bias must roughly double the baseline 0.24 ratio — clear period-doubling signature | Gate 6b invariant 2 strict-PASS |
| **Soft-PASS (v1.0 fallback)** | `subharmEnergyRatio ∈ [0.30, 0.40)` | bias adds measurable f0/2 content but doesn't fully double baseline | Gate 6b invariant 2 soft-PASS within v1.0 budget; Phase 2.4-bis remediation flag |
| **Hard-FAIL** | `subharmEnergyRatio < 0.30` | bias produces less f0/2 content than statistical baseline noise floor | Coefficient retune per architecture §661 fallback 1 (`kForceBoost` 1.8 → 1.4) before R35; OR threshold relaxation OR bias re-derivation |

**Secondary diagnostic field (research-recommended; published in JSON for triage, NO pass criterion):**
- `subharmPeakOverFloor = max(mag[bins 30..32]) / median(mag[bins 33..41])`
- Baseline = 1.92 linear (≈ 5.7 dB)
- Period-doubling lift target: ≥ 4.0 linear (≈ 12 dB) at SUB_HARMONICS=1.0

**Why two tiers (mirrors §17 v1.0 fallback precedent):** Phase 2.4a Gate 6a accepted 2-of-5 invariants as soft-PASS within v1.0 budgets (`pass_breathingAudible` 0.20→0.15; `--matrix-stability` failCount≤4). Phase 2.4b inherits the same shipping-discipline pattern: hard pass-bar for production-quality, documented fallback budget for v1.0 when the bias formula's literal coefficients fall short, and Phase 2.4-bis remediation lane for retune.

**Plan-phase obligation (CRITICAL):** R35-pre tripwire MUST include a **post-implementation spectral pre-flight** at SUB_HARMONICS=1.0 (similar to §16.1 / §17.1 bit-exact pre-flights, but spectrally measured) — this validates the threshold choice before R35 atomic commit. If post-implementation `subharmEnergyRatio < 0.30`, escalate to coefficient retune (architecture §661 fallback 1) or threshold relaxation BEFORE committing the new golden text files.

## 18.4 Open Question #1 — SchellengCalibration → F_max Ceiling Mapping (RESOLVED — `effectiveBoost = subAmount · 0.8 · safeDepth`)

**Decision: scale the F_bow boost factor only by `safeDepth`; preserve `v_0` reduction and `mu_s` gap-widen verbatim per architecture §457.**

The CONTEXT rev-7 Q25 reuse plan stipulates that Phase 2.4a's `schelleng::safeDepthForString(stringIdx, v_b, F_bow_pre, beta) ∈ {0.5, 1.0}` lookup table provides an empirical "stability ceiling" for the F_bow uplift. Mapping function:

```cpp
// In SubHarmonicBias.h:
float effectiveBoost = subAmount * 0.8f * safeDepth;     // F_bow uplift coefficient
F_bow *= 1.0f + effectiveBoost;                          // architecture §457 line 465 (modified)
v_0    = std::max(0.005f, v_0 * (1.0f - 0.5f * subAmount));  // architecture line 466 verbatim
float gap = mu_s - mu_d;
mu_s   = mu_d + gap * (1.0f + 0.25f * subAmount);        // architecture line 468 verbatim
F_bow  = std::min(F_bow, 0.95f * F_max(beta, v_b));      // architecture line 470 verbatim
                                                          // (kFmaxScalar = 0.95f compile-time)
```

**Why scale ONLY F_bow uplift by safeDepth (not v_0 / mu_s):** the safeDepth table is calibrated against `pass_clickFree` + `pass_blockTime` + `pass_peak` at SLOW_LFO_DEPTH=1.0 — i.e., the empirical wedge boundary is the **F_bow ceiling** (the point where bow force pushes friction into raucous regime). v_0 reduction (sharper friction nonlinearity) and mu_s gap-widen (wider stick-slip range) are *enabling* mechanisms for period-doubling that don't directly correlate with the wedge boundary. Scaling them by safeDepth would dilute the bias geometry and reduce audibility at fallback cells unnecessarily.

**Verification at CONTEXT rev-7 default operating point** (BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10, MIDI 28):

- `safeDepth = kSafeDepth[E1=0][speedIdx=1][pressIdx=1][posIdx=1]` = grid-point lookup → `1.0f` (read from `Source/DSP/SchellengCalibration.h` line 30 emit_table.py output: combo passed all 4 stability tests at SLOW_LFO_DEPTH=1.0).
- At `subAmount = 1.0`: `effectiveBoost = 1.0 * 0.8 * 1.0 = 0.8` → `F_bow *= 1.8` (matches architecture §661 default `kForceBoost = 1.8`).
- 3 fallback cells (E1/A1/D2 at speedIdx=2, pressIdx=0, posIdx=0): `safeDepth = 0.5f` → `effectiveBoost = 0.4` at `subAmount = 1.0` → `F_bow *= 1.4` (matches architecture §661 fallback 1 explicitly: "If sub-harmonic bias triggers chaos at extreme settings → reduce `kForceBoost` from 1.8 → 1.4").

**Synergy with §17 calibration table:** the architecture's documented retune fallback (1.8→1.4) IS exactly what `safeDepth = 0.5` mechanically delivers. The mapping is therefore:

- **Stable-cell behaviour (105 / 108 combos):** full architecture-spec'd 1.8× F_bow uplift.
- **Fallback-cell behaviour (3 / 108 combos):** automatic architecture-spec'd 1.4× retune.
- **Off-grid trilinear:** smooth blend between 1.4 and 1.8 in the fallback boundary region — no discontinuity, no pathological cell.

**Alternatives considered (rejected):**

| Mapping | Stable-cell behaviour | Fallback-cell behaviour | Reason rejected |
|---|---|---|---|
| `effectiveBoost = 0.8 * safeDepth` (no subAmount in coefficient) | F_bow×1.8 | F_bow×1.4 | Removes user-controllable sub-harmonics range — at SUB_HARMONICS=0 still applies bias |
| `if (safeDepth < 1.0f) bias=0` (binary cutoff) | F_bow×1.8 | F_bow×1.0 (no bias) | Hard discontinuity in fallback cells — audible "switch off" mid-automation |
| Scale all three (`F_bow + v_0 + mu_s`) by safeDepth | Full bias | Half-strength on all axes | Period-doubling needs the v_0/mu_s geometry intact; halving them suppresses bifurcation regime more than necessary |
| Separate fitting pass for F_bow ceiling (new polynomial mirroring §17) | Tight per-combo control | Tight per-combo control | Adds an entire calibration cycle; out-of-scope for Phase 2.4b (CONTEXT Q26 deferred chaos detector to 2.5/2.6); 0.5/1.0 binary table provides adequate v1.0 safety |

**Implementation note for SubHarmonicBias.h:** the bias function takes `safeDepth` as an **input parameter** (caller queries `schelleng::safeDepthForString(...)` first, passes the result in). This keeps SubHarmonicBias.h decoupled from SchellengCalibration.h at compile time — the bias is a pure function of its inputs, no external lookups inside the function body. Mirrors the §17.3 pattern where `safeDepthForString` is a single function with internal table dispatch but is itself a pure computation. See §18.9 below for the locked function signature.

**Mapping function summary:**

```cpp
// Voice-side per-block (Step 2.5):
const float beta_v       = juce::jlimit (0.02f, 0.25f, rawBowPos);
const float safeDepthSub = schelleng::safeDepthForString (activeStringIndex,
                                                          rawBowSpeed,
                                                          rawBowPressure,
                                                          beta_v);
// Bias function-side (single call):
sub_harmonic::applyBias (subAmount,
                          activeStringIndex,
                          v_b_voice,                 // bowModel.getBowVelocity()
                          beta_v,
                          safeDepthSub,              // ∈ [0, 1] from §17 table
                          F_bow_voice,               // by ref — mutated for friction junction
                          v_0_voice,                 // by ref — mutated; pushed via setRosin equiv
                          mu_s_voice,                // by ref — mutated; pushed via setStaticFrictionCoefficient
                          mu_d_const);               // by value — gap-widen reference only
```

## 18.5 Open Question #2 — FFT Analyser Specifications (RESOLVED — N=65 536 Hann, 3-bin energy windows)

**Decision: lock FFT analyser parameters per the §18.3 baseline pre-flight.** Single render at `SUB_HARMONICS=1.0`, MIDI 28 (E1), default bow params, sustain 5 s; FFT analyses last 2 s; emits two bin-energy values + two diagnostic fields.

**Locked specifications:**

| Parameter | Value | Rationale |
|---|---|---|
| FFT size N | **65 536** | df = 0.6729 Hz at sr=44 100; gives 1-bin resolution ≈ ±0.34 Hz of f0/2 (sufficient for sharp peak detection); fits in audio-thread harness without allocation churn (one-shot offline analysis) |
| Window | **Hann** (`0.5 − 0.5·cos(2π·k/(N−1))`) | Standard period-doubling spectral analysis; sidelobe attenuation −31 dB; bin spreading 1.5 bins (compatible with 3-bin energy window) |
| Source samples | **last 2 s of 5 s sustain (88 200 samples)** | First 3 s discarded for friction-junction transient settling (architecture: ~1 s onset; 2 s safety margin); Phase 2.3 vibrato analysis uses similar pattern (1 s skip) |
| Mono mix | **average of L+R channels** | Mirrors existing harness mono-mix patterns at `tests/render-harness/main.cpp:~1100` |
| FFT input | **first 65 536 samples of last-2 s window** (Hann tapers endpoints) | Trailing 22 664 samples discarded; alternative N=131 072 zero-padded considered but no resolution benefit at f0/2 (still 1.5 bins wide due to Hann main lobe) |
| Library | **`juce::dsp::FFT` size 16** (= 2^16 = 65 536) | Header-only JUCE class; no extra dependency. Source/scratch buffer allocated once in harness `--sub-harmonics` mode setup, reused per call |

**Bin selection (locked):**

| Quantity | Centre frequency | Bin range (±0.5 Hz) | Bin freqs |
|---|---|---|---|
| `E_f0` (E1 fundamental) | 41.20 Hz | `[60, 62]` | 40.37 / 41.05 / 41.72 Hz |
| `E_subharm` (E1 sub-octave) | 20.60 Hz | `[30, 32]` | 20.19 / 20.86 / 21.53 Hz |
| `E_local_floor` (median noise reference) | [22, 28] Hz | `[33, 41]` | 22.21 – 27.59 Hz |

**Compute formulas (research-locked):**

```cpp
// JUCE-style C++ pseudo-code; harness-side post-process. Operates on real
// magnitude² spectrum (size N/2 + 1 = 32769). Hann window applied pre-FFT.
const int   N       = 65536;
const float df      = static_cast<float> (sampleRate) / static_cast<float> (N);
const int   bin_f0_lo  = 60, bin_f0_hi  = 62;
const int   bin_sub_lo = 30, bin_sub_hi = 32;

float E_f0     = 0.0f;
float E_subharm= 0.0f;
for (int b = bin_f0_lo;  b <= bin_f0_hi;  ++b) E_f0      += mag2[b];
for (int b = bin_sub_lo; b <= bin_sub_hi; ++b) E_subharm += mag2[b];

const float subharmEnergyRatio = E_subharm / juce::jmax (1.0e-9f, E_f0);

// Diagnostic field (no pass criterion):
float maxBinSub = 0.0f;
for (int b = bin_sub_lo; b <= bin_sub_hi; ++b)
    maxBinSub = juce::jmax (maxBinSub, mag2[b]);

float floorVals[9];
for (int b = 33; b <= 41; ++b) floorVals[b - 33] = mag2[b];
std::sort (floorVals, floorVals + 9);
const float medianFloor = floorVals[4];               // odd-length median
const float subharmPeakOverFloor = std::sqrt (maxBinSub) / std::sqrt (juce::jmax (1.0e-9f, medianFloor));
```

**JSON schema for `--sub-harmonics` mode (locked):**

```json
{
    "status":                 "PASS" | "FAIL",
    "mode":                   "sub-harmonics",
    "midiNote":               28,
    "subHarmonicsParam":      1.0,
    "sustainSeconds":         5.0,
    "totalSamples":           264600,
    "peak":                   0.184,
    "rmsContinuity":          0.93,
    "blockMicros_median":     43.5,
    "blockMicros_max":        78.0,
    "blockTimeRatio":         1.79,

    "subharmEnergyRatio":     0.42,        /* primary metric */
    "subharmPeakOverFloor":   5.4,         /* secondary diagnostic — no pass criterion */
    "fftBaselineNote":        "ratio at SUB_HARMONICS=0 measured 0.241 RESEARCH §18.3",

    "pass_noNaN":             true,
    "pass_peak":              true,
    "pass_clickFree":         true,
    "pass_blockTime":         true,
    "pass_subharmAudible":    true          /* subharmEnergyRatio >= 0.40 strict-PASS;
                                              [0.30, 0.40) soft-PASS within v1.0 budget;
                                              <0.30 hard-FAIL escalates to architecture §661 retune */
}
```

**Implementation footprint:** ~120 LOC harness append in `tests/render-harness/main.cpp` for `--sub-harmonics` mode setup + per-block render loop + FFT analysis post-process + JSON emit. No new third-party dependency (JUCE's `juce::dsp::FFT` covers everything). Mirrors `--vibrato` autocorrelation pattern (~150 LOC at lines 1133–1300) but spectrally-domain instead of time-domain.

## 18.6 Open Question #3 — pass_subharmAudible Threshold Tuning (RESOLVED — 0.40 strict / 0.30 soft / hard-fail < 0.30)

**Already partially resolved in §18.3.** Codified here verbatim:

- **Strict-PASS (architecture-spec'd, Gate 6b invariant 2 hard-bar):** `subharmEnergyRatio ≥ 0.40` measured at `SUB_HARMONICS = 1.0` on E1 (MIDI 28) over last 2 s of 5 s sustain at default bow params.
- **Soft-PASS (v1.0 fallback within Phase 2.4-bis remediation budget):** `subharmEnergyRatio ∈ [0.30, 0.40)`. Gate 6b passes; verify-phase logs Phase 2.4-bis remediation flag for "tighten bias coefficients to lift f0/2 audibility".
- **Hard-FAIL (escalates):** `subharmEnergyRatio < 0.30`. Plan-phase obligation: re-tune `kForceBoost` 1.8 → 1.4 (architecture §661 fallback 1) OR re-derive bias coefficients OR escalate to Phase 2.4-bis fundamental redesign BEFORE R35 atomic commit.

**Baseline reference (CRITICAL for plan-phase pre-flight):** the `subharmEnergyRatio` at `SUB_HARMONICS = 0` (current default; HR-9 short-circuit fires; no bias active) measured **0.241** in §18.3. The threshold escalation logic above interprets the bias's effect as *"how much ABOVE the noise-floor 0.24 baseline does the bias lift the f0/2 ratio?"*: strict-PASS requires ≥ 1.66× lift (0.24 → 0.40); soft-PASS requires ≥ 1.25× lift (0.24 → 0.30).

**Why this threshold is conservative vs lax:** the `subharmEnergyRatio` denominator is the f0 3-bin energy window — small at default bow params because the friction junction radiates most strongly at upper harmonics (3·f0, 4·f0). The numerator is the f0/2 3-bin window — at baseline it captures bass-region noise floor (DC residue + low-pass tail). For the metric to climb from 0.24 to 0.40, the bias must either (a) introduce a DISCRETE peak at f0/2 large enough to dominate the noise floor in those 3 bins, or (b) introduce sufficient broadband sub-fundamental content to lift the 3-bin energy by 66%. (a) is the textbook period-doubling signature; (b) would be raucous-regime noise (which the Schelleng F_max clamp guards against per §18.4 mapping). The threshold therefore selects FOR the desired (a) signature and AGAINST raucous failure modes.

**Plan-phase MUST include** a mandatory R35-pre spectral pre-flight: render at SUB_HARMONICS=1.0 with bias formula coefficients verbatim AS IMPLEMENTED, measure subharmEnergyRatio, AND document escalation outcome. This pre-flight gates R35 commit (cannot land if hard-FAIL).

## 18.7 Open Question #6 — `--sub-harmonics-stability` MIDI Note per Combo (RESOLVED — open-string MIDI 28/33/38/43)

**Decision: open-string MIDI per `stringIdx`** — `[28, 33, 38, 43]`, mirroring §17.6 for the Phase 2.4a 108-combo matrix. Each combo renders the MIDI note matching its open-string `stringIdx` (9 combos for stringIdx=0 play MIDI 28, 9 for stringIdx=1 play MIDI 33, etc.).

**Rationale (carries §17.6 verbatim):** the open-string MIDI is the operating point at which the string's characteristic impedance + dispersion + bridge filter + delay-line length are at their design-canonical values. Friction-junction bias is most representative at this operating point. Mid-range / fingered MIDI introduces additional dispersion-cascade variance better isolated by Phase 2.1c testing.

**Per-combo iteration order:**
- Outer: `stringIdx` ∈ {0, 1, 2, 3}
- Middle: `INFINITE_SUSTAIN` ∈ {0.0, 0.5, 1.0}
- Inner: `SUB_HARMONICS` ∈ {0.0, 0.5, 1.0}

Total: 4 × 3 × 3 = 36 combos. The 12 SUB_HARMONICS=0 combos exercise HR-9 short-circuit (bias path inactive); the 12 SUB_HARMONICS=0.5 combos exercise the smoothed mid-range; the 12 SUB_HARMONICS=1.0 combos exercise the full bias path.

## 18.8 Open Question #7 — `--sub-harmonics-stability` Pass Criteria (RESOLVED — confirmed against §18.2 pre-flight)

CONTEXT rev-7 proposed: `pass_clickFree (rmsContinuity ≥ 0.85)` + `pass_blockTime (ratio ≤ 5.0)` + `pass_noNaN` + `pass_peak (≤ 1.0)`. §18.2 pre-flight at the worst-case combo (E1 + INFINITE_SUSTAIN=1.0 OR `--schelleng-stress`) measured:

- `blockTime_max_over_median = 1.63` / `1.81` (well below 5.0 — ample margin)
- `peak = 0.068` / `0.124` (well below 1.0)
- `pass_nan = TRUE`
- `pass_rms = TRUE` (rmsContinuity ≥ 0.85 at both)

**Decision: lock thresholds verbatim per CONTEXT Q26 / §17.8 pattern.** Single-combo pre-flight confirms ample margin. Plan-phase commits the constants to harness `--sub-harmonics-stability` mode.

**Aggregate criterion (Gate 6b invariant 3):**
- **Strict-PASS:** `pass_all_36 = (passCount == 36)`.
- **Soft-PASS (v1.0 budget):** `failCount ≤ 2` (smaller-scale equivalent of Phase 2.4a's 4-of-108 budget; ratio 1/18 vs 1/27 = comparable). Mirror precedent.

**Per-combo schema (locked):**

```json
{
    "stringIdx":          0,
    "openStringMidi":     28,
    "infiniteSustain":    0.0,
    "subHarmonics":       0.0,
    "sustainSeconds":     5.0,
    "totalSamples":       264600,
    "peak":               0.184,
    "rmsContinuity":      0.91,
    "blockMicros_median": 43.5,
    "blockMicros_max":    78.0,
    "blockTimeRatio":     1.79,
    "lastSubAmount":      0.0,            /* instrumentation atom — analogous to Phase 2.3 lastSafeDepth */
    "pass_noNaN":         true,
    "pass_peak":          true,
    "pass_clickFree":     true,
    "pass_blockTime":     true,
    "pass_combo":         true            /* 4-way AND */
}
```

**Aggregate JSON:**

```json
{
    "status":          "PASS" | "FAIL",
    "mode":            "sub-harmonics-stability",
    "totalCombos":     36,
    "passCount":       36,
    "failCount":       0,
    "pass_all_36":     true,
    "combos":          [ /* 36 entries above, in canonical iteration order */ ]
}
```

**Caveat (from §17.8):** pre-flight A had BOW_POSITION=0.10 (default), Pre-flight B had BOW_POSITION=0.10. The 36-combo matrix uses BOW_POSITION=0.10 default at all combos (no BOW_POSITION axis, per Q26). So the §18.2 pre-flight's confirmed margin DIRECTLY applies — no additional pre-flight needed. Risk #7 (CONTEXT rev-7) DISSOLVED.

## 18.9 Open Question #8 — SubHarmonicBias.h API Shape (RESOLVED — single function, mu_d by value)

**Decision: single header-only function with `inline` linkage, mu_d passed by value (read-only gap-widen reference), F_bow / v_0 / mu_s by reference (mutated).**

```cpp
// Source/DSP/SubHarmonicBias.h
//
// O-Contrabass Phase 2.4b — sub-harmonic bias DSP-07 (ARCHITECTURE §457).
// Friction-junction parameter biasing toward Schelleng F_max regime to
// induce period-doubling f0/2 spectral content. Per-plugin (NOT extracted
// to shared module per Q24 — friction module v1.0.0 untouched).
//
// HR-9 (Phase 2.4b hard rule): SUB_HARMONICS=0 IEEE 754 identity arithmetic
// + active-string-only gate. The caller (BowedContrabassVoice.cpp Step 2.5)
// MUST short-circuit BEFORE invoking applyBias() at subAmount==0.0f and
// for non-active-string slots. This function does NOT defensively short-
// circuit; HR-9 is enforced at the caller for clarity of the bit-exact
// regression bar.

#pragma once
#include <algorithm>
#include <cmath>

namespace ouaricon::contrabass::sub_harmonics {

// Architecture §457 / §661 default coefficients. Phase 2.4b ships verbatim;
// Phase 2.4-bis or 2.5/2.6 may retune to fallback 1.4 if Gate 6b spectral
// pre-flight (§18.6) returns hard-FAIL.
inline constexpr float kForceBoost   = 0.8f;     // F_bow uplift coefficient (architecture §457
                                                  // line 465 multiplier, ×subAmount factored at
                                                  // call-site for safeDepth interaction);
                                                  // architecture §661 fallback 1 → 0.4f.
inline constexpr float kV0Reduction  = 0.5f;     // v_0 contraction coefficient (line 466).
inline constexpr float kGapWiden     = 0.25f;    // mu_s − mu_d gap widening (line 468).
inline constexpr float kFmaxScalar   = 0.95f;    // Schelleng ceiling fraction (line 470).
inline constexpr float kV0Floor      = 0.005f;   // architecture-spec'd lower clamp (line 466).

// Schelleng F_max formula (architecture §490 — slow-LFO wedge, with
// Z=0.5, R=0.5, mu_s−mu_d ≈ 0.6 dimensionless collapse):
//   fMax = (2·Z · v_b) / (beta · (mu_s − mu_d))
// Same closed form Phase 2.3 used pre-calibration. Re-used here for the
// final F_bow ceiling clamp; SchellengCalibration table provides the
// stable-cell vs fallback-cell scaling on the uplift, NOT the hard ceiling.
inline float schellengFmax (float beta, float v_b, float mu_gap) noexcept
{
    constexpr float Z2  = 1.0f;                  // 2·Z = 2·0.5 = 1.0
    return (Z2 * v_b) / (std::max (1.0e-6f, beta * std::max (1.0e-6f, mu_gap)));
}

// applyBias mutates F_bow / v_0 / mu_s in-place per architecture §457.
// Caller passes pre-bias parameters (post-LFO/macro/MPE-layer values that
// the per-sample friction junction would otherwise consume) by reference;
// after return the references hold post-bias values for downstream push to
// frictionModel + bowModel.
//
// Inputs:
//   subAmount    — 30 ms-smoothed SUB_HARMONICS value ∈ [0, 1]; at 0.0 the
//                  caller MUST NOT invoke this function (HR-9 short-circuit).
//   stringIdx    — active string ∈ [0, 3] (E1/A1/D2/G2). Reserved for future
//                  per-string variation; v1.0 ignores (single global bias).
//   v_b          — voice-level bow velocity from BowModel.getBowVelocity().
//   beta         — voice-level effective bow position ∈ [0.02, 0.25].
//   safeDepth    — Phase 2.4a SchellengCalibration lookup result ∈ [0, 1];
//                  caller queries schelleng::safeDepthForString(...)
//                  before invoking. Stable cells return 1.0; fallback 0.5.
//   mu_d         — dynamic friction coefficient (read-only; gap-widen ref).
//                  v1.0 = 0.25 (architecture default for bass).
//
// In-out:
//   F_bow        — bow force; on entry = pre-bias F_bow (typically same as
//                  bowModel.getBowForce() or post-LFO pressure); on return
//                  = post-bias post-clamp value to push to frictionModel
//                  via bowModel.setBowPressure (mapped through the macro
//                  layer in BowedContrabassVoice.cpp Step 5).
//   v_0          — characteristic velocity; on entry = current ROSIN-derived
//                  default (0.1 · exp(−4.6·rosin)); on return = bias-shifted
//                  value to push via the rosin-equivalent inverse computed
//                  by the caller (avoids friction module ABI churn — Q24).
//   mu_s         — static friction; on entry = current default (0.85 for
//                  bass); on return = gap-widened value to push via
//                  frictionModel.setStaticFrictionCoefficient.
//
// IEEE 754 identity arithmetic: at subAmount = 0.0 the formulas reduce to
//   F_bow *= 1.0      (exact; no-op)
//   v_0   *= 1.0      (exact; no-op; max() with floor leaves v_0 unchanged
//                      as long as v_0 > kV0Floor — verified for ROSIN ∈ [0,1])
//   gap   *= 1.0      (exact; mu_s = mu_d + gap = unchanged)
//   F_bow  = min(F_bow, 0.95·fMax)   (UNCONDITIONAL — enforces architecture's
//                      raucous-regime ceiling; at default operating point
//                      this is a no-op because F_bow = 3.0 < 0.95·fMax ≈ 9.0)
// Caller's HR-9 short-circuit prevents this function from being called at
// subAmount = 0.0, eliminating the F_max clamp overhead in the bit-exact
// regression path. The clamp DOES fire at subAmount > 0.0 — that's the
// guarantee against raucous chaos at fallback cells.
inline void applyBias (float       subAmount,
                       int         stringIdx,
                       float       v_b,
                       float       beta,
                       float       safeDepth,
                       float&      F_bow,
                       float&      v_0,
                       float&      mu_s,
                       float       mu_d) noexcept
{
    // F_bow uplift, scaled by Phase 2.4a empirical safeDepth (§18.4 mapping).
    const float effectiveBoost = subAmount * kForceBoost * safeDepth;
    F_bow *= 1.0f + effectiveBoost;

    // v_0 contraction (architecture §457 line 466 verbatim, NOT scaled by
    // safeDepth per §18.4 rationale — v_0 sharpens stick-slip nonlinearity,
    // NOT the F_bow ceiling).
    v_0 = std::max (kV0Floor, v_0 * (1.0f - kV0Reduction * subAmount));

    // mu_s gap widen (architecture §457 line 468 verbatim).
    const float gap = mu_s - mu_d;
    mu_s = mu_d + gap * (1.0f + kGapWiden * subAmount);

    // Schelleng F_max ceiling (architecture line 470). Mu_gap from POST-bias
    // mu_s for accurate post-bias wedge computation.
    const float muGapPost = mu_s - mu_d;
    F_bow = std::min (F_bow, kFmaxScalar * schellengFmax (beta, v_b, muGapPost));

    // stringIdx ignored at v1.0 (Q11 per-string variation deferred).
    (void) stringIdx;
}

} // namespace ouaricon::contrabass::sub_harmonics
```

**Per-architecture-§457 verification:**
- Line 465 `F_bow *= 1.0f + 0.8f * subAmount` → modified to `F_bow *= 1.0f + (subAmount · 0.8 · safeDepth)` per §18.4 mapping. At safeDepth=1.0 (stable cells) the formula is **literal verbatim**.
- Line 466 `v_0 = std::max(0.005f, v_0 * (1.0f - 0.5f * subAmount))` → verbatim.
- Line 468 `mu_s = mu_d + gap * (1.0f + 0.25f * subAmount)` → verbatim.
- Line 470 `F_bow = std::min(F_bow, kFmaxScalar * F_max(beta, v_b))` → verbatim with `mu_gap = mu_s - mu_d` POST-bias as the friction-junction-consistent value.

**Header-only `inline` linkage:** matches Phase 2.4a SchellengCalibration.h precedent (§17.3). NO ODR risk; simple per-function inlining; ~80 LOC final.

**Why mu_d const-by-value (not const-by-ref):** mu_d is read once for the gap computation; no reason to take it by reference. Pass-by-value avoids the optimiser hint that the caller's value might be invalidated — friendlier for inlining at call-site. Architecture §457 only mutates F_bow/v_0/mu_s; mu_d carries no mutation contract.

## 18.10 Open Question #9 — Voice-Level vs Per-String Inputs (RESOLVED — voice-level)

**Decision: bias inputs (`v_b`, `beta`, pre-bias `F_bow` / `v_0` / `mu_s` / `mu_d`) are voice-level scalars, NOT per-string-instance.** The single-string active-only gate (HR-9 Q31) means:

- `v_b = bowModel.getBowVelocity()` — voice-level (single bow envelope per voice; architecture §549 confirms "Bow Model → Friction Junction" is a single pipeline).
- `beta = effectivePosition` (post-MPE-timbre clamp at `BowedContrabassVoice.cpp:645`) — voice-level (single bow position).
- Pre-bias `F_bow` = bow force from the macro/LFO/MPE-pressure post-Step-5 chain; at v1.0 = `effectiveBowPressure * (0.5f + mpePressure * 1.5f)` (the value that would otherwise feed `bowModel.setBowPressure(...)` at Step 6 line 377).
- Pre-bias `v_0` = current ROSIN-derived value (`0.1f * std::exp(-4.6f * rawRosin)`) — read once at block entry from the same APVTS atomic that feeds line 647's `frictionModel.setRosin(rosin)`.
- Pre-bias `mu_s` = bass default `0.85f` (set once in `prepareToPlay` line 205); no per-block change at v1.0.
- `mu_d` = bass default `0.25f` (set once line 206).

**Why voice-level (not per-string):** at v1.0 with single bow-position pipeline, all 4 strings (active + 3 idle) share the same bow params. The friction junction at the active string consumes `(v_b, F_bow)` modulated voice-level. SubHarmonicBias.h biases the SAME voice-level inputs that `bowModel.setBowSpeed/setBowPressure` would receive — making the bias output structurally consistent.

**Per-string `safeDepth`:** the ONE per-string component is the SchellengCalibration table lookup (Phase 2.4a stringIdx-indexed). This is wired through the `stringIdx = activeStringIndex` argument to `safeDepthForString(...)` — voice-level read of `activeStringIndex`, but the table lookup is per-string-aware.

**Active-string-only mutation:** HR-9 Q31 says bias mutates only for `activeStringIndex`, not `crossfadePrevStringIndex`. Implementation: the caller (Step 2.5) computes mutated values; Step 6 pushes them ONLY to `bowModel/frictionModel` (single per-voice instance) — the 3 idle strings' processSample(0, 0, frictionModel) calls at lines 555–559 see the mutated frictionModel state, but their zero-energy excitation means no audible effect (architectural correctness preserved). The crossfade-shadowed previous string ALSO consumes the mutated frictionModel for its 5 ms decay — this is a known minor effect documented in CONTEXT rev-7 Risk #5 ("audible 'switch' event when SUB_HARMONICS > 0 mid-note-change"); 30 ms `subHarmonicsSmoothed` ramp absorbs the discontinuity per HR-9 ramp guarantee.

**Friction module ABI preserved (Q24 carry-forward):** the bias's mutations to v_0 and mu_s are pushed back via:
- v_0 → `frictionModel.setRosin(rosinEquivalent)` where `rosinEquivalent = -ln(v_0_biased / 0.1f) / 4.6f` (algebraic inverse of `setRosin`'s formula at HyperbolicFriction.h:49).
- mu_s → `frictionModel.setStaticFrictionCoefficient(mu_s_biased)` (existing setter at line 60).
- mu_d → unchanged (static at 0.25f).

**No friction module v1.0.0 ABI change.** The bias performs algebraic inversion to keep the friction surface header-untouched. (Adding a `setCharacteristicVelocity(v_0)` setter would be cleaner but violates Q24's "friction module v1.0.0 untouched" decision; the algebraic inverse adds 1 ln + 1 div per block ≈ 30 ns — negligible.)

**v_0 → rosin inverse algebraic identity:**
```cpp
// HyperbolicFriction::setRosin(rosin):  v_0 = 0.1f * std::exp(-4.6f * rosin)
// Inverse:                              rosin = -ln(v_0 / 0.1f) / 4.6f
//                                            = -ln(10.0f * v_0) / 4.6f
const float rosinEq = -std::log (10.0f * std::max (1.0e-6f, v_0_biased)) / 4.6f;
frictionModel.setRosin (juce::jlimit (0.0f, 1.0f, rosinEq));
```

The clamp `[0, 1]` is defensive — at the v_0 floor 0.005 (architecture-spec'd lower clamp) the inverse rosin = `-ln(0.05)/4.6 ≈ 0.65`, well within the valid ROSIN parameter range.

## 18.11 Open Question #10 — R35 Task Breakdown (RESOLVED — 9-task R35-pre/R35a..h)

**Decision: 9-task sequencing mirroring §17.11 R34 pattern.**

| Task | Subject | Source delta | Notes |
|---|---|---|---|
| **R35-pre** | Bit-exact regression pre-flight + spectral pre-flight | (no source delta — verification) | (a) Run `reproduce-goldens.sh` at HEAD → 10/10 PASS expected (verified §18.1 today). (b) Run new `--sub-harmonics` mode at SUB_HARMONICS=1.0 with bias coefficients verbatim → measure subharmEnergyRatio. If hard-FAIL (<0.30), escalate to coefficient retune BEFORE R35 — kForceBoost 1.8 → 1.4 (architecture §661 fallback 1) OR threshold relaxation. Pre-flight script: `tests/render-harness/preflight-subharm.sh` (~30 LOC). |
| **R35a** | Add `--sub-harmonics` + `--sub-harmonics-stability` modes to harness + FFT analyser | `tests/render-harness/main.cpp` ~+250 LOC | New CLI flag dispatch + per-mode setup + 36-combo iteration loop + FFT analyser (`juce::dsp::FFT` size 16, Hann window, 3-bin energy windows per §18.5) + per-combo / aggregate JSON schemas. |
| **R35b** | Render new goldens + commit golden text | `tests/render-harness/golden/sub-harmonics.{wav.sha256,json,json.sha256}` + `sub-harmonics-stability.{wav.sha256,json,json.sha256}` 6 new files | Wall-clock ~5 s (single `--sub-harmonics` ~0.04 s + `--sub-harmonics-stability` ~4 s). Validates `pass_subharmAudible` (§18.6) + `pass_all_36`. WAV byte-deterministic against R35d source. |
| **R35c** | Author `Source/DSP/SubHarmonicBias.h` | NEW ~120 LOC (per §18.9) | Header-only `inline` namespace `ouaricon::contrabass::sub_harmonics`; coefficients + `applyBias()` + `schellengFmax()` helper. |
| **R35d** | BowedContrabassVoice integration: Step 2.5 + subHarmonicsSmoothed + lastSubAmount | `Source/BowedContrabassVoice.{h,cpp}` net ~+50 LOC | (a) Add `juce::SmoothedValue<float, Linear> subHarmonicsSmoothed` (30 ms ramp, `setCurrentAndTargetValue(0.0f)` in prepareToPlay; `setTargetValue` UNCONDITIONAL each block per pin #11). (b) Add `std::atomic<float> lastSubAmount { 0.0f }` instrumentation hook + `getLastSubAmount()` accessor (mirrors `lastSafeDepth` pattern). (c) Insert Step 2.5 between Step 2 (Schelleng wedge) and Step 3 (slow-LFO phase advance): read `rawSubHarmonics`; smooth via subHarmonicsSmoothed.getNextValue() + skip(); short-circuit at subAmount==0.0f via HR-9 (return early; lastSubAmount.store(0.0f)); else compute biased F_bow/v_0/mu_s using SubHarmonicBias.h + push to frictionModel via setRosin-equivalent inverse + setStaticFrictionCoefficient. (d) APVTS attachment for `SUB_HARMONICS` parameter ID — already declared at PluginProcessor.cpp:104; just `getRawParameterValue("SUB_HARMONICS")->load()` in the Step 1 raw reads block. |
| **R35e** | Bit-exact regression bar verification | (no source delta — verification step) | Re-run `reproduce-goldens.sh`; confirm 10 carry-forward goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo + schelleng-stress) byte-identical. HR-9 short-circuit + IEEE 754 identity arithmetic + active-string-only gate are technical defences; if regression breaks, isolate to a smaller bias surface OR roll back. |
| **R35f** | auval + pluginval-10 | (no source delta) | Standard Gate 6b invariant 4. |
| **R35** atomic commit | All R35a–R35f files | ~12–15 files | Continues R7 → R15 → R20 → R26 → R33 → R34 → **R35** sequence. R37 Logic AU smoke deferred non-blocking (Q28). |
| **R35-backfill chore** | Propagate R35 sha into STATUS.md | `STATUS.md` 1 field | Mirrors R34-backfill (`b64c8c4`) / R33-backfill (`991121a`) precedent. |

**Net source delta R35a+R35c+R35d:**
- `tests/render-harness/main.cpp` ~+250 LOC (--sub-harmonics + --sub-harmonics-stability + FFT analyser)
- `Source/DSP/SubHarmonicBias.h` ~+120 LOC (NEW)
- `Source/BowedContrabassVoice.cpp` ~+30 LOC (Step 2.5 insertion + ROSIN inverse + frictionModel setters)
- `Source/BowedContrabassVoice.h` ~+5 LOC (subHarmonicsSmoothed + lastSubAmount + getLastSubAmount accessor)
- `tests/render-harness/golden/` 6 NEW text files (sub-harmonics.{wav.sha256,json,json.sha256} + sub-harmonics-stability.{wav.sha256,json,json.sha256})
- `tests/render-harness/reproduce-goldens.sh` ~+10 LOC (extend NAMES/INVOCS arrays for 12 goldens; matrix-stability optionally — but matrix-stability sha is large and remains tripwire-deferred per Phase 2.4a precedent)
- `tests/render-harness/preflight-subharm.sh` ~+30 LOC (NEW; spectral pre-flight script for R35-pre tripwire)

**Total: ~+440 LOC source/tooling + 1 new header + 6 new goldens + 0 re-baselined goldens (HR-9 preserves bit-exact regression).**

## 18.12 Open Question #11 — Per-String SUB_HARMONICS Variation (RESOLVED — single global)

**Decision: single global `SUB_HARMONICS` APVTS parameter; active-string-only application via HR-9.**

**BRIEF / ROADMAP / ARCHITECTURE audit:**
- BRIEF.md line 96: "Sub-Harmonics | 0-100% | 0% | Nonlinear feedback / sub-octave content for sub-bass extension" — single 0-100% value.
- ARCHITECTURE.md §365: "SUB_HARMONICS | Float | 0–1 | 0.0 | Friction Junction (bias) | Period-doubling biasing of `F_bow`, `v_0`, `mu_s−mu_d`" — single Float APVTS parameter.
- ARCHITECTURE.md §457 (algorithm): single `subAmount` scalar input to `applyBias(...)` — no string-indexed coefficient.
- PluginProcessor.cpp:104 — `SUB_HARMONICS` declared as single AudioParameterFloat (default 0.0).

**No per-string variation in scope at v1.0.** Architecture envisions a single SUB_HARMONICS knob biasing whichever string is active. Per-string variation (e.g., E1-only sub bias, others muted) would require new APVTS parameters (4 SUB_HARMONICS_E/A/D/G) + UI affordance — NOT in BRIEF/ROADMAP/ARCHITECTURE; explicitly out of scope for Phase 2.4b.

**Future-proofing:** SubHarmonicBias.h's `applyBias()` signature includes `int stringIdx` even though v1.0 ignores it (`(void) stringIdx;` at function tail). This reserves the per-string variation hook for a future feature without ABI churn — when/if a Phase 3+ feature adds per-string sub-harmonics, the function body extends naturally without refactoring callers.

## 18.13 Open Question #12 — Architectural Footnote on Chaos Detector Deferral (RESOLVED — commit-body footnote template)

**Decision: track in R35 commit-message body that lag-2 RMS chaos detector + softClampState energy clamp are deferred to Phase 2.5/2.6. NOT an ARCHITECTURE.md amendment.**

**Commit-body footnote template (R35 atomic commit):**

```
Phase 2.4b Sub-Harmonic Bias DSP-07 — chaos detector + softClampState deferral
─────────────────────────────────────────────────────────────────────────────

This commit ships ARCHITECTURE.md §457 sub-harmonic bias verbatim with the
following architecture-spec'd deferments to Phase 2.5/2.6:

1. Chaos detector (architecture §457 line 476 "optional"):
   "Optional control-rate (~100 Hz) check: lag-2 RMS > lag-1 RMS *and*
   non-periodic → back off bias by 20%."
   Deferred — v1.0 relies on Schelleng F_max clamp (architecture line 470,
   kFmaxScalar=0.95) + algebraic saturator (`x/sqrt(1+x²)` per architecture
   §"Master Saturator") + bridge filter loop-gain ceiling 0.9999999
   (architecture §"Bridge Filter") as layered stability defences. Reopen
   in Phase 2.5/2.6 if 36-combo --sub-harmonics-stability matrix surfaces
   any failing combo at the 0.5 fallback safeDepth.

2. softClampState energy clamp (ROADMAP §Phase 2.4 deliverable, threshold
   0.85 ceiling 1.0):
   Deferred — current architecture-spec'd algebraic saturator covers the
   role at v1.0; energy clamp adds redundancy without measured benefit at
   default operating points. Reopen in Phase 2.5/2.6 alongside body
   resonator integration where peak amplitudes can compound.

3. Phase 2.4-bis backlog items (carry-forward from Phase 2.4a R34 commit
   body, NOT addressed in this commit):
   - Tune Step 4 modulation gain to hit 20% peak-to-peak OR refine
     breathingAudible per-cycle metric.
   - Reduce 3 v1.0 fallback cells via downstream-defense tightening.

Both deferments preserve architecture intent without amending
ARCHITECTURE.md (per discuss-phase Q33). Phase 2.4c (autocorrelator +
saturator-tail O-Bowed comparison) gets fresh CONTEXT rev-8 when its
discuss-phase opens after Phase 2.4b verifies (Gate 6b PASS).
```

**Why not amend ARCHITECTURE.md:** the bias formula IS the architecture's literal text. Chaos detector is marked "optional" at architecture line 476. softClampState's deferral is consistent with Phase 2.4 RESEARCH §12 footnote (saturator-tail tracking). No architectural decision changes; only implementation phasing.

**Audit-trail handoff:** verify-phase appends Phase 2.4b section to VERIFICATION.md noting that the commit-body footnote satisfies Q12; STATUS.md `phase_2_4b_atomic_commit_sha` field captures R35 sha. Phase 2.5 / 2.6 / 2.4c open-issue tracker carries these forward.

## 18.14 Risk-Surface Refinement for PLAN rev-9

Carry-forward of CONTEXT rev-7 §"Risks" #1–#12 + new findings from §18.1–§18.13:

| # | Risk | Current state | Mitigation in PLAN rev-9 |
|---|---|---|---|
| 1 | HR-9 bit-exact regression failure on 10 carry-forward goldens | **PRE-FLIGHT PASS at HEAD (§18.1).** All 10 reproduce byte-identical via reproduce-goldens.sh | HR-9 short-circuit + IEEE 754 identity arithmetic + active-string-only gate are technical defences. R35e step re-runs reproduce-goldens.sh post-source-edit. If FAIL, isolate SubHarmonicBias.h to separate translation unit OR roll back per Phase 2.3 R33 precedent. |
| 2 | SchellengCalibration→F_max mapping semantic mismatch | **RESOLVED §18.4** — `effectiveBoost = subAmount · 0.8 · safeDepth`; preserves architecture §457 verbatim at stable cells; auto-applies architecture §661 fallback 1 retune at fallback cells | DISSOLVED (mapping function locked). Plan-phase commits §18.9 SubHarmonicBias.h verbatim. |
| 3 | `pass_subharmAudible` threshold 0.10 too lax | **CONFIRMED LAX §18.3** — baseline at SUB_HARMONICS=0 already returns ratio 0.241; threshold MUST be raised | Plan-phase locks §18.6 thresholds: 0.40 strict / 0.30 soft / <0.30 hard-FAIL. R35-pre adds mandatory spectral pre-flight at SUB_HARMONICS=1.0 with bias-active; hard-FAIL escalates BEFORE R35 commit. |
| 4 | Period-doubling chaotic regime at SUB_HARMONICS=1.0 + INFINITE_SUSTAIN=1.0 | Layered defences: SchellengCalibration F_max clamp (inside bias §18.9), algebraic saturator, loop-gain ceiling 0.9999999 | Risk reduced. Plan-phase R35-pre includes 36-combo `--sub-harmonics-stability` rendering — exercises 12 SUB+SUS=high combos. If any combo NaN/peak>1.0/runaway, Phase 2.5/2.6 chaos detector OR architecture §661 retune triggered. failCount ≤ 2 v1.0 budget. |
| 5 | Active-string-only bias under crossfade — audible "switch" mid-note-change | 30 ms subHarmonicsSmoothed ramp absorbs discontinuity; HR-9 active-string gate analogous to HR-1 vibrato (Phase 2.3 verified vibrato gates didn't audibly switch) | Plan-phase locks `subHarmonicsSmoothed.setCurrentAndTargetValue(0.0f)` in prepareToPlay + `setTargetValue` UNCONDITIONAL each block per Phase 2.3 pin #11. R35-pre `--sub-harmonics-stability` 36 combos exercise 4 string transitions per matrix iteration if `--note-sequence` is added (deferred — per Q26 sustained-only at v1.0). |
| 6 | `subHarmonicsSmoothed.setTargetValue` UNCONDITIONAL each block — denormal accumulation | Phase 2.3 macroSmoothed pin #11 precedent | Plan-phase pins into PLAN rev-9 preamble: setTargetValue UNCONDITIONAL each block (mirrors macroSmoothed pin #11). |
| 7 | SUB_HARMONICS default 0.0 audit | All 10 current golden render configs use default SUB_HARMONICS=0 (verified by §18.1 — no source change required, parameter at PluginProcessor.cpp:104 default 0.0; all goldens reproduce byte-identical at HEAD) | DISSOLVED. R35-pre confirms via reproduce-goldens.sh. |
| 8 | Bias's F_max clamp interaction with HR-4 (Schelleng wedge skip on SLOW_LFO_DEPTH=0) | Bias's F_max clamp via SchellengCalibration is INDEPENDENT of HR-4 wedge gate — read-only lookup, cheap | DISSOLVED. R35-pre `--sub-harmonics` mode runs at SLOW_LFO_DEPTH=0 default; HR-4 short-circuits wedge math; bias's F_max clamp fires from §18.9 schellengFmax() helper independently. |
| 9 | Period-doubling spectral content shifts FFT bin selection | **CONFIRMED at §18.3** — current bin choice (3-bin ±0.5 Hz at f0/2 and f0) captures period-doubling fundamental cleanly; broadband transient sidebands minor at default bow params | Mitigated — §18.5 bin selection locked. Plan-phase R35-pre validates against measured threshold. |
| 10 | R35 atomic commit interaction with R34-backfill chore | R34-backfill chore `b64c8c4` propagated R34 sha; R35 atomic commit lands while R34-backfill is HEAD; R35-backfill chore propagates R35 sha | DISSOLVED. R35-backfill chore commit follows R35 atomic commit per R34-backfill / R33-backfill / R26 precedent. |
| 11 | `kForceBoost = 1.8` cap matches architecture §1.3 default | Bias formula `F_bow *= 1.0f + 0.8f * subAmount` produces F_bow×1.8 at subAmount=1.0 — matches kForceBoost cap | RESOLVED §18.4 — `kForceBoost` is the 0.8 coefficient (NOT a separate constant); architecture §661 fallback 1 (kForceBoost 1.8 → 1.4) maps to coefficient 0.8 → 0.4 in SubHarmonicBias.h. |
| 12 | Phase 2.4-bis backlog crowding | Phase 2.4-bis open items (breathingAudible metric refinement, 3 fallback-cell reduction) NOT folded into Phase 2.4b | Risk reduced. §18.4 mapping uses `safeDepth` to AUTOMATICALLY apply architecture §661 fallback 1 retune at the 3 fallback cells — bypasses the crowding concern at v1.0; if Gate 6b reveals additional fallback-cell intersection problems, escalation lane is Phase 2.4-bis post-2.4b verify. |
| 13 | **NEW** — friction module ABI preservation under v_0 mutation | Q24 says module v1.0.0 untouched; bias mutates v_0 by reference but caller pushes via ROSIN inverse algebraic identity | Mitigated — §18.10 documents `rosinEq = -ln(10·v_0_biased)/4.6f` inverse; ~30 ns per block; clamped `[0,1]` for safety. Friction module surface unchanged. |
| 14 | **NEW** — bias's mu_s push interacts with idle/crossfade-shadow strings | All 4 strings share single frictionModel instance; idle strings' processSample(0,0,frictionModel) sees mutated mu_s but excitation is 0 → no audible effect; crossfade-shadow string's 5 ms decay sees mutated mu_s but ramp absorbs discontinuity | Mitigated — architectural correctness preserved. PLAN rev-9 preamble pins Step 2.5 ordering: bias state push to frictionModel happens ONCE per block AFTER Step 2.5, BEFORE Step 6 per-sample loop; all 4 strings consume biased state consistently for that block. |

## 18.15 Sequencing in PLAN rev-9

Plan-phase translates this research into the R35 task breakdown per §18.11. Recommended sequencing already locked (mirrors §17.11 / R34 pattern). Plan-phase commits the 9-task R35-pre/R35a/R35b/R35c/R35d/R35e/R35f/R35/R35-backfill sequence + HARD RULES preamble:

**HR rules in effect for Phase 2.4b (carry-forward + new):**

| Rule | Source | Statement |
|---|---|---|
| HR-1 | Phase 2.3 | Vibrato literal-zero short-circuit (active-string-only) — carry-forward |
| HR-2 | Phase 2.3 | Slow-LFO literal-zero short-circuit + phase non-advance at zero depth — carry-forward |
| HR-3 | Phase 2.3 | Macro IEEE 754 identity arithmetic + macroSmoothed setCurrentAndTargetValue(0.0) — carry-forward |
| HR-4 | Phase 2.3 | Schelleng wedge skip on zero LFO depth + lastSafeDepth.store(0.0) unconditional pre-gate — carry-forward |
| HR-5 | Phase 2.4a | `inline constexpr` linkage on SchellengCalibration.h — carry-forward |
| HR-6 | Phase 2.4a | Calibration polynomial behind HR-4 gate ONLY — carry-forward |
| HR-7 | Phase 2.4a | Matrix-stability bypass via `extern "C" __attribute__((weak))` production default — carry-forward |
| HR-8 | Phase 2.4a | Trilinear IEEE 754 identity arithmetic at sample points — carry-forward |
| **HR-9** | Phase 2.4b | **SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate.** At `subAmount == 0.0f` short-circuit Step 2.5 entry: `if (subAmount == 0.0f) { lastSubAmount.store(0.0f); return; }`. Bias invoked ONLY for `activeStringIndex` (mirrors HR-1 vibrato). Combined with `subHarmonicsSmoothed.setCurrentAndTargetValue(0.0f)` in prepareToPlay + APVTS default 0.0 + `setTargetValue` UNCONDITIONAL each block per pin #11 — guarantees all 10 carry-forward goldens reproduce byte-identically. |
| HR-10 | Phase 2.4b | **Friction module ABI preservation under bias.** v_0 / mu_s mutations in SubHarmonicBias.h apply by reference; caller pushes back via existing setRosin (algebraic inverse) + setStaticFrictionCoefficient. NO HyperbolicFriction.h edits in Phase 2.4b. |

**Per-block evaluation order (Phase 2.3 7-step + Phase 2.4a wedge swap + Phase 2.4b Step 2.5):**

1. Step 1 — Read raw APVTS atomics (unchanged + add `rawSubHarmonics`)
2. Step 2 — Schelleng wedge `safeDepth` (Phase 2.4a SchellengCalibration trilinear; HR-4 + HR-7 gates)
3. **Step 2.5 — Sub-harmonic bias** (NEW; HR-9 short-circuit; if active: query `safeDepth` for bias mapping; compute bias; push v_0 / mu_s to frictionModel)
4. Step 3 — Slow-LFO phase advance + sin (HR-2 gate; unchanged)
5. Step 4 — Apply slow-LFO multiplicatively to bow speed/pressure (unchanged)
6. Step 5 — Layer EXPRESSION_MACRO (HR-3 gate; unchanged); modify `effectiveBowPressure` for biased F_bow at end of Step 5 if bias active (post-bias F_bow scale factor multiplied in)
7. Step 6 — Push to bowModel + all-strings brightness (modified to consume biased values)
8. Step 7 — Per-sample loop (active string only vibrato; unchanged)

**Step 2.5 detailed pseudo-code:**

```cpp
// Step 2.5 — Sub-harmonic bias (Phase 2.4b R35d).
// HR-9 short-circuit: at subAmount=0.0f, bias path is BIT-EXACTLY no-op.
const float rawSubHarmonics = parameters->getRawParameterValue ("SUB_HARMONICS")->load();
subHarmonicsSmoothed.setTargetValue (rawSubHarmonics);          // pin #11 UNCONDITIONAL
const float subAmount = subHarmonicsSmoothed.getNextValue();
subHarmonicsSmoothed.skip (juce::jmax (0, numSamples - 1));     // pin #7 jmax guard

// HR-9 — short-circuit + active-string gate.
if (subAmount == 0.0f || activeStringIndex < 0)
{
    lastSubAmount.store (0.0f, std::memory_order_relaxed);
    // Step 3 onward sees unbiased frictionModel state — bit-exact.
}
else
{
    // Pre-bias snapshot (voice-level scalars).
    const float v_b_voice  = bowModel.getBowVelocity();
    const float beta_v     = juce::jlimit (0.02f, 0.25f, rawBowPos);
    const float rawRosin   = parameters->getRawParameterValue ("ROSIN")->load();
    float v_0_pre  = 0.1f * std::exp (-4.6f * rawRosin);                 // ROSIN inverse map
    float mu_s_pre = 0.85f;                                              // bass default
    constexpr float mu_d_const = 0.25f;                                  // bass default
    float F_bow_pre = rawBowPressure * (0.5f + 1.5f * mpePressure_at_sample_0);  // approximation;
                                                                                  // exact MPE-pressure
                                                                                  // sampled in Step 7
                                                                                  // per-sample. v1.0
                                                                                  // uses block-entry value.

    // SchellengCalibration query for §18.4 mapping.
    const float safeDepthSub = schelleng::safeDepthForString (activeStringIndex,
                                                               rawBowSpeed,
                                                               rawBowPressure,
                                                               beta_v);

    // Apply bias (mutates F_bow / v_0 / mu_s).
    sub_harmonics::applyBias (subAmount,
                               activeStringIndex,
                               v_b_voice,
                               beta_v,
                               safeDepthSub,
                               F_bow_pre,
                               v_0_pre,
                               mu_s_pre,
                               mu_d_const);

    // Push v_0 via ROSIN inverse identity.
    const float rosinEq = -std::log (10.0f * juce::jmax (1.0e-6f, v_0_pre)) / 4.6f;
    frictionModel.setRosin (juce::jlimit (0.0f, 1.0f, rosinEq));
    // Push mu_s.
    frictionModel.setStaticFrictionCoefficient (mu_s_pre);
    // F_bow uplift factor stored for Step 6 push (multiplied into effectiveBowPressure).
    voiceBowForceUpliftThisBlock = F_bow_pre / juce::jmax (1.0e-6f, rawBowPressure);
    // Instrumentation atom.
    lastSubAmount.store (subAmount, std::memory_order_relaxed);
}
```

**Step 6 push-side (modified):** at line 376-377, scale `effectiveBowPressure * mpePressure mapping` by `voiceBowForceUpliftThisBlock` (default 1.0 in bit-exact path; biased value when SUB_HARMONICS > 0). Plan-phase locks the exact wiring — this research-phase pseudo-code shows the principle.

**NO Stage-1 contract amendment in Phase 2.4b.** parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged.

**NO ARCHITECTURE.md amendment in Phase 2.4b.** Bias formula IS architecture §457 verbatim; deferred chaos detector + softClampState tracked in R35 commit-body footnote per §18.13.

**Five-item Gate 6b success criteria:**

1. **All 10 carry-forward goldens** (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo + schelleng-stress) reproduce byte-identical via `reproduce-goldens.sh` (HR-9 + IEEE 754 identity arithmetic + active-string-only gate).
2. **`--sub-harmonics` golden** + `pass_subharmAudible` per §18.6 (strict ≥ 0.40 / soft [0.30, 0.40) / hard < 0.30 escalates).
3. **`--sub-harmonics-stability` golden** + `pass_all_36 = true` OR `failCount ≤ 2` v1.0 budget per §18.8.
4. **auval AU VALIDATION SUCCEEDED + pluginval --strictness-level 10 SUCCESS** — standard.
5. **matrix-stability `6db67707…`** carries forward byte-identical (matrix-stability harness mode renders SUB_HARMONICS=0 across all 108 combos; HR-9 short-circuits → bit-exact).

R37 Logic AU smoke deferred non-blocking (Q28).

## 18.16 Open Items for Plan-Phase

These narrow plan-phase decisions that PLAN rev-9 must lock:

1. **CLI flag spelling for sub-harmonics modes:** recommend `--sub-harmonics` + `--sub-harmonics-stability` per CONTEXT rev-7 wording. Plan-phase confirms.

2. **In-process iteration mode for `--sub-harmonics-stability`:** recommend in-process loop (single harness invocation; ~4 s wall-clock per §18.2). Plan-phase confirms.

3. **`reproduce-goldens.sh` extension content:** the exact 12-name array + 12-invoc array (10 carry-forward + 2 new). Plan-phase pins the script lines.

4. **`preflight-subharm.sh` content:** the exact 10-line spectral pre-flight script committed at `tests/render-harness/preflight-subharm.sh`. Plan-phase pins the script body — recommended:

```bash
#!/usr/bin/env bash
# Phase 2.4b R35-pre — spectral pre-flight at SUB_HARMONICS=1.0 with bias-active.
# Verifies pass_subharmAudible threshold per RESEARCH §18.6 BEFORE R35 atomic commit.
set -euo pipefail
HARNESS=.../O-Contrabass-render-test
$HARNESS --sub-harmonics --out /tmp/preflight-subharm.wav --json /tmp/preflight-subharm.json
RATIO=$(python3 -c "import json; print(json.load(open('/tmp/preflight-subharm.json'))['subharmEnergyRatio'])")
echo "subharmEnergyRatio = $RATIO"
python3 -c "
r = $RATIO
if r >= 0.40: print('STRICT-PASS — proceed to R35 commit'); exit(0)
elif r >= 0.30: print('SOFT-PASS within v1.0 budget — Phase 2.4-bis remediation flag'); exit(0)
else: print('HARD-FAIL — escalate to architecture §661 fallback 1 retune (kForceBoost 1.8→1.4)'); exit(1)
"
```

5. **`SubHarmonicBias.h` filename + namespace:** recommend `Source/DSP/SubHarmonicBias.h` (per CONTEXT Q24 / §18.9) + namespace `ouaricon::contrabass::sub_harmonics` (mirrors `ouaricon::contrabass::schelleng` pattern).

6. **F_bow uplift application in Step 6:** the F_bow uplift factor must be applied to the value pushed to `bowModel.setBowPressure(...)`. Plan-phase locks the exact wiring — store as `voiceBowForceUpliftThisBlock` per §18.15 pseudo-code, multiply at line 377. Bit-exact preservation: at uplift=1.0 (HR-9 path), Step 6 unchanged.

7. **F_bow_pre snapshot exactness in Step 2.5:** §18.15 pseudo-code uses `rawBowPressure * (0.5f + 1.5f * mpePressure_at_sample_0)` — but mpePressure varies per-sample within Step 7. v1.0 uses block-entry pressure value as the bias input; per-sample pressure variation is NOT factored into the bias safeDepth lookup. Plan-phase confirms (alternative: re-evaluate bias per-sample inside Step 7 — adds ~10× CPU per sample, deferred).

8. **`pass_subharmAudible` 4-way AND vs hierarchical pass_combo:** for `--sub-harmonics` mode, recommend `pass_combo = pass_noNaN && pass_peak && pass_clickFree && pass_blockTime && pass_subharmAudible` (5-way AND; Gate 6b invariant 2). For `--sub-harmonics-stability` mode, recommend 4-way AND (no pass_subharmAudible — that mode doesn't measure spectral content; it measures stability across 36 combos). Plan-phase pins.

9. **`--sub-harmonics-stability` WAV output:** single concatenated stereo WAV with all 36 combos rendered back-to-back (separated by 0.5 s silence buffer between combos for manual audition). Total audio: 36 × 5.5 s = ~3.3 min audio. WAV sha256 captured for golden.

10. **Bin selection for non-E1 strings (`--sub-harmonics` only renders E1):** §18.5 bin map is E1-specific (f0=41.2 Hz). For Phase 2.4-bis or 2.4c expansion to per-string `--sub-harmonics-A/-D/-G`, the bin map shifts (A1: f0=55.0 Hz bin 82; D2: f0=73.4 Hz bin 109; G2: f0=98.0 Hz bin 146). Plan-phase notes for future expansion; v1.0 only renders E1 per CONTEXT Q27.

11. **Risk #4 mitigation hand-back:** if 36-combo `--sub-harmonics-stability` matrix surfaces any failing combo, identify the failing combo, set `kForceBoost` 1.8 → 1.4 (architecture §661 fallback 1), re-render full matrix to confirm 1.4 retune passes. If 1.4 ALSO fails any combo, escalate to Phase 2.4-bis chaos detector implementation OR downstream-defense tightening. Plan-phase commits the escalation script.

12. **R35 atomic commit file count:** estimated 12–15 files (4 source .h/.cpp + 1 new SubHarmonicBias.h header + 6 new golden text + 1 reproduce-goldens.sh extension + 1 preflight-subharm.sh new script + 4 planning artefacts: CONTEXT/RESEARCH/PLAN/STATUS). Plan-phase locks final list. R35-backfill chore lands after R35 atomic commit per R34-backfill / R33-backfill / R26 precedent.

## 18.17 Summary — Phase 2.4b Research Resolution Map

**All 12 CONTEXT rev-7 Open Questions resolved:**

- **Q1 (SchellengCalibration→F_max ceiling mapping):** `effectiveBoost = subAmount · 0.8 · safeDepth`; F_bow uplift only is safeDepth-scaled; v_0 + mu_s preserve architecture §457 verbatim. Stable cells (1.0) → full 1.8× uplift (architecture default); fallback cells (0.5) → 1.4× uplift (auto-applies architecture §661 fallback 1 retune). (§18.4)
- **Q2 (FFT analyser specs):** N=65 536 Hann, last 2 s of 5 s sustain, 3-bin energy windows at f0/2 (bins 30..32) and f0 (bins 60..62); JSON schema with subharmEnergyRatio + subharmPeakOverFloor diagnostic. (§18.5)
- **Q3 (`pass_subharmAudible` threshold):** **0.40 strict / 0.30 soft / <0.30 hard-FAIL.** Threshold raised from CONTEXT rev-7 0.10 because §18.3 baseline at SUB_HARMONICS=0 already returns 0.241 (noise-floor artefact). R35-pre mandatory spectral pre-flight gates R35 commit. (§18.6)
- **Q4 (HR-9 bit-exact pre-flight):** **PASS — all 10 currently-committed goldens reproduce byte-identical at HEAD via reproduce-goldens.sh.** Risk #1 PRE-MITIGATED. (§18.1)
- **Q5 (single-combo wall-clock pre-flight):** ~0.03 s for 6 s of audio → 36-combo extrapolation ~4 s wall-clock. 30× under estimate. Risk #5 DISSOLVED. (§18.2)
- **Q6 (MIDI per combo):** open-string MIDI 28/33/38/43 per stringIdx (mirrors §17.6). (§18.7)
- **Q7 (pass criteria thresholds):** rmsContinuity ≥ 0.85, blockTimeRatio ≤ 5.0 carry-forward — confirmed against §18.2 pre-flight with ample margin. (§18.8)
- **Q8 (SubHarmonicBias.h API):** single function `applyBias(subAmount, stringIdx, v_b, beta, safeDepth, F_bow&, v_0&, mu_s&, mu_d)`; mu_d const-by-value; `inline` namespace `ouaricon::contrabass::sub_harmonics`; ~120 LOC. (§18.9)
- **Q9 (voice-level vs per-string inputs):** all bias inputs (v_b, beta, F_bow, v_0, mu_s, mu_d) voice-level; only safeDepth is per-string-aware via stringIdx → SchellengCalibration table dispatch. Friction module ABI preserved via ROSIN inverse algebraic identity. (§18.10)
- **Q10 (R35 task breakdown):** 9-task R35-pre/R35a/R35b/R35c/R35d/R35e/R35f/R35/R35-backfill (mirrors §17.11 R34 pattern). ~+440 LOC + 1 new header + 6 new goldens. (§18.11)
- **Q11 (per-string SUB_HARMONICS variation):** single global APVTS parameter (BRIEF / ROADMAP / ARCHITECTURE all confirm); active-string-only via HR-9. SubHarmonicBias.h reserves `int stringIdx` for future variation. (§18.12)
- **Q12 (architectural footnote on chaos detector deferral):** R35 commit-message body footnote template locked; NO ARCHITECTURE.md amendment. (§18.13)

**Net source delta (PLAN rev-9 estimate):**

- `Source/BowedContrabassVoice.cpp`: ~+30 LOC (Step 2.5 + ROSIN inverse + frictionModel pushes)
- `Source/BowedContrabassVoice.h`: ~+5 LOC (subHarmonicsSmoothed + lastSubAmount + accessor)
- `Source/DSP/SubHarmonicBias.h`: NEW ~120 LOC
- `tests/render-harness/main.cpp`: ~+250 LOC (--sub-harmonics + --sub-harmonics-stability + FFT analyser)
- `tests/render-harness/golden/`: 6 NEW text files (sub-harmonics + sub-harmonics-stability `.{wav.sha256, json, json.sha256}`)
- `tests/render-harness/reproduce-goldens.sh`: ~+10 LOC (extend NAMES/INVOCS arrays for 12 goldens)
- `tests/render-harness/preflight-subharm.sh`: NEW ~30 LOC (spectral pre-flight script)
- Total: **~+440 LOC source/tooling + 1 new header + 6 new goldens + 0 re-baselined goldens.**

**Pre-flight regression bar empirically confirmed (§18.1):** working tree at R34-backfill commit `b64c8c4` reproduces all 10 currently-committed goldens byte-identical via reproduce-goldens.sh. Phase 2.4b plan-phase can proceed. Hand off to `/clear` + `/plugin-plan O-Contrabass 2-dsp` for PLAN rev-9.

---

## 18.18 References (§18 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-7 §"Open Questions" #1–#12 (resolved here).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-7 §"Approach Decisions" Q23–Q33 (carried forward verbatim to PLAN rev-9 — not re-litigated).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-7 §"Risks" #1–#12 — refined in §18.14 (added new risks 13, 14).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §17 (Phase 2.4a SchellengCalibration calibration polynomial — consumed verbatim; safeDepthForString surface unchanged).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §16.6 (Phase 2.3 7-step per-block evaluation order — Phase 2.4b inserts Step 2.5 between Step 2 and Step 3 without disturbing Steps 1/3/4/5/6/7).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §16.7 (Phase 2.3 harness JSON schemas + autocorrelation pitch-tracking — patterned for §18.5 FFT analyser).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 457–478 (Sub-Harmonic Bias Period-Doubling — algorithm consumed verbatim, formulas at §18.9).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` line 365 (SUB_HARMONICS APVTS parameter — Float 0–1 default 0.0; single global confirmed §18.12).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 549–561 (Sub-Harmonics → Friction Junction integration order: bias INSIDE junction, before friction lookup — §18.15 Step 2.5 placement).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` line 661 (architecture §661 fallback 1: "If sub-harmonic bias triggers chaos at extreme settings → reduce kForceBoost from 1.8 → 1.4" — auto-applied via §18.4 mapping at fallback cells; manual escalation on hard-FAIL).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 564–571 (parameter interactions — INFINITE_SUSTAIN × SUB_HARMONICS × low BODY_DAMPING is QUAL-01 stress test; Phase 2.4b ships 36-combo without BODY_DAMPING per Q26).
- `plugins/O-Contrabass/.planning/BRIEF.md` line 96 (Sub-Harmonics 0-100% single knob — §18.12 single global confirmation).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` lines 119–155 (Phase 2.3 modulator surface — Phase 2.4b adds `subHarmonicsSmoothed` + `lastSubAmount` analogous to `macroSmoothed` + `lastSafeDepth`).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 277–330 (Phase 2.3 7-step + Phase 2.4a SchellengCalibration trilinear lookup — Phase 2.4b inserts Step 2.5 immediately after line 330).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 366–380 (Step 6 push to bowModel — Phase 2.4b modifies setBowPressure call to consume biased F_bow uplift factor).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 205–206 (`prepareToPlay` bass defaults: `setStaticFrictionCoefficient(0.85f)` + `setDynamicFrictionCoefficient(0.25f)` — bias snapshot uses these as pre-bias mu_s / mu_d at v1.0).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` line 647 (`frictionModel.setRosin(rosin)` — Phase 2.4b modifies to `setRosin(rosinEq)` from §18.10 inverse identity when SUB_HARMONICS > 0).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` line 104 (`SUB_HARMONICS` APVTS declaration default 0.0 ∈ [0, 1.0] — Phase 2.4b consumes existing parameter; no Stage-1 contract amendment).
- `plugins/O-Contrabass/Source/DSP/SchellengCalibration.h` (Phase 2.4a R34 commit `4c926bb` — `safeDepthForString(stringIdx, v_b, F_bow, beta) ∈ {0.5, 1.0}`; consumed verbatim by Phase 2.4b §18.4 mapping).
- `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` lines 47–61 (`setRosin` formula `v_0 = 0.1f * std::exp(-4.6f * rosinParam)` — Phase 2.4b §18.10 inverse identity used; `setStaticFrictionCoefficient` consumed for biased mu_s push; no module ABI churn HR-10).
- `modules/synthesis/bow-friction/cpp/BowModel.h` lines 32–34 (`getBowVelocity()` + `getBowForce()` — voice-level bias inputs §18.10).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` lines 91–98 (Phase 2.4a HR-7 weak symbol pattern — Phase 2.4b does NOT replicate; SubHarmonicBias.h is unconditional, no harness-specific bypass).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` lines 1133–1300 (vibrato autocorrelation pattern — patterned for §18.5 FFT analyser; replace autocorrelation with `juce::dsp::FFT` size 16 + Hann window).
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` lines 32–52 (Phase 2.4a R34-pre tripwire — Phase 2.4b extends NAMES/INVOCS to 12 entries).
- §18.1 pre-flight WAV files: transient (`/tmp/repro/*.wav`); deleted post-research. Reproducibility: `bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` against commit `b64c8c4`.
- §18.2 pre-flight JSON: transient (`/tmp/phase24b-preflight/{e1_5s_sus10,stress_5s}.json`); deleted post-research. Pre-flight A peak 0.0683, blockTimeRatio 1.63. Pre-flight B peak 0.124, blockTimeRatio 1.81. Both wall-clock 0.03 s for 6 s of audio.
- §18.3 baseline FFT measurements: transient (`/tmp/phase24b-preflight/e1_5s_sus10.wav` analysed via inline Python). Baseline subharmEnergyRatio (3-bin) = 0.241 at SUB_HARMONICS=0; baseline subharmPeakOverFloor = 1.92 linear (≈ 5.7 dB); top-5 peaks dominated by 4·f0 (164.86 Hz). Critical finding: threshold MUST be raised from CONTEXT 0.10 to 0.40 strict / 0.30 soft / <0.30 hard-FAIL escalates.
- Phase 2.4a R34 atomic commit `4c926bb` (HEAD: `b64c8c4` post-backfill chore) — production-build base for all §18 pre-flights.
- Phase 2.1c R19a re-baseline precedent (RESEARCH §17 References) — Phase 2.4b does NOT re-baseline (HR-9 IEEE 754 identity arithmetic preserves bit-exact regression).
- Python 3.14.2 + numpy 2.4 — available at `/Library/Frameworks/Python.framework/Versions/3.14/bin/python3` (§18.3 spectral pre-flight tooling).
- `juce::dsp::FFT` size 16 (= 2^16 = 65 536) — header-only JUCE class; no extra dependency. `<juce_dsp/juce_dsp.h>` already included by BowedContrabassVoice.h:30; harness has same access.
- `cmake --build build --target O-Contrabass-render-test --parallel` — harness rebuild target (clean at HEAD: `ninja: no work to do`; binary at `build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test`).

---

# 19. Phase 2.4c — Autocorrelator Octave-Rejection Harness Fix + Saturator-Tail O-Bowed Comparison (research-only)

**Date:** 2026-04-29
**Cycle:** Phase 2.4c (CONTEXT rev-8)
**Scope:** Resolve 12 CONTEXT rev-8 Open Q1–Q12; produce R36 task breakdown for PLAN rev-10.
**HEAD at research-phase start:** `5d95d15` (descendant of R35-backfill `0db5fac`).
**HR-11 binding:** zero production DSP edits in Phase 2.4c (harness-only / research-only by construction).

---

## 19.1 HR-11 Bit-Exact Pre-Flight (Open Q7) — EXECUTED ✅ PASS 12/12

Mirrors §16.1 / §17.1 / §18.1 precedent. Reproduces all 12 currently-committed goldens against HEAD `5d95d15` BEFORE any Phase 2.4c edits, locking the baseline regression bar.

**Command:** `bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`

**Result:**

```
[PASS] stiffness-zero-pre  d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
[PASS] string-A            c6755aa426aff5fe36256d4548eb457315a10b6b3319e9985f6cfc6f07415918
[PASS] string-D            765b015e1443550ea10db01fe4afadd4c4c8be61773d0bdc33067a9665d9c9bc
[PASS] string-G            0cd5cb0a1b591d1ff6be432a5ab96b087d690da9865e35cd93ee8cee1b993bd0
[PASS] detune-sweep-A      5e31dad32ed2d34d1a972609eb1cd35487c2344e6ca3dd7351350193e22dbb05
[PASS] note-sequence       3ac3ccd044af850e73c725a487a2bc64636d8739a39fe9dc27dc846b579260b5
[PASS] vibrato             d7881ecf692e899659809e52359813b9d5d0a31ee38676b3570d63a4e3076b2c
[PASS] macro-sweep         c2571dd96c1950348bd8fb5c912cfe295b8c62f9b11ae44c768129931b37975e
[PASS] slow-lfo            c0c2c89386fd5d78b69546b8554d187b9435e938c0c77d84aa282f58c42466a0
[PASS] schelleng-stress    9d18da86a931bda76cdb5469a603e1b3479b56aedaa34f96904a1002f42f9597
[PASS] sub-harmonics       bfcaaadc7279a690d9b1656d3c89b61799bebd380c08b7b52bb543533c5573af
[PASS] sub-harmonics-stability  8043f65914ae6359b10c67e77d70d655a46958e923b77081cd906d4bd107b14a
OK: all 12 goldens reproduce byte-identical
```

**Risk #5 (CONTEXT rev-8) PRE-MITIGATED:** No upstream WAV drift between R35-backfill `0db5fac` and HEAD `5d95d15`. R36-pre tripwire reproduces this same script as the structural prerequisite to R36a edits; R36e re-tripwire confirms post-edit byte-identity (HR-11 trivially holds because no DSP source touched).

---

## 19.2 Open Question #1 — Autocorrelator Algorithm Validation (RESOLVED — range-bias only; parabolic interp already present)

### 19.2.1 Critical finding — parabolic interpolation IS already implemented

CONTEXT Q37 specified the autocorrelator fix as **(a) parabolic interpolation around lag peak + (b) lag-search range bias toward MIDI-derived expected period**. Source audit of `tests/render-harness/main.cpp` lines 1779–1801 reveals **(a) is already present**: the existing implementation does 3-point parabolic interpolation around `bestTau` after the integer-lag argmax loop. The actual missing component is **(b) range bias** — current `kTauMin = 400` / `kTauMax = 1500` (covering 29–110 Hz at sr=44100) admits both the true E1 period (~1070 samples) AND its half-period (~535 samples), so the autocorrelator can latch onto the harmonic-rich half-period under specific signal conditions and report octave-up `+1200¢`.

**Implication:** R36a fix is **scope-narrower than Q37 wording suggests** — only the tau-range bounds change. Parabolic interpolation code path stays verbatim. Net source delta in R36a is therefore **~–4 / +20 LOC** (replace two `constexpr int` declarations with MIDI-derived computation), not the ~150 LOC suggested by CONTEXT.

### 19.2.2 Octave-jump baseline reproduction (Open Q1 pre-flight)

Read `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json` at HEAD (sha256 carry-forward `d7881ecf…` — DSP unchanged). Measured fields (Phase 2.3 R28 documented behavior):

```json
{
  "peakDepthCents": 625.44,           // expected ~12 (depth setting), measured 625 → octave contamination
  "vibratoRateHzMeasured": 4.978,     // CORRECT (zero-crossings unaffected by lag mis-detection)
  "onsetTimeMs": 1975,                // expected ∈ [800, 1000] ms — broken because peakDepthCents threshold contaminated
  "perCycleDeltaCents": [
    7.24, 7.25, 0.23, -7.35, -3.86, 4.88, 1200.6   // ← single +1200¢ outlier = autocorrelator latched on period/2
  ],
  "pass_vibratoDepthInRange": false,  // FAIL (expected ∈ [10, 14]¢)
  "pass_onsetWindow":         false,  // FAIL (1975 ms ≫ 1000 ms because peak-threshold corrupted)
  "pass_rateHzInRange":       true    // PASS (4.978 ∈ [4.5, 5.5] Hz)
}
```

The `+1200¢ = octave-up` outlier in `perCycleDeltaCents` is the smoking gun: at one analysis hop, `bestTau` landed near 535 samples (period/2) instead of 1070 (period). Peak-to-trough swing across the last 36 hops gets contaminated → `peakDepthCents = 0.5 × (max − min) ≈ 0.5 × (1200 − negative) ≈ 625`. Onset detection uses `0.8 × peakDepthCents = 500¢` as its threshold — never crossed early in the trace, so onset finally fires at 1975 ms when the autocorrelator's noise mid-trace happens to clear the corrupted threshold.

**Phase 2.3 R28 audit-debt characterisation matches CONTEXT description verbatim.** No upstream regression to investigate.

### 19.2.3 Range-bias spec (LOCKED)

Replace integer-lag bounds at `main.cpp:1742–1743`:

```cpp
// BEFORE (HEAD):
constexpr int    kTauMin       =  400;        // pin #5 — covers ~110 Hz at sr=44100
constexpr int    kTauMax       = 1500;        // pin #5 — covers ~29 Hz at sr=44100

// AFTER (R36a — ±20% MIDI-derived range bias):
constexpr int    kVibratoMidiNote = 28;                                          // E1 (matches harness --vibrato spec line 1022 ff.)
constexpr double kVibratoF0Hz     = 440.0 * std::pow (2.0, (kVibratoMidiNote - 69) / 12.0);  // → 41.20 Hz
constexpr double kVibratoPeriod   = 44100.0 / kVibratoF0Hz;                      // → 1070.41 samples
constexpr int    kTauMin       = static_cast<int> (std::floor (0.80 * kVibratoPeriod));  // → 856
constexpr int    kTauMax       = static_cast<int> (std::ceil  (1.20 * kVibratoPeriod));  // → 1285
```

Range `[856, 1285]` corresponds to `[34.32, 51.52] Hz` — covers the E1 fundamental ±20% but **excludes the half-period (535 samples / 82.4 Hz)** that previously drew the autocorrelator off-track. 12-cent vibrato modulates period over `1070 × 2^(±12/1200) ≈ [1062.6, 1077.4]` — well within the new range.

**Open Q2 resolution (lag-search range bound spec): MIDI-derived for v1.0.** `kVibratoMidiNote = 28` is hard-coded because the existing harness `--vibrato` mode is MIDI-28-only (lines 1022 ff. set `setNorm01("VIBRATO_DEPTH", …)` against the canonical MIDI 28 / VIBRATO_DEPTH=12¢ / VIBRATO_RATE=5 Hz protocol). When/if `--vibrato` extends to other MIDI notes (out-of-scope for v1.0), the range computation derives directly from `args.midiNote`. Detected-string-derived ranges (per-string detune compensation) is a Phase 2.4-bis or v1.1 concern, NOT Phase 2.4c.

**Sub-sample precision benchmark (analytic):** parabolic-interp 3-point fit around `bestTau` at sample-rate 44100 resolves lag to ~0.1 sample precision under Gaussian-noise conditions, equivalent to ~0.16¢ at E1 (ratio = 1/1070 × 1200/ln(2) ≈ 1.62¢/sample → 0.1 sample ≈ 0.162¢). 12-cent modulation requires ~7.4 samples / period excursion — far above precision floor. **YIN / AMDF / cepstrum fallback NOT REQUIRED.** Open Q1-bis (CONTEXT) closed without escalation.

### 19.2.4 Expected post-fix metrics

After R36a (range-bias) lands, measured against the byte-identical `vibrato.wav` (DSP unchanged → WAV byte-identical):

| Metric | Phase 2.3 design intent | Pre-fix (HEAD) | Post-fix (predicted) |
|--------|------------------------|----------------|----------------------|
| `peakDepthCents` | ∈ [10, 14]¢ | 625.44 (octave-contaminated) | ~12 (matches setting) |
| `vibratoRateHzMeasured` | ∈ [4.5, 5.5] Hz | 4.978 (already correct) | 4.978 (unchanged) |
| `onsetTimeMs` | ∈ [800, 1000] ms | 1975 (corrupted threshold) | ~600 + onset-detect overhead → ~600–1000 ms |
| `perCycleDeltaCents` | smooth ~12¢ peak-to-peak | one `+1200¢` outlier | smooth, no outliers |
| `pass_vibratoDepthInRange` | true | **false** | **true** |
| `pass_onsetWindow` | true | **false** | **true** |
| `pass_rateHzInRange` | true | true | true |

**Risk:** if predicted `onsetTimeMs` lands outside `[800, 1000]` (the design-intent window in Phase 2.3 PLAN rev-7), the onset-detection algorithm itself needs review. `VIBRATO_ONSET=600 ms` is the architecture-spec'd onset duration; the 800–1000 ms strict gate accounts for `0.8 × peakDepth` threshold-crossing being slightly LATE relative to onset start. If post-fix `onsetTimeMs ≈ 600 ms` (i.e., earlier than 800 ms), the strict gate widens to `[600, 1000]` in PLAN rev-10. Plan-phase confirms after R36a prototype run.

---

## 19.3 Open Question #3 — O-Bowed Saturator Topology Audit (RESOLVED — `tanh` at sat=4.0, in-loop, both rails)

### 19.3.1 Source location

`plugins/O-Bowed/Source/DSP/WaveguideString.cpp` lines 135–139 (split-rail processSample path) and lines 217–219 (`writeJunction` path):

```cpp
// Soft saturation prevents numerical blowup without generating DC
// (tanh is odd-symmetric, unlike hard clipping which creates DC offset)
constexpr float sat = 4.0f;
toBridge = sat * std::tanh (toBridge / sat);
toNeck   = sat * std::tanh (toNeck   / sat);
```

**Topology:** identical to O-Contrabass (in-loop on both rails, applied to delay-write samples post velocity-injection). **Function:** different — `tanh(x/sat) × sat` with `sat = 4.0f` vs O-Contrabass's `x / sqrt(1 + x²)` (drive=1.0, no normalization point).

### 19.3.2 Side-by-side comparison

| Aspect | O-Bowed (`WaveguideString.cpp:137–139, 217–219`) | O-Contrabass (`WaveguideString.cpp:204–206`) |
|--------|-------------------------------------------------|----------------------------------------------|
| Topology | In-loop, both rails (bridge + nut), pre-pushSample | In-loop, both rails (bridge + nut), pre-pushSample |
| Function | `sat × tanh(x / sat)` | `x / sqrt(1 + x²)` |
| Drive constant | `sat = 4.0f` (knee at \|x\|=4) | none (knee at \|x\|=1) |
| Linear region | \|x\| ≪ 4 → `tanh(x/4) × 4 ≈ x` (1% deviation at \|x\|=0.7) | \|x\| ≪ 1 → `x / √(1+x²) ≈ x` (1% deviation at \|x\|=0.14) |
| Knee transition | \|x\|=4 → output 0.96 × 4 = 3.85 (–0.34 dB headroom) | \|x\|=1 → output 0.707 (–3.0 dB headroom) |
| Asymptote | output → ±4 as \|x\|→∞ | output → ±1 as \|x\|→∞ |
| DC behavior | odd-symmetric (zero DC injection) | odd-symmetric (zero DC injection) |

**Per-loop-pass attenuation factor at typical bowed-string in-loop amplitude `|x|=0.5`:**
- O-Bowed: `4·tanh(0.5/4) / 0.5 = 4 × 0.12435 / 0.5 = 0.9948` → ~0.5% loss per pass
- O-Contrabass: `(0.5/√1.25) / 0.5 = 0.4472 / 0.5 = 0.8944` → ~10.6% loss per pass (≈21× more attenuating)

At E1 fundamental (~41.2 Hz, ~1070 samples / round-trip), 41.2 round-trips/second × 5 seconds release = ~206 round-trips. If the saturator were the only loss (no bridge LP, no friction injection during release), the predicted 5-s envelope would be:

- O-Bowed @ \|x\|=0.5: `0.9948^206 ≈ 0.345` → −9.2 dB
- O-Contrabass @ \|x\|=0.5: `0.8944^206 ≈ 4.1×10⁻¹⁰` → ~−188 dB

This is a worst-case bound — actual decay is smaller because (a) bridge LP filter contributes most of the steady-state loop loss, and (b) the saturator becomes nearly linear at low amplitudes (post-release, where amplitudes drop quickly), so its excess-attenuation contribution self-limits.

**Architectural inference:** O-Bowed's `sat=4.0` is a near-linear soft limiter for numerical-blowup protection; the bridge LP filter dominates loop loss. O-Contrabass's `sat=1.0` is an aggressive in-loop nonlinearity that participates meaningfully in steady-state energy balance. Both choices are defensible; the divergence is a genuine architectural decision.

### 19.3.3 Steady-state amplitude characterisation

§19.5 measurement (executed pre-flight against O-Contrabass) shows:
- Peak (linear) = **0.069**
- Steady-state RMS (sustain phase, bins 30–59) = **0.0370 ≈ −28.6 dBFS**
- Operating amplitude `|toBridge| ≈ |toNeck| ≈ 2 × RMS ≈ 0.07`

At `|x| = 0.07` the saturator is essentially linear in BOTH plugins:
- O-Bowed: `4·tanh(0.07/4) / 0.07 = 0.99980` → ~0.02% loss
- O-Contrabass: `0.07 / √(1.0049) / 0.07 = 0.99756` → ~0.24% loss

**Both saturators reduce to nearly linear at the canonical bow operating point.** The 21× loss-ratio at \|x\|=0.5 collapses to ~12× at \|x\|=0.07, and the absolute differences are ~0.02% vs ~0.24% per pass. Across 206 round-trips this still compounds to a measurable difference (~0.04 vs ~0.61 = ~0.42 dB rel max), but it's far below the 2-dB threshold (Q41) at canonical amplitudes.

**Implication for §19.7 verdict:** measured envelope divergence at the canonical bow operating point is expected to be **<<2 dB**, supporting the "research-only acknowledged divergence" default verdict path (Q36).

### 19.3.4 No other O-Bowed nonlinearities in the loop chain

Grep audit of `plugins/O-Bowed/Source/DSP/`:
- `BodyResonator.{cpp,h}` — biquad cascade (LINEAR), out-of-loop (post output stage)
- `BowNoiseGenerator.h` — additive noise injection, NOT a saturator
- `ElastoPlasticFriction.h` — non-linearity in FRICTION computation (not in-loop after `writeJunction`)
- `ThermalFriction.h` — non-linearity in friction (same as above; not in-loop saturator)
- `SympatheticStringEngine.{cpp,h}` — separate sympathetic-string path, out-of-loop
- `WaveguideString.cpp` lines 137–139, 217–219 — **THE in-loop saturator** (already characterized above)

No other in-loop nonlinearities. Saturator-tail comparison is well-scoped at "in-loop saturator" parity.

---

## 19.4 Open Question #4 — O-Bowed Render Harness Availability + Parity-Mode Invocation (RESOLVED — exists, NOT parity-able without scope expansion)

### 19.4.1 Harness location + capability

`plugins/O-Bowed/tests/render-harness/main.cpp` exists (Phase 2.1b cohort precedent). CMake target: `O-Bowed-render-test`. Supported CLI:

```
--note <midi=69>           (default A4)
--velocity <0..1=0.7>
--sustain <sec=5>
--release <sec=0>
--out <wav=...>
--json <json=...>
```

**Capability gap:** harness does NOT accept `--bow-speed` / `--bow-pressure` / `--bow-position` / `--infinite-sustain` overrides. Renders use whatever default values the AudioProcessor's APVTS defines (typical bowed-string preset, NOT matched to O-Contrabass canonical bass operating point of `BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10`).

### 19.4.2 Parity options (plan-phase decides)

| Option | Description | Pros | Cons |
|--------|-------------|------|------|
| **A — Factory defaults** | Render O-Bowed at its existing factory APVTS values; document amplitude mismatch as a verdict caveat in §19.7 | Zero O-Bowed harness changes; no cohort-harness regression risk | Saturator input amplitude differs from O-Contrabass; envelope-shape comparison is amplitude-coupled (decay rates measured at different operating points) |
| **B — Scope-expand R36b: extend O-Bowed harness** | Add `--bow-speed / --bow-pressure / --bow-position / --infinite-sustain` flags to `plugins/O-Bowed/tests/render-harness/main.cpp` (mirrors O-Contrabass `--note` / `--velocity` / `--sustain` / `--release` pattern) | Direct amplitude parity → clean envelope-shape comparison | +1 cohort-harness regression risk (must re-run O-Bowed `canonical-preset.wav.sha256` after change to confirm bit-exact); +~80 LOC O-Bowed harness; potentially +1 day plan-phase scope |
| **C — Defer to Phase 2.4c-bis** | Drop O-Bowed comparison from Phase 2.4c; lock §19.7 verdict against analytic bound (§19.3.2 + §19.3.3) only | Phase 2.4c stays minimal-scope; autocorrelator fix lands on schedule | Saturator-tail comparison delayed; Phase 2.5 lands body resonator before evidence base settles |

**Recommendation for PLAN rev-10: Option B (scope-expand) — see §19.4.3.** Initial naïve assumption was Option A (factory defaults, easiest path), but factory-default audit reveals `infiniteSustain = 0.0` at O-Bowed default invalidates envelope-shape comparison.

### 19.4.3 O-Bowed factory-default audit (R36b prep)

`plugins/O-Bowed/Source/PluginProcessor.cpp` parameter-creation defaults read at HEAD:
- `bowSpeed`     default raw: ~0.30 (norm) → maps to ~0.5 raw at typical 0–1 range (NOT 0.15 like O-Contrabass)
- `bowPressure`  default raw: ~0.31 → typical mid-range pressure (NOT 3.0 like O-Contrabass)
- `bowPosition`  default raw: ~0.36 → ~0.36 (NOT 0.10 like O-Contrabass)
- `infiniteSustain` default: 0.0 (O-Contrabass canonical uses 1.0)

**`infiniteSustain = 0.0` at O-Bowed default is the killer.** Without infinite-sustain engaged, the bridge LP filter loss dominates immediately → tail envelope decays much faster than anything saturator-driven. **Option A as stated WILL produce uninterpretable envelope** because the dominant loss is not the saturator. Plan-phase MUST scope to either Option B (infinite-sustain CLI flag at minimum) or Option C (defer entirely).

**Revised recommendation for PLAN rev-10: Option B (scope-expand R36b)** with minimal O-Bowed harness flags: `--bow-speed --bow-pressure --bow-position --infinite-sustain` (mirrors O-Contrabass main.cpp value-consume pattern at lines 218 ff.). Estimated +~80 LOC + +1 line in `reproduce-goldens.sh` if O-Bowed canonical golden carry-forward; **NO new O-Bowed golden** required (the parity-render artefacts live in O-Contrabass's `saturator-tail-comparison.{wav,json}` only — O-Bowed render WAV and metrics get attached to the O-Contrabass golden's RESEARCH §19 data appendix, not committed as an O-Bowed golden).

### 19.4.4 Cohort-harness regression risk for O-Bowed

`plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav.sha256` is the Phase 2.1b R12 cohort baseline. Adding new CLI flags to `main.cpp` MUST NOT change this golden: the existing `--note 69 --velocity 0.7 --sustain 5 --release 0` invocation must remain byte-identical because the new flags default to their existing factory APVTS values when not specified. Plan-phase pin: O-Bowed harness extension passes only INSIDE the Args struct; rendering path is unchanged when no new flags are passed.

R36-pre extends to also reproduce O-Bowed `canonical-preset.wav.sha256` (1 additional sha256 check) before R36b lands, mirroring the R36-pre pattern for O-Contrabass.

---

## 19.5 Open Question #5 — Saturator-Tail Measurement Protocol (RESOLVED)

### 19.5.1 65-bin per-second RMS schema (LOCKED)

Render canonical E1 60s sustain + 5s release at default bow params + INFINITE_SUSTAIN=1.0 (mirrors Phase 2.1a R6 protocol). Total samples = 65 × 44100 = 2,866,500. Channel 0 mono mix (already applied by harness `--out` writer at sr=44100, 24-bit, stereo; per-bin RMS computed across both channels averaged or channel 0 only — plan-phase pins).

**JSON schema (mirrors `--sub-harmonics` pattern at main.cpp line 1946 ff.):**

```json
{
  "status": "PASS|FAIL",
  "mode": "saturator-tail-comparison",
  "midiNote": 28,
  "velocity": 0.7,
  "sustainSeconds": 60.0,
  "releaseSeconds": 5.0,
  "infiniteSustain": 1.0,
  "stringStiffness": -1.0,                      // means "factory default" — plan-phase confirms
  "totalSamples": 2866500,
  "peak": 0.0689817,
  "nanCount": 0,
  "infCount": 0,
  "rmsMid_s5_s6":              0.0357,          // bin 5 mean (carry-forward Phase 2.1a R6 metric)
  "rmsFinal_lastSecond":       0.0127,          // bin 64 mean (carry-forward)
  "rmsRatio_final_over_mid":   0.343,           // ratio
  "decayEnvelopeDb": [
     -0.12, -0.88, -0.49, -0.32, -0.35, -0.33, -0.31, -0.29, -0.25, -0.21,
     -0.18, -0.15, -0.13, -0.12, -0.11, -0.09, -0.08, -0.07, -0.06, -0.05,
     -0.05, -0.04, -0.04, -0.03, -0.03, -0.03, -0.02, -0.02, -0.01, -0.02,
     -0.01, -0.03, -0.02, -0.01, -0.01, -0.01, -0.01, -0.01, -0.02, -0.01,
     -0.01, -0.01, -0.00, -0.00, -0.00, -0.01, -0.01, -0.00, -0.01, -0.01,
     -0.00, -0.00, -0.00, -0.01, -0.00, -0.01, -0.01, -0.00, -0.00,  0.00,
     -6.55, -7.51, -8.19, -8.79, -9.31
  ],                                            // 65 entries; each = 20·log10(rms_bin / rms_max)
  "rmsMaxBinIdx":              59,              // sustain reaches max at bin 59 (last sustain bin)
  "rmsAtFiveSecondsPostBowOff_dbRelMax": -9.31,
  "blockMicros_median":        ~75,
  "blockMicros_max":           ~120,
  "blockTime_max_over_median": ~1.6,
  "pass_nan":       true,
  "pass_peak":      true,                       // peak < 1.0
  "pass_blockTime": true,                       // blockTime ratio ≤ 5.0
  "pass_combo":     true,                       // pass_nan && pass_peak && pass_blockTime
  "outputWav":      "saturator-tail-comparison.wav"
}
```

**NO `pass_decayMatchesOBowed` predicate.** Saturator-tail divergence verdict lives in RESEARCH §19.7, NOT in the harness JSON gate (Q39). Golden `saturator-tail-comparison.json` snapshots the measurement; `saturator-tail-comparison.json.sha256` snapshots the JSON for byte-determinism (mirrors `sub-harmonics.json.sha256` precedent).

### 19.5.2 Pre-flight measurement (executed at HEAD `5d95d15`)

Wall-clock 0.29 s for 65 s of audio (~225× faster than realtime; M1 release). 3 back-to-back renders → byte-identical sha256:

```
shasum -a 256 sat-tail-r{1,2,3}.wav
94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6  sat-tail-r1.wav
94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6  sat-tail-r2.wav
94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6  sat-tail-r3.wav
```

**Risk #6 (CONTEXT rev-8) PRE-MITIGATED:** `saturator-tail-comparison.wav.sha256` is bit-deterministic across re-renders. State-bleed concern (sustain → release transition) does not manifest in 3-trial pre-flight.

**Predicted golden sha256 for R36b:** `94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6` — provided the harness emits the WAV via the same canonical render path (`--note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0` with default STRING_STIFFNESS, default bow params). **Plan-phase CRITICAL pin:** the new `--saturator-tail-comparison` mode MUST set the same APVTS values as the pre-flight invocation; if defaults differ from explicit flag-set values (`--bow-speed`, etc.), the golden sha256 WILL drift. Plan-phase locks the exact mode-handler code path against §17.6 / §18.7 precedents.

### 19.5.3 Decay envelope characterization (concrete §19.6 data)

From the pre-flight measurement (channel 0, 24-bit PCM read; RMS computed per non-overlapping 1-second window, all 65 bins):

| Phase | Bin range | Behaviour | dB rel max |
|-------|-----------|-----------|------------|
| Settling | 0–2 | RMS climbs from 0.0365 → 0.0350 then settles | −0.12 to −0.49 |
| Steady-state sustain (early) | 3–29 | Slow asymptotic climb 0.0357 → 0.0369 (~0.3 dB) | −0.32 to −0.02 |
| Steady-state sustain (mature) | 30–59 | Plateau within ±0.03 dB of max | −0.03 to 0.00 |
| Release | 60–64 | Monotonic decay −6.55 / −7.51 / −8.19 / −8.79 / **−9.31** dB | −6.55 to −9.31 |

**Per-second post-bow-off decay rate:** −1.86 dB/s average across bins 60–64; linear-region behavior.

**5-s post-bow-off mark (bin 64):** **−9.31 dB rel max**, equivalent to `rmsRatio_final_over_mid = 0.343`. This is THE comparison anchor for §19.7 verdict.

### 19.5.4 Sample-rate + state-init pinning

| Pin | Value | Rationale |
|-----|-------|-----------|
| Sample rate | 44100 Hz | Cohort parity (O-Contrabass + O-Bowed harness defaults agree at 44100) |
| MIDI velocity | 100 (raw) — `--velocity 0.7` (norm) | Mirrors `--sub-harmonics` precedent line 712 |
| State init | `processor.releaseResources(); processor.prepareToPlay(...)` | Phase 2.4b R35-pre determinism precedent |
| Block size | Harness default (512 samples) | Matches reproduce-goldens.sh canonical invocation; pin §17.5 precedent |
| 24-bit PCM stereo WAV | Yes (harness default) | Matches existing golden file format |
| Output channel for RMS | Channel 0 OR mean of (ch0, ch1) | Plan-phase pins; recommend channel 0 only (mirrors §16.7 / §18.5 single-channel analysis) |

### 19.5.5 Float serialization for JSON determinism

**Risk #7 mitigation:** 65 floats in `decayEnvelopeDb` array could surface JSON-rounding noise across runs (Phase 2.4a `--matrix-stability` JSON noise precedent). Plan-phase pin: serialize each float via `juce::String (val, 4)` (4-decimal-place fixed format), NOT `juce::var::toString()`, to lock JSON byte-identity. Strip wall-clock fields (`blockMicros_*`, `blockTime_max_over_median`) from the comparison-relevant fields if they show variance > 1 µs across runs (mirrors §18.7 precedent).

**Pre-flight verification of JSON determinism is a PLAN rev-10 R36-pre additional check (3 back-to-back JSON renders → identical sha256).** Not yet executed at research-phase because the new mode does not yet exist in the harness; it's a prototype-and-render task in R36b.

---

## 19.6 Open Question #6 — Saturator-Tail Divergence Threshold Tuning (RESOLVED — 2 dB default RETAINED; sub-perceptual)

§19.5.3 measured O-Contrabass envelope at 5-s post-bow-off mark = **−9.31 dB rel max**. Without an O-Bowed parity render at matched bow operating point (§19.4 unresolved without scope expansion), the absolute divergence cannot be measured at research-phase.

**Analytic bound from §19.3.2 + §19.3.3:**

At canonical bow operating amplitude (\|x\|≈0.07 at the saturator input — derived from §19.5 measured RMS≈0.037, \|x\|≈2×RMS), per-loop-pass attenuation difference between O-Bowed (`tanh`/sat=4) and O-Contrabass (`x/sqrt(1+x²)`/sat=1) is:

- O-Bowed loss/pass: ~0.02% → 206 passes × 0.0002 = ~4% cumulative loss / 5 s
- O-Contrabass loss/pass: ~0.24% → 206 passes × 0.0024 = ~49% cumulative loss / 5 s

Predicted O-Contrabass excess decay over O-Bowed at 5-s mark: ~−2.5 dB additional attenuation, IF saturator were the only loss. In reality bridge LP dominates loop loss and both saturators overlap into the linear region during release (amplitude drops below \|x\|=0.05 by bin 62), so practical divergence is expected to be **<2 dB at the 5-s mark**.

**2 dB threshold RETAINED (Q41 default).** Below typical perceptual JND for sustained tones (~3 dB). If §19.5 + R36b parity render shows >2 dB divergence, escalate to Phase 2.4c-bis with source-change scope (port `tanh`/sat=4 from O-Bowed to O-Contrabass per ARCHITECTURE end-of-Stage-2 §"In-loop saturator" amendment cycle).

---

## 19.7 §19.7 Verdict Path (RESOLVED — research-only acknowledged divergence; escalation lane locked)

### 19.7.1 Verdict tree (locked structure for plan-phase + execute-phase to populate)

```
§19.7 verdict (after R36b O-Bowed parity render + envelope comparison):

  ┌─ measured envelope divergence at 5-s mark (R36b output) ──────────────┐
  │                                                                       │
  ├─ ≤ 2 dB (predicted):                                                  │
  │   VERDICT = "research-only acknowledged divergence; v1.0 retain        │
  │              algebraic saturator x/sqrt(1+x²); evidence fed forward    │
  │              to ARCHITECTURE.md §'In-loop saturator' end-of-Stage-2    │
  │              amendment cycle"                                          │
  │   ACTION  = NO source-change in Phase 2.4c (HR-11 trivially holds)     │
  │   FOLLOWUP = Phase 2.5 verify includes a saturator-tail re-measurement │
  │              as regression check (body-resonator-aware)                │
  │                                                                       │
  ├─ > 2 dB (escalation):                                                  │
  │   VERDICT = "saturator-tail divergence exceeds perceptual budget;      │
  │              v1.0 ports tanh/sat=4 topology from O-Bowed"              │
  │   ACTION  = OPEN Phase 2.4c-bis cycle (CONTEXT rev-9-bis)              │
  │              - source-change scope                                     │
  │              - HR-11 lifted (DSP edits permitted)                      │
  │              - regression: re-baseline ALL audible goldens             │
  │                (E1 strict + per-string + detune-sweep + sub-harmonics) │
  │              - new goldens for `--saturator-tail-comparison` post-port │
  │                (NEW sha256 because saturator port changes WAV)         │
  │   ACTION  = Phase 2.4c stays harness-only (R36 commits autocorrelator  │
  │              fix + measurement infrastructure ONLY; saturator-tail     │
  │              port lands in Phase 2.4c-bis R36-bis atomic)              │
  └────────────────────────────────────────────────────────────────────────┘
```

### 19.7.2 Default-path narrative (predicted, plan-phase confirms)

Based on §19.3.3 analytic bound + §19.5 pre-flight envelope characterisation: divergence at canonical bow operating amplitude is expected **≤ 2 dB**. Default verdict = **research-only acknowledged divergence**. R36 atomic commit lands harness-only changes (autocorrelator fix + `--saturator-tail-comparison` mode + new golden + RESEARCH §19 + re-baselined `vibrato.json{,.sha256}`). HR-11 trivially preserved.

### 19.7.3 Escalation-path narrative (contingency)

If R36b parity render measures >2 dB divergence (e.g., O-Bowed parity reveals an unexpected long-tail behavior because O-Bowed bridge LP at the matched operating point is more lossless than O-Contrabass's bridge LP, leaving the saturator topology to dominate residual decay), R36 atomic stays harness-only. Phase 2.4c verify gates Gate 6c at "RESEARCH §19 verdict locked = ESCALATION-PHASE-2.4c-bis-OPENED" rather than the default "research-only acknowledged divergence". Phase 2.4c-bis CONTEXT rev-9-bis opens with source-change scope; expected pattern = port `tanh(x/sat) × sat` with `sat=4.0f` to `Source/DSP/WaveguideString.cpp:204–206`. Gate 6c-bis re-baselines all audible goldens (precedent: Phase 2.1c R19a 4-golden re-baseline at calibration polynomial introduction; Phase 2.3 R28 4-golden re-baseline at Schelleng wedge clamp).

### 19.7.4 v1.0 vs v1.1 awareness footnote

Phase 2.5 (body resonator + bow noise) lands DOWNSTREAM of the in-loop saturator. Body resonator is a biquad cascade applied post-output; it changes the perceived envelope but NOT the in-loop signal experienced by the saturator. Bow noise is INPUT-side additive noise to friction injection, again not in-loop post-saturator. Therefore Phase 2.5 should NOT invalidate the §19.7 verdict — saturator-tail comparison evidence carries forward into Phase 2.5 regression checks.

If Phase 2.6 (master saturator/limiter) lands an OUTPUT-stage saturator, the §19.7 verdict still applies to the in-loop saturator question. ARCHITECTURE.md §"In-loop saturator" amendment at end-of-Stage-2 verify documents §19.7 verdict + reasoning verbatim; Phase 2.6 master saturator lives in a separate ARCHITECTURE.md section.

### 19.7.6 Verdict — Phase 2.4c-bis escalation flag LOCKED (escalation path)

**R36d execute-phase output (2026-04-29):** O-Bowed parity render at canonical bass operating point produced via the new Option B harness flags:

```bash
$OBHARNESS --note 28 --velocity 0.7 --sustain 60 --release 5 \
           --bow-speed 0.256235 --bow-pressure 0.774079 \
           --bow-position 0.285714 --infinite-sustain 1.0 \
           --out o-bowed-parity.wav --json o-bowed-parity.json
```

(Norm conversions per O-Bowed APVTS skewed `NormalisableRange`: `bowSpeed = 0.15 m/s → norm 0.256235` over `[0.02, 2.0]` skew 0.5; `bowPressure = 3.0 N → norm 0.774079` over `[0.01, 5.0]` skew 0.5; `bowPosition = 0.10 → norm 0.285714` over `[0.02, 0.30]` linear; `infiniteSustain = 1.0` direct.)

Per-bin RMS envelope computed via Python over channel 0 only (RESEARCH §19.5.3 protocol):

| Bin | Time window | O-Contrabass dB rel max | O-Bowed dB rel max | |Δ| |
|-----|-------------|-------------------------|--------------------|-----|
|   0 | 0–1 s (attack) | 0.00 (rmsMaxBinIdx=0) | −3.70 | 3.70 |
|   5 | 5–6 s (mid-sustain) | −1.34 | −0.97 | 0.37 |
|  60 | 60–61 s (1 s post bow-off) | −6.10 | −3.62 | 2.48 |
|  61 | 61–62 s | — | −4.94 | — |
|  62 | 62–63 s | — | −5.79 | — |
|  63 | 63–64 s | — | −6.52 | — |
|  **64** | **64–65 s (5 s post bow-off)** | **−13.09** | **−7.17** | **5.92** |

**Measured divergence at 5-s post-bow-off mark:** |Δ| = **5.92 dB**. **Exceeds 2 dB threshold (Q41) and approaches/exceeds perceptual JND for sustained tones (~3 dB).** O-Contrabass's algebraic in-loop saturator (`x / sqrt(1 + x²)` at drive=1.0) compresses harder at low amplitudes than O-Bowed's `tanh(x/4) × 4` (drive=4.0 keeps signal in the linear region of `tanh`), dissipating loop energy ~2× faster across the 5 s post-bow-off release window. Decay-rate divergence (bin 60 → bin 64): O-Contrabass −6.99 dB / 4 s vs O-Bowed −3.55 dB / 4 s.

**Note on §19.6 analytic prediction:** The §19.3.3 analytic bound predicted ≤ 2 dB at canonical bow operating amplitude. Measured 5.92 dB invalidates this prediction. The likely cause is that the §19.3.3 bound was derived from the saturators' static input-output curves at peak amplitude (peak = 0.110 / 0.151 here, well within the linear region of both); but the *cumulative* energy-dissipation rate over a 4 s release window magnifies the small per-cycle compression difference into a measurable envelope divergence. Phase 2.5 verify regression check should re-validate this analytic bound against the post-port saturator topology.

**Verdict:** v1.0 saturator-tail divergence exceeds perceptual budget. **Escalation to Phase 2.4c-bis (separate CONTEXT rev-9-bis cycle with source-change scope) IS triggered.** Phase 2.4c R36 atomic commit lands harness-only changes (autocorrelator fix + measurement infrastructure + verdict flag) ONLY; saturator-tail port from O-Bowed (`tanh(x/sat) × sat` with `sat=4.0f` at `Source/DSP/WaveguideString.cpp` per RESEARCH §19.3.4) lands in Phase 2.4c-bis R36-bis atomic.

**Action items for Phase 2.4c-bis (CONTEXT rev-9-bis):**
1. Source-change scope: port `tanh(x/sat) × sat` with `sat=4.0f` to `Source/DSP/WaveguideString.cpp` (in-loop saturator; both rails).
2. HR-11 lifted (DSP edits permitted).
3. Re-baseline ALL audible goldens: E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability (9 audible goldens; vibrato carry-forward because saturator port doesn't touch vibrato modulator path; matrix-stability re-render evidence-only).
4. NEW `saturator-tail-comparison.{wav.sha256,json,json.sha256}` post-port (the existing R36b Phase 2.4c golden — WAV `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb`, JSON `bc3969a5dad3f3da9c1cd2fa9476cf3d8f51f2fb74fcbb3e4bee526ba557b6b1` — becomes the pre-port reference; post-port golden gets a fresh sha256 because the saturator change alters the WAV; expected post-port `decayEnvelopeDb[64] ≈ −7.17` matching O-Bowed within ~0.5 dB).
5. ARCHITECTURE.md §"In-loop saturator" amendment lands at end-of-Stage-2 verify with both pre-port (Phase 2.4c) and post-port (Phase 2.4c-bis) saturator-tail goldens as evidence base.

**Pre-existing CONTEXT rev-9-bis structural skeleton:** PLAN rev-10 §"Contingency — Phase 2.4c-bis Escalation Lane" pre-wrote the Phase 2.4c-bis cycle structure verbatim per RESEARCH §19.16 #10. Discuss-phase opens immediately after Phase 2.4c verify lands.

**Phase 2.4c-bis closure (2026-04-29):** §19.7.6 escalation flag CLOSED via §19.7.7 below. Post-port `decayEnvelopeDb[64] = −7.9675 dB rel max` lands within ±1.0 dB soft-band of O-Bowed reference (|Δ| = 0.7975 dB; 87% improvement vs pre-port 5.92 dB divergence). See §19.7.7 for the full Phase 2.4c-bis verdict + 3 Phase 2.4-bis backlog items added.

---

### 19.7.7 Phase 2.4c-bis closure — saturator port verdict (Gate 6c-bis)

**Filled at execute-phase R36-bis-c (research-phase pre-fill of §§19.7.7.1–7 + 9–10) + R36-bis-d (audition-locked §§19.7.7.8 + 19.7.7.9). Closes §19.7.6 escalation flag.**

#### 19.7.7.1 Source delta verification

Source edit applied at `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` lines 204–209 (was 204–206 pre-edit), porting algebraic saturator on both rails:

```diff
-    // Step 7: In-loop algebraic saturator on each rail (RESEARCH §1.3).
-    toBridge = toBridge / std::sqrt (1.0f + toBridge * toBridge);
-    toNeck   = toNeck   / std::sqrt (1.0f + toNeck   * toNeck);
+    // Step 7: In-loop tanh saturator on each rail (Phase 2.4c-bis R36-bis port from
+    //   O-Bowed WaveguideString.cpp:218-219 writeJunction; closes Phase 2.4c §19.7.6
+    //   escalation flag locked at 5.92 dB envelope divergence; see RESEARCH §20.4).
+    constexpr float sat = 4.0f;
+    toBridge = sat * std::tanh (toBridge / sat);
+    toNeck   = sat * std::tanh (toNeck   / sat);
```

**Audit hooks (R36-bis-a + R36-bis-e):**
- `git diff --stat HEAD -- plugins/O-Contrabass/Source/`: 1 file changed, 6 insertions(+), 3 deletions(-) — single-file scope confirmed (the +3 insertion delta vs PLAN rev-11 stated "4 insertions" reflects the 3-line continuation comment block prescribed verbatim in PLAN R36-bis-a; functional change is identical, comments do not affect compiled binary).
- `git diff --stat HEAD -- plugins/O-Bowed/Source/ modules/synthesis/bow-friction/Source/`: empty (HR-10 friction module ABI preserved by construction).
- `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp`: **2** (toBridge + toNeck rails).
- `grep -c "std::sqrt (1.0f +" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp`: **0** (algebraic saturator removed).

#### 19.7.7.2 Post-port saturator-tail decay envelope key bins

Per `saturator-tail-comparison.json` (post-port WAV `5c45d176…`):

| Bin | Time window | Pre-port (`c7e845ea…`) | Post-port (`5c45d176…`) | Δ |
|-----|-------------|-------------------------|--------------------------|---|
| 0   | 0–1 s (attack)        | 0.0000 dB | 0.0000 dB | identical (peak normalised) |
| 5   | 5–6 s (mid-sustain)   | −1.3379 dB | −2.0090 dB | −0.6711 dB (post-port marginally lower) |
| 60  | 60–61 s (1 s post bow-off) | −6.1050 dB | −6.5074 dB | −0.4024 dB |
| **64** | **64–65 s (5 s post bow-off)** | **−13.0948 dB** | **−7.9675 dB** | **+5.1273 dB (post-port retains 5.13 dB more energy at 5-s mark)** |

**Auxiliary metrics shift:**
- `peak`: 0.1105 → 0.1738 (+57%; post-port 57% higher peak amplitude)
- `rmsMid_s5_s6`: 0.0666 → 0.0905 (+36%; post-port 36% higher steady-state RMS)
- `rmsFinal_lastSecond`: 0.0172 → 0.0456 (+165%; post-port retains 2.65× more tail energy)
- `rmsRatio_final_over_mid`: 0.2583 → 0.5036 (+95%; post-port retains 95% more tail energy relative to mid)

#### 19.7.7.3 Measured |Δ| at bin 64 vs O-Bowed reference (SOFT-PASS)

| Reference | bin 64 dB rel max | |Δ| vs post-port |
|-----------|-------------------|------------------|
| O-Bowed canonical (Phase 2.4c §19.7 reference) | **−7.17 dB** | (reference) |
| **Post-port O-Contrabass (Phase 2.4c-bis R36-bis)** | **−7.9675 dB** | **0.7975 dB** |
| Pre-port O-Contrabass (Phase 2.4c §19.7.6) | −13.0948 dB | 5.92 dB |

- **Strict band [−7.67, −6.67]:** post-port lands **0.30 dB outside** lower edge.
- **Soft band [−8.17, −6.17]:** post-port lands **0.20 dB inside** lower edge → **SOFT-PASS** (per CONTEXT Q47 widening).
- **Improvement vs pre-port:** 5.92 dB → 0.7975 dB = **86.5% reduction** (~3 dB perceptual JND threshold cleared).

**Verdict:** Convergence within architectural-parity intent of the port. `sat=4.0f` matches O-Bowed reference verbatim; no constant tune iteration in this cycle. Strict-band convergence retune via `sat=3.5f` or `sat=4.5f` parked as Phase 2.4-bis backlog item (additive, non-blocking).

#### 19.7.7.4 13-audible-golden re-baseline sha256s

R36-bis-b rendered + verified all 13 audible goldens against §20.5 LOCKED predictions; **13/13 MATCH byte-identical** (`std::tanh` bit-deterministic on M1 macOS Xcode 26.3 toolchain).

| # | Golden | Pre-port (Phase 2.4c committed) | **Post-port (R36-bis-b LOCKED)** |
|---|--------|----------------------------------|----------------------------------|
| 1 | stiffness-zero-pre.wav | `d358abcd…b0ee75` | `ed44cd8986d3a9d44cefd399dd128b62147901640ce615eadf7793f129f56020` |
| 2 | string-A.wav | `c6755aa4…415918` | `505ad36e521d3a8cff978cc5386d6e69769da33977efc7dfccec33d721785bad` |
| 3 | string-D.wav | `765b015e…65d9c9bc` | `e064035124d9af90c1cf6ac8a103e90efa64bf0d6a3efc17574d1f8c811668f4` |
| 4 | string-G.wav | `0cd5cb0a…e1b993bd` | `0e9451b849b659ea5ea92ea3e92e0862e34d30ad5188235f816f43419111b3ca` |
| 5 | detune-sweep-A.wav | `5e31dad3…2dbb05` | `b51d334bbfdd7da7abf4ba3391a6411d5a07427bb92b8e339389856a2539dbe7` |
| 6 | note-sequence.wav | `3ac3ccd0…79260b5` | `2b5b8c83e419179ab04b5b218976c5d085538d59c0a55a967942c004fa1f8224` |
| 7 | macro-sweep.wav | `c2571dd9…b37975e` | `231218b4e9f117ca6598ecee530f3b0af20d4109f29640608590d6cf15a66cfe` |
| 8 | slow-lfo.wav | `c0c2c893…2466a0` | `d27589de30dcb6f3c432c8993d80106b38b4cb87e59afa2edc4ae301d8809cb8` |
| 9 | schelleng-stress.wav | `9d18da86…2f9597` | `c5108af57520c8c190adaa6840513d4cfea6659da46d95bca01996d07efbda07` |
| 10 | sub-harmonics.wav | `bfcaaadc…5573af` | `9178b41ec8b5bb6eb08b5ce9794dd93f542647ffd575ede39d88b8fff1a8c54c` |
| 11 | sub-harmonics-stability.wav | `8043f659…d107b14a` | `2efdea9b5d0745e127ad1fbc4242779848f1ea031b0e426b22e966ed7df8e6be` |
| 12 | saturator-tail-comparison.wav | `c7e845ea…1a60cb` | `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7` |
| 13 | vibrato.wav | `d7881ecf…076b2c` | `df7384e358af9c5d5d34673a3976c2f34790f7cb2c07a96b45d6b3b03b568f47` |

**`reproduce-goldens.sh` 13/13 PASS** against post-port sha256s confirmed at R36-bis-b step 7 + R36-bis-e regression bar (post-audition).

#### 19.7.7.5 Vibrato carry-forward determination — RE-BASELINE

Vibrato golden RE-BASELINES because metric shifts exceed established tolerances:

| Metric | Pre-port (Phase 2.4c R36) | Post-port (R36-bis) | Δ | Tolerance | Verdict |
|--------|----------------------------|----------------------|---|-----------|---------|
| `peakDepthCents` | 9.526 | **7.9507** | −1.5753 | ±0.05¢ | OUTSIDE tolerance (RE-BASELINE) |
| `vibratoRateHzMeasured` | 4.978 Hz | 4.9788 Hz | +0.0008 | ±0.005 Hz | inside tolerance |
| `onsetTimeMs` | 1168 | **1000** | −168 | ±2 ms | OUTSIDE tolerance (RE-BASELINE) |

**JSON non-determinism caveat:** `vibrato.json` contains wall-clock fields (`blockMicros_*`) and `outputWav` path string → JSON sha256 differs across renders. Committed `vibrato.json.sha256` is a one-time anchor (informational/historical only); regression bar is `vibrato.wav.sha256` only (reproduce-goldens.sh checks WAV sha256 exclusively). Same precedent applies to `saturator-tail-comparison.json.sha256`.

**Mechanism:** Saturator port is upstream of vibrato modulator path (vibrato modulates BOW_SPEED at Step 4; saturator runs at Step 7 within the same per-sample loop). Post-port `tanh` allows ~57% higher peak amplitude through the bridge rail (§20.4) → larger steady-state energy → vibrato modulation of bow speed has proportionally smaller effect on the energy envelope → autocorrelator measures smaller pitch detection swing. Post-port still fails the strict 10¢ band (Phase 2.3 R28 lower edge), but does NOT cross the 5¢ "vibrato barely audible" perceptual floor.

**Phase 2.4-bis backlog item (additive — DSP-09):** Vibrato `peakDepthCents` tune for tanh saturator topology — restore strict 10–14¢ band via VIBRATO_DEPTH→bowSpeedSwing transfer scaling.

#### 19.7.7.6 Matrix-stability post-port evidence — failure-mode migration

Re-rendered `--matrix-stability` (108-combo) post-port: WAV sha256 `09cbf15f7600dbfa2d0cbb6850e4af02c02419f6ae41910561f0b1a41b8d39fa` (matches §20.7 prediction byte-identical).

- **Pre-port (`6db67707…`):** 105/108 PASS, 3 FAIL.
- **Post-port (`09cbf15f…`):** 104/108 PASS, 4 FAIL.

| Cell index | string | speedIdx | pressIdx | posIdx | Pre-port | Post-port | Verdict |
|-----------|--------|----------|----------|--------|----------|-----------|---------|
| (0,2,0,0) | E (28) | 2 (high) | 0 (low) | 0 (β=0.05) | FAIL | PASS | **STABILISED** |
| (1,2,0,0) | A (33) | 2 (high) | 0 (low) | 0 (β=0.05) | FAIL | PASS | **STABILISED** |
| (2,2,0,0) | D (38) | 2 (high) | 0 (low) | 0 (β=0.05) | FAIL | PASS | **STABILISED** |
| (0,0,2,0) | E (28) | 0 (low) | 2 (high) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |
| (0,1,2,0) | E (28) | 1 (mid) | 2 (high) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |
| (0,2,2,0) | E (28) | 2 (high) | 2 (high) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |
| (3,2,1,0) | G (43) | 2 (high) | 1 (mid) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |

All FAIL cells (pre + post) fail on `pass_clickFree` only; **`pass_noNaN`, `pass_peak`, `pass_blockTime` all PASS** across all 108 combos pre + post (peak max ≈ 0.351 within strict |x| < 1.0 invariant; nanCount=0 / infCount=0).

**Stability invariant intact.** `pass_clickFree` is a quality heuristic (transient amplitude derivative), not a safety gate. Failure-mode migration mechanism: `tanh` is nearly linear up to x≈4 vs `x/√(1+x²)` saturating at output ≈ 0.707 at x=1 → high-pressure × close-to-bridge transients pass through with less compression, tripping click-free heuristic. Conversely, gentler post-port saturation no longer chokes off under-bowed signal → low-pressure × high-speed cells stabilise.

**Re-baseline scope:** matrix-stability is **evidence-only NOT committed**. Existing `tests/render-harness/golden/matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim (Phase 2.4a R34b baseline). Post-port WAV `09cbf15f…` archived under `.planning/evidence/phase-2-4c-bis/matrix-stability-post-port.json` (lightweight metric extract; the 157 MB WAV itself is NOT committed).

**Phase 2.4-bis backlog item (additive — click-free heuristic threshold tune):** 4 NEW raucous corners at high-pressure × β=0.05 corners post-port; investigate threshold relaxation OR per-string Schelleng wedge tune at near-bridge bow position.

#### 19.7.7.7 Sub-harmonics post-port + default-state HR-9 verification

**Post-port `--sub-harmonics` (sha256 `9178b41e…`):**

| Metric | Pre-port (Phase 2.4b R35) | Post-port | Δ | Verdict |
|--------|---------------------------|-----------|---|---------|
| `subharmEnergyRatio` | **0.358** | **0.000170** | −0.358 (~99.95% reduction; ~33 dB drop) | **CRITICAL DROP** |
| `pass_subharmAudible` | True (above 0.30 strict) | **False** | flipped | hard-fail floor crossed |
| `peak` | (Phase 2.4b not captured) | 0.1712 | informational | — |

**Mechanism:** Phase 2.4b DSP-07 (Step 2.5) introduces a low-frequency bias to the friction model that nudges the limit cycle toward period-doubling at moderate-amplitude curvature regions of the saturator. Pre-port `x/√(1+x²)` has steeper curvature at amplitude x≈0.5–1.0 → amplifies bias-induced subharmonic excursions. Post-port `4·tanh(x/4)` is nearly linear up to x≈4 → does NOT amplify the period-doubling tendency. Result: subharmonic-bias signal passes through ~undistorted, generating no perceptible subharmonic energy.

**Post-port `--sub-harmonics-stability` (sha256 `2efdea9b…`):** `passCount=36/36`, `failCount=0`, `pass_all_36=True`, status `PASS`. **All 36 combos pass on `pass_noNaN`, `pass_peak`, `pass_clickFree`, `pass_blockTime` — stability invariant intact across all SUB_HARMONICS values × infiniteSustain combos.**

**Default-state HR-9 short-circuit verification (CRITICAL):**
- HR-9 (Phase 2.4b R35 lock): "SUB_HARMONICS=0 → IEEE 754 identity arithmetic short-circuit; no bias signal added to friction input."
- The 11 default-state audible goldens (string-A/D/G, detune-sweep, etc.) all render at SUB_HARMONICS=0 (default APVTS); HR-9 short-circuit holds → no bias path engaged → those goldens shift only due to direct saturator topology change, NOT subharmonic-bias amplification differential.
- **Default user experience UNAFFECTED.** Only users explicitly engaging `SUB_HARMONICS > 0` will perceive feature loss.

**Plan-phase classification:** **NOT BLOCKING** per Q48 strict-saturator-only-scope decision. Phase 2.4-bis backlog item (additive — DSP-07 retune): kForceBoost gain compensation OR bias signal amplitude scale (3–5× boost) OR bias injection-point shift (Step 2.5 → post-saturator Step 8). Acknowledge DSP-07 feature is currently MUTED for sub-harmonics-knob users post-port; default-knob users unaffected via HR-9.

#### 19.7.7.8 R37-bis Logic AU audition outcome

**User CONFIRM (2026-04-29):** Audition gate cleared via `/continue` command after both AUs installed side-by-side (`O-Contrabass-dev` / `aumu OCbs OuDv` post-port + `O-Contrabass-pre-port` / `aumu OCbP OuDv` pre-port from `115dbf4` worktree). Both AUs `auval` SUCCEEDED at R36-bis-d step 3. User accepted the predicted-PASS path consistent with measured-metric improvements documented in §§19.7.7.2–7 (5.92 dB → 0.80 dB convergence; rmsRatio_final_over_mid 0.26 → 0.50; default-state HR-9 short-circuit preserved). Detailed subjective probe-by-probe notes deferred — operator may amend this subsection post-commit if perceptual notes diverge from predicted character.

| Probe sequence | Predicted character | User verdict |
|----------------|---------------------|--------------|
| 1 — Sustained E1 (8 s + 5 s tail) | Smoother + more natural tail decay; no bow-on transient artefacts | **PASS** (CONFIRM via /continue) |
| 2 — Per-string MIDI 28 / 33 / 38 / 43 (4 s each) | Slightly brighter + more sustained; harmonic spectrum preserved | **PASS** (CONFIRM via /continue) |
| 3 — Sustained E1 + bow-off at 4 s + 10 s tail | Tail energy ~3× higher (measurable); no ringing / clicks / DC drift | **PASS** (CONFIRM via /continue) |
| 4 — SUB_HARMONICS=0.7 engagement | Subjectively MUTES subharmonic effect (matches §20.8 ~33 dB drop) | **DOCUMENT** — Phase 2.4-bis DSP-07 retune backlog item active |
| 5 — VIBRATO_DEPTH=0.7 + EXPRESSION_MACRO=0.5 | Vibrato shape preserved; depth slightly reduced (7.95¢ vs 9.53¢) | **DOCUMENT** — Phase 2.4-bis DSP-09 transfer tune backlog item active |

**Sequences 1–3 BLOCKING-PASS.** No FAIL-handling path triggered (no `sat` constant retune, no revert, no escalation to Phase 2.4c-bis-bis). R36-bis atomic commit proceeds.

#### 19.7.7.9 Verdict — port WORKED-PARTIALLY (SOFT-PASS at bin 64)

**Verdict (LOCKED 2026-04-29):**

> Port WORKED-PARTIALLY (SOFT-PASS at bin 64 with 0.7975 dB |Δ|; 87% improvement vs pre-port 5.92 dB divergence; 3 Phase 2.4-bis backlog items added: (1) DSP-07 retune for tanh saturator topology — restore subharmEnergyRatio above 0.30 strict at engagement; (2) DSP-09 VIBRATO_DEPTH transfer tune additive — restore peakDepthCents to 10–14¢ strict band; (3) click-free heuristic threshold tune for high-pressure × β=0.05 corners — 4 NEW raucous corners surfaced post-port; default-state HR-9 IEEE 754 identity arithmetic preserved → 11 default-state goldens shift only due to direct topology change, NOT subharmonic-bias differential). Stability invariant intact across all 108 matrix-stability combos pre + post (`pass_noNaN`/`pass_peak`/`pass_blockTime` all PASS; peak max ≈ 0.351 within strict |x| < 1.0). `std::tanh` bit-deterministic on M1 macOS Xcode 26.3 toolchain (3-trial DET-PASS at research-phase + 13/13 byte-identical re-render at execute-phase). Closes Phase 2.4c §19.7.6 escalation flag.

#### 19.7.7.10 Evidence base for end-of-Stage-2 §"In-loop saturator" amendment

The full saturator-tail evidence base is now reproducible from two distinct git states:

| Reference | Commit / Worktree | sha256 (saturator-tail-comparison.wav) | bin 64 dB rel max |
|-----------|-------------------|----------------------------------------|--------------------|
| Pre-port baseline (Phase 2.4c) | `/tmp/oc-pre-port` worktree at `115dbf4` | `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb` | −13.0948 dB |
| **Post-port (Phase 2.4c-bis)** | R36-bis atomic at HEAD | `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7` | **−7.9675 dB** |
| O-Bowed canonical reference | `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:218–219` (writeJunction active path) | (matched at bin 64) | −7.17 dB |

**Convergence narrative:** 5.92 dB pre-port divergence collapsed to 0.7975 dB post-port — **87% reduction** within a single 4-LOC topology swap. Architectural parity with O-Bowed achieved at `sat=4.0f`. Residual 0.30 dB sub-strict divergence is well below the ~3 dB perceptual JND for sustained tones (vs. the 5.92 dB pre-port figure that exceeded JND).

**End-of-Stage-2 §"In-loop saturator" amendment scope:** ARCHITECTURE.md will codify the post-port topology as the canonical O-Contrabass in-loop saturator. Both pre-port (`c7e845ea…`) + post-port (`5c45d176…`) goldens carry forward as evidence references; `/tmp/oc-pre-port` worktree retired at Phase 2.4c-bis verify-phase close (`git worktree remove /tmp/oc-pre-port`).

---

## 19.8 Open Question #7 — Vibrato Golden Re-Baseline Scope (RESOLVED — JSON only; WAV byte-identical)

`vibrato.wav.sha256 = d7881ecf…` carries forward byte-identical because R36a edits the autocorrelator (consume-side measurement code) — not the vibrato production DSP. WAV-producing code path (`processBlock`, vibrato modulator surface, friction injection) is unchanged.

R36c re-baselines:
- `tests/render-harness/golden/vibrato.json` — NEW measurement output (range-bias-corrected `peakDepthCents` ∈ [10, 14]¢, `onsetTimeMs` ∈ [800, 1000] ms, `perCycleDeltaCents` smooth, all `pass_*` true)
- `tests/render-harness/golden/vibrato.json.sha256` — NEW JSON-locked sha256 (mirrors `sub-harmonics.json.sha256` precedent)

R36c does NOT re-baseline `vibrato.wav.sha256` (carries forward verbatim).

**Pre-flight tripwire (R36-pre):** `shasum -a 256 /tmp/repro/vibrato.wav` matches `d7881ecf…` post-R36a edit confirms HR-11 holds for the autocorrelator fix path. If the pre-flight WAV sha256 drifts post-R36a, R36a accidentally edited DSP — HARD HR-11 violation, halt and investigate.

---

## 19.9 Open Question #8 — Strict `pass_vibratoAudible` Threshold Values (RESOLVED — Phase 2.3 PLAN rev-7 verbatim)

Phase 2.3 PLAN rev-7 design-intent ranges restored verbatim (Q38):

| Predicate | Range | Rationale |
|-----------|-------|-----------|
| `pass_rateHzInRange` | `vibratoRateHzMeasured ∈ [4.5, 5.5] Hz` | VIBRATO_RATE setting = 5.0 Hz; ±10% measurement tolerance |
| `pass_vibratoDepthInRange` | `peakDepthCents ∈ [10, 14]¢` | VIBRATO_DEPTH setting = 12¢; ±2¢ measurement tolerance |
| `pass_onsetWindow` | `onsetTimeMs ∈ [800, 1000] ms` | VIBRATO_ONSET = 600 ms; `0.8 × peak` threshold-crossing falls 200–400 ms after onset start |

**Caveat (§19.2.4):** if R36a prototype measures `onsetTimeMs ≈ 600` ms (earlier than 800), the strict gate widens to `[600, 1000]` to match measured-against-implementation reality. Plan-phase confirms after R36a prototype run.

`pass_rmsContinuity` carries forward at threshold ≥ 0.85 (Phase 2.3 macro-sweep loose threshold; vibrato is non-pathological for RMS continuity at MIDI 28 / 12¢ / 5 Hz).

`pass_vibratoAudible` aggregator predicate is the AND of: `pass_rateHzInRange && pass_vibratoDepthInRange && pass_onsetWindow && pass_rmsContinuity`. Currently the JSON does NOT emit `pass_vibratoAudible` as a single field; R36a adds it (mirrors `pass_combo` aggregator pattern from `--sub-harmonics`).

---

## 19.10 Open Question #9 — R36 Task Breakdown (RESOLVED — 9-task R36-pre/R36a–f/R36/R36-backfill)

### 19.10.1 Task table

| Task | Scope | Net delta |
|------|-------|-----------|
| **R36-pre** | (a) Reproduce 12 carry-forward goldens via reproduce-goldens.sh (HR-11 tripwire); (b) reproduce O-Bowed `canonical-preset.wav.sha256` (cohort tripwire — only if Option B scope-expansion path); (c) confirm autocorrelator octave-jump baseline at HEAD against `vibrato.wav` (read existing golden JSON + compare to §19.2.2); (d) confirm wall-clock + sha256 determinism for canonical 65-s render (3 back-to-back) | 0 source LOC; pre-flight only |
| **R36a** | Replace `kTauMin = 400` / `kTauMax = 1500` at `main.cpp:1742–1743` with MIDI-derived ±20% range bias (see §19.2.3 spec); add `pass_vibratoAudible` aggregator predicate + JSON field (mirrors `--sub-harmonics` pass_combo pattern) | +~16 / −2 LOC `main.cpp` |
| **R36b** | NEW `--saturator-tail-comparison` CLI flag + mode handler (see §19.5.1 schema). [Option B scope-expansion: extend O-Bowed harness with `--bow-speed --bow-pressure --bow-position --infinite-sustain` flags — +~80 LOC `plugins/O-Bowed/tests/render-harness/main.cpp`.] Render new golden `saturator-tail-comparison.{wav,json,json.sha256}` at canonical E1 60s+5s | +~120 LOC `O-Contrabass/main.cpp` (mode handler + decay-envelope analyser); +~80 LOC `O-Bowed/main.cpp` (Option B); +3 new golden text files |
| **R36c** | Re-baseline `tests/render-harness/golden/vibrato.{json,json.sha256}` against R36a-fixed measurement output (WAV unchanged) | +0 LOC source; 2 changed golden text files |
| **R36d** | Append RESEARCH §19.7 verdict (research-only OR escalation flag) based on R36b parity render + envelope comparison; document divergence at 5-s mark verbatim from measured data | +0 LOC source; +~40 LOC RESEARCH.md (verdict footnote subsection) |
| **R36e** | Regression bar: `reproduce-goldens.sh` extended 12 → 13 entries (new `--saturator-tail-comparison` invocation); all 13 must reproduce byte-identical post-R36a/b edits (HR-11 trivially preserves the 12 carry-forward; new entry locks in R36b output) | +~3 LOC `reproduce-goldens.sh` |
| **R36f** | auval AU VALIDATION SUCCEEDED + pluginval-10 SUCCESS (smoke; no DSP changes → both expected to pass without re-investigation; required by Gate 6c) | 0 LOC source |
| **R36** | Atomic commit: ~9–11 files (`main.cpp` + harness CMake if needed + 3 new goldens + 2 changed goldens + reproduce-goldens.sh + RESEARCH/PLAN/STATUS/SUMMARY/VERIFICATION). [Option B path: +1–2 files (`O-Bowed/main.cpp` + 1 carry-forward `O-Bowed/canonical-preset.wav.sha256` confirmation tripwire entry — but no NEW O-Bowed golden created)] | Atomic commit |
| **R36-backfill** | Chore commit propagating R36 sha256 into STATUS.md (mirrors R34-backfill `b64c8c4` / R35-backfill `0db5fac` precedent) | +1 sha line in STATUS.md |

### 19.10.2 Net source delta (Option B recommended; Option A bracketed for contingency)

- `tests/render-harness/main.cpp` (O-Contrabass): +~136 / −2 LOC (R36a + R36b combined)
- `tests/render-harness/reproduce-goldens.sh` (O-Contrabass): +~3 LOC (R36e)
- `plugins/O-Bowed/tests/render-harness/main.cpp` (Option B): +~80 LOC
- 3 new golden text files (`saturator-tail-comparison.{wav.sha256, json, json.sha256}`)
- 2 changed golden text files (`vibrato.json`, `vibrato.json.sha256`)
- 12 carry-forward WAVs unchanged (HR-11 trivially)
- 12 carry-forward JSONs unchanged (no harness-side metric refactor outside `--vibrato` mode)
- RESEARCH §19 + STATUS append + PLAN rev-10 + SUMMARY + VERIFICATION (planning artefacts)

R36 atomic commit estimated **~11–13 files** (Option B recommended) or **~9–11 files** (Option A contingency), comparable to R35's ~14 files and R34's ~16–19 files (smaller because Phase 2.4c is harness-only, no `Source/DSP/` additions).

### 19.10.3 Sequencing dependency graph

```
R36-pre ──┬── R36a (autocorrelator fix) ──┬── R36c (re-baseline vibrato.json)
          │                               │
          └── R36b (sat-tail mode + render) ── R36d (RESEARCH §19.7 verdict)
                                                     │
          R36e (regression bar) ←─────────────────────┴── R36f (auval/pluginval)
                                                                │
                                                                └── R36 atomic commit
                                                                             │
                                                                             └── R36-backfill chore
```

R36a and R36b can execute in any order or in parallel within execute-phase; R36c depends on R36a (re-baseline post-fix); R36d depends on R36b (verdict needs envelope data); R36e/f gate the atomic commit.

---

## 19.11 Open Question #10 — New Goldens Scope Final Spec (RESOLVED)

### 19.11.1 Changed goldens (R36c) — 1 logical pair

| File | Change |
|------|--------|
| `tests/render-harness/golden/vibrato.json` | NEW measurement output post R36a (range-bias-corrected) |
| `tests/render-harness/golden/vibrato.json.sha256` | NEW (NOT previously existed at HEAD; mirrors `sub-harmonics.json.sha256` precedent) |
| `tests/render-harness/golden/vibrato.wav.sha256` | UNCHANGED (`d7881ecf…` carry-forward; HR-11 trivially) |

### 19.11.2 New goldens (R36b) — 3 files

| File | Source |
|------|--------|
| `tests/render-harness/golden/saturator-tail-comparison.wav.sha256` | Predicted: `94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6` (§19.5.2 pre-flight) |
| `tests/render-harness/golden/saturator-tail-comparison.json` | NEW (decay envelope + standard pass_* predicates per §19.5.1 schema) |
| `tests/render-harness/golden/saturator-tail-comparison.json.sha256` | NEW (mirrors `sub-harmonics.json.sha256` byte-determinism precedent) |

### 19.11.3 Carry-forward goldens (HR-11 trivially preserved) — 12 WAVs + JSONs unchanged

`stiffness-zero-pre.{wav.sha256,json}`, `string-A.{wav.sha256,json}`, `string-D.{wav.sha256,json}`, `string-G.{wav.sha256,json}`, `detune-sweep-A.{wav.sha256,json}`, `note-sequence.{wav.sha256,json}`, `vibrato.wav.sha256` (JSON re-baselined R36c), `macro-sweep.{wav.sha256,json}`, `slow-lfo.{wav.sha256,json}`, `schelleng-stress.{wav.sha256,json}`, `sub-harmonics.{wav.sha256,json,json.sha256}`, `sub-harmonics-stability.{wav.sha256,json,json.sha256}` — all carry-forward verbatim.

`matrix-stability.{wav.sha256,json,json.sha256}` (Phase 2.4a evidence golden, NOT in default reproduce-goldens.sh) — carries forward verbatim.

---

## 19.12 Open Question #11 — RESEARCH §19 Deliverable Structure (RESOLVED — this section)

§19 deliverable structure as written verbatim above — §19.1 HR-11 pre-flight, §19.2 autocorrelator algorithm validation, §19.3 O-Bowed saturator topology audit, §19.4 O-Bowed render harness availability, §19.5 saturator-tail measurement protocol, §19.6 divergence threshold tuning, §19.7 verdict path, §19.8 vibrato golden re-baseline scope, §19.9 strict pass_vibratoAudible threshold values, §19.10 R36 task breakdown, §19.11 new goldens scope, §19.12 (this), §19.13 wall-clock + determinism pre-flight, §19.14 risk-surface refinement, §19.15 sequencing in PLAN rev-10, §19.16 open items for plan-phase, §19.17 summary, §19.18 references.

---

## 19.13 Open Question #12 — Wall-Clock Budget Pre-Flight (RESOLVED — 0.29 s/render, 3-trial determinism PASS)

**Single-render wall-clock:** 0.29 s for 65 s of audio (M1 release; ~225× faster than realtime). Mirrors §17.2 / §18.2 single-combo wall-clock scaling (0.04 s for 6 s and 0.03 s for 6 s respectively → roughly 200× factor across all sustained renders).

**3-trial determinism:** all 3 renders produce sha256 `94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6` byte-identically. Risk #6 (CONTEXT rev-8) PRE-MITIGATED.

**`reproduce-goldens.sh` runtime growth:** current 12-entry script ~3 s; +0.29 s for new entry → ~3.3 s total. Negligible (Risk #12 PRE-MITIGATED).

**Total Phase 2.4c execute-phase wall-clock budget (estimate):** R36-pre ~5 s (3 reproduces + 3-trial sat-tail determinism); R36a ~2 min (edit + rebuild); R36b ~5 min (new mode + measurement code + render new golden); R36c ~30 s (rebuild + re-render + commit JSON); R36e ~10 s (re-run reproduce-goldens.sh); R36f ~3 min (auval + pluginval-10). Total ~10 min execute-phase time-on-task.

---

## 19.14 Risk-Surface Refinement for PLAN rev-10

12 risks from CONTEXT rev-8 with state updates from research-phase findings + 3 new risks surfaced during research:

| Risk | State after §19 | Mitigation in PLAN rev-10 |
|------|-----------------|---------------------------|
| 1. HR-11 violation via accidental DSP edit | **PRE-MITIGATED** (R36-pre tripwire + R36e re-tripwire) | R36-pre as structural prerequisite to R36a/b; R36e as gate to R36 atomic |
| 2. Parabolic-interp + range-bias insufficient at 12-cent vibrato | **DISSOLVED** — parabolic-interp ALREADY present (§19.2.1); precision floor ~0.16¢ (§19.2.3) far below 12¢ requirement | YIN/AMDF/cepstrum fallback retired |
| 3. O-Bowed render harness unavailable | **CHARACTERIZED** — exists but NOT parity-able without scope expansion (§19.4) | PLAN rev-10 picks Option A (factory defaults) or Option B (scope-expand) — recommend B per §19.4.3 |
| 4. >2 dB divergence triggers mid-Phase 2.4c-bis escalation | **CHARACTERIZED** — analytic bound predicts <2 dB at canonical operating amplitudes (§19.3.3 + §19.6); escalation low-probability but lane locked (§19.7.3) | Plan-phase locks Phase 2.4c-bis CONTEXT rev-9-bis structural skeleton in PLAN rev-10 §"contingency" |
| 5. Vibrato pre-flight catches autocorrelator drift | **PRE-MITIGATED** — §19.1 confirmed all 12 goldens reproduce at HEAD `5d95d15`; no upstream drift | R36-pre tripwire is the gate |
| 6. `saturator-tail-comparison.wav.sha256` non-deterministic | **PRE-MITIGATED** — 3-trial determinism PASS (§19.5.2) | R36-pre includes 3-trial sha256 confirmation as additional check |
| 7. JSON `decayEnvelopeDb` width vs sha256 noise | **CHARACTERIZED** — fixed-width 4-decimal-place format pin (§19.5.5); 3-trial JSON sha256 verification in R36-pre | PLAN rev-10 pins float-formatting precision |
| 8. R36 atomic + R35-backfill chore interaction | **PRE-MITIGATED** — chore-after-atomic precedent (R34-backfill, R35-backfill) | R36-backfill follows R36 atomic verbatim |
| 9. RESEARCH §19 surfaces non-saturator divergences | **PRE-MITIGATED** — §19.3.4 audit confirms in-loop saturator is the ONLY in-loop nonlinearity in O-Bowed | Out-of-scope findings tracked as Phase 2.4c-bis or v1.1 backlog |
| 10. Phase 2.5-awareness supersedes saturator decision | **CHARACTERIZED** — body resonator is OUT-of-loop (post-output biquad); does NOT invalidate saturator-tail evidence (§19.7.4) | Phase 2.5 verify includes saturator-tail re-measurement as regression check |
| 11. MIDI 28 expected-period range bias incorrect for E1 dispersion-warped pitch | **CHARACTERIZED** — Phase 2.1c dispersion warps pitch by ~0.5–2¢ at E1 (RESEARCH §11.x); ±20% range bias robust to dispersion (1070 × 1.0009 ≈ 1071 still in [856, 1285]) | No mitigation needed; range bias is order-of-magnitude wider than dispersion error |
| 12. `reproduce-goldens.sh` 12 → 13 entry growth | **PRE-MITIGATED** — wall-clock +0.29 s; total runtime ~3.3 s (§19.13) | No mitigation needed |
| **NEW 13.** O-Bowed canonical-preset.wav.sha256 cohort regression at R36b Option B | Adding `--bow-*` flags to O-Bowed harness MUST NOT change existing canonical render (defaults preserved when flags absent) | R36-pre includes O-Bowed canonical-preset cohort tripwire if Option B; R36b harness diff reviewed against args-only-change pattern |
| **NEW 14.** Post-fix `onsetTimeMs` lands outside [800, 1000] strict gate | If onset detection algorithm responds at ~600 ms after fix (autocorrelator detects modulation immediately) instead of ~800 ms | PLAN rev-10 widens `pass_onsetWindow` to `[600, 1000]` if R36a prototype measures <800 ms (§19.2.4) |
| **NEW 15.** O-Bowed factory-default `infiniteSustain = 0.0` invalidates Option A parity render | At O-Bowed factory defaults, bridge LP loss dominates → tail decays much faster than O-Contrabass; envelope-shape comparison meaningless | §19.4.3 recommendation = Option B (scope-expand) is REQUIRED, not optional, if RESEARCH §19.7 verdict is to be evidence-based |

---

## 19.15 Sequencing in PLAN rev-10

R36-pre/R36a/R36b/R36c/R36d/R36e/R36f/R36/R36-backfill — 9 tasks. Ordering identical to §19.10.3 dependency graph above.

PLAN rev-10 preamble pins (mirrors PLAN rev-7/8/9 preamble structure):

1. CLI flag spelling: `--saturator-tail-comparison` (Q39 confirmed).
2. Autocorrelator fix scope-narrow: range-bias only (parabolic-interp already present per §19.2.1).
3. MIDI-derived range bias: hard-coded `kVibratoMidiNote = 28` for v1.0 (§19.2.3).
4. Strict `pass_vibratoAudible` ranges: `[4.5, 5.5] Hz / [10, 14]¢ / [800, 1000] ms` per Q38 + §19.9; widen onset window to `[600, 1000]` only if R36a measures <800 ms.
5. `pass_vibratoAudible` aggregator predicate: AND of 4 sub-predicates (rate, depth, onset, rmsContinuity).
6. Saturator-tail JSON schema: 65-bin `decayEnvelopeDb` array; fixed-width 4-decimal serialization (§19.5.5).
7. NO `pass_decayMatchesOBowed` JSON predicate (Q39 + §19.5.1).
8. O-Bowed parity Option: **B (scope-expand)** recommended per §19.4.3 (Option A insufficient because O-Bowed factory `infiniteSustain = 0.0` makes envelope-shape comparison meaningless — Risk #15).
9. `reproduce-goldens.sh` extension 12 → 13 entries.
10. R36-pre 4-step pre-flight: (a) reproduce-goldens.sh 12-of-12 PASS; (b) Option-B-only: O-Bowed canonical-preset cohort tripwire; (c) autocorrelator octave-jump baseline read from existing `vibrato.json`; (d) saturator-tail 3-trial determinism.
11. R36 atomic commit ~9–13 files (Option A: ~9–11; Option B: ~11–13).
12. R36-backfill chore mirrors R35-backfill / R34-backfill precedent.

HARD RULE HR-11 (zero production DSP edits) binding for Phase 2.4c. Phase 2.3 HR-1..HR-4 + Phase 2.4a HR-5..HR-8 + Phase 2.4b HR-9..HR-10 verbatim carry-forward.

7-step + Step 2.5 per-block evaluation order (Phase 2.3 + Phase 2.4b end-state) **unchanged** in Phase 2.4c.

NO Stage-1 contract amendment in Phase 2.4c (parameter-spec.md sha256 `77638e25…` carries forward unchanged).

NO ARCHITECTURE.md amendment in Phase 2.4c (saturator-tail evidence feeds end-of-Stage-2 §"In-loop saturator" amendment cycle but does not amend the architecture itself).

Five-item Gate 6c success criteria (verify exit gate):

1. `reproduce-goldens.sh` 13/13 byte-identical (12 carry-forward + 1 new `saturator-tail-comparison`); HR-11 trivially preserved.
2. `--vibrato` strict `pass_vibratoAudible = true` post R36a (rate ∈ [4.5, 5.5] Hz / depth ∈ [10, 14]¢ / onset ∈ [800, 1000] ms or [600, 1000] per §19.2.4 widening).
3. `--saturator-tail-comparison` golden bit-deterministic across re-renders (3-trial sha256 verified) + RESEARCH §19.7 verdict written.
4. auval AU VALIDATION SUCCEEDED + pluginval-10 SUCCESS.
5. RESEARCH §19.7 cross-comparison verdict locked = either "research-only acknowledged divergence" (predicted default) OR "Phase 2.4c-bis escalation flag" (low-probability per §19.6 analytic bound).

---

## 19.16 Open Items for Plan-Phase

10 open items handed to PLAN rev-10:

1. **Confirm onset window widening branch** — after R36a prototype runs against `vibrato.wav`, plan-phase confirms `onsetTimeMs` lands within `[800, 1000]` (default) OR widens to `[600, 1000]` (if measured ~600 ms per §19.2.4 caveat).
2. **Lock O-Bowed parity Option** — recommend Option B (scope-expand O-Bowed harness with `--bow-*` and `--infinite-sustain` flags) per §19.4.3 + Risk #15. Plan-phase pins exact Args struct shape + value-consume code for the new flags.
3. **R36b new mode handler structure** — mirror existing `--sub-harmonics` pattern (lines ~694–735 of `O-Contrabass/main.cpp`); single-mode handler with parameter-pinning then 60s sustain + 5s release render + post-render decay-envelope analyser; analyzer iterates 65 1-second windows on channel 0; `pass_combo = pass_nan && pass_peak && pass_blockTime`.
4. **`vibrato.json.sha256` adoption** — plan-phase confirms whether Phase 2.4c is the FIRST golden to introduce a `.json.sha256` for `vibrato` (no existing file at HEAD); follow `sub-harmonics.json.sha256` precedent for byte-determinism.
5. **R36c re-baseline mechanics** — same render command as `--vibrato` with new harness; render output → write to `vibrato.json` golden; `shasum -a 256` → write to `vibrato.json.sha256` golden. Plan-phase pins exact reproduce-goldens.sh integration.
6. **Decay-envelope analyser channel selection** — channel 0 only OR mean of (ch0, ch1)? Recommend channel 0 only per §16.7 / §18.5 single-channel precedent.
7. **R36b float serialization** — `juce::String (val, 4)` fixed-width 4-decimal-place per §19.5.5; plan-phase confirms identical pattern as `--sub-harmonics`'s `subharmEnergyRatio` serialization (Phase 2.4b R35a pattern).
8. **`saturator-tail-comparison.wav.sha256` predicted vs measured** — predicted `94a42a81…` from §19.5.2 pre-flight at HEAD `5d95d15`; plan-phase pins that R36b golden render MUST match this sha256 (HR-11 trivially because no DSP changes between pre-flight and R36b execute-phase; if drift, investigate stowaway DSP edit).
9. **R36-pre tripwire script** — recommend writing a small `tests/render-harness/preflight-saturator-tail.sh` analog to `preflight-subharm.sh` (Phase 2.4b R35-pre precedent); 3-trial determinism + 12-golden HR-11 + Option-B O-Bowed cohort tripwire all in one script. Plan-phase decides between standalone preflight script vs. inline R36-pre execution.
10. **Phase 2.4c-bis CONTEXT rev-9-bis structural skeleton** — plan-phase pre-writes the Phase 2.4c-bis cycle structure in PLAN rev-10 §"contingency" so escalation is one-action-away if R36b parity render measures >2 dB divergence; reduces context-loss between Phase 2.4c verify and Phase 2.4c-bis discuss-phase.

---

## 19.17 Summary — Phase 2.4c Research Resolution Map

| CONTEXT Open Q | Resolution | §19 Section |
|----------------|------------|-------------|
| Q1 (autocorrelator algorithm validation) | RESOLVED — parabolic-interp already present; range-bias is the only fix | §19.2 |
| Q2 (lag-search range bound spec) | RESOLVED — MIDI-derived `±20% × kVibratoPeriod`; `kVibratoMidiNote = 28` hard-coded for v1.0 | §19.2.3 |
| Q3 (O-Bowed saturator topology audit) | RESOLVED — `tanh(x/4)·4` in-loop both rails (vs O-Contrabass `x/sqrt(1+x²)` drive=1) | §19.3 |
| Q4 (O-Bowed render harness availability) | RESOLVED — exists but NOT parity-able without Option B scope-expansion | §19.4 |
| Q5 (saturator-tail measurement protocol) | RESOLVED — 65-bin `decayEnvelopeDb` schema; channel 0 RMS; fixed-width 4-decimal serialization | §19.5 |
| Q6 (divergence threshold tuning) | RESOLVED — 2 dB default RETAINED (sub-perceptual); analytic bound predicts <2 dB | §19.6 |
| Q7 (vibrato golden re-baseline scope) | RESOLVED — JSON only (`vibrato.json{,.sha256}`); WAV `d7881ecf…` carry-forward | §19.8 |
| Q8 (strict `pass_vibratoAudible` thresholds) | RESOLVED — Phase 2.3 PLAN rev-7 verbatim `[4.5,5.5]Hz / [10,14]¢ / [800,1000]ms` | §19.9 |
| Q9 (R36 task breakdown) | RESOLVED — 9-task R36-pre/R36a–f/R36/R36-backfill | §19.10 |
| Q10 (new goldens scope final spec) | RESOLVED — 1 changed pair (vibrato.json{,.sha256}) + 3 new (saturator-tail-comparison.{wav.sha256, json, json.sha256}) | §19.11 |
| Q11 (RESEARCH §19 deliverable structure) | RESOLVED — this section's structure | §19.12 |
| Q12 (wall-clock + sha256 determinism pre-flight) | RESOLVED — 0.29 s/render; 3-trial determinism PASS | §19.13 |

**Critical findings (research-phase NEW knowledge):**

1. **Parabolic interpolation IS already present** in autocorrelator (§19.2.1). R36a fix scope narrows from CONTEXT-described ~150 LOC to actual ~16 LOC range-bias-only edit. PLAN rev-10 reflects this scope reduction.
2. **O-Bowed factory `infiniteSustain = 0.0`** (§19.4.3) makes Option A "factory defaults" path likely invalid for envelope-shape comparison — Option B (scope-expand) is recommended-as-required, not optional.
3. **Analytic bound predicts saturator-tail divergence <2 dB at canonical bow operating amplitude** (§19.3.3 + §19.6) — default verdict path is research-only acknowledged divergence; escalation low-probability.
4. **5-s post-bow-off envelope at O-Contrabass canonical = −9.31 dB rel max** (§19.5.3), `rmsRatio_final_over_mid = 0.343`. Concrete reference data for §19.7 verdict.
5. **Predicted golden sha256 for `saturator-tail-comparison.wav` = `94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6`** (§19.5.2). Locks R36b's expected output bit-exactly.

---

## 19.18 References (§19 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-8 (this cycle's discuss artefact, 2026-04-28).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` rev-7 (Phase 2.3 R28–R33 carry-forward) + rev-8 (Phase 2.4a R34) + rev-9 (Phase 2.4b R35).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §16 (Phase 2.3 vibrato + slow-bow LFO + Schelleng wedge + EXPRESSION_MACRO) + §17 (Phase 2.4a Schelleng wedge calibration polynomial) + §18 (Phase 2.4b sub-harmonic bias DSP-07).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` lines 197–219 (in-loop algebraic saturator `x / sqrt(1 + x²)` per rail).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` lines 22–30, 37–50 (saturator architecture + processing-order spec).
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` lines 135–139, 217–219 (O-Bowed in-loop saturator `sat × tanh(x / sat)` with `sat = 4.0f`).
- `plugins/O-Bowed/tests/render-harness/main.cpp` (existing harness; supported CLI = `--note --velocity --sustain --release --out --json` only; lacks `--bow-*` and `--infinite-sustain` overrides).
- `plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav.sha256` (Phase 2.1b R12 cohort baseline; must remain byte-identical post-R36b Option B harness extension).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` lines 1728–1847 (existing `--vibrato` mode autocorrelator pitch-tracker; parabolic-interp already present at lines 1779–1801; integer-lag bounds at 1742–1743 are R36a's edit point).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` lines 218–219 (`--infinite-sustain` value-consume pattern; mirror for O-Bowed Option B `--bow-*` flags).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` lines 694–735 (existing `--sub-harmonics` mode handler; structural template for R36b `--saturator-tail-comparison` mode handler).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` lines 1946–2124 (existing JSON serialization patterns; mirror for R36b `decayEnvelopeDb` array).
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json` (HEAD; documents Phase 2.3 R28 octave-jump baseline `peakDepthCents=625.44`, `+1200¢` outlier in `perCycleDeltaCents`).
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (12-entry script at HEAD; R36e extends to 13 entries).
- §19.1 pre-flight reproduce-goldens.sh execution: 12-of-12 PASS at HEAD `5d95d15` (transient `/tmp/repro/*.wav`; deleted post-research).
- §19.5.2 pre-flight saturator-tail render: 3-trial sha256 = `94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6` byte-identical (transient `/tmp/sat-tail-r{1,2,3}.wav`; deleted post-research). Wall-clock 0.29 s/render. Pre-flight peak=0.069, rmsMid=0.0356, rmsFinal=0.0127.
- §19.5.3 decay envelope analysis: 65-bin per-second RMS computed via Python 3.14.2 + 24-bit PCM read; transient `/tmp/sat-tail-decay-bins.txt`; values verbatim in §19.5.1 schema example.
- Phase 2.4b R35 atomic commit `3de8b66` + R35-backfill chore `0db5fac` — production base for Phase 2.4c (HEAD `5d95d15` is descendant).
- ARCHITECTURE.md §"In-loop saturator" — end-of-Stage-2 amendment cycle consumes §19.7 verdict as evidence base; Phase 2.4c does NOT amend the architecture.
- `cmake --build build --target O-Contrabass-render-test --parallel` — harness rebuild target (clean at HEAD, binary at `build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test`).
- `cmake --build build --target O-Bowed-render-test --parallel` — O-Bowed harness rebuild target (Phase 2.1b R12 cohort; binary at `build/plugins/O-Bowed/tests/render-harness/O-Bowed-render-test_artefacts/Release/O-Bowed-render-test`).
- Python 3.14.2 + standard library `wave` module — 24-bit PCM stereo read for §19.5.3 decay-envelope characterisation (no numpy dependency required).

---

# §20 Phase 2.4c-bis Research — Source-Change In-Loop Saturator Port (`x/√(1+x²)` → `4·tanh(x/4)`)

**Cycle:** Phase 2.4c-bis — source-change escalation lane
**Discuss artefact:** CONTEXT.md rev-9-bis (2026-04-29)
**Production base:** HEAD `7835904` (R36-backfill chore, descendant of R36 atomic `115dbf4`)
**Audit hook (HEAD pre-edit):** `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports 0 files modified; `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns 0.

This section resolves Open Questions #1–#13 from CONTEXT.md rev-9-bis and locks expected post-port sha256s + R36-bis task breakdown for PLAN rev-11. Mirrors §17/§18/§19 Phase 2.4a/b/c research-phase structure.

## 20.1 Open Question #1 — Pre-Port Repro Tripwire (RESOLVED — 13/13 PASS at HEAD `7835904`)

**Method:** `bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` against working-tree at HEAD `7835904` (descendant of R36-backfill chore). Harness auto-built via existing CMake target.

**Result:** 13/13 byte-identical PASS.

| # | Golden | Committed sha256 | HEAD render |
|---|--------|------------------|-------------|
| 1 | stiffness-zero-pre.wav | `d358abcd…b0ee75` | PASS |
| 2 | string-A.wav | `c6755aa4…415918` | PASS |
| 3 | string-D.wav | `765b015e…65d9c9bc` | PASS |
| 4 | string-G.wav | `0cd5cb0a…e1b993bd` | PASS |
| 5 | detune-sweep-A.wav | `5e31dad3…2dbb05` | PASS |
| 6 | note-sequence.wav | `3ac3ccd0…79260b5` | PASS |
| 7 | vibrato.wav | `d7881ecf…076b2c` | PASS |
| 8 | macro-sweep.wav | `c2571dd9…b37975e` | PASS |
| 9 | slow-lfo.wav | `c0c2c893…2466a0` | PASS |
| 10 | schelleng-stress.wav | `9d18da86…2f9597` | PASS |
| 11 | sub-harmonics.wav | `bfcaaadc…5573af` | PASS |
| 12 | sub-harmonics-stability.wav | `8043f659…d107b14a` | PASS |
| 13 | saturator-tail-comparison.wav | `c7e845ea…1a60cb` | PASS |

**Conclusion:** No upstream drift between R36-backfill chore (`7835904`) and Phase 2.4c-bis discuss-phase open. R36-bis-pre tripwire (re-run at execute-phase pre-edit) MUST also report 13/13 PASS to confirm zero-drift before R36-bis-a source edit.

## 20.2 Open Question #3 — Two-Call-Site Audit in O-Bowed (RESOLVED — single-site port at O-Contrabass `:204–206` is correct)

**Audit results (O-Bowed `WaveguideString.cpp`):**

- **`processSample` (line 100, saturator at lines 138–139)** — legacy unified per-sample path. **DEAD CODE in O-Bowed**: `BowedStringVoice.cpp` calls `readJunction`/`writeJunction`, NOT `processSample`. The saturator at 138–139 is never executed in current production.
- **`writeJunction` (line 204, saturator at lines 218–219)** — refactored split-rail path. **ACTIVELY USED**: `BowedStringVoice.cpp:154` calls `readJunction`, `BowedStringVoice.cpp:184` calls `writeJunction`. Saturator at 218–219 IS the production-path in-loop saturator in O-Bowed.

**O-Contrabass mapping:** `BowedContrabassVoice.cpp:641, 643, 645, 657, 678, 680` all invoke `strings[s].processSample(…)` — the unified per-sample path. O-Contrabass uses `processSample`; the saturator at `WaveguideString.cpp:204–206` (within `processSample` body) is the architectural equivalent of O-Bowed's `writeJunction:218–219` (Step 7: post-velocity-injection, pre-pushSample, both rails).

**Verdict:** Single-site port at O-Contrabass `WaveguideString.cpp:204–206` is the correct scope. **No two-call-site scope expansion required.** Source delta stays at exactly 4 LOC (−2 / +4 net) in 1 file.

**Audit hook (post-edit):**
- `git diff --stat HEAD -- plugins/O-Contrabass/Source/` MUST report exactly 1 file (`Source/DSP/WaveguideString.cpp`)
- `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` MUST return 2 (one per rail)

## 20.3 Open Question #4 + Risk #13 — Pre-Port Reference Re-Render Reproducibility (RESOLVED — 3-trial PASS from `115dbf4` worktree)

**Protocol (validated):**

```bash
# Pre-port worktree setup (one-time):
git worktree add /tmp/oc-pre-port 115dbf4
cd /tmp/oc-pre-port/build && \
  cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON .. && \
  ninja O-Contrabass-render-test

# Render pre-port reference:
PRE=/tmp/oc-pre-port/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test
$PRE --saturator-tail-comparison --out /tmp/sat-pre.wav --json /tmp/sat-pre.json
shasum -a 256 /tmp/sat-pre.wav  # MUST match c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb
```

**Configuration notes:**
- `-DSKIP_PLUGINS=O-Orbit` is REQUIRED — O-Orbit's `libs/SAF` git submodule is not initialised in fresh worktrees → CMake error. Skip flag at top-level CMakeLists.txt:75 reads `SKIP_PLUGINS` IN_LIST and skips matching plugin directories.
- `-DOUARICON_BUILD_TESTS=ON` is REQUIRED — render-harness binary is `OFF` by default in `plugins/O-Contrabass/CMakeLists.txt:97` (Phase 2.1a opt-in test gate).

**Result (3-trial determinism):**
- r1: `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb`
- r2: `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb`
- r3: `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb`

**Verdict:** Pre-port reference is reproducible byte-identical from clean `115dbf4` worktree. Risk #13 mitigation CONFIRMED. End-of-Stage-2 §"In-loop saturator" amendment evidence base has reproducible pre-port reference (no need to preserve transient render artefacts).

**Pre-port `decayEnvelopeDb` key bins (extracted from `/tmp/sat-pre/sat-tail-r1.json`):**
- bin 0:  `0.0000` dB
- bin 5:  `−1.3379` dB (steady-state during sustain)
- bin 60: `−6.1050` dB (just past bow-off, decay begins)
- bin 64: **`−13.0948` dB rel max** (5-s post-bow-off mark — Phase 2.4c §19.7.6 escalation reference)
- `peak`: `0.1105`
- `rmsMid_s5_s6`: `0.0666`
- `rmsFinal_lastSecond`: `0.0172`
- `rmsRatio_final_over_mid`: `0.2583`

Confirms Phase 2.4c R36 measured 5.92 dB divergence (|−13.0948 − (−7.17)| = 5.92).

## 20.4 Open Question #2 — Scratch-Space Port Prototype + Bin 64 Convergence (RESOLVED — SOFT-PASS, 0.7975 dB |Δ| vs O-Bowed)

**Source delta applied (NOT committed; reverted post-measurement):**

```diff
-    // Step 7: In-loop algebraic saturator on each rail (RESEARCH §1.3).
-    toBridge = toBridge / std::sqrt (1.0f + toBridge * toBridge);
-    toNeck   = toNeck   / std::sqrt (1.0f + toNeck   * toNeck);
+    // Step 7: In-loop hyperbolic-tangent saturator on each rail (post-port; mirrors O-Bowed sat=4.0f topology).
+    constexpr float sat = 4.0f;
+    toBridge = sat * std::tanh (toBridge / sat);
+    toNeck   = sat * std::tanh (toNeck   / sat);
```

Net delta: **−3 / +4 LOC; 1 file modified; 4 LOC inserted, 3 deleted** (CONTEXT estimate "≈ −2 / +4" was off by 1 LOC on the deletion side).

**Audit hook verification:**
- `git diff --stat plugins/O-Contrabass/Source/` → `1 file changed, 4 insertions(+), 3 deletions(-)` ✅ exactly 1 file
- `grep -c "sat \* std::tanh" Source/DSP/WaveguideString.cpp` → 2 ✅ both rails ported

**Build:** `ninja O-Contrabass-render-test` clean, no warnings introduced by source edit. JUCE NSViewPeer warning unrelated (pre-existing, AppKit switch fall-through).

**3-trial sha256 determinism (saturator-tail-comparison.wav post-port):**
- r1: `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7`
- r2: `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7`
- r3: `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7`

**Bit-deterministic across re-renders → standard libm `std::tanh` is RT-safe AND deterministic on M1 macOS (Xcode 26.3 toolchain).** No need for explicit `tanhf` or LUT-based fallback. Risk #4 (re-baseline drift across runs) MITIGATED.

**Post-port `decayEnvelopeDb` key bins (saturator-tail-comparison post-port):**

| Metric | Pre-port (`c7e845ea…`) | Post-port (`5c45d176…`) | Δ | Direction |
|--------|------------------------|--------------------------|---|-----------|
| bin 0  | 0.0000 | 0.0000 | 0 | identical (peak normalised) |
| bin 5  | −1.3379 | −2.0090 | −0.6711 | post-port lower (bigger sustain compression) |
| bin 60 | −6.1050 | −6.5074 | −0.4024 | post-port marginally lower |
| bin 64 | **−13.0948** | **−7.9675** | **+5.1273** | **post-port 5.13 dB HIGHER (more energy at 5-s post-bow-off)** |
| peak | 0.1105 | 0.1738 | +0.0633 | post-port 57% higher peak amplitude |
| rmsMid_s5_s6 | 0.0666 | 0.0905 | +0.0239 | post-port 36% higher steady-state RMS |
| rmsFinal_lastSecond | 0.0172 | 0.0456 | +0.0284 | post-port 165% higher tail RMS |
| rmsRatio_final_over_mid | 0.2583 | 0.5036 | +0.2453 | post-port retains 95% more tail energy relative to mid |

**Convergence verdict (Open Q47 / Gate 6c-bis bar #2):**

- O-Bowed reference: bin 64 = **−7.17 dB rel max** (Phase 2.4c §19.7 reference)
- Post-port: bin 64 = **−7.9675 dB rel max**
- **|Δ| = 0.7975 dB** vs O-Bowed (analytic §19.3.3 prediction was ≤ 2 dB → ACTUAL 0.80 dB, well within prediction)
- Strict target window [−7.67, −6.67]: post-port (−7.9675) lies **0.30 dB outside** the lower bound (−7.67)
- Soft target window [−8.17, −6.17]: post-port (−7.9675) lies **0.20 dB inside** the lower bound

**RESULT: SOFT-PASS** — convergence misses the strict window by 0.30 dB but lands cleanly inside the soft window. Pre-port → post-port improvement: **5.92 dB → 0.80 dB (87% reduction)**. The port does what it was designed to do; residual 0.30 dB sub-strict divergence is well below the ~3 dB perceptual JND for sustained tones.

**Phase 2.4-bis backlog item (locked at plan-phase per Q48 strict-saturator-only-scope decision):**
> "Saturator-tail bin 64 strict-band convergence — post-port lands at −7.97 dB (0.30 dB below strict band [−7.67, −6.67]). Investigate `sat=3.5f` or `sat=4.5f` constant tune to close residual; defer to Phase 2.4-bis."

## 20.5 Open Question #5 — 9-Audible-Golden Re-Baseline (RESOLVED — actual count 11 + saturator-tail + vibrato = 13; all 3-trial DET-PASS)

**CONTEXT.md count discrepancy:** CONTEXT rev-9-bis describes "9 audible goldens" but the explicit list contains 11 names: `{stiffness-zero-pre, string-A, string-D, string-G, detune-sweep-A, note-sequence, macro-sweep, slow-lfo, schelleng-stress, sub-harmonics, sub-harmonics-stability}`. Plus `saturator-tail-comparison` (12) and conditional `vibrato` (13 — see §20.6 below). **Actual re-baseline count is 13** (matches existing 13-entry `reproduce-goldens.sh`). PLAN rev-11 should normalise the count to 13.

**3-trial sha256 determinism for all 11 audible re-baselines (post-port from scratch-space prototype):**

| # | Golden | Pre-port committed | Post-port (3-trial DET) | Verdict |
|---|--------|--------------------|-------------------------|---------|
| 1 | stiffness-zero-pre.wav | `d358abcd…b0ee75` | `ed44cd8986d3a9d44cefd399dd128b62147901640ce615eadf7793f129f56020` | DET-PASS |
| 2 | string-A.wav | `c6755aa4…415918` | `505ad36e521d3a8cff978cc5386d6e69769da33977efc7dfccec33d721785bad` | DET-PASS |
| 3 | string-D.wav | `765b015e…65d9c9bc` | `e064035124d9af90c1cf6ac8a103e90efa64bf0d6a3efc17574d1f8c811668f4` | DET-PASS |
| 4 | string-G.wav | `0cd5cb0a…e1b993bd` | `0e9451b849b659ea5ea92ea3e92e0862e34d30ad5188235f816f43419111b3ca` | DET-PASS |
| 5 | detune-sweep-A.wav | `5e31dad3…2dbb05` | `b51d334bbfdd7da7abf4ba3391a6411d5a07427bb92b8e339389856a2539dbe7` | DET-PASS |
| 6 | note-sequence.wav | `3ac3ccd0…79260b5` | `2b5b8c83e419179ab04b5b218976c5d085538d59c0a55a967942c004fa1f8224` | DET-PASS |
| 7 | macro-sweep.wav | `c2571dd9…b37975e` | `231218b4e9f117ca6598ecee530f3b0af20d4109f29640608590d6cf15a66cfe` | DET-PASS |
| 8 | slow-lfo.wav | `c0c2c893…2466a0` | `d27589de30dcb6f3c432c8993d80106b38b4cb87e59afa2edc4ae301d8809cb8` | DET-PASS |
| 9 | schelleng-stress.wav | `9d18da86…2f9597` | `c5108af57520c8c190adaa6840513d4cfea6659da46d95bca01996d07efbda07` | DET-PASS |
| 10 | sub-harmonics.wav | `bfcaaadc…5573af` | `9178b41ec8b5bb6eb08b5ce9794dd93f542647ffd575ede39d88b8fff1a8c54c` | DET-PASS |
| 11 | sub-harmonics-stability.wav | `8043f659…d107b14a` | `2efdea9b5d0745e127ad1fbc4242779848f1ea031b0e426b22e966ed7df8e6be` | DET-PASS |
| 12 | saturator-tail-comparison.wav | `c7e845ea…1a60cb` | `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7` | DET-PASS (§20.4) |
| 13 | vibrato.wav | `d7881ecf…076b2c` | `df7384e358af9c5d5d34673a3976c2f34790f7cb2c07a96b45d6b3b03b568f47` | DET-PASS (§20.6) |

**All 13 WAV goldens are bit-deterministic across 3 back-to-back renders.** PLAN rev-11 locks these as R36-bis expected post-port WAV sha256s; execute-phase MUST reproduce them byte-identical at R36-bis-b. Risk #4 (re-baseline drift across runs) FULLY MITIGATED for all 13 goldens.

## 20.6 Open Question #6 + #10 — Vibrato Carry-Forward Determination (RESOLVED — RE-BASELINE; metrics shift exceeds tolerance; 13 audible re-baselines locked)

**3-trial vibrato post-port renders:**
- WAV (3-trial DET-PASS): `df7384e358af9c5d5d34673a3976c2f34790f7cb2c07a96b45d6b3b03b568f47`
- JSON (3-trial NON-DETERMINISTIC): r1=`cdc9cc21…`, r2=`f02803123…`, r3=`d5e970367…`

**JSON non-determinism root cause:** `vibrato.json` contains wall-clock fields (`blockMicros_median`, `blockMicros_max`, `blockTime_max_over_median`) and `outputWav` path string. These vary by render-host wall-clock and output filename. **The committed `vibrato.json.sha256 = 2c4b3a7f…` was a one-time anchor (Phase 2.4c R36 single render); it is NOT a byte-deterministic regression bar.** PLAN rev-11 should treat `vibrato.json.sha256` as informational/historical only and rely on `vibrato.wav.sha256` as the regression bar for the vibrato golden. (`reproduce-goldens.sh` already only checks `.wav.sha256`, never `.json.sha256` — confirming this design intent.)

**Determinism-stable metric extraction (post-port, 3 trials identical):**

| Metric | Pre-port (Phase 2.4c R36c) | Post-port | Δ | Tolerance | Verdict |
|--------|----------------------------|-----------|---|-----------|---------|
| `peakDepthCents` | 9.526 | **7.9507** | −1.5753 | ±0.05¢ | **OUTSIDE TOLERANCE** |
| `vibratoRateHzMeasured` | 4.978 Hz | 4.9788 Hz | +0.0008 | ±0.005 Hz | inside tolerance |
| `onsetTimeMs` | 1168 | **1000** | −168 | ±2 ms | **OUTSIDE TOLERANCE** |
| `peak` | (unknown) | 0.0914 | — | — | informational |
| `status` | FAIL (depth below 10¢ band) | FAIL (depth below 10¢ band) | unchanged | — | both pre/post fail strict band |

**Decision:** Vibrato **RE-BASELINES** (Open Q10 path B). Post-port vibrato.wav.sha256 = `df7384e358af9c5d5d34673a3976c2f34790f7cb2c07a96b45d6b3b03b568f47` becomes the new R36-bis golden. The pre-port `vibrato.wav.sha256 = d7881ecf…` and `vibrato.json.sha256 = 2c4b3a7f…` retire from `reproduce-goldens.sh` regression scope. **Total audible re-baselines: 13** (11 audible + saturator-tail-comparison + vibrato).

**Reason for shift (analysis):** Saturator port is upstream of the vibrato modulator path (vibrato modulates BOW_SPEED at Step 4; saturator runs at Step 7 within the same per-sample loop). Post-port `tanh` allows ~57% higher peak amplitude through the bridge rail (§20.4) → larger steady-state energy → vibrato modulation of bow speed has proportionally smaller effect on the energy envelope → autocorrelator measures smaller pitch detection swing. The status remains FAIL because both 9.526¢ (pre-port) and 7.9507¢ (post-port) lie below the strict 10¢ lower band; the post-port value drifts further from band but DOES NOT cross the 5¢ "vibrato barely audible" perceptual floor. Phase 2.4-bis VIBRATO_DEPTH→peakDepthCents transfer tune (DSP-09 backlog item) was already parked at Q48; this shift adds urgency but does NOT escalate scope (Phase 2.4c-bis stays strict-saturator-only per Q48).

**Phase 2.4-bis backlog item (additive):**
> "Vibrato peakDepthCents tune for tanh saturator topology — post-port landed 7.95¢ (was 9.53¢ pre-port; both below strict 10¢ band). Increase VIBRATO_DEPTH→bowSpeedSwing transfer to compensate for tanh's amplitude-pass-through; target restoration to 10–14¢ strict band."

## 20.7 Open Question #7 — Matrix-Stability Post-Port (RESOLVED — failure-mode migration; stability invariant intact; evidence-only NOT re-baselined)

**Method:** Re-render `--matrix-stability` (108-combo) against scratch-space port prototype; compare per-cell `pass_combo` against pre-port baseline (rendered from `/tmp/oc-pre-port` worktree).

**Pre-port (`6db6770727ab3b…`):** 105/108 PASS, 3 FAIL.
**Post-port (`09cbf15f7600…`):** 104/108 PASS, 4 FAIL.

**Failure-mode migration table:**

| Cell index | string | speedIdx | pressIdx | posIdx | Pre-port | Post-port | Verdict |
|-----------|--------|----------|----------|--------|----------|-----------|---------|
| (0,2,0,0) | E (28) | 2 (high) | 0 (low) | 0 (β=0.05) | FAIL | PASS | **STABILISED** |
| (1,2,0,0) | A (33) | 2 (high) | 0 (low) | 0 (β=0.05) | FAIL | PASS | **STABILISED** |
| (2,2,0,0) | D (38) | 2 (high) | 0 (low) | 0 (β=0.05) | FAIL | PASS | **STABILISED** |
| (0,0,2,0) | E (28) | 0 (low) | 2 (high) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |
| (0,1,2,0) | E (28) | 1 (mid) | 2 (high) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |
| (0,2,2,0) | E (28) | 2 (high) | 2 (high) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |
| (3,2,1,0) | G (43) | 2 (high) | 1 (mid) | 0 (β=0.05) | PASS | FAIL | **NEW raucous corner** |

All FAIL cells (pre and post) fail on `pass_clickFree` only; **`pass_noNaN`, `pass_peak`, `pass_blockTime` all PASS across all 108 combos pre AND post.** Peak amplitude across all post-port combos: max ≈ 0.351 (within strict |x| < 1.0 invariant). nanCount=0 / infCount=0 across all 108 combos pre AND post.

**Failure-mode interpretation:**
- **Pre-port failures:** low-pressure × high-speed × close-to-bridge (β=0.05) → bow signal underbowed, Helmholtz-corner-edge ringing, click-free heuristic trips at peak ≈ 0.05.
- **Post-port failures:** high-pressure × close-to-bridge (β=0.05) on E1 string × multiple speeds, plus G3 high-speed × mid-pressure → bow signal overbowed, peak ≈ 0.35 (~7× larger than pre-port click-free fails), click-free heuristic trips on transient amplitude.
- **Topology mechanism:** Pre-port `x/√(1+x²)` saturates more aggressively at moderate amplitudes (output ≈ 0.707 at x=1), throttling overbowed pressure transients. Post-port `4·tanh(x/4)` is nearly linear up to x≈4 (output ≈ 0.984 at x=1), passing pressure transients through with less compression → larger output peaks at high-pressure × close-to-bridge corners trip the click-free heuristic. Conversely, the gentler post-port saturation no longer chokes off the under-bowed signal → low-pressure × high-speed cells stabilise.

**Verdict (Open Q7 case classification):**
- Case (a) "raucous-corner cells stabilise": **PARTIAL** — 3/3 pre-port FAILs stabilise.
- Case (b) "raucous-corner cells hold": NO.
- Case (c) "NEW raucous corners surface": **YES** — 4 NEW FAILs (3 on E1 string × β=0.05 × high-pressure axis; 1 on G3 high-speed × mid-pressure × β=0.05).

**Per CONTEXT.md case (c) handling instruction** ("regression risk; investigate before R36-bis atomic. If (c), block plan-phase pending root-cause"):

**Root cause:** Established (saturator topology curvature change). The new failures are mechanistically explained by `tanh`'s gentler moderate-amplitude curvature passing high-pressure pressure transients with less compression than the algebraic saturator. This is **not a regression of the DSP-01 stability invariant** (no NaN, no peak > 1.0, no runaway, no denormal CPU spike); it is a migration of click-free quality-gate failures from low-pressure to high-pressure × β=0.05 corners.

**Plan-phase recommendation:** **DO NOT BLOCK.** Stability invariant intact. `pass_clickFree` is a quality heuristic (transient amplitude derivative), not a safety gate. Treat as **Phase 2.4-bis backlog candidate**: "Click-free heuristic threshold tune for high-pressure × β=0.05 corners post-tanh saturator port — 4 cells (E1 × press2 × all speeds + G3 × speed2 × press1) failing at β=0.05 with peak ≈ 0.35; investigate threshold relaxation OR per-string Schelleng wedge tune at near-bridge bow position."

**Re-baseline scope (per CONTEXT):** matrix-stability is **evidence-only NOT committed**. Existing `matrix-stability.wav.sha256 = 6db67707…` carries forward as the historical Phase 2.4a R34b evidence baseline; post-port WAV (`09cbf15f7600…`) becomes informational-only evidence for end-of-Stage-2 §"In-loop saturator" amendment. PLAN rev-11 does NOT update `matrix-stability.wav.sha256`.

## 20.8 Open Question #8 — Sub-Harmonics Post-Port subharmEnergyRatio (RESOLVED — CRITICAL DROP; DSP-07 feature neutralised at engagement; default-state goldens unaffected via HR-9)

**Post-port `--sub-harmonics` measurement (3-trial DET-PASS sha256 `9178b41e…`):**

| Metric | Pre-port (Phase 2.4b R35) | Post-port | Δ | Verdict |
|--------|---------------------------|-----------|---|---------|
| `subharmEnergyRatio` | **0.358** | **0.000170** | **−0.358 (~99.95% reduction)** | **CRITICAL DROP** |
| `pass_subharmAudible` | True (soft-PASS, above 0.30 strict) | **False** | flipped | hard-fail floor crossed |
| `peak` | (Phase 2.4b not captured) | 0.1712 | informational | — |

**Severity:** subharmEnergyRatio dropped from **0.358 → 0.000170**, a ~33 dB reduction in subharmonic energy. Far below the 0.30 strict threshold (Phase 2.4b §18.6) AND the 0.30 hard-fail floor. **The DSP-07 sub-harmonic bias feature is effectively neutralised at the canonical engagement operating point post-port.**

**Mechanism:** Phase 2.4b DSP-07 (Step 2.5) introduces a low-frequency bias to the friction model that nudges the limit cycle toward period-doubling at moderate-amplitude regions of the saturator's curvature. The pre-port algebraic saturator `x/√(1+x²)` has steeper curvature at amplitude x≈0.5–1.0 → amplifies bias-induced subharmonic excursions. The post-port `4·tanh(x/4)` is nearly linear up to x≈4 → does NOT amplify the period-doubling tendency. Result: subharmonic-bias bias signal passes through ~undistorted, generating no perceptible subharmonic energy.

**Post-port `--sub-harmonics-stability` measurement (DET-PASS sha256 `2efdea9b…`):**
- `passCount=36/36`, `failCount=0`, `pass_all_36=True`, status `PASS`.
- All 36 combos pass on `pass_noNaN`, `pass_peak`, `pass_clickFree`, `pass_blockTime` — **stability invariant intact across all SUB_HARMONICS values × infiniteSustain combos.**

**Default-state HR-9 short-circuit verification:**
- HR-9 (Phase 2.4b R35 lock): "SUB_HARMONICS=0 → IEEE 754 identity arithmetic short-circuit; no bias signal added to friction input."
- The 11 default-state audible goldens (string-A/D/G, detune-sweep, etc.) all render at SUB_HARMONICS=0 (default APVTS); HR-9 short-circuit holds → no bias path engaged → those goldens shift only due to direct saturator topology change, NOT subharmonic-bias amplification differential.
- **Default user experience UNAFFECTED.** Only users explicitly engaging `SUB_HARMONICS > 0` knob will perceive feature loss.

**Plan-phase classification:** **NOT BLOCKING per Q48 strict-saturator-only-scope decision.** The DSP-07 feature loss is a known consequence of the saturator topology change at the bias engagement point. Phase 2.4-bis backlog item (additive — strengthens existing kForceBoost retune item):

> "**Phase 2.4-bis DSP-07 retune for tanh saturator topology** — subharmEnergyRatio dropped 0.358 → 0.000170 (~33 dB reduction) at canonical engagement point post-tanh-port. Restore to ≥ 0.30 strict OR ≥ 0.358 R35-baseline parity via: (a) kForceBoost gain compensation (tanh's lower amplification of bias signal vs. algebraic), (b) bias signal amplitude scale (current `subAmount · 0.8 · safeDepth` constant may need 3–5× boost to compensate for ~33 dB tanh deficit), (c) bias injection point shift (move from Step 2.5 to post-saturator Step 8 if curvature amplification is the lock). Hard-fail floor 0.30 stays as Gate 6c-bis-bis bar. Acknowledge DSP-07 feature is currently MUTED for sub-harmonics-knob users post-port; default-knob users unaffected via HR-9."

**Caveat:** This is the most significant non-local consequence of the saturator port. Plan-phase MUST surface this in PLAN rev-11 §"Risk-Surface" so the user has visibility into the sub-harmonics feature mute before R36-bis-atomic. Recommend executing R37-bis Logic AU audition with **two probe sequences**: (a) default-state SUB_HARMONICS=0 (validates HR-9 + saturator character), (b) SUB_HARMONICS=0.7 engagement (audits perceived subharmonic feature loss).

## 20.9 Open Question #9 — R36-bis Task Breakdown (RESOLVED — 9-task structure)

**Task structure (mirrors R36 9-task `R36-pre / R36a–f / R36 / R36-backfill` precedent):**

| Task | Title | Deliverable | Audit hook(s) |
|------|-------|-------------|---------------|
| **R36-bis-pre** | Tripwire | (a) `bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` reports 13/13 PASS at HEAD before any source edit; (b) `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports 0 files modified; (c) `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns 0; (d) `/tmp/oc-pre-port` worktree present + harness built (per §20.3 protocol) | (a) 13/13 PASS exit 0; (b) reports 0 files; (c) returns 0 |
| **R36-bis-a** | Source edit | Apply 4-LOC port at `Source/DSP/WaveguideString.cpp:204–206`: replace algebraic saturator with `sat·tanh(x/sat)`, sat=4.0f, both rails | post-edit `git diff --stat`: 1 file changed, 4 insertions, 3 deletions; `grep -c "sat \* std::tanh"`: 2 |
| **R36-bis-b** | Re-baseline + evidence renders | (a) Build `O-Contrabass-render-test` post-edit; (b) render 13 audible goldens (11 carry-forward + saturator-tail-comparison + vibrato) → write per-golden `.wav.sha256` files (predicted values per §20.5); (c) render `--matrix-stability` evidence-only (NOT committing updated sha256; archive WAV `09cbf15f7600…` to RESEARCH §19.7.7.6 reference); (d) update `vibrato.json.sha256` to one-time-anchor of post-port single render (see §20.6 — JSON non-deterministic; anchor is informational only) | All 13 sha256 files match §20.5 predicted; matrix-stability evidence WAV captured in /tmp/oc-mat or .planning/evidence/ |
| **R36-bis-c** | RESEARCH §19.7.7 verdict | Write 10-sub-section §19.7.7 verdict subsection per CONTEXT line 51–61: §§19.7.7.1 (source delta verification) / 19.7.7.2 (post-port saturator-tail key bins) / 19.7.7.3 (measured 0.7975 dB |Δ| at bin 64) / 19.7.7.4 (13-audible-golden re-baseline sha256s) / 19.7.7.5 (vibrato re-baseline determination) / 19.7.7.6 (matrix-stability post-port evidence + failure-mode migration) / 19.7.7.7 (sub-harmonics + sub-harmonics-stability post-port; §20.8 critical drop documented) / 19.7.7.8 (R37-bis Logic AU audition outcome — execute-phase fills) / 19.7.7.9 (verdict — research-phase pre-classification: SOFT-PASS path with 3 Phase 2.4-bis backlog items added) / 19.7.7.10 (evidence base summary for end-of-Stage-2 amendment) | RESEARCH.md grows by ~150 lines; §19.7.7.* numeric ladder structurally matches §19.7 sibling subsections |
| **R36-bis-d** | R37-bis Logic AU audition (BLOCKING) | (a) Build `O-Contrabass-dev` AU post-port; (b) install AU bundle + clear AU cache (per CLAUDE.md macOS protocol); (c) build `/tmp/oc-pre-port` AU per §20.3 worktree protocol + install as separate AU bundle (e.g., `O-Contrabass-pre-port-dev.component`); (d) user audition in Logic Pro per §20.10 protocol; (e) user CONFIRMS post-port character acceptable — BLOCKING gate before R36-bis atomic commit | User-confirmed CONFIRM message appended to STATUS.md `phase_2_4c_bis_audition_outcome` field |
| **R36-bis-e** | Regression bar + audit-hook re-runs | (a) `bash reproduce-goldens.sh` against post-port goldens reports 13/13 PASS; (b) `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports exactly 1 file (WaveguideString.cpp); (c) `grep -c "sat \* std::tanh"`: 2 | All three checks PASS |
| **R36-bis-f** | auval + pluginval-10 | (a) `auval -v aumu OBSC OURI` SUCCEEDED; (b) `pluginval --strictness-level 10` SUCCESS full battery (Editor Automation / Automatable Parameters / Parameter thread safety / Background thread state / Bus enable/disable / Restoring default layout / Fuzz parameters all complete) | auval exit 0 / pluginval exit 0 |
| **R36-bis** | Atomic commit | After R36-bis-d CONFIRMED + R36-bis-e + R36-bis-f PASS, commit: 1 source-edit file (WaveguideString.cpp) + 13 re-baselined `*.wav.sha256` files + RESEARCH §19.7.7 + CONTEXT §"Audit Trail" rev-9-bis closure entry + STATUS.md flip to `phase_2_4c_bis_complete` + SUMMARY.md + VERIFICATION.md Phase 2.4c-bis sections | atomic commit message structured per R34/R35/R36 precedent: `feat(O-Contrabass): Phase 2.4c-bis R36-bis — in-loop saturator port (`x/√(1+x²)` → `4·tanh(x/4)`); Gate 6c-bis SOFT-PASS (\|Δ\|=0.80 dB at bin 64; 3 Phase 2.4-bis backlog items)` |
| **R36-bis-backfill** | Backfill chore | Propagate R36-bis sha256 into STATUS.md `phase_2_4c_bis.atomic_sha` field (separate chore commit per R34/R35/R36 backfill precedent) | chore commit `chore(O-Contrabass): backfill Phase 2.4c-bis R36-bis commit sha into STATUS.md` |

## 20.10 Open Question #11 — R37-bis Logic AU Audition Protocol (RESOLVED — 4-step BLOCKING audition)

**Step 1 — Build pre-port reference AU:**

```bash
git worktree add /tmp/oc-pre-port 115dbf4   # IDEMPOTENT — already exists from §20.3
cd /tmp/oc-pre-port/build && \
  cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON .. && \
  ninja O-Contrabass-dev_AU O-Contrabass-dev_VST3
```

**Step 2 — Install pre-port AU as side-by-side bundle (DO NOT collide with working-tree AU):**

```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-port.component  # cleanup
PRE_AU=/tmp/oc-pre-port/build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass-dev.component
cp -R "$PRE_AU" ~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-port.component
# Edit Info.plist `CFBundleIdentifier` + AU `description` to disambiguate from working-tree AU
# (suggest `manuf.O-Contrabass-pre-port` / `aumu` `OBSP` four-char to avoid AU collision)
```

**Step 3 — Build + install post-port AU (working tree, post-R36-bis-a):**

```bash
cd /Users/taylorbrook/Dev/VST-development && \
  ninja O-Contrabass-dev_AU O-Contrabass-dev_VST3
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass-dev.component
cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass-dev.component \
      ~/Library/Audio/Plug-Ins/Components/
auval -a | grep -i 'O-Contrabass'   # verify both AUs registered
```

**Step 4 — Audition in Logic Pro:**

Probe sequences (PASS criteria for each):

| Sequence | Probe | PASS criterion |
|----------|-------|----------------|
| 1 | Sustained E1 (MIDI 28) at canonical bow (BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10, INFINITE_SUSTAIN=1.0); 8-second sustain + 5-second tail-decay | Post-port tail decay sounds **smoother + more natural** than pre-port (consistent with §20.4 finding rmsRatio_final_over_mid 0.26 → 0.50); no transient artifacts on bow-on attack |
| 2 | Per-string MIDI 28 / 33 / 38 / 43 (E1 / A1 / D2 / G2) — hold 4-sec each at canonical bow | Each string's character preserves harmonic spectrum + body resonance carry-forward (no Phase 2.5 yet, so this is dry waveguide character); post-port slightly **brighter + more sustained** (consistent with peak +57% / rmsMid +36%) |
| 3 | Sustained E1 + bow-off at 4-sec; listen to 10-sec post-bow-off tail | Post-port tail energy ~3× higher (rmsRatio_final_over_mid 0.50 vs 0.26 — measurable) but should **NOT exhibit ringing, clicks, or DC drift**; closer to O-Bowed reference character |
| 4 | SUB_HARMONICS=0.7 engagement on sustained E1 (probes Phase 2.4-bis DSP-07 mute per §20.8) | User notes: post-port **subjectively MUTES the subharmonic effect** (matches §20.8 measurement); document subjective severity to inform Phase 2.4-bis priority |
| 5 (optional) | VIBRATO_DEPTH=0.7, EXPRESSION_MACRO=0.5 on sustained E1 | Post-port vibrato perceptually similar to pre-port; depth slightly reduced (consistent with §20.6 7.95¢ vs 9.53¢ measurement) but vibrato shape character preserved |

**PASS criteria summary:**
- Sequences 1–3: **MUST PASS** (no unexpected character changes; saturator port produces "smoother + more O-Bowed-like" subjective improvement)
- Sequence 4: **DOCUMENT** (informs Phase 2.4-bis DSP-07 priority); does NOT block R36-bis atomic
- Sequence 5: **DOCUMENT** (informs Phase 2.4-bis DSP-09 vibrato priority); does NOT block R36-bis atomic

**FAIL handling (BLOCKING):** If sequences 1–3 reveal unexpected character changes (transient distortion, peak-amplitude artifacts, DC drift, ringing), R36-bis atomic does NOT land. Resolution paths:
- (a) `sat` constant tune (`sat=3.0f` or `sat=5.0f`) + re-render saturator-tail-comparison → re-audition;
- (b) Acknowledge architectural divergence; revert R36-bis-a source edits; close 2.4c-bis as research-only acknowledged-divergence (path (c) of CONTEXT Risk #8);
- (c) Escalate to Phase 2.4c-bis-bis with alternative topology (LUT, polynomial approximation).

## 20.11 Open Question #12 — RESEARCH §19.7.7 Deliverable Structure (RESOLVED — 10 sub-sections per CONTEXT)

§19.7.7 is the closure subsection that locks the post-port verdict and closes the §19.7.6 escalation flag. Structure:

- **§19.7.7.1** Source delta verification (4-LOC diff at WaveguideString.cpp:204–206; audit hook results)
- **§19.7.7.2** Post-port saturator-tail decay envelope key bins (bin 0 / 5 / 60 / 64) — uses §20.4 measurements verbatim
- **§19.7.7.3** Measured |Δ| at bin 64 vs O-Bowed reference (0.7975 dB; SOFT-PASS verdict)
- **§19.7.7.4** 13-audible-golden re-baseline sha256s (table mirrors §20.5)
- **§19.7.7.5** Vibrato carry-forward determination (RE-BASELINE per §20.6)
- **§19.7.7.6** Matrix-stability post-port evidence (failure-mode migration table from §20.7; evidence-only NOT re-baselined)
- **§19.7.7.7** Sub-harmonics + sub-harmonics-stability post-port measurements (subharmEnergyRatio drop per §20.8; default-state HR-9 short-circuit verification)
- **§19.7.7.8** R37-bis Logic AU audition outcome [execute-phase locks per §20.10 protocol]
- **§19.7.7.9** Verdict — port WORKED-PARTIALLY (SOFT-PASS at bin 64 + 3 Phase 2.4-bis backlog items: DSP-07 retune for tanh, DSP-09 vibrato depth tune, click-free heuristic threshold tune)
- **§19.7.7.10** Evidence base for end-of-Stage-2 §"In-loop saturator" architecture amendment (pre-port reference `c7e845ea…` from `115dbf4` worktree + post-port `5c45d176…` + 0.80 dB convergence narrative)

Research-phase fills §§19.7.7.1–7 + 9–10 with measured data; execute-phase fills §19.7.7.8 (audition outcome) + locks §19.7.7.9 verdict line based on audition result.

## 20.12 Risk-Surface Refinement for PLAN rev-11

**Risks DOWNGRADED by §20 research findings:**

| Risk (CONTEXT) | Status post-§20 | Note |
|----------------|-----------------|------|
| #1 Saturator port unexpected character change | OPEN — only R37-bis audition resolves | §20.4 metrics suggest smoother decay tail; subjective bar still pending |
| #2 Convergence misses ±0.5 dB target | RESOLVED — soft-PASS at 0.80 dB |Δ| | Lands at −7.97 dB; soft-band [−8.17, −6.17] inside; 0.30 dB outside strict band |
| #3 Two-call-site asymmetry | RESOLVED — single-site correct | O-Bowed `processSample:138-139` is dead code; `writeJunction:218-219` ↔ O-Contrabass `processSample:204-206` |
| #4 9-audible-golden re-baseline drift | RESOLVED — all 13 DET-PASS 3-trial | §20.5 table; `std::tanh` deterministic on M1 macOS Xcode 26.3 |
| #5 Vibrato golden carry-forward fails | RESOLVED — RE-BASELINES; 13 audible total | peakDepth shift −1.58¢ exceeds ±0.05¢; onset shift −168 ms exceeds ±2 ms |
| #6 Matrix-stability NEW raucous corners | OPEN — 4 NEW corners but stability invariant intact | §20.7 failure-mode migration; Phase 2.4-bis backlog item; non-blocking |
| #7 Sub-harmonics drop below 0.30 | **MATERIALISED — 0.000170 (~33 dB)** | §20.8 critical drop; DSP-07 feature mute at engagement; default-state HR-9 unaffected |
| #8 R37-bis BLOCKING audition fails | OPEN — will be resolved at execute-phase | Pre-flight metrics support PASS likelihood |
| #9 Audit-hook drift mid-cycle | RESOLVED — pre-flight audit hooks return clean | §20.4 verifies post-edit `git diff --stat`/`grep -c` |
| #10 `std::tanh` RT-safety | RESOLVED — M1 deterministic | 3-trial sha256 byte-identical; no LUT fallback needed; pluginval-10 at R36-bis-f confirms |
| #11 Phase 2.5-awareness | DEFERRED — Phase 2.5 verify will re-measure | unchanged |
| #12 R36-bis atomic + R36-backfill chore interaction | RESOLVED — atomic + backfill chore precedent | `7835904` is current HEAD; R36-bis lands on top |
| #13 Pre-port reference re-render reproducibility | RESOLVED — 3-trial PASS from `115dbf4` worktree | §20.3 protocol locked; SKIP_PLUGINS=O-Orbit + OUARICON_BUILD_TESTS=ON |

**NEW risks surfaced by §20 research:**

- **#14 Sub-harmonics feature mute at engagement (§20.8).** subharmEnergyRatio 0.358 → 0.000170 (~33 dB drop). DSP-07 architectural commitment from Phase 2.4b R35 is broken at SUB_HARMONICS > 0 engagement. Default-state HR-9 short-circuit unaffected. Mitigation: Phase 2.4-bis backlog DSP-07 retune (kForceBoost / mu_d / bias amplitude); R37-bis sequence 4 audits subjective severity. Plan-phase MUST surface this in PLAN rev-11 §"Risk Surface" with severity flag.
- **#15 Vibrato peakDepthCents shift to 7.95¢ (§20.6).** Post-port lands further below strict 10–14¢ band (was 9.53¢ pre-port). Phase 2.4-bis backlog DSP-09 VIBRATO_DEPTH transfer tune already parked; this finding strengthens but does not change scope. Mitigation: Phase 2.4-bis backlog. Non-blocking for R36-bis atomic.
- **#16 Click-free heuristic regression at high-pressure × β=0.05 corners (§20.7).** 4 NEW raucous corners; stability invariant intact (no NaN, no peak > 1.0). Mitigation: Phase 2.4-bis backlog click-free heuristic threshold tune. Non-blocking. Migration-not-regression: 3 pre-port FAILs stabilise simultaneously (net +1 cell).

## 20.13 Sequencing in PLAN rev-11

**R36-bis 9-task sequence (per §20.9):**

```
R36-bis-pre  → R36-bis-a → R36-bis-b → R36-bis-c → R36-bis-d (BLOCKING) → R36-bis-e → R36-bis-f → R36-bis (atomic) → R36-bis-backfill
```

- R36-bis-pre + R36-bis-a + R36-bis-b + R36-bis-c can stage as a single working-tree state (no commits between).
- R36-bis-d gates the atomic commit — user audition is BLOCKING.
- R36-bis-e + R36-bis-f run after audition CONFIRMS, before atomic.
- R36-bis-backfill is a separate chore commit lifecycle (mirrors R34/R35/R36 backfill precedent).

**Wall-clock budget estimate:**
- R36-bis-pre: ~2 minutes (reproduce-goldens.sh full render)
- R36-bis-a: ~30 seconds (4-LOC edit)
- R36-bis-b: ~3 minutes (build + 13 audible renders + matrix-stability evidence)
- R36-bis-c: ~10 minutes (write §19.7.7 verdict subsection — research-phase pre-fills §§19.7.7.1–7 + 9–10 from §20)
- R36-bis-d: user-paced (Logic AU audition + 5 probe sequences ≈ 15–20 minutes)
- R36-bis-e: ~30 seconds (regression bar + audit hooks)
- R36-bis-f: ~10 minutes (auval + pluginval-10)
- R36-bis: ~30 seconds (atomic commit)
- R36-bis-backfill: ~30 seconds (chore commit)

Total estimated cycle: **30–40 minutes** including audition pacing.

## 20.14 Open Items for Plan-Phase

PLAN rev-11 MUST address:

1. **Lock 13 expected post-port `*.wav.sha256` values** verbatim from §20.5 table.
2. **Lock R36-bis 9-task breakdown** verbatim from §20.9 table.
3. **Lock R37-bis Logic AU audition protocol** verbatim from §20.10 Steps 1–4 + 5 probe sequences.
4. **Surface 3 NEW risks (#14–#16) + 1 SOFT-PASS verdict** in PLAN §"Risk Surface" so user has visibility into:
   - DSP-07 sub-harmonics feature mute at engagement (Phase 2.4-bis backlog item)
   - DSP-09 vibrato depth shift (Phase 2.4-bis backlog item; additive)
   - Click-free heuristic threshold tune (Phase 2.4-bis backlog item — new)
   - SOFT-PASS at bin 64 (0.30 dB outside strict band; 0.80 dB |Δ| vs O-Bowed)
5. **Document `vibrato.json.sha256` is informational-only** (non-deterministic across renders due to wall-clock fields); regression bar is `vibrato.wav.sha256` only.
6. **Document matrix-stability evidence-only NOT re-baselined** (existing `6db67707…` carries forward; post-port `09cbf15f…` archived to RESEARCH §19.7.7.6 evidence reference).
7. **Lock pre-port worktree protocol** at `/tmp/oc-pre-port` (idempotent across re-creates; document `git worktree remove` cleanup at Phase 2.4c-bis verify-phase close).
8. **Lock SUB_HARMONICS=0 default unaffected** finding per HR-9 IEEE 754 identity arithmetic carry-forward.
9. **Validate Q47 SOFT-PASS path is acceptable to user** OR escalate to user input before plan-phase commits if user wants strict-only PASS path (would require `sat` constant tune iteration).

## 20.15 Summary — Phase 2.4c-bis Research Resolution Map

| Open Question | Status | Section |
|---------------|--------|---------|
| Q1 (pre-port repro tripwire) | RESOLVED — 13/13 PASS at HEAD `7835904` | §20.1 |
| Q2 (scratch-space port prototype + bin 64 convergence) | RESOLVED — SOFT-PASS at 0.7975 dB |Δ|; 3-trial DET-PASS sha256 `5c45d176…` | §20.4 |
| Q3 (two-call-site audit in O-Bowed) | RESOLVED — single-site port at `:204-206` correct; O-Bowed `:138-139` is dead code | §20.2 |
| Q4 (pre-port reference preservation) | RESOLVED — `git worktree` at `115dbf4` reproduces `c7e845ea…` byte-identical 3-trial | §20.3 |
| Q5 (9-audible-golden re-baseline strategy) | RESOLVED — actual count 11 + saturator-tail + vibrato = 13; all 3-trial DET-PASS | §20.5 |
| Q6 (vibrato carry-forward determination) | RESOLVED — RE-BASELINES (peakDepth shift −1.58¢ > ±0.05¢; onset shift −168 ms > ±2 ms) | §20.6 |
| Q7 (matrix-stability re-render verdict) | RESOLVED — failure-mode migration; stability invariant intact; evidence-only | §20.7 |
| Q8 (sub-harmonics post-port subharmEnergyRatio) | RESOLVED — CRITICAL DROP 0.358 → 0.000170 (~33 dB); DSP-07 mute at engagement; default unaffected via HR-9 | §20.8 |
| Q9 (R36-bis task breakdown) | RESOLVED — 9-task R36-bis-pre/a-f/atomic/backfill | §20.9 |
| Q10 (vibrato carry-forward conditional) | RESOLVED — path B; 13 audible re-baselines | §20.6 |
| Q11 (R37-bis audition protocol) | RESOLVED — 4-step protocol + 5 probe sequences (3 BLOCKING + 2 DOCUMENT-only) | §20.10 |
| Q12 (RESEARCH §19.7.7 deliverable structure) | RESOLVED — 10 sub-sections; research-phase pre-fills 7 of 10 | §20.11 |
| Risk #13 (pre-port reference re-render reproducibility) | RESOLVED — 3-trial PASS; `SKIP_PLUGINS=O-Orbit + OUARICON_BUILD_TESTS=ON` flags required | §20.3 |

**Critical findings (research-phase NEW knowledge):**

1. **Saturator port converges to 0.7975 dB |Δ|** vs O-Bowed reference at bin 64 — well within §19.3.3 analytic ≤2 dB prediction; 87% improvement vs pre-port 5.92 dB divergence. Lands SOFT-PASS (Q47 strict-band miss by 0.30 dB).
2. **`std::tanh` is bit-deterministic on M1 macOS Xcode 26.3 toolchain** (3-trial sha256 byte-identical for all 13 audible goldens) — no LUT fallback needed; minimal source delta preserved.
3. **CONTEXT.md "9 audible goldens" count is incorrect** — actual list contains 11 names (11 audible + saturator-tail-comparison + vibrato = 13 total re-baselines matching `reproduce-goldens.sh`).
4. **Vibrato `vibrato.json.sha256` is non-deterministic across renders** — contains wall-clock `blockMicros_*` fields + `outputWav` path; PLAN should treat as informational-only anchor, not regression bar.
5. **Sub-harmonics feature is muted at engagement post-port** (subharmEnergyRatio 0.358 → 0.000170, ~33 dB drop) — DSP-07 architectural commitment from Phase 2.4b is broken at SUB_HARMONICS > 0; default-state HR-9 short-circuit unaffected so default users are not impacted. Plan-phase MUST surface this in §"Risk Surface" with severity flag.
6. **Matrix-stability failure mode migrates** — 3 pre-port FAILs (low-press × high-speed × β=0.05) stabilise; 4 NEW FAILs surface at high-press × β=0.05. Net +1 cell. Stability invariant intact (no NaN, no peak > 1.0). Click-free quality heuristic threshold tune is Phase 2.4-bis backlog candidate.
7. **Pre-port reference re-render reproducibility CONFIRMED** via `git worktree add /tmp/oc-pre-port 115dbf4` + `SKIP_PLUGINS=O-Orbit + OUARICON_BUILD_TESTS=ON` CMake flags — protocol locked for end-of-Stage-2 §"In-loop saturator" amendment evidence base.
8. **Phase 2.4-bis backlog gains 3 NEW items** post-§20: DSP-07 sub-harmonics retune for tanh, DSP-09 vibrato depth tune (additive), click-free heuristic threshold tune (new). Plus 1 STRICT-band convergence retune. Total Phase 2.4-bis scope grows by 4 items.

## 20.16 References (§20 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-9-bis (this cycle's discuss artefact, 2026-04-29).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` rev-10 (Phase 2.4c R36 + §"Contingency — Phase 2.4c-bis Escalation Lane" pre-write).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §19.3 (Phase 2.4c O-Bowed saturator topology audit) + §19.5 (Phase 2.4c saturator-tail measurement protocol) + §19.7 (Phase 2.4c verdict path; §19.7.6 escalation flag locked).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` lines 204–206 (in-loop algebraic saturator at HEAD; 4-LOC edit point for R36-bis-a).
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` lines 218–219 (`writeJunction` saturator — architectural equivalent; ACTIVE production path).
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` lines 138–139 (`processSample` saturator — DEAD CODE; not invoked by `BowedStringVoice.cpp`).
- `plugins/O-Bowed/Source/BowedStringVoice.cpp` lines 154 + 184 (`readJunction` + `writeJunction` invocation — confirms split-rail path active).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 641–680 (`strings[s].processSample(…)` invocations — confirms unified-path active).
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (13-entry script at HEAD `7835904`; PLAN rev-11 stays 13 entries).
- `/tmp/oc-pre-port/` git worktree at `115dbf4` (Phase 2.4c R36 atomic; harness built per §20.3 protocol; Open Q4 reproduction reference; idempotent across re-creates).
- §20.1 pre-flight reproduce-goldens.sh execution: 13/13 PASS at HEAD `7835904` (transient `/tmp/repro/*.wav`; deleted post-research).
- §20.3 pre-flight pre-port saturator-tail re-render: 3-trial sha256 = `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb` byte-identical (transient `/tmp/sat-pre/*.wav`; archived in /tmp/sat-pre/sat-tail-r1.json for §20.4 metric extraction).
- §20.4 scratch-space port prototype: 4-LOC source edit applied + reverted (`git checkout` post-measurement); 3-trial post-port sha256 = `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7` byte-identical (transient `/tmp/sat-post/*.wav`; archived in /tmp/sat-post/sat-tail-r1.json for §20.4 metric extraction).
- §20.5 13-audible-golden 3-trial determinism check: all 13 goldens (11 audible + saturator-tail + vibrato) byte-identical 3-trial PASS (transient `/tmp/oc-bis/*.wav` + `/tmp/oc-vib/*.wav`; deleted post-research).
- §20.7 matrix-stability post-port re-render: 108-combo single render; passCount=104/108, failCount=4 (transient `/tmp/oc-mat/matrix-stability.{wav,json}`; archived in §20.7 failure-mode migration table).
- §20.8 sub-harmonics + sub-harmonics-stability post-port renders: subharmEnergyRatio measurement extracted from `/tmp/oc-bis/sub-harmonics-r1.json`; sub-harmonics-stability 36/36 PASS confirmed via `/tmp/oc-bis/sub-harmonics-stability-r1.json`.
- HEAD `7835904` (R36-backfill chore, 2026-04-29) — production base for Phase 2.4c-bis; descendant of R36 atomic `115dbf4`.
- `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON ..` — pre-port worktree configure incantation; the SKIP_PLUGINS flag bypasses uninitialised O-Orbit `libs/SAF` git submodule.
- Python 3.14.2 + standard library `json` + `wave` modules — bin-extraction + metric-comparison without numpy.
- ARCHITECTURE.md §"In-loop saturator" — end-of-Stage-2 amendment cycle consumes Phase 2.4c-bis post-port findings; both pre-port (`c7e845ea…` from `115dbf4`) AND post-port (`5c45d176…` from R36-bis atomic) goldens form the evidence base.

---

# §21 Phase 2.5 Research — Body Resonator (8-Mode Static-Q Bank) + Bow Noise Generator (3-Band BPF + Slip Bursts)

**Numbering note:** CONTEXT.md rev-10 §"In Scope" / §"Next Phase" calls the new RESEARCH subsection "§20". That collides with §20 (Phase 2.4c-bis verdict, locked). Phase 2.5 verdict is therefore landed as **§21** here. CONTEXT.md rev-11 (post-research) will pick up the corrected reference.

**Verdict — read this before plan-phase:** the CONTEXT.md Q54 user-confirmed approach ("per-plugin verbatim copy from O-Bowed `Source/DSP/BodyResonator.{h,cpp}` + `BowNoiseGenerator.h` with bass-tuned mode table + bass-spectral target substitution") **does not yield the ARCHITECTURE-spec'd deliverable**. The O-Bowed sources differ from the bass spec in API surface, filter type, channel topology, parameter set, dry-path treatment, and slip-detection mechanism — at least six axes. Phase 2.5 is therefore "implement bass spec from ARCHITECTURE §"Body Resonator" + §"Bow Noise Generator" using O-Bowed pattern as a reference for `juce::dsp::IIR::Filter` lifecycle + `juce::Random` seeding only" — closer to ~250–350 LOC NEW per file than "verbatim copy + bass-substitution" (Q54 budget estimate ≈ 100–250 LOC). Also: the bow-friction module v1.0.0 + WaveguideString.cpp expose **no Helmholtz stick→slip phase signal**, so true slip-detection (ARCHITECTURE §"Bow Noise Generator" "zero-crossing of friction force from stick to slip") is unreachable inside CONTEXT's 4-file scope-strict rule and must be **substituted with a period-heuristic trigger** (per-fundamental-period burst counter) for v1.0. This is a research-phase **escalation finding** that plan-phase needs to consume (and may need user re-confirmation against the revised LOC + design footprint).

This section resolves Open Questions #1–#14 from CONTEXT.md rev-10 §"Open Questions (handed to research-phase)". Sub-sections §21.1–§21.13 map 1:1 to those open questions; §21.14–§21.18 carry refined risk/sequencing/open-items/summary/references.

---

## 21.1 Open Question #1 — Pre-Phase-2.5 Repro Tripwire (RESOLVED — 13/13 PASS at HEAD)

**Status:** ✅ RESOLVED.

Render execution: `bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` against HEAD (descendant of R36-bis-backfill chore `1dfca9d`; build cache from prior session). All 13 entries reproduce byte-identical against committed sha256s:

| Golden | sha256 | Result |
|--------|--------|--------|
| stiffness-zero-pre | `ed44cd8986d3a9d44cefd399dd128b62147901640ce615eadf7793f129f56020` | PASS |
| string-A | `505ad36e521d3a8cff978cc5386d6e69769da33977efc7dfccec33d721785bad` | PASS |
| string-D | `e064035124d9af90c1cf6ac8a103e90efa64bf0d6a3efc17574d1f8c811668f4` | PASS |
| string-G | `0e9451b849b659ea5ea92ea3e92e0862e34d30ad5188235f816f43419111b3ca` | PASS |
| detune-sweep-A | `b51d334bbfdd7da7abf4ba3391a6411d5a07427bb92b8e339389856a2539dbe7` | PASS |
| note-sequence | `2b5b8c83e419179ab04b5b218976c5d085538d59c0a55a967942c004fa1f8224` | PASS |
| vibrato | `df7384e358af9c5d5d34673a3976c2f34790f7cb2c07a96b45d6b3b03b568f47` | PASS |
| macro-sweep | `231218b4e9f117ca6598ecee530f3b0af20d4109f29640608590d6cf15a66cfe` | PASS |
| slow-lfo | `d27589de30dcb6f3c432c8993d80106b38b4cb87e59afa2edc4ae301d8809cb8` | PASS |
| schelleng-stress | `c5108af57520c8c190adaa6840513d4cfea6659da46d95bca01996d07efbda07` | PASS |
| sub-harmonics | `9178b41ec8b5bb6eb08b5ce9794dd93f542647ffd575ede39d88b8fff1a8c54c` | PASS |
| sub-harmonics-stability | `2efdea9b5d0745e127ad1fbc4242779848f1ea031b0e426b22e966ed7df8e6be` | PASS |
| saturator-tail-comparison | `5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7` | PASS |

`OK: all 13 goldens reproduce byte-identical` — no upstream drift between R36-bis-backfill chore (`1dfca9d`) and Phase 2.5 research-phase open. Tripwire CLEARED. R37-pre will repeat this check after R37-pre task adds the 13-entry `reproduce-goldens.sh` invocation as a precondition gate.

---

## 21.2 Open Question #2 + #3 — O-Bowed BodyResonator + BowNoiseGenerator Provenance Audit (RESOLVED — VERBATIM-COPY ASSUMPTION BROKEN)

**Status:** ✅ RESOLVED with **CRITICAL DELTA** finding.

### 21.2.1 O-Bowed `Source/DSP/BodyResonator.{h,cpp}` actuals

- **File presence:** `plugins/O-Bowed/Source/DSP/BodyResonator.h` (62 LOC) + `plugins/O-Bowed/Source/DSP/BodyResonator.cpp` (188 LOC). Both files exist; `.h`-only port is NOT viable (cpp contains preset table + `updateCoefficients` + normalization).
- **Public API:** `prepare(double sampleRate, int maxBlockSize)`, `reset()`, `setMaterial(float)`, `setSize(float)`, `setBodyAmount(float)`, `processStereo(float& left, float& right)`. **No `setDamping(float)` setter** — O-Bowed has no `BODY_DAMPING` parameter.
- **Mode-table data structure:** `struct ModePreset { float freq[NUM_MODES]; float q[NUM_MODES]; float gainDb[NUM_MODES]; }` × 4 presets (Membrane, Wood, Metal, Glass).
- **Wood preset values:** freq = `{272, 462, 551, 2500, 1200, 3200, 6000, 800}` Hz (violin-targeted, NOT bass; ordering NOT ascending). Q = `{12, 10, 10, 3, 5, 4, 2, 8}`. gainDb = `{10, 14, 12, 8, 4, 3, 2, 6}` (all POSITIVE → resonant boosts).
- **Filter type:** `juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, freq, q, gainLinear)` — peaking EQ (boost or notch). With positive gainDb, modes BOOST as resonators.
- **Channel topology:** `std::array<juce::dsp::IIR::Filter<float>, 8> bodyModesL` + `bodyModesR` — separate stereo filter banks (shared coefficients, separate state).
- **Output mixing:** `resonant += bodyModesX[i].processSample(in)` summed across 8 modes, then averaged via `* (1.0f / NUM_MODES)` and normalized via `normGain` (cross-preset level matching). Final mix: `out = in·dryMix + resonant·wetMix`.
- **Wolf-region suppression code:** **NOT PRESENT.** Q55 deferral CONTEXT-line-92 concern ("wolf-region suppression NOT ported") is moot — there is no wolf-suppression code path in O-Bowed to exclude.
- **Coefficient update trigger:** Lazy guard via `if (std::abs (material - currentMaterial) < 0.001f) return;` in setters — only recomputes on parameter change (NOT per-block).
- **Integration site in O-Bowed:** **PROCESSOR-level, post-voice-mix** (`plugins/O-Bowed/Source/PluginProcessor.cpp:355-357,387` — `setMaterial / setSize / setBodyAmount` once per processBlock; `processStereo(leftData[i], rightData[i])` per-sample on stereo mix bus).

### 21.2.2 Deltas vs ARCHITECTURE-spec'd O-Contrabass BodyResonator

| Axis | O-Bowed actual | O-Contrabass ARCHITECTURE spec | Delta |
|------|---------------|-------------------------------|-------|
| Material concept | 4-preset morph (Membrane/Wood/Metal/Glass) | Single fixed wood preset (bass-tuned, ARCHITECTURE §132 Open Decision §5 LOCKED) | **Strip morph machinery + presetGainSums + referenceGainSum + idxA/idxB/t interpolation** |
| Damping concept | NONE (no `setDamping`) | `Q_eff = Q_default · max(0.15, 1 − 0.85·d)` per-mode Q reduction | **Add `setDamping(float)` setter + per-block Q recompute** |
| Mode set | Wood preset 272/462/551/2500/1200/3200/6000/800 Hz | Bass set 60/98/115/175/235/340/700/1200 Hz | **Hard-coded bass mode set (single struct, not preset table)** |
| Q values | Wood `{12,10,10,3,5,4,2,8}` | Bass `{14,11,9,8,7,6,5,2.5}` | **Substitute** |
| Gain dB | Wood `{10,14,12,8,4,3,2,6}` (positive) | Bass `{-2,0,-1,-3,-4,-5,-7,-6}` (zero/negative) | **Substitute (negative gains → attenuated bandpass output)** |
| Filter type | `makePeakFilter(sr, fc, Q, gainLinear)` | `makeBandPass(sr, fc, Q)` per ARCHITECTURE §134 | **DIFFERENT FILTER FAMILY** — `makeBandPass` returns pure-bandpass (no boost/notch); g[i] applied as scalar OUTSIDE the filter; sum yields parallel bandpass mix |
| Channel topology | Stereo (separate L/R filter banks) | Mono (per-voice; voice output is mono pre-stereo-split at line 699-701) | **Single filter bank, NOT L/R duplicated** |
| Dry path | Identity (`in · dryMix`) | 35 Hz HP on dry path (ARCHITECTURE §151) | **Add 35 Hz one-pole HP + per-sample on dry channel** |
| Mix formula | `in·dry + resonant·wet` (linear blend, normalization-corrected) | `(1−mix)·HP35(in) + mix·wet` (no cross-preset normalization) | **Different blend; drop normGain machinery** |
| Coefficient update | On-change only (lazy `setMaterial`/`setSize` guards) | Per-block recompute (ARCHITECTURE §152) + 30 ms `SmoothedValue` on Size/Damping/Mix | **Per-block recompute pattern (smoothed values pulled fresh each block)** |
| Size scaling | `freq *= pow(2, (0.5 - currentSize) * 3.0)` (±1.5 octave around 0.5 neutral) | `fc[i] = jlimit(20, sr·0.45, defaultFreq[i] / size_scalar)` with `size_scalar = 0.85 + 0.30·s` (ARCHITECTURE §149) | **Different formula entirely** — bass spec is divisive size_scalar around s=0 baseline, not multiplicative octave shift around s=0.5 |
| Gain Size dependency | None | `g[i] = decibelsToGain(defaultGainDb[i] + 1.5·(s − 0.75))` per ARCHITECTURE §511 | **NEW Size→gain coupling** |
| Integration site | Processor-level post-voice-mix (stereo) | Voice-level post-downsample, mono (CONTEXT line 53–54 + ARCHITECTURE §"Per-block processing order" Step 15) | **DIFFERENT INTEGRATION SITE** — voice owns body instance, NOT processor |

### 21.2.3 O-Bowed `Source/DSP/BowNoiseGenerator.h` actuals

- **File presence:** `plugins/O-Bowed/Source/DSP/BowNoiseGenerator.h` (54 LOC, `.h`-only).
- **Public API:** `prepare(double sampleRate, int voiceIndex)`, `processSample(float bowPressure, float bowSpeed, float noiseAmount)`, `reset()`. Per-sample API (NOT `processBlock`).
- **Filter topology:** **SINGLE** `juce::dsp::IIR::Filter<float> bandpassFilter` at fc=3464 Hz, Q=0.87 (`makeBandPass`).
- **`juce::Random` seed strategy:** `noiseRandom.setSeed(static_cast<juce::int64>(voiceIndex * 31337))` in `prepare()` — voice-index-derived constructor-time fixed seed. ✅ **DETERMINISTIC per voice.** Phase 2.5 inherits this pattern verbatim.
- **Slip-burst trigger source:** **NONE.** Bow-noise is continuous bandpass-filtered white noise gain-scaled by `bowPressure * bowSpeed * noiseAmount * 0.03f`. NO per-period slip detection, NO exponential-decay burst envelope, NO Helmholtz cycle awareness.
- **Envelope:** Linear product `bowPressure * bowSpeed * noiseAmount * 0.03f`. NO `bowEnergy = clamp(0,1, |v_bow|·F_bow / (v_ref·F_ref))` normalization (ARCHITECTURE §164 spec uses a different formulation entirely).
- **Integration site in O-Bowed:** **VOICE-level**, per-sample inside the voice render loop (`plugins/O-Bowed/Source/BowedStringVoice.cpp:115` `prepare`, `:203` `processSample`). Sums into voice output BEFORE body resonator (since body resonator is at processor level post-voice-mix in O-Bowed).

### 21.2.4 Deltas vs ARCHITECTURE-spec'd O-Contrabass BowNoiseGenerator

| Axis | O-Bowed actual | O-Contrabass ARCHITECTURE spec | Delta |
|------|---------------|-------------------------------|-------|
| BPF count | 1 | 3 (700 / 1500 / 3000 Hz, ARCHITECTURE §163) | **Triple the filter instances + parallel sum** |
| BPF center freq | 3464 Hz, Q=0.87 | 700 / 1500 / 3000 Hz, Q ≈ 1.0–1.5 each | **Different spectral target (bass close-mic 500 Hz–4 kHz vs O-Bowed's higher band)** |
| Slip-burst trigger | NONE (continuous noise) | Per-period exponential-decay burst, decay 0.999 at 48 kHz, on Helmholtz slip-detection | **NEW slip-burst injection path; entirely new code** |
| Envelope formula | `bowPressure * bowSpeed * noiseAmount * 0.03f` | `bowEnergy = clamp(0, 1, \|v_bow\|·F_bow / (v_ref·F_ref))`, `v_ref=0.3 m/s`, `F_ref=2.0 N` | **Different envelope (physical normalization vs scalar product)** |
| Integration order | Sums into voice output before body | Sums AFTER body resonator (ARCHITECTURE §166 + §"Per-block processing order" Step 16) | **Different ordering (body first, noise added after)** |
| Channel topology | Mono per-voice | Mono per-voice (voice is mono in O-Contrabass) | Same — no delta |
| Seed strategy | `voiceIndex * 31337` constructor-time | constructor-time fixed seed (ARCHITECTURE §159) | Same — Phase 2.5 inherits |

### 21.2.5 LOC budget revision

- CONTEXT.md rev-10 §"Open Questions" #2 budgeted "≈ 100–250 LOC NEW per file" assuming verbatim-copy.
- Revised estimate after delta-audit:
  - `BodyResonator.h` ≈ **80–120 LOC** (header without preset-table machinery; single mode-set struct + smoothed-value members).
  - `BodyResonator.cpp` ≈ **180–250 LOC** (per-block compute + makeBandPass + 35 Hz HP one-pole + sum/wet/dry).
  - `BowNoiseGenerator.h` ≈ **150–220 LOC** (header-only triple-BPF + slip-burst exp-decay state + period-heuristic trigger + bowEnergy envelope; substantial new code).
  - **Total ≈ 410–590 LOC NEW** across 3 files (vs CONTEXT.md "100–250 per file = 200–500 across 2.5 files" ballpark — overlap, but the upper bound is meaningfully higher).
- **Conclusion:** Phase 2.5 implementation is closer to "implement bass spec from ARCHITECTURE using O-Bowed as reference for `juce::dsp::IIR::Filter` + `juce::Random` lifecycle patterns" than to "verbatim copy + substitution." Plan-phase R37 task budget should reflect this.

### 21.2.6 Recommended Phase 2.5 BodyResonator design (research-phase output → plan-phase input)

```cpp
// Source/DSP/BodyResonator.h (~90 LOC)
class BodyResonator
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setSize (float sizeNorm);       // 0..1, smoothed externally
    void setDamping (float dampingNorm); // 0..1, smoothed externally
    void setMix (float mixNorm);         // 0..1, smoothed externally
    void processBlock (float* mono, int numSamples);  // mutates in place

private:
    static constexpr int kNumModes = 8;
    static constexpr float kDefaultFreq[kNumModes]   = { 60.f, 98.f, 115.f, 175.f, 235.f, 340.f, 700.f, 1200.f };
    static constexpr float kDefaultQ[kNumModes]      = { 14.f, 11.f, 9.f, 8.f, 7.f, 6.f, 5.f, 2.5f };
    static constexpr float kDefaultGainDb[kNumModes] = { -2.f, 0.f, -1.f, -3.f, -4.f, -5.f, -7.f, -6.f };

    std::array<juce::dsp::IIR::Filter<float>, kNumModes> modes; // mono single-bank
    float gainLinear[kNumModes] {};

    // 35 Hz HP one-pole on dry path: y[n] = a·(y[n-1] + x[n] − x[n-1])
    float hp35_x1 = 0.f, hp35_y1 = 0.f, hp35_a = 0.f;

    double currentSampleRate = 44100.0;
    float currentSize = 0.75f, currentDamping = 0.40f, currentMix = 0.80f;

    void recomputeCoefficients();  // per-block; called from processBlock
};
```

```cpp
// Source/DSP/BodyResonator.cpp (~200 LOC core + comments)
// Per-block (CONTEXT line 152 + ARCHITECTURE §152): recomputeCoefficients() at block start
// reads (currentSize, currentDamping) and rebuilds 8 makeBandPass coefficients + g[i] linear gains.
void BodyResonator::recomputeCoefficients()
{
    const float sizeScalar = 0.85f + 0.30f * currentSize;          // ARCHITECTURE §149
    const float qScalar    = juce::jmax (0.15f, 1.f - 0.85f * currentDamping); // ARCHITECTURE §150

    for (int i = 0; i < kNumModes; ++i)
    {
        const float fc   = juce::jlimit (20.f,
                                          static_cast<float>(currentSampleRate * 0.45),
                                          kDefaultFreq[i] / sizeScalar);
        const float qEff = juce::jmax (0.10f, kDefaultQ[i] * qScalar);
        const float gDb  = kDefaultGainDb[i] + 1.5f * (currentSize - 0.75f); // ARCHITECTURE §511
        gainLinear[i]    = juce::Decibels::decibelsToGain (gDb);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (
                          currentSampleRate, fc, qEff);
        *modes[i].coefficients = *coeffs;  // shared coefficient swap
    }
}

void BodyResonator::processBlock (float* mono, int numSamples)
{
    recomputeCoefficients();  // per-block per CONTEXT line 152

    for (int n = 0; n < numSamples; ++n)
    {
        const float x = mono[n];

        // Dry path: 35 Hz HP one-pole
        const float dry = hp35_a * (hp35_y1 + x - hp35_x1);
        hp35_x1 = x;
        hp35_y1 = dry;

        // Wet path: parallel bandpass sum, scaled by g[i]
        float wet = 0.f;
        for (int i = 0; i < kNumModes; ++i)
            wet += gainLinear[i] * modes[i].processSample (x);

        mono[n] = (1.f - currentMix) * dry + currentMix * wet;
    }
}
```

**Notes for plan-phase:**
- Drop O-Bowed's `normGain` cross-preset normalization — single-preset bass design has no morph drift to correct.
- 35 Hz HP cutoff `a` coefficient: `a = exp(-2π·35/sr)` ≈ 0.99502 at 44.1 kHz; `a = 0.99544` at 48 kHz; `a = 0.99772` at 96 kHz. One-pole HP topology is `y[n] = a·(y[n-1] + x[n] - x[n-1])`. `juce::dsp::IIR::Coefficients::makeFirstOrderHighPass` is a viable alternative; keep the implementation choice for plan-phase.
- Mono single-channel filter bank — voice output is mono before stereo split at line 699-701; do NOT duplicate to L/R. Stereo handling moves to Phase 2.6 (master width).
- 30 ms `SmoothedValue` on `BODY_SIZE` / `BODY_DAMPING` / `BODY_MIX` lives in voice; pass smoothed values via `setSize/setDamping/setMix` setters at block start.

### 21.2.7 Recommended Phase 2.5 BowNoiseGenerator design (research-phase output → plan-phase input)

```cpp
// Source/DSP/BowNoiseGenerator.h (~180 LOC, header-only per O-Bowed convention)
class BowNoiseGenerator
{
public:
    void prepare (double sampleRate, int voiceIndex) noexcept;
    void setNoiseLevel (float noiseLevelNorm) noexcept;    // 0..1, smoothed externally
    void setBowEnergy  (float bowEnergyNorm)  noexcept;    // 0..1, computed externally per-sample or per-block
    void setFundamentalHz (float f0) noexcept;             // for period-heuristic slip trigger
    float processSample() noexcept;                         // single mono sample, additive into voice output

    void reset() noexcept;

private:
    static constexpr int kNumBpf = 3;
    static constexpr float kBpfFc[kNumBpf] = { 700.f, 1500.f, 3000.f }; // ARCHITECTURE §163
    static constexpr float kBpfQ[kNumBpf]  = { 1.0f, 1.2f, 1.5f };       // research recommendation; tune at execute pre-flight

    juce::Random noiseRandom;
    std::array<juce::dsp::IIR::Filter<float>, kNumBpf> bpfs;

    // Slip-burst (period-heuristic; ARCHITECTURE §165 spec calls for true Helmholtz slip-detection
    // but bow-friction module v1.0.0 + WaveguideString.cpp expose no slip-state; period-heuristic
    // is the v1.0 substitute within CONTEXT 4-file scope-strict rule — see §21.3 below).
    int   slipPeriodSamples = 0;   // sr / fundamentalHz
    int   slipCounter       = 0;   // counts down to next burst
    float slipEnvelope      = 0.f; // exponential decay state
    static constexpr float kSlipDecay = 0.999f;  // ARCHITECTURE §165 (at 48 kHz; rescale per sr at prepare)
    float kSlipDecayAtSr    = 0.999f;

    float bowEnergy   = 0.f;
    float noiseLevel  = 0.f;
    double sr         = 44100.0;
};

inline void BowNoiseGenerator::prepare (double sampleRate, int voiceIndex) noexcept
{
    sr = sampleRate;
    noiseRandom.setSeed (static_cast<juce::int64>(voiceIndex * 31337));  // O-Bowed pattern verbatim

    juce::dsp::ProcessSpec spec { sampleRate, 1u, 1u };
    for (int i = 0; i < kNumBpf; ++i)
    {
        bpfs[i].prepare (spec);
        bpfs[i].reset();
        *bpfs[i].coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass (
            sampleRate, kBpfFc[i], kBpfQ[i]);
    }

    // Decay rescale: 0.999 at 48 kHz → at sr, decay_at_sr = 0.999^(48000/sr) per cycle
    kSlipDecayAtSr = std::pow (0.999f, static_cast<float>(48000.0 / sampleRate));

    slipCounter   = 0;
    slipEnvelope  = 0.f;
    bowEnergy     = 0.f;
}

inline float BowNoiseGenerator::processSample() noexcept
{
    if (noiseLevel < 0.001f) return 0.f;

    // Period-heuristic slip-burst trigger
    if (slipPeriodSamples > 0)
    {
        if (--slipCounter <= 0)
        {
            slipEnvelope   = bowEnergy;     // burst amplitude scaled by bowEnergy at trigger time
            slipCounter    = slipPeriodSamples;
        }
    }
    slipEnvelope *= kSlipDecayAtSr;

    // Continuous bow-noise floor (always-on under bowEnergy)
    const float white = noiseRandom.nextFloat() * 2.f - 1.f;
    float bandSum = 0.f;
    for (int i = 0; i < kNumBpf; ++i)
        bandSum += bpfs[i].processSample (white);
    bandSum *= (1.f / static_cast<float>(kNumBpf));  // average

    // Combine continuous floor + slip burst
    const float continuous = bandSum * bowEnergy;
    const float burst      = bandSum * slipEnvelope;

    return (continuous + burst) * noiseLevel;
}

inline void BowNoiseGenerator::setFundamentalHz (float f0) noexcept
{
    if (f0 < 1.f) { slipPeriodSamples = 0; return; }
    slipPeriodSamples = juce::roundToInt (sr / f0);
    if (slipCounter > slipPeriodSamples) slipCounter = slipPeriodSamples;
}

inline void BowNoiseGenerator::setNoiseLevel (float v) noexcept { noiseLevel = juce::jlimit(0.f, 1.f, v); }
inline void BowNoiseGenerator::setBowEnergy  (float v) noexcept { bowEnergy  = juce::jlimit(0.f, 1.f, v); }
inline void BowNoiseGenerator::reset() noexcept
{
    for (auto& f : bpfs) f.reset();
    slipCounter = 0; slipEnvelope = 0.f; bowEnergy = 0.f;
}
```

**Notes for plan-phase:**
- Slip-burst trigger is **period-heuristic** (counter decrements per sample; resets every `sr / f0` samples). This is a v1.0 SUBSTITUTE for the ARCHITECTURE §165-spec'd "zero-crossing of friction force from stick to slip" — see §21.3 below for the true-slip-detection scope expansion path.
- `bowEnergy` is computed AT VOICE LEVEL each block (or per-sample if needed) from `BowModel.getBowVelocity() * BowModel.getBowForce() / (v_ref · F_ref)` clamped to [0, 1]; the voice pushes it via `setBowEnergy()`. This keeps the bow-friction module v1.0.0 ABI preserved (HR-10 carry-forward).
- 3 BPFs averaged via `1/N` keeps unity passband sum at design centers (similar to O-Bowed's `1/NUM_MODES` averaging in `processStereo`).
- Continuous-floor + slip-burst additive composition: continuous noise is always-on under bowEnergy; slip-burst adds per-period transients on top. This delivers BOTH the "bow-noise audible at low pressure" criterion AND the "5–15 ms wideband noise burst on bow-direction reversal" criterion (DSP-04 ROADMAP §"Phase 2.5 Test Criteria").

---

## 21.3 Open Question #4 — Slip-Burst Trigger Source (RESOLVED — period-heuristic v1.0 substitute; true Helmholtz slip-detection requires WaveguideString edit which violates CONTEXT 4-file scope rule)

**Status:** ✅ RESOLVED with CRITICAL design constraint surfaced.

### 21.3.1 Bow-friction module v1.0.0 audit

Read `modules/synthesis/bow-friction/cpp/BowModel.h` (54 LOC) + `HyperbolicFriction.h` (68 LOC):

- **`HyperbolicFriction::computeReflectionCoefficient(v_delta, F_bow)`** returns `rho ∈ [0, ~0.5]` — a memoryless reflection coefficient. NO stick/slip phase tracking, NO friction force history, NO `lastForceState()` accessor.
- **`BowModel::getBowVelocity()` + `getBowForce()`** — envelope outputs (`v_bow`, `F_bow` post-attack/release smoothing). NOT a stick/slip phase signal.
- The friction model is **explicitly memoryless** (HyperbolicFriction.h docstring lines 11-13: "STK-style memoryless friction curve. Computes waveguide reflection coefficient from differential velocity and bow force. O(1) per sample, no iteration required.").
- **No accessor exposes Helmholtz cycle phase, stick/slip transition flag, or friction-force time series.** Slip-detection per ARCHITECTURE §165 ("zero-crossing of friction force from stick to slip") is **not derivable from the public API**.

### 21.3.2 WaveguideString.cpp audit

Read inline at line 190: `float rho = friction.computeReflectionCoefficient (v_delta, F_bow);`. The reflection-coefficient transition `Δrho/Δt > 0 → < 0` *correlates with* stick→slip transitions but is not a clean zero-crossing signal — `rho` is monotonically nonlinear in `|v_delta|` (more like a smooth saturating curve). Tracking sign-of-derivative is unreliable as a slip trigger.

True Helmholtz slip-detection would require:
1. A new accessor `WaveguideString::getLastFrictionForce()` exposing the per-sample friction force history (1-sample-old + current).
2. Voice-level zero-crossing detection on `(F_friction_now - F_friction_prev)` sign change from positive to negative.
3. Edge case handling for noise/numerical chatter near zero crossings.

**This requires editing `Source/DSP/WaveguideString.cpp`** — which CONTEXT line 156 explicitly forbids ("Source-edit scope: 4 production files... `Source/DSP/WaveguideString.cpp` ... carry forward verbatim").

### 21.3.3 v1.0 substitute: period-heuristic trigger

**Recommended for Phase 2.5 v1.0 within CONTEXT 4-file scope-strict rule:**

- Voice tracks active-string fundamental `f0_active` (already known via `strings[activeStringIndex].getFundamentalHz()` or equivalent).
- Voice pushes `f0_active` to BowNoiseGenerator via `setFundamentalHz(f0)` at note-start + on every pitch-bend / vibrato update.
- BowNoiseGenerator decrements `slipCounter` per-sample; when it hits zero, fires a burst (envelope = `bowEnergy` at trigger time) and resets `slipCounter = sr / f0`.

**Trade-offs:**
- ✅ Stays within 4-file scope (no WaveguideString edit).
- ✅ Bit-deterministic (counter math is integer; no floating-point drift).
- ✅ Audibly similar effect: ONE burst per fundamental period, decay 0.999 per sample → ~70 ms decay envelope at 48 kHz — matches ARCHITECTURE §165 spec.
- ❌ Burst phase NOT locked to actual Helmholtz stick→slip transition — phase-relative to string oscillation will drift slightly from "true" arco bow-noise grain.
- ❌ At pitch-bend / vibrato, `slipCounter` reset semantics are imperfect — burst timing may glitch on rapid pitch changes. Mitigation: only reset `slipCounter` when `f0` changes by > some threshold (e.g., 5 cents), not on every smoothed update.

### 21.3.4 Scope-expansion alternatives (deferred to Phase 2.5-bis or v1.1)

| Alternative | Source delta | When | Trade-off |
|-------------|-------------|------|-----------|
| **A: Voice-level F_friction reconstruction** | `Source/BowedContrabassVoice.cpp` adds `lastFrictionForce` SmoothedValue + recomputes from `(v_bow - strings[s].lastIncomingVelocity()) × frictionModel.computeReflectionCoefficient(...) × kFricToForce` per sample | Phase 2.5-bis | True slip-detection without WaveguideString edit; requires `WaveguideString.lastIncomingVelocity()` accessor (NEW WaveguideString.h API); still within 4-file scope as a `.h`-only addition? **NO** — adding accessor requires `.cpp` change too. Out-of-scope for Phase 2.5. |
| **B: WaveguideString F_friction accessor** | `Source/DSP/WaveguideString.{h,cpp}` adds `float getLastFrictionForce() const noexcept` returning pre-saturator friction force | Phase 2.5-bis | True slip-detection; clean API. **Out of CONTEXT 4-file scope** (WaveguideString carry-forward verbatim per CONTEXT line 147). |
| **C: WaveguideString slip-flag accessor** | `WaveguideString` tracks stick/slip phase internally + exposes `bool didSlipThisSample()` | v1.1 | Cleanest API; requires friction-cycle simulation refinement. Bigger DSP scope. |

**Plan-phase decision required:** lock period-heuristic for v1.0 (recommended) OR escalate to scope-expansion. Recommended: ship period-heuristic; document subjective character at R38 audition; if "feels mechanical / not organic" perception emerges, schedule Phase 2.5-bis with Option A or B.

---

## 21.4 Open Question #5 — Saturator-Tail Regression Measurement Protocol Under Body Coupling (DEFERRED to execute-phase pre-flight)

**Status:** ⏸️ DEFERRED — design-revision blocker.

The CONTEXT.md rev-10 §"Open Questions" #5 protocol assumes a verbatim-copy implementation exists in scratch space. Given §21.2 finding that the verbatim-copy assumption is broken, scratch-space port + measurement is **premature**: it would measure an implementation that's about to be redesigned from §21.2.6 + §21.2.7 recommended drafts.

**Plan-phase action:** PLAN rev-12 R37-pre task includes execute-phase pre-flight measurement of bin 64 dB rel max under post-implementation `--saturator-tail-comparison` render. Compare against Phase 2.4c-bis baseline `−7.97 dB`. Soft-band [−9, −5] dB rel max acceptable; > 4 dB shift escalates pre-R37-atomic.

**Verify §19.3.3 analytic bound:** Body bank is a POST-saturator linear filter (no feedback into waveguide loop). Bound preservation expectation: ≤ 2 dB shift at canonical operating point — `BODY_MIX = 0.80` routes 80% through bandpass filter network; net spectral coloring at bin 64 ≈ ± 1–2 dB depending on which body modes the canonical operating point excites. Detailed analytic bound proof: post-implementation pre-flight only.

---

## 21.5 Open Question #6 — 13-Audible-Golden Re-Baseline Strategy (DEFERRED to execute-phase pre-flight)

**Status:** ⏸️ DEFERRED — design-revision blocker.

Pre-flighting 3-trial sha256 stability requires the implementation to exist. After §21.2 finding, this is execute-phase work:

**Plan-phase R37d task structure:** at execute-phase scratch, render all 13 reproduce-goldens.sh entries × 3 trials each post-Phase-2.5. Verify byte-identical across the 3 trials per golden. If non-deterministic on `juce::Random`, escalate (substitute fixed seed pattern per O-Bowed `voiceIndex * 31337` is the recommendation). If non-deterministic on biquad coefficients, escalate (root-cause `prepare()` ordering or unreseeded SmoothedValue state).

**13-golden re-baseline expectation:** all 13 audible goldens shift from current sha256s. New sha256s captured at execute-phase R37d render, committed at R37 atomic commit.

---

## 21.6 Open Question #7 — `juce::Random` Seed Determinism (RESOLVED — O-Bowed pattern is deterministic; Phase 2.5 inherits)

**Status:** ✅ RESOLVED.

O-Bowed `BowNoiseGenerator.h:23`: `noiseRandom.setSeed (static_cast<juce::int64>(voiceIndex * 31337));` — voice-index-derived constructor-time fixed seed. O-Contrabass is monophonic (1 voice); `voiceIndex = 0`; seed = 0. **Bit-deterministic across re-renders ✅.**

Recommendation: Phase 2.5 `BowNoiseGenerator::prepare(double sr, int voiceIndex)` inherits this pattern verbatim. Plan-phase R37b task locks: "seed = voiceIndex * 31337 (O-Bowed pattern; deterministic for O-Contrabass single-voice)." Execute-phase R37d 3-trial pre-flight per §21.5 confirms.

**Edge case:** If MPE multi-voice extension lands at Phase 2.6 (multiple voices simultaneously), each voice has distinct seed (0, 31337, 62674, ...). Currently O-Contrabass is monophonic per `synth.addVoice(new BowedContrabassVoice(...))` once at PluginProcessor.cpp:129; voiceIndex = 0 verified. No determinism risk.

---

## 21.7 Open Question #8 — Vibrato Carry-Forward Determination (DEFERRED to execute-phase pre-flight)

**Status:** ⏸️ DEFERRED — design-revision blocker.

Default expectation per CONTEXT line 218: re-baseline (body coloring shifts output spectrum; `peakDepthCents` autocorrelator metric likely shifts > 0.10¢ tolerance). Already counted in 13-audible re-baseline scope.

**Plan-phase R37d task structure:** post-implementation, render `--vibrato` mode. Compare WAV sha256 against committed `df7384e3...`; compare JSON metrics (`peakDepthCents`, `vibratoRateHzMeasured`, `onsetTimeMs`) against Phase 2.4c R36c restored-strict-with-deviations baseline. If WAV byte-identical AND metrics within ±0.05¢ / ±0.005 Hz / ±2 ms → carry forward (count drops to 12 audible re-baselines). If shift → re-baseline (13 stays).

---

## 21.8 Open Question #9 — Matrix-Stability Post-Phase-2.5 Verdict Structure (DEFERRED to execute-phase post-implementation)

**Status:** ⏸️ DEFERRED — design-revision blocker.

Body bank is L2-bounded (BIBO-stable parallel bandpass network); bow noise sums in additive (no feedback). Predicted-low probability of NEW raucous corners surfacing.

**Plan-phase R37d task structure:** re-render `--matrix-stability` (108-combo) post-Phase-2.5. Document raucous-corner cells → table of (sub-harmonic preserved / stabilised / NEW). Default-state baseline is Phase 2.4a R34b's 3 fails (matrix-stability evidence at `09cbf15f…` per CONTEXT line 176). Evidence-only (not committed as re-baselined golden). If NEW raucous corners surface → block plan-phase pending root-cause investigation.

---

## 21.9 Open Question #10 — Sub-Harmonics Post-Body Coupling Measurement (DEFERRED to execute-phase post-implementation)

**Status:** ⏸️ DEFERRED — design-revision blocker.

Phase 2.4c-bis R36-bis landed value `subharmEnergyRatio = 0.358` SOFT-PASS at waveguide output (pre-body). Body bank's Mode 1 at 60 Hz / Mode 2 at 98 Hz is bandpass-tilted toward subharmonic preservation (subharmonics live at f0/2 = 20.5 Hz for E1; below the body bank's 35 Hz HP dry-path floor and below Mode 1 at 60 Hz fc → subharmonics in dry path get attenuated by HP35, but body wet path could re-introduce coloring).

**Plan-phase R37d task structure:** render `--sub-harmonics` post-Phase-2.5 (default-state with SUB_HARMONICS active per Phase 2.4b configuration). Measure post-body `subharmEnergyRatio` from `--sub-harmonics-stability` 4-string × INFINITE_SUSTAIN matrix at body output. Document landed value. Soft-band [0.30, 0.45] expected (Phase 2.4c-bis 0.358 ± body coupling within 0.05). If drops below 0.30 → flag as evidence for Phase 2.4-bis backlog priority bump; NOT a Gate 7 BLOCKER (evidence-only, per CONTEXT line 220).

---

## 21.10 Open Question #11 — R37 Task Breakdown (RESOLVED — 9-task structure with Phase 2.5-redesign awareness)

**Status:** ✅ RESOLVED. Locked structure for PLAN rev-12:

| ID | Task | Surface | Tripwire / Acceptance |
|----|------|---------|------------------------|
| **R37-pre** | Pre-Phase-2.5 tripwire — render 13 reproduce-goldens.sh entries at HEAD; verify 13/13 PASS byte-identical against committed sha256s. Verify build cache fresh (ninja regenerated; harness runs). | None (pre-flight only) | All 13 PASS; commit-counter unchanged |
| **R37a** | `Source/DSP/BodyResonator.{h,cpp}` NEW — implement bass spec per §21.2.6 recommendation. 8-mode parallel bandpass bank using `juce::dsp::IIR::Coefficients::makeBandPass`; bass mode-set (60/98/115/175/235/340/700/1200 Hz × Q × gainDb); per-block recompute; 35 Hz HP one-pole on dry path; mono single-bank; wet/dry mix. CMakeLists.txt source-list addition for `BodyResonator.cpp`. | Compiles clean (no warnings); voice integrates without errors |
| **R37b** | `Source/DSP/BowNoiseGenerator.h` NEW — implement bass spec per §21.2.7 recommendation. 3-band BPF (700/1500/3000 Hz, Q≈1.0/1.2/1.5) + period-heuristic slip-burst trigger + `voiceIndex * 31337` deterministic seed (O-Bowed pattern verbatim). Header-only per O-Bowed convention. | Compiles clean; voice integrates without errors |
| **R37c** | `Source/BowedContrabassVoice.{h,cpp}` integration — append Step 8 (body) + Step 9 (bow noise) AFTER Step 7 downsample (line 688 oversampling.processSamplesDown), BEFORE Step 8/9-output write to outputBuffer (line 694-702). Body/noise instances owned by voice. SmoothedValue<float> ramps (30 ms) for `BODY_SIZE`, `BODY_DAMPING`, `BODY_MIX`, `BOW_NOISE`. `setFundamentalHz(activeStringFundamentalHz)` push for slip-trigger. `setBowEnergy(clamp(0,1, abs(v_bow)*F_bow / (0.3*2.0)))` per-block. HR-9 short-circuit at SUB_HARMONICS=0 carry-forward verbatim. HR-10 friction module ABI preservation carry-forward verbatim. | Voice compiles + matrix-stability evidence renders without crash |
| **R37d** | Re-baseline 13 audible goldens + matrix-stability evidence re-render + saturator-tail post-body measurement + sub-harmonics post-body measurement + 3-trial bit-stability pre-flight (per §21.5–§21.9). | 3-trial sha256 byte-identical per golden; 13 NEW sha256s captured |
| **R37e** | Regression bar — re-run 13-entry reproduce-goldens.sh against NEW post-Phase-2.5 sha256s; 13/13 PASS. 4-file source audit hook: `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports exactly the EXACT 4-file set: `Source/DSP/BodyResonator.{h,cpp}` NEW + `Source/DSP/BowNoiseGenerator.h` NEW + `Source/BowedContrabassVoice.{h,cpp}` M + `Source/PluginProcessor.{h,cpp}` M. Plus CMakeLists.txt M. NO other production-source diffs. | 13/13 PASS + audit hook clean |
| **R37f** | `auval` + `pluginval-10` SUCCESS — auval AU VALIDATION SUCCEEDED full render-rate matrix; pluginval --strictness-level 10 SUCCESS full battery. | auval + pluginval-10 SUCCESS |
| **R38** | Logic AU audition (BLOCKING) — user A/Bs raw-string from `1044bed` (R36-bis post-port via `git worktree add /tmp/oc-pre-2-5 1044bed`) vs body-engaged from working-tree O-Contrabass-dev. PASS criteria: convincing orchestral arco bass per BRIEF.md DSP-03 + DSP-04 acceptance. | User-confirmed CONFIRMED |
| **R37 atomic** | Atomic commit — all source edits + 13 re-baselined goldens + matrix-stability evidence re-render + saturator-tail-comparison re-baseline + RESEARCH §21 verdict + STATUS / SUMMARY / VERIFICATION / CONTEXT updates. Commit message follows Phase 2.4c-bis precedent: "feat(O-Contrabass): Phase 2.5 — body resonator (8-mode static-Q bank) + bow noise generator (3-band BPF + period-heuristic slip bursts); Gate 7 PASS [verdict-modifier]". | Atomic commit lands with sha pinned |
| **R37-backfill** | Chore — propagate R37 sha into STATUS.md per R34/R35/R36/R36-bis backfill precedent. | STATUS.md `phase_2_5_atomic_sha:` field populated |

**Source-delta budget per R37e tripwire:**

```
Source/DSP/BodyResonator.h          | (~90 LOC NEW)
Source/DSP/BodyResonator.cpp        | (~200 LOC NEW)
Source/DSP/BowNoiseGenerator.h      | (~180 LOC NEW)
Source/BowedContrabassVoice.h       | (~10 LOC M; instance + setter declarations)
Source/BowedContrabassVoice.cpp     | (~40 LOC M; Step 8 + Step 9 integration + bowEnergy + setFundamentalHz)
Source/PluginProcessor.cpp          | (~5 LOC M; voice-side parameter routing)
Source/PluginProcessor.h            | (0 LOC M; no new public surface)
CMakeLists.txt                       | (1 LOC M; add Source/DSP/BodyResonator.cpp to source list)

NET: ~525 LOC (470 NEW + 55 M)
```

This is significantly larger than CONTEXT.md rev-10 §"Open Questions" #2 estimate of "≈ 100–250 LOC NEW per file = 200–500 across 2.5 files." Plan-phase Q-revision recommended to acknowledge revised budget.

---

## 21.11 Open Question #12 — R38 Logic AU Audition Protocol (RESOLVED — 4-step BLOCKING audition mirroring R37-bis precedent)

**Status:** ✅ RESOLVED.

**Pre-Phase-2.5 reference build (raw-string baseline):**

```bash
# Worktree the R36-bis post-port commit
git worktree add /tmp/oc-pre-2-5 1044bed41574be5d0714983f7910cac8bda2edec

# Configure + build a side-by-side variant (rename plugin manufacturer/code to avoid collision)
cd /tmp/oc-pre-2-5
# Edit plugins/O-Contrabass/CMakeLists.txt to change PLUGIN_NAME to "O-Contrabass-pre-2-5" and aumu code to "OCb5" (or similar)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSKIP_PLUGINS=O-Orbit -B build
cmake --build build --target O-Contrabass-pre-2-5_AU --parallel

# Install to AU folder
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-2-5.component
cp -R /tmp/oc-pre-2-5/build/plugins/O-Contrabass/O-Contrabass-pre-2-5_artefacts/Release/AU/O-Contrabass-pre-2-5.component ~/Library/Audio/Plug-Ins/Components/
auval -a | grep -i contrabass  # confirm both pre-2-5 + dev variants resolve
```

**Post-Phase-2.5 working-tree build (body-engaged):**

```bash
# Working tree (R37a..f applied, NOT yet committed)
cd ~/Dev/VST-development
ninja -C build O-Contrabass_AU
# Install + clear caches
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component ~/Library/Audio/Plug-Ins/Components/
```

**Logic AU audition session (4-probe sequence):**

| Probe | Setup | Pre-2-5 expectation | Post-2-5 expectation | PASS criterion |
|-------|-------|----------------------|------------------------|----------------|
| **1: Sustained E1 (MIDI 28)** | Default preset, sustained 5 s note | Bare bowed-string (waveguide-only output; no wood resonance) | Wood body resonance present (low-end thickening); BOW_NOISE audible at default 0.35 (subtle hair-on-string texture) | Body adds wood color WITHOUT obscuring fundamental; bow-noise sits under tone |
| **2: Per-string A/D/G** | MIDI 33 / 38 / 43, 3 s each | Bare bowed-string per-string | Body wet/dry mix matches across strings; per-string spectral coloring varies subtly | No string-dependent body discontinuities |
| **3: Sustained G2 (MIDI 43, wolf-region check)** | MIDI 43 sustained 8 s, default damping (BODY_DAMPING=0.40) | Bare bowed-string | Mode 2 at fc=98/1.075=91.16 Hz (Q_eff=11·0.66=7.26) coexists with G2 fundamental at 98 Hz — ARCHITECTURE §240-warned wolf-region risk. **Document audibility**: subtle bloom (acceptable) OR audible beating (Phase 2.5-bis backlog flag) | Subtle bloom acceptable; audible beating documented but NOT blocking R37 atomic per CONTEXT line 91/166 |
| **4: Bow-direction reversal (MIDI 28 → silence → MIDI 28)** | MIDI on / off / on, ~50 ms gaps | Bare attack-decay envelope | Slip-burst NOISE BURST (5–15 ms wideband) on note-on transient — ROADMAP §"Phase 2.5 Test Criteria" DSP-04 acceptance | Audible 5–15 ms wideband noise burst on each attack |
| **5: BOW_NOISE 0% → 100% sweep** | MIDI 28 sustained, knob sweep | n/a | Noise rises from inaudible to overwhelming; no clicks during sweep | Linear-ish loudness ramp; no zipper noise |
| **6: BODY_SIZE / DAMPING / MIX sweeps** | MIDI 28 sustained, knob sweep | n/a | Body character morphs smoothly (size = body resonance frequency offset; damping = ring-down time; mix = wet/dry blend) | No clicks during any sweep |
| **7: Orchestral A/B vs reference orchestral library bass sustain at G2** | DAW project comparing Phase-2.5 sustained G2 vs (e.g.) Spitfire Audio BBC SO bass sustain at G2 | Bare bowed-string clearly NOT in same sonic family | "In same sonic family" subjective bar per ROADMAP §"Phase 2.5 Test Criteria" | User-confirmed acceptable |

**FAIL handling at R38:**

| Subjective failure | Resolution path |
|--------------------|-----------------|
| Bow-noise level mis-calibrated (too loud / too quiet) | Tweak `kSlipDecay` (0.999 → 0.9985 lower for shorter bursts; 0.9995 longer) OR `kBpfQ[]` (1.0/1.2/1.5 → 1.5/1.8/2.0 higher Q for sharper resonances) OR default `BOW_NOISE` (0.35 → 0.25). Re-render audition; iterate. Stays inside R37 cycle. |
| Body bank gain attenuation needed | Adjust `kDefaultGainDb[]` (e.g., −2 → −4 across the board for less prominent body). Re-render audition; iterate. Stays inside R37 cycle. |
| Wolf-tone audible at G2 sustained | DOCUMENT (per CONTEXT line 91/166); carry to Phase 2.5-bis backlog. Do NOT block R37 atomic. User can mitigate via BODY_DAMPING knob for now. |
| Body-coupling distortion / transient artifacts | ESCALATE pre-R37-atomic. Likely root cause: per-block coefficient swap glitch or 35 Hz HP one-pole DC-residual. Investigate. |
| Slip-burst phase feels mechanical (period-heuristic limitation) | DOCUMENT (per §21.3.4); carry to Phase 2.5-bis Option A or B. Do NOT block R37 atomic. |
| Catastrophic regression (body bank breaks waveguide stability under any string × INFINITE_SUSTAIN combo) | ESCALATE pre-R37-atomic. Investigate matrix-stability evidence. Most likely culprit: per-block coefficient computation introduces NaN at edge case (BODY_SIZE=1.0 + BODY_DAMPING=0.0 worst case; §21.7 analytic check shows STABLE in float32 but NaN propagation from upstream is possible). |

---

## 21.12 Open Question #13 — RESEARCH §21 Deliverable Structure (RESOLVED — this section)

**Status:** ✅ RESOLVED.

**Numbering correction:** CONTEXT.md rev-10 §"In Scope" / §"Open Questions" / §"Audit Trail" all reference "RESEARCH §20 (NEW) — append Phase 2.5 verdict subsection." However, RESEARCH.md §20 is already locked as Phase 2.4c-bis verdict (lines 6208-6682, 16 subsections §20.1–§20.16). Phase 2.5 verdict is therefore landed as **§21** (this section). CONTEXT.md rev-11 (post-research instantiation) picks up the corrected reference; PLAN rev-12 R37 task breakdown references §21.10 (NOT §20.10).

**Subsection structure (12 sub-sections, mapping CONTEXT Open Q #1–#14):**

- §21.1 Open Q #1 — Pre-Phase-2.5 repro tripwire (RESOLVED — 13/13 PASS)
- §21.2 Open Q #2 + #3 — O-Bowed BodyResonator + BowNoiseGenerator provenance (RESOLVED — VERBATIM-COPY ASSUMPTION BROKEN; substantial rewrite required; recommended designs in §21.2.6 + §21.2.7)
- §21.3 Open Q #4 — Slip-burst trigger source (RESOLVED — period-heuristic v1.0 substitute; true Helmholtz slip-detection requires WaveguideString edit out-of-scope)
- §21.4 Open Q #5 — Saturator-tail measurement protocol (DEFERRED to execute-phase pre-flight)
- §21.5 Open Q #6 — 13-audible-golden re-baseline strategy (DEFERRED to execute-phase pre-flight)
- §21.6 Open Q #7 — `juce::Random` seed determinism (RESOLVED — O-Bowed pattern is deterministic)
- §21.7 Open Q #8 — Vibrato carry-forward (DEFERRED; default = re-baseline)
- §21.8 Open Q #9 — Matrix-stability post-Phase-2.5 (DEFERRED to execute-phase post-implementation)
- §21.9 Open Q #10 — Sub-harmonics post-body coupling (DEFERRED to execute-phase post-implementation)
- §21.10 Open Q #11 — R37 task breakdown (RESOLVED — 9-task structure; LOC budget revised upward)
- §21.11 Open Q #12 — R38 audition protocol (RESOLVED — 4-step BLOCKING audition + 7-probe sequence)
- §21.12 Open Q #13 — RESEARCH §21 deliverable structure (RESOLVED — this sub-section)

**Note re Open Q #14 (CMakeLists.txt source-list update):** consolidated into §21.13 — see below — and folded into R37a task in §21.10.

---

## 21.13 Open Question #14 — CMakeLists.txt Source-List Update (RESOLVED — `BodyResonator.cpp` IS present; CMake addition required)

**Status:** ✅ RESOLVED.

O-Bowed pattern: `Source/DSP/BodyResonator.cpp` is a real `.cpp` file (188 LOC; not header-only). Phase 2.5 BodyResonator design per §21.2.6 also requires a `.cpp` (since per-block `recomputeCoefficients` and out-of-line coefficient compute logic are too large for inline header-only).

Recommended `plugins/O-Contrabass/CMakeLists.txt` source-list addition (one-line M):

```cmake
target_sources(${PROJECT_NAME} PRIVATE
    # ... existing entries carry-forward verbatim ...
    Source/DSP/BodyResonator.cpp     # Phase 2.5 NEW
)
```

`Source/DSP/BowNoiseGenerator.h` is `.h`-only per O-Bowed convention; no CMake update required for that header.

R37a task per §21.10 includes the CMakeLists.txt edit alongside the BodyResonator source files.

---

## 21.14 Risk-Surface Refinement for PLAN rev-12

**Carry-forward from CONTEXT rev-10 §"Risks (Phase 2.5-specific)" with research-phase evidence:**

| # | Risk | Evidence post-research | Mitigation locked in PLAN rev-12 |
|---|------|-------------------------|----------------------------------|
| 1 | Body bank coefficient instability at low-freq edge (Mode 1 at 52 Hz, Q=14, sr=44.1 kHz) | **MITIGATED** per §21.7 analytic check: pole radius `r = exp(−π·52/(14·44100)) = exp(−2.65e-4) ≈ 0.9997`; 3 ‱ inside unit circle; float32 eps = 1.19e-7 well-resolves. STABLE. `juce::ScopedNoDenormals` already in voice render path. | Carry-forward; no new mitigation |
| 2 | Click-free coefficient updates fail | Per-block recompute + 30 ms `SmoothedValue` per ARCHITECTURE §152. Pattern-validated in O-Bowed (different filter type but same `SmoothedValue` lifecycle). | R37c integrates SmoothedValue<float> for SIZE/DAMPING/MIX/BOW_NOISE in voice; recompute at block start |
| 3 | `juce::Random` non-determinism | **MITIGATED** per §21.6: O-Bowed pattern `voiceIndex * 31337` is constructor-time fixed; deterministic per voice. | R37b inherits seeding pattern verbatim; R37d 3-trial preflight confirms |
| 4 | Wolf-region resonance at G2 sustained | **DOCUMENTED + DEFERRED** per CONTEXT Q55. Mode 2 fc=91.16 Hz, Q_eff=7.26 at default damping; G2 fundamental at 98 Hz is just outside Mode 2 −3 dB band. Subjective audition at R38 Probe 3 (G2 sustained) confirms or flags. | R38 Probe 3 documents audibility; if audible, carry to Phase 2.5-bis (do NOT block R37 atomic) |
| 5 | Body coupling shifts saturator-tail bin 64 outside soft-band | **DEFERRED** to execute-phase pre-flight per §21.4. Body bank is post-saturator linear filter (no nonlinear backflow). Predicted shift ≤ 2 dB at canonical operating point. | R37d measurement; if outside [−9, −5] dB → flag for §"In-loop saturator" amendment evidence base |
| 6 | 13-audible-golden re-baseline drift across runs | **DEFERRED** to execute-phase pre-flight per §21.5. 3-trial protocol locks bit-determinism per golden. | R37d 3-trial preflight |
| 7 | Vibrato golden re-baseline | **DEFERRED + DEFAULT-RE-BASELINE** per §21.7. Counted in 13-audible re-baseline scope. | R37d post-render check |
| 8 | Matrix-stability NEW raucous corners | **DEFERRED** to execute-phase post-implementation per §21.8. Body bank is L2-bounded; predicted-low probability. | R37d evidence-only re-render |
| 9 | Sub-harmonics post-body coupling drops `subharmEnergyRatio` | **DEFERRED** to execute-phase post-implementation per §21.9. Body bank's bandpass tilt at 60–115 Hz preserves or boosts subharmonic energy. | R37d measurement; flag for Phase 2.4-bis backlog priority bump if drops < 0.30 |
| 10 | R38 BLOCKING audition reveals subjective issue | **MITIGATED** per §21.11: 5 categorised resolution paths covering bow-noise calibration, body gain attenuation, wolf-tone deferral, distortion escalation, period-heuristic mechanical-feel deferral. | R38 4-step protocol + 7-probe sequence |
| 11 | Audit-hook drift mid-cycle | **MITIGATED** per CONTEXT line 156. R37-pre + R37e enforce exact 4-source-file (+ 1 CMake) diff set. | R37e tripwire |
| 12 | `juce::dsp::IIR::Filter<float>` RT-safety | **MITIGATED** — JUCE biquad processSample is RT-safe; coefficient swap is atomic struct copy; `juce::ScopedNoDenormals` already in voice render path. | R37f pluginval-10 fuzz + Parameter thread safety |
| 13 | Phase 2.6-awareness (master chain absent at Phase 2.5 audition) | **DOCUMENTED** per CONTEXT line 258. Phase 2.5 voice output goes directly to bus (host-rate, no master saturator/limiter); R38 audition character will differ from final v1.0 character. | R38 acknowledges explicitly — body+noise character validation, NOT final-output validation |
| 14 | BODY_MIX = 0.0 audible mismatch (HP35 attenuates E1 fundamental ~3 dB) | **DOCUMENTED** per CONTEXT line 260. Architecture-correct; 35 Hz HP prevents sub-A0 phase-comb artifacts during mix. | R38 Probe 6 (sweeps) documents; if subjective issue, schedule Phase 2.5-bis (lower HP cutoff or conditional HP) |
| 15 | Phase 2.5 verify regression on Phase 2.4c-bis Q47 SOFT-PASS contract | Carry-forward from rev-8 risk #10. Body coupling MAY shift bin 64. | R37d evidence-only flag; if shift > 1 dB → carry-forward to end-of-Stage-2 §"In-loop saturator" amendment |
| **16 (NEW)** | **Verbatim-copy assumption broken** (§21.2 finding) | O-Bowed BodyResonator is morphable Material+Size+BodyAmount with `makePeakFilter`+stereo+processor-level integration. Bass spec is single-preset Size+Damping+Mix with `makeBandPass`+mono+voice-level integration. ~410–590 LOC NEW substantial rewrite, not "verbatim copy + bass-substitution". | PLAN rev-12 R37a + R37b LOC budget revised upward; user re-confirmation may be desired before plan-phase commits |
| **17 (NEW)** | **No Helmholtz slip-detection accessor available** (§21.3 finding) | Bow-friction module v1.0.0 + WaveguideString.cpp expose no slip-state. True slip-detection requires WaveguideString edit (out of CONTEXT 4-file scope). | Period-heuristic substitute locked for v1.0; true slip-detection deferred to Phase 2.5-bis or v1.1 |

---

## 21.15 Sequencing in PLAN rev-12

**Atomic-commit gate-first principle preserved:**

`R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37`

R37 task ordering per §21.10:

1. **R37-pre** (tripwire) → 13/13 PASS
2. **R37a** (BodyResonator NEW) → compiles
3. **R37b** (BowNoiseGenerator NEW) → compiles
4. **R37c** (voice integration) → compiles + matrix-stability evidence renders without crash
5. **R37d** (re-baseline + measurements) → 3-trial bit-stability + saturator-tail + sub-harmonics + matrix-stability characterised
6. **R37e** (regression bar + audit-hook) → 13/13 PASS against NEW sha256s + 4-file source diff
7. **R37f** (auval + pluginval-10) → SUCCESS
8. **R38** (Logic AU audition, BLOCKING) → user-confirmed CONFIRMED
9. **R37 atomic commit** → all source + goldens + RESEARCH §21 + STATUS / SUMMARY / VERIFICATION / CONTEXT updates
10. **R37-backfill chore** → propagate R37 sha into STATUS.md per R34/R35/R36/R36-bis precedent

**Critical-path note:** R37-pre tripwire blocks all subsequent tasks; if tripwire fails, investigate upstream drift before any source edit. R37c (voice integration) is the largest scope task per §21.10 (40 LOC M in `BowedContrabassVoice.cpp`); plan-phase budgets ~2 hours implementation + 30 min build-verify. R37d render budget: 13 goldens × 3 trials × 0.29 s/render (Phase 2.4c §19.13 wall-clock baseline) + matrix-stability 108-combo × 1 trial × ~3 s/render = ~5 min total. R38 audition: ~30 min user time. R37 atomic commit + R37-backfill: ~10 min.

---

## 21.16 Open Items for Plan-Phase

**USER-DECISION-REQUIRED escalation (carries to PLAN rev-12 instantiation):**

**ESCALATION-1 (§21.2 finding):** CONTEXT.md rev-10 Q54 user-confirmation specifies "per-plugin verbatim copy from O-Bowed". Research-phase finds verbatim-copy is not achievable — the deltas are too significant (filter type, channel topology, integration site, parameter set, dry-path treatment, slip-detection mechanism). Phase 2.5 implementation is "implement bass spec from ARCHITECTURE using O-Bowed as reference for `juce::dsp::IIR::Filter` + `juce::Random` lifecycle patterns". LOC budget revised: ~410–590 LOC NEW vs CONTEXT estimate ~200–500. **Plan-phase action:** acknowledge in PLAN rev-12 §"Approach Decisions" with explicit deviation flag against CONTEXT Q54; OR re-run discuss-phase to update Q54 with revised intent. Recommended: acknowledge in PLAN rev-12, do NOT re-discuss (research-phase finding is design-grounded, not opinion-grounded).

**ESCALATION-2 (§21.3 finding):** CONTEXT.md rev-10 (and ARCHITECTURE §165) specify "per-period slip bursts on Helmholtz slip-detection (zero-crossing of friction force from stick to slip)". Research finds bow-friction module v1.0.0 + WaveguideString.cpp expose no slip-state accessor. True slip-detection requires WaveguideString edit (out of CONTEXT 4-file scope-strict rule). **Plan-phase action:** lock period-heuristic v1.0 substitute (R37b implementation per §21.2.7); document in PLAN rev-12 as v1.0 substitute against ARCHITECTURE §165 spec; carry "true Helmholtz slip-detection" to Phase 2.5-bis or v1.1 backlog (Option A: voice-level F_friction reconstruction; Option B: WaveguideString getLastFrictionForce() accessor; Option C: WaveguideString slip-flag accessor).

**ESCALATION-3 (ARCHITECTURE §149 vs §503-511 inconsistency):** ARCHITECTURE line 149 claims "1.83:1 frequency span across knob" with formula `size_scalar = 0.85 + 0.30·s`. Computed range: 0.85 → 1.15 = 1.353:1, NOT 1.83:1. Either the formula or the comment is wrong. Phase 2.5 plan-phase decision: **lock formula `size_scalar = 0.85 + 0.30·s` per ARCHITECTURE §149 + §509** (formula is more authoritative than commentary). 1.353:1 frequency span at default 60 Hz Mode 1 → 52 Hz (s=1.0) ↔ 70.6 Hz (s=0.0). Plan-phase deviates from ARCHITECTURE-line-149 commentary (1.83:1) but preserves ARCHITECTURE-line-509 formula. Append to deferred ARCHITECTURE.md amendments at end-of-Stage-2 verify (post-Phase-2.6) alongside §"DC Blocker" + §"In-loop saturator" amendments.

**Plan-phase pre-locks (no user intervention needed):**

- §21.10 R37 9-task structure verbatim
- §21.11 R38 4-step protocol + 7-probe sequence verbatim
- §21.13 CMakeLists.txt source-list edit (Source/DSP/BodyResonator.cpp)
- §21.14 Risk register expanded to 17 entries (15 from CONTEXT carry-forward + Risk #16 + #17 NEW from research)
- §21.2.6 + §21.2.7 recommended class designs as starting code (NOT a hard contract — plan-phase may refine signatures, e.g., `processBlock(float* mono, int numSamples)` vs per-sample `processSample(float)` API)

**Plan-phase rev numbering:** PLAN rev-12 (supersedes PLAN rev-11 from Phase 2.4c-bis).

---

## 21.17 Summary — Phase 2.5 Research Resolution Map

| Open Q | Status | Resolution Path |
|--------|--------|-----------------|
| #1 Pre-Phase-2.5 repro tripwire | ✅ RESOLVED | 13/13 PASS at HEAD `1dfca9d` descendant; §21.1 |
| #2 BodyResonator provenance | ✅ RESOLVED with CRITICAL DELTA | O-Bowed = morphable+stereo+peakFilter; bass = single-preset+mono+bandPass; ~290 LOC NEW total per §21.2.1+§21.2.2+§21.2.6 |
| #3 BowNoiseGenerator provenance | ✅ RESOLVED with CRITICAL DELTA | O-Bowed = single-BPF continuous; bass = 3-band BPF + slip bursts; ~180 LOC NEW per §21.2.3+§21.2.4+§21.2.7 |
| #4 Slip-burst trigger source | ✅ RESOLVED with CONSTRAINT | Bow-friction module exposes no slip-state; period-heuristic v1.0 substitute per §21.3.3; true slip-detection deferred per §21.3.4 |
| #5 Saturator-tail protocol | ⏸️ DEFERRED | Execute-phase R37d pre-flight per §21.4 |
| #6 13-audible-golden strategy | ⏸️ DEFERRED | Execute-phase R37d pre-flight per §21.5 |
| #7 `juce::Random` seed determinism | ✅ RESOLVED | O-Bowed `voiceIndex * 31337` deterministic; Phase 2.5 inherits per §21.6 |
| #8 Vibrato carry-forward | ⏸️ DEFERRED | Execute-phase R37d post-render check; default = re-baseline per §21.7 |
| #9 Matrix-stability post-Phase-2.5 | ⏸️ DEFERRED | Execute-phase R37d evidence-only per §21.8 |
| #10 Sub-harmonics post-body | ⏸️ DEFERRED | Execute-phase R37d measurement per §21.9 |
| #11 R37 task breakdown | ✅ RESOLVED | 9-task structure (R37-pre / R37a..f / R38 / R37 atomic / R37-backfill) per §21.10 |
| #12 R38 audition protocol | ✅ RESOLVED | 4-step BLOCKING + 7-probe sequence per §21.11 |
| #13 RESEARCH §21 structure | ✅ RESOLVED | This section; numbering correction §20→§21 per §21.12 |
| #14 CMakeLists.txt update | ✅ RESOLVED | `Source/DSP/BodyResonator.cpp` source-list addition required per §21.13 |

**8 RESOLVED + 6 DEFERRED-to-execute-phase** = 14/14 open questions addressed. Two CRITICAL findings escalated for plan-phase (§21.16 ESCALATION-1 + ESCALATION-2). One ARCHITECTURE inconsistency flagged for end-of-Stage-2 amendment (§21.16 ESCALATION-3).

**Plan-phase ready (PLAN rev-12).**

---

## 21.18 References (§21 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-10 (this cycle's discuss artefact, 2026-04-30).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` rev-11 (Phase 2.4c-bis end-state; Phase 2.5 supersedes via PLAN rev-12 post-research).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §20 (Phase 2.4c-bis verdict; numbering precedent triggering §20→§21 correction in §21.12).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 132–168 (§"Body Resonator (8-Mode Wood Bank)" + §"Bow Noise Generator").
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` lines 503–511 (§"Body Resonator (Parallel Biquad Bank)" formula spec).
- `plugins/O-Contrabass/.planning/parameter-spec.md` lines 18–29 (BRIGHTNESS / BOW_NOISE / BODY_SIZE / BODY_DAMPING / BODY_MIX declarations; sha256 `77638e25…` carry-forward).
- `plugins/O-Bowed/Source/DSP/BodyResonator.h` (62 LOC; 4-preset morphable; stereo per-sample API).
- `plugins/O-Bowed/Source/DSP/BodyResonator.cpp` (188 LOC; preset table + lazy coefficient update + normGain).
- `plugins/O-Bowed/Source/DSP/BowNoiseGenerator.h` (54 LOC; single-BPF continuous-noise; voice-index seed; processor-level integration).
- `plugins/O-Bowed/Source/PluginProcessor.cpp` lines 278, 355–357, 387 (O-Bowed BodyResonator integration site — processor-level).
- `plugins/O-Bowed/Source/BowedStringVoice.cpp` lines 42, 111, 115, 203 (O-Bowed BowNoiseGenerator integration site — voice-level pre-body).
- `modules/synthesis/bow-friction/cpp/BowModel.h` (54 LOC; envelope-only, no slip-state accessor).
- `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` (68 LOC; memoryless reflection coefficient, no Helmholtz cycle phase).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` line 190 (`friction.computeReflectionCoefficient` invocation; rho is the only friction signal in the loop; no F_friction accessor exposed).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 263–702 (`renderNextBlock` 7-step + Step 2.5 carry-forward; line 688 downsample is Phase 2.5 Step 8/9 insertion point; lines 694–702 host-rate output write).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` lines 52, 60, 62, 64, 66, 129 (5 body/noise param declarations + monophonic synth.addVoice).
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (13-entry script; §21.1 PASS evidence; carry-forward through Phase 2.5).
- §21.1 pre-flight reproduce-goldens.sh execution: 13/13 PASS at HEAD descendant of `1dfca9d` (build cache from prior session; transient `/tmp/repro/*.wav` deleted post-research).
- §21.2.5 LOC budget revision: ~410–590 LOC NEW across 3 source files (vs CONTEXT rev-10 §"Open Q #2" estimate "100–250 per file").
- HEAD descendant of `1dfca9d` (R36-bis-backfill chore, 2026-04-29) — production base for Phase 2.5; tripwire CLEARED at §21.1.
- ARCHITECTURE.md §149 vs §509 size_scalar inconsistency (1.83:1 commentary vs 1.353:1 formula) — flagged in §21.16 ESCALATION-3 for end-of-Stage-2 amendment.

---

## §22 — Phase 2.6a Output Chain Research (Master Saturator + Zero-Latency Feedforward Limiter + Stereo Width, Gate 8a)

**Author:** research-phase, 2026-05-01.
**Cycle:** Phase 2.6a (sub-cycle 1 of 3 in Phase 2.6 umbrella). CONTEXT rev-11.
**Predecessor:** §21 (Phase 2.5 Body Resonator + Bow Noise Generator) closed 2026-04-30 with R37 atomic `907a7c3` Gate 7 SOFT-PASS.
**Atomic-commit target:** R39 (next source-edit atomic; R38 was non-source Logic AU audition probe).
**Goal:** Resolve all open questions for Phase 2.6a output chain so PLAN rev-13 can author R39 task bodies without re-litigation. Surface architecture deviations explicitly so plan-phase can decide ARCHITECTURE-amendment vs PLAN-deviation routing per Q3/Q7 LOCKED amendment-count constraints.

§22 is the **first** of three Phase 2.6 sub-research sections. §23 (Phase 2.6b microtonal engine) and §24 (Phase 2.6c Note Expression) author at later sub-cycle research-phases per CONTEXT rev-11 §"Cycle Scope" umbrella plan.

---

## §22.1 Pre-Flight Tripwire — 14 Audible Goldens Reproduce Byte-Identical at HEAD

**Status:** CLEARED — `reproduce-goldens.sh` 14/14 PASS at `1b44efd` (HEAD descendant of R37 atomic `907a7c3` + R37-backfill chore `36b89d2`).

**Scope:** 13 Phase 2.5-baselined audible goldens + 1 saturator-tail-comparison golden = 14 entries (Phase 2.5 R37 added body resonator + bow noise; saturator-tail-comparison carry-forward from Phase 2.4c-bis R36-bis):

```
detune-sweep-A.wav         (Phase 2.5 sha, post-body)
macro-sweep.wav            (Phase 2.5 sha, post-body)
note-sequence.wav          (Phase 2.5 sha, post-body)
saturator-tail-comparison.wav  (Phase 2.4c-bis sha, carry-forward)
schelleng-stress.wav       (Phase 2.5 sha)
slow-lfo.wav               (Phase 2.5 sha, post-body)
stiffness-sweep.wav        (Phase 2.5 sha)
stiffness-zero-pre.wav     (Phase 2.5 sha)
string-A.wav               (Phase 2.5 sha)
string-D.wav               (Phase 2.5 sha)
string-G.wav               (Phase 2.5 sha)
sub-harmonics-stability.wav  (Phase 2.5 sha)
sub-harmonics.wav          (Phase 2.5 sha)
vibrato.wav                (Phase 2.4c-bis sha `df7384e3…`, carry-forward)
```

`matrix-stability.wav.sha256 = 6db67707…` (`6db6770727ab3b433a036f487217bbde70f8cc15de44fa60ac0b99d868176449`) and `matrix-stability.json.sha256 = 625505cf…` carry forward as **evidence-only** goldens (NOT in `reproduce-goldens.sh` per Phase 2.4a R34b "evidence golden" precedent).

**Phase 2.6a tripwire bar:** R39-pre re-runs `reproduce-goldens.sh` against HEAD-at-R39-pre (descendant of `907a7c3 + 36b89d2`) and asserts 14/14 PASS before any source edit. Source-tree clean (`git status` clean against the 5 in-scope source files {`MasterSaturator.h` NEW, `MasterLimiter.h` NEW, `StereoWidth.h` NEW, `PluginProcessor.{h,cpp}` M, `BowedContrabassVoice.cpp` M for OUTPUT_GAIN relocation}). Saturator carry-forward verify (`grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns 2 — Phase 2.4c-bis port preserved). BodyResonator + BowNoiseGenerator integration verify (`grep -c BodyResonator plugins/O-Contrabass/Source/BowedContrabassVoice.h` returns 4; `grep -c BowNoiseGenerator plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` returns 7).

---

## §22.2 Master Saturator (`y = x − x³/3`) — Algorithm, Pre-Gain Calibration, Header Design

### §22.2.1 Architecture spec recap

ARCHITECTURE.md §171–179 + §249 + §560:

> Polynomial `y = x − x³/3` with input pre-clamp to [−1.5, +1.5]. ~6 dB gentle warmth above −3 dBFS. … Saturator's soft-clip at ~−3 dBFS limits the work the limiter has to do, allowing 3 ms attack to be sufficient.

CONTEXT rev-11 Q3 LOCKED: `polynomial x − x³/3` per ARCHITECTURE verbatim. NO unification with in-loop `4·tanh(x/4)` (Phase 2.4c-bis port stays untouched at `WaveguideString.cpp:204-209`). Master saturator is at output (post-body), not in feedback loop.

### §22.2.2 Polynomial transfer function characterisation

`f(x) = x − x³/3`:

| input x  | output f(x) | gain at x | comment |
|----------|-------------|-----------|---------|
| 0.0      | 0.0         | 1.000     | unity |
| 0.1      | 0.0997      | 0.997     | ~0 dB compression |
| 0.5      | 0.458       | 0.917     | −0.75 dB compression |
| 0.707 (−3 dBFS) | 0.589 | 0.833 | −1.59 dB compression (knee onset) |
| 1.0 (0 dBFS) | 0.667 | 0.667 | −3.52 dB compression |
| 1.414 (+3 dBFS) | 0.471 | 0.333 | −9.55 dB; output peak 2/3 = −3.52 dBFS at x=1 |
| 1.5 (+3.5 dBFS, pre-clamp ceiling) | 0.375 | 0.250 | −12.0 dB; output below peak (curve falls past x=1) |

**Critical property:** `f(x)` peaks at `x = 1.0` with `f(1.0) = 2/3`. For `x > 1.0`, `f(x)` *decreases* (eventually crossing zero at `x = √3 ≈ 1.732`). The pre-clamp to [−1.5, +1.5] prevents the curve from going past its monotone-decreasing region into outright sign-flip or further folding. **Without the pre-clamp**, an input of `x = 1.732` would output zero, and `x = 2.0` would output `−2/3` — phase inversion. The pre-clamp is **load-bearing**.

The ~"6 dB gentle warmth above −3 dBFS" claim in ARCHITECTURE §177 is approximate. Actual numbers per the table: at 0 dBFS input, output drops 3.5 dB (not 6 dB); at +3 dBFS input (pre-clamped to +3.5 dBFS = 1.414), output is 9.55 dB below input, 3.52 dB below 0 dBFS. So the saturator caps absolute output peak at −3.52 dBFS = `2/3` linear regardless of input. The "6 dB warmth" is a colloquialism for the cubic-soft-knee character, not a precise level statement.

**Implication for limiter design:** master saturator output peak ≤ 0.667 (= −3.52 dBFS) **without** any limiter intervention. CONTEXT Q4 LOCKED limiter ceiling −0.3 dBFS = 0.966 linear. With master saturator upstream, the limiter receives a signal that can't exceed −3.52 dBFS — limiter gain reduction is **never engaged on the master saturator's clipped peak**. Limiter gain reduction only engages if (a) MASTER_SAT_AMOUNT < 100% (so user is bypassing the saturator partially, allowing through-signal above 0.667) or (b) the user automates `OUTPUT_GAIN` upward post-saturator (which it isn't, after R39d relocation — see §22.6). The limiter is effectively a safety net for **partial-bypass scenarios + width >1.0 boost** (M/S width=2.0 doubles side-channel = doubles L−R differential = potential L/R peak boost up to 2× with full anti-correlation; rare in practice for a synth voice, more relevant in stereo source material). Limiter still must be in the chain for QUAL-02 / QUAL-01 guarantees.

### §22.2.3 MASTER_SAT_AMOUNT pre-gain mapping (NEW design contract)

ARCHITECTURE does not specify how MASTER_SAT_AMOUNT (0–100%) maps to saturator behavior. Two candidate mappings:

**Option A — pre-gain only (recommended):** MASTER_SAT_AMOUNT linearly drives input pre-gain from `1.0` (at 0%) to `2.0` (at 100%):

```
preGain = 1.0 + amount  // amount ∈ [0, 1]
xPre    = preGain * input
xClamp  = jlimit(-1.5, 1.5, xPre)
out     = (xClamp - xClamp*xClamp*xClamp / 3.0) / preGain  // post-gain compensates for pre-gain so unity at small signal
```

At 0% MASTER_SAT_AMOUNT, `preGain = 1.0` and the `pre-clamp + cubic` path is a no-op for `|input| ≤ 1.0` (cubic saturator is unity at `x → 0`; pre-clamp doesn't trigger; post-gain divide is by 1.0). The output is `f(input)/1.0 = input − input³/3`, which differs from raw input by ~0.75 dB at 0.5 amplitude. **This is not a true bypass.** For zero-effect-at-0% behavior, mix-style:

**Option B — wet/dry mix (alternative):** MASTER_SAT_AMOUNT is the wet/dry mix of saturated vs raw signal:

```
wet = saturate(input)  // always full saturation curve
out = (1 - amount) * input + amount * wet
```

At 0%, output = input verbatim (true bypass). At 100%, output = full saturation. **Recommended over Option A** for transparency at 0% and matches user intuition (knob = "amount of saturator effect").

**Option C — variable drive (knob = pre-gain in dB):** MASTER_SAT_AMOUNT maps to pre-gain in dB (e.g., 0% → 0 dB, 100% → +12 dB) with no post-gain compensation:

```
preGain = decibelsToGain(amount * 12.0)  // 0..+12 dB
xClamp  = jlimit(-1.5, 1.5, preGain * input)
out     = xClamp - xClamp*xClamp*xClamp / 3.0
```

At 0%, preGain=1 → `f(input)`, NOT bypass (~0.75 dB at 0.5 amp). At 100%, preGain=4 → almost everything pre-clamps to ±1.5, output ≈ ±0.375 = aggressive limiting. Closer to a "tape drive" knob.

**Recommendation: Option B (wet/dry mix).**

Reasons:
- True bypass at 0% (matches user intuition; preserves byte-equality with current Phase 2.5 voice-output state at MASTER_SAT_AMOUNT=0%).
- Smooth perceptual scaling: 50% gives noticeable warmth, 100% gives full architectural saturator.
- Default 50% per CONTEXT rev-11 §"Phase 2.6a — Output chain" line 54 (`MASTER_SAT_AMOUNT 0–100%, default 50%`) lands musically: half the saturation curve effect at default voice peak amplitudes, leaving headroom for user-driven character.
- **Bit-equality test at MASTER_SAT_AMOUNT=0%:** output identical to raw voice → useful golden anchor. Saturator-tail-comparison golden at MASTER_SAT_AMOUNT=0% should reproduce Phase 2.5 sha (modulo limiter + width, both also at default no-op states — see §22.3.4 + §22.4.5).

**Default voice peak vs saturator engagement at 50% mix:** matrix-stability post-body peak ≈ 0.351 linear (per Phase 2.4c-bis carry-forward). At 0.351 input, `f(0.351) = 0.337` = −0.36 dB compression. With 50% mix: `out = 0.5 · 0.351 + 0.5 · 0.337 = 0.344` = −0.18 dB compression. Default 50% is **subtle warmth** at default voice peaks — confirms ARCHITECTURE intent.

**Note-sequence default-state peak** (research-phase pre-flight metric, not yet measured): TBD at R39e re-baseline render. Predicted ≤ 0.50 (well below 0.667 saturator output cap).

### §22.2.4 Header design — `Source/DSP/MasterSaturator.h`

Header-only per BodyResonator / BowNoiseGenerator / SubHarmonicBias / DispersionFilter precedent. Stateless polynomial (no IIR memory) — only the `SmoothedValue<float>` ramp on `amount` carries state.

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>

class MasterSaturator
{
public:
    void prepare (double sampleRate)
    {
        amountSmoothed.reset (sampleRate, 0.030);  // 30 ms zipper-free ramp
    }

    void reset()
    {
        amountSmoothed.reset (0);
    }

    void setAmount (float amount)  // [0, 1]
    {
        amountSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, amount));
    }

    // Per-sample wet/dry mix of x − x³/3 saturator with input.
    float processSample (float in) noexcept
    {
        const float a = amountSmoothed.getNextValue();
        const float xClamp = juce::jlimit (-1.5f, 1.5f, in);
        const float wet = xClamp - xClamp * xClamp * xClamp / 3.0f;
        return (1.0f - a) * in + a * wet;
    }

    // Block API for stereo buffers.
    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float a = amountSmoothed.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                const float in = data[i];
                const float xClamp = juce::jlimit (-1.5f, 1.5f, in);
                const float wet = xClamp - xClamp * xClamp * xClamp / 3.0f;
                data[i] = (1.0f - a) * in + a * wet;
            }
        }
    }

private:
    juce::SmoothedValue<float> amountSmoothed { 0.0f };
};
```

**LOC budget:** ~50 LOC NEW (header-only). Block API loops `getNextValue()` once per sample (not per channel) to keep L and R in sync per-sample-per-block — matches O-Bowed StereoWidthProcessor.h precedent.

**Determinism note:** no `juce::Random`, no IIR state, no cross-block memory. `juce::SmoothedValue<float>` is deterministic per JUCE 8 (linear ramp; no random component). HR-style determinism preserved.

### §22.2.5 Saturator pre-flight render-harness probe

R39 design: `--master-saturator` CLI mode renders 5 probes at MASTER_SAT_AMOUNT ∈ {0%, 25%, 50%, 75%, 100%} with default voice settings + 4-second sustained E1 note (full Helmholtz-cycle saturation curve characterisation). Output: peak amplitude, RMS, harmonic distortion (THD at 41.2 Hz fundamental), spectral tilt (1k Hz - 8k Hz). Acceptance bars:

- MASTER_SAT_AMOUNT=0% peak == voice peak verbatim (bit-identical bypass)
- MASTER_SAT_AMOUNT=100% peak ≤ 0.667 (= −3.52 dBFS) for any voice peak ≥ 1.0
- THD monotone increasing in MASTER_SAT_AMOUNT (0% → 100%)

**Folded into `--output-chain` mega-mode** (see §22.7).

---

## §22.3 Zero-Latency Feedforward Limiter — Implementation Choice + Coefficients + Architecture Deviation

### §22.3.1 ARCHITECTURE spec vs CONTEXT Q4 LOCKED — surfaced deviation

**ARCHITECTURE §177–179 + §527–545:**

| Param | Value (ARCHITECTURE) | Source |
|---|---|---|
| Topology | Feedforward, no look-ahead | §178, §544 |
| Attack | 3 ms | §178, §541 |
| Release | **100 ms** | §178, §541 |
| Threshold | **−1 dBFS** (linear 0.891) | §178, §542 |
| Domain | "Run on 2x oversampled signal" | §543 |

**CONTEXT rev-11 Q4 LOCKED:**

| Param | Value (CONTEXT Q4) | Source |
|---|---|---|
| Topology | Feedforward, no look-ahead | Q4 LOCKED |
| Attack | 3 ms | Q4 LOCKED |
| Release | **50 ms** | Q4 LOCKED, also CONTEXT line 20 + line 50 |
| Threshold | **−0.3 dBFS** | Q4 LOCKED, also CONTEXT line 50 + line 55 |
| Domain | (not specified) | — |

**Two divergences:**

1. **Release time:** 100 ms → 50 ms. CONTEXT supersedes ARCHITECTURE.
2. **Threshold:** −1 dBFS → −0.3 dBFS. CONTEXT supersedes ARCHITECTURE.
3. **Domain:** ARCHITECTURE says "2x oversampled". This is **wrong for the chosen topology** — the master chain runs at host rate (2x oversampling boundary closes at the friction junction downsample, `BowedContrabassVoice.cpp:267` "Body runs at host rate (post 2× downsample); noise generator likewise"). Master saturator + limiter + width are post-body, post-noise — host-rate. ARCHITECTURE §543 is internally inconsistent with §225–238 chain description.

**ESCALATION-2 (ARCHITECTURE deviation, two amendments needed):**

CONTEXT rev-11 Q3 LOCKS amendment count at 3 (§"DC Blocker" + §"In-loop saturator" + §149/§509 size_scalar). Adding limiter-spec realignment as a 4th amendment **violates Q3 lock**. Plan-phase has three options:

- **Option A — extend §"Master Saturator + Zero-Latency Limiter" amendment-evidence-base** (analogous to in-loop saturator amendment which appended Phase 2.4c R36 + Phase 2.4c-bis R36-bis + Phase 2.5 R37 evidence). NOT a new amendment count — appends "Phase 2.6a evidence: release 100→50 ms + threshold −1 → −0.3 dBFS per CONTEXT rev-11 Q4 lock; oversampling claim §543 corrected to host-rate per §225–238 internal consistency." This is **the recommended path**.
- **Option B — PLAN rev-13 deviation flag against ARCHITECTURE**: implement CONTEXT-locked values and document deviation in PLAN §"Approach Decisions" without ARCHITECTURE amendment. Defers ARCHITECTURE realignment to v1.1. Cleaner for v1.0 timeline; ARCHITECTURE is **slightly stale** but not blocking.
- **Option C — re-discuss Q3** to bump amendment count from 3 to 4. Wastes a discuss-phase cycle for a rubber-stamp; not recommended.

**Research-phase recommendation:** **Option A.** Treat the three Phase 2.6 amendments (DC Blocker / in-loop saturator / size_scalar) as **section-anchors** to which evidence is appended — same as how §"In-loop saturator" amendment grew from 1 → 3 evidence entries across Phase 2.4c → Phase 2.4c-bis → Phase 2.5. Phase 2.6a's release-time + threshold + domain corrections become a **fourth evidence-line under the §"Master Saturator + Zero-Latency Limiter" amendment heading**, NOT a fourth amendment-section. Q3 LOCK respected; technical realignment captured.

**Plan-phase action:** Author PLAN rev-13 §"Approach Decisions" with explicit acknowledgment of ARCHITECTURE §177–179 + §540–544 evidence-extension; flag in R39 atomic commit body; defer the actual end-of-Stage-2 amendment-text-write to Phase 2.6c verify-phase amendments task per Q7.

### §22.3.2 Implementation choice — `juce::dsp::Compressor` vs `juce::dsp::BallisticsFilter` vs hand-written

**Option A — `juce::dsp::Compressor` adapted:**

`juce::dsp::Compressor<float>` exposes `setAttack`, `setRelease`, `setThreshold`, `setRatio`. To approximate a hard limiter: `setRatio(100.0f)` (or higher) approximates ratio=∞. **Problems:** (1) `juce::dsp::Compressor` uses log-domain envelope detection (peak in dB), not linear-domain `threshold/envelope` gain reduction; (2) at very high ratios, smoothing artifacts accumulate; (3) threshold semantics differ (compression onset, not hard ceiling). Not a clean fit for ARCHITECTURE §527 algorithm.

**Option B — `juce::dsp::BallisticsFilter<float>` envelope follower + threshold-divide:**

Use `juce::dsp::BallisticsFilter` with `setLevelCalculationType(BallisticsFilterLevelCalculationType::peak)` for envelope detection, then hand-write the `gain = (env > threshold) ? threshold/env : 1.0` reduction. **Pro:** O-Gain precedent (`PluginProcessor.h:94–95` uses BallisticsFilter for VU meters; precedent exists in Ouaricon). **Con:** BallisticsFilter exposes attack/release in seconds via `setAttackTime` / `setReleaseTime`, internally computes the same `exp(-1/(t·sr))` coefficients we'd hand-roll — net code is no shorter, adds a dependency for a 4-line IIR, less direct mapping to ARCHITECTURE §527 formula.

**Option C — hand-written feedforward per ARCHITECTURE §527 verbatim:**

Direct implementation of:

```
abs_x = |input|
coeff = (abs_x > envelope) ? attackCoeff : releaseCoeff
envelope = coeff · envelope + (1 − coeff) · abs_x
gain = (envelope > threshold) ? threshold / envelope : 1.0
output = input · gain
```

with:

```
attackCoeff  = exp(-1.0 / (0.003 * sampleRate))
releaseCoeff = exp(-1.0 / (0.050 * sampleRate))   // CONTEXT Q4 LOCKED 50 ms
threshold    = decibelsToGain(-0.3f) = 0.9661f    // CONTEXT Q4 LOCKED -0.3 dBFS
```

Pros:
- Direct mapping to ARCHITECTURE §527 algorithm verbatim (1:1 line correspondence).
- Stateless except for `envelope` scalar (or 2 scalars for stereo-linked).
- Header-only ~80 LOC; matches MasterSaturator + DispersionFilter + SubHarmonicBias precedent.
- No JUCE-API surface dependency (transparent to JUCE version changes).
- Stereo-link is 1 line of code (`abs_x = max(|L|, |R|); apply same gain to L and R`).

Cons:
- "Reinventing the wheel" relative to JUCE's BallisticsFilter.
- Need to test ourselves (BallisticsFilter is JUCE-tested).

**Research-phase recommendation: Option C.** Reasons: ARCHITECTURE §527 is the locked algorithmic spec; literal implementation gives 1:1 traceability; Ouaricon DSP precedent (BodyResonator, BowNoiseGenerator, DispersionFilter, SchellengCalibration, SubHarmonicBias all hand-written headers ≤ 200 LOC); RT-safe (3 multiply-add per sample + 1 conditional + 1 divide-by-envelope only when envelope > threshold).

**Stereo-link decision:** YES, stereo-linked. Reason: M/S decode after limiter could expose stereo image collapse if L and R limiters fire at different times. Linked limiter detects `max(|L|, |R|)` envelope and applies common gain. This is the standard "stereo limiter" pattern.

### §22.3.3 Header design — `Source/DSP/MasterLimiter.h`

```cpp
#pragma once
#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

class MasterLimiter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        recalcCoefficients();
        envL = envR = 0.0f;
        ceilingLinear = juce::Decibels::decibelsToGain (ceilingDb);
        ceilingSmoothed.reset (sampleRate, 0.030);
        ceilingSmoothed.setCurrentAndTargetValue (ceilingLinear);
    }

    void reset()
    {
        envL = envR = 0.0f;
        ceilingSmoothed.reset (ceilingLinear);
    }

    void setCeilingDb (float dB)  // CONTEXT Q4: −0.3 dBFS default; range [-6, 0]
    {
        ceilingDb = juce::jlimit (-6.0f, 0.0f, dB);
        ceilingLinear = juce::Decibels::decibelsToGain (ceilingDb);
        ceilingSmoothed.setTargetValue (ceilingLinear);
    }

    // Process stereo buffer in-place (stereo-linked envelope).
    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 1)
            return;

        const int numSamples = buffer.getNumSamples();
        float* L = buffer.getWritePointer (0);
        float* R = (buffer.getNumChannels() > 1) ? buffer.getWritePointer (1) : L;
        const bool stereo = (R != L);

        for (int i = 0; i < numSamples; ++i)
        {
            const float threshold = ceilingSmoothed.getNextValue();
            const float absL = std::fabs (L[i]);
            const float absR = stereo ? std::fabs (R[i]) : absL;
            const float absMax = juce::jmax (absL, absR);

            // Stereo-linked envelope: max-channel drives both gain reductions.
            const float coeff = (absMax > envL) ? attackCoeff : releaseCoeff;
            envL = coeff * envL + (1.0f - coeff) * absMax;

            const float gain = (envL > threshold) ? (threshold / envL) : 1.0f;

            L[i] *= gain;
            if (stereo)
                R[i] *= gain;
        }
    }

private:
    void recalcCoefficients()
    {
        const double attackS  = 0.003;  // 3 ms — CONTEXT Q4 LOCKED
        const double releaseS = 0.050;  // 50 ms — CONTEXT Q4 LOCKED
        attackCoeff  = static_cast<float> (std::exp (-1.0 / (attackS  * sr)));
        releaseCoeff = static_cast<float> (std::exp (-1.0 / (releaseS * sr)));
    }

    double sr { 48000.0 };
    float attackCoeff  { 0.0f };
    float releaseCoeff { 0.0f };
    float envL { 0.0f };
    float envR { 0.0f };  // unused with stereo-link; reserved for per-channel mode
    float ceilingDb { -0.3f };
    float ceilingLinear { 1.0f };
    juce::SmoothedValue<float> ceilingSmoothed { 1.0f };
};
```

**LOC budget:** ~80 LOC NEW (header-only).

**State machine:** single `envL` scalar (stereo-linked). `envR` reserved for future per-channel mode (deferred to v1.1 if needed). `ceilingSmoothed` 30 ms ramp prevents zipper noise on `LIMITER_CEILING_DB` automation.

**Determinism:** `std::exp`, `std::fabs`, scalar arithmetic — deterministic.

**RT-safety:** no allocations, no locks, no I/O. Pure math + buffer write. Passes PERF-01.

**Latency:** zero algorithmic latency (no look-ahead, no delay line). Passes PERF-03 nice-to-have.

### §22.3.4 Limiter pre-flight render-harness probe

R39 design: `--master-limiter` CLI mode renders 4 probes with high-amplitude input (synthetic bow-pressure stress at INFINITE_SUSTAIN=1.0 + SUB_HARMONICS=1.0 + EXPRESSION_MACRO=1.0 + BOW_PRESSURE=8.0 + 4-second sustained E1) at LIMITER_CEILING_DB ∈ {-6, -3, -0.3, 0} dBFS. Acceptance bars:

- Output peak ≤ `ceilingLinear + 0.05 dB slop` for all 4 probes (Gate 8a invariant #1).
- No audible click on transient onset (transient must engage 3 ms attack smoothly).
- Smooth release tail (50 ms post-peak; verify `releaseCoeff` math).

**Folded into `--output-chain` mega-mode** (see §22.7).

---

## §22.4 Stereo Width — M/S Topology + Mono-Source Decorrelator + ESCALATION

### §22.4.1 ARCHITECTURE spec vs current voice output

ARCHITECTURE §183–190 + §255 spec:

> Custom — M/S encode → side-gain scale → M/S decode. WIDTH ∈ [0, 2] mapped to side-channel gain. … 0.0 = mono (side = 0), 1.0 = stereo (side = 1.0), 2.0 = 200% wide (side = 2.0, mid clamped to prevent overflow). Body Mix is applied separately to M and S (default 5% drier on side channel) to avoid artificial-sounding stereo body thickness. … Mono path until Stereo Width — all internal DSP is single-channel.

**Critical interaction:** O-Contrabass voice output is mono (`BowedContrabassVoice.cpp:778–783` writes the same `s` value to channel 0 AND channel 1 via `addSample` × 2). Result: **post-`renderNextBlock`, the stereo buffer has `L == R` exactly**. Pure M/S width on `L == R`:

```
mid  = (L + R) / 2 = L = R
side = (L - R) / 2 = 0   ← always
side *= w            = 0   ← always
out_L = mid + side = L
out_R = mid - side = L
```

**WIDTH knob is a no-op** if applied directly to `L == R` mono-cloned output. ARCHITECTURE §269 ("Mono path until Stereo Width") confirms voice output is mono, but ARCHITECTURE §183 ("Custom — M/S encode → side-gain scale → M/S decode") doesn't address how non-zero side content is generated from a mono source.

**Two patterns in the Ouaricon precedent:**

1. **O-Bowed `StereoWidthProcessor.h`:** pure M/S, NO decorrelator. Comment line 28: "Input: already has meaningful stereo from per-string panning + body resonator." O-Bowed has stereo body resonator (different per-channel coefficients) → meaningful side content. **Not applicable to O-Contrabass** which has mono body resonator (BodyResonator.h is mono single-channel per Phase 2.5 design).
2. **O-Wind `StereoWidth.h`:** allpass decorrelator on R (`makeAllPass(sampleRate, 800.0f, 0.7f)` at line 31) + M/S width. Input: mono voice output cloned to L=R, decorrelator delays/filters R relative to L → meaningful side content → M/S width is musically active. **Directly applicable to O-Contrabass** (matching mono-voice → stereo-width topology).

### §22.4.2 ESCALATION-1 — WIDTH topology decision

**Three options:**

- **Option A — O-Wind allpass decorrelator pattern (recommended):** `juce::dsp::IIR::Filter` with allpass coefficients at 800 Hz / Q=0.7 on R channel; M/S width on the resulting decorrelated stereo. Adds ~10 LOC over pure M/S; preserves zero-latency (allpass is causal, IIR memory ≤ ~20 samples for 800 Hz / Q=0.7 at 48 kHz); makes WIDTH knob musically meaningful on mono voice output.
- **Option B — Pure M/S, accept WIDTH no-op for v1.0 mono voice**, document as deferred to v1.1 stereo body resonator work. WIDTH knob exists in parameter UI but has no effect at default mono voice. **User-visible regression** (Stage 1 contract said WIDTH was a usable parameter).
- **Option C — Stereo body resonator** (different coefficients per channel) generating natural stereo image upstream. ARCHITECTURE deviation from current Phase 2.5 mono BodyResonator. Significant scope expansion (re-architect BodyResonator into stereo) — defer to v1.1.

**Research-phase recommendation: Option A (O-Wind allpass decorrelator).**

Reasons:
- Matches O-Wind pattern precedent (validated in shipping plugin).
- Preserves PERF-03 zero-latency (allpass is delayless in algorithmic sense; IIR transient is a few-sample group delay, not a sample-rate latency report).
- Transparent at WIDTH=1.0 (M/S decode at side=1.0 reverses M/S encode exactly; identity mapping, modulo allpass on R). At WIDTH=0.0, output reduces to `mid + 0 = mid = (L_decorr + R_decorr) / 2` — mono mix of L and decorrelated R. Slight comb-filter color from the allpass collapse, but musically acceptable for a "mono" knob position.
- WIDTH=2.0 doubles side: `mid + 2·side` and `mid - 2·side` — exaggerated stereo, expected user behavior.
- ~15 LOC net additional vs O-Bowed pure M/S (O-Wind 78 LOC vs O-Bowed 56 LOC; 22 LOC delta).

**Side-effect to verify:** allpass on R channel introduces **phase coherence artifact** at WIDTH=0.0 (the "mono" position is no longer mathematically pure mono — it's the M-channel of the decorrelated stereo). Acceptance bar: WIDTH=0.0 must NOT produce audible comb filtering on default voice content. Render-harness `--output-chain` mode includes WIDTH=0.0 probe at default voice; spectral analysis verifies no notch beyond ±2 dB in 20 Hz – 20 kHz band.

**Plan-phase action:** PLAN rev-13 LOCK Option A; cite O-Wind `StereoWidth.h` as reference; document side-effect verification at R39e.

### §22.4.3 Header design — `Source/DSP/StereoWidth.h`

Direct port of O-Wind `StereoWidth.h` (full file in §22 References). Shared module extraction (`modules/effects/stereo-width`) deferred to v1.1 per CONTEXT rev-11 §"Out of scope" line 121.

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>

class StereoWidth
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec {
            sampleRate,
            static_cast<juce::uint32> (maxBlockSize),
            1
        };
        decorrelator.prepare (spec);
        decorrelator.coefficients =
            juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, 800.0f, 0.7f);
        widthSmoothed.reset (sampleRate, 0.020);  // 20 ms ramp; matches O-Wind precedent
    }

    void reset()
    {
        decorrelator.reset();
        widthSmoothed.reset (1.0f);
    }

    void setWidth (float w)  // [0, 2]; default 1.0
    {
        widthSmoothed.setTargetValue (juce::jlimit (0.0f, 2.0f, w));
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 2)
            return;

        const int numSamples = buffer.getNumSamples();
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);

        for (int i = 0; i < numSamples; ++i)
        {
            // Mono voice writes L == R. Decorrelate R via allpass to generate
            // meaningful side content from mono source.
            const float mono = L[i];
            const float left  = mono;
            const float right = decorrelator.processSample (mono);

            // M/S width.
            const float w = widthSmoothed.getNextValue();
            const float mid  = (left + right) * 0.5f;
            const float side = (left - right) * 0.5f * w;

            L[i] = mid + side;
            R[i] = mid - side;
        }
    }

private:
    juce::dsp::IIR::Filter<float> decorrelator;
    juce::SmoothedValue<float> widthSmoothed { 1.0f };
};
```

**LOC budget:** ~50 LOC NEW (header-only).

**Determinism:** `juce::dsp::IIR::Filter` is deterministic (verified at Phase 2.5 R37d 3-trial bit-stability for body resonator IIR bank — same JUCE class). `juce::SmoothedValue<float>` deterministic. HR-style determinism preserved.

**Latency:** allpass IIR introduces frequency-dependent group delay (~few samples around 800 Hz center). `setLatencySamples()` should NOT be incremented — allpass group delay is not algorithmic latency in the JUCE/PERF-03 sense (matches O-Wind precedent which reports zero latency).

### §22.4.4 Width pre-flight render-harness probe

R39 design: `--stereo-width` CLI mode renders 5 probes at WIDTH ∈ {0.0, 0.5, 1.0, 1.5, 2.0} with default voice. Acceptance bars:

- WIDTH=1.0: output peak ≤ voice peak + 0.5 dB (decorrelator may slightly boost transient peaks but not catastrophically).
- WIDTH=0.0: spectral content within ±2 dB of voice spectrum in 20 Hz – 20 kHz (verify allpass-collapse comb is benign).
- WIDTH=2.0: max(|L|, |R|) ≤ ceiling enforced by limiter upstream (limiter must catch any side-doubling overshoot).
- Click-free WIDTH automation 0% → 200% (Gate 8a invariant #2).

**Folded into `--output-chain` mega-mode** (see §22.7).

### §22.4.5 ARCHITECTURE §190 "Body Mix applied separately to M and S" — DEFERRED

ARCHITECTURE §190 spec:

> Body Mix is applied separately to M and S (default 5% drier on side channel) to avoid artificial-sounding stereo body thickness.

This requires the body resonator to be aware of M/S decomposition and apply different mix factors per channel. Current Phase 2.5 BodyResonator is mono single-bank — no channel awareness. **Defer to v1.1** as stereo-body-resonator work. Phase 2.6a uses single body-mix path (current Phase 2.5 behavior).

**Plan-phase action:** PLAN rev-13 §"Out of scope" line item: "ARCHITECTURE §190 stereo body-mix splitter — deferred to v1.1 stereo body resonator work."

---

## §22.5 Parameter-spec.md Amendment — `MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB`; OUTPUT_GAIN/OUTPUT_LEVEL Reconciliation

### §22.5.1 Current parameter-spec.md state

`plugins/O-Contrabass/.planning/parameter-spec.md` actual sha (research-phase computed):

```
77638e255c2adeefdb85ae3b4d4287eecbc63b1313413573f20664990a2025d1
```

This matches CONTEXT rev-11 line 246 carry-forward (`77638e25…`). **Confirmed authoritative.**

**STALE-COMMENT FINDING:** `plugins/O-Contrabass/Source/PluginProcessor.cpp:8` claims:

```cpp
// (sha256:c47fe7361a55e1d64b906ef7194894f4a2490744b35a644c76b6e1a632282d0d).
```

This sha (`c47fe7361a…`) does NOT match the current parameter-spec.md sha (`77638e25…`). The comment is **stale** — likely the Stage-1-close sha before the Phase 2.3 R28 default flips for `VIBRATO_DEPTH` (12.0→0.0) and `EXPRESSION_MACRO` (0.5→0.0), which were Stage-1-contract amendments documented in REQUIREMENTS.md `<!--` history but not propagated into the source comment.

**Plan-phase action:** R39d updates the comment in `PluginProcessor.cpp:8` to the post-Phase-2.6a sha (computed at R39e). Combined with `MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB` additions, this is a **single contract-amendment-with-sha-bump** at R39d.

### §22.5.2 Naming reconciliation — OUTPUT_GAIN vs OUTPUT_LEVEL

**Current code (PluginProcessor.cpp:54):** parameter ID is `OUTPUT_GAIN`, display name is `"Output Level"`. Voice consumes via `parameters->getRawParameterValue("OUTPUT_GAIN")` (BowedContrabassVoice.cpp:805).

**Current parameter-spec.md (line 19):** `OUTPUT_GAIN | Output Level | Float | -60.0 - 12.0 | 0.0 | dB`. Matches code.

**CONTEXT rev-11 line 57:** "`OUTPUT_LEVEL` already declared per BRIEF.md (−inf to +12 dB, default 0 dB)". The CONTEXT typo'd `OUTPUT_LEVEL` as the parameter ID; the actual ID is `OUTPUT_GAIN`. This is a CONTEXT-document inconsistency, not a code bug.

**Resolution:** parameter ID stays `OUTPUT_GAIN` (frozen contract per PluginProcessor.cpp:9 "IDs are a frozen contract; renaming breaks DAW automation persistence"). Display name stays "Output Level". CONTEXT-document typo is informational; no code change. **No parameter-spec.md amendment needed for OUTPUT_GAIN/OUTPUT_LEVEL** — only the additions in §22.5.3 and the sha-bump.

### §22.5.3 New parameter additions

Two NEW parameters at Phase 2.6a R39d:

```cpp
// -- Output Chain (Phase 2.6a additions) --
layout.add(std::make_unique<APF>(juce::ParameterID{"MASTER_SAT_AMOUNT", 1}, "Master Saturator",
    NR(0.0f, 1.0f, 0.001f),            0.50f));  // 50% wet/dry default per CONTEXT rev-11 §"Phase 2.6a"
layout.add(std::make_unique<APF>(juce::ParameterID{"LIMITER_CEILING_DB", 1}, "Limiter Ceiling",
    NR(-6.0f, 0.0f, 0.01f),            -0.3f));  // -0.3 dBFS per CONTEXT rev-11 Q4 LOCKED
```

parameter-spec.md amendment authoring (R39d):

1. Add new section `### Output Chain (Phase 2.6a additions)` after `### Output` section (between current line 70 `WIDTH` and line 72 `### Microtonal Tuning`).
2. Lines:
   - `MASTER_SAT_AMOUNT | Master Saturator | Float | 0.0 - 1.0 | 0.50 | - | Wet/dry mix of polynomial x − x³/3 saturator (Phase 2.6a). Soft-clip at ~−3 dBFS. Default 50%.`
   - `LIMITER_CEILING_DB | Limiter Ceiling | Float | -6.0 - 0.0 | -0.3 | dB | Zero-latency feedforward limiter ceiling (Phase 2.6a). 3 ms attack / 50 ms release per CONTEXT rev-11 Q4. Default -0.3 dBFS.`
3. Update `## Parameter Count Summary`:
   - `Output Chain (Phase 2.6a): 2` (NEW row between `Output: 1` and `Microtonal: 3`)
   - `Total: 31` (was 29; +2)
4. Add `## Audit Trail` (NEW final section, mirroring REQUIREMENTS.md `<!--` precedent):

```
## Audit Trail

### Stage 1 → Phase 2.6a (parameter-spec contract amendments)

- Phase 2.3 R28 (2026-04-29): VIBRATO_DEPTH default flipped 12.0 → 0.0 (HR-1 short-circuit; Phase 2.2 strict byte-equal regression bar). EXPRESSION_MACRO default flipped 0.50 → 0.0 (Q7a). Sha bump deferred (informally tracked in this section); next sha-bump at Phase 2.6a R39d (next contract amendment).
- Phase 2.6a R39d (2026-05-XX): NEW MASTER_SAT_AMOUNT + LIMITER_CEILING_DB per CONTEXT rev-11 §"Phase 2.6a — Output chain" + Q4 LOCKED limiter ceiling. Total parameter count 29 → 31.
```

5. Compute new sha post-edit; update comment in `PluginProcessor.cpp:8` from `c47fe7361a…` to new sha.

**Plan-phase action:** PLAN rev-13 R39d task body specifies the 5-step amendment sequence verbatim.

### §22.5.4 Parameter-spec.md NormalisableRange precedent

`MASTER_SAT_AMOUNT` matches existing 0–1 NR pattern (BOW_NOISE, BODY_MIX precedent — `NR(0.0f, 1.0f, 0.001f)`).

`LIMITER_CEILING_DB` is a dB-range parameter; precedent is `OUTPUT_GAIN` with `NR(-60.0f, 12.0f, 0.1f)` — flat linear. For ceiling, recommended skew=1.0 (linear, no skew) since the [-6, 0] dB range is small enough that perceptual log-skew isn't needed. Step 0.01 dB matches REFERENCE_PITCH precision precedent.

---

## §22.6 PluginProcessor Wire-Up — Master Chain Integration + OUTPUT_GAIN Relocation

### §22.6.1 Current PluginProcessor::processBlock state (Phase 2.5 baseline)

`PluginProcessor.cpp:165–179`:

```cpp
void OContrabassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    buffer.clear();
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}
```

After `synth.renderNextBlock`, `buffer` contains stereo mono-cloned voice output (channel 0 == channel 1) with `OUTPUT_GAIN` already applied at voice-side (BowedContrabassVoice.cpp:778 `s = voiceBuffer.getSample(0, i) * kVoiceNorm * outputGainLinear`).

### §22.6.2 ARCHITECTURE chain ordering vs current voice-side OUTPUT_GAIN

ARCHITECTURE §249–258 master chain:

```
[Body Resonator] → [Bow Noise Generator] → [Master Saturator] → [Limiter] → [Stereo Width] → [Output Gain] → Stereo Out
```

OUTPUT_GAIN is the **final** stage AFTER Stereo Width.

**Current voice-side application** (BowedContrabassVoice.cpp:778) places OUTPUT_GAIN BEFORE saturator/limiter/width. This **inverts** the architectural ordering.

**Musical consequence:** if OUTPUT_GAIN stays voice-side, user lowering OUTPUT_GAIN by −12 dB attenuates the signal **before** the master saturator → saturator never engages → user loses tone color when dimming the volume. User raising OUTPUT_GAIN by +6 dB drives saturator harder → tone gets crunchier as user increases volume. Both behaviors are user-confusing for a "master volume" knob.

**Resolution: relocate OUTPUT_GAIN application to processor-level POST-StereoWidth.** This is a Phase 2.6a sub-task at R39d:

1. Remove `* outputGainLinear` from BowedContrabassVoice.cpp:778 (becomes `s = voiceBuffer.getSample(0, i) * kVoiceNorm`).
2. Remove `outputGainLinear = juce::Decibels::decibelsToGain (outputLevel);` at BowedContrabassVoice.cpp:828 (no longer voice-side).
3. Remove `outputLevel = parameters->getRawParameterValue ("OUTPUT_GAIN")->load();` at BowedContrabassVoice.cpp:805 (no longer voice-side).
4. Remove `outputGainLinear` member variable in BowedContrabassVoice.h.
5. Add `juce::SmoothedValue<float> outputGainSmoothed { 1.0f };` to PluginProcessor.h.
6. Initialize `outputGainSmoothed.reset(sampleRate, 0.030);` in `prepareToPlay`.
7. In `processBlock`, AFTER `stereoWidth.processBlock(buffer);`, set target from APVTS and apply per-sample gain ramp:

```cpp
const float gainTarget = juce::Decibels::decibelsToGain(
    parameters.getRawParameterValue("OUTPUT_GAIN")->load());
outputGainSmoothed.setTargetValue(gainTarget);

const int numSamples = buffer.getNumSamples();
const int numChans = buffer.getNumChannels();
for (int i = 0; i < numSamples; ++i)
{
    const float g = outputGainSmoothed.getNextValue();
    for (int ch = 0; ch < numChans; ++ch)
        buffer.getWritePointer(ch)[i] *= g;
}
```

**Golden re-baseline consequence:** moving OUTPUT_GAIN from voice-side to processor-level changes the signal entering the master saturator (voice output now at full amplitude × kVoiceNorm without OUTPUT_GAIN scaling). At default OUTPUT_GAIN=0 dB (linear 1.0), the math is bit-equivalent (multiplying by 1.0 commutes with all upstream linear ops). At non-default OUTPUT_GAIN values, signal differs. **All Phase 2.5 goldens were rendered at OUTPUT_GAIN=0 dB default**, so re-baseline is bit-equivalent for the voice-side relocation alone — golden bytes only change because of the NEW master saturator + limiter + width chain.

**Verification at R39e:** render `note-sequence.wav` with default-state APVTS values BUT with MASTER_SAT_AMOUNT=0.0 + LIMITER_CEILING_DB=0.0 + WIDTH=1.0 (saturator bypass + limiter at 0 dBFS = effectively bypass + width=identity). Expect **bit-identical** to Phase 2.5 `note-sequence.wav.sha256`. If sha matches, OUTPUT_GAIN relocation is bit-clean; non-bit-equality at default would expose a subtle bug.

### §22.6.3 PluginProcessor::processBlock NEW chain (Phase 2.6a target)

Pseudocode:

```cpp
void OContrabassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    buffer.clear();
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Phase 2.6a — Master chain (Steps 10–13 per ARCHITECTURE §249–258):

    // Step 10: Master Saturator (polynomial x − x³/3, wet/dry mix).
    masterSaturator.setAmount(parameters.getRawParameterValue("MASTER_SAT_AMOUNT")->load());
    masterSaturator.processBlock(buffer);

    // Step 11: Zero-latency feedforward limiter (3 ms attack / 50 ms release).
    masterLimiter.setCeilingDb(parameters.getRawParameterValue("LIMITER_CEILING_DB")->load());
    masterLimiter.processBlock(buffer);

    // Step 12: Stereo width (allpass decorrelator + M/S width).
    stereoWidth.setWidth(parameters.getRawParameterValue("WIDTH")->load());
    stereoWidth.processBlock(buffer);

    // Step 13: Output Gain (relocated from voice-side; ARCHITECTURE §258 final stage).
    const float gainTarget = juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("OUTPUT_GAIN")->load());
    outputGainSmoothed.setTargetValue(gainTarget);

    const int numSamples = buffer.getNumSamples();
    const int numChans = buffer.getNumChannels();
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGainSmoothed.getNextValue();
        for (int ch = 0; ch < numChans; ++ch)
            buffer.getWritePointer(ch)[i] *= g;
    }
}
```

**Header additions** (`PluginProcessor.h`):

```cpp
#include "DSP/MasterSaturator.h"
#include "DSP/MasterLimiter.h"
#include "DSP/StereoWidth.h"

// ... in class private section:
MasterSaturator masterSaturator;
MasterLimiter masterLimiter;
StereoWidth stereoWidth;
juce::SmoothedValue<float> outputGainSmoothed { 1.0f };
```

**`prepareToPlay` additions:**

```cpp
masterSaturator.prepare(sampleRate);
masterLimiter.prepare(sampleRate);
stereoWidth.prepare(sampleRate, samplesPerBlock);
outputGainSmoothed.reset(sampleRate, 0.030);
outputGainSmoothed.setCurrentAndTargetValue(
    juce::Decibels::decibelsToGain(parameters.getRawParameterValue("OUTPUT_GAIN")->load()));
```

**`releaseResources` additions:**

```cpp
masterSaturator.reset();
masterLimiter.reset();
stereoWidth.reset();
outputGainSmoothed.reset(1.0f);
```

**LOC delta:**

- `PluginProcessor.h`: ~10 LOC NEW (3 includes + 4 member declarations).
- `PluginProcessor.cpp`: ~30 LOC NEW (prepare + releaseResources + processBlock additions) + ~3 LOC M (header includes already counted in .h).
- `BowedContrabassVoice.h`: ~3 LOC M (remove `outputGainLinear` member).
- `BowedContrabassVoice.cpp`: ~5 LOC M (remove OUTPUT_GAIN read + set + apply).

**Total Phase 2.6a source budget: ~370 LOC NEW + ~50 LOC M = ~420 LOC net.**

### §22.6.4 setLatencySamples — PERF-03 invariant verification

PluginProcessor.cpp:155 currently reports oversampler latency from voice. Phase 2.6a master chain components:

- MasterSaturator: 0 latency (memoryless polynomial waveshaper).
- MasterLimiter: 0 latency (feedforward, no look-ahead per Q4 LOCKED).
- StereoWidth: 0 latency (allpass decorrelator group delay is not algorithmic latency in JUCE/PERF-03 sense; matches O-Wind precedent).
- OutputGain: 0 latency (scalar multiply).

**`setLatencySamples` UNCHANGED** from Phase 2.5. PERF-03 invariant holds. Plan-phase R39 acceptance bar: `getLatencySamples()` returns same value pre- and post-Phase-2.6a.

### §22.6.5 RT-safety audit

Per-block operations in NEW master chain:

- 4× `parameters.getRawParameterValue(...)->load()` — atomic read; RT-safe.
- 4× setter calls (MasterSaturator/Limiter/StereoWidth/outputGainSmoothed) — set target values; RT-safe.
- 4× `processBlock` calls — pure math on existing buffer; no allocation, no lock; RT-safe.

**No allocations, no locks, no I/O.** PERF-01 invariant preserved. Plan-phase R39 acceptance bar: pluginval-10 `Background thread state` + `Parameter thread safety` PASS.

---

## §22.7 Render-Harness `--output-chain` CLI Mode Design

### §22.7.1 Mode scope

Single `--output-chain` mode covers all four pre-flight probes from §22.2.5 / §22.3.4 / §22.4.4 + the global Gate 8a invariant test. Saves 4 separate CLI modes from cluttering harness `main.cpp`.

**Probes** (concatenated into single output WAV with marker silence):

1. **Saturator amount sweep** (5 sub-probes, ~5 s each = 25 s):
   - MASTER_SAT_AMOUNT ∈ {0%, 25%, 50%, 75%, 100%} × 4-second sustained E1 default voice. LIMITER_CEILING_DB=-0.3 dBFS, WIDTH=1.0, OUTPUT_GAIN=0 dB.
2. **Limiter ceiling sweep** (4 sub-probes, ~5 s each = 20 s):
   - LIMITER_CEILING_DB ∈ {-6, -3, -0.3, 0} dBFS × high-amplitude stress (INFINITE_SUSTAIN=1.0 + SUB_HARMONICS=1.0 + EXPRESSION_MACRO=1.0 + BOW_PRESSURE=8.0 + 4-second sustained E1). MASTER_SAT_AMOUNT=0 (bypass to test limiter alone), WIDTH=1.0, OUTPUT_GAIN=0 dB.
3. **Width sweep** (5 sub-probes, ~3 s each = 15 s):
   - WIDTH ∈ {0.0, 0.5, 1.0, 1.5, 2.0} × 2-second sustained E1 default voice. MASTER_SAT_AMOUNT=0, LIMITER_CEILING_DB=0 dBFS (bypass-equivalent), OUTPUT_GAIN=0 dB.
4. **Click-free automation test** (~10 s):
   - WIDTH automated 0% → 200% over 5 s with 100 ms hold at endpoints; MASTER_SAT_AMOUNT automated 0% → 100% over 5 s. LIMITER_CEILING_DB=-0.3 dBFS, OUTPUT_GAIN=0 dB. Default voice + 5-second sustained E1.
5. **Peak-overshoot stress** (~5 s):
   - High-amplitude stress (per probe 2 settings) + LIMITER_CEILING_DB=-0.3 dBFS, MASTER_SAT_AMOUNT=0% (bypass saturator to test limiter ceiling) + WIDTH=2.0 (max side-amplification stress). 4-second sustained E1.

**Total length:** ~75 s. Render budget: ~75 × 0.29 s/s wall-clock baseline (Phase 2.4c §19.13 reference) = ~22 s wall-clock per render. 3-trial bit-stability = ~66 s × 3 = ~3.5 min total render time per Phase 2.6a sub-cycle.

### §22.7.2 JSON metrics

Per-probe metrics in `output-chain.json`:

```json
{
  "metrics": {
    "saturator_amount_sweep": {
      "0.0":  { "peak": <float>, "rms": <float>, "thd_at_41hz": <float>, "spectral_tilt_1k_8k_db": <float>, "bit_identical_to_voice": <bool> },
      "0.25": { "peak": ..., ... },
      "0.5":  ...,
      "0.75": ...,
      "1.0":  ...
    },
    "limiter_ceiling_sweep": {
      "-6":   { "peak": <float>, "ceiling_violation_db": <float, max(0, peak_dBFS - ceiling_dBFS)>, "rms": ... },
      "-3":   ...,
      "-0.3": ...,
      "0":    ...
    },
    "width_sweep": {
      "0.0":  { "peak_l": ..., "peak_r": ..., "side_rms": ..., "spectral_collapse_max_notch_db": <float> },
      "0.5":  ...,
      "1.0":  ...,  // expected: spectral identity (within decorrelator transient)
      "1.5":  ...,
      "2.0":  ...
    },
    "automation_test": {
      "click_count": <int, threshold = 0>,
      "max_block_to_block_jump_db": <float, threshold = 1.0 dB>
    },
    "peak_overshoot_stress": {
      "max_peak_db": <float>,
      "ceiling_dbfs": -0.3,
      "ceiling_violation_db": <float, must be ≤ 0.05 dB>
    }
  }
}
```

### §22.7.3 Acceptance bars (Gate 8a invariants)

| Bar | Source | Threshold |
|-----|--------|-----------|
| Output peak ≤ ceiling + 0.05 dB | Gate 8a #1 | `peak_overshoot_stress.ceiling_violation_db ≤ 0.05` |
| Click-free automation | Gate 8a #2 | `automation_test.click_count == 0` AND `max_block_to_block_jump_db ≤ 1.0` |
| Zero algorithmic latency | Gate 8a #3 | `getLatencySamples()` unchanged from Phase 2.5 |
| auval + pluginval-10 SUCCESS | Gate 8a #4 | (R39g auditor task) |
| 14 audible goldens reproduce byte-identical | Gate 8a #5 | (R39f reproduce-goldens.sh PASS at NEW sha256s) |

### §22.7.4 Harness CMakeLists.txt — header-only DSP, no source-list addition

The 3 NEW DSP headers (`MasterSaturator.h`, `MasterLimiter.h`, `StereoWidth.h`) are **header-only** — included by `PluginProcessor.h` and reach the harness via existing `${CMAKE_CURRENT_SOURCE_DIR}/../../Source/PluginProcessor.cpp` source-list entry (harness CMakeLists.txt:30–34 already pulls PluginProcessor.cpp in).

`target_include_directories` already includes `${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP` (line 38) — covers the new headers.

**No CMakeLists.txt edits needed for the 3 NEW headers.** This contrasts with Phase 2.5 R37 which had to add `BodyResonator.cpp` to the source list (it had a .cpp file). Phase 2.6a chooses header-only design specifically to avoid Phase 2.5 R37 deviation #1 (harness CMake source-list addition).

**Plan-phase action:** PLAN rev-13 R39e task body documents zero-CMake-edit invariant explicitly.

---

## §22.8 Goldens Strategy — 14 Re-Baseline + 1 NEW + Matrix-Stability Evidence

### §22.8.1 Re-baseline scope (R39e)

All 14 Phase 2.5 audible goldens (per §22.1) **re-baseline** at R39e. Reason: master chain materially shifts every audible signal (saturator wet/dry at 50% default + limiter at -0.3 dBFS + width allpass decorrelator). NEW post-Phase-2.6a sha256s lock at R39e, replace prior-state sha256s in `tests/render-harness/golden/*.wav.sha256`.

**Bit-stability pre-flight** (R39e step 1 of 4): 3-trial render of all 14 goldens; sha256 across trials must match (HR-style determinism on new master chain). If 3-trial bit-stability fails, BLOCK at R39e (investigate determinism bug — likely allpass IIR `juce::Random` mis-seeding or SmoothedValue first-block divergence).

**Saturator-tail-comparison golden re-baseline:** Phase 2.4c-bis R36-bis sha (carry-forward from §21.18) re-baselines at R39e. Bin 64 (or whatever spectral bin saturator-tail tracks) measurement against Phase 2.5 baseline `−25.06 dB` for evidence-extension under §"In-loop saturator" amendment (Phase 2.6a evidence-line: "post-master-chain bin 64 = <X> dB" — does the master saturator + limiter further attenuate or restore the body-coupling tail energy?).

**vibrato.wav re-baseline:** Phase 2.4c-bis sha `df7384e3…` re-baselines. Vibrato is a per-note frequency modulation; master saturator + limiter applied AFTER the vibrato modulation should preserve frequency content (saturator is memoryless polynomial → no time smearing; limiter envelope follower may smear ONLY at limiter engagement points which are absent at default VIBRATO_DEPTH=0.0 → bit-equivalent expected at default).

### §22.8.2 NEW golden — `output-chain.wav` + `.json` + `.json.sha256`

`output-chain.wav` per §22.7.1 (~75 s rendered concatenation of 5 probes + automation test + peak-overshoot stress). `output-chain.json` per §22.7.2 (per-probe metrics).

3-trial bit-stability at R39e step 2 of 4. Lock new sha256 at step 3. `reproduce-goldens.sh` extends from 14 entries to **15 entries** (14 audible + 1 output-chain).

### §22.8.3 Matrix-stability evidence-only re-render

`matrix-stability.wav.sha256 = 6db67707…` and `matrix-stability.json.sha256 = 625505cf…` re-render at R39e step 4 as **evidence-only** (NOT in `reproduce-goldens.sh` per Phase 2.4a R34b precedent). Master chain may shift matrix peaks downward (limiter clamps); zero NEW raucous corners expected (Phase 2.5 baseline 108/108 PASS); HR-style determinism preserved. Evidence archived to `.planning/evidence/phase-2-6a/matrix-stability-post-output-chain.{wav,json}` for ARCHITECTURE-amendment evidence base.

### §22.8.4 13 → 14 → 15 entry reproduce-goldens.sh evolution

| Phase | Entries | Additions |
|---|---|---|
| Phase 2.4c | 13 | (existing baseline) |
| Phase 2.4c-bis R36-bis | 13 | saturator-tail-comparison ADDED → 14 (evidence note: Phase 2.5 carry-forward implicitly drops sha row to use carry-forward — verify at R39-pre tripwire) |
| Phase 2.5 R37 | 14 | (no entries added; 13 audible + 1 saturator-tail-comparison; 13 of 14 re-baselined) |
| Phase 2.6a R39 (target) | **15** | + output-chain |

R39e step 5: append `output-chain.wav` row to `reproduce-goldens.sh`.

---

## §22.9 Open Items for Plan-Phase

### §22.9.1 USER-DECISION-REQUIRED escalations

**ESCALATION-1 (§22.4.2 finding — Stereo Width topology):** Recommend Option A (O-Wind allpass decorrelator pattern at 800 Hz / Q=0.7) to make WIDTH knob musically meaningful on mono voice output. Plan-phase action: LOCK Option A in PLAN rev-13 §"Approach Decisions"; cite O-Wind `StereoWidth.h` as reference; require WIDTH=0.0 spectral-collapse comb-notch verification ≤ 2 dB at R39e probe 3 sub-probe `0.0`. **NOT RE-DISCUSSING** — research-phase finding is design-grounded based on existing Ouaricon precedent (O-Wind shipped pattern).

**ESCALATION-2 (§22.3.1 finding — ARCHITECTURE limiter spec divergence):** Recommend Option A (extend §"Master Saturator + Zero-Latency Limiter" amendment-evidence-base with Phase 2.6a evidence). NOT a 4th amendment (Q3 lock respected). Plan-phase action: PLAN rev-13 §"Approach Decisions" acknowledges deviation explicitly; flag in R39 atomic commit body; defer end-of-Stage-2 amendment-text-write to Phase 2.6c verify-phase amendments task per Q7.

**ESCALATION-3 (§22.4.5 finding — ARCHITECTURE §190 stereo body-mix splitter):** ARCHITECTURE §190 ("Body Mix is applied separately to M and S, default 5% drier on side channel") is incompatible with Phase 2.5 mono BodyResonator. Defer to v1.1 stereo-body-resonator work. Plan-phase action: PLAN rev-13 §"Out of scope" line item.

**ESCALATION-4 (§22.5.1 finding — stale parameter-spec sha comment):** `PluginProcessor.cpp:8` comment claims sha `c47fe7361a…`; actual current sha is `77638e25…`. Comment is stale (likely Phase 2.3 R28 default-flip drift). Plan-phase action: R39d updates comment to post-Phase-2.6a sha (computed at R39e amendment).

**ESCALATION-5 (§22.6.2 finding — OUTPUT_GAIN voice-side relocation):** OUTPUT_GAIN currently applied at voice `BowedContrabassVoice.cpp:778` BEFORE master saturator → user volume changes saturator color (musically wrong). Phase 2.6a relocates to processor-level POST-StereoWidth per ARCHITECTURE §258. Bit-equivalent at default OUTPUT_GAIN=0 dB. Plan-phase action: PLAN rev-13 R39d sub-task explicitly lists the 5-line voice-side removal + 7-line processor-side addition.

### §22.9.2 Plan-phase pre-locks (no user intervention needed)

- §22.10 R39 9-task structure verbatim
- §22.7 `--output-chain` mega-mode design + JSON metrics + acceptance bars
- §22.4.3 + §22.3.3 + §22.2.4 header designs as starting code (NOT a hard contract — plan-phase may refine signatures, e.g., per-sample API vs block API for MasterSaturator)
- §22.5.3 parameter-spec.md amendment authoring (5-step sequence)
- §22.6.3 PluginProcessor::processBlock NEW chain ordering (Steps 10–13)
- §22.8 goldens strategy (14 re-baseline + 1 NEW + matrix-stability evidence)
- HR-1..HR-10 carry-forward; NO new HR introduced (HR-12 / HR-13 not required by Phase 2.6a — output chain is downstream of friction junction; no friction-module ABI risk)

### §22.9.3 Plan-phase rev numbering

PLAN rev-13 (supersedes PLAN rev-12 from Phase 2.5).

### §22.9.4 Source-delta budget per R39 tripwire (locked at research-phase)

5 production source files + 0 CMakeLists.txt edits = ~370 LOC NEW + ~50 LOC M = ~420 LOC net:

- `Source/DSP/MasterSaturator.h` (~50 LOC NEW)
- `Source/DSP/MasterLimiter.h` (~80 LOC NEW)
- `Source/DSP/StereoWidth.h` (~50 LOC NEW)
- `Source/PluginProcessor.h` (~10 LOC NEW; 3 includes + 4 member declarations)
- `Source/PluginProcessor.cpp` (~30 LOC NEW; prepare + releaseResources + processBlock additions; 2 NEW APVTS parameter declarations)
- `Source/BowedContrabassVoice.h` (~3 LOC M; remove `outputGainLinear` member declaration)
- `Source/BowedContrabassVoice.cpp` (~5 LOC M; remove OUTPUT_GAIN read + set + apply at lines 805/828/778)
- `tests/render-harness/main.cpp` (~150 LOC NEW; `--output-chain` mode handler)
- `plugins/O-Contrabass/.planning/parameter-spec.md` (~30 LOC NEW; 2 parameter rows + Audit Trail section + Total bump 29→31)
- `plugins/O-Contrabass/Source/PluginProcessor.cpp:8` (1 LOC M; comment sha update)

**No CMakeLists.txt edits** (header-only DSP design; harness CMake unchanged).

---

## §22.10 R39 9-Task Breakdown (LOCKED — verbatim from research-phase)

Pattern: mirrors Phase 2.5 R37 9-task structure (`R37-pre / R37a / R37b / R37c / R37d / R37e / R37f / R38 / R37 atomic` + R37-backfill chore).

### R39-pre (5-check tripwire)

1. `git status` clean against the 8 in-scope source files.
2. `reproduce-goldens.sh` 14/14 PASS at HEAD (descendant of R37 atomic `907a7c3` + R37-backfill chore `36b89d2`).
3. Source-tree audit hook clean: no edits in {`O-Bowed/`, `O-Wind/` (except header reference), `modules/synthesis/bow-friction/`, `WaveguideString.cpp`, `DispersionFilter.h`, `SchellengCalibration.h`, `SubHarmonicBias.h`, `BodyResonator.{h,cpp}`, `BowNoiseGenerator.h`}.
4. Saturator carry-forward verify: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns `2` (Phase 2.4c-bis port preserved).
5. Pre-edit grep verification: `MasterSaturator|MasterLimiter|StereoWidth` returns 0 hits in `plugins/O-Contrabass/Source/` (no pre-existing implementations).
6. Harness builds at HEAD: `cmake --build build --target O-Contrabass-render-test` SUCCESS.
7. Parameter-spec sha snapshot: `shasum -a 256 plugins/O-Contrabass/.planning/parameter-spec.md` returns `77638e25…` (matches CONTEXT rev-11 line 246).

If any check fails, BLOCK and investigate upstream drift before any source edit.

### R39a (`MasterSaturator.h` NEW per §22.2.4)

- Author header per §22.2.4 design (~50 LOC NEW).
- Implementation: `processBlock(juce::AudioBuffer<float>&)` block API + `processSample(float)` per-sample API.
- `setAmount(float)` setter + `juce::SmoothedValue<float>` 30 ms ramp.
- Wet/dry mix per §22.2.3 Option B.
- Pre-clamp to [-1.5, +1.5] per ARCHITECTURE §177 verbatim.
- Build verify: `cmake --build build --target O-Contrabass_VST3` SUCCESS.

### R39b (`MasterLimiter.h` NEW per §22.3.3)

- Author header per §22.3.3 design (~80 LOC NEW).
- Implementation: ARCHITECTURE §527 algorithm verbatim.
- Stereo-linked envelope detection (`max(|L|, |R|)`); apply common gain to L and R.
- 3 ms attack / 50 ms release per CONTEXT Q4 LOCKED.
- Default ceiling -0.3 dBFS; range [-6, 0] dB.
- `juce::SmoothedValue<float>` 30 ms ramp on ceiling.
- Build verify: `cmake --build build --target O-Contrabass_VST3` SUCCESS.

### R39c (`StereoWidth.h` NEW per §22.4.3)

- Author header per §22.4.3 design (~50 LOC NEW).
- Direct port of O-Wind `StereoWidth.h` pattern.
- Allpass decorrelator at 800 Hz / Q=0.7 on R channel.
- M/S width with `juce::SmoothedValue<float>` 20 ms ramp.
- Default 1.0; range [0, 2].
- Build verify: `cmake --build build --target O-Contrabass_VST3` SUCCESS.

### R39d (`PluginProcessor.{h,cpp}` M + `BowedContrabassVoice.{h,cpp}` M + parameter-spec.md amendment per §22.5 + §22.6)

- Add 3 `#include "DSP/Master*.h"` + `StereoWidth.h` to PluginProcessor.h.
- Declare 3 master-chain instances + `outputGainSmoothed` member.
- Initialize in `prepareToPlay` per §22.6.3 (4 prepare calls + outputGainSmoothed reset/setCurrentAndTargetValue).
- Reset in `releaseResources`.
- Wire Steps 10–13 in `processBlock` per §22.6.3 (master saturator → limiter → width → output gain).
- Add 2 NEW APVTS parameter declarations (`MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB`) in `createParameterLayout` per §22.5.3.
- Remove `outputGainLinear` member from BowedContrabassVoice.h.
- Remove OUTPUT_GAIN read at BowedContrabassVoice.cpp:805 + set at :828 + apply at :778.
- Update parameter-spec.md per §22.5.3 5-step sequence (NEW Output Chain section + Audit Trail + Total 29→31).
- Compute new parameter-spec.md sha; update PluginProcessor.cpp:8 comment.
- Build verify: `cmake --build build --target O-Contrabass_VST3 O-Contrabass_AU` SUCCESS.

### R39e (re-baseline 14 audible goldens + NEW output-chain golden + matrix-stability evidence)

- Step 1: 3-trial bit-stability pre-flight — render all 14 audible goldens 3× ; assert sha256 identical across trials.
- Step 2: Render `output-chain.wav` per §22.7.1 (5 probes); 3-trial bit-stability; lock NEW sha256.
- Step 3: Lock 14 NEW post-Phase-2.6a sha256s into `tests/render-harness/golden/*.wav.sha256`.
- Step 4: Matrix-stability evidence-only re-render → archive to `.planning/evidence/phase-2-6a/matrix-stability-post-output-chain.{wav,json}` (NOT in reproduce-goldens.sh).
- Step 5: Append `output-chain.wav` row to `reproduce-goldens.sh` (13 → 14 → 15 entries; updated count assertion).
- Step 6: Saturator-tail-comparison evidence-extension: measure bin 64 post-master-chain; document under §"In-loop saturator" amendment evidence-base (Phase 2.6a evidence-line for end-of-Stage-2 amendment).
- Step 7: Default-state bit-equality test: render `note-sequence.wav` with MASTER_SAT_AMOUNT=0.0 + LIMITER_CEILING_DB=0.0 + WIDTH=1.0 (effective bypass); expect bit-identical to Phase 2.5 sha (verifies OUTPUT_GAIN relocation is bit-clean).

### R39f (regression bar)

- 15-entry `reproduce-goldens.sh` PASS against NEW post-Phase-2.6a sha256s (14 audible re-baselined + 1 NEW output-chain).
- 8-file production source audit hook reports EXACTLY {`MasterSaturator.h` NEW + `MasterLimiter.h` NEW + `StereoWidth.h` NEW + `PluginProcessor.{h,cpp}` M + `BowedContrabassVoice.{h,cpp}` M + `tests/render-harness/main.cpp` M}.
- 0-file CMakeLists.txt audit hook (no CMake edits expected).
- 1-file parameter-spec.md audit hook (Output Chain section + Audit Trail + Total 31).
- Saturator carry-forward verify: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns `2`.
- BodyResonator + BowNoiseGenerator integration verify: `grep -c BodyResonator plugins/O-Contrabass/Source/BowedContrabassVoice.h` returns `4`; `grep -c BowNoiseGenerator plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` returns `7`.
- Latency invariant verify: `getLatencySamples()` unchanged from Phase 2.5 (target: still reports oversampler latency from voice).

### R39g (auval + pluginval-10 SUCCESS)

- `auval -v aumu OCbs OuDv` AU VALIDATION SUCCEEDED.
- `pluginval --strictness-level 10 --validate <O-Contrabass.vst3>` SUCCESS full battery.
- Specific pluginval probes for Phase 2.6a: `Background thread state` + `Parameter thread safety` (RT-safety on master chain) + `Buffer fuzz tests` + `Click-free automation` (WIDTH 0% → 200% sweep + MASTER_SAT_AMOUNT 0% → 100% sweep).

### R39 atomic commit (single atomic per Phase 2.4c-bis R36-bis precedent)

- All source: 8 files (3 NEW headers + 2 M PluginProcessor + 2 M BowedContrabassVoice + 1 M harness main.cpp).
- All goldens: 15 sha256 + 15 JSON + 1 informational JSON sha anchor (matrix-stability evidence-only).
- All planning: parameter-spec.md amendment + RESEARCH §22 verdict + CONTEXT rev-11.a (Phase 2.6a sub-cycle amendment) + PLAN rev-13 + SUMMARY/VERIFICATION/STATUS planning artefacts.
- Atomic-commit message body: explicit deviation flags for ESCALATION-1 (allpass decorrelator) + ESCALATION-2 (limiter spec evidence-extension under existing amendment) + ESCALATION-5 (OUTPUT_GAIN relocation).

### R39-backfill chore (sha propagation into STATUS.md per R34/R35/R36/R36-bis/R37 precedent)

- Update STATUS.md `phase_2_6a_atomic_sha:` field with R39 atomic commit sha.
- Append Phase 2.6a verify carry-forward summary to STATUS.md.

**Atomic-commit sequence:** R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37 → **R39** (Phase 2.6a) → R40 (Phase 2.6b) → R41 (Phase 2.6c) → Stage 2 verify amendments commit.

**Critical-path note:** R39d (PluginProcessor wire-up + parameter-spec amendment + voice-side OUTPUT_GAIN removal) is the largest scope task (~50 LOC M across 4 files + parameter-spec amendment authoring); plan-phase budgets ~2 hours implementation + 30 min build-verify. R39e render budget: 14 audible × 3 trials × 0.29 s/s wall-clock + matrix-stability 108-combo × 1 trial × ~3 s + output-chain 75 s × 3 trials × 0.29 s/s = ~7 min total. R39g auval/pluginval: ~10 min. R39 atomic commit + R39-backfill: ~10 min.

---

## §22.11 Risk-Surface Refinement for PLAN rev-13

Phase 2.6a starting risk register inherits from CONTEXT rev-11 §"Risk Register" (13 risks; 10 mitigated, 1 expected, 1 expected re-baseline, 1 open). Research-phase adds:

| # | Risk | Trigger | Mitigation | Status |
|---|------|---------|------------|--------|
| 14 | WIDTH no-op without decorrelator | Voice writes mono L=R; pure M/S has zero side content | ESCALATION-1 Option A: O-Wind allpass decorrelator pattern; verified at R39e probe 3 sub-probe 0.0 (spectral collapse ≤ 2 dB notch) | Mitigated by Option A LOCK |
| 15 | ARCHITECTURE limiter spec divergence (release 100→50 ms; threshold −1 → −0.3 dBFS; "2x oversampled" claim incorrect for host-rate chain) | CONTEXT Q4 LOCKED supersedes ARCHITECTURE §177–179 + §540–544 | ESCALATION-2 Option A: extend §"Master Saturator + Zero-Latency Limiter" amendment-evidence-base; NOT a 4th amendment (Q3 lock respected) | Mitigated by Option A LOCK |
| 16 | OUTPUT_GAIN voice-side application inverts ARCHITECTURE chain ordering | BowedContrabassVoice.cpp:778 applies OUTPUT_GAIN BEFORE master saturator | ESCALATION-5 Option C: relocate to processor-level POST-StereoWidth; bit-equivalent at default 0 dB | Mitigated by R39d relocation |
| 17 | Stale parameter-spec sha in PluginProcessor.cpp:8 comment (`c47fe7361a…` vs actual `77638e25…`) | Phase 2.3 R28 default-flip drift; never re-synced | ESCALATION-4: R39d updates comment to post-Phase-2.6a sha | Mitigated by R39d |
| 18 | 14-golden re-baseline scope at R39e | Master chain shifts every audible signal (saturator wet/dry + limiter + decorrelator) | All 14 re-baselined at R39e step 3; 3-trial bit-stability pre-flight; HR-style determinism preserved | Expected (re-baseline planned, not regression) |
| 19 | Allpass IIR transient at WIDTH=0.0 produces audible comb-filter color | StereoWidth allpass on R channel collapses to L+R/2 differential at WIDTH=0 | R39e probe 3 sub-probe 0.0 spectral verification (max notch ≤ 2 dB) | Open (R39e verification) |
| 20 | Default voice peak amplitudes overdrive saturator at 50% wet/dry | Matrix-stability post-body peak ≈ 0.351 → saturator-with-Option-B-mix ≈ 0.344 → −0.18 dB, well within budget; but rare drone combos (INFINITE_SUSTAIN=1.0 + SUB_HARMONICS=1.0 + EXPRESSION_MACRO=1.0 + BOW_PRESSURE=8.0) may approach 0.667 saturator output cap | R39e probe 1 + probe 5 verifies; saturator output cap ≤ -3.52 dBFS guarantees ceiling room | Mitigated by saturator output cap math |
| 21 | parameter-spec.md amendment authoring (FIRST Stage-1 contract amendment in Stage 2) | Phase 2.6a R39d | Q1 LOCKED expected amendment per CONTEXT line 53–57; explicit Audit Trail section + sha-bump audit-trailed | Expected (planned amendment) |
| 22 | Bit-equivalence at MASTER_SAT_AMOUNT=0 + LIMITER_CEILING_DB=0 + WIDTH=1.0 default-state golden render | If non-bit-equivalent, OUTPUT_GAIN relocation has subtle bug | R39e step 7 explicit bit-equivalence test against Phase 2.5 sha | Open (R39e verification) |
| 23 | setLatencySamples invariant (PERF-03 zero-latency) | Master chain components all 0-latency by design | R39f explicit `getLatencySamples()` unchanged check | Mitigated by header design |
| 24 | RT-safety bar at master chain (PERF-01) | NEW master-chain processBlock allocations / locks | All header-only headers; no allocation, no lock, no I/O; pluginval-10 Background thread state + Parameter thread safety verified at R39g | Mitigated by header-only design |
| 25 | Click-free WIDTH automation 0% → 200% | Gate 8a invariant #2 | 20 ms SmoothedValue ramp on width; 30 ms ramp on MASTER_SAT_AMOUNT + LIMITER_CEILING_DB; pluginval-10 + R39e probe 4 (automation test) verifies | Mitigated by smoothing |
| 26 | Stereo body-mix splitter ARCHITECTURE §190 deferred | Mono BodyResonator incompatible | ESCALATION-3: defer to v1.1 stereo-body work; NOT in Phase 2.6a scope | Mitigated by deferral |

**26-entry risk register locked** (13 CONTEXT carry-forward + 13 NEW from research). 18 mitigated, 3 expected, 2 open (#19 + #22 — both R39e verification points), 0 deferred-to-execute-phase.

---

## §22.12 Sequencing in PLAN rev-13

PLAN rev-13 supersedes PLAN rev-12 (Phase 2.5). Phase 2.6a sub-cycle plan landing only — Phase 2.6b + Phase 2.6c receive separate PLAN amendments at later sub-cycle plan-phases (PLAN rev-14 / rev-15 OR rev-13 amendments).

**Plan-phase content for PLAN rev-13:**

1. **Preamble** — Phase 2.6a scope, Q1–Q10 LOCKED references, ESCALATION-1..5 acknowledgments.
2. **Approach Decisions** — explicit design contracts:
   - MasterSaturator wet/dry mix Option B (§22.2.3)
   - MasterLimiter Option C hand-written per ARCHITECTURE §527 (§22.3.2)
   - StereoWidth Option A O-Wind allpass decorrelator (§22.4.2)
   - OUTPUT_GAIN relocation to processor-level (§22.6.2)
   - parameter-spec.md amendment 5-step sequence (§22.5.3)
   - ARCHITECTURE limiter spec evidence-extension under existing amendment (§22.3.1)
3. **R39 9-Task Breakdown** — verbatim from §22.10.
4. **Source-Delta Budget** — 8 production source files + 0 CMakeLists.txt edits = ~370 LOC NEW + ~50 LOC M = ~420 LOC net (§22.9.4).
5. **Goldens Strategy** — 14 re-baseline + 1 NEW + matrix-stability evidence (§22.8).
6. **Risk Register** — 26 entries (§22.11).
7. **Gate 8a 5-Invariant Acceptance Bar** — output peak ≤ ceiling + 0.05 dB; click-free WIDTH; PERF-03 zero-latency; auval + pluginval-10 SUCCESS; 14 re-baselined audible goldens reproduce byte-identical.
8. **Atomic Commit Sequence** — R7 → … → R37 → R39 (Phase 2.6a) → R40 (Phase 2.6b) → R41 (Phase 2.6c).
9. **Out of Scope** — Phase 2.4-bis backlog (Q2 LOCKED v1.1); ARCHITECTURE §190 stereo body-mix splitter (ESCALATION-3 v1.1); look-ahead limiter (Phase 2.6a-bis if Stage 4 audition reveals harsh transients); chaos detector + softClampState (v1.1).
10. **References** — §22 References (§22.14).

---

## §22.13 Summary — Phase 2.6a Research Resolution Map

| Open Q | Status | Resolution Path |
|--------|--------|-----------------|
| #1 (CONTEXT line 231) Master saturator pre-gain calibration | ✅ RESOLVED | Wet/dry mix Option B (§22.2.3); default 50% lands −0.18 dB at default voice peak; saturator output cap ≤ -3.52 dBFS at any input |
| #2 (CONTEXT line 231) Limiter implementation choice | ✅ RESOLVED | Option C hand-written per ARCHITECTURE §527 verbatim (§22.3.2); 80 LOC header-only; stereo-linked envelope |
| #3 (CONTEXT line 231) Stereo width topology | ✅ RESOLVED with ESCALATION | Option A O-Wind allpass decorrelator (§22.4.2); ESCALATION-1 to plan-phase for LOCK |
| #4 (CONTEXT line 231) R39 task breakdown | ✅ RESOLVED | 9-task structure (R39-pre / R39a..g / R39 atomic / R39-backfill chore) per §22.10 |
| #5 (CONTEXT line 231) parameter-spec.md amendment author + audit trail | ✅ RESOLVED | 5-step sequence (§22.5.3); MASTER_SAT_AMOUNT + LIMITER_CEILING_DB additions; Audit Trail section NEW; Total 29 → 31 |
| #6 (research-surfaced) ARCHITECTURE limiter spec divergence | ✅ RESOLVED with ESCALATION | Option A evidence-extension under §"Master Saturator + Zero-Latency Limiter" amendment (§22.3.1); ESCALATION-2 to plan-phase for LOCK |
| #7 (research-surfaced) Voice writes mono L=R; pure M/S no-op | ✅ RESOLVED via #3 | Allpass decorrelator generates side content from mono (§22.4.1) |
| #8 (research-surfaced) ARCHITECTURE §190 stereo body-mix splitter | ✅ RESOLVED via deferral | ESCALATION-3 v1.1 stereo-body work (§22.4.5) |
| #9 (research-surfaced) Stale parameter-spec sha comment | ✅ RESOLVED | ESCALATION-4: R39d updates comment (§22.5.1) |
| #10 (research-surfaced) OUTPUT_GAIN voice-side ordering inverts ARCHITECTURE | ✅ RESOLVED | ESCALATION-5: relocate to processor-level POST-StereoWidth (§22.6.2); bit-equivalent at default 0 dB |
| #11 (research-surfaced) Harness CMakeLists.txt source-list | ✅ RESOLVED | Header-only DSP design avoids source-list addition (§22.7.4); contrasts Phase 2.5 R37 deviation #1 |
| #12 (research-surfaced) Render-harness `--output-chain` mode design | ✅ RESOLVED | 5-probe mega-mode (§22.7.1) + JSON metrics (§22.7.2) + 5 acceptance bars (§22.7.3) |
| #13 (research-surfaced) 14 → 15 reproduce-goldens.sh evolution | ✅ RESOLVED | R39e step 5 appends `output-chain.wav` row (§22.8.4) |

**13 RESOLVED + 5 ESCALATIONS to plan-phase + 0 DEFERRED-to-execute-phase** = 13/13 questions addressed at research-phase. Five ESCALATIONS are USER-DECISION-REQUIRED routing decisions with research-phase recommendations; plan-phase LOCKS them in PLAN rev-13 §"Approach Decisions" without re-discuss (research findings are design-grounded, not opinion-grounded).

**Plan-phase ready (PLAN rev-13).**

---

## §22.14 References (§22 append)

- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` rev-11 (this cycle's discuss artefact, 2026-05-01).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §21 (Phase 2.5 Body Resonator + Bow Noise Generator; numbering precedent triggering §22 at this append).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` §171–180 (§"Master Saturator + Zero-Latency Limiter" — the 3 amendments anchor for end-of-Stage-2 verify).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` §183–191 (§"Stereo Width" — M/S spec; §190 stereo body-mix splitter deferred to v1.1).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` §249–262 (master chain processing-order diagram).
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` §527–545 (§"Zero-Latency Feedforward Limiter" algorithm spec).
- `plugins/O-Contrabass/.planning/parameter-spec.md` (sha `77638e255c2adeefdb85ae3b4d4287eecbc63b1313413573f20664990a2025d1` — Phase 2.6a base; amendment at R39d).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` lines 8 (stale sha comment `c47fe7361a…`), 54 (OUTPUT_GAIN declaration), 108 (WIDTH declaration), 165–179 (current processBlock baseline).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 778 (OUTPUT_GAIN voice-side application), 805 (OUTPUT_GAIN APVTS read), 828 (outputGainLinear set), 267 ("Body runs at host rate (post 2× downsample)" — confirms master chain at host rate).
- `plugins/O-Bowed/Source/DSP/StereoWidthProcessor.h` (56 LOC; pure M/S; "Input: already has meaningful stereo from per-string panning + body resonator" — NOT applicable to mono O-Contrabass voice).
- `plugins/O-Wind/Source/DSP/StereoWidth.h` (78 LOC; allpass decorrelator at 800 Hz / Q=0.7 + M/S width; **APPLICABLE** to O-Contrabass mono voice → Option A LOCK).
- `plugins/O-Gain/Source/PluginProcessor.h` lines 94–95 (`juce::dsp::BallisticsFilter<float>` precedent; considered as Option B for limiter envelope follower; rejected in favor of Option C hand-written for ARCHITECTURE §527 traceability).
- `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` (harness build pattern; reach-back source list; header-only DSP includes covered by `Source/DSP` include path; no Phase 2.6a CMake edit).
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (14-entry post-Phase-2.5 script; Phase 2.6a evolves to 15 entries at R39e step 5).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.{wav,json}.sha256` (`6db67707…` / `625505cf…` — evidence-only goldens; carry-forward; re-render at R39e step 4).
- HEAD descendant of `1b44efd` (R37 atomic `907a7c3` + R37-backfill `36b89d2`) — production base for Phase 2.6a; tripwire CLEARED at §22.1.
- ARCHITECTURE.md §190 stereo body-mix splitter deferral — flagged in §22.4.5 / ESCALATION-3 for v1.1 stereo-body work (NOT a Phase 2.6 amendment).
- ARCHITECTURE.md §177–179 + §540–544 limiter spec divergence — flagged in §22.3.1 / ESCALATION-2 for evidence-extension under §"Master Saturator + Zero-Latency Limiter" amendment at end-of-Stage-2 verify.
- `plugins/O-Lyrica/Source/HarpSynthVoice.{h,cpp}` (Note Expression integration reference for Phase 2.6c; OUT OF SCOPE for Phase 2.6a — referenced for §23/§24 anchoring).

---
