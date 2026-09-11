# Stage 2 — DSP (live wave-terrain oscillator): RESEARCH

**Plugin:** O-Strata
**Stage:** 2 of 4 — DSP; one RESEARCH.md for **Round A** (Phases 2.1–2.3) and **Round B** (Phases 2.4–2.5) per CONTEXT D1
**Phase:** research ✓
**Date:** 2026-09-11
**Input:** `stages/2-dsp/CONTEXT.md` (D1–D4, findings 1–9, open items 1–12), `research/ARCHITECTURE.md` v2, `ROADMAP.md` v2 Stage 2, the fork at `b04690c3` (docs-only on top of `e9686d4d`; `git diff --stat e9686d4d HEAD -- Source` is empty, so every line number below holds at both)
**Method:** four parallel investigations (harness/CMake, JUCE DSP APIs, fork code, harness numerics), every claim read from `Source/`, the JUCE 8.0.14 tree at `/Users/taylorbrook/JUCE`, the repo scripts or the configured `build/` tree in this session, or measured with scratch programs on this Mac (Apple M4 Max, Apple clang 17, `-O2 -arch arm64`; sources under the session scratchpad `strata-r/`). `[VERIFIED: path:line]` = read from source; `[MEASURED: how]` = compiled and run; `[ASSUMPTION]` = neither.

---

## 1. Summary

All twelve open items resolve. Seven findings change what the planner writes; the rest confirm the CONTEXT.

1. **The harness builder already exists.** `scripts/param-dump/ParamDump.cmake` defines `ouaricon_add_processor_console(plugin src main suffix)` — the generic builder behind `ouaricon_add_param_dump`, whose doc comment says further processor-level gates "should come through here rather than copy the derivation". It derives the source list from the plugin target (drops `PluginEditor*`, picks up `NoteExpression.cpp` automatically), sets `JUCE_WEB_BROWSER=0`, never links `O-Strata_UIResources`. One call from `plugins/O-Strata/CMakeLists.txt` gives `O-Strata-render-test`. O-Bowed's hand-mirrored block is superseded (§2.1).
2. **The 2× decimator is 5 sections, not 4, and 4× latency rounds to 2, not 1.** `designIIRLowpassHalfBandPolyphaseAllpassMethod(0.06, −70)` → N = 5 (3 direct + 2 delayed), DC latency **1.26** base samples; `(0.15, −60)` → N = 3; chained 4× = **1.74** base samples. ARCHITECTURE's 1.15 / ≈ 1.4 came from the bench's tw = 0.10 coefficients. The `≤ 2` assertion still holds; the "+1 constant" is 0.26 low at 2× and 0.74 low at 4× (§2.2, decision for the plan in §4).
3. **C-note exact-cycle pinning is impossible through the parameters**; use **A2 / A4 / A6** (exact powers of two of 440 Hz) and pick the harness sample rate or FFT length so the cycles fit. A float-rounded a4 already leaves −54 dB of leakage; windowed fallbacks cannot reach −60 dB on a total-energy gate. H1 additionally needs the reference to carry the 5 Hz DC blocker (0.65° phase lead at 440 Hz = −39 dB otherwise) and to run at OS = 1 (§2.6).
4. **Ramp count is 22 per voice (11 per oscillator) + 2**, not 22 per oscillator; per-voice ramps need a `setCurrentAndTargetValue` at note-on (inactive voices never advance them); the shared-ramp saving is a ramping-state figure — at the H7 patch `getNextValue()` is a predicted branch and the real saving is ≈ 3–4 ns per oscillator-sample, not 12. `juce::Synthesiser` sub-blocks at MIDI events, so shared ramp buffers must be indexed by the **absolute** sample position (§2.4).
5. **`operator new` is not enough for H8.** `MidiBuffer::addEvent`, `AudioBuffer::setSize` and `AsyncUpdater::triggerAsyncUpdate` (fired by the **first** `processBlock` because the tuning sentinels are −1e9) allocate through `HeapBlock` → `std::realloc`. Copy O-Octagon's full new/delete family + foreign-thread tally, `ensureSize` the MidiBuffer, warm up one block, and state the coverage gap (§2.5).
6. **Only `getActiveOscFrame` is called by the page** (three sites in the `WavetableDisplay` class, awaited at page load); `getActiveOscInfo` has no caller. Stub `getActiveOscFrame` with `"[]"` in 2.1 (§2.7).
7. **Round B numbers:** M = 48 at F = 2 / 32 at F = 1 confirmed (Cosine Wells drives it); the direct Gaussian blur is **113 ms** at Blur 1 on 1024² (over the 100 ms criterion) — a 3-pass box blur is 3 ms; the triangular Clenshaw is compiler-sensitive (43 ns at −O3, 91 ns at −O2) — keep 57 ns in the PERF model (§2.10).

Confirmed as the CONTEXT expects: the 17 voice call sites and their lines; `kMaxUnison` is used only inside `WavetableOscillator`; `FastMathApproximations::tanh` is a Padé approximant that needs a clamp (±3 → 1e−6 error; non-monotone from 4.0, > 1 from 4.97); `std::exp2f` is the right exp2 for the Terrain Freq ramp on this machine; block-rate `pow` is 0.04 % of a core; `ScopedNoDenormals` sets FPCR.FZ on arm64 and the harness must render under it; `ScopedJuceInitialiser_GUI` lives in `juce_events`; `juce::Timer` and `callAsync` fire only inside `runDispatchLoopUntil`, `ThreadPool` does not need the loop; `FactoryPresets::build` returns exactly one `Init` def; the tuning gates (FUNC-09/10) drive `getTuningEngine()` directly as the smoke does, so Round A needs no message loop.

---

## 2. Open items resolved

### 2.1 Item 1 — Harness CMake shape

**Use the repo's generic builder** `[VERIFIED: scripts/param-dump/ParamDump.cmake:59-67, 83-85, 139-176, 179-265]`:

| Concern | What `ouaricon_add_processor_console` does |
|---|---|
| Sources | `get_target_property(SOURCES)` of the plugin, keeps `.cpp/.cc/.cxx/.mm`, **drops anything matching `PluginEditor`**, FATAL if `PluginProcessor.cpp` is missing (:139-176) |
| Target | `juce_add_console_app(<folder>-<suffix>)` + `target_sources(main.cpp + derived list)` (:179-186) |
| Includes / order | `${plugin_source_dir}` + `$<TARGET_PROPERTY:<plugin>,INCLUDE_DIRECTORIES>`; `add_dependencies(<console> <plugin>)` so `JuceHeader.h` exists first (:188-195) |
| Defines | `JUCE_STANDALONE_APPLICATION=1 JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JucePlugin_Build_Standalone=1` + every `JucePlugin_*` identity macro derived from the plugin target's properties (:88-133, :197-215) |
| Links | the plugin's non-`juce::`, non-`*UIResources` PRIVATE links, then the module list incl. `juce_dsp`, `juce_cryptography`, `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`, `juce_events`; binary-data targets deliberately excluded (:217-265) |
| Re-entrant | `if(TARGET …) return()` (:83-85); FATAL if `main.cpp` does not exist (:75-78) — commit the driver before configuring |

Precedent: O-Prism's second driver `ouaricon_add_processor_console(O-Prism …/Source …/tests/geometry_check.cpp geometry-check)` `[VERIFIED: plugins/O-Prism/CMakeLists.txt:132-133]`. Proof for O-Strata: the configured `build/build.ninja` link rule for `O-Strata-param-dump` carries 44 objects — the 19 non-editor `Source/**/*.cpp`, `modules/tuning/note-expression/cpp/NoteExpression.cpp.o` (arrives via `ouaricon_add_module(O-Strata note-expression)` → `target_sources` on the plugin `[VERIFIED: CMakeLists.txt:78; modules/cmake/OuariconModules.cmake:57-66]`), no `PluginEditor`, no `UIResources` `[VERIFIED: build/build.ninja:44380]`.

