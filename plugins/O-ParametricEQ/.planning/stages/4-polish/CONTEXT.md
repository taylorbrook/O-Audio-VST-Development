# Stage 4: Polish and validation - Context

## Discussion Summary

**Date:** 2026-11-16
**Participants:** Taylor, Claude
**Starting point:** 0.9.0 at `fde885d7`. Stage 3 is VERIFIED (`stages/3-gui/VERIFICATION-3.3.md`).
**Requirements this stage closes:** FUNC-07, UI-05, PERF-02, COMPAT-02, and QUAL-01 (its listening criterion). ROADMAP also asks for a full re-run of every gate.

## Requirements Confirmed

- **P1: Factory presets (FUNC-07).** About 12 task-oriented presets through `preset-manager` 1.1.1, on the O-AnalogEQ pattern (`initializeFactoryPresets`).
  - Each preset is a mixing or mastering starting point. Working set: Vocal Presence, Kick Tighten, De-Mud, Air Shelf, Rumble Cut, Telephone, Surgical Notch, Gentle Master Tilt. The rest are picked in research.
  - Together they show off the engine: steep slopes (up to 96 dB/oct), M/S and L/R placement, notch and band pass, and auto gain.
  - Suite rules apply:
    - Reset to defaults before every apply.
    - Presets never set `output_gain`.
    - Factory values are normalised with skew in mind (`pattern_factory_preset_normalized_ignores_skew`).
    - The `savePreset` factory guard and validated apply get rows. These are the E16 / R2 rows carried from VERIFICATION-3.1 / 3.3.
  - Presets carry no UI state: `analyzerMode`, `displayRange`, `selectedBand` and the other UI fields stay in the `UI` child.
- **P2: Localisation (UI-05).** Glossary and lint come first (`pattern_parallel_localizers_need_glossary_first`).
  - Claude revises every `fr` (`reviewed: false`) and `zh-Hans` (`'mt'`) draft against the suite glossary. This includes the `bands-full` tip, the preset names and any new copy.
  - Taylor spot-reads the French in the Standalone. French then flips to reviewed.
  - zh-Hans stays at `'mt'`, per the suite rollout convention (`project_i18n_zh_hans_rollout`).
  - All UI gates stay at 0: check-i18n, the fr / zh lint, check-ui-labels, boot-all-uis `--strict-tips`. Widths are checked at the widest language.
- **P3: G10 CPU (PERF-02).** Run the ROADMAP budget as written: 24 cuts × 96 dB/oct, stereo, 48 kHz.
  - Settled ≤ 10 % of one core.
  - All sweeping ≤ 35 %.
  - Nothing enabled ≤ 0.3 %.
  - INFO row: 24 shelves × 96 dB/oct, all sweeping.
  - Lane-pair SIMD only if the scalar budget is missed. The correction hold is the polish fallback.
  - A fail on a busy machine counts as noise first, with one allowed re-run (VERIFICATION-3.3 Issue 4).
- **P4: Multi-rate and hosts (COMPAT-02).**
  - G13 and renders at 44.1, 48, 88.2, 96 and 192 kHz.
  - Logic AU, mono and stereo.
  - **Reaper** is the VST3 host, mono and stereo.
  - **Windows VST3 CI dry run with pluginval, no tag.** This also retires the WebView2 timing for 196 relays (ROADMAP risk row).
- **P5: Full gate battery and listening pass (QUAL-01).**
  - Fresh rebuild.
  - Re-run of the designer, render (default and `--long`), G9 (b), meter-ballistics, the editor bench, the five UI gates, pluginval 10 VST3 / AU and auval.
  - Listening pass by Taylor across every factory preset.
- **P6: Hands-on, folded together.** The 3.3 hands-on rows owed by Taylor (VERIFICATION-3.3) join Stage 4's Logic and Reaper pass. It becomes one checklist:
  - analyzer modes;
  - meter against Logic at 2048;
  - listen by ear;
  - Standalone live input;
  - meter look at 800 × 500 and 1600 × 1000;
  - presets;
  - French read.

## Carry-forward items in scope

