---
name: Dorico Microtonal VST Research
description: How Dorico communicates per-note microtonal pitch to VSTs, why JUCE 8 drops these events, and the concrete implementation pattern needed for our plugins
type: note
date: 2026-04-22
topic: microtonal-playback
related_plugins: [O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, all-pitched]
---

# Dorico Microtonal VST Research

Exploration of how Dorico communicates per-note microtonal pitch (from custom accidentals / tonality systems) to VST instruments, and what our JUCE-based plugins need to implement to receive it.

## TL;DR

- **Dorico sends microtonal data via one of three wire mechanisms** (selected per expression map): VST3 Note Expression (`kTuningTypeID`), VST2 per-note detune, or MIDI pitch bend. **Not MPE, not MTS-ESP.**
- **HALion works** because it's Steinberg VST3 and uses VST3 Note Expression. **Pianoteq works** because Dorico specifically routes microtonal deltas through VST2 `detune`. Most other VSTs don't handle either cleanly.
- **For our JUCE plugins, VST3 Note Expression is the target.** But **JUCE 8.0.4 silently drops `kNoteExpressionValueEvent`** in its VST3 wrapper — confirmed by direct source inspection. Implementing this requires a small JUCE patch + plugin-side code.
- **MTS-ESP is not a substitute** for our use case. MTS-ESP is scale-based and global; it doesn't carry per-note deltas from Dorico's tonality system custom accidentals. Adopting MTS-ESP would defeat the goal.

## Part 1: How Dorico communicates microtones

Dorico routes microtonal data through one of three mechanisms, chosen per expression map via the **Microtonality** setting (Library → Expression Maps → Microtonality):

| Setting | Wire format | Used by |
|---|---|---|
| **VST3 Note Expression** | VST3 `kTuningTypeID` events (per-note, normalized `[0,1]` → `[-120, +120]` semitones) | HALion, HALion Sonic, Retrologue, Padshop, Groove Agent, all Steinberg VST3 |
| **VST2 Detune** | VST2 per-note `detune` parameter on `VstMidiEvent` | Pianoteq, NotePerformer (Dorico detects these by name) |
| **Pitch Bend** | Standard MIDI CC pitch bend on the note's channel | Universal fallback. Monophonic per channel — chords with differing inflections require separate channels |
| **Auto** (default) | Dorico picks Note Expression for Steinberg VST3, VST2 detune for Pianoteq/NotePerformer, else pitch bend | — |

Tuning deltas are defined in the **Tonality System** (Write → Key Signatures) in tenths of a cent; internally Dorico uses 12000 divisions/octave. Pitch-bend mode requires the user to configure bend range (in semitones) in the expression map.

**Not used on the wire:** MPE, MTS single-note tuning sysex, MTS bulk tuning dumps, MTS-ESP. Dorico 6 (as of 2025) has no native MPE or MTS-ESP support.

## Part 2: The VST3 Note Expression path (the HALion path)

### Why it's the right target for us

- Per-`noteId` addressable — polyphonic microtonal chords work natively without channel gymnastics
- It's the "preferred" path Dorico picks automatically for Steinberg VST3s
- Value is a clean normalized float: `plain = 240 * (norm - 0.5)` maps `0.5 → 0 semitones`, `1.0 → +120`, `0.0 → -120`
- VST3-only — not available on AU builds

### The JUCE problem

**JUCE 8.0.4 does not forward Note Expression events to `AudioProcessor::processBlock`.** Verified by direct source inspection of local JUCE at `/Users/taylorbrook/JUCE`:

| File | Line(s) | Issue |
|---|---|---|
| `modules/juce_audio_processors/format_types/juce_VST3Common.h` | 1506-1544 | `MidiEventList::toMidiMessage` has no case for `kNoteExpressionValueEvent`. Returns empty `Optional<MidiMessage>` → silently dropped. |
| `modules/juce_audio_processors/format_types/juce_VST3Common.h` | 1176-1190 | `toMidiBuffer` iterates `IEventList`, only adds non-empty optionals. NE events filtered here. |
| `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` | 3697-3698 | Only call site. Raw `data.inputEvents` is never exposed elsewhere. |
| `modules/juce_audio_processors/format_types/juce_VST3Common.h` | 1510-1518 | Incoming `kNoteOnEvent` → `MidiMessage` strips `noteId`. Without preserving it, NE events can't be correlated to playing voices. |
| `modules/juce_audio_processors/format_types/juce_VST3Common.h` | 1298-1319 | Outgoing `createNoteOnEvent` / `createNoteOffEvent` hardcode `noteId = -1`. (Not relevant for receiving, only for sending.) |

Zero references to `NoteExpression`, `noteExpression`, `kTuningTypeID`, or `INoteExpressionController` in `juce_audio_plugin_client_VST3.cpp`. JUCE makes no attempt to advertise or forward note expression.

