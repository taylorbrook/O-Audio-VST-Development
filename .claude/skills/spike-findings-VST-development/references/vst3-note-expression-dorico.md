# VST3 Note Expression for Dorico Microtonal Playback

Reference distilled from spikes 001, 002, 003 (2026-04-22 → 2026-04-23). Validates that Dorico's tonality-system microtones can drive per-note tuning in JUCE-based VST3s via `Steinberg::Vst::kTuningTypeID` Note Expression events, despite JUCE 8.0.4 silently dropping those events in its wrapper.

Research origin: `.planning/notes/dorico-microtonal-vst-research.md` (Ouaricon, 2026-04-22).

## Validated Patterns

### Pattern 1 — JUCE patch uses Approach 2 (side-channel queue), not sysex tunneling

Two files in the local JUCE fork at `/Users/taylorbrook/JUCE/`:

1. `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` — add `Vst3RawEvent` struct + `virtual void onVst3RawEvent(...)` default no-op.
2. `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` — in the wrapper's process path, right before `MidiEventList::toMidiBuffer`, iterate `data.inputEvents` and forward NoteOn/NoteOff/NoteExpressionValue events to `pluginInstance->getVST3ClientExtensions()->onVst3RawEvent(...)`.

Full patch hunks at `sources/shared-code/juce-patch.md`. Both edits are commented `// JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)` for grep discoverability after JUCE upgrades.

**Why this works:**
- Upstream JUCE drops Note Expression events in `MidiEventList::toMidiBuffer` because they can't be represented as `MidiMessage`. Same function strips noteIds from NoteOn/NoteOff.
- Our patch captures the raw events before the lossy conversion. `toMidiBuffer` still runs after — MIDI flow is unchanged, so the rest of the plugin works exactly as it did pre-patch.
- No modification to `juce_VST3Common.h` needed. Research estimated ~30 LOC there; actual is 0.

### Pattern 2 — Plugin advertises NEC via `VST3ClientExtensions::queryIEditController`

```cpp
int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override
{
    if (Steinberg::FUnknownPrivate::iidEqual (targetIID,
                                              Steinberg::Vst::INoteExpressionController::iid))
    {
        nec.addRef();
        *obj = static_cast<Steinberg::Vst::INoteExpressionController*> (&nec);
        return Steinberg::kResultOk;
    }
    *obj = nullptr;
    return Steinberg::kNoInterface;
}
```

The `TuningNoteExpressionController` class implements `INoteExpressionController` and declares exactly one NE type: `kTuningTypeID`, bipolar, absolute, `[0,1]` → `[-120, +120]` semitones.

Full implementation at `sources/shared-code/NoteExpressionSupport.spike.h`. The spike version contains `neTrace` diagnostic logging — **strip all calls to `OLyrica::detail::neTrace(...)` and the `detail::neTrace` / `detail::iidToHex` helpers plus `#include <fstream>` before merging to production.**

### Pattern 3 — Processor drains the queue at the top of `processBlock`, before `renderNextBlock`

Full code at `sources/shared-code/processor-drain.cpp`. Two-pass over the block's raw events:

1. Build `std::map<noteId, MIDI pitch>` from NoteOns.
2. For each `kTuningTypeID` NE event, look up `pitch` via `noteId`, compute `semitones = 240.0 * (value - 0.5)`, `store` into `pendingTuningSemis[pitch]`.

Drain happens BEFORE `synthesiser.renderNextBlock` so voices see the pending tuning when their `startNote` fires.

### Pattern 4 — Voice consumes tuning with `exchange(0.0)` before DSP trigger

Full code at `sources/shared-code/voice-startNote.cpp`. In `startNote`, after the base frequency is computed but before the DSP model's trigger call:

```cpp
if (pendingTuningSource != nullptr && midiNoteNumber >= 0 && midiNoteNumber < 128)
{
    const double semis = (*pendingTuningSource)[(size_t) midiNoteNumber]
                              .exchange (0.0, std::memory_order_acq_rel);
    if (semis != 0.0)
        currentFrequency *= std::pow (2.0, semis / 12.0);
}
```

`exchange` over `load` + `store` prevents retriggered notes at the same pitch from inheriting a stale offset when a later block's NoteOn arrives without a new NE.

### Pattern 5 — Measurement math

VST3 `kTuningTypeID` normalized value → plain semitones:
```
plain_semitones = 240.0 * (normalized - 0.5)
```

| Dorico accidental | Representative NE value | Semitones | Cents |
|---|---|---|---|
| quarter-sharp | `0.497917` (confirmed) | `-0.5000` | `-50.0` |

Frequency conversion from cents: `freq_new = freq_old * std::pow(2.0, cents / 1200.0)`.

## Landmines

### Landmine 1 — Dorico does not use the NEC handshake

Across 2103 log lines in the validated Spike 002 trace, Dorico never queried `INoteExpressionController::iid` via `queryInterface`. Dorico sends NE events based **solely on the expression map's Microtonality setting**, not on the plugin's NEC advertisement.

Consequences:
- The `TuningNoteExpressionController` class is dead code for Dorico. Keep it anyway — other VST3 hosts (Cubase, Nuendo) may use it.
- Don't use NEC advertisement as a signal for "does this host support NE?". That info comes from per-host testing.

