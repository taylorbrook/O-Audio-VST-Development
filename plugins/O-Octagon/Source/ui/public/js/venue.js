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
// O-Octagon — the Venue screen (Stage 3 Phase 3.2)
//
// The plugin stops being a renderer of state and becomes an EDITOR of it: 42
// measured values, a named file to keep them in, a preset store that does not
// disturb them, and a ping that confirms every speaker by ear.
//
// ── ONE setVenue CALL CARRYING ALL 42 (CONTEXT-3.2 D8) ────────────────────
// There is no per-field write surface, and that is P38's torn-read argument
// arriving on the write side: 42 async round trips whose promises may resolve
// out of order, against a model that recomputes bbox, centroid, rigScale and
// the convex hull on every one of them. Blur or Enter commits; Escape reverts.
//
// ── VALIDATE BEFORE APPLY, BECAUSE mapInvalid IS AUDIBLE ──────────────────
// RESEARCH-3.2 N8, and it is live in shipped Stage-2 code: mappedOutputAvailable()
// false sends GainStage to its else arm, which writes
// out[ch][n] = ch == 0 ? sL : sR with numWrite 8. SPEAKER 1 GETS THE LEFT INPUT
// AND SPEAKERS 2 THROUGH 8 ALL GET THE RIGHT ONE, AT UNITY. Under commit-on-blur
// a label swap would hold that state for as long as the user takes to type the
// second label. So while the label set is not a permutation this page does not
// call setVenue AT ALL, and applyVenueEditChecked() in C++ is the backstop for
// the backstop — the page can be wrong (a bug) and the state can arrive from
// elsewhere (a session restore).
//
// ── THE LABEL COLUMN HOLDS AND MARKS; EVERY OTHER COLUMN REVERTS ──────────
// D12 fixed "reject means revert, not hold", and that is right for a numeric
// field: a field holding invalid text describes a room that does not exist.
// It is WRONG for the label column, and the reason is reachability, not taste.
// EVERY ROUTE FROM (L, R) TO (R, L) PASSES THROUGH A DUPLICATE. A label that
// reverts on collision makes swapping two speakers UNREACHABLE.
//
// ── NO UI STATE DEPENDS SOLELY ON A PROMISE RESOLVING (RESEARCH-3.2 N4) ───
// emitCompletionEvent calls emitEventIfBrowserIsVisible, which DROPS the event
// when the component is hidden: no error, no rejection, no log, and the await
// simply never returns — inside a page that withKeepPageLoadedWhenBrowserIsHidden()
// has kept alive to have awaited it. Every write below uses its promise only for
// ADVISORY rendering (a rejection reason, a chosen filename). The authoritative
// state converges on the venueGen poll app.js already runs. The ping poll is
// started BEFORE its start promise is awaited for exactly this reason.
//
// ── A SECOND VIEW, NEVER A SECOND PROJECTION (RESEARCH-3.2 Q8) ────────────
// The mini-plan is drawn by roomplan.js's metresToPx() through a view built by
// its makeView() and fitted by its fitBox(). Section 19 of the static gate is
// widened by that rather than weakened.
// ============================================================================

import { metresToPx, fitBox, makeView } from "./roomplan.js";

// ── Module-level bindings. Nothing here runs at module-evaluation time except
//    the declarations themselves; app.js calls createVenueScreen() from inside
//    its own init(), in its own try/catch (pattern_module_toplevel_init_tdz). ──

const SPEAKER_COUNT = 8;

// The four NUMERIC per-speaker columns. The label column is deliberately not in
// this list: it is the one column with different commit semantics.
const NUMERIC_KEYS = ["x", "y", "z", "trim"];

// 100 ms, and ONLY while pinging. app.js's STATUS_POLL_MS is 500, which is
// LONGER than the 400 ms auto-cycle gap — the existing poll can miss a gap
// entirely and lag a speaker change by half a period. A push transport was
// rejected: emitEvent IS emitEventIfBrowserIsVisible, so a dropped push never
// retries where a poll self-heals on its next tick (PLAN-3.2 P61).
const PING_POLL_MS = 100;

