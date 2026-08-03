---
phase: quick-260803-bhf
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .gitignore
  - PUBLIC-RELEASE-READINESS.md
autonomous: true
requirements: [BHF-01, BHF-02, BHF-03, BHF-04]
must_haves:
  truths:
    - "Neither installer blob is tracked by git any more, and `git ls-files` on both paths returns nothing."
    - "Both blobs exist MD5-identical outside the repo before anything is removed from tracking."
    - "A future `git add -A` cannot re-add either blob — `git check-ignore -v` resolves both paths to a real rule and `git add -An` on the directory stages zero paths."
    - "The hand-authored source that lives in the same directories — O-Tremolo's `installer.iss` and both `install-readme` text files — is still tracked and still stageable."
    - "Section 6 step 12 reads as done in the same visual style as the twelve already-ticked steps, and records what was removed, where it was preserved, and that history was deliberately left intact."
    - "Section 4.3 carries a resolved heading with a body that does not contradict it, including a correction of its own three-blob / 13.5 MB figure."
    - "No commit rewrites history, force-pushes, or pushes at all."
  artifacts:
    - "~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/O-Polystutter-OuariconAudio.pkg"
    - "~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/PolyStutter.zip"
    - .gitignore
    - PUBLIC-RELEASE-READINESS.md
  key_links:
    - "The ignore rule is added BEFORE the untracking, so the pkg is never simultaneously untracked-and-unignored — the exact window that reopened step 6."
    - "Backup MD5s are compared against the on-disk originals before `git rm` runs, so the preservation gate is proven, not assumed."
    - "The tracked-but-ignored census moves 10 → 9 and contains zero installer extensions — one number proving both that the blob left tracking and that the new rules shadowed nothing else."
---

<objective>
Close readiness checklist step 12: get the two committed O-Polystutter installer blobs out of git tracking, make it impossible for them (or future installer output) to come back, and record the result in `PUBLIC-RELEASE-READINESS.md`.

Purpose: the repository is now public (`github.com/taylorbrook/O-Audio-VST-Development`). Two multi-megabyte v1.8.0-era distribution binaries are tracked in it, which is noise the release workflow already handles properly via GitHub Releases. More importantly this is a repeat of the shape that reopened step 6 — that step untracked `build-release/` but never ignored it, leaving a compiled helper binary one `git add -A` away from returning to a public repo. This plan does untrack **and** ignore, in that safe order, and proves the ignore with a runnable gate rather than asserting it.

Output: two modified files (`.gitignore`, `PUBLIC-RELEASE-READINESS.md`), two files removed from git tracking, and two preservation copies written outside the repository.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@PUBLIC-RELEASE-READINESS.md
@.gitignore
</context>

<scope_boundary>
**This task is checkout-only. The following are explicit NON-GOALS and must not be attempted under any circumstance:**

- No `git filter-repo`. No `git filter-branch`. No history rewrite of any kind.
- No `git push`, no `git push --force`, no remote interaction whatsoever.
- No work on checklist steps 11, 13, or 14. Step 14 is where the history/size reclaim lives; it is deliberately left open.
- No ticking of any checklist step other than 12.
- Do not delete or modify `plugins/O-Polystutter/dist/install-readme.txt`, `plugins/O-Tremolo/dist/install-readme-windows.txt`, or `plugins/O-Tremolo/dist/installer.iss`. All three are tracked, hand-authored source that happens to share a directory name with build output.

The ~9.5 MB stays in `.git` after this task. That is correct and intended — section 4.6 of the readiness doc explains why only a rewrite reclaims it, and that rewrite is step 14's decision to make.
</scope_boundary>

<verified_facts>
Measured this session; treat as ground truth but re-assert the destructive-adjacent ones in Task 2.

- Exactly two installer blobs are tracked repo-wide:
  - `plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg` — 4,756,409 B — MD5 `c778e3de586f536d0972f54336de2463`
  - `plugins/O-Polystutter/dist/PolyStutter.zip` — 4,742,481 B — MD5 `705eae9b8dcf00cc6cf3d0c77e5ca857`
