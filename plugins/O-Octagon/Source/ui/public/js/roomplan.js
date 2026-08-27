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
// O-Octagon — the Room plan (Stage 3 Phase 3.1)
//
// THREE LAYERS, ONE PROJECTION (PLAN-3.1 P46):
//     <canvas #plan-backdrop>   envelope fill, rig extent, centroid
//     <svg    #plan-geometry>   hull polygon + 8 speaker glyphs
//     <div    #plan-controls>   puck + the 8 in-plan weight controls
//
// All three are positioned by metresToPx() below and by nothing else. Three
// layers with three coordinate derivations would be three things free to drift;
// one function used by all three cannot. Section 19 of ui_frontend_check.js
// asserts there is no second (v - min) / (max - min) form anywhere in the JS.
//
// Drawing the whole plan on the canvas instead would make UI-02 criteria 1, 2
// and 3 UNMEASURABLE without a debug back-channel — a canvas has no boxes to
// measure and no vertices to count. In a plugin whose whole discipline is
// "measure, do not assert", that is disqualifying.
//
// ── TWO BOXES, AND THEY ARE NOT THE SAME BOX ──────────────────────────────
//   envelope  speaker bbox + 15 % margin per axis (min 1.0 m) — ARCHITECTURE
//             section 6.2 / AD-14. Derived in C++; this file receives it. It is
//             what the plan DRAWS, so the plan's proportions follow the venue.
//   bbox      the raw speaker bounding box. It is what VenueModel::normToMetres
//             denormalises srcX / srcY against, so it is what the puck's metres
//             position and the footer readout resolve through.
// Using one where the other belongs puts the puck in the wrong place by the
// margin — 1.80 m and 2.25 m on the default venue — and the error is a smooth
// offset that looks entirely plausible.
//
// ── THE CANVAS IS A CSS REPLACED ELEMENT ──────────────────────────────────
// left + right does NOT stretch it, so styles.css sizes it with explicit
// width/height in calc(), and the backing store is set here as
// round(rect * devicePixelRatio) followed by ctx.scale(dpr, dpr). The bug is
// invisible at exactly one DPR, which is why ui_layout_check.js section 6
// measures at DPR 1 AND DPR 2 (o-textureforge-cursor-bug).
// ============================================================================

// ── THE ONE COORDINATE MAPPING ─────────────────────────────────────────────
// Metres (venue space) -> pixels (plan-box space). y grows downward on screen
// and into the room in venue space, so the front of the hall — speakers 1 and 2
// at y = 4.50 m — renders at the TOP of the plan, which is how a hall plan is
// conventionally drawn.
export function metresToPx(mx, my, view) {
  return {
    x: ((mx - view.minX) / view.spanX) * view.w,
    y: ((my - view.minY) / view.spanY) * view.h,
  };
}

// ── The JS twin of oo::plane::normToMetres ─────────────────────────────────
// Normalised srcX / srcY (0..1, the host-facing parameter range) -> metres, via
// the SPEAKER BOUNDING BOX.
//
// THE DEGENERATE DECISION IS TAKEN FROM THE PAYLOAD FLAG, NOT FROM A
// TRANSCRIBED THRESHOLD. VenueModel::normToMetres pins a degenerate axis to its
// minimum rather than dividing by zero (VenueModel.h:185-192, via
// plane::kMinSpan); a naive min + n * (max - min) here would diverge from the
// C++ on exactly the degenerate venues Phase 2.1 spent a whole matrix on. The
// comparison against kMinSpan is made ONCE, in C++, and arrives as a boolean.
export function normToMetres(nx, ny, geometry) {
  const b = geometry.bbox;

  return {
    x: b.degenerateX ? b.minX : b.minX + nx * (b.maxX - b.minX),
    y: b.degenerateY ? b.minY : b.minY + ny * (b.maxY - b.minY),
  };
}

// ── The JS twin of oo::plane::earHeight ────────────────────────────────────
// Audience-plane ear height at a depth y, in metres: linear from rakeFront at
// bbMinY to rakeRear at bbMaxY. Phase 3.3's elevation strip needs it; it lives
// HERE, beside normToMetres, because that is where the other metres-space twin
// already lives and two copies of one physical law is exactly the drift this
// file's whole discipline is against (P46).
//
// THE INTERPOLATION IS metresToPx ITSELF, not a second copy of the arithmetic.
// earHeight is the same linear map — (v - lo) / span, scaled — so it is
// computed BY the one projection function rather than beside it. Section 19
// asserts there is no second (v - min) / span form anywhere in the shipped JS,
// and this construction is what makes that true rather than argued around.
//
// The zero-span decision is the payload FLAG, taken from C++ against
// oo::plane::kMinSpan, never a threshold transcribed here. VenueModel collapses
// the plane to the constant rakeFront rather than dividing by zero
// (VenueModel.h:176-177), and this returns the same constant on the same flag.
export function earHeight(yMetres, geometry) {
  const b = geometry.bbox;
  const r = geometry.rake;

  if (b.degenerateY) return r.front;

  const rakeView = makeView({ minX: b.minY, maxX: b.maxY, minY: 0, maxY: 1 },
                            r.rear - r.front, 1);

  return r.front + metresToPx(yMetres, 0, rakeView).x;
}

