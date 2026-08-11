# Stage 1 — Foundation: SUMMARY

**Plugin:** O-Octagon
**Stage:** 1 of 4 — Foundation + Shell
**Phase:** execute ✓
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`

---

## Outcome

A loadable, validating 8-channel shell. Builds clean in three formats, registers as an AU,
passes `auval` and pluginval strictness 10, and exposes all 17 parameters with correct names,
ranges, defaults, units and grouping.

**12 of 14 tasks complete.** Task 13 (Logic) is an outstanding manual gate; one sub-item of
Task 12 is unverified. Both are stated in full under *Outstanding* below — Stage 1 is **not**
fully exited until Task 13 runs.

---

## Files

**Created**

| File | Task |
|---|---|
| `plugins/O-Octagon/.planning/parameter-spec.md` | 1 |
| `plugins/O-Octagon/CMakeLists.txt` | 2 |
| `plugins/O-Octagon/Source/PluginProcessor.h` | 3 |
| `plugins/O-Octagon/Source/PluginProcessor.cpp` | 4–8 |
| `plugins/O-Octagon/.planning/stages/1-foundation/SUMMARY.md` | — |

**Modified**

| File | Change |
|---|---|
| `plugins/O-Octagon/.planning/parameter-spec-draft.md` | superseded banner |
| `PLUGINS.md` | `🚧 Stage 0` → `🚧 Stage 1`, version `1.0.0-dev` |
| `plugins/O-Octagon/.planning/STATUS.md` | phase table, results, outstanding items |
| `.planning/workflow/scripts/run-gate.sh` | three `set -e` crash fixes (see *Incidental* below) |

Root `CMakeLists.txt` untouched — the `plugins/*` glob at `:48-58` picked the new directory up on
re-configure, as RESEARCH §5 predicted.

---

## Verification results

| Gate | Command | Result |
|---|---|---|
| Build, 3 formats | `cmake --build build --target OuariconOctagon_{VST3,AU,Standalone}` | ✓ linked; **0 warnings, 0 errors** in O-Octagon's own TU |
| AU registration | `auval -a \| grep -i octagon` | ✓ `aufx OuOc OuDv` |
| AU full validation | `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED** |
| Parameter count | (from auval) | ✓ 17 × `-parameter PASS` |
| pluginval VST3 | strictness 10, 3 runs | ✓ SUCCESS × 3 |
| pluginval AU | strictness 10, 3 runs | ✓ SUCCESS × 3 |
| 17 params vs spec | programmatic diff of the auval dump | ✓ all 17 match name, range, default **and group** |
| Standalone COMPAT-04 | launch on 2-ch device | ✓ opens, no error; editor renders 5 groups + units |

### F2 confirmed empirically — the headline finding

RESEARCH F2 predicted JUCE would *derive* the AU channel-config set from
`isBusesLayoutSupported()` as `{(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}`. `auval` reports:

```
Reported Channel Capabilities (explicit):
      [1, 1]  [1, 2]  [1, 8]  [2, 1]  [2, 2]  [2, 8]
```

Exact match. This is hard evidence that the predicate is the sole authority and that
`PLUGIN_CHANNEL_CONFIGURATIONS` is redundant as well as harmful. It also confirms SAFE mode is
load-bearing for **AU**, not merely Standalone: `auval` exercised the 1-out and 2-out configs and
passed, which is the real test of Task 6's `buffer.getNumChannels()` bound.

---

## Plan decisions as built

- **P1** — five groups (`position`/`solve`/`weights`/`space`/`output`), separator `"|"`. Confirmed
  live: they surface as AU clumps and render as sections in the generic editor.
- **P2** — venue slot claimed as a comment marker above `apvts` in `PluginProcessor.h`. No member.
- **P3** — the placeholder loop carries `// PHASE-2.2-REPLACE:` and it is the **only** occurrence
  of that token in the plugin.
- **P4** — `parameter-spec.md` promoted; the draft carries a superseded banner.
- `srcX`/`srcY` display normalised in the host lane; no `this`-capturing value→text lambda.
- No `PluginEditor.{h,cpp}` — `GenericAudioProcessorEditor`, five lines.

Deviation from ARCHITECTURE §12, per plan: `VST3_CATEGORIES "Fx" "Spatial"` (not `"Spatial" "Fx"`)
— JUCE hoists `Fx` to index 0 regardless, so this makes the source match what ships (RESEARCH F6).

---

## Outstanding — must be closed before Stage 1 exits

1. **Task 13 — Logic 8-channel negotiation. NOT DONE.** Requires Logic and a surround audio track;
   not automatable. Strongest available evidence for **FUNC-01 / COMPAT-01**. Confirm:
   - plugin appears in the menu and instantiates on a 7.1 / 7.1-SDDS / 5.1.2 track
   - **all 8 lanes of the surround meter move**
   - `outputGain` moved off default survives save → close → reopen (FUNC-05 slice)
   - automation menu lists **17** parameters under the five groups
   - **record which container Logic negotiated** — R2 predicts 7.1-SDDS. Observation, not a gate;
     feeds COMPAT-02 at Stage 4.
2. **Task 12 item 3 — audio at unity through Standalone, unverified.** JUCE's Standalone mutes
   input by default ("Audio input is muted to avoid feedback loop"). Needs a manual unmute, or is
   subsumed by Task 13's meter check.
3. **Not attempted:** the 3–7-output-device F3 hazard. No such interface was available. Release is
   defended by the `buffer.getNumChannels()` bound; the defence is reasoned, not measured.

## Known-benign

`pluginval` on the AU emits `WARNING: Current program is -1... Is this correct?`. This is JUCE's
AU-wrapper program reporting, is present across the repo, and the run still returns SUCCESS.

---

## Incidental: three fixes to `run-gate.sh`

The `0-ideation → 1-foundation` gate **crashed** rather than reporting, and none of its checks ran.
Three `set -euo pipefail` aborts, all the same class — an operation that legitimately fails before
the first build was allowed to kill the script:

| Line | Cause | Fix |
|---|---|---|
| ~137 | `< "$PLUGIN_DIR/CMakeLists.txt"` — the file cannot exist at a 0→1 transition | guard with `[ -f ... ]` |
| ~158 | `find` on a not-yet-existent artefacts dir exits 1 | `\|\| true` |
| ~348 | same `find`, re-resolved after the build check | `\|\| true` |

These blocked the 0→1 gate for **every** new plugin, not just this one.

**The gate was then still BLOCKED and was run with `--force`** (exit 2, logged to
`.planning/gate-bypasses.log`): its build check is unconditional on the target stage and cannot
pass before `CMakeLists.txt` exists. The command spec says the 0→1 gate should check *schema and
artifacts only*. Making the build check stage-aware is a semantic change to shared workflow
infrastructure and was deliberately **not** made here — flagged for a decision instead.

Schema also reported SKIPPED: the individual phase commands write no `HANDOFF.json`.

---

## Non-goals honoured

No `ChannelMap`, no `VENUE` tree, no `VenueModel`/`ConvexHull2D`/`DbapSolver`/`SourceShaper`/
`HullProcessor`/`GainStage`/`VerifyPing`, no control grid, no `SmoothedValue`, no render harness,
no WebView or `juce_add_binary_data`, no `PluginEditor.{h,cpp}`, no module dependency, no SAF.

## Carried forward to Stage 2

Unchanged from PLAN.md §Carried forward. The two that Stage 1 makes more concrete:

- **[2.1]** F1 still stands — all three accepted containers have initializer order == enum-bit
  order, so a hardcoded 0..7 map is byte-identical to a correct one under every accepted layout.
  The channel-map test **must** drive non-identity `map1..map8` label maps; a container-only test
  is vacuous.
- **[2.2]** The output loop bound is `buffer.getNumChannels()` — never `8`, never
  `getTotalNumOutputChannels()`. The `PHASE-2.2-REPLACE` block is the template.
