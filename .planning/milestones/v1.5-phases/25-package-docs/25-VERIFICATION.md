---
phase: 25-package-docs
verified: 2026-04-27T00:00:00Z
status: human_needed
score: 5/5
overrides_applied: 0
must_haves_total: 5
must_haves_verified: 5
requirement_ids_satisfied: [INST-01, INST-02, INST-03, INST-04, DOCS-01, DOCS-02, DOCS-03, DOCS-04, DOCS-05]
human_verification:
  - test: "Retest Windows installer on a non-admin Windows account with UAC elevation via a separate admin credential"
    expected: "doricolib lands at the launching user's %APPDATA%\\Ouaricon\\Microtonal Suite\\ (not the admin's roaming profile)"
    why_human: "BL-01 from 25-REVIEW.md: {userappdata} under PrivilegesRequired=admin resolves to the elevated process's APPDATA. The D-08 canary ran on a single-account dev box where launcher == admin, masking the divergence. Requires a two-account Windows environment to observe."
---

# Phase 25: package-docs Verification Report

**Phase Goal:** Single canonical `Ouaricon-VST3-NoteExpression.doricolib` authored, owned by the note-expression module, bundled in all 8 cohort plugin installers, installed to a platform-specific Ouaricon shared path, activated via one-time Library Manager Import, with internal developer-reference notes at `research/microtonal-dorico-integration.md`.
**Verified:** 2026-04-27
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Canonical `.doricolib` exists, Dorico-valid, 48-container kScoreLibrary skeleton + injected ExpressionMapDefinition, 6,431 B | VERIFIED | File at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`: `wc -c` = 6431; `xmllint --noout` exits 0; `xmllint --xpath 'count(/kScoreLibrary/*)'` = 48; 1× `xmap.ouaricon.vst3_note_expression`; 1× `kVST3NoteExpression`; `<pluginNames />` present (CID-free); `<creator>Ouaricon Audio</creator>` present |
| 2 | CMake install logic single-write to platform-specific Ouaricon shared path | VERIFIED | `install-microtonal-suite.cmake.in`: no Path A strings (`SUITE_PT`, `tar xf`, `foreach.*_v`, `Default Library Additions`, `Steinberg/Dorico`); exactly 2 non-comment `file(COPY)` lines; `if(APPLE)` branch targets `~/Library/Application Support/Ouaricon/Microtonal Suite`; `elseif(WIN32)` branch targets `$ENV{APPDATA}/Ouaricon/Microtonal Suite`; `module.cmake` wires it via `configure_file(...@ONLY)` + `install(SCRIPT)` per-consumer |
| 3 | All 8 cohort plugins' installers bundle the canonical `.doricolib` + README | VERIFIED | `pkg-creation.md` contains Microtonal Suite payload-copy block and SUITE_DIR postinstall block; `inno-template.iss` contains `MICROTONAL_SUITE_DORICOLIB_PATH` and `MICROTONAL_SUITE_README_PATH` `[Files]` entries to `{userappdata}\Ouaricon\Microtonal Suite`; D-08 validation matrix `25-02-VALIDATION-MATRIX.md` records `.doricolib bytes in payload = 6431` for all 8 plugins × 2 platforms; macOS canary O-Lyrica strict-PASS; Windows canary O-Lyrica strict-PASS |
| 4 | Cross-platform validation (macOS + Windows) PASS for at least one canary per platform | VERIFIED | `25-02-VALIDATION-MATRIX.md` `Final Verdict: PASS`; macOS O-Lyrica: build exit 0, install OK, Library Manager Import SUCCESS, quarter-sharp C4 ~269 Hz, no attack zipper, polyphonic isolation; Windows O-Lyrica: build exit 0, EXE install OK, `.doricolib` at `%APPDATA%\Ouaricon\Microtonal Suite\` at 6431 B, Library Manager Import SUCCESS, quarter-sharp gate confirmed. One open human-verification item (see below) does not invalidate the canary PASS itself — it concerns multi-account elevation edge case not exercised during the sweep |
| 5 | Developer-reference doc at `research/microtonal-dorico-integration.md` | VERIFIED | File exists, 554 lines (≥200); front-matter: `audience: internal-dev-only`, `type: reference`, `domain: dsp`; all 4 H2 sections present (`## Module Architecture`, `## Canonical Dorico Setup Procedure`, `## Host-Side Behavior Quirks`, `## Troubleshooting Signatures`); key strings verified: `kVST3NoteExpression`, `kScoreLibrary`, `Library Manager`, `invalid file format`, `Ouaricon::NoteExpression`, `modules/tuning/note-expression/cpp/NoteExpression.h`, `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp`; no Path A residue |

