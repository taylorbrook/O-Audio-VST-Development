# Plugin Freedom System — Read-Only System Review (260701-in8)

## Scope and Method

Read-only inspection of the **system** that drives this JUCE VST/AU monorepo — not
the DSP of individual plugins. Inspected surfaces:

- `scripts/` (build-and-install.sh, .ps1, apply-juce-patches.sh, verify-au-link.sh, verify-backup.sh, generate_placeholder_models.py)
- `.github/workflows/build-and-release.yml` (CI/CD)
- `.claude/` — settings(.local).json, hooks/, agents/, agent-memory/, skills/, commands/, references/, critics/, resource-index.json, stray artifacts
- `modules/registry.yaml` + on-disk consumer reconciliation
- Root + per-plugin `CMakeLists.txt` cross-cutting patterns (WebView2, dev/release suffix, licensing residue)
- `PLUGINS.md`, `.planning/STATE.md`

No inspected file was modified. The only file written by this task is this REVIEW.md.
Every bug cites `file:line` or a unique grep-locatable token.

## Severity Legend

- **CRITICAL** — breaks the system now / data-loss / security exposure requiring immediate action.
- **HIGH** — silently broken subsystem, or a latent break that fires on a common path (e.g. a specific plugin release).
- **MEDIUM** — correctness/consistency defect with a workaround or narrow blast radius.
- **LOW** — cosmetic, stale artifact, or hardening nice-to-have.

## Executive Summary

- **Two orchestration hooks are wired to non-existent event names** and silently never
  fire — the agent-memory injection and task-validation subsystems are effectively dead
  (BUG-02). This is the single most urgent item: the `.claude/agent-memory/` investment is not being applied at subagent spawn.
- **CI target resolution is more fragile than the local script** and will break a real
  release: `O-Texture` (whose `juce_add_plugin` target is `${PROJECT_NAME}`) cannot be
  released through GitHub Actions as written (BUG-01).
- **`modules/registry.yaml` is pervasively out of sync with disk** — `used_by` lists cite
  retired folder names, `scala-tuning-engine` shows `used_by: []` despite 12 real consumers,
  and the header version/date is frozen at 2026-01-14 (BUG-later / UPD-02, UPD-03).
- **A blanket `Bash(rm -rf *)` sits in the auto-approve allow-list** (BUG-03).
- **Positive:** the historically-tracked WebView2 gap is closed — 38/38 real plugins now set
  **both** `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`.
- Housekeeping debt: the JUCE patch filename still lies about its version, a stray
  `plugins/tache_plugins/` tree pollutes the plugin namespace, and several ops artifacts are stale.

---

## Bugs

| ID | Severity | Location | Finding | Recommendation |
|----|----------|----------|---------|----------------|
| BUG-01 | HIGH | `.github/workflows/build-and-release.yml:87` and `:439` | The CI "Resolve CMake Target" step uses `grep 'juce_add_plugin(' \| sed 's/.*juce_add_plugin(\([^ )]*\).*/\1/'`. It does **not** resolve `${PROJECT_NAME}`-style variable targets and cannot read a target placed on the line *after* `juce_add_plugin(`. `O-Texture` uses `juce_add_plugin(${PROJECT_NAME})`, so `TARGET` becomes the literal `${PROJECT_NAME}` and the subsequent `cmake --build --target "${TARGET}_VST3"` fails. The local `scripts/build-and-install.sh:186-238` `resolve_cmake_target()` already handles both cases — CI reimplements a weaker version (local/CI drift). | Reuse the robust resolver (source it, or port the `awk` + `${VAR}` resolution) into the CI step. |
| BUG-02 | HIGH | `.claude/settings.json:68` (`SubagentStart`) and `:80` (`TaskCompleted`) | Both matchers reference event names that are **not** in Claude Code's hook event set (SessionStart, PostToolUse, PreToolUse, UserPromptSubmit, Notification, Stop, SubagentStop, PreCompact, SessionEnd). `inject-agent-memory.py` (agent-memory injection) and `task-validator-dispatch.py` (post-task validation) therefore never execute — the `.claude/agent-memory/*.md` corpus is never injected into subagents. | Re-wire `inject-agent-memory.py` to a supported event (e.g. `SessionStart`/`UserPromptSubmit` gated by agent name) and `task-validator-dispatch.py` to `SubagentStop`; or remove the dead blocks. Verify each hook fires with a debug echo. |
| BUG-03 | HIGH | `.claude/settings.local.json:12` | Auto-approve allow-list contains `"Bash(rm -rf *)"` — grants unattended recursive deletion of anything matching. Adjacent broad globs `"Bash(git add *)"`, `"Bash(git commit -m ' *)"`, `"Bash(git push *)"`, `"Bash(cd *)"` (lines 28-35) are also wider than necessary. | Remove `Bash(rm -rf *)`; scope git/cd allows to specific paths/subcommands. |
| BUG-04 | MEDIUM | `.claude/settings.json:70` vs `.claude/agent-memory/` | The `inject-agent-memory` matcher is `troubleshoot-agent\|dsp-agent\|gui-agent\|research-planning-agent\|validation-agent\|research-lead`, but `agent-memory/dorico-agent.md` exists and is **excluded** from the matcher, while `research-lead` **is** matched yet has **no** `agent-memory/research-lead.md`. Even after BUG-02 is fixed the wiring is inconsistent. | Add `dorico-agent` to the matcher; either create `research-lead` memory or drop it from the matcher. |
| BUG-05 | MEDIUM | `scripts/apply-juce-patches.sh:66` | Idempotency guard requires the `JUCE-NE-PATCH` marker in **both** target files (`FOUND -ge 2`, line 48). If a prior run patched one file then failed on the second, a rerun falls through to `patch -p1 < "$PATCH_FILE"` with no `--forward`/dry-run guard and re-applies to the already-patched file. | Add `patch --forward --dry-run` preflight (skip already-applied hunks) or check per-file marker before applying. |
| BUG-06 | LOW | `scripts/build-and-install.sh:2` | Uses `set -e` only — no `set -u` (unbound vars) or `set -o pipefail`. `execute()` compensates with `${PIPESTATUS[0]}` but the verbose `eval \| tee` path (line 114) still masks non-zero exits of the left side in some shells. | Add `set -uo pipefail` and quote-audit expansions. |

