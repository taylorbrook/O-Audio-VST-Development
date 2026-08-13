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
// O-Octagon — the side-elevation strip (Stage 3 Phase 3.3, UI-05)
//
// A SECTION THROUGH THE HALL. Depth runs left (FRONT) to right (REAR), height
// runs up. It answers the one question the plan cannot: where the audience
// plane sits, where the eight speakers hang above it, and where the source is
// relative to both.
//
// ── D26: THE ENDS ARE LABELLED, AND THAT IS NOT DECORATION ────────────────
// The Room plan's depth axis runs TOP-TO-BOTTOM; this one runs LEFT-TO-RIGHT.
// The two views are rotated 90 degrees relative to each other, so FRONT and
// REAR are written on the strip rather than relying on a shared orientation
// the two views do not have.
//
// ── NO EXAGGERATION FACTOR, AND NONE IS NEEDED (P76) ──────────────────────
// Measured on the 552 x 125 box against the default venue: depth 28.31 px/m,
// height 19.23 px/m, ratio 0.68. Compressed, not faked. Two scales is ordinary
// for a section drawing; what it requires is that the height axis be LABELLED,
// which it is. The §OQ4 grading 4.50 -> 5.40 m spans 17.3 px and is legible,
// and rakeRear moves the rear end at 19.23 px/m — a 0.5 m edit moves it 9.6 px,
// an order of magnitude more than a probe needs.
//
// ══ THE THREE CONSTRUCTION RULES, EACH CLOSING A NAMED TRAP ═══════════════
//
// RULE 1 — THE RAKE LINE IS DRAWN ONLY BETWEEN bbMinY AND bbMaxY.
//   earHeight() EXTRAPOLATES LINEARLY outside that span
//   (VenueModel.h:173-177 / plane::earHeight). A single line across the whole
//   ENVELOPE therefore has BOTH ends move when rakeRear moves — and UI-05
//   criterion 1's negative half is precisely that the FRONT endpoint does NOT
//   move, because earHeight(bbMinY) == rakeFront for any rakeRear
//   (RESEARCH-2.2 H5). The extrapolated continuation is a SEPARATE, dashed
//   element so the margins are not a lie and no assertion can confuse the two.
//   Section 41 gates the construction; NC7 draws one line across the whole
//   envelope and watches the negative half fail.
//
// RULE 2 — THE HEIGHT AXIS IS VENUE-DERIVED AND QUANTISED TO A 1 m STEP.
//   An axis that auto-fits rakeRear RESCALES when rakeRear changes, and
//   UI-05/1 would then be measuring a rescale instead of a move — passing its
//   positive half for the wrong reason. Quantising means an ordinary rake edit
//   never moves the axis at all, and the criterion's own "front endpoint
//   unchanged" half becomes the guard against a rescaling axis. Section 42
//   gates it.
//
// RULE 3 — THE MARKER CLAMPS; THE NUMBERS NEVER DO.
//   srcZ spans -2.0 .. 8.0 m (parameter-spec row 3), so absolute source height
//   reaches ~11.5 m — far above a 6.5 m axis. The dot pins to the axis edge
//   with a chevron; both numeric readouts stay exact. UI-05 criterion 2
//   already requires both readings shown, so the NUMBER is never the thing
//   that is clamped.
//
// ── ONE PROJECTION, BOTH AXES (P46 / §19) ─────────────────────────────────
// metresToPx() is imported from roomplan.js and is the only coordinate mapping
// in this file. The two scales are expressed as ONE VIEW with two spans, and
// the height axis is INVERTED in the view (minY = axisMax, maxY = 0) so that
// "up" is up without a second sign convention living here. earHeight is
// likewise imported — it is the JS twin of oo::plane::earHeight and is defined
// once, beside normToMetres, for the same reason.
// ============================================================================

import { metresToPx, makeView, normToMetres, earHeight } from "./roomplan.js";

// Rule 2's quantum. Metres.
const AXIS_STEP_M = 1;

// Headroom above the tallest thing in the room, before quantising. Enough that
// the top speaker is not drawn on the frame, small enough that the axis does
// not waste the 125 px it has.
const AXIS_HEADROOM_M = 1;

