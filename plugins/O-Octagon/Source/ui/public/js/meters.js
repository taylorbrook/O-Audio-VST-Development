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
// O-Octagon — the eight per-speaker level meters (Stage 3 Phase 3.3, UI-03)
//
// UI-03 IS A SAFETY FEATURE WEARING A VISUALISATION COSTUME. §R7 names it a
// second human line of defence on R1 — the channel map — which is the highest
// risk in the project and whose defining property is SILENCE. The whole value
// of this module is that the speaker which lights is the speaker that sounds,
// so a map error presents as the WRONG SPEAKER LIT rather than as nothing at
// all.
//
// WHAT IS MEASURED, AND WHERE. The plugin meters THE WRITTEN OUTPUT BUFFER,
// after GainStage and after the venue trim, indexed through
// snapshot.speakerToBuffer — not the solve vector v_i. A meter on v_i would
// light correctly under a BYPASSED CHANNEL MAP, which is the exact NC3 failure
// caught at Phase 2.2. This file receives linear peaks and never asks what
// produced them.
//
// ── TWO CLOCKS, AND CONFLATING THEM IS A REAL BUG ─────────────────────────
// The ~30 Hz poll refreshes the TARGET only. The 0.5 attack / 0.12 decay
// coefficients are PER requestAnimationFrame FRAME, and the 1.5 s hold and
// 20 dB/s release are WALL CLOCK from timestamps. Applying a per-frame
// coefficient on the poll clock makes the ballistics depend on the poll rate —
// pattern_block_rate_envelope_breaks_blocksize_invariance in UI form, where a
// throttled background tab would change the attack time.
//
// ── EVERY GUARD ON THIS PAGE RELEASES ON A DEADLINE (PLAN-3.3 P71 / N9) ───
// A native completion is DROPPED, not rejected, when the browser is hidden
// (RESEARCH-3.2 N4). A dropped completion is not an exception and not a
// rejection, so NEITHER `catch` NOR `finally` RUNS: the await never resumes,
// and a flag cleared only in a `finally` stays true for the life of the page.
//
// THIS IS NOT HYPOTHETICAL. RESEARCH-3.3 N9 measured it in SHIPPED 3.2 code:
// one dropped getVenueGeometry left the envelope readout frozen at
// 15.60 x 19.50 m against a real 39.00 x 52.00 m, permanently, with no error
// anywhere. D20's "fixed interval + in-flight guard" is NECESSARY BUT NOT
// SUFFICIENT as written — released only on settlement it is the same latch with
// a shorter fuse.
//
// So the guard below carries a TIMESTAMP and is released when the outstanding
// request is older than kGuardDeadlineTicks intervals, whether or not it ever
// settles. Section 33 of ui_frontend_check.js asserts that shape statically for
// every guard on the page, and NC5 proves it fires.
//
// AND IT IS A FIXED setInterval, NEVER `poll().then(poll)` — a recursion whose
// next tick is scheduled by the previous completion dies permanently the first
// time one is dropped (pattern_webview_completion_gated_on_isvisible).
// Section 34 asserts the absence.
// ============================================================================

// ~30 Hz. UI-03 criterion 4's rate.
const METER_POLL_MS = 33;

// The guard releases after this many missed intervals — ~165 ms, which is far
// longer than any real round trip and far shorter than a human notices. The
// unit is INTERVALS rather than milliseconds so the two cannot drift apart.
const GUARD_DEADLINE_TICKS = 5;

// UI-03 criterion 3, verbatim. Per FRAME, not per poll.
const ATTACK_PER_FRAME = 0.5;
const DECAY_PER_FRAME = 0.12;

// The displayed range. −60 dBFS is the bottom of the arc, 0 dBFS the top.
const FLOOR_DB = -60;

// Peak hold, in wall-clock milliseconds, then a linear dB release.
const PEAK_HOLD_MS = 1500;
const PEAK_RELEASE_DB_PER_S = 20;

// Above this the arc takes the pale-gold stop. One hue, two stops.
const HOT_DB = -6;

const SPEAKERS = 8;

const clamp01 = (v) => Math.min(1, Math.max(0, v));

// Linear peak -> dBFS, with an explicit floor rather than -Infinity: a
// -Infinity that reached the arithmetic below would poison `cur` for the life
// of the page, which is the same latch class this whole file is written
// against.
function toDb(linear) {
  const v = Number(linear);
  if (!(v > 0)) return FLOOR_DB;
  const db = 20 * Math.log10(v);
  return db < FLOOR_DB ? FLOOR_DB : db;
}

// dBFS -> 0..1 arc fraction.
const dbToFraction = (db) => clamp01((db - FLOOR_DB) / -FLOOR_DB);

