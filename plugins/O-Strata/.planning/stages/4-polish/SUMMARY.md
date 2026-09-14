# Stage 4 — Polish / Validation, Round B (Phase 4.2): SUMMARY

**Plugin:** O-Strata · **Stage:** 4 of 4 · **Round:** B (ROADMAP Phase 4.2 — this round carries the stage verdict) · **Phase:** execute
**Date:** 2026-09-13 · **Mode:** manual · **Branch:** `main` (trunk-based, path-scoped commits)
**Plan:** `stages/4-polish/PLAN.md` (12 tasks / 4 waves / 3 commits, Decisions 27–45)
**Baseline:** `ab3994e4` (Round A verified; code `29803cb5`)
**Machine:** Apple M4 Max, macOS 26 / Darwin 25.6, Apple clang 17, node v24, Release build. CI runner: `macos-14` (Apple M1 Virtual) + `windows-latest`.

## Commits

| # | SHA | Scope | What |
|---|---|---|---|
| 4.2a | `76a90874` | `plugins/O-Strata/{CMakeLists.txt,Source,tests/render-harness}` | padded Chebyshev evaluator (D4), S&H LFO under the harness seed, `clenshaw` / H7 / `exportPresets` rows, LF golden, CMake `AND APPLE`, `FileLogger` log folder |
| ci | `36a6c1d4` | `.github/workflows/ci-tests.yml` | `plugin` dispatch input, `probes-macos` / `windows-vst3`, O-Strata macOS harness + orbit golden steps |
| fix | `5e044fd1` | `plugins/O-Strata/tests/render-harness` | one shared `writeWav24` for both export gates — the new gate had added 3 compiler warnings against the round's 0-new bar |
| ci (2) | `dcbc609a` | `.github/workflows/ci-tests.yml` | the orbit golden no longer skips when `--gate all` fails; two independent checks stop masking each other |
| release | (this commit) | `plugins/O-Strata` + `PLUGINS.md` | CHANGELOG v1.0.0, NOTES, ARCHITECTURE D4 paragraph, LISTENING.md, SUMMARY, STATUS, PLUGINS.md 📦 Installed |

No **4.2b** was needed — the Windows MSVC build and pluginval both passed first time. The two commits beyond the plan's three are deviations 1 and 6 below; both are small, path-scoped and self-contained.
`git push origin main` pushed `a2cf6513..36a6c1d4` (8 commits: the 6 already ahead of origin plus 4.2a and `ci`), then `36a6c1d4..dcbc609a`. **No tag** (`git tag -l '*Strata*'` empty).

---

## Task-by-task

### Task 0 — Baselines (on the untouched binary)

| Gate | Baseline |
|---|---|
| `--gate clenshaw` | 81.5 ns per `clenshaw2D` evaluation |
| `--gate H7` | 2× delta **4.40 %**; Bandlimited delta **11.69 %** (total 18.57, baseline 6.89) |
| `--gate H1` | Chebyshev identity **−128.3 dB** |
| `--gate H9` | Bandlimited row `0.000e+00` |
| `--gate H6` | 6 checks, 20.4 s; 144 `[H6 BL]` rows captured for the Decision 30 diff |
| `--gate H2` | 20 / 20; Squarcle Storm −3.6 dB on this run (Round A saw −2.2 … −4.0) |
| warning census | 26 warning lines across the Round B TUs, matching Round A's A15 list (line drift only) |

The H7 Bandlimited baseline read 11.69 %, inside the plan's expected 11.6–12.1 %; `clenshaw` read 81.5 ns against the plan's ≈ 85.

### Task 1 — Padded Chebyshev evaluator (Decisions 28, 43)

`dsp/ChebyshevSet.h` gained `kChebPadRow` (20), `kChebPadded` (340), `chebBasis17` and `chebEvalPadded` exactly as Appendix A specifies, plus a `static_assert` on the pad row and a header comment naming `chebEvalPadded` as the audio-thread evaluator and `clenshaw2D` as the reference / off-thread form. `dsp/TerrainOscillator.h`'s `chebW` became `alignas (16) float[2][kChebPadded]`; `buildChebWeights` writes the padded layout with fixed trip counts and the condition on loop indices only; the one scan site now calls `chebEvalPadded`.

