# Stage 3: GUI — Phase 3.3 (MOD page, voice section, meters) - Context

## Discussion Summary

**Date:** 2026-11-10
**Participants:** User, Claude
**Phase goal (ROADMAP):** UI-02. Instrument sources on the MOD page, drag-to-modulate, "mono" badges for per-voice → global routes, mod rings driven by the newest voice, the voice / amp section, the voice-activity strip, output and sidechain meters (max since read), and the `grain_limit` heavy hint.

## Starting state (observed)

- Phase 3.2 verified ⚠️ PARTIAL (`VERIFICATION-3.2.md`, `1a3cbcf9`). Every automated gate is green; UI-01 is partial. HANDS-ON-3.1 (16 rows) and HANDS-ON-3.2 (27 rows) have not been run.
- **The page is already drawn and has its readers.** `index.html` (== `mockups/v1-ui.html`) has:
  - all 16 `mrow`s with `mono` badges (`mono-on` toggled from per-voice source × global destination);
  - the source chips (`attachSourceDrag` → `addRoute`, with `toast.duplicate` / `toast.routed` / `toast.routedMono` / `toast.matrixFull`);
  - Alt-drag depth (`bindAltDepthDrag`), mod rings (`renderModRing` / `placeRingDot`);
  - the `vcell.newest` voice cells, the `#voice-ind` dots, the Env 1 / 2 scopes and read-outs, the LFO phase views, `#strip.frozen`;
  - the output and Record-in meters, and `#heavy` (static: `grain_limit` index > 2, i.e. > 128 grains, S1).
- **Telemetry the page reads that is still a placeholder** (`GiUiJson.h` 3.2 shape, "unchanged until 3.3"):
  - `newest: -1`, `menv: []`, `sources: []`, `modNorm: {}`;
  - `out: [0, 0]`, `limit: 0`, voice row column 8 `wantSt` = 0;
  - `lfoPh`, `frozen` and `modRouted` are omitted.
- **Engine side already present:**
  - `Engine::getDestTelemetry (d)` (`destTel[]`) and `getLfoPhaseTelemetry (n)`;
  - the newest-voice rule (`newestW`, `newestSer`, 2.5b);
  - `GiTelemetry::takeOutPeak()`, a **single scalar** max since read, while the page expects `[L, R]`. Research resolves this.
- The parent O-Granulator fills `modNorm` for routed destinations only (`modRouted`), and fills `lfoPhase` in its processor telemetry (`PluginProcessor.cpp:697–739`). This is the precedent for the port.

## Requirements Confirmed

- **UI-02** closes in this phase:
  - Drag-to-modulate works from every source chip, onto every knob and loop handle that has a `data-dest`.
  - The MOD page shows the 17 instrument sources.
  - Rings move for per-voice routes, driven by the newest voice.
  - The mono badge shows on per-voice → global routes.
- The voice strip reflects notes, steals and releases. The newest-voice marker follows the engine's newest-voice rule, not a page guess.
- Meters are max since read. They match known sine levels (harness / probe rows with stated dBFS and tolerance). The output meter and the sidechain (Record-in) meter are both live.
- The `grain_limit` heavy hint shows as v1 draws it.
- All values come through the existing ~30 Hz `getTelemetry` pull and the one processor-side `buildUiTelemetry` (sole reader of each consuming feed, PD13 / 3.2). No `juce::Timer` push, no `emitEvent`.
- 0 audio-thread allocations (UAL1 carries). Stage 2 audio must not move:
  - harness 402 / 0 carried rows, parity 168 / 0 + GV 2 / 0;
  - renders-2.4 + legacy byte-identical, param-dump identical;
  - fork drift 0 / 13, X2 0.
