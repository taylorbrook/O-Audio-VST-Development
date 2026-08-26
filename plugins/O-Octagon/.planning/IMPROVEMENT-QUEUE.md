---
plugin: O-Octagon
created: 2026-08-25
source_reviews:
  - CODE_REVIEW.md (1 Critical, 5 Warnings, 34 Info)
  - .planning/SIMPLIFICATION-AUDIT.md (1 HIGH, 7 MEDIUM, 8 LOW)
  - .planning/FEATURE-REVIEW.md (10 ranked gaps)
starting_version: 1.3.0
current_version: 1.3.2
status: in_progress
completed:
  - "Round 1 — CR-01 meter displacement + rendered-geometry gate (v1.3.1)"
  - "Round 2 — WR-01..WR-05 (v1.3.2)"
  - "Round 3 — /improve-verify PASS (CODE_REVIEW.md verified: 2026-08-26)"
next: "Round 4 — Info tail"
---

> **Line numbers in `SIMPLIFICATION-AUDIT.md` were captured against v1.3.0.** Rounds 1–2 edited
> `styles.css`, `app.js`, `venue.js`, `roomplan.js`, `PluginProcessor.cpp` and `VenueModel.cpp`, so
> two anchors have drifted. **Locate every candidate by content, not by line.** Verified 2026-08-26:
>
> | Candidate | Audit says | Actually at |
> |---|---|---|
> | `LOW` `.vfield-label { text-transform: none; }` | `styles.css:1014` | **`styles.css:1028`** |
> | `LOW` `writeToState` + `++scenesGeneration` pair | `PluginProcessor.cpp:643` | **`:682`, `:803`, `:918`** |
>
> All other anchors re-resolved correctly, including `HIGH-01` (`venue.js:270`) and `MEDIUM-04`
> (`PluginEditor.cpp:1176`).

# O-Octagon — Improvement Queue

Ordered execution plan for everything the 2026-08-25 three-level review turned up.

**Run one round per session, with `/clear` between rounds.** Each round is sized to fit one context
comfortably. Rounds 1–5 are correctness and hygiene on a shipped v1.3.0; rounds 6+ are new
capability.

**Ordering principle:** fix what's wrong → prove it's fixed → clean up meaning → clean up structure
→ only then add features. Never build new capability on a base with a known audio-thread race or a
test gate that certifies bugs.

---

## ~~Round 1 — CR-01: the meter displacement~~ ✅ DONE (v1.3.1)

Split out from the general review sweep because it is the only finding that breaks a shipped safety
feature on every platform, **and** because its fix must include a test-gate upgrade that a
mechanical finding-sweeper will not infer.

```
/improve O-Octagon Fix CR-01 from CODE_REVIEW.md. All eight meter arcs render 507 px off their speaker glyphs in BOTH Chromium/WebView2 and WebKit/WKWebView: .meter-arc (styles.css:410-418) and .meter-peak (styles.css:427-432) set transform-origin:center with no transform-box, and the SVG default transform-box is view-box, so the origin resolves to the viewBox centre in the element's glyph-translated local space. Set transform-origin: 0 0 on BOTH rules (simplest uniform fix, needs no transform-box; do NOT use fill-box on .meter-peak, whose own fill-box centre is (0,-15) and would spin in place instead of sweeping). THEN upgrade tests/ui_layout_check.js section 23, titled "eight meter arcs, at their glyph positions", to assert RENDERED geometry via getBoundingClientRect/getScreenCTM: today it reads only DOM parentage plus the r and stroke-dasharray ATTRIBUTES and passes identically with the arcs 507 px off-glyph. Before accepting the upgraded gate, revert the CSS and confirm the gate FAILS — a probe that passes both ways is decoration.
```

Version: **PATCH → v1.3.1**. Verify by eye in Standalone on the Room screen, both DPI settings.

---

## ~~Round 2 — the remaining Critical/Warning tier~~ ✅ DONE (v1.3.2)

```
/clear
/improve-review O-Octagon
```

Sweeps `WR-01`–`WR-05`. Two things to steer while it runs:

