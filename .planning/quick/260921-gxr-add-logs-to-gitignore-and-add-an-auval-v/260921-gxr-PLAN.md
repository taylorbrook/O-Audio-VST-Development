---
phase: quick-260921-gxr
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .gitignore
  - .github/workflows/ci-tests.yml
autonomous: true
requirements: [QT-gxr-01, QT-gxr-02, QT-gxr-03]

estimate:
  tokens: 28000
  raw_tokens: 28000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "A non-`.log` file written anywhere under the repo-root `logs/` tree is ignored by git, and the rule that fires for it is the new anchored one (QT-gxr-01)."
    - "The anchored rule does NOT swallow a hypothetical `plugins/<Name>/logs/` — a nested logs directory elsewhere in the tree stays visible to git (QT-gxr-01)."
    - "Nothing under `logs/` was tracked before the change and nothing was untracked by it — no `git rm --cached` is performed anywhere in this plan."
    - "The probes-macos job builds the dispatched plugin's AU target and runs Apple's `auval` against the installed bundle, with the AU type/subtype/manufacturer codes DERIVED from CMakeLists rather than written as literals in the workflow (QT-gxr-02)."
    - "The auval steps cannot be silently skipped by an unrelated probe failure: both carry `!cancelled()`, matching the Orbit-golden step's existing independent-check reasoning (QT-gxr-02)."
    - "The workflow still parses as YAML, still triggers on `workflow_dispatch` only, still declares `permissions: contents: read`, and still references exactly 3 third-party actions (QT-gxr-02)."
    - "The commit touches exactly two files; the pre-existing unrelated modification to `.claude/agent-memory/research-planning-agent.md` is still uncommitted and unmodified afterwards (QT-gxr-03)."
  artifacts:
    - .gitignore
    - .github/workflows/ci-tests.yml
  key_links:
    - "New anchored `/logs/` rule -> `scripts/build-and-install.sh:92` (`log_dir=\"logs/$PLUGIN_NAME\"`), the sole writer into that tree."
    - "`Build AU` step -> `scripts/resolve-target.sh cmake-target` — O-Octagon's CMake target is `OuariconOctagon`, not the folder name, so `${folder}_AU` would not exist."
    - "`Install AU and validate (auval)` step -> `scripts/verify-au-link.sh`, which parses PLUGIN_CODE + AU main type from the plugin CMakeLists, resolves the manufacturer code from the root CMakeLists dev branch, kicks AudioComponentRegistrar, and runs auval."
    - "Dev branding (this job passes no `OUARICON_RELEASE`, line 150-155) -> the `-dev` bundle name AND the dev manufacturer code that verify-au-link.sh resolves. Both sides must stay on the dev branch or auval looks for a component that was never installed."
    - "`!cancelled()` on both new steps -> the same reasoning already written at the Orbit-golden step (line 199-201): independent checks must not be skipped by an earlier failure."
---

<objective>
Close two small, independent gaps found in the live tree: an unignored generated-output directory, and an AU format that no CI job has ever validated.

Both premises in the task description were checked against the live tree and **both are partly wrong**. The plan is written against what is actually there, not against the description:

**1. `logs/` — 51 MB, 50 per-plugin directories, zero tracked files.** The description says it is "untracked only by path-scoped commit discipline." That is not what is happening. Every file currently under it is ALREADY ignored: the 739 build logs by `.gitignore:219:*.log`, the one stray `logs/.DS_Store` by `.gitignore:73:.DS_Store`. Confirmed with `git check-ignore -v`. The real gap is narrower and forward-looking: the **directory itself** and **any non-`.log` file written into it** are not ignored (`git check-ignore logs/` and `git check-ignore logs/X/report.json` both exit 1 today). This is a public repo, so the next `.json` status dump or `.txt` report that the harness drops next to a build log is one wide `git add` away from permanent history.

