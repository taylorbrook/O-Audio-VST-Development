# Stage 4: Polish / Validation - Context

## Discussion Summary

**Date:** 2026-11-07
**Participants:** Taylor, Claude
**Inputs:** ROADMAP.md §Stage 4, REQUIREMENTS.md (FUNC-09, UI-04, COMPAT-02, COMPAT-03, QUAL-04 pending), VERIFICATION-3.3.md (carried items), VERIFICATION-2.6.md (decisions a, c, d), VERIFICATION-2.4.md (head-stop accepted).

## Requirements Confirmed

- **FUNC-09: factory presets.** 16 presets, 4 in each category: ambient, harmoniser (scale_root + pitch_follow), rhythmic (Tempo sync), feedback-cascade. They go through the existing preset-manager wiring (`setExcludedParameters({"output_gain"})` and `setValidatedApply(true)` are already in `PluginProcessor.cpp:184-187`). Presets never set `output_gain`, and apply resets to defaults first. Set the normalised values with skew taken into account (memory: factory presets ignore skew).
- **COMPAT-02: state round-trip on a fresh instance.** Covers every parameter, all 16 mod routings and the int64 seed, and runs through both the session codec and the preset files. Cover the second-reopen case too (the stale `<CustomState>` child pattern) and the bool-as-string ValueTree round-trip.
- **COMPAT-03: I/O layouts plus Windows.** Gate the mono→mono, mono→stereo and stereo→stereo layouts. Release CI proves the Windows VST3 build; there is no Windows hands-on in this stage.
- **UI-04: EN/FR/zh-Hans.** Gates only. Lint, fit and labels must be green at the widest language, which they already are as of 3.3. All 301 FR strings and all zh strings stay `reviewed: false`, matching the suite norm. A human review is deferred to after 1.0.0.
- **QUAL-04: hands-on DAW pass.** Logic AU, plus one macOS VST3 host (Reaper or Ableton), covering the four preset use cases and the restaged AGC (D43). The freeze A/B (head-stop vs virtual head) is **already closed**: head-stop was accepted at 2.4 (VERIFICATION-2.4 deviation 11, listening item 1 PASS). No A/B is needed. Re-listen only to confirm.
- **Release:** pluginval strictness 10 on VST3 and AU, auval, and a CHANGELOG `## [1.0.0]` heading in bracket form. Stage 4 ends at `/install-plugin`. A public 1.0.0 through `/publish` (tag `O-Granulator-v1.0.0`) is a **separate, later step**. Do not tag in Stage 4.

## Constraints Identified

- **D43 is the only DSP change in Stage 4.** Every other item must leave the audio bitwise-unchanged against the 3.3 baseline. Take a new baseline (D) **after** D43 lands, and gate every later item against D.
- Carried rows move under D43. Re-baseline them deliberately and record the old and new values. Do not loosen thresholds (W2o rule: widen the seeds, never the threshold).
- Release CI configures only the tagged plugin. The Windows build runs at `/publish`, not in Stage 4. Optionally, run a local CI-equivalent configure check if one exists.
- Trunk-based, path-scoped commits: `git commit -- plugins/O-Granulator PLUGINS.md`.

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D42 | Preset bank | 16 presets (4 × ambient / harmoniser / rhythmic / feedback-cascade). Each preset gets a description, never sets output_gain, and resets to defaults before it applies | Shows the range of each use case without a long curation pass |
| D43 | AGC / saturator staging (3.3 finding, option c) | **Restage and gate.** Measure the loop at the AGC tap, then move `kAgcCeil` (or the AGC's position relative to the saturator) so the AGC engages at the default `fb_saturation` 20 %. Gate it with a runaway / dB-per-generation row at defaults, plus a negative control on the old staging. The loop-gain readout must then show AGC action at defaults | Today g_agc ≡ 1 for any saturation above ≈ 6.1 %, so the AGC is dead code at defaults |
| D44 | Release target | Public 1.0.0 through `/publish`, after Stage 4. Windows proven by CI | COMPAT-03 is a "should". CI already builds Windows |
| D45 | i18n | Gates only. Strings stay `reviewed: false` | Suite norm. Taylor can review FR later |
| D46 | VST3 host pass | One macOS VST3 host besides Logic AU | 3.3 deferred this pass until COMPAT-03 called for it |
| D47 | Carried housekeeping (Claude default, all in) | (a) Env 1e30 pin: add a `\|x\| ≤ kAnalysisTapMax` predicate at the env input in `ModSources.h` (bitwise-neutral up to +80 dBFS). (b) Make `eg22b::renderPool` event-aware so Z6a's freeze arm is real. (c) C7 machine-speed calibration. (d) Write down the W2o rule in the harness notes. (e) Reset `uiSnap` in `prepare` (removes the one stale frame) | Every carried item closes before 1.0.0. (a), (b), (d) and (e) are audio-neutral |

## Open Questions (for research)

- D43: where to put the AGC. Options: (1) lower `kAgcCeil` below the saturator ceiling 1/d at default d, (2) move the AGC before the saturator, (3) make the ceiling track saturation. Measure each against the feedback-cascade presets and the runaway gates (FB*, coherent tap law, 0.95 cap). Must hold: g_agc < 1 is reachable at defaults, there is no audible pumping at moderate feedback, and the ∞-tail / freeze behaviour from DAW-2.6 is unchanged.
- Which preset path to gate: the factory bank sentinel can make preset gates read a stale INSTALLED bank (memory). Gate against the bank in the build tree.
- Preset category UI: does the preset-manager module support categories/folders, or do the presets use a name prefix?
- Whether a local CI-equivalent Windows configure check exists that runs without tagging.

## Next Phase

Ready for: research phase (`/plugin-research O-Granulator 4-polish`)
