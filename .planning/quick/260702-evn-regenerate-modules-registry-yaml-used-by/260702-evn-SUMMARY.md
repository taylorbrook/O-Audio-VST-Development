---
phase: quick-260702-evn
plan: 01
subsystem: modules-registry
status: complete
tags: [registry, tooling, disk-truth, used_by, python, bash]
requires:
  - plugins/*/CMakeLists.txt + plugins/*/Source (disk truth)
  - modules/<path>/{cpp,js} file basenames
provides:
  - scripts/regen-registry-used-by.sh (re-runnable used_by regenerator)
  - modules/registry.yaml (used_by lists synced to disk truth)
affects:
  - modules/registry.yaml
tech-stack:
  added: []
  patterns:
    - "Disk-truth regeneration: derive used_by by grepping consumers for a module's own file-basename + name tokens"
    - "Surgical block-splice YAML rewrite (stdlib only, no PyYAML) preserving all non-used_by lines byte-for-byte"
    - "Idempotent header bump: version/last_updated advance only when content changes"
key-files:
  created:
    - scripts/regen-registry-used-by.sh
  modified:
    - modules/registry.yaml
decisions:
  - "vu-meter and playable-keyboard regenerate to [] — their historical consumers ship inline/divergent implementations, not the module's files. This is correct disk truth, not an error; consumers were NOT hand-added back."
  - "Version resolution order: PLUGIN_VERSION -> non-cmake_minimum_required VERSION -> root project VERSION (1.0.0 fallback) -> unknown."
metrics:
  duration: ~6min
  completed: 2026-07-02
  tasks: 2
  files: 2
requirements: [UPD-02, IMP-02, UPD-03]
---

# Quick Task 260702-evn: Regenerate modules/registry.yaml used_by Summary

Shipped `scripts/regen-registry-used-by.sh`, a deterministic, idempotent regenerator that rewrites every module's `used_by` list in `modules/registry.yaml` from disk truth, then ran it to correct the badly-drifted lists and bump the registry header.

## What Was Built

**`scripts/regen-registry-used-by.sh`** — bash wrapper (`set -euo pipefail`, repo-root via `git rev-parse --show-toplevel`) delegating to an embedded `python3` (stdlib only — no PyYAML) rewriter that:

1. Parses module entries under the top-level `modules:` line (ignoring the `categories:` block that also uses `  - name:`).
2. Derives each module's grep TOKENS = basenames of every `*.h/*.cpp/*.js` under `modules/<path>/{cpp,js}` (recursive), plus the registry `name` as a CMake `ouaricon_add_module(...)` token.
3. Enumerates consumers = every `plugins/*` dir except `tache_plugins`, matching file tokens via `grep -rlI -F` over `CMakeLists.txt` + `Source/`, and the name token via `grep -rlE 'ouaricon_add_module\([^)]*\b<name>\b'` over `CMakeLists.txt`.
4. Resolves each consumer's version (PLUGIN_VERSION → non-`cmake_minimum_required` VERSION → root `project(... VERSION ...)` fallback 1.0.0 → `unknown`).
5. Surgically splices only the `used_by` blocks (byte-for-byte preservation of every other line) and, for UPD-03, inserts a reminder comment above `version:` and bumps `version` patch + `last_updated` when content changes.

No plugin names, retired-name mappings, or consumer lists are hardcoded — everything derives from disk. A second consecutive run produces zero further diff.

## Per-Module before/after used_by Diff (script stdout, verbatim)

