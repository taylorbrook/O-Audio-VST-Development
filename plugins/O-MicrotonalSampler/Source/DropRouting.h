/*
  ==============================================================================

    DropRouting.h
    O-MicrotonalSampler — pure drag-drop hit-test geometry.

    Extracted in v1.23.5 (IN-02 / IN-03) from PluginEditor::filesDropped so the
    XY→zone routing can be unit-tested in isolation (drop_routing_check.cpp)
    without dragging the WebView / message thread into a console harness.

    The routing matrix (unchanged from the inline v1.23.4 logic):
      1. Cell hit-test wins first — the sample grid sits *below* the folder
         zone, so an overlapping point routes to the cell.
      2. Folder-zone hit-test second. An *empty* zone (w<=0 || h<=0) never
         matches — this is the guard that IN-02 relies on: once
         reportCellLayout resets folderZoneRect on an omitted payload, a stale
         rectangle can no longer mis-route a later drop.
      3. Otherwise → None (silent reject).

    All hit-tests use half-open intervals [x, x+w) / [y, y+h) — the right/bottom
    edge is excluded, matching the original `x < c.x + c.w` comparisons.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oms
{
    // Cell layout shadow published by JS via the reportCellLayout native
    // function. Editor stores a juce::Array<CellRect>; hit-tested on drop.
    struct CellRect { int midiNote = 0, velocityLayer = 0, x = 0, y = 0, w = 0, h = 0; };

    enum class DropZone { None, Cell, FolderZone };

    struct DropHit
    {
        DropZone zone     = DropZone::None;
        int      cellIndex = -1;   // valid only when zone == Cell
    };

    // Route a drop at (x, y) against the published cell grid and folder zone.
    // folderZone is given as (x, y, w, h); a non-positive w or h means "no
    // zone" and is never matched. Pure — no editor / WebView state touched.
    inline DropHit hitTestDrop (const juce::Array<CellRect>& cells,
                                int fzX, int fzY, int fzW, int fzH,
                                int x, int y) noexcept
    {
        // 1. Cell hit-test (highest priority — grid sits below the zone).
        for (int i = 0; i < cells.size(); ++i)
        {
            const auto& c = cells.getReference (i);
            if (x >= c.x && x < c.x + c.w && y >= c.y && y < c.y + c.h)
                return { DropZone::Cell, i };
        }

        // 2. Folder-zone hit-test (empty zone never matches).
        if (fzW > 0 && fzH > 0
            && x >= fzX && x < fzX + fzW
            && y >= fzY && y < fzY + fzH)
            return { DropZone::FolderZone, -1 };

        // 3. Out-of-bounds.
        return { DropZone::None, -1 };
    }
}