/**
 * @param deps.nativeFn   name -> callable, the shared cache in app.js
 * @param deps.onLevels   (levels[8], peaks[8], hot[8]) -> void. Called once per
 *                        rAF frame with 0..1 fractions. THE DOM IS NOT TOUCHED
 *                        HERE — roomplan.js owns every glyph-space write, so
 *                        there is one owner of the view rather than two.
 */
export function createMeters(deps) {
  const state = Array.from({ length: SPEAKERS }, () => ({
    targetDb: FLOOR_DB,
    cur: 0,
    peakDb: FLOOR_DB,
    peakAt: 0,
  }));

  let pollTimer = null;
  let rafHandle = null;
  let lastFrameMs = 0;

  // The in-flight guard and ITS DEADLINE. Two variables, not one, and the
  // second is the whole of P71: a boolean alone can only be cleared by a
  // completion that may never arrive.
  let inFlight = false;
  let inFlightSince = 0;

  // Diagnostics for Gate 13's hidden-editor test and for NC5: a latched guard
  // is invisible from the outside, so it is counted.
  let dropped = 0;

  function applyPeaks(peaks) {
    for (let i = 0; i < SPEAKERS; ++i) {
      state[i].targetDb = toDb(peaks?.[i]);
    }
  }

  function tick() {
    const now = performance.now();

    if (inFlight) {
      // RELEASED ON A DEADLINE, NEVER ON SETTLEMENT. If the previous request
      // was dropped — the editor was hidden when it completed — this is the
      // only line in the module that can ever clear the flag again.
      if (now - inFlightSince < METER_POLL_MS * GUARD_DEADLINE_TICKS) return;
      ++dropped;
    }

    inFlight = true;
    inFlightSince = now;

    // The `finally` is the FAST path, not the guarantee. It clears the flag on
    // a completion that arrives; the deadline above clears it on one that never
    // does. Both are required and neither is redundant.
    Promise.resolve(deps.nativeFn("getMeters")())
      .then((payload) => {
        if (payload !== null && typeof payload === "object") applyPeaks(payload.peaks);
      })
      .catch((err) => {
        console.error("getMeters failed", err);
      })
      .finally(() => {
        inFlight = false;
      });
  }

  // ── The ballistics, per FRAME ────────────────────────────────────────────
  function frame(nowMs) {
    rafHandle = window.requestAnimationFrame(frame);

    const dtSec = lastFrameMs === 0 ? 0 : Math.max(0, (nowMs - lastFrameMs) / 1000);
    lastFrameMs = nowMs;

    const levels = new Array(SPEAKERS);
    const peaks = new Array(SPEAKERS);
    const hot = new Array(SPEAKERS);

    for (let i = 0; i < SPEAKERS; ++i) {
      const s = state[i];
      const target = dbToFraction(s.targetDb);

      // Asymmetric, and the asymmetry is the point: a peak must be visible the
      // frame it happens, and must not vanish before an eye crossing a dark
      // hall can find it.
      const k = target > s.cur ? ATTACK_PER_FRAME : DECAY_PER_FRAME;
      s.cur += (target - s.cur) * k;

      // WALL CLOCK, not frames. A hold measured in frames would be 1.5 s at
      // 60 fps and 6 s in a throttled tab.
      if (s.targetDb > s.peakDb) {
        s.peakDb = s.targetDb;
        s.peakAt = nowMs;
      } else if (nowMs - s.peakAt > PEAK_HOLD_MS) {
        s.peakDb = Math.max(FLOOR_DB, s.peakDb - PEAK_RELEASE_DB_PER_S * dtSec);
      }

      levels[i] = clamp01(s.cur);
      peaks[i] = dbToFraction(s.peakDb);
      hot[i] = s.targetDb > HOT_DB;
    }

    if (typeof deps.onLevels === "function") deps.onLevels(levels, peaks, hot);
  }

  return {
    start() {
      if (pollTimer === null) pollTimer = window.setInterval(tick, METER_POLL_MS);
      if (rafHandle === null) rafHandle = window.requestAnimationFrame(frame);
    },

    stop() {
      if (pollTimer !== null) window.clearInterval(pollTimer);
      if (rafHandle !== null) window.cancelAnimationFrame(rafHandle);
      pollTimer = null;
      rafHandle = null;
    },

    // Gate 13 / NC5. `dropped` counts deadline releases: it is 0 on a healthy
    // page and non-zero the moment a completion is lost, which is the only
    // externally visible symptom a latched guard would otherwise have.
    diagnostics: () => ({ dropped, inFlight }),
  };
}