**Score:** 5/5 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` | Canonical Path B asset — 48-container kScoreLibrary skeleton + ExpressionMapDefinition | VERIFIED | 6431 B; XML-valid; 48 containers; load-bearing IDs present |
| `modules/tuning/note-expression/install-microtonal-suite.cmake.in` | Single-write install script — macOS + Windows, no Path A | VERIFIED | 2 non-comment `file(COPY)` calls; both platform branches; no forbidden strings |
| `modules/tuning/note-expression/module.cmake` | configure_file + install(SCRIPT) only; JUCE-NE-PATCH lines 1-41 preserved; no Path A | VERIFIED | Path A strings absent; `configure_file` and `install(SCRIPT)` present |
| `modules/cmake/OuariconModules.cmake` | 3 functions; `ouaricon_extract_vst3_cids` deleted | VERIFIED | `grep -c '^function('` = 3; `ouaricon_extract_vst3_cids` absent |
| `modules/tuning/note-expression/resources/README-microtonal-suite.txt` | 6-section Path B user-facing README; all 8 plugins listed; no Path A | VERIFIED | Library Manager present; all 8 plugin names present; macOS + Windows paths present; no Path A strings |
| `modules/tuning/note-expression/README.md` | Path B rewrite; no `auto-discovery`; `Library Manager` present; `kVST3NoteExpression` preserved | VERIFIED | `auto-discovery` absent (case-insensitive); `Library Manager` present; `kVST3NoteExpression` present; `Path B` present |
| `modules/tuning/note-expression/resources/playback-template/` | DELETED | VERIFIED | Directory does not exist |
| `.claude/skills/plugin-packaging/references/pkg-creation.md` | Microtonal Suite payload-copy block + SUITE_DIR postinstall block | VERIFIED | `Microtonal Suite` strings present |
| `.claude/skills/plugin-packaging/assets/inno-template.iss` | `MICROTONAL_SUITE_DORICOLIB_PATH` + `MICROTONAL_SUITE_README_PATH` [Files] entries | VERIFIED | `MICROTONAL_SUITE` strings present |
| `.claude/skills/plugin-packaging/references/inno-setup-creation.md` | Section 3.4 with PowerShell substitution example | VERIFIED | `Section 3.4` / `MICROTONAL_SUITE` present |
| `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` | D-08 gate evidence; PASS verdict; 8×2 matrix | VERIFIED | File exists; `Final Verdict: PASS`; all 8 plugins × 2 platforms |
| `research/microtonal-dorico-integration.md` | 4 H2 sections; `audience: internal-dev-only`; ≥200 lines | VERIFIED | 554 lines; all 4 H2 present; front-matter verified |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `module.cmake` | `install-microtonal-suite.cmake.in` | `configure_file @ONLY` + `install(SCRIPT)` | WIRED | Both directives present; per-target cmake name pattern `install-microtonal-suite-${TARGET_NAME}.cmake` confirmed |
| `install-microtonal-suite.cmake.in` | `~/Library/Application Support/Ouaricon/Microtonal Suite/` | `file(MAKE_DIRECTORY)` + `file(COPY)` | WIRED | Both calls present; macOS path string confirmed |
| `install-microtonal-suite.cmake.in` | `$ENV{APPDATA}/Ouaricon/Microtonal Suite/` | `file(MAKE_DIRECTORY)` + `file(COPY)` | WIRED | `elseif(WIN32)` branch with `APPDATA` confirmed |
| `Ouaricon-VST3-NoteExpression.doricolib` | Dorico expression-map dropdown | User one-time Library Manager Import | WIRED (human-verified) | O-Lyrica canary on both platforms: import SUCCESS; "Ouaricon VST3 Note Expression" appears in dropdown |
| `pkg-creation.md` | all 8 plugins' PKG installers | shared-template cascade (zero forks per preflight audit) | WIRED | Task 0 preflight GREEN; 8 plugins all category (a) |
| `inno-template.iss` | all 8 plugins' EXE installers | shared-template cascade | WIRED | Same preflight audit; `MICROTONAL_SUITE_*_PATH` vars substituted at packaging time |
| DOCS-01 | `modules/tuning/note-expression/cpp/NoteExpression.h` + `NoteExpression_VST3.cpp` | markdown code-reference links | WIRED | Both source paths present in `research/microtonal-dorico-integration.md` |
| DOCS-02 | `Library → Library Manager → Import…` menu path | named exact menu paths | WIRED | String present 4× in research doc |
| DOCS-04 | DOCS-03 kScoreLibrary schema | symptom-cause table row 3 | WIRED | `kScoreLibrary` present in both sections; `invalid file format` row cross-references 48-container requirement |

### Data-Flow Trace (Level 4)

Not applicable — phase delivers static file artifacts (CMake scripts, XML/doc files, installer template extensions), not components that render dynamic data. All artifacts are configuration and documentation; no runtime data-flow paths exist to trace.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| `.doricolib` XML well-formed | `xmllint --noout .../Ouaricon-VST3-NoteExpression.doricolib` | exit 0 | PASS |
| `.doricolib` has 48 kScoreLibrary children | `xmllint --xpath 'count(/kScoreLibrary/*)'` | `48` | PASS |
| `.doricolib` file size | `wc -c` | `6431` | PASS |
| Path A strings absent from module.cmake | `grep -q 'dorico_pt|tar cf|...'` | CLEAN | PASS |
| `ouaricon_extract_vst3_cids` absent from OuariconModules.cmake | `grep -q 'ouaricon_extract_vst3_cids'` | ABSENT | PASS |
| OuariconModules.cmake has exactly 3 functions | `grep -c '^function('` | `3` | PASS |
| install script: exactly 2 non-comment `file(COPY)` calls | `grep -v '#' \| grep -c 'file(COPY'` | `2` | PASS |
| `auto-discovery` absent from module README | `grep -qi 'auto-discovery'` | ABSENT | PASS |
| All 8 cohort plugins listed in README-microtonal-suite.txt | per-plugin grep | all 8 FOUND | PASS |
| research doc ≥ 200 lines | `wc -l` | `554` | PASS |
| research doc `audience: internal-dev-only` | `grep 'audience'` | FOUND | PASS |
| Key commits exist in git log | `git log --oneline ad9e5e4 db20a04 98479ba 93d29d6 c45703b 8091d64` | all 6 present | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| INST-01 | Plan 25-01 | Canonical Dorico-valid `.doricolib` with kVST3NoteExpression authored | SATISFIED | `.doricolib` exists at canonical path; XML-valid; 48-container; load-bearing strings verified; Library Manager Import + quarter-sharp PASS on macOS |
| INST-02 | Plan 25-01 | `.doricolib` stored at `modules/tuning/note-expression/resources/library/` as single source of truth | SATISFIED | File confirmed at that exact path |
| INST-03 | Plan 25-02 | 8 plugin installers bundle the `.doricolib` (PKG + EXE) | SATISFIED | Shared templates extended; D-08 matrix: all 8 × 2 platforms with `.doricolib bytes = 6431` |
| INST-04 | Plan 25-02 | Installed `.doricolib` at discoverable path; installer emits Library Manager Import hint | SATISFIED | PKG postinstall echoes import hint; Inno Setup `CurStepChanged` logs activation hint; companion README-microtonal-suite.txt shipped at install path |
| DOCS-01 | Plan 25-03 | Module architecture note (NEC flow, queue semantics, voice-routing, TuningEngine) | SATISFIED | `## Module Architecture` section in research doc; NoteExpression.h + NoteExpression_VST3.cpp paths referenced; `Ouaricon::NoteExpression`, `noteId`, `applyPendingTuning`, `g_neUpdate`/`g_neQuery` present |
| DOCS-02 | Plan 25-03 | Canonical Dorico setup procedure (Path B Library Manager Import; exact menu paths) | SATISFIED | `## Canonical Dorico Setup Procedure` section; `Library → Library Manager → Import…` 4×; both platform install paths; `Play → Endpoints`; ~269 Hz verification |
| DOCS-03 | Plan 25-03 | Host-side behavior quirks (neighbor-semitone, NEC handshake, sample-offset timing, kScoreLibrary schema, D-01/D-02 rationale) | SATISFIED | `## Host-Side Behavior Quirks` section; 7 subsections covering all required topics including kScoreLibrary 48-container requirement and explicit-import rationale |
| DOCS-04 | Plan 25-03 | Troubleshooting signatures table (expression-map-skipped trap, invalid-file-format, semitone-not-quartertone) | SATISFIED | `## Troubleshooting Signatures` 9-row symptom-cause-fix table; `invalid file format` row present; expression-map-in-dropdown-but-semitone row present |
| DOCS-05 | Plan 25-03 | Internal notes in `research/` directory; `audience: internal-dev-only` front-matter | SATISFIED | File at `research/microtonal-dorico-integration.md`; YAML front-matter `audience: internal-dev-only` verified by grep |

