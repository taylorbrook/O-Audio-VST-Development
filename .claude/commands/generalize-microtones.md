---
description: Promote the validated VST3 Note Expression pattern into a shared Ouaricon module and propagate Dorico microtonal playback across all pitched plugins
---

# Generalize Microtones Across the Plugin Suite

The VST3 Note Expression pipeline was validated end-to-end on O-Lyrica (see auto-loaded `spike-findings-VST-development` skill, synthesized from spikes 001–003 on 2026-04-22/23). The next step is to promote that pattern into a reusable shared module so every pitched Ouaricon plugin inherits Dorico microtonal playback as a standard feature.

## Scope

### 1. Shared module extraction
Create a new Ouaricon shared module (suggested name: `dsp/note-expression` — confirm against existing module naming by running `/module-list` first).

Module contents, derived from the spike and **cleaned of diagnostic logging**:
- `NoteExpressionController.h` — `TuningNoteExpressionController` class (advertises `kTuningTypeID`).
- `NoteExpressionExtensions.h` — `VST3ClientExtensions` subclass with raw-event queue and NEC query dispatch.
- A header-only voice helper (e.g. `applyPendingTuning(pendingSource, midiNote, currentFrequency)`) so each plugin's `startNote` is a one-line integration.
- `README.md` documenting module use, the required local JUCE patch (copy from `sources/shared-code/juce-patch.md` in the spike skill), and the end-user Dorico expression-map setup.

**Dead code to strip before module creation** (per the skill's landmine list):
- All `OLyrica::detail::neTrace(...)` call sites.
- The `detail::neTrace` function and `detail::iidToHex` helper.
- `#include <fstream>` in the header.

### 2. Target plugins (priority order)

| # | Plugin | Status |
|---|--------|--------|
| 1 | **O-Lyrica** | Strip spike throwaway code, consume the shared module, integrate NE with existing `TuningEngine` (spike currently bypasses it). Keep as the reference integration. |
| 2 | **O-Bells** | Apply module. |
| 3 | **O-IntonationPad** | Apply module. |
| 4 | **O-Prism** | Apply module. |
| 5 | **O-Wind** | Apply module. |
| 6 | **O-Reed** | Apply module. |
| 7 | **O-Bowed** | Apply module. |
| 8 | **O-Formant** | Apply module. |

For each plugin: `/module-add` → wire `getVST3ClientExtensions()` → add per-voice tuning source + `applyPendingTuning` in `startNote` → build + install + pluginval → Dorico quarter-sharp smoke test.

### 2a. Dorico Delegation (Phase B per-plugin)

The "Dorico quarter-sharp smoke test" step in the per-plugin loop above delegates to `dorico-agent` rather than re-deriving the smoke-test pattern inline.

**Phase B invocation pattern (sequential — one Task() per plugin, serial):**
```
for plugin in [O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant]:
    Task(subagent_type="dorico-agent",
         description="${plugin} Phase B Dorico bring-up",
         prompt="Apply Dorico expression-map + endpoint config + playback template entry for ${plugin} based on the O-MicrotonalSampler v1.16.x reference. Run TC-1..TC-5 smoke test. Report status.")
    # Wait for completion, inspect report, halt-on-failure for triage.
```

**Why sequential:** one-time propagation — wall-clock cost acceptable; per-plugin failure isolation simpler; clean stop-on-first-failure for debugging.

**Reference docs:** `.claude/agents/dorico-agent.md` (Scope, Capabilities, Output Contract); `plugins/O-MicrotonalSampler/Resources/dorico/` (canonical reference).

### 3. End-user deliverable
Ship a pre-configured Dorico expression map file (`.doricoexpmap`) with Microtonality = "VST3 Note Expression" in each plugin's installer. Without this, end users will experience the exact UX trap we hit in Spike 002 (default maps route non-Steinberg VST3s to pitch bend or VST2 detune → no microtones, plugin appears broken). Place canonical copy in `modules/[new-module]/resources/`.

## Known landmines (from spike-findings-VST-development)
- Dorico represents a quarter-sharp C4 as `pitch=C#4, NE=-50¢`, **not** `pitch=C4, NE=+50¢`. Always correlate by `noteId`, never by MIDI pitch math.
- Dorico never queries `INoteExpressionController` — NEC advertisement is dead code for Dorico but kept for other VST3 hosts. Not a reason to drop it.
- Voice must apply NE tuning to `currentFrequency` BEFORE the DSP model's `trigger(...)`, otherwise first sample renders at untuned pitch → zipper at attack.
- Spike only tested same-block NE. Real module should maintain a persistent `noteId → voice` map across blocks for hosts that emit mid-note NE.

## Out of scope (file as follow-ups)
- Windows VST3 verification (spike was macOS-only).
- Additional NE types beyond `kTuningTypeID` (custom IDs `[100000, 200000]` reserved for future per-note timbre/vibrato).
- MTS-ESP as an orthogonal microtonal path for Reaper/Bitwig users.
- Quarter-flat / ¾-sharp / ¾-flat test matrix expansion (only quarter-sharp was aurally/log-verified).

## Kickoff

This is milestone-scale work (1 module + 8 plugins + installer changes). Propose:

**Phase structure**
- **Phase A — Extract:** build the shared module from cleaned spike code; prove it on O-Lyrica as the reference consumer (existing integration replaced with module consumption).
- **Phase B — Propagate:** apply the module to the remaining 7 pitched plugins.
- **Phase C — Ship:** bundle the Dorico expression map + user docs in each plugin's installer.

Decide whether this warrants a fresh milestone (`/gsd-new-milestone`) or slots into the current one (`/gsd-add-phase`) based on project state, then route to `/gsd-plan-phase` for Phase A.

Assume the auto-loaded `spike-findings-VST-development` skill is the implementation bible — every pattern, code snippet, and landmine is already synthesized there.