**2. The probes-macos job does not build any AU.** The description says "the runner already builds the AU." It does not. That job builds C++ *test targets* only — `O-Octagon-geometry-test`, `O-Octagon-render-test`, `O-Strata-render-test`. No plugin format is produced anywhere in this workflow on macOS. So auval is not "a few lines on an existing build": it requires adding a real `<TARGET>_AU` build — a full plugin compile of JUCE modules, SharedCode and the AU wrapper. **That runtime cost is the main thing to weigh, and it is called out in the workflow comment as well as here.** The auval invocation itself genuinely is a few lines, because `scripts/verify-au-link.sh` already exists and does the whole code-derivation + registrar-kick + validate sequence.

Purpose: make the generated-output exclusion match its real shape, and give the AU format its first automated gate in CI.
Output: two edited files, one path-scoped commit.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@.gitignore
@.github/workflows/ci-tests.yml
@scripts/verify-au-link.sh
@scripts/resolve-target.sh

**Live observations made at planning time (2026-09-21). These are the edit authority; do not re-derive scope from STATE.md or from the task description.**

| Observation | Command | Result |
|---|---|---|
| `logs/` location | `find . -maxdepth 4 -type d -name logs` | `./logs` only — repo root. No `plugins/*/logs`. |
| Size / shape | `du -sh logs`, `ls logs \| wc -l` | 51 MB, 50 per-plugin dirs, 739 `*.log` + 1 `.DS_Store` |
| Tracked content | `git ls-files logs \| wc -l` | **0** — nothing tracked, so no `git rm --cached` anywhere |
| Already covered | `git check-ignore -v logs/O-SimpleReverb/build_20260904_092153.log` | `.gitignore:219:*.log` |
| Already covered | `git check-ignore -v logs/.DS_Store` | `.gitignore:73:.DS_Store` |
| **Gap** | `git check-ignore -v logs/` | exit 1 — not ignored |
| **Gap** | `git check-ignore -v logs/O-SimpleReverb/report.txt` | exit 1 — not ignored |
| Sole writer | `grep -rn 'logs/' scripts/` | `scripts/build-and-install.sh:92: local log_dir="logs/$PLUGIN_NAME"` |
| probes-macos build targets | read of ci-tests.yml lines 164-197 | `O-Octagon-geometry-test`, `O-Octagon-render-test`, `O-Strata-render-test` — **no plugin format, no AU** |
| CMake target ≠ folder | `bash scripts/resolve-target.sh cmake-target O-Octagon` | `OuariconOctagon` (O-Strata resolves to `O-Strata`) |
| Both plugins declare AU | `plugins/O-Octagon/CMakeLists.txt:13`, `plugins/O-Strata/CMakeLists.txt:10` | `FORMATS VST3 AU Standalone` in both |
| AU artefact path shape | `ls -d build/plugins/O-Octagon/OuariconOctagon_artefacts/Release/AU/*` | `.../AU/O-Octagon-dev.component` |
| Dev branding in this job | ci-tests.yml lines 150-155 | `OUARICON_RELEASE` deliberately NOT passed -> `-dev` bundle, dev manufacturer code |
| Reusable gate exists | `bash scripts/verify-au-link.sh O-Octagon` | **rc=0**, derived `type=aufx subtype=OuOc manuf=OuDv`, `AU VALIDATION SUCCEEDED` |
| auval available | `command -v auval` | `/usr/bin/auval` (ships with macOS; present on `macos-14` runners) |
| YAML tooling | `python3 -c "import yaml"` | PyYAML present. `actionlint` NOT installed. |
| Parsed workflow shape | PyYAML load | trigger key parses as Python `True` (PyYAML folds `on:`); `permissions == {'contents':'read'}`; exactly **3** steps carry `uses` |
| Existing independent-check idiom | ci-tests.yml line 203 | `if: ${{ !cancelled() && inputs.plugin == 'O-Strata' }}` |
| Working tree | `git status --short` | one unrelated modification: `.claude/agent-memory/research-planning-agent.md` — must stay out of the commit |
</context>