### Anti-Patterns Found

| File | Finding | Severity | Impact |
|------|---------|----------|--------|
| `inno-template.iss` | `{userappdata}` with `PrivilegesRequired=admin` — resolves to elevated process's APPDATA, not launcher's, on multi-account Windows machines (BL-01 from 25-REVIEW.md) | WARNING | Goal truth SC-4 (cross-platform validation) was proven on a single-account dev machine where the failure mode is masked. Real-world multi-account installs may land the `.doricolib` in the wrong user's profile. Surfaced in human_verification section. |
| `install-microtonal-suite.cmake.in` | No guard on `$ENV{HOME}` / `$ENV{APPDATA}` being empty — would silently write to filesystem root (HI-01 from 25-REVIEW.md) | WARNING | Developer-path only (`cmake --install`); production install path is PKG postinstall which bypasses this script on macOS. Robustness concern, not a ship blocker for SC-2. |
| `pkg-creation.md` | `sed s/PLUGIN_NAME_PLACEHOLDER/` metacharacter injection risk if future plugin names contain `/` or `&` (HI-02 from 25-REVIEW.md) | WARNING | Current 8-cohort plugin names are safe (alphanumeric + hyphen). Affects future plugin authoring safety; not a current breakage. |

No stub patterns, placeholder returns, or TODO/FIXME anti-patterns found in the verified artifacts. The `ouaricon_extract_vst3_cids` reference in `research/microtonal-dorico-integration.md:481` is intentional historical narrative ("this helper is dead code"), not Path A residue.

