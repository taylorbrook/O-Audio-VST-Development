---
title: "O-MicrotonalSampler — Case A audit charter (load → voice → audio pipeline)"
created: 2026-04-28
phase: 2.1-audit
status: open; deep_audit_required
parent: PHASE-2.1-REOPEN-SUMMARY.md
opened_by: user request after Phase 2.1 reopen (commit 4d20d42) shipped engineering-green but failed user perceptual verification with Case A pattern
---

# Case A audit charter — only MIDI 51 (D#3) and 52 (E3) produce audio

## Symptom

After loading `/Users/taylorbrook/Documents/samples/vln_long_mp/`
(42 violin sample files, all named `vln_long_mp-{Pitch}-V127-{XXXX}.aif`,
chromatic coverage G2..C6 = MIDI 43..84) into the **post-fix** binary
(commit `4d20d42`, with `findSlot` nearest-pitch + `CappedSynthesiser`
polyphony enforcement):

- Sample Map UI shows **42 yellow squares** in the second-to-bottom row
  (= velocity layer 1, since `(NUM_LAYERS-1) - row = 4-1-2 = 1`).
- "Skipped files" surface shows **0 entries**.
- Playing keys above velocity 64 produces audio **only at MIDI 51 (D#3)
  and MIDI 52 (E3)**.
- All other keys, including exact-match recorded pitches like C4
  (MIDI 60), D3 (MIDI 50), F3 (MIDI 53), G3 (MIDI 55), etc., are
  silent.
- Below velocity 65, all keys silent (this part is correct — empty
  layer 0).

## Why this is non-trivial

With my Phase 2.1 reopen `findSlot` change (nearest-pitch within layer)
**every key in velocity ≥ 65** must produce sound — the layer-1 search
finds *some* slot for any MIDI input, and `computePlayRateForSlot`
repitches it. The fact that even **C4 (exact match in the loaded map)**
is silent rules out:

- The nearest-pitch fallback being broken (exact match wouldn't need it)
- A velocity-routing bug (vel ≥ 65 → layer 1 lookup confirmed)
- An out-of-range gate (C4 is well within MIDI 43..84)

So either the slots **don't actually contain valid audio** despite the
UI showing them as loaded, or the voice **isn't actually reading them**
the way the code suggests. The fact that **exactly two adjacent MIDI
notes** sound (51, 52) is the signal — there's something pinning the
voice to one specific slot or address range.

## What's been ruled out

| Hypothesis | Evidence ruling out |
|---|---|
| Phase 2.1 in-memory test fixture still active | `build/CMakeCache.txt` shows `OMTS_PHASE_2_1_TEST_FIXTURE:BOOL=OFF`; clean rebuild + cache-clear + reinstall completed |
| Pre-fix binary still in memory | User did clean `Cmd+Q` of the host, verified no stale process via `ps aux`, reopened, retested. Bundle timestamps fresh (16:59 on 2026-04-28). Velocity-layer routing behaves consistently with the post-fix binary (numLayers=2 detection working). |
| Filename parser dropping files | UI shows 42 yellow squares; skipped-files surface shows 0; manual trace through `FilenameParser::parse("vln_long_mp-A#2-V127-T6N6")` correctly yields `(midi=46, layer=1)` |
| `findSlot` nearest-pitch logic broken in a way that returns nullptr | Even exact-match queries (e.g., MIDI 60 for the C4 file) return silence — exact-match path doesn't need the nearest fallback at all |
| Velocity → layer routing | Confirmed by user: vel ≥ 65 → layer 1 (where samples loaded) routes correctly; vel ≤ 64 → empty layer 0 (correctly silent) |
| `MicrotonalSamplerSound::appliesToNote` rejecting notes | Returns `true` for any note (`MicrotonalSamplerSound.h:20`) |
| Engineering-bar regression | pluginval --strictness-level 10 (skip-gui + with-gui) and auval all pass against the post-fix binary; the bug is data-shape-specific, not API-shape |

## Open hypotheses (audit targets)

### H1 — `SampleSlot::audio` shared_ptr is set up but the buffer isn't actually populated

**Where to look:** `Source/SampleLoader.cpp` `processOneFile`
(specifically the audio decode + SR-convert path). Lines 170-181 set up
the buffer; lines 175 / 180-181 do `copyFrom (workBuf, ...)`. Verify:

- Is `workBuf` actually filled with PCM samples before the copy?
- Does `outNumSamples` reflect the actual decoded frame count, or is it
  zero / mis-clamped for most files?
- Are stereo files / 24-bit aif files / specific bit depths failing
  silently inside the JUCE reader without `processOneFile` returning
  false?
- The user's files are `.aif` at 24-bit / 44.1 or 48 kHz / stereo (likely).
  Test with the actual files: open one in JUCE's `AudioFormatReader` and
  verify `read(...)` populates the buffer.

### H2 — `SampleMap::slots` vector emplacement order changes which slot is at `slots[0]`

D#3 (MIDI 51) and E3 (MIDI 52) being the only audible notes is suspicious.
Are they the **first two slots** in `builtSlots` after enumeration? Does
something in the voice's render path read only `slots[0]` or `slots[1]`?

The macOS file enumeration order from `ls`:
A#2, A#3, A#4, A#5, A2, A3, A4, A5, B2..B5, C#3, C#4, C#5, C3..C6,
**D#3, D#4, D#5, D3..D5, E3..E5**, F#3..F#5, F3..F5, G#2..G#5, G2..G5

D#3 is iteration index 19 (0-based); E3 is iteration index 25. They are
**not** at slots[0]. So this hypothesis weakens — but worth verifying
which `s.midiNote` values are actually present in the map.

### H3 — Voice or sound's instance state pins to one slot for the lifetime of the voice

Could `slotLow` be cached across notes incorrectly, e.g., set once on
the first note-on and never re-resolved? Trace
`MicrotonalSamplerVoice::startNote` (lines ~424-590) and verify
`slotLow` is reassigned every call.

D#3/E3 sounding could indicate: the voice was first triggered with one
of those notes (during initial test or load), `slotLow` got pinned, and
subsequent notes inherit that pinning but the voice is mostly silent
because `currentMidiNote` doesn't match.

### H4 — `MicrotonalSamplerVoice::canPlaySound` mismatch

`canPlaySound` returns `true` only for the right `MicrotonalSamplerSound`
type. If somehow the voices are `dynamic_cast`-failing for most note
queries, JUCE's Synthesiser would skip the voice for those notes. But
this would cause ALL voices to fail — inconsistent with "two notes
work".

Probably weak hypothesis but cheap to verify.

### H5 — Audio buffer pointer is being read from the wrong shared_ptr level

`SampleSlot::audio` is `std::shared_ptr<juce::AudioBuffer<float>>`. The
voice does `slot->audio.get()` then `lowBuf->getReadPointer(0)`. If
`audio` is somehow null for most slots (despite `processOneFile`
returning success and the slot being added to `builtSlots`), the voice
silently silences (`renderNextBlock` lines 691-707).

`renderNextBlock` line 701 explicitly silences if `readLowL == nullptr`
or `slotLowN <= 0`. So if `lowBuf` is null OR has zero samples for most
slots, those notes go silent — UI would still show yellow squares
(slot exists) but no audio.

This is the strongest hypothesis. Audit `processOneFile` for any path
that:

1. Returns `true` (slot gets added to `builtSlots` → yellow square)
2. Leaves `outSlot.audio` null OR with `getNumSamples() == 0`

Look for:
- Early-return paths after partial buffer setup
- SR-convert path that produces a buffer with 0 frames
- Lagrange resampler dropping frames silently
- Stereo-to-mono / mono-to-stereo path that overwrites the populated
  buffer with an empty one

### H6 — D#3 / E3 are the only files at 44.1 kHz (or some other SR) and the SR-convert path silently zeros the others

Per the file naming, all files look identical (V127, mp dynamic). But
the `T6N6` / `KINM` / etc. tail suffixes might encode tempo / take info,
not necessarily sample rate. Worth confirming.

### H7 — D#3 / E3 are the *exact* sample rates the host uses; resampler corrupts everything else

If host SR is 48 kHz and most source files are 44.1, the SR-convert
path (`juce::LagrangeInterpolator` per channel at load time, D2-9)
must successfully produce a populated output buffer. If the resampler
silently fails for non-equal SRs and produces zero output, only files
already at 48 kHz would survive — D#3 and E3 might be those.

This is testable cheaply: query each file's SR via `afinfo`:

```bash
for f in /Users/taylorbrook/Documents/samples/vln_long_mp/*.aif; do
    afinfo "$f" 2>&1 | grep -E "sample rate|format" | head -2
    echo "  $f"
done
```

If D#3 / E3 differ from the rest, that's the smoking gun.

## Audit charter for fresh context

A fresh-context investigation should:

1. **Read** the following files in this order:
   - `.planning/STATUS.md` (current state)
   - This file (`CASE-A-AUDIT-CHARTER.md`)
   - `.planning/stages/2-dsp/PHASE-2.1-REOPEN-SUMMARY.md` (recent work)
   - `Source/SampleLoader.cpp` (especially `processOneFile`,
     SR-convert, mono/stereo conversion)
   - `Source/SampleMap.h` (post-fix `findSlot`)
   - `Source/MicrotonalSamplerVoice.cpp` (`startNote` lines ~424-590,
     `renderNextBlock` lines ~637-790, especially the
     null/empty-buffer silence path at 701-708)
   - `Source/PluginProcessor.cpp` (synth setup, processBlock,
     `setVoiceCap` call site)
   - `Source/MicrotonalSamplerSound.h`

2. **Run the H7 sample-rate audit** (one-line bash command above) —
   cheapest and most likely smoking gun.

3. **Add a one-shot diagnostic dump** in the loader's completion
   callback (or `loadSampleFolder` finalisation) that writes per-slot
   `(midi, layer, audio.get() != nullptr, audio->getNumSamples(),
   audio->getNumChannels(), sourceSampleRate)` to
   `/tmp/microtonal-sampler-load-dump.json`. User reloads the folder,
   pastes the dump. This nails H1 and H5 without instrumentation noise
   in shipping code (strip after diagnosis).

4. **Trace one concrete note** end to end: user plays C4 at vel 100.
   Walk through:
   - `OMicrotonalSamplerAudioProcessor::processBlock` MIDI dispatch
   - `CappedSynthesiser::noteOn` (active count, steal logic)
   - `juce::Synthesiser::noteOn` → `findFreeVoice` → voice picked
   - `MicrotonalSamplerVoice::startNote` (vel layer, findSlot, playRate)
   - `renderNextBlock` (slotLow null check, audio buffer read)

5. **Identify the exact code path** that silences C4 despite the slot
   having been emplaced in the map.

6. **Patch + verify** with a real-folder reload (this folder is the
   reproducer). The fix lands as **Phase 2.1 second reopen** (separate
   commit from `4d20d42`), with REQUIREMENTS.md / STATUS.md updates
   and a `PHASE-2.1-SECOND-REOPEN-SUMMARY.md`.

## Reproducer

- **Plugin:** `O-MicrotonalSampler-dev` (post-fix binary at commit
  `4d20d42`); `OMTS_PHASE_2_1_TEST_FIXTURE=OFF`.
- **Folder:** `/Users/taylorbrook/Documents/samples/vln_long_mp/` (42
  files, chromatic G2..C6, layer `mp` = velocity layer 1).
- **Steps:**
  1. Launch `build/.../Standalone/O-MicrotonalSampler-dev.app`
     (or load AU/VST3 in a DAW).
  2. Drag-drop the folder into the Sample Map tab.
  3. Confirm 42 yellow squares appear in second-to-bottom row.
  4. Confirm 0 skipped files.
  5. Play C4 at velocity 100 → expected: violin tone at C4.
     Actual: silence.
  6. Play D#3 / E3 at velocity 100 → expected and actual: violin tone.
- **Engineering bar (must remain green after the fix):**
  - `pluginval --strictness-level 10 --skip-gui-tests --random-seed 0xc0ffee --timeout-ms 120000`
  - `pluginval --strictness-level 10` (with GUI, same seed/timeout)
  - `auval -v aumu OMtS OuDv`

## Constraints (must hold after the fix)

- All Stage 2 D2-* decisions remain in force.
- No reintroduction of the `OMTS_PHASE_2_1_TEST_FIXTURE` cache as the
  default.
- No regression of Phase 2.1 reopen fixes (commit `4d20d42`):
  `findSlot` nearest-pitch + `CappedSynthesiser` polyphony enforcement
  must continue to work.
- No new latency (`setLatencySamples` invariant: single comment-only
  hit at `PluginProcessor.cpp:133`).
- No new module dependencies in `modules.json`.
- Cross-platform WebView2 flags preserved.

## Resume protocol

After a fresh-context session diagnoses + fixes Case A:

1. Atomic commit Phase 2.1 second reopen.
2. Update `.planning/STATUS.md` to clear Case A.
3. User re-runs perceptual verification (single-note coverage,
   16-voice chord, polyphony cap test).
4. If green, resume Stage 4 from Phase 4.2 (PERF-02 Logic Pro CPU
   meter measurement).

This file (`CASE-A-AUDIT-CHARTER.md`) can be left as a forensic
artefact, or moved into the second-reopen summary, or archived after
close-out — your call.