| Check | Result |
|---|---|
| compile | 0 errors; warnings identical to baseline modulo line drift |
| `--gate H1` identity | **−128.3 dB** — identical to baseline |
| `--gate H9` Bandlimited | **`0.000e+00`** |
| Decision 30 H6 diff (144 rows) | max shift **0.100 dB** at Cosine Wells × Epitrochoid 5, A2 (bound 0.5) |
| `h_max` per row | **0 mismatches** |
| MUTED state per row | **0 mismatches**; the 17-row muted list is character-identical |
| bound-equality line | identical (`equality on 13`) |
| 2× / 4× / 1× rows | **bit-identical** (67 printed rows) |
| the 5 permanent H6 checks | identical text |
| `grep -c clenshaw2D Source/dsp/TerrainOscillator.cpp` | **0** |

### Task 2 — S&H LFO under the harness seed (Decision 32)

`dsp/LFO.h` gained a one-line `seed (juce::int64)`; `StrataVoice.cpp` seeds the four LFOs from `seedFor (2..5)` immediately before their resets, guarded by `harnessSeed != 0u`. The existing `harnessSeed` / `seedFor` locals were already in scope (declared at the note-on site well above the reset lines), so no hoist was needed. `LFO.cpp` is untouched; O-Prism's copy is untouched.

| Check | Result |
|---|---|
| `--gate H2` ×3 | **byte-identical full output** on all three runs |
| Squarcle Storm | **−3.6 dB / 100 % windows** on every run (Round A: −2.2 … −4.0) |
| all 20 preset rows | identical to the pre-change baseline run |
| `--gate H3` | 9 / 9 (LFO / mod rows unchanged) |
| `FactoryPresets.cpp` | the `// 13 Squarcle Storm` comment now records −3.6 dB / 100 % and why it is stable; comment-only, so the bank content stamp is unchanged |

### Task 3 — CMake guard, perf-log folder (Decision 35)

`if(OUARICON_BUILD_TESTS AND APPLE)` with a comment naming `posix_memalign` / `getpid` as the reason. `cmake -B build -DOUARICON_BUILD_TESTS=ON` still configures and builds `O-Strata-render-test` on this Mac. `logViewPerf` now uses `juce::FileLogger::getSystemLogFileFolder().getChildFile ("O-Strata")` — unchanged `~/Library/Logs/O-Strata` on macOS, `%APPDATA%\O-Strata` on Windows instead of a fake `Library/Logs` tree; the header comment names both.

### Task 4 — Harness rows (Decisions 29, 31, 39)

**`--gate clenshaw`** is now four lines, three of them checks:

```
  [clenshaw] clenshaw2D 83.9 ns / chebEvalPadded 30.3 ns per dependency-carried evaluation (reported)
PASS  [clenshaw] 1e6 evaluations finite on both evaluators, 83.9 / 30.3 ns each (reported)
PASS  [clenshaw] max |chebEvalPadded - clenshaw2D| = 9.775e-06 over 100k uniform points on the random set (need <= 2e-5)
PASS  [clenshaw] max |chebEvalPadded - clenshaw2D| = 2.980e-07 on a projected set (Sine Product, F 1, mx = my = 0.5; need <= 1e-6)
  [clenshaw] max |chebEvalPadded - double reference| 3.41e-06 (random set) / 2.49e-07 (projected set) (reported)
```

Across the session the two timing figures ranged 83.9–95.2 ns (`clenshaw2D`) against 28.0–30.3 ns (`chebEvalPadded`) — **2.8–3.4×**, against the research bench's 2.9× / 3.5×. Speed stays reported; the ratio verdict lives in H7, where both sides are measured the same way.

**H7** — the Bandlimited row is now a check against the same run's 2× delta plus 2.0, and the `machine:` line reads `juce::SystemStats::getCpuModel()`:

```
  machine: Apple M4 Max (Release build)
PASS  [H7] oscillator delta = 4.46 % (need <= 12; total 11.62, baseline 7.16)
PASS  [H7] Bandlimited delta 4.54 % <= 2x delta 4.46 % + 2.0 (D4 padded evaluator; was ~11.9 % with Clenshaw; total 11.55, baseline 7.01)
```

**`--gate exportPresets`** (new, never in `all`): every `FactoryPresets::build` def in bank order — reset to defaults, apply the def (**no** `bypassFx()`), harness phase seed, note 60 velocity 1.0, 2 s held + 1 s release, 24-bit stereo WAV at 48 kHz into `tests/exports/presets/NN-<Name>.wav`, rms printed per file. **18 / 18 written, 0 silent** (rms 0.0117 Butterfly Choir … 0.1783 Limacon Lead), each file 864 104 bytes = exactly 3 s.

**LF golden** — the grid golden is written with `\n`. The 198 checksums are **identical** to the committed CRLF file (`diff <(tr -d '\r' < old) new` empty), and `(cd tests/exports && shasum -a 256 -c ../render-harness/golden/round-a-grid.sha256 | grep -vc ': OK')` is **0** with no `tr`.

`tests/render-harness/README.md`: the `clenshaw`, H7 and `export` rows rewritten, a new `exportPresets` row, and the gate list in the options line.

### Task 5 — PERF-03 (Decision 36): **the unreachable branch**

No `view-perf.log` was produced, so **no `gl.finish()` flag was added**, `terrain-view.js` and `tests/ui_layout_check.js` are untouched, and PERF-03 stays pending. Four attempts, two mechanisms:

| # | Approach | Outcome |
|---|---|---|
| 1 | `open -g`, then `System Events … tell process "O-Strata-dev" to click at {1339, 208}` (Terrain tab) | the click returned `menu bar 1`; the page was white immediately afterwards |
| 2 | relaunch, `CGEventPostToPid` left-click at the same point | the page was **already white before the click** — the WKWebView never painted on that launch |
| 3 | relaunch activated; the window came back at x = −1428 (off-screen); `postToPid` click at (−348, 208) | the page painted in the captured buffer but the click did not land — negative screen coordinates |
| 4 | window pinned to (40, 60) by editing `~/Library/Application Support/O-Strata-dev.settings` (backed up and restored afterwards); `postToPid` click at (1120, 136) | the static page painted but the rAF-driven canvases were blank and the tab did not change — the documented occluded-WKWebView behaviour |

Brave Browser was frontmost throughout. `System Events` reported **0 AX windows** for the process on every launch, so the memory pattern's `set position` nudge was unavailable; the only remaining route was to activate the app, which would have taken the screen from Taylor. This reproduces Stage 3 verify Issue 2 exactly. The measurement remains a ~30-second human step: `LISTENING.md` Table C row 1.

Neither the page nor the instrumentation is implicated: `ui_shell_diff_check.js` reports **0 px** outside the masks (planted control 1576 px) and `ui_layout_check.js` **607 / 0**, unchanged.

### Task 6 — `ci-tests.yml` (Decision 33)

One workflow, one required `plugin` choice input (`O-Octagon` | `O-Strata`, default `O-Octagon`); jobs renamed `probes-macos` / `windows-vst3`. The three O-Octagon-specific macOS steps keep their bodies **byte-for-byte** and gained `if: inputs.plugin == 'O-Octagon'` — the `git diff` over those steps is the `if:` lines and nothing else. The shared steps are parameterised (`PLUGIN="${{ inputs.plugin }}"` in both configure steps, `resolve-target.sh cmake-target "${{ inputs.plugin }}"`, `build/plugins/${{ inputs.plugin }}/…`, artefact `pluginval-windows-${{ inputs.plugin }}-log`). O-Strata's macOS steps name the exact binary path `build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test`.