- The third blob named in section 4.3, `O-Polystutter-by-TACHES.pkg`, is **not tracked** (history only). There is nothing to remove for it — do not try.
- `plugins/O-Polystutter/dist/install-readme.txt` is also tracked and **stays** tracked.
- `plugins/O-Tremolo/dist/` holds two tracked source files: `installer.iss` and `install-readme-windows.txt`. This is why a directory-wide ignore rule covering every plugin's dist folder is the wrong tool here — it would silently shadow hand-authored source.
- `.gitignore` today: `*.exe` at line 123, `*.zip` at line 202, `*.log` at line 197, file is 231 lines. There is **no** `*.pkg`, `*.dmg`, or `*.msi` rule. `PolyStutter.zip` is tracked despite `*.zip` because ignore rules never apply to already-tracked files.
- `git ls-files -i -c --exclude-standard` currently returns **10** paths: nine vendored `preset-manager.js` copies (a known, deliberate, deferred situation) plus `PolyStutter.zip`. After this task it must return **9**, all `preset-manager.js`.
- Preservation gate: neither blob matches any GitHub Release asset. The only published O-Polystutter release is `O-Polystutter-v1.12.4` (assets `O-Polystutter-1.12.4-macos.pkg` 10,151,465 B and `O-Polystutter-1.12.4-windows.exe` 4,822,035 B) — different version, different sizes. The local backup at `~/VST-development-releases-backup-20260803/assets/` holds only `O-Polystutter-v1.12.2/` and `O-Polystutter-v1.12.4/`. Hence Task 2's backup copy, which is a locked user decision — do not re-litigate it.
- `.gitmodules` declares one submodule, `plugins/O-Orbit/libs/SAF`. No target path is inside it; the guard in Task 2 is a precaution, not a suspicion.
- Section 6 checklist currently stands at 12 ticked / 5 open (4, 11, 12, 13, 14). After this task: 13 ticked / 4 open.

**Trap worth knowing before you write a gate.** `git check-ignore` consults the index by default and prints **nothing** (rc 1) for a tracked path, even when a rule plainly matches — confirmed on `PolyStutter.zip` against `*.zip`. So while a file is still tracked you must use `--no-index` to see the matching rule; once it is untracked the plain form becomes the meaningful proof. Task 1 uses `--no-index`; Task 2 uses the plain form. Do not swap them.
</verified_facts>

<tasks>

<task type="auto">
  <name>Task 1: Add installer-output ignore rules to .gitignore, before anything is untracked</name>
  <files>.gitignore</files>
  <action>
Append a new commented section to the very end of `.gitignore`, after the final `scratch-pv/` entry. Appending rather than inserting is deliberate: the readiness document cites `.gitignore` line numbers in three places, and an insertion higher up would silently invalidate the ones below it — which is exactly how the `*.log` citation already drifted when step 6 inserted `build-release/` at line 67. Do not renumber anything by inserting mid-file.

The section header follows the file's established banner style — a `# ===` rule line, a title, another `# ===` rule line — and carries rationale prose the way the "Backup Directories" and "Render-Harness / Test-Capture Scratch Output" sections do. Title it for installer and distribution output.

The rules to add are exactly three: `*.pkg`, `*.dmg`, `*.msi`.

The comment must record three things, because each one is a decision a future reader would otherwise have to re-derive:
1. Built installer packages are release artifacts, not source — they belong on GitHub Releases, which the release workflow already publishes to, and every rebuild otherwise stores another multi-megabyte blob in git forever.
2. `*.exe` and `*.zip` already exist elsewhere in this file and cover the Windows-installer and archive forms, so these three close the macOS and Windows-installer gap rather than duplicating.
3. A wildcard directory rule covering every plugin's `dist` folder was considered and **deliberately rejected**, because those folders also hold hand-authored source — name O-Tremolo's Inno Setup script and the install-readme text files as the reason. Without this note someone will "tidy up" by adding the broader rule and silently stop tracking new installer scripts.

