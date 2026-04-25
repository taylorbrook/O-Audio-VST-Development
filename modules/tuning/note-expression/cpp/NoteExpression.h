/*
  ==============================================================================
    NoteExpression.h — note-expression module v1.0.0
    VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback.
    Public API lives under the Ouaricon::NoteExpression nested namespace.

    Translation-unit segregation (Plan 23-05 amended D-22) — two-TU split:
      - This header is Steinberg-free. SharedCode (used by AU / Standalone /
        VST2 / AAX / LV2 / Unity link lines) can include it without pulling in
        any VST3-SDK symbol.
      - cpp/NoteExpression.cpp (SharedCode-bound) defines VST3Extensions ctor
        and dtor and drainAndUpdate. Zero Steinberg refs. Links into every
        format's link line.
      - cpp/vst3/NoteExpression_VST3.cpp (VST3-only via OuariconModules.cmake
        per-format routing) defines the Controller body, queryIEditController,
        updatePendingFromEvents, and a static-init that registers
        updatePendingFromEvents into the SharedCode TU's dispatch slot.

    Custom-deleter pimpl (D-21 amended):
      - VST3Extensions::nec is std::unique_ptr<Controller, void(*)(Controller*)>.
      - SharedCode's ctor initializes nec(nullptr, &noopControllerDelete) — a
        no-op deleter defined in cpp/NoteExpression.cpp.
      - The VST3 TU's queryIEditController lazy-creates the Controller via
        move-assignment with realControllerDelete (defined in that TU), which
        atomically swaps both the managed pointer AND the deleter slot of nec.
      - The custom function-pointer deleter is what allows the SharedCode-bound
        dtor (= default) to compile without seeing Controller's body — the
        unique_ptr dtor calls a function pointer, never `delete Controller*`
        directly, so Controller's definition is irrelevant at the dtor
        instantiation site.
  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
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
    pow(2, semis/12) call so voice code never invokes it directly. Composes
    multiplicatively with the caller's base frequency — caller should pass the
    frequency AFTER any TuningEngine / humanize lookup (D-10).

    This helper is Steinberg-free and therefore stays inline in the header.
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
// Forward declarations — full bodies live in cpp/vst3/NoteExpression_VST3.cpp.

/** NEC implementation. Forward-declared so the header carries no Steinberg
    SDK references; full body in cpp/vst3/NoteExpression_VST3.cpp.
*/
class Controller;

/** Two-pass correlation: (1) build noteId -> midi-pitch map from NoteOns in
    the block; (2) for each kTuningTypeID NE, compute 240*(value-0.5) semitones
    and store into table[pitch]. Caller must call
    VST3Extensions::drainBlockEvents(events) immediately before this.

    Defined out-of-line in cpp/vst3/NoteExpression_VST3.cpp because the
    kTuningTypeID reference must not leak into SharedCode (Plan 23-05).
*/
void updatePendingFromEvents (
    const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
    PendingTuningTable&                                          table);

//==============================================================================
// Dispatch-slot wiring (Plan 23-05 amended D-22).
//
// SharedCode-bound cpp/NoteExpression.cpp owns a std::atomic<NEUpdateFn>
// dispatch slot. The VST3-only cpp/vst3/NoteExpression_VST3.cpp registers its
// updatePendingFromEvents body into that slot via static-init at TU load.
// SharedCode's drainAndUpdate loads the slot and dispatches if non-null.
// Non-VST3 builds: VST3 TU is not linked, slot stays nullptr, correlation is
// skipped — correct because non-VST3 hosts cannot deliver kTuningTypeID events.
using NEUpdateFn = void (*) (
    const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>&,
    PendingTuningTable&);

void registerNEUpdate (NEUpdateFn fn) noexcept;

//==============================================================================
/** VST3 client extensions for note-expression-aware plugins.
    - Advertises the Controller on IEditController queries (VST3 only — body
      lives in cpp/vst3/NoteExpression_VST3.cpp).
    - Buffers raw VST3 events (pushed by the patched JUCE wrapper) so the
      processor can correlate NE tuning deltas with their NoteOn noteIds and
      apply per-voice pitch offsets before voices emit their first sample.
    - Owns the 128-slot PendingTuningTable (D-09): plugins do not need to
      re-declare it, voices receive a pointer via getPendingTable().

    The class itself is fully usable from non-VST3 builds (AU / Standalone /
    VST2 / AAX / LV2 / Unity) — those builds simply never invoke
    queryIEditController, the VST3 TU is not linked, the dispatch slot stays
    nullptr, and drainAndUpdate is a fast pass-through.

    Custom-deleter pimpl (D-21 amended): nec is
        std::unique_ptr<Controller, void(*)(Controller*)>
    which lets the SharedCode-bound dtor (= default in cpp/NoteExpression.cpp)
    compile without seeing Controller's body. The deleter starts as
    noopControllerDelete (cpp/NoteExpression.cpp); queryIEditController's
    lazy-create idiom in the VST3 TU swaps it to realControllerDelete via
    move-assignment.
*/
class VST3Extensions : public juce::VST3ClientExtensions
{
public:
    VST3Extensions();
    ~VST3Extensions() override;

    int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override;

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
        processBlock before renderNextBlock. Defined out-of-line in
        cpp/NoteExpression.cpp; dispatches via the dispatch slot (D-22). */
    void drainAndUpdate();

    /** Voice wiring entry point. Hand this to each voice's
        setPendingTuningSource(). */
    PendingTuningTable& getPendingTable() noexcept { return pendingTable; }

private:
    // Custom function-pointer deleter pimpl (D-21 amended). The deleter slot
    // is set by the ctor (in cpp/NoteExpression.cpp) to noopControllerDelete;
    // the VST3 TU swaps it to realControllerDelete via move-assignment in
    // queryIEditController's lazy-create. The dtor (= default) only invokes
    // the stored function pointer — Controller's body is not required here.
    std::unique_ptr<Controller, void(*)(Controller*)> nec;

    std::vector<Vst3RawEvent>   blockEvents;
    std::vector<Vst3RawEvent>   rawEventScratch;
    PendingTuningTable          pendingTable {};
};

} // namespace Ouaricon::NoteExpression
