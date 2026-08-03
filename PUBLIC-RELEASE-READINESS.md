# Public Release Readiness — VST-development

*Assessment date: 2026-07-30. Every figure below is measured, not estimated; the raw measurements live in `.planning/quick/260730-vwx-audit-repo-for-security-and-efficiency-i/260730-vwx-SCOUT.md` and each finding cites its scout ID inline.*

---

## 1. Verdict

**No secret has ever been committed to this repository.** A credential-extension scan over every tracked file, a content scan over the working tree, the same content scan over the *full* commit history, and a scan of deleted files all returned zero hits [S1]. There is therefore **no security reason to rewrite history** before going public. There is now a *legal* one to weigh — see the open decision below.

**Both hard blockers, legal rather than technical, are now closed.** ✅ The missing root `LICENSE` [L1] is resolved — the repository is licensed **AGPL-3.0**, following the JUCE election in section 5.2. ✅ The audio-sample provenance blocker [L4] is resolved as of 2026-08-01: investigation found the real problem was **three** files, not twelve — three commercial-library assets in O-simpleSampler, since removed with no replacement — while the other nine were provably self-authored and now carry provenance documents. Four of the twelve had never been embedded in a binary at all, and neither affected plugin has ever been released. Section 2.2 has the corrected record.

**✅ That open decision is now resolved — history was rewritten 2026-08-02.** The three withdrawn commercial samples were expunged from all history with `git filter-repo` (targeted at exactly the three paths; every ref, tag, and stash rewritten; the O-simpleSampler v1.0.0 commit formerly `4ca27977` is now `6734552a` and contains no samples). Verified post-rewrite: zero matching blobs across `git rev-list --all --objects`, HEAD tree hash byte-identical, the `plugins/O-Orbit/libs/SAF` gitlink intact, all 291 tags preserved. A full pre-rewrite backup bundle exists at `~/VST-development-pre-rewrite-20260802.bundle` — **that bundle still contains the samples; do not publish it.** One residual: GitHub retains unreachable commits fetchable by SHA until its own GC runs — see checklist **step 3b** for the pre-flip mitigation.

**The licensing decision is worth reading in full (section 5.2), because it was not the obvious one.** JUCE 8 is dual-licensed, and "the free JUCE license" resolves two different ways with opposite consequences. The free *Starter* tier caps you at $20,000 gross annual revenue — and JUCE counts donations and pay-what-you-want income toward that — while its EULA §1.17/§2.3 conflict with publishing the **80 JUCE-owned source files this repository redistributes** (4,451 lines of vendored JUCE overrides plus 78 vendored JUCE JS files). Taking JUCE under **AGPLv3** instead removes the revenue cap and makes those 80 files legally publishable as-is. Nothing needs deleting on license grounds.

Everything else in this document is hardening, repository size, or a disclosure decision that is yours to make — not a blocker.

---

## 2. Blockers — must fix before going public

### 2.1 No root LICENSE file [L1] — ✅ RESOLVED 2026-08-01

**What it was.** There was no `LICENSE`, `COPYING`, or `NOTICE` at the repository root. The only license file present was `plugins/O-Bassoon/research/reference-recordings/LICENSE.md`, a provenance note covering one asset folder that does not govern the repository [L1].

**Why it mattered.** A public repository with no license is not open source. Under default copyright, "public" means all-rights-reserved: readers may view the code but may not legally use it, fork it, redistribute it, or contribute to it — full disclosure with zero collaboration rights.

**Resolution.** Root `LICENSE` now contains the **GNU Affero General Public License v3.0**, installed verbatim from `https://www.gnu.org/licenses/agpl-3.0.txt` (661 lines, byte-identical to source, all required sections verified present). This follows the JUCE election recorded in section 5.2 — AGPLv3 rather than the free Starter tier — which also settles the redistribution question for the 80 JUCE-owned files this repo carries.

**Still open (housekeeping, not blocking):** per-file AGPL notice headers across the plugin sources, and the JUCE election statement in `THIRD-PARTY-NOTICES.md`. Both detailed at the end of section 5.2.

---

### 2.2 Undocumented audio samples shipped inside plugin binaries [L4] — ✅ RESOLVED 2026-08-01

**What it was.** Twelve files across three sample sets were listed as having no documented provenance, all three characterised as "compiled into distributed plugin binaries via `BinaryData` rather than merely sitting in the repository" [L4]:

| Location | Files |
|---|---|
| `plugins/O-simpleGrain/Source/samples/` | `fire.wav`, `piano.wav`, `voice.wav`, `water.wav` |
| `plugins/O-simpleSampler/Source/samples/` | `cello.aif`, `hit.wav`, `piano.wav`, `pizz.aif` |
| `plugins/O-MicrotonalSampler/tests/fixtures/4-layer/` | `C4_v1.wav` … `C4_v4.wav` |

**Why it mattered.** Undocumented material compiled into a signed, notarised, distributed product is a different class of problem from undocumented material sitting in a repository. Scout rated this the highest-risk legal item after the missing LICENSE [L4].

#### What investigation actually found — the finding was three files, not twelve

The audit's framing was broader than the evidence supported. Corrections, in the order they matter:

- **The real blocker was three files, not twelve.** Only `cello.aif`, `pizz.aif`, and `hit.wav` in `plugins/O-simpleSampler/Source/samples/` came from a commercial sample library. The other **nine were provably self-authored** and needed documentation, not remediation.
- **The nine cleared files are procedurally generated and bit-reproducible.** O-simpleGrain's four are synthesized by `plugins/O-simpleGrain/tools/generate_samples.py` at `SEED = 20260624`; re-running the script reproduces all four **bit-identically** (MD5 match on every file, verified 2026-08-01). O-simpleSampler's `piano.wav` is **byte-identical** to O-simpleGrain's (both MD5 `68efbe27e85979e723e1d777c2209e46`), so it carries the same origin. The four O-MicrotonalSampler fixtures are 440 Hz sines from `generate.py` in their own folder.
- **Four of the twelve were never embedded in any binary.** The O-MicrotonalSampler `4-layer` fixtures are **not referenced by any `juce_add_binary_data` target** — a grep of every `CMakeLists.txt` under `plugins/` for `4-layer` or `C4_v` returns no matches. They are loaded from disk by a test. The "compiled into distributed plugin binaries" claim did not hold for them.
- **Neither affected plugin has ever been released.** `gh release list --limit 400` returns 38 releases and **zero** for O-simpleSampler or O-simpleGrain; both sat at `VERSION "1.0.0"`. **No shipped, signed, or notarised binary has ever contained the commercial samples.** The exposure was real, but narrower than "attaches to every release already shipped".

#### Resolution

Path 3 for the three commercial files, path 1 for the nine cleared ones:

- **`cello.aif`, `pizz.aif`, and `hit.wav` were removed with no replacement** (developer decision — substituting generated or CC0 audio was offered and explicitly declined). The `BinaryData` `SOURCES` list, every C++ reference, and the UI control were removed with them.
- **O-simpleSampler's source-selector parameter was dropped**, taking the APVTS contract from 21 to 20 parameters. With one built-in left, a single-entry `AudioParameterChoice` is invalid in JUCE (`jassert (choices.size() > 1)`) and builds a degenerate `NormalisableRange {0, 0}` whose `convertTo0to1` is `0/0` — a NaN that `jlimit` does not clamp. Removing the parameter eliminates that by construction.
- **O-simpleSampler was rebuilt and re-verified at v1.1.0.** auval succeeds reporting **20 Global Scope Parameters at Component Version 1.1.0**; six pluginval strictness-10 runs (3 VST3, 3 AU) all exit 0 with logs free of NaN, Inf, and `FAILED`; the offline render harness reports 9/9. Neither installed bundle contains a withdrawn filename or a matching string in its Mach-O. Full figures: `.planning/quick/260801-u3o-remove-commercial-library-samples-from-o/260801-u3o-VERIFICATION.md`.
- **Three provenance `LICENSE.md` files were written**, modelled on `plugins/O-Bassoon/research/reference-recordings/LICENSE.md` — each naming its generator script, its seed where it has one, its dependencies, its exact format, and a **recorded MD5 per file** so the claim stays checkable rather than merely asserted:
  - `plugins/O-simpleGrain/Source/samples/LICENSE.md`
  - `plugins/O-simpleSampler/Source/samples/LICENSE.md`
  - `plugins/O-MicrotonalSampler/tests/fixtures/4-layer/LICENSE.md`

#### ✅ Resolved 2026-08-02 — history rewritten, samples expunged

The decision was made to **rewrite**. `git filter-repo --invert-paths` was run against exactly the three paths (`plugins/O-simpleSampler/Source/samples/{cello.aif,hit.wav,pizz.aif}`), preceded by a full `git bundle --all` backup at `~/VST-development-pre-rewrite-20260802.bundle`.

