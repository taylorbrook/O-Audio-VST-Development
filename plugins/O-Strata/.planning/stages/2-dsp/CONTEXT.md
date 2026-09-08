# Stage 2 — DSP: CONTEXT

**Plugin:** O-Strata
**Stage:** 2 of 4 — DSP (five ROADMAP phases 2.1–2.5)
**Phase:** discuss ✓
**Date:** 2026-09-08
**Mode:** manual (interactive — three decisions put to Taylor, answers recorded below)
**Branch:** `main` (trunk-based; path-scoped commits under `plugins/O-Strata`)
**Inputs:** `BRIEF.md`, `REQUIREMENTS.md`, `research/ARCHITECTURE.md` (Core 1–9, Algorithm Details, Thread Boundaries, Decisions 1–7), `ROADMAP.md` Stage 2, `parameter-spec.md` v1, `stages/1-foundation/VERIFICATION.md` (carry-forward), `stages/1-foundation/SUMMARY.md` (notes for later stages)

---

## Goal

Replace the sine placeholder with the geometry bake pipeline: three generators (mesh slicer, volume orbit, terrain orbit), whole-table conditioning (alignment + normalisation), the inherited mipmap engine, and a debounced background scheduler that publishes tables to the unchanged O-Prism voice through `oscTablePtr[osc]`. Every gate runs in offline harnesses. Zero new audio-thread code.

**Requirements verified in this stage:** FUNC-01, FUNC-02, FUNC-03, FUNC-04, FUNC-05, FUNC-09, FUNC-10, FUNC-12, DSP-01..07, PERF-01, PERF-02, QUAL-01, QUAL-02 (+ FUNC-06 timing and robustness preconditions, ahead of the Stage 3 import UI).

## Starting point (Stage 1 verified, 2026-09-07)

- `oscTablePtr[2]` is initialised once in the constructor to `placeholderTable` (1-frame procedural sine). The reaper is live and idle: `retireTable()`, `retiredTables`, `blockGeneration`, `updateWavetableAssignments()` at the top of `processBlock` (`PluginProcessor.h:176-302`).
- `getActiveOscInfo` hard-codes `"Sine"` / `factoryIndex 0`; `getActiveOscFrame` reads the published table (`PluginEditor.cpp:492-506`). The `evaluateJavascript` held-notes push (`PluginEditor.cpp:817`) stays until Stage 3 (Decision 7).
- No `Source/geometry/` directory exists (Stage 1 D2). The CMake snippet's hunk 2 (`Source/geometry/*.cpp`) is applied in Phase 2.1.
- `createEditor()` is guarded by `#if JUCE_WEB_BROWSER … #else GenericAudioProcessorEditor`, so a headless processor link works (`stages/1-foundation/smoke/build.sh` proves it by relinking the param-dump objects).
- `juce::juce_cryptography` is already linked. `OUARICON_BUILD_TESTS` builds only the param-dump target today.
- `stereoWidth` unbound (as O-Prism); every factory preset is re-authored in 4.1 — neither is Stage 2 work.

## Inherited contracts — NOT re-opened

Settled by Stage 0 and locked in `research/ARCHITECTURE.md`. Stage 2 implements them verbatim; the plan cites section names, it does not restate the maths.

