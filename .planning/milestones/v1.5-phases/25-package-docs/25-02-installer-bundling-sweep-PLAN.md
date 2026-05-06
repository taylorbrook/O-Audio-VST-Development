---
phase: 25-package-docs
plan: 02
type: execute
wave: 2
depends_on: [25-01]
files_modified:
  - .claude/skills/plugin-packaging/references/pkg-creation.md
  - .claude/skills/plugin-packaging/assets/inno-template.iss
  - .claude/skills/plugin-packaging/references/inno-setup-creation.md
  - .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md
  - .planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md
autonomous: false
requirements: [INST-03, INST-04]
tags: [installer, pkg, inno-setup, dorico, doricolib, atomic-sweep, path-b]

must_haves:
  truths:
    - "Per-plugin packaging mechanisms for the 8-plugin cohort are audited and confirmed to consume the shared template references (or, where they fork, the fork is captured and explicitly addressed)."
    - "PKG postinstall (shared template) writes the canonical .doricolib + README to ~/Library/Application Support/Ouaricon/Microtonal Suite/ on every plugin install."
    - "Inno Setup template writes the canonical .doricolib + README to %APPDATA%\\Ouaricon\\Microtonal Suite\\ on every plugin install."
    - "All 8 plugins' freshly-built installers, when installed, produce a working Path B import flow on macOS and Windows."
    - "Cross-platform validation gate (D-08) passes: 1 representative install on macOS + 1 on Windows confirms Library Manager Import + quarter-sharp ~269 Hz."
  artifacts:
    - path: ".planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md"
      provides: "Per-plugin packaging consumption audit (8-plugin × 2-platform mapping table); identifies any forked templates that BLOCK Tasks 1-3 from proceeding"
    - path: ".claude/skills/plugin-packaging/references/pkg-creation.md"
      provides: "Shared PKG postinstall reference; new section copies .doricolib + README to Ouaricon shared path"
      contains: "Ouaricon/Microtonal Suite"
    - path: ".claude/skills/plugin-packaging/assets/inno-template.iss"
      provides: "Shared Inno Setup template; new [Files] entries for .doricolib + README"
      contains: "Ouaricon-VST3-NoteExpression.doricolib"
    - path: ".claude/skills/plugin-packaging/references/inno-setup-creation.md"
      provides: "Shared Inno Setup reference; new template variables for the two suite paths"
    - path: ".planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md"
      provides: "Cross-platform install + Library Manager Import + quarter-sharp results"
  key_links:
    - from: "PKG postinstall script"
      to: "~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib"
      via: "cp + chown ${ACTUAL_USER}:staff"
      pattern: "Ouaricon/Microtonal Suite"
    - from: "Inno Setup [Files] section"
      to: "%APPDATA%\\Ouaricon\\Microtonal Suite\\Ouaricon-VST3-NoteExpression.doricolib"
      via: "{userappdata}\\Ouaricon\\Microtonal Suite — Flags: ignoreversion"
      pattern: "userappdata.*Ouaricon"
    - from: "any of 8 freshly-built installers"
      to: "Library Manager Import + quarter-sharp playback"
      via: "PKG/EXE install → user runs Library Manager Import once → assigns map per-channel"
      pattern: "xmap.ouaricon.vst3_note_expression"
---

<objective>
Audit how each of the 8 cohort plugins actually consumes the shared installer templates, then extend the shared PKG postinstall reference and Inno Setup template so every PKG/EXE installer built across the 8-plugin cohort bundles the canonical `.doricolib` + user-facing README, lands them at the Ouaricon shared path on the target platform, and works with a one-time Library Manager Import in Dorico. Then build fresh installers for all 8 plugins on whichever platforms are accessible and run the cross-platform validation gate (D-08): a representative install on macOS + a representative install on Windows.

Purpose: Plan 25-01 ships the canonical asset and module-level install rule, but those only fire when running `cmake --install` directly. End users install plugins via PKG (macOS) or EXE (Windows) — those installer pipelines need to bundle and write the suite asset themselves. This plan modifies the shared installer templates so all 8 plugins' next installer rebuild picks up the new bundling; it does NOT redesign the per-plugin packaging workflow (the existing `plugin-packaging` and `build-installer` skills remain authoritative). The new Task 0 preflight ensures we know which plugins consume the shared templates vs. fork them BEFORE editing the shared sources.

Output: Per-plugin packaging consumption audit, two shared template files modified (the mechanical sweep), 8 plugin installers rebuilt + validated, and a cross-platform validation matrix recording PASS/FAIL per plugin per platform per gate point. v1.5 ships when this plan's matrix shows the required PASSes (D-08).

Per D-09: Plan 25-02 v3 is dramatically smaller than v2 — no Pascal `[Code]` Dorico-version detection, no `Default Library Additions` directory creation, no spaces-vs-no-spaces variance handling. One destination per platform.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/phases/25-package-docs/25-CONTEXT.md
@.planning/phases/25-package-docs/25-01-author-and-install-collapse-PLAN.md
@.planning/phases/24-propagate/24-08-final-sweep-PLAN.md
@.claude/skills/plugin-packaging/SKILL.md
@.claude/skills/plugin-packaging/SKILL-windows.md
@.claude/skills/plugin-packaging/references/pkg-creation.md
@.claude/skills/plugin-packaging/references/inno-setup-creation.md
@.claude/skills/plugin-packaging/assets/inno-template.iss
@.claude/skills/build-installer/SKILL.md
@CLAUDE.md

<placeholder_convention>
This plan introduces two new substitution tokens consumed by per-plugin packaging at packaging time. The choice between literal-placeholder (sed-substituted) vs. shell-variable (env-resolved) conventions was made based on Task 0 preflight findings (when run) and the existing `pkg-creation.md` convention.

**Decision (made at revision time, informed by audit of `pkg-creation.md` lines 1-216):**

The existing pkg-creation.md template uses **two coexisting conventions**:
1. **Sed-substituted literal placeholders** — `PLUGIN_NAME_PLACEHOLDER`, `PRODUCT_NAME_PLACEHOLDER` — used for values that vary per-plugin (e.g., the postinstall heredoc references `/tmp/PLUGIN_NAME_PLACEHOLDER/...` and is sed-substituted by lines 212-213 of the existing reference into `${PLUGIN_NAME}` / `${PRODUCT_NAME}`).
2. **Shell variables** — `$HOME`, `${PRODUCT_NAME}`, `${PLUGIN_NAME}`, `${TEMP_DIR}` — used for values that are part of the orchestrating script's own runtime context.

For the **project-root path** (where the canonical `.doricolib` lives in the repo), the value is part of the orchestrating script's runtime context (it is wherever the packaging command is invoked from), NOT a per-plugin variant. Therefore, **option (b) — shell variable convention** is the correct choice:

- Use `${PROJECT_ROOT}` (a shell variable, not a sed-substituted literal placeholder).
- The orchestrating per-plugin packaging script (or the `/package` skill invocation) MUST `export PROJECT_ROOT="$(git rev-parse --show-toplevel)"` (or equivalent) BEFORE invoking the steps from `pkg-creation.md` Section 4a.
- Document this requirement explicitly in `pkg-creation.md` Section 4a as a precondition (see Task 1 action).
- The Task 0 preflight audit verifies whether existing per-plugin scripts already set `PROJECT_ROOT`, or whether the `/package` skill is invoked from the repo root such that `pwd` resolves correctly.