## Improvement Suggestions

| ID | Severity | Location | Finding | Recommendation |
|----|----------|----------|---------|----------------|
| IMP-01 | MEDIUM | `.claude/hooks/` | Orphan hook scripts present but never referenced in `settings.json`/`settings.local.json`: `Stop.py`, `UserPromptSubmit.py`, `detect-research-conflicts.py`, `merge-critic-reports.py`. They read as active infra but are dead code. | Wire the ones that should run (`UserPromptSubmit.py`, `Stop.py` map to real events) or delete to reduce confusion. |
| IMP-02 | MEDIUM | `modules/registry.yaml` | `used_by` is hand-maintained and drifts badly (see BUG-cluster in Section B). | Generate `used_by` from disk (`grep -rl <module-token> plugins/*/CMakeLists.txt`) via a small script run in CI or a pre-commit check, making disk the single source of truth. |
| IMP-03 | MEDIUM | CI vs `scripts/build-and-install.sh` | CI duplicates target/PRODUCT_NAME resolution with weaker `sed` (root cause of BUG-01, and PRODUCT_NAME parsing at `build-and-release.yml:235,453,566`). | Factor the resolver into a shared `scripts/resolve-target.sh` sourced by both local and CI paths. |
| IMP-04 | LOW | `plugins/tache_plugins/` | A nested tree of ~18 non-Ouaricon dirs (AngelGrain, AutoClip, ClapMachine, DriveVerb, Drum808, DrumRoulette, FlutterVerb, …) with no top-level `CMakeLists.txt` lives inside `plugins/`, inflating plugin counts and polluting `ls plugins/` tooling. | Move to `archive/` or a sibling path outside `plugins/`. |
| IMP-05 | LOW | `scripts/build-and-install.sh:87` | `logs/$PLUGIN_NAME` is written under CWD with no gitignore confirmation shown here; repeated builds accrue `build_*.log`. | Confirm `logs/` is gitignored; add log rotation/retention. |

## Update Recommendations

