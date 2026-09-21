---
phase: quick-260921-hno
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - scripts/gen-juce-overrides.sh
  - scripts/apply-juce-patches.sh
  - .github/workflows/ci-tests.yml
  - .github/workflows/build-and-release.yml
autonomous: true
requirements: [QUICK-260921-hno]

estimate:
  tokens: 45000
  raw_tokens: 45000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "`gen-juce-overrides.sh --check` exits 0 on the unmodified repo, in BOTH network mode and `--juce-root` mode."
    - "`--check` exits non-zero when any byte of a vendored override changes, naming the file and showing the diff."
    - "`--check` exits non-zero when the patch changes without the vendored overrides being re-cut."
    - "`--check` hard-errors when `.github/juce-version.txt` names a version with no matching patch file."
    - "`--check` exits non-zero when a file rides along in `vendored/JUCE-overrides/modules/` that the patch does not produce."
    - "`--write` is refused when `CI` is set, and writes nothing in that case."
    - "`--write` on the unmodified repo reproduces the committed vendored bytes byte-for-byte (`git status` clean)."
    - "Both workflows run `--check` against the pristine JUCE tree before their override copy."
    - "`check-juce-overrides.sh --check-wiring` still exits 0 after the new steps are added (no adjacency regression)."
  artifacts:
    - scripts/gen-juce-overrides.sh
  key_links:
    - ".github/juce-version.txt <-> scripts/juce-patches/note-expression-juce-<version>.patch (derived, never mirrored)"
    - "patch `--- a/<path>` lines <-> the file set under vendored/JUCE-overrides/modules/ (set equality)"
    - "`gen-juce-overrides.sh --check` step <-> `cp -R vendored/JUCE-overrides/modules/.` (must precede it)"
    - "apply-juce-patches.sh PATCH_FILE <-> .github/juce-version.txt (same derived link as the generator)"
---

<objective>
Make `vendored/JUCE-overrides/` a *generated* artifact of
`scripts/juce-patches/note-expression-juce-<pinned>.patch` applied to pristine JUCE, and prove that
in CI, so the local patch path and the CI whole-file path cannot drift apart.

Purpose: `scripts/check-juce-overrides.sh` (quick task 260921-ha2) proves the overrides were cut
from the JUCE version now pinned. It does NOT prove they were cut **by the patch**. Today the two
representations of the same local fork — a 5 KB patch that only the local machine applies, and two
177 KB whole files that only CI copies — are linked by nothing but a human remembering to update
both. Edit one and no gate anywhere notices.

Output: a generator with a `--check` mode wired into both workflows, plus `apply-juce-patches.sh`
deriving its patch filename from the same pinned-version file the generator uses.

This is purely additive. The existing provenance guard and the existing `cp -R` copy path are not
modified — the new check runs as its own step between them.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md

Read before writing code:
@scripts/check-juce-overrides.sh
@scripts/apply-juce-patches.sh
@scripts/juce-patches/note-expression-juce-8.0.14.patch
@.planning/quick/260921-ha2-add-a-ci-guard-that-fails-when-vendored-/260921-ha2-SUMMARY.md

Live observations made at planning time (2026-09-21). These are the authority for the design
below — several were measured, not assumed, and one **has already proven the whole thesis**.

**F1 — THE FEASIBILITY PROBE ALREADY PASSED.** The planner ran the exact pipeline this plan
specifies: fetch the two upstream files at tag `8.0.14` from `raw.githubusercontent.com`,
CRLF→LF normalize, `patch -p1 -F0` with the 8.0.14 patch, then compare CR-stripped sha256 against
the committed vendored overrides:

| file | generated | committed vendored | match |
|---|---|---|---|
| `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` | `c02f2774…a416ec8c` | `c02f2774…a416ec8c` | YES |
| `modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h` | `4e2b9023…739ed720` | `4e2b9023…739ed720` | YES |

`patch` applied both hunks with **zero fuzz** and left no `.rej`/`.orig`. Both generated values also
equal the `<override_sha>` column already recorded in `MANIFEST.txt`. The generator is not
speculative — build it to reproduce this result, and treat any deviation as a bug in the script,
not in the thesis.

**F2 — every upstream source measured is CRLF, and so are the committed overrides.**

| source | CR bytes | LF bytes | verdict |
|---|---|---|---|
| `raw.githubusercontent.com/.../8.0.14/...VST3.cpp` | 4062 | 4062 | uniformly CRLF |
| `raw.githubusercontent.com/.../8.0.14/...Extensions.h` | 305 | 305 | uniformly CRLF |
| `vendored/.../juce_audio_plugin_client_VST3.cpp` | 4112 | 4112 | uniformly CRLF |
| `vendored/.../juce_VST3ClientExtensions.h` | 339 | 339 | uniformly CRLF |

