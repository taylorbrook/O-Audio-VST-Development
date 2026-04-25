/*
  ==============================================================================
    NoteExpression.cpp — SharedCode-bound translation unit for note-expression
    module v1.0.0.

    Plan 23-05 amended D-22 — two-TU split:
      - This file (cpp/NoteExpression.cpp) is picked up by the new non-recursive
        `file(GLOB cpp/*.cpp ...)` glob in OuariconModules.cmake and links into
        SharedCode (libO-Lyrica-dev_SharedCode.a). Because PluginProcessor (in
        SharedCode) holds `vst3Extensions` as a value member and calls
        `drainAndUpdate()` from processBlock, the SharedCode link line MUST be
        able to resolve VST3Extensions::VST3Extensions(), ~VST3Extensions(),
        and drainAndUpdate(). Those definitions live here.
      - The companion file (cpp/vst3/NoteExpression_VST3.cpp) carries the
        Steinberg-touching code (Controller body, queryIEditController,
        updatePendingFromEvents) and is routed only into ${TARGET}_VST3.

    This file is required to be Steinberg-symbol-free. The non-VST3 link lines
    (AU, Standalone, VST2, AAX, LV2, Unity) link this file via SharedCode but
    DO NOT link pluginterfaces, so any pluginterfaces-symbol reference here
    would re-introduce the D-23-04-A undefined-symbol failure class.

    Dispatch-slot pattern:
      - This TU owns `std::atomic<NEUpdateFn> g_neUpdate {nullptr}`.
      - drainAndUpdate() loads the slot and dispatches if non-null.
      - When the VST3 TU is linked, its static-init calls
        registerNEUpdate(&updatePendingFromEvents) at TU load — populating the
        slot with the Steinberg-aware NE-correlation body.
      - When the VST3 TU is NOT linked (AU/Standalone), the slot stays nullptr
        and drainAndUpdate skips correlation. This is correct: non-VST3 hosts
        cannot deliver kTuningTypeID events anyway.

    Custom-deleter pimpl (D-21 amended):
      - VST3Extensions::nec is `std::unique_ptr<Controller, void(*)(Controller*)>`.
      - This file initializes `nec(nullptr, &noopControllerDelete)` in the ctor.
      - The dtor calls the function pointer, NOT `delete nec.get()`, so it can
        compile without seeing Controller's body. When nec is null,
        noopControllerDelete is a no-op.
      - The VST3 TU's queryIEditController lazy-creates the Controller via
        move-assignment with realControllerDelete (defined in that TU), which
        atomically swaps both the managed pointer AND the deleter.
  ==============================================================================
*/

#include "NoteExpression.h"

#include <atomic>

namespace Ouaricon::NoteExpression
{

//==============================================================================
// noopControllerDelete — internal-linkage no-op deleter.
//
// Used as the initial deleter for `nec` in VST3Extensions's ctor. Only ever
// invoked when nec holds a nullptr (e.g. AU/Standalone builds where the VST3
// TU's lazy-create swap-deleter pattern never runs). With nullptr managed
// pointer, the unique_ptr's dtor still calls the deleter; this no-op accepts
// the call and does nothing.
//
// Internal linkage (anonymous namespace): the function pointer's address is
// taken in the ctor below, so the symbol must exist in this TU but does not
// need external visibility.
//==============================================================================
namespace
{
    void noopControllerDelete (Controller* /*p*/) noexcept
    {
        // No-op. Real deletion happens via realControllerDelete defined in
        // cpp/vst3/NoteExpression_VST3.cpp, which is installed via swap-deleter
        // during lazy-create in queryIEditController.
    }

    //==========================================================================
    // Dispatch slot for NE-correlation hand-off (D-22 amended).
    //
    // Non-zero only when the VST3-only TU has been linked (its static-init
    // registers updatePendingFromEvents into this slot). Audio-thread readers
    // use acquire ordering paired with the VST3 TU's release-store registration.
    //==========================================================================
    std::atomic<NEUpdateFn> g_neUpdate { nullptr };
}

//==============================================================================
// registerNEUpdate — namespace-scoped (external linkage). Called by the VST3
// TU's static-init at TU load. Single-write expected; release-store semantics
// pair with the audio-thread acquire-load in drainAndUpdate.
//==============================================================================
void registerNEUpdate (NEUpdateFn fn) noexcept
{
    g_neUpdate.store (fn, std::memory_order_release);
}

//==============================================================================
// VST3Extensions out-of-line members — SharedCode-bound (D-22 amended).
//
// These three symbols MUST link from SharedCode because PluginProcessor (in
// SharedCode) holds `vst3Extensions` as a value member and calls
// drainAndUpdate() from processBlock. Defining them here keeps the AU /
// Standalone / VST2 / AAX / LV2 / Unity link lines resolvable without pulling
// in any Steinberg symbol.
//==============================================================================

VST3Extensions::VST3Extensions()
    : nec (nullptr, &noopControllerDelete)   // custom-deleter pimpl per D-21 amended
{
    // Reserves match the previous header inline ctor (T-23-02: no audio-thread
    // allocation; pre-reserve to 64 events per block).
    blockEvents.reserve (64);
    rawEventScratch.reserve (64);
}

VST3Extensions::~VST3Extensions() = default;
// = default works because nec's deleter is a function pointer (the stored
// noopControllerDelete or the swapped-in realControllerDelete), NOT a default
// `delete Controller*`. The unique_ptr dtor instantiation only needs to call
// the function pointer — Controller's body is irrelevant at this site.

void VST3Extensions::drainAndUpdate()
{
    drainBlockEvents (rawEventScratch);

    // Dispatch via slot. Non-null iff the VST3 TU is linked and its static-init
    // registered updatePendingFromEvents. Acquire ordering pairs with the VST3
    // TU's release-store via registerNEUpdate.
    if (auto fn = g_neUpdate.load (std::memory_order_acquire))
        fn (rawEventScratch, pendingTable);

    // Non-VST3 builds: fn is null, correlation is skipped. Correct because
    // non-VST3 hosts cannot deliver kTuningTypeID events to this plugin.
}

} // namespace Ouaricon::NoteExpression
