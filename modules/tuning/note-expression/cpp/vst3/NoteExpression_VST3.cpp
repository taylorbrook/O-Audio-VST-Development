/*
   This file is part of the Ouaricon Audio note-expression module.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================
    NoteExpression_VST3.cpp — VST3-only translation unit for note-expression
    module v1.0.0.

    Plan 23-05 amended D-22 — two-TU split:
      - This file owns all `<pluginterfaces/...>` includes and all
        Steinberg-symbol references.
      - This file compiles ONLY into ${TARGET}_VST3 via the per-format routing
        in modules/cmake/OuariconModules.cmake (D-27). SharedCode does NOT
        compile this file, so non-VST3 link lines never see Steinberg symbols.
      - Companion SharedCode-bound TU (cpp/NoteExpression.cpp) owns the
        VST3Extensions ctor/dtor/drainAndUpdate (Steinberg-free).

    Contents:
      - Full Controller class body (NEC implementation; subclass of
        Steinberg::Vst::INoteExpressionController).
      - realControllerDelete — internal-linkage actual deleter, swapped into
        nec via the lazy-create idiom in queryIEditController.
      - VST3Extensions::queryIEditController body — references
        INoteExpressionController::iid; lazy-creates Controller on first call.
      - updatePendingFromEvents free helper — references kTuningTypeID; called
        by SharedCode's drainAndUpdate via the dispatch slot.
      - Static-init dispatcher: registers updatePendingFromEvents into the
        SharedCode TU's std::atomic<NEUpdateFn> g_neUpdate slot at TU load.

    Custom-deleter pimpl (D-21 amended) — load-bearing detail:
      - The header declares `std::unique_ptr<Controller, void(*)(Controller*)> nec;`
      - SharedCode's ctor initializes it with `noopControllerDelete`.
      - When THIS TU's queryIEditController is invoked (VST3 host only), it
        lazy-creates a Controller AND swaps in realControllerDelete via:
          nec = std::unique_ptr<Controller, void(*)(Controller*)>(
                    new Controller, &realControllerDelete);
      - The move-assignment atomically swaps both the managed pointer AND the
        deleter member of the unique_ptr. (Note: unique_ptr's deleter slot is
        immutable post-construction in the sense that you can't `.reset(ptr)`
        and pick a new deleter — `.reset()` keeps the existing deleter. The
        only way to install a different deleter is to construct a fresh
        unique_ptr and move-assign it.)
  ==============================================================================
*/

#include "../NoteExpression.h"

#include <pluginterfaces/vst/ivstnoteexpression.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/base/ustring.h>
#include <public.sdk/source/common/pluginview.h>

#include <cstdio>
#include <cstring>
#include <map>

namespace Ouaricon::NoteExpression
{

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
// realControllerDelete — internal-linkage real deleter. Swapped into nec by
// the lazy-create idiom in queryIEditController. Controller is complete in
// THIS TU, so `delete p` is a complete-type expression here.
//==============================================================================
namespace
{
    void realControllerDelete (Controller* p) noexcept
    {
        delete p;
    }
}

//==============================================================================
// updatePendingFromEvents — body of the free helper declared in
// NoteExpression.h. References Steinberg::Vst::kTuningTypeID, so its body
// must live in this VST3-only TU. Called by SharedCode's drainAndUpdate via
// the std::atomic<NEUpdateFn> dispatch slot (registered at static-init below).
//==============================================================================
void updatePendingFromEvents (
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
// vst3QueryIEditController — Steinberg-aware body for the q-slot dispatch.
// Registered into SharedCode's std::atomic<NEQueryFn> g_neQuery slot via the
// static-init below. SharedCode's VST3Extensions::queryIEditController loads
// the slot and forwards into this function.
//
// Friend access: this free helper needs to mutate VST3Extensions::nec, which
// is private. We use a const_cast pattern through a public accessor instead
// of declaring this function a friend in the header (which would force the
// header to mention this function by name and unique_ptr<Controller, void(*)>
// in a way the friend declaration can match exactly). Since VST3Extensions
// has no other state we mutate here besides nec, and the public lazy-create
// idiom is well-defined behavior, we instead expose a private member via a
// pointer-to-member workaround. Simpler: declare this function a friend in
// the header. Done — see header.
//
// Lazy-create swap-deleter idiom (LOAD-BEARING — D-21 amended):
//   The header declares `nec` as
//     std::unique_ptr<Controller, void(*)(Controller*)>
//   initialized in SharedCode's ctor with the no-op deleter. unique_ptr's
//   deleter member is fixed at construction; .reset(ptr) reuses the existing
//   deleter. The only way to install a different deleter is to construct a
//   fresh unique_ptr (with the new deleter and the new managed pointer) and
//   move-assign into nec — the move-assignment atomically swaps BOTH the
//   managed pointer AND the deleter slot.
//==============================================================================
namespace
{
    int32_t vst3QueryIEditController (VST3Extensions& self,
                                      const Steinberg::TUID targetIID,
                                      void** obj)
    {
        if (Steinberg::FUnknownPrivate::iidEqual (targetIID, Steinberg::Vst::INoteExpressionController::iid))
        {
            auto& nec = self._internalNecPimpl();
            if (! nec)
            {
                // Move-assign a fresh unique_ptr that carries (a) the freshly
                // constructed Controller and (b) the real deleter. This swaps both
                // slots atomically — replacing the noopControllerDelete that was
                // installed by SharedCode's ctor. Subsequent dtor of `nec` will
                // call realControllerDelete (which actually `delete`s the object,
                // because Controller is complete in THIS TU).
                nec = std::unique_ptr<Controller, void(*)(Controller*)> (
                          new Controller, &realControllerDelete);
            }
            nec->addRef();
            *obj = static_cast<Steinberg::Vst::INoteExpressionController*> (nec.get());
            return Steinberg::kResultOk;
        }

        *obj = nullptr;
        return Steinberg::kNoInterface;
    }
}

//==============================================================================
// Static-init dispatch registration (D-22 amended).
//
// Registers updatePendingFromEvents into the SharedCode-bound TU's
// std::atomic<NEUpdateFn> g_neUpdate slot at TU load. Static-init is correct
// here because:
//   (a) the VST3 host loads the wrapper before any audio-thread callback
//       fires — TU-load static initializers run during dlopen/LoadLibrary,
//       which completes before the host calls processBlock;
//   (b) registerNEUpdate is a single atomic store (release ordering), well-
//       defined under the C++ memory model; the audio-thread load in
//       drainAndUpdate uses paired acquire ordering.
//
// When this file is NOT linked (AU / Standalone / VST2 / AAX / LV2 / Unity
// — all non-VST3 builds), the static-init never runs, the slot stays nullptr,
// and SharedCode's drainAndUpdate skips correlation. Correct: non-VST3 hosts
// cannot deliver kTuningTypeID events.
//==============================================================================
namespace
{
    struct DispatchRegistrar
    {
        DispatchRegistrar() noexcept
        {
            registerNEUpdate (&updatePendingFromEvents);
            registerNEQuery  (&vst3QueryIEditController);
        }
    };

    [[maybe_unused]] static const DispatchRegistrar g_dispatchRegistrar;
}

} // namespace Ouaricon::NoteExpression