Header: trigger-policy and why-separate paragraphs unchanged; "THE GATE IS THE BUILD STEP" marked O-Octagon-only with the O-Strata equivalent stated; a new two-plugin-contract block carrying the CAN / CANNOT sentences verbatim plus the macOS claim; the stale "eleven other harnesses" corrected to "the other plugins' harnesses (37 carry the option; SKIP_PLUGINS keeps them out)". `permissions: contents: read`, the SHA-pinned actions, the pluginval 1.0.3 URL, the 600 s timeout and the deliberate `OUARICON_RELEASE` omission all unchanged. YAML parses under PyYAML and `gh workflow view ci-tests.yml --yaml` shows the new header after the push. `build-and-release.yml` untouched.

### Task 7 — Push, dispatch, record (Decision 34)

`git push origin main` pushed `a2cf6513..36a6c1d4` — 8 commits, all of them already-intended trunk work: the 6 that were ahead of origin at session start plus 4.2a and `ci`. Nothing was force-pushed and no branch was created. A later push carried `36a6c1d4..dcbc609a` (the harness `writeWav24` fix and the orbit-golden `if:` fix — see Deviations).

`gh workflow view ci-tests.yml --yaml` confirmed the new header and input on `origin/main` before dispatching.

**Three runs.** All three were dispatched with `-f plugin=O-Strata`; the O-Octagon path was never dispatched, and its steps show `skipped` in every run, which is the `if:` gating working.

| Run | `headSha` | `windows-vst3` | `probes-macos` | URL |
|---|---|---|---|---|
| 1 | `36a6c1d4` | **success**, 6m35s | failure, 8m17s — 162 checks, **2** failures | [34808363835](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/34808363835) |
| 2 (Decision 33 re-dispatch) | `36a6c1d4` | **success**, 6m01s | failure, 8m25s — 162 checks, **1** failure | [34808930140](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/34808930140) |
| 3 (after the orbit-golden `if:` fix) | `dcbc609a` | **success**, 6m12s | failure, 8m39s — 162 checks, **2** failures; **orbit golden 44 / 44** | [34809565500](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/34809565500) |

**Windows (the COMPAT-02 subject) is green on every run.** MSVC compiled the plugin TUs, WebView2 linked, the VST3 loaded, and pluginval strictness 10 completed every suite — including Editor, Open editor whilst processing, Editor Automation and Fuzz parameters — with no `FAIL` line in the 148-line log. The step's shell is GitHub's `bash --noprofile --norc -e -o pipefail`, so `pluginval.exe … | tee` does **not** mask the exit code: the step being green means pluginval exited 0. Artefact name `pluginval-windows-O-Strata-log`, downloaded and read.

**macOS**: the harness builds and runs; the check **count matches the local 162 exactly**; the orbit golden reports **44 / 44 within 1e-4, worst 2.93e-6 — the same figure as local**. What the runner does not meet is wall-clock:

| Row | Local (M4 Max) | Run 1 | Run 2 | Run 3 |
|---|---|---|---|---|
| `--gate all` wall-clock | 111.3 s | 288.8 s | 303.8 s | 292.0 s |
| `[H7]` 2x delta (<= 12 %) | 4.46 % PASS | 8.58 % PASS | 9.57 % PASS | 8.23 % PASS |
| `[H7]` Bandlimited delta (<= 2x + 2.0) | 4.54 <= 6.46 PASS | 16.89 <= 10.58 **FAIL** | 11.02 <= 11.57 PASS | 10.37 <= 10.23 **FAIL** |
| `[sched]` publish latency (<= 120 ms) | PASS | 264 ms **FAIL** | 176 ms **FAIL** | 154 ms **FAIL** |
| `machine:` line | `Apple M4 Max` | `Apple M1 (Virtual)` | same | same |

Both rows are in Decision 33's named wall-clock set, both missed more than once, so per that decision they are **recorded as runner-flaky and the local thresholds are not loosened**. The `machine:` line Decision 31 added did its job — the runner's log names its own CPU, which is what makes the comparison above readable at all.

