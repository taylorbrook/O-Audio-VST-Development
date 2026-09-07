---
phase: quick-260907-f5x
plan: 01
subsystem: O-MultiBandCompressor UI / release
tags: [i18n, canvas, product-decision, version-bump, deferred-item]
status: complete
requires: []
provides:
  - "O-MultiBandCompressor v1.12.1 — spectrum placeholder paints one centred localized caption and no internal build-stage number"
  - "Wave 4d deferred item D3 answered"
  - "CODE_REVIEW.md IN-09 closed by its first proposed remedy"
affects:
  - plugins/O-MultiBandCompressor
tech-stack:
  added: []
  patterns:
    - "Canvas 2D context state is shared with the live analyser path — textBaseline deliberately NOT set"
    - "Two-arm CMake version reader (single-arm returns empty on a `VERSION ${VAR}` file)"
key-files:
  created: []
  modified:
    - plugins/O-MultiBandCompressor/Source/ui/public/js/app.js
    - plugins/O-MultiBandCompressor/Source/ui/public/js/i18n.js
    - plugins/O-MultiBandCompressor/CMakeLists.txt
    - plugins/O-MultiBandCompressor/CHANGELOG.md
    - PLUGINS.md
decisions:
  - "Caption re-centred by changing `height / 2 - 10` to `height / 2` rather than by setting `ctx.textBaseline = 'middle'`"
  - "CODE_REVIEW.md left unedited — no resolved/struck convention exists in it; the CHANGELOG is the shipping record"
  - "The 1.12.0 CHANGELOG entry left byte-identical — its 'left exactly as found' sentence is correct history of that release"
metrics:
  duration: ~12min
  completed: 2026-09-07
actuals:
  tokens: 54552
  tasks: 3
  commits: 1
---

# Quick Task 260907-f5x: O-MultiBandCompressor build-stage marker removal Summary

**O-MultiBandCompressor v1.12.1 — the spectrum-analyzer placeholder canvas stops
painting an internal build-stage phase number on a shipping surface, the surviving
localized caption moves to the canvas centre, and the verification premise recorded
in `i18n.js` was amended in the same commit rather than left to age.**

Wave 4d recorded this as deferred item **D3** and was explicit that it was a
*product* question with no localization budget attached, so the marker was left
exactly as found. This task is that decision being taken. It also closes **IN-09**
in `CODE_REVIEW.md` (the v1.6.0 review), which flagged the same string and offered
two remedies — the first, drop the dev-phase caption, is the one taken.
`initializeSpectrumPlaceholder()` itself is kept.

## What Was Built

| Task | Name | Result |
|------|------|--------|
| 1 (tracer) | Remove the marker at source, re-centre the caption, correct the `i18n.js` premise | Done — all six source gates flipped or held as predicted |
| 2 | Bump to 1.12.1 across the three version records | Done — CMakeLists, CHANGELOG, PLUGINS.md |
| 3 | Rebuild, reinstall, verify the AU at 1.12.1, commit path-scoped | Done — auval `1.12.1 (0x10C01)`, SUCCEEDED; one commit `abb0ea2d` |

One commit, path-scoped, on `main`: **`abb0ea2d`** — exactly the five planned
files (53 insertions, 11 deletions), carrying the `Claude-Session:` trailer.

## The Before/After Gate Pair

Every gate below was fired against the **unfixed** tree first and printed the "was"
value the plan predicted, so each zero is a proven discriminator rather than a
hopeful one.

| Gate | Command | Was | Now |
|---|---|---|---|
| marker present | `grep -c 'Phase 5\.3' app.js` | **1** | **0** |
| negative grep, code lines only | `grep -v '^[[:space:]]*//' app.js \| grep -cE "fillText\((\"\|')"` | **1** | **0** |
| **positive control** | `grep -c 'fillText(tr(' app.js` | **1** | **1** |
| re-centred caption | `grep -c 'canvasLang).t, width / 2, height / 2)'` | 0 | **1** |
| old offset gone | `grep -c 'height / 2 - 10'` | 1 | **0** |
| `Phase 5.2` header untouched | `grep -c 'Phase 5.2: Full parameter binding'` | 1 | **1** |
| repo i18n gate | `node scripts/check-i18n.js` | exit 0, 43/43 | **exit 0, `ALL CHECKS PASS — 43 localized plugin(s)`** |