<tasks>

<task type="tracer">
  <name>Task 1: Anchor a /logs/ rule in .gitignore, with the real reason recorded</name>
  <files>.gitignore</files>
  <action>
Add ONE new commented section to `.gitignore` containing the single rule `/logs/`.

Placement: immediately after the `scratch-pv/` rule that closes the "Debug Artifacts (screenshots, audio captures, playwright)" section, and BEFORE the `# ===== Installer / Distribution Packages` banner. A dedicated commented section is this file's own convention for any rule whose reason is not self-evident — see how Backup Directories, Render-Harness Scratch Output, and GSD Run-Scoped State are each introduced.

Use the Edit tool for a scoped insertion. Do not rewrite the file.

The comment must record four things, because a future reader will otherwise look at the rule, notice the logs are already covered, and delete it as redundant:

1. **What is there and who writes it.** 50 per-plugin directories, 739 build logs, 51 MB. The only writer is `scripts/build-and-install.sh` line 92, which sets its log directory to a per-plugin subdirectory of the repo-root tree. Every file under it is generated output; nothing there is ever authored by hand.

2. **That this rule changes nothing today, and why it is still worth having.** The build logs already match the `*.log` rule earlier in this file, and the stray macOS metadata file already matches the `.DS_Store` rule. What is NOT covered is the directory itself and any file with a different extension — a JSON status dump, a text report, a WAV capture dropped beside a build log. This repo is public. The rule closes the next artifact, not the current one.

3. **Why a whole-directory rule is correct here when this file elsewhere refuses to broaden past the named artifact.** The installer block, the O-Bowed golden block and the GSD sentinel block all deliberately match an artifact rather than its containing directory, because those directories also hold hand-authored source. This directory does not — it is generated output end to end, so the directory IS the artifact.

4. **Why the rule is anchored with a leading slash.** The anchor pins it to the repo root, the only place this tree exists. An unanchored form would additionally swallow any future nested logs directory elsewhere in the tree — a per-plugin or per-test one that might legitimately hold something tracked. The anchor is what keeps a correct directory rule from broadening into the failure mode the rest of this file exists to avoid.

Also record, in the comment, that nothing under the tree was tracked when the rule was authored, so no cached-index removal was needed.

**Do not run `git rm --cached` on anything.** `git ls-files logs` was empty at planning time; it is re-asserted in the verify below. If that assertion unexpectedly fails, STOP and report it rather than untracking anything — dropping a tracked file is a data decision, not a cleanup.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && WHO=$(git check-ignore -v logs/O-SimpleReverb/report.json) && printf '%s\n' "$WHO" | grep -q ':/logs/' && git check-ignore -q logs/O-SimpleReverb/build_20260904_092153.log && ! git check-ignore -q plugins/O-Octagon/logs/probe.json && TRACKED=$(git ls-files logs) && [ -z "$TRACKED" ] && echo "GITIGNORE GATE PASS"</automated>
  </verify>
  <done>
`GITIGNORE GATE PASS` prints. Broken out, the four clauses are:
- A non-`.log` path under the root tree is now ignored AND the new anchored rule is the one credited for it (this clause fails today — it is the discriminator that proves the edit did the work, not `*.log`).
- An existing build log is still ignored (no regression on the 739 files).
- `plugins/O-Octagon/logs/probe.json` is still NOT ignored — the negative control that proves the anchor held and the rule did not broaden to nested logs directories. An unanchored rule passes every other clause and fails only this one.
- Nothing under the tree is tracked, so nothing was untracked by the change.
  </done>
</task>