This decision ALSO aligns with the existing inno-setup-creation.md template-variable convention: `{{VST3_SOURCE_PATH}}` is resolved at packaging time by the per-plugin PowerShell script that invokes the substitution. The new `{{MICROTONAL_SUITE_DORICOLIB_PATH}}` and `{{MICROTONAL_SUITE_README_PATH}}` follow the same pattern (Task 2).

**Rationale for not picking option (a) — `PROJECT_ROOT_PLACEHOLDER` sed-substitution:** A literal placeholder would require every per-plugin script to re-implement the sed substitution rule. A shell variable is naturally inherited from the invoking shell context. Lower coupling to per-plugin scripts.
</placeholder_convention>

<interfaces>
<!-- Canonical asset paths (created by Plan 25-01) -->
- modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib  (6,431 B Dorico-valid)
- modules/tuning/note-expression/resources/README-microtonal-suite.txt                     (Path B user-facing fallback)

<!-- Build-tree paths (after `ninja <Plugin>_VST3 <Plugin>_AU`) -->
- build/plugins/<Plugin>/<Plugin>_artefacts/Release/VST3/<Plugin>{,-dev}.vst3
- build/plugins/<Plugin>/<Plugin>_artefacts/Release/AU/<Plugin>{,-dev}.component   (macOS only)

<!-- Install destinations (Path B, Plan 25-01 D-07) -->
- macOS PKG postinstall:  ~/Library/Application Support/Ouaricon/Microtonal Suite/
- Windows Inno Setup:     %APPDATA%\Ouaricon\Microtonal Suite\

<!-- 8-plugin cohort -->
OLyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant

<!-- Existing shared template structure (unchanged surfaces — extend, do not redesign) -->
PKG (pkg-creation.md):
- Section 4a "Copy Binaries to Payload" copies VST3+AU into $TEMP_DIR/payload/${PLUGIN_NAME}/
- Section 4b "Create Postinstall Script" emits a /bin/bash heredoc with ACTUAL_USER detection,
  mkdir -p Plug-Ins dirs, cp from /tmp/PLUGIN_NAME_PLACEHOLDER/, chown -R ACTUAL_USER:staff,
  rm -rf /tmp/PLUGIN_NAME_PLACEHOLDER, exit 0.
- Placeholders replaced via sed -i ''  s/PLUGIN_NAME_PLACEHOLDER and PRODUCT_NAME_PLACEHOLDER

Inno Setup (inno-template.iss):
- [Files] section contains: Source: "{{VST3_SOURCE_PATH}}\*"; DestDir: "{commonpf}\Common Files\VST3\{#MyAppName}.vst3"
- [Code] section has CurStepChanged(ssPostInstall) that logs Ableton dir
- Template variables in {{double-curlies}} replaced by build-installer skill PowerShell

Plan 25-02 EXTENDS:
- pkg-creation.md Section 4a: copy 2 additional files into $TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite/
  using shell variable `${PROJECT_ROOT}` to locate the source files (set by orchestrating script)
