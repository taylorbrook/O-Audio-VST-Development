---
plugin: O-Octagon
stage: 1
phase: verify
status: verified_partial
last_updated: 2026-08-11
branch: feat/o-octagon
complexity_tier: 6
complexity_score: 5.0
research_depth: DEEP
staged_implementation: true
orchestration_mode: true
next_action: discuss_stage_2
next_stage: 2
ready_for_implementation: true
stage_1_open_manual_gate: logic_8channel_negotiation
build_target: OuariconOctagon
plugin_code: OuOc
musical_parameter_count: 17
venue_value_count: 42
contract_checksums:
  brief: sha256:697a4f32890d7420cdef85bafbf8fe45775bf805cf1ff7b449ed2c14f6b9fbd6
  parameter_spec: sha256:b45f88dc5017ec2c1a9da49ba35242d01903000a4ff199d16758e1b6cbb9e02f
  architecture: sha256:bff8a83b379113ac8b1e2a8915d6f1edc7183558b992bdc3808877f86c406cfe
  roadmap: sha256:aec7d0ce0db9ad6c78cb1c9e9574a0a2f8ddb1cf258e6e4b701f2e2e0137ee29
---

# O-Octagon Status

## Current Position

Stage: 1 of 4 (Foundation) — **verify phase complete: ⚠️ PARTIAL**
Status: Every automated gate re-run independently at verify and passing with zero failures. The one
open Stage 1 exit criterion is **Task 13 (Logic)** — a manual gate that blocks Stage 1 *sign-off*,
not Stage 2 *start*. Task 12 item 3 (audio through Standalone) remains unverified.
Progress: `[#######.............]` 33%
Branch: `feat/o-octagon` (cut from `docs/logic-multichannel-dbap-research` @ 12ae50dd)

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ✓ | 2026-08-11 |
| verify | ⚠️ PARTIAL | 2026-08-11 |

## Stage 1 Verify Results (2026-08-11) — `stages/1-foundation/VERIFICATION.md`

**Verdict: ⚠️ PARTIAL. Ready for Stage 2: yes, with one caveat.**

Every automated gate below was **re-run from scratch at verify**, not read out of SUMMARY.md.
All passed; none regressed.

| Gate | Verify-phase result |
|---|---|
| Clean rebuild, 3 formats (forced TU recompile) | ✓ **0 warnings, 0 errors** in the entire log |
| `auval -a` | ✓ `aufx OuOc OuDv` |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED** |
| AU channel config set | ✓ `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` — exactly RESEARCH F2 |
| pluginval s10 VST3 / AU | ✓ SUCCESS ×3 each |
| State round-trip | ✓ pluginval *Plugin state* + *Plugin state restoration*, ×3 |
| 17 params vs `parameter-spec.md` | ✓ 17/17 on name, range, default **and** group |
| Standalone on a 2-ch device (COMPAT-04) | ✓ launched, stayed running |
| `PHASE-2.2-REPLACE` uniqueness | ✓ exactly 1 occurrence |
| Forbidden CMake keywords, `setLatencySamples`, non-goals | ✓ all clean |

### Requirements

| Req | Status |
|---|---|
| COMPAT-01 (pluginval VST3+AU s10) | ✅ **complete** |
| COMPAT-04 (defined behaviour on stereo) | ✅ **complete** |
| FUNC-01 (8 discrete feeds) | ⚠️ **partial → re-mapped to stage-2** |