| Contract | Source | Value |
|---|---|---|
| Pipeline order | ARCHITECTURE "Processing Order Requirements → Bake" | resolve source → rotate/slab-index or pre-blur → N frames sequential (`shouldExit()` per frame) → DC → global peak → Shape Drive tanh((1+15d)·x) → DC → chained xcorr alignment + polarity → global peak → `WavetableData::allocate(N)` → `generateMipmaps()` → `setGuardSamples()` → `callAsync` publish |
| Normalisation | DSP-05, Core 6 | per-frame DC removal, global peak = 1.0 across all frames; per-frame peak normalisation must not exist anywhere in the code |
| Alignment | DSP-04, Algo "Frame alignment" | `juce::dsp::FFT` order 11 circular cross-correlation, chained k−1 → k, polarity flip when C[m] < 0; frame 0 rotated to its max-positive-slope zero crossing |
| Mesh slicer | DSP-02/03, Algo "Mesh slice" notes 1–8 | R = Rx(tiltX)·Ry(tiltY) in double; h_k half-step; ε-nudge 1e-7·bbox height; edge-key (min,max) chaining; open chains closed + `warnOpenLoop`; CCW orientation; area-weighted centroid; Largest with hysteresis (nearer to c_prev AND area ≥ 0.5×largest); Sum = separate unwrap then sum; arc-length resample to 2048; XY = cos φ·x + sin φ·y; Centroid Distance behind the 0.02 flatness floor with RMS-matched XY fallback; empty slice = repeat previous, leading empties back-filled, all-empty = sine + `warnDegenerate`. No radial unwrap, no concatenation |
| Built-in meshes | Decision 4, Core 1 | procedural C++: sphere 200×100 (40 000 tris), torus, twisted star `(nu=128, nz=64, points=5, inner=0.45, outer=1.0, twist=π, height=2.0)`, crescent, torus knot (2,3) tube, fBm blob (UV 200×100 + 3-octave fBm). Welded, normalised to longest bbox axis 2.0. No binary-data target |
| Golden | Algo "Mesh slice" note 8 | twisted star, 128 frames, Centroid Distance, Largest, tilt 0 — match `mesh_slice_wavetable.py`'s `twisted_star_d_128` within 1e-3 RMS per frame |
| Volume | Core 4, Algo "Volume orbit" | torus knot (2,3)/(3,5)/(5,7) on R=0.6 r=0.3; Lissajous (3,4,7; 0.7, 0.2, 0)×0.8; sweep axes Orbit Scale / Knot Phase / Z Offset with the stated formulas; SDF torus/box/sphere, gyroid, fBm (custom Perlin-style, constant seed, lacunarity 2, gain 0.5, 1+round(7·d) octaves); v = tanh(4·f) |
| Terrain | Core 5, Algo "Terrain orbit" | Ellipse / Epitrochoid 3·5·7 / Hypocycloid 3·5·7 / Superellipse n=4, unit max radius; aspect → rotate → scale → translate; radius clamp so bbox ⊂ [−0.999, 0.999]² (never the points; `jassert` on samples); sweep axes Radius / Rotation / Centre X / Centre Y; PNG luminance → [−1,1], separable Gaussian σ = blur·W/32 px cached per (image, blur), bilinear, Mirror (even extension) / Window (raised cosine) |
| Analytic terrains | Algo "Terrain orbit" | Sine Product, Radial Rings, Mitsuhashi, Saddle, Ridged Cosines — clean-room formulas as written; nothing from *Terrain*'s sources |
| Scheduler | Core 8, Algo "Bake scheduling" | message-thread `juce::Timer` 50 ms; `BakeKey` = FNV-1a over the **active** family's params + GeoSource + GeoFrames + GeoDrive + importRevision (+ blob SHA when Imported); 150 ms quiet → submit; `juce::ThreadPool` 2 threads named "O-Strata Bake"; one in-flight job per osc; `signalJobShouldExit()` on supersede; result dropped on key mismatch; `callAsync` with a `std::weak_ptr` token; publish = `oscTablePtr[osc].store(new, release)` + `retireTable(old)` + `bakeGeneration[osc]++` under `JUCE_ASSERT_MESSAGE_THREAD`; `bakeProgress[osc]` atomic float; destructor `removeAllJobs(true, 4000)` first; `prepareToPlay` never bakes or publishes. **Amended by D3 below (drag throttle).** |
| No APVTS listener | Decision 2 | bake params read by `getRawParameterValue()->load()` in the timer only; never a listener, never mod-matrix destinations |
| Thread boundaries | ARCHITECTURE "Thread Boundaries" | audio thread: unchanged O-Prism; message thread: timer, publish, reaper, import lock; bake threads: parse/generate/condition/mipmap/view caches — never APVTS writes, `oscTablePtr`, `retireTable`, WebView |
| Parser thread assert | Decision 6, FUNC-06 | `jassert (isThisTheMessageThread() || Thread::getCurrentThreadName().startsWith ("O-Strata Bake"))` at every parser and generator entry |
| Failure → placeholder | Core 8, Algo "Frame alignment" | zero peak → sine + `warnSilent`; any non-finite sample → abort to sine + `warnNaN`; zero/degenerate mesh → sine + `warnDegenerate`; a bake never publishes silence |
| Determinism | Core 6 | single thread per table, no parallel-for inside a table, no fast-math-dependent reductions; identical params → identical bytes on the same machine |
| Harness split | ROADMAP Stage 2 preamble | `O-Strata_BakeHarness` (generator sources only, no processor) and `O-Strata_RenderHarness` (headless processor, `JUCE_WEB_BROWSER=0`, pumps the message loop) under `plugins/O-Strata/tests/`, gated by `OUARICON_BUILD_TESTS` |
| PERF-02 measurement | ARCHITECTURE Notes wording | ≤ 100 ms = generator + conditioning at 256 frames on the 40 k-tri fBm blob; mipmap stage (~90 ms, inherited) reported alongside, target ≤ 200 ms total |
| Frame Count | FUNC-12, Core 2 | N ∈ {64, 128, 256}; `FrameSet` = N × 2048 floats + per-frame metadata; guard sample added only in `WavetableData` |

