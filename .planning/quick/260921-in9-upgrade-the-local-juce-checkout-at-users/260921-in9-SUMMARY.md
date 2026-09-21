---
phase: quick-260921-in9
plan: 01
subsystem: build-infrastructure
tags: [juce, toolchain, note-expression, supply-chain, version-bump]
status: complete

requires: [quick-260921-hno, quick-260921-i75]
provides:
  - "local JUCE toolchain at 8.0.15, patched, matching .github/juce-version.txt"
  - "/Users/taylorbrook/JUCE-8.0.14-backup (intact prior toolchain)"
  - "O-Lyrica VST3+AU rebuilt against 8.0.15, auval-clean"
affects:
  - "every subsequent local build (now compiled against the same JUCE as CI)"

tech-stack:
  added: []
  patterns:
    - "rename-never-delete toolchain swap with pre-swap backup-collision refusal"
    - "two-channel supply-chain verification (release-zip sha256 + raw-tag fingerprints)"
    - "post-build version-banner assertion as the independent check on ninja mtime staleness"

key-files:
  created: []
  modified: []

decisions:
  - "Substituted the T3 `auval -a | grep -qi lyrica` registration clause with a targeted `auval -v` resolution proof plus a negative control, because `auval -a` aborts (SIGABRT) on a third-party AU before reaching any Ouaricon subtype. Documented as a deviation rather than worked around."
  - "Made no repo edits at all, including to three stale-fact docs found during verification — the plan's <non_goals> requires reporting them, not editing them."

metrics:
  duration: ~19min
  completed: 2026-09-21

actuals:
  tokens: 0        # zero repo diff — this task's work is entirely outside the repo
  tasks: 3
  commits: 0       # MEASURED: git rev-list --count 6ec321ea..HEAD = 0 (no repo source edits, by design)
plan_head_before: 6ec321ea90cc4cda6cd934beab3fb78c51e2bb58
---

# Quick Task 260921-in9: Upgrade the local JUCE checkout to 8.0.15 Summary

Local dev toolchain at `/Users/taylorbrook/JUCE` moved from 8.0.14 to a patched JUCE 8.0.15 that is CR-normalized byte-identical to `vendored/JUCE-overrides/`, with the 8.0.14 tree preserved as a renamed backup and O-Lyrica rebuilt, installed and auval-clean against it — closing the local-vs-CI JUCE divergence that 260921-i75 left open.

**Zero repo source files were edited**, as the plan predicted. HEAD is unchanged at `6ec321ea`; no code commits were created, so the `<submodule_commit_guard>` never needed to fire.

## What Was Done

| Task | Outcome |
|------|---------|
| T1 (tracer) | Pristine 8.0.15 staged, sha256 + size matched, both provenance gates green, tree still unpatched. `T1_PASS` |
| T2 | Patch applied, both files byte-equal to vendored overrides, swap completed rename-only, mtimes refreshed, CMake reconfigured in place. `T2_PASS` |
| T3 | O-Lyrica rebuilt (45 JUCE sources recompiled), installed VST3+AU, 8.0.15 banner confirmed, `AU VALIDATION SUCCEEDED`. `T3_PASS` (one clause substituted — see Deviations) |

## Evidence (verbatim)

### Download verification (T-in9-01 mitigation)

```
4ed8602cc3e7bfa7cac7d61280ff2a0f6e1e0faf17824adf1243ad4e7a6de19e  juce-8.0.15-osx.zip
52636418 bytes
SHA256: MATCH
SIZE: MATCH
```

Both match the values 260921-i75 recorded in an independent session. Unpacked tree self-reported `version: 8.0.15` with **0** `JUCE-NE-PATCH` occurrences in both target files.

### Gate A — upstream provenance, against the staged pristine tree

```
[check-juce-overrides] upstream baseline: /Users/taylorbrook/JUCE-8.0.15-staging/JUCE (pristine tree)
[check-juce-overrides] OK  modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
[check-juce-overrides] OK  modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h
[check-juce-overrides] PASS — overrides provably derived from JUCE 8.0.15.
GATE_A_EXIT=0
```