// The axis never collapses, however flat the rig.
const AXIS_MIN_M = 2;

// Left and right insets, in px, for the FRONT / REAR end labels.
const PAD_X = 26;

// Top and bottom insets, in px. The bottom carries the 0 m baseline.
const PAD_TOP = 10;
const PAD_BOTTOM = 16;

const SVG_NS = "http://www.w3.org/2000/svg";

function el(name, attrs) {
  const node = document.createElementNS(SVG_NS, name);
  for (const [k, v] of Object.entries(attrs)) node.setAttribute(k, String(v));
  return node;
}

export function createElevation(deps) {
  const stage = document.getElementById("elev-stage");
  const svg = document.getElementById("elev-strip");
  const axisGroup = document.getElementById("elev-axis");
  const rake = document.getElementById("elev-rake");
  const rakeExt = document.getElementById("elev-rake-ext");
  const speakerGroup = document.getElementById("elev-speakers");
  const markerStem = document.getElementById("elev-marker-stem");
  const markerDot = document.getElementById("elev-marker-dot");
  const chevron = document.getElementById("elev-marker-chevron");
  const endFront = document.getElementById("elev-end-front");
  const endRear = document.getElementById("elev-end-rear");
  const earOut = document.getElementById("elev-ear");
  const srcOut = document.getElementById("elev-src");

  if (stage === null || svg === null || axisGroup === null || rake === null
      || rakeExt === null || speakerGroup === null || markerStem === null
      || markerDot === null || chevron === null || endFront === null
      || endRear === null || earOut === null || srcOut === null)
    throw new Error("elevation: a required element is missing");

  const srcY = deps.sliders.get("srcY");
  const srcZ = deps.sliders.get("srcZ");

  if (srcY === undefined || srcZ === undefined)
    throw new Error("elevation: srcY / srcZ are not bound");

  let geometry = null;
  let view = null;
  let axisMaxM = AXIS_MIN_M;
  let box = { w: 0, h: 0 };

  // ── RULE 2 ───────────────────────────────────────────────────────────────
  // Derived from the venue — the tallest speaker and both rake ends — then
  // QUANTISED UP to the next whole metre. On the §OQ4 venue the tallest
  // speaker is 5.40 m and both rakes are 0, so the axis is 7 m and an ordinary
  // rake edit of half a metre does not move it at all.
  function deriveAxisMax(g) {
    let tallest = 0;
    for (const s of g.speakers) tallest = Math.max(tallest, Number(s.z) || 0);
    tallest = Math.max(tallest, Number(g.rake.front) || 0, Number(g.rake.rear) || 0);

    const wanted = Math.max(AXIS_MIN_M, tallest + AXIS_HEADROOM_M);
    return Math.max(AXIS_MIN_M, Math.ceil(wanted / AXIS_STEP_M) * AXIS_STEP_M);
  }

  function relayout() {
    if (geometry === null) return;

    // clientWidth / clientHeight, NOT getBoundingClientRect: .elev-stage
    // carries a 1 px border, and the rect includes it. Fitting to the rect
    // would size the strip 2 px larger than the box it has to live in — small
    // enough to look right and large enough to make section 21's guard report
    // green over a real 2 px overflow. The Room plan's stage has no border,
    // which is why it can use the rect.
    if (stage.clientWidth <= 0 || stage.clientHeight <= 0) return;   // hidden screen

    // The strip fills its stage. There is no aspect to fit — a section drawing
    // at two scales does not have one — so unlike the two plans this box is
    // the stage's own CONTENT box, floored. Section 21's fitted-box guard is
    // what proves that stays true; NC1 oversizes it by 120 px to prove the
    // guard fires while the document-level assertion still passes.
    box = { w: Math.floor(stage.clientWidth), h: Math.floor(stage.clientHeight) };
    if (!(box.w > 0) || !(box.h > 0)) return;

    svg.style.setProperty("--elev-w", String(box.w));
    svg.style.setProperty("--elev-h", String(box.h));
    svg.setAttribute("viewBox", `0 0 ${box.w} ${box.h}`);
    svg.setAttribute("width", String(box.w));
    svg.setAttribute("height", String(box.h));

    axisMaxM = deriveAxisMax(geometry);

    const e = geometry.envelope;

    // ONE VIEW, TWO SCALES. The x span is the envelope's DEPTH and the y span
    // is the HEIGHT AXIS, INVERTED — minY is the top of the axis and maxY is
    // the floor — so metresToPx returns screen coordinates with "up" up and no
    // second sign convention lives in this file.
    const drawW = Math.max(1, box.w - 2 * PAD_X);
    const drawH = Math.max(1, box.h - PAD_TOP - PAD_BOTTOM);

    view = makeView({ minX: e.minY, maxX: e.maxY, minY: axisMaxM, maxY: 0 }, drawW, drawH);

    draw();
  }

  // Depth metres + height metres -> strip pixels, through the ONE projection.
  function at(depthM, heightM) {
    const p = metresToPx(depthM, heightM, view);
    return { x: PAD_X + p.x, y: PAD_TOP + p.y };
  }

  function drawAxis() {
    while (axisGroup.firstChild !== null) axisGroup.removeChild(axisGroup.firstChild);

    for (let m = 0; m <= axisMaxM; m += AXIS_STEP_M) {
      const p = at(view.minX, m);
      axisGroup.appendChild(el("line", {
        class: "elev-axis-line", x1: PAD_X, y1: p.y, x2: box.w - PAD_X, y2: p.y,
      }));
      axisGroup.appendChild(el("text", {
        class: "elev-axis-label", x: 2, y: p.y + 3,
      })).textContent = `${m}`;
    }
  }

  function drawRake() {
    const b = geometry.bbox;
    const e = geometry.envelope;
    const r = geometry.rake;

    // ── RULE 1 ─────────────────────────────────────────────────────────────
    // THE SOLID LINE SPANS THE SPEAKER BOUNDING BOX AND NOTHING ELSE. Its two
    // endpoints ARE rakeFront at bbMinY and rakeRear at bbMaxY, which is what
    // makes UI-05/1's negative half a DOM fact: move rakeRear and x2/y2 move
    // while x1/y1 do not.
    const front = at(b.minY, r.front);
    const rear = at(b.maxY, r.rear);

    rake.setAttribute("x1", String(front.x));
    rake.setAttribute("y1", String(front.y));
    rake.setAttribute("x2", String(rear.x));
    rake.setAttribute("y2", String(rear.y));

    // The extrapolation, DASHED and on its own element. earHeight extrapolates
    // linearly past both ends, and drawing it makes the margins honest — but
    // it is never what an assertion reads, because BOTH of its ends move when
    // rakeRear moves and that is the trap rule 1 exists to close.
    const extA = at(e.minY, earHeight(e.minY, geometry));
    const extB = at(e.maxY, earHeight(e.maxY, geometry));

    rakeExt.setAttribute("x1", String(extA.x));
    rakeExt.setAttribute("y1", String(extA.y));
    rakeExt.setAttribute("x2", String(extB.x));
    rakeExt.setAttribute("y2", String(extB.y));
  }

  function drawSpeakers() {
    while (speakerGroup.firstChild !== null) speakerGroup.removeChild(speakerGroup.firstChild);

    // UI-05 criterion 3. The §OQ4 grading 4.50 -> 5.40 m is 17.3 px on this
    // box, which is why the heights are drawn at all rather than assumed flat:
    // a dropped (z_i - z_s)^2 term is invisible on a flat rig, and DSP-01/2
    // depends on these being different.
    //
    // ── COINCIDENT SPEAKERS: THE DOT IS EXACT, THE NUMERAL STEPS ASIDE ─────
    // A SECTION COLLAPSES THE LATERAL AXIS. On the §OQ4 rig the eight speakers
    // sit at four depths in mirrored pairs, so each pair lands on ONE point —
    // which is true, and which rendered as four dots with four numerals
    // stacked illegibly on top of each other.
    //
    // The DOTS stay exactly where the geometry puts them: moving one would
    // make the strip lie about a depth or a height, and it would break the
    // "as distinct as the returned z values" assertion that proves the grading
    // is visible. Only the LABEL steps aside, and only when a previous speaker
    // already occupies that point.
    const placed = [];

    for (const s of geometry.speakers) {
      const p = at(Number(s.y), Number(s.z));

      speakerGroup.appendChild(el("circle", {
        class: "elev-spk", cx: p.x, cy: p.y, r: 4.5, "data-speaker": s.n,
      }));

      const clash = placed.filter(q => Math.abs(q.x - p.x) < 9 && Math.abs(q.y - p.y) < 9).length;
      placed.push(p);

      speakerGroup.appendChild(el("text", {
        class: "elev-spk-num", x: p.x + (clash === 0 ? -5 : 5), y: p.y - 7,
      })).textContent = String(s.n);
    }
  }

  function drawMarker() {
    // The source's DEPTH comes from srcY through the same normToMetres the
    // plan and the footer readout use — never a second denormalisation. Only
    // the y half is read here, so the x argument is a placeholder: this strip
    // is a SECTION and has no lateral axis to place a source on.
    const depthM = normToMetres(0, srcY.state.getNormalisedValue(), geometry).y;

    const earM = earHeight(depthM, geometry);
    const srcM = earM + srcZ.state.getScaledValue();

    // ── RULE 3 ─────────────────────────────────────────────────────────────
    // The DOT clamps to the axis; the NUMBERS below do not. srcZ reaches
    // +8.00 m, which puts absolute source height ~11.5 m against a 7 m axis.
    const clamped = srcM > axisMaxM || srcM < 0;
    const shownM = Math.min(axisMaxM, Math.max(0, srcM));

    const ear = at(depthM, Math.min(axisMaxM, Math.max(0, earM)));
    const src = at(depthM, shownM);

    markerStem.setAttribute("x1", String(ear.x));
    markerStem.setAttribute("y1", String(ear.y));
    markerStem.setAttribute("x2", String(src.x));
    markerStem.setAttribute("y2", String(src.y));

    markerDot.setAttribute("cx", String(src.x));
    markerDot.setAttribute("cy", String(src.y));

    if (clamped) {
      const up = srcM > axisMaxM;
      const tip = up ? src.y - 8 : src.y + 8;
      const base = up ? src.y - 3 : src.y + 3;
      chevron.setAttribute("d", `M ${src.x - 5} ${base} L ${src.x} ${tip} L ${src.x + 5} ${base}`);
      chevron.removeAttribute("hidden");
    } else {
      chevron.setAttribute("hidden", "hidden");
    }

    // BOTH READINGS (UI-05/2). The ear height is what srcZ = 0 rides; the
    // source height is where the puck actually is. Exact, never clamped.
    earOut.textContent = `${earM.toFixed(2)} m`;
    srcOut.textContent = `${srcM.toFixed(2)} m`;
  }

  function drawEnds() {
    endFront.setAttribute("x", String(PAD_X));
    endFront.setAttribute("y", String(box.h - 3));
    endRear.setAttribute("x", String(box.w - PAD_X));
    endRear.setAttribute("y", String(box.h - 3));
  }

  function draw() {
    if (view === null || geometry === null) return;
    drawAxis();
    drawRake();
    drawSpeakers();
    drawMarker();
    drawEnds();
  }

  // The source marker follows the two parameters that move it, on the echo —
  // exactly as the puck does. Host automation therefore moves it too.
  srcY.state.valueChangedEvent.addListener(() => { if (view !== null) drawMarker(); });
  srcZ.state.valueChangedEvent.addListener(() => { if (view !== null) drawMarker(); });

  return {
    setGeometry(g) {
      geometry = g;
      relayout();
    },
    relayout,
    // Read back for the layout gate: the axis maximum is a VENUE-DERIVED,
    // QUANTISED number and section 42 asserts it does not move when rakeRear
    // does. Exposed rather than inferred from tick positions.
    axisMax: () => axisMaxM,
  };
}