// ── D12's EXPLICIT PARSE ───────────────────────────────────────────────────
// Number.parseFloat("7.25abc") returns 7.25, so parseFloat alone accepts
// trailing junk. The guard is the point: a coordinate that silently became 7.25
// because the operator fat-fingered a unit is a measurement error that
// propagates into the DBAP solve with nothing on screen to show for it.
//
// This is also why the fields are type="text": on invalid content a number
// input reports .value === "" and .valueAsNumber === NaN, so "typed abc" and
// "cleared the field" are indistinguishable and FUNC-02 criterion 1 becomes
// untestable.
function parseNumber(text) {
  const s = String(text).trim();
  if (s === "") return null;
  if (!/^[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?$/.test(s)) return null;

  const v = Number.parseFloat(s);
  return Number.isFinite(v) ? v : null;
}

const fmt = (v, dp) => Number(v).toFixed(dp);

export function createVenueScreen(deps) {
  const nativeFn = deps.nativeFn;
  if (typeof nativeFn !== "function") throw new Error("venue screen: nativeFn is not wired");

  // ── Elements ─────────────────────────────────────────────────────────────
  const need = (id) => {
    const node = document.getElementById(id);
    if (node === null) throw new Error(`venue screen: missing element ${id}`);
    return node;
  };

  const rows = [];
  for (let n = 1; n <= SPEAKER_COUNT; ++n) {
    const row = { n, fields: {}, classNode: need(`vclass-${n}`), pingButton: need(`btn-ping-${n}`) };
    row.fields.label = need(`vf-label-${n}`);
    for (const key of NUMERIC_KEYS) row.fields[key] = need(`vf-${n}-${key}`);
    rows.push(row);
  }

  const rakeFront = need("vf-rake-front");
  const rakeRear = need("vf-rake-rear");

  const venueNameNode = need("vvenue-name");
  const miniStage = need("miniplan");
  const miniSvg = need("mini-geometry");
  const miniHull = need("mini-hull");
  const miniGlyphs = rows.map((r) => need(`mglyph-${r.n}`));

  const presetSelect = need("preset-list");
  const presetCurrentNode = need("vpreset-current");
  const pingStateNode = need("vping-state");

  // ── State ────────────────────────────────────────────────────────────────
  // `committed` is the last model the PLUGIN reported. `pending` holds raw text
  // the operator has typed and that has not been accepted yet. A geometry
  // refresh repaints from `committed` and never stamps on a pending edit or on
  // the field currently under the cursor.
  let geometry = null;
  let committed = null;
  const pending = new Map();
  let pingTimer = null;

  const fieldId = (n, key) => (key === "rakeFront" ? "vf-rake-front"
    : key === "rakeRear" ? "vf-rake-rear"
      : key === "label" ? `vf-label-${n}` : `vf-${n}-${key}`);

  // ── Reading the model out of committed + pending ──────────────────────────

  function pendingNumber(n, key) {
    const raw = pending.get(fieldId(n, key));
    return raw === undefined ? undefined : parseNumber(raw) ?? undefined;
  }

  function labelOf(n) {
    const raw = pending.get(fieldId(n, "label"));
    return raw === undefined ? committed.speakers[n - 1].label : String(raw).trim();
  }

  // The 42 values, assembled ONCE, for the ONE setVenue call site below.
  function buildPayload() {
    const speakers = rows.map((r) => {
      const base = committed.speakers[r.n - 1];
      return {
        n: r.n,
        label: labelOf(r.n),
        x: pendingNumber(r.n, "x") ?? base.x,
        y: pendingNumber(r.n, "y") ?? base.y,
        z: pendingNumber(r.n, "z") ?? base.z,
        trimDb: pendingNumber(r.n, "trim") ?? base.trimDb,
      };
    });

    const front = pendingNumber(0, "rakeFront") ?? committed.rake.front;
    const rear = pendingNumber(0, "rakeRear") ?? committed.rake.rear;

    return { speakers, rake: { front, rear } };
  }

  // ── THE LABEL PREDICATE ──────────────────────────────────────────────────
  // "Is this a permutation of the negotiated output set?" is answered against
  // the COMMITTED labels rather than against a transcribed list of channel
  // names. The committed set is by construction a valid resolution of whatever
  // the host negotiated, so a permutation of it is a permutation of that set —
  // and the page never has to carry a second copy of a rule that lives in
  // buildSpeakerToBuffer() (pattern_test_fixture_mirrors_drift_silently).
  //
  // Returns the set of 0-based rows to MARK. Empty means "commit may proceed".
  function collidingRows() {
    const base = committed.speakers.map((s) => s.label);
    const next = rows.map((r) => labelOf(r.n));

    const counts = new Map();
    for (const l of next) counts.set(l, (counts.get(l) ?? 0) + 1);

    const marks = new Set();
    next.forEach((l, i) => {
      if (!base.includes(l) || counts.get(l) > 1) marks.add(i);
    });
    return marks;
  }

  function applyLabelMarks(marks) {
    rows.forEach((r, i) => r.fields.label.classList.toggle("is-colliding", marks.has(i)));
  }

  // ── Rendering ────────────────────────────────────────────────────────────

  function paintFields() {
    if (committed === null) return;

    for (const r of rows) {
      const s = committed.speakers[r.n - 1];
      const painted = [
        [r.fields.label, s.label],
        [r.fields.x, fmt(s.x, 2)],
        [r.fields.y, fmt(s.y, 2)],
        [r.fields.z, fmt(s.z, 2)],
        [r.fields.trim, fmt(s.trimDb, 1)],
      ];

      for (const [input, text] of painted) {
        // Never stamp on an edit in progress, and never on the focused field.
        if (pending.has(input.id) || document.activeElement === input) continue;
        input.value = text;
      }

      const value = r.classNode;
      value.textContent = String(s.class ?? "");
    }

    for (const [input, text] of [[rakeFront, fmt(committed.rake.front, 2)],
                                 [rakeRear, fmt(committed.rake.rear, 2)]]) {
      if (pending.has(input.id) || document.activeElement === input) continue;
      input.value = text;
    }

    const el = venueNameNode;
    el.textContent = String(geometry === null ? "" : geometry.venueName ?? "");
  }

  // The mini-plan. Fitted to the SMALLER of its two bounds by the same fitBox()
  // the Room plan uses — a width-bound portrait plan wants 348 px of height in a
  // rail that has 337, and overflows it while document.scrollHeight stays 720
  // (Q11). Section 11 of the layout gate measures the RAIL for exactly that.
  function drawMini() {
    if (geometry === null) return;

    const rect = miniStage.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return;   // hidden screen

    const e = geometry.envelope;
    const spanX = e.maxX - e.minX;
    const spanY = e.maxY - e.minY;
    if (!(spanX > 0) || !(spanY > 0)) return;

    const { w, h } = fitBox(rect.width, rect.height, spanX / spanY);
    if (!(w > 0) || !(h > 0)) return;

    miniStage.style.setProperty("--mini-w", String(w));
    miniStage.style.setProperty("--mini-h", String(h));
    miniSvg.setAttribute("viewBox", `0 0 ${w} ${h}`);

    const view = makeView(e, w, h);

    miniHull.setAttribute(
      "points",
      geometry.hull
        .map((p) => {
          const q = metresToPx(p.x, p.y, view);
          return `${q.x},${q.y}`;
        })
        .join(" "),
    );

    geometry.speakers.forEach((s, i) => {
      const g = miniGlyphs[i];
      if (g === undefined || g === null) return;

      const p = metresToPx(s.x, s.y, view);
      g.setAttribute("transform", `translate(${p.x} ${p.y})`);

      g.classList.toggle("is-vertex", s.class === "VERTEX");
      g.classList.toggle("is-onedge", s.class === "ON_EDGE");
      g.classList.toggle("is-interior", s.class === "INTERIOR");
    });
  }

  // ── THE ONE WRITE PATH ───────────────────────────────────────────────────
  // Called only when the label set is a permutation. There is exactly one
  // setVenue call site in this file and section 22 of the static gate asserts
  // it, together with the absence of any per-field write surface.
  function commit() {
    if (committed === null) return;

    const marks = collidingRows();
    applyLabelMarks(marks);
    if (marks.size > 0) return;   // BLOCKED — the plugin is never asked

    const payload = buildPayload();

    // The promise is ADVISORY. The authoritative effect — a new venueGen, a new
    // geometry, a repainted table — arrives on the poll app.js already runs, so
    // a dropped completion leaves the UI correct rather than stale (N4 / P64).
    nativeFn("setVenue")(payload)
      .then((result) => {
        if (result !== null && typeof result === "object" && result.ok === false) {
          const marked = Number(result.speaker);
          if (Number.isFinite(marked) && marked >= 0 && marked < SPEAKER_COUNT)
            rows[marked].fields.label.classList.add("is-colliding");
        }
      })
      .catch((err) => console.error("setVenue failed", err));

    pending.clear();
  }

  // ── Field behaviour ──────────────────────────────────────────────────────

  function bindNumeric(input, revertText) {
    input.addEventListener("input", () => {
      pending.set(input.id, input.value);
      input.classList.remove("is-invalid");
    });

    input.addEventListener("blur", () => {
      const raw = pending.get(input.id);
      if (raw === undefined) return;

      // REVERT, do not hold. A numeric field left holding invalid text
      // describes a room that does not exist (D12).
      if (parseNumber(raw) === null) {
        pending.delete(input.id);
        input.value = revertText();
        input.classList.add("is-invalid");
        return;
      }

      input.classList.remove("is-invalid");
      commit();
    });

    input.addEventListener("keydown", (e) => {
      if (e.key === "Enter") { input.blur(); return; }
      if (e.key !== "Escape") return;

      pending.delete(input.id);
      input.value = revertText();
      input.classList.remove("is-invalid");
      input.blur();
    });
  }

  function bindLabel(row) {
    const input = row.fields.label;

    input.addEventListener("input", () => {
      pending.set(input.id, input.value);
      // Mark live, so both colliding rows light up while the operator is still
      // holding the first half of a swap.
      applyLabelMarks(collidingRows());
    });

    // HOLD, do not revert. Reverting would make L <-> R unreachable, because
    // every route from (L, R) to (R, L) passes through a duplicate (P53).
    input.addEventListener("blur", () => {
      if (!pending.has(input.id)) return;
      commit();
    });

    input.addEventListener("keydown", (e) => {
      if (e.key === "Enter") { input.blur(); return; }
      if (e.key !== "Escape") return;

      pending.delete(input.id);
      input.value = committed === null ? "" : committed.speakers[row.n - 1].label;
      applyLabelMarks(collidingRows());
      input.blur();
    });
  }

  for (const r of rows) {
    bindLabel(r);
    for (const key of NUMERIC_KEYS) {
      const dp = key === "trim" ? 1 : 2;
      bindNumeric(r.fields[key], () => fmt(committed.speakers[r.n - 1][key === "trim" ? "trimDb" : key], dp));
    }
  }

  bindNumeric(rakeFront, () => fmt(committed.rake.front, 2));
  bindNumeric(rakeRear, () => fmt(committed.rake.rear, 2));

  // ── The .venue store ─────────────────────────────────────────────────────
  // Both open a NATIVE MODAL through FileChooser::launchAsync on the C++ side.
  // Nothing here waits on the chooser's promise: a LOAD that succeeds moves
  // venueGen, and the table repaints from the refresh that follows.

  need("btn-venue-save").addEventListener("click", () => {
    nativeFn("saveVenue")().catch((err) => console.error("saveVenue failed", err));
  });

  need("btn-venue-load").addEventListener("click", () => {
    nativeFn("loadVenue")().catch((err) => console.error("loadVenue failed", err));
  });

  // ── The musical preset store ─────────────────────────────────────────────
  // FOUR native functions and O-Octagon's own DOM. createPresetBar() is never
  // called: it writes container.innerHTML and then queries its own injected
  // markup, which erases authored labels by construction
  // (pattern_js_state_updater_overwrites_html_labels). The options below are
  // built with createElement and appended.

  function renderPresets(payload) {
    if (payload === null || typeof payload !== "object") return;

    const names = Array.isArray(payload.presets) ? payload.presets : [];
    const keep = presetSelect.value;

    while (presetSelect.firstChild !== null) presetSelect.removeChild(presetSelect.firstChild);

    for (const name of names) {
      const value = document.createElement("option");
      value.value = String(name);
      value.textContent = String(name);
      presetSelect.appendChild(value);
    }

    if (names.includes(keep)) presetSelect.value = keep;
    else if (typeof payload.current === "string") presetSelect.value = payload.current;

    const el = presetCurrentNode;
    el.textContent = String(payload.current ?? "");
  }

  async function refreshPresets() {
    try {
      renderPresets(await nativeFn("getPresetList")());
      const current = await nativeFn("getCurrentPreset")();
      if (current !== null && typeof current === "object") {
        const el = presetCurrentNode;
        el.textContent = String(current.name ?? "");
      }
    } catch (err) {
      console.error("preset list failed", err);
    }
  }

  need("btn-preset-save").addEventListener("click", () => {
    const name = String(presetSelect.value ?? "").trim();
    if (name === "") return;
    nativeFn("savePreset")(name)
      .then(refreshPresets)
      .catch((err) => console.error("savePreset failed", err));
  });

  need("btn-preset-load").addEventListener("click", () => {
    const name = String(presetSelect.value ?? "").trim();
    if (name === "") return;
    nativeFn("loadPreset")(name)
      .then(refreshPresets)
      .catch((err) => console.error("loadPreset failed", err));
  });

  // ── The verify ping ──────────────────────────────────────────────────────
  // C++ OWNS THE CYCLE AND JS NEVER RE-DERIVES THE STEP (CONTEXT-3.2 D14). The
  // indicated speaker is getPingState().speaker and nothing else: a drifted
  // setInterval would name speaker 5 while 6 sounds, during the one procedure
  // whose entire purpose is confirming that speaker N is speaker N.

  function renderPing(state) {
    const active = state !== null && typeof state === "object" && state.active === true;
    const lit = active ? Number(state.speaker) : 0;

    rows.forEach((r) => r.pingButton.classList.toggle("is-pinging", active && lit === r.n));

    const value = pingStateNode;
    value.textContent = active ? `${String(state.mode ?? "")} ${lit}` : "—";
  }

  function stopPingPoll() {
    if (pingTimer === null) return;
    window.clearInterval(pingTimer);
    pingTimer = null;
  }

  async function pollPing() {
    try {
      const state = await nativeFn("getPingState")();
      renderPing(state);
      if (state === null || typeof state !== "object" || state.active !== true) stopPingPoll();
    } catch (err) {
      console.error("getPingState failed", err);
      stopPingPoll();
    }
  }

  // Started BEFORE the start promise is awaited. If that completion is dropped
  // the poll still converges on the truth; if the poll were started inside a
  // .then() a dropped completion would leave a sounding ping with a dead
  // indicator, which is the worst possible state for this particular tool.
  function startPingPoll() {
    if (pingTimer !== null) return;
    pingTimer = window.setInterval(pollPing, PING_POLL_MS);
  }

  function requestPing(target) {
    startPingPoll();

    nativeFn("startPing")(target)
      .then((result) => {
        if (result === null || typeof result !== "object") return;
        if (result.ok === false) {
          stopPingPoll();
          renderPing(null);
          // ADVISORY: the refusal reason. The refusal itself is already visible
          // as "nothing is lit", which is what P64 requires.
          const value = pingStateNode;
          value.textContent = String(result.reason ?? "");
          return;
        }
        renderPing(result);
      })
      .catch((err) => console.error("startPing failed", err));
  }

  for (const r of rows) r.pingButton.addEventListener("click", () => requestPing(r.n));
  need("btn-ping-auto").addEventListener("click", () => requestPing("auto"));

  need("btn-ping-stop").addEventListener("click", () => {
    stopPingPoll();
    renderPing(null);
    nativeFn("stopPing")().catch((err) => console.error("stopPing failed", err));
  });

  window.addEventListener("pagehide", stopPingPoll);

  // ── v1.1.0 — the output-order presets ────────────────────────────────────
  // Two one-click label sets, applied WHOLE in C++ (applyOutputOrderPreset)
  // through the same guard the table's commit uses. NOT a second setVenue call
  // site (section 22): their own registration, and the device-order table
  // lives in C++ (D19). Committed state — the label column, the plan badges —
  // converges on the venueGen poll (P64); the completion feeds only the
  // advisory line beside the buttons, exactly as requestPing's refusal does.
  const ooStateNode = need("voo-state");

  function requestOutputOrder(id) {
    nativeFn("applyOutputOrderPreset")(id)
      .then((result) => {
        if (result === null || typeof result !== "object") return;

        const value = ooStateNode;
        value.textContent = result.ok === true
          ? (id === "direct" ? "direct 1–8" : "roles")
          : String(result.reason ?? "");
      })
      .catch((err) => console.error("applyOutputOrderPreset failed", err));
  }

  need("btn-oo-direct").addEventListener("click", () => requestOutputOrder("direct"));
  need("btn-oo-roles").addEventListener("click", () => requestOutputOrder("roles"));

  refreshPresets();

  // ── The surface app.js drives ────────────────────────────────────────────
  return {
    setGeometry(g) {
      if (g === null || typeof g !== "object") return;

      geometry = g;
      committed = {
        speakers: g.speakers.map((s) => ({
          x: Number(s.x), y: Number(s.y), z: Number(s.z),
          label: String(s.label), trimDb: Number(s.trimDb ?? 0), class: String(s.class ?? ""),
        })),
        rake: {
          front: Number(g.rake === undefined || g.rake === null ? 0 : g.rake.front ?? 0),
          rear: Number(g.rake === undefined || g.rake === null ? 0 : g.rake.rear ?? 0),
        },
      };

      paintFields();
      applyLabelMarks(collidingRows());
      drawMini();
    },

    relayout() {
      drawMini();
    },
  };
}
