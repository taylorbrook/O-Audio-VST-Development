---
plugin: O-Octagon
created: 2026-10-11
source_review: .planning/REVIEW-v1.13.1.md (four goals — 25 bugs, 7 gate defects, 17 simplifications, 16 UI, 14 features)
starting_version: 1.13.1
status: in_progress
completed:
  - "Round 1 — stereo bus first-class + silent data loss (v1.13.2, 2026-10-11)"
  - "Round 2 — presets that behave (v1.14.0, 2026-10-11)"
  - "Round 3a — clean transitions, the smoothing trio (v1.15.0, 2026-10-12)"
  - "B19 (pulled forward from Round 3b) — run flags re-derived on the dirty-check skip; air + decorr + delay (v1.15.1, 2026-10-12)"
next: "/improve-verify + /publish (Rounds 1-2 fix publicly shipped defects), then Round 3b"
supersedes: >
  The 2026-08-25 queue (last committed at 0c7154f2). Its rounds 1–3 shipped as v1.3.1–v1.3.2; its
  round 5 was partly applied (v1.3.3–v1.3.5); its rounds 6–8 and the motion milestone shipped as
  v1.4.0, v1.5.0, v1.7.0 and v1.8.0. Its round 4 (the v1.3.0 Info tail) never ran — every one of
  those 34 items was re-checked at HEAD in REVIEW-v1.13.1.md, and the live ones are folded below.
---

# O-Octagon — Improvement Queue (from the v1.13.1 review)

**One round per session, `/clear` between rounds.** IDs (B/G/S/U/F) refer to
`.planning/REVIEW-v1.13.1.md`, which holds the evidence, file:line anchors and gate couplings.

**Ordering principle (unchanged):** fix what's wrong → prove it's fixed → clean up meaning →
clean up structure → only then add features. Rounds 1–2 fix defects that shipped in the v1.13.0
public release, so they come before any polish. **Every round fixes the gate that hid its bug**,
and must show that gate FAILING against the unfixed code before it is accepted.

Baseline at queue creation (HEAD 59404a1b): render 75/0, geometry 59/0, `ui_frontend_check` 43/43,
`ui_layout_check` 34/34.

---

## ~~Round 1 — stereo bus first-class + silent data loss → PATCH v1.13.2~~ ✅ DONE (v1.13.2, 2026-10-11)