**Facts that close the CONTEXT's sub-questions:**
- `BinaryData::` appears only in `PluginEditor.cpp:97-116`; the generated `JuceHeader.h` includes it only under `JUCE_TARGET_HAS_BINARY_DATA`, an INTERFACE define of the binary-data target `[VERIFIED: build/plugins/O-Strata/O-Strata_artefacts/JuceLibraryCode/JuceHeader.h:29-31; JUCEUtils.cmake:530]` → **`O-Strata_UIResources` is not needed**.
- `#include <JuceHeader.h>` is pervasive (`PluginProcessor.h:33`, `StrataVoice.h:31`, every `dsp/*.h`) — the include dir comes with `INCLUDE_DIRECTORIES` `[VERIFIED: JUCEUtils.cmake:583-601]`.
- The plugin's PUBLIC `JUCE_WEB_BROWSER=1` (`CMakeLists.txt:99`) does not leak: the console never links `O-Strata`, only reads its include dirs; JUCE modules are INTERFACE libraries whose sources are `target_sources(INTERFACE)`, so each target compiles its own module objects `[VERIFIED: JUCEModuleSupport.cmake:98-99; JUCEUtils.cmake:2183-2189]`.
- Tuning sources are **vendored in `Source/`** (`TuningEngine.cpp`, `ScaleGenerator.cpp`, `TuningExporter.cpp`, `EmbeddedTunings.cpp`), not reached from `modules/tuning/scala-tuning-engine/` as O-Bowed does `[VERIFIED: CMakeLists.txt:21-42 lists exactly the 20 `.cpp` under Source/]`.
- Editor guard: `PluginProcessor.cpp:1135-1137` (`#if JUCE_WEB_BROWSER #include "PluginEditor.h"`), `:1139-1146` (`createEditor` → `GenericAudioProcessorEditor` fallback) `[VERIFIED]`. Nothing else in `Source/` references `JUCE_WEB_BROWSER`.
- `OUARICON_BUILD_TESTS` is a per-plugin `option()` (`CMakeLists.txt:106`), not set by root CMake or `build-and-install.sh` (`:264`); the local tree has it ON (`build/CMakeCache.txt:634`) and already contains `O-Bowed-render-test`. **CI does not build it**: `ci-tests.yml:97-131` skips every plugin except O-Octagon and names its two targets — a Stage 4 workflow edit if O-Strata's harness should run in CI.
- `smoke/build.sh` scrapes the `O-Strata-param-dump` link rule from `build.ninja` (objects, `LINK_LIBRARIES`, `DEFINES`, `INCLUDES`) and re-links with `clang++` `[VERIFIED: smoke/build.sh:9-21]` — retired by the CMake target.

**Recommended block** (inside the existing `if(OUARICON_BUILD_TESTS)` in `plugins/O-Strata/CMakeLists.txt`, after `ouaricon_add_param_dump`):

```cmake
    # Stage 2 offline render harness — same derived-identity console as
    # param-dump (JUCE_WEB_BROWSER=0, no editor TU, no UIResources).
    ouaricon_add_processor_console(O-Strata ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-test)

    # Fixtures resolve from the source tree, never from cwd (Stage 1 trap).
    target_compile_definitions(O-Strata-render-test PRIVATE
        STRATA_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/fixtures"
        STRATA_EXPORTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/exports")

    option(STRATA_HARNESS_ASAN "AddressSanitizer on O-Strata-render-test only" OFF)
    if(STRATA_HARNESS_ASAN)
        target_compile_options(O-Strata-render-test PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(O-Strata-render-test PRIVATE -fsanitize=address)
    endif()
```

Artefact: `build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test` `[VERIFIED: JUCEUtils.cmake:2113-2121]`. **Trap:** calling the builder from a `tests/render-harness/CMakeLists.txt` subdirectory names the target `render-harness-render-test` (folder-derived, `ParamDump.cmake:80-81`) — call it from the plugin CMakeLists. The `--fixtures` CLI flag overrides the compile-time define for ad-hoc runs; README commands use neither (the define is the default).

Build command for the executor (background, per the watchdog rule): `cmake --build build --target O-Strata-render-test` from the repo root; the configured tree already has tests ON.

### 2.2 Item 2 — `FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod`

Files are `juce_dsp/filter_design/juce_FilterDesign.{h,cpp}` (not `processors/`).

**Signature and return** `[VERIFIED: juce_FilterDesign.h:265-269, 291-292]`:
```cpp
struct IIRPolyphaseAllpassStructure {
    ReferenceCountedArray<IIRCoefficients> directPath, delayedPath;   // IIR::Coefficients<float>
    Array<double> alpha;                                              // all N allpass coefficients, design order
};
static IIRPolyphaseAllpassStructure designIIRLowpassHalfBandPolyphaseAllpassMethod (FloatType tw, FloatType stopbandDb);
```
Asserts `tw ∈ (0, 0.5]`, `stopbandDb ∈ (−300, −10)` (`.cpp:624-625`).

**Section form** `[VERIFIED: .cpp:687-696]`: each section is a second-order allpass in z⁻² at the oversampled rate, H(z) = (aᵢ + z⁻²)/(1 + aᵢ z⁻²), built as `new IIR::Coefficients (ai, 0, 1,  1, 0, ai)`; even-indexed aᵢ → `directPath`, odd-indexed → `delayedPath`, and `delayedPath[0]` is a pure z⁻¹ element `(0, 1, 1, 0)`. The `Coefficients` ctor normalises by a0 and drops it (`juce_IIRFilter_Impl.h:41-58`), so a section stores `coefficients = {b0 = aᵢ, b1 = 0, b2 = 1, a1 = 0, a2 = aᵢ}`; `coefficients` is a public `Array` (`juce_IIRFilter.h:278`).

**Extraction — one float per section** (what `juce_Oversampling.cpp:291-301` itself does; note `i = 1` skips the delay element):
```cpp
auto s = juce::dsp::FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod (tw, sbDb);   // prepare() only: allocates
jassert (s.directPath.size() == kDirect && s.delayedPath.size() == kDelayed + 1);
for (int i = 0; i < kDirect;  ++i) aDirect[i]  = s.directPath [i]->coefficients[0];      // == (float) s.alpha[2*i]
for (int i = 0; i < kDelayed; ++i) aDelayed[i] = s.delayedPath[i + 1]->coefficients[0];  // == (float) s.alpha[2*i+1]
```

**Section count** (algorithm replicated verbatim from `.cpp:627-645`, `[MEASURED: hb.cpp]`):

| Design | n | **N sections** | direct / delayed | aᵢ (design order) | stopband | passband edge |
|---|---|---|---|---|---|---|
| 2× stage, tw 0.06, −70 dB | 11 | **5** | 3 / 2 | 0.07472298, 0.26194636, 0.48801796, 0.70238295, 0.89916644 | −71.2 dB | 0.000 dB |
| 4×→2× stage, tw 0.15, −60 dB | 7 | **3** | 2 / 1 | 0.09886189, 0.36036368, 0.74375464 | −64.8 dB | 0.000 dB |
| bench `kernels.h Halfband2x` | 9 | 4 | 2 / 2 | = `design(0.10, −60…−70)` exactly | −68.9 dB | — |

So `kDirect = 3, kDelayed = 2` (2×) and `2, 1` (4×→2×) — `std::array<float, 5>` and `<3>`, not the "4 coefficients" of ARCH Core 5. Per-partial cost at 4× = 8 sections ≈ 8 × 1.66 ns `[MEASURED: mbench.cpp, one chained section]` — the same order as the bench's 8.95 ns.

