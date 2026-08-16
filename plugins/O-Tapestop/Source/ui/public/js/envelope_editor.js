/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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
// O-Tapestop — drawable bipolar speed-vs-time envelope editor (UI-01)
//
// Adapted from Path C §2.2 EnvelopeEditor (research/stutter-effects/
// path-c-playhead-modulator.md) with the required Stage-3 changes:
//   1. DPR backing store (the reference is DPR-naive) — house envResize()
//      pattern: rewrite canvas.width/height only when changed, setTransform,
//      draw in CSS px, mouse via getBoundingClientRect().
//   2. Bipolar y-axis: y ∈ [−1, +1], speed r = 2·y. Labelled 1× line at
//      y = +0.5, 0-line, tinted reverse zone (y < 0), labels +2×…−2×.
//   3. Per-segment curve HANDLES (the reference stores `curve` but has no
//      edit gesture): a diamond at each segment's visual midpoint; vertical
//      drag adjusts curve ∈ [−1, +1]; double-click resets to 0.
//   4. Sanitize parity with ScratchEnvelope.h: 2–64 points, endpoints pinned
//      at x = 0 / 1, min x-separation 0.01, clamps on x/y/curve. On commit
//      the caller redraws from the C++-sanitized echo, so any disagreement
//      resolves to the C++ truth.
//
// This module exports a CLASS ONLY — no top-level side effects, no touching
// of later let/const (pattern_module_toplevel_init_tdz). It has no Juce
// dependency, so it renders against a bridge stub unchanged.
// ============================================================================

const MIN_POINTS = 2;
const MAX_POINTS = 64;
const MIN_SEP    = 0.01;   // min x separation (mirrors the UI contract)
const HIT_R      = 9;      // px hit radius for points
const HANDLE_R   = 6;      // px hit radius for curve diamonds
const PAD_L      = 34;     // room for the axis labels
const PAD_R      = 10;
const PAD_T      = 10;
const PAD_B      = 10;
const SEG_STEPS  = 32;     // curve-preview samples per segment
const COMMIT_DEBOUNCE_MS = 50;

// Per-segment curve law — copied CHARACTER-FOR-CHARACTER from
// plugins/O-Tapestop/Source/dsp/ScratchEnvelope.h:242-247 (valueAt). These two
// must stay in step; a drifted preview keeps looking plausible, which is why
// every commit redraws from the C++-sanitized echo rather than trusting this.
function applyCurve(t, curve) {
  if (Math.abs(curve) > 0.01) {
    if (curve > 0.0)
      t = Math.pow(t, 1.0 + curve * 2.0);
    else
      t = 1.0 - Math.pow(1.0 - t, 1.0 - curve * 2.0);
  }
  return t;
}

export class EnvelopeEditor {
  /**
   * @param {HTMLCanvasElement} canvas
   * @param {(json: string) => void} onCommit — called with the serialized
   *        envelope on mouse-up, debounced 50 ms. The caller is expected to
   *        push the C++-sanitized echo back through setFromJson().
   */
  constructor(canvas, onCommit) {
    this.canvas = canvas;
    this.ctx = canvas.getContext("2d");
    this.onCommit = onCommit || null;

    // Default mirrors ScratchEnvelope::defaultPoints() (gentle wobble) so a
    // stub-rendered page shows the same curve a fresh instance bakes.
    this.points = [
      { x: 0.00, y: 0.50, curve: 0.0 },
      { x: 0.25, y: 0.65, curve: 0.0 },
      { x: 0.50, y: 0.35, curve: 0.0 },
      { x: 0.75, y: 0.60, curve: 0.0 },
      { x: 1.00, y: 0.50, curve: 0.0 },
    ];

    this.playhead = null;      // φ ∈ [0,1] while a scratch pass runs, else null
    this.dragPoint = -1;       // index of the point being dragged
    this.dragHandle = -1;      // index of the LEFT point of the segment whose
                               // curve handle is being dragged
    this.dragStartY = 0;
    this.dragStartCurve = 0;
    this.commitTid = null;

    this.onPointerDown = this.onPointerDown.bind(this);
    this.onPointerMove = this.onPointerMove.bind(this);
    this.onPointerUp = this.onPointerUp.bind(this);
    this.onDblClick = this.onDblClick.bind(this);
    this.onContextMenu = this.onContextMenu.bind(this);

    canvas.addEventListener("pointerdown", this.onPointerDown);
    canvas.addEventListener("pointermove", this.onPointerMove);
    canvas.addEventListener("pointerup", this.onPointerUp);
    canvas.addEventListener("pointercancel", this.onPointerUp);
    canvas.addEventListener("lostpointercapture", this.onPointerUp);
    canvas.addEventListener("dblclick", this.onDblClick);
    canvas.addEventListener("contextmenu", this.onContextMenu);

    this.draw();
  }

