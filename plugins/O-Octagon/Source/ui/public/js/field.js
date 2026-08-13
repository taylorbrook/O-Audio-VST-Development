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
// ============================================================================
// O-Octagon — the DBAP field gradient (Stage 3 Phase 3.3, UI-04)
//
// THE RISK THIS MODULE EXISTS TO AVOID IS "BEAUTIFUL AND WRONG": a picture the
// solver does not produce is worse than no picture, because it is believed.
// Everything below is therefore arranged so that NOTHING IS DERIVED HERE. The
// plugin samples the FULL CHAIN — shaper::shape -> hull-project if outside ->
// dbap::solve -> hullTrimGain — through the shipping functions, quantises to
// 8 bits and base64s it; this file decodes and blits. UI-04 criterion 1's
// twenty-point comparison against a direct solve to 1e-3 is a C++ UNIT PROBE
// (CB) on the sampler's FLOAT output, strictly upstream of this transport, so
// the quantisation cannot weaken it.
//
// ── WHY THE QUANTITY IS 1/k = sqrt(denom), NOT max_i v_i^2 (P69 / N10) ────
// ROADMAP named max_i v_i^2. It is DISQUALIFIED BY MEASUREMENT, not by taste.
// DBAP normalises to sum v_i^2 = 1, so that expression measures CONCENTRATION,
// not level: it is IDENTICALLY 1.0000 at every point in the room whenever
// exactly one weight is non-zero, and 3.2-5.4 dB otherwise. The picture would
// go blank precisely when the spatial situation is most extreme. 1/k is what
// the solver already computes as `denom` before normalising, gives 1.3-10.4 dB
// with correct radial structure, and never degenerates.
//
// ── WHY THE COLOUR MAP IS NORMALISED AND THE SPAN IS PRINTED ──────────────
// The field over a raked audience plane is GENUINELY FLAT: every grid point is
// at z = 0 while the speakers are 4.50-5.40 m up, so the minimum 3-D distance
// is >= 4.5 m in a 12 x 15 m hall. An absolute 0..1 colour map renders a
// uniform wash WHILE LOOKING AS THOUGH IT CARRIES INFORMATION. So the ramp is
// normalised to the per-recompute observed min/max the plugin returned, and the
// actual dB span goes in the legend beside the plan caption. Without the number
// the picture is decoration.
//
// ── 32 x 40, AND THE CONSTRAINT IS THE PAYLOAD, NOT THE MATHS (Q2) ────────
// Measured against the shipping solver: pow is NOT the bottleneck — even
// 112 x 140 (125,440 pow) is 660 us on the message thread. THE BRIDGE IS. A
// float-per-cell JSON payload is 61 kB per recompute through a transport that
// serialises every value; 32 x 40 quantised to 8 bits and base64'd is 1.7 kB.
//
// atob -> Uint8Array -> putImageData onto a 32 x 40 OFFSCREEN canvas ->
// drawImage scaled onto #plan-backdrop. That satisfies UI-04 criterion 3's
// "offscreen canvas and blitted" DIRECTLY rather than by argument, and the
// browser's own smoothing is what turns a coarse grid into a gradient.
//
// ── THE RECOMPUTE IS COALESCED, AND THE COUNTER IS IN C++ (N12 / P73) ─────
// UI-04 criterion 2 says "on geometry/weight change only". GainStage shows
// THREE more inputs the criterion does not name — rolloff -> a, blur -> r_s and
// hullAtten -> the hull trim — and all three are AUTOMATABLE AT AUDIO RATE, so
// a literal "recompute on change" makes a blur ramp recompute every block.
// app.js marks the field dirty and the existing 2 Hz status poll spends it, so
// there is at most one recompute per tick.
//
// srcX / srcY / srcZ / width are genuinely NOT inputs, which is why the
// criterion's assertion is the puck one and why it is exactly right. Layout
// section 27 drags the puck across N frames and asserts the stub's
// getFieldGrid INVOCATION count is unchanged.
// ============================================================================

// The same deadline discipline as js/meters.js, for the same reason: a dropped
// completion runs neither `catch` nor `finally`, so a flag cleared only on
// settlement latches for the life of the page (P71 / N9, MEASURED in shipped
// 3.2 code). Expressed in milliseconds here because this poll is spent by the
// 500 ms status tick rather than by an interval of its own.
const GUARD_DEADLINE_MS = 3000;

/**
 * @param deps.nativeFn  name -> callable, the shared cache in app.js
 * @param deps.onUpdated () -> void, called when a new grid has been decoded so
 *                       the plan can repaint. The plan module owns the canvas.
 */
