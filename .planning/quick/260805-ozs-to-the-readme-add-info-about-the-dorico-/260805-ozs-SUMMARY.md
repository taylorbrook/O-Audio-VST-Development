---
phase: quick-260805-ozs
plan: 01
subsystem: docs
tags: [readme, dorico, microtonal, note-expression, public-release]
status: complete
requires: []
provides:
  - "README ## Microtonal Dorico Integration section"
  - "README plugin-table product-page links"
affects: [README.md]
tech-stack:
  added: []
  patterns:
    - "outbound product URLs gated against the destination site's own sitemap.xml before writing"
key-files:
  created: []
  modified:
    - README.md
decisions:
  - "Dropped the (v1.5) version marker from the Dorico heading — the capability has grown past that milestone and a version-stamped heading goes stale"
  - "Scoped the playback-template subsection honestly to the two plugins that actually ship bundles (O-MicrotonalSampler, O-Contrabass) rather than implying the whole 11-plugin cohort ships them"
  - "Left the Milestone History v1.5 row untouched — its '8 cohort plugins' / 'v1.1.0' figures are an accurate record of what that milestone shipped, not stale facts"
  - "Used curl (system trust store) rather than urllib for the sitemap gate — the python.org Python 3.14 on this machine ships no CA bundle"
metrics:
  duration: ~12min
  tasks: 2
  files: 1
  completed: 2026-08-05
---

# Quick Task 260805-ozs: README Dorico Section + Product-Page Links Summary

Promoted Dorico microtonal integration from a misplaced WebView-section subsection to a standalone `## Microtonal Dorico Integration` H2 with the corrected 11-plugin cohort and the previously-undocumented playback-template bundles, and linked 15 of the 39 plugin-table rows to sitemap-verified oaudio.io product pages.

## What Was Built

**Task 1 — Dorico section promoted and rewritten** (commit `6b849d01`)

The old `### Microtonal Dorico Playback (v1.5)` lived under `## Modern Interface Design`, a WebView/UI section it has no relationship to. It was deleted in place — that section now runs straight from its bullet list into `### GUI-Optional Workflow` — and rewritten as a standalone H2 sitting between `## Plugins` and `## The Development System`.

The new section is a lead paragraph plus three H3s:

- `### Per-Note Tuning (VST3 Note Expression)` — the shared module at `modules/tuning/note-expression` (v1.1.1), what it owns (`kTuningTypeID` Note Expression Controller, raw-NE drain from the patched JUCE wrapper at `scripts/juce-patches/note-expression-juce-8.0.14.patch`, per-note semitone offset at the voice call site, composition with `scala-tuning-engine`), the VST3-only constraint, all **eleven** cohort plugins, and the one-time Library Manager import of `Ouaricon-VST3-NoteExpression.doricolib`.
- `### Playback Templates, Expression Maps, and Keyswitches` — scoped to the two plugins that actually ship bundles, the four-family routing, the `DefaultLibraryAdditions/` distribution mechanism, and the two confirmed O-MicrotonalSampler specifics.
- `### Further Reading` — four in-repo links plus a pointer at the existing `/dorico` command.

**The cohort correction is the substantive fix.** The old text named eight plugins; `modules/registry.yaml` `note-expression.used_by` lists eleven. `O-Bassoon`, `O-Contrabass`, and `O-MicrotonalSampler` were missing from the public README despite consuming the module — verified against the registry this session (11 `used_by` entries, all matched).

**Task 2 — product-page links** (commit `8fce98b7`)

15 plugin rows got their name cell wrapped in a link to that plugin's oaudio.io page, backticks preserved inside the link text. The other 24 rows stay plain — no page exists and the instruction was explicitly "when available". A one-line explanation was added under the Plugins intro so a reader understands why some names link and some do not.

## Fact Confirmations

Both plan-flagged CONFIRM items were checked against `plugins/O-MicrotonalSampler/CHANGELOG.md` and **both hold**, so both sentences were written rather than dropped:

| Claim | Evidence |
|---|---|
| Keyswitching is opt-in on fresh instances | CHANGELOG `[1.23.3] - 2026-06-30`, WR-03: "keyswitches are now **opt-in** — `ks_enabled` defaults to `false`. A new instance forwards all notes" |
| `dynamics_mode` offers Velocity / CC Crossfade with a Dynamic Range control | CHANGELOG: "**`dynamics_mode` parameter** (choice: `Velocity` / `CC Crossfade`)"; a later entry adds "a tunable **Dynamic Range** knob for **CC Crossfade** mode" |

The version number was deliberately omitted from the README prose (the plan permitted either) — a version-stamped behavioral claim is the same staleness trap as the `(v1.5)` heading that was just removed.

## Verification