```
webview-relay-manager: [] -> [O-Prism]
resource-provider: [] -> []
webview-drop-streaming: [O-MicrotonalSampler] -> [O-MicrotonalSampler, O-simpleGrain, O-simpleSampler]
preset-manager: [OuariconMarimba, OuariconTremolo, OFreqPulse] -> [O-AnalogEQ, O-Bass, O-Bassoon, O-Bells, O-Bowed, O-Chorus, O-Comp, O-Detune, O-DigiDelay, O-Formant, O-FreqPulse, O-Lyrica, O-Polystutter, O-Prism, O-Reed, O-simpleFM, O-simplePhysicalModelSynth, O-SimpleReverb, O-SpectralShaper, O-Tremolo, O-Wind]
vu-meter: [OuariconComp, OuariconAnalogEQ] -> []
scala-tuning-engine: [] -> [O-Bassoon, O-Bells, O-Bowed, O-Contrabass, O-Formant, O-IntonationPad, O-Lyrica, O-Marimba, O-MicrotonalSampler, O-Prism, O-Reed, O-Wind]
note-expression: [OLyrica, O-Bells, O-Prism, O-Wind, O-IntonationPad, O-Reed, O-Bowed, O-Formant] -> [O-Bassoon, O-Bells, O-Bowed, O-Contrabass, O-Formant, O-IntonationPad, O-Lyrica, O-MicrotonalSampler, O-Prism, O-Reed, O-Wind]
bow-friction: [O-Bowed, O-Contrabass, O-simplePhysicalModelSynth] -> [O-Bowed, O-Contrabass, O-simplePhysicalModelSynth]
analog-eq-unit: [OuariconMarimba] -> [O-Marimba]
compressor-unit: [OuariconMarimba] -> [O-Marimba]
playable-keyboard: [OuariconMarimba] -> []
REGEN: changed
```

### Drift corrections of note
- **scala-tuning-engine**: `[]` → **12 consumers** (previously blank despite wide use).
- **note-expression**: 8 stale entries → **11 correct** — added O-Bassoon, O-Contrabass, O-MicrotonalSampler; fixed `OLyrica` → `O-Lyrica`.
- **preset-manager**: 3 retired names → **21 real O-* consumers**.
- **vu-meter** and **playable-keyboard** → `[]`. Their historical consumers ship inline/divergent implementations, not this module's files. This is **correct disk truth** — consumers were deliberately NOT hand-added back.
- **analog-eq-unit / compressor-unit**: retired `OuariconMarimba` → `O-Marimba` (version now 1.11.0 from disk).
- **webview-relay-manager** `[]` → `O-Prism`; **webview-drop-streaming** picked up O-simpleGrain + O-simpleSampler.

## Verification

- `bash -n` clean; script executable; shebang `#!/usr/bin/env bash`; hardcodes no plugin names.
- Consumer counts match reviewer expectations: scala-tuning-engine=12, note-expression=11, vu-meter=0, playable-keyboard=0.
- In-flight **bow-friction** working-tree edit PRESERVED — script re-derives O-simplePhysicalModelSynth from disk, reproducing (not clobbering) the uncommitted addition.
- No retired directory names (`OuariconMarimba|OuariconComp|OuariconAnalogEQ|OuariconTremolo|OFreqPulse|OLyrica`) survive.
- Only `used_by` blocks + 3 header lines/comment changed — `git diff` confirms no description/provides/config/tags/reuse_score field altered; 11 module entries intact; YAML parses.
- Header (UPD-03): `version` 1.0.0 → 1.0.1, `last_updated` = 2026-07-02, reminder comment present.
- **Idempotent**: second consecutive run reports `REGEN: no-op (already fresh)` and yields zero further `git diff`.

## Deviations from Plan

None — plan executed exactly as written.

## Commits

- `27e890c` feat(quick-260702-evn): add disk-truth used_by regenerator script (Task 1)
- `6d046c8` fix(quick-260702-evn): regenerate registry used_by from disk truth (Task 2)

## Working-Tree Isolation

Executed on the user's main checkout amid unrelated in-flight work. All commits used explicit pathspecs (`git commit -- <file>`); `git show --stat` confirmed each commit contained only its intended file. No pre-existing staged files were swept in; no submodule (plugins/O-Orbit/libs/SAF) paths touched.

## Self-Check: PASSED

- FOUND: scripts/regen-registry-used-by.sh
- FOUND: modules/registry.yaml
- FOUND commit 27e890c
- FOUND commit 6d046c8