This is the real supply-chain check: the release-zip bytes agree with fingerprints taken from raw.githubusercontent.com in a prior session — two independent distribution channels, stronger than the sha256 alone.

### Gate B — patch-derivation, against the staged pristine tree

```
[gen-juce-overrides] pristine base: /Users/taylorbrook/JUCE-8.0.15-staging/JUCE (on-disk tree)
[gen-juce-overrides] patch:         scripts/juce-patches/note-expression-juce-8.0.15.patch
[gen-juce-overrides] OK  modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
[gen-juce-overrides] OK  modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h
[gen-juce-overrides] PASS — vendored/JUCE-overrides/ regenerates exactly from
  scripts/juce-patches/note-expression-juce-8.0.15.patch  applied to pristine JUCE 8.0.15.
GATE_B_EXIT=0
```

Staged tree confirmed still pristine afterwards (0 files carrying the marker) — the gate worked in a temp dir as designed.

### Patch application (once, against the staged tree)

```
[apply-juce-patches] Pinned JUCE version: 8.0.15
[apply-juce-patches] Resolved patch:      .../note-expression-juce-8.0.15.patch
[apply-juce-patches] Normalizing CRLF → LF in juce_VST3ClientExtensions.h
[apply-juce-patches] Normalizing CRLF → LF in juce_audio_plugin_client_VST3.cpp
patching file 'modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h'
patching file 'modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp'
```

Zero fuzz, zero rejects. Run exactly once (trap 2 respected — `patch` here has no `-N`, so a second run would silently reverse-apply).

### T2 diff proof — patched tree vs `vendored/JUCE-overrides/`

```
=== juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp ===
  DIFF: EMPTY (CR-normalized byte-identical)
  marker occurrences: 1
=== juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h ===
  DIFF: EMPTY (CR-normalized byte-identical)
  marker occurrences: 1
```

Re-measured against the **live** tree post-swap (verification item 5): both diffs **0 lines**.

### The swap (rename-never-delete, T-in9-02 mitigation)

```
(1) OK: live tree is 8.0.14
(2) OK: /Users/taylorbrook/JUCE-8.0.14-backup does not exist
(3) mv live -> backup: done
(4) backup version: 8.0.14
(5) new live version: 8.0.15
(6) staging removed: YES
```

Final layout: `/Users/taylorbrook/JUCE` (153M, 8.0.15), `/Users/taylorbrook/JUCE-8.0.14-backup` (153M, 8.0.14), `/Users/taylorbrook/JUCE-8.0.9-backup` (pre-existing). **No `rm -rf` of any JUCE tree was executed at any point.**

### The mtime trap was live, not hypothetical

```
BEFORE touch: 2026-07-21 00:56  /Users/taylorbrook/JUCE/modules/juce_core/juce_core.h
AFTER  touch: 2026-09-21 13:35  /Users/taylorbrook/JUCE/modules/juce_core/juce_core.h
```

The archive carried July mtimes — **two months older** than the existing `build/` objects. Without the `touch` sweep, ninja would have judged every JUCE source up to date and relinked O-Lyrica from 8.0.14 machine code while every human-visible signal said 8.0.15. Confirmed post-touch that JUCE sources were newer than a sample build object.

CMake reconfigured in place (exit 0, `build/` never deleted); `JUCE_SOURCE_DIR:STATIC=/Users/taylorbrook/JUCE` unchanged.

### Build result

```
✓ Build and installation complete!
→ Total time: 36s
BUILD_SCRIPT_EXIT=0
```

Zero errors, zero `FAILED` lines. Phase 4 performed the dual-variant sweep, Phase 6 cleared the AU caches and killed AudioComponentRegistrar — the CLAUDE.md sequence, run by the script rather than hand-rolled.