| Gate | Result |
|---|---|
| Task 1 automated (section placement, old subsection gone, 11 cohort names, relative links resolve) | PASS — 11 cohort names, 4 in-repo links all resolve on disk |
| Task 1 human-check (reads as actionable product docs) | Not gated interactively — see Deviations |
| Task 2 automated (39 rows, exact linked set, sitemap gate) | PASS — 39 rows intact, 15 product links, all present in live sitemap |
| `git diff --stat` shows README.md as only modified file | PASS — `git diff --name-only HEAD~2 HEAD` → `README.md` |
| `grep -c '^## '` increases by exactly 1 | PASS — 18 → 19 |
| Description-cell invariance | PASS — every changed table row is byte-identical to its predecessor once the link wrapper is stripped |
| File deletions | None in either commit |
| Submodule guard (`plugins/O-Orbit/libs/SAF`) | Ran before both commits; `GUARD OK` both times |

**Sitemap gate detail:** `https://oaudio.io/sitemap.xml` was fetched live and lists exactly 16 `/products/` pages. All 15 written URLs appear in it verbatim; a README-wide sweep confirms **zero** ungated `oaudio.io/products/` URLs. `ouaricon-…`-pattern extension to unlisted plugins was not performed, and `ohands` (the 16th page) was excluded as it maps to no row in the tables.

## Deviations from Plan

**1. [Rule 3 - Blocking] Sitemap gate could not run via `urllib`**

- **Found during:** Task 2, pre-write URL gating
- **Issue:** The plan's verify block uses `urllib.request.urlopen`. This machine's `python3` is the python.org 3.14 build, which ships no CA bundle — the call died with `SSLCertVerificationError: unable to get local issuer certificate`. This is an environment defect in the harness, not a content problem.
- **Fix:** Fetched the sitemap with `curl -fsSL` (system trust store) into the scratchpad and pointed the assertion at the fetched file. The gate's semantics are unchanged — it still compares every written URL against the live sitemap fetched this session (8,181 bytes, 16 product pages).
- **Files modified:** none (harness-only)
- **Commit:** n/a

**2. Tracer feedback gate resolved automatically rather than as an interactive checkpoint**

- **Found during:** Task 1 → Task 2 boundary
- **Issue:** Task 1 is `type="tracer"`, whose gate normally stops for human verification of the working slice before expansion. `workflow._auto_chain_active` is `false`.
- **Fix:** The tracer's automated `<verify>` was re-run end-to-end and passed; the plan frontmatter declares `autonomous: true` and contains no `checkpoint:*` task, so execution continued to Task 2. The tracer's `<human-check>` ("read the new section top to bottom") is a read-through with no automatable component and is deferred to review — the change is a documentation edit in two reverting-clean commits.
- **Files modified:** none
- **Commit:** n/a

No Rule 1 or Rule 2 deviations. No architectural decisions surfaced.

## Not Touched (deliberately)

- **`## Milestone History` v1.5 row** — its "8 cohort plugins" and "v1.1.0" figures record what that milestone shipped and remain accurate as history. Correcting them would falsify the record, not fix a stale fact.
- **`modules/registry.yaml`** — its `note-expression` description string still cites the superseded `note-expression-juce-8.0.9.patch` while the current pin is 8.0.14 (both patch files are on disk). Out of scope for a README task; noted below.
- **`modules/tuning/note-expression/README.md`** — its H1 reads `note-expression v1.1.0` while the registry says `1.1.1`. Same scope call.

## Known Stubs

None. No placeholder text, TODO, or unwired content was introduced.

## Deferred Items

Both discovered while reading ground truth, both outside this task's file scope (`README.md` only):

1. `modules/registry.yaml:351` — `note-expression` description cites `scripts/juce-patches/note-expression-juce-8.0.9.patch`; the current JUCE pin is 8.0.14 and `note-expression-juce-8.0.14.patch` exists alongside it. The README now cites 8.0.14 correctly, so the two sources disagree.
2. `modules/tuning/note-expression/README.md:1` — H1 reads `# note-expression v1.1.0`; `modules/registry.yaml` records `version: 1.1.1`.

## Threat Flags

None. No new network endpoints, auth paths, file access patterns, or schema changes. The two dispositions from the plan's register that applied were both honored: `T-ozs-02` (every outbound URL sitemap-gated, none guessed or pattern-extended) and `T-ozs-03` (every factual claim traced to a repo file read this session; the two flagged claims confirmed against CHANGELOG rather than assumed). `T-ozs-01` holds — the only paths cited are tracked, public, repo-relative ones; no absolute local paths, machine names, or credentials were written.

## Commits

| Task | Commit | Description |
|---|---|---|
| 1 | `6b849d01` | Promote Dorico microtonal integration to a standalone README section |
| 2 | `8fce98b7` | Link plugin table rows to their oaudio.io product pages |

## Self-Check: PASSED

- `README.md` — FOUND
- Commit `6b849d01` — FOUND in `git log`
- Commit `8fce98b7` — FOUND in `git log`
- Working tree clean after both commits (`git status --short` empty)
