# Stage 4 — Polish / Validation (persistence, factory presets, CI Windows, listening pass, release), Round B: PLAN

**Plugin:** O-Strata · **Stage:** 4 of 4 (Polish / Validation) · **Round:** B = ROADMAP Phase 4.2 (CONTEXT D1) — **this round carries the stage verdict** · **Phase:** plan
**Date:** 2026-09-13 · **Mode:** manual · **Branch:** `main` (trunk-based; path-scoped commits under `plugins/O-Strata` + `PLUGINS.md`; one separate commit under `.github/workflows/ci-tests.yml` — Decision 27; one `git push origin main` + one `gh workflow run` — Decision 34)
**Inputs:** `stages/4-polish/CONTEXT.md` (D1–D4, Finding 6 ceiling, Findings 7–10, requirements map row B), `stages/4-polish/RESEARCH.md` (§2.6–2.10 Round B items, §3 C9–C15, §4 decisions 12–17, §5.3 Round B run list, §5.4 defaults-change commands), `stages/4-polish/round-a/VERIFICATION.md` (Issue 1 S&H LFO seed, Issues 2–4 carried, §Human rows 1–6), `stages/3-gui/VERIFICATION.md` §Human rows 1–6, `ROADMAP.md` v2 Phase 4.2, `REQUIREMENTS.md` v2.0.0 (COMPAT-02, QUAL-04, PERF-02, UI-01 / FUNC-07 / PERF-03, COMPAT-01), `research/ARCHITECTURE.md` (Core 6, Decisions 5 / 6 / 8 / 10 / 11 / 25, §Licence), `Source/` at `ab3994e4` (code unchanged since the Round A verdict at `29803cb5`), `.github/workflows/ci-tests.yml` (O-Octagon's two jobs, `workflow_dispatch` with no inputs), the research bench (`cheb_bench.cpp` + `bench_O3_final.txt`, copied into this session's scratchpad; the (e) form is Appendix A of this plan so the executor needs neither).
**Baseline commit:** `ab3994e4` (Round A verify ✓; code = `29803cb5`: `--gate all` 159 / 159 in 111.4 s, H2 20 / 20, H10 24 / 24, labels 1809 / 0 over `default + 42`, layout 607 / 0, tip gate 3215 at 122, 34 natives, lints 0 with 53 `'mt'`, orbit golden 44 / 44, pluginval ×2 + auval, installed `-dev` bundles byte-identical to the build).
**Line numbers** are not quoted as edit sites; every site is named by a token the executor re-locates by grep (memory `pattern_deferred_item_line_number_drifts_record_a_token`).
**Decision numbering:** Round A was Decisions 1–26; this plan is **Decisions 27–45**.

---

## Goal

Make the v1.0.0 tree releasable and install it. **PERF (D4):** replace the audio-thread 2-D Clenshaw with the portable padded basis evaluator the research benched (17 × 20 coefficient copy, per-row 4-lane dot products, 2.9× / 3.5× faster, bit-identical to the NEON form) so the H7 Bandlimited delta comes down from ≈ 12 % to ≈ 5 %, gated by a new accuracy bound against `clenshaw2D`, the H6 Bandlimited rows held inside ± 0.5 dB, H1 / H9 unchanged, and the H7 Bandlimited row promoted from "reported" to a check. **Determinism:** the S&H LFO draws from the harness phase seed when one is set, so the last non-reproducible H2 preset row (Squarcle Storm) becomes bit-stable; production is untouched. **CI (COMPAT-02):** `ci-tests.yml` gains a `plugin` dispatch input; the macOS job builds `O-Strata-render-test`, runs `--gate all` and the orbit golden, the Windows job builds `O-Strata_VST3` under MSVC + WebView2 and runs pluginval strictness 10 — dispatched by hand once, green, URL + `headSha` recorded. **Listening (QUAL-04):** `stages/4-polish/LISTENING.md` with Tables A / B / C ready for Taylor's sitting, the 198-WAV grid re-verified against an LF golden, and 18 new preset renders. **Release-ready:** CHANGELOG v1.0.0 in the O-Emulator shape (Added / Behaviour to know / Known limits / Technical notes / Stage history), NOTES.md rewritten (attribution + licence), PLUGINS.md → 📦 Installed, `build-and-install.sh` with the dual-variant sweep, pluginval ×2 + auval on the installed bundles. **No tag, no `/publish`** (CONTEXT D3).

The one outward-facing action in this plan is **Decision 34**: the CI commit must be pushed to `origin/main` before `gh workflow run` can see the new input, and that push carries the five commits already ahead of origin. Taylor can veto it before `/plugin-execute`; without the push COMPAT-02 stays pending and everything else in the plan still lands.

---

## Decisions resolved for the executor

These close RESEARCH §4 Round B decisions 12–17, the Round A verify's Issue 1, and every ambiguity found while planning. The executor applies them without asking.