// ── THE FIT RULE, STATED ONCE ──────────────────────────────────────────────
// A plan box is fitted to the SMALLER of its two bounds. Both plans use this
// one implementation (PLAN-3.2 P62).
//
// It is exported because Q11 found the rule being needed a second time, one
// layer down: the Venue screen's mini-plan at the default venue's PORTRAIT
// aspect 0.800 is width-bound at 278 x 348 and pushes the rail stack to 601 px
// inside 592, while document.scrollHeight stays 720 the whole time. That is
// D7's portrait consequence reappearing, and a second copy of four lines of
// arithmetic is exactly the kind of thing that drifts. Height-binding gives
// 270 x 337 and the rail comes to 592 == 592.
export function fitBox(rectW, rectH, aspect) {
  if (!(rectW > 0) || !(rectH > 0) || !(aspect > 0)) return { w: 0, h: 0 };

  let w = rectW;
  let h = w / aspect;

  if (h > rectH) {
    h = rectH;
    w = h * aspect;
  }

  return { w: Math.floor(w), h: Math.floor(h) };
}

// The view metresToPx() projects through. Exported so a second plan is a second
// VIEW rather than a second PROJECTION (RESEARCH-3.2 Q8) — section 19 of the
// static gate is widened by this rather than weakened, which is what keeps P46
// honoured rather than argued around.
export function makeView(envelope, w, h) {
  return {
    minX: envelope.minX,
    minY: envelope.minY,
    spanX: envelope.maxX - envelope.minX,
    spanY: envelope.maxY - envelope.minY,
    w,
    h,
  };
}

// ── THE HULL POINTS STRING, AND THE GLYPH PLACEMENT, STATED ONCE ───────────
// Same reasoning as fitBox and makeView directly above, one layer further in.
// Q8's rule is "a second view, never a second projection", and until v1.3.3
// only the projection, the fit and the view were actually shared: the
// points-string construction and the three classification toggles had been
// copy-pasted into venue.js's drawMini(). Both plans drew the same hull the
// same way from two copies of the same six lines, so a fourth classification
// class — or any change to the points format — had to be made twice and would
// have drifted the first time it was not. Extraction WIDENS section 19 rather
// than weakening it: every coordinate still goes through the one metresToPx,
// and now there is one fewer call site that could stop doing so.
//
// THE OUTPUT BADGE STAYS BEHIND, DELIBERATELY. drawGeometryLayer() also writes
// the v1.1.0 output badge into a node the mini plan has no DOM for (gout-N,
// index.html), so the shared part is scoped to hull + transform + toggles and
// the badge is rendered by the caller that has somewhere to put it. Pulling it
// in here would mean a null-node branch on the mini's behalf, which is a second
// thing to keep true rather than one fewer.
export function hullPoints(hull, view) {
  return hull
    .map((p) => {
      const q = metresToPx(p.x, p.y, view);
      return `${q.x},${q.y}`;
    })
    .join(" ");
}

// The glyph classes come straight from ConvexHull2D::classify() in the payload,
// so neither plan can drift from the behaviour it depicts — both are drawing
// the same hull the solver solves against. classify() was made a first-class
// return value at Phase 2.1 (P11) for exactly this.
export function placeGlyph(g, s, view) {
  const p = metresToPx(s.x, s.y, view);
  g.setAttribute("transform", `translate(${p.x} ${p.y})`);

  g.classList.toggle("is-vertex", s.class === "VERTEX");
  g.classList.toggle("is-onedge", s.class === "ON_EDGE");
  g.classList.toggle("is-interior", s.class === "INTERIOR");
}

const clamp01 = (v) => Math.min(1, Math.max(0, v));

