---
title: "O-MicrotonalSampler Phase 4.3 — QUAL-01 listening pass"
created: 2026-04-28
phase: 4.3
status: complete
verifies_requirements:
  - QUAL-01   # partial → complete (6/7 unambiguous PASS, item 1 audio-quality PASS with v1.1 behavioral follow-up, item 6 skipped)
---

# Phase 4.3 — QUAL-01 listening pass (SUMMARY)

## Goal recap

Run the 7-item targeted artifact-pass listening checklist per
`PLAN.md §Phase 4.3`. All-pass flips `QUAL-01` from `partial` to
`complete`; any defect reopens the relevant Stage 2 sub-phase per
`PLAN.md §Failure Routing`.

## What landed

User-driven listening pass on the Phase 4.1 gate-time bundle
(`O-MicrotonalSampler-dev`, installed at
`~/Library/Audio/Plug-Ins/{VST3,Components}/`).

| # | Test | Verdict |
|---|---|---|
| 1 | Sustained sine | **PASS** (audio quality) — behavioral observation logged as v1.1 V11-LOOP-FALLBACK |
| 2 | Cello vibrato / organic legato | PASS |
| 3 | Transient / plucked | PASS |
| 4 | ±50 c retune sweep | PASS |
| 5 | Voice-steal stress (24 notes / 16 cap) | PASS |
| 6 | Mixed-SR fixture | **SKIPPED** at user discretion — logged as v1.1 V11-MIXED-SR-EXPLICIT |
| 7 | Short-region loop edge case | PASS |

Full per-item evidence and verdict rationale in
`.planning/stages/4-polish/VERIFICATION.md §QUAL-01`.

## Path A taken (per pre-execute discuss)

Item 1 surfaced a **behavioral / spec gap** rather than a QUAL-01
audio-quality defect: the sustained sine plays cleanly through its
sample length but does not loop until note-off because
`LoopDetector::detectLoop`'s variance gate rejects sine waves (they
have constant RMS, no quiet window) and falls through to `OneShot`.
The user-stated expectation — "the entire sample should repeat by
default until there is a noteoff message" — is **not met** for
sine-like sustained material under the current heuristic.

Three paths considered before execute:

- **(A) v1.1 backlog item, ship v1.0 today** — chosen.
- (B) Reopen Stage 2.5 LoopDetector before closing Stage 4.
- (C) UI option for default loop mode (Auto / Full-sample / OneShot).

**Rationale for A:**

1. The QUAL-01 criterion as written is "no clicks / zipper / aliasing
   across vel · poly · retune." Item 1 produced no audio artefacts —
   the envelope is clean, playback simply stops at sample length. The
   criterion is met.
2. The loop-point editor (DSP-06, complete) already provides a per-slot
   workaround in v1.0: the user sets manual loop points on any
   sustained sample that falls through the heuristic. The v1.0 release
   is internal-use only (no public distribution per Stage 4 D4-3), so
   per-slot manual override is an acceptable workaround.
3. Stage 2.5 reopen would delay Stage 4 closure for ~30-60 min of
   heuristic-tuning work + retest, on a `should`-priority finding that
   has a v1.0 workaround.
4. Real-use feedback over the next week of internal use is more
   valuable than speculative heuristic tuning now. If the heuristic
   feels wrong on too many real samples, V11-LOOP-FALLBACK gets
   prioritised in v1.1.

Item 6 was skipped at user discretion. The Lagrange resample-on-load
path (`SampleLoader::loadSingleSlot`) and the Cubic-Hermite runtime
interpolator (`MicrotonalSamplerVoice::cubicInterp`) are exercised
indirectly by items 2 and 5; both produced clean output. Logged as
v1.1 V11-MIXED-SR-EXPLICIT for an explicit fixture-driven retest in
the next milestone.

## Files modified

- `.planning/stages/4-polish/VERIFICATION.md` — QUAL-01 section
  populated with the 7-item outcome table, verdict rationale, and
  the three v1.1 follow-up entries (V11-LOOP-FALLBACK,
  V11-PERF-METER, V11-MIXED-SR-EXPLICIT).
- `.planning/REQUIREMENTS.md` — `QUAL-01` row flipped `partial →
  complete`, `verified at` field carries the qualification (6/7
  unambiguous PASS, v1.1 follow-up references) for surface-level
  visibility.

## Files NOT modified (invariants held)

- All Stage 2 / Stage 3 source paths — frozen.
- `CMakeLists.txt`, `modules.json` — untouched.
- No new pluginval / auval runs (Phase 4.1's runs are current; the
  strictness-10 run is Phase 4.4's responsibility).

## Risk register

| Risk | Mitigation |
|---|---|
| V11-LOOP-FALLBACK feels wrong in real internal use | Surfaces quickly via day-to-day use; v1.1 reopens Stage 2.5 to add the "loop-entire-sample" fallback when length gate passes but variance / headroom fail. |
| Item 6 (mixed-SR) regression in v1.0 | Mitigated indirectly by items 2 and 5 PASS; explicit retest deferred to v1.1 V11-MIXED-SR-EXPLICIT. |
| QUAL-01 closure rests partly on a workaround (DSP-06 manual loop edit) | Workaround is documented in VERIFICATION.md; v1.1 V11-LOOP-FALLBACK eliminates the dependency for sustained material. |

## Next phase

Phase 4.4 — final stage gate. Triple build, cache-clear+install, then:

- `pluginval --strictness-level 10 --skip-gui-tests --random-seed 0xC0FFEE --timeout-ms 120000`
- `pluginval --strictness-level 10 --random-seed 0xC0FFEE --timeout-ms 120000` (with-GUI)
- `auval -v aumu OMtS OuDv`
- Logic AU smoke (USER)
- Dorico microtonal smoke per RQ4-4 (USER)
- 4 invariant greps (`setLatencySamples`, WebView2 flags, no `v0.1.0`, no new modules.json deps)
- Final VERIFICATION + STATUS update + atomic commit closing Stage 4.

The strictness-10 timing run is the **objective gate-of-record** for
PERF-02 (per Phase 4.2 deviation). If 4.4 strictness-10 surfaces a
timing regression, PERF-02 rolls back and Stage 2 sub-phase 2.4 / 2.5
reopens.