- **WR-01 (snapshot race) — take the one-liner, not the seqlock.** Verification established that
  `setStateInformation` is the *only* reachable double-publish path today; the reviewer's "speaker
  drag at mouse-move rate" claim was wrong (`venue.js` commits on blur/Enter, and every UI entry
  point routes through `applyVenueEdit`, which publishes exactly once). Give `readVenueFromState()`
  a `publish=false` variant when a `rebuildChannelMap()` follows — that closes the reachable path.
  The seqlock / 3-slot buffer is defence-in-depth; file it as optional hardening rather than doing
  it here.
- **WR-03 (`readFloat` accepts NaN/Inf)** is the one with sticky consequences — a hand-edited
  `.venue` carrying `x="nan"` persists the NaN back into saved sessions and re-saved files. Fix the
  ingestion, not just the read.

Version: **PATCH → v1.3.2**.

---

## ~~Round 3 — prove rounds 1–2 actually hold~~ ✅ DONE (PASS 2026-08-26)

```
/clear
/improve-verify O-Octagon
```

Gate before touching anything else. Expect: unit 45/45, render-harness 50/50 byte-identical (none
of rounds 1–2 touches DSP arithmetic), `ui_frontend_check` 42, `ui_layout_check` 28 **+ the new
rendered-geometry assertion**, `auval` PASS, `pluginval --strictness-level 10` SUCCESS.

If the render goldens move at all, stop — rounds 1–2 should be golden-neutral, so drift means
something unintended reached the audio path.

---

## ▶ Round 4 — the Info tail  ← YOU ARE HERE

```
/clear
/improve-review-info O-Octagon
```

34 findings, all low-risk. Two deserve attention rather than bulk approval:

- **IN-20** — the `PresetPolicy` doc table still shows the pre-v1.3.0 blur column. Anyone
  "reconciling" the rows to match that table would silently triple every factory preset's blur.
  Fix the table, don't fix the rows.
- **IN-16 / IN-25 / IN-27 / IN-30** carry a ⚠️ premise-disproved banner: the hidden-editor
  completion drop they assume cannot fire in this plugin. Only the wasted-work half of each is
  real. Don't let the sweeper "fix" a hazard that doesn't exist.

Version: **PATCH → v1.3.3**.

---

## Round 5 — simplification

Phase 1 (the single LOW-risk HIGH item) goes through `/improve`; Phase 2 is **empty** for this
plugin, so skip straight to Phase 3.

```
/clear
/improve O-Octagon Apply HIGH-01 from .planning/SIMPLIFICATION-AUDIT.md: venue.js drawMini() re-implements roomplan.js's hull points-string construction and three glyph class toggles verbatim. Extract the shared renderer into roomplan.js (ui_frontend_check section 32 bans the reverse direction). Scope the extraction to hull + transform + class toggles only — leave the v1.1.0 output-badge rendering in roomplan, since the mini plan has no DOM nodes for it. DOM output must stay byte-identical.
```

```
/clear
/simplify-phase3 O-Octagon
```

15 items (7 MEDIUM + 8 LOW). **Pull `MEDIUM-04` to the front of the batch** — it is not cosmetic:
`getFieldGrid`'s `readParam` fallback for `blur` still reads `0.1f` while the live default moved to
`0.03f` in v1.3.0. Deriving the fallback from the live parameter removes the whole drift class.

Versions: HIGH-01 **PATCH → v1.3.4**, then phase 3 **PATCH → v1.3.5** (each command bumps once).

---

## Round 6 — per-speaker delay compensation

First feature, because it is the highest-value item that fits a single improve cycle and it
completes a story the venue model already half-tells (positions are measured; only level is
compensated).

```
/clear
/improve O-Octagon Add per-speaker delay compensation, from the HIGH/small gap in .planning/FEATURE-REVIEW.md. A venue-scoped delay per speaker (0-50 ms, enterable as metres) sitting beside the existing per-speaker trims, applied post-solve. Auto-derive a suggested value from the measured speaker distances to a chosen reference point, with manual override. Extends the .venue file schema — bump schemaVersion and keep the loader backward-compatible with unstamped files. 8 delay lines, 8 new venue values.
```

