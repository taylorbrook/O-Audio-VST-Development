# Quick Task 260730-vwx — Scout Findings

**Gathered:** 2026-07-30 by orchestrator (pre-planning)
**Repo:** `github.com/taylorbrook/VST-development` (currently private)
**Purpose:** Ground-truth facts for the public-release readiness plan. All figures measured, not estimated.

---

## Measurement commands used

```bash
git ls-files | wc -l                              # 3308 tracked files
du -sh .git                                       # 912M  (.git/objects = 596M)
du -sh .                                          # 7.6G working tree
git rev-list --objects --all | git cat-file --batch-check='%(objecttype) %(objectname) %(objectsize) %(rest)'
git grep -lI '/Users/taylorbrook' -- .            # 354 files
git ls-files -i -c --exclude-standard             # tracked-but-gitignored
```

---

## SECURITY

### S1 — No credential material found (CLEAN)
- Extension scan (`.env .pem .key .p12 .cer .crt .mobileprovision .keystore .jks .pfx`) over `git ls-files`: **0 hits**.
- Content scan over the working tree for `sk-ant-`, `sk-…`, `ghp_`, `gho_`, `github_pat_`, `AKIA[0-9A-Z]{16}`, `xox[baprs]-`, `AIza…`, `-----BEGIN … PRIVATE KEY-----`: **0 hits**.
- Same pattern scan over **full history** (`git log --all -p`): **0 hits**.
- Deleted-file scan (`--diff-filter=D`) for sensitive filenames: **0 hits**.
- Verdict: no secret has ever been committed. **No history rewrite is required for secrets.**

### S2 — Username / local-path disclosure (354 tracked files)
`/Users/taylorbrook` appears in 354 tracked files. Concentrated in `.planning/` and `.claude/`:
`.claude/agent-memory/*.md`, `.claude/skills/**`, `.claude/system-config.json`,
`.planning/STATE.md`, `.planning/codebase/*.md`, `.planning/milestones/**/*-PLAN.md`.
Discloses the macOS account name and full local directory layout. Low severity, high volume.

### S3 — `.claude/system-config.json` is tracked despite being in `.gitignore`
`.gitignore` line 1 lists `.claude/system-config.json`, but it is still tracked
(`git ls-files -i -c --exclude-standard` confirms). `.gitignore` does not untrack existing
entries — it needs `git rm --cached`. The file contains machine-local absolute toolchain paths.

### S4 — CI is currently safe, but is one line away from being unsafe
`.github/workflows/build-and-release.yml` (658 lines):
- Triggers: `push` + `workflow_dispatch` **only**. No `pull_request`, no `pull_request_target`.
  → Fork PRs cannot reach secrets today. This is the correct posture and must be preserved.
- Consumes 8 Apple signing secrets: `MACOS_CERTIFICATE`, `MACOS_CERTIFICATE_PWD`,
  `MACOS_INSTALLER_CERTIFICATE`, `MACOS_INSTALLER_CERTIFICATE_PWD`, `APPLE_ID`,
  `APPLE_ID_PASSWORD`, `APPLE_IDENTITY_NAME`, `APPLE_TEAM_ID`.
  Compromise = ability to ship signed+notarised malware under the Ouaricon identity.
- No **top-level** `permissions:` block. Only the release job (line 599) sets one.
  Public repos inherit the org/repo default token scope for the other jobs.
- Actions are **tag-pinned, not SHA-pinned**: `actions/checkout@v4` (×2),
  `actions/upload-artifact@v4` (×3), `actions/download-artifact@v4`,
  `softprops/action-gh-release@v2`. A tag is mutable — supply-chain exposure to a
  compromised upstream tag, on a workflow that holds signing certs.

### S5 — Internal AI-workflow artifacts become public (852 files)
`.claude/` = 417 tracked files, `.planning/` = 435 tracked files. Includes agent memory,
session-derived developer profile, phase plans, verification reports, quick-task history.
Not a vulnerability — a disclosure **decision**. Needs an explicit keep/strip call.

### S6 — Tracked build logs (31 files under `logs/`)
`logs/**/build_*.log` is tracked and also matched by `.gitignore`. Build logs embed
absolute paths and local environment detail.

---

## LEGAL / LICENSING (release blockers)

### L1 — No root LICENSE file
No `LICENSE`, `COPYING`, or `NOTICE` at repo root. Only `plugins/O-Bassoon/research/reference-recordings/LICENSE.md`
(a provenance note for one asset). **Without a root license, "public" means all-rights-reserved:**
no one may legally use, fork, or contribute. Hard blocker.

### L2 — JUCE license interaction (needs a decision, not just a file)
Plugin sources are JUCE-derived. JUCE 8 is dual-licensed AGPLv3 / commercial.
Additionally `vendored/JUCE-overrides/` ships **2 modified JUCE source files**
(`juce_audio_plugin_client_VST3.cpp`, `juce_VST3ClientExtensions.h`) — redistributing modified
JUCE source publicly carries license obligations that depend on which JUCE licence is held.
Also `plugins/*/Source/ui/public/js/juce/*.js` (JUCE-shipped JS) is vendored across many plugins.
The chosen root license (L1) must be compatible with whichever JUCE terms apply.