Do not add any other rule. Do not modify any existing line. This edit must be pure addition: `git diff --numstat .gitignore` must show zero deletions.
  </action>
  <verify>
    <automated>node -e "process.exit(0)" && bash -c 'set -e; cd "$(git rev-parse --show-toplevel)"; [ "$(grep -v "^#" .gitignore | grep -cx "\*\.pkg")" = 1 ]; [ "$(grep -v "^#" .gitignore | grep -cx "\*\.dmg")" = 1 ]; [ "$(grep -v "^#" .gitignore | grep -cx "\*\.msi")" = 1 ]; [ "$(grep -v "^#" .gitignore | grep -c "plugins/\*/dist/")" = 0 ]; [ "$(grep -n "^\*\.log$" .gitignore | cut -d: -f1)" = 197 ]; [ "$(grep -n "^\*\.zip$" .gitignore | cut -d: -f1)" = 202 ]; [ "$(git diff --numstat -- .gitignore | cut -f2)" = 0 ]; git check-ignore -v --no-index plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg | grep -q ":\*\.pkg"; echo GATE1-PASS'</automated>
  </verify>
  <done>`.gitignore` contains active `*.pkg`, `*.dmg`, and `*.msi` rules and no directory-wide plugin-dist rule; the pre-existing `*.log` and `*.zip` rules still sit on lines 197 and 202 respectively (proving the edit shifted nothing); the diff has zero deletions; and `git check-ignore --no-index` resolves the pkg path to the new `*.pkg` rule. Committed as its own atomic commit.</done>
</task>

<task type="auto">
  <name>Task 2: Preserve both blobs outside the repo, verify by MD5, then untrack them</name>
  <precondition>`~/VST-development-releases-backup-20260803/assets/` exists and is writable, and Task 1's ignore rules are already committed.</precondition>
  <files>plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg, plugins/O-Polystutter/dist/PolyStutter.zip</files>
  <action>
Re-assert the ground state before touching anything, since this is the destructive step. Confirm `git ls-files` lists exactly the two target blobs plus `install-readme.txt` under `plugins/O-Polystutter/dist`, and confirm the two on-disk MD5s still match the values recorded in the verified-facts block above. If either MD5 differs, stop and report — do not proceed on a changed file.

Then, in this order:

**1. Preserve.** Create `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/` and copy both blobs into it. This satisfies the preservation gate literally: neither blob matches any GitHub Release asset, so the backup copy — not git history — is what makes the removal safe. This is a locked decision; do not substitute a different location or skip it because history retains the files.

**2. Verify the preservation.** Compute MD5s of the two copies and compare against the originals. Both must match exactly. Do not run the removal until this comparison has passed — a copy that silently truncated would turn a reversible change into a real loss.

**3. Untrack, keeping the files on disk.** Use the cached form of `git rm` on both paths so they leave the index but remain in the working tree. Keeping them on disk is the deliberate choice, for three reasons: it matches the precedent set by the three prior untracking steps (5, 6, and 7 all untracked without deleting), it is the non-destructive option now that the files are also ignored and therefore inert, and on-disk tidying of stale renders is separately scoped to checklist step 13, which this task must not touch. Record this choice and its rationale in the summary.

**4. Guard the submodule.** Before committing, confirm the staged change touches nothing under `plugins/O-Orbit/libs/SAF` and that `git submodule status` for that path is unchanged. The gitlink surviving is a standing requirement in this repository.