Shipped as specified, with two deliberate deviations recorded in the CHANGELOG: `venuefile::describe()` was
NOT wired (it builds English prose in C++; the page maps result codes to i18n keys instead — it stays for
Round 5's dead-code pass), and the stub's `savePreset` factory guard moved to Round 2 with B3.

```
/clear
/improve O-Octagon Round 1 of .planning/IMPROVEMENT-QUEUE.md — fix B1, B2, B4, B6, B7, B11, B12, B23 from .planning/REVIEW-v1.13.1.md plus U1 and the gate defects G1, G2, G4 and the stub half of G3. (B1) On any stereo/mono bus the header grows to ~1317 px and clips the right column on both screens, because MAP (notEightChannels) and the binaural MONITOR banner both show: suppress MAP when mapInvalidReason is notEightChannels, give .frame grid-template-columns minmax(0,1fr), .header min-width 0, title/subtitle nowrap + flex-shrink 0, and give the binaural face a calm treatment (U1) so a stereo bus shows exactly one quiet banner. (B2) venue.js buildPayload fills unedited fields from the stale `committed` cache, so a second commit inside one 500 ms poll reverts the first (trims AND label swaps): send only pending fields (C++ setVenue already falls back per field). (B4) .puck z-index above the w-cells. (B6) applyVenueEditChecked refuses every venue write on a non-8-channel bus although the binaural fold renders that venue: validate against create7point1() when the bus is not a real rig. (B7) render every venue/preset/popover/Derive/load/save result with literal i18n keys (en/fr/zh-Hans), wire venuefile::describe(), make @index coverage (v1.3.0 IN-03) and schemaVersion parsing (IN-23) strict. (B11) move #miniplan under the venue table (U3 core: numbered dots, row highlight optional) — it is crushed to ~20x25 px in the rail. (B12) settings dismiss on capture-phase pointerdown; Headphones toggles the stored preference; startPing validates its argument. (B23) empty user slots not struck through; user-select none on the plan. GATES: i18n-states BINAURAL state must be the REAL one (safeMode + notEightChannels + binaural); assert header/column/rail right edges <= 1100 there; assert a minimum mini-plan size; restate ui_frontend §22(3)/layout §13 and add a two-commits-in-one-tick section; give juce-stub.js savePreset a factory guard. Show each new assertion FAILING on the unfixed tree first.
```

Breaking-change check: none. No parameter, preset or state-format change. Venue writes on a stereo
bus go from refused to accepted, which is the fix. Verify in Logic on a **stereo** track (layout,
Load .venue, typed edits) and on a 7.1 track (no regression).

---

## ~~Round 2 — presets that behave → MINOR v1.14.0~~ ✅ DONE (v1.14.0, 2026-10-11)

Shipped as specified. Deviations, all recorded in the CHANGELOG: the preserved-set fix is a LOCAL
apply in `PresetPolicy.h` (no module opt-in — nine plugins share the header), and the v1.3.0 /
v1.13.0 migration moved into `oo::presets::migrateParameters` so both paths share it;
`getCurrentPreset` was unregistered (the page stopped calling it; 28 → 27 native functions); the
B10 counter is gated statically (§44) plus the key rule at runtime (layout §39) — the editor cannot
be built into the render harness, so the destructor drain is a hand check in a host.


```
/clear
/improve O-Octagon Round 2 of .planning/IMPROVEMENT-QUEUE.md — fix B3, B5, B10, B16 from .planning/REVIEW-v1.13.1.md plus U4 and gate defect G5. (B3/U4) User presets cannot be created: the rail is a <select> of existing names and Save hits the factory guard silently. Add a name field with Save as, Factory/User optgroups, Save disabled on factory names with a read-only hint, a rendered {ok, reason} with a distinct factoryName reason, a "— unsaved —" placeholder, and persist the current preset name in processor state (it resets to "Default" on every editor open today). (B5) outputGain is authored by presets (-1.5/-3/-2 dB) and reset to 0 dB by loads, against the suite rule that presets never touch output gain: move it to kPreserved and drop the column. AND loadPreserving lets the module reset+apply the twelve preserved params before restoring them, so srcX/w* swing through defaults mid-load: never write the preserved set (module opt-in filter or a local apply). (B10) open slider gestures only on value keys, and port O-simpleWavetable's closeOpenGestures() with a per-parameter COUNTER (loadPreset/applySceneWeights nest brackets inside a live drag). (B16) stamp a root pluginVersion attribute in getStateInformation so the next law change can migrate sessions. GATES: CP gains a zero-callbacks-on-the-preserved-set-during-load clause and moves its -3.0 dB expectation; ui_frontend §28's indexOf('presetManager.loadPreset') is -1 today (vacuous) — repoint it at loadPreserving; a layout section for Save-as with the stub's factory guard.
```

Why MINOR: preset semantics change. Factory files are rewritten without the gain column, stored
user gains are ignored, and a UI control is added. Then:

```
/clear
/improve-verify O-Octagon
/publish O-Octagon
```

Rounds 1–2 fix defects that shipped publicly in v1.13.0, so publish once they verify.

---

## ~~Round 3a — clean transitions, the smoothing trio → MINOR v1.15.0~~ ✅ DONE (v1.15.0, 2026-10-12)

Shipped with two measured deviations recorded in the CHANGELOG: the air coefficient ramps over
5 ms (one change in flight) rather than across one control block, and the decorrelator tap fade
is 128 samples @48k rather than 32–64. Probes DU/DV/DW use a worst-second-difference ratio against
a matched control, with a 2x ceiling.

```
/clear
/improve O-Octagon Round 3a of .planning/IMPROVEMENT-QUEUE.md — fix B8, B9, B13 from .planning/REVIEW-v1.13.1.md plus gate defect G6. (B8) Air filter: the exit is a hard switch (GainStage.cpp:832) and the cutoff steps every 64 samples; since v1.13.0's listener-distance law it runs over most of the plane down to 552 Hz, so Motion off / preset change / air lane to 0 / stepped position click at up to 52x (207x at 200 Hz). Add a per-sub-point airMix smoother (fL = sL + airMix*(LP(sL)-sL), literally sL when settled at 0 — bit-transparency kept) and interpolate G per sample between boundary values. (B9) Decorrelator integer taps step while depth tracks wEff below 2 m: crossfade old/new taps over 32-64 samples, one change in flight. (B13) alignment-delay edits sweep the read tap over 5 ms (plays backwards at up to -8x): two-tap crossfade over 10-20 ms. GATES: probe BB asserts equality with |H-1| on a premise v1.13.0 killed — make it a bound and add an exit-click probe with an air-0 control; re-derive BC (it models the staircase); new click probes for decorr and delay edits each with a control render. CU/DC digests must not move.
```

Each of these changes audio during transitions, so the version is MINOR. Steady state and the CU/DC
digests stay bit-identical.

## Round 3b — render-start, ping and SAFE/bypass transitions → PATCH v1.15.2

```
/clear
/improve O-Octagon Round 3b of .planning/IMPROVEMENT-QUEUE.md — fix B14, B15, B17, B20, B25 from .planning/REVIEW-v1.13.1.md. (B14) the verify ping hard-cuts programme at start and steps it back at stop/abort/retarget: duck programme by (1-env) with the ping's own raised-cosine, fade on abort, and abort when isNonRealtime(). (B15) every Logic offline bounce of a stereo track opens with a 240-sample swell from raw speaker-1/2 lanes into the fold (MonitorFold::prepare forces mix 0) — teleport the fold mix on first engage after prepare; defer prepare's motion teleport to the first process() boundary so motion renders don't start at the anchor; crossfade the SAFE<->binaural switch on the Headphones toggle. (B17) getTailLengthSeconds >= 0.35 s. (B20) clear or clock alignment lines, decorrelator rings, decorr smoothers and Air state across SAFE<->REAL and bypass (IN-04). (B25) re-base the motion phase on Free<->Sync (v1.8.0 IN-03). Each fix needs a probe that fails on the unfixed tree.
```

B19 is DONE in v1.15.1 (`deriveAirRun/DecorrEngaged/DelayEngaged()` called from both
`updateControl()` exits; probe DX). B20 builds on those helpers.

Open decision to raise during this round: **B18**. Decorrelate costs a mono source about 2.8 dB.
Level-compensating it changes existing sessions that use decorr > 0, so it is the user's call.

---

## Round 4 — UI polish + localization → MINOR v1.16.0

```
/clear
/improve O-Octagon Round 4 of .planning/IMPROVEMENT-QUEUE.md — UI items U2, U5, U6, U7, U8, U10, U11, U12, U13, U14 and bugs B21, B22 from .planning/REVIEW-v1.13.1.md, plus v1.3.0 IN-26 (tip-suppression latch), IN-28 (inputmode=decimal on label fields — §23 enforces it, rescope), IN-29 (meter ballistics per rAF frame -> per second), IN-31 (elevation label de-clash beyond pairs), IN-34 (stop meter polling on the Venue screen / while hidden). Keep the dark venue theme, Iowan Old Style + ui-monospace and the v1.13.1 AA tokens; every new string in en/fr/zh-Hans through the glossary lint and check-ui-labels widths. Measure plan readout contrast against the PAINTED canvas, not --panel (B22: v1.13.1's census could not see it).
```

Decide before this round: **U9** (footer status bar), **U15** (fold Output into Space for a taller
elevation strip) and **U16** (hover help on by default).

---

## Round 5 — simplification, code → PATCH

```
/clear
/improve O-Octagon Round 5 of .planning/IMPROVEMENT-QUEUE.md — behaviour-preserving simplifications S2-S17 from .planning/REVIEW-v1.13.1.md (C++ first, then JS/CSS). Move every coupled gate in the SAME commit (§2, §8/§9, §19, §21, §22, §26, §29 as listed per item). Render and geometry goldens must stay byte-identical; layout §1 (zero console errors) is the extraction proof. Do NOT do MEDIUM-07; LOW-06 only as §6 hardening.
```

## Round 6 — comments only → PATCH

```
/clear
/improve O-Octagon Round 6 of .planning/IMPROVEMENT-QUEUE.md — S1 from .planning/REVIEW-v1.13.1.md as a COMMENT-ONLY pass: delete counts a gate owns (point at the gate), move version narration and measurement logs to CHANGELOG/NOTES, keep one copy of each incident story, keep every invariant listed under S1's "must stay", move i18n.js's review log to NOTES. Beware §30 (.reset ( counted in raw text), §17 (setSize (), §9 (url == "). No code token may change — diff with comments stripped must be empty.
```

---

## Round 7+ — features, one `/improve` each, in this order

| Order | Command | Version |
|---|---|---|
| 1 | `/improve O-Octagon F1 from .planning/REVIEW-v1.13.1.md — lock Free-rate motion phase to the host timeline` | MINOR |
| 2 | `/improve O-Octagon F2 from .planning/REVIEW-v1.13.1.md — time-of-flight in the headphone fold (centroid first; seat later)` | MINOR |
| 3 | `/improve O-Octagon F3 from .planning/REVIEW-v1.13.1.md — one Monitor switch for every instance` | MINOR |
| 4 | `/improve O-Octagon F4 from .planning/REVIEW-v1.13.1.md — scene glide` | MINOR |
| 5 | `/improve O-Octagon F5 from .planning/REVIEW-v1.13.1.md — feed solo + speaker out-of-service` | MINOR |
| 6 | F6 setup checklist, F7 width axis | MINOR each |

Milestones (`/improve-milestone`), in rough priority order: F8 Scene Morph, F9 OSC / ADM-OSC, F10
Perimeter/Drawn paths, F11 modulation lane, F12 multi-instance overview, F13 Acoustic Measure, F14
HRTF + head tracking.

**Feature rules:** new parameters take version hint 4 (`kHintV1_14`) and exactly one of
`kPreserved` / `kAuthored`. Never append to `motionPath` / `motionSync`. CU / DC must hold at
defaults. Every string ships in three languages.