**The negative grep's zero is only evidence because the positive control held at 1
beside it.** A zero on its own is equally consistent with a broken grep — which is
exactly what the plan's *first* spelling of it did, reading 0 on a tree where the
marker was still present.

The nine legitimate `Phase 5.3` hits elsewhere in the plugin are untouched, and the
distribution is unchanged: CHANGELOG 1, CODE_REVIEW.md 1, NOTES.md 1,
`.contracts/plan.md` 2, `PluginProcessor.h` 2, `PluginProcessor.cpp` 2 = **9**.

## Version and AU Identity

| Record | Reading |
|---|---|
| two-arm CMake reader | **`1.12.1`** (single-arm reader returns empty on this file — wave 4d M4) |
| stale `1.12.0` literal in CMakeLists.txt | **0** |
| render harness `JucePlugin_VersionString` | still `${OMBC_VERSION}` — a variable, no second bump site |
| CHANGELOG | `## Version 1.12.1 (2026-09-07)` at line 3, above an **unmodified** `## Version 1.12.0 (2026-09-06)` at line 39 |
| PLUGINS.md row 39 | `\| O-MultiBandCompressor \| 📦 Installed \| 1.12.1 \| Audio Effect (Dynamics) \| 2026-09-07 \|`, no duplicate rows |
| installed AU `Info.plist` | **68608 → 68609** (`0x010C01` = 1.12.1) |
| `auval -v aufx OMbc OuDv` | `Component Version: 1.12.1 (0x10C01)` … **`AU VALIDATION SUCCEEDED.`** |
| installed bundles | **only `-dev` variants** in both folders; no unsuffixed shadow bundle |

The plist integer moving 68608 → 68609 is what proves the host sees the **new**
binary and not a stale bundle — the source gates would read green either way
(threat T-f5x-02).

Build: `./scripts/build-and-install.sh O-MultiBandCompressor` exit 0 in 50s.
`ninja -C build O-MultiBandCompressor_Standalone` exit 0 separately, because
`build-and-install.sh` builds VST3 + AU only and the Standalone would otherwise
have kept the old `app.js` and misled any later visual inspection.

## Corrections to the Plan's Own Measurements

**D3's recorded line number was wrong, and predictably so.**
`deferred-items.md` line 315 records the marker at **`js/app.js:899`**. It was
actually at **line 927** — a 28-line drift in two days. The plan anticipated this
and instructed a re-grep before editing rather than trusting the number; that
re-grep returned exactly one hit and the edit proceeded.

**The same defect is already sitting in `CODE_REVIEW.md`, one release older and
much further off.** IN-09 (line 206) locates the same string at **`app.js:427-428`**
— off by ~500 lines. Two independent records of one string, both stale, neither
detectably so from reading it.

**The generalisation:** a bare line number in a live file has a shelf life measured
in days. Any future deferral or review finding that records a location should
record a **grep-able token** beside it — here, `Phase 5.3` would have survived every
drift and located the site exactly. The line number is a convenience for a human
reading today; the token is what makes the record still executable next month.
This is the same class as the plan's own note that a transcribed gate command must
be re-fired rather than trusted as spelled.

## The i18n.js Premise (wave 4d M3 class)

`i18n.js:250-258` stated, as a permanent fact, that **one** `fillText` string
literal survives one line below the localized caption, and characterised it as a
build-stage marker left as found. **This change makes zero survive**, so the
sentence was false the moment the marker went.

It was amended in the same commit. The negative grep the block nominates as its
verification is now stated as **absolute** — no string-literal call of that shape
remains anywhere in `app.js` — and the removal is attributed to the D3 product
decision. The `v1.12.0.` provenance line at the top of the block is untouched;
that dating is correct history for when the key landed.

This is exactly wave 4d's **M3**: a comment stating a permanent fact about a plugin
is wrong the first time the plugin changes, and O-ReverseDelay shipped a RED gate
for two releases because nobody amended one. The premise was load-bearing, not
decoration — it is the *only* recorded description of how this canvas string is
verified, since no gate in the repo can see canvas text.

### Sweep for other falsified in-tree statements

Fired `grep -rniE 'build[- ]stage'` across `plugins/` and `scripts/` over source,
markup, style, C++, CMake and JSON. **Two hits, both the comments just written, both
now correct.** No other plugin's `i18n.js` references this key. Additional checks:

- **`NOTES.md:110`** — `**Phase 5.3:** GR meters + FFT visualization threading`. This
  is the development roadmap's own phase list, not a claim about the canvas.
  Correct history; left.
- **`CODE_REVIEW.md:206`** — IN-09 states the placeholder "still says (Phase 5.3)".
  This *is* now false, but it was left deliberately: the file carries no
  resolved/struck convention, and inventing one mid-task would be a format nobody
  else in the repo writes. The CHANGELOG's 1.12.1 entry is the shipping record and
  names IN-09 explicitly, so the closure is discoverable from the release notes.
  **Flagged as a standing item** — see below.
- **`CHANGELOG.md` 1.12.0 entry** — describes the marker as left exactly as found.
  Correct history *of that release*; the diff confirms 36 insertions and **0
  deletions** in that file, so the entry is byte-identical.

## Decisions Made

**The caption was re-centred by moving the y-argument, not by setting
`textBaseline`.** `ctx.textBaseline = 'middle'` at `height / 2` would centre the
glyph block exactly and would pair naturally with the `ctx.textAlign = 'center'`
already present — and it is rejected. `initializeSpectrumPlaceholder()` takes
`canvas.getContext('2d')` on `#spectrum-canvas`; the live analyser path lazily
caches `spectrumCtx = canvas.getContext('2d')` on the **same element**, and
`getContext('2d')` returns the identical object. `textBaseline` is persistent
context state, so setting it in the placeholder silently arms a trap for the first
person who paints text on the live spectrum path — inert today, free later, and
exactly the shape of defect M3 warns about. The default alphabetic baseline at
`height / 2` puts the optical centre ~5px high, which costs nothing. Recorded at the
call site in `app.js` so the next reader does not "improve" it.

## Deviations from Plan

**None affecting behaviour or scope.** One procedural note:

**[Procedural] Build and auval logs written to `/tmp/`, not the session scratchpad.**
The orchestrator's constraints ask for scratchpad logs; the plan's `<verify>` blocks
grep `/tmp/mbc-1121-build.log` and `/tmp/mbc-1121-auval.log` by exact path. Running
each gate exactly as spelled was the stronger constraint, so the plan's paths were
used. Both commands ran in the background with polling, well inside the watchdog
band (build 50s, auval ~5s — no cold registry rescan occurred).

## Gates Deliberately Not Run

- **`check-ui-labels.js`** — spelled correctly and would execute, but structurally
  blind to this diff: it reads computed style and rendered geometry on *elements*,
  and this change touches only `fillText` calls inside a canvas paint routine. Both
  `app.js` and `i18n.js` state in-tree that it has no canvas awareness. Running it
  could only produce a vacuous green while risking the known UI-test port clash
  between concurrent sessions.
- **The optional `<human-check>`** (stub server visual confirmation) — explicitly
  marked OPTIONAL and non-blocking in the plan. Not run, for the same port-clash
  reason: with another session potentially serving a UI on the same port, a
  confirming screenshot could be of a *different plugin* and would be worse than no
  observation. The placeholder branch runs only while the analyser is empty, so it
  is not reliably observable in a DAW either. The source-level gate pair plus the
  installed-plist version integer is the evidence this change actually rests on.

## Known Stubs

None.

## Threat Flags

None. No new network endpoint, auth path, file-access pattern or schema change.
No package-manager install occurred (T-f5x-SC accepted as not applicable).

## Standing Items

- **`CODE_REVIEW.md` IN-09 is now stale in two ways** — its recorded location
  (`app.js:427-428`) is ~500 lines off, and its premise ("still says (Phase 5.3)")
  is false as of v1.12.1. Left unedited by design. If a resolved/struck convention
  is ever adopted for `CODE_REVIEW.md`, IN-09 is a ready first entry.
- **Line numbers in deferral and review records** — adopt a grep-able token
  alongside any recorded location. Two records of one string, both stale, is the
  evidence.

## Self-Check: PASSED

- `plugins/O-MultiBandCompressor/Source/ui/public/js/app.js` — FOUND
- `plugins/O-MultiBandCompressor/Source/ui/public/js/i18n.js` — FOUND
- `plugins/O-MultiBandCompressor/CMakeLists.txt` — FOUND
- `plugins/O-MultiBandCompressor/CHANGELOG.md` — FOUND
- `PLUGINS.md` — FOUND
- Commit `abb0ea2d` — FOUND in `git log`, on `main`, working tree clean
