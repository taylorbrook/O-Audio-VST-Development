---
phase: 24-propagate
plan: 08
type: execute
wave: 8
depends_on: [24-07]
files_modified:
  - .planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md
autonomous: false
requirements: [PROP-01, PROP-02, PROP-03, PROP-04, PROP-05, PROP-06, PROP-07, TRACK-05]

must_haves:
  truths:
    - "All 8 affected plugins (O-Lyrica + 7 Phase 24 propagation targets) freshly rebuilt and installed per CLAUDE.md — `~/Library/Audio/Plug-Ins/VST3/<Plugin>.vst3` and `~/Library/Audio/Plug-Ins/Components/<Plugin>.component` mtimes within last 5 minutes for all 8."
    - "All 8 pass `scripts/verify-au-link.sh <Plugin>` — auval validates each AU loads."
    - "`modules/registry.yaml` `note-expression.used_by:` list contains exactly 8 entries: OLyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant."
    - "Each of the 7 Phase 24 plugins re-passes the Dorico C4 quarter-sharp smoke (post-fresh-install regression check — fresh installs sometimes regress; D-07 mini-rerun)."
    - "`/module-info note-expression` output captured in 24-08-final-sweep-SUMMARY.md, showing all 8 consumers."
    - "Aggregate Dorico-test results table written to 24-08-final-sweep-SUMMARY.md — one row per plugin (8 rows) with 3-point gate result for each."
  artifacts:
    - path: "~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3"
      provides: "Freshly rebuilt+installed (regression sweep)"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3"
      provides: "Freshly rebuilt+installed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Prism.vst3"
      provides: "Freshly rebuilt+installed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Wind.vst3"
      provides: "Freshly rebuilt+installed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-IntonationPad.vst3"
      provides: "Freshly rebuilt+installed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Reed.vst3"
      provides: "Freshly rebuilt+installed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Bowed.vst3"
      provides: "Freshly rebuilt+installed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3"
      provides: "Freshly rebuilt+installed"
    - path: ".planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md"
      provides: "Phase 24 closeout — aggregate Dorico results table + registry audit + /module-info output"
      contains: "all 8"
  key_links:
    - from: "modules/registry.yaml note-expression.used_by"
      to: "8 plugin entries"
      via: "consumed by /module-add invocations during plans 24-01..07 (7 new) + Phase 23 (OLyrica)"
      pattern: "plugin: (OLyrica|O-Bells|O-IntonationPad|O-Prism|O-Wind|O-Reed|O-Bowed|O-Formant)"
    - from: "scripts/verify-au-link.sh"
      to: "all 8 plugins"
      via: "exit 0 from each invocation"
      pattern: "verify-au-link\\.sh.*PASS"
---

<objective>
Final sweep for Phase 24. After all 7 per-plugin propagations complete (plans 24-01..07), this plan:

1. **Re-builds and freshly re-installs all 8 affected plugins** (O-Lyrica + the 7 Phase 24 targets) per CLAUDE.md — AU cache cleared, old bundles removed, fresh `.vst3` and `.component` copied. Maps to TRACK-05 (every affected plugin freshly installed) and Success Criterion #4 (all 8 freshly installed).
2. **Validates `scripts/verify-au-link.sh` for all 8** — auval gate per plugin (D-30/D-31 inherited).
3. **Audits `modules/registry.yaml`** — `note-expression.used_by:` MUST list exactly all 8 (D-14 audit pass).
4. **Captures `/module-info note-expression` output** — confirms registry schema reflects 8 consumers.
5. **Re-runs Dorico C4 quarter-sharp smoke on each of the 8** (per CONTEXT.md "Whether 24-08 re-runs Dorico smoke on all 8" — recommended yes; fresh installs occasionally regress).
6. **Aggregates per-plugin SUMMARY results** into one closeout table for the phase verify gate.

This plan does NOT invoke `/improve` (no per-plugin version bump or CHANGELOG edit happens here — those landed in plans 24-01..07). It runs build + install + audit operations directly. Re-claims all 7 PROP requirements via the post-sweep regression smoke (each plugin still works after fresh install). TRACK-05 binds here.