- pkg-creation.md Section 4b heredoc: append a new "Microtonal Suite" block AFTER existing chown lines and BEFORE rm -rf /tmp/...
- inno-template.iss [Files]: append 2 lines for .doricolib + README (using `{{MICROTONAL_SUITE_*_PATH}}` substituted by per-plugin PowerShell)
- inno-setup-creation.md: document 2 new template variables (paths to canonical asset + README)
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 0: Preflight — audit how each of the 8 cohort plugins consumes the shared packaging templates</name>
  <files>.planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md</files>
  <read_first>
    - .claude/skills/plugin-packaging/SKILL.md (macOS PKG flow; the `/package` invocation)
    - .claude/skills/plugin-packaging/SKILL-windows.md (Windows EXE flow)
    - .claude/skills/plugin-packaging/references/pkg-creation.md (shared PKG postinstall reference; THIS is what we want all 8 plugins to consume)
    - .claude/skills/plugin-packaging/assets/inno-template.iss (shared Inno Setup template)
    - .claude/skills/build-installer/SKILL.md (Windows installer build CLI)
    - plugins/O-Lyrica/ (full directory listing — look for dist/, packaging/, scripts/, installer/ subdirs)
    - plugins/O-Bells/, plugins/O-IntonationPad/, plugins/O-Prism/, plugins/O-Wind/, plugins/O-Reed/, plugins/O-Bowed/, plugins/O-Formant/ (same: enumerate any per-plugin packaging script paths)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-06 single bundled asset; carry-forward note that all 8 plugins use the existing plugin-packaging skill)
  </read_first>
  <action>
    This preflight audit confirms the load-bearing assumption of Tasks 1-3: that editing the shared template files cascades to all 8 plugins. If any plugin has forked the postinstall heredoc or the `.iss` template into a per-plugin copy, that fork breaks the cascade and must be flagged BEFORE we touch the shared templates.

    **Step A — Enumerate per-plugin packaging mechanisms (macOS PKG):**

    For each of the 8 plugins, identify how its PKG is built. Run:
    ```bash
    for p in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
        echo "=== $p ==="
        find "plugins/$p" -maxdepth 4 -type f \( \
            -name "package*.sh" -o -name "package*.bash" -o -name "build-pkg*" -o \
            -name "postinstall" -o -name "Makefile" -o -name "package.json" \
        \) 2>/dev/null
        find "plugins/$p" -maxdepth 4 -type d \( \
            -name "dist" -o -name "packaging" -o -name "installer" -o -name "scripts" \
        \) 2>/dev/null
    done
    ```

    For each plugin, classify into ONE of three categories:
    - **(a) Consumes shared reference directly** — no per-plugin packaging script; `/package` skill or equivalent reads `.claude/skills/plugin-packaging/references/pkg-creation.md` at packaging time. (Expected default.)
    - **(b) Has per-plugin orchestrator** — there is a `package.sh` or similar in `plugins/<Plugin>/` that DOES read the shared reference (sources or includes it). This is fine; document the path.
    - **(c) Has forked postinstall heredoc** — there is a `package.sh` or similar that contains its OWN copy of the postinstall heredoc (not sourcing the shared reference). **THIS IS A BLOCKER.**

    For category (c), grep specifically:
    ```bash
    for p in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
        # Look for forked postinstall logic — if any of these strings appear in plugins/$p/, the plugin forks
        grep -rn 'cat > .*postinstall\|cp -R "/tmp/.*\.vst3"\|chown -R "$ACTUAL_USER:staff"' "plugins/$p/" 2>/dev/null
    done
    ```

    **Step B — Enumerate per-plugin packaging mechanisms (Windows EXE / Inno Setup):**

    ```bash
    for p in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
        echo "=== $p ==="
        find "plugins/$p" -maxdepth 4 -type f \( -name "*.iss" -o -name "build-installer*" \) 2>/dev/null
    done
    ```

    Repeat the (a)/(b)/(c) classification for Windows. A category-(c) plugin would have a per-plugin `.iss` that does NOT generate-from-template (i.e., it has hardcoded `[Files]` entries that must be hand-edited).

    **Step C — Capture evidence per plugin:**

    For each plugin in each platform, capture:
    - The category (a / b / c)
    - The path of the consuming script (or "none" for category a)
    - A 3-line excerpt from the consuming script showing how it references the shared template (or "n/a" for category a)

    Write the result as a markdown table to `.planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md`:

    ```markdown
    # Plan 25-02 Preflight: Per-Plugin Packaging Consumption Audit

    Date: <YYYY-MM-DD>
    Auditor: <executor>

    ## Summary

    | Plugin | macOS Category | macOS Path | Windows Category | Windows Path |
    |--------|---------------|------------|------------------|--------------|
    | O-Lyrica | a / b / c | <path or "none"> | a / b / c | <path or "none"> |
    | ... |

    Total category-(c) BLOCKERS: <N>

    ## macOS PKG details

    ### O-Lyrica
    - Category: <a / b / c>
    - Path: <path or "none">
    - Excerpt (3 lines, if not category a):
      ```
      <3 lines from the consuming script>
      ```

    ### O-Bells
    - ...

    (repeat for all 8)

    ## Windows EXE details

    (same shape)

    ## Verdict

    - All 8 plugins are category (a) or (b): GREEN — proceed to Tasks 1-3 unmodified.
    - One or more plugins are category (c): YELLOW/RED — surface to user. User chooses:
      - (i) Refactor the forked plugin(s) to consume the shared reference (out of scope for Plan 25-02 → escalate to a `25-02-NN-fix-PLAN.md` per D-11).
      - (ii) Accept that Tasks 1-3 must ALSO edit each forked copy (expand `files_modified` and adjust acceptance criteria for Tasks 1-3).
      - (iii) Defer the forked plugin(s) to v1.6 with explicit `DEFER-N-PLUGINS-TO-v1.6` rationale recorded.
    ```

    **Step D — Report counts:**

    Capture and report:
    ```bash
    # Total relevant packaging files across the cohort
    find plugins/O-Lyrica plugins/O-Bells plugins/O-IntonationPad plugins/O-Prism \
         plugins/O-Wind plugins/O-Reed plugins/O-Bowed plugins/O-Formant \
         -maxdepth 4 -type f \( -name "*.iss" -o -name "package*.sh" -o -name "package*.ps1" \) 2>/dev/null | wc -l
    # Document the count in the audit; note expected count based on classification
    ```

    Stage as a single atomic commit `chore(25-02): preflight audit of per-plugin packaging consumption mechanisms`.

    **GATE (BLOCKER if violated):**

    If ANY plugin lands in category (c) — i.e., has a forked copy of the postinstall heredoc OR a hand-edited `.iss` template that does NOT consume the shared template — the plan halts here. The executor:
    1. Records the finding in `25-02-PREFLIGHT-AUDIT.md`
    2. Halts Plan 25-02 (does NOT proceed to Tasks 1-3)
    3. Surfaces the finding to the user via the resume-signal `preflight-blocker [details]`
    4. User decides: refactor (escalate to fix-PLAN per D-11), expand-scope (edit forked copies in Tasks 1-3), or defer (record `DEFER-N-PLUGINS-TO-v1.6`)

    **GATE (proceed if violated only on category (b)):**

    Category (b) is FINE — those plugins have orchestrators that consume the shared reference. No special handling needed; the orchestrator picks up the shared template edits automatically. Document the path for traceability.
  </action>
  <verify>
    <automated>
      test -f .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q '^## Summary' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q '^## Verdict' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-Lyrica' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-Bells' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-IntonationPad' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-Prism' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-Wind' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-Reed' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-Bowed' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -q 'O-Formant' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md && \
      grep -qE 'Total category-\(c\) BLOCKERS: [0-9]+' .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `.planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md` exists with `## Summary`, `## macOS PKG details`, `## Windows EXE details`, and `## Verdict` H2 sections
    - Summary table has one row per cohort plugin (8 rows): O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant
    - For each plugin per platform, the table records: category (a / b / c), path of consuming script (or "none" for category a)
    - For every category-(b) plugin, a 3-line excerpt is captured in the platform-detail section showing how it references the shared template
    - For every category-(c) plugin (if any), a 3-line excerpt is captured showing the forked code, AND the plugin appears in the "Total category-(c) BLOCKERS" count line in the Summary
    - The audit reports the output of `find plugins -maxdepth 4 -type f ... | wc -l` (the total relevant-packaging-files count) so future audits can detect drift
    - The Verdict section explicitly states one of: GREEN (all (a)/(b); proceed), YELLOW (some (c) but refactor planned), RED (some (c); user must decide)
    - **GATE:** If Verdict is RED, the plan halts at this task; Tasks 1-3 do not run until the user provides the resume-signal `preflight-resolved [decision: refactor / expand-scope / defer]` which is then incorporated into Tasks 1-3 as appropriate
    - Atomic commit `chore(25-02): preflight audit of per-plugin packaging consumption mechanisms` modifies only `25-02-PREFLIGHT-AUDIT.md`
  </acceptance_criteria>
  <done>
    Audit complete; per-plugin packaging consumption mechanism classified for all 8 plugins on both platforms; verdict recorded; user has confirmed the path forward (GREEN → proceed to Tasks 1-3; YELLOW/RED → user-chosen handling applied to Tasks 1-3 scope).
  </done>
</task>