The osx release zip CI downloads is **also CRLF** — measured by the prior task (260921-ha2 SUMMARY,
"Corrections to Planning-Time Findings"; its own PLAN had wrongly called it LF). The line deltas
confirm the derivation arithmetic exactly: cpp 4062 → 4112 = +50 lines and the patch's cpp hunk is
`@@ -3588,7 +3588,57 @@` = +50; header 305 → 339 = +34 and its hunk is `@@ -59,7 +59,41 @@` = +34.

**F3 — the two line-ending axes are separate decisions, and both must be made.**

1. *Applying* the patch needs an LF base: the patch is LF and GNU/Apple `patch` rejects hunks whose
   context line endings differ. `apply-juce-patches.sh` already CRLF→LF normalizes its targets for
   exactly this reason (its step 3.5).
2. *Comparing* the result to the committed tree crosses an uncontrolled axis: `.gitattributes`
   carries **only** a `PLUGINS.md merge=union` line — there is no `eol`/`text` entry for
   `vendored/`, so an autocrlf checkout of this repo is unpinned.

Decision: the generator normalizes to LF internally, patches, then re-emits **CRLF** on write (keeps
`--write` byte-stable against today's committed tree, so a regeneration during a bump produces a
content diff rather than 4451 lines of line-ending churn); `--check` compares **CR-stripped** bytes,
matching the convention `MANIFEST.txt` already documents. Same declared blindness as the manifest: a
line-ending-only change is invisible, deliberately — `cp -R` is byte-preserving and neither CMake
nor the compiler cares about CR.

**F4 — the pristine-base decision.** `/Users/taylorbrook/JUCE` is NOT usable as a base: it is
patched (prior task NC4 proved it fails provenance check 3 against 8.0.14), it is not a git repo (no
`.git`, so no `git show <tag>:<path>`), and `apply-juce-patches.sh` has destructively CRLF→LF
normalized it in place, so it is not byte-equal to any distribution. Reverse-applying the patch is
therefore rejected — it would yield an LF tree with no provenance at all.

Decision, mirroring `check-juce-overrides.sh` exactly so there is one mechanism and not two:

- **default (local runs):** fetch each patched path from
  `https://raw.githubusercontent.com/juce-framework/JUCE/<pinned>/<path>`. Verified live today:
  HTTP 200 for both paths, and both CR-stripped sha256 values equal the `<upstream_sha>` column
  already recorded in `MANIFEST.txt` (`4b8ff18d…fec4e2`, `5488c7ef…c2a4c0`). Two files, ~170 KB —
  no 52 MB zip needed.
- **`--juce-root DIR` (CI):** read the pristine tree already on disk. Zero network, and it is the
  exact bytes about to be overwritten.

**F5 — `patch` on macOS is `patch 2.0-12u11-Apple`.** Usage is
`patch [-bCcEeflNnRstuv] ... [-F max-fuzz] [-i patchfile] [-o out-file] [-p strip-count] [-r rej-name]`.
`-F0`, `-t` (batch), `-r`, `-p1` and `-i` all exist on both Apple patch and GNU patch. It creates no
`.orig` unless `-b` is given (the probe produced none). Prefer these short flags over GNU long
options for portability.

**F6 — CI topology (re-grep before editing; line numbers drift).** There is **no push/PR-triggered
workflow in this repo at all.** `ci-tests.yml` is `workflow_dispatch:` only and carries the standing
"NO BUILD ON PUSH" rule in its header; `build-and-release.yml` fires on a release tag push and holds
eight Apple signing secrets. Do NOT add a new workflow or a new trigger. Four copy sites exist:

| workflow | job | Setup JUCE | provenance guard | `cp -R vendored/...` |
|---|---|---|---|---|
| `ci-tests.yml` | `probes-macos` | 120 | 131–133 | 140 |
| `ci-tests.yml` | `windows-vst3` | 329 | 342 | 347 |
| `build-and-release.yml` | `build-macos` | 148 | 161–163 | 169 |
| `build-and-release.yml` | `build-windows` | 522 | 537 | 543 |