**Verify finding — FUNC-01 was mis-staged.** Its third acceptance criterion (*"all 8 output channels
carry independent, non-duplicated signal"*) cannot be met by a shell whose D1 placeholder writes the
same mono sum to all 8 **by design**. Independence requires the DBAP solve (DSP-01/DSP-05). The first
two criteria were met at Stage 1 and are recorded as such in REQUIREMENTS.md so Stage 2 does not
re-derive them. `verifiedAt` moved `stage-1` → `stage-2`.

Corollary: Task 13's "all 8 lanes move" proves **negotiation and writability**, not independence —
with the placeholder in place all 8 lanes carry identical signal. Do not over-read it.

### Why PARTIAL, and why Stage 2 is not blocked

Nothing in Stage 2 depends on the Logic result: Phase 2.1 builds `ChannelMap` and the `VENUE` tree,
and the plugin accepts all three 8-channel containers regardless of which one Logic picks. Running
Task 13 first is nonetheless the cheaper order — if Logic fails to negotiate 8 channels the fault is
in the bus predicate, and unpicking that after a DBAP solver exists costs materially more.

## Stage 1 Execute Results (2026-08-11)

**Files created:** `CMakeLists.txt`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`,
`.planning/parameter-spec.md` (promoted; draft banner-marked superseded).

| Gate | Result |
|---|---|
| Build — VST3 + AU + Standalone | ✓ clean, **zero warnings** from O-Octagon's own TU |
| `auval -a` | ✓ `aufx OuOc OuDv — Ouaricon Audio Development: O-Octagon-dev` |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `-parameter PASS` |
| AU channel configs | ✓ `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` — **exactly RESEARCH F2's prediction** |
| pluginval strictness 10, VST3 | ✓ SUCCESS × 3 runs |
| pluginval strictness 10, AU | ✓ SUCCESS × 3 runs |
| 17 parameters vs `parameter-spec.md` | ✓ verified **programmatically** from the auval dump — all 17 match name, range, default *and* group |
| Standalone on 2-ch device (COMPAT-04) | ✓ opens, no error, generic editor renders 5 groups + units (`m`, `dB/2x`) |

**F2 confirmed empirically.** The AU config set is derived from `isBusesLayoutSupported()` exactly
as researched — this is the first hard evidence that the predicate is the sole authority.

### Outstanding at Stage 1 — carry into verify

1. **Task 13 — Logic 8-channel negotiation is NOT yet done.** Requires Logic and a surround track;
   cannot be automated. It is the strongest available evidence for **FUNC-01 / COMPAT-01**.
   Must confirm: plugin instantiates on a surround track; **all 8 surround-meter lanes move**;
   `outputGain` survives save/close/reopen (FUNC-05 slice); automation menu lists 17 under 5 groups.
   **Record which container Logic actually negotiated** — R2 predicts 7.1-SDDS. Observation, not a
   gate (all three are accepted); it feeds COMPAT-02 at Stage 4.
2. **Task 12 item 3** — audio reaching outputs at unity in Standalone was not confirmed: JUCE's
   Standalone mutes input by default ("Audio input is muted to avoid feedback loop"). Needs a
   manual unmute, or is subsumed by Task 13's meter check.
3. **Gate bypass on record.** The `0-ideation → 1-foundation` gate was run with `--force`: its build
   check is unconditional on stage and cannot pass before `CMakeLists.txt` exists. Logged to
   `.planning/gate-bypasses.log`.
4. **Benign:** pluginval AU emits `WARNING: Current program is -1` — JUCE AU-wrapper reporting,
   present across the repo, not a failure (run still returns SUCCESS).

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (Logic-native 8-channel DBAP spatializer for an irregular, non-flat concert array)
- Architecture inherited as locked constraints from `research/logic-pro-multichannel-octaphonic-dbap.md`
- **17** musical parameters + 42 venue values specified *(corrected from 18 at Stage 0 — see ARCHITECTURE §11)*
- Preset strategy settled (two separate stores)
- Signal flow, UI concept, use cases captured
- 30 requirements extracted with acceptance criteria
- v1.1+ deferrals recorded explicitly

**Stage 0:** ✓ Complete
- Complexity tier 6 (DEEP research); complexity score **5.0** (capped; raw 13.0)
- 9 features researched: bus transport, channel map, venue model, convex hull, DBAP solver,
  source shaping, outside-hull processing, gain stage, verify-ping
- All JUCE APIs verified **directly against the local JUCE 8.0.14 source tree** with file:line
  references (Context7 doc-fetch was unavailable; local source is the stronger authority)
- 8 core DSP components specified with full algorithms
- 3-layer channel-map test strategy designed (runtime invariant → source-parsed golden with a
  committed SHA → offline tone-per-speaker render)
- All 5 open questions resolved with concrete defaults
- Parameter-count discrepancy resolved: **17**, arithmetic slip demonstrated
- 8 risks registered; 2 design defects found and fixed before code exists
- Strategy: **staged implementation** — Stage 2 in 3 phases, Stage 3 in 3 phases
- ARCHITECTURE.md and ROADMAP.md documented

## Stage 0 Findings Worth Carrying

1. **`kAudioChannelLayoutTag_Emagic_Default_7_1`** (`juce_CoreAudioLayouts_mac.h:117`) shows Logic's
   native 7.1 ordering corresponds to JUCE's `create7point1SDDS()` membership, not `create7point1()`.
   `isBusesLayoutSupported()` therefore accepts all three 8-channel containers, and the label map is
   keyed on `ChannelType`. Settled at Stage 4.
2. **For 7.1 the enum-bit order coincidentally equals the initializer-list order** — a hardcoded
   0..7 map would appear correct today. The locked constraint is *more* important because of this,
   not less. **Amplified at Stage 1 research (F1): this holds for all THREE accepted 8-channel
   containers**, so no container choice can discriminate a hardcoded map. Only a non-identity
   `map1..map8` label assignment permutes the buffer — the Phase 2.1 test must drive those.
3. **PERF-02 and QUAL-03 are incompatible under a per-block solve.** Resolved with a fixed
   64-sample absolute-sample-aligned control grid.
4. **Centre-crossing L/R flip** in the stereo sub-point geometry found at design time; fixed with an
   `rFade` spread collapse.

## Stage 1 Research Findings Worth Carrying

- **F1** — all three accepted 8-channel containers have initializer order == enum-bit order (see above)
- **F2** — AU channel configs are *derived* from `isBusesLayoutSupported()`:
  `AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}`. `auval` exercises all six, so the Stage 1
  placeholder must be correct at 1 and 2 output channels, not only 8. SAFE mode is load-bearing for
  AU, not just Standalone. Confirms `PLUGIN_CHANNEL_CONFIGURATIONS` is redundant as well as harmful.
- **F3 (hazard)** — Standalone on a 3–7 output device: `canonicalChannelSet(n)` yields LCR/quad/5.0/
  5.1/7.0, all rejected; Debug asserts, **Release keeps the 7.1 layout while the buffer has n
  channels**. Bound every output loop by `buffer.getNumChannels()` — never by `8`, never by
  `getTotalNumOutputChannels()`, which is the accessor that lies in exactly this state.
- **F4** — `canonicalChannelSet(8) == create7point1()`, so Standalone on an 8-out interface
  negotiates REAL mode with no host. Free Stage 2 listening rig.
- **F5** — `create5point1point2()` has no VST3 layout-table entry; it resolves via the generic
  bit-order fallback. Works, least-exercised of the three; keep 7.1 primary.
- **F6** — JUCE hoists `Fx` to index 0 of `VST3_CATEGORIES`; the emitted string is always
  `Fx|Spatial`. Declare `"Fx" "Spatial"` so the source matches what ships.
- **F7** — `AU_MAIN_TYPE kAudioUnitType_Effect` is already JUCE's default. Both keywords are new to
  this repo — no sibling CMakeLists sets either.
- **State root is `OOctagon`** (not the sibling's `OOrbitParams` idiom) and must never change —
  Phase 2.1 attaches `VENUE` to that node. Stage 1 sessions carry **no** `VENUE` child, so
  `readVenueFromState()` must treat a missing/partial node as "use defaults".
- **`-Wswitch-enum` bans switching on `AudioChannelSet::ChannelType`** (~60 enumerators; warns even
  with a `default:`). The Phase 2.1 label map must be a table or `if`-chain.

## Stage 1 Plan Decisions (plan phase)

The four open items RESEARCH §9 handed to the plan phase are resolved in PLAN.md:

- **P1** — five parameter groups: Position / Solve / Weights / Space / Output. The headline
  gesture is automating eight weights; a flat 17-entry menu buries them. Reversible.
- **P2** — the venue-store member slot is claimed **above `apvts`** in `PluginProcessor.h` as a
  comment marker. Declaration order is fixed at Stage 1 and annoying to change once 2.1 depends
  on it. No member exists yet.
- **P3** — the D1 placeholder loop carries the greppable token `// PHASE-2.2-REPLACE:` so Phase
  2.1's "zero hardcoded output indices" gate retires it rather than grandfathering it.