<task type="auto">
  <name>Task 2: Build the AU and run auval in the probes-macos job</name>
  <files>.github/workflows/ci-tests.yml</files>
  <precondition>`scripts/verify-au-link.sh O-Octagon` can run locally: `/usr/bin/auval` exists and `~/Library/Audio/Plug-Ins/Components/O-Octagon-dev.component` is installed. Both were true at planning time. If the component is absent, the local proxy in the verify cannot run — build and install it per CLAUDE.md first, or report the gap rather than skipping the clause.</precondition>
  <action>
Append two steps to the END of the `probes-macos` job, after the existing `Orbit golden` step. Use the Edit tool. Do not reorder or modify any existing step.

**Step A — `Build AU`**, with `id: au_build` and `if: ${{ !cancelled() }}`. Its script resolves the plugin's CMake target by invoking `scripts/resolve-target.sh cmake-target` with the dispatch input, writes that target to `$GITHUB_OUTPUT` as `cmake_target`, and then runs `cmake --build build` against the `_AU`-suffixed target, passing the same config and `-j$(sysctl -n hw.ncpu)` parallelism the neighbouring build steps use. Resolving the target is not optional: one plugin's CMake target differs from its folder name, so a folder-derived target name would not exist. The Windows job already resolves the target the same way — mirror its style. `scripts/resolve-target.sh` documents a contract requiring the repo root as working directory, which is the runner default.

No `if: inputs.plugin == ...` guard belongs on either step: both dispatchable plugins declare AU in their formats list, so the steps are plugin-agnostic and derive everything from the input.

**Step B — `Install AU and validate (auval)`**, with `if: ${{ !cancelled() && steps.au_build.outcome == 'success' }}`. Its script reads the resolved target back from the build step's output, globs the single built bundle out of the AU subdirectory of that target's artefacts directory for the dispatched plugin and build config, creates the user Components directory if absent, removes any same-named bundle already sitting there, copies the built bundle in, and finally invokes `scripts/verify-au-link.sh` with the dispatch input.

Two details that are load-bearing:

- **Do not write the AU type, subtype or manufacturer code into this workflow.** `verify-au-link.sh` parses them from the plugin's own CMakeLists and resolves the manufacturer from the root one, then kicks the component registrar before validating. A literal here would be a second copy of a value that lives elsewhere, and this file already carries a comment explaining why a mirrored value drifts silently in the direction that matters — it keeps reporting green about something nobody ships.
- **The dev-branding coupling.** This job deliberately omits the release-branding CMake option (see the Configure CMake comment), so the produced bundle carries the dev suffix and the dev manufacturer code. `verify-au-link.sh` resolves the dev branch of the root CMakeLists by design. Both sides therefore agree — but they agree by coincidence of two independent choices, so say so in the comment. Globbing the bundle rather than naming it also keeps the step correct if that branding choice ever changes.

**Comments to write.** This file documents every gate in terms of what it can and cannot claim; match that voice.

On the new block, cover: (a) that this job previously built test binaries only, so the AU target is a full plugin compile and a real addition to job runtime — state the cost plainly rather than burying it; (b) that the gate can claim the AU bundle links, registers with the system component registry, and passes Apple's own validation battery; (c) that it cannot claim anything about the editor — auval does not open the WebView, and pluginval on the Windows job is what covers that surface; (d) the derived-not-mirrored rationale above; (e) that the pre-copy removal exists so a warm or re-run runner cannot validate a stale bundle, which is the same registry-slot pinning hazard CLAUDE.md describes for local installs.

On the `if:` conditions specifically, record that `!cancelled()` is used rather than a plain condition because auval is an INDEPENDENT check from the render harness and the orbit golden — a failing harness row must not leave the AU unmeasured. This is the same reasoning already written on the Orbit-golden step; point at it. Then record the asymmetry: the validate step additionally requires its own build to have succeeded, because validating a bundle that was never produced reports a registry miss rather than a plugin defect, which is a misleading failure rather than a useful one.

