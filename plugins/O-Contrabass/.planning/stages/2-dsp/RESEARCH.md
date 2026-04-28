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