export function createRoomPlan(deps) {
  const stage = document.getElementById("plan-stage");
  const box = document.getElementById("plan-layers");
  const canvas = document.getElementById("plan-backdrop");
  const svg = document.getElementById("plan-geometry");
  const hullPoly = document.getElementById("hull-poly");
  const puck = document.getElementById("puck");

  if (stage === null || box === null || canvas === null || svg === null
      || hullPoly === null || puck === null)
    throw new Error("room plan: a required element is missing");

  // v1.8.0 — the trace, the Drift tail and the live puck. Optional on purpose:
  // a page without them still draws the 3.1 plan.
  const traceEl = document.getElementById("motion-trace");
  const tailEl = document.getElementById("motion-tail");
  const puckLive = document.getElementById("puck-live");

  const glyphs = deps.weightIds.map((_, i) => document.getElementById(`glyph-${i + 1}`));
  const wCells = deps.weightIds.map((_, i) => document.getElementById(`wcell-${i + 1}`));

  // Phase 3.3. The meter arc and its peak tick are CHILDREN of the glyph <g>,
  // so they inherit the transform metresToPx already wrote — the plan module
  // stays the single owner of the view transform (P46) and js/meters.js never
  // touches a coordinate.
  const meterArcs = deps.weightIds.map((_, i) => document.getElementById(`meter-${i + 1}`));
  const meterPeaks = deps.weightIds.map((_, i) => document.getElementById(`mpeak-${i + 1}`));

  // v1.1.0. The per-glyph output badge — a dedicated value node this module
  // owns, beside the AUTHORED numeral it must never touch
  // (pattern_js_state_updater_overwrites_html_labels). Content comes whole from
  // the payload's per-speaker `output` (D19); this file renders the number and
  // never owns a device-order table.
  const outBadges = deps.weightIds.map((_, i) => document.getElementById(`gout-${i + 1}`));

  // The speaker→output popover. Opened by double-click on a glyph; its eight
  // numerals are authored in index.html and this module toggles classes on
  // them. `assignOutput` is app.js's wrapper around assignSpeakerOutput — the
  // swap arithmetic lives in C++, and authoritative state converges on the
  // venueGen poll, never on the completion (P64).
  const outPop = document.getElementById("out-pop");
  const outPopButtons = deps.weightIds.map((_, i) => document.getElementById(`out-btn-${i + 1}`));
  let outPopSpeaker = 0;   // 1-based; 0 = closed

  // 2 * pi * 15, the meter arc's radius in index.html. Authored ONCE, here,
  // where the dash arithmetic is — a second copy in the CSS would be a
  // mirrored constant free to drift from the r attribute it describes.
  const METER_CIRCUMFERENCE = 2 * Math.PI * 15;

  const srcX = deps.sliders.get("srcX");
  const srcY = deps.sliders.get("srcY");

  if (srcX === undefined || srcY === undefined)
    throw new Error("room plan: srcX / srcY are not bound");

  let geometry = null;
  let view = null;
  let dragging = false;
  let dpr = window.devicePixelRatio || 1;

  // Phase 3.3 hooks. `fieldPainter` is js/field.js's blit, called from inside
  // drawBackdrop so the field lands UNDER the rig extent and the centroid
  // rather than over them; `previewSet` is the scene module's resolved
  // membership, which this file renders and never derives.
  let fieldPainter = null;
  let previewSet = [];

  // The relative-delta accumulator, in NORMALISED parameter units.
  const acc = { x: 0, y: 0 };

  // ── v1.8.0 — motion state, all of it HANDED IN ───────────────────────────
  // `trace` is getMotionTrace's payload verbatim ({ cyclic, points: [[x,y,z]] }
  // in anchor-relative metres); `live` is the polled offset triple. This file
  // adds the anchor's metres and calls metresToPx. IT NEVER COMPUTES A PATH —
  // ui_frontend_check §41 holds that: no sin/cos, no Perlin, no second
  // generator that could drift from the one the audio thread runs
  // (pattern_test_fixture_mirrors_drift_silently).
  let trace = null;
  let motionOn = false;
  let live = [0, 0, 0];

  // Drift has no closed cycle to trace, so it gets a TAIL: the last 48 polled
  // ABSOLUTE positions (anchor + offset, metres) as a fading polyline.
  const TAIL_LENGTH = 48;
  const tail = [];

  // ── Layout ───────────────────────────────────────────────────────────────
  // The plan box is fitted to the RETURNED envelope's aspect inside a MEASURED
  // stage rect. Two things follow, and both are the point:
  //
  //   * the aspect is never authored anywhere — UI-02 criterion 1 requires the
  //     proportions to follow the plugin's computed envelope, and
  //     ui_layout_check.js section 2 proves it by mutating the stub's bbox and
  //     re-measuring the rendered box;
  //   * there is NO ROW ARITHMETIC. getBoundingClientRect() is the real box.
  //     O-ReverseDelay carried 93.5 px of phantom slack through five releases
  //     because every comment reasoned about rows and never subtracted from the
  //     frame height (pattern_flex1_container_slack_invisible_to_row_sum).
  function relayout() {
    if (geometry === null) return;

    const rect = stage.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return;   // hidden screen

    const e = geometry.envelope;
    const spanX = e.maxX - e.minX;
    const spanY = e.maxY - e.minY;
    if (!(spanX > 0) || !(spanY > 0)) return;

    const { w, h } = fitBox(rect.width, rect.height, spanX / spanY);
    if (!(w > 0) || !(h > 0)) return;

    box.style.setProperty("--plan-w", String(w));
    box.style.setProperty("--plan-h", String(h));

    view = makeView(e, w, h);

    svg.setAttribute("viewBox", `0 0 ${w} ${h}`);

    resizeCanvas();
    draw();
  }

  // DPR backing store. `canvas.width` is a BACKING-STORE size in device pixels
  // and is unrelated to the CSS width; setting only the CSS size gives a blurry
  // canvas at DPR 2 and a correct one at DPR 1, which is why the gate measures
  // both.
  function resizeCanvas() {
    const rect = canvas.getBoundingClientRect();
    canvas.width = Math.round(rect.width * dpr);
    canvas.height = Math.round(rect.height * dpr);

    const ctx = canvas.getContext("2d");
    if (ctx !== null) {
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.scale(dpr, dpr);
    }
  }

  // ── Drawing ──────────────────────────────────────────────────────────────

  function drawBackdrop() {
    const ctx = canvas.getContext("2d");
    if (ctx === null || view === null || geometry === null) return;

    ctx.clearRect(0, 0, view.w, view.h);

    // The envelope fill — the whole plan box IS the envelope, by construction.
    ctx.fillStyle = "#241E1A";
    ctx.fillRect(0, 0, view.w, view.h);

    // Phase 3.3 (UI-04). THE FIELD GRADIENT, BLITTED FROM AN OFFSCREEN CANVAS.
    // It goes on immediately after the fill and BEFORE the rig extent and the
    // centroid, so those two stay legible over it.
    //
    // UI-04 criterion 4 — "descopable without touching any other component" —
    // is exactly this line: `fieldPainter` is null until app.js sets it, and
    // nothing else in this file, or any other, knows the field exists. Section
    // 39 asserts js/field.js is imported by nothing but app.js.
    if (fieldPainter !== null) fieldPainter(ctx, view.w, view.h);

    // The rig extent: the speaker bounding box, which is the region srcX / srcY
    // actually span. Drawing it makes the margin legible as "outside the rig"
    // rather than as unexplained padding.
    const b = geometry.bbox;
    const tl = metresToPx(b.minX, b.minY, view);
    const br = metresToPx(b.maxX, b.maxY, view);

    ctx.strokeStyle = "rgba(240, 232, 220, 0.10)";
    ctx.lineWidth = 1;
    ctx.strokeRect(tl.x + 0.5, tl.y + 0.5, br.x - tl.x - 1, br.y - tl.y - 1);

    // Centroid — the point rigScale is measured about.
    const c = metresToPx(geometry.centroid.x, geometry.centroid.y, view);
    ctx.strokeStyle = "rgba(240, 232, 220, 0.20)";
    ctx.beginPath();
    ctx.moveTo(c.x - 5, c.y);
    ctx.lineTo(c.x + 5, c.y);
    ctx.moveTo(c.x, c.y - 5);
    ctx.lineTo(c.x, c.y + 5);
    ctx.stroke();
  }

  // The hull polygon carries exactly hullCount points, in the order
  // ConvexHull2D returned them, so polygon.points.numberOfItems is a DOM fact a
  // test can read (UI-02 criterion 2). Both the points string and the glyph
  // placement are the shared hullPoints()/placeGlyph() above — the mini plan on
  // the Venue screen draws through the same two functions, so the two views
  // cannot disagree about what the hull is or which speakers are on it.
  function drawGeometryLayer() {
    if (view === null || geometry === null) return;

    hullPoly.setAttribute("points", hullPoints(geometry.hull, view));

    geometry.speakers.forEach((s, i) => {
      const g = glyphs[i];
      if (g === undefined || g === null) return;

      placeGlyph(g, s, view);

      // v1.1.0 — the output badge. Empty when the speaker already reaches its
      // own physical output; "→k" when remapped; "→?" for a label outside the
      // 7.1 set. The number arrives COMPUTED from the payload (D19) — this
      // file renders it and owns no device-order table. The badge is this
      // module's own value node; the authored numeral beside it is untouched.
      const value = outBadges[i];
      if (value !== null && value !== undefined) {
        const out = Number(s.output);
        value.textContent = out === i + 1
          ? ""
          : `→${out >= 1 && out <= outBadges.length ? out : "?"}`;
      }
    });
  }

  // ── Phase 3.3 — the eight meters (UI-03 / D23) ───────────────────────────
  //
  // Arc length by stroke-dasharray, peak by rotation. NO TEXT NODE IS WRITTEN:
  // the numeral inside each glyph is HTML-authored and a state updater that
  // touched it would erase the only thing identifying the speaker
  // (pattern_js_state_updater_overwrites_html_labels).
  //
  // The fractions arrive already ballistic'd from js/meters.js, which owns both
  // clocks. This function does geometry and nothing else, which is the same
  // split that keeps the view transform in one place.
  function setMeters(levels, peaks, hot) {
    for (let i = 0; i < meterArcs.length; ++i) {
      const arc = meterArcs[i];
      const tick = meterPeaks[i];
      const g = glyphs[i];

      const level = clamp01(Number(levels?.[i]) || 0);
      const peak = clamp01(Number(peaks?.[i]) || 0);

      if (arc !== null && arc !== undefined) {
        arc.setAttribute("stroke-dasharray",
                         `${level * METER_CIRCUMFERENCE} ${METER_CIRCUMFERENCE}`);
      }

      if (tick !== null && tick !== undefined)
        tick.setAttribute("transform", `rotate(${peak * 360})`);

      if (g !== null && g !== undefined) {
        g.classList.toggle("is-hot", hot?.[i] === true);
        g.classList.toggle("is-metered", peak > 0);
      }
    }
  }

  // ── Phase 3.3 — the scene preview (D21 / FUNC-06/3) ──────────────────────
  //
  // RENDERS A SET THE PLUGIN RESOLVED. There is no membership test here and no
  // speaker coordinate is compared with anything — the indices arrive whole,
  // and section 32 asserts that absence across every shipped module.
  function setPreview(indices) {
    previewSet = Array.isArray(indices) ? indices : [];

    glyphs.forEach((g, i) => {
      if (g === null || g === undefined) return;
      g.classList.toggle("is-preview", previewSet.includes(i + 1));
    });
  }

  // The weight cells are sited at their speakers and clamped inside the plan
  // box, so a speaker close to a wall does not push its control off the plan.
  // This function moves the CELL and nothing else — the numeral in .w-label is
  // HTML-authored, and .w-value is written by app.js's own binding
  // (pattern_js_state_updater_overwrites_html_labels).
  function placeWeightCells() {
    if (view === null || geometry === null) return;

    const half = 34;
    const cellH = 30;

    geometry.speakers.forEach((s, i) => {
      const cell = wCells[i];
      if (cell === undefined || cell === null) return;

      const p = metresToPx(s.x, s.y, view);
      const left = Math.min(view.w - half, Math.max(half, p.x));
      const top = Math.min(view.h - cellH, Math.max(0, p.y + 14));

      cell.style.left = `${left}px`;
      cell.style.top = `${top}px`;
    });
  }

  // OPTIMISTIC DURING A DRAG, AUTHORITATIVE OTHERWISE (PLAN-3.1 P40).
  //
  // The JS -> C++ -> JS round trip returns the SNAPPED value asynchronously, so
  // a puck rendered only from the echo lags the pointer by a full round trip and
  // reads as rubbery at 60 fps. While dragging it renders from the local
  // accumulator; the rest of the time it renders from the parameter, which is
  // what makes HOST AUTOMATION move it.
  function renderPuck() {
    if (view === null || geometry === null) return;

    const nx = dragging ? acc.x : srcX.state.getNormalisedValue();
    const ny = dragging ? acc.y : srcY.state.getNormalisedValue();

    const m = normToMetres(nx, ny, geometry);
    const p = metresToPx(m.x, m.y, view);

    puck.style.left = `${p.x}px`;
    puck.style.top = `${p.y}px`;

    renderTrace(m);
    renderLive(m);
  }

  // The anchor in metres — the puck's own derivation, reused so the trace and
  // the live puck translate with a drag exactly as the puck does.
  function anchorMetres() {
    const nx = dragging ? acc.x : srcX.state.getNormalisedValue();
    const ny = dragging ? acc.y : srcY.state.getNormalisedValue();
    return normToMetres(nx, ny, geometry);
  }

  // ── v1.8.0 — the trace: translate, project, join. No other arithmetic. ────
  function renderTrace(anchor) {
    if (traceEl === null || view === null || geometry === null) return;

    const pts = motionOn && trace !== null && Array.isArray(trace.points) ? trace.points : [];

    if (pts.length < 2) {
      traceEl.setAttribute("d", "");
      return;
    }

    const parts = pts.map((pt, i) => {
      const p = metresToPx(anchor.x + Number(pt[0]), anchor.y + Number(pt[1]), view);
      return `${i === 0 ? "M" : "L"}${p.x.toFixed(2)} ${p.y.toFixed(2)}`;
    });

    traceEl.setAttribute("d", parts.join(" ") + (trace.cyclic === true ? " Z" : ""));
  }

  // ── v1.8.0 — the live puck and the Drift tail ────────────────────────────
  function renderLive(anchor) {
    if (view === null || geometry === null) return;

    if (puckLive !== null) {
      if (!motionOn) {
        puckLive.hidden = true;
      } else {
        const p = metresToPx(anchor.x + live[0], anchor.y + live[1], view);
        puckLive.style.left = `${p.x}px`;
        puckLive.style.top = `${p.y}px`;
        puckLive.hidden = false;
      }
    }

    if (tailEl !== null) {
      const showTail = motionOn && trace !== null && trace.cyclic === false;
      tailEl.setAttribute("points", showTail
        ? tail.map((m) => { const p = metresToPx(m.x, m.y, view); return `${p.x.toFixed(2)},${p.y.toFixed(2)}`; }).join(" ")
        : "");
    }
  }

  // ── v1.1.0 — the speaker→output popover ──────────────────────────────────
  //
  // Double-click a glyph, pick the physical output that speaker is wired to.
  // The choice travels as two integers to assignSpeakerOutput; the swap and
  // the device-order table live in C++ (D19), and the authoritative result —
  // badges, highlight, the venue table's label column — converges on the
  // venueGen poll, never on the completion (P64). The completion is used for
  // nothing; the popover has already closed by the time it settles.

  function renderOutPop() {
    if (outPop === null || outPopSpeaker === 0 || geometry === null || view === null) return;

    const s = geometry.speakers[outPopSpeaker - 1];
    if (s === undefined) return;

    const value = document.getElementById("outpop-num");
    if (value !== null) value.textContent = String(outPopSpeaker);

    const current = Number(s.output);

    outPopButtons.forEach((b, k) => {
      if (b === null || b === undefined) return;
      b.classList.toggle("is-current", current === k + 1);
    });

    // Sited at the glyph and clamped inside the plan box — the weight cells'
    // discipline. The popover is visible by the time this measures it, and its
    // width is PINNED in the CSS: a shrink-to-fit box measured at the viewport
    // edge lies about its own size
    // (pattern_fixed_tooltip_shrink_to_fit_edge).
    const p = metresToPx(s.x, s.y, view);
    const left = Math.min(view.w - outPop.offsetWidth, Math.max(0, p.x - outPop.offsetWidth / 2));
    const top = Math.min(view.h - outPop.offsetHeight, Math.max(0, p.y + 20));

    outPop.style.left = `${left}px`;
    outPop.style.top = `${top}px`;
  }

  function openOutPop(speakerN) {
    if (outPop === null) return;
    outPopSpeaker = speakerN;
    outPop.hidden = false;
    renderOutPop();
  }

  function closeOutPop() {
    outPopSpeaker = 0;
    if (outPop !== null) outPop.hidden = true;
  }

  glyphs.forEach((g, i) => {
    if (g === null || g === undefined) return;

    g.addEventListener("dblclick", (e) => {
      openOutPop(i + 1);
      e.preventDefault();
      e.stopPropagation();
    });
  });

  outPopButtons.forEach((b, k) => {
    if (b === null || b === undefined) return;

    b.addEventListener("click", () => {
      const speakerN = outPopSpeaker;
      closeOutPop();

      if (speakerN === 0 || typeof deps.assignOutput !== "function") return;

      // Fire and move on. Truth arrives on the venueGen poll (P64); app.js's
      // wrapper owns the error log.
      deps.assignOutput(speakerN, k + 1);
    });
  });

  document.addEventListener("pointerdown", (e) => {
    if (outPopSpeaker === 0 || outPop === null) return;
    if (outPop.contains(e.target)) return;
    closeOutPop();
  });

  window.addEventListener("keydown", (e) => {
    if (e.key === "Escape") closeOutPop();
  });

  function draw() {
    drawBackdrop();
    drawGeometryLayer();
    placeWeightCells();
    renderPuck();

    // An open popover follows the venue it describes — a re-fit or a venue
    // edit moves the glyph it is anchored to and may change the highlighted
    // output.
    renderOutPop();
  }

  // ── The puck gesture ─────────────────────────────────────────────────────
  //
  // THE ONLY TWO-PARAMETER GESTURE AT 3.1, and therefore the one that makes
  // RESEARCH-3.1 N1 a 3.1 problem rather than a 3.3 one:
  // WebSliderParameterAttachment routes a JS write through
  // setValueAsPartOfGesture — setValueNotifyingHost with NO
  // beginChangeGesture / endChangeGesture. The brackets have to come from here,
  // on BOTH srcX and srcY, and they must close on pointercancel and
  // lostpointercapture too: a drag interrupted by a window-focus change that
  // never closes leaves the host in an open automation-write region.
  //
  // RELATIVE DELTA, NEVER ABSOLUTE CURSOR TRACKING. Absolute tracking makes the
  // puck JUMP to the cursor on any grab that is not dead-centre, which is why
  // ui_layout_check.js section 4 grabs OFF-CENTRE — a centred grab passes under
  // absolute tracking too, so a centred probe would be vacuous.
  //
  // THE ACCUMULATOR IS CLAMPED AT THE ACCUMULATOR, not only at the write.
  // Clamping the written value while letting the accumulator run past 1.0 IS the
  // sticky-edge bug: drag 200 px past the right edge, reverse, and the puck sits
  // still for 200 px while the surplus unwinds. Clamping the stored value makes
  // reversal immediate by construction, and section 5 of the layout gate drags
  // well past an edge and reverses by one step to prove it.

  // Pixels per unit of normalised parameter. Derived from the BBOX extent, not
  // the envelope, so pointer and puck move together: the normalised range spans
  // the speaker bounding box, and the plan box spans the wider envelope.
  function spanPx() {
    if (view === null || geometry === null) return { x: 1, y: 1 };

    const b = geometry.bbox;
    const lo = metresToPx(b.minX, b.minY, view);
    const hi = metresToPx(b.maxX, b.maxY, view);

    // A degenerate axis has zero pixel extent. The parameter is still a real
    // parameter and must stay drivable, so fall back to the plan width; the
    // puck stays pinned on that axis because normToMetres pins it, which is the
    // honest depiction of a rig with no extent on that axis.
    return {
      x: b.degenerateX || !(hi.x - lo.x > 0) ? view.w : hi.x - lo.x,
      y: b.degenerateY || !(hi.y - lo.y > 0) ? view.h : hi.y - lo.y,
    };
  }

  let lastPointer = null;

  function openPuckGesture() {
    if (dragging) return;
    dragging = true;
    acc.x = srcX.state.getNormalisedValue();
    acc.y = srcY.state.getNormalisedValue();

    // BOTH. Not one, not "the one that moved".
    srcX.state.sliderDragStarted();
    srcY.state.sliderDragStarted();

    puck.classList.add("is-dragging");
  }

  function closePuckGesture() {
    if (!dragging) return;
    dragging = false;
    lastPointer = null;

    srcX.state.sliderDragEnded();
    srcY.state.sliderDragEnded();

    puck.classList.remove("is-dragging");

    // Discard the local position and re-sync from the parameter.
    renderPuck();
  }

  puck.addEventListener("pointerdown", (e) => {
    puck.setPointerCapture(e.pointerId);
    lastPointer = { x: e.clientX, y: e.clientY };
    openPuckGesture();
    e.preventDefault();
  });

  puck.addEventListener("pointermove", (e) => {
    if (!dragging || lastPointer === null) return;

    const dx = e.clientX - lastPointer.x;
    const dy = e.clientY - lastPointer.y;
    lastPointer = { x: e.clientX, y: e.clientY };

    const s = spanPx();

    // Clamp, then STORE BACK. This line is the whole of P41.
    acc.x = clamp01(acc.x + dx / s.x);
    acc.y = clamp01(acc.y + dy / s.y);

    srcX.state.setNormalisedValue(acc.x);
    srcY.state.setNormalisedValue(acc.y);

    renderPuck();

    // Metres are computed from the CACHED geometry — no native round trip in a
    // pointermove handler (PLAN-3.1 P42 / RESEARCH Q4).
    if (typeof deps.onSourceMoved === "function") deps.onSourceMoved();
  });

  puck.addEventListener("pointerup", closePuckGesture);
  puck.addEventListener("pointercancel", closePuckGesture);
  puck.addEventListener("lostpointercapture", closePuckGesture);

  // Host automation and the srcX / srcY sliders in the controls column both
  // arrive here. Render on echo; never write on echo.
  srcX.state.valueChangedEvent.addListener(renderPuck);
  srcY.state.valueChangedEvent.addListener(renderPuck);

  // A window dragged between a Retina and a non-Retina display changes DPR
  // without changing any CSS size, so the backing store has to be rebuilt.
  const dprWatch = () => {
    const next = window.devicePixelRatio || 1;
    if (next === dpr) return;
    dpr = next;
    resizeCanvas();
    draw();
  };

  // ── WHAT `resize` COULD NOT SEE (CODE_REVIEW WR-02) ──────────────────────
  //
  // Until v1.3.2 the comment above was hooked to `resize` and NOTHING ELSE,
  // which cannot fire for the case the comment describes. The editor is a fixed
  // 1100 x 720 non-resizable surface (PluginEditor.cpp setSize, the single
  // sizing call — there is no setResizable and no constrainer anywhere in
  // Source/), so the CSS viewport never changes size, and a backing-scale change
  // with an unchanged CSS viewport dispatches no `resize` event in either
  // engine. Dragging the plugin window from a non-Retina display to a Retina one
  // therefore left a 1x backing store upscaled 2x — a visibly soft Room plan —
  // until some other relayout happened to run, and none of them re-read
  // devicePixelRatio either: resizeCanvas() reads this module-level `dpr`, and
  // dprWatch was its only post-construction writer.
  //
  // matchMedia("(resolution: Xdppx)") is the standard hook, and it is ONE-SHOT
  // by nature: the query is written against the CURRENT ratio, so once it stops
  // matching it never fires again. Re-arm on every fire, against the new ratio.
  //
  // The `resize` listener stays as a belt-and-braces path — a genuine viewport
  // change still needs the backing store rebuilt, and that is the case it does
  // catch.
  function armDprWatch() {
    if (typeof window.matchMedia !== "function") return;

    const mq = window.matchMedia(`(resolution: ${window.devicePixelRatio || 1}dppx)`);

    // Detaches BEFORE re-arming, on both surfaces. `{ once: true }` would cover
    // the modern one, but the deprecated addListener has no equivalent and a
    // re-arm on every fire would otherwise stack a new listener per DPR change.
    const onChange = () => {
      if (typeof mq.removeEventListener === "function") mq.removeEventListener("change", onChange);
      else if (typeof mq.removeListener === "function") mq.removeListener(onChange);
      dprWatch();
      armDprWatch();
    };

    // addEventListener is the modern surface; addListener is the deprecated one
    // some WebViews still ship. Neither is assumed.
    if (typeof mq.addEventListener === "function")
      mq.addEventListener("change", onChange);
    else if (typeof mq.addListener === "function")
      mq.addListener(onChange);
  }

  armDprWatch();
  window.addEventListener("resize", dprWatch);

  return {
    setGeometry(g) {
      geometry = g;
      relayout();
    },
    relayout,

    // ── v1.8.0 hooks. Hooks ONLY — this module keeps the view transform. ────

    /** getMotionTrace's payload, verbatim. Drift arrives as { cyclic: false,
        points: [] } and is drawn as a tail instead. */
    setTrace(payload) {
      trace = payload !== null && typeof payload === "object" ? payload : null;
      tail.length = 0;
      if (geometry !== null && view !== null) renderPuck();
    },

    /** The ghost anchor. The puck keeps every gesture — it is still the thing
        you drag — and goes hollow while the audible source is elsewhere. */
    setMotionOn(on) {
      motionOn = on === true;
      puck.classList.toggle("puck--ghost", motionOn);
      if (!motionOn) tail.length = 0;
      if (geometry !== null && view !== null) renderPuck();
    },

    /** The polled offset triple (anchor-relative metres) from getMeters. */
    setMotion(offset, on) {
      if (Array.isArray(offset) && offset.length >= 3)
        live = [Number(offset[0]) || 0, Number(offset[1]) || 0, Number(offset[2]) || 0];

      if (on !== motionOn) this.setMotionOn(on);
      if (geometry === null || view === null) return;

      if (motionOn && trace !== null && trace.cyclic === false) {
        const a = anchorMetres();
        tail.push({ x: a.x + live[0], y: a.y + live[1] });
        if (tail.length > TAIL_LENGTH) tail.shift();
      }

      renderLive(anchorMetres());
    },

    // ── Phase 3.3 hooks. Hooks ONLY — this module keeps the view transform. ──
    setMeters,
    setPreview,

    /** UI-04's blit, installed by app.js. Null keeps the 3.1 backdrop exactly
        as it was, which is what makes the descope a flag rather than a
        refactor (UI-04 criterion 4). */
    setFieldPainter(fn) {
      fieldPainter = typeof fn === "function" ? fn : null;
    },

    /** Repaint the raster layer only — the field arrives asynchronously and
        must not force a full relayout, which would refit the box and rewrite
        every glyph transform for a change that touched none of them. */
    redrawBackdrop() {
      drawBackdrop();
    },
  };
}