Version: **MINOR → v1.4.0**. Breaking-change check: this adds venue fields, so confirm old `.venue`
files and old sessions still load before shipping.

---

## Round 7 — mono decorrelator behind Width

Small DSP add that makes an existing headline control deliver on mono material — the v1.3.0
changelog already names this as Width's known limitation.

```
/clear
/improve O-Octagon Add an optional mono decorrelator behind the Width control (MEDIUM/small gap in .planning/FEATURE-REVIEW.md). Today width separates two IDENTICAL sub-point feeds in space, so on mono material — which most fixed-media stems effectively are — the two feeds mostly comb instead of widening. Add an all-pass or velvet-noise decorrelation of the two sub-point feeds, defeatable, defaulting OFF so existing sessions and render goldens are bit-unchanged.
```

Version: **MINOR → v1.5.0**. Must be default-OFF or every render golden moves.

---

## Round 8 — binaural / stereo monitoring fold-down

The highest-value gap overall, but medium effort and it needs a design decision about how the fold
is written, so give it its own session.

```
/clear
/improve O-Octagon Add a binaural/stereo monitoring fold-down (HIGH/medium gap in .planning/FEATURE-REVIEW.md). The brief's own use case -- preparing and revising a piece away from the venue -- is currently inaudible, and every competitor (SpatGRIS BINAURAL, L-ISA Studio, SPAT Revolution, dearVR) treats headphone monitoring as table stakes. Fold the 8 solved feeds to 2: position-derived stereo pan + inter-aural delay + distance gain at minimum, HRTF convolution as a stretch. Write into 2 of the 8 carrier channels with the rest muted, behind a loud MONITOR banner so it can never leak into a bounce unnoticed. Treat "must not silently contaminate a render" as the primary design constraint.
```

Version: **MINOR → v1.6.0**.

---

## Round 9+ — the large items (milestones, not improvements)

These are Tier 3: multi-session, research-heavy, and they multiply the test matrix. They belong in
`/improve-milestone`, which runs discuss → research → plan → execute → verify with context clearing
between phases — not in a single `/improve`.

```
/clear
/improve-milestone O-Octagon Motion engine: generative trajectories for srcX/srcY/srcZ — circular and elliptical orbits, figure-8s, line sweeps, random walks, tempo-synced rate/depth/phase — written as parameter modulation inside the plugin rather than hand-drawn host automation. O-Orbit in this repo already owns the path vocabulary to borrow. Biggest creative-capability gap; ControlGRIS, L-ISA and even ReaSurroundPan all make motion cheap.
```

Then, in rough priority order, each as its own milestone:

| Item | Value / effort | Note |
|---|---|---|
| OSC input for live diffusion | medium / medium | Turns a fixed-media renderer into a performance instrument. `juce_osc` makes transport trivial; care needed around Logic's plugin threading. |
| Snapshot morphing / cue list | medium / medium | The scene infrastructure is already ~70% of the machinery. |
| Higher channel counts (Reaper/Nuendo discrete-N) | medium / large | **Approach last and carefully.** Widens the niche most, but multiplies the channel-map test matrix — precisely where R1 (silent wrong map) lives. |
| Venue interchange import (SpatGRIS / L-ISA configs) | low / small | Cheap adoption lever whenever convenient. |
| Multiple simultaneous sources | low / large | Per-instance model already matches Logic's track paradigm. |
| VBAP A/B mode, hull-distance reverb | low / large | Brief deferrals. VBAP A/B arguably contradicts the plugin's own thesis. |

---

## After round 5

O-Octagon is still a dev-branded build (`O-Octagon-dev`, v1.3.0-dev in PLUGINS.md) and has never
been released. Once rounds 1–5 land and `/improve-verify` is green, it is a genuine release
candidate:

```
/clear
/publish O-Octagon
```

Worth doing before the feature rounds, so there is a shipped, tagged baseline to regress against.