Finally, update the one-line job banner comment above `probes-macos:` — it currently describes the job as building and running the C++ test targets, which stops being the whole story once a plugin format is built here.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && python3 - <<'PY'
import yaml
d = yaml.safe_load(open('.github/workflows/ci-tests.yml'))
steps = d['jobs']['probes-macos']['steps']
names = [s.get('name','') for s in steps]
b = next(s for s in steps if s.get('name') == 'Build AU')
v = next(s for s in steps if s.get('name','').startswith('Install AU'))
assert b.get('id') == 'au_build', 'build step needs id au_build'
assert '!cancelled()' in str(b.get('if','')), 'build step must not be skippable by an earlier failure'
assert '!cancelled()' in str(v.get('if','')), 'validate step must not be skippable by an earlier failure'
assert 'steps.au_build.outcome' in str(v.get('if','')), 'validate step must require its own build'
assert 'scripts/resolve-target.sh' in b['run'], 'target must be resolved, not folder-derived'
assert '_AU' in b['run'], 'AU target not built'
assert 'scripts/verify-au-link.sh' in v['run'], 'must reuse the existing derivation gate'
assert names.index('Build AU') > names.index('Orbit golden'), 'new steps must be appended last'
allsteps = [s for j in d['jobs'].values() for s in j['steps']]
assert sum(1 for s in allsteps if 'uses' in s) == 3, 'third-party action count changed'
assert list(d[True].keys()) == ['workflow_dispatch'], 'trigger widened'
assert d['permissions'] == {'contents': 'read'}, 'permissions widened'
print('WORKFLOW GATE PASS')
PY
echo "--- local proxy for the CI sequence (CI itself cannot run here) ---"
test "$(bash scripts/resolve-target.sh cmake-target O-Octagon)" = "OuariconOctagon" || exit 1
test -d build/plugins/O-Octagon/OuariconOctagon_artefacts/Release/AU || exit 1
bash scripts/verify-au-link.sh O-Octagon >/dev/null 2>&1 || exit 1
echo "LOCAL PROXY PASS"</automated>
  </verify>
  <done>
Both `WORKFLOW GATE PASS` and `LOCAL PROXY PASS` print.

The first gate is structural and reads the PARSED workflow, not its text, so nothing written in a comment can satisfy or break it. Note for whoever maintains it: PyYAML folds the trigger key into the Python boolean `True`, which is why the trigger assertion indexes with `True` rather than a string. The last three assertions are the security regression guard — no new third-party action, no widened trigger, no widened permission.

The second gate is an honest local stand-in, and its limits are worth stating: it proves the target resolver returns the non-obvious target name, that the AU artefacts directory has the shape the new step globs, and that the derivation gate exits clean against a real installed bundle with real derived codes. It does NOT prove the CI copy path, the runner's registrar behaviour, or the AU build time on `macos-14`. Those are only observable from a dispatched run.

Both commands were executed successfully at planning time and are reused verbatim.
  </done>
</task>

<task type="auto">
  <name>Task 3: Path-scoped commit of exactly the two edited files</name>
  <files>.gitignore, .github/workflows/ci-tests.yml</files>
  <precondition>Re-read `git branch --show-current` and `git status --short` immediately before committing — not values carried from earlier in this run. This checkout is shared by concurrent sessions, so another session's staging can join this commit in the gap between checking and committing, and a session-start snapshot is already stale.</precondition>
  <action>
Confirm the current branch is `main`. Then commit with an explicit two-path pathspec naming `.gitignore` and the workflow file.

`git add -A` and `git commit -a` are prohibited. The working tree carries an unrelated modification to a file under `.claude/agent-memory/` that must not enter this commit.