**Read-only in this stage:** `BRIEF.md`, `REQUIREMENTS.md` (status column updates only, at verify), `plugins/O-Prism/**`, the oscillator read path (`WavetableOscillator.*`, `StrataVoice.*` — DSP-01 requires a clean diff against O-Prism v1.24.0 except table-source plumbing). `parameter-spec.md` is edited exactly once, by D2 below.

## Decisions taken this phase

### D1 — Per-phase plan → execute → verify loops inside one stage cycle

Taylor: per-phase plan/execute/verify (recommended option).

This CONTEXT and the next RESEARCH.md cover the whole stage. From there the cycle loops **once per ROADMAP phase**: `/plugin-plan O-Strata 2-dsp` produces `PLAN.md` for the current phase only; execute lands that phase's commit; verify gates it; STATUS.md advances `phase_current` (2.1 → 2.2 → 2.3 → 2.4 → 2.5). Stage 2 verify is the 2.5 verify, which also re-checks every earlier phase's gate in one run.

**Consequences:**
- Five commits, one per phase, in ROADMAP order: 2.1 pipeline → 2.2 slicer + golden → 2.3 volume → 2.4 terrain → 2.5 gates. 2.3 and 2.4 may swap if 2.2 blocks (ROADMAP "Critical Path").
- Every execute stays inside the 600 s executor watchdog (memory `pattern_executor_watchdog_stall_run_long_commands_in_background`): full rebuilds and harness runs go to the background with logs under the phase directory.
- Plan files are `stages/2-dsp/PLAN-2.1.md` … `PLAN-2.5.md`; summaries `SUMMARY-2.N.md`; one `VERIFICATION.md` at the end with a per-phase section, plus a short `VERIFY-2.N.md` per phase so a phase verdict is recorded before the next plan is written.
- A phase's verify may not pass on a gate the ROADMAP assigns to a later phase (e.g. 2.1 does not fail because QUAL-01 is unmeasured); it fails only on its own criteria.

### D2 — Terrain choice list grows to six in Phase 2.4; `parameter-spec.md` → v1.1

Taylor: insert the three analytic terrains (recommended option).