**Reference per-sample loop** — mirror `Oversampling2TimesPolyphaseIIR::processSamplesDown` `[VERIFIED: juce_Oversampling.cpp:385-386, 396-428]`: the sections run at the **decimated** rate on the even / odd sub-streams, so the z⁻² form collapses to a one-state transposed allpass (`out = a·in + v; v = in − a·out; in = out`), one `v` per section (the bench's DF-I `AP` with four states does the same maths at higher cost); `directStages = N − N/2` (direct gets ⌈N/2⌉); the delayed path's extra z⁻¹ is realised as `delay = oddPathOut` **after** the sum, i.e. `y = 0.5·(directOut + previousOddOut)`. `snapToZero` is compiled out on arm64 (`JUCE_SNAP_TO_ZERO` → `ignoreUnused` unless `JUCE_INTEL`, `juce_FloatVectorOperations.h:38-44`) — state hygiene relies on FTZ (§2.3).

**Group delay at DC / latency** — first-order allpass (a + z⁻¹)/(1 + a z⁻¹): τ(0) = (1 − a)/(1 + a); in z⁻² at the oversampled rate: 2(1 − a)/(1 + a). D₀ = Σ_even, D₁ = 1 + Σ_odd, L_os = ½(D₀ + D₁), **L_out = L_os / 2** at the decimated rate. Cross-checked against a numeric d(arg H)/dω and against JUCE's `getPhaseForFrequency(0.0001, 1)` method (`juce_Oversampling.cpp:283-289`): all three agree to four decimals `[MEASURED: hb.cpp]`.

| Stage | D₀ | D₁ | L_os | **latency at output rate** |
|---|---|---|---|---|
| 2× (0.06, −70) | 2.5162 | 2.5194 | 2.5178 | **1.259** base samples |
| 4×→2× (0.15, −60) | 1.9340 | 1.9404 | 1.9372 | 0.969 (at 2 fs) |
| chained 4× (JUCE-style sum `L4/4 + L2/2`) | | | | **1.743** base samples |
| bench prototype (tw 0.10) | | | 2.2944 | 1.147 (ARCH's "1.15") |

Assertion for `prepare()` (no polynomial expansion needed):
```cpp
auto dc = [] (auto& a) { double d = 0; for (float x : a) d += 2.0 * (1.0 - x) / (1.0 + x); return d; };
const double L2 = 0.5 * (dc (aDirect2) + 1.0 + dc (aDelayed2)) / 2.0;             // ≈ 1.259
const double L4 = 0.5 * (dc (aDirect4) + 1.0 + dc (aDelayed4)) / 4.0 + L2;         // ≈ 1.743
jassert (L2 <= 2.0 && L4 <= 2.0);
```
Passband group-delay ripple near the band edge (2×: 2.5 → 9.7 oversampled samples) is normal for elliptic halfbands and irrelevant to the latency report. Consequence for Decision 5: see §4 (plan decision).

### 2.3 Item 3 — Fast-maths bounds and costs

**Implementations** `[VERIFIED: juce_dsp/maths/juce_FastMathApproximations.h:111-117 (tanh), 243-248 (exp), 181-187 (sin), 152-158 (cos)]` — all Padé approximants; `juce_MathsFunctions.h` has no fast transcendental helpers.

**`tanh` error, float, 200 k-point scans** `[MEASURED: fastmath.cpp]`:

| range | max abs err | | failure |
|---|---|---|---|
| [−1, 1] | 1.4e−7 | | first non-monotone point **x = 4.004** |
| [−2, 2] | 1.8e−7 | | exceeds 1.0 at **x = 4.972** |
| **[−3, 3]** | **1.1e−6** | | diverges as x/28: 1.009 at 10, 3.67 at 100 |
| [−4, 4] | 1.5e−5 | | |
| [−5, 5] (documented) | 1.0e−4 | | |

**Clamp the Saturation argument to ±3** (monotone, 1e−6) — `jlimit + Padé` = 7.2 ns chained vs `std::tanhf` 13.0 ns. The bench's `saturate()` scales by 1.313 before tanh; clamp the scaled argument.

`exp` (Padé): usable to ±2 (2.3e−5); the documented −6…+4 is far too generous (exp(−6) is +106 % off). `sin`/`cos`: 1e−5 / 7e−5 over [−π, π], 0.1 outside — wrap first. Not needed: Apple libm `sinf`/`cosf` are 8.1 / 8.4 ns chained, 1.24 ns in a 16-lane throughput loop, and clang already pairs them (`sinf + cosf` = 9.4 ns = `__sincosf`).

**exp2 for the log-domain Terrain Freq ramp** (x ∈ [−2, 3]) `[MEASURED: mbench.cpp / tput.cpp / e2chk.cpp]`:

| call | chained ns | 16-lane ns |
|---|---|---|
| `std::exp2f` | 6.34 | 0.97 |
| deg-4 minimax + exponent bits (rel err 5.4e−6 = 0.009 cent) | 7.34 | 0.37 |
| deg-5 (2.9e−7) | 8.07 | — |
| `std::powf(2, x)` | 6.35 | — |

**Verdict: `std::exp2f`.** The polynomial lengthens the dependency chain (floor + convert + bit add) and only wins 0.6 ns in the throughput regime; one call per oscillator per sample is ≤ 1 ns amortised. (If ever needed: `p = 1.000005256f + f·(0.692974285f + f·(0.241508801f + f·(0.051989599f + f·0.013511545f)))` on f = x − ⌊x⌋; Taylor polynomials are 100× worse — do not use them.)

**Other per-partial costs** `[MEASURED: mbench.cpp]`: `powf(x, 0.5f)` 3.35 ns is already special-cased (`sqrtf` 2.91); `powf(x, 5.f)` 14.7 ns vs `s²·s²·s` 2.8 ns — confirms ARCH's sin⁵ note. Butterfly's `expf(cosf θ)` = 11.8 ns chained / 2.1 ns 16-lane; a degree-6 Chebyshev fit of exp(c) on [−1, 1] (`0.999999862 + c(1.000016602 + c(0.500004966 + c(0.166517675 + c(0.041639423 + c(0.008659274 + c·0.001435877)))))`, rel err 1.9e−5) saves ≈ 2 ns of a ≈ 29 ns orbit — optional, only if Butterfly shows up in H7. Squarcle's `1/tanh k` per block: one `tanhf` = 13 ns per block, trivial.

**Block-rate `pow`** (Item 4 of the brief): `powf(ratio, track)` 8.6 ns, `powf(2, −1 − 9·damp)` 6.8 ns, double `pow` 10.6 ns; 64 calls per block ≈ 0.5 µs = **0.04 %** of a core at 64-sample blocks — negligible, confirmed. Note `WavetableOscillator::setUnison` already does `std::pow` per partial per block (`.cpp:132`, called every block from `StrataVoice.cpp:473-480`); the port either caches on (count, detune, width) change or the DSP-05 grep gate allows it under `updateBlockRate`.

**`ScopedNoDenormals` on arm64** `[VERIFIED: juce_FloatVectorOperations.cpp:1544-1563, 1451, 1479]`: ORs **FPCR bit 24 (FZ)** via inline `mrs/msr`, restores on scope exit; per-thread CPU state, no process-type dependency. `[MEASURED: fpcr.cpp]`: a console process starts with FPCR = 0 (denormals **on**; `1e-38f·1e-3f` = 9.99967e−42), after the bit `= 0`. FZ on arm64 flushes inputs and outputs (no separate DAZ). **The harness must call `processBlock` under a `ScopedNoDenormals`** (or `processBlock`'s own one covers it — it does, `PluginProcessor.cpp:728`; the harness's *reference* renders and any direct `TerrainOscillator` micro-benchmarks need their own) or decaying-tail timings will not match the plugin.

### 2.4 Item 4 — Shared-ramp design

**How values reach the oscillator today** `[VERIFIED]`: `juce::Synthesiser synthesiser` (private, `PluginProcessor.h:168`), 16 voices `new`ed in the ctor (`.cpp:580-590`), `renderNextBlock` at `.cpp:803`. `juce::Synthesiser::processNextBlock` **splits the block at MIDI events** into sub-blocks of ≥ `minimumSubBlockSize = 32` and calls `renderVoices(buffer, startSample, numSamples)` per sub-block (`juce_Synthesiser.cpp:180-236, 254-258`) — `StrataVoice::renderNextBlock` may run several times per `processBlock`, each with an absolute `startSample`; the per-sample loop already uses that absolute index (`StrataVoice.cpp:512`). The mod matrix is **per voice** (`StrataVoice.h:84-85`; `destOffsets` `std::array<float, 46>`, `ModulationMatrix.h:124, 166`; `evaluate()` fills then accumulates over 16 slots, `.cpp:70-87`). Sources set per sample at `StrataVoice.cpp:543-552`: LFO1–4 are per-voice objects (phase-locked to processor phases only when free-running, `:454-464`), envelopes per voice, Velocity / NoteNum note-constant, ModWheel / Aftertouch processor atomics read once per block (`:483-484`) — the only stepped, global sources. No smoothing exists in the voice (`grep SmoothedValue Source/` → `PluginProcessor.h:268-269`, `dsp/DelayProcessor.h:60` only). Inactive voices return before any of this (`:365-366`).

**What ARCH actually smooths:** the **APVTS base value** — "one `SmoothedValue` per modulated base value (22 per voice)" = **11 per oscillator (Pos + 10 new)**, plus two for ModWheel / Aftertouch as sources `[VERIFIED: ARCHITECTURE.md:149, 153]`; mod offsets are added raw. So there is no per-voice "(value, increment)" ramp to design — per-voice sources are continuous (env, LFO) or note-constant.

**`SmoothedValue<float, Linear>::getNextValue`** `[VERIFIED: juce_SmoothedValue.h:284-303, 309-322, 374, 384-389]`: `if (!isSmoothing()) return target;` else countdown + `currentValue += step`; `setTargetValue` early-outs on an equal target. Two regimes `[ASSUMPTION — µop reasoning]`: steady state ≈ 0.3–0.5 ns (load, compare, predicted branch), ramping ≈ 1–1.5 ns. ARCH's "≈ 1 ns per ramp → −12 ns per partial" is the **ramping** figure; at the H7 default patch (no automation) the per-voice ramps cost ≈ 4–5 ns per oscillator-sample and the shareable saving is **≈ 3–4 ns**, not 12. Counts: (a) per-voice = 24 × 16 = **384** `getNextValue()` per sample at 16 active voices (the brief's 704 doubled the count); (b) shared = 24 × blockSize per **block** + 22 L1 loads per voice-sample.

**Recommendation for the planner:**
- **Phase 2.1 — design (a) exactly as ARCH Core 9**, plus one line the CONTEXT lacks: in `startNote` when `!wasActive`, `setCurrentAndTargetValue` on all 24 ramps — an idle voice never advances its ramps (`:365-366`), so it would otherwise glide a stale base value over 5 ms at note-on. `setTargetValue` per sub-block is safe (early-out).
- **Phase 2.3 — design (b), processor-owned buffers**: `std::array<juce::SmoothedValue<float>, 24> baseRamps` + `juce::AudioBuffer<float> rampBuffers` (24 × `samplesPerBlock`, sized in `prepareToPlay`); before `synthesiser.renderNextBlock` (`:803`) set targets (Terrain Freq in log2) and fill each row — `getNextValue()` per sample only while `isSmoothing()`, else `FloatVectorOperations::fill` — so the steady-state cost is 24 vectorised fills per block. Voices cache the 22 read pointers at block start and index `ramp[sample]` with the **absolute** loop index (this is what makes the `Synthesiser` sub-block split free); Terrain Freq = `exp2f(ramp[sample] + 2·offset)` once per oscillator; ModWheel / Aftertouch rows replace the block-constant `modWheelVal` (`:551-552`). Behavioural bonus: every voice sees identical, always-current base values and the note-on reset disappears. **Capacity guard:** never `setSize` on the audio thread — `jassert` + clamp the index, or chunk `renderNextBlock` when a host exceeds `samplesPerBlock` (planner's call, flag in PLAN).
- **H7 reports the per-voice → shared delta explicitly** (both builds, best of 3) so SUMMARY records the real saving, not the model's −12 ns. If the saving is ≈ 3 ns the ladder's next rung (fb = 0 block skip, then the float terrain phase path) is reached sooner than ARCH expects.

### 2.5 Item 5 — Allocation counter

**In-repo precedent to copy:** `plugins/O-Octagon/tests/render-harness/main.cpp:110-241` `[VERIFIED]` replaces **every** variant — `operator new`, `new[]`, both `std::align_val_t` overloads and the eight matching `delete`s (":118-119: an un-replaced aligned-new is silently uncounted, and a probe that counts nothing passes") — counts only when `armed && this_thread::get_id() == ownerThread`, tallies other threads into `foreignAllocations` and prints it beside the verdict (:143-155, :181-192); warm-up rule at :1843-1845 ("the first processBlock triggers libc++/JUCE first-touch initialisation"); `-fsanitize=realtime` is unsupported by Apple clang 17 (:113, verified by running it). A `thread_local bool armed` is equivalent and naturally excludes the message / Timer / `callAsync` threads — keep the foreign tally visible either way.

**JUCE after `prepareToPlay` (8.0.14)** `[VERIFIED]`:

| Class | Allocates? | Evidence |
|---|---|---|
| `MidiBuffer::addEvent` | **yes, via `Array<uint8>` → `HeapBlock::realloc` = `std::realloc`** on growth (`juce_MidiBuffer.h:358; .cpp:155; juce_ArrayBase.h:240, 428-432; juce_HeapBlock.h:366`); `clear()` keeps storage | `midi.ensureSize(N)` before arming |
| `AudioBuffer::setSize` | early-out when size unchanged (`juce_AudioSampleBuffer.h:390`); else `HeapBlock` calloc | pre-size once |
| `SmoothedValue` | no heap members | — |
| `Synthesiser::renderNextBlock` / `noteOn` / `startVoice` | no (`ScopedLock` on a `CriticalSection`; `usableVoicesToStealArray` pre-reserved in `addVoice`, `std::sort`); `MidiMessage` from `getMessage()` is inline for ≤ 8 bytes (`juce_MidiMessage.h:1028-1038`) — **SysEx would allocate** | keep MIDI to channel messages |
| `dsp::Oversampling` (distortion) / `DryWetMixer` | all `setSize` in `initProcessing` / `prepare` / `setWetLatency` | — |
| **`AsyncUpdater::triggerAsyncUpdate`** | **yes**: `activeMessage->post()` → mac `MessageQueue::post` → `ReferenceCountedArray::add` → `std::realloc` on growth (`juce_AsyncUpdater.cpp:74-81; juce_MessageQueue_mac.h:65-72`); only when not already pending | see below |
| `HeapBlock` | `std::malloc/calloc/realloc/free`, never `operator new` (`juce_HeapBlock.h:147, 354-367`) | **invisible to the override** |

**O-Strata `processBlock` audit (`PluginProcessor.cpp:726-969`)** `[VERIFIED]`: no `vector::push_back`, `String`, `MemoryBlock`, `ValueTree`, `setSize` anywhere in the range; `retiredTables.push_back / erase` are message-thread (`:998, 1023-1026`); `StrataVoice.cpp` grep for `new/vector/push_back/String/setSize/resize/make_unique/HeapBlock/Array<` hits only a comment (`:318`). **One trap:** `:751-781` tuning change detection → `triggerAsyncUpdate()` at `:780`; the sentinels `lastMasterTune / lastOctaveStretch / lastPitchBendRange = −1.0e9f`, `lastTuningPreset = lastTonic = −1` (`PluginProcessor.h:180-181, 243-245`) mean **the very first `processBlock` after construction always posts** (malloc-level). The warm-up block absorbs it; any tuning-parameter change inside an armed window posts again (harness: never change the five tuning params while armed). `[ASSUMPTION]` `vst3Extensions.drainAndUpdate()` (`:733`) and `ModulationMatrix::evaluate` are arithmetic only (grep-clean, bodies not read line by line).

**Coverage statement for SUMMARY:** covered = every `new`-expression on the arming thread (`std::vector`, `std::function` captures, `make_unique`, `juce::String`'s `StringHolder` — `[ASSUMPTION]` `String` may use `std::malloc`; treat as not covered unless verified in 2.1); **not covered** = anything through `HeapBlock` (`AudioBuffer`, `Array`, `MidiBuffer`, `ReferenceCountedArray`, `OwnedArray`, `MemoryBlock`) and the `AsyncUpdater` post path. Options: (i) state the gap exactly as O-Octagon does ("measured via the replaced operator new family; HeapBlock / malloc paths remain grep + inspection") — **recommended for Round A**; (ii) add a macOS `malloc_zone_t` hook (copy the default zone's function pointers, install counting wrappers) — catches `HeapBlock`, but coexists poorly with ASan (which installs its own zone) `[ASSUMPTION]`, so counter and ASan are **separate configurations**. `-Wl,--wrap` does not exist in ld64.

**ASan (Round B)**: no sanitizer use exists anywhere in `plugins/*/tests`, `cmake/`, `scripts/`, `.github/` `[VERIFIED: grep]`. The block in §2.1 instruments the console target **and** the JUCE module objects it compiles (INTERFACE sources inherit `COMPILE_OPTIONS`); the plugin target is untouched; no LTO on the console (the builder links only `config_flags` + `warning_flags`). Run with `ASAN_OPTIONS=detect_leaks=0` (LSan unsupported on Apple Silicon `[ASSUMPTION]`) and either `alloc_dealloc_mismatch=0` or `#ifndef STRATA_HARNESS_ASAN` around the operator-new family (its `delete` calls `std::free`).

### 2.6 Item 6 — Exact-cycle f0 inside the plugin

**Bench method** `[VERIFIED: alias.cpp:22-23, 41-74, 81, 94-95, 107, 123-124]`: N = 65536 at fs = 48000; f0 = fs·k/N with k odd (gcd(k, N) = 1) so harmonics sit on bins k·h and nothing else does; render exactly k cycles at OS·fs, decimate with a 1023-tap Kaiser sinc, rectangular window over a true period; `nonHarmPow` = Σ|X[b]|² over b mod k ≠ 0. Part 1 prints **nonharm / fundamental**, Part 2 **nonharm / strongest harmonic** + the highest harmonic index above −100 dB. ARCH H6 uses the Part-2 convention (`ARCHITECTURE.md:531`); the −60.2 dB "sine-product 2×" in `alias-output.txt:5` is nonharm/fund, which coincides there because h1 is strongest. **The harness prints both and gates on strongest-harmonic.**

**Pitch path in the plugin** `[VERIFIED]`: `TuningEngine::calculate12TETFrequency` = `a4Frequency · 2^((note − 69)·octaveStretch/12)` (`TuningEngine.cpp:807-811`); `setMasterTune` clamps 400–480 Hz and ignores |Δ| ≤ 0.01 Hz (`:105-111`); tonic is inert in 12-TET. Parameters: `masterTune` `NormalisableRange(420, 460, 0.1)` default 440 (`PluginProcessor.cpp:289-290`), `octaveStretch` (0.95–1.25, 0.001) default 1, `pitchBendRange` int default 2, `tuningPreset` default 0 = 12-TET. No reference-note parameter exists. `setValueNotifyingHost` **snaps to the 0.1 Hz grid** (`RangedAudioParameter::convertFrom0to1` → `snapToLegalValue`, `juce_RangedAudioParameter.cpp:54-58`): 439.453125 → 439.5 `[MEASURED: acc.cpp]`. Parameter → engine sync is **deferred to `handleAsyncUpdate`** (`PluginProcessor.cpp:751-780, 975-989`), which never runs without a message loop (a Round A harness) and runs *later* with one (Round B) — but `setStateInformation` syncs synchronously (`:1097-1105`) and `getTuningEngine()` is public (`.h:81`; the smoke harness drives `loadScalaFile` through it, `smoke/main.cpp:152-158`). Voice: `freq = tuning · 2^((Coarse + Fine/100)/12)` (`StrataVoice.cpp:253, 395`); glide Off snaps and Legato / Always snap on the first note (`GlideProcessor.h:59-64`); pitch mod only if a slot is On (all default Off); unison 1 ⇒ `unisonDetuneFactors[0] = 1.0` exactly.

**Why lattice pinning cannot work** `[MEASURED: leak.cpp — N = 65536, 20 flat harmonics, rectangular]`: exact f0 → −224 dB; a float-rounded a4 (ε = 6e−8) → **−54 dB** (fails −60); a 0.1 Hz-grid residual → +6.6 dB; Hann / Blackman-Harris fallbacks with ±5-bin guards → −10 / −11 dB with a −6 dB/oct spectrum. Leakage of a fractional-bin error scales as (πδ)²/3 with δ = h·k·ε, growing with harmonic index; single mid-point bins are clean (Hann −143 dB at 178 bins) but a **total** non-harmonic-energy gate at −60 / −90 dB needs an exactly periodic window. **Windowed fallback: rejected.**

**Recipe — exact in double through the ordinary path:** `pow(2, n)` is exact for integer n `[MEASURED: acc.cpp]`, so with default tuning (a4 440, stretch 1, 12-TET), Coarse / Fine 0, Unison 1, glide Off, no routes, no bend, **A2 / A4 / A6 (MIDI 45 / 69 / 93) = 110 / 440 / 1760 Hz exactly.** Then fit the cycles:
- **Option C1 (recommended, zero new FFT code):** `prepareToPlay(fs = 440·65536/600 = 48059.7333… Hz)`, N = 65536 → A2 / A4 / A6 = 150 / 600 / 2400 cycles, harmonic bins 150h / 600h / 2400h (h ≤ 200 / 50 / 12 below 22 kHz). Rate rows: 44.1 k → fs = 440·65536/652 = 44226 Hz; 96 k → k = 300, 96119 Hz. The 0.12 % rate offset is immaterial to an alias ratio; the ROADMAP "within 3 dB across rates" criterion still holds.
- Option C2: nominal fs and N = 2.5 s·fs (mixed radix → Bluestein over a 2¹⁸ `dsp::FFT`, ≈ 15 ms per render).
- Option A (only if C-notes are mandatory): `getTuningEngine()->setMasterTune(a4)` as a **double** after the first `processBlock` + one `runDispatchLoopUntil` drained the pending sync, never touching the tuning params afterwards, re-applied after any `setStateInformation` — fragile.
- **Wording change for the plan:** H6 rows are A2 / A4 / A6 (ARCHITECTURE:531, ROADMAP:166, 190 say C2 / C4 / C6 — recorded correction, contracts not edited). H4's partial counts are a windowed count and stay on C-notes.

**Clean patch for H6** `[VERIFIED: defaults in PluginProcessor.cpp]`: oscB Level 0, sub / noise 0, FX mixes 0, `filtAEnvDepth` 0 already. Traps: `ampSustain` 0.7 / decay 0.3 s — discard ≥ 0.35 s before the analysis window or set sustain 1.0; **both filters default LP24 at 20 kHz in series** (no "Off" type) — analyse 0–20 kHz, or add a harness-only **pre-filter oscillator tap** (recommended; consistent with the four harness-only switches). Oversampling keeps harmonics on-bin (LTI decimator): −134 dB at 2× `[MEASURED: acc.cpp]`. **Accumulator precision:** a `double` [0, 1) accumulator with float θ at the kernel gives a −130 dB floor; a **float radians accumulator gives −52 dB** `[MEASURED: acc.cpp]` — if the PERF-02 "float terrain phase path" fallback is taken, only the kernel input may be float; the accumulator stays double.

**H1 θ-parity specifics** `[VERIFIED: ARCHITECTURE.md:524-526]`: the reference is **analytic** and must re-implement the θ pipeline — `applyWarp` (Bend `pow(phase, 1 + 3a)`, FM `frac(phase + fmIn·a)`), Sync ratio 1 + 3a with re-seed on master wrap, Window `× sin(π·masterPhase)`, detune `2^(pos·detune·50/1200)`, pan cos / sin, gain 1/√n (`WavetableOscillator.cpp:110-139, 192-256`). **These formulas are deleted in Phase 2.1 — copy them into `tests/render-harness/reference/theta_reference.h` before the deletion** (first task of the 2.1 plan). Metric (unspecified in ARCH): recommend 20·log10(RMS(y − g·ref)/RMS(ref)) over the steady-state window, g = least-squares scalar (absorbs level 0.8 × sustain 0.7 × master 0.8 × pan). Two blockers at −80 dB: (i) the per-oscillator 5 Hz DC blocker leaves 0.65° phase lead at 440 Hz → |1 − H| ≈ **−39 dB** — apply the same one-pole to the reference or tap pre-blocker; (ii) run H1 at **OS = 1** (Bandlimited with `chebPtr == nullptr` falls back to the analytic terrain at 1× — the 2× decimator adds 1.26 samples of delay and passband ripple). `kernels.h` must **not** become the H1 reference: its terrains are transcribed from *Terrain* (GPL-3.0; `kernels.h:39` says so) and use the 2πF-style law — only its neutral pieces (radix-2 FFT, `analyse()`, Kaiser decimator) are worth copying into `tests/render-harness/reference/` with a provenance comment; no cross-tree `#include` from `research/`.

### 2.7 Item 7 — Native-function stubs vs page strip

**Page callers** `[VERIFIED: Source/ui/public/index.html]`: `getActiveOscInfo` — **zero** callers. `getActiveOscFrame` — three, all in class `WavetableDisplay`: `:3431` `this.getFrame = Juce.getNativeFunction('getActiveOscFrame')`; `:3448` lazy re-acquire in `fetchAndDraw`; `:3453-3457` `const result = await this.getFrame(this.oscIndex, position); if (result) { this.samples = typeof result === 'string' ? JSON.parse(result) : result; this.draw(); }` inside `try/catch`; `draw()` returns at `if (!samples.length)` (`:3461-3463`). Instances `wtDisplayA/B` (`:3494-3495`); `fetchAndDraw` runs on every `oscAPos`/`oscBPos` change **and once at page load** (`refresh()`, `:3506`) — the awaited promise at load is exactly the bridge-gap hang.

**C++ side** `[VERIFIED: PluginEditor.cpp:492-523]`: both native functions read `processorRef.getActiveOscTable(osc)` (`PluginProcessor.cpp:1052-1055`; `oscTablePtr` at `.h:118, 176-179`).

**Recommendation: stub both in Phase 2.1, remove in Phase 3.1** (page untouched, Stage 1 D1). `getActiveOscFrame` → `complete ("[]")` (truthy → `JSON.parse` → `[]` → `draw()` early-returns; nothing throws, canvas stays blank). `getActiveOscInfo` → `complete ("{}")` or drop (no caller). Both stubs are table-free, so `WavetableData`, `getActiveOscTable`, `oscTablePtr`, `placeholderTable`, `lastAssignedTable`, `updateWavetableAssignments` (`.cpp:1026-1046`, called every block at `:784`) and `setWavetableA/B` (`StrataVoice.cpp:169-177`) go in 2.1 and `grep -rn Wavetable Source/` only needs the stub bodies clean.

### 2.8 Item 8 — Phase seeding path

`[VERIFIED: PluginProcessor.cpp:94-96; StrataVoice.cpp:262-271, 285-292; WavetableOscillator.cpp:89-96, 98-108]`: `osc?Phase` is `AudioParameterFloat(0..1, 0.001, default 0)`; in `startNote`, `if (!wasActive || glideMode == 0)` → `phase > 0.0001f ? resetWithPhase(phase) : resetWithRandomPhases()` — **no reset at all on a legato retrigger**. `resetWithRandomPhases` is an LCG re-seeded from `this ^ 0x12345678` **on every call** (same voice object ⇒ same phases every note; differs across instances / ASLR). `resetWithPhase` sets all partials to one phase.

**Recommendation: processor-level atomic.** `std::atomic<uint32_t> harnessPhaseSeed { 0 }` + public setter / getter; `startNote` reads it relaxed and calls `TerrainOscillator::resetWithRandomPhases(seed)`: `seed == 0` → the existing address hash (production unchanged), else `seed ^ (voiceIndex · 0x9E3779B9u) ^ (oscIndex << 16)` so 32 partial sets stay distinct. The voice does not know its index today — add `setVoiceIndex(i)` in the ctor loop (`PluginProcessor.cpp:580-590`). A harness-only setter on the voice would need `synthesiser` (private, `.h:168`) exposed — more surface for no gain. Keep the Stage 1 smoke's second leg (`osc?Phase = 0.25`, `oscMix = 0`, `smoke/main.cpp:263-287`) for H9 / H10 where a single fixed phase suffices; the seed is what gives unison-4 renders distinct-but-deterministic partials.

### 2.9 Item 9 — `kMaxUnison` fan-out

`grep -rn kMaxUnison Source/` `[VERIFIED]`: **every** occurrence is inside `WavetableOscillator` — `.h:76` (`= 8`), `.h:77-80, 88` (`phaseAccumulators`, `unisonDetuneFactors`, `unisonPanL/R`, `masterPhases`, all `double`), `.cpp:40, 82, 91, 102, 112`. The voice and processor have none; the per-sample loop runs to `unisonCount` (`.cpp:228`). `osc?Unison` is `AudioParameterInt(1, 4, 1)` (`PluginProcessor.cpp:97-98`), `int`-cast at `StrataVoice.cpp:257, 280, 473, 477`. **8 → 4 is complete inside the new class.**

**Unison laws are at `WavetableOscillator.cpp:110-140`** (CONTEXT: 104-131) — copy verbatim: `unisonGain = 1/√count`; `centerIndex = (count − 1)/2`; `normalizedPos = (i − centerIndex)/centerIndex`; `detuneFactor = 2^(normalizedPos·detune·50/1200)`; `panNorm = clamp((normalizedPos·width + 1)/2)`; `panL = cos(panNorm·π/2)`, `panR = sin(…)`; count 1 → unity everything. Contains `std::pow` per partial per call (see §2.3).

### 2.10 Item 10 — Round B pre-research

**Message loop in a console harness** `[VERIFIED]`: `ScopedJuceInitialiser_GUI` is declared in **`juce_events`** (`juce_Initialisation.h:67-87`; `initialiseJuce_GUI` in `juce_MessageManager.cpp:457-465`) — the smoke (`main.cpp:115`) and O-Bowed (`main.cpp:132`) construct one first thing; the constructing thread is the message thread. `MessageManager::runDispatchLoopUntil(ms)` on macOS pumps `CFRunLoopRunInMode` + `NSApp nextEventMatchingMask` until the deadline (`juce_MessageManager_mac.mm:379-407`). **`juce::Timer` fires only inside the loop** (`TimerThread` posts a `CallTimersMessage`, `juce_Timer.cpp:109, 127, 264-270`) — so `OStrataAudioProcessor::timerCallback` (`startTimer(500)`, `PluginProcessor.cpp:647`), the reaper (`:1002-1024`), the latency follow-up and `handleAsyncUpdate` (`:975`) never run in Round A; Round B pumps `runDispatchLoopUntil(≥ 600)` after each stimulus to drain the 500 ms timer, and the scheduler's `callAsync` publish goes through the same queue. `ThreadPool` has zero references to `MessageManager` (`juce_ThreadPool.cpp/.h`) — independent. `ImageFileFormat::loadFrom(const void* rawData, size_t)` / `loadFrom(InputStream&)` / `loadFrom(const File&)` (`juce_ImageFileFormat.h:129-148`), `PNGImageFormat` at `:159`, default list PNG / JPEG / GIF (`.cpp:55-57`); `juce_graphics` declares `OSXFrameworks: Cocoa QuartzCore` (`juce_graphics.h:54-55`) — linked automatically by the builder.

**Quadrature node count M** `[VERIFIED: ARCHITECTURE.md:318 states M = max(32, 8·⌈2F⌉ + 16) → 32 at F ≤ 1, 48 at F = 2]` — **confirmed** `[MEASURED: quad.cpp / quad2.cpp — contamination of the 153 triangle coefficients vs an M = 512 truth, F = 2, mx = my = 0.5, max|Δc|/max|c|]`:

| terrain | M = 24 | 28 | 32 | 40 | 48 |
|---|---|---|---|---|---|
| Sine Product / Saddle / Rings | ≤ −207 dB | ≤ −298 | ≤ −298 | ≤ −293 | ≤ −303 |
| **Cosine Wells** (cos(4π·)⁴ → 16π) | −30 | −47 | −48 | **−115** | −241 |
| Ridged Cosines (C⁰) | −24 | −29 | −31 | −42 | −40 |
| Mitsuhashi (C⁰) | −29 | −39 | −41 | −47 | −43 |

Wells is the driver (M ≥ 40 at F = 2; at F = 1 it needs M ≥ 28 → 32 is right). The C⁰ terrains never converge below ≈ −40 dB at any practical M — a **fit** question only: any polynomial terrain over a trig-polynomial orbit is exactly bandlimited whatever its coefficients, so H6's −90 dB does not depend on M. Cost incl. node / T-table build: M = 32 → 0.11 ms, 48 → 0.17 ms, 64 → 0.30 ms (ARCH's 0.1 / 0.3 ms confirmed). No per-note refit exists (the set is pitch-invariant; D_max masking is per voice). Theory for the record: cos(a x) = J₀(a) + 2Σ(−1)ᵏJ₂ₖ(a)T₂ₖ(x); Gauss–Chebyshev nodes alias T_{2Mj ± n} onto T_n, so coefficient 16 is first contaminated by mode 2M − 16; |J_n(4π)|: n = 22 → −85 dB, 26 → −130 dB.

**PNG import cost, 1024², M4 Max** `[MEASURED: blur.cpp]`: separable Gaussian by direct FIR: σ = 6.4 px (Blur 0.2) 18 ms, σ = 16 54 ms, **σ = 32 (Blur 1.0) 113 ms — over the ≤ 100 ms Phase 2.5 criterion (ROADMAP:220) by itself**; ARCH's "10–20 ms" holds only at the default blur. **Fix: 3-pass box blur** (running sums, O(1)/px, box width √(4σ² + 1) matched to σ): **3.2 ms at σ = 6.4, 2.9 ms at σ = 32**, σ-independent. 64² node grid = box-average W/64 = 16 px at each node (bilinear + node-matched low-pass in one step) + separable 17 × 64 contraction: **0.36 ms**. The scheduler's analytic projection and the import projection share the node / contraction code (M = 64 vs 32 / 48).

**Clenshaw cost** `[MEASURED: clen.cpp / clen2.cpp, bench-style dependency-carried loop incl. a 12.5 ns orbit]`: 16 × 16 tensor 56.8 ns at −O2 and −O3 (matches `bench-output.txt:15`); 17 × 17 tensor 83 ns; **naive triangle 91 ns at −O2, 43 ns at −O3** (variable trip counts defeat the row interleaving the compiler does for the fixed tensor). CMake Release is −O3 here (no override in either CMakeLists). **Keep 57 ns in the PERF model**; the 2.4 plan adds a harness micro-check comparing the triangle layout against the 16 × 16 tensor + two T₁₆ corner terms (58.5 ns) as the fallback layout. Operation count: triangle ≈ 153 + 17 + 17 FMAs; tensor 256 + 16.

**D_max and taper**: D_max = clamp(⌊0.5·fs/(K·f_note)⌋ − 1, 1, 16) `[VERIFIED: ARCHITECTURE.md:116]` keeps one full harmonic of margin below Nyquist ((D_max + 1)·K·f_note ≤ fs/2); table checks K = 1 at C8 → 4 ✓, K = 8 at C4 → 10 ✓. ARCH gives **no taper formula**; `[ASSUMPTION]` for click-free glide the weight must be continuous in f_note, not in the integer D_max: D_c = 0.5·fs/(K·f_note) − 1 (real), w(d) = ½(1 + cos(π·clamp(d − D_c + 2, 0, 2)/2)) for diagonal d = n + m — w = 1 for d ≤ D_c − 2, → 0 at d = D_c; per block per voice, ≤ 2 diagonals (≤ 33 coefficients) rescaled. The 2.4 plan should adopt this or an equivalent continuous law.

### 2.11 Item 11 — H2 preset loop

`FactoryPresets::build(apvts)` returns `std::vector<OuariconPresetManager::FactoryPresetDef>` (`FactoryPresets.h:40-41`) with exactly one def — `category = name = "Init"`, every `RangedAudioParameter` at `getDefaultValue()` (`FactoryPresets.cpp:37-48`). **Vacuous confirmed.** `FactoryPresetDef = { String category, name; std::map<String, float> parameters; var customState; }` (`OuariconPresetManager.h:97-103`). The manager is disk-based (`loadPreset` reads `~/Library/O-Strata/Presets/…`, `OPM.h:306-363`) — **never route H2 through it** (the constraints forbid `~/Library` in the harness). Its private `applyPresetJson` (`OPM.h:249-269`) resets every non-excluded param to default, then `setValueNotifyingHost` each key; `excludedParameterIds` (`OPM.h:118`) and `getPresetManager()` (`PluginProcessor.h:151`) are public.

**Harness API (no disk, no message thread), re-runs unchanged in Stage 4:**
```cpp
for (const auto& def : FactoryPresets::build (proc.getAPVTS()))
{
    for (auto* p : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! proc.getPresetManager().excludedParameterIds.contains (rp->getParameterID()))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (const auto& [id, v] : def.parameters)
        if (auto* p = proc.getAPVTS().getParameter (id)) p->setValueNotifyingHost (v);
    // render 1 s at C4, H2 gate
}
```
The PLAN specifies that H2 in Round A iterates the **terrain × orbit grid** as the real coverage and prints `presets: 1 (Init)` so the vacuity is visible in the log (memory `pattern_whitelist_gate_goes_vacuous_on_generic_receiver`); Phase 4.1 re-runs the same loop over the full bank.

### 2.12 Item 12 — `.gitignore` scope

Root `.gitignore` `[VERIFIED]`: `build/`-style dirs (:85-92), `*.o` (:117), executables only `*.exe / *.out / *.app` (:143-145) — **an extensionless binary is not ignored**; `*.log` (:217); `*.wav` only at root (`/*.wav`, :245) and under `plugins/*/.planning/evidence/**` (:48); one exact path for O-Bowed's golden WAV (:68) with the rationale "match the artifact, never broaden to the containing directory" (:63-67). Checks: `strata-smoke` → `git check-ignore -v` returns nothing (untracked, 11.5 MB, regenerable); `smoke-output.log` is **tracked** despite `*.log` (index-aware); hypothetical `tests/exports/x.wav`, `tests/render-harness/golden/x.wav` → not ignored. Nested precedent: `plugins/O-Octagon/tests/monitor-fold/.gitignore` = `*.o` + the binary name. O-Bowed tracks `golden/canonical-preset.json` + `.wav.sha256` only.

**Recommended `plugins/O-Strata/tests/render-harness/.gitignore`:**
```gitignore
# Harness run products — regenerable; goldens are tracked as .sha256 ONLY
# (pattern_golden_tracked_as_checksum_only). Match artifacts, not directories.
exports/
golden/*.wav
golden/*.json
!golden/*.sha256
*.png
```
(`tests/exports/` per the CONTEXT sits beside `render-harness/`; if it stays at `plugins/O-Strata/tests/exports/`, put the file at `plugins/O-Strata/tests/.gitignore` with `exports/` and `render-harness/golden/*.wav` paths instead.) With the CMake target there is no in-tree binary to ignore. For the smoke dir: add a one-line `smoke/.gitignore` with `strata-smoke`, or delete the binary — the plan's first commit should do one of the two so `git status` is clean at every Stage 2 commit.

---

## 3. Corrections to the CONTEXT (recorded, contracts not edited)

| # | CONTEXT / ARCH said | Found |
|---|---|---|
| 1 | Harness after O-Bowed's `tests/render-harness/CMakeLists.txt` (Decision 12) | `ouaricon_add_processor_console` in `ParamDump.cmake:59-67` is the sanctioned generic builder (O-Prism geometry-check precedent); O-Bowed's block mirrors `JucePlugin_*` literals, compiles the editor and links UIResources — none needed |
| 2 | Source list = PluginProcessor, StrataVoice, FactoryPresets, dsp/*, tuning / note-expression module sources | `NoteExpression.cpp` comes from `modules/tuning/note-expression/cpp/` via the plugin's `target_sources`; tuning TUs are vendored in `Source/`; the builder derives all of it |
| 3 | 2× stage "2 second-order sections each path, 4 coefficients" (ARCH Core 5) | tw 0.06 / −70 dB → **5** sections (3 + 2); 4×→2× → 3 (2 + 1) |
| 4 | Latency 1.15 (2×) / ≈ 1.4 (4×), "≤ 0.4 sample inter-path skew" (ARCH Core 5, Decision 5) | **1.26 / 1.74** with the specified designs; 2×-vs-4× skew 0.48; 4× rounds to 2 — see §4 |
| 5 | 22 ramps "per oscillator" / 704 `getNextValue` per sample (research brief) | 22 per **voice** (11 per osc) + 2 = 384 per sample; saving ≈ 3–4 ns at the H7 patch, not 12 |
| 6 | "LFOs global, envelopes per-voice" | LFOs are per-voice objects; only ModWheel / Aftertouch are global and block-constant |
| 7 | Unison laws at `WavetableOscillator.cpp:104-131`; phase members `.h:70-77`; timer latency site `:1005-1009` | `:110-140`; `masterPhases` at `.h:88`; `:1006-1009` |
| 8 | Both `getActiveOsc*` functions called by the page | only `getActiveOscFrame` (index.html:3431, 3448, 3453; awaited at load `:3506`) |
| 9 | H6 exact-cycle at C2 / C4 / C6 (ARCH:531, ROADMAP:166, 190) | not reachable exactly through any parameter; **A2 / A4 / A6** with fs = 440·65536/600 (or mixed-radix N) |
| 10 | Import job 1024² "≈ 10–20 ms" (ARCH Core 8), ≤ 100 ms criterion (ROADMAP:220) | direct Gaussian = 113 ms at Blur 1; 3-pass box = 3 ms |
| 11 | Triangle Clenshaw "≈ 35 ns expected" (ARCH Core 6) | 43 ns at −O3, 91 ns at −O2; keep the 57 ns ceiling |
| 12 | Allocation gate = global `operator new` counter (ARCH H8) | HeapBlock / `AsyncUpdater` paths are malloc-level and invisible; first `processBlock` always posts an async update |
| 13 | `ScopedJuceInitialiser_GUI` "in juce_gui_basics" (implied by item 10) | `juce_events` |
| 14 | Feedback / DC: H1 reference = analytic cosine | must include the 5 Hz DC-blocker response (−39 dB otherwise) and run at OS = 1 |
| 15 | `setUnison` is allocation- and pow-free block-rate plumbing | contains `std::pow` per partial per call, every block (`WavetableOscillator.cpp:132`) — DSP-05 grep gate must allow it under `updateBlockRate` or the port caches it |
| 16 | CI runs `OUARICON_BUILD_TESTS` harnesses | `ci-tests.yml` builds only O-Octagon's two targets; O-Strata's harness needs a Stage 4 workflow edit to run in CI |

---

## 4. Decisions the plan must take (with recommendations)

1. **Latency report: +1 or +2?** Measured 1.26 (2×, default) / 1.74 (4×) / 0 (Bandlimited). **Recommend keeping the contract's constant +1** — it is exact to 0.26 sample on the default path, Decision 5 already accepts per-oscillator skew, and the alternative (report +2 and integer-delay the 2× / Bandlimited paths by one sample) adds state to the hot path for a synth-inaudible gain. `prepare` asserts the measured values ≤ 2 and SUMMARY prints them. If Taylor prefers +2, the change is confined to the report sites and a one-sample delay line per oscillator output.
2. **H6 pitch rows → A2 / A4 / A6 at fs = 48059.73 Hz** (Option C1). Alternative C2 (Bluestein) if the harness must stay at 48000 exactly. The rate-invariance row (44.1 / 96 k) uses k = 652 / 300.
3. **Harness-only pre-filter oscillator tap** for H1 / H6 (the default LP24 @ 20 kHz series filters have no Off), added to the four harness switches. Alternative: analyse 0–20 kHz only.
4. **H8 coverage in Round A = operator-new family + stated HeapBlock gap** (O-Octagon precedent); a `malloc_zone_t` hook is a Round B option, never combined with ASan.
5. **Shared ramps (2.3): processor buffers indexed by absolute sample position**, capacity guard by assert + clamp (not `setSize` on the audio thread); H7 prints the per-voice → shared delta.
6. **Native stubs**: `getActiveOscFrame → "[]"`, `getActiveOscInfo → "{}"`, both removed in 3.1.
7. **Phase seed**: processor atomic + `setVoiceIndex`; `seed == 0` = production behaviour.
8. **Blur**: 3-pass box blur in 2.5 (σ-matched), not direct Gaussian.
9. **Taper law**: the continuous-in-f_note raised cosine of §2.10 (or equivalent) — ARCH leaves it unspecified.
10. **Copy the warp formulas into `tests/render-harness/reference/theta_reference.h` as the first 2.1 task**, before `WavetableOscillator.cpp` is deleted.

---

## 5. Harness skeleton the planner can lift

- **CLI** (`main.cpp`): `--gate H1..H11|smoke|all`, `--note`, `--velocity`, `--seconds`, `--terrain`, `--orbit`, `--quality`, `--set <id>=<norm>` (repeatable), `--preset <name>`, `--fs`, `--block`, `--seed`, `--fixtures <dir>` (default `STRATA_FIXTURES_DIR`), `--export <name>` (into `STRATA_EXPORTS_DIR`), `--print-only`. Exit code = number of failed checks; every check prints its measured number and its threshold.
- **Processor access**: `createPluginFilter()` → `dynamic_cast<OStrataAudioProcessor&>`; `getAPVTS()`, `getTuningEngine()`, `getPresetManager()` are public; harness-only switches as public atomics on the processor (`feedbackPathEnabled`, `terrainKernelBypass`, `singleSampleFeedback`, `preFilterTap`, `harnessPhaseSeed`).
- **Render loop**: `prepareToPlay(fs, block)`; `AudioBuffer` pre-sized once; `MidiBuffer` `ensureSize(64)`; one warm-up `processBlock` (absorbs the first-block `triggerAsyncUpdate` post and libc++ first touch) before arming the counter; `ScopedNoDenormals` around any direct kernel benchmark.
- **Spectral core** (`reference/spectrum.h`): radix-2 FFT, exact-cycle `analyse()` printing nonharm/fund **and** nonharm/max + highest harmonic index, Hann 8192 / hop 100 ms windowed h1 test for H2, spectral centroid for the Mod X / Y / Orbit Size criteria; provenance comment for the pieces adapted from `alias.cpp`.
- **Round A has no `runDispatchLoopUntil`** (a `ScopedJuceInitialiser_GUI` is still constructed first); Round B adds a `pump(ms)` helper used by the scheduler and import gates.
- **`--smoke`** reproduces Stage 1 checks [1]–[6] with [4] inverted (LFO1 → `OscA Terrain Freq`, index 31, **changes** the render; the unrouted render is the control) and fixture paths from `STRATA_FIXTURES_DIR` (`test-tunings/just-major.scl` copied into `tests/render-harness/fixtures/`).

---

## 6. References

- Repo: `scripts/param-dump/ParamDump.cmake`; `plugins/O-Prism/CMakeLists.txt:132-133`; `plugins/O-Octagon/tests/render-harness/main.cpp:110-241` (allocation counter); `plugins/O-Bowed/tests/render-harness/` (superseded pattern); `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/{alias.cpp, kernels.h, bench.cpp, alias-output.txt, bench-output.txt}`
- JUCE 8.0.14: `juce_dsp/filter_design/juce_FilterDesign.{h,cpp}`, `juce_dsp/processors/juce_Oversampling.cpp`, `juce_dsp/maths/juce_FastMathApproximations.h`, `juce_audio_basics/utilities/juce_SmoothedValue.h`, `juce_audio_basics/synthesisers/juce_Synthesiser.cpp`, `juce_audio_basics/buffers/juce_FloatVectorOperations.cpp`, `juce_events/messages/{juce_Initialisation.h, juce_MessageManager.cpp, juce_AsyncUpdater.cpp}`, `juce_events/native/juce_MessageManager_mac.mm`, `juce_graphics/images/juce_ImageFileFormat.h`, `extras/Build/CMake/{JUCEUtils.cmake, JUCEModuleSupport.cmake}`
- Scratch measurements (session scratchpad, not committed): `hb.cpp`, `fastmath.cpp`, `mbench.cpp`, `tput.cpp`, `fpcr.cpp`, `e2chk.cpp`, `strata-r/{quad, quad2, leak, clen, clen2, blur, acc}.cpp`
- Memories applied: `pattern_render_harness_breaks_on_webview_editor`, `pattern_test_fixture_mirrors_drift_silently`, `pattern_recorded_gate_command_not_executable_as_spelled`, `pattern_random_start_phase_seeded_from_this_breaks_render_diff`, `pattern_webview_native_fn_bridge_gap`, `pattern_whitelist_gate_goes_vacuous_on_generic_receiver`, `pattern_golden_tracked_as_checksum_only`, `pattern_probe_must_target_the_branch_the_fix_changed`, `pattern_wallclock_inside_a_stability_verdict`, `pattern_check_ignore_is_index_aware`