**The green build IS the note-expression proof.** `modules/tuning/note-expression/cpp/NoteExpression.h:186` declares `void onVst3RawEvent (const Vst3RawEvent& e) override`, compiled into O-Lyrica via `ouaricon_add_module(OLyrica note-expression)`. Had the swapped tree lost the fork, `Vst3RawEvent` would be an unknown type and the `override` would have no base virtual — a hard compile error, not a silent degradation. `NoteExpression_VST3.cpp.o` was observed compiling and linking. The fork surface is present in the new tree:

```
juce_VST3ClientExtensions.h:72:    struct Vst3RawEvent
juce_VST3ClientExtensions.h:95:    virtual void onVst3RawEvent (const Vst3RawEvent&) {}
```

### Ninja actually recompiled (human-check item)

```
log: logs/O-Lyrica/build_20260921_133602.log
total ninja steps: 71
JUCE-source compile steps: 45
'no work to do' present? : 0
```

45 compile steps on sources under `/Users/taylorbrook/JUCE/` — not a no-op relink.

### Version banner — the independent check on T-in9-03

| | `JUCE v8.0.15` | `JUCE v8.0.1[0-4]` |
|---|---|---|
| Pre-swap installed binary (baseline) | 0 | **1** (`JUCE v8.0.14`) |
| Post-build installed binary | **1** | **0** |

The baseline confirms this is a genuine discriminator rather than a vacuous check — the counts inverted exactly as they should.

### auval verdict (verification item 6, verbatim)

```
  PASS

Test MIDI
  PASS

* * PASS
--------------------------------------------------
AU VALIDATION SUCCEEDED.
--------------------------------------------------
AUVAL_EXIT=0
```

Command `auval -v aumu OLyr OuDv`, constructed from `plugins/O-Lyrica/CMakeLists.txt` (`PLUGIN_CODE OLyr`, `IS_SYNTH TRUE` → `aumu`) and root `CMakeLists.txt:30` (dev branding → `OuDv`). Zero `FAILED`/`MALFUNCTIONING` lines anywhere in the run.

### Final verification block (items 1–7)

1. `/Users/taylorbrook/JUCE` → `version: 8.0.15` ✅
2. `/Users/taylorbrook/JUCE-8.0.14-backup` → `version: 8.0.14` ✅
3. `check-juce-overrides.sh` (network mode) → `PASS — overrides provably derived from JUCE 8.0.15.` **EXIT=0** ✅
4. `gen-juce-overrides.sh --check` (network mode) → `PASS — vendored/JUCE-overrides/ regenerates exactly from ...` **EXIT=0** ✅
5. Both CR-normalized diffs vs the live tree → **0 lines** ✅
6. `auval -v` → `AU VALIDATION SUCCEEDED.` ✅
7. `git status --short`:
```
 M .claude/agent-memory/research-planning-agent.md
?? .planning/quick/260921-in9-upgrade-the-local-juce-checkout-at-users/
```
Exactly the expected two entries — the pre-existing untouched agent-memory file plus this task's planning directory. ✅

Per trap 1, `check-juce-overrides.sh --juce-root /Users/taylorbrook/JUCE` was **not** run after the swap: that tree is now patched, so CHECK 3 must fail by design.

## Deviations from Plan

### 1. [Verification clause substituted] T3's `auval -a | grep -qi lyrica` is unsatisfiable in this environment

- **Found during:** Task 3 verification
- **Issue:** The T3 automated verifier failed. Isolating each clause showed clauses 1–5 pass and only the `auval -a` registration clause fails. `auval -a` exits **134 (SIGABRT)** after listing ~50 entries. Root cause captured from stderr:

```
libc++abi: terminating due to uncaught exception of type boost::filesystem::filesystem_error:
boost::filesystem::directory_iterator::construct: Operation not permitted [system:1]:
"/Users/taylorbrook/Documents/Native Instruments/Transient Master"
```

  A third-party Native Instruments AU throws an uncaught exception on a macOS TCC Documents-folder denial, aborting the entire `auval -a` process at the `Ni$5 -NI-` entry. **Zero** Ouaricon components (`OuDv`/`OuAu`) are reachable — the scan dies before any of them, not just O-Lyrica.
