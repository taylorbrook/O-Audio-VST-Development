---
title: "O-ParametricEQ Stage 4 (Polish) — Phase 4.2 Execution Plan (validate: content pin, C1 push + Windows dispatch, Taylor sitting, re-voice + fr flip, binding battery)"
created: 2026-11-16
juce_version: "8.0.15"
phase: "4.2"
summary: "Eight waves: (W0) pre-flight on e3ee9546 / b529f9a6, origin 2 behind; (W1) harness-only rows from VERIFICATION-4.1 Issue 2: FC1 spec-table pin, FC2 feature invariants per preset, FP5i Mono Low End gated (FP5m); PN1 repoRoot token (Issue 4); mutants V9 / V10 re-run now caught; (W2) C1: ask Taylor at the moment, push main (the 4.1 commits + W1), dispatch ci-tests.yml -f plugin=O-ParametricEQ in the background; (W3) HANDS-ON-4.2.md, then STOP for Taylor's sitting (P6 checklist + listening pass + French read + comma call); (W4) apply outcomes: re-voice in units with the FC1 pin moved in the same edit, fr flip to reviewed: true, optional comma rule with HT1 rows, 0.11.0; (W5) the binding battery on the final tree: clean rebuild, designer, render default + --long with --lid-dir, sessions vs 0.5.0, N1e, G9 (b), meter-ballistics, --g10 ALONE (one re-run), editor bench, five UI gates, PN1, WebView2 gate, install, pluginval 10, auval; (W6) Windows result read, re-dispatch only if C++ changed after the push; (W7) SUMMARY, STATUS, REQUIREMENTS, CHANGELOG, path-scoped commit, push only with a second go-ahead. Audio frozen: XR-lid SHA-equal to 0.5.0, N1e 6274ff9b5b0e488a (a re-voice changes presets, not the engine). No tag."
stages: [4]
agents: [dsp-agent, gui-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 4 / Phase 4.2 Execution Plan (validate)

## Goal

Close Stage 4. Every stage-4 requirement reaches Complete on a binding reading of the final tree:

- **FUNC-07** — the 13-preset bank loads in Logic (AU) and Reaper (VST3), passes Taylor's listening pass, and its *content* is gated (not only its mechanics, VERIFICATION-4.1 Issue 2).
- **UI-05** — Taylor's French spot read; fr flips to `reviewed: true`; zh stays `'mt'`; five UI gates 0.
- **PERF-02** — `--g10` binding reading, run alone (Issue 1): G10a ≤ 10 %, G10b ≤ 35 %, G10c ≤ 0.3 % and ≤ 1.5 × pass-through (+ the 0.01 % floor, ARCHITECTURE), G10d net-cost linearity, G10d0 self-check PASS; G10i INFO.
- **COMPAT-02** — FP5 at five rates mono / stereo (already gated) + B1 layouts (G13) + Logic mono / stereo + Reaper mono / stereo + **Windows VST3 pluginval 10 on windows-latest** (`ci-tests.yml` dispatch).
- **QUAL-01** — listening criterion met across the factory presets.
- ROADMAP "full re-run of every gate" on a fresh rebuild.

Version: **0.11.0** if anything in source changes (the fr flip will). Audio engine frozen.

**In scope:** `tests/render-harness/main.cpp` (FC1, FC2, FP5m), `tests/preset-name-fit/preset_name_fit.js` (one token), `Source/FactoryPresets.h` (re-voice values only, from the sitting), `Source/ui/public/js/i18n.js` (fr `reviewed` flags; wording fixes from the read), `Source/ParamText.h` (only if Taylor takes the comma rule), `CMakeLists.txt` (0.11.0), `CHANGELOG.md`, `.planning/REQUIREMENTS.md`, `STATUS.md`, `PLUGINS.md` (row status), evidence under `stages/4-polish/execute-4.2/`, `stages/4-polish/HANDS-ON-4.2.md` (new).

**Out of scope:**

| Item | Why |
|---|---|
| Tag, release, `/publish` | `feedback_never_tag_unless_publish`; beta 0.x, CONTEXT constraint. |
| SIMD, correction hold | Only if G10 fails twice on a quiet machine (P3). Then STOP and report; do not implement inside 4.2. |
| WebView2 open-time / preset-settle timing | Owed to the first Windows hands-on (ROADMAP risk row, PLAN-4.1). |
| Other plugins' lint findings raised by the 4.1 glossary | Reported only. |
| zh `reviewed` change | Stays `'mt'` (`project_i18n_zh_hans_rollout`). |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/4-polish/VERIFICATION-4.1.md` | Issues 1 – 5; the human checklist (W3 copies it). |
| `stages/4-polish/SUMMARY-4.1.md` "For 4.2" | Carry list. |
| `stages/4-polish/CONTEXT.md`, `RESEARCH.md` Q2 / Q3 / Q5, pitfalls | Binding decisions, battery recipe. |
| `stages/4-polish/verify-4.1/run-harness.sh` | The battery recipe on a fresh compile (session dump → SHA list → designer → render `--lid-dir` → `--dump-params` → probe N1e → `--long`). W5 copies it. |
| `stages/4-polish/verify-4.1/mutants41v.py` | V9 / V10 mutant texts (W1 re-runs them). |
| `stages/4-polish/execute-4.1/editor-bench/` | `run-all.sh`, `verdict33.py`, `verdict41.py`, `lockstate`. W5 copies the bench. `3-gui/execute-3.2/editor-bench/verdict32.py` for the 3.2 rows if the JSON carries them. |
| `Source/FactoryPresets.h:120-200` (spec table), `:339` (`bankHash`) | W1 pin, W4 re-voice site. |
| `tests/render-harness/main.cpp:17860-17970` (FP1 / FP1o), `:18149-18372` (FP5 / FP5i), `:18553+` (`--g10`), `:1055-1062` (B1) | W1 edit points; G13 evidence. |
| `tests/preset-name-fit/preset_name_fit.js:74` | Issue 4 token. |
| `.github/workflows/ci-tests.yml` | Dispatch input `plugin`, option already listed (4.1). |
| Memory: `index_git_shared_checkout`, `pattern_executor_watchdog_stall_run_long_commands_in_background`, `pattern_locked_screen_throttles_wkwebview_bench`, `pattern_realtime_paced_bench_core_idle_artefact`, `pattern_juce_mac_home_ignores_HOME_use_cffixed_user_home`, `feedback_presets_never_set_output_gain`, `pattern_factory_preset_normalized_ignores_skew`, `pattern_gate_inherits_the_review_findings_scope`, `pattern_ci_macos14_runner_retired`, `critical_webview2_runtime_gotchas_windows`, `pattern_gate_stimulus_below_threshold_is_vacuous` | Traps. |

---

## Plan-phase decisions

**State change since RESEARCH (C1 re-scoped).** `origin/main` was pushed with the CI runner fixes (`b082bc36`, `112fa6b5`). `git rev-list --count origin/main..main` = **2** today (`e3ee9546` execute 4.1, `b529f9a6` verify 4.1), both O-ParametricEQ only. C1 (push at 4.2) stands, and is now small. Still outward-facing: the executor asks Taylor once more at the moment of the push, showing `git log --oneline origin/main..main` (other sessions may have committed in between).

**D1 Push early, once.** Push in W2, before the sitting, so the Windows result (≈ 10 min) is in hand while Taylor listens and any MSVC fix lands inside 4.2. Everything W4 can change afterwards is preset values (floats in a table), i18n flags / strings, and possibly `ParamText.h`. Rule: **re-dispatch at the end only if a `.cpp` / `.h` other than the spec-table values changed after the pushed SHA** (W6). The final 4.2 commit is pushed only with a second go-ahead at that moment.

**D2 Content gate (Issue 2), two layers.**
- **FC1 pin.** A canonical hash of `factoryPresetSpecs()` in engineering units (names sorted; per preset `{ id, value }` sorted by id; values printed `%.6g`), XXH64 hex, compared with a literal `kFactorySpecPin` in the harness. Independent of `bankHash` (which hashes normalised defs). A re-voice moves the literal in the same edit; the row prints the new hash on FAIL so the edit is mechanical.
- **FC2 invariants.** A pin update would bless an accident made in the same edit, so each preset's *defining feature* gets a structural assertion that survives any re-voice:
  - Rumble Cut: b01 Low Cut, slope 96 dB/oct. Telephone: Low Cut + Band Pass + High Cut, both cuts ≥ 48 dB/oct, `auto_gain` on.
  - Hum Notch 50 / 60 Hz: all bands Notch, frequencies integer multiples of 50 / 60 Hz.
  - Mono Low End: Low Cut, placement Side. Side Air: one band Mid, one band Side. L-R Pair Match: at least one Left and one Right placement.
  - Air Shelf: High Shelf ≥ 8 kHz, gain > 0. Gentle Master Tilt: a Tilt band, |gain| ≤ 3 dB. Kick Tighten / Vocal Presence / De-Mud: type and sign of gain as in the current table (executor reads them and writes them in).
  - Global: no preset names `output_gain` / `bypass` (already FP1o); |gain| ≤ 5 dB on every bell / shelf (RESEARCH Q1 bound); band indices ascending in frequency from b01.
- **FP5m (Issue 2, FP5i gated).** Mono Low End's broadband level change on mono is *more negative* than on stereo by a margin. Executor reads the 4.1 FP5i INFO figures, sets the margin to half the measured gap (record both numbers), and adds a control: the same comparison on Air Shelf (stereo placement) must not pass the margin. FP5i's remaining INFO stays INFO.
- **Mutants to prove it:** VERIFICATION-4.1's V9 (placement dropped from every def) and V10 (Rumble Cut 96 → 6 dB/oct), re-run against the new rows. Both must be caught by FC2 (and V9 by FP5m). Plus V14: a one-value drift in Kick Tighten's frequency → FC1.

**D3 Issue 4.** `S.buildRoot('O-ParametricEQ', { repoRoot })` with `repoRoot` from the script's own location (`path.resolve(__dirname, '../../../..')`), default behaviour unchanged. Prove it by re-running VERIFICATION-4.1's V12 in a scratch copy without the workaround: caught.

**D4 Issue 3 (comma) is Taylor's call at the sitting.** Options for the checklist line:
- (a) **Keep** (recommended): comma = decimal point, as JUCE's default parser and the page's own entry do; French "0,7" works; "1,200" → clamps. No change.
- (b) Thousands rule: a comma followed by exactly three digits and no further separator is a thousands separator ("1,200" → 1200), anything else a decimal point. Costs HT1 rows for "1,200", "0,7", "1,25", "12,000 Hz"; French "1,250" (meaning 1.25) then reads 1250 — the reason (a) is recommended.
Exponent ("1e3" → 1): no change either way (host text is not a calculator); recorded.

**D5 The sitting is a hard stop inside execute.** W3 writes `HANDS-ON-4.2.md` and STOPS. Execute resumes when Taylor reports results in the session (same shape as 4.1's in-session calls). A FAIL that needs more than a re-voice, a string fix or the comma rule (for example a host-text display bug in Logic, a meter dip at 2048) is recorded and **4.2 stops at that point** with a SUMMARY that names it; it is not fixed inside the validate phase without a fresh plan.

**D6 G10 run discipline (Issue 1).** `--g10` runs as its own step after everything else in W5 has finished: `quiet()` (no pluginval / auval / ninja / harness / EditorBench), screen unlocked, load average printed. One FAIL on G10 or G10d0 → wait 2 min, re-run once; the second reading is binding. Two FAILs on a quiet machine → STOP (SIMD is out of scope here).

**D7 Re-voice rules.** Values in engineering units in the spec table only; bands in ascending frequency from b01; never `output_gain`; |gain| ≤ 5 dB; FC1 literal moved in the same edit; FC2 must pass unmodified (if a re-voice *intends* to change a defining feature, Taylor says so and FC2's line for that preset changes with a comment naming the sitting). The bank hash carries the re-voice to `Factory/` at the same version; a reinstall is still needed, and the DAW closed.

**D8 fr flip.** All `reviewed: false` rows for fr (53, RESEARCH Q4) → `true`, after Taylor's read and any wording fixes. `i18n-fr-lint` and `check-i18n` re-run; the header comment in `i18n.js` (lines 33 – 40) updated to say who read it and when.

---

## Tasks

### Wave 0 — pre-flight

1. **State.** `git branch --show-current` = main; `git status --short -- plugins/O-ParametricEQ` empty; HEAD contains `b529f9a6`; `git rev-list --count origin/main..main` recorded; `gh auth status` OK. Record installed bundle versions (0.10.0 `-dev`, no unsuffixed orphan) in `execute-4.2/baseline/state.txt`.
   - Depends on: none.

### Wave 1 — harness-only content gate (dsp-agent; no plugin source change)

2. **FC1 + FC2** in `tests/render-harness/main.cpp`, beside FP1 / FP1o, in the default run and in `--rows41` (rename nothing; add `--rows42` that runs FC1, FC2, FP5m only, for fast iteration). Spec canonicalisation lives in the harness, not in `FactoryPresets.h`.
   - Depends on: 1.
3. **FP5m** beside FP5i (D2). Reuse FP5i's level measurement; no second render pass.
   - Depends on: 1.
4. **PN1 token** (D3).
   - Depends on: 1.
5. **Build the harness, run default with `--lid-dir` once**: 183 + 3 = 186 / 0 (FC1, FC2, FP5m). Record the FP5m margin and its two source numbers.
   - Depends on: 2, 3.
6. **Mutants V9, V10, V14** (D2) via a small `execute-4.2/mutants/mutants42.py` modelled on `mutants41v.py`, scratch copies only; plus V12 re-run in a scratch root with the new PN1 (D3). Expected: V9 → FC2 + FP5m, V10 → FC2, V14 → FC1, V12 → PN1. Control (unmutated) all PASS.
   - Depends on: 4, 5.

### Wave 2 — C1 push and Windows dispatch

7. **Commit W1** path-scoped (re-check branch and staging first; temp-index if anything foreign is staged):
   `git commit -- plugins/O-ParametricEQ` — message `test: O-ParametricEQ 4.2 W1 - FC1 / FC2 / FP5m content rows, PN1 repoRoot`.
   - Depends on: 6.
8. **Ask Taylor** (AskUserQuestion): show `git log --oneline origin/main..main` and the dispatch command. On yes: `git push origin main`, then `gh workflow run ci-tests.yml -f plugin=O-ParametricEQ --ref main`; record the run id. Watch it in the background (`gh run watch <id> --exit-status`, `run_in_background`). On no: COMPAT-02's Windows criterion stays Pending; continue.
   - Depends on: 7.

### Wave 3 — Taylor's sitting (HARD STOP)

9. **Write `stages/4-polish/HANDS-ON-4.2.md`**: the VERIFICATION-4.1 Human Verification list verbatim, plus:
   - Logic AU **mono** and **stereo** track; Reaper VST3 **mono** and **stereo** track: load, every factory preset, save / reload a session (COMPAT-02).
   - Reaper: render offline at 96 kHz once with a steep preset (Rumble Cut) — no error, no silence.
   - The comma call (D4) with (a) recommended.
   - Per-preset table for the listening pass: name | keep / re-voice | what to change (in Hz / dB / Q / slope).
   - French read: list of any row to reword (id → new text).
   - Reminder: the installed build is 0.10.0 (identical audio to 4.2's final).
   Then **STOP** and present the checklist to Taylor. Do not proceed until results are given.
   - Depends on: 7 (8 may still be running).

### Wave 4 — apply the sitting's outcomes (dsp-agent for C++, gui-agent for i18n)

10. **Re-voice** per the table (D7). FC1 literal moved; FC2 unchanged unless Taylor named a feature change. Skip if all presets are "keep".
    - Depends on: 9 (results in).
11. **French**: apply rewordings; flip fr to `reviewed: true` (D8); update the header comment. zh untouched unless a zh 800 px overflow was reported (then pin the segment width, never shrink the caption).
    - Depends on: 9.
12. **Comma rule** only if Taylor took (b): `ParamText.h` `splitNumber` + HT1 rows for the four strings in D4.
    - Depends on: 9.
13. **0.11.0**: `CMakeLists.txt` `OPEQ_VERSION`; CHANGELOG `## [0.11.0] - <date>` (bracket form) listing re-voiced presets by name, the fr review, and the comma decision. If 10 – 12 changed nothing in source, stay at 0.10.0 and say so.
    - Depends on: 10, 11, 12.

### Wave 5 — binding battery on the final tree (build-gate-runner; long steps in the background)

14. **Fresh rebuild**: delete the harness and plugin objects; build designer, render harness, probe N1e, plugin VST3 / AU / Standalone (Release).
    - Depends on: 13.
15. **Harness battery** (copy `verify-4.1/run-harness.sh` → `execute-4.2/run-harness.sh`): session dump → SHA list vs `verify-4.1/sessions-0100.sha` (16 / 16, = 0.5.0) → designer 150 / 0 / 18 → render default `--lid-dir` 186 / 0 (+ HT rows if 12) → `--dump-params` byte-equal to golden → probe N1e 16 / 16 `6274ff9b5b0e488a` → render `--long` 186 / 0.
    - Depends on: 14.
16. **G9 (b)** `tests/g9b/run.sh` and **meter-ballistics**: PASS.
    - Depends on: 14.
17. **Install** with `./scripts/build-and-install.sh O-ParametricEQ` (cache clear, dual-variant sweep); `diff -rq` installed vs artefacts; real `Factory/` = 13 + `.factory-version` + `.bank-hash`, and the `.bank-hash` equals the harness's `bankHash` print (a re-voice changes it; no re-voice keeps `2dca9fb8ba6a7172`); `User/` untouched.
    - Depends on: 14.
18. **Validators** on the installed bundles: pluginval strictness 10 VST3 and AU; `auval -v aufx OuPQ OuDv`. Hash the `Factory/` listing before and after.
    - Depends on: 17.
19. **Five UI gates + PN1 + WebView2 static gate**: `check-i18n`, `i18n-fr-lint` (0 findings, now reviewed rows), `i18n-zh-lint`, `check-ui-labels` (widest language), `boot-all-uis --strict-tips`; PN1 / PN1n; `tests/windows-webview2-gate.sh`.
    - Depends on: 11, 14.
20. **Editor bench** (copy `execute-4.1/editor-bench/` → `execute-4.2/editor-bench/`, rebuild against the final tree): Release `RUNS=1` + Debug `--live`; score `verdict33.py` and `verdict41.py` (and `verdict32.py` on any 3.2 rows the JSON carries); Debug 0 assertions. lockstate first.
    - Depends on: 15, 16, 18, 19 (machine quiet).
21. **`--g10` alone** (D6): after 20 has exited. Record load average, QoS, all rows; one re-run allowed.
    - Depends on: 20.

### Wave 6 — Windows result

22. Read the CI run from task 8: `windows-vst3` job conclusion, pluginval log artifact (strictness 10, editor opened, no failure), `probes-macos` auval. Save the logs under `execute-4.2/ci/`.
    - If it failed on MSVC / WebView2: fix in source (dsp-agent / gui-agent), rebuild, re-run 15 + 18 + 19, and ask Taylor before a second push + re-dispatch.
    - If C++ other than spec-table values changed after the pushed SHA (D1): ask Taylor, push, re-dispatch, read again.
    - Depends on: 8, 21.

### Wave 7 — bookkeeping and commit

23. **`SUMMARY-4.2.md`**: every reading with numbers; the sitting's results (checklist ticked, re-voices, comma call, French read); Windows run id + verdict; mutants table; deviations. **REQUIREMENTS.md**: FUNC-07, UI-05, PERF-02, COMPAT-02, QUAL-01 → `complete` only where the evidence is in hand (a declined push leaves COMPAT-02 `partial`, named). **STATUS.md**: execute 4.2 ✓, `next_action: stage_4_verify_4.2`. **PLUGINS.md** row: version and status.
    - Depends on: 22.
24. **Commit**, branch and staging re-checked immediately before:
    `git commit -- plugins/O-ParametricEQ PLUGINS.md`
    Message `phase: O-ParametricEQ 4-polish/execute 4.2 complete - …`. Gitignored `Source/ui/public/modules/*.js` stay out. **No tag.** Push only on a fresh go-ahead.
    - Depends on: 23.

---

## Dependency graph

```
W0: 1
W1: 1 → {2, 3, 4} ; {2,3} → 5 ; {4,5} → 6
W2: 6 → 7 → 8 (CI runs in background)
W3: 7 → 9 → STOP (Taylor)
W4: 9 → {10, 11, 12} → 13
W5: 13 → 14 → {15, 16, 17} ; 17 → 18 ; {11,14} → 19 ; {15,16,18,19} → 20 → 21
W6: {8, 21} → 22
W7: 22 → 23 → 24
```

---

## Success criteria

- [ ] FC1, FC2, FP5m PASS; V9, V10, V14 caught by the named rows; V12 caught by PN1 in a scratch root with no workaround; control clean.
- [ ] Taylor's sitting complete: every HANDS-ON-4.2 line ticked or recorded with a disposition; listening pass across 13 presets; comma call made.
- [ ] Re-voices (if any) in units, FC1 moved, FC2 unchanged (or changed by Taylor's named call); real `.bank-hash` equals the harness's.
- [ ] fr `reviewed: true` on every row; zh `'mt'`; five UI gates 0; PN1 PASS / PN1n fires; WebView2 static gate PASS.
- [ ] Fresh rebuild: designer 150 / 0 / 18; render 186 (+ HT) / 0 default and `--long`; sessions SHA-equal to 0.5.0 16 / 16; N1e `6274ff9b5b0e488a`; params golden byte-equal; G9 (b) and meter-ballistics PASS.
- [ ] `--g10` on a quiet machine: G10a ≤ 10 %, G10b ≤ 35 %, G10c ≤ 0.3 % and ≤ 1.5 × pass-through, G10d PASS, G10d0 PASS; G10i printed.
- [ ] Editor bench Release + Debug ALL-PASS on every verdict script; 0 assertions.
- [ ] Installed 0.11.0 (or 0.10.0 if no source change) `-dev` only, identical to artefacts; pluginval 10 VST3 / AU and auval pass; `Factory/` unchanged across them.
- [ ] Windows: `ci-tests.yml` run green for O-ParametricEQ (pluginval 10 VST3 on windows-latest), on a SHA whose C++ equals the final tree's — or COMPAT-02 explicitly left partial because Taylor declined the push.
- [ ] Path-scoped commit; no tag; any push only with a go-ahead given at that moment.

---

## Risks

| Risk | Level | Mitigation |
|---|---|---|
| Windows build fails (first MSVC compile of this plugin) | MEDIUM | Early push (D1) so the fix fits inside 4.2; the static gate already covers the WebView2 wiring; `ci-tests.yml` configures only the dispatched plugin. If the fix is not small, STOP and report (D5). |
| `probes-macos` steps assume O-Octagon / O-Strata test targets | LOW-MEDIUM | RESEARCH Q2: probe steps are gated on the input and skip. If the job fails for that reason alone, record it as a workflow defect, not a plugin failure; the Windows job is the COMPAT-02 evidence. |
| Push carries another session's commits | LOW | Show `git log origin/main..main` in the go-ahead question; Taylor sees exactly what goes. |
| G10d0 / G10 flake under load | MEDIUM | D6: run last and alone, one re-run, load average recorded. |
| Bench rows flake (locked screen, occlusion) | LOW-MEDIUM | lockstate + `quiet()` from the 4.1 bench; void and re-run a locked run. |
| A re-voice breaks a preset's defining feature | LOW | FC2 (independent of the pin). |
| Taylor's sitting finds a defect bigger than 4.2's scope | MEDIUM | D5: record, stop, plan a 4.3. |
| Sitting spans sessions; context lost | LOW | HANDS-ON-4.2.md holds the checklist and results; STATUS `next_action` set to `stage_4_execute_4.2_sitting` while waiting. |
| Other sessions' staging joins a commit | MEDIUM | Re-check before each of the two commits; pathspec commits; temp-index method. |

---

## After 4.2

`/plugin-verify O-ParametricEQ 4-polish` (verify 4.2; on VERIFIED, Stage 4 and the plugin's workflow are complete), then `/install-plugin O-ParametricEQ`. `/publish` stays a separate, later decision.
