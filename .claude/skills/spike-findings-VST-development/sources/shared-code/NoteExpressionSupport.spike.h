/*
  ==============================================================================

    NoteExpressionSupport.h
    VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback.

    Spike 001 (2026-04-22) — header-only support module. Relies on a local JUCE
    patch that adds VST3ClientExtensions::onVst3RawEvent() to surface raw VST3
    events (NE + noteIds) before they're flattened to a MidiBuffer.
    See: .planning/notes/dorico-microtonal-vst-research.md

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
#include <fstream>
#include <mutex>
#include <vector>

namespace OLyrica
{

namespace detail
{
    // Spike 002 diagnostic trace. Lives at /tmp/olyrica-ne-trace.log.
    // Safe to call from audio thread for spike purposes (locked ofstream).
    // Remove this machinery before real build.
    inline void neTrace (const juce::String& msg)
    {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock (mtx);
        static std::ofstream log ("/tmp/olyrica-ne-trace.log", std::ios::app);
        log << juce::Time::getCurrentTime().toISO8601 (true).toStdString()
            << " " << msg.toStdString() << "\n";
        log.flush();
    }

    inline juce::String iidToHex (const char* iid)
    {
        juce::String s;
        for (int i = 0; i < 16; ++i)
            s += juce::String::toHexString ((juce::uint8) iid[i]).paddedLeft ('0', 2);
        return s;
    }
}

//==============================================================================
/** Advertises kTuningTypeID as a supported Note Expression for all (busIndex, channel)
    pairs. Dorico queries this on plugin load to decide whether to send NE or fall
    back to pitch bend.
*/
class TuningNoteExpressionController : public Steinberg::Vst::INoteExpressionController
{
public:
    TuningNoteExpressionController() = default;

    Steinberg::int32 PLUGIN_API getNoteExpressionCount (Steinberg::int32 busIndex,
                                                        Steinberg::int16 channel) override
    {
        detail::neTrace (juce::String::formatted ("[NEC] getNoteExpressionCount busIndex=%d channel=%d -> 1",
                                                  (int) busIndex, (int) channel));
        return 1;
    }

    Steinberg::tresult PLUGIN_API getNoteExpressionInfo (Steinberg::int32 busIndex,
                                                         Steinberg::int16 channel,
                                                         Steinberg::int32 noteExpressionIndex,
                                                         Steinberg::Vst::NoteExpressionTypeInfo& info) override
    {
        detail::neTrace (juce::String::formatted ("[NEC] getNoteExpressionInfo busIndex=%d channel=%d idx=%d",
                                                  (int) busIndex, (int) channel, (int) noteExpressionIndex));
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

        // Format "±N.NN" as ASCII, then widen to char16. Avoids juce::String <-> UString
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

        // Narrow char16 → ASCII for atof. Values only contain digits / '.' / '-' / '+'.
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
/** O-Lyrica VST3 extensions.
    - Advertises the TuningNoteExpressionController on IEditController queries.
    - Buffers raw VST3 events (pushed by the patched JUCE wrapper) so the
      processor can correlate NE tuning deltas with their NoteOn noteIds and
      apply per-voice pitch offsets before voices emit their first sample.
*/
class LyricaVST3Extensions : public juce::VST3ClientExtensions
{
public:
    LyricaVST3Extensions() { blockEvents.reserve (64); }

    int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override
    {
        const bool isNEC = Steinberg::FUnknownPrivate::iidEqual (
                                targetIID, Steinberg::Vst::INoteExpressionController::iid);

        detail::neTrace (juce::String::formatted ("[EXT] queryIEditController iid=%s isNEC=%d",
                                                  detail::iidToHex (targetIID).toRawUTF8(),
                                                  isNEC ? 1 : 0));

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
        using K = Vst3RawEvent::Kind;
        const char* kindStr = e.kind == K::NoteOn ? "NoteOn"
                            : e.kind == K::NoteOff ? "NoteOff"
                            : "NoteExpressionValue";
        detail::neTrace (juce::String::formatted (
            "[RAW] kind=%s sampleOffset=%d noteId=%d pitch=%d channel=%d typeId=%u value=%.6f",
            kindStr, (int) e.sampleOffset, (int) e.noteId,
            (int) e.pitch, (int) e.channel,
            (unsigned) e.typeId, e.value));

        // Called on the audio thread just before processBlock. Plain push is
        // safe: drain happens on the same thread at the top of processBlock.
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

private:
    TuningNoteExpressionController nec;
    std::vector<Vst3RawEvent>      blockEvents;
};

} // namespace OLyrica