`parameter-spec.md` v1 carries the mockup's 3-entry Terrain list (`Sine Product / Radial Rings / Imported…`). FUNC-05 requires ≥ 4 analytic terrains; ARCHITECTURE and ROADMAP 2.4 specify five. Phase 2.4 inserts **Mitsuhashi / Saddle / Ridged Cosines** between `Radial Rings` and `Imported…`, giving `Sine Product / Radial Rings / Mitsuhashi / Saddle / Ridged Cosines / Imported…` (6 entries; `Imported…` stays last, index 5).

**Consequences:**
- `parameter-spec.md` bumps to **v1.1** in Phase 2.4 with the reconciliation note the spec itself already prescribes (line 27); `params.tsv` is re-dumped (219 rows, only `oscATerrain` / `oscBTerrain` choice strings change; default `Sine Product` stays index 0 = 0.000000). STATUS.md `contract_checksums.parameter_spec` is re-recorded.
- Legal now because no preset has shipped; after v1.0.0 the list is append-only (spec line 39).
- The mockup YAML/HTML (`mockups/v1-ui.yaml`, `v1-ui.html`) list 3 entries. Stage 3.1 binds the dropdown from the spec, not the mockup — record this in the Stage 3.1 CONTEXT; do not edit the mockup in Stage 2.
- Blur / Edge stay inert unless `Terrain = Imported…` (BakeKey excludes them for analytic terrains — ROADMAP 2.4 criterion).
- The v1-integration-checklist's 3-entry list is superseded; the checklist is not a contract.

### D3 — Scheduler gains a drag throttle (ARCHITECTURE Core 8 amendment)

Taylor: add a drag throttle (recommended option).

Core 8 specifies a pure trailing debounce (150 ms quiet time). With only that, a knob drag re-bakes nothing until the gesture ends and the player hears the old table for the whole drag, then a jump. Amendment:

> **Throttle rule:** if an oscillator's `BakeKey` has been changing continuously (no 150 ms quiet window) for ≥ 250 ms since the last *submission* for that oscillator, and no job for that oscillator is in flight, submit a bake with the current key. The trailing rule is unchanged: a key stable for ≥ 150 ms that differs from `lastBakedKey` submits. Superseded in-flight jobs are cancelled by `signalJobShouldExit()` as before; a result whose key ≠ `lastRequestedKey[osc]` is dropped at publish.

