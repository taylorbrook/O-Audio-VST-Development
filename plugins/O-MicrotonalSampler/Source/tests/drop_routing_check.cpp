/*
  ==============================================================================

    drop_routing_check.cpp
    O-MicrotonalSampler — v1.23.5 drag-drop hit-test routing tests.

    Manual run:
      ninja O-MicrotonalSampler_DropRoutingCheck \
        && ./build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_DropRoutingCheck
    Exit code = number of failed assertions (0 = all pass).

    What this exercises
    -------------------
    The pure XY→zone routing extracted into DropRouting.h (oms::hitTestDrop),
    the geometry that PluginEditor::filesDropped uses to decide whether a drop
    lands on a sample cell, the folder zone, or nowhere. Extracted from the
    inline v1.23.4 logic so it is testable without the WebView / message thread.

    Invariants (matching the v1.23.4 inline behaviour, plus the IN-02 guard):

      1. Cell hit — a point inside a published cell returns Cell + its index.
      2. Cell priority — the grid sits *below* the folder zone, so a point that
         overlaps both a cell and the zone routes to the Cell.
      3. Folder-zone hit — a point in the zone but no cell returns FolderZone.
      4. IN-02 guard — an EMPTY folder zone (w<=0 or h<=0) never matches, even
         at its origin. This is what disarms a stale rectangle once
         reportCellLayout resets folderZoneRect on an omitted/failed payload.
      5. Out-of-bounds — a point in neither returns None.
      6. Half-open intervals — the right/bottom edge (x == cx+w, y == cy+h) is
         excluded; the left/top edge is included.
      7. First-match-wins — with overlapping cells, the lowest index is returned.
      8. Empty grid + empty zone — returns None (no phantom routing).

  ==============================================================================
*/

#include <juce_core/juce_core.h>

#include "../DropRouting.h"

#include <iostream>
#include <string>

namespace
{
    int failed = 0;

    void check (bool cond, const std::string& desc)
    {
        if (cond)
            std::cout << "  PASS: " << desc << "\n";
        else
        {
            std::cout << "  FAIL: " << desc << "\n";
            ++failed;
        }
    }

    oms::CellRect cell (int midi, int layer, int x, int y, int w, int h)
    {
        oms::CellRect c;
        c.midiNote = midi; c.velocityLayer = layer;
        c.x = x; c.y = y; c.w = w; c.h = h;
        return c;
    }
}

int main()
{
    using oms::DropZone;

    std::cout << "drop_routing_check — oms::hitTestDrop routing matrix\n";

    // A small 2-cell grid: cell A at (0,100,50,50), cell B at (50,100,50,50).
    // Folder zone spans the top strip (0,0,200,80).
    juce::Array<oms::CellRect> grid;
    grid.add (cell (60, 0,  0, 100, 50, 50));   // index 0 — cell A
    grid.add (cell (61, 0, 50, 100, 50, 50));   // index 1 — cell B

    const int fzX = 0, fzY = 0, fzW = 200, fzH = 80;

    // 1. Cell hit — centre of cell B.
    {
        const auto h = oms::hitTestDrop (grid, fzX, fzY, fzW, fzH, 75, 125);
        check (h.zone == DropZone::Cell && h.cellIndex == 1,
               "1. point in cell B → Cell, index 1");
    }

    // 1b. Cell hit — centre of cell A.
    {
        const auto h = oms::hitTestDrop (grid, fzX, fzY, fzW, fzH, 25, 125);
        check (h.zone == DropZone::Cell && h.cellIndex == 0,
               "1b. point in cell A → Cell, index 0");
    }

    // 2. Cell priority — a cell overlapping the zone wins. Build a grid whose
    //    cell overlaps the folder zone, then drop inside the overlap.
    {
        juce::Array<oms::CellRect> overlap;
        overlap.add (cell (60, 0, 10, 10, 40, 40));   // sits inside the zone
        const auto h = oms::hitTestDrop (overlap, fzX, fzY, fzW, fzH, 20, 20);
        check (h.zone == DropZone::Cell && h.cellIndex == 0,
               "2. overlap of cell + zone → Cell wins (grid below zone)");
    }

    // 3. Folder-zone hit — top strip, above the grid, no cell there.
    {
        const auto h = oms::hitTestDrop (grid, fzX, fzY, fzW, fzH, 100, 40);
        check (h.zone == DropZone::FolderZone && h.cellIndex == -1,
               "3. point in zone (no cell) → FolderZone");
    }

    // 4. IN-02 guard — an empty zone never matches.
    {
        const auto hz = oms::hitTestDrop (grid, 0, 0, 0, 80, 5, 5);   // w == 0
        check (hz.zone == DropZone::None,
               "4a. zero-width zone → None (stale-rect guard)");
        const auto hh = oms::hitTestDrop (grid, 0, 0, 200, 0, 5, 5);  // h == 0
        check (hh.zone == DropZone::None,
               "4b. zero-height zone → None (stale-rect guard)");
        const auto ho = oms::hitTestDrop (grid, 0, 0, 0, 0, 0, 0);    // origin, empty
        check (ho.zone == DropZone::None,
               "4c. empty zone at origin, point (0,0) → None");
    }

    // 5. Out-of-bounds — right of everything.
    {
        const auto h = oms::hitTestDrop (grid, fzX, fzY, fzW, fzH, 500, 500);
        check (h.zone == DropZone::None && h.cellIndex == -1,
               "5. point in neither cell nor zone → None");
    }

    // 6. Half-open intervals. Cell A is [0,50) x [100,150).
    {
        // Left/top edge included.
        const auto in = oms::hitTestDrop (grid, fzX, fzY, fzW, fzH, 0, 100);
        check (in.zone == DropZone::Cell && in.cellIndex == 0,
               "6a. left/top edge (0,100) included → Cell A");
        // Right edge x==50 belongs to cell B (its left edge), not A.
        const auto rightEdge = oms::hitTestDrop (grid, fzX, fzY, fzW, fzH, 50, 125);
        check (rightEdge.zone == DropZone::Cell && rightEdge.cellIndex == 1,
               "6b. x==50 is cell B's left edge, not cell A's right");
        // Bottom edge y==150 excluded (falls through the grid; y in zone? no,
        // zone is y<80). So it is None.
        const auto bottomEdge = oms::hitTestDrop (grid, fzX, fzY, fzW, fzH, 25, 150);
        check (bottomEdge.zone == DropZone::None,
               "6c. bottom edge y==150 excluded → None");
    }

    // 7. First-match-wins — two cells covering the same point, lower index wins.
    {
        juce::Array<oms::CellRect> dup;
        dup.add (cell (60, 0, 0, 0, 100, 100));   // index 0
        dup.add (cell (61, 0, 0, 0, 100, 100));   // index 1 — same rect
        const auto h = oms::hitTestDrop (dup, 0, 0, 0, 0, 50, 50);
        check (h.zone == DropZone::Cell && h.cellIndex == 0,
               "7. overlapping cells → lowest index wins");
    }

    // 8. Empty grid + empty zone — nothing routes.
    {
        juce::Array<oms::CellRect> none;
        const auto h = oms::hitTestDrop (none, 0, 0, 0, 0, 10, 10);
        check (h.zone == DropZone::None,
               "8. empty grid + empty zone → None");
    }

    std::cout << (failed == 0 ? "\nALL PASS\n"
                              : "\n" + std::to_string (failed) + " FAILED\n");
    return failed;
}