**The VST3 SDK shipped in JUCE 8.0.4 fully supports it** — `format_types/VST3_SDK/pluginterfaces/vst/ivstnoteexpression.h` defines everything. The plumbing simply isn't wired on the wrapper side. JUCE's Q3 2025 roadmap doesn't include it.

## Part 3: Implementation pattern

Three parts must be built:

### (a) Advertise support — `INoteExpressionController` on the edit controller

Custom class implementing `Vst::INoteExpressionController`. Declares a single NE type: `kTuningTypeID`, bipolar, absolute, range `[0,1]` → `[-120, +120]` semitones.

```cpp
class TuningNoteExpressionController : public Vst::INoteExpressionController
{
    int32 PLUGIN_API getNoteExpressionCount(int32, int16) override { return 1; }

    tresult PLUGIN_API getNoteExpressionInfo(int32, int16, int32 idx,
                                             Vst::NoteExpressionTypeInfo& info) override
    {
        if (idx != 0) return kResultFalse;
        memset(&info, 0, sizeof(info));
        info.typeId = Vst::kTuningTypeID;
        UString(info.title, 128).assign(STR16("Tuning"));
        UString(info.shortTitle, 128).assign(STR16("Tun"));
        UString(info.units, 128).assign(STR16("semitones"));
        info.unitId = -1;
        info.valueDesc.defaultValue = 0.5;
        info.valueDesc.minimum = 0.0;
        info.valueDesc.maximum = 1.0;
        info.associatedParameterId = Vst::kNoParamId;
        info.flags = Vst::NoteExpressionTypeInfo::kIsBipolar
                   | Vst::NoteExpressionTypeInfo::kIsAbsolute;
        return kResultTrue;
    }

    // getNoteExpressionStringByValue / getNoteExpressionValueByString: format semitones for display
    DECLARE_FUNKNOWN_METHODS
};
IMPLEMENT_FUNKNOWN_METHODS(TuningNoteExpressionController,
                           Vst::INoteExpressionController,
                           Vst::INoteExpressionController::iid)
```

### (b) Wire into JUCE — `VST3ClientExtensions::queryIEditController`

No JUCE patching needed for the discovery side. JUCE's edit controller consults `VST3ClientExtensions::queryIEditController` before its own interface table (verified at `juce_audio_plugin_client_VST3.cpp:872-881`).

```cpp
class MyExtensions : public juce::VST3ClientExtensions
{
    int32_t queryIEditController(const Steinberg::TUID targetIID, void** obj) override
    {
        if (FUnknownPrivate::iidEqual(targetIID, Vst::INoteExpressionController::iid))
        {
            nec.addRef();
            *obj = static_cast<Vst::INoteExpressionController*>(&nec);
            return Steinberg::kResultOk;
        }
        *obj = nullptr;
        return -1;
    }
private:
    TuningNoteExpressionController nec;
};
```

In `PluginProcessor.h`:
```cpp
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &extensions; }
private:
    MyExtensions extensions;
```

### (c) Receive events at process-time — **requires JUCE patch**

This is the unavoidable cost. Two proposed approaches:

**Approach 1: SysEx tunneling** (minimal JUCE change, slightly hacky)

Patch `juce_VST3Common.h` around line 1531 to convert `kNoteExpressionValueEvent` into a private-SysEx `MidiMessage` (manufacturer `0x7D`, reserved for non-commercial) carrying `{typeId, noteId, value}`. Also patch the noteOn/noteOff cases at lines 1510-1518 to emit a parallel sentinel SysEx carrying the VST3 `noteId` immediately before the standard MIDI message.

In `processBlock`, scan `MidiBuffer` for the sentinel, decode, correlate noteIds to voices, route tuning deltas.

**Approach 2: Side-channel ring buffer** (cleaner separation, similar patch footprint)

Add a thread-safe lock-free SPSC queue to the `VST3ClientExtensions` object. Patch the wrapper's `process()` prologue in `juce_audio_plugin_client_VST3.cpp:3697` to push raw NE events into the queue before `toMidiBuffer` is called. Processor drains the queue at the top of `processBlock`. Keeps MIDI buffer clean.

### (d) Voice routing

Each voice needs:
```cpp
int vst3NoteId = -1;
double tuningSemitones = 0.0;
```

On noteOn: capture the pending noteId (from the preceding SysEx / queue entry), store on voice.
On NE tuning event for that noteId: `voice.tuningSemitones = 240.0 * (value - 0.5);` → apply as frequency ratio `std::pow(2.0, semi / 12.0)` to the oscillator each block.

### Gotchas

