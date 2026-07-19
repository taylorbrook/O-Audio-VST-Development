# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-07-14
**Participants:** User, Claude
**Entry state:** Stage 3 (GUI) VERIFIED 2026-07-11 (verdict ✅, VERIFICATION.md in `stages/3-gui/`).
Automated bar green at HEAD: 19/19 render goldens byte-identical, auval `aumu OCbs OuDv`
SUCCEEDED, pluginval strictness-10 SUCCESS (warm), ui_frontend_check 14/14, bridge gate
32 JS = 32 C++. Stage 4 is the final v1.0 stage: preset banks, subjective/perf/Dorico
validation, Windows cross-platform validation, documentation.

## Requirements Confirmed

Stage 4 is the verification stage for the four requirements the traceability table
assigns to it (REQUIREMENTS.md line 255):

- **FUNC-03 (must, pending):** Same engine plays convincing orchestral arco sustains AND
  ambient drone material. Verified subjectively via A/B against reference libraries
  (Spitfire Albion / CSS / VSL for orchestral; Stephen O'Malley / Tony Conrad for drone).
- **FUNC-04 (should, pending):** Ship two 5-preset banks — Orchestral (Cinematic Bass
  Sustain [default], Section Bass, Solo Arco Bass, Pianissimo Bass, Forte Bass) + Drone
  (Infinite Drone, Just-Intoned Drone [7-limit detune], Scordatura Bass, Sub Drone, Dark
  Pad Bass). **Claude authors all 10 from DSP defaults + house references; user auditions
  and requests tweaks during execute.**
- **PERF-02 (should, pending):** CPU < 5% per voice on Apple Silicon at 44.1/48 kHz,
  256-sample block, typical settings. Benchmark + confirm.
- **COMPAT-02 (must, pending):** Loads and plays microtonal notation correctly in Dorico
  via VST3 Note Expression, verified with the spike-findings pattern. **Ship a full
  `.doricolib` Playback Template bundle** (see Approach Decisions).

Also closes the Stage-4-owned remainder of a Stage-1 requirement:

- **COMPAT-01 (must, partial):** pluginval strictness-10 on **Windows VST3** — owned by
  Stage 4 / CI. macOS VST3 + AU already SUCCESS at every gate. **Run Windows build +
  pluginval via CI this stage (release held — see scope decision).**

Subjective carry-forwards promoted alongside FUNC-03:

- **DSP-10 (must, partial):** Slow expressive attack — final subjective character
  confirmation rolls to this stage's audition (DSP evidence already in place from R38).

## Constraints Identified

### Release scope (USER DECISION)
- **Validate cross-platform, hold public release.** Run the full production validation
  battery INCLUDING Windows VST3 build + pluginval-10 via CI (closes COMPAT-01), but do
  NOT push a public GitHub release or PKG installer this stage. User decides on release
  after auditioning presets + validation results.
- Implication: `plugin-publishing` (public GitHub release) and `plugin-packaging`
  (branded PKG) are **out of scope** for this stage's execute; the CI Windows-build path
  is **in scope** (it is the COMPAT-01 gate, not a release artifact).

### Test surface (USER DECISION)
- Manual DAW checks run in **Logic Pro** (primary, AU+VST3) and **Dorico** (COMPAT-02)
  only. Ableton / Reaper / Cubase / Bitwig are NOT part of this stage's manual pass —
  their coverage is delegated to automated pluginval-10 + auval, not hand-testing.
- The five human-in-the-loop DAW checks carried from Stage 3 verify all run in Logic:
  (1) Logic Release editor open/close ×10 (destruction-order gate), (2) 31-param DAW
  interaction incl. 6 skewed-param generic-view spot check + detune detents + ACTIVE_STRINGS
  stepper + TUNING_SYSTEM gating + NOTE_EXPRESSION toggle, (3) picker UAF scenario
  (open picker → close window → choose/cancel), (4) Logic smoke (E1 drone / automate
  BOW_SPEED / project reload), (5) visual QA at 1000×650 (7 sections + tab bar, console
  clean, preset bar vs `~/Library/O-Contrabass/Presets/`, tuning survives preset
  round-trip, Schelleng dot / spectrum / VU tracking).

### Dorico distribution (USER DECISION)
- Ship the **full `.doricolib` Playback Template bundle** — not a bare `.doricoexpmap`.
  Per project memory `critical_dorico_distribution_mechanism`, Dorico does NOT
  auto-ingest a `.doricoexpmap` dropped into User/; distribution requires a Playback
  Template / EndpointConfig / `.doricolib`. Delegate authoring to the `dorico-agent`.
  Cross-check `critical_dorico_microtonal_top_level_fields` (top-level `<pitchBendRange>`
  + `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` per
  ExpressionMapDefinition are LOAD-BEARING — a recurring silent 12-TET regression) and
  `critical_dorico_keyswitch_routing` (O-Contrabass is sustained-arco only, so keyswitch
  routing is minimal, but the exp-map schema layer still applies).

### Preset authoring gotchas (project memory — apply at execute)
- `applyPresetJson` must reset ALL params to defaults before applying
  (`pattern_preset_apply_needs_reset_to_defaults`) — hand-authored factory presets that
  omit keys silently inherit stale state otherwise.
- Author factory-preset values in **engineering units + `convertTo0to1`**, NOT linear
  fractions — O-Contrabass has skewed params (BOW_SPEED, BOW_PRESSURE, BRIGHTNESS,
  VIBRATO_RATE, SLOW_LFO_RATE, REFERENCE_PITCH). A skewed value authored as a raw
  normalized fraction recalls 10–30× wrong (`pattern_factory_preset_normalized_ignores_skew`).
- Just-Intoned Drone preset uses per-string detune for 7-limit intervals
  (DETUNE_A=+204¢, DETUNE_D=−14¢, DETUNE_G=+182¢ per ROADMAP Phase 2.2 criteria).

### DSP is FROZEN — this stage does NOT touch the signal path
- 19/19 render goldens must stay byte-identical through the entire stage (same invariant
  as Stage 3). Preset authoring writes parameter STATE, not DSP arithmetic. Any change
  that shifts a golden is out of scope and signals a defect.
- The following are LOCKED as v1.1 and must NOT be "fixed" during polish:
  - **STRING_TENSION** bound-but-inert (D2) — DSP wiring is v1.1 (default 0.5 is not a
    no-op; wiring it re-baselines goldens). Ships inert, annotated.
  - **AnaMark `.tun` parser** absent (D1) — picker is `.scl`-only; TUN → v1.1 shared-module
    upgrade. Choice index mapping frozen (0=Scala, 1=MTS-ESP, 2=12-TET).
  - **DSP-07 sub-harmonic** audible-engagement collapse post-tanh-port/post-body → v1.1
    priority-bumped retune (kForceBoost / bias-amplitude / injection-point). Wire-up is
    stable + default-state bit-exact; only audible depth is deferred.
  - **DSP-08 slow-LFO** 15.7% vs 20% breathing target → v1.1 metric/gain tune.
  - **DSP-09 vibrato** peakDepthCents ~7.95¢ vs 10–14¢ band → v1.1 transfer tune.
  - **FUNC-07 MTS-ESP** present-but-stub (returns 12-TET) → v1.1 SDK linkage. Scala/TUN
    import itself is COMPLETE.
  - **registry.yaml staleness (R5)** — lists preset-manager 1.0.2; module.yaml (1.0.4)
    is authoritative. Cosmetic; fix opportunistically, non-blocking.

### Version / status housekeeping
- CHANGELOG currently at `[1.1.0-dev]` for the Stage-3 GUI entry. Stage 4 ships **v1.0.0**
  (the pre-release `-dev` engine versioning collapses to the first real product version).
  Reconcile the CHANGELOG header + any version macro at execute.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Release scope | Validate cross-platform (incl. Windows CI/pluginval), HOLD public release | User: production-ready without committing to publish; decide after audition |
| Windows COMPAT-01 | Build + pluginval-10 via CI this stage | It is the COMPAT-01 gate, not a release artifact; flags in place since Stage 1, never exercised |
| Public release / PKG | OUT of scope this stage | User holding release; `plugin-publishing` + `plugin-packaging` deferred to a post-audition trigger |
| Manual DAW test set | Logic Pro + Dorico only | User's available/relevant hosts; other DAWs covered by automated pluginval-10 + auval |
| Preset authoring | Claude authors all 10, user auditions + tweaks in execute | Momentum; house references known (Spitfire/CSS orchestral, O'Malley/Conrad drone) |
| Dorico distribution | Full `.doricolib` Playback Template bundle | `.doricoexpmap` alone is not auto-ingested (memory); `.doricolib` is the correct mechanism; delegate to dorico-agent |
| DSP signal path | FROZEN — 19/19 goldens byte-identical invariant holds | Polish stage writes state + docs, not DSP; v1.1 deferrals stay deferred |
| Product version | v1.0.0 | Collapse `1.x-dev` engine track to first shipped product version |

## Open Questions (for research/plan phase)

- **Preset format & storage:** Confirm the exact OuariconPresetManager factory-preset
  format and where factory presets live vs user presets (`~/Library/O-Contrabass/Presets/`).
  Are factory presets embedded (BinaryData) or seeded to disk on first run? Verify against
  a sibling OuariconPresetManager plugin (O-Polystutter / O-SpectralShaper precedent) so
  the reset-to-defaults + skew-safe authoring patterns apply correctly.
- **Dorico `.doricolib` schema for a sustained-arco NE instrument:** What is the minimal
  correct Playback Template + EndpointConfig for an instrument that is NoteExpression-only
  (no keyswitches)? HSO is the KS reference but O-Contrabass has no articulation switching —
  confirm the exp-map reduces to the microtonal top-level fields + a single natural/arco
  technique entry. (dorico-agent research.)
- **Windows CI path:** Does the existing `build-and-release.yml` / CI already build
  O-Contrabass on Windows, or does the plugin need adding to the matrix? Confirm the
  WebView2 static-linking flags produce a working Windows VST3 (blank-UI risk per memory).
  Note the plugin was NEVER Windows-built despite flags in place since Stage 1.
- **CPU benchmark method:** Which harness/host for the PERF-02 <5% measurement — the
  offline render-harness timing, a Logic CPU meter reading at typical settings, or
  pluginval's timing? Confirm "typical settings" = default preset at 44.1/48 kHz /
  256-block, single voice.
- **A/B reference audition rig:** How does the user want FUNC-03 / DSP-10 subjective sign-off
  captured — informal Logic A/B, or a documented audition checklist like the Stage-2 R38
  probe sequence?

## Next Phase

Ready for: **research** phase — investigate the OuariconPresetManager factory-preset
format + skew-safe authoring, the minimal `.doricolib` schema for an NE-only instrument,
the Windows CI matrix path, and the PERF-02 benchmark method. Then plan.
