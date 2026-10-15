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
//
// v1.3.3 finished the job. Until then this file OWNED a second copy of the
// points-string construction and of the three classification toggles — the same
// six lines as drawGeometryLayer(), so the rule was honoured for the projection
// and quietly broken one layer out. Both now come from roomplan.js as
// hullPoints() and placeGlyph(), and this file no longer calls metresToPx at
// all: the mini plan is a second VIEW passed to shared drawing code, which is
// what Q8 actually asked for.
// ============================================================================

import { fitBox, hullPoints, makeView, placeGlyph } from "./roomplan.js";

// ── Module-level bindings. Nothing here runs at module-evaluation time except
//    the declarations themselves; app.js calls createVenueScreen() from inside
//    its own init(), in its own try/catch (pattern_module_toplevel_init_tdz). ──

const SPEAKER_COUNT = 8;

// The four NUMERIC per-speaker columns. The label column is deliberately not in
// this list: it is the one column with different commit semantics.
const NUMERIC_KEYS = ["x", "y", "z", "trim", "delay"];

// ── v1.4.0 — THE DELAY COLUMN'S UNIT IS A VIEW STATE ──────────────────────
// Storage is always milliseconds. The toggle changes what the column DISPLAYS
// and what it PARSES; the payload is unconditionally ms. Deliberately not
// written into the .venue file — a room does not have a preferred unit.
//
// The conversion constant is NOT A LITERAL HERE. It arrives on the
// getVenueGeometry payload as `speedOfSound`, computed from
// oo::plane::kSpeedOfSoundMps, which is the same constant
// VenueModel::suggestedDelaysMs() divides by. A 343 written out in this file
// would be a mirrored fixture over that one — free to drift until the column
// reads metres the Derive button disagrees with
// (pattern_test_fixture_mirrors_drift_silently).
const DELAY_UNITS = ["ms", "m"];