**Decision 31's runner-safety rationale does not hold on this runner**, and that is worth recording rather than smoothing over. The reasoning was that the Bandlimited and 2x deltas "move together on a slower machine", so an absolute +2.0 slack would be safe. On the M4 Max the Bandlimited delta is 1.02x the 2x delta; on the M1 Virtual runner it is 1.15–1.97x, so the gap grows faster than the constant absorbs. The row is sound as a **local** regression guard — it is what pinned the 11.69 % -> 4.5 % improvement and would catch a regression back — but it needs a ratio form or a `STRATA_CI` band before it can gate a runner. That is a v1.1 item, not a v1.0 threshold change.

**No 4.2b was needed** — no MSVC compile error, no pluginval failure, nothing in the Windows path that is ours.

### Task 8 — `LISTENING.md` (Decisions 37, 39)

`stages/4-polish/LISTENING.md`, every verdict cell blank:

| Table | Contents | Blank verdicts |
|---|---|---|
| A | the 18 presets × {Logic, Live 12}, with category, terrain × orbit, one "what to listen for" line each, and an extra "chord above C6" cell on the three Bandlimited presets | 18 × 2 + 3 = **39** |
| B | the 66 terrain × orbit C4 rows by filename, then the flagged pairs: the H6 2× worst (Mitsuhashi × Epitrochoid 7, −70.7 dB), the H2 worst (Radial Rings × Ellipse, −2.3 dB), and each of the 17 pairs the Bandlimited H6 run mutes at A6 | 66 + 19 = **85** |
| C | Stage 3 rows 1–6, Round A rows 1–6, plus three Round B rows (open the CI run URL, acknowledge the Windows deferral, PERF-03 Logic half), each with its source pointer | **15 rows** |

Plus the sitting order, a "How to run the material" block (`afplay` loop, the preset-render folder, the re-render commands) and the Decision 38 CHANGE protocol. Every WAV path in Table B exists (198 grid + 18 preset files verified by `ls`).

### Task 9 — Gate run B1–B20

