---
phase: quick-260921-gxr
plan: 01
subsystem: repo-hygiene, ci
tags: [gitignore, github-actions, auval, audio-unit, macos]
status: complete
requires: []
provides:
  - "Anchored /logs/ ignore rule covering the repo-root build-log tree"
  - "First automated AU-format gate in CI (probes-macos: Build AU + auval)"
affects:
  - .gitignore
  - .github/workflows/ci-tests.yml
tech-stack:
  added: []
  patterns:
    - "CI codes DERIVED from CMakeLists via scripts/verify-au-link.sh, never mirrored as literals"
    - "!cancelled() on independent checks so an earlier failure cannot leave a gate unmeasured"
key-files:
  created: []
  modified:
    - .gitignore
    - .github/workflows/ci-tests.yml
decisions:
  - "Whole-directory rule (/logs/) rather than an extension match, because unlike dist/ or the golden dirs this tree holds no hand-authored source — the directory IS the artifact."
  - "Leading-slash anchor, so a future nested plugins/<Name>/logs/ stays visible to git."
  - "No git rm --cached anywhere: git ls-files logs was empty before and after."
  - "AU codes derived by verify-au-link.sh rather than written into the workflow."
  - "Validate step gated on steps.au_build.outcome == 'success' as well as !cancelled(), so a missing bundle reports as a build failure rather than a misleading registry miss."
metrics:
  duration: ~6min
  completed: 2026-09-21
actuals:
  tokens: 9000
  tasks: 3
  commits: 1
plan_head_before: 6848764ece63095d0a7e2be6e0e8516c8b0f726f
---

# Quick Task 260921-gxr: logs/ ignore rule + auval gate Summary

Anchored `/logs/` in `.gitignore` to close the non-`.log` gap in the repo-root build-log tree, and gave the AU format its first automated CI coverage by building the dispatched plugin's AU target in `probes-macos` and validating it through the existing `scripts/verify-au-link.sh` derivation gate.

## Both Task-Description Premises Were Wrong — Corrected Before Executing

These were established at planning time and re-confirmed against the live tree during execution. The implementation is written against what is actually there.

**Premise 1 — "logs/ is untracked only by path-scoped commit discipline." FALSE.**
Every file currently under `logs/` was *already ignored*: the 739 build logs by `.gitignore:219:*.log`, the single stray `logs/.DS_Store` by `.gitignore:73:.DS_Store` (both confirmed with `git check-ignore -v`). `git ls-files logs` returned **0** — nothing was ever tracked, so no `git rm --cached` was needed and none was run.

The real gap was narrower and forward-looking: the **directory itself** and **any file with a non-`.log` extension**. Before the change both `git check-ignore logs/` and `git check-ignore logs/O-SimpleReverb/report.json` exited 1. On a public-origin repo, the next JSON status dump or text report a harness drops beside a build log was one wide `git add` from permanent published history. The new rule closes the *next* artifact, not the current one — which is why the comment says so explicitly, so a future reader does not notice the logs are already covered and delete the rule as redundant.

**Premise 2 — "the runner already builds the AU." FALSE.**
The `probes-macos` job built C++ *test targets* only (`O-Octagon-geometry-test`, `O-Octagon-render-test`, `O-Strata-render-test`). No plugin format was produced anywhere in this workflow on macOS. So auval was not "a few lines on an existing build": it required adding a real `<TARGET>_AU` target — a full plugin compile of JUCE modules, SharedCode and the AU wrapper. **That runtime cost is the main thing to weigh**, and it is stated plainly in the new workflow comment rather than buried. The auval invocation itself genuinely is a few lines, because `verify-au-link.sh` already does the code-derivation + registrar-kick + validate sequence.

## What Was Built

### Task 1 — `.gitignore`: anchored `/logs/` rule (type: tracer)

One new commented section inserted between `scratch-pv/` and the `# Installer / Distribution Packages` banner, matching this file's own convention of introducing any non-self-evident rule with a comment block. The comment records five things: what is there and who writes it (`scripts/build-and-install.sh:92`, the sole writer); that the rule changes nothing today and why it is still worth having; why a whole-directory rule is correct here when the file elsewhere refuses to broaden past the named artifact; why the leading-slash anchor is load-bearing; and that nothing was untracked.

### Task 2 — `.github/workflows/ci-tests.yml`: `Build AU` + `Install AU and validate (auval)`

Two steps appended after the existing `Orbit golden` step; no existing step reordered or modified. The `probes-macos` job banner was updated, since "builds the C++ test targets" stopped being the whole story.