  // ── Serialization (ScratchEnvelope JSON contract: {"v":1,"points":[…]}) ───
  toJson() {
    return JSON.stringify({
      v: 1,
      points: this.points.map((p) => ({ x: p.x, y: p.y, curve: p.curve })),
    });
  }

  /** Replace the model from a JSON string or parsed object. Invalid input is
      ignored (the current curve stays — the C++ side is the truth keeper). */
  setFromJson(raw) {
    try {
      const obj = typeof raw === "string" ? JSON.parse(raw) : raw;
      if (!obj || !Array.isArray(obj.points) || obj.points.length < MIN_POINTS) return;

      const pts = obj.points
        .slice(0, MAX_POINTS)
        .map((p) => ({
          x: Math.min(1, Math.max(0, Number(p.x) || 0)),
          y: Math.min(1, Math.max(-1, Number(p.y) || 0)),
          curve: Math.min(1, Math.max(-1, Number(p.curve) || 0)),
        }))
        .sort((a, b) => a.x - b.x);

      pts[0].x = 0;
      pts[pts.length - 1].x = 1;

      this.points = pts;
      this.draw();
    } catch (e) {
      console.error("EnvelopeEditor.setFromJson: invalid payload", e);
    }
  }

  /** φ ∈ [0,1] to show the pass playhead, or null to hide it. Skips the
      redraw when nothing changed (steady-state costs nothing). */
  setPlayhead(phi) {
    const next = phi == null ? null : Math.min(1, Math.max(0, Number(phi)));
    if (next === this.playhead) return;
    this.playhead = next;
    this.draw();
  }

  destroy() {
    clearTimeout(this.commitTid);
    const c = this.canvas;
    c.removeEventListener("pointerdown", this.onPointerDown);
    c.removeEventListener("pointermove", this.onPointerMove);
    c.removeEventListener("pointerup", this.onPointerUp);
    c.removeEventListener("pointercancel", this.onPointerUp);
    c.removeEventListener("lostpointercapture", this.onPointerUp);
    c.removeEventListener("dblclick", this.onDblClick);
    c.removeEventListener("contextmenu", this.onContextMenu);
  }

  // ── Geometry ──────────────────────────────────────────────────────────────
  // House envResize() pattern: size the backing store for the DPR only when it
  // changed (assignment CLEARS the canvas), then draw in CSS px.
  resize() {
    const dpr = window.devicePixelRatio || 1;
    const w = this.canvas.clientWidth || 380;
    const h = this.canvas.clientHeight || 224;

    const bw = Math.round(w * dpr);
    const bh = Math.round(h * dpr);

    if (this.canvas.width !== bw || this.canvas.height !== bh) {
      this.canvas.width = bw;
      this.canvas.height = bh;
    }

    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    return { w, h };
  }

  plotRect() {
    const w = this.canvas.clientWidth || 380;
    const h = this.canvas.clientHeight || 224;
    return { x0: PAD_L, y0: PAD_T, pw: w - PAD_L - PAD_R, ph: h - PAD_T - PAD_B };
  }

