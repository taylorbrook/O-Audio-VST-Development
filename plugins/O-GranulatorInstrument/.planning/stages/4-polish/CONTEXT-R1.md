# Stage 4: Polish / Validation - Context, Rework R1 (per-stream feedback + reverb)

## Discussion Summary

**Date:** 2026-11-11
**Participants:** User, Claude

Second discuss pass for Stage 4. The first pass (`CONTEXT.md`, D51–D59) was planned, executed and verified PARTIAL. The D51 hands-on session then produced two findings (`VERIFICATION.md` Addendum, `HANDS-ON-SESSION-4.md` triage F1 / F2). F2 is a scope change decided by the user: feedback and reverb become per-stream, before 1.0.0. This document scopes that rework. D51–D59 stand unless amended below.

Already decided before this pass (VERIFICATION Addendum): lands before 1.0.0; the 8 global parameters are removed (no master Amount, no output reverb); Freeze and Reset All stay global; nothing is published, so no state migration and the append-only rule may be broken once.

## Requirements Confirmed

- **Per-stream feedback:** each stream gets `Amount`, `Low Cut`, `High Cut`, `Saturate`, `Diffuse` (ranges, defaults and skews of today's globals) plus an on / off toggle.
- **Per-stream reverb:** each stream gets `Mix`, `Size`, `Damp` (today's ranges and defaults) plus a Pre / Post switch that places the reverb inside or outside that stream's feedback loop.
- **Parameter count 261 → 293:** −8 globals, +32 per-stream floats, +4 feedback toggles, +4 reverb Pre / Post switches.
- **Modulation destinations 126 → 150:** the 8 global entries go, 32 per-stream entries arrive. The toggles and switches are not destinations.
- **FX · Feedback page** carries all of it, on the same tab, in the 1200 × 800 frame.
- **Factory bank** (64 presets) rewritten to the new parameters.
- **Record-in Arm** returns to Off after a take commits or is discarded.
- **F1** (Record-in "Select a sidechain input" copy, already in source, uncommitted, not built) ships with this rework: build, install, by-eye retest, CHANGELOG line.
- VERSION stays 1.0.0; the CHANGELOG `## [1.0.0]` entry is amended, not a new version.

## Constraints Identified

- Frame is fixed at 1200 × 800, not resizable (`PluginEditor.cpp:1190`). Stream columns are 294 px pitch / 282 px wide; knob pitch 72 px, so 4 knobs per row with no figure.
- Feedback 0 must remain a bitwise no-op on the working copy, and renders with feedback 0 and reverb 0 should stay byte-identical to today's goldens. The toggle Off must equal Amount 0 for the tape (no writes).
- DSP-04 boundedness (convex write, chain AGC 0.7) must hold in both reverb positions, including Pre with reverb Mix 100 %.
- CPU budgets unchanged: worst case ≤ 40 % (C25m), typical factory preset ≤ 12 % (`--cpu4`), now with four reverb tanks instead of one.
- These destinations are per stream, not per voice: D4 (newest voice, "mono" badge) still applies to routes from per-voice sources.
- The engine and listeners never write APVTS from the audio thread. Auto-clearing `rec_arm` is a parameter write and must be done from the message thread, and reach the host (memory: a nested write inside `parameterChanged` is swallowed by VST3).
- The contract checksums in STATUS.md (`parameter_spec`, `parameter_spec_body`, `requirements` if touched) re-pin; `params.tsv`, the param-dump golden and the destination golden re-pin.
- Hands-on rows 2.4-1 – 2.4-7, 2.5a-7, 2.5a-11, 3.2-10 – 3.2-12 and the Pass 2 audition are provisional; the sheet is regenerated at the end of the rework.
- Trunk-based: all work on `main`, path-scoped commits (`plugins/O-GranulatorInstrument`, `PLUGINS.md`).

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D60 | Scope and timing | **Per-stream feedback + reverb before 1.0.0; globals removed; Freeze and Reset All stay global.** | User decision at hands-on (F2). Nothing published, so no migration. |
| D61 | Page layout | **Same tab, 1200 × 800, compressed.** The three "fig." cells per column (filter, drive, ring) are dropped and the row pitch tightens from 111 px to about 88 px, giving five rows per column: Filter, Drive / Crush, Ring, feedback ×4, feedback / reverb ×4. Freeze and Reset All sit in a global strip below. | User choice over a fifth tab or a taller frame. Amends D40 (v1 locked) for this page only. |
| D62 | Reverb position | **Per-stream Pre / Post switch.** Post = outside the loop (today's behaviour, tails not written to tape). Pre = the stream's reverb sits before the feedback tap, so tails are written into the working copy. Default Post. | User choice. Default Post keeps existing feedback renders unchanged. |
| D63 | Feedback toggle | **Per-stream on / off (Bool), default On.** Off stops that stream's writes without losing its Amount. | User choice. Default On means Amount alone behaves as before. |
| D64 | Factory presets | **Copy + showcase set.** Each preset's old global values are copied to all four streams, toggles On, reverb Post. Then about 8–12 presets are re-voiced to show per-stream contrasts (dry stream beside a washed one, one stream feeding back, a Pre-reverb smear). | User choice. Copy keeps 52+ presets sounding as verified; the showcase set demonstrates the feature. |
| D65 | Layout approval | **v2 mockup first.** A v2 browser mockup of the FX · Feedback page only is produced and approved by the user before execute edits `index.html`. Other pages stay at v1. | User choice. A rejected layout after a build costs a full cycle. |
| D66 | Record-in Arm | **Auto-clear.** `rec_arm` returns to Off when a take commits or is discarded, so one click starts the next take and the lane Record button always works. Amends D34 (one-shot). | User choice; found while tracing F1. |
| D67 | Destination numbering | **In each stream block.** Every stream's 8 new destinations follow its existing ones (block 26 → 34 entries); global entries after the stream blocks renumber. The whole destination golden and all preset route indices re-pin once, then the table is append-only again. | User choice. Clean table and MOD menu order; the one allowed break is used fully. |
| D68 | Parameter IDs (default, not asked) | `sN_feedback`, `sN_fb_on`, `sN_fb_low_cut`, `sN_fb_high_cut`, `sN_fb_saturation`, `sN_fb_diffusion`, `sN_reverb_mix`, `sN_reverb_size`, `sN_reverb_damp`, `sN_reverb_pre`. | Follows the existing `sN_` convention and the old global names. Research may adjust. |
| D69 | i18n (default, not asked) | FR + zh-Hans for every new or changed string through the suite lint gates; `tab.fx` keeps "FX · Feedback". | Suite rule (D59 carries). |

## Open Questions (for research)

1. **Pre-mode signal definition.** The feedback tap is an amp-weighted mean of the voice clouds (ARCHITECTURE §16), the audible path is the stream sum. With one tank per stream, what exactly feeds the tap in Pre mode (wet of the stream sum renormalised, or the tank fed by the tap signal)? Does Pre need a second tank per stream, and what does that do to CPU?
2. **Boundedness in Pre mode.** Does the chain AGC (0.7) plus the convex write still bound the tape with reverb Mix 100 %, Size 100 % in the loop? Which DSP-04 gates need a Pre arm?
3. **Switching without clicks.** Pre ↔ Post and toggle On ↔ Off while sounding: ramp lengths, tank state on a switch, behaviour under Freeze.
4. **CPU of four tanks.** Measure C25m and `--cpu4` with four `ReverbUnit` instances. Can a tank at Mix 0 be skipped (and its tail handled) to keep the 12 % typical budget? Does `reverb-unit` 1.2.0 atomic bind support four instances as is?
5. **Reverb and mute / solo / enable ramps.** Does a muted stream's tail ring out or cut? Where does the wet safety knee sit now that the reverb is before the stream sum?
6. **Byte-identity.** Which existing render goldens stay identical (feedback 0, reverb 0) and which re-pin? Is "four identical Post tanks" close enough to "one output tank" for the copied presets to pass FP rows unchanged, or do FP0–FP6 re-pin?
7. **Layout detail for the v2 mockup.** Placement of the toggle and the Pre / Post switch in a 282 px column; row grouping (feedback ×4 / Diffuse + reverb ×3, or a different split); section headings at 88 px pitch; value-entry and tooltip clamp gates at the new pitch; where Freeze / Reset All and the old `reverb_view` decay figure go (dropped with the other figures?).
8. **Gate fallout of dropping figures.** `data-display` readers, `drawReverbFigure` and the filter / drive / ring figure code, i18n states (47), check-ui-labels, boot-all-uis, the closure / binding proofs (261 relays → 293).
9. **`rec_arm` auto-clear mechanics.** Message-thread write path that the host sees in VST3 and AU; interaction with `rec_arm` being excluded from presets, with Threshold / Next Note triggers, and with automation of `rec_arm`.
10. **Preset tooling.** Is the 64-preset bank generated from specs (so the copy is a script change) and which presets set `feedback` (10) / `reverb_mix` (59); candidates for the showcase set; bank-hash stamp re-pin.
11. **Mockup tooling trap.** A v2 mockup `CMakeLists.txt` header that mentions `juce_add_plugin(` in a comment breaks `resolve-target.sh` (memory); keep v2 to HTML + i18n only.

## Out of Scope (R1)

- A master feedback Amount, an output reverb, or a link-all-streams control.
- Changes to the Streams, Mod and Voice pages beyond what the new destinations require in the MOD menu.
- Windows hands-on, Reaper / Dorico / MPE passes (D54, D57 unchanged).
- Version bump or tag (D56: tagging only via `/publish`).

## Next Phase

Ready for: research phase (`RESEARCH-R1.md`), then plan with the v2 mockup approval gate (D65) ahead of execute.