| # | Result |
|---|---|
| B1 `--gate all` | **162 checks, 0 failures, 111.7 s**, re-run on the final (post-`writeWav24`) binary at **111.3 s**, from the repo root (159 + 2 `clenshaw` + 1 H7); `presets: 18`; `[H2] factory presets: 20 / 20` |
| B2 `--gate clenshaw` | 83.9 / 30.3 ns; random maxΔ **9.775e-06** (≤ 2e-5); projected **2.980e-07** (≤ 1e-6); double-ref 3.41e-06 / 2.49e-07 |
| B3 `--gate H6` + Decision 30 script | permanent checks green (2× worst −70.7 dB at Mitsuhashi × Epitrochoid 7 A2; Bandlimited 127 / 127 sounding, worst −98.4 dB); max shift **0.100 dB**; `h_max` / 17 muted / 13 equality identical; 2× / 4× / 1× bit-identical |
| B4 `--gate H7` | 2× delta **4.46 %** (≤ 12); Bandlimited **4.54 %** ≤ 4.46 + 2.0; `machine: Apple M4 Max (Release build)` |
| B5 `--gate H1`; `--gate H9` | identity **−128.3 dB** (identical); Bandlimited row **`0.000e+00`** |
| B6 `--gate H2` ×3 | **20 / 20** on every run; full output byte-identical across runs; 19 non-S&H rows identical to Round A |
| B7 `--gate H10`; `smoke --with-disk`; `--gate H8` | **24 / 24**; **39 / 39** with stamp `1.0.0+18c17735821c` and 19 files; **8 / 8**, 0 allocations across 60 and 100 changes |
| B8 orbit golden | **44 / 44** within 1e-4, worst **2.93e-6** |
| B9 `--gate export`; `shasum -c`; `--gate exportPresets` | **198 / 198**; **0** non-`OK` lines with no `tr`; **18** files, 0 silent |
| B10 page + i18n gates | `check-i18n` / fr-lint / zh-lint exit 0 (53 at `reviewed:'mt'`); `check-ui-labels` **1809 / 0** over **default + 42** states; boot `clean 1 / 1`, **DEAD 0**, late 0, no `stub invented:`, `i18n=203`; tip gate **3215 passed** at **122 bindings**; layout **607 / 0**, `--viewport 1200x780` **526 / 81** (the documented liveness fails); shell diff **0 px** with the planted control at 1576 px; `measure-ui --mode box --report all` undeclared-font **5** (the inherited `‹ › ★ ⚙` header glyphs under zh-Hans), line-height-normal 3, wrap-count **0**, svg-font-attr **0**; CDP probe **16 runs, 0 nodes missing, 0 Han runs off face** |
| B11 `params.tsv` | **diff empty** (205 rows) |
| B12 warning census | **0 new** — see the deviation below; the final harness TU warning set is the baseline set |
| B13 MSVC greps | `= juce::Component::SafePointer` **0**; C3493 function-local-`constexpr`-in-lambda **0** (the only `constexpr` locals found are namespace-scope in `TerrainViewFeed.cpp`); the new `chebEvalPadded` / `chebBasis17` are `inline`, not `constexpr`, and `kChebPadRow` / `kChebPadded` are initialised constants; non-ASCII in C++ string literals **0** (the five hits are comments, the same five as Round A) |
| B14 CI runs | three, see Task 7: Windows **success** ×3; macOS harness + **orbit golden 44 / 44** on run 3, two wall-clock rows recorded runner-flaky |
| B15 install + validators | see Task 10 |
| B16 on-disk bank | **18 JSONs**, 9 category folders (Bass, Drone, FX, Init, Keys, Lead, Pads, Pluck, Sequence), stamp **`1.0.0+18c17735821c`** unchanged |
| B17 tags / duplicates / scoping | `git tag -l '*Strata*'` **empty**; PLUGINS.md duplicate check **empty**; every commit path-scoped |
| B18 `LISTENING.md` counts | A **39**, B **85** (66 + 19), C **15** — all verdicts blank |
| B19 `view-perf.log` | **absent** — recorded pending with the four attempts (Task 5) |
| B20 docs | CHANGELOG v1.0.0 with Added / Behaviour to know / Known limits / Technical notes / Stage history; NOTES with Attribution & licence + the v1.1 list; ARCHITECTURE's evaluator paragraph; PLUGINS.md 📦 Installed; STATUS updated. `git diff --stat -- plugins/O-Strata/.planning/REQUIREMENTS.md` **empty** |

### Task 10 — Install, validate (Decision 42)

`./scripts/build-and-install.sh O-Strata` completed cleanly and printed **no** `⚠ Sweeping ALTERNATE-variant` warning (only the `-dev` variant has ever been installed).

| Check | Result |
|---|---|
| `auval -a \| grep -i strata` ×2 | `aumu OuSt OuDv  -  Ouaricon Audio Development: O-Strata-dev`, once each pass |
| `auval -v aumu OuSt OuDv` | **AU VALIDATION SUCCEEDED** |
| pluginval 10, installed VST3 | **SUCCESS** |
| pluginval 10, installed AU | **SUCCESS** |
| installed vs built | `cmp` **identical** for both; sha256 `c86546cd…` (VST3) / `89ba772f…` (AU) |
| `git tag -l '*Strata*'` | empty |

### Task 11 — Docs, STATUS, SUMMARY, release commit

`CHANGELOG.md` restructured to the O-Emulator shape (`## v1.0.0 — 2026-09-13`, tagline, **Added** / **Behaviour to know** / **Known limits** / **Technical notes** / **Stage history**), with the existing per-stage entries and their Known-limits sub-blocks moved verbatim under Stage history. `NOTES.md` rewritten with Status 📦 Installed (not published), the timeline in date order plus the two Round B rows, a new **Attribution & licence** section, Known Issues, a pointer to `LISTENING.md` Table C for the human rows, and a v1.1 list. `research/ARCHITECTURE.md` gained the evaluator paragraph under the Bandlimited per-sample-evaluation heading. `PLUGINS.md` → 📦 Installed with the D3 wording. **`REQUIREMENTS.md` untouched** — the Round B verify annotates it.