### Human Verification Required

#### 1. Windows multi-account installer elevation (BL-01 from 25-REVIEW.md)

**Test:** On a Windows machine with two accounts (a standard user account and a separate admin account), log in as the standard user. Launch the O-Lyrica EXE installer. When UAC prompts, elevate using the **admin account's credentials** (not the standard user's). Complete the install.

**Expected:** After install, verify `%APPDATA%\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib` exists at 6,431 B from a session running as the **standard user**. Open Dorico under the standard user account; Library Manager Import from the documented path should succeed.

**Why human:** The D-08 validation matrix canary was run on a single-account Windows 11 dev machine where the user launching the installer and the user satisfying UAC elevation are the same account. Inno Setup's `{userappdata}` constant resolves to the elevated process's `%APPDATA%` when `PrivilegesRequired=admin` is in effect. If launcher != admin-account-used-for-elevation, the `.doricolib` lands in the wrong user's profile and Library Manager Import cannot find it at the documented path. This is the BL-01 finding from `25-REVIEW.md`. The test requires a two-account Windows environment; cannot be automated programmatically here.

**Fix if fails:** Replace `{userappdata}` with `{autoappdata}` in `inno-template.iss` lines 52-53 and 81, and add `PrivilegesRequiredOverridesAllowed=dialog` at line 39. See BL-01 in `.planning/phases/25-package-docs/25-REVIEW.md` for the complete fix.

---

## Gaps Summary

No gaps. All 5 roadmap success criteria verified against the codebase. The single human-verification item (BL-01 multi-account Windows elevation) is a robustness/edge-case concern that was identified via code review; the D-08 canary itself PASSed with the documented evidence. The phase goal — canonical `.doricolib` authored, module-owned via CMake, bundled in all 8 installers, landing at the documented platform paths, activated via Library Manager Import, with developer-reference doc — is achieved.

The BL-01 finding does not block the phase goal per the verification evidence context provided: the matrix-PASS was user-confirmed and the failure mode only surfaces on multi-account managed Windows environments not exercised during the sweep. The finding is elevated to human verification for a targeted retest, not treated as a blocker for goal achievement.

---

_Verified: 2026-04-27_
_Verifier: Claude (gsd-verifier)_
_Phase: 25-package-docs — v3 Path B locked_