- **Retuning stability at voice start.** Dorico typically emits the NE value event at the same `sampleOffset` as `noteOn`. The voice allocator must consume the pending tuning BEFORE synthesizing the first sample, or you get a pitch glide/zipper at note attack. Pre-scan each buffer's events, bucket by note-ID, apply tunings to nascent voices first.
- **MPE mode is not related.** Enabling `setSupportsMPE(true)` does nothing for `kTuningTypeID`. But the voice-allocation patterns from MPE (one voice per note-ID with independent pitch state) are directly reusable.
- **Edit-controller vs. component separation.** `INoteExpressionController` lives on the `IEditController` (GUI side), not the audio component. JUCE's edit controller is auto-generated; we inject the NE controller via `queryIEditController`. This works because our controller is state-free (describes types only).
- **AU is unaffected.** Note Expression is VST3-only. Dorico users running our plugins should use the VST3 for microtonal playback. (Open question: what does Dorico fall back to on AU? Likely pitch bend — see research questions.)
- **Dorico falls back silently.** If `INoteExpressionController` is absent on plugin load, Dorico uses its VST2-detune code path (which our plugins lack) and produces no microtonal output. Discovery side must work before events are even sent.

## Part 4: Why not MTS-ESP?

MTS-ESP (ODDSound) is the de facto microtonal standard across Reaper/Bitwig/Ableton/Cubase, and many popular plugins implement it (Surge XT, Vital, u-he synths, Pianoteq). It's ~20 lines of client code vs. ~250 for the VST3 NE path plus a JUCE patch. Tempting.

**But it doesn't solve our use case.** MTS-ESP is a scale-based, globally-shared retuning protocol: the master plugin broadcasts a 128-note tuning table, clients retune MIDI notes to match. It does **not** carry per-note deltas from Dorico's tonality system custom accidentals. A user who wanted microtones via MTS-ESP would have to:

1. Load an ODDSound MTS-ESP Master alongside our plugin
2. Manually configure a scale in the master
3. Ensure Dorico's pitches map correctly to scale degrees

This defeats the whole point of using Dorico's tonality system to drive playback from notation.

MTS-ESP would still be useful for Reaper/Bitwig users who want microtonal tuning independently — but it's an orthogonal feature, not a replacement for the Dorico NE path.

## Part 5: Summary of implementation cost

| Piece | Cost |
|---|---|
| `INoteExpressionController` implementation | ~80 LOC, plugin-side |
| `VST3ClientExtensions` subclass | ~30 LOC, plugin-side |
| Voice-side tuning storage + per-block frequency adjustment | ~100 LOC across voice/synth |
| JUCE patch to `juce_VST3Common.h` | ~30 LOC, in local JUCE fork |
| **Total** | **~240 LOC + maintained JUCE patch** |

Maintenance cost: re-apply the JUCE patch whenever JUCE updates. Tractable — we already use a local JUCE at `/Users/taylorbrook/JUCE`.

Once the pattern is proven on one plugin, extraction into a shared module (`Ouaricon` module system already used in this repo) makes propagation cheap.

## Decision

**Direction locked: VST3 Note Expression (`kTuningTypeID`).** Pitch-bend fallback explicitly out of scope.

**Next step: spike on O-Lyrica** to prove the full pipeline end-to-end before generalizing — see the spike artifact.

## Sources (confidence levels)

### HIGH (direct source inspection)
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/format_types/juce_VST3Common.h` lines 1176-1190, 1298-1319, 1506-1544
- `/Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` lines 355-372, 872-881, 3666-3715
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h`
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst/ivstnoteexpression.h`

### HIGH (Steinberg / Dorico primary sources)
- Steinberg VST3 Dev Portal — [INoteExpressionController change history](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Change+History/3.5.0/INoteExpressionController.html)
- Dorico forum: [Microtonal Playback & Input](https://forums.steinberg.net/t/microtonal-playback-input/1006380)
- Dorico forum: [Dorico 6 Microtonal MPE / Pitch-Bend MIDI Export](https://forums.steinberg.net/t/dorico-6-microtonal-mpe-pitch-bend-midi-export/1024843)
- Dorico forum: [Pianoteq, VST3 tuning and playback templates](https://forums.steinberg.net/t/pianoteq-vst3-tuning-and-playback-templates/856196)
- Dorico forum: [VST3s that work with microtonal playback](https://forums.steinberg.net/t/vst3s-that-work-with-microtonal-playback/110279)
- Steinberg: [Edit Tonality System dialog](https://archive.steinberg.help/dorico_pro/v5/en/dorico/topics/library/library_tonality_systems_edit_tonality_system_dialog_r.html)

### MEDIUM (community / secondary)
- [Scoring Notes — Microtonal playback in Dorico](https://www.scoringnotes.com/reviews/microtonal-playback-in-dorico/)
- [JUCE forum — VST3 Note Expression etc](https://forum.juce.com/t/vst3-note-expression-etc/31544)
- [JUCE issue #902 — custom VST3 interfaces](https://github.com/juce-framework/JUCE/issues/902)
- [JUCE VST3ClientExtensions docs](https://docs.juce.com/master/structjuce_1_1VST3ClientExtensions.html)
- [MTS-ESP GitHub (ODDSound)](https://github.com/ODDSound/MTS-ESP)