Commit as a single atomic commit. The staged diff must contain exactly two deletions and zero insertions.
  </action>
  <verify>
    <automated>bash -c 'set -e; cd "$(git rev-parse --show-toplevel)"; B="$HOME/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist"; [ "$(md5 -q "$B/O-Polystutter-OuariconAudio.pkg")" = c778e3de586f536d0972f54336de2463 ]; [ "$(md5 -q "$B/PolyStutter.zip")" = 705eae9b8dcf00cc6cf3d0c77e5ca857 ]; [ -z "$(git ls-files plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg plugins/O-Polystutter/dist/PolyStutter.zip)" ]; [ -n "$(git ls-files plugins/O-Polystutter/dist/install-readme.txt)" ]; [ -n "$(git ls-files plugins/O-Tremolo/dist/installer.iss)" ]; [ -f plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg ]; [ -f plugins/O-Polystutter/dist/PolyStutter.zip ]; git check-ignore -v plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg; git check-ignore -v plugins/O-Polystutter/dist/PolyStutter.zip; [ -z "$(git add -An plugins/O-Polystutter/dist/)" ]; [ -z "$(git status --porcelain plugins/O-Polystutter/dist/)" ]; [ "$(git ls-files -i -c --exclude-standard | wc -l | tr -d " ")" = 9 ]; [ "$(git ls-files -i -c --exclude-standard | grep -cvE "preset-manager\.js$" || true)" = 0 ]; [ -z "$(git diff HEAD --name-only -- plugins/O-Orbit/libs/SAF)" ]; echo GATE2-PASS'</automated>
  </verify>
  <done>Both blobs exist MD5-identical in `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/`; both are absent from `git ls-files` yet present on disk; plain index-aware `git check-ignore -v` resolves both to a rule; `git add -An` and `git status --porcelain` on the dist directory are both silent; the tracked-but-ignored census is down to 9 entries, all `preset-manager.js`; `install-readme.txt` and O-Tremolo's `installer.iss` are still tracked; the SAF gitlink is untouched. Committed atomically.</done>
</task>

<task type="auto">
  <name>Task 3: Tick step 12 and re-tense section 4.3 in PUBLIC-RELEASE-READINESS.md</name>
  <files>PUBLIC-RELEASE-READINESS.md</files>
  <action>
Read the already-completed steps 5, 6, and 7 in the section 6 checklist and section headings 2.3, 2.4, and 3.4 first, and match their house style exactly rather than inventing one. The checklist convention is a ticked box, the original step text wrapped in strikethrough, then a checkmark and bold `Done 2026-08-03`, then an em-dash and the substantive record, then the parenthetical section-and-scout-ID citation. The section convention is a heading suffixed with a checkmark and bold `RESOLVED 2026-08-03`, with the body re-tensed to the past so no resolved heading sits over present-tense prose claiming the problem is live — that mismatch is exactly what the previous quick task had to go back and fix.

**Rewrite the step 12 line.** It must record, concretely enough to be checkable later: both blob paths with their byte sizes; that they were untracked with the cached form of `git rm` and left on disk; that they were copied to the named backup directory and MD5-verified *first*, and why that was necessary — neither matches any GitHub Release asset, the only published O-Polystutter release being v1.12.4 at different sizes; which ignore rules were added; and that `git check-ignore` now resolves both removed paths while a dry-run add on the directory stages nothing, i.e. the step-6 residual was not repeated. Close with an explicit statement that **history was intentionally left intact** — this was a checkout-only change, the bytes remain in `.git`, both blobs stay recoverable by addressing the blob at its old commit, and reclaiming that space belongs to step 14, whose section 4.6 proposal already lists the dist path.

**Re-tense section 4.3.** Suffix the heading in the resolved style. Keep the original three-bullet inventory as the historical record, then add a correction the way section 2.2 and section 3.2 corrected their own figures — because the headline number is wrong in a way that matters:

- Only **two** of the three blobs were ever tracked. The `-by-TACHES` one was already history-only at the assessment date, so there was never anything to untrack for it.
- The tracked total was **9,498,890 bytes (~9.5 MB)**, not 13.5 MB.
- The two overlap rather than being independent: the zip contains a copy of the pkg at exactly 4,756,409 bytes plus an install-readme, so the pair is one v1.8.0 distribution package stored twice.

Then add the resolution note. Beyond restating the removal, it must record two things the checklist line has no room for: that `plugins/O-Polystutter/dist/install-readme.txt` remains tracked because it is source rather than output, so the directory was not emptied from git; and that a directory-wide ignore rule over plugin dist folders was considered and rejected because O-Tremolo's Inno Setup script and the install-readme files live in exactly such a folder and must stay tracked. Cross-reference section 4.6's pivotal mechanic so the reader is not left thinking the 912 MB moved — it did not.

**Fix the collateral line-number drift.** Section 3.4's body and checklist step 7 both cite a `.gitignore` line number for the `*.log` rule, in two slightly different formats, and both are one line low — pre-existing drift caused by step 6's insertion at line 67, not by this task. Run `grep -n` to find the line the rule actually occupies and correct both citations to it. This is a citation repair only: do not change either item's status, wording, or tick state.

