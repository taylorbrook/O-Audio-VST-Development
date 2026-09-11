# Stage 2 — DSP (live wave-terrain oscillator): CONTEXT

**Plugin:** O-Strata
**Stage:** 2 of 4 — DSP; ROADMAP Phases 2.1 → 2.5, run as **two rounds** (D1): Round A = 2.1–2.3, Round B = 2.4–2.5
**Phase:** discuss ✓
**Date:** 2026-09-10
**Mode:** manual (interactive — four decisions put to Taylor, answers recorded below)
**Branch:** `main` (trunk-based; path-scoped commits under `plugins/O-Strata`)
**Baseline commit:** `e9686d4d` (Stage 1 second pass ✅ VERIFIED — 205 parameters, 46 mod destinations, sine placeholder oscillator, `Init` factory bank, COMPAT-01 complete)
**Inputs:** `research/ARCHITECTURE.md` v2 (Core 1–11, Algorithms, Decisions 1–12, Harness H1–H11), `ROADMAP.md` v2 Stage 2, `REQUIREMENTS.md` v2.0.0, `parameter-spec.md` v2 (locked), `stages/0-ideation/CONTEXT.md` (D1–D12 + open questions), `stages/1-foundation/VERIFICATION.md` §Stage Verdict (carry-forward list), the fork in `Source/`

---

## Goal

Replace the sine placeholder with the live wave-terrain oscillator end to end on the DSP side: `TerrainOscillator` in place of `WavetableOscillator` (11 orbits, 6 analytic terrains, θ parity with the inherited warps), the 20 new mod destinations read per sample with smoothing, pitch-tracked terrain frequency, bounded trajectory feedback, per-oscillator 2× / 4× halfband oversampling with the CPU and aliasing gates, the Chebyshev Bandlimited mode with its off-thread scheduler, and the PNG terrain path through a processor API — every phase landing with its exit-code harness gates in `plugins/O-Strata/tests/render-harness/`.

Requirements verified in this stage: **FUNC-01..06, FUNC-09, FUNC-10, DSP-01..07, PERF-01, PERF-02, QUAL-01, QUAL-02**, plus the DSP halves of FUNC-07 (import API) and FUNC-08 (bytes determinism). UI is Stage 3; persistence child content, presets and CI are Stage 4.

## Inherited contracts — NOT re-opened

Settled by Stage 0 v2 (`stages/0-ideation/CONTEXT.md` D1–D12), the locked `parameter-spec.md` v2 and the Stage 1 second pass. Where ROADMAP / ARCHITECTURE and the spec disagree, **the spec wins**; where ARCHITECTURE and ROADMAP disagree with each other, the Stage 0 CONTEXT decision table wins (it was written last).

