/*
  ==============================================================================
    NoteExpression.h — note-expression module v1.0.0
    VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback.
    Header-only. Public API lives under the Ouaricon::NoteExpression nested namespace.
    Requires a local JUCE patch (see scripts/apply-juce-patches.sh).
  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>

#include <pluginterfaces/vst/ivstnoteexpression.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/base/ustring.h>
#include <public.sdk/source/common/pluginview.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

namespace Ouaricon::NoteExpression
{

//==============================================================================
/** 128-slot atomic table of pending NE tuning offsets, indexed by MIDI pitch.
    Owned by VST3Extensions; voices receive a pointer via setPendingTuningSource.
    Each slot stores a semitone delta (in 12-TET semitones, the unit Dorico uses
    when computing the offset against the EDO12 neighbour pitch).
*/
using PendingTuningTable = std::array<std::atomic<double>, 128>;

//==============================================================================
/** Consumes the pending NE tuning delta for `midiNoteNumber` and returns the
    new frequency. exchange(0.0) ensures a retriggered note at the same pitch
    in a later block does not inherit a stale offset. Encapsulates the
    pow(2, semis/12) call so voice code never invokes it directly. Composes multiplicatively with
    the caller's base frequency — caller should pass the frequency AFTER any
    TuningEngine / humanize lookup (D-10).
*/
inline double applyPendingTuning (PendingTuningTable& table,
                                  int                 midiNoteNumber,
                                  double              currentFrequency)
{
    if (midiNoteNumber < 0 || midiNoteNumber >= 128)
        return currentFrequency;

    const double semis = table[(size_t) midiNoteNumber]
                             .exchange (0.0, std::memory_order_acq_rel);
    if (semis == 0.0)
        return currentFrequency;

    return currentFrequency * std::pow (2.0, semis / 12.0);
}

//==============================================================================
/** Two-pass correlation: (1) build noteId -> midi-pitch map from NoteOns in
    the block; (2) for each kTuningTypeID NE, compute 240*(value-0.5) semitones
    and store into table[pitch]. Caller must call
    VST3Extensions::drainBlockEvents(events) immediately before this.

    Important: Dorico represents microtonal notes by the *nearest* neighbour
    semitone + an NE tuning delta (e.g. quarter-sharp C4 arrives as pitch=C#4
    with NE=-50c, not pitch=C4 with NE=+50c). Correlate via noteId, never via
    MIDI pitch arithmetic.
*/
inline void updatePendingFromEvents (
    const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
    PendingTuningTable&                                          table)
{
    if (events.empty()) return;

    using Kind = juce::VST3ClientExtensions::Vst3RawEvent::Kind;

    std::map<int32_t, int> noteIdToPitch;
    for (const auto& e : events)
        if (e.kind == Kind::NoteOn)
            noteIdToPitch[e.noteId] = (int) e.pitch;

    for (const auto& e : events)
    {
        if (e.kind != Kind::NoteExpressionValue)        continue;
        if (e.typeId != Steinberg::Vst::kTuningTypeID)  continue;

        auto it = noteIdToPitch.find (e.noteId);
        if (it == noteIdToPitch.end())                  continue;

        const int pitch = it->second;
        if (pitch < 0 || pitch >= 128)                  continue;

        // VST3 kTuningTypeID: norm [0,1] -> plain semitones [-120, +120].
        const double semitones = 240.0 * (e.value - 0.5);
        table[(size_t) pitch].store (semitones, std::memory_order_release);
    }
}

//==============================================================================
/** Advertises kTuningTypeID as a supported Note Expression for all (busIndex,
    channel) pairs. Dorico queries this on plugin load to decide whether to
    send NE or fall back to pitch bend.
*/
class Controller : public Steinberg::Vst::INoteExpressionController
{
public:
    Controller() = default;

    Steinberg::int32 PLUGIN_API getNoteExpressionCount (Steinberg::int32 /*busIndex*/,
                                                        Steinberg::int16 /*channel*/) override
    {
        return 1;
    }