| # | Question | Decision |
|---|---|---|
| 27 | Round boundary, commits, artifact location | Round B = ROADMAP Phase 4.2 in one execute session, **three commits**: **4.2a** after Wave 2 — `perf(O-Strata): Phase 4.2a — padded Chebyshev evaluator (D4), S&H LFO under the harness seed, clenshaw / H7 / exportPresets rows` scoped `plugins/O-Strata`; **ci** after Wave 3's edit — `ci: ci-tests.yml plugin dispatch input — O-Strata macOS harness + Windows VST3 / pluginval-10 jobs` scoped `.github/workflows/ci-tests.yml` only; **release** at close — the ROADMAP message `release(O-Strata): v1.0.0 — CI Windows, listening pass, changelog` scoped `plugins/O-Strata PLUGINS.md` (docs, LISTENING.md, SUMMARY, STATUS). A fix forced by the Windows run lands as **4.2b** (`fix(O-Strata): MSVC — …`, `plugins/O-Strata`) before the release commit. `git branch --show-current` + `git status --short` immediately before each commit; never `-a` / `-A`; the modified `.claude/agent-memory/research-planning-agent.md` in the tree belongs to another session — never stage it. Round B artifacts stay at `stages/4-polish/` (PLAN, SUMMARY, VERIFICATION, **LISTENING.md**) as the stage-level set — RESEARCH's `round-b/LISTENING.md` name is superseded; Round A's live in `round-a/`. `Claude-Session:` trailer on every commit. |
| 28 | Evaluator (D4; RESEARCH 13, C12) | **Form (e), portable padded**, Appendix A verbatim: `dsp/ChebyshevSet.h` gains `constexpr int kChebPadRow = 20;`, `constexpr int kChebPadded = (kChebDegree + 1) * kChebPadRow;` (= 340), `inline void chebBasis17 (float x, float* T) noexcept` (three-term recurrence, T[0..16]) and `inline float chebEvalPadded (const float* cp, float x, float y) noexcept` beside `clenshaw2D` (token `inline float clenshaw2D`), with a header comment stating the layout (`cp[n * kChebPadRow + m]`, pad lanes zero) and that `clenshaw2D` remains the reference for the unpadded triangle. `dsp/TerrainOscillator.h`: `float chebW[2][kChebCoeffs]` (token) → `alignas (16) float chebW[2][kChebPadded]`, comment updated. `dsp/TerrainOscillator.cpp` `buildChebWeights` (token `dst[row + m] = src[row + m] * chebTaperWeight`) writes the padded layout with **fixed trip counts**: for every `n` in 0..16, `m` in 0..19: `dst[n * kChebPadRow + m] = (m + n <= kChebDegree) ? src[chebRowStart (n) + m] * chebTaperWeight (n + m, dc) : 0.0f` — the condition is on loop indices, not data (DSP-05 branch-free-on-data holds; block-rate anyway). The scan site (token `y = clenshaw2D (chebW[shadow ? shadowIdx : chebCur], px, py);`) → `chebEvalPadded (…)`; the shadow-crossfade path reads the same padded copy by index — nothing else changes. `clenshaw2D` **stays** for the off-thread consumers (`ChebyshevProjector.cpp`, `TerrainScheduler.cpp` readout, `TerrainViewFeed.cpp`) which read the unpadded `set->c` — no processor switch, no other call site touched. Index arithmetic in `int` (`-Wconversion` family is on the plugin TU). NEON intrinsics **only** if Decision 31's H7 row misses on this Mac, and then behind `#if JUCE_ARM` with (e) as the other branch. |
| 29 | Accuracy contract for the evaluator (RESEARCH 13) | `--gate clenshaw` becomes four lines, three of them checks: (1) `[clenshaw] clenshaw2D %.1f ns / chebEvalPadded %.1f ns per dependency-carried evaluation (reported)` — the padded figure on `padTriangle` of the same random set (uniform ± 0.5, seed `0xC1E45AA7`), same `x = 0.5 v` / `y = −0.5 v + 0.1` chain; `check (finite)` as today; (2) `check (maxΔ ≤ 2e-5)` — `max |chebEvalPadded − clenshaw2D|` over 100 000 uniform points in [−1, 1]² on that random set (bench: 1.19e-5, dominated by Clenshaw's own float error); (3) `check (maxΔ ≤ 1e-6)` on a **projected** set — `ChebyshevProjector::projectAnalytic (TerrainKind::SineProduct, 1.0f, 0.5f, 0.5f, set, nullptr)` (the scheduler's own call), same 100 k points (bench: ≤ 4.6e-7 on every projected set); (4) `max |chebEvalPadded − double reference| %.2e (reported)` with the 17-term double recurrence from the bench. **Speed is reported, never gated in `clenshaw`** — the ratio verdict lives in H7 (Decision 31), where both sides are measured the same way (memory `pattern_wallclock_inside_a_stability_verdict`). README `clenshaw` row rewritten. |
| 30 | H6 / H1 / H9 regression contract (RESEARCH 13, C12) | The permanent harness checks stay as they are (`≤ −90 dB` on sounding rows, `h_max ≤ D_max·K`, identity `≤ −80 dB`, H9 `≤ 1e-5`). The **± 0.5 dB band is an executor comparison**, not a new golden: before Task 1's edit, run `$H --gate H6 > $SCRATCH/h6-before.txt` on the baseline binary; after, `h6-after.txt`; a 20-line script pairs the 144 `[H6 BL]` row figures (nonharm/max dB and the `h_max` integer) by row label and asserts **max |Δ| ≤ 0.5 dB, `h_max` identical on every row, the 17-row muted set identical, the 13-row equality set identical**, and the 2× / 4× / 1× figures **bit-identical** (those paths are untouched); the worst |Δ| and its row go into SUMMARY. H1 Chebyshev identity: the identity set is bit-exact on every evaluator (bench), so the row's figure is expected **identical** (−128.3 dB); a move of more than 0.1 dB is a defect, not noise. H9 Bandlimited row: `max|d|` expected `0.000e+00` as today. |
| 31 | H7 promotion + machine line (RESEARCH 13) | In `gateH7`, the Bandlimited row (token `Decision 25 fallback layout only if > 12`) becomes `check (tq1 − bq1 <= (total − base) + 2.0, fmt ("[H7] Bandlimited delta %.2f %% <= 2x delta %.2f %% + 2.0 (D4 padded evaluator; was ~11.9 %% with Clenshaw)", …))` — the 2× delta of the same run, not a literal (both sides move together on a slower machine, which is what makes the row runner-safe). Expected ≈ 4.8 % vs 2× ≈ 4.2–4.4 %. The `machine:` line (token `"  machine: Apple M4 Max (Release build)\n"`) prints `juce::SystemStats::getCpuModel()` so the runner's log names its CPU. README H7 row updated; the "Decision 25 fallback layout" note retired (the fallback was never needed). |
| 32 | S&H LFO under the harness seed (Round A verify Issue 1; memory `pattern_sh_lfo_clock_seeded_random_breaks_harness_determinism`) | `dsp/LFO.h` gains `void seed (juce::int64 s) { random.setSeed (s); }` (one line, beside `setPhase`); `LFO.cpp` untouched. `StrataVoice.cpp` at the note-on site, **immediately before** the four `lfoN.reset()` lines (token `pLfo1FreeRun == nullptr || pLfo1FreeRun->load() < 0.5f`): `if (harnessSeed != 0u) { lfo1.seed (seedFor (2)); lfo2.seed (seedFor (3)); lfo3.seed (seedFor (4)); lfo4.seed (seedFor (5)); }` — `harnessSeed` / `seedFor` are the existing locals of that function (token `const uint32_t harnessSeed =`); if they are scoped after the reset lines, hoist the two declarations, do not duplicate them. **Production (`harnessSeed == 0`) is untouched**: the `LFO` keeps its default-constructed clock-seeded `juce::Random`; O-Prism's `LFO.h` is not edited (`Source/dsp/LFO.h` is O-Strata's fork copy). `random` is drawn only in `reset()` and the S&H branch, so sine / triangle / saw / square LFOs render bit-identically before and after. Gate: `$H --gate H2` **three times** → `[H2] preset Squarcle Storm` identical to the digit on all three runs, 20 / 20; the other 19 preset rows identical to the Round A verify figures. Record the now-stable figure in the `FactoryPresets.cpp` comment `// 13 Squarcle Storm` (token) in place of `H2 0.0 dB / 100 %`. Any per-preset export golden may now include this preset. |
| 33 | `ci-tests.yml` design (CONTEXT D2, RESEARCH 12, C9, C15) | **One workflow, one `plugin` input, two jobs.** `on.workflow_dispatch.inputs.plugin`: `type: choice`, `description: Plugin whose probes / Windows VST3 to build`, `options: [O-Octagon, O-Strata]`, `default: O-Octagon`, `required: true`. Jobs renamed `probes-macos` / `windows-vst3`. Every O-Octagon-specific step keeps its body byte-for-byte and gains `if: inputs.plugin == 'O-Octagon'`; the shared steps (checkout, JUCE version, setup, overrides, WebView2 NuGet, configure, resolve target, build VST3, pluginval, upload) are parameterised: `PLUGIN="${{ inputs.plugin }}"` in both configure steps, `resolve-target.sh cmake-target "${{ inputs.plugin }}"`, `build/plugins/${{ inputs.plugin }}/${TARGET}_artefacts/…`, artefact `name: pluginval-windows-${{ inputs.plugin }}-log`. **O-Strata macOS steps** (`if: inputs.plugin == 'O-Strata'`): `cmake --build build --target O-Strata-render-test --config Release -j$(sysctl -n hw.ncpu)`; `./build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test --gate all` from the repo root (fixtures resolve through `STRATA_FIXTURES_DIR`, cwd-independent — RESEARCH §2.6); `… --gate orbits --out "$RUNNER_TEMP/orbits.json" && node plugins/O-Strata/tests/orbit-golden.mjs "$RUNNER_TEMP/orbits.json"` (Node 22 is on `macos-14`). The Windows job needs no O-Strata-specific step. Header: the trigger-policy / why-separate paragraphs stay verbatim; "THE GATE IS THE BUILD STEP" is marked O-Octagon-only; a new paragraph states the two-plugin contract and, for O-Strata, the CAN / CANNOT claims **reusing the existing sentences verbatim** ("What this job CAN claim: the code compiles under MSVC; the VST3 loads; pluginval 10 OPENS THE EDITOR …" / "Named deferral, owner none, blocked on hardware.") plus the macOS claim ("the render harness passes every H1–H11 row and the orbit golden on a clean checkout"); the stale "enables eleven other harnesses" → "the other plugins' harnesses (37 carry the option; SKIP_PLUGINS keeps them out)". `permissions: contents: read`, the SHA-pinned actions, the pluginval 1.0.3 URL, the 600 s timeout, the no-`OUARICON_RELEASE` comment — all unchanged. `SKIP` construction unchanged (every other `plugins/*/` with a CMakeLists). A local sanity check before the commit: `python3 -c 'import yaml,sys; yaml.safe_load(open(".github/workflows/ci-tests.yml"))'` (PyYAML is available to Python 3.14 here or the check is skipped with a note) and `gh workflow view ci-tests.yml --yaml` after the push. **Wall-clock rows on the runner** (H7 ≤ 12 %, scheduler (c) ≤ 120 ms, import (d) ≤ 100 ms, H10 (d) < 1000 ms): accepted as is for v1.0 — a miss on one of these four rows is re-dispatched once; a second miss on the same row is recorded as runner-flaky in SUMMARY and the verify (the gate's letter is the build + pluginval), and the local thresholds are **not** loosened. A `STRATA_CI=1` band is v1.1. |
| 34 | Push + dispatch protocol | `workflow_dispatch` reads the workflow file **at the ref**, so the `ci` commit must be on `origin/main` before the input exists: `git push origin main` (this also pushes the five commits already ahead of origin — all `main`, all already intended for origin under trunk-based development; nothing is force-pushed, no branch is created). Then `gh workflow run ci-tests.yml --ref main -f plugin=O-Strata`; `sleep 20; gh run list --workflow ci-tests.yml -L 1 --json databaseId,url,headSha,status` → the run id; `gh run watch <id> --exit-status` **in the background** (memory `pattern_executor_watchdog_stall_run_long_commands_in_background`; expect 5–7 min per job, parallel); on completion `gh run view <id> --json jobs,conclusion,url,headSha` and `gh run download <id> -n pluginval-windows-O-Strata-log -D $SCRATCH/ci/`. Record in SUMMARY: run URL, `headSha` (must equal the `ci` commit), each job's conclusion + duration, the runner's `--gate all` count line and its H7 line, the pluginval log's final `SUCCESS` line. **Failure branches:** MSVC compile error → the memory traps first (`critical_msvc_constexpr_lambda_capture`, `critical_msvc_safepointer_init_capture_nested_lambda`, `critical_msvc_c3615_uninitialised_constexpr_local`, `critical_juce_string_char_ctor_is_ascii_only`), fix in `plugins/O-Strata` as **4.2b**, push, re-dispatch; a wall-clock miss → Decision 33; pluginval failure on Windows → read the log, fix as 4.2b if it is ours, else record as a named finding and stop the round for Taylor (do not mark COMPAT-02 complete). The O-Octagon path is **not** dispatched (its steps are unchanged apart from the `if:`). If Taylor vetoes the push before execute: skip Task 7, leave the `ci` commit local, record COMPAT-02 as pending with the exact command in SUMMARY, and finish every other task. |
| 35 | Harness TU guard + perf-log folder (RESEARCH 14, C11) | `CMakeLists.txt` (token `if(OUARICON_BUILD_TESTS)`) → `if(OUARICON_BUILD_TESTS AND APPLE)` with a one-line comment (`posix_memalign` / `getpid` in the harness TU; the Windows job never passes the option — this makes the omission a guard). `PluginEditor.cpp` `logViewPerf` (token `getChildFile ("Library/Logs/O-Strata")`) → `juce::FileLogger::getSystemLogFileFolder().getChildFile ("O-Strata")` — on macOS that is `~/Library/Logs/O-Strata` (JUCE returns `File ("~/Library/Logs")` under `JUCE_MAC`), so the Stage 3 documented path is unchanged and Windows gets `%APPDATA%\O-Strata\view-perf.log` instead of a fake `Library/Logs` tree. The header comment above `logViewPerf` names both paths. |
| 36 | PERF-03 WKWebView number + the `gl.finish()` flag (CONTEXT Finding 8, RESEARCH 16) | **Land the number first, by script**, then decide. The executor drives the Standalone burst itself (memory `pattern_standalone_hands_on_via_ax_scripting_and_window_capture`): `open -g build/plugins/O-Strata/O-Strata_artefacts/Release/Standalone/O-Strata-dev.app`, find the window id (CGWindow list by owner name), nudge it on-screen if occluded (rAF stops on an occluded WKWebView → blank canvases), select the Terrain tab (process-scoped click at the tab's rect — take the rects from a `screencapture -l <id>` screenshot at the window's scale, not from a guess), ⌥-click the bottom-left `θ · r · c` HUD three times, quit, read `~/Library/Logs/O-Strata/view-perf.log`. **Rule:** `mean ≤ 2` at `mode: "webgl2"`, DPR 2 → **no flag**, the figure goes to CHANGELOG + SUMMARY and the verify marks PERF-03 `partial (WKWebView)`; `mean > 2` → add the flag: `terrain-view.js` module-level `const perfOptions = { finishPerFrame: true }`, `scene.render` calls `finishFn` only when `perfOptions.finishPerFrame` (the **burst** finish stays unconditional so `burstMean` remains comparable), the log line gains `finish: perfOptions.finishPerFrame`, `window.__strataPerf.setFinish (bool)` toggles it; re-run the burst with the flag off, record both figures, ship the default that reads ≤ 2 ms (off if the submit-only mean is ≤ 2 and the finish-inclusive one is not — the wait is the cost, not the draw); the layout gate `webgl` section gains one row asserting `__strataPerf.setFinish` exists and the log line carries `finish` (only when the flag lands). Window not found / Taylor active on the machine / no log after three attempts → **no flag**, the row stays human (LISTENING Table C row 1), CHANGELOG says "pending". The Logic half is Taylor's in every branch. No page string changes in any branch (no i18n). |
| 37 | Listening record (RESEARCH 15, C13) | **`stages/4-polish/LISTENING.md`**, written by the executor as a complete skeleton with **every verdict cell blank** — the executor never fills a verdict (QUAL-04 is Taylor's sign-off). Header: date, code sha, hosts (Standalone `O-Strata-dev.app` from the build tree, Logic Pro AU, Live 12 VST3), device, verifier Taylor, vocabulary **PASS · NOTE** (→ CHANGELOG Known limits) **· CHANGE** (→ Decision 38) **· FAIL** (→ a bug: full Decision 46 / Task 9 gate list). **Table A** — the 18 presets × {Logic, Live 12} with columns preset / category / what to listen for (one line lifted from the RESEARCH §5.1 "Key values" cell) / Logic verdict / Live verdict / note; the three Bandlimited presets carry an extra "held chord above C6" cell. **Table B** — the 66 terrain × orbit C4 grid rows (`tests/exports/<terrain>_<orbit>_C4.wav`) plus the flagged C2 / C6 pairs: H6 2× worst (Mitsuhashi × Epitrochoid 7, −70.7 dB), H2 worst (Radial Rings × Ellipse, −2.3 dB), and every pair the Bandlimited H6 run mutes at A6 (the 17-row list from the harness output, 2× WAVs); listen with an `afplay` loop or a Logic track. **Table C** — the human rows folded in: Stage 3 rows 1–6 (`stages/3-gui/VERIFICATION.md` §Human) and Round A rows 1–6 (`round-a/VERIFICATION.md` §Human), each as one row with its source pointer, plus three Round B rows (Locate… + the Windows named deferral acknowledgement + the CI run URL opened once). Sitting order (one session): Standalone (Table C row 2 + the ⌥-click burst) → Logic (Table A, ⌘Z, ⌥-click) → Live 12 (Table A) → the WAV grid (Table B). A "How to run the material" block lists the `afplay` loop, the preset-render folder and the §5.4 commands. |
| 38 | CHANGE protocol (RESEARCH §5.4) | Applied **after** the sitting, one CHANGE per commit, in the verify session or a short execute re-entry — never in this execute (no listening verdict exists yet). Each CHANGE: the constant named by file + token (`Orbits.h` orbit-mod ranges → also the JS mirror in `terrain-view.js` + `--gate orbits` + golden re-record; `Terrains.h` → the scheduler fit table + ARCH rows; damp law / clamps / saturation in `TerrainOscillator.cpp`), then `ninja` the four targets, `$H --gate H2 --gate H3 --gate H6 --gate H7` (+ `scheduler` / `saturation` where §5.4 says), `$H --gate export` (re-records the golden — expect diffs on the changed terrain / orbit rows only), `$H --gate all`, re-listen the touched Table A / B rows, `git commit -- plugins/O-Strata` code + golden together. A NOTE never changes code. The factory bank is **not** re-authored for a NOTE; a CHANGE that moves a preset's H2 figure re-runs `--gate H2` and updates the `FactoryPresets.cpp` comments (the stamp moves with the def; the on-disk bank regenerates itself). |
| 39 | Listening material: preset renders + LF golden | New `--gate exportPresets` (never in `all`, beside `export`): for each `FactoryPresets::build` def in bank order, a fresh `Instance`, reset → apply the def (the H2 preset loop's apply, **without** `bypassFx()` — the listening material is the real sound) → `setHarnessPhaseSeed (opt.seed)` → note 60 velocity 1.0 held **2 s + 1 s release** → 24-bit stereo WAV at 48 kHz to `tests/exports/presets/NN-<Name>.wav` (NN = 01–18, name with spaces → `-`; the folder is under the gitignored `tests/exports/`), the processor output (post-FX), `check (written == 18)`; the header line prints the folder. **No golden for the preset renders** (the FX chain's own `juce::Random` users are outside the seed; the files are listening material, not a gate). The grid golden writer (token `golden.replaceWithText (sha)`) → `golden.replaceWithText (sha, false, false, "\n")` so `shasum -a 256 -c` works without `tr`; run `--gate export` once in this round, confirm the 198 checksums are **identical** to the committed CRLF golden modulo `\r` (`diff <(tr -d '\r' < old) new` empty), commit the LF golden with 4.2a (the diff is line endings only; memory `pattern_golden_tracked_as_checksum_only` — the WAVs stay untracked); README `export` row loses the `tr -d '\r'` note. |
| 40 | CHANGELOG v1.0.0 (RESEARCH 17, CONTEXT Finding 10) | `CHANGELOG.md` restructured to the O-Emulator shape: `## v1.0.0 — <execute date>` (the "(unreleased)" tag dropped; the tree is releasable, D3 — the tag comes with `/publish`), a two-sentence tagline, then **`### Added`** (the live wave-terrain oscillator — six terrains × eleven orbits, feedback, pitch tracking, Bandlimited mode with the Chebyshev projection, 2× / 4× oversampling; PNG import by chooser / drop, persistence in both forms, Locate…; the 18-preset bank with the two preset notes; the Terrain tab 3D view, gestures, ≋ view, readout; tuning / preset / i18n inherited from O-Prism v1.24.0 with en / fr / zh-Hans), **`### Behaviour to know`** (ARCH D5 constant +1-sample latency on top of the distortion oversampler's figure; D6 no wavetable mode — baked sources are v1.1; D8 2× is the default Quality, Bandlimited the showcase; PERF-02 = oscillator delta, already in REQUIREMENTS — recorded as done, no edit), **`### Known limits`** (the Stage 2 "Known limits — Bandlimited mode" paragraph lifted verbatim — F clamp 0.25–2, the 17 muted A6 rows, `silent above <note>`; the Round A persistence bullets — absolute path on another machine, drop ≤ 2 MB / Import… ≤ 8 MB by path, image-less preset clears the slots, one-ulp skewed-parameter note; **Windows**: O-Octagon's sentence verbatim — "Named deferral — owner none, blocked on hardware. No human sees the Windows UI this milestone." — with what the CI run proved; **PERF-03**: the WKWebView figure or "pending, Stage 3 human row 1"; the S&H LFO is clock-seeded in production, seeded only under the harness), **`### Technical notes`** (harness: `--gate all` count + time, H2 20 / 20, H6 / H7 figures, D4 before / after ns + the H7 Bandlimited delta before / after, the accuracy bound; CI: the dispatch command, run URL, `headSha`, both job durations; validation: pluginval ×2 + auval + the installed = built `cmp`; the bank stamp), **`### Stage history`** — the existing Stage 1–4 entries and their "Known limits" sub-blocks moved under this heading **verbatim, not deleted** (they are the audit trail). |
| 41 | NOTES.md, ARCH, PLUGINS.md, REQUIREMENTS (RESEARCH 17) | `NOTES.md` rewritten: **Status** `📦 Installed (not published)` / Version 1.0.0 / Type; **Lifecycle Timeline** in date order (the 2026-09-10 second-pass line moved to its place), + Stage 4 Round A verify (2026-09-13) + Round B rows; **Attribution & licence** (new section): ARCH Decision 10 clean-room — Mitsuhashi 1982 via Mills & de Souza 1999, standard curves; *Terrain* (GPL-3.0) inspiration only, no code; AGPL-3.0-or-later with SPDX headers; the botanical plate credit line **copied from `plugins/O-Prism/NOTES.md` at write time** (read it, do not paraphrase); **Known Issues** (the `stereoWidth` line, the path-form line, the Windows deferral, PERF-03 if pending); **Human rows open** → `stages/4-polish/LISTENING.md` Table C; **v1.1 list** (REQUIREMENTS §Out of Scope + CONTEXT: analytic fallback above the muting note, pitch-tracking soft knee, F-lattice, baked sources, RGB morphing, O-Prism octave-stretch port, `cdp-font-probe.js` promotion, `TerrainImage::view` deletion, ASan re-run, `STRATA_CI` band, base64 memoisation). `research/ARCHITECTURE.md`: under Core 6 / Decision 25 one paragraph — the padded 17 × 20 copy, `chebEvalPadded`, the accuracy bound, Clenshaw kept off-thread, the before / after figures. `PLUGINS.md` row → `| O-Strata | 📦 Installed | 1.0.0 | Synth (Microtonal Wave Terrain) | <date> |` with the D3 wording ("installed locally, not published; Stage 4 Round B …"), then the duplicate check `grep "^| O-" PLUGINS.md \| awk -F'|' '{print $2}' \| sort \| uniq -d` empty. **`REQUIREMENTS.md` untouched** — the Round B verify annotates COMPAT-02 (complete at the Finding 6 ceiling), QUAL-04 (complete if Tables A + B are signed, else pending with the sitting as the named step), PERF-02 (the Bandlimited row note), UI-01 / FUNC-07 (the named-deferral sentence, complete at "CI-built + editor opened under pluginval"), PERF-03 (partial WKWebView or pending), COMPAT-01 regression. |
| 42 | Install + validators (RESEARCH §2.10, CONTEXT D3) | `./scripts/build-and-install.sh O-Strata` (Phase 4 dual-variant sweep — expect no `⚠ Sweeping ALTERNATE-variant` since only `-dev` was ever installed; if it prints one, record which path it removed); `auval -a \| grep -i strata` **twice** (memory `pattern_cold_auval_after_install_rescans_registry`), `auval -v aumu OuSt OuDv` → `AU VALIDATION SUCCEEDED`; pluginval strictness 10 on the **installed** `~/Library/Audio/Plug-Ins/VST3/O-Strata-dev.vst3` and `Components/O-Strata-dev.component`; `cmp` the installed VST3 / AU binaries against the build tree's (sha256 recorded); the Standalone for hands-on rows is `build/…/O-Strata-dev.app` (memory `pattern_build_install_skips_standalone_stale_ui`); `git tag -l '*Strata*'` empty. The on-disk bank is untouched this round (no def changes → stamp `1.0.0+18c17735821c` unchanged; smoke [5] asserts it). |
| 43 | MSVC sweep (RESEARCH 14, §2.7) | After Tasks 1–2 and again at Task 9: `grep -c "= juce::Component::SafePointer" Source/PluginEditor.cpp` = 0; the C3493 grep (`constexpr` locals referenced inside a lambda) = 0; a bare-declaration scan of every new `constexpr` function (`chebEvalPadded` / `chebBasis17` are `inline`, not `constexpr` — keep them so; `kChebPadRow` / `kChebPadded` are initialised constants); the non-ASCII-in-literal grep = 0 (no non-ASCII in any new C++ string literal; comments may keep the arrows / dashes as the Round A rule allows). The `alignas (16)` array member and fixed-trip loops are MSVC-clean; `std::memset` is not used (the pad is written by the loop). The dispatch run is the oracle (Decision 34). |
| 44 | Human rows this round writes for the verify | All in `LISTENING.md` Table C (Decision 37) so there is one list: Stage 3 rows 1–6, Round A rows 1–6, plus (a) open the CI run URL once and confirm both jobs green, (b) the Windows named deferral acknowledged, (c) PERF-03 Logic half. None blocks the automated verdict; QUAL-04's status at the verify follows Decision 41. |
| 45 | Not touched this round | APVTS layout, `parameter-spec.md`, `params.tsv` (diff must stay empty); the persistence format (`v=1`, `customState {v, slots[]}`) and the factory defs (the S&H seed touches the harness path only; the Squarcle Storm comment is the only `FactoryPresets.cpp` edit); the taper law, the pitch-tracking law, `Orbits.h` / `Terrains.h` constants (a CHANGE is Decision 38, after the sitting); `clenshaw2D` and its three off-thread consumers; every page string (no i18n change — lints stay at 53 `'mt'`, labels at `default + 42`); `build-and-release.yml`; other plugins (O-Prism's `LFO.h`, other preset-manager copies); `stages/3-gui/*`, `round-a/*`; **no tag** (`git tag -l '*Strata*'` stays empty; the tag `O-Strata-v1.0.0` belongs to a later `/publish`). |

---

## Tasks

Waves group tasks that touch disjoint files. Inside a wave the executor may run tasks in any order or in parallel subagents; across waves the order is strict, except that Wave 4 may start while Wave 3's dispatch is running (Task 11 waits for Task 7's result).

### ━━━ Wave 1 — Evaluator, LFO seed, CMake guard + perf-log folder (parallel: disjoint files) ━━━

### Task 0 — Baselines (before any edit)
- **Run** on the current binaries (`ninja -C build O-Strata-render-test` must be a no-op first): `$H --gate H6 > $SCRATCH/h6-before.txt`; `$H --gate H7 > $SCRATCH/h7-before.txt`; `$H --gate clenshaw > $SCRATCH/clenshaw-before.txt`; `$H --gate H1 > $SCRATCH/h1-before.txt`; `$H --gate H2 > $SCRATCH/h2-before-1.txt` (Squarcle Storm's figure varies — that is the point); a warning census of the Round B translation units (`TerrainOscillator.cpp`, `LFO.cpp`, `StrataVoice.cpp`, `PluginEditor.cpp`, harness `main.cpp`) via forced recompile → `$SCRATCH/warnings-before.txt`.
- **Check:** `--gate H7` Bandlimited delta reads ≈ 11.6–12.1 % (the figure D4 is measured against); `--gate clenshaw` ≈ 85 ns.

### Task 1 — Padded Chebyshev evaluator (Decisions 28, 43)
- **Modify:** `Source/dsp/ChebyshevSet.h` — `kChebPadRow`, `kChebPadded`, `chebBasis17`, `chebEvalPadded` (Appendix A) after `clenshaw2D`; the file's header comment (token `Audio-thread budget (DSP-05): clenshaw2D is noexcept`) now names `chebEvalPadded` as the audio-thread evaluator and `clenshaw2D` as the off-thread / reference form.
- **Modify:** `Source/dsp/TerrainOscillator.h` — `chebW` declaration + the comment block above it (token `chebW[2]: the per-oscillator TAPERED coefficient copies`); `chebActive`'s comment (token `this block evaluates clenshaw2D`).
- **Modify:** `Source/dsp/TerrainOscillator.cpp` — `buildChebWeights` padded write with fixed trip counts; the scan site → `chebEvalPadded`; the comment above the scan site (token `The Chebyshev value is deliberately NOT clamped`) unchanged in meaning.
- **Check:** plugin + harness compile with **0 new warnings** vs `warnings-before.txt`; `$H --gate H1` identity row −128.3 dB (identical); `$H --gate H9` Bandlimited row `0.000e+00`; `$H --gate H6 > h6-after.txt` and the Decision 30 comparison script → max |Δ| ≤ 0.5 dB, `h_max` / muted / equality sets identical, 2× / 4× / 1× bit-identical; `$H --gate H7` Bandlimited delta ≈ 5 % (record); `grep -c clenshaw2D Source/dsp/TerrainOscillator.cpp` = 0 (comments excepted — state which).

### Task 2 — S&H LFO under the harness seed (Decision 32)
- **Modify:** `Source/dsp/LFO.h` (`seed()`), `Source/StrataVoice.cpp` (the four `seed` calls before the reset lines), `Source/FactoryPresets.cpp` (the `// 13 Squarcle Storm` comment figure — after Task 5's H2 runs).
- **Check:** `diff Source/dsp/LFO.cpp ../O-Prism/Source/dsp/LFO.cpp` unchanged from before; `$H --gate H2` ×3 → Squarcle Storm identical across runs, 20 / 20, the other 19 rows identical to `round-a/VERIFICATION.md` Goal 3's figures; `$H --gate H3` (LFO / mod rows) unchanged.

### Task 3 — CMake `AND APPLE`, perf-log folder (Decision 35)
- **Modify:** `CMakeLists.txt` (token `if(OUARICON_BUILD_TESTS)`); `Source/PluginEditor.cpp` `logViewPerf` + its header comment.
- **Check:** `cmake -B build` re-configure still builds `O-Strata-render-test` on this Mac (`ninja -n` lists it); the editor compiles; a Debug `DBG` line is not required — the path is exercised by Task 6.

### ━━━ Wave 2 — Harness rows, PERF-03 number, exports; build; gates; commit 4.2a ━━━

### Task 4 — Harness: `clenshaw` four lines, H7 promotion + machine line, `exportPresets`, LF golden, README (Decisions 29, 31, 39)
- **Modify:** `tests/render-harness/main.cpp` — `gateClenshaw` (padded copy helper `padTriangle` local to the harness, the 100 k-point Δ loops, the projected set via `ChebyshevProjector::projectAnalytic`, the double reference), `gateH7` (the check + `SystemStats::getCpuModel()`), `gateExportPresets()` under `wantsExact ("exportPresets")` beside `export` (token `wantsExact ("export")` / the usage line token `| export | topnote | orbits | h2cli | all`), the golden writer's line endings; `README.md` rows `clenshaw`, H7, `export`, + a new `exportPresets` row and the usage list.
- **Check:** `$H --gate clenshaw` → both ns figures, maxΔ random ≤ 2e-5 (expect ≈ 1.2e-5), projected ≤ 1e-6 (expect ≤ 5e-7), double-ref reported; `$H --gate H7` → 2× delta ≤ 12, Bandlimited check green with both numbers in the line; `$H --gate export` → 198 / 198, the golden is LF (`file` says "ASCII text" without CRLF), `diff <(tr -d '\r' < <(git show HEAD:plugins/O-Strata/tests/render-harness/golden/round-a-grid.sha256)) tests/render-harness/golden/round-a-grid.sha256` empty; `(cd tests/exports && shasum -a 256 -c ../render-harness/golden/round-a-grid.sha256 | grep -vc ': OK')` = 0; `$H --gate exportPresets` → 18 WAVs in `tests/exports/presets/`, each ≈ 3 s, none silent (`rms > 1e-3` printed per file), Pierce Bell / Wells Drone / Glass Rings audibly Bandlimited (spot-check one by spectrum: no content above the muting line at C4 — informational).

### Task 5 — PERF-03 burst by script + the `gl.finish()` decision (Decision 36)
- **Run:** the scripted Standalone burst (Decision 36) against the **current** Standalone build (rebuild `O-Strata_Standalone` first so Task 3's log-folder change is in); read the log.
- **Modify (only on the `> 2 ms` branch):** `Source/ui/public/js/terrain-view.js` (`perfOptions`, the conditional `finishFn`, the `finish` log field, `setFinish`); `tests/ui_layout_check.js` one `webgl` row.
- **Check:** the log line(s) with `mode`, `dpr`, `mean`, `max`, `submitMean`, `burstMean` recorded verbatim in SUMMARY; on the flag branch, both figures and the shipped default; on the unreachable branch, the three attempts' failure reasons in one line each. In every branch: `node plugins/O-Strata/tests/ui_shell_diff_check.js` 0 px and `ui_layout_check.js` ALL PASS (the page is otherwise untouched).
- **Then:** `ninja -C build O-Strata_VST3 O-Strata_AU O-Strata_Standalone O-Strata-render-test O-Strata-param-dump`; `$H --gate all` → **≥ 162 checks / 0 fail / ≤ 180 s** from the repo root (159 + 2 clenshaw + 1 H7); `--gate H2` ×3 stable; the Decision 43 greps; **commit 4.2a** (Decision 27) scoped `plugins/O-Strata` (includes the LF golden).

### ━━━ Wave 3 — CI workflow; push; dispatch ━━━

### Task 6 — `ci-tests.yml` plugin input + O-Strata steps (Decision 33)
- **Modify:** `.github/workflows/ci-tests.yml` only.
- **Check:** the YAML parses; `grep -c "O-Octagon" .github/workflows/ci-tests.yml` is now the `if:` lines + the `options` / `default` + the header mentions only (list them); every original O-Octagon step body is unchanged (`git diff` shows indentation / `if:` / parameterisation lines only in those steps); the O-Strata macOS steps name the exact binary path `build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test`; **commit ci** (Decision 27) scoped `.github/workflows/ci-tests.yml`.

### Task 7 — Push, dispatch, record (Decision 34)
- **Run:** `git branch --show-current` (= `main`), `git status --short` (nothing of ours unstaged), `git log origin/main..HEAD --oneline` (the list that will be pushed — record it), `git push origin main`; `gh workflow run ci-tests.yml --ref main -f plugin=O-Strata`; the run id; `gh run watch <id> --exit-status` **in the background**; on completion the `gh run view` JSON, the pluginval log download, and the runner's `--gate all` / H7 / orbit-golden lines from `gh run view <id> --log`.
- **Check:** both jobs `success`; `headSha` = the `ci` commit; the runner's `--gate all` count = the local count (≥ 162), 0 failures; pluginval log ends in `SUCCESS`; the artefact name `pluginval-windows-O-Strata-log`. Failure branches per Decision 34 (4.2b commit, re-dispatch; record every run URL, not only the green one).

### ━━━ Wave 4 — Listening material + record, gate run, install, docs, release commit ━━━

### Task 8 — `LISTENING.md` skeleton + material check (Decisions 37, 39)
- **Create:** `.planning/stages/4-polish/LISTENING.md` (Decision 37: header, vocabulary, Tables A / B / C with blank verdicts, the sitting order, "How to run the material", the §5.4 command block, the Decision 38 CHANGE protocol in three lines).
- **Check:** Table A = 18 × 2 verdict cells (+ 3 chord cells), Table B = 66 C4 rows + the flagged C2 / C6 pairs (count printed), Table C = 12 folded rows + 3 Round B rows; every WAV path in Table B exists (`ls` over the 198 + 18 files); the `afplay` loop line runs on one file.

### Task 9 — Gate run B1–B20 (RESEARCH §5.3 Round B list, pinned)
Run from the repo root after a clean `ninja` of the five targets; record every number in SUMMARY:

| # | Command | Expect |
|---|---|---|
| B1 | `$H --gate all` | ≥ 162 checks, 0 fail, ≤ 180 s; `presets: 18`; `[H2] factory presets: 20 / 20` |
| B2 | `$H --gate clenshaw` | both ns figures (padded ≤ ≈ 0.4 × Clenshaw, reported); maxΔ random ≤ 2e-5; projected ≤ 1e-6; double-ref reported |
| B3 | `$H --gate H6` + the Decision 30 script vs `h6-before.txt` | permanent checks green; max \|Δ\| ≤ 0.5 dB; `h_max` / 17 muted / 13 equality identical; 2× / 4× / 1× bit-identical |
| B4 | `$H --gate H7` | 2× delta ≤ 12 (≈ 4.2–4.4); Bandlimited delta ≤ 2× + 2.0 (≈ 5); CPU model line from `SystemStats` |
| B5 | `$H --gate H1`; `$H --gate H9` | identity −128.3 dB (identical); Bandlimited row `0.000e+00` |
| B6 | `$H --gate H2` ×3 | 20 / 20 on every run; Squarcle Storm identical; 19 rows identical to Round A verify |
| B7 | `$H --gate H10`; `--gate smoke --with-disk`; `--gate H8` | 24 / 24; 39 / 39 with the stamp `1.0.0+18c17735821c`; 8 / 8 with 0 allocations |
| B8 | `$H --gate orbits --out $SCRATCH/orbits.json && node plugins/O-Strata/tests/orbit-golden.mjs $SCRATCH/orbits.json` | 44 / 44 |
| B9 | `$H --gate export`; `shasum -a 256 -c` (no `tr`); `$H --gate exportPresets` | 198 / 198 OK; 18 files |
| B10 | `node scripts/check-i18n.js --plugin O-Strata`; `i18n-fr-lint.js`; `i18n-zh-lint.js`; `check-ui-labels.js`; `boot-all-uis.js --strict-tips`; `tests/ui_tip_render_check.js`; `tests/ui_layout_check.js` (+ `--viewport 1200x780`); `tests/ui_shell_diff_check.js`; `measure-ui.js`; CDP font probe | exit 0; exit 0 / exit 0 with 53 `'mt'`; `default + 42`, ≥ 1809 / 0; DEAD 0 / late 0 / no `stub invented:` / 34 natives; 122 bindings, ≥ 3215 / 0; 607 / 0 (+1 row only if the flag landed) + liveness fails (81); 0 px; ≤ 5 / 0; 0 off-face |
| B11 | `O-Strata-param-dump` vs `.planning/params.tsv` | diff empty |
| B12 | warning census (forced recompile of the Round B TUs) vs `warnings-before.txt` | 0 new |
| B13 | the Decision 43 greps | 0; 0; 0 bare declarations; 0 |
| B14 | Task 7's run | URL, `headSha` = `ci` commit, both jobs `success`, runner `--gate all` count, pluginval `SUCCESS` |
| B15 | Task 10's install + validators | `auval -a` ×2 lists `aumu OuSt OuDv` once; `AU VALIDATION SUCCEEDED`; pluginval VST3 + AU `SUCCESS`; installed = built (`cmp`) |
| B16 | `ls -R ~/Library/O-Strata/Presets/Factory` + `.factory-version` | 18 JSONs / 9 folders; stamp unchanged |
| B17 | `git tag -l '*Strata*'`; PLUGINS.md duplicate check; `git log --stat` of the three (or four) commits | empty; empty; every commit path-scoped |
| B18 | `LISTENING.md` counts (Task 8) | A 36 (+3) cells, B 66 + flagged, C 15 rows; all verdicts blank |
| B19 | `~/Library/Logs/O-Strata/view-perf.log` | present with ≥ 3 lines (number recorded) **or** the row recorded pending with the three attempts' reasons |
| B20 | docs | CHANGELOG (five sections + Stage history), NOTES (Attribution & licence), ARCH (D4 paragraph), PLUGINS.md 📦 Installed, STATUS; `git diff --stat -- plugins/O-Strata/.planning/REQUIREMENTS.md` empty |

### Task 10 — Install, validate (Decision 42)
- **Run:** `./scripts/build-and-install.sh O-Strata`; `auval -a | grep -i strata` ×2; `auval -v aumu OuSt OuDv`; pluginval strictness 10 on both installed bundles; `cmp` installed vs built; `git tag -l '*Strata*'`.
- **Check:** B15–B17 rows.

### Task 11 — Docs, STATUS, SUMMARY, release commit (Decisions 40, 41)
- **Modify:** `CHANGELOG.md`, `NOTES.md`, `.planning/research/ARCHITECTURE.md`, `.planning/STATUS.md` (frontmatter `phase: execute`, `status: stage_4_round_b_execute_complete`, `next_action: /plugin-verify O-Strata 4-polish`, the Stage 4 table row `execute (Round B)`, Current Position, Next Steps, Files), `PLUGINS.md` row.
- **Create:** `.planning/stages/4-polish/SUMMARY.md` (Round B: deliverables per task, B1–B20 numbers, the CI run record, the PERF-03 branch taken, the H6 before / after table, D4 before / after figures, deviations, the Table C human list, handoff to verify).
- **Commit release** (Decision 27) scoped `plugins/O-Strata PLUGINS.md`.

---

## Parallelism

| Wave | Tasks | Can run together | Blocked by |
|---|---|---|---|
| 1 | 0 → (1, 2, 3) | 1 ∥ 2 ∥ 3 — `ChebyshevSet.h` + `TerrainOscillator.*` vs `LFO.h` + `StrataVoice.cpp` vs `CMakeLists.txt` + `PluginEditor.cpp`; Task 0 first (baselines on the untouched binary) | — |
| 2 | 4, 5 (+ build, gates, commit 4.2a) | 4 ∥ 5 — harness `main.cpp` + README vs the Standalone burst (+ `terrain-view.js` only on the flag branch); 5's rebuild of the Standalone needs Task 3 | 1, 2, 3 (4 needs `chebEvalPadded`, `padTriangle` needs `kChebPadRow`; 2's comment figure needs 4's H2 run) |
| 3 | 6 → 7 | 7 waits on 6's commit; 7's watch runs in the background | 4.2a committed (the dispatch must build the D4 code) |
| 4 | 8 ∥ 9 ∥ 10 → 11 | 8, 9, 10 may run while 7's run is in flight; 11 needs 7's result (URL / headSha in CHANGELOG + SUMMARY) and 9 / 10's numbers | 7 (for 11 only) |

---

## Files

**Create:** `.planning/stages/4-polish/LISTENING.md`, `.planning/stages/4-polish/SUMMARY.md`; `tests/exports/presets/*.wav` (18, gitignored).
**Modify (plugin, commit 4.2a):** `Source/dsp/ChebyshevSet.h`, `Source/dsp/TerrainOscillator.h/.cpp`, `Source/dsp/LFO.h`, `Source/StrataVoice.cpp`, `Source/FactoryPresets.cpp` (one comment), `Source/PluginEditor.cpp` (`logViewPerf`), `CMakeLists.txt` (one condition), `tests/render-harness/main.cpp` + `README.md`, `tests/render-harness/golden/round-a-grid.sha256` (LF, same checksums); `Source/ui/public/js/terrain-view.js` + `tests/ui_layout_check.js` **only on the PERF-03 flag branch**.
**Modify (CI, commit ci):** `.github/workflows/ci-tests.yml`.
**Modify (docs, commit release):** `CHANGELOG.md`, `NOTES.md`, `.planning/research/ARCHITECTURE.md`, `.planning/STATUS.md`, `PLUGINS.md`.
**Untouched (must stay byte-identical):** `Source/dsp/ChebyshevProjector.*`, `TerrainScheduler.*`, `TerrainViewFeed.*`, `Orbits.h`, `Terrains.h`, `LFO.cpp`; `Source/PluginProcessor.*`; `Source/ui/public/index.html`, `js/i18n.js`; `tests/i18n-states.json`, `tests/ui-stub/*`; `.planning/params.tsv`, `parameter-spec.md`, `REQUIREMENTS.md`; `build-and-release.yml`; every other plugin.

---

## Success criteria (ROADMAP 4.2 test criteria as amended by CONTEXT D1–D4 and Decisions 27–45)

- [ ] `ci-tests.yml` dispatched with `plugin=O-Strata` from `origin/main`: macOS job builds `O-Strata-render-test`, `--gate all` passes at the local count and the orbit golden 44 / 44; Windows job builds `O-Strata_VST3` under MSVC + WebView2 and pluginval strictness 10 reports `SUCCESS`; run URL + `headSha` recorded (COMPAT-02 at the Finding 6 ceiling; the "VST3 loads in a Windows host with the 3D view" half is the named deferral, owner none, blocked on hardware)
- [ ] The padded evaluator is on the audio path: `--gate clenshaw` maxΔ ≤ 2e-5 (random) / ≤ 1e-6 (projected), H6 Bandlimited rows within ± 0.5 dB with identical `h_max` / muted / equality sets, H1 identity and H9 unchanged, and the H7 Bandlimited delta checks ≤ 2× delta + 2 % (PERF-02 Bandlimited item, D4)
- [ ] The S&H LFO renders bit-stably under the harness seed: `--gate H2` ×3 identical on all 20 preset rows; production seed path untouched (Round A verify Issue 1 closed)
- [ ] pluginval strictness 10 VST3 + AU `SUCCESS` and `auval` `SUCCEEDED` on the **installed** bundles after `build-and-install.sh` with the dual-variant sweep; installed = built by `cmp`; `auval -a` lists `aumu OuSt OuDv` once (COMPAT-01 regression; "no stale variant shadowing")
- [ ] `LISTENING.md` exists with Tables A / B / C complete and every verdict blank; the 198-WAV grid matches the LF golden by `shasum -c`; 18 preset renders exist (QUAL-04 material ready — the sign-off is Taylor's sitting, recorded at the verify)
- [ ] PERF-03: a WKWebView log line from the Standalone burst recorded, and the `gl.finish()` flag added only if the mean exceeded 2 ms; otherwise the row is recorded pending with reasons
- [ ] CHANGELOG `## v1.0.0 — <date>` with Added / Behaviour to know (ARCH D5, D6, D8; PERF-02 wording recorded as already resolved — no REQUIREMENTS edit) / Known limits (Bandlimited, persistence, Windows deferral sentence verbatim, PERF-03) / Technical notes (harness, D4 before / after, CI run) / Stage history; NOTES.md with Attribution & licence; ARCH D4 paragraph; PLUGINS.md → 📦 Installed
- [ ] Every B1–B20 row at its pinned number; page gates unchanged (labels `default + 42` / 0, boot 34 natives, tips 122, layout 607 / 0, shell diff 0 px, lints 53 `'mt'`); `params.tsv` diff empty; 0 new warnings; MSVC greps 0
- [ ] Three path-scoped commits (4.2a; ci; release) + at most one 4.2b; `git push origin main` once; **no tag**; `REQUIREMENTS.md` untouched

---

## Out of scope (do not do in this round)

- `/publish O-Strata 1.0.0`, any tag, any change to `build-and-release.yml` (CONTEXT D3; memory `feedback_never_tag_unless_publish`).
- Listening verdicts, defaults changes, bank re-authoring (Decision 38 — after Taylor's sitting, one CHANGE per commit).
- The analytic fallback above the muting note, the pitch-tracking soft knee, the F-lattice, baked sources, RGB morphing (v1.1 — REQUIREMENTS §Out of Scope).
- NEON intrinsics unless Decision 31's row misses; a processor-side evaluator switch; touching `clenshaw2D`'s off-thread consumers.
- A `STRATA_CI` runner band, loosening any local threshold, an O-Octagon dispatch.
- Any page string, i18n row, or `index.html` edit (the flag branch touches `terrain-view.js` only).
- Upgrading other plugins' modules; editing O-Prism's `LFO.h`.
- REQUIREMENTS status edits (the Round B verify).

---

## Requirements traceability (Round B = the stage verdict)

| Requirement | Round B evidence | Status after Round B verify |
|---|---|---|
| COMPAT-02 | Task 7 dispatch: Windows job green (MSVC + WebView2 build, pluginval 10 `SUCCESS`), run URL + `headSha`; macOS job green as the harness's CI home | complete (Finding 6 ceiling; the hands-on Windows half a named deferral) |
| QUAL-04 | `LISTENING.md` Tables A / B / C + the WAV grid + 18 preset renders (Task 8); Taylor's sitting fills the verdicts | complete if A + B signed at the verify; else pending with the sitting as the named step (Decision 41) |
| PERF-02 (Bandlimited item, D4) | B2 accuracy rows, B3 H6 band, B4 H7 Bandlimited check ≤ 2× + 2 % | complete (the 2× row was Stage 2); annotation only |
| COMPAT-01 (regression) | B15 pluginval ×2 + auval on the installed bundles, `cmp` | held |
| UI-01 / FUNC-07 (WebView2 halves) | CI Windows build + pluginval opened the editor; the named-deferral sentence | complete at the Finding 6 ceiling (annotated) |
| PERF-03 | Task 5's WKWebView number (or pending); WebView2 half = named deferral | partial (WKWebView) or pending |
| DSP-06 (regression, Squarcle Storm stability) | B6 H2 ×3 identical | held |
| FUNC-08 / FUNC-11 / QUAL-03 / UI-04 (regression) | B7 H10 24 / 24, smoke [5], `--gate all` | held |

## Handoff to verify

`stages/4-polish/SUMMARY.md` (Round B), the B1–B20 numbers, the H6 before / after table, the D4 ns + H7 figures, the CI run URL + `headSha` + both job durations + the pluginval log, the PERF-03 branch taken with its log line, `LISTENING.md` ready for the sitting, the three (or four) commit SHAs, `git push origin main` recorded. The Round B verify re-measures every gate from the release commit, re-dispatches nothing unless a commit landed after `headSha`, records Taylor's LISTENING counts (or QUAL-04 pending), annotates COMPAT-02 / QUAL-04 / PERF-02 / UI-01 / FUNC-07 / PERF-03 / COMPAT-01 in REQUIREMENTS.md with the deferral sentence, and writes `stages/4-polish/VERIFICATION.md` as the **Stage 4 verdict** → `/install-plugin O-Strata` is already satisfied by Task 10 (the handoff names it per the CLAUDE.md table; `/publish` stays a separate decision).

---

## Appendix A — the evaluator (Decision 28; lifted from the research bench `cheb_bench.cpp`, form (e), measured 28.5–29.1 ns dependency-carried vs 82.7–84.9 ns for `clenshaw2D`, bit-identical to the NEON form)

```cpp
// dsp/ChebyshevSet.h — after clenshaw2D

/** Audio-thread layout (Stage 4 Round B, Decision 28): each of the 17 rows padded to
    kChebPadRow floats, cp[n * kChebPadRow + m], pad lanes zero. buildChebWeights
    writes it by loop index; the evaluator has fixed trip counts and no data branch. */
constexpr int kChebPadRow = 20;
constexpr int kChebPadded = (kChebDegree + 1) * kChebPadRow;   // 340
static_assert (kChebPadRow >= kChebDegree + 1 && kChebPadRow % 4 == 0, "pad row");

/** T_0..T_16 (x) by the three-term recurrence. */
inline void chebBasis17 (float x, float* T) noexcept
{
    T[0] = 1.0f; T[1] = x;
    const float x2 = 2.0f * x;
    for (int k = 2; k <= kChebDegree; ++k) T[k] = x2 * T[k - 1] - T[k - 2];
}

/** f(x, y) = Σ_{n+m<=16} c_nm T_n(x) T_m(y) on a padded copy (17 x kChebPadRow).
    Per-row 4-lane dot products, even / odd rows into separate accumulators;
    max |Δ| vs clenshaw2D <= 2e-5 on a uniform ±0.5 set, <= 1e-6 on a projected set
    (harness --gate clenshaw). Portable plain C — MSVC x64 and clang auto-vectorise it. */
inline float chebEvalPadded (const float* cp, float x, float y) noexcept
{
    float T[kChebDegree + 1]; chebBasis17 (x, T);
    alignas (16) float U[kChebPadRow] = {}; chebBasis17 (y, U);   // U[17..19] stay 0
    float accE[4] = {}, accO[4] = {};
    for (int n = 0; n <= kChebDegree; n += 2)
    {
        const float* row = cp + n * kChebPadRow;
        float g[4] = {};
        for (int j = 0; j < kChebPadRow; j += 4)
            for (int l = 0; l < 4; ++l) g[l] += row[j + l] * U[j + l];
        for (int l = 0; l < 4; ++l) accE[l] += g[l] * T[n];
        if (n + 1 <= kChebDegree)
        {
            const float* r1 = row + kChebPadRow; float h[4] = {};
            for (int j = 0; j < kChebPadRow; j += 4)
                for (int l = 0; l < 4; ++l) h[l] += r1[j + l] * U[j + l];
            for (int l = 0; l < 4; ++l) accO[l] += h[l] * T[n + 1];
        }
    }
    return ((accE[0] + accO[0]) + (accE[1] + accO[1])) + ((accE[2] + accO[2]) + (accE[3] + accO[3]));
}
```

```cpp
// dsp/TerrainOscillator.cpp — buildChebWeights (padded write, fixed trip counts)
const float* src = chebSet->c.data();
float* dst = chebW[idx];
for (int n = 0; n <= kChebDegree; ++n)
{
    const int rowStart = chebRowStart (n);
    for (int m = 0; m < kChebPadRow; ++m)
        dst[n * kChebPadRow + m] = (m + n <= kChebDegree) ? src[rowStart + m] * chebTaperWeight (n + m, dc) : 0.0f;
}
```

```cpp
// tests/render-harness/main.cpp — harness-side helpers for --gate clenshaw
static void padTriangle (const float* c, float* cp) noexcept   // 153 -> 340, pad lanes zero
{
    for (int n = 0; n <= kChebDegree; ++n)
        for (int m = 0; m < kChebPadRow; ++m)
            cp[n * kChebPadRow + m] = (m + n <= kChebDegree) ? c[chebRowStart (n) + m] : 0.0f;
}
static double chebRefDouble (const float* c, double x, double y) noexcept   // 17-term double recurrence
{
    double T[17], U[17]; T[0] = 1; T[1] = x; U[0] = 1; U[1] = y;
    for (int k = 2; k <= 16; ++k) { T[k] = 2 * x * T[k - 1] - T[k - 2]; U[k] = 2 * y * U[k - 1] - U[k - 2]; }
    double acc = 0;
    for (int n = 0; n <= 16; ++n) for (int m = 0; m + n <= 16; ++m) acc += (double) c[chebRowStart (n) + m] * T[n] * U[m];
    return acc;
}
```