<task type="auto">
  <name>Task 1: Extend shared PKG postinstall (single source of truth across 8 plugins) for Path B suite copy</name>
  <files>.claude/skills/plugin-packaging/references/pkg-creation.md</files>
  <read_first>
    - .claude/skills/plugin-packaging/references/pkg-creation.md (full file; understand Section 4a/4b structure before editing)
    - .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md (Task 0 output; confirms category (a)/(b) cascade is sound; informs whether forked copies must also be edited)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-06 single bundled asset; D-07 single-write to Ouaricon shared path)
    - .planning/phases/25-package-docs/25-01-author-and-install-collapse-PLAN.md (Task 4 README content; install destination strings)
    - CLAUDE.md (postinstall must run as root but write user-owned files via chown -R ACTUAL_USER:staff)
  </read_first>
  <action>
    Edit `.claude/skills/plugin-packaging/references/pkg-creation.md` to extend the shared PKG postinstall with Microtonal Suite copy logic.

    **Edit 0 — Document the `${PROJECT_ROOT}` precondition at the top of Section 4a:**

    Add a new paragraph at the start of Section 4a, BEFORE the existing `cp -R` lines:

    ```markdown
    **Precondition (Phase 25 v3 Path B):** The orchestrating per-plugin packaging script (or the `/package` skill invocation) MUST set the `PROJECT_ROOT` shell variable to the absolute path of the Ouaricon plugin source repo BEFORE invoking the steps below. Recommended:

    ```bash
    : "${PROJECT_ROOT:=$(git rev-parse --show-toplevel)}"
    export PROJECT_ROOT
    ```

    The Microtonal Suite asset copy (this section, plus Section 4b's postinstall heredoc) reads from `${PROJECT_ROOT}/modules/tuning/note-expression/resources/...`. If `PROJECT_ROOT` is unset, the cp will fail and the PKG build will halt.
    ```

    **Edit 1 — Extend Section 4a "Copy Binaries to Payload":**
    Append a new sub-block AFTER the existing two `cp -R` lines (the ones that copy VST3 + AU). Add:

    ```bash
    # Microtonal Suite asset (Phase 25 v3 Path B) — bundled in every PKG built
    # against a plugin that consumes the note-expression module.
    # Uses ${PROJECT_ROOT} shell variable (see Section 4a precondition above).
    mkdir -p "$TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite"
    cp "${PROJECT_ROOT}/modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib" \
       "$TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite/"
    cp "${PROJECT_ROOT}/modules/tuning/note-expression/resources/README-microtonal-suite.txt" \
       "$TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite/"
    ```

    Use the `${PROJECT_ROOT}` shell variable convention (per the existing `pkg-creation.md` style — see lines 86-89 / 173-174 which use `$HOME` and `${PRODUCT_NAME}` shell vars). Do NOT use a literal `<PROJECT_ROOT>` placeholder; that would require a sed-substitution rule that doesn't exist.

    **Edit 2 — Extend Section 4b postinstall heredoc:**
    Insert a NEW block of bash AFTER the existing `chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Audio/Plug-Ins/Components/PRODUCT_NAME_PLACEHOLDER.component"` line (currently around line 202) and BEFORE the `# Clean up temp files` / `rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"` lines (currently around line 204-205).

    Insert this block:

    ```bash

    # Microtonal Suite (Phase 25 v3 Path B): copy canonical .doricolib + README
    # to the Ouaricon shared path. User performs a one-time Library Manager
    # Import in Dorico per machine. No Dorico auto-discovery write.
    SUITE_DIR="$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite"
    mkdir -p "$SUITE_DIR"
    cp "/tmp/PLUGIN_NAME_PLACEHOLDER/microtonal-suite/Ouaricon-VST3-NoteExpression.doricolib" "$SUITE_DIR/"
    cp "/tmp/PLUGIN_NAME_PLACEHOLDER/microtonal-suite/README-microtonal-suite.txt" "$SUITE_DIR/"
    chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Application Support/Ouaricon"
    echo "[Ouaricon] Microtonal Suite installed at: $SUITE_DIR"
    echo "[Ouaricon] To activate in Dorico: Library -> Library Manager -> Import... -> select Ouaricon-VST3-NoteExpression.doricolib"
    ```

    Place the new block exactly between the existing chown lines and the cleanup. Sed-placeholder substitution (`PLUGIN_NAME_PLACEHOLDER` → `${PLUGIN_NAME}` etc.) already occurs after the heredoc; the new block uses `PLUGIN_NAME_PLACEHOLDER` consistently with the rest of the script and gets substituted automatically.

    **Edit 3 — Add a section header above the new Section 4a sub-block:**
    Add a small inline subsection header `#### Microtonal Suite asset (Phase 25 v3 Path B)` right above the new block, with a one-paragraph explanation: "Plugins that consume the `note-expression` module ship the canonical Dorico expression-map library bundle alongside the VST3+AU artefacts. The asset is single-source-of-truth at `modules/tuning/note-expression/resources/library/`. The postinstall script lands it at `~/Library/Application Support/Ouaricon/Microtonal Suite/` on the user system; one-time Library Manager Import in Dorico activates it (D-01)."

    Stage as a single atomic commit `docs(25-02): extend PKG postinstall shared template for Path B suite (INST-03)`.
  </action>
  <verify>
    <automated>
      grep -q 'microtonal-suite' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'Ouaricon-VST3-NoteExpression.doricolib' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'README-microtonal-suite.txt' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'Library/Application Support/Ouaricon/Microtonal Suite' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'Library Manager.*Import' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q '\${PROJECT_ROOT}' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      grep -q 'git rev-parse --show-toplevel' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      ! grep -q '<PROJECT_ROOT>' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      ! grep -q 'PROJECT_ROOT_PLACEHOLDER' .claude/skills/plugin-packaging/references/pkg-creation.md && \
      ! grep -q 'PlaybackTemplateSpecs\|Default Library Additions\|dorico_pt' .claude/skills/plugin-packaging/references/pkg-creation.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `pkg-creation.md` Section 4a contains a precondition paragraph documenting that `PROJECT_ROOT` MUST be set by the orchestrating script (mentions `git rev-parse --show-toplevel` as the canonical resolution)
    - `pkg-creation.md` Section 4a contains a sub-block referencing `microtonal-suite` directory under payload
    - `pkg-creation.md` Section 4a copies BOTH `Ouaricon-VST3-NoteExpression.doricolib` AND `README-microtonal-suite.txt` from the module's `resources/` tree into the payload, using the `${PROJECT_ROOT}` shell variable to resolve source paths
    - `pkg-creation.md` does NOT contain literal `<PROJECT_ROOT>` (angle-bracket placeholder) — only `${PROJECT_ROOT}` shell variable form
    - `pkg-creation.md` does NOT contain `PROJECT_ROOT_PLACEHOLDER` (sed-substitution placeholder form was rejected per `<placeholder_convention>`)
    - `pkg-creation.md` Section 4b postinstall heredoc contains a `SUITE_DIR` variable resolving to `$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite`
    - `pkg-creation.md` Section 4b postinstall heredoc contains exactly 2 `cp` lines copying from `/tmp/PLUGIN_NAME_PLACEHOLDER/microtonal-suite/` into `$SUITE_DIR/`
    - `pkg-creation.md` Section 4b postinstall heredoc contains a `chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Application Support/Ouaricon"` line (ownership fix per CLAUDE.md root-vs-user pattern)
    - `pkg-creation.md` Section 4b heredoc echoes the canonical activation hint mentioning `Library Manager` and `Import`
    - The new postinstall block is placed BEFORE the `rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"` cleanup line (verified by line ordering — `grep -n` shows SUITE_DIR appears before `rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"`)
    - `pkg-creation.md` does NOT contain Path A strings: `PlaybackTemplateSpecs`, `Default Library Additions`, `dorico_pt`, `EndpointConfigs`
    - Atomic commit `docs(25-02): extend PKG postinstall shared template for Path B suite (INST-03)` modifies only this file
  </acceptance_criteria>
  <done>
    Shared PKG postinstall reference describes how to bundle and install the Microtonal Suite asset on macOS, with explicit `${PROJECT_ROOT}` shell-variable convention documented as a precondition. All 8 plugins' next PKG rebuild picks up the change automatically (assuming Task 0 verdict was GREEN; otherwise scope expanded per Task 0's user-chosen handling).
  </done>
</task>