Touch nothing else. Steps 4, 11, 13, and 14 stay open and unmodified.
  </action>
  <verify>
    <automated>bash -c 'set -e; cd "$(git rev-parse --show-toplevel)"; F=PUBLIC-RELEASE-READINESS.md; [ "$(grep -c "^- \[x\]" $F)" = 13 ]; [ "$(grep -c "^- \[ \]" $F)" = 4 ]; [ "$(grep -c "^- \[ \] \*\*12\." $F)" = 0 ]; grep -q "^- \[x\] ~~\*\*12\." $F; grep -q "^### 4\.3 .*RESOLVED 2026-08-03" $F; [ "$(grep -cE "gitignore.{0,7}196" $F)" = 0 ]; [ "$(grep -cE "gitignore.{0,7}197" $F)" = 2 ]; grep -q "9,498,890" $F; grep -q "O-Polystutter-v1.8.0-repo-dist" $F; for n in 4 11 13 14; do grep -qE "^- \[ \] \*\*$n\." $F; done; git diff --numstat -- $F | awk "{ if (\$2 > 12) exit 1 }"; echo GATE3-PASS'</automated>
    <human-check>Read the rewritten step 12 line and section 4.3 side by side with steps 5-7 and sections 2.3/2.4/3.4. The new text should be indistinguishable in voice and structure from the ones written previously — same tick shape, same date bolding, same citation parenthetical, no resolved heading sitting above live-tense prose.</human-check>
  </verify>
  <done>Section 6 shows 13 ticked and 4 open steps, with 12 ticked in house style and 4, 11, 13, 14 untouched and still open. Section 4.3's heading is marked resolved, its 13.5 MB / three-blob figure is corrected to the measured 9,498,890 bytes across two tracked files, and its body records the retained `install-readme.txt`, the rejected directory-wide ignore rule, and the intact history. The stale `*.log` line citations in section 3.4 and step 7 both now name line 197. Committed atomically.</done>
</task>

</tasks>

<verification>
Run from the repository root after all three commits land:

1. `git ls-files | grep -icE '\.(pkg|exe|dmg|msi|zip|tar\.gz|deb)$'` returns **0** — no installer blob is tracked anywhere in the repository.
2. `git ls-files -i -c --exclude-standard` returns exactly 9 paths, every one a vendored `preset-manager.js`.
3. `git add -An plugins/O-Polystutter/dist/ plugins/O-Tremolo/dist/` stages nothing.
4. `git ls-files plugins/O-Tremolo/dist plugins/O-Polystutter/dist` still lists the three hand-authored source files.
5. `git log --oneline -3` shows three atomic commits and `git status --porcelain` is clean.
6. `git submodule status plugins/O-Orbit/libs/SAF` reports the same commit as before the task.
7. `git reflog | grep -ciE 'filter-repo|filter-branch|push'` returns **0** — the checkout-only boundary held.
</verification>

<success_criteria>
- Zero tracked installer artifacts repo-wide; the two v1.8.0 blobs remain on disk, ignored and inert.
- Both blobs preserved MD5-identically at `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/`.
- `.gitignore` gained exactly three rules plus one rationale banner, as a zero-deletion diff that shifted no existing line number.
- Readiness step 12 ticked; section 4.3 resolved and its own figure corrected; no other checklist step altered except two stale `.gitignore` line citations repaired.
- Three atomic commits, no history rewrite, no push.
</success_criteria>

<output>
Create `.planning/quick/260803-bhf-move-committed-installer-artifacts-out-o/260803-bhf-SUMMARY.md` when done, and **commit it** — leaving it uncommitted blocks worktree cleanup.

The summary must state explicitly:
- Which working-tree disposition was chosen (untrack-only, files kept on disk) and the three reasons for it.
- That the preservation gate was satisfied by the backup copy, not by git history, and the MD5s that prove it.
- That history was left intact by design, with the ~9.5 MB reclaim deferred to step 14.
- The rejected directory-wide dist ignore rule and why, so the decision is not re-derived later.
</output>