- **P4** — `parameter-spec.md` is promoted from the draft as Task 1. The executor reads the
  promoted file, **never** `parameter-spec-draft.md` (which still marks OQ3/4/5 and the
  17-vs-18 count as open, all four resolved at Stage 0).

Also settled at plan: `srcX`/`srcY` display **normalised** in the host lane (metres are a Stage
3.1 UI-side conversion — a value→text lambda is captured at construction and cannot read a live
venue); **no `PluginEditor.{h,cpp}`** at Stage 1 (`GenericAudioProcessorEditor` is five lines and
is itself the 17-parameter exit criterion).

## Next Steps

1. **Run the Logic check (Task 13)** — the one Stage 1 exit criterion still open. Blocks Stage 1
   sign-off, not Stage 2 start.
2. **Stage 2 discuss** — `/plugin-discuss O-Octagon 2-dsp` (Phase 2.1: `ChannelMap` + `VENUE` tree)
3. UI mockup — two screens, Room + Venue. Due before Stage 3.1; not a Stage 1/2 blocker.
4. Measure Roy Barnett Recital Hall — 8 × (x, y, z) metres + front/rear ear heights
5. Pause here

## Context to Preserve

**Build constraints for Stage 1:**
- Target `OuariconOctagon`, folder `plugins/O-Octagon`, `PRODUCT_NAME "O-Octagon${OUARICON_DEV_SUFFIX}"`
- `PLUGIN_CODE OuOc` (verified unused across all 39 existing plugins)
- **`VERSION 1.0.0`**, never `PLUGIN_VERSION`
- **No `PLUGIN_CHANNEL_CONFIGURATIONS`**
- `juce::juce_dsp` linked; `BusesProperties` in the constructor init list
- **Must not link SAF** (unlike sibling O-Orbit)