Output: One commit landing the SUMMARY + any updates to `~/Library/Audio/Plug-Ins/` (binary not committed to git). Aggregate table is the phase verify-gate input.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/ROADMAP.md
@.planning/REQUIREMENTS.md
@.planning/phases/24-propagate/24-CONTEXT.md
@.planning/phases/24-propagate/24-INTEGRATION-MATRIX.md
@.planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md
@.planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md
@.planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md
@.planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md
@.planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md
@.planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md
@.planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md
@modules/registry.yaml
@CLAUDE.md
@scripts/verify-au-link.sh

<integrations>
This plan is the **closing checkpoint for Phase 24**. It does not modify plugin sources or registry — it validates the cumulative state.

The 8 plugins:
| # | Plugin | Final version | Plan |
|---|--------|---------------|------|
| 1 | OLyrica | 2.3.0 (Phase 23) | (Phase 23 23-04) |
| 2 | O-Bells | 4.1.0 | 24-01 |
| 3 | O-Prism | 1.17.0 | 24-02 |
| 4 | O-Wind | 1.16.0 | 24-03 |
| 5 | O-IntonationPad | 2.8.0 | 24-04 |
| 6 | O-Reed | 1.1.0 | 24-05 |
| 7 | O-Bowed | 1.3.0 | 24-06 |
| 8 | O-Formant | 1.25.0 | 24-07 |
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight — confirm all 7 prior plans closed cleanly</name>
  <read_first>
    - .planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md
    - .planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md
    - .planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md
    - .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md
    - .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md
    - .planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md
    - .planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md
    - modules/registry.yaml (verify `used_by:` already lists 8 entries — appended by /module-add inside each /improve cycle, D-14)
  </read_first>
  <action>
    1. Confirm all 7 SUMMARY.md files exist and each records a 3-point Dorico smoke PASS (or document any FAIL/triage from D-12 escalations).
    2. Confirm `modules/registry.yaml` `note-expression.used_by:` list contains exactly 8 entries:
       ```bash
       awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | awk '/used_by:/,/^[[:space:]]*[a-z]+:/' | grep -c 'plugin:'
       ```
       MUST equal 8 (OLyrica from Phase 23 + the 7 Phase 24 targets, each appended by their `/improve` cycle's `/module-add` invocation).
    3. Confirm git working tree clean.
    4. Confirm no in-flight 24-NN-fix-PLAN.md from D-12 escalations remain unresolved.
  </action>
  <verify>
    <automated>
      ls -1 .planning/phases/24-propagate/24-0[1-7]-*-SUMMARY.md | wc -l | grep -E '^7$' && \
      test "$(awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -cE '^      - plugin:')" = "8" && \
      git diff --quiet
    </automated>
  </verify>
  <acceptance_criteria>
    - All 7 per-plugin SUMMARY.md files present.
    - registry.yaml `note-expression.used_by:` has 8 entries.
    - Working tree clean.
    - No unresolved fix-plans.
  </acceptance_criteria>
  <done>Pre-sweep state validated.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Rebuild + freshly install all 8 plugins per CLAUDE.md</name>
  <read_first>
    - CLAUDE.md (Plugin Cache Clearing protocol — kill AudioComponentRegistrar, clear AudioUnitCache, remove old `.vst3` and `.component`, copy fresh)
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md (8-plugin list)
  </read_first>
  <action>
    Execute the canonical CLAUDE.md cache-clear + fresh-install sequence for ALL 8 plugins. This is the **all-8 sweep** mandated by Success Criterion #4 + TRACK-05.

    For each `Plugin` in `OLyrica O-Bells O-Prism O-Wind O-IntonationPad O-Reed O-Bowed O-Formant`:

    **Step A — Build (per plugin):**
    ```bash
    ninja -C build ${Plugin}_VST3 ${Plugin}_AU ${Plugin}_Standalone 2>&1 | tee /tmp/24-08-${Plugin}-build.log
    ```
    MUST exit 0; MUST NOT contain `Undefined symbols.*Steinberg::*`.

    **Step B — Cache clear (once before installs, per CLAUDE.md):**
    ```bash
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/
    rm -rf ~/Library/Caches/com.apple.audiounits.cache
    ```

    **Step C — Per-plugin uninstall + reinstall:**
    ```bash
    rm -rf ~/Library/Audio/Plug-Ins/VST3/${Plugin}.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/${Plugin}.component
    cp -R build/plugins/${Plugin}/${Plugin}_artefacts/Release/VST3/${Plugin}.vst3 ~/Library/Audio/Plug-Ins/VST3/
    cp -R build/plugins/${Plugin}/${Plugin}_artefacts/Release/AU/${Plugin}.component ~/Library/Audio/Plug-Ins/Components/
    ```

    **Failure handling:** If any plugin fails to build clean OR the install copy fails (e.g., source artefact missing), halt and triage. Phase 23 D-22..D-29 per-format convention should hold across all 8; failure here would indicate a regression in the convention itself (escalate to a fix-plan).

    Document each plugin's build/install completion in a working scratch table for use in Task 5 (SUMMARY aggregation).
  </action>
  <verify>
    <automated>
      for p in OLyrica O-Bells O-Prism O-Wind O-IntonationPad O-Reed O-Bowed O-Formant; do
        test -d ~/Library/Audio/Plug-Ins/VST3/$p.vst3 || { echo "MISSING VST3: $p"; exit 1; }
        test -d ~/Library/Audio/Plug-Ins/Components/$p.component || { echo "MISSING AU: $p"; exit 1; }
      done
      ! grep -rE 'Undefined symbols.*Steinberg::' /tmp/24-08-*-build.log 2>/dev/null
    </automated>
  </verify>
  <acceptance_criteria>
    - All 8 ninja builds exit 0 with no Steinberg undefined-symbol errors.
    - All 8 `~/Library/Audio/Plug-Ins/VST3/<Plugin>.vst3` exist with mtimes within last 30 minutes (cap allows the full sweep duration).
    - All 8 `~/Library/Audio/Plug-Ins/Components/<Plugin>.component` exist with recent mtimes.
    - AU cache cleared before installs (per CLAUDE.md).
  </acceptance_criteria>
  <done>All 8 plugins freshly installed.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: AU verify gate for all 8 — `scripts/verify-au-link.sh` PASS</name>
  <read_first>
    - scripts/verify-au-link.sh
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md (8-plugin list)
  </read_first>
  <action>
    Run the AU verify gate on all 8 plugins:
    ```bash
    for p in OLyrica O-Bells O-Prism O-Wind O-IntonationPad O-Reed O-Bowed O-Formant; do
        echo "=== verify-au-link.sh $p ==="
        bash scripts/verify-au-link.sh "$p" 2>&1 | tee /tmp/24-08-$p-auval.log
    done
    ```
    Each invocation MUST exit 0 (auval validates each AU loads). The script auto-parses PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / AU type from each plugin's CMakeLists.

    Also confirm `auval -a` lists each:
    ```bash
    auval -a 2>/dev/null | grep -iE 'OLyrica|O.Bells|O.Prism|O.Wind|O.IntonationPad|O.Reed|O.Bowed|O.Formant'
    ```
    MUST return ≥8 lines.

    Document results per plugin in scratch table for Task 5.
  </action>
  <verify>
    <automated>
      for p in OLyrica O-Bells O-Prism O-Wind O-IntonationPad O-Reed O-Bowed O-Formant; do
        bash scripts/verify-au-link.sh "$p" >/dev/null 2>&1 || { echo "FAIL: $p"; exit 1; }
      done
      test "$(auval -a 2>/dev/null | grep -ciE 'OLyrica|O.Bells|O.Prism|O.Wind|O.IntonationPad|O.Reed|O.Bowed|O.Formant')" -ge 8
    </automated>
  </verify>
  <acceptance_criteria>
    - All 8 `verify-au-link.sh` invocations exit 0.
    - `auval -a` lists ≥8 matching plugins.
  </acceptance_criteria>
  <done>AU verify gate PASS for all 8.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 4: Registry audit + /module-info capture (D-14)</name>
  <read_first>
    - modules/registry.yaml
  </read_first>
  <action>
    1. Audit `modules/registry.yaml` `note-expression.used_by:` list — MUST contain exactly 8 entries with these plugin names (order doesn't matter):
       - OLyrica
       - O-Bells
       - O-IntonationPad
       - O-Prism
       - O-Wind
       - O-Reed
       - O-Bowed
       - O-Formant

       Verify via:
       ```bash
       awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin:' | sort
       ```
       Compare output to expected list; any missing entry → halt and re-run the missing plugin's `/module-add note-expression` (planning-time guard against incomplete prior plans).

    2. Run `/module-info note-expression` and capture the full output. Pipe into `/tmp/24-08-module-info.txt` for inclusion in SUMMARY.md.

    3. Verify the registry version of `note-expression` is still `1.0.0` (D-33: no module bump during Phase 24).
  </action>
  <verify>
    <automated>
      test "$(awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -cE '^      - plugin:')" = "8" && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: OLyrica$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: O-Bells$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: O-IntonationPad$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: O-Prism$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: O-Wind$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: O-Reed$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: O-Bowed$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^      - plugin: O-Formant$' && \
      awk '/^  - name: note-expression/,/^  - name:/{print}' modules/registry.yaml | grep -E '^    version: 1\.0\.0$'
    </automated>
  </verify>
  <acceptance_criteria>
    - `note-expression.used_by:` lists exactly 8 entries with the names above.
    - Module version unchanged at 1.0.0 (D-33).
    - `/module-info note-expression` output captured for SUMMARY inclusion.
  </acceptance_criteria>
  <done>Registry audit PASS.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 5: Dorico C4 quarter-sharp regression smoke on all 8 — human-verified</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07 (3-point gate; D-10 SUMMARY format)
    - All 7 per-plugin SUMMARY.md files (carry-forward Dorico observations from initial smoke)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    All 8 affected plugins freshly rebuilt and reinstalled per CLAUDE.md. AU verify gate PASS for each. Registry audit PASS. This is the post-sweep regression check — fresh installs occasionally regress; D-07 mini-rerun confirms the system state is good for phase verify.
  </what-built>
  <how-to-verify>
    Run a **quick** quarter-sharp C4 smoke on each of the 8 plugins. This is a regression check, not the full 3-point gate from each per-plugin plan — just confirm that after fresh install, each plugin still produces the expected microtonal response.

    **Procedure (per plugin, ~1 minute each):**
    1. Open Dorico Pro 5+.
    2. Add an instrumental staff with the plugin assigned. Set Microtonality on the expression map to "VST3 Note Expression".
    3. Notate a single quarter-sharp C4. Play.
    4. Verify pitch lands at +50¢ above C4 (~269.29 Hz).
    5. Record PASS/FAIL with observed Hz in the working table.

    **Order (suggested for efficiency — keep one Dorico project, swap instrument):**
    OLyrica → O-Bells → O-Prism → O-Wind → O-IntonationPad → O-Reed → O-Bowed → O-Formant.

    Total time: ~10-15 minutes for all 8.

    If any FAIL after a previously PASSing per-plugin smoke, this is a regression — escalate to triage. Likely causes: (a) AU cache stale despite clear (re-clear and retry), (b) fresh install copied a stale artefact (re-build and re-install).

    Type `approved` once all 8 PASS regression smoke. If any FAIL, describe which plugin and observed pitch.
  </how-to-verify>
  <resume-signal>Type `approved` if all 8 regression PASS; else describe FAILing plugin(s) and observed pitch values.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 6: Write 24-08-final-sweep-SUMMARY.md — phase closeout</name>
  <read_first>
    - All 7 per-plugin SUMMARY.md files (24-01..07)
    - .planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md (LYR-03 PASS reference for OLyrica row)
    - $HOME/.claude/get-shit-done/templates/summary.md
    - Working scratch tables from Tasks 2, 3, 5
    - /tmp/24-08-module-info.txt (Task 4 capture)
  </read_first>
  <action>
    Create `.planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md` with the following structure:

    ### Section 1: Header
    Plan id, phase, completion date, atomic commit SHA (the SUMMARY.md commit, not /improve transactions).

    ### Section 2: Requirements Closed
    - PROP-01..07 (re-claimed via post-sweep regression smoke — each plugin still PASSes Dorico C4 quarter-sharp after fresh install).
    - TRACK-05 (all 8 plugins freshly rebuilt + installed per CLAUDE.md; binds here).

    ### Section 3: Aggregate Plugin Results Table
    Markdown table — one row per plugin (8 rows):

    ```markdown
    | # | Plugin | Final Ver | Plan | Build | Install | verify-au-link | Initial Smoke | Regression Smoke | Notes |
    |---|--------|-----------|------|-------|---------|----------------|---------------|------------------|-------|
    | 1 | OLyrica | 2.3.0 | (Phase 23) | PASS | PASS | PASS | PASS (LYR-03) | PASS | reference/canary |
    | 2 | O-Bells | 4.1.0 | 24-01 | PASS | PASS | PASS | PASS | PASS | TuningEngine compose; float→double cast |
    | ... | ... | ... | ... | ... | ... | ... | ... | ... | ... |
    ```

    Fill in actual results from the working scratch tables. Each cell has PASS or specific FAIL detail.

    ### Section 4: Registry Audit
    - `note-expression.used_by:` list snapshot showing all 8 entries.
    - Module version still 1.0.0 (D-33 confirmed).
    - `/module-info note-expression` output (paste from /tmp/24-08-module-info.txt).

    ### Section 5: Build Sweep Stats
    - Total plugins built: 8.
    - Total ninja invocations: 24 (3 formats × 8 plugins).
    - Total Steinberg undefined-symbol errors: 0 (D-22..D-29 per-format convention held).

    ### Section 6: Dorico Smoke Sweep Stats
    - Total smoke tests run: 8 (one per plugin during this plan; 8 + 7 prior = 15 cumulative for Phase 24 if we count per-plugin plan smokes).
    - Pass rate: X/8.
    - Any FAILs: itemize with cause + remediation.

    ### Section 7: Anomalies / Carry-Forward Notes for Phase Verify
    - Any anomalies observed during the sweep (cache flakiness, install-script edge cases, Dorico version-specific quirks).
    - Anything that should propagate into Phase 25 planning context (e.g., installer-side considerations for the .doricoexpmap; DOCS-01..05 hooks).

    ### Section 8: Phase 24 Closeout
    Statement: "Phase 24 (Propagate) complete. 7 per-plugin propagations + 1 final sweep landed. All 8 affected plugins freshly installed and Dorico-microtonal-functional. Ready for `/gsd-verify-phase 24`."
  </action>
  <verify>
    <automated>
      test -f .planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md && \
      grep -E 'PROP-0[1-7]' .planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md && \
      grep -E 'TRACK-05' .planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md && \
      grep -iE 'all 8' .planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md && \
      grep -E 'OLyrica|O-Bells|O-IntonationPad|O-Prism|O-Wind|O-Reed|O-Bowed|O-Formant' .planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md | wc -l | awk '$1 >= 8 {exit 0} {exit 1}'
    </automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY.md exists at canonical path.
    - References PROP-01..07 + TRACK-05.
    - Aggregate table has 8 rows (one per plugin).
    - Registry audit + /module-info output included.
    - Phase closeout statement present.
  </acceptance_criteria>
  <done>Phase 24 complete; SUMMARY committed; ready for `/gsd-verify-phase 24`.</done>
</task>

</tasks>

<verification>
- All 7 per-plugin SUMMARY.md files exist with PASS results.
- modules/registry.yaml `note-expression.used_by:` contains exactly 8 entries (OLyrica + 7 Phase 24 targets).
- Module version still 1.0.0 (D-33 honored).
- All 8 ninja builds exit 0; no Steinberg undefined-symbol errors.
- All 8 `~/Library/Audio/Plug-Ins/VST3/<Plugin>.vst3` and `<Plugin>.component` mtimes within last 30 min.
- All 8 `scripts/verify-au-link.sh <Plugin>` exit 0.
- All 8 Dorico C4 quarter-sharp regression smoke PASS.
- `24-08-final-sweep-SUMMARY.md` aggregates all 8 results in one table.
</verification>

<success_criteria>
**Phase 24 success criteria (from ROADMAP.md §Phase 24):**
1. ✅ All 7 target plugins consume the microtonal module (registry `used_by:` confirms).
2. ✅ Each of the 7 plugins passes Dorico quarter-sharp smoke (initial smoke per plan + regression sweep).
3. ✅ Every Phase 24 rollout traceable to `/improve [PluginName]` cycle (TRACK-01 enforced).
4. ✅ All 8 affected plugins rebuilt and freshly installed per CLAUDE.md (this plan's Tasks 2-3 + the regression smoke validation).
5. ✅ Each plan named `/improve [PluginName]` as execution mechanism; STATUS.md reflects integration.

PROP-01..07 + TRACK-01..05 all closed. Phase ready for `/gsd-verify-phase 24`.
</success_criteria>

<output>
- `.planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md` (new)
- All 8 freshly installed bundles in `~/Library/Audio/Plug-Ins/{VST3,Components}/`
- One commit landing the SUMMARY.md (the binary install state is not committed)
- Phase 24 ready for verification gate.
</output>