- The five UI gates stay green on the real tree. New live `[data-i18n]` nodes get states in `tests/i18n-states.json` (D42 carried). `cmp` `index.html` == `v1-ui.html` and `js/i18n.js` == `v1-i18n.js`.

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D47 | Hands-on scheduling | **Everything runs once at 3.3 verify:** HANDS-ON-2.1–2.6 (79 rows, D39), HANDS-ON-3.1 (16), HANDS-ON-3.2 (27), HANDS-ON-3.3 (new), and the Logic AU pass (sidechain routing, Record-in; ROADMAP 3.3 criterion). 3.3 execute is **not** gated on 3.1 / 3.2. 3.1 and 3.2 stay verify-partial until that session. A ❌ in any row routes to `/plugin-execute O-GranulatorInstrument 3-gui` for the owning phase's code. | One human session on the finished UI. Accepted risk: a lane / host-plumbing defect found late may need a fix under 3.3's code. |
| D48 | Design latitude | **D40 carries: v1 locked, defects only.** 3.3 wires what v1 already draws (chips, rings, mono badges, voice cells, Env scopes, LFO views, meters, heavy hint). No new controls, no visual polish beyond defects. | The page and its readers exist. The work is telemetry and proof, not design. |
| D49 | 3.2 carry-overs | **Fix all three in 3.3:** (a) rewrite or remove the vacuous i18n states `rec-wait-level`, `rec-no-sidechain` and `rec-recording` so that each state holds against the live poll; (b) fix the CM6 timing flake by making take names collision-proof within one second, or by fixing the row (research decides); (c) correct UPM6b's printed note to D45 wording. | They are small, and (a) and (b) weaken gates that 3.3 relies on. |
| D50 | Carried, not 3.3 | fr `vstate.held` "tenue" → decided at HANDS-ON-3.2 row 26. `WavFailed` silent after a take, one toast for several failing lanes, a save worker joined without timeout on a hung disk, the `output_gain` default of 7.15e-07 dB, the Windows drop check → Stage 4. | Scope stays UI-02. |

## Constraints Identified

- `buildUiTelemetry` stays the **only** reader of consuming feeds. `takeOutPeak` joins the frame and `takeScPeak` there. The second reader that a `getTelemetry` pull plus a harness or probe read would create is the trap (one-reader rule, PLAN-3.2 PD13).
- "Max since read" means the page's ~30 Hz pull defines the window. If the editor is closed, nothing reads, and the next open shows the max over the closed period (same as 3.2's SC peak). Page ballistics (the 0.88 / 0.94 decay) stay as v1 has them (D48).
- Rings: show the range for routed destinations only (the parent's `modRouted` gate). The live dot for a per-voice source follows the newest voice. With no voice sounding, the dot sits at the base value. Global sources move the dot without notes.
- Voice strip: 20 rows; voice state / steal / release come from the engine. A steal must be distinguishable for long enough to see at 30 Hz. Research decides between a sticky flag and a hold, as with `GiRecRunHold`.
- No listener writes another parameter. Drag-to-modulate writes `modN_source` / `_dest` / `_depth` from a gesture, as 3.1 already does.
- Trunk-based, path-scoped commits (`plugins/O-GranulatorInstrument`). The foreign working-tree changes (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, O-GrainScatter) are not touched.

## Open Questions (for research)

1. **Output meter shape:** `takeOutPeak()` is one scalar, while the page draws `[L, R]`. Should it be split into two atomics, or should the one value be sent twice (as `rec.peak` does today)? Which choice keeps the audio path byte-identical?
2. **`sources[17]`:** which value per per-voice source: the newest voice's live value, or the "from" value of the newest-voice rule (`newestW` ramp)? Which units and polarity does the page expect (`BIPOLAR_SOURCES`)?
3. **`menv`:** the per-voice Env 1 / 2 values for the scope dots. What shape does the page read (`[voice][2]`, or the newest voice only)? Is there a cost at 20 voices?
4. **`modNorm` / `modRouted`:** port the parent's routed-only fill. Is `destTel` already the post-modulation normalised value in the shape that `placeRingDot` wants (0…1, destination-normalised)?
5. **`wantSt` and steal / release visibility:** what the engine exposes, and the hold length.
6. **`limit`:** the effective grain limit (after onset thinning / the D3 cap) or the parameter value? The heavy hint stays parameter-driven (S1) unless research finds that v1 meant live.
7. **`frozen` and `lfoPh`:** the source per key (Freeze state; `getLfoPhaseTelemetry`).
8. **Sine-level meter gate:** the stimulus (for example a full-scale sine and −6 / −20 dBFS through a known voice gain, and an SC sine), the tolerance, and where it lives (harness U-rows + probe pixel rows).
9. **CM6 fix:** a sub-second or sequence suffix in `GI-Rec-<stamp>-S<n>.wav` vs. a row-only fix; existing `rec:` paths must stay resolvable.
10. **Proof plan:** ports of the parent's MOD / ring / meter rows (harness + overrides + closure + probe), NCs per new key, and the new i18n states for the voice strip / MOD read-outs.

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 3-gui`).