### Landmine 2 — Dorico represents microtonal notes by the *neighbor* semitone, not the named pitch

A quarter-sharp C4 in Dorico arrives as `pitch=61 (C#4), NE=-50¢`. Not `pitch=60, NE=+50¢`.

Final frequency is correct either way, but:
- Never derive tuning direction from NE sign.
- Never assume `NE value * pitch_delta_from_reference` correlates with accidental.
- **Always correlate NE events to NoteOns by `noteId`, never by MIDI pitch.**

Unverified but likely (real build must confirm):
- Quarter-flat C4 → `pitch=59 (B3), NE=+50¢`
- ¾-sharp C4 → `pitch=61 (C#4), NE=+50¢`? or `pitch=62 (D4), NE=-50¢`?

### Landmine 3 — End-user setup is non-trivial

**Default expression maps (NotePerformer, HSSE, HALion, etc.) route microtones to VST2 detune or pitch bend — neither reaches our VST3 plugin.** Users who "plug O-Lyrica into Dorico and play a quarter-sharp" hear 12-TET and assume the plugin is broken.

Required user procedure:
1. `Library → Expression Maps…`
2. Duplicate an existing map.
3. Set the duplicate's **Microtonality** dropdown to **"VST3 Note Expression"**.
4. In Play mode → VST Instruments → gear icon → Endpoint Setup, assign the new expression map to the Ouaricon plugin's channel.

Real build must ship:
- A pre-configured Dorico expression map file (`*.doricoexpmap`) in the installer, ideally also dropped into `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/`.
- Setup instructions in the README / release notes.
- Consider a first-run dialog on Standalone mode that warns "Dorico microtonal playback requires a dedicated expression map — see docs".

### Landmine 4 — Voice-side drain MUST happen before DSP trigger

The spike's `startNote` applies tuning to `currentFrequency` *then* calls `stringModel.trigger(currentFrequency, …)`. If a future refactor moves the trigger earlier (or calls a `setFrequency` after trigger), the first sample is rendered at the untuned pitch → audible glide / zipper at every quarter-tone onset.

Code comment on the pattern is mandatory. Test: quarter-sharp note should sound identical to plain note at the attack except for pitch — no click, no sweep.

### Landmine 5 — Diagnostic logging in the spike is NOT production-ready

`sources/shared-code/NoteExpressionSupport.spike.h` includes `OLyrica::detail::neTrace(...)` calls that write to `/tmp/olyrica-ne-trace.log` synchronously from the audio thread. This is acceptable for spiking but unacceptable for release:
- Synchronous file I/O on audio thread causes dropouts.
- `/tmp` writes leak data to other processes on the machine.
- `std::ofstream` with static lifetime holds an fd forever; deleting the file unlinks the inode but the fd stays — diagnosing this on the spike itself cost a cycle.

**Strip before merge:**
- Every `OLyrica::detail::neTrace(...)` call site in `NoteExpressionSupport.h`, `PluginProcessor.cpp`, `HarpSynthVoice.cpp`.
- The `OLyrica::detail::neTrace` function and `detail::iidToHex` helper.
- `#include <fstream>` in `NoteExpressionSupport.h`.

## Constraints

- **JUCE version:** validated on JUCE 8.0.4. Every JUCE upgrade requires reapplying the patch — no automated mechanism.
- **Format:** VST3 only. Note Expression does not exist in AU. AU builds should fall back to 12-TET or pitch bend (Dorico sends pitch bend for AU per research).
- **Platform:** spike validated on macOS (Darwin 25.3.0, Apple Silicon). Windows VST3 should work identically — the patch is in cross-platform wrapper code — but untested as of wrap-up date.
- **Host tested:** Dorico 6. Cubase / Nuendo / Wavelab untested but theoretically work (they use the NEC handshake, which our plugin exposes).
- **Block-locality assumption:** spike code assumes NE and NoteOn arrive in the same block. Dorico satisfies this; other hosts may not. Real build should maintain a persistent `noteId → voice` map across blocks for mid-note retuning support.
- **Cross-plugin reuse:** this pattern should be extracted to a shared Ouaricon module. All pitched plugins (O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-Lyrica) will want it. Pulling it into `modules/dsp/note-expression/` or similar is the obvious next step.
- **Refcount discipline:** `TuningNoteExpressionController::release()` just decrements without deleting. Safe because the NEC is owned by the extensions object, but a host that aggressively releases could send refCount negative. Not hit in Dorico 6 testing; watch for it on other hosts.
- **Custom NE type IDs reserved range:** `kCustomStart = 100000, kCustomEnd = 200000`. We use only `kTuningTypeID = 2` (predefined). Future additions (e.g., per-note timbre, per-note vibrato depth) should use custom IDs in that range.

## Origin

Synthesized from spikes: 001 (patch-build-load), 002 (quarter-sharp-end-to-end), 003 (attack-transient-check).
Source files in: `sources/001-patch-build-load/`, `sources/002-quarter-sharp-end-to-end/`, `sources/003-attack-transient-check/`, `sources/shared-code/`.

Upstream research: `.planning/notes/dorico-microtonal-vst-research.md`.