  // Bipolar mapping: canvasY = (1 − (y+1)/2)·h within the plot rect.
  xToPx(x) { const r = this.plotRect(); return r.x0 + x * r.pw; }
  yToPx(y) { const r = this.plotRect(); return r.y0 + (1 - (y + 1) / 2) * r.ph; }
  pxToX(px) { const r = this.plotRect(); return Math.min(1, Math.max(0, (px - r.x0) / r.pw)); }
  pxToY(py) { const r = this.plotRect(); return Math.min(1, Math.max(-1, 1 - 2 * ((py - r.y0) / r.ph))); }

  valueAt(x) {
    const pts = this.points;
    let rightIdx = pts.length - 1;
    for (let i = 0; i < pts.length; i += 1) {
      if (pts[i].x >= x) { rightIdx = i; break; }
    }
    const leftIdx = rightIdx > 0 ? rightIdx - 1 : 0;
    if (leftIdx === rightIdx) return pts[leftIdx].y;

    const left = pts[leftIdx];
    const right = pts[rightIdx];
    const width = right.x - left.x;
    if (width <= 0) return right.y;

    const t = applyCurve((x - left.x) / width, left.curve);
    return left.y + (right.y - left.y) * t;
  }

  // ── Hit tests (CSS px via getBoundingClientRect) ──────────────────────────
  eventPos(e) {
    const rect = this.canvas.getBoundingClientRect();
    return { px: e.clientX - rect.left, py: e.clientY - rect.top };
  }

  hitPoint(px, py) {
    for (let i = 0; i < this.points.length; i += 1) {
      const dx = this.xToPx(this.points[i].x) - px;
      const dy = this.yToPx(this.points[i].y) - py;
      if (dx * dx + dy * dy <= HIT_R * HIT_R) return i;
    }
    return -1;
  }

  hitHandle(px, py) {
    for (let i = 0; i < this.points.length - 1; i += 1) {
      const m = this.handlePos(i);
      const dx = m.px - px;
      const dy = m.py - py;
      if (dx * dx + dy * dy <= (HANDLE_R + 3) * (HANDLE_R + 3)) return i;
    }
    return -1;
  }

  handlePos(segIdx) {
    const left = this.points[segIdx];
    const right = this.points[segIdx + 1];
    const midX = (left.x + right.x) / 2;
    return { px: this.xToPx(midX), py: this.yToPx(this.valueAt(midX)) };
  }