**Consequences:**
- The table follows the knob at roughly 4–8 Hz at Frame Count 64/128 and ~3 Hz at 256 (mipmap stage ≈ 90 ms). Nothing changes on the audio thread.
- ROADMAP 2.1 gates still hold as written: "50 key changes in 100 ms → exactly one publish" (100 ms < 250 ms, so no throttle submission occurs); "new table within 150–250 ms of the last change" (trailing rule).
- New 2.1 gate: a key changing every 20 ms for 2 s produces ≥ 4 and ≤ 9 published tables, the last of which carries the final key; zero leaks.
- The throttle interval is a `static constexpr` in the scheduler (`kDragThrottleMs = 250`), not a parameter.
- ARCHITECTURE.md Core 8 and "Bake scheduling" get a one-paragraph amendment in Phase 2.1 (the contract is otherwise immutable; the amendment cites this decision and STATUS's `contract_checksums.architecture` is re-recorded).

### D4 — Harnesses become CMake targets; the Stage 1 relink trick retires (taken by the orchestrator)

`tests/bake-harness/CMakeLists.txt` and `tests/render-harness/CMakeLists.txt`, both `juce_add_console_app`, wired from the plugin's `CMakeLists.txt` under the existing `if(OUARICON_BUILD_TESTS)` block, following the repo pattern (`plugins/O-simpleFM/tests/render-harness/CMakeLists.txt`: reach into `../../Source`, `add_dependencies` on the plugin for `JuceHeader.h`, borrow `INCLUDE_DIRECTORIES`, define the `JucePlugin_*` macros, `JucePlugin_VersionString` tracks the plugin version). The render harness compiles with `JUCE_WEB_BROWSER=0` so the `GenericAudioProcessorEditor` branch links and no `O-Strata_UIResources` dependency exists (memory `pattern_render_harness_breaks_on_webview_editor`). Each harness exits non-zero on any failed check and prints one `PASS`/`FAIL` line per gate so a verify run can grep it. `stages/1-foundation/smoke/main.cpp` is the seed for the render harness's MIDI/render/state helpers.

### D5 — Golden and fixtures are generated, not committed (taken by the orchestrator)

- `twisted_star_d_128.wav` is produced by running `research/wavetable-synthesis-3d-geometry-prototypes/mesh-slice/mesh_slice_wavetable.py` (numpy 2.4.0 present; the script writes to its `OUT` directory). The harness reads it from a path given on the command line; `tests/goldens/twisted_star_d_128.sha256` is committed so a regeneration is checked before it is trusted (memory `pattern_golden_tracked_as_checksum_only`). The Python script is the reference; if the C++ disagrees, the C++ is wrong until proven otherwise on the maths.
- Ugly-mesh corpus (open cylinder, two disjoint shells, flipped winding, sliver triangles, zero-area) and the 1 M-triangle UV sphere OBJ are **generated by the bake harness at run time** into a temp directory — no OBJ bytes in git.
- `tests/exports/*.wav` (level-0 frames for the Stage 4 listening pass) are git-ignored (root `.gitignore` already ignores `/*.wav` only at the root; add `plugins/O-Strata/tests/exports/` in 2.5).

### D6 — Manual audition path during Stage 2 (no UI work)

The Geometry panel arrives in Phase 3.1. During Stage 2 the 48 geometry parameters are driven from the host: Logic's AU **Controls** view lists every parameter as a generic slider, and automation lanes work in any host. That is the by-ear check for each phase's verify; the Standalone shows no geometry controls until 3.1 (the WebView has no generic fallback). No throwaway dev UI is added — it would trip `check-ui-labels` / i18n gates for nothing.

## Constraints carried into implementation

- **No audio-thread code.** `git diff --stat v1.24.0-O-Prism-equivalent -- WavetableOscillator.* StrataVoice.*` must show only table-source plumbing (DSP-01). If a change there looks necessary, stop and re-plan.
- **Publish only on the message thread** (`JUCE_ASSERT_MESSAGE_THREAD`); `oscTablePtr` has exactly one writer. The audio thread never sees a `GeometrySource`, a `FrameSet`, a lock or an allocation (PERF-01). Debug-allocator hook: a global `operator new` counter armed around `processBlock` in the render harness — any increment is a FAIL.
- **Leak check on macOS:** LeakSanitizer is unavailable on arm64 macOS; use `leaks --atExit -- <harness>` plus a live-instance counter on `WavetableData` (constructor/destructor atomic) asserted back to the baseline after the 50-changes storm and the D3 drag storm. ASan (`-fsanitize=address`, separate `build-asan/` configured on demand) for the storm and cancellation tests.
- **Message-loop pumping in the render harness:** the scheduler timer and `callAsync` need `MessageManager::runDispatchLoopUntil()` between render blocks (`JUCE_MODAL_LOOPS_PERMITTED=1` on the harness target only). A harness that forgets to pump measures the placeholder and passes vacuously — every bake-dependent check first asserts `bakeGeneration[osc]` advanced (memory `pattern_zipper_sweep_probe_needs_liveness_gate`).
- **Negative controls are mandatory** for DSP-04 (`alignmentEnabled=false` raises torus adjacent RMS to ≥ 0.4), for the Sum/Largest policy, and for the inactive-family BakeKey (a Terrain knob on a Mesh oscillator produces no bake). A probe that passes both ways is decoration (memory `pattern_probe_must_target_the_branch_the_fix_changed`).
- **Harness fixtures must not mirror plugin constants** by copy: read `WavetableData::kTableSize`, `kMaxFrames`, and the library parametrisation from the plugin headers (memory `pattern_test_fixture_mirrors_drift_silently`).
- **Determinism across machines is not claimed:** `tanh`/`sin` differ across libm; SHA-256 equality (2.3 criterion, FUNC-08 later) is same-machine, same-build. Record the machine in the summary.
- **FUNC-10 pitch check:** O-Strata *is* the O-Prism engine, so "equals O-Prism's pitch" is verified as: measured fundamental of the rendered note (FFT peak at 48 kHz, parabolic interpolation) equals `TuningEngine` frequency for that key under a `.scl` and under a 31-EDO generator, within 0.5 cent, on ≥ 5 keys. No O-Prism build is required.
- **Offline bounce caveat:** publish rides `callAsync`; hosts that do not pump the message thread during offline render would keep the previous table (cf. memory `pattern_offline_render_asyncupdater_dynamics_gap`). Logic bounces on the main thread. Record as a Stage 4.2 DAW-check item, not a Stage 2 gate.
- **Choice params ≥ 2 entries** (memory `critical_choice_param_needs_two_choices`) — D2's list has 6.
- **Version:** stays `VERSION 1.0.0` throughout Stage 2 (unreleased). No tags (memory `feedback_never_tag_unless_publish`).
- **Commit discipline:** `git branch --show-current` + `git status --short` immediately before each `git commit -- plugins/O-Strata PLUGINS.md`; new files `git add`ed first (memory `pattern_git_commit_pathspec_takes_only_tracked_files`). PLUGINS.md row → 🚧 Stage 2 at the 2.1 commit.
- **Background the long runs:** full ninja rebuilds, ASan builds, the 1 M-triangle timing and the 2.5 sweep (16 entries × 3 notes) run with `run_in_background` and logs in `stages/2-dsp/logs/` (ignored by `*.log`; kept as evidence, as Stage 1 did with `smoke-output.log`).

## Stage 2 test criteria (as amended by D1–D6)

ROADMAP 2.1–2.5 criteria apply verbatim, plus:

- [ ] **D3:** key changing every 20 ms for 2 s → 4 ≤ published tables ≤ 9, final table carries the final key, live `WavetableData` count returns to baseline (2.1)
- [ ] **D3:** 50 key changes in 100 ms → exactly one publish (unchanged ROADMAP gate, re-asserted after the throttle lands) (2.1)
- [ ] **Liveness:** every bake-dependent render check asserts `bakeGeneration[osc]` advanced before measuring (2.1, 2.5)
- [ ] **Inactive family:** Terrain and Volume knobs on a Mesh oscillator → BakeKey unchanged, no job submitted (2.1)
- [ ] **D2:** `oscATerrain` / `oscBTerrain` list = 6 entries, `Imported…` last; param-dump 219 rows; `parameter-spec.md` v1.1; checksums re-recorded (2.4)
- [ ] **Allocation gate:** zero `operator new` calls inside `processBlock` during continuous bakes at 16 voices (2.5)
- [ ] **Leak gate:** `leaks --atExit` clean on the bake harness storm run; instance counter at baseline (2.1, 2.5)
- [ ] **DSP-01 diff gate:** `WavetableOscillator.*`, `StrataVoice.*` differ from O-Prism v1.24.0 only in the rename and table-source plumbing (2.5)
- [ ] **FUNC-10:** rendered fundamental = `TuningEngine` frequency within 0.5 cent on ≥ 5 keys under `just-major.scl` and 31-EDO (2.1)

## Open for the research phase

1. **`juce::ThreadPool` API surface in 8.0.14:** `ThreadPoolOptions::withThreadName` / thread count / priority; whether `removeAllJobs(true, 4000)` interrupts a job blocked inside `generateMipmaps` (it cannot — `shouldExit()` is only polled per frame; confirm the 4 s budget covers a worst-case 1 M-triangle bake or the destructor must wait longer).
2. **`generateMipmaps` reentrancy and cost:** confirm it is a pure function of the `WavetableData` (no static FFT state) so two bake threads can run it concurrently; measure its wall time at N = 64/128/256 to fix the throttle's realistic rate (D3) and the PERF-02 split.
3. **FFT cross-correlation via `juce::dsp::FFT`:** exact real-only forward/inverse layout (interleaved complex in a 2N float buffer), scaling of the inverse, and how to compute conj-multiply on the packed format — prototype in the bake harness before the aligner is written.
4. **Golden regeneration:** run `mesh_slice_wavetable.py`, record where it writes `OUT`, its runtime, the WAV format (bit depth, frame layout), and the SHA-256 to commit; confirm the Python twisted star's `nu/nz/inner/outer/twist/height` and its arc-length resampling match ARCHITECTURE note 6 exactly (any divergence is a golden-definition question, not a C++ bug).
5. **Render-harness pattern for a WebView plugin:** which `JucePlugin_*` macros the O-Strata processor needs at compile time, whether `PluginEditor.cpp` must be compiled with `JUCE_WEB_BROWSER=0` or excluded, and how the Stage 1 `smoke/main.cpp` helpers port (MIDI drive, block render, state round-trip, RMS).
6. **Debug-allocator hook:** portable way to count allocations on the audio thread from a console harness (global `operator new`/`delete` override with a thread-local arm flag vs `malloc` interposition on macOS) — pick one and prove it catches a deliberate allocation.
7. **FNV-1a BakeKey inputs:** hash the normalised float bit patterns, not display values; confirm `getRawParameterValue` returns the denormalised value for Choice params (index) so the key is stable across hosts.
8. **PNG decode path** (`juce::ImageFileFormat::loadFrom` on a `MemoryInputStream`, `Image::BitmapData` read-only) needs `juce_graphics` — already linked; confirm it works with `JUCE_WEB_BROWSER=0` in the bake harness and whether 16-bit greyscale PNGs decode to 8-bit (acceptable loss? note for 2.4).
9. **Spectral measurement helpers** shared by the harnesses: 8192-point Blackman-Harris FFT, harmonic-bin masking for QUAL-01, spectral centroid, THD, adjacent-frame RMS, partial count above −40 dB — one header under `tests/common/`.
10. **1 M-triangle synthetic OBJ:** generation time and file size of a UV sphere at that density (~50–100 MB text); whether to generate binary STL instead for the timing gate (FUNC-06 says OBJ; keep OBJ for the parse budget, STL as a second data point).

## Requirements traceability

| ID | Phase | Evidence |
|---|---|---|
| FUNC-01, FUNC-12 | 2.1 | scheduler gates, Position sweep at N ∈ {64,128,256} |
| FUNC-09, FUNC-10 | 2.1 | `params.tsv` diff = −2 +48; pitch check under `.scl` and 31-EDO |
| DSP-04, DSP-05, DSP-07 | 2.1 | alignment negative control, global-peak/DC checks, drive bypass identity |
| PERF-01, PERF-02 (scheduling) | 2.1, 2.5 | message-thread-only publish assert, storm + throttle gates, allocation + leak gates |
| FUNC-02, DSP-02, DSP-03 | 2.2 | golden 1e-3 RMS, torus hysteresis, sphere THD, tilt/φ distinctness, ugly-mesh corpus |
| FUNC-03 | 2.3 | knot/torus-SDF adjacent RMS, fBm detail partial count, determinism |
| FUNC-04, DSP-06 | 2.4 | monotonic centroid, orbit clamp, PNG Mirror/Window edge gates |
| FUNC-05 | 2.2 + 2.4 | 6 meshes + 5 analytic terrains bake clean at 256 frames (D2) |
| DSP-01, QUAL-01, QUAL-02, PERF-02 (budget) | 2.5 | non-harmonic ≤ −90 dB at C6, no step > 0.1, adjacent RMS ≤ 0.05 library-wide, fBm blob timing, 1 M-tri timing, oscillator diff gate |
