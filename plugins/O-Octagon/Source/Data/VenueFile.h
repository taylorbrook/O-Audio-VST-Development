/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>

#include "VenueModel.h"

namespace oo
{
namespace venuefile
{

/**
    `.venue` file I/O — the 42 measured values, on disk, under a name the operator chose.

    ── WHY THIS IS ITS OWN TU AND NOT A VenueModel MEMBER ────────────────────────────────────────
    No processor reference, and nothing here needs one. `juce::File`, `ValueTree` and `createXml()`
    all live inside `juce_core` + `juce_data_structures`, which is exactly the narrow link line the
    unit target already has (Gate 11: no `juce_dsp`, no `juce_gui_extra`, no `juce_audio_processors`).
    So this file joins the FAST test target, and probe BN — UI-01 criterion 3(a) — runs there rather
    than behind a plugin.

    ── WHY A FILE IS NOT SESSION STATE (RESEARCH-3.2 Q6) ─────────────────────────────────────────
    `VenueModel::readFromState()` is deliberately forgiving: `@schemaVersion` is read but never
    branched on, and every absent attribute falls back to THE VALUE THE MODEL ALREADY HELD. That is
    right for session state, where the tree came from this plugin and the model was just
    default-constructed beside it.

    It is WRONG for a file. A `.venue` written by a future build, loaded straight into the LIVE
    model, produces a room that is partly measured and partly whatever was there before — with
    nothing on screen to distinguish it from a correct load. In a hall that is unrecoverable,
    because the operator has no way to tell which numbers are theirs.

    So this API imposes three rules the session path does not need:

      1. **Load into a FRESH model, never the live one.** A missing attribute then falls back to a
         known §OQ4 default rather than to a neighbouring venue's value, so the worst case is
         legible rather than plausible.
      2. **A forward `@schemaVersion` loads, but is SURFACED.** Refusing a forward file is worse
         than loading it — the operator has the numbers either way — but silent best-effort is the
         one option to rule out.
      3. **A root that is not `VENUE`, or that carries fewer than 8 `SPEAKER` children, is rejected
         OUTRIGHT** and `out` is not touched at all.

    Message thread only. PERF-01: there is no file I/O anywhere in `processBlock`, and both entry
    points here are user-initiated.
*/

//==============================================================================
enum class LoadResult
{
    ok,               ///< Parsed, validated and read into `out`.
    forwardVersion,   ///< Read into `out`, but @schemaVersion is NEWER than this build writes.
    malformedRoot,    ///< Not a VENUE root, or fewer than 8 SPEAKER children. `out` is UNTOUCHED.
    unreadable        ///< Missing file, or the XML did not parse. `out` is UNTOUCHED.
};

/** The canonical extension and the chooser's wildcard. One definition, used by the file dialogs
    and by the probes, so a rename cannot leave the two disagreeing. */
inline constexpr const char* kFileExtension = ".venue";
inline constexpr const char* kFileWildcard  = "*.venue";

//==============================================================================
/** Writes the venue's own subtree — `VenueModel::toValueTree()` — as XML.

    @returns false if the file could not be written. Nothing else can fail: the tree is built from
             the model, so there is no serialisation path here that could disagree with the one
             session state uses.
*/
bool save (const VenueModel& venue, const juce::File& target);

/** Loads `source` into `out`.

    @param out          MUST be a model the caller is willing to have overwritten. It is never the
                        live venue: the editor loads into a local and applies the result through
                        `applyVenueEditChecked()`, so a file whose labels do not resolve is rejected
                        by the same guard a typed label is.
    @param fileVersion  optional; receives `@schemaVersion` whenever the root parsed, INCLUDING on
                        `forwardVersion`. Untouched on `unreadable`.
*/
LoadResult load (const juce::File& source, VenueModel& out, int* fileVersion = nullptr);

/** A human-readable reason for the banner. ASCII only, and built with `<<` where it is not a
    literal: `juce::String (const char*)` converts through `CharPointer_ASCII` and mangles every
    byte above 127 with no compiler warning (critical_juce_string_char_ctor_is_ascii_only). */
juce::String describe (LoadResult result, int fileVersion);

} // namespace venuefile
} // namespace oo