<task type="auto">
  <name>Task 2: Extend shared Inno Setup template + reference (single source of truth across 8 plugins) for Path B suite copy</name>
  <files>
    .claude/skills/plugin-packaging/assets/inno-template.iss
    .claude/skills/plugin-packaging/references/inno-setup-creation.md
  </files>
  <read_first>
    - .claude/skills/plugin-packaging/assets/inno-template.iss (full file; understand current [Files] + [Code] structure)
    - .claude/skills/plugin-packaging/references/inno-setup-creation.md (full file; understand template-variable substitution pattern)
    - .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md (Task 0 output; informs whether forked .iss copies must also be edited)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-06 single bundled asset; D-07 single-write to %APPDATA%\Ouaricon\Microtonal Suite\)
    - .planning/phases/25-package-docs/25-01-author-and-install-collapse-PLAN.md (Task 1 + Task 4 — canonical asset path; README path)
  </read_first>
  <action>
    Two coordinated edits across the Inno Setup shared template files.

    **Edit 1 — `.claude/skills/plugin-packaging/assets/inno-template.iss`:**

    In the existing `[Files]` block, AFTER the existing `Source: "{{VST3_SOURCE_PATH}}\*"; ...` line (currently line 49), append two new lines:

    ```iss
    ; Microtonal Suite (Phase 25 v3 Path B) — bundled with every plugin install
    ; Lands at %APPDATA%\Ouaricon\Microtonal Suite\ for one-time Dorico Library Manager Import.
    Source: "{{MICROTONAL_SUITE_DORICOLIB_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
    Source: "{{MICROTONAL_SUITE_README_PATH}}"; DestDir: "{userappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
    ```

    The existing `[Code]` block's `CurStepChanged(ssPostInstall)` procedure can be extended to log the suite install location for the user. Update the existing procedure body (currently logs Ableton plugin rescan) to ALSO log the Microtonal Suite hint:

    ```iss
    [Code]
    procedure CurStepChanged(CurStep: TSetupStep);
    var
      AbletonDir: String;
    begin
      if CurStep = ssPostInstall then
      begin
        // Clear Ableton plugin cache (optional, non-fatal)
        AbletonDir := ExpandConstant('{userappdata}\Ableton');
        if DirExists(AbletonDir) then
        begin
          Log('Ableton preferences directory found - plugin rescan will occur on next launch');
        end;
        // Microtonal Suite (Phase 25 v3 Path B) installed via [Files]; user activates via:
        //   Dorico -> Library -> Library Manager -> Import... -> select Ouaricon-VST3-NoteExpression.doricolib
        Log('[Ouaricon] Microtonal Suite installed at: ' + ExpandConstant('{userappdata}\Ouaricon\Microtonal Suite'));
        Log('[Ouaricon] Activate in Dorico via Library -> Library Manager -> Import...');
      end;
    end;
    ```

    Do NOT add: a `function ExtractZipTo(...)` (no zip extraction needed under Path B), a `for V := 6 downto 4 do` loop (no Dorico-version probe), any `ForceDirectories(DoricoDir + ...)` calls (no auto-discovery write).

    **Edit 2 — `.claude/skills/plugin-packaging/references/inno-setup-creation.md`:**

    Find Section 3 (the section that documents the template-variable substitution PowerShell). Add a new subsection 3.x titled "Microtonal Suite template variables (Phase 25 v3 Path B)" documenting two new template variables:

    - `{{MICROTONAL_SUITE_DORICOLIB_PATH}}` — absolute path to `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` at packaging time. Resolve from project root.
    - `{{MICROTONAL_SUITE_README_PATH}}` — absolute path to `modules/tuning/note-expression/resources/README-microtonal-suite.txt` at packaging time.

    Provide a 4-5-line PowerShell snippet showing how the per-plugin packaging script resolves these to absolute paths and substitutes them in the template (mirroring the existing `{{VST3_SOURCE_PATH}}` substitution pattern). Example:

    ```powershell
    $repoRoot = Resolve-Path "${PSScriptRoot}\..\..\..\.."  # adjust depth per script location
    $suiteDoricolib = "$repoRoot\modules\tuning\note-expression\resources\library\Ouaricon-VST3-NoteExpression.doricolib"
    $suiteReadme = "$repoRoot\modules\tuning\note-expression\resources\README-microtonal-suite.txt"
    $issContent = $issContent -replace '\{\{MICROTONAL_SUITE_DORICOLIB_PATH\}\}', $suiteDoricolib
    $issContent = $issContent -replace '\{\{MICROTONAL_SUITE_README_PATH\}\}', $suiteReadme
    ```

    Also add a one-paragraph note: "Plugins that consume the `note-expression` module bundle the canonical Dorico expression-map library bundle. The asset lands at `%APPDATA%\Ouaricon\Microtonal Suite\` on user install. The user performs a one-time `Library → Library Manager → Import…` per machine to activate the map. No Dorico auto-discovery directory is written; the install destination is Dorico-version-agnostic (D-07)."

    Stage as a single atomic commit `docs(25-02): extend Inno Setup shared template + reference for Path B suite (INST-03)`.
  </action>
  <verify>
    <automated>
      grep -c 'MICROTONAL_SUITE_DORICOLIB_PATH' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -c 'MICROTONAL_SUITE_README_PATH' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -q 'userappdata.*Ouaricon.*Microtonal Suite' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -q 'ignoreversion' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      ! grep -q 'function ExtractZipTo\|for V := 6 downto\|ForceDirectories\|DefaultLibraryAdditions\|dorico_pt' .claude/skills/plugin-packaging/assets/inno-template.iss && \
      grep -q 'MICROTONAL_SUITE_DORICOLIB_PATH' .claude/skills/plugin-packaging/references/inno-setup-creation.md && \
      grep -q 'MICROTONAL_SUITE_README_PATH' .claude/skills/plugin-packaging/references/inno-setup-creation.md && \
      grep -q 'Library Manager' .claude/skills/plugin-packaging/references/inno-setup-creation.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `inno-template.iss` `[Files]` section contains exactly one line matching `MICROTONAL_SUITE_DORICOLIB_PATH` with `DestDir: "{userappdata}\Ouaricon\Microtonal Suite"` and `Flags: ignoreversion`
    - `inno-template.iss` `[Files]` section contains exactly one line matching `MICROTONAL_SUITE_README_PATH` with same `DestDir` + `ignoreversion`
    - `inno-template.iss` `[Code]` `CurStepChanged` procedure contains 2 new `Log(...)` calls referencing the Microtonal Suite path and the Library Manager Import activation hint
    - `inno-template.iss` does NOT contain Path A strings: `function ExtractZipTo`, `for V := 6 downto`, `ForceDirectories`, `DefaultLibraryAdditions`, `dorico_pt`, `PlaybackTemplateSpecs`
    - `inno-setup-creation.md` documents both template variables with descriptions and a PowerShell substitution example
    - `inno-setup-creation.md` contains a paragraph explaining the Path B import flow (`Library Manager` + `Import` mentioned)
    - Atomic commit `docs(25-02): extend Inno Setup shared template + reference for Path B suite (INST-03)` modifies exactly these 2 files
  </acceptance_criteria>
  <done>
    Shared Inno Setup template + reference describe how to bundle and install the Microtonal Suite asset on Windows. All 8 plugins' next EXE rebuild picks up the change automatically (assuming Task 0 verdict was GREEN).
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 3: Cross-platform validation gate (D-08) — 1 representative install per platform + quarter-sharp smoke + 8-plugin matrix with per-plugin evidence</name>
  <files>.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md</files>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-08 cross-platform validation gate; canary on O-Lyrica per Phase 23 precedent; phase goal: ALL 8 plugins bundle the asset)
    - CLAUDE.md (build + install commands; cache-clear protocol — mandatory)
    - .claude/skills/plugin-packaging/SKILL.md (macOS PKG build flow; the `/package` command)
    - .claude/skills/plugin-packaging/SKILL-windows.md (Windows EXE build flow)
    - .claude/skills/build-installer/SKILL.md (Windows installer build CLI)
    - .planning/phases/24-propagate/24-08-final-sweep-PLAN.md (atomic-sweep shape; per-plugin task table reference)
    - modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib (canonical asset Plan 25-01 created)
    - .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md (Plan 25-01 canary log; this validation extends that pattern)
    - .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md (Task 0 output)
  </read_first>
  <action>Execute the human-verified cross-platform validation gate described in <how-to-verify>. The task pauses for the user to rebuild installers across the 8-plugin cohort, install the canary representative on each accessible platform, run Dorico Library Manager Import, and run the 3-point smoke gate. Results are recorded in the validation matrix file with **per-plugin evidence (PKG output path, sha256, .doricolib bytecount inside the PKG payload)** for each of the 8 plugins. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    Tasks 1+2 modified two shared template files. Tasks 0 confirmed cascade. Those templates feed every per-plugin installer build via the existing `plugin-packaging`/`build-installer` skills. The shipped feature only works if (a) the templates are correct, (b) per-plugin installers are rebuilt, and (c) running an installer on a clean target system produces a working end-to-end import flow. This checkpoint exercises (b) and (c).

    **Phase 25 v3 goal (CONTEXT.md In-Scope):** "All 8 affected plugins' installer configs updated to bundle and write the file (mechanical sweep)." A 1/8 PASS verdict is NOT phase-goal completion. Per-plugin evidence is required for each of the 8 plugins, not just the canary.
  </what-built>
  <how-to-verify>
    Two phases. Run on the dev machine; coordinate with the user for Windows access if not present.

    Append all results to `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` (NEW file). Use a single matrix table per platform with **per-plugin evidence rows** (path, sha256, bytecount inside payload).

    ### Phase A — macOS sweep + canary

    **A1. Rebuild macOS PKG installer for O-Lyrica (representative; Phase 23 precedent):**
    ```bash
    # Pre-reqs: clean build of OLyrica VST3+AU per CLAUDE.md
    cd /Users/taylorbrook/Dev/VST-development
    : "${PROJECT_ROOT:=$(git rev-parse --show-toplevel)}"
    export PROJECT_ROOT
    ninja -C build OLyrica_VST3 OLyrica_AU
    ```
    Then trigger PKG packaging for O-Lyrica via the `/package` skill or its Bash equivalent (whichever the team uses for individual plugin packaging — the user-facing slash command is `/package O-Lyrica` per `.claude/skills/package/SKILL.md`). The build script must consume the updated `pkg-creation.md` reference; if Task 0 surfaced a fork, use the user-chosen handling (refactor / expand-scope / defer). Capture: PKG output path (e.g., `plugins/O-Lyrica/dist/O-Lyrica-OuariconAudio.pkg`).

    **A2. Clean target environment:**
    ```bash
    # Cache clear per CLAUDE.md
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
    rm -rf ~/Library/Audio/Plug-Ins/VST3/OLyrica*.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/OLyrica*.component
    # Suite slate
    rm -rf "$HOME/Library/Application Support/Ouaricon/Microtonal Suite"
    ```

    **A3. Install the freshly-built PKG:**
    ```bash
    sudo installer -pkg plugins/O-Lyrica/dist/O-Lyrica-OuariconAudio.pkg -target /
    ```
    Expected console: pkg-creation log lines including the new `[Ouaricon] Microtonal Suite installed at: ...` echo.

    **A4. Verify suite asset landed at Ouaricon shared path:**
    ```bash
    ls -la "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/"
    stat -f%z "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib"
    # Must show 6431 bytes
    ```
    Verify ownership: file owner is `$USER`, not `root`.

    **A5. Dorico Library Manager Import + quarter-sharp gate:**
    Repeat the procedure from Plan 25-01 Task 5 (Library Manager Import → assign expression map → quarter-sharp ~269 Hz on a chord with quarter-sharp C4). All 3 gate points (per Phase 24 D-07): pitch ~269 Hz, no attack zipper, polyphonic isolation (other notes play 12-TET).

    Record in matrix: PKG built, install succeeded, suite landed (size + owner), Library Manager Import PASS, quarter-sharp gate PASS/FAIL.

    **A6. Bulk PKG sweep with per-plugin evidence (remaining 7 plugins):**

    Per Phase 24 atomic-sweep shape, rebuild PKG for each remaining plugin sequentially **AND capture per-plugin evidence** (PKG path + sha256 + .doricolib bytecount inside the PKG payload). Per-plugin evidence is required for ALL 8 plugins; matrix rejects "skipped" / "DEFERRED" cells.

    ```bash
    : "${PROJECT_ROOT:=$(git rev-parse --show-toplevel)}"
    export PROJECT_ROOT

    for p in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
        # Map plugin slug to ninja target (OLyrica is the lone CamelCase exception)
        case "$p" in
            O-Lyrica) target="OLyrica" ;;
            *) target="$p" ;;
        esac

        ninja -C build ${target}_VST3 ${target}_AU
        # Run /package $p (or its non-interactive equivalent — capture output path)

        pkg_path="plugins/$p/dist/${p}-OuariconAudio.pkg"
        if [ ! -f "$pkg_path" ]; then
            echo "MISSING PKG for $p at $pkg_path — gate FAIL"
            exit 1
        fi

        # Per-plugin evidence
        pkg_sha=$(shasum -a 256 "$pkg_path" | awk '{print $1}')

        # Extract .doricolib from PKG payload to verify it was bundled
        tmp=$(mktemp -d)
        pkgutil --expand-full "$pkg_path" "$tmp/expanded" 2>/dev/null || \
            xar -xf "$pkg_path" -C "$tmp"   # fallback if pkgutil refuses signed pkg
        # Find the .doricolib in the expanded payload
        doricolib_in_pkg=$(find "$tmp" -name "Ouaricon-VST3-NoteExpression.doricolib" 2>/dev/null | head -1)
        if [ -z "$doricolib_in_pkg" ]; then
            echo "FAIL: .doricolib NOT found in $pkg_path payload — gate FAIL"
            rm -rf "$tmp"
            exit 1
        fi
        doricolib_bytes=$(stat -f%z "$doricolib_in_pkg")
        rm -rf "$tmp"

        echo "$p | $pkg_path | $pkg_sha | $doricolib_bytes B"
    done
    ```

    For each plugin, the bulk-sweep evidence row in the matrix MUST contain:
    - PKG output path (string)
    - PKG sha256 (full hash)
    - `.doricolib` bytecount inside the PKG payload (must equal 6431)

    Spot-check: install one of the non-canary PKGs (any of the 7) and verify the suite copy landed (running A4 step). Stop-on-first-failure per D-11 — escalate any structural failure to `25-02-NN-fix-PLAN.md`.

    ### Phase B — Windows sweep + canary

    **B1. Determine Windows access:**
    Check whether a Windows dev environment is currently available. If NOT (e.g., user only has macOS access today), record `Windows: ACCESS-BLOCKED` in the matrix. Per D-08: if Windows access is blocked, this plan halts hard — don't silently degrade to macOS-only. The user MUST decide whether to (a) defer Plan 25-02 closeout until Windows access is available, or (b) accept a documented "macOS-only validated" risk for v1.5 ship and explicitly punt Windows validation to v1.6.

    If Windows IS available:

    **B2. Rebuild Windows EXE installer for O-Lyrica:**
    ```powershell
    # On the Windows host:
    cd C:\path\to\VST-development
    cmake --build build --config Release --target OLyrica_VST3 --parallel
    # Trigger build-installer for O-Lyrica (per .claude/skills/build-installer/SKILL.md)
    ```
    The build-installer script must consume the updated `inno-template.iss` and `inno-setup-creation.md`. Capture: EXE output path.

    **B3. Clean target environment:**
    ```powershell
    Remove-Item -Recurse -Force "$env:COMMONPROGRAMFILES\VST3\OLyrica*.vst3" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$env:APPDATA\Ouaricon\Microtonal Suite" -ErrorAction SilentlyContinue
    Remove-Item "$env:APPDATA\Ableton\*\PluginScanDb.txt" -Force -ErrorAction SilentlyContinue
    ```

    **B4. Install the freshly-built EXE silently or with UI:**
    ```powershell
    .\plugins\O-Lyrica\dist\O-Lyrica-OuariconAudio-Setup.exe /SILENT
    # or interactive: .\plugins\O-Lyrica\dist\O-Lyrica-OuariconAudio-Setup.exe
    ```

    **B5. Verify suite asset landed:**
    ```powershell
    Get-ChildItem "$env:APPDATA\Ouaricon\Microtonal Suite\"
    (Get-Item "$env:APPDATA\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib").Length
    # Must show 6431
    ```

    **B6. Dorico Library Manager Import + quarter-sharp gate (Windows):**
    Repeat A5 in Dorico Windows. Same 3-point gate. Record PASS/FAIL.

    **B7. Bulk EXE sweep with per-plugin evidence (remaining 7 plugins):**
    Mirror A6 on Windows: capture EXE path + sha256 + `.doricolib` bytecount inside the EXE payload (use Inno Setup's `/EXTRACT` flag or 7-Zip to read the EXE contents). Spot-check one of the 7 by installing + suite-asset check (B5 step).

    ### Matrix output

    Create `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` with this structure:

    ```markdown
    # Plan 25-02 Cross-Platform Validation Matrix

    Date: <YYYY-MM-DD>
    Dorico version: <version>
    macOS host: <version>
    Windows host: <version-or-ACCESS-BLOCKED>

    ## macOS PKG Sweep — Per-plugin evidence (8 plugins)

    | Plugin | PKG built (Y/N) | PKG path | PKG sha256 | .doricolib bytes in payload | Install OK | Library Mgr Import | 3-point gate |
    |--------|-----------------|----------|------------|-----------------------------|------------|--------------------|--------------|
    | O-Lyrica (canary) | Y | <path> | <hash> | 6431 | Y | PASS | PASS |
    | O-Bells | Y | <path> | <hash> | 6431 | (canary spot-check; Y if spot-checked) | n/a (canary covers) | n/a |
    | O-IntonationPad | Y | <path> | <hash> | 6431 | n/a | n/a | n/a |
    | O-Prism | Y | <path> | <hash> | 6431 | n/a | n/a | n/a |
    | O-Wind | Y | <path> | <hash> | 6431 | n/a | n/a | n/a |
    | O-Reed | Y | <path> | <hash> | 6431 | n/a | n/a | n/a |
    | O-Bowed | Y | <path> | <hash> | 6431 | n/a | n/a | n/a |
    | O-Formant | Y | <path> | <hash> | 6431 | n/a | n/a | n/a |

    Required: every plugin row has PKG built = Y, a non-empty PKG path, a non-empty sha256, and `.doricolib bytes in payload = 6431`. Cells reading "DEFERRED" / "skipped" / "n/a" in the "PKG built" or ".doricolib bytes" columns are REJECTED.

    ## Windows EXE Sweep — Per-plugin evidence (8 plugins)

    (same shape; mark ACCESS-BLOCKED row if applicable)

    ## Verdict

    macOS: PASS / FAIL
    Windows: PASS / FAIL / ACCESS-BLOCKED
    Overall: PASS / FAIL / DEFER-WINDOWS-TO-v1.6 / DEFER-N-PLUGINS-TO-v1.6 [<plugin-list>] [<rationale>]

    ### Strict acceptance:
    - PASS = all 8 plugins (per-platform per-row) have PKG/EXE built = Y AND .doricolib bytes in payload = 6431, AND canary row has Library Mgr Import = PASS AND 3-point gate = PASS, AND (both platforms PASS).

    ### Explicit-deferral escape valves (only if user types matching resume-signal):
    - DEFER-WINDOWS-TO-v1.6: macOS PASS, Windows ACCESS-BLOCKED, user explicitly accepts.
    - DEFER-N-PLUGINS-TO-v1.6 [<plugin-list>] [<rationale>]: <N> plugins deferred from this milestone (each named explicitly in plugin-list); rationale field required. Remaining (8 - N) plugins must still PASS strict acceptance.

    No silent / implicit deferrals are permitted. Cells with "DEFERRED" without a corresponding explicit verdict line are matrix failures.
    ```
  </how-to-verify>
  <acceptance_criteria>
    - `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` exists with the documented structure
    - Per-plugin evidence rows for ALL 8 plugins on macOS:
      - O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant
      - Each row: `PKG built` column = `Y` (NOT `DEFERRED`, NOT `skipped`, NOT empty)
      - Each row: non-empty `PKG path` column (string)
      - Each row: non-empty `PKG sha256` column (full hash)
      - Each row: `.doricolib bytes in payload` column = `6431`
    - macOS row "O-Lyrica (canary)" additionally has Install OK = Y, Library Mgr Import = PASS, 3-point gate = PASS
    - Suite landed verification shows file size 6,431 bytes on macOS (`stat -f%z`)
    - Suite file owner on macOS is `$USER` (NOT `root`) — verifies the postinstall chown step worked
    - If Windows accessible: per-plugin evidence rows for all 8 plugins on Windows (EXE built, EXE path, EXE sha256, .doricolib bytes in payload = 6431), AND Windows row "O-Lyrica (canary)" has EXE built, Install OK, Suite landed (size 6,431), Library Mgr Import = PASS, 3-point gate = PASS
    - If Windows blocked: matrix records `ACCESS-BLOCKED` and the Verdict line records the user's explicit decision
    - **The Verdict line is one of the following ONLY** (silent / implicit deferrals are REJECTED — must match a documented escape valve):
      - `PASS` — all 8 plugins on both platforms PASS strict acceptance
      - `FAIL` — any required gate fails (escalates to fix-plan per D-11)
      - `DEFER-WINDOWS-TO-v1.6` — macOS strict-PASS for all 8 plugins; Windows ACCESS-BLOCKED; user explicitly accepts
      - `DEFER-N-PLUGINS-TO-v1.6 [<plugin-list>] [<rationale>]` — `<N>` plugins explicitly deferred (named in list, rationale field non-empty); the remaining `(8 - N)` plugins on each in-scope platform must PASS strict acceptance
    - **The matrix REJECTS** cells with `DEFERRED` / `skipped` / `n/a` in the "PKG built" or ".doricolib bytes in payload" columns UNLESS the corresponding plugin appears in the Verdict line's `DEFER-N-PLUGINS-TO-v1.6` plugin-list
    - On any structural FAIL: a `25-02-NN-fix-PLAN.md` is created per D-11 stop-on-first-failure protocol; main plan halts pending fix-plan completion
  </acceptance_criteria>
  <resume-signal>Type "matrix-pass" if all 8 plugins on both platforms PASS strict acceptance; "matrix-defer-windows" with user-decision rationale if Windows is blocked and v1.6-deferral is accepted (with macOS strict-PASS for all 8); "matrix-defer-plugins [O-X,O-Y,...] [rationale]" if specific plugins are explicitly deferred to v1.6 (must list plugin slugs); or "matrix-fail [details]" to escalate to a fix-plan per D-11.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Build host → user system (PKG/EXE) | Installer payload is signed (PKG productsign on macOS; Inno Setup signature optional on Windows). Plan 25-02 does not change signature posture; the existing `plugin-packaging` skill is authoritative for codesigning. |
| Postinstall script → user filesystem | macOS postinstall runs as root with elevated privileges (`sudo installer`). Writes to `$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite/`. |
| Inno Setup [Files] copy → user filesystem | Runs in installer process context (typically admin per existing `PrivilegesRequired=admin`). Writes to `%APPDATA%\Ouaricon\Microtonal Suite\`. |
| Shared template files (in-repo) | Single source of truth for all 8 plugins' installers. Tampering or accidental edit affects all 8 next rebuilds. Mitigation: existing PR review + atomic commit per task. |
| Per-plugin orchestrator → shared template | Task 0 preflight verifies cascade integrity. Forked copies (category c) are flagged as BLOCKERS. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-02-01 | Tampering | Shared `pkg-creation.md` and `inno-template.iss` are single-source-of-truth across all 8 plugins; an unintended edit cascades | mitigate | Each task is one shared-template edit, atomic commit, with grep-verifiable acceptance criteria pinning the expected strings (Microtonal Suite path, ignoreversion flag, Library Manager hint). Reviewer can diff to confirm scope. Task 0 preflight verifies cascade integrity. |
| T-25-02-02 | Elevation of Privilege | macOS postinstall runs as root and writes user-owned files; if `$USER_HOME` were attacker-controlled, files could land outside intended dir | mitigate | `$USER_HOME` is derived from `eval echo ~$ACTUAL_USER` where `ACTUAL_USER` comes from `stat -f '%Su' /dev/console` (existing pattern). Both are resolved in-script from system state, not user input. The `chown -R "$ACTUAL_USER:staff"` step ensures correct ownership transfer. ASVS L1 path-traversal mitigation: hard-coded subdir name `Ouaricon/Microtonal Suite` (no user-controlled segment). |
| T-25-02-03 | Tampering | `.doricolib` could be tampered between authoring (Plan 25-01) and packaging (this plan) | mitigate | Plan 25-01 committed the canonical bytes to the repo. Packaging reads from `${PROJECT_ROOT}/modules/tuning/note-expression/resources/library/`, the same in-repo path. Task 3's per-plugin evidence rows include sha256 of the PKG/EXE — drift detectable. PKG payload includes the file as-is from repo; PKG signing covers the bundle. Inno Setup [Files] reads the same path; EXE is signed (existing process). |
| T-25-02-04 | Information Disclosure | Postinstall script logs (echo lines) printed to installer log — could leak user paths | accept | Logged paths are well-known canonical locations (`$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite`, `%APPDATA%\Ouaricon\Microtonal Suite`). No PII. Standard installer behavior. |
| T-25-02-05 | Denial of Service | Per-plugin installer rebuild could fail across the cohort if a plugin has a broken `note-expression` consumption (regression from earlier phases) | mitigate | Plan 25-01 Task 5 already validates O-Lyrica works; Phase 24 closed with all 8 plugins passing. Per-plugin rebuild that fails is detected at PKG/EXE build time, not at install time. Stop-on-first-failure (D-11) escalates structural failures to fix plans. Task 0 preflight catches forked copies before edits propagate. |
| T-25-02-06 | Repudiation | The cross-platform validation gate is human-driven (canary install + Library Manager Import + audible smoke) | accept | This is the same architecture as Phase 24's per-plugin Dorico smoke gate. Audit-trail is the matrix file with per-plugin evidence (path + sha256 + payload bytecount); the gate is the milestone-defining shipped behavior, validated by the human user (visual + auditory). Automated alternatives are deferred (CONTEXT.md "Automated Dorico smoke harness" deferred). |
| T-25-02-07 | Tampering | Inno Setup `Flags: ignoreversion` causes overwrite without version check — if an attacker placed a malicious file at `%APPDATA%\Ouaricon\Microtonal Suite\` before install, our install would NOT detect it | accept | Pre-install state of `%APPDATA%\Ouaricon\Microtonal Suite\` is user-controlled; if compromised, the entire user environment is compromised. ASVS L1 scope is the installer payload itself, which is signed. `ignoreversion` is the standard Inno Setup pattern for non-versioned static assets. |
| T-25-02-08 | Tampering | `${PROJECT_ROOT}` could be unset or attacker-controlled in the orchestrating shell environment | mitigate | Task 1 documents the precondition that `PROJECT_ROOT` MUST be set to `git rev-parse --show-toplevel` (a git-derived value, not user-input). If unset, the cp fails immediately and the PKG build halts. Task 0 preflight verifies the orchestrating script sets PROJECT_ROOT correctly. |

**Severity:** All HIGH severity threats (Tampering of shared templates, Tampering of `.doricolib` in transit, EoP via path manipulation) mitigated. MEDIUM/LOW threats accepted.

</threat_model>

<verification>
- Task 0 preflight audit captured per-plugin packaging mechanism for all 8 cohort plugins; Verdict GREEN/YELLOW/RED documented
- `.claude/skills/plugin-packaging/references/pkg-creation.md` extended; `${PROJECT_ROOT}` shell-variable convention documented as precondition; postinstall block writes 2 files to `~/Library/Application Support/Ouaricon/Microtonal Suite/` with correct chown
- `.claude/skills/plugin-packaging/assets/inno-template.iss` extended; 2 new `[Files]` entries write to `%APPDATA%\Ouaricon\Microtonal Suite\` with `ignoreversion`
- `.claude/skills/plugin-packaging/references/inno-setup-creation.md` documents 2 new template variables with PowerShell substitution example
- macOS validation: O-Lyrica PKG built, install lands the asset (6,431 B, user-owned), Library Manager Import PASS, quarter-sharp ~269 Hz PASS, no attack zipper, polyphonic isolation works
- macOS bulk sweep: all 8 plugins have PKG built + sha256 + .doricolib bytecount in payload = 6431 (per-plugin evidence rows complete)
- Windows validation: same end-to-end (or explicit `DEFER-WINDOWS-TO-v1.6` user decision)
- Validation matrix file recorded in `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` with strict acceptance verdict
</verification>

<success_criteria>
1. Task 0 preflight audit confirms per-plugin packaging cascade integrity (Verdict GREEN); OR Task 0 surfaces forked copies and user has chosen handling (refactor / expand-scope / defer).
2. Both shared templates (`pkg-creation.md` postinstall + `inno-template.iss` [Files]/[Code]) extended with Path B suite copy logic, using `${PROJECT_ROOT}` shell-variable convention.
3. `inno-setup-creation.md` documents the 2 new template variables for the per-plugin packaging script substitution.
4. All 8 plugins have freshly-built PKG (and EXE if Windows accessible) installers that bundle the suite asset, with per-plugin evidence (path + sha256 + .doricolib bytecount in payload = 6431) recorded for each.
5. Cross-platform validation gate (D-08) passes on macOS + (Windows or explicit deferral): canary plugin install lands the asset, Library Manager Import PASS, quarter-sharp ~269 Hz, no attack zipper, polyphonic isolation works.
6. Validation matrix file documents the per-platform per-plugin per-gate-point result with strict-acceptance Verdict; no silent/implicit deferrals allowed.
7. No Path A residue in shared templates: zero matches for `dorico_pt`, `PlaybackTemplateSpecs`, `Default Library Additions`, `DefaultLibraryAdditions`, `EndpointConfigs` in modified files.
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-02-SUMMARY.md` documenting:
- Task 0 preflight audit verdict (GREEN/YELLOW/RED) and any forked-template handling decisions
- The two shared-template edits (single-source-of-truth across 8 plugins)
- Per-plugin installer rebuild status (built/skipped/failed) with per-plugin evidence (path + sha256 + .doricolib bytecount)
- Cross-platform validation matrix verdict (PASS / FAIL / DEFER-WINDOWS-TO-v1.6 / DEFER-N-PLUGINS-TO-v1.6)
- Any in-flight escalations to fix plans (per D-11)
- Final ship-readiness gate: this plan + Plan 25-03 close v1.5 Phase 25.
</output>
</content>
</invoke>