export function createField(deps) {
  // The OFFSCREEN surface. Created once, at the grid's own resolution, and
  // re-sized only when the plugin changes the grid shape.
  const offscreen = document.createElement("canvas");
  offscreen.width = 1;
  offscreen.height = 1;

  let grid = null;
  let haveImage = false;

  let inFlight = false;
  let inFlightSince = 0;
  let dropped = 0;

  // The ramp. Two stops of the interface's only hue, over a transparent floor,
  // so the envelope fill and the rig-extent rectangle underneath stay legible.
  // Authored as bytes rather than as CSS colours because it is applied per
  // pixel to an ImageData buffer.
  const RAMP_LO = [138, 106, 47];
  const RAMP_HI = [232, 200, 122];

  function decode(payload) {
    const cols = Number(payload.cols);
    const rows = Number(payload.rows);
    const b64 = String(payload.data ?? "");

    if (!(cols > 0) || !(rows > 0) || b64 === "") return false;

    // atob -> a byte per cell. The plugin quantised to 8 bits against ITS OWN
    // observed min/max, so 0 is the weakest point in this recompute and 255 the
    // strongest; the dB values those two ends stand for travel alongside.
    const bin = atob(b64);
    if (bin.length < cols * rows) return false;

    if (offscreen.width !== cols || offscreen.height !== rows) {
      offscreen.width = cols;
      offscreen.height = rows;
    }

    const ctx = offscreen.getContext("2d");
    if (ctx === null) return false;

    const image = ctx.createImageData(cols, rows);
    const out = image.data;

    for (let i = 0; i < cols * rows; ++i) {
      const t = bin.charCodeAt(i) / 255;
      const o = i * 4;
      out[o] = Math.round(RAMP_LO[0] + (RAMP_HI[0] - RAMP_LO[0]) * t);
      out[o + 1] = Math.round(RAMP_LO[1] + (RAMP_HI[1] - RAMP_LO[1]) * t);
      out[o + 2] = Math.round(RAMP_LO[2] + (RAMP_HI[2] - RAMP_LO[2]) * t);

      // ── ALPHA CARRIES THE LEVEL, AND IT IS DELIBERATELY RESTRAINED ──────
      // The field is a BACKDROP. Everything the operator actually acts on —
      // the hull polygon, the rig extent, the eight glyphs, the puck — is
      // drawn over it, and at 30..180 the gradient washed all of them out:
      // legible as a picture, useless as a plan. Measured on the rendered
      // page, not reasoned about.
      //
      // 0 at the weakest cell means the panel shows through untouched where
      // the field has nothing to say, which is the honest rendering of a
      // quantity that spans only 1.3-10.4 dB over a real hall.
      out[o + 3] = Math.round(96 * t);
    }

    ctx.putImageData(image, 0, 0);

    grid = payload;
    haveImage = true;
    return true;
  }

  return {
    /** One recompute. Called at most once per status tick, never per frame. */
    refresh() {
      const now = performance.now();

      if (inFlight) {
        if (now - inFlightSince < GUARD_DEADLINE_MS) return;
        ++dropped;
      }

      inFlight = true;
      inFlightSince = now;

      Promise.resolve(deps.nativeFn("getFieldGrid")())
        .then((payload) => {
          if (payload === null || typeof payload !== "object") return;
          if (decode(payload) && typeof deps.onUpdated === "function") deps.onUpdated();
        })
        .catch((err) => {
          console.error("getFieldGrid failed", err);
        })
        .finally(() => {
          inFlight = false;
        });
    },

    /** THE BLIT. Scaled from the grid's own resolution onto the plan box. */
    drawInto(ctx, w, h) {
      if (!haveImage || ctx === null || !(w > 0) || !(h > 0)) return false;

      // The smoothing IS the gradient: a 32 x 40 grid drawn at 448 x 560 is
      // bilinearly interpolated by the browser, which is why a coarse grid is
      // sufficient and why no per-pixel work happens on this thread.
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = "high";
      ctx.drawImage(offscreen, 0, 0, w, h);
      return true;
    },

    /** The legend, from the RETURNED span. Never re-derived from the bytes. */
    legendText() {
      if (grid === null) return "—";
      const lo = Number(grid.minDb);
      const hi = Number(grid.maxDb);
      if (!Number.isFinite(lo) || !Number.isFinite(hi)) return "—";
      return `${lo.toFixed(1)} – ${hi.toFixed(1)} dB`;
    },

    /** UI-04/4's descope hook, and NC6's read: the count is the plugin's. */
    diagnostics: () => ({
      dropped,
      computeCount: grid === null ? 0 : Number(grid.computeCount),
      cols: grid === null ? 0 : Number(grid.cols),
      rows: grid === null ? 0 : Number(grid.rows),
    }),
  };
}