| Item | Source | Disposition |
|------|--------|-------------|
| Listen outlives Delete by up to 80 ms + 50 ms fade | VERIFICATION-3.3 Issue 5 | **Fix now.** Page `deleteBand` calls `endListenOn(b)` at once, because a page delete is never a preset pass-through. The 80 ms settle hook stays for host and preset disables. It needs a bench row that kills the regression. |
| Meter hold +1 margin ungated | VERIFICATION-3.3 Issue 1 | Add an M1 variant with jittered block completion (about ±30 % of a block period). H without the +1 must show empty reads. |
| Strip Listen `aria-pressed` follows the latch, code-only | VERIFICATION-3.3 Issue 3 | Add an E-listen33 row: latch off, drag band 3, `aria-pressed` stays false. |
| `.tip-point` removed at the start of the fade | VERIFICATION-3.2 Issue 5 / 3.3 Issue 6 | Cosmetic polish: remove it after the fade. |
| Point-tip housekeeping | VERIFICATION-3.2 Issue 3 | Include. |
| Host text units | VERIFICATION-3.3 carry list | Include. Check value-to-text in Logic and Reaper. |
| `bands-full` fr / zh review | 3.2 CONTEXT U5 amendment | Part of P2. |
| E16 / R2 preset rows (`savePreset` guard, validated apply) | VERIFICATION-3.1 survivors | Part of P1. |
| `pageReady` resend reset, code-only | VERIFICATION-3.3 Issue 2 | Info only, no row (a reload only follows a content-process restart). |

## Constraints Identified

- **Release target: beta 0.x.** Stage 4 does not cut 1.0.0.
  - Versions continue from 0.9.0. Each phase that changes source bumps the minor version.
  - No tag (`feedback_never_tag_unless_publish`). `/publish` is a separate, later decision.
- **Windows CI is a dry run.**
  - The release workflow fires only on `<Plugin>-vX.Y.Z` tags, and this stage creates no tag.
  - The Windows build and pluginval run through a dispatchable workflow, or through a temporary dispatch path.
  - CI configures only the target plugin.
- The audio path is frozen unless a gate forces a change.
  - The 16 XR-lid sessions must stay SHA-256-equal to the 0.5.0 hashes.
  - The probe N1e hash must stay `6274ff9b5b0e488a`.
  - Any DSP change (for example SIMD) must keep both, or must explain each difference.
- Trunk-based work: path-scoped commits (`git commit -- plugins/O-ParametricEQ ...`) and a re-check of branch and staging before every commit. Other plugins have uncommitted work in this checkout (O-Octagon and others).
- Harness runs re-exec with `CFFIXED_USER_HOME`. Factory-preset stamping in the processor constructor must not touch the real home.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset set | ~12 task presets | Matches O-AnalogEQ. Each preset is a usable starting point that also shows off a feature. |
| fr / zh sign-off | Claude revises against the glossary; Taylor spot-reads fr; zh stays `'mt'` | Taylor reads French. This is the suite zh convention. |
| Release target | Beta 0.x, no tag | Some DAW time before 1.0. |
| VST3 host | Reaper | Mono and stereo, many rates and offline-render checks, all fast to run. |
| Windows | CI dry run in Stage 4, no tag | Proves WebView2 and the 196-relay timing while the code is fresh. |
| Listen on Delete | Fix now | The fix is cheap and certain. A page delete never passes through a preset. |
| 3.3 hands-on | Folded into the Stage 4 DAW pass | One sitting for Taylor. |

## Open Questions (for research)

1. **Preset content.** The final list of about 12, with exact band values chosen by ear-oriented rationale. Check each against auto gain and against the display range it opens in. Also decide whether a "Flat / Init" entry belongs in the factory set.
2. **Windows dry-run mechanism.** Find out whether `ci-tests.yml` or `build-and-release.yml` can be dispatched for one plugin on Windows with pluginval and no tag. If neither can, find the smallest change that allows it. Also find how to measure WebView2 relay open time and preset settle on CI, or decide that this is owed to a Windows hands-on.
3. **G10 method.** Choose the measurement harness, either render-harness timing or a dedicated bench, with a pinned thread and repeat counts. Then decide whether the scalar engine meets 10 / 35 / 0.3 % on this machine, or whether SIMD is in play.
4. **Glossary.** Check whether a suite EQ glossary exists for fr / zh terms such as shelf, bell, notch, slope, placement and Mid / Side. If none exists, write it before any revision.
5. **Phase split.** Find out whether Stage 4 needs sub-phases (for example 4.1 presets + i18n + carry-forward fixes, then 4.2 G10 + compat + full battery + hands-on), or whether one phase is enough.

## Next Phase

Ready for: research phase.