**Locked by prior research — do NOT re-litigate:**
- mono/stereo in → `AudioChannelSet::create7point1()` out; 7.1 is only an 8-channel carrier
- Never `octagonal()` or `discreteChannels(8)` — Logic ignores both
- Not multi-output (`aumu` only; `aufx` gets one bus)
- Speaker→buffer map built ONCE in `prepareToPlay()` via `getChannelIndexForType()` — a wrong map
  is SILENT and passes every automated gate
- DBAP per the **2011-04-14 revised** equations

**Deferred to v1.1+ — do not plan work for these:**
VBAP A/B mode; binaural/stereo fold-down; quadraphonic variant; internal diffuse reverb; motion
engine; multiple simultaneous sources.

**Highest risk:** the speaker→buffer channel map (R1, CRITICAL, silent failure).

## Files Created

- `plugins/O-Octagon/.planning/BRIEF.md` *(Ideation; parameter count corrected at Stage 0)*
- `plugins/O-Octagon/.planning/REQUIREMENTS.md` *(Ideation)*
- `plugins/O-Octagon/.planning/parameter-spec-draft.md` *(Ideation)*
- `plugins/O-Octagon/.planning/research/ARCHITECTURE.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/ROADMAP.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/stages/0-ideation/CONTEXT.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/stages/1-foundation/CONTEXT.md` *(Stage 1 discuss)*
- `plugins/O-Octagon/.planning/stages/1-foundation/RESEARCH.md` *(Stage 1 research)*
- `plugins/O-Octagon/.planning/stages/1-foundation/PLAN.md` *(Stage 1 plan — 14 tasks)*
- `plugins/O-Octagon/.planning/stages/1-foundation/SUMMARY.md` *(Stage 1 execute)*
- `plugins/O-Octagon/.planning/parameter-spec.md` *(Stage 1 execute — promoted, supersedes the draft)*
- `plugins/O-Octagon/CMakeLists.txt` *(Stage 1 execute)*
- `plugins/O-Octagon/Source/PluginProcessor.h` *(Stage 1 execute)*
- `plugins/O-Octagon/Source/PluginProcessor.cpp` *(Stage 1 execute)*
- `plugins/O-Octagon/.planning/STATUS.md`
