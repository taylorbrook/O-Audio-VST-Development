# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-10-14
**Participants:** User, Claude

Inputs: ROADMAP.md §Stage 4, REQUIREMENTS.md (FUNC-08, FUNC-09, COMPAT-02, QUAL-02 pending), `stages/3-gui/VERIFICATION.md` (Human Verification list + Issues Found carried here), sibling precedent `O-simpleWavetable/.planning/stages/4-polish/CONTEXT.md`.

## Requirements Confirmed

- **FUNC-08 (should)** — factory bank = **Init + the 8 lessons** (Point Source (FOA), Order 1 → 2 → 3, Behind You, Overhead, Rotate the Room, Far Away / Up Close, Slow Orbit, Speakers vs Headphones), via the **preset-manager module + browser panel** (suite norm). Users can save/load their own scenes.
  - Lesson buttons stay. Lesson buttons and the factory bank read **one shared recipe table** (today: provisional `kLessons` in `PluginProcessor.cpp:431`, "Stage 4 refines"). Refine recipes so each isolates one idea.
  - Invariants kept: presets / lessons **never set `outputLevel`** (project feedback); no recipe combines order 3 with distance < 1 m (Binaural headroom note at `PluginProcessor.cpp:424`); lesson ids stable (captions localize, ids don't).
  - Lesson = partial delta (moves only its recipe); factory preset load = full state (reset-to-defaults first, project pattern). Keep both semantics explicit.
- **FUNC-09 (nice)** — creative binaural panner sanity: orbit + headphones usable in a mix (hands-on note, no new feature).
- **COMPAT-02 (must)** — Logic Pro mono + stereo tracks, Ableton Live, no I/O configuration; `auval -v`. Windows VST3 via CI.
- **QUAL-02 (must)** — human headphone listening at order 3: front / back / left / right / above distinct; +90° louder LEFT.
- **Validation sweep** — pluginval s10 VST3 + AU re-run, harness, all UI gates, CHANGELOG `## [1.0.0]`.
- **Carried human hands-on** (from Stage 3 VERIFICATION): Logic R1–R3 + general list, Standalone keyboard/lesson checks ("each lesson moves exactly its recipe, never Output"; Overhead at order 3 doesn't clip), D16 reflow eye-check, R9 long-block idle report.

## Carried Fixes (in scope)

| Item | Fix |
|------|-----|
| `verify_2_1.py` V3 Binaural arm stale since 2.3 | **Approved:** narrow V3 to Stereo (keep its 3 Stereo checks); header note points Binaural to `verify_2_3.py` V-onset |
| en `label.encReadN` ellipsised 8.0 px at order 3 | Shorten (candidate "(3+1)² = {ch} ch"); re-check fr / zh equivalents + check-ui-labels |
| Chevron / puck proximity when azimuth ≈ yaw | Separate visually (offset / layering) — UI polish, no param change |
| Critic pass | `critic-orchestrator` over Stage 2–3 output; fix W-tier + triaged N; log by-design items |

## Constraints Identified

- 12 parameters, IDs frozen — no additions (ROADMAP Pre-Stage-1).
- Latency constant 64 samples; `processBlock` allocation count stays 0 (PERF-01 re-gate after preset work).
- Preset-manager v1.0.6+ (migration hook) — watch the known stale `<CustomState>` child issue and `savePresetToFile()` has no factory guard (project memory).
- Any second binary-data target (factory preset XML) needs a distinct `NAMESPACE`.
- Native-function registry (`PluginEditor.cpp:181`) must grow with the preset-manager natives; every page call registered.
- New preset-panel strings go through en / fr / zh with lint gates; fr stays `reviewed: false`, zh stays `'mt'` (ship bar unchanged this stage).
- Trunk-based: path-scoped commits only.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset scope | Preset-manager module + browser panel; factory = Init + 8 lessons | Suite norm (O-simpleFM, O-simpleWavetable); users save their own scenes |
| Lesson buttons | Kept; one shared recipe table with the factory bank | No drift between buttons and bank |
| Carried fixes | verify_2_1 V3 → Stereo, encReadN shorten, chevron/puck separation | User approved all three |
| Critic scope | Full critic pass, W-tier + triaged N fixed, by-design logged | Ship-quality without churning accepted behaviour |
| Windows | CI build + pluginval via `workflow_dispatch` validate-only; no hands-on | Proves the Windows build without publishing |
| i18n ship bar | Keep fr `reviewed: false` / zh `'mt'` | Native review deferred |
| Version | 1.0.0, CHANGELOG, CODE_REVIEW.md, PLUGINS.md row; **no tag** | Release is a separate `/publish` |
| Cadence (suggested) | Part 1 = critic fixes + carried fixes + preset manager → build/install → preset-panel visual checkpoint; Part 2 = validation sweep, CI Windows, release docs, human hands-on + QUAL-02 sign-off | Mirrors Stage 2/3 checkpoint rhythm; planner may adjust |

## Open Questions (for research)

- Where the preset browser fits in the 1180×780 frame without breaking the lesson row / footer budget (fr width is the binding language; lesson-row slack 74 px).
- Final lesson recipes: what each must set to isolate its idea from any prior state (delta vs. full) — e.g. does "Order 1 → 2 → 3" step on repeated clicks or set one order?
- Factory preset storage: generated XML in BinaryData (distinct namespace) vs. built in code from the shared table.
- Init preset = APVTS defaults (excluding `outputLevel`?) — confirm against project preset rules.
- Whether Logic suspends `processBlock` on stopped non-armed tracks (R1) affects the "well dims" behaviour — record, don't redesign.

## Next Phase

Ready for: research phase

## Amendments (research, 2026-10-14)

Superseded by RESEARCH.md §0: lessons are **full reset + recipe** (not partial delta); `inputMode` is user-owned like `outputLevel`; "Order 1 → 2 → 3" steps on repeated clicks; preset bar goes in the header by retiring `#chipLatency`.