    Steinberg::tresult PLUGIN_API getNoteExpressionInfo (Steinberg::int32 /*busIndex*/,
                                                         Steinberg::int16 /*channel*/,
                                                         Steinberg::int32 noteExpressionIndex,
                                                         Steinberg::Vst::NoteExpressionTypeInfo& info) override
    {
        if (noteExpressionIndex != 0)
            return Steinberg::kResultFalse;

        std::memset (&info, 0, sizeof (info));
        info.typeId = Steinberg::Vst::kTuningTypeID;
        Steinberg::UString (info.title,      128).assign (STR16 ("Tuning"));
        Steinberg::UString (info.shortTitle, 128).assign (STR16 ("Tun"));
        Steinberg::UString (info.units,      128).assign (STR16 ("semitones"));
        info.unitId = -1;
        info.valueDesc.defaultValue = 0.5;
        info.valueDesc.minimum      = 0.0;
        info.valueDesc.maximum      = 1.0;
        info.valueDesc.stepCount    = 0;
        info.associatedParameterId  = Steinberg::Vst::kNoParamId;
        info.flags = Steinberg::Vst::NoteExpressionTypeInfo::kIsBipolar
                   | Steinberg::Vst::NoteExpressionTypeInfo::kIsAbsolute;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API getNoteExpressionStringByValue (Steinberg::int32 /*busIndex*/,
                                                                  Steinberg::int16 /*channel*/,
                                                                  Steinberg::Vst::NoteExpressionTypeID id,
                                                                  Steinberg::Vst::NoteExpressionValue valueNormalized,
                                                                  Steinberg::Vst::String128 string) override
    {
        if (id != Steinberg::Vst::kTuningTypeID)
            return Steinberg::kResultFalse;

        // Format "+/-N.NN" as ASCII, then widen to char16. Avoids juce::String <-> UString
        // conversion pitfalls. This is display-only; Dorico doesn't rely on it.
        const double semitones = 240.0 * (valueNormalized - 0.5);
        char ascii[32];
        std::snprintf (ascii, sizeof (ascii), "%.2f", semitones);
        int i = 0;
        for (; i < 31 && ascii[i] != '\0'; ++i)
            string[i] = (Steinberg::Vst::TChar) (unsigned char) ascii[i];
        string[i] = 0;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API getNoteExpressionValueByString (Steinberg::int32 /*busIndex*/,
                                                                  Steinberg::int16 /*channel*/,
                                                                  Steinberg::Vst::NoteExpressionTypeID id,
                                                                  const Steinberg::Vst::TChar* string,
                                                                  Steinberg::Vst::NoteExpressionValue& valueNormalized) override
    {
        if (id != Steinberg::Vst::kTuningTypeID)
            return Steinberg::kResultFalse;

        // Narrow char16 -> ASCII for atof. Values only contain digits / '.' / '-' / '+'.
        char ascii[32] = {};
        for (int i = 0; i < 31 && string[i] != 0; ++i)
            ascii[i] = (char) (string[i] & 0x7F);
        const double semitones = std::atof (ascii);
        valueNormalized = juce::jlimit (0.0, 1.0, semitones / 240.0 + 0.5);
        return Steinberg::kResultTrue;
    }

    //==============================================================================
    // FUnknown / refcount plumbing — NEC is owned by the extensions object so the
    // refcount is effectively ignored, but Steinberg hosts may still query it.
    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) override
    {
        if (obj == nullptr)
            return Steinberg::kInvalidArgument;

        if (Steinberg::FUnknownPrivate::iidEqual (iid, Steinberg::Vst::INoteExpressionController::iid)
         || Steinberg::FUnknownPrivate::iidEqual (iid, Steinberg::FUnknown::iid))
        {
            *obj = static_cast<Steinberg::Vst::INoteExpressionController*> (this);
            addRef();
            return Steinberg::kResultOk;
        }

        *obj = nullptr;
        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef()  override { return ++refCount; }
    Steinberg::uint32 PLUGIN_API release() override { return --refCount; }

private:
    std::atomic<Steinberg::uint32> refCount { 1 };
};

//==============================================================================
/** VST3 client extensions for note-expression-aware plugins.
    - Advertises the Controller on IEditController queries.
    - Buffers raw VST3 events (pushed by the patched JUCE wrapper) so the
      processor can correlate NE tuning deltas with their NoteOn noteIds and
      apply per-voice pitch offsets before voices emit their first sample.
    - Owns the 128-slot PendingTuningTable (D-09): plugins do not need to
      re-declare it, voices receive a pointer via getPendingTable().
*/
class VST3Extensions : public juce::VST3ClientExtensions
{
public:
    VST3Extensions()
    {
        blockEvents.reserve (64);
        rawEventScratch.reserve (64);
    }

    int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override
    {
        const bool isNEC = Steinberg::FUnknownPrivate::iidEqual (
                                targetIID, Steinberg::Vst::INoteExpressionController::iid);

        if (isNEC)
        {
            nec.addRef();
            *obj = static_cast<Steinberg::Vst::INoteExpressionController*> (&nec);
            return Steinberg::kResultOk;
        }

        *obj = nullptr;
        return Steinberg::kNoInterface;
    }

    void onVst3RawEvent (const Vst3RawEvent& e) override
    {
        // Called on the audio thread just before processBlock. Plain push is
        // safe: drain happens on the same thread at the top of processBlock.
        // blockEvents is reserved to 64 slots in the constructor (T-23-02).
        blockEvents.push_back (e);
    }

    /** Called by the processor at the top of processBlock. Moves the current
        block's raw events to `out` and clears the internal buffer.
    */
    void drainBlockEvents (std::vector<Vst3RawEvent>& out)
    {
        out.clear();
        out.swap (blockEvents);
        blockEvents.clear();
        blockEvents.reserve (64);
    }

    /** Convenience: drain + correlate in one call. Plugins use this from
        processBlock before renderNextBlock. */
    void drainAndUpdate()
    {
        drainBlockEvents (rawEventScratch);
        updatePendingFromEvents (rawEventScratch, pendingTable);
    }

    /** Voice wiring entry point. Hand this to each voice's
        setPendingTuningSource(). */
    PendingTuningTable& getPendingTable() noexcept { return pendingTable; }

private:
    Controller                  nec;
    std::vector<Vst3RawEvent>   blockEvents;
    std::vector<Vst3RawEvent>   rawEventScratch;
    PendingTuningTable          pendingTable {};
};

} // namespace Ouaricon::NoteExpression