- **`Build AU`** (`id: au_build`, `if: ${{ !cancelled() }}`) resolves the CMake target via `scripts/resolve-target.sh cmake-target`, writes it to `$GITHUB_OUTPUT`, then builds `${CMAKE_TARGET}_AU`. Target resolution is not optional — O-Octagon's CMake target is `OuariconOctagon`, so a folder-derived `O-Octagon_AU` would not exist. Mirrors the Windows job's existing resolution style.
- **`Install AU and validate (auval)`** (`if: ${{ !cancelled() && steps.au_build.outcome == 'success' }}`) globs the single built `.component` out of the artefacts AU directory, creates the user Components directory, removes any same-named bundle first (so a warm/re-run runner cannot validate a stale bundle — the registry-slot pinning hazard CLAUDE.md describes for local installs), copies in, then runs `scripts/verify-au-link.sh`.

No `if: inputs.plugin == ...` guard on either step: both dispatchable plugins declare `AU` in their formats list, so the steps are plugin-agnostic. No AU type, subtype or manufacturer code appears anywhere in the workflow.

### Task 3 — one path-scoped commit

`f749ae8d`, on `main`, touching exactly `.gitignore` and `.github/workflows/ci-tests.yml`. 121 insertions, 1 deletion.

## Gate Results

All three gates were run verbatim as written in the plan. Literal PASS tokens:

| Gate | Token | Result |
|---|---|---|
| Task 1 | `GITIGNORE GATE PASS` | printed |
| Task 2 structural | `WORKFLOW GATE PASS` | printed |
| Task 2 local proxy | `LOCAL PROXY PASS` | printed |
| Task 3 | `COMMIT GATE PASS` | printed |
| Submodule guard | `SUBMODULE GUARD PASS` | printed (no staged path under `plugins/O-Orbit/libs/SAF`) |

The Task 1 gate's discriminating clause is that `git check-ignore -v logs/O-SimpleReverb/report.json` credits the **new anchored rule** (`:/logs/`), not `*.log` — a clause that failed before the edit. Its negative control (`plugins/O-Octagon/logs/probe.json` still NOT ignored) is the only clause an unanchored `logs/` would fail, and it held.

The Task 2 structural gate reads the **parsed** workflow, not its text, so nothing written in a comment can satisfy or break it. Its last three assertions are the security regression guard: exactly 3 third-party actions repo-wide, trigger still `workflow_dispatch` only, permissions still `{contents: read}` — all unchanged.

Post-commit checks: `git diff --diff-filter=D HEAD~1 HEAD` empty (no deletions), no untracked files left behind, `git rev-list --count 6848764e..HEAD` = **1**.

## What No Gate Here Establishes — No CI Dispatch Was Run

**No workflow dispatch was performed** (explicitly out of scope). Therefore:

- **The added AU build cost on `macos-14` is UNMEASURED.** It is a full plugin compile and the single most material consequence of this change, but the only figure available is the local one, which is not comparable to a cold runner.
- Whether auval's battery passes on a clean CI checkout is unverified.
- The runner's component-registrar behaviour under a non-interactive session is unverified.
- The CI copy path itself (artefacts → `$HOME/Library/Audio/Plug-Ins/Components`) is unexercised.

The local proxy is an honest stand-in for the *derivation* half only: it proves the resolver returns the non-obvious target name `OuariconOctagon`, that the AU artefacts directory has the shape the new step globs, and that `verify-au-link.sh` exits clean against a real installed bundle with real derived codes (`type=aufx subtype=OuOc manuf=OuDv`).

**Caution for whoever triages the first dispatched run:** locally, `auval -a` did NOT list the installed components even after a registrar kick, while `auval -v` against the derived codes validated cleanly. Use the direct validate form; do not read an empty registry listing as a missing bundle.

## Deviations from Plan

None — plan executed exactly as written. No deviation rule fired, no auto-fix was needed, no architectural question arose.

## Known Stubs

None.

## Threat Flags

None. This change installs no package, adds no dependency, downloads nothing new, opens no listener, and touches no plugin or runtime code. The one `high` threat in the plan's register (T-gxr-03, widened trigger or permissions) is held closed by machine assertion in the Task 2 gate, which passed.

## Self-Check: PASSED

- `.gitignore` — FOUND, modified, rule active (`git check-ignore -v` credits `/logs/`)
- `.github/workflows/ci-tests.yml` — FOUND, parses as YAML, both new steps present in the parsed structure
- Commit `f749ae8d` — FOUND in `git log`, touches exactly the 2 intended files
- `.claude/agent-memory/research-planning-agent.md` — still dirty in the working tree, unmodified by this run