  // ── Interaction ───────────────────────────────────────────────────────────
  onPointerDown(e) {
    const { px, py } = this.eventPos(e);

    // alt-click / right-click: delete (endpoints and the 2-point floor are
    // protected — sanitize parity with ScratchEnvelope).
    if (e.altKey || e.button === 2) {
      const idx = this.hitPoint(px, py);
      if (idx > 0 && idx < this.points.length - 1 && this.points.length > MIN_POINTS) {
        this.points.splice(idx, 1);
        this.draw();
        this.scheduleCommit();
      }
      e.preventDefault();
      return;
    }

    if (e.button !== 0) return;

    const pIdx = this.hitPoint(px, py);
    if (pIdx >= 0) {
      this.dragPoint = pIdx;
      try { this.canvas.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
      e.preventDefault();
      return;
    }

    const hIdx = this.hitHandle(px, py);
    if (hIdx >= 0) {
      this.dragHandle = hIdx;
      this.dragStartY = py;
      this.dragStartCurve = this.points[hIdx].curve;
      try { this.canvas.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
      e.preventDefault();
    }
  }

  onPointerMove(e) {
    if (this.dragPoint < 0 && this.dragHandle < 0) return;

    const { px, py } = this.eventPos(e);

    if (this.dragPoint >= 0) {
      const i = this.dragPoint;
      const pt = this.points[i];
      const last = this.points.length - 1;

      pt.y = this.pxToY(py);

      // Endpoints stay pinned at x = 0 / 1; interior points are clamped
      // between their neighbours with the 0.01 minimum separation.
      if (i > 0 && i < last) {
        const lo = this.points[i - 1].x + MIN_SEP;
        const hi = this.points[i + 1].x - MIN_SEP;
        pt.x = Math.min(hi, Math.max(lo, this.pxToX(px)));
      }

      this.draw();
      e.preventDefault();
      return;
    }

    if (this.dragHandle >= 0) {
      const i = this.dragHandle;
      const left = this.points[i];
      const right = this.points[i + 1];

      // Drag up bows the segment up: for an ascending segment that means
      // easing OUT (curve < 0 reaches the right value early); for a
      // descending one it means easing IN (curve > 0 holds the left value).
      const sign = right.y >= left.y ? -1 : 1;
      const delta = ((this.dragStartY - py) / 80) * sign;
      left.curve = Math.min(1, Math.max(-1, this.dragStartCurve + delta));

      this.draw();
      e.preventDefault();
    }
  }

  onPointerUp(e) {
    if (this.dragPoint < 0 && this.dragHandle < 0) return;

    this.dragPoint = -1;
    this.dragHandle = -1;

    if (e && e.pointerId !== undefined) {
      try { this.canvas.releasePointerCapture(e.pointerId); } catch (_) { /* already released */ }
    }

    this.scheduleCommit();
  }

  onDblClick(e) {
    const { px, py } = this.eventPos(e);

    // Double-click on a curve handle resets that segment to linear.
    const hIdx = this.hitHandle(px, py);
    if (hIdx >= 0 && this.hitPoint(px, py) < 0) {
      this.points[hIdx].curve = 0;
      this.draw();
      this.scheduleCommit();
      e.preventDefault();
      return;
    }

    // Double-click on empty canvas adds a point (max 64, min separation kept).
    if (this.hitPoint(px, py) >= 0) return;
    if (this.points.length >= MAX_POINTS) return;

    const x = this.pxToX(px);
    const y = this.pxToY(py);

    let insertAt = this.points.length - 1;
    for (let i = 0; i < this.points.length; i += 1) {
      if (this.points[i].x > x) { insertAt = i; break; }
    }

    const loNeighbour = this.points[insertAt - 1];
    const hiNeighbour = this.points[insertAt];
    if (x - loNeighbour.x < MIN_SEP || hiNeighbour.x - x < MIN_SEP) return;

    this.points.splice(insertAt, 0, { x, y, curve: 0 });
    this.draw();
    this.scheduleCommit();
    e.preventDefault();
  }

  onContextMenu(e) {
    e.preventDefault();   // delete handled in pointerdown (button === 2)
  }

  scheduleCommit() {
    if (!this.onCommit) return;
    clearTimeout(this.commitTid);
    this.commitTid = setTimeout(() => this.onCommit(this.toJson()), COMMIT_DEBOUNCE_MS);
  }

  // ── Rendering ─────────────────────────────────────────────────────────────
  draw() {
    const ctx = this.ctx;
    if (!ctx) return;

    const { w, h } = this.resize();
    const { x0, y0, pw, ph } = this.plotRect();

    // Palette from the page's own custom properties, never hardcoded hexes.
    const css = getComputedStyle(document.documentElement);
    const ink      = css.getPropertyValue("--green-dark").trim() || "#3C5C1A";
    const rule     = css.getPropertyValue("--brown-border").trim() || "#8B7355";
    const frame    = css.getPropertyValue("--brown-frame").trim() || "#5C4033";
    const reverse  = css.getPropertyValue("--red-brown").trim() || "#9A4A32";
    const playInk  = css.getPropertyValue("--green-mid").trim() || "#6B8E4E";

    ctx.clearRect(0, 0, w, h);

    // Reverse zone (y < 0): faded red-brown wash.
    ctx.save();
    ctx.globalAlpha = 0.07;
    ctx.fillStyle = reverse;
    ctx.fillRect(x0, this.yToPx(0), pw, y0 + ph - this.yToPx(0));
    ctx.restore();

    // Gridlines at r = +2, +1, 0, −1, −2 (y = ±1, ±0.5, 0). The 1× line
    // (y = +0.5) is the prominent "normal speed" reference; the 0-line marks
    // the stop.
    const gridRows = [
      { y: 1.0,  label: "+2×", strong: false },
      { y: 0.5,  label: "1×",  strong: true  },
      { y: 0.0,  label: "0",        strong: true  },
      { y: -0.5, label: "−1×", strong: false },
      { y: -1.0, label: "−2×", strong: false },
    ];

    ctx.save();
    ctx.font = "9px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "right";
    ctx.textBaseline = "middle";

    for (const row of gridRows) {
      const py = Math.round(this.yToPx(row.y)) + 0.5;

      ctx.strokeStyle = row.strong ? frame : rule;
      ctx.globalAlpha = row.strong ? (row.y === 0.5 ? 0.8 : 0.55) : 0.3;
      ctx.lineWidth = row.y === 0.5 ? 1.4 : 1;
      ctx.setLineDash(row.strong ? [] : [2, 3]);
      ctx.beginPath();
      ctx.moveTo(x0, py);
      ctx.lineTo(x0 + pw, py);
      ctx.stroke();

      ctx.globalAlpha = 0.75;
      ctx.fillStyle = frame;
      ctx.fillText(row.label, x0 - 6, py);
    }
    ctx.restore();

    // The curve itself, sampled per segment through the verbatim curve law.
    ctx.save();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 1.8;
    ctx.lineJoin = "round";
    ctx.beginPath();

    for (let i = 0; i < this.points.length - 1; i += 1) {
      const left = this.points[i];
      const right = this.points[i + 1];

      if (i === 0) ctx.moveTo(this.xToPx(left.x), this.yToPx(left.y));

      for (let s = 1; s <= SEG_STEPS; s += 1) {
        const t = applyCurve(s / SEG_STEPS, left.curve);
        const x = left.x + (right.x - left.x) * (s / SEG_STEPS);
        const y = left.y + (right.y - left.y) * t;
        ctx.lineTo(this.xToPx(x), this.yToPx(y));
      }
    }
    ctx.stroke();
    ctx.restore();

    // Curve handles: a small diamond at each segment's visual midpoint.
    ctx.save();
    ctx.fillStyle = rule;
    ctx.strokeStyle = frame;
    ctx.lineWidth = 1;
    for (let i = 0; i < this.points.length - 1; i += 1) {
      const m = this.handlePos(i);
      ctx.globalAlpha = Math.abs(this.points[i].curve) > 0.01 ? 0.95 : 0.5;
      ctx.beginPath();
      ctx.moveTo(m.px, m.py - HANDLE_R + 1);
      ctx.lineTo(m.px + HANDLE_R - 1, m.py);
      ctx.lineTo(m.px, m.py + HANDLE_R - 1);
      ctx.lineTo(m.px - HANDLE_R + 1, m.py);
      ctx.closePath();
      ctx.fill();
      ctx.stroke();
    }
    ctx.restore();

    // Points: forward points in ink, reverse (y < 0) in the red-brown.
    ctx.save();
    for (let i = 0; i < this.points.length; i += 1) {
      const pt = this.points[i];
      ctx.fillStyle = pt.y < 0 ? reverse : ink;
      ctx.strokeStyle = frame;
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.arc(this.xToPx(pt.x), this.yToPx(pt.y), 4.5, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();
    }
    ctx.restore();

    // Pass playhead — drawn LAST, over the curve.
    if (this.playhead != null) {
      const px = Math.round(this.xToPx(this.playhead)) + 0.5;
      ctx.save();
      ctx.strokeStyle = playInk;
      ctx.globalAlpha = 0.9;
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(px, y0);
      ctx.lineTo(px, y0 + ph);
      ctx.stroke();
      ctx.restore();
    }
  }
}