---

## Deviations from the plan

1. **`--gate exportPresets` was refactored into a shared `writeWav24` helper** (harness-local) used by both `gateExport` and `gateExportPresets`. The first version duplicated the existing WAV-writing block, which added a second `createWriterFor` deprecation warning plus a `-Wsign-compare` and an extra `-Wsign-conversion` from an `int` loop over `std::vector::size()` — three new warnings against B12's "0 new". The helper removes the duplication and leaves exactly the baseline warning set. `--gate export` re-run after the refactor: the golden is **unchanged**, `shasum -c` still 0 non-`OK`, `exportPresets` still 18 / 18.
2. **PERF-03 took Decision 36's unreachable branch** (Task 5): four attempts, no log, **no `gl.finish()` flag**, row stays human. `terrain-view.js` and `tests/ui_layout_check.js` are untouched, so the layout gate stays at 607 rows rather than 608.
3. **Decision 41's botanical-plate credit could not be copied.** There is no plate credit line in `plugins/O-Prism/NOTES.md` — or anywhere else in the repository. The line in `NOTES.md` was written from the asset's own filename (`shell_conchologiaiconi12reev_0090.png` → Lovell Reeve, *Conchologia Iconica* vol. 12, 1854–1878, a public-domain plate) and flagged as such in the text itself, so a later reader knows it was derived rather than sourced.
4. **Decision 31's runner-safety rationale did not hold** — see Task 7. The Bandlimited and 2× deltas do **not** move together on a slower machine (1.02× locally, 1.15–1.97× on the runner), so the +2.0 absolute slack is a local guard rather than a runner gate. The row stays as it is for v1.0; a ratio form or a `STRATA_CI` band is a v1.1 item (recorded in NOTES).
5. `--gate all` is **exactly** 162, not "≥ 162": the `clenshaw` gate went from 1 check to 3 and H7 from 3 to 4, so 159 + 3.
6. **A second `ci` commit (`dcbc609a`) was not in the plan.** Task 6 chained O-Strata's two macOS checks with a plain `if:`, so a wall-clock miss in `--gate all` also **skipped** the orbit golden — leaving a PLAN success criterion ("the orbit golden 44 / 44" on the runner) unmeasured on runs 1 and 2. `if: ${{ !cancelled() && inputs.plugin == 'O-Strata' }}` makes the two independent checks report independently; the job still fails if either fails, so nothing red is turned green. Run 3 then reported the golden at 44 / 44, worst 2.93e-6. This was a defect in my own step ordering rather than a runner flake, which is why it was fixed instead of recorded.
7. The Decision 30 H6 comparison was implemented as a ~45-line Python script rather than the plan's "20-line script" — the `[H6 BL]` line format packs three notes per row with `MUTED` and `QUIET` variants, which needs real parsing. Same contract, same assertions.

## Carried to the verify

- **QUAL-04** — `LISTENING.md` Tables A and B are the sitting. Nothing in this round fills a verdict.
- **PERF-03** — pending; the WKWebView figure and the Logic half are both human.
- **15 human rows** — `LISTENING.md` Table C (Stage 3 rows 1–6, Round A rows 1–6, three Round B rows).
- **The runner wall-clock question** — whether to add a `STRATA_CI` band (v1.1 in NOTES) or leave the two rows recorded as runner-flaky.

## Handoff

`/plugin-verify O-Strata 4-polish` — the Round B verify re-measures every gate from the release commit, records Taylor's LISTENING counts (or QUAL-04 pending), annotates COMPAT-02 / QUAL-04 / PERF-02 / UI-01 / FUNC-07 / PERF-03 / COMPAT-01 in `REQUIREMENTS.md`, and writes `stages/4-polish/VERIFICATION.md` as the **Stage 4 verdict**. `/install-plugin O-Strata` is already satisfied by Task 10; `/publish` stays a separate decision.