**F7 — the new step goes in the macOS jobs only, and that is a decision, not an omission.** The
regeneration check is a property of *repo content* (`scripts/juce-patches/*.patch` versus
`vendored/JUCE-overrides/**`), not of the runner OS, so one check per workflow run discharges it.
Putting it in the Windows jobs would add a hard dependency on `patch` being present in the Windows
runner's git-bash — a new failure mode in the secret-carrying release workflow, for zero added
coverage. The Windows jobs keep their existing provenance guard, unchanged.

**F8 — adjacency risk to guard against.** `check-juce-overrides.sh --check-wiring` greps the literal
`check-juce-overrides\.sh --juce-root JUCE` and pairs hit-count against copy-count per workflow. The
new step's command (`gen-juce-overrides.sh --check --juce-root JUCE`) does not contain that
substring, so it should not be counted — but this must be **verified**, not assumed: a miscount
would turn the working wiring gate red.

**Scratch dir for this task (used by the verify commands below):**
`/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/62fc2fda-da15-4a01-8b77-95d07ce4d206/scratchpad`
</context>

<tasks>

<task type="tracer" tdd="true">
  <name>Task 1: End-to-end regeneration — pristine JUCE through the patch to a PASS/FAIL verdict</name>
  <files>scripts/gen-juce-overrides.sh</files>
  <read_first>
    `scripts/check-juce-overrides.sh` — copy its structure where it applies: the
    `REPO_ROOT`/`OVERRIDE_ROOT`/`VERSION_FILE`/`RAW_BASE` header block, the `die()` helper, the
    runtime `shasum`/`sha256sum` detection, the hardened
    `curl --fail --show-error --silent --location --proto '=https' --tlsv1.2`, and its "a fetch
    failure is a HARD ERROR, never a skipped check" stance. One mechanism, not two.
    `scripts/apply-juce-patches.sh` step 3.5 — the portable in-place CRLF→LF normalization.
    `scripts/juce-patches/note-expression-juce-8.0.14.patch` — note the leading `#` comment block
    before the first `--- a/` line; `patch` skips it, as the F1 probe confirmed.
  </read_first>
  <behavior>
    - `--check` on the unmodified repo, network mode: exit 0; the PASS line names the pinned version and the resolved patch file.
    - `--check --juce-root <pristine tree>`: exit 0, no network call made.
    - `--write` on the unmodified repo: rewrites both vendored files byte-identically — `git status --short -- vendored/` stays empty.
    - `--write` with `CI=true`: exit non-zero, nothing written.
    - `.github/juce-version.txt` naming a version with no patch file: hard error naming the expected patch path, raised before any network call.
  </behavior>
  <action>
    Create `scripts/gen-juce-overrides.sh` (bash 3.2 — the macOS system bash; no bash-4
    constructs), `set -euo pipefail`, mode 0755. Modes: `--check` (default), `--write`,
    `--juce-root DIR`, `-h|--help`. Follow the existing guard's argument-parsing loop shape.

    Give it a header comment block in the same voice as the existing guard, stating WHAT THIS GATE
    CLAIMS (that `vendored/JUCE-overrides/modules/` is exactly what the pinned patch produces from
    pristine JUCE) and WHAT IT DOES NOT CLAIM (blind to line-ending-only changes, by design, per F3;
    does not verify the JUCE download itself; not insider-resistant — a human can run `--write` to
    launder an edit, the same accepted limitation the manifest's `--update` carries).

    Pipeline shared by `--check` and `--write`:

    1. Resolve `PINNED_VERSION` from `.github/juce-version.txt` (`tr -d '[:space:]'`). Hard-error on
       missing or empty, reusing the existing guard's wording.
    2. Resolve `PATCH_FILE` as
       `$REPO_ROOT/scripts/juce-patches/note-expression-juce-$PINNED_VERSION.patch`. This derived
       link is the point of the task — never hardcode a version literal. If absent, hard-error:
       name the exact expected path and instruct the reader to cut a patch against pristine JUCE
       `$PINNED_VERSION` and commit it under that name. Raise this before any network call so a
       bump fails instantly and offline.
    3. Derive the target path set FROM THE PATCH, not from the vendored tree:
       `grep '^--- a/' "$PATCH_FILE" | sed 's|^--- a/||' | awk '{print $1}' | LC_ALL=C sort -u`.
       The `awk` field pick is required — the 8.0.9-era patch format carries a tab-separated
       timestamp after the path. Hard-error if the set comes back empty. Deriving from the patch is
       what makes a newly patched third file surface as a mismatch instead of being ignored.
    4. `WORK="$(mktemp -d)"`, cleaned on EXIT via trap. For each path put the pristine bytes at
       `$WORK/<path>` (`mkdir -p` the parents):
       - `--juce-root DIR` given: copy `$DIR/<path>`; hard-error with an actionable message if it is
         missing. Copy — never read-modify the JUCE tree in place; in CI that tree is the build input.
       - otherwise: `curl` from `$RAW_BASE/$PINNED_VERSION/<path>` with the flags named in
         read_first. A fetch failure is a hard error whose message offers `--juce-root` as the
         offline path.
    5. CRLF→LF normalize every file in `$WORK` using the portable form `apply-juce-patches.sh`
       already uses (`sed -i.bak 's/\r$//'` then remove the `.bak`).
    6. Apply: `( cd "$WORK" && patch -p1 -F0 -t -r "$REJ" -i "$PATCH_FILE" )` where `$REJ` is a
       `mktemp` path OUTSIDE `$WORK` — a reject file inside the work tree would be walked as a
       generated artifact. `-F0` forbids fuzz, because a fuzzed apply silently produces a shifted
       result, which is the exact drift this gate exists to catch. Hard-error if `patch` exits
       non-zero, if `$REJ` exists non-empty, or if any `*.rej`/`*.orig` is found under `$WORK`.
       Hard-error if `patch` is not on PATH — never a skipped check.
    7. LF→CRLF re-emit each generated file per the F3 decision, via
       `awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }'` into a temp file, then move it into place.
       Do NOT reach for `sed 's/$/\r/'`: BSD/macOS sed does not interpret that escape in the
       replacement and emits a literal letter instead of a carriage return.

    `--check` then compares the generated set against `vendored/JUCE-overrides/modules/`
    (`cd "$OVERRIDE_ROOT" && find modules -type f | LC_ALL=C sort`; `MANIFEST.txt` sits above
    `modules/` and is out of scope here):
    - only in generated → fail: the patch produces this path but `vendored/JUCE-overrides/` does not
      carry it; tell the reader to re-run with `--write`.
    - only on disk → fail: this path is committed under `vendored/JUCE-overrides/modules/` but the
      patch does not produce it, and an unlisted file cannot ride along into the JUCE tree.
    - in both → compare CR-stripped bytes with
      `diff -u <(tr -d '\r' < "$gen") <(tr -d '\r' < "$ven")`. On mismatch, fail and print the
      first ~40 lines of that unified diff, so the failure is actionable rather than a sha pair.

    Accumulate failures in a counter like the existing guard. Print a green per-file line only for a
    file that cleared its comparison, and a red one to stderr otherwise — the existing guard was
    bug-fixed twice for printing a green line beside its own failure (commits `cf6e6d69`,
    `95123893`); do not reintroduce that shape. Exit 1 if the counter is non-zero, else print a PASS
    line naming `$PINNED_VERSION` and the resolved patch file.

    `--write` refuses outright when `${CI:-}` is non-empty, with a message saying CI must only ever
    run `--check` because a write in CI would launder drift into a green run. Otherwise copy each
    generated file over `vendored/JUCE-overrides/<path>` (`mkdir -p` parents), leave `MANIFEST.txt`
    untouched, and print the follow-up instruction to re-run
    `bash scripts/check-juce-overrides.sh --update` so the manifest fingerprints match the newly
    written bytes.
  </action>
  <verify>
    <automated>
cd /Users/taylorbrook/Dev/VST-development
SP=/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/62fc2fda-da15-4a01-8b77-95d07ce4d206/scratchpad
mkdir -p "$SP/JUCE/modules/juce_audio_plugin_client" "$SP/JUCE/modules/juce_audio_processors_headless/utilities"
for p in modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h; do
  curl --fail --show-error --silent --location --proto '=https' --tlsv1.2 \
    "https://raw.githubusercontent.com/juce-framework/JUCE/8.0.14/$p" -o "$SP/JUCE/$p" || exit 1
done
bash scripts/gen-juce-overrides.sh --check;                 echo "NETWORK_CHECK_EXIT=$?"
bash scripts/gen-juce-overrides.sh --check --juce-root "$SP/JUCE"; echo "ROOT_CHECK_EXIT=$?"
bash scripts/gen-juce-overrides.sh --write;                 echo "WRITE_EXIT=$?"
GIT_OUT=$(git status --short -- vendored/); GIT_RC=$?
[ "$GIT_RC" -eq 0 ] || { echo "GIT_STATUS_FAILED rc=$GIT_RC"; exit 1; }
echo "VENDORED_DIRTY_LINES=$(printf '%s' "$GIT_OUT" | grep -c . || true)"
CI=true bash scripts/gen-juce-overrides.sh --write;         echo "CI_WRITE_EXIT=$?"
    </automated>
  </verify>
  <done>
    `NETWORK_CHECK_EXIT=0`, `ROOT_CHECK_EXIT=0`, `WRITE_EXIT=0`, `VENDORED_DIRTY_LINES=0`
    (the write reproduced the committed bytes exactly — the thesis, proven by the script itself),
    and `CI_WRITE_EXIT` non-zero with `vendored/` still clean. The reusable pristine tree now exists
    at `$SP/JUCE` for tasks 2 and 3.
  </done>
</task>

<task type="auto">
  <name>Task 2: Falsify the gate — six negative controls, each restored by reverse edit</name>
  <files>scripts/gen-juce-overrides.sh</files>
  <read_first>
    Task 1's script. No new file is created here; edits to `gen-juce-overrides.sh` in this task are
    only the fixes any control exposes.
  </read_first>
  <action>
    A gate never observed failing is not a gate. Run every control below, record the **verbatim**
    exit code and the naming line of each failure message for the SUMMARY, and restore after each
    one before starting the next.

    **Restoration rule (hard):** restore by targeted REVERSE EDIT only. Never
    `git checkout`/`git restore` a file to undo a control — the working tree also carries the
    unrelated pre-existing `M .claude/agent-memory/research-planning-agent.md` and, during this
    task, your own uncommitted script work; a checkout-based restore has wiped exactly that kind of
    in-flight fix before in this repo.

    - **NC1 — vendored override drifted.** Append a line such as a `// NC1-DRIFT` comment to the end
      of `vendored/JUCE-overrides/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h`.
      Expect `--check` exit 1, the failing path named, and a unified diff printed. Delete that exact
      line to restore.
    - **NC2 — the patch drifted, the overrides did not.** In
      `scripts/juce-patches/note-expression-juce-8.0.14.patch`, change one character inside an
      ADDED (`+`) line — for example the `noteId` default `-1` to `-2` — so hunk line counts are
      unchanged and the patch still applies cleanly. Expect `--check` exit 1 on the cpp/header the
      edit lands in. This is the control that proves the two representations are actually tied
      together; without it the task has delivered nothing. Reverse-edit the character back.
    - **NC3 — bump with no patch.** Set `.github/juce-version.txt` to `8.0.15`. Expect a hard error
      naming `scripts/juce-patches/note-expression-juce-8.0.15.patch` as the missing file, raised
      before any network call (verify that claim by re-running the control with networking
      irrelevant — the error must appear immediately, not after a fetch timeout). Restore `8.0.14`.
    - **NC4 — a rider file.** Create
      `vendored/JUCE-overrides/modules/juce_core/juce_core_NC4.cpp` with any content. Expect
      `--check` exit 1 reporting it as present on disk but not produced by the patch. Remove the
      file and the now-empty `juce_core` directory.
    - **NC5 — `--write` under CI.** Already exercised in task 1; re-confirm here after any script
      change and capture its message verbatim.
    - **NC6 — non-pristine base.** Run `--check --juce-root /Users/taylorbrook/JUCE` (the real
      patched local tree). Expect a non-zero exit: the base already contains the patch, so either
      `patch` refuses the already-applied hunks or the regenerated output differs from the vendored
      bytes. Record which of the two it is and make sure the message explains it rather than
      surfacing a raw `patch` error — if it does not, improve the message. This is the strongest
      control because the tree is genuinely non-pristine rather than synthetically edited.

    If any control exposes a defect in the script, fix it and re-run EVERY control from the top —
    a control that passed against a prior revision has not been observed against the shipped one.
  </action>
  <verify>
    <automated>
cd /Users/taylorbrook/Dev/VST-development
SP=/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/62fc2fda-da15-4a01-8b77-95d07ce4d206/scratchpad
# All controls restored: both modes green again.
bash scripts/gen-juce-overrides.sh --check;                        echo "POST_RESTORE_NETWORK=$?"
bash scripts/gen-juce-overrides.sh --check --juce-root "$SP/JUCE"; echo "POST_RESTORE_ROOT=$?"
bash scripts/check-juce-overrides.sh --juce-root "$SP/JUCE";       echo "PROVENANCE_GUARD=$?"
# No control residue anywhere it could hide.
GIT_OUT=$(git status --short -- vendored/ scripts/juce-patches/ .github/juce-version.txt); GIT_RC=$?
[ "$GIT_RC" -eq 0 ] || { echo "GIT_STATUS_FAILED rc=$GIT_RC"; exit 1; }
echo "RESIDUE_LINES=$(printf '%s' "$GIT_OUT" | grep -c . || true)"
echo "NC_TOKENS=$(grep -rn 'NC1-DRIFT\|NC4' vendored/ scripts/juce-patches/ 2>/dev/null | wc -l | tr -d ' ')"
echo "VERSION_PIN=$(tr -d '[:space:]' < .github/juce-version.txt)"
    </automated>
  </verify>
  <done>
    All six controls observed failing with their exit codes and naming lines recorded verbatim for
    the SUMMARY. After restoration: `POST_RESTORE_NETWORK=0`, `POST_RESTORE_ROOT=0`,
    `PROVENANCE_GUARD=0`, `RESIDUE_LINES=0`, `NC_TOKENS=0`, `VERSION_PIN=8.0.14`.
  </done>
</task>

<task type="auto">
  <name>Task 3: Wire the check into both workflows and close the patch-filename mirror</name>
  <files>.github/workflows/ci-tests.yml, .github/workflows/build-and-release.yml, scripts/apply-juce-patches.sh, scripts/gen-juce-overrides.sh</files>
  <read_first>
    Re-grep the copy sites before editing — the F6 line numbers are a planning-time snapshot and
    drifted within a single day on the prior task:
    `grep -n 'Setup JUCE\|check-juce-overrides\|cp -R vendored/JUCE-overrides' .github/workflows/*.yml`
    The wiring-check mode in `scripts/check-juce-overrides.sh` (its `MODE=wiring` branch) is the
    template for the new script's own wiring mode.
  </read_first>
  <action>
    **3a — add a `--check-wiring` mode to `scripts/gen-juce-overrides.sh`.** Two assertions, both
    pure text analysis over `.github/workflows/build-and-release.yml` and
    `.github/workflows/ci-tests.yml`, no network and no JUCE tree:
    1. Neither workflow invokes `gen-juce-overrides.sh` with `--write`. This is the live enforcement
       of threat `T-hno-03`: a `--write` in CI would rewrite the vendored tree to match whatever the
       patch currently says and report green. Refusing at runtime (task 1) plus asserting absence
       here gives the mitigation two independent legs.
    2. Every workflow that contains a `cp -R vendored/JUCE-overrides` site contains at least one
       `gen-juce-overrides.sh --check` invocation at a LOWER line number than its FIRST copy site.
       Deliberately "at least one before the first copy", NOT the 1:1 pairing
       `check-juce-overrides.sh --check-wiring` uses — the new step is wired macOS-only by decision
       F7, so a 1:1 pairing would false-fail by construction. Say so in a comment at the assertion,
       or a later reader will "fix" it into a false-failing pair.
    Follow the existing guard's per-workflow accounting shape, and its fix: print a green
    per-workflow summary line only for a workflow that cleared every assertion.

    **3b — insert the CI step**, in `ci-tests.yml` job `probes-macos` and `build-and-release.yml`
    job `build-macos` ONLY (decision F7). Place it immediately AFTER the existing
    `Verify JUCE overrides provenance` step and BEFORE `Apply JUCE overrides`. Do not modify either
    existing step, and do not touch the two Windows jobs.

    ```yaml
      # Proves vendored/JUCE-overrides/ is still exactly what
      # scripts/juce-patches/note-expression-juce-<pinned>.patch produces from a
      # pristine JUCE. The provenance guard above proves the overrides were cut
      # from THIS JUCE; this proves they were cut BY THE PATCH — so the local
      # patch path and the vendored whole-file path cannot drift apart.
      # macOS-only on purpose: this is a property of repo content, not of the
      # runner, so once per workflow run discharges it — and it keeps the release
      # workflow free of a dependency on `patch` existing in Windows git-bash.
      # Like the guard above it must run BEFORE the copy: the copy destroys the
      # pristine tree this check reads.
      - name: Verify vendored overrides regenerate from the patch
        shell: bash
        run: bash scripts/gen-juce-overrides.sh --check --juce-root JUCE
    ```

    **3c — close the last mirror.** `scripts/apply-juce-patches.sh` hardcodes
    `note-expression-juce-8.0.14.patch`. That is a second literal for a version that already has a
    single source of truth, and it drifts in the worst direction: after a bump it would keep
    applying the old patch to a new tree. Derive it instead — read
    `$SCRIPT_DIR/../.github/juce-version.txt`, hard-error if missing or empty, and build
    `PATCH_FILE="$PATCH_DIR/note-expression-juce-${JUCE_VERSION}.patch"`. Echo the resolved version
    and patch path so the run states which patch it chose. Today this resolves to the identical file
    (the pin reads `8.0.14`), so there is no behaviour change now and a correct one after a bump —
    prove the no-change claim in the verify below. Leave every other step of that script alone,
    including its idempotency marker check and its in-place CRLF→LF normalization.
  </action>
  <verify>
    <automated>
cd /Users/taylorbrook/Dev/VST-development
SP=/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/62fc2fda-da15-4a01-8b77-95d07ce4d206/scratchpad
# New wiring mode, and NO adjacency regression in the existing guard's wiring mode (F8).
bash scripts/gen-juce-overrides.sh --check-wiring;    echo "GEN_WIRING=$?"
bash scripts/check-juce-overrides.sh --check-wiring;  echo "EXISTING_WIRING=$?"
# Both workflows still parse as YAML.
python3 -c "import yaml,sys; [yaml.safe_load(open(f)) for f in ['.github/workflows/ci-tests.yml','.github/workflows/build-and-release.yml']]; print('YAML_OK')"
# Step placement: one --check per workflow, macOS jobs only, zero --write anywhere.
echo "GEN_CHECK_STEPS=$(grep -c 'gen-juce-overrides.sh --check --juce-root JUCE' .github/workflows/ci-tests.yml .github/workflows/build-and-release.yml | paste -sd, -)"
echo "GEN_WRITE_IN_CI=$(grep -c 'gen-juce-overrides.sh.*--write' .github/workflows/*.yml | awk -F: '{s+=$2} END{print s}')"
# apply-juce-patches.sh resolves to the SAME patch file as before (no behaviour change today).
echo "RESOLVED_PATCH=$(bash -c 'V=$(tr -d "[:space:]" < .github/juce-version.txt); echo scripts/juce-patches/note-expression-juce-$V.patch')"
grep -c 'note-expression-juce-8\.0\.14\.patch' scripts/apply-juce-patches.sh | sed 's/^/HARDCODED_LITERAL_REMAINING=/'
# Full CI sequence simulated against the pristine tree, then the drift case.
rm -rf "$SP/JUCEsim" && cp -R "$SP/JUCE" "$SP/JUCEsim"
bash scripts/check-juce-overrides.sh --juce-root "$SP/JUCEsim"        && \
bash scripts/gen-juce-overrides.sh   --check --juce-root "$SP/JUCEsim" && \
cp -R vendored/JUCE-overrides/modules/. "$SP/JUCEsim/modules/"         && \
grep -q "JUCE-NE-PATCH" "$SP/JUCEsim/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h" && \
grep -q "JUCE-NE-PATCH" "$SP/JUCEsim/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp" && echo "CI_SEQUENCE_OK"
    </automated>
    <human-check>
      In the rendered diff of both workflows, confirm the new step sits between
      `Verify JUCE overrides provenance` and `Apply JUCE overrides`, and that no line of either
      pre-existing step changed.
    </human-check>
  </verify>
  <done>
    `GEN_WIRING=0`, `EXISTING_WIRING=0` (the existing gate did not regress), `YAML_OK`,
    `GEN_CHECK_STEPS` reports exactly 1 per workflow, `GEN_WRITE_IN_CI=0`,
    `HARDCODED_LITERAL_REMAINING=0` with `RESOLVED_PATCH` pointing at the 8.0.14 patch that is
    already committed, and `CI_SEQUENCE_OK` printed. Then, as the closing negative control,
    re-inject NC1 into a vendored override, confirm the simulated sequence now stops at
    `gen-juce-overrides.sh --check` with a non-zero exit BEFORE the `cp -R` runs, and reverse-edit
    it out — never by checkout.
  </done>
</task>

</tasks>

<threat_model>
ASVS level 1. Blocking threshold: `high`. No npm/pip/cargo installs are in scope for this task, so
no package-legitimacy audit or install checkpoint applies.

## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| `raw.githubusercontent.com` → local work tree | Untrusted network bytes become the derivation base for source files that are compiled into every shipped plugin |
| `scripts/juce-patches/*.patch` → generated override bytes | A repo file is executed as a content transformation over those source files |
| repo working tree → CI runner | A script in the repo decides whether the release workflow proceeds past the override copy |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-hno-01 | Tampering | upstream fetch in `gen-juce-overrides.sh` (default mode) | medium | mitigate | Reuse the existing guard's hardened `curl --fail --show-error --silent --location --proto '=https' --tlsv1.2` against a tag-pinned path; CI never uses this mode (it runs `--juce-root JUCE`, zero network); any fetch failure is a hard error, never a skipped check |
| T-hno-02 | Tampering | `scripts/juce-patches/note-expression-juce-<v>.patch` as transformation input | medium | mitigate | `patch -p1 -F0` forbids fuzz so a shifted apply cannot pass silently; non-zero exit, a non-empty reject file, or any `*.rej`/`*.orig` is fatal; the resulting bytes remain independently pinned by `MANIFEST.txt` via the pre-existing `check-juce-overrides.sh` |
| T-hno-03 | Elevation of Privilege | `--write` mode reachable from CI | high | mitigate | `--write` refuses outright when `CI` is set and writes nothing (task 1); `--check-wiring` asserts no workflow invokes `--write` at all (task 3a) — two independent legs, verified by NC5 and `GEN_WRITE_IN_CI=0` |
| T-hno-04 | Tampering | file set derived from the vendored tree instead of the patch | medium | mitigate | The target set is parsed from the patch's `--- a/` lines; set equality against `modules/` on disk fails in both directions, so neither a rider file nor a newly patched file can pass unnoticed (NC4) |
| T-hno-05 | Denial of Service | network unavailable during a local `--check` | low | accept | Local-only failure mode; the error names `--juce-root` as the offline path and CI does not use the network mode at all |
| T-hno-06 | Information Disclosure | new step added to the secret-carrying release workflow | low | mitigate | The step consumes no secrets, adds no trigger, and runs in the existing `build-macos` job; no new workflow file and no fork-reachable trigger is created (F6) |
| T-hno-07 | Repudiation | a human runs `--write` to launder an edited override | medium | accept | Identical accepted limitation to the manifest's `--update` (prior task T-ha2-03). The gate targets silent drift, not a motivated insider; the write shows up as a plain-text diff in review |
</threat_model>

<verification>
1. `bash scripts/gen-juce-overrides.sh --check` exits 0 (network mode).
2. `bash scripts/gen-juce-overrides.sh --check --juce-root <pristine 8.0.14 tree>` exits 0.
3. `bash scripts/gen-juce-overrides.sh --write` leaves `git status --short -- vendored/` empty.
4. Six negative controls observed failing, each with its exit code and naming line recorded verbatim.
5. `bash scripts/gen-juce-overrides.sh --check-wiring` exits 0.
6. `bash scripts/check-juce-overrides.sh --check-wiring` still exits 0 — no adjacency regression.
7. Both workflows parse as YAML; the new step appears once per workflow, in the macOS job only,
   between the provenance guard and the copy.
8. `scripts/apply-juce-patches.sh` carries no hardcoded version literal and resolves to the
   already-committed 8.0.14 patch today.
9. Simulated CI sequence passes end-to-end against a pristine tree, and stops before `cp -R` when a
   vendored override is drifted.
10. `git status --short` shows no control residue and no unrelated file staged; the
    `plugins/O-Orbit/libs/SAF` submodule is untouched.
</verification>

<success_criteria>
`vendored/JUCE-overrides/modules/` is a reproducible product of
`scripts/juce-patches/note-expression-juce-<pinned>.patch` + pristine JUCE at the pinned version,
that fact is asserted in both workflows before the override copy, and the assertion has been
observed failing for every way the two representations can drift apart: a changed override, a
changed patch, a version bump without a patch, a rider file, and a non-pristine base.
</success_criteria>

<commit_discipline>
Trunk-based on `main`, single checkout, concurrent sessions share `.git/index` and HEAD.

Immediately before EVERY commit (not once at task start — a snapshot minutes old is not a guarantee):

```bash
git branch --show-current
git status --short
```

Path-scope every commit. Never `git add -A`, never `git commit -a`:

```bash
git commit -- scripts/gen-juce-overrides.sh scripts/apply-juce-patches.sh .github/workflows
```

Do NOT stage `.claude/agent-memory/research-planning-agent.md` (pre-existing, unrelated, already
modified in the tree). Do not touch the `plugins/O-Orbit/libs/SAF` submodule. No tags, no pushes,
no plugin builds in this task.

One commit per task: `feat(quick-260921-hno): ...`, `test(quick-260921-hno): ...`,
`feat(quick-260921-hno): ...`.
</commit_discipline>

<output>
Create `.planning/quick/260921-hno-generate-vendored-juce-overrides-from-sc/260921-hno-SUMMARY.md`
when done. It must carry the verbatim exit code and naming line of each of the six negative
controls, the resolved `RESOLVED_PATCH`/`EXISTING_WIRING` values, and any correction to the
planning-time findings F1–F8.
</output>