### L3 — Third-party dependency attribution
- `plugins/O-Orbit/libs/SAF` — git submodule → `github.com/leomccormack/Spatial_Audio_Framework`.
  Submodule URL is public HTTPS, so it clones fine for outsiders. Ships its own `LICENSE.md`.
- `plugins/O-TextureForge/.../app.bundle.js.LICENSE.txt` — bundled JS deps.
- moodycamel concurrent queue (`cameron@moodycamel.com` in headers).
- Other FetchContent deps (umappp, nanoflann, ONNX/ANIRA) per project notes.
→ Needs a `THIRD-PARTY-NOTICES.md` aggregating these.

### L4 — Audio asset provenance is documented for ONE folder only
- **Documented (CC0):** `plugins/O-Bassoon/research/reference-recordings/` — VSCO-2-CE, CC0 1.0,
  with a proper provenance `LICENSE.md`. Redistributable.
- **Undocumented, and these SHIP inside plugin binaries via BinaryData:**
  - `plugins/O-simpleGrain/Source/samples/` — `fire.wav`, `piano.wav`, `voice.wav`, `water.wav`
  - `plugins/O-simpleSampler/Source/samples/` — `cello.aif`, `hit.wav`, `piano.wav`, `pizz.aif`
  - `plugins/O-MicrotonalSampler/tests/fixtures/4-layer/` — `C4_v1..v4.wav`
  Provenance unknown → cannot confirm redistribution rights. Highest-risk legal item after L1,
  because these are compiled into distributed products, not just repo content.

---

## EFFICIENCY / REPO SIZE

Baseline: **`.git` = 912 MB** (`.git/objects` = 596 MB) for only 3308 tracked files.
Working tree = 7.6 GB (mostly untracked `build/`, `.cache/`, `backups/`, `.playwright-mcp/`).

### E1 — Binary test goldens dominate history (~250 MB of blobs)
Largest blobs in `git rev-list --objects --all`. Several appear **twice** (re-committed → each
revision stored again):

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

32 `.wav` tracked total. Note `plugins/O-Contrabass/e1-max-sustain.wav` (16.4 MB) is a stray
scratch render committed into a plugin folder, not a golden.

### E2 — Shipped installers committed to git (13.5 MB, three blobs)
- `plugins/O-Polystutter/dist/O-Polystutter-OuariconAudio.pkg` — 4.5 MB
- `plugins/O-Polystutter/dist/O-Polystutter-by-TACHES.pkg` — 4.5 MB (history only)
- `plugins/O-Polystutter/dist/PolyStutter.zip` — 4.5 MB
Build outputs belong on GitHub Releases (the workflow already publishes there), not in git.

### E3 — `build-release/` is tracked, including a compiled binary
10 tracked files: `CPackConfig.cmake`, `CPackSourceConfig.cmake`, `JUCE/*.cmake` (×7), and
**`build-release/plugins/O-Bowed/O-Bowed_vst3_helper`** — a compiled executable. `.gitignore`
covers `build/` but not `build-release/`.

### E4 — Root-level scratch files
Untracked but present and confusing for a public first impression: `e1-max-sustain.wav` (17 MB),
`o-bowed-pre-extraction-canonical.wav` (1.3 MB), `mbc-v150.png`, `o-reversedelay-484.png`,
`tooltip-knob.png`, `.DS_Store`, `scratch-pv/`. Also 38 `.png` + 23 `.jpg` tracked repo-wide
(e.g. `plugins/O-AnalogEQ/Source/ui/public/images/flower_ferdinandibauer00baue_0021.png`, 3 MB).

### E5 — Clone cost
At 912 MB, a `git clone` is slow and burns GitHub bandwidth on every fork/CI run.
GitHub soft-warns above 1 GB and hard-caps individual files at 100 MB (largest here is 50 MB —
under the cap, so no push will be rejected, but it is well past comfortable).

**Size reduction is the only item that requires history rewriting** (`git filter-repo` / BFG).
Rewriting changes every commit SHA — it must happen *before* the repo is public and before
anyone forks, or not at all. Removing files in a new commit shrinks the checkout but **not** `.git`.

---

## Facts that constrain the plan

1. **No secrets were ever committed** → history rewrite is optional (size-only), not mandatory.
   Do not let a size cleanup be framed as a security necessity.
2. **CI triggers are already fork-safe** → the CI work is hardening + a guardrail note, not a fix.
3. **The only hard legal blockers are L1 (no LICENSE) and L4 (undocumented shipped samples).**
4. `.gitignore` already lists several tracked paths — the fix is `git rm --cached`, not editing
   `.gitignore` again.
5. A submodule exists (`plugins/O-Orbit/libs/SAF`). Any `git filter-repo` run must preserve the
   gitlink entry, and per project convention worktree isolation is unsafe for submodule paths.
