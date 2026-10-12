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
#include "VenueFile.h"

namespace oo
{
namespace venuefile
{

namespace
{
    /** The wrapper `readFromState()` expects: it looks for a VENUE child of the tree it is given,
        so a bare VENUE tree has to be parented before it can be read.

        Deliberately reusing `readFromState()` rather than writing a second attribute reader here.
        A second reader is a second thing to keep in step with the schema, and the schema is the
        on-disk contract for every session saved from Phase 2.1 onwards — the ONE thing that must
        never have two implementations. What this TU adds is the VALIDATION in front of it, not a
        parallel deserialiser. ui_frontend_check.js section 29 asserts that no createXml / parseXML
        / writeTo appears anywhere outside this file.
    */
    juce::ValueTree parentOf (const juce::ValueTree& venue)
    {
        juce::ValueTree wrapper { juce::Identifier ("VENUEFILE") };
        wrapper.appendChild (venue.createCopy(), nullptr);
        return wrapper;
    }

    int countSpeakerChildren (const juce::ValueTree& venue) noexcept
    {
        int n = 0;

        for (int i = 0; i < venue.getNumChildren(); ++i)
            if (venue.getChild (i).hasType (VenueModel::speakerTag))
                ++n;

        return n;
    }

    /** v1.13.2 (v1.3.0 review IN-03) — does EVERY speaker resolve, and resolve UNAMBIGUOUSLY?

        Counting SPEAKER children is not enough. Eight children where two claim @index 5 and none
        claims 4, or one written `index="03"`, pass that count — and readFromState() then leaves the
        unresolved speaker at its placeholder default and the load reports `ok`. A room that is
        seven-eighths measured, presented as measured.

        The test is written as the LOOKUP readFromState() performs, not as a parallel reading of the
        schema: getChildWithProperty (propIndex, i + 1) — a juce::var equality, under which the
        attribute text "03" never equals 3 — then the positional fallback onto an UN-indexed SPEAKER
        child at that position. A claim is counted the same way, so "resolves here" and "resolves
        there" cannot disagree. Two children claiming one speaker is refused as ambiguous: the reader
        would take the first, and nothing on screen would say which. */
    bool everySpeakerResolves (const juce::ValueTree& venue)
    {
        for (int i = 0; i < VenueModel::kNumSpeakers; ++i)
        {
            const juce::var index (i + 1);
            int claims = 0;

            for (int c = 0; c < venue.getNumChildren(); ++c)
            {
                const auto child = venue.getChild (c);

                if (child.hasType (VenueModel::speakerTag)
                    && child.hasProperty (VenueModel::propIndex)
                    && child.getProperty (VenueModel::propIndex) == index)
                    ++claims;
            }

            if (claims > 1)
                return false;

            if (claims == 1)
                continue;

            if (i >= venue.getNumChildren())
                return false;

            const auto positional = venue.getChild (i);

            if (! positional.hasType (VenueModel::speakerTag)
                || positional.hasProperty (VenueModel::propIndex))
                return false;
        }

        return true;
    }

    /** v1.13.2 (v1.3.0 review IN-23) — @schemaVersion, when present, must be a positive integer.
        Through v1.13.1 `static_cast<int>` read "abc" as 0 and the load reported `ok`. Absent is
        still accepted as the current schema, which is what every hand-built file relies on. */
    bool readSchemaVersion (const juce::ValueTree& venue, int& version)
    {
        version = VenueModel::kSchemaVersion;

        if (! venue.hasProperty (VenueModel::propSchemaVersion))
            return true;

        const auto text = venue.getProperty (VenueModel::propSchemaVersion).toString().trim();

        if (text.isEmpty() || ! text.containsOnly ("0123456789"))
            return false;

        version = text.getIntValue();
        return version >= 1;
    }
} // namespace

//==============================================================================
bool save (const VenueModel& venue, const juce::File& target)
{
    const auto tree = venue.toValueTree();

    if (! tree.isValid())
        return false;

    const auto xml = tree.createXml();

    if (xml == nullptr)
        return false;

    // createDirectory() on the parent rather than assuming the chooser made it: a user who types a
    // path into the native dialog's name field can name a directory that does not exist yet.
    target.getParentDirectory().createDirectory();

    return xml->writeTo (target);
}

//==============================================================================
LoadResult load (const juce::File& source, VenueModel& out, int* fileVersion)
{
    if (! source.existsAsFile())
        return LoadResult::unreadable;

    const auto xml = juce::parseXML (source);

    if (xml == nullptr)
        return LoadResult::unreadable;

    const auto tree = juce::ValueTree::fromXml (*xml);

    // ── The two structural rejections, BEFORE `out` is touched ───────────────────────────────────
    //
    // A file that is not a venue at all, or one carrying three speakers, must leave the caller's
    // model exactly as it found it. This is the difference that matters against the session path:
    // readFromState() would happily produce a room with five measured speakers and three
    // placeholders, and nothing downstream — not the solve, not the plan, not the table — would
    // look any different from a correct load.
    if (! tree.isValid() || ! tree.hasType (VenueModel::venueTag))
        return LoadResult::malformedRoot;

    if (countSpeakerChildren (tree) < VenueModel::kNumSpeakers)
        return LoadResult::malformedRoot;

    // v1.13.2 — the two remaining ways a file could half-apply while reporting `ok` (see the
    // helpers above). Both are refused before `out` is touched, like the two checks above.
    if (! everySpeakerResolves (tree))
        return LoadResult::malformedRoot;

    // The root parsed, so the version is reportable even when it is one this build does not know.
    int version = VenueModel::kSchemaVersion;

    if (! readSchemaVersion (tree, version))
        return LoadResult::malformedRoot;

    if (fileVersion != nullptr)
        *fileVersion = version;

    // readFromState() resets to the §OQ4 defaults before reading, so `out` is fresh by construction
    // as well as by contract — an attribute the file omits falls back to a KNOWN default rather
    // than to whatever the caller's model happened to hold.
    const auto wrapper = parentOf (tree);
    out.readFromState (wrapper);

    // LOADED, THEN SURFACED. Refusing a forward file would be worse — the operator has the numbers
    // either way — but a silent best-effort load is the one option ruled out (Q6 / P56 rule 2).
    if (version > VenueModel::kSchemaVersion)
        return LoadResult::forwardVersion;

    return LoadResult::ok;
}

//==============================================================================
juce::String describe (LoadResult result, int fileVersion)
{
    switch (result)
    {
        case LoadResult::ok:
            return {};

        case LoadResult::forwardVersion:
        {
            // Built with << onto a NAMED local rather than onto the temporary: String::operator<<
            // takes an lvalue reference, so `juce::String ("...") << x` binds to the private
            // `operator bool` instead and fails with a message that names neither. Keeping the <<
            // habit is cheaper than remembering which lines are ASCII-safe.
            juce::String message ("venue file was written by a newer build (format ");

            message << fileVersion << ", this build writes " << VenueModel::kSchemaVersion
                    << ") - some values may be missing";

            return message;
        }

        case LoadResult::malformedRoot:
            return "not a venue file, or fewer than 8 speakers - nothing was changed";

        case LoadResult::unreadable:
            return "venue file could not be read";
    }

    return "venue file could not be read";
}

} // namespace venuefile
} // namespace oo