Suggested message: `chore(repo): ignore generated logs/ tree and add auval gate to probes-macos`, with a body noting that the logs rule closes the non-`.log` gap rather than the already-ignored build logs, and that the auval step required adding an AU build because the job previously built test targets only.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && test "$(git branch --show-current)" = "main" && FILES=$(git show --name-only --format= HEAD) && test "$(printf '%s\n' "$FILES" | awk 'NF' | wc -l | tr -d ' ')" = "2" && printf '%s\n' "$FILES" | grep -qxF '.gitignore' && printf '%s\n' "$FILES" | grep -qxF '.github/workflows/ci-tests.yml' && ! printf '%s\n' "$FILES" | grep -qF 'agent-memory' && DIRTY=$(git status --short) && printf '%s\n' "$DIRTY" | grep -qF 'agent-memory/research-planning-agent.md' && echo "COMMIT GATE PASS"</automated>
  </verify>
  <done>
`COMMIT GATE PASS` prints: on `main`, HEAD touches exactly 2 files, both are the intended paths, nothing under `agent-memory` was swept in, and the unrelated modification is still sitting dirty in the working tree where it started. That last clause is the positive half of the guard — asserting only that the commit excluded the file would also pass if the file had been reverted or deleted.
  </done>
</task>

</tasks>

<threat_model>
Scope note: proportionate to the change. This plan edits one ignore file and one CI workflow. It installs no package, adds no dependency, downloads nothing new, opens no listener, and touches no plugin or runtime code. ASVS level 1; blocking threshold `high`. One threat reaches `high` and is mitigated by an executable assertion; nothing is left at `high` undispositioned.

## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| working tree -> public git history | This repo has a public origin. Anything committed is published irreversibly; `.gitignore` is the automated half of the barrier and path-scoped commit discipline is the manual half. |
| workflow trigger + permissions -> GitHub Actions runner | `on:` and `permissions:` decide who can start this job and what token it holds. An edit in this file can widen either. |
| CI runner -> third-party code | Every `uses:` and every download pulls code onto a runner that checks out the repository. |
| built bundle -> system AudioComponent registry | The validate step installs a freshly built binary into the runner user's plug-in directory and asks the OS to load it. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-gxr-01 | Information Disclosure | `logs/` tree -> public git history | medium | mitigate | This is the threat Task 1 exists to close. Today only `*.log` and `.DS_Store` files under the tree are ignored, so a harness that drops a JSON status file or a text report beside a build log produces a path that is untracked but fully visible to a wide `git add` — and build logs routinely carry absolute local paths and machine names. The anchored directory rule covers every extension. Verified by the first clause of Task 1's gate, which is written to fail today and pass only after the edit. |
| T-gxr-02 | Tampering | `.gitignore` scope | medium | mitigate | A directory rule is exactly the over-broad form this file's own comments repeatedly reject: an unanchored variant would silently stop a future nested `plugins/<Name>/logs/` from ever being tracked, and the failure is invisible — the file simply never appears in `git status`. Mitigated by the anchor plus a dedicated negative control in Task 1's gate asserting that a nested path stays visible to git. That control is the only clause an unanchored rule fails. |
| T-gxr-03 | Tampering | `on:` and `permissions:` in ci-tests.yml | high | mitigate | This workflow's header records a standing prohibition on fork-reachable triggers, held in place by it consuming no secrets and declaring read-only contents. An editing accident that widened the trigger to `pull_request` or added a write scope would make the whole repository's CI trust model wrong, and it would look like an ordinary diff. Mitigated by machine assertion, not by care: Task 2's gate reads the PARSED workflow and requires the trigger map to contain exactly `workflow_dispatch` and permissions to equal read-only contents. Reading the parsed structure rather than the text means a comment cannot satisfy the check. |
| T-gxr-04 | Tampering | third-party actions on the runner | low | mitigate | The job checks out the repository, so any action it runs sees the source. The three actions currently referenced are SHA-pinned. This plan adds no action at all — auval ships with macOS and both helper scripts are already in-tree — and Task 2's gate pins the repo-wide count carrying `uses` at exactly 3, so a pasted-in action cannot arrive unnoticed alongside these steps. |
| T-gxr-05 | Elevation of Privilege | freshly built bundle -> runner component registry | low | accept | The validate step installs a binary the same job just compiled from the checked-out source and asks the OS to load it. Provenance is the repository itself, no external artifact is fetched, the runner is ephemeral and single-tenant, and the workflow holds no secret a loaded bundle could reach. Registry staleness — validating a bundle a previous run left behind — is handled as a correctness matter by the pre-copy removal plus the registrar kick already inside `verify-au-link.sh`. |
| T-gxr-06 | Information Disclosure | auval output in the CI log | low | accept | auval prints component metadata, format negotiation and parameter names. This workflow consumes no secrets and holds only read-only contents, so nothing sensitive is in scope to leak; the plugin metadata printed is already public in shipped builds. No log is uploaded as an artifact by the new steps. |
| T-gxr-07 | Tampering | shared `.git/index` and HEAD | medium | mitigate | Concurrent sessions share this checkout, and an unrelated modification under `.claude/agent-memory/` is already dirty in the tree. Mitigated concretely: Task 3 carries a precondition requiring branch and status to be re-read at commit time rather than reused from earlier in the run, prohibits `git add -A` and `git commit -a`, mandates a two-path pathspec, and verifies afterwards that HEAD touches exactly two files, that neither is under `agent-memory`, and — the positive half — that the unrelated modification is still dirty rather than reverted or swept away. |

