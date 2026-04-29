---
title: "O-MicrotonalSampler Phase 2.1 — Reopen (FUNC-03 + FUNC-04 rectification)"
created: 2026-04-28
phase: 2.1-reopen
status: engineering_bar_green; user_perceptual_verification_failed_case_a; second_reopen_required
verifies_requirements:
  - FUNC-03   # polyphony cap enforcement (was structurally complete, functionally inert)
  - FUNC-04   # nearest-pitch fallback (spec'd in REQUIREMENTS line 80, never implemented)
triggered_by: stage-4 user testing (Phase 4.2 prep, after fixture-OFF rebuild surfaced sparse-folder behaviour)
followup: CASE-A-AUDIT-CHARTER.md — fresh-context deep audit required to resolve "only D#3/E3 sound" Case A symptom
---

> **STATUS UPDATE (2026-04-28, post-commit):** Engineering bar (pluginval-10
> + auval) remains green. User-side perceptual verification **failed** with
> "Case A": after a clean host restart, only MIDI 51 (D#3) and 52 (E3)
> produce audio when the user's 42-file violin folder is loaded — even
> exact-match recorded pitches like C4 are silent. The Phase 2.1 reopen
> fixes (`findSlot` nearest-pitch + `CappedSynthesiser` polyphony
> enforcement) remain valid; a separate, deeper bug in the load → voice →
> audio buffer pipeline needs investigation. See
> `CASE-A-AUDIT-CHARTER.md` for the audit charter. A second Phase 2.1
> reopen will land separately once the audit identifies the root cause.

# Phase 2.1 — Reopen (FUNC-03 + FUNC-04 rectification)

## Trigger

After the Stage 4 build-config rectification (`OMTS_PHASE_2_1_TEST_FIXTURE`
flipped OFF, see `.planning/stages/4-polish/AUTOMATED-CHECKS.md` §"Build-config
rectification"), the user loaded a real sparse sample folder
(`vln_long_mp-A#2-V127-T6N6.aif` × 43 files; one dynamic, sparse pitch
coverage). Two symptoms surfaced immediately:

1. **Single-note key presses produce silence on most keys.**
2. **A 16-note chord plays a single pitch only.**

Per the PLAN failure-routing table (Stage 4 PLAN, "Failure Routing", row
"Task 9: any listening checklist item fails"), Phase 2.1 (Voice DSP / SampleMap)
owns these defects and is reopened. Stage 4 halted until this reopen
shipped green.

## Defects

### DEF-2.1-R1 — `SampleMap::findSlot` exact-match only (FUNC-04)

**Spec** (`REQUIREMENTS.md:80`):

> Triggering MIDI note `N` plays the sample mapped to pitch `N`
> *(or nearest if `N` is unsampled)*

**As-was** (`SampleMap.h`):

```cpp
const SampleSlot* findSlot (int midiNote, int velocityLayer) const noexcept
{
    for (const auto& s : slots)
        if (s.midiNote == midiNote && s.velocityLayer == velocityLayer)
            return &s;
    return nullptr;       // ← exact-match only; spec's "or nearest" never honoured
}
```

The voice (`MicrotonalSamplerVoice.cpp:487-495`) treats `nullptr` as "out of
range" and silences via `clearCurrentNote()`. With sparse folders most keys
return `nullptr` — silence on most keys.

**Why Stage 2 verify missed it.** The Phase 2.1 in-memory test fixture
(`PluginProcessor.cpp:150-230`, `#ifdef O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE`)
generated a sample for **every** MIDI note 21..108. So `findSlot`'s exact-match
path always succeeded; the spec's nearest-fallback clause was never exercised.
The fixture rode through Stage 2 verify, all of Stage 3, and Phase 4.1, until
the Stage 4 build-config fix turned the fixture off and surfaced real-folder
behaviour for the first time.

The repitch infrastructure (`computePlayRateForSlot`) was already in place —
it was waiting for `findSlot` to deliver a candidate slot.

### DEF-2.1-R2 — `polyphony` APVTS parameter not enforced (FUNC-03)

**Spec** (`REQUIREMENTS.md:24`):

> Supports up to 16-voice polyphony

**As-was**: the `polyphony` parameter was created
(`PluginProcessor.cpp:65-69`, range 1..16, default 16), exposed via WebSlider
(`PluginEditor.cpp:43, 461-462`), and pre-allocated 16 voices
(`PluginProcessor.cpp:103-114`) — but **its value was never read by the audio
engine**. Lowering the cap had no effect; all 16 voices ran always.

**Why Stage 2 verify missed it.** Pluginval and verify never lowered the cap
to test it; the fixture had every key sampled so it always sounded "lots of
voices working". The pre-allocation count (16) and the parameter cap (1..16)
both default to 16 — the inert path was masked by the matching defaults.

## Fixes

### Fix 1 — Nearest-pitch fallback in `SampleMap::findSlot`

`Source/SampleMap.h` — replaced exact-match with linear scan returning the
slot in `velocityLayer` whose `midiNote` is closest to the target. Returns
`nullptr` only when the requested layer contains no slots at all (consistent
with the existing voice silence path for that case).

**Tie-breaking:** equidistant choice prefers the lower `midiNote` so the
substitute transposes UP. Preserves more of the sample's low-frequency
content than a downshift would (downshift loses headroom and sounds dull;
upshift sounds brighter, which is the conventional choice in Kontakt /
Decent Sampler / SFZ implementations).

**Per-call cost:** linear scan over slots (≤ 88 × 4 = 352 entries for a
fully-populated map; typically much less for sparse folders). On the audio
thread, called once per `startNote` (not per sample). Trivial.

### Fix 2 — `CappedSynthesiser` enforces the polyphony APVTS cap

`Source/PluginProcessor.h` — new `CappedSynthesiser : juce::Synthesiser`
subclass with:

- `setVoiceCap(int)` — atomic store, called from `processBlock` before
  `renderNextBlock`.
- `noteOn` override — counts active voices; if `≥ cap`, calls
  `findVoiceToSteal` + `stopVoice` to free a slot, then defers to
  `juce::Synthesiser::noteOn`. Runs under the synth's recursive lock
  (already held by `renderNextBlock`), so the count + steal is race-free.

`Source/PluginProcessor.cpp` — added the `setVoiceCap` call to `processBlock`,
right after the NE drain and before `renderNextBlock`. Reads the `polyphony`
raw parameter atomically (consistent with `output_gain` pattern at line 271).

The pre-allocated voice pool stays at 16 to preserve PERF-01 (no RT alloc
when the user *raises* the cap). Only the runtime cap changes.

## Files modified

| File | Change |
|---|---|
| `Source/SampleMap.h` | `findSlot` — exact-match → nearest-pitch-within-layer; added `<cstdlib>` + `<limits>` |
| `Source/PluginProcessor.h` | New `CappedSynthesiser` class; `synthesiser` member changed from `juce::Synthesiser` → `CappedSynthesiser`; added `<atomic>` |
| `Source/PluginProcessor.cpp` | `processBlock` — `setVoiceCap` call before `renderNextBlock` |
| `.planning/REQUIREMENTS.md` | FUNC-03 + FUNC-04 rows annotated with "rectified stage-4 phase-2.1 reopen" |

No changes to: `MicrotonalSamplerVoice.{h,cpp}`, `LoopDetector.{h,cpp}`,
`SampleLoader.{h,cpp}`, `FilenameParser.{h,cpp}`, `MicrotonalSamplerSound.h`,
`PluginEditor.{h,cpp}`, `CMakeLists.txt`. Voice DSP, loop detection, loader,
and parser paths are untouched.

## Engineering verification

Triple build green (clean rebuild after edits) → cache-clear + reinstall
per `CLAUDE.md`. All Stage-4-grade automated checks re-run against the fixed
binary and pass:

| Check | Result |
|---|---|
| `pluginval --strictness-level 10 --skip-gui-tests --random-seed 0xc0ffee --timeout-ms 120000` | ✅ SUCCESS, 21 tests, 0 fail |
| `pluginval --strictness-level 10` (with GUI, same seed/timeout) | ✅ SUCCESS, 25 tests, 0 fail |
| `auval -v aumu OMtS OuDv` | ✅ AU VALIDATION SUCCEEDED |

Logs (overwritten with the post-fix run) at
`.planning/stages/4-polish/logs/{pluginval-10-skipgui.log, pluginval-10-withgui.log, auval.log}`.

## Pending — perceptual verification (user)

Engineering bar is green; the user-side perceptual verification is what closes
the reopen and unblocks Stage 4 Phase 4.2 / 4.3:

- [ ] **Single-note coverage:** load the `vln_long_mp-A#2-V127-T6N6.aif`
      folder; play single notes across the full range; every key in the
      layer's pitch-zone reach should produce sound (transposed from the
      nearest sampled pitch). Silence is acceptable only at velocities that
      route to a layer with no samples (here: vel ≤ 64 → empty layer 0).
- [ ] **Polyphony cap:** load a sustained-content folder; play a 16-note
      chord — all 16 should ring. Then drop the polyphony slider to 4 and
      re-play — only 4 should ring at any one time, oldest stolen first.
- [ ] **Voice-steal smoothness:** at the cap boundary, the 5 ms steal ramp
      should be inaudible at moderate velocity (regression check on D2-3 /
      Phase 2.4 voice-steal).

If all three pass, Stage 4 resumes from where it halted (Phase 4.2 PERF-02
measurement). If any fail, file the defect against the responsible
sub-phase per the PLAN failure-routing table.

## Plan deviations / observations

1. **No new module dependencies.** Both fixes are local to existing files;
   `modules.json` invariant from PLAN §"Constraints" still holds.
2. **No edits to Stage 2 audio-thread code paths beyond what FUNC-03 and
   FUNC-04 require.** Voice DSP, loop-detect, ADSR, loader audio path,
   filename parser, and resampler are all unchanged. PERF-01 frozen-path
   invariant is preserved for everything except the new
   findSlot-nearest-search (intent-equivalent — same call site, slightly
   different return logic).
3. **Pluginval-10 + auval re-run replaces the prior post-fixture-OFF run.**
   Logs in `.planning/stages/4-polish/logs/` reflect the canonical
   FUNC-03/FUNC-04-fixed binary, not the stale state from before this
   reopen.
4. **Why this wasn't caught earlier.** Stage 2 verify covered the in-memory
   fixture path (every-MIDI-note coverage; polyphony default = pre-allocated
   count = 16). Both defects required *real-world* configuration (sparse
   folder, lowered polyphony) to surface. Future Stage 2 plans should
   include at least one verification scenario with a *sparse* fixture and
   one with a *lowered* polyphony cap, to flush these blind spots.

## Resume

Stage 4 halts/resumes at: `.planning/stages/4-polish/AUTOMATED-CHECKS.md`
§"What is still pending (human-in-loop)". Next concrete step is the user's
perceptual verification (above), then Phase 4.2 PERF-02 measurement.