**Post-rewrite verification (all passed):**
- `git rev-list --all --objects | grep -Ei '(cello\.aif|hit\.wav|pizz\.aif)'` → zero hits across every ref, tag, and stash.
- HEAD tree hash **byte-identical** before and after (`3e998f41…`) — the rewrite touched history only, not the current tree.
- `plugins/O-Orbit/libs/SAF` gitlink survived (§4.6's required check).
- All 291 tags rewritten and preserved; both stashes preserved; commit count 1753.
- The O-simpleSampler v1.0.0 commit formerly `4ca27977` is now **`6734552a`** and contains no sample files. Historical SHA references in archived `.planning/quick/260801-u3o-*` artifacts refer to the pre-rewrite history and are intentionally left as-is.

**Residuals:**
1. **The backup bundle contains the old history including the samples.** It lives outside the repo (`~/VST-development-pre-rewrite-20260802.bundle`) — never publish or share it.
2. **GitHub keeps force-push-orphaned commits fetchable by SHA** until its server-side GC runs (which is not user-triggerable). Anyone with an old SHA could fetch the pre-rewrite commits from github.com even after the force-push. Since the repo is still private, this is mitigated before the visibility flip by either (a) contacting GitHub Support to run GC / purge cached commits, or (b) **deleting the GitHub repo and recreating it from the clean local clone** — simplest and fully deterministic. Tracked as the remaining condition on checklist step 3b.

---

### 2.3 `.claude/system-config.json` is tracked despite its `.gitignore` entry [S3] — ✅ RESOLVED 2026-08-03

**What.** `.gitignore` line 9 lists `.claude/system-config.json`, yet the file was still tracked — confirmed by `git ls-files -i -c --exclude-standard`. The file contains machine-local absolute toolchain paths [S3].

**Why it matters.** `.gitignore` only prevents *untracked* files from being added; it has no effect on a file that git is already tracking. This is a common and silent failure mode: the ignore rule is present, the intent is clear, and the file keeps getting committed anyway. On a public repo it publishes your local toolchain layout.

**What is needed.**

> Note: `git rm --cached` untracks the file and leaves it on disk. It does **not** remove it from history — earlier commits still contain it. If you also want it gone from history, that is the rewrite discussed in section 4.

```bash
git rm --cached .claude/system-config.json
git commit -m "chore: untrack machine-local system-config.json"
```

**Resolved 2026-08-03.** `ecf3fa39` untracked the file — 1 of the 42 files in that commit (5897 deletions, 0 insertions, all `git rm --cached` untrackings). `git ls-files .claude/system-config.json` now returns nothing, and the file remains on disk where the local toolchain still reads it. `.gitignore` line 9 keeps it from being re-added.

**Effort.** Under a minute.

---

### 2.4 `build-release/` is tracked, including a compiled executable [E3] — ✅ RESOLVED 2026-08-03

**What (as found 2026-07-30).** Ten tracked files lived under `build-release/`: `CPackConfig.cmake`, `CPackSourceConfig.cmake`, seven files matching `JUCE/*.cmake`, and `build-release/plugins/O-Bowed/O-Bowed_vst3_helper` — a compiled binary. `.gitignore` covered `build/` but not `build-release/` [E3].

**Why it matters.** Committed build output is noise at best. A committed *executable* is worse: a public repo shipping an unexplained binary invites reasonable suspicion, and nobody reviewing the repo can verify what it was built from.

**What was needed.** Untrack the tree — and, because `.gitignore` had no `build-release/` rule, add one, or the untracked tree simply gets re-added. Both were done; see the resolution note below.

> Note: as above, `git rm -r --cached` untracks and leaves the files on disk; it does not remove them from history.

```bash
git rm -r --cached build-release/
echo 'build-release/' >> .gitignore   # without this, the untracked tree gets re-added
git commit -m "chore: untrack build-release output"
```

**Resolved 2026-08-03 — fully.** `ecf3fa39` untracked all ten files, the compiled `O-Bowed_vst3_helper` among them, and `git ls-files build-release` now returns nothing.

> **The residual — also closed, 2026-08-03.** Untracking alone left the tree exposed: `.gitignore` had **no `build-release/` rule**, so `git check-ignore` returned nothing and the directory sat on disk as an untracked `??` entry. A single `git add -A` would have re-committed the whole tree — compiled helper binary included — into a repository that is now public. Closed by adding `build-release/` to `.gitignore` at line 67, beside the existing `build/` and `Build/` rules. Verified: `git check-ignore -v` on `CPackConfig.cmake`, on `plugins/O-Bowed/O-Bowed_vst3_helper`, and on the directory itself all now resolve to `.gitignore:67`; `git status --porcelain` no longer lists `build-release/`; and `git add -An` stages **zero** paths under it. This is the sole `.gitignore` edit made from this document, taken on explicit instruction.

**Effort.** Under a minute.

---

## 3. Security hardening — should fix

### 3.1 CI workflow — the posture is already correct; keep it that way [S4]

**Start from the good news.** `.github/workflows/build-and-release.yml` triggers on `push` and `workflow_dispatch` **only**. There is no `pull_request` trigger and no `pull_request_target` trigger, which means a fork pull request cannot reach the eight Apple signing secrets the workflow consumes: `MACOS_CERTIFICATE`, `MACOS_CERTIFICATE_PWD`, `MACOS_INSTALLER_CERTIFICATE`, `MACOS_INSTALLER_CERTIFICATE_PWD`, `APPLE_ID`, `APPLE_ID_PASSWORD`, `APPLE_IDENTITY_NAME`, `APPLE_TEAM_ID` [S4]. This is the correct posture for a public repo. The work here is preserving it and hardening around it — not fixing a defect.

**The stakes, in one line.** Compromise of this workflow means an attacker can ship signed and notarised malware under the Ouaricon identity [S4].

**Three concrete actions:**

1. **Add a top-level `permissions:` block.** Only the release job sets `permissions:` today, at line 599 of the workflow. Public repositories inherit the org or repo default token scope for every other job [S4]. Add at the top of the file:

   ```yaml
   permissions:
     contents: read
   ```

   The release job keeps its own broader block; every other job then runs read-only.

   ✅ **Done 2026-08-03** — added between the `on:` and `env:` blocks. `create-release` retains its `contents: write` override, so Release publishing is unaffected.

2. **SHA-pin every tag-pinned action reference.** A tag is mutable — the upstream owner can move it — so a tag pin is a standing supply-chain exposure on a workflow that holds signing certificates. The tag-pinned references are [S4]:

   | Action reference | Uses recorded |
   |---|---|
   | `actions/checkout@v4` | ×3 |
   | `actions/upload-artifact@v4` | ×3 |
   | `actions/download-artifact@v4` | ×1 |
   | `softprops/action-gh-release@v2` | ×1 |

   Replace each with the full commit SHA of the release you intend, keeping the tag in a trailing comment for readability.

   ✅ **Done 2026-08-03** — all eight references are now pinned to 40-hex commit SHAs, and every pin carries a trailing `# vN (vN.N.N)` comment recording both the major tag that was pinned and the precise release it corresponded to.

3. **Adopt a standing rule.** Never add `pull_request_target`, and never add a secrets-bearing `pull_request` trigger, to this workflow while it carries signing certificates [S4]. Record the rule where the next person editing the workflow will see it.

   ✅ **Done 2026-08-03** — recorded as a set-off block in the workflow's own header comment, above `name:`. It names both forbidden triggers, lists the eight Apple signing secrets, and states the stake. Note this is a **documentary** control only: nothing technically prevents a future editor from adding the trigger. A branch ruleset on `.github/workflows/**` would be the enforcing control and remains unimplemented.

---

### 3.2 Local-path and username disclosure [S2]

The string `/Users/taylorbrook` appears in **354 tracked files** [S2]. The occurrences are concentrated in the AI-workflow directories: `.claude/agent-memory/*.md`, `.claude/skills/**`, `.claude/system-config.json`, `.planning/STATE.md`, `.planning/codebase/*.md`, and `.planning/milestones/**/*-PLAN.md` [S2].

> **Update 2026-08-03.** One entry in that list is no longer tracked: `.claude/system-config.json` was untracked in `ecf3fa39` (see §2.3), so the measured figure is now **353**. The 354 above stands as the [S2] measurement at the 2026-07-30 assessment date. The file still sits on disk, so the paths it contains remain local — they are simply no longer published.

This discloses the macOS account name and the full local directory layout. **Low severity, high volume** [S2] — no credential is exposed, but the footprint is large enough that piecemeal editing is not practical. In practice this resolves as a side effect of the keep-or-strip decision in section 3.3: strip those trees and the great majority of the 354 files go with them.

---

### 3.3 Internal AI-workflow artifacts — a disclosure decision, not a defect [S5]

`.claude/` holds **417 tracked files** and `.planning/` holds **435 tracked files** — **852 files** in total. The content includes agent memory, a session-derived developer profile, phase plans, verification reports, and quick-task history [S5].

This is **not a vulnerability**. It is a decision about what you are willing to publish, and it needs an explicit call before the repo goes public. Both sides are real:

| Keep them public | Strip them |
|---|---|
| The full planning and verification history is genuinely valuable documentation — it shows how each plugin was designed, researched, and validated. | Agent memory and the session-derived developer profile describe your working habits, not the product. |
| Reviewers and contributors can see the reasoning behind decisions, not just the outcome. | Internal verification reports may reference unshipped work, known defects, or judgements you would phrase differently in public. |
| Removing them costs the repo its most complete record of intent. | These trees are also where the 354 local-path disclosures live [S2]. |

This document does not recommend a side. It does flag one sequencing consequence: **resolve this before any history rewrite**, because it changes what a rewrite would need to strip — see section 6.

---

### 3.4 Tracked build logs [S6] — ✅ RESOLVED 2026-08-03

**31 files** matching `logs/**/build_*.log` were tracked, and were also matched by `.gitignore` — the same already-tracked-despite-ignored situation as section 2.3. Build logs embed absolute paths and local environment detail [S6].

Same remedy shape as [S3]:

> Note: untracks and leaves on disk; does not remove from history.

```bash
git rm -r --cached logs/
git commit -m "chore: untrack build logs"
```

**Resolved 2026-08-03.** `ecf3fa39` untracked all 31 matching files. `.gitignore` line 197 (`*.log`) already covers the pattern, so — unlike `build-release/` — these cannot silently return to tracking.

---

## 4. Efficiency and repository size

### 4.1 Measured baseline [E1]

| Measure | Value |
|---|---|
| `.git` | 912 MB |
| `.git/objects` | 596 MB |
| Tracked files | 3308 |
| Working tree | 7.6 GB |

The working tree figure is mostly untracked material — `build/`, `.cache/`, `backups/`, `.playwright-mcp/` — and does not travel with a clone [E1]. The number that matters for anyone cloning is the 912 MB `.git`.

### 4.2 Binary test goldens dominate history [E1]

Roughly **250 MB of blobs** are audio goldens. Several appear **twice** in history — a golden that was re-committed stores each revision in full [E1]:

| Size | Path | Dupes in history |
|---|---|---|
| 50.0 MB | `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav` | 2 |
| 16.4 MB | `.../golden/string-G.wav`, `string-D.wav`, `string-A.wav` | 1 each |
| 16.4 MB | `.../golden/stiffness-zero-pre.wav` | 2 |
| 16.4 MB | `.../golden/saturator-tail-comparison.wav` | 1 |
| 16.4 MB | `plugins/O-Contrabass/e1-max-sustain.wav` | 1 |
| 15.6 MB | `.../golden/slow-lfo.wav` | 2 |
| 8.1 MB | `.../golden/schelleng-stress.wav`, `detune-sweep-A.wav` | 2 / 1 |
| 5.6 MB | `.../golden/mpe-yz.wav`, `macro-sweep.wav` | 1 / 2 |
| 5.0 MB | `.../golden/note-sequence.wav` | 2 |
| 4.5 MB | `.../golden/note-expression.wav`, `output-chain.wav` (×2) | |

**32 `.wav` files are tracked in total** [E1]. One entry in the table is not a golden at all: `plugins/O-Contrabass/e1-max-sustain.wav` (16.4 MB) is a stray scratch render that was committed into a plugin folder [E1].

### 4.3 Shipped installers committed to git [E2] — ✅ RESOLVED 2026-08-03

**As assessed 2026-07-30 —** three blobs totalling **13.5 MB** [E2]:

- `plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg` — 4.5 MB
- `plugins/O-Polystutter/dist/O-Polystutter-by-TACHES.pkg` — 4.5 MB (history only)
- `plugins/O-Polystutter/dist/PolyStutter.zip` — 4.5 MB

Build outputs belong on GitHub Releases, not in git — and the release workflow already publishes there [E2].

**Correction to this section's own figures, measured 2026-08-03.** The headline number overstated the problem in three ways, each worth recording because the corrected figure is the one a later step will be checked against:

- **Only two of the three blobs were ever tracked.** `O-Polystutter-by-TACHES.pkg` was already history-only at the assessment date — the bullet above says so, yet the 13.5 MB total counts it anyway. There was never anything to untrack for it.
- **The tracked total was 9,498,890 bytes (~9.5 MB), not 13.5 MB** — 4,756,409 B for the `.pkg` plus 4,742,481 B for the `.zip`.
- **The two overlap rather than being independent.** `PolyStutter.zip` contains a copy of the `.pkg` at exactly 4,756,409 bytes plus an install-readme, so the pair is one v1.8.0 distribution package stored twice, not two distinct artifacts.

**Resolved 2026-08-03.** Both tracked blobs were untracked with `git rm --cached` and left on disk, after first being copied to `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/` and MD5-verified against the originals. That backup — not git history — is what satisfies the preservation gate here, because neither blob matches any published Release asset: the only published O-Polystutter release is v1.12.4, a different version at different sizes. Two further decisions are recorded so nobody has to re-derive them:

- **`plugins/O-Polystutter/dist/install-readme.txt` remains tracked.** It is hand-authored source rather than build output, so the directory was not emptied from git.
- **A directory-wide ignore rule over plugin `dist` folders was considered and deliberately rejected.** O-Tremolo's Inno Setup script (`installer.iss`) and the install-readme text files live in exactly such a folder and must stay tracked; a broad rule would silently stop future installer scripts from being tracked at all. The rules added instead match the built-artifact extensions `*.pkg`, `*.dmg`, and `*.msi`, and they went in **before** the untracking so the files were never simultaneously untracked and unignored.

**Per §4.6's pivotal mechanic, the 912 MB did not move.** Removing files in a new commit shrinks the checkout, not `.git`. The ~9.5 MB stays in history by design and both blobs remain recoverable by addressing the blob at its pre-removal commit; reclaiming that space is step 14's decision, and its proposal already lists this path.

### 4.4 Root-level scratch files and first impressions [E4]

These are untracked, so they will not be published, but they are present in the working directory and they make the repo root read as a scratch space rather than a project: `e1-max-sustain.wav` (17 MB), `o-bowed-pre-extraction-canonical.wav` (1.3 MB), `mbc-v150.png`, `o-reversedelay-484.png`, `tooltip-knob.png`, `.DS_Store`, and `scratch-pv/` [E4].

Separately, **38 `.png` and 23 `.jpg` files are tracked** repo-wide — including `plugins/O-AnalogEQ/Source/ui/public/images/flower_ferdinandibauer00baue_0021.png` at 3 MB [E4]. These are real UI assets; the point is that image weight is a second contributor to clone size after the audio goldens.

### 4.5 Clone cost and GitHub limits [E5]

At 912 MB, a `git clone` is slow and consumes GitHub bandwidth on every fork and every CI run [E5]. Two thresholds matter: GitHub **soft-warns above 1 GB** for repository size, and **hard-caps individual files at 100 MB**. The largest single file here is 50 MB — **under the cap, so no push will be rejected** [E5]. The repo is not at a wall; it is well past comfortable.

---

### 4.6 The pivotal mechanic — read this before deciding

> **Removing files in a new commit shrinks the checkout but NOT `.git`.** The old blobs stay in history, so the 912 MB does not move. Only a **history rewrite** reclaims that space [E5]. A rewrite changes **every commit SHA** in the repository. That makes it a one-time, before-the-fact operation: it must happen **before the repo is public and before anyone forks**, or not at all [E5].

**This is a size decision, not a security one.** Scout confirmed zero credential material across the entire history [S1], so nothing in this section is required to make the repo safe. Do not let the size cleanup [E1] [E2] [E5] be mistaken for a security necessity — if you decide the 912 MB is acceptable, skipping the rewrite entirely is a legitimate and safe choice.

> **✅ The legal dimension of this was resolved 2026-08-02.** The three commercial-library sample paths were expunged in a **targeted** `git filter-repo` rewrite (see §2.2 "Resolved"), so the samples exposure no longer rides on this section. What remains here is the original **size** decision only — the 912 MB reclaim — which stays genuinely optional. Note that a size rewrite would now be the repository's *second* rewrite; that is mechanically fine (rewrites compose), it just changes every SHA again.

If you do decide to proceed, this is the shape of the operation. It is a **proposal for your approval**, not a step this document has taken:

> **IRREVERSIBLE — decide before running.**

```bash
# PROPOSAL ONLY — rewrites every commit SHA. Do not run until you have decided.
# Take a full backup clone first:
#   git clone --mirror . ../VST-development-backup.git

git filter-repo \
  --path plugins/O-Contrabass/tests/render-harness/golden/ \
  --path plugins/O-Contrabass/e1-max-sustain.wav \
  --path plugins/O-Polystutter/dist/ \
  --path build-release/ \
  --invert-paths

# Then verify the submodule gitlink survived:
git submodule status plugins/O-Orbit/libs/SAF
```

**Submodule caveat.** `plugins/O-Orbit/libs/SAF` is a git submodule pointing at `github.com/leomccormack/Spatial_Audio_Framework` [L3]. Any rewrite must preserve that gitlink entry — verify it explicitly afterwards, as in the last line above. Per project convention, worktree isolation is unsafe for submodule paths, so run the rewrite on a dedicated backup clone rather than inside a worktree.

---

## 5. Legal and licensing

### 5.1 The root LICENSE — settled [L1]

Restating section 2.1 in its legal context: with no root license, publishing grants no rights. That gate is now closed — the repository is **AGPL-3.0**, following the JUCE election in 5.2. Read 5.2 for why that election was the load-bearing decision and not a formality.

### 5.2 The JUCE licensing question — RESOLVED: AGPLv3 [L2]

> **Decided 2026-08-01.** JUCE is taken under **AGPLv3**, not under the free Starter tier. Root LICENSE is `AGPL-3.0`, installed verbatim from `gnu.org/licenses/agpl-3.0.txt`.

**Why this was a real fork and not a formality.** "The free JUCE license" is ambiguous, and the two readings lead to opposite outcomes. JUCE 8 is dual-licensed — you take it under *either* the [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.en.html) *or* the [commercial JUCE 8 EULA](https://juce.com/legal/juce-8-licence/), whose tiers are Starter (free, ≤ $20,000 annual revenue), Indie (≤ $300,000), Pro (uncapped), and Educational.

**The conflict that decided it.** This repository does not merely *use* JUCE — it *redistributes JUCE source*:

| What | Count | Detail |
|---|---|---|
| `vendored/JUCE-overrides/` | 2 files, **4,451 lines** | `juce_audio_plugin_client_VST3.cpp` (4,112) + `juce_VST3ClientExtensions.h` (339), both carrying Raw Material Software's copyright header verbatim |
| `plugins/*/…/js/juce/*.js` | **78 files** | JUCE-shipped WebView JS (`index.js`, `check_native_interop.js`), vendored per plugin |

That is 80 JUCE-owned files in a repo about to become public. The commercial EULA is hostile to exactly this: **§1.17** — "You may not sell, sub-license, or otherwise Distribute the Framework, or any subset of the Framework, on its own"; **§2.3** — the Framework may not be used in a way that subjects it to open-source licensing requiring it to be "disclosed or distributed in source code form" or made "redistributable at no charge." A root LICENSE covering the whole repository would purport to license those 80 files under it.

AGPLv3 dissolves the conflict rather than working around it — redistributing modified JUCE source is *expressly permitted* under that horn. The vendored files say so themselves; their own header reads *"You may also use this code under the terms of the AGPLv3."*

**The second reason, specific to pay-what-you-want.** JUCE measures the Starter cap as **gross revenue, and counts donations and pay-what-you-want income toward it**. Across 37+ PWYW plugins that is a $20,000 ceiling requiring active tracking, with an upgrade obligation on crossing it. AGPLv3 has no revenue cap, so this concern disappears entirely.

**Dependency compatibility — checked, all clear.** AGPLv3 is the most restrictive license in the tree, so every other component must be compatible *with* it, and each is: Spatial_Audio_Framework (ISC, permissive), moodycamel (BSD/Boost), nanoflann (BSD), umappp and ONNX Runtime (MIT). The VST3 SDK is dual GPLv3-or-Steinberg-proprietary, and AGPLv3 §13 expressly permits linking with GPLv3 works.

**What you are accepting.** AGPLv3 is strong copyleft: anyone who distributes a derivative of these plugins must release their source under AGPLv3 too. For pay-what-you-want releases this is usually neutral or actively desirable, but it does foreclose a closed-source commercial fork — including by you, later, without relicensing. Relicensing is possible since you hold copyright on your own code, but it would require replacing or re-permissioning the JUCE dependency.

**Consequences now settled.** The 80 JUCE-derived files above are fine to publish as-is with their headers intact. No file needs removing on license grounds. `scripts/juce-patches/*.patch` may stay. Section 2.1 is unblocked and closed.

**AGPL housekeeping:**
- ✅ **Per-file notice headers — done 2026-08-01.** 707 files (358 `.h`, 231 `.cpp`, 56 `.js`, 40 `.html`, 22 `.css`) across 39 plugins and 11 shared modules, each carrying its product subject, an `SPDX-License-Identifier: AGPL-3.0-or-later`, and the full warranty disclaimer. Applied by **`scripts/add-agpl-headers.py`**, which is idempotent — re-run it after adding a plugin and it stamps only the new files.

  The script deliberately excludes third-party code, and this is the part worth preserving: AGPLv3 lets us *redistribute* JUCE's files, but stamping **our** copyright onto them would be a false attribution. Excluded and verified untouched: the 80 `js/juce/*` files, `vendored/JUCE-overrides/`, the generated `app.bundle.js`, and 24 plugin-local `.planning/` mockups. `--audit` is a content-based backstop that fails the run if any file bearing a foreign copyright notice escapes the path rules.

  One trap found in the process, recorded because it would silently recur: Python's `Path.read_text()` performs universal-newline translation, so a CRLF file becomes a whole-file rewrite rather than a one-block insertion. Exactly one file here is CRLF (`plugins/O-Tremolo/Source/ui/public/index.html`) and it produced a 1,264-line phantom diff before the script was switched to newline-preserving I/O.

- State the JUCE election explicitly in `THIRD-PARTY-NOTICES.md` (§5.3): *"JUCE is used under the AGPLv3 option of its dual license, not under a commercial JUCE licence."* This matters because JUCE's dual license makes the election otherwise unknowable to a reader.
- Note in the README that binaries distributed as PWYW are AGPLv3 and that source is at this repository. AGPLv3 does not restrict charging money — pay-what-you-want is fully compatible.

### 5.3 Third-party attribution inventory [L3]

The repository carries several third-party components that need aggregated attribution [L3]:

| Component | Location | Note |
|---|---|---|
| Spatial_Audio_Framework | `plugins/O-Orbit/libs/SAF` (git submodule) | Points at `github.com/leomccormack/Spatial_Audio_Framework`. The submodule URL is public HTTPS, so it clones fine for outsiders. Ships its own `LICENSE.md`. |
| Bundled JS dependencies | `plugins/O-TextureForge/.../app.bundle.js.LICENSE.txt` | Already carries its own license text; needs surfacing. |
| moodycamel concurrent queue | header attribution (`cameron@moodycamel.com`) | Vendored header. |
| FetchContent dependencies | build configuration | umappp, nanoflann, ONNX/ANIRA per project notes. |

**Recommended deliverable:** a `THIRD-PARTY-NOTICES.md` at the repository root aggregating all of the above, with each component's license text or a link to it. This document does not create that file.

### 5.4 Audio asset provenance — ✅ RESOLVED 2026-08-01 [L4]

Every audio asset folder in the repository now carries a provenance document. The template was `plugins/O-Bassoon/research/reference-recordings/` — VSCO-2-CE under CC0 1.0 — and the pattern has been applied to the rest:

| Folder | Origin | Provenance doc |
|---|---|---|
| `plugins/O-Bassoon/research/reference-recordings/` | VSCO-2-CE, CC0 1.0 (third-party, redistributable) | `LICENSE.md` (pre-existing) |
| `plugins/O-simpleGrain/Source/samples/` | self-authored — `tools/generate_samples.py`, `SEED = 20260624`, bit-reproducible | `LICENSE.md` (new) |
| `plugins/O-simpleSampler/Source/samples/` | self-authored — byte-identical to O-simpleGrain's `piano.wav`, same generator | `LICENSE.md` (new) |
| `plugins/O-MicrotonalSampler/tests/fixtures/4-layer/` | self-authored — `generate.py`, 440 Hz sines; **test fixtures, never embedded in a binary** | `LICENSE.md` (new) |

Each of the three new documents records a **per-file MD5 measured from disk**, so a future drift between the document and what it describes is detectable rather than silent — a provenance document that has quietly gone false is worse than none.

The three commercial-library files that were the actual blocker were removed rather than documented. **Section 2.2 carries the full record. The history exposure was closed 2026-08-02 by a targeted rewrite — the files no longer exist anywhere in git history.**

---

## 6. Ordered execution checklist

Work top to bottom. The order is not arbitrary — each step either gates the next or changes what the next step operates on.

- [x] ~~**1. Answer the JUCE licensing question.**~~ ✅ **Done 2026-08-01 — AGPLv3**, not the free Starter tier. Removes the $20k PWYW revenue cap and makes the 80 redistributed JUCE files publishable as-is. *(Section 5.2 — [L2].)*
- [x] ~~**2. Add a root `LICENSE` file.**~~ ✅ **Done 2026-08-01** — AGPL-3.0, verbatim from gnu.org, 661 lines. *(Section 2.1 / 5.1 — [L1].)*
- [x] ~~**3. Resolve the undocumented sample provenance** in O-simpleGrain, O-simpleSampler, and the O-MicrotonalSampler 4-layer fixtures.~~ ✅ **Done 2026-08-01.** Three commercial-library files removed with no replacement; O-simpleSampler rebuilt and re-verified at v1.1.0 (auval 20 params, pluginval@10 ×6, harness 9/9); three provenance `LICENSE.md` files written with per-file MD5s. *(Section 2.2 / 5.4 — [L4].)*
- [x] ~~**3b. DECIDE: the withdrawn commercial samples remain in git history.**~~ ✅ **Done 2026-08-02 — resolved by targeted rewrite.** `git filter-repo` expunged the three sample paths from all history (backup bundle at `~/VST-development-pre-rewrite-20260802.bundle`; blobs verified gone across all refs; HEAD tree byte-identical; SAF gitlink intact; formerly-`4ca27977` now `6734552a`). *(Section 2.2 "Resolved" — [L4].)* **One condition rides forward to step 15: GitHub retains orphaned pre-rewrite commits fetchable by SHA until server-side GC. Before flipping visibility, either have GitHub Support purge them or delete and recreate the repo from the clean clone.**
- [ ] **4. Write `THIRD-PARTY-NOTICES.md`** aggregating SAF, the bundled JS licenses, moodycamel, and the FetchContent dependencies — and stating explicitly that **JUCE is used under the AGPLv3 option of its dual license**, which a reader cannot otherwise determine. *(Section 5.3 / 5.2 — [L3] [L2].)*
- [x] ~~**4b. Add AGPL notice headers** to your own source files.~~ ✅ **Done 2026-08-01** — 707 files across 39 plugins and 11 modules, via the idempotent `scripts/add-agpl-headers.py`. Third-party excluded and verified untouched. *(Section 5.2 — [L2].)*
- [x] ~~**5. Untrack `.claude/system-config.json`** with `git rm --cached`.~~ ✅ **Done 2026-08-03** — untracked in `ecf3fa39` (1 file, part of a 42-file, 5897-deletion untracking commit); `git ls-files` now returns nothing for the path and the file remains on disk. `.gitignore:9` keeps it from being re-added. *(Section 2.3 — [S3].)*
- [x] ~~**6. Untrack `build-release/`**, including the compiled `O-Bowed_vst3_helper`.~~ ✅ **Done 2026-08-03** — all 10 tracked files untracked in `ecf3fa39`, the compiled helper binary among them; `git ls-files build-release` now returns nothing. *(Section 2.4 — [E3].)* **Fully closed:** untracking alone was not enough — `.gitignore` had no `build-release/` rule, leaving the tree on disk as an untracked `??` entry that a future `git add -A` would have re-committed into a now-public repository. `build-release/` added to `.gitignore` (line 67, beside `build/`); `git check-ignore` now resolves the tree and the compiled helper to that rule, and `git add -An` stages nothing under it.
- [x] ~~**7. Untrack the build logs** under `logs/`.~~ ✅ **Done 2026-08-03** — 31 `logs/**/build_*.log` files untracked in `ecf3fa39`; `.gitignore:197` (`*.log`) already covers them, so unlike `build-release/` this one cannot silently return to tracking. *(Section 3.4 — [S6].)*
- [x] ~~**8. Add a top-level `permissions: contents: read` block** to the release workflow.~~ ✅ **Done 2026-08-03** — added between the `on:` and `env:` blocks as the least-privilege default for every job's `GITHUB_TOKEN`. The `create-release` job keeps its own `contents: write` override, so Release publishing is unaffected. *(Section 3.1 — [S4].)*
- [x] ~~**9. SHA-pin every tag-pinned action reference** in the release workflow.~~ ✅ **Done 2026-08-03** — all 8 references pinned to 40-hex commit SHAs, each with a trailing `# vN (vN.N.N)` comment carrying both the major tag pinned and the precise release. Zero mutable-tag references remain. *(Section 3.1 — [S4].)*
- [x] ~~**10. Record the standing CI rule** — never add `pull_request_target` or a secrets-bearing `pull_request` trigger while the workflow holds signing certificates.~~ ✅ **Done 2026-08-03** — recorded as a set-off standing-rule block in the workflow's own header comment, where the next editor sees it first. It names both forbidden triggers, lists the eight Apple signing secrets, and states the stake: signed, notarised malware shipped under the Ouaricon identity. *(Section 3.1 — [S4].)*
- [ ] **11. Decide: keep or strip `.claude/` and `.planning/`.** *(Section 3.3 — [S5]; also resolves the bulk of [S2].)* Must resolve **before** step 12, because it changes what a rewrite would need to strip.
- [x] ~~**12. Move the committed installers off git** and onto GitHub Releases.~~ ✅ **Done 2026-08-03** — the two tracked O-Polystutter v1.8.0 blobs, `plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg` (4,756,409 B) and `plugins/O-Polystutter/dist/PolyStutter.zip` (4,742,481 B), were untracked with the cached form of `git rm` and left on disk. *(Section 4.3 — [E2].)* **The preservation gate came first, and it was the backup that satisfied it — not git history.** Neither blob matches any published Release asset: the only published O-Polystutter release is `O-Polystutter-v1.12.4`, a different version at different sizes. Both were therefore copied to `~/VST-development-releases-backup-20260803/assets/O-Polystutter-v1.8.0-repo-dist/` and MD5-verified against the originals — `c778e3de586f536d0972f54336de2463` and `705eae9b8dcf00cc6cf3d0c77e5ca857` — **before** anything left the index. **Step 6's residual was not repeated:** `*.pkg`, `*.dmg`, and `*.msi` rules were added *before* the untracking, so the files were never simultaneously untracked and unignored; `git check-ignore -v` now resolves both removed paths to a rule, and a dry-run `git add -An` on `plugins/O-Polystutter/dist/` stages **zero** paths. The tracked-but-ignored census fell from 10 entries to 9, all of them vendored `preset-manager.js`. **History was intentionally left intact.** This was a checkout-only change — the ~9.5 MB of blobs remains in `.git`, both files stay recoverable by addressing the blob at its pre-removal commit, and reclaiming that space belongs to **step 14**, whose section 4.6 proposal already lists `plugins/O-Polystutter/dist/`.
- [ ] **13. Tidy the repo root** — the scratch renders and images that make the root read as a workspace. *(Section 4.4 — [E4].)*
- [ ] **14. Rewrite history — optional, size only.** The legal dimension was already closed by the targeted step-3b rewrite of 2026-08-02, so this step is back to being **genuinely optional**: it reclaims repository size and nothing else. If run, take a mirror backup, run the `git filter-repo` proposal in section 4.6 (the three sample paths are already gone — no need to include them), and verify the `plugins/O-Orbit/libs/SAF` gitlink survived. *(Sections 4.2 / 4.5 / 4.6 — [E1] [E5].)* **Last among all local changes** — it changes every commit SHA again, so nothing else should follow it.
- [x] ~~**15. Flip visibility to public.**~~ ✅ **Done 2026-08-03 — via a separate public repo, not a visibility flip.** The orphaned pre-rewrite commits were confirmed still fetchable on the private repo (API served `4ca27977` post-force-push), so instead of deleting the repo or waiting on GitHub Support, the clean rewritten history was pushed to a **new** repo: **`github.com/taylorbrook/O-Audio-VST-Development`** (main + 289 main-reachable tags; the 2 off-main tags `O-GrainScatter-v2.4.2` / `v1.4.1-intonationpad` deliberately excluded), the 18 releases published in the prior 48 h restored from backup with all assets, verified (orphan SHA 404s; v1.0.0 samples dir contains only `piano.wav`), then made public. **The private `VST-development` repo remains private and untouched** — it keeps all 56 releases, the 8 Apple signing secrets, and the orphaned commits (unexposed while private). Full release backup: `~/VST-development-releases-backup-20260803/` (999 MB, verified byte-perfect). Steps 4, 8–13 were consciously deferred past the flip.

  > **IRREVERSIBLE — decide before running.**

  ```bash
  # PROPOSAL ONLY — publishing cannot be fully undone; forks and caches persist.
  gh repo edit taylorbrook/VST-development --visibility public
  ```

---

### Closing note on step 14

Step 14 is **genuinely optional**. It reclaims repository size and nothing else. Scout verified across the full history that no credential material has ever been committed [S1], so there is no security requirement to rewrite anything. If you would rather keep every commit SHA stable — for existing clones, for links into history, for the release tags the workflow has already published — skipping step 14 is a legitimate choice, and the repo is just as safe to publish without it. The only cost is that clones stay at the measured 912 MB [E5].

Steps 1 through 3 are the ones that actually block you.