No supply-chain threat (`T-gxr-SC`) is registered: this plan runs no npm, pip, or cargo install, so the package-legitimacy gate does not apply. `auval`, `python3` and PyYAML are all pre-existing on both the local machine and the `macos-14` runner image; none is installed by this plan.
</threat_model>

<verification>
Run all three task gates in order. Each is self-contained and prints a single PASS token.

1. `GITIGNORE GATE PASS` — the new anchored rule ignores non-`.log` paths under the root tree, does not regress the existing build logs, does not broaden to nested logs directories, and nothing was untracked.
2. `WORKFLOW GATE PASS` + `LOCAL PROXY PASS` — the workflow still parses, the two new steps are present with the correct independent-check conditions, the AU codes are derived rather than mirrored, and the trigger/permissions/action-count security posture is unchanged. The local proxy re-runs the exact commands the new CI steps invoke.
3. `COMMIT GATE PASS` — exactly two files committed on `main`, the unrelated working-tree modification untouched.

**What no gate here can establish.** CI cannot be dispatched from this session, so the added AU build time on `macos-14`, the runner's component-registrar behaviour under a non-interactive session, and whether auval's battery passes on a clean CI checkout are all unmeasured. They are only observable from a dispatched run. One caution worth recording for that first dispatch: locally, `auval -a` did NOT list the installed components even after a registrar kick, while `auval -v` against the derived codes validated cleanly — so if the first CI run is triaged, use the direct validate form and do not read an empty registry listing as a missing bundle.
</verification>

<success_criteria>
- `git check-ignore -v` credits the new anchored rule for a non-`.log` path under the root logs tree, and a nested `plugins/<Name>/logs/` path remains visible to git.
- `git ls-files logs` is empty and no `git rm --cached` was run.
- The `probes-macos` job builds the dispatched plugin's AU target and validates it via the existing derivation gate, with no AU type, subtype or manufacturer code written as a literal in the workflow.
- Both new steps carry `!cancelled()`; the validate step additionally requires its own build to have succeeded.
- The workflow parses, triggers on `workflow_dispatch` only, declares read-only contents, and references exactly 3 third-party actions.
- The new AU build cost is stated in the workflow comment, not left implicit.
- One commit, two files, on `main`; the pre-existing `agent-memory` modification still dirty and unmodified.
</success_criteria>

<output>
Create `.planning/quick/260921-gxr-add-logs-to-gitignore-and-add-an-auval-v/260921-gxr-SUMMARY.md` when done.

Record in it: the two corrected premises (the logs were already covered by `*.log`; the job built no AU), the measured AU build cost if a dispatch was run, and whether the first dispatched run passed auval.
</output>