| ID | Severity | Location | Finding | Recommendation |
|----|----------|----------|---------|----------------|
| UPD-01 | HIGH | `scripts/juce-patches/note-expression-juce-8.0.4.patch` (referenced `scripts/apply-juce-patches.sh:22`, `modules/registry.yaml:288`) | Filename says `8.0.4` but the content is a JUCE **8.0.9** diff (local + CI both pinned to 8.0.9 per `build-and-release.yml:20`). Known-pending rename; still unresolved and actively misleading. | Rename to `note-expression-juce-8.0.9.patch`; update the two references. |
| UPD-02 | HIGH | `modules/registry.yaml` `used_by` fields | Pervasively stale: `vu-meter.used_by` → `OuariconComp`/`OuariconAnalogEQ` (dirs gone; now `O-Comp`/`O-AnalogEQ`); `preset-manager.used_by` lists 3 retired names but **19** plugins consume `preset-manager.js` on disk; `scala-tuning-engine.used_by: []` (line 278) despite **12** consumers (O-Bassoon, O-Bells, O-Bowed, O-Contrabass, O-Formant, O-IntonationPad, O-Marimba, O-Lyrica, O-MicrotonalSampler, O-Prism, O-Reed, O-Wind); `note-expression.used_by` (lines 299-315) omits on-disk consumers O-Bassoon, O-Contrabass, O-MicrotonalSampler and uses `OLyrica` for dir `O-Lyrica`. | Regenerate all `used_by` from disk (see IMP-02). |
| UPD-03 | MEDIUM | `modules/registry.yaml:11-12` | Header `version: 1.0.0` / `last_updated: 2026-01-14` frozen while modules bumped (scala 2.0.0, note-expression 1.1.1, preset-manager 1.0.2, compressor-unit 1.2.1, analog-eq-unit 1.2.0). | Bump header version + date on every registry edit (or drive from git). |
| UPD-04 | LOW | `.claude/compaction-snapshot.md`, `.claude/frontmatter-issues.txt`, `.claude/resource-index.json` | `compaction-snapshot.md` (2026-03-07) references a `MinimalKick` plugin that does not exist on disk; `frontmatter-issues.txt` is a 2026-04-04 scan log; `resource-index.json` `generated: 2026-04-05` predates months of activity. | Delete the transient snapshot/scan artifacts; regenerate `resource-index.json` or document its refresh cadence. |
| UPD-05 | LOW | `.claude/skills/…` | Post–PWYW Path B licensing removal, the `licens*` token still appears in skill templates (`plugin-planning/assets/architecture-template.md`, `spike-findings-VST-development/SKILL.md`, `plugin-packaging/assets/inno-template.iss`, `system-setup/references/execution-notes.md`, `ui-mockup/references/ui-design-rules.md`). Some are benign (installer `LicenseFile`), some may be stale guidance. | Audit each hit; strip stale licensing-integration guidance, keep legitimate installer license references. |
| UPD-06 | LOW | `.planning/STATE.md:38` | Carry-forward **BL-01** (Windows non-admin UAC installer retest) is still open from Phase 25. | Schedule the retest or close BL-01 explicitly in the next milestone. |

---

## Appendix — Section A: Automation and Orchestration (detail)

- **A1 → BUG-01.** CI target/PRODUCT_NAME resolution (`build-and-release.yml:87,235,439,453,566`)
  is a naive `sed` that assumes the target is a literal on the same line as `juce_add_plugin(`.
  Local `resolve_cmake_target()` (`build-and-install.sh:186-238`) handles `${VAR}` and next-line
  tokens and even falls back to the built `*_artefacts` dir. CI should not diverge.
- **A2 → BUG-02.** `settings.json` hook events `SubagentStart` (line 68) and `TaskCompleted`
  (line 80) are not real Claude Code events → dead subsystems.
- **A3 → BUG-04.** Matcher/agent-memory-file mismatch (dorico-agent excluded; research-lead has no memory).
- **A4 → IMP-01.** Orphan hooks: `Stop.py`, `UserPromptSubmit.py`, `detect-research-conflicts.py`, `merge-critic-reports.py`.
- **A5 → BUG-03.** `settings.local.json:12` `Bash(rm -rf *)` + broad git/cd allows.
- **A6 → BUG-06.** `build-and-install.sh` strict-mode gap.
- **A7 → BUG-05.** `apply-juce-patches.sh` partial-apply reapply risk.
- **Positive:** `verify-au-link.sh` is well-guarded (macOS preflight, explicit RC capture, cache-clear before auval). CI signing/notarization/stapling flow (`build-and-release.yml:96-379`) is complete and cleans up its keychain with `if: always()`.

## Appendix — Section B: Modules, CMake and Cross-Cutting Drift (detail)

- **B1 → UPD-02.** Registry `used_by` drift (retired folder names; `scala-tuning-engine` empty; preset-manager 3-vs-19; note-expression omissions).
- **B2 → UPD-03.** Registry header `version`/`last_updated` frozen at 2026-01-14.
- **B3 → IMP-04.** Stray `plugins/tache_plugins/` (18 nested non-Ouaricon dirs, no CMakeLists) — the reason a raw `ls plugins/` reports "39" while only **38** real plugins have a `CMakeLists.txt`.
- **B4 → UPD-01.** JUCE patch filename mismatch (8.0.4 name / 8.0.9 content).
- **B5 (positive).** WebView2 coverage: **38/38** real plugins set **both** `NEEDS_WEBVIEW2 TRUE`
  and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. The historical "34/35 missing" regression class
  (per MEMORY.md) is resolved. Only exception: `O-Orbit` is `SKIP_PLUGINS`-excluded from CI
  (`build-and-release.yml:81,432`) due to its `libs/SAF` submodule — expected, not a defect.
- **B6.** No licensing/Supabase residue in root or plugin `CMakeLists.txt` (PWYW Path B removal
  is clean at the CMake layer); residual `licens*` tokens are confined to skill templates (UPD-05).
- **B7.** Root CMake dev/release branding (`CMakeLists.txt:25-31`) and `SKIP_PLUGINS` gate (`:45-52`) are internally consistent with `verify-au-link.sh`'s dev-suffix manufacturer-code resolution.

---

Method: read-only; no files changed.
