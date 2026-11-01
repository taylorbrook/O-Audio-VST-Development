---
name: finish-cleanup
description: Close out the 2026-10-29 repo cleanup (quick 261029-md9) — backup mirror, private remote, registry script, STATUS versions, remaining audit batches
argument-hint: "(no arguments)"
---

# /finish-cleanup

Finish quick task **261029-md9** in one pass. Everything you need is on disk. Don't re-audit.

## Read first

- `.planning/quick/261029-md9-do-a-thorough-review-through-this-repo-f/261029-md9-AUDIT.md`. It has the findings table F-01..F-28 and batches B1–B14.
- `.planning/quick/261029-md9-do-a-thorough-review-through-this-repo-f/261029-md9-SUMMARY.md`
- `.planning/quick/261029-md9-do-a-thorough-review-through-this-repo-f/261029-md9-FORK-REVIEW.md`. Read the "Side notes" only. The upgrades are `/converge-modules`.
- Memory `project_history_rewrite_20261029.md`.

**Already done. Don't redo these:**
- F-01 to F-08, F-10 to F-15, F-17, F-18, F-25.
- The history rewrite and force-push to origin.
- The O-Contrabass goldens re-baseline (`edabddc9`).

## Rules (all mandatory)

1. **Leave other sessions' work alone.** At start, run `git status --short`. Every modified or untracked path that this command didn't create belongs to another session, and you must never stage, edit or restore it. Re-run the check before each commit. At last check that meant `plugins/O-simpleGrain/`, `.claude/agent-memory/`, `scripts/i18n-*-glossary.js`, the O-Prism and O-simpleGrain rows in PLUGINS.md, and `.claude/compaction-snapshot.md`.
2. **Commit through a temp index, scoped to your paths**, on `main` only. Never use `git add -A` on the real index, and never `commit -a`:
   ```bash
   # bash, not zsh. Usage: pc "msg" path...
   pc() { msg="$1"; shift
     [ "$(git branch --show-current)" = main ] || { echo NOT ON MAIN; return 1; }
     export GIT_INDEX_FILE=$(mktemp -u "${TMPDIR:-/tmp}/idx.XXXXXX"); cp .git/index "$GIT_INDEX_FILE"
     git reset -q HEAD -- . && git add -A -- "$@" && git diff --cached --name-only
     git commit -q -m "$msg" -m "Co-Authored-By: Claude <noreply@anthropic.com>"
     rm -f "$GIT_INDEX_FILE"; unset GIT_INDEX_FILE; git reset -q -- "$@"; git log --oneline -1; }
   ```
3. **zsh traps.** Run git refspec and `${SHA}:path` work in `bash -c`, because `$x:r`/`:P` modifiers eat the text after the colon. Don't put `===` in echo strings.
4. **Never create tags.** Only `/publish` tags.

## Step 1 — Ask once, up front

Ask all three decisions in one AskUserQuestion, then run the whole command without stopping:

1. **Backup mirror** `~/VST-development-prerewrite-20261029.git` (about 961 MB; the disk is about 96% full).
   - Delete (Recommended)
   - Keep
2. **The `private` remote** (`taylorbrook/VST-development`). It still has the pre-rewrite history, and its `Build and Release` workflow is **active**.
   - Leave it as is, and remove only the local remote config (Recommended)
   - Force-push the rewritten history to it
   - Leave everything as is
3. **The 5 foreign `.planning/quick/archive/` items** from F-20 (purchase flow, email confirmation, OAuth, manuscript).
   - Delete (Recommended)
   - Move them to `~/Dev/_foreign-quick-archive/`
   - Keep

## Step 2 — Do the confirmed destructive actions

- **Mirror delete.** First verify `git fsck --no-dangling` is clean, `git rev-list --count main` is ≥ 3136, `git tag | wc -l` is ≥ 432, and `git rev-parse main` equals `git rev-parse origin/main`. Then run `rm -rf ~/VST-development-prerewrite-20261029.git`.
- **Private remote:**
  - **Remove config:** run `git remote remove private`. Don't delete the GitHub repo. Tell the user that's their call, and give them the `gh repo delete` line.
  - **Force-push:** run these four steps in order, never otherwise:
    1. `gh workflow disable "Build and Release" -R taylorbrook/VST-development`
    2. Push `main` and the tags that already exist on that remote with `--force` (build the refspecs in bash), and delete `pre-cleanup-backup-20260201` there (F-24).
    3. Verify `gh run list -R taylorbrook/VST-development -L 5` shows no new runs.
    4. Re-enable the workflow.