| Contract | Source | Value |
|---|---|---|
| Oscillator interface | ARCH Core 1; `WavetableOscillator.h:45-63` | `TerrainOscillator` keeps the public interface minus `setWavetable`; the voice's call sites compile unchanged after the member type swap (`StrataVoice.h:181-182`); new block-rate setters + per-sample float setters as listed in Core 1 |
| Libraries | ARCH Core 2 / 3, Decision 10 | 11 orbits (K per orbit as tabled), 6 terrains under the **πF convention**, clean-room formulas, `pow`-free (Superellipse LUT in `prepare`, Wells' fractional power via integer-power lerp, Butterfly 2π-periodic wing term) |
| Feedback | ARCH Core 4, Decision 1 | leaky integrator on a two-sample average, a = 1 − 2^(−1 − 9·Damp) rate/OS-corrected, `d` ±0.5, `p` clamped to [−1, 1]², `y` clamped, NaN-scrubbed, reset at note-on, û = orbit rotation vector, 5 Hz DC blocker on the oscillator output only; Feedback 0 bit-identical (0·finite) |
| Pitch tracking | ARCH Algorithm "Pitch-tracked terrain frequency", Decision 2 | F_eff = F_mod · min(1, C4/f_note)^track, block-rate, glide **target** frequency; F_mod = clamp(F_base · 2^(2·offset), 0.25, 8) |
| Oversampling | ARCH Core 5, Decision 5 | hand-rolled per-sample polyphase-IIR decimator per partial, coefficients from `FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod` in `prepare` (2×: tw 0.06 / −70 dB; 4×→2×: tw 0.15 / −60 dB); both stages pre-allocated; Quality switch = enum flip + state reset + 64-sample equal-gain crossfade; **constant +1 sample** latency added to the distortion latency at both report sites (`PluginProcessor.cpp:720`, `1005-1009`) |
| Bandlimited | ARCH Core 6 / 7, Decision 3 | degree-16 triangle (153 coeffs), Clenshaw at OS = 1, F clamped [0.25, 2], Pitch Track inert, D_max = clamp(⌊0.5·fs/(K·f_note)⌋ − 1, 1, 16) with the 2-diagonal raised-cosine taper, one-block crossfade on pointer change; terrain destinations block-rate for global sources / bypassed for per-voice sources; scheduler 50 ms poll / 50 ms debounce / one job per oscillator / `callAsync` publish / generation-counted reaper; `prepareToPlay` and `setStateInformation` never publish |
| PNG path (DSP side) | ARCH Core 8, Decision 4 | decode ≤ 1024², luminance, separable Gaussian pre-blur (σ = blur · W/32 px), Mirror / Window edge, bilinear read, 64² node-matched Chebyshev projection at import at F = 1; import job on the ThreadPool; processor API `importTerrainImage(osc, MemoryBlock, name)` / `importTerrainFile(osc, File)` |
| Voice integration | ARCH Core 9, Decision 7 | 22 `SmoothedValue` base ramps (5 ms) + ModWheel / Aftertouch ramps; offset → parameter mappings as tabled (Rotation offset × 180°, Centre additive clamped, Terrain Freq log-domain ±2 octaves, the rest additive clamped); `updateBlockRate` feed; FM cross-feed / pan / level / mix unchanged |
| Mod-destination strings | spec §Mod-matrix destinations; `ModulationMatrix.h:102-111` | already in the binary: `OscA Orbit Size … OscA Saturation` long form (ARCH Core 9's short `"OscA CtrX"` style is superseded — Stage 1 CONTEXT). `static_assert (NumDests == 46)` at `ModulationMatrix.h:91` |
| Deletion | ARCH Decision 6 | `WavetableOscillator`, `WavetableData`, `WavetableGenerator`, `oscTablePtr[]`, `placeholderTable`, `lastAssignedTable`, `getActiveOscTable`, `getActiveOscInfo` / `getActiveOscFrame` go in Phase 2.1; `retireTable` → type-erased `retire(std::unique_ptr<Retirable>)`; `updateWavetableAssignments` → `updateOscillatorAssignments` |
| Quality default | Decision 8 | `2×` (index 1); Bandlimited is the showcase |
| Symmetry gate | Decision 9, harness H2 | defaults c = (0.13, 0.21), aspect 0.7, r = 0.5, F = 1; gate per preset, not monotone in F; negative control Centre (0, 0) / Aspect 1 / Sine Product |
| Harness | Decision 12, ARCH "Harness design" | `tests/render-harness/` `juce_add_console_app` under `OUARICON_BUILD_TESTS`, `PluginProcessor.cpp` compiled with `JUCE_WEB_BROWSER=0`, no editor TU; harness-only switches `feedbackPathEnabled`, `terrainKernelBypass`, `TerrainKind::HarnessIdentityX`, allocation counter; gates H1–H11 exit-code, numbers printed, negative controls on the branch they target; WAVs under `tests/exports/` gitignored, goldens `.sha256` only |
| Unison cap | Decision 10 / PERF-02 | `kMaxUnison = 4` in the oscillator (parameter already clamps 1–4) |
| Parameters | spec v2 (locked) | **no parameter, range, list or default changes in Stage 2.** The only sanctioned list change is the H6 fallback (2): an `Auto` fourth Quality entry **appended** at index 3, `2×` stays the default at index 1 |

**Read-only in this stage:** `parameter-spec.md`, `BRIEF.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` (checksummed contracts — corrections are recorded here and in SUMMARY.md, not edited in), everything under `plugins/O-Prism/`, `Source/ui/public/**` (Stage 3 re-forks the page). `REQUIREMENTS.md` is **not** checksummed and is amended once in this phase (D2).

## Discuss findings (the fork as found at `e9686d4d`)

1. **The voice has no smoothing and reads none of the new destinations.** `StrataVoice.h:181-182` holds `WavetableOscillator oscA, oscB`; `StrataVoice.cpp` has no `SmoothedValue`; the 17 oscillator call sites are at `.cpp:147-148` (prepare), `171` / `176` (`setWavetable` — removed with the class), `254-291` (frequency, unison, phase / `resetWithRandomPhases`), `383-384` (warp type), `473-480` (unison), `533-534` (glided frequency), `577-578` (`setPosition`), `585-586` (warp amount), `589-590` (FM cross-feed), `594-595` (`getNextSampleStereo`). ARCHITECTURE Core 1's enumeration holds. `ModulationMatrix::destOffsets` is `std::array<float, 46>` and cells 26–45 accumulate unread (Stage 1 D3).
2. **The reaper to generalise:** `struct RetiredTable` + `retireTable` (`PluginProcessor.h:296-302`), `retireTable` body at `.cpp:995`, `timerCallback` reaper at `.cpp:1002-1024`, `blockGeneration.fetch_add(release)` at `.cpp:968`. Latency report sites: `.cpp:720` (`prepareToPlay`) and `.cpp:1005-1009` (timer follow-up) — both add the constant +1 in Phase 2.3.
3. **Test-target block is param-dump only.** `plugins/O-Strata/CMakeLists.txt:104-116` gates a single `ouaricon_add_param_dump` under `OUARICON_BUILD_TESTS`; `plugins/O-Strata/tests/` holds only UI fixtures (`i18n-states.json`, `ui_tip_render_check.js`, `ui-stub/`). `tests/render-harness/` is new. O-Bowed's harness (`plugins/O-Bowed/tests/render-harness/CMakeLists.txt`) compiles `PluginEditor.cpp` for the `createEditor` symbol — O-Strata's editor is a WebView, so the harness must instead compile the processor with `JUCE_WEB_BROWSER=0` (the `createEditor()` guard at `PluginProcessor.cpp:1137-1145` returns the generic editor) and never list the editor TU — the Stage 1 smoke pattern, not O-Bowed's.
4. **The bench prototype lives at the repo root**, not under the plugin: `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/` (`alias.cpp` exact-cycle spectral harness, `bench.cpp`, `kernels.h` with `Halfband2x::AP`, Clenshaw and the orbit / terrain kernels, `alias-output.txt`, `bench-output.txt`). BRIEF §Research cites this path; `plugins/O-Strata/.planning/research/` does not contain it.
5. **Smoke harness cwd trap and polarity flip.** `stages/1-foundation/smoke/main.cpp:116` resolves `test-tunings/just-major.scl` from `getCurrentWorkingDirectory()`, and check [5] resolves `~/Library/O-Strata/Presets` (`:301`). Its check [4] (D3: a slot routed to a new destination leaves the placeholder output unchanged) **inverts** the moment Phase 2.1 reads the destinations. The smoke's `build.sh` relinks the param-dump objects by scraping `build.ninja` — a hack that breaks when the object list changes in 2.1. The untracked `smoke/strata-smoke` binary must stay untracked.
6. **CPU reference machine = the benchmark machine.** This Mac is an Apple M4 Max, the §7.2 machine, so the CPU budget model's ns figures (≈ 101 ns per partial before savings → ≈ 74 ns after; 15.5 % → ≈ 11.4 % oscillator delta) transfer without re-scaling.
7. **`WavetableOscillator` uses `double` phase** (`WavetableOscillator.h:70-77`: `currentSampleRate`, `frequency`, `phaseIncrement`, `phaseAccumulators[kMaxUnison]`), `kMaxUnison = 8`. Decision 11's fallback (float θ for the terrain branch) is a real change, not a no-op; `kMaxUnison` → 4 shrinks every per-partial array.
8. **Native functions that read the wavetable** (`getActiveOscInfo` / `getActiveOscFrame`, `PluginEditor.cpp` ≈ 492-524) are registered by the guarded editor and called by the stripped v1.24.0 page for the ≋ view. Removing the C++ side while the page still calls them hangs the promise silently (memory `pattern_webview_native_fn_bridge_gap`) — research item 7 decides stub vs. strip.
9. **Fork base still O-Prism v1.24.0** (v1.25.0 = wavetable bank, v1.26.0 = markup only — nothing to pull; Stage 1 CONTEXT).

## Decisions taken this phase

### D1 — Two rounds, split at the Phase 2.3 decision point

Taylor: *"Two rounds, split at 2.3."*

- **Round A = Phases 2.1 + 2.2 + 2.3** (core at 1×, feedback + tracking, oversampling with H6 / H7). **Round B = Phases 2.4 + 2.5** (Bandlimited + scheduler, PNG path + consolidation). One `RESEARCH.md` covers both rounds; `PLAN.md` / `SUMMARY.md` / `VERIFICATION.md` are written **per round**.
- **Artifact layout** mirrors Stage 1's two passes: Round A writes `stages/2-dsp/PLAN.md`, `SUMMARY.md`, `VERIFICATION.md`; when Round A verifies, those three move to `stages/2-dsp/round-a/` and Round B writes fresh ones at the fixed paths (the tooling resolves fixed paths). The final `VERIFICATION.md` cites `round-a/VERIFICATION.md` for the Round A gates and re-runs `render-harness --all`. STATUS carries `round: A` / `round: B`.
- **Commits stay per ROADMAP phase** (three in Round A, two in Round B) with the ROADMAP's commit messages; each phase's harness gates are green before its commit.
- **Round A's verify is the decision point:** the H6 / H7 outcomes (which fallbacks were applied, if any: shallower lobes, `Auto` Quality, F_eff harmonic budget; shared ramps, fb = 0 skip, float phase path) are recorded in Round A's SUMMARY and are **inputs** to Round B's plan — Bandlimited and images are layered onto the settled 2× loop, never onto a loop that may still change.
- **Why:** ROADMAP "Critical Path" names 2.3 as the point where the default design is proven or falls back; a five-wave single plan would land a fallback rework mid-run under Bandlimited and image code; per-phase cycles add three discuss/plan/verify rounds for phases whose decisions are already settled.

### D2 — PERF-02 amended to "oscillator delta ≤ 12 %"

Taylor: *"Amend to oscillator delta ≤ 12 %."*

- `REQUIREMENTS.md` PERF-02 row and its acceptance line are amended **in this phase** (the file is not a checksummed contract): the gate is the **oscillator delta** = wall ÷ audio at the default patch minus the same render with the harness-only `terrainKernelBypass` switch (scan returns 0, decimator skipped, rest of the voice loop unchanged) — 16 voices × 2 osc, unison 1, Quality 2×, 48 kHz, 10 s, Release build, pinned, best of 3. The harness prints **total, baseline and delta**; the whole-plugin total is **reported, not gated**. Unison 4, 4× and Bandlimited deltas are reported alongside.
- ARCHITECTURE Decision 11's open question is closed; ARCHITECTURE / ROADMAP text that still says "of one core" is read as the delta (recorded here; contracts not edited).
- **Fallback ladder stands** (ARCH Risks "CPU over the 12 % line"): the two planned savings (ramps shared per oscillator, fb = 0 block skip) run **before** the gate; then the float terrain phase path; then dirty-flag smoothing (ramp only routed / automated destinations); last resort 15 % with Taylor's sign-off, documented in SUMMARY.
- **Why:** §7.2's 8.8 % never counted the inherited O-Prism voice loop (4 LFOs, 16-slot matrix, 4 SVFs, envelopes, FX); the full-path model puts the oscillator alone near 15 % before savings, so a whole-plugin 12 % would force the fallback ladder for a number the brief never meant.

### D3 — No *Terrain* listening pass before Stage 2; Decisions 1 and 3 stand as designed

Taylor: *"Skip it; keep D1 and D3 as designed."*

- Phase 2.2 implements the bounded feedback exactly as ARCH Core 4 (two-sample average + leaky integrator); **no** Damp < 0.1 → single-sample blend. Phase 2.4 implements the degree-16 single-set Bandlimited mode exactly as ARCH Core 6 / 7.
- **The listening material is the harness:** Phase 2.3 exports the terrain × orbit × pitch WAV grid to `tests/exports/` (gitignored, goldens as `.sha256`); the listening pass proper is QUAL-04 in Stage 4. If it asks for rawer feedback, the Risks-table blend is an internal change to the damp law (no parameter, no list, no range) and can land in Round B or Stage 4 with the H3 grid re-run.
- The Stage 0 open question 2 is closed; BRIEF "Next Step 1" is left unticked as a Stage 4 listening note.

### D4 — Bandlimited F-lattice is v1.1

Taylor: *"v1.1."*

- Phase 2.4 ships **one coefficient set per oscillator**; Terrain Freq clamped [0.25, 2] in this mode, Pitch Track inert, the three terrain destinations block-rate for ModWheel / Aftertouch (folded into `ChebKey`) and bypassed for per-voice sources. The readout atomics (`partialsAtC4`, fit %, exact / approximate) are exposed for Stage 3 so the Terrain tab can state the limits.
- `CHANGELOG.md` v1.0.0 gains a "Known limits — Bandlimited mode" entry naming the clamp, the inert Pitch Track and the block-rate terrain modulation; the F-lattice (13 sets over F at ¼-octave, per-voice block-rate lerp) is recorded under the v1.1 candidates in REQUIREMENTS "Out of Scope".
- Stage 0 open question 3 is closed. (Open question 4, host strings, was closed by the mockup v2 lock; all four Stage 0 questions are now answered.)

## Corrections recorded, not edited in (orchestrator)

- ARCH Core 9's mod-destination strings (`"OscA Aspect"`, `"OscA CtrX"`, …) are superseded by the spec's long form already in the binary (Stage 1 CONTEXT). Phase 2.1 does not touch `getModDestNames()`.
- ROADMAP Phase 2.1 says "22 `SmoothedValue` base ramps" per voice; ARCH Decision 11 moves them to the processor in Phase 2.3. Both are right in sequence: **2.1 per voice (Core 9), 2.3 shared** — and "shared" cannot mean shared `SmoothedValue` objects (`getNextValue()` mutates), so it means processor-owned per-block ramp buffers that voices index (research item 4).
- ROADMAP Phase 2.5 H10 is the bytes-determinism half only; the `terrainImports` state-child content, the > 2 MB path + SHA branch and the missing-file notice are Stage 4 (Phase 4.1), as REQUIREMENTS FUNC-08 / QUAL-03 already say.
- The Stage 1 smoke harness's check [4] polarity inverts in Phase 2.1 (finding 5); its six checks are folded into the render harness as `--smoke`, with [4] asserting that a slot routed to a new destination **changes** the render. `stages/1-foundation/smoke/` is kept as history and not run again.

## Constraints carried into implementation

- **DSP-05 grep gate:** no `pow`, `std::function`, `new` / `make_unique` in `TerrainOscillator.cpp`, `Orbits.h`, `Terrains.h`, `HalfbandDecimator.h`, `ChebyshevSet.h` outside `prepare` (block-rate `pow` for the tracking ratio and the damp coefficient lives in `updateBlockRate`, one call each per oscillator per block).
- **Harness fixtures never resolve from cwd** (Stage 1 trap): fixture paths come from a compile-time `STRATA_FIXTURES_DIR` define or a `--fixtures` flag; every README command is executable as written from any directory (memory `pattern_recorded_gate_command_not_executable_as_spelled`).
- **Deterministic start phase for render diffs:** H9 (block-size invariance) and H10 (identical bytes ⇒ identical SHA) need a start phase that does not depend on the oscillator's address or block count — `TerrainOscillator` gets a seedable `resetWithRandomPhases(uint32 seed)` (Stage 1 carry-forward; memory `pattern_random_start_phase_seeded_from_this_breaks_render_diff`); the harness also pins `osc?Phase > 0` where it compares renders sample-for-sample.
- **Terrain Freq is an exact-log range:** smoothing and modulation of `osc?TerFreq` work in the log2 domain (ramp log2 F_base, add 2·offset, one exp2 per oscillator per sample — not per partial), never a linear offset on the 0.25–8 value (Stage 1 carry-forward).
- **Block-size invariance traps** (memories): `a_eff`, `r_track`, D_max and the Superellipse re-normalisation are computed from constants / note frequency at block start, never from the block length; one RNG per purpose, never interleaved across partials; no per-block envelope followers in the oscillator path; the SmoothedValue ramp length is time-based (5 ms) and the ramp buffers (2.3) are filled per block from the same time base.
- **Every gate has a negative control on the branch it targets** (memory `pattern_probe_must_target_the_branch_the_fix_changed`): H2 (0,0)/Aspect 1; H3 `feedbackPathEnabled = false` + `singleSampleFeedback` Nyquist peak; H4 Track 0 at F = 8; H5 0 s ramp; H7 `terrainKernelBypass`; H1 a deliberately wrong warp must fail.
- **Verdicts are the measured delta, not the wall clock** (memory `pattern_wallclock_inside_a_stability_verdict`); H7 runs Release, pinned, best of 3.
- **`getLatencySamples()` is non-virtual** — `setLatencySamples(distortion + 1)` from `prepareToPlay` and the timer, every Quality combination.
- **Choice lists append-only from v1.0.0** — the only sanctioned change is the H6 fallback `Auto` Quality entry at index 3; nothing else in the parameter set moves.
- **Fast maths ranges:** `FastMathApproximations::tanh` is a Padé approximant — clamp its input (research item 3 fixes the bound); Saturation branch skipped exactly at 0 (DSP-07 identity is exact).
- **`ScopedNoDenormals` already wraps `processBlock`**; the decimator allpasses and the DC blocker run under it.
- **Publish / retire on the message thread only**; the harness pumps `MessageManager` only in Round B (scheduler, import job); Round A's harness has no message loop and must not need one.
- **Executor 600 s watchdog** (memory): builds and the `--all` run in the background; individual gates stay under ≈ 3 min (`--all` ≤ 3 min total per ROADMAP 2.5).
- **Standalone stays stale after `build-and-install.sh`**; the Stage 2 verify installs VST3 + AU via the script, pluginval strictness 10 and `auval` (background, 2-minute budget) at the end of each round.
- **O-Prism isolation:** `git status --short plugins/O-Prism` and `git diff --stat HEAD -- plugins/O-Prism` empty at every commit.
- **Commit discipline:** `git branch --show-current` + `git status --short` immediately before `git commit -- plugins/O-Strata PLUGINS.md`; new files `git add`ed first; never `-a` / `-A`; **no tag**; `tests/exports/**` and harness binaries gitignored (scoped `.gitignore` inside `plugins/O-Strata/tests/render-harness/`).
- **Two sessions share the checkout** — path-scope everything.

## Stage 2 test criteria (ROADMAP Phases 2.1–2.5 as amended by D1–D4)

Round A (Phases 2.1–2.3) and Round B (Phases 2.4–2.5) each verify **every** ROADMAP test criterion of their phases verbatim, with these amendments:

- [ ] **PERF-02 (H7):** oscillator **delta** ≤ 12 % at the default patch (16 v × 2 osc, unison 1, 2×, 48 kHz, 10 s, Release, best of 3); total and baseline printed; unison 4 / 4× / Bandlimited deltas reported (D2)
- [ ] **H9 / H10 determinism:** renders at 64 / 256 / 1024 block sizes identical to −100 dB **with the seeded phase**; the unseeded path documented as non-deterministic (constraint above)
- [ ] **Harness cwd:** every `tests/render-harness/README` command runs identically from the repo root and from `/`
- [ ] **Smoke fold-in:** `render-harness --smoke` reproduces the Stage 1 checks [1]–[6] with [4] inverted (routing LFO1 → `OscA Terrain Freq` **changes** the render; the unrouted render is the control)
- [ ] **Round A exit:** all of H1–H9 green in one `render-harness --all` run; pluginval strictness 10 VST3 + AU and `auval` pass on the 2× build; `grep -rn Wavetable Source/` empty; the H6 / H7 outcome (fallbacks applied or not) recorded in `SUMMARY.md`
- [ ] **Round B exit:** H1–H11 green in one `--all` run ≤ 3 min; swap-storm and import-job assertions per ROADMAP 2.4 / 2.5; pluginval + auval pass again; `CHANGELOG.md` "Known limits — Bandlimited mode" entry (D4); `stages/2-dsp/SUMMARY.md` (Round B) carries the full H6 table, H7 total / baseline / delta and the measured latencies
- [ ] **FUNC-09 / FUNC-10 formal verification (Phase 2.1):** the tuning tab loads a Scala file and a 31-EDO generator; rendered fundamental = `TuningEngine` frequency within 0.5 cent on ≥ 5 keys (harness, not smoke)
- [ ] No parameter, range, list or default changed except a possible appended `Auto` Quality entry (param-dump diff vs `.planning/params.tsv` empty, or exactly the `osc?Quality` rows)

## Open for the research phase

1. **Harness CMake shape:** a `juce_add_console_app` under `OUARICON_BUILD_TESTS` that compiles `Source/PluginProcessor.cpp`, `StrataVoice.cpp`, `FactoryPresets.cpp`, the `dsp/*.cpp` set and the tuning / note-expression module sources with `JUCE_WEB_BROWSER=0` and **no** `PluginEditor.cpp` — confirm the `O-Strata_UIResources` binary-data dependency, `JuceHeader.h` generation order (`add_dependencies` + `$<TARGET_PROPERTY:O-Strata,INCLUDE_DIRECTORIES>` as O-Bowed), the module list (`juce_dsp`, `juce_cryptography`, `juce_graphics` for PNG in Round B) and that the plugin target's own `JUCE_WEB_BROWSER=1` objects are not reused. Replace the smoke's `build.ninja` scrape.
2. **`FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod` in 8.0.14:** the returned `IIRPolyphaseAllpassStructure` (`directPath` / `delayedPath` arrays of `IIR::Coefficients`) — how to extract one allpass coefficient per second-order section into a fixed `std::array`, the section count for (0.06, −70) and (0.15, −60), and the group-delay-at-DC formula to assert latency ≤ 2 in `prepare`.
3. **Fast-maths bounds:** the valid input range of `FastMathApproximations::tanh` (Padé) and the exp2 choice for the log-domain Terrain Freq ramp (`std::exp2` ≈ 5 ns vs a polynomial `FastMathApproximations::exp`); the Squarcle's `1/tanh k` cache and the Butterfly's `std::exp` cost per partial.
4. **Shared-ramp design for the Phase 2.3 saving:** processor-owned per-block ramp buffers (22 × blockSize floats, filled once per `processBlock` from `SmoothedValue`s) indexed by every voice, vs. per-voice ramps kept and the saving taken elsewhere; measure the actual per-voice ramp cost first (the model says ≈ 12 ns per partial).
5. **Allocation counter:** a global `operator new` / `delete` override in the harness TU armed only around `processBlock` on the calling thread; confirm JUCE's own audio-thread code paths (`MidiBuffer`, `AudioBuffer` resize on first block, `SmoothedValue`) allocate nothing after `prepareToPlay`, and how to keep message-thread allocations (Round B's `callAsync`) out of the count.
6. **Exact-cycle spectral analysis in the plugin:** `alias.cpp` picks f0 = fs·k/N; find the parameter that pins the plugin's fundamental exactly (tuning reference frequency / root — `TuningEngine` API in scala-tuning-engine v3.0.1) so H1 / H6 can use the exact-cycle method; otherwise specify the Hann-windowed fallback and its −60 / −90 dB reachability.
7. **Native-function stubs vs. page strip:** which `getActiveOsc*` calls the stripped v1.24.0 page still makes (`Source/ui/public/index.html`), and whether Phase 2.1 keeps a stub returning `{}` (no bridge gap, page untouched per Stage 1 D1) or strips the JS call now; recommend the stub, removed in Phase 3.1.
8. **Phase seeding path:** `StrataVoice.cpp:254-291` (`resetWithPhase` / `resetWithRandomPhases`) and the `osc?Phase` semantics — where a harness seed enters (processor-level `std::atomic<uint32>` read at note-on, or a harness-only setter on the voice).
9. **`kMaxUnison` fan-out:** every array sized by `WavetableOscillator::kMaxUnison` in the voice / processor, so the 8 → 4 change is complete; the unison detune / pan / gain laws (`WavetableOscillator.cpp:104-131`) copied verbatim.
10. **Round B pre-research (single RESEARCH.md):** `juce::ThreadPool` + `juce::Timer` + `callAsync` inside a console harness (a `ScopedJuceInitialiser_GUI` and a `MessageManager::runDispatchLoopUntil` pump per gate), `ImageFileFormat::loadFrom(MemoryBlock)` for PNG bytes, the 64² node-matched box filter cost, an ASan configuration for the harness target only (`-fsanitize=address` on the console app, not the plugin), and the Chebyshev–Gauss quadrature node count M at F = 2.
11. **H2 preset loop:** `FactoryPresets::build` currently yields `Init` only (Stage 1 D2); H2's "every factory preset" loop is vacuous until Phase 4.1 — specify that H2 iterates the terrain × orbit grid now and re-runs over the bank in Stage 4 (memory `pattern_whitelist_gate_goes_vacuous_on_generic_receiver`).
12. **`.gitignore` scope:** `tests/exports/`, `tests/render-harness/build*/` and the harness binary via a `.gitignore` inside `plugins/O-Strata/tests/` (path-scoped), plus whether the root `.gitignore` already covers `*.o`-style outputs for the smoke binary.

## Requirements traceability

| ID | Round | Gate / evidence |
|---|---|---|
| FUNC-01 | A (2.1) | H1 θ parity ≤ −80 dB, C4 ± 1 cent |
| FUNC-02, FUNC-03 | A (2.1) | Mod X / Y / Orbit Mod centroid delta ≥ 5 % (Ellipse exempt); Orbit Size centroid monotone |
| FUNC-04 | A (2.2) | H3 grid finite / bounded / DC; fb = 0 `memcmp` |
| FUNC-05, QUAL-02 | A (2.1) | H5 22 destinations + 0 s negative control + 40 Hz sideband proof |
| FUNC-06, DSP-03 | A (2.3) + B (2.4) | Quality switch no-alloc / no-click; latency = distortion + 1; Bandlimited half in 2.4 |
| FUNC-09, FUNC-10 | A (2.1) | ID parity (param-dump) + Scala / 31-EDO fundamental ≤ 0.5 cent on ≥ 5 keys |
| DSP-01 | A (2.2) | H4 partial count ≤ 2 across C2 / C4 / C6; Track 0 at F = 8 control |
| DSP-02 | B (2.4) | H6 Bandlimited rows; swap storm; fit readout |
| DSP-04 | A (2.2) clamp half, B (2.5) edge half | H3 clamps; H11 edge continuity |
| DSP-05, PERF-01 | A (2.1, 2.3) + B (2.4) | grep gate; H8 zero allocations across terrain / orbit / Quality / Chebyshev swaps |
| DSP-06 | A (2.1) | H2 symmetry + negative control |
| DSP-07 | A (2.2) | Saturation 0 bit-identical; 1 raises partial count |
| PERF-02 | A (2.3) | H7 oscillator delta ≤ 12 % (D2) |
| QUAL-01 | A (2.3) 2× half, B (2.4) Bandlimited half | H6 ≤ −60 dB at 2×; ≤ −90 dB Bandlimited |
| FUNC-07 (DSP half), FUNC-08 (bytes) | B (2.5) | import API renders; Blur monotone; identical bytes ⇒ identical SHA |

## Next

1. `/clear`, then `/plugin-research O-Strata 2-dsp` — one RESEARCH.md for both rounds, items 1–12 above.
2. `/plugin-plan O-Strata 2-dsp` → Round A `PLAN.md` (Phases 2.1–2.3).
3. After Round A's verify: move `PLAN.md` / `SUMMARY.md` / `VERIFICATION.md` to `round-a/`, then `/plugin-plan O-Strata 2-dsp` for Round B (Phases 2.4–2.5).