// The fallbacks below are used ONLY before the first geometry payload arrives,
// when the table has nothing to display anyway. They are not a second source of
// truth: setGeometry() overwrites both on every refresh.
const DELAY_FALLBACK_C  = 343;
const DELAY_FALLBACK_MAX = 50;

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

  // v1.4.0. `el` is the ONE textContent receiver this feature adds, and it is a dedicated
  // .vcell-value span holding nothing but the unit — the word "Delay" beside it is authored and
  // never touched. The two unit buttons carry authored labels and are switched by class, so they
  // are not receivers at all.
  const delayUnitNode = need("vcol-delay-unit");
  const delayUnitButtons = { ms: need("btn-delay-ms"), m: need("btn-delay-m") };
  const delayDeriveButton = need("btn-delay-derive");

  const venueNameNode = need("vvenue-name");
  const miniStage = need("miniplan");
  const miniSvg = need("mini-geometry");
  const miniHull = need("mini-hull");
  const miniGlyphs = rows.map((r) => need(`mglyph-${r.n}`));

  const presetSelect = need("preset-list");
  const presetNameInput = need("preset-name");
  const presetSaveButton = need("btn-preset-save");
  const presetLoadButton = need("btn-preset-load");
  const presetStateNode = need("vpreset-state");
  const presetGroupCaptions = { factory: need("preset-group-factory"), user: need("preset-group-user") };
  const presetUnsavedCaption = need("preset-unsaved");
  const pingStateNode = need("vping-state");

  // v1.13.2 (B7) — where the result of a venue write is shown.
  const tableStateNode = need("vtable-state");
  const fileStateNode = need("vfile-state");

  // ── State ────────────────────────────────────────────────────────────────
  // `committed` is the last model the PLUGIN reported. `pending` holds raw text
  // the operator has typed and that has not been accepted yet. A geometry
  // refresh repaints from `committed` and never stamps on a pending edit or on
  // the field currently under the cursor.
  let geometry = null;
  let committed = null;
  const pending = new Map();
  let pingTimer = null;

  // v1.4.0. Session-local, resets to ms when the editor is reopened — see the
  // note on DELAY_UNITS. `speedOfSound` and `maxDelayMs` are replaced by every
  // setGeometry() call; the initialisers only cover the first paint.
  let delayUnit = "ms";
  let speedOfSound = DELAY_FALLBACK_C;
  let maxDelayMs = DELAY_FALLBACK_MAX;

  // ── ms <-> metres, in ONE pair of functions ──────────────────────────────
  // Metres of PATH DIFFERENCE, not absolute distance: the stored delay is how
  // much further the sound has to travel to reach the reference seat at the
  // same time as the farthest speaker's does. That is why 0 ms is 0 m and not
  // "the distance to the nearest speaker".
  const msToDisplay = (ms) => (delayUnit === "m" ? (ms * 0.001) * speedOfSound : ms);
  const displayToMs = (v) => (delayUnit === "m" ? (v / speedOfSound) * 1000 : v);

  // 2 dp in metres (about 7 mm of resolution, well under a measurement error),
  // 2 dp in ms (about 0.7 mm — the finer of the two, so the round trip through
  // the toggle does not visibly lose the value).
  const delayText = (ms) => fmt(msToDisplay(ms), 2);

  const fieldId = (n, key) => (key === "rakeFront" ? "vf-rake-front"
    : key === "rakeRear" ? "vf-rake-rear"
      : key === "label" ? `vf-label-${n}` : `vf-${n}-${key}`);

  // ── Reading the model out of committed + pending ──────────────────────────

  // Rails a millisecond value into [0, maxDelayMs]. The bound comes from C++
  // (OOctagonProcessor::kVenueDelayClampMs on the geometry payload) rather than
  // from a literal, so the field and the funnel cannot rail differently.
  const clampDelayMs = (ms) => (Number.isFinite(ms) ? Math.min(Math.max(ms, 0), maxDelayMs) : 0);

  function pendingNumber(n, key) {
    const raw = pending.get(fieldId(n, key));
    return raw === undefined ? undefined : parseNumber(raw) ?? undefined;
  }

  function labelOf(n) {
    const raw = pending.get(fieldId(n, "label"));
    return raw === undefined ? committed.speakers[n - 1].label : String(raw).trim();
  }

  // The payload for the ONE setVenue call site below.
  //
  // ── v1.13.2 (B2) — ONLY WHAT WAS TYPED GOES ON THE WIRE ─────────────────
  // Through v1.13.1 this sent all 50 values and filled every untouched one from
  // `committed` — the page's copy of the plugin's venue, refreshed only when the
  // 2 Hz status poll sees venueGen move. So a second commit inside one poll tick
  // carried the FIRST commit's field at its pre-edit value, and C++ applied it:
  // trim 1 -> -3 dB, Tab, trim 2 -> -6 dB, and speaker 1 was back at 0 dB. The
  // same path reverted an L <-> R swap behind a quick numeric edit, and the
  // reverted set is a valid permutation, so no MAP banner said so (R1's class).
  //
  // C++ setVenue starts from the LIVE venue and falls back PER FIELD — finiteOr()
  // for every number, hasProperty("label") for the label — so a row carrying
  // only `n` changes nothing. The page's cache no longer has any way onto the
  // wire. Eight rows are still sent, because setVenue checks the row count.
  function buildPayload() {
    const speakers = rows.map((r) => {
      const row = { n: r.n };

      if (pending.has(fieldId(r.n, "label"))) row.label = labelOf(r.n);

      const x = pendingNumber(r.n, "x");
      if (x !== undefined) row.x = x;

      const y = pendingNumber(r.n, "y");
      if (y !== undefined) row.y = y;

      const z = pendingNumber(r.n, "z");
      if (z !== undefined) row.z = z;

      const trim = pendingNumber(r.n, "trim");
      if (trim !== undefined) row.trimDb = trim;

      // ALWAYS MILLISECONDS ON THE WIRE. A pending edit is in whatever unit the
      // toggle is showing, so it converts here, exactly once. Clamped to the
      // rail C++ reports, so a typed 900 m rails in the field the operator is
      // looking at rather than silently at publishSnapshot().
      const delay = pendingNumber(r.n, "delay");
      if (delay !== undefined) row.delayMs = clampDelayMs(displayToMs(delay));

      return row;
    });

    const rake = {};

    const front = pendingNumber(0, "rakeFront");
    if (front !== undefined) rake.front = front;

    const rear = pendingNumber(0, "rakeRear");
    if (rear !== undefined) rake.rear = rear;

    return { speakers, rake };
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
        [r.fields.delay, delayText(s.delayMs)],
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

    // v1.4.0. The unit lives in the HEADER, once, rather than repeated in eight cells — and
    // ONLY the unit is written. Reusing the shared `el` receiver keeps section 6's whitelist
    // exactly as long as it was.
    delayUnitNode.textContent = delayUnit;

    for (const u of DELAY_UNITS) {
      delayUnitButtons[u].classList.toggle("is-active", u === delayUnit);
      delayUnitButtons[u].setAttribute("aria-pressed", u === delayUnit ? "true" : "false");
    }
  }

  // The mini-plan. Fitted to the SMALLER of its two bounds by the same fitBox()
  // the Room plan uses — a width-bound portrait plan wants more height than its
  // stage has, and overflows it while document.scrollHeight stays 720 (Q11).
  // Section 11 of the layout gate measures the fitted box against the stage for
  // exactly that, and section 12 its minimum size (v1.13.2: the stage moved from
  // the rail, where later groups had crushed it to 25 px, to under the table).
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

    miniHull.setAttribute("points", hullPoints(geometry.hull, view));

    geometry.speakers.forEach((s, i) => {
      const g = miniGlyphs[i];
      if (g === undefined || g === null) return;

      placeGlyph(g, s, view);
    });
  }

  // ── v1.13.2 (B7) — WHAT THE PLUGIN SAID, ON SCREEN ──────────────────────
  //
  // Every venue write answers { ok, reason, speaker }. Through v1.13.1 this page
  // dropped all of them except the output-order line, which printed the raw C++
  // code — on the French page too. A refusal now reads as a sentence in the
  // page's language: the three map failures through the ladder app.js owns
  // (window.__setReasonLabel), the file codes through the literal ladder below.
  // A code neither knows is printed AS the code, with the label mirror torn down
  // first, for the reason renderMapBanner gives. Still ADVISORY: the model
  // converges on the venueGen poll either way (N4).
  function renderRefusal(node, result) {
    const reason = String(result?.reason ?? "");
    const speaker = Number(result?.speaker);

    node.dataset.tone = "alert";
    if (window.__setReasonLabel(node, reason, speaker)) return;

    if (reason === "malformedRoot") {
      window.__setLabel(node, "vfile.malformed");
    } else if (reason === "unreadable") {
      window.__setLabel(node, "vfile.unreadable");
    } else {
      delete node.dataset.i18n;
      delete node.dataset.label;
      node.textContent = reason;
    }
  }

  // Back to the idle text, with the mirror torn down so a later language sweep
  // cannot resurrect a result that no longer applies.
  function clearResult(node, idle) {
    delete node.dataset.tone;
    delete node.dataset.i18n;
    delete node.dataset.label;
    node.textContent = idle;
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

          // ── THE SCREEN MUST GO BACK TO WHAT THE PLUGIN ACTUALLY HOLDS ─────
          //                                             (CODE_REVIEW WR-05)
          //
          // `pending` was cleared synchronously below, so from here on
          // buildPayload() falls back to `committed` for every field — the next
          // commit will send the OLD value. Until v1.3.2 nothing repainted, so
          // the rejected text stayed in the input and the operator was reading
          // room measurements the plugin does not hold, in a live-hall
          // calibration tool.
          //
          // Nothing else heals it. paintFields() has exactly one other caller,
          // setGeometry(), which app.js runs only when venueGen moves — and a
          // rejected setVenue never reaches applyVenueEdit(), so it never
          // publishes and never bumps venueGen. relayout() redraws the mini-plan
          // only, so a tab switch does not repaint the table either.
          //
          // The unbounded case is the one that matters: in SAFE mode (mono or
          // stereo output, a supported AU configuration) buildSpeakerToBuffer
          // fails with notEightChannels on EVERY commit, and the venue inputs
          // are not disabled. There `speaker` is -1, so not even the
          // is-colliding mark above appears — without this repaint the operator
          // gets no feedback of any kind, forever.
          //
          // Safe to call unconditionally: paintFields() skips any field in
          // `pending` (typed since this commit) and the focused field by design,
          // and commit() is only ever reached from a blur handler, so the field
          // that was just edited is no longer focused.
          //
          // Note this is the C++-reject path ONLY. The JS-detected collision
          // path returns above, before pending.clear(), and keeps its
          // hold-and-mark semantics intact — reverting there would make an
          // L <-> R swap unreachable (P53).
          paintFields();

          // v1.13.2 (B7) — and SAY WHY. The repaint shows what the plugin
          // holds; this line shows that it refused, and on which row.
          renderRefusal(tableStateNode, result);
        } else if (result !== null && typeof result === "object" && result.ok === true) {
          clearResult(tableStateNode, "");
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
      // The DELAY column reverts through delayText() rather than fmt(), because
      // what Escape must restore is the value IN THE UNIT CURRENTLY SHOWN. A
      // revert to the raw stored ms while the column is displaying metres would
      // put a number in the field that is wrong by a factor of 343 and looks
      // entirely plausible.
      if (key === "delay") {
        bindNumeric(r.fields.delay, () => delayText(committed.speakers[r.n - 1].delayMs));
        continue;
      }

      const dp = key === "trim" ? 1 : 2;
      bindNumeric(r.fields[key], () => fmt(committed.speakers[r.n - 1][key === "trim" ? "trimDb" : key], dp));
    }
  }

  bindNumeric(rakeFront, () => fmt(committed.rake.front, 2));
  bindNumeric(rakeRear, () => fmt(committed.rake.rear, 2));

  // v1.13.2 (B11) — THE ROW BEING TYPED INTO LIGHTS ITS DOT on the mini-plan,
  // which is the question the plan exists to answer while 24 coordinates go
  // in. focusin/focusout bubble from any of the row's six fields; moving between
  // two fields of one row is an out-then-in, so the dot stays lit.
  for (const r of rows) {
    const rowNode = need(`vrow-${r.n}`);
    const glyph = miniGlyphs[r.n - 1];
    rowNode.addEventListener("focusin", () => glyph.classList.add("is-editing"));
    rowNode.addEventListener("focusout", () => glyph.classList.remove("is-editing"));
  }

  // ── The .venue store ─────────────────────────────────────────────────────
  // Both open a NATIVE MODAL through FileChooser::launchAsync on the C++ side.
  // Nothing here waits on the chooser's promise: a LOAD that succeeds moves
  // venueGen, and the table repaints from the refresh that follows.

  // v1.13.2 (B7) — and the result is SHOWN. Through v1.13.1 both completions
  // were dropped: a newer-format file that loaded with values missing, a
  // malformed or unreadable one that changed nothing, a label set the plugin
  // refused, and a save that never reached the disk all looked like success.
  // A cancelled chooser changes nothing, so it leaves the line as it was.
  function renderFileResult(op, result) {
    if (result === null || typeof result !== "object") return;

    const reason = String(result.reason ?? "");
    if (reason === "cancelled") return;

    if (result.ok === true) {
      if (reason === "forwardVersion") {
        fileStateNode.dataset.tone = "alert";
        window.__setLabel(fileStateNode, "vfile.forward");
      } else if (op === "save") {
        delete fileStateNode.dataset.tone;
        window.__setLabel(fileStateNode, "vfile.saved");
      } else {
        delete fileStateNode.dataset.tone;
        window.__setLabel(fileStateNode, "vfile.loaded");
      }
      return;
    }

    // A failed SAVE is reported by C++ as "unreadable" — the same code a
    // failed read uses — so the direction comes from which button was pressed.
    if (op === "save") {
      fileStateNode.dataset.tone = "alert";
      window.__setLabel(fileStateNode, "vfile.writeFailed");
      return;
    }

    renderRefusal(fileStateNode, result);
  }

  need("btn-venue-save").addEventListener("click", () => {
    nativeFn("saveVenue")()
      .then((result) => renderFileResult("save", result))
      .catch((err) => console.error("saveVenue failed", err));
  });

  need("btn-venue-load").addEventListener("click", () => {
    nativeFn("loadVenue")()
      .then((result) => renderFileResult("load", result))
      .catch((err) => console.error("loadVenue failed", err));
  });

  // ── The musical preset store ─────────────────────────────────────────────
  // FOUR native functions and O-Octagon's own DOM. createPresetBar() is never
  // called: it writes container.innerHTML and then queries its own injected
  // markup, which erases authored labels by construction
  // (pattern_js_state_updater_overwrites_html_labels). The options below are
  // built with createElement and appended.
  //
  // ── v1.14.0 (B3/U4) — A RAIL THAT CAN CREATE A PRESET ─────────────────────
  // Through v1.13.2 Save sent presetSelect.value — always an existing name, and
  // on a stock install always a FACTORY name — the module refused it, and the
  // refusal was dropped (.then(refreshPresets)). Now the NAME FIELD is what Save
  // writes, Save is disabled on a factory name with the read-only hint, every
  // completion is rendered through setLabel, and the current name comes from
  // the PROCESSOR (it rides the session, so a reopen shows the preset that is
  // actually loaded instead of "Default").
  let presetFactory = new Set();
  let presetUser = new Set();
  let presetCurrent = "";

  // The last RESULT, as a CODE, so a hint that stops applying hands the line
  // back to it. One switch turns a code into a LITERAL setLabel key — check-i18n
  // [13] refuses a computed key, because a computed key cannot be checked.
  let presetResult = "idle";

  function setPresetLine(code) {
    const node = presetStateNode;
    node.dataset.tone = code === "readOnly" || code === "replaces" ? "hint"
      : code === "idle" || code === "saved" || code === "loaded" ? "" : "alert";
    if (node.dataset.tone === "") delete node.dataset.tone;

    switch (code) {
      case "saved":       window.__setLabel(node, "preset.saved"); break;
      case "loaded":      window.__setLabel(node, "preset.loaded"); break;
      case "readOnly":    window.__setLabel(node, "preset.readOnly"); break;
      case "replaces":    window.__setLabel(node, "preset.replaces"); break;
      case "factoryName": window.__setLabel(node, "preset.factoryName"); break;
      case "emptyName":   window.__setLabel(node, "preset.emptyName"); break;
      case "notFound":    window.__setLabel(node, "preset.notFound"); break;
      case "idle":        window.__setLabel(node, "preset.idle"); break;
      default:            window.__setLabel(node, "preset.writeFailed"); break;
    }
  }

  // The optgroup captions and the placeholder live on hidden data-i18n spans
  // (index.html), because an <optgroup> displays its label ATTRIBUTE and the
  // i18n sweep writes textContent. Copied across on every render and whenever
  // a language switch rewrites the spans.
  function relabelPresetList() {
    for (const group of presetSelect.querySelectorAll("optgroup")) {
      const caption = presetGroupCaptions[group.dataset.group];
      if (caption) group.label = caption.textContent;
    }
    const placeholder = presetSelect.querySelector("option[data-placeholder]");
    // .label, not textContent: an <option> displays its label when one is set.
    if (placeholder) placeholder.label = presetUnsavedCaption.textContent;
  }
  new MutationObserver(relabelPresetList).observe(presetGroupCaptions.factory.parentNode,
    { subtree: true, childList: true, characterData: true });

  // Save is possible for any non-empty name that is not a factory preset. The
  // hint says why it is not, or that the name replaces a user preset — the
  // module overwrites without asking, so the page says so before the click.
  // CASE-INSENSITIVE, because the store is: on a stock macOS / Windows volume
  // "chamber" opens Chamber.json, so C++'s guard refuses it and a user file
  // named "my hall" replaces "My Hall".
  const hasName = (set, name) => [...set].some((n) => n.toLowerCase() === name.toLowerCase());

  // v1.14.1 (R1) — the name the STORE holds. The module writes "A/B" as
  // A_B.json and lists it as "A_B" (OuariconPresetManager::sanitizePresetName,
  // mirrored by oo::presets::fileSafeName). Every comparison against the list
  // goes through this, so typing "A/B" over a saved "A_B" warns that it replaces.
  const storedName = (name) => name.replace(/[\/\\:]/g, "_");

  function syncPresetSave(showHint) {
    const name = storedName(presetNameInput.value.trim());
    const isFactory = hasName(presetFactory, name);
    presetSaveButton.disabled = name === "" || isFactory;
    presetLoadButton.disabled = presetSelect.value === "";
    if (!showHint) return;
    if (isFactory) setPresetLine("readOnly");
    else if (hasName(presetUser, name)) setPresetLine("replaces");
    else if (presetStateNode.dataset.tone === "hint") setPresetLine(presetResult);
  }

  function renderPresets(payload) {
    if (payload === null || typeof payload !== "object") return;

    const names = Array.isArray(payload.presets) ? payload.presets.map(String) : [];
    presetFactory = new Set(Array.isArray(payload.factory) ? payload.factory.map(String) : []);
    presetUser = new Set(names.filter((n) => !presetFactory.has(n)));
    presetCurrent = typeof payload.current === "string" ? payload.current : "";

    const keep = presetSelect.value;
    presetSelect.replaceChildren();

    // "— unsaved —" stands in for a blank select whenever the current name is
    // not in the list: a fresh instance, or a session whose preset file is gone.
    if (!names.includes(presetCurrent)) {
      const placeholder = document.createElement("option");
      placeholder.value = "";
      placeholder.disabled = true;
      placeholder.dataset.placeholder = "";
      presetSelect.appendChild(placeholder);
    }

    for (const [group, members] of [["factory", presetFactory], ["user", presetUser]]) {
      const list = names.filter((n) => members.has(n));
      if (list.length === 0) continue;
      const optgroup = document.createElement("optgroup");
      optgroup.dataset.group = group;
      for (const name of list) {
        const value = document.createElement("option");
        value.value = name;
        value.textContent = name;
        optgroup.appendChild(value);
      }
      presetSelect.appendChild(optgroup);
    }
    relabelPresetList();

    presetSelect.value = names.includes(keep) ? keep : (names.includes(presetCurrent) ? presetCurrent : "");
    syncPresetSave(false);
  }

  async function refreshPresets() {
    try {
      renderPresets(await nativeFn("getPresetList")());
    } catch (err) {
      console.error("preset list failed", err);
    }
  }

  // The name field starts on the CURRENT preset, and follows a successful load
  // or save — so "Save as" after a tweak to a user preset is one click.
  async function initPresets() {
    await refreshPresets();
    presetSelect.value = presetCurrent !== "" && (presetFactory.has(presetCurrent) || presetUser.has(presetCurrent))
      ? presetCurrent : "";
    presetNameInput.value = presetCurrent;
    syncPresetSave(false);
  }

  presetNameInput.addEventListener("input", () => syncPresetSave(true));
  presetNameInput.addEventListener("keydown", (e) => {
    // v1.14.1 (R3) — the Enter that COMMITS an IME composition (Japanese,
    // Chinese pinyin) is not a save. WebKit reports it as keyCode 229 with
    // isComposing sometimes already false, so both are checked.
    if (e.isComposing || e.keyCode === 229) return;
    if (e.key === "Enter" && !presetSaveButton.disabled) presetSaveButton.click();
  });
  presetSelect.addEventListener("change", () => syncPresetSave(false));

  function renderPresetResult(op, name, result) {
    if (result === null || typeof result !== "object") return;
    // An unknown refusal code falls through setPresetLine's default to
    // "could not write the preset" — the honest reading of any failed write.
    presetResult = result.ok === true ? (op === "save" ? "saved" : "loaded") : String(result.reason ?? "");
    if (result.ok === true) presetNameInput.value = name;
    setPresetLine(presetResult);
  }

  presetSaveButton.addEventListener("click", () => {
    const name = presetNameInput.value.trim();
    if (name === "" || hasName(presetFactory, storedName(name))) { syncPresetSave(true); return; }
    nativeFn("savePreset")(name)
      .then(async (result) => {
        await refreshPresets();
        // C++ answers with the name it stored; the mirror covers an older host.
        const stored = typeof result?.name === "string" && result.name !== "" ? result.name : storedName(name);
        if (result?.ok === true) presetSelect.value = stored;
        renderPresetResult("save", stored, result);
        syncPresetSave(false);
      })
      .catch((err) => console.error("savePreset failed", err));
  });

  presetLoadButton.addEventListener("click", () => {
    const name = String(presetSelect.value ?? "").trim();
    if (name === "") return;
    nativeFn("loadPreset")(name)
      .then(async (result) => {
        await refreshPresets();
        renderPresetResult("load", name, result);
        syncPresetSave(false);
      })
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

    // v1.13.2 — a refusal is now written through setLabel (below), so tear its
    // mirror down before writing state text, or the next language sweep would
    // put the refusal back over a running ping.
    const value = pingStateNode;
    delete value.dataset.i18n;
    delete value.dataset.label;
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
          //
          // v1.13.2 (B7) — as a sentence, not the C++ code: through v1.13.1 the
          // line printed "mapInvalid", on the French page too. The ping is
          // refused for the same reason the monitor is — no mapped rig on this
          // output — so it says the same words.
          const value = pingStateNode;
          if (String(result.reason ?? "") === "mapInvalid") {
            window.__setLabel(value, "monitor.unavailable");
          } else {
            delete value.dataset.i18n;
            delete value.dataset.label;
            value.textContent = String(result.reason ?? "");
          }
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

  // ── v1.7.0 — the monitor fold-down toggle ────────────────────────────────
  //
  // WRITE-ONLY FROM HERE. It sends the request and renders NOTHING: the arm is
  // reflected by renderMonitor() on the existing 2 Hz getStatus poll in app.js,
  // which is the only reader. Two reasons, and the second is the sharp one:
  //
  //   * setMonitorArmed REFUSES in SAFE mode and on an unresolved monitor pair,
  //     so the request is not the result. Rendering the request would light a
  //     banner claiming the rig lanes are muted while eight speakers keep
  //     playing — the banner lying in the dangerous direction.
  //   * The processor clears the arm behind the page's back in four other
  //     situations (bypass, SAFE flip, ping start, editor close). Only a poll
  //     sees those.
  //
  // The completion feeds the advisory line and nothing else, exactly as
  // requestPing's refusal and the output-order presets' do.
  const monitorNode = need("vmonitor-state");
  const monitorBtn = need("monitor-toggle");

  monitorBtn.addEventListener("click", () => {
    // The CURRENT state comes from the attribute the poll wrote, never from a
    // local boolean this handler keeps — a local one would drift out of step
    // the first time the processor disarmed on its own, and the next click
    // would then send the wrong request.
    const armed = monitorBtn.getAttribute("aria-pressed") === "true";

    // v1.11.0 — on a STEREO bus the same button toggles the binaural
    // preference instead. data-stereo-bus is written by renderMonitor() from
    // the poll, for the reason aria-pressed is: the page never re-derives
    // which output sets count as stereo (P43). The setter is never refused,
    // so there is no refusal branch to render — the ACTIVE state arrives on
    // the next poll as binauralActive.
    //
    // v1.13.2 (B12) — it toggles the stored PREFERENCE, which renderMonitor()
    // writes to data-binaural-pref from getStatus().stereoBinaural. Through
    // v1.13.1 it sent the inverse of aria-pressed, which reflects what the
    // audio thread DID: with audio idle (preference on, nothing rendered yet)
    // every click sent `true` and the fold could not be switched off.
    if (monitorBtn.dataset.stereoBus === "true") {
      const preferred = monitorBtn.dataset.binauralPref !== "false";
      nativeFn("setStereoBinaural")(!preferred)
        .catch((err) => console.error("setStereoBinaural failed", err));
      return;
    }

    nativeFn("setMonitorArmed")(!armed)
      .then((result) => {
        if (result === true || armed) return;
        // Asked to arm, came back disarmed: refused.
        //
        // THROUGH window.__setLabel, NOT A LITERAL. This module is a sibling of
        // the canon block, which lives in app.js and exports nothing — the canon
        // publishes setLabel on window for exactly this case. A literal here
        // would be stranded English the moment the selector fired, and it is the
        // same string renderMonitor() writes from app.js, so the two paths must
        // resolve the same key rather than carry two copies of one sentence.
        window.__setLabel(monitorNode, "monitor.unavailable");
      })
      .catch((err) => console.error("setMonitorArmed failed", err));
  });

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

        // if/else with LITERAL keys, never a ternary in the argument — the
        // rule check-i18n assertion 13 states, applied here by hand because
        // assertion 12 only scans the ONE controller module and never sees this
        // file. A gate that cannot reach a file is not a reason to write worse
        // code in it.
        const value = ooStateNode;
        if (result.ok !== true) {
          // A REFUSAL REASON FROM C++. Until v1.13.2 it was printed as the raw
          // code ("notEightChannels" on every stereo bus, on the French page
          // too); it now reads as the same sentence the MAP banner uses, and
          // only a code no ladder knows is printed as sent.
          renderRefusal(value, result);
        } else if (id === "direct") {
          delete value.dataset.tone;
          window.__setLabel(value, "oo.direct");
        } else {
          delete value.dataset.tone;
          window.__setLabel(value, "oo.roles");
        }
      })
      .catch((err) => console.error("applyOutputOrderPreset failed", err));
  }

  need("btn-oo-direct").addEventListener("click", () => requestOutputOrder("direct"));
  need("btn-oo-roles").addEventListener("click", () => requestOutputOrder("roles"));

  // ── v1.16.0 — the factory rooms ──────────────────────────────────────────
  // requestOutputOrder's shape exactly: the room is built in C++
  // (applyFactoryRoom), the table and the plan converge on the venueGen poll,
  // and the completion feeds only the advisory line beside the buttons — the
  // header's own result line, tableStateNode.

  function requestFactoryRoom(id) {
    nativeFn("applyFactoryRoom")(id)
      .then((result) => {
        if (result === null || typeof result !== "object") return;

        const value = tableStateNode;
        if (result.ok !== true) {
          renderRefusal(value, result);
        } else if (id === "ring") {
          delete value.dataset.tone;
          window.__setLabel(value, "room.ring");
        } else {
          delete value.dataset.tone;
          window.__setLabel(value, "room.placeholder");
        }
      })
      .catch((err) => console.error("applyFactoryRoom failed", err));
  }

  need("btn-room-placeholder").addEventListener("click", () => requestFactoryRoom("placeholder"));
  need("btn-room-ring").addEventListener("click", () => requestFactoryRoom("ring"));

  // ── v1.4.0 — the delay column's two controls ─────────────────────────────

  // THE TOGGLE IS PURELY LOCAL. It calls nothing: the stored values do not
  // change, only their presentation, so there is nothing for the plugin to
  // agree to. Any half-typed edit is dropped rather than reinterpreted —
  // `pending` holds RAW TEXT with no unit attached to it, so carrying "12.5"
  // across a ms->m flip would silently turn 12.5 ms into 12.5 m.
  for (const u of DELAY_UNITS) {
    delayUnitButtons[u].addEventListener("click", () => {
      if (delayUnit === u) return;

      for (const r of rows) {
        pending.delete(r.fields.delay.id);
        r.fields.delay.classList.remove("is-invalid");
      }

      delayUnit = u;
      paintFields();
    });
  }

  // DERIVE IS A C++ WRITE, not a JS fill followed by a commit. The law needs the
  // centroid, the audience plane and eight 3-D distances; re-deriving any of
  // that here would be a mirrored fixture over VenueModel::suggestedDelaysMs()
  // (D19). The page sends nothing and renders what comes back.
  //
  // Pending delay edits are dropped first for the same reason the toggle drops
  // them: the eight fields are about to be replaced wholesale, and a stale
  // pending entry would win over the derived value on the next commit.
  //
  // The promise is ADVISORY, like every other write on this page (N4). The
  // authoritative repaint arrives on app.js's venueGen poll.
  delayDeriveButton.addEventListener("click", () => {
    for (const r of rows) {
      pending.delete(r.fields.delay.id);
      r.fields.delay.classList.remove("is-invalid");
    }

    // v1.13.2 (B7) — a refusal (labels that no longer resolve on this rig) is
    // said in the venue head, beside the table it was meant to fill.
    nativeFn("applySuggestedDelays")()
      .then((result) => {
        if (result === null || typeof result !== "object") return;
        if (result.ok === false) renderRefusal(tableStateNode, result);
        else clearResult(tableStateNode, "");
      })
      .catch((err) => console.error("applySuggestedDelays failed", err));
  });

  initPresets();

  // ── The surface app.js drives ────────────────────────────────────────────
  return {
    setGeometry(g) {
      if (g === null || typeof g !== "object") return;

      geometry = g;

      // v1.4.0. Taken from the payload on EVERY refresh rather than once at
      // init: both are C++ constants today, but reading them once would make
      // this page quietly wrong on the first build that makes either depend on
      // the venue. The guards keep a payload from an older build (or the
      // ui-stub) from writing NaN into a divisor.
      if (Number.isFinite(Number(g.speedOfSound)) && Number(g.speedOfSound) > 0)
        speedOfSound = Number(g.speedOfSound);

      if (Number.isFinite(Number(g.maxDelayMs)) && Number(g.maxDelayMs) > 0)
        maxDelayMs = Number(g.maxDelayMs);

      committed = {
        speakers: g.speakers.map((s) => ({
          x: Number(s.x), y: Number(s.y), z: Number(s.z),
          label: String(s.label), trimDb: Number(s.trimDb ?? 0), class: String(s.class ?? ""),
          delayMs: Number(s.delayMs ?? 0),
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