- **Foreign archive items:** do whatever was chosen.

## Step 3 — Fix `scripts/regen-registry-used-by.sh` (FORK-REVIEW side note)

The script matches module basenames anywhere in `Source/`, comments included, so it wrongly lists O-Marimba and O-IntonationPad as preset-manager consumers.

- Match only real references:
  - `#include` lines in C++.
  - `<script`/`<link`/`import`/`src=`/`href=` in HTML and JS.
  - Non-comment CMake lines.
- Strip `//`, `/* */` and `#` comments before matching.
- Also scan `plugins/*/Resources/ui/`. O-Bells and O-MicrotonalSampler keep `tuning-panel.{js,css}` there.
- Keep the script deterministic and idempotent, and keep its header comment accurate.

**Gate:**
- After a run, `preset-manager` `used_by` does **not** contain O-Marimba or O-IntonationPad.
- A second run produces an empty `git diff`.
- Every *added* consumer has a real reference that you checked by grep.

Commit the script and registry together.

## Step 4 — Add `version:` to the STATUS.md files that lack one

The canon is a frontmatter `version: X.Y.Z` line. List them with:
```bash
for f in plugins/*/.planning/STATUS.md; do grep -q '^version:' "$f" || echo "$f"; done
```
At last check there were about 25. For each file:
- Take the version from `project(... VERSION x.y.z)` in that plugin's `CMakeLists.txt`.
- If the file has frontmatter, insert the line into it. Otherwise add a minimal `---\nversion: x.y.z\n---` block at the top.
- Don't change any other line.

Skip any plugin whose path has uncommitted changes (Rule 1), and list the skips.

## Step 5 — Remaining audit batches

Apply each recommendation from the AUDIT row. Make one commit per batch.

- **B3 / F-09.** Add a "Historical — superseded 2026-08-03. Decision record only; §4.1 size figures predate the 2026-10-29 rewrite." banner to `PUBLIC-RELEASE-READINESS.md`. Don't move it; 4 files reference it.
- **B10 / F-16.**
  - Delete `docs/codebase/`.
  - Repoint its 6 references to `.planning/codebase/`, checking each one by grep.
  - Don't regenerate the map; mention `/gsd-map-codebase` in the summary instead.
- **B9 / F-20, F-21, F-23.**
  - Archive the `.planning/quick/` dirs older than 2026-10-01 into `.planning/quick/archive/<YYYY-Qn>/` using `git mv`.
  - Leave `261029-md9` and anything newer in place.
  - Remove the empty dirs.
  - Move the 10 `.planning/verification/` reports into `plugins/<Name>/.planning/verification/` and remove the empty dirs.
  - Move the 7 root-level `.phase-*` / `.dsp-agent-report-*` dotfiles into each plugin's `.planning/`.
  - Add a link from `plugins/O-Octagon/CODE_REVIEW.md` to `.planning/CODE_REVIEW-v1.3.0.md`. Don't merge the two.
  - Grep for references to every moved path and fix them.
- **B5 / F-22.** For the 5 tracked-but-ignored evidence files, keep them as evidence: add `!` negations to `.gitignore` next to the rules they collide with. Gate: `git ls-files -ci --exclude-standard` drops by 5.
- **B13 / F-19, F-26, F-27, F-28.** Keep all of these; record "kept, by decision" in the AUDIT.
- **F-24 lowercase tags.** Keep them.

## Step 6 — Close out

1. In AUDIT.md, add a "Closed (2026-11)" section with a status for every F-row (`done <sha>`, `kept`, or `deferred → /converge-modules`).
2. Set SUMMARY.md frontmatter to `status: complete` and add a short outcome section.
3. Update `.planning/STATE.md`: the 261029-md9 row in the Quick Tasks table, plus the "Last activity" line.
4. Update memory `project_history_rewrite_20261029.md` if the mirror or private remote changed.
5. Run `git push origin main`. A push to main triggers no release. Then verify `git rev-list --left-right --count origin/main...main` is `0 0`.

## Handoff, then STOP

Report what changed, with SHAs, and anything you skipped and why. Then:

```
Next Up
  Step 1: /clear
  Step 2: /converge-modules 1
Alternatives:
  /improve O-Contrabass   — oversampler overruns on an oversized host block (also O-Bowed, O-Reed)
  /improve O-Strata       — default DryWetMixer ignores setWetLatency (also O-Marimba)
```