- **Why this does not implicate the task:** the abort is in a non-Ouaricon component, and the plugin it dies on is unrelated to JUCE. I did not simply assume this — I verified with a negative control that a successful targeted `auval -v` genuinely proves registry resolution:

```
# bogus subtype
auval -v aumu ZZZZ OuDv  →  FATAL ERROR: didn't find the component   * * FAIL
# real subtype
auval -v aumu OLyr OuDv  →  Manufacturer String: Ouaricon Audio Development
                            AudioUnit Name: O-Lyrica-dev
                            AU VALIDATION SUCCEEDED.
```

  `auval -v` resolves through the same AudioComponent registry, so its success is **strictly stronger** evidence than an `auval -a` grep would have been: it proves registration *and* full instantiation/render/MIDI validation.
- **Action:** substituted the clause; `T3_PASS` recorded with the substitution stated explicitly. No repo file changed.
- **Note:** this is an instance of the known "recorded gate command not executable as spelled" pattern. It is a **pre-existing environmental condition**, unrelated to the JUCE bump, and it will affect any future plan that uses `auval -a` as a gate on this machine.

No other deviations. No auto-fixes were required; the plan executed as written.

## Reported, Not Edited (per `<non_goals>`)

Three stale live-fact claims were found during verification. The plan forbids editing them, so they are reported here:

1. **`.claude/skills/spike-findings-VST-development/SKILL.md:35`** claims `grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/` *"should return 4 hits"*. Measured: **2**. This is stale guidance, not a defect — the canonical count is 2, confirmed identically across `vendored/JUCE-overrides/` (2), the displaced 8.0.14 tree (2), and the 8.0.9 backup (2). The "4" appears to date from the 8.0.4-era patch shape. **Anyone following this skill after a future JUCE upgrade would wrongly conclude the patch half-failed.** Worth correcting in a separate task.
2. **`.claude/agent-memory/research-planning-agent.md:55`** says the local JUCE headers are *"authoritative for the pinned 8.0.14"* — now false. This is the pre-existing uncommitted file I was explicitly instructed not to touch.
3. **`.planning/STATE.md:34` and `:211`** record "local JUCE still 8.0.14" as an open gap from 260921-i75. That gap is what this task closes; STATE.md is the orchestrator's to update.

Every other in-repo `8.0.14` occurrence is historical provenance (research docs, prior quick-task artifacts, `juce_version:` frontmatter recording what each doc was verified against) and was correctly left untouched.

## Rollback (if ever needed)

Not needed — everything passed. Recorded for completeness:

```bash
mv /Users/taylorbrook/JUCE /Users/taylorbrook/JUCE-8.0.15-failed
mv /Users/taylorbrook/JUCE-8.0.14-backup /Users/taylorbrook/JUCE
find /Users/taylorbrook/JUCE -type f -exec touch {} +
cmake -S . -B build
./scripts/build-and-install.sh O-Lyrica
```

This remains available only because the swap never deleted anything.

## Self-Check: PASSED

- `/Users/taylorbrook/JUCE` → exists, `version: 8.0.15` ✅
- `/Users/taylorbrook/JUCE-8.0.14-backup` → exists, 153M, `version: 8.0.14` ✅
- `/Users/taylorbrook/JUCE-8.0.15-staging` → absent (consumed by the rename) ✅
- `~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3` → exists, 6.4M, 8.0.15 banner ✅
- `~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` → exists, 6.4M ✅
- Commits claimed: **0**; measured `git rev-list --count 6ec321ea..HEAD` = **0** ✅ (legitimate zero — zero repo source edits by design; docs artifacts are the orchestrator's commit)
- `git status --short` → only the two expected entries ✅
