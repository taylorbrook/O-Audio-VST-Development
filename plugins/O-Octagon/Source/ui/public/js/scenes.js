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
// O-Octagon — weight scenes (Stage 3 Phase 3.3, FUNC-06)
//
// Ten controls: six NAMED scenes derived from the measured geometry, and four
// USER slots the operator captures. This is the phase's ONLY write path, and it
// writes to the eight parameters whose all-zero state is DSP-05's silence.
//
// ── THIS MODULE PERFORMS NO SPEAKER ARITHMETIC (D19 / P79) ────────────────
// Membership arrives WHOLE, in the getVenueGeometry payload, computed in C++
// from the measured geometry by the same pure function the editor's applyScene
// consults. There is no centroid here, no bounding box, no comparison of a
// speaker coordinate against anything.
//
// That is not tidiness. A JS re-derivation would be a mirrored fixture
// (pattern_test_fixture_mirrors_drift_silently) over R1, the highest-risk
// component in the project — and it is exactly what makes FUNC-06 criterion 2's
// PERMUTATION probe meaningful: rotate the speaker indices by k against the
// same eight physical positions and FRONT must return the indices that NOW hold
// y < cy. A fixed-index implementation returns {1,2,3,8} and FAILS. Section 32
// of ui_frontend_check.js asserts the absence statically; NC2 fires it.
//
// ── THE WRITE IS IN C++, IN ONE CALL, WITH EIGHT GESTURE BRACKETS (D18) ───
// beginChangeGesture -> setValueNotifyingHost -> endChangeGesture on EACH of
// w1..w8. setValueNotifyingHost opens no gesture of its own, so without the
// brackets Logic's Touch/Latch modes may MOVE THE SOUND AND NOT RECORD IT —
// invisible to build, auval and pluginval alike. This is the THIRD AND FINAL
// site of that obligation, after the 3.1 puck and the 3.2 preset load.
//
// It is one native call rather than eight SliderState writes because eight
// writes scatter the bracket obligation across 24 bridge messages, where no
// single grep can confirm it. The parameter ECHO still repaints this page —
// WebSliderParameterAttachment listens to the parameter, so a C++-side
// setValueNotifyingHost moves the eight in-plan weight cells with no extra
// plumbing here.
//
// ── AN EMPTY SET IS LEGIBLE AND NOT WRITABLE (D20) ────────────────────────
// All-zero weights are DSP-05's silence path. Reaching it by a mis-derived
// scene click mid-concert is unrecoverable, and a degenerate venue can
// legitimately empty a named scene — RESEARCH-3.3 Q7's proscenium fixture
// (4 corners plus 2 points on each of the front and rear edges, a physically
// plausible rig) empties SIDES while every speaker is non-INTERIOR. Disabling
// the control is the AFFORDANCE; applyScene's `{ok:false, reason:"emptyScene"}`
// in C++ is the GUARANTEE, and both exist.
//
// ── PREVIEW ON HOVER *OR* KEYBOARD FOCUS (D21) ────────────────────────────
// FUNC-06 criterion 3 requires the set to be shown BEFORE commit. A
// pointer-only preview satisfies that only for a user who happens to hover, so
// focus gives the identical preview. Zero extra gestures in a concert, which a
// two-step arm/commit would have doubled.
//
// ── STORE IS THE ONE ASYMMETRY (D22) ──────────────────────────────────────
// STORE arms, the next slot click captures, and it AUTO-DISARMS after one
// capture. Deliberately unlike D21: recalling a scene is reversible;
// OVERWRITING A SLOT IS NOT, and it is the one gesture where a mid-concert
// mis-click destroys data the user measured.
//
// EVERY LABEL IS HTML-AUTHORED AND NOTHING BELOW WRITES textContent ON ONE.
// State goes to data-* and aria-pressed. A shared updater writing textContent
// erases ALL / FRONT / REAR silently and passes every build gate
// (pattern_js_state_updater_overwrites_html_labels), which is why ROADMAP names
// it a 3.3 test criterion in its own right and why section 36 gates it.
// ============================================================================

const SLOT_COUNT = 4;

export function createScenes(deps) {
  const row = document.getElementById("scene-row");
  const storeToggle = document.getElementById("btn-scene-store");

  if (row === null || storeToggle === null)
    throw new Error("scenes: a required element is missing");

  const buttons = Array.from(row.querySelectorAll(".scene-btn"));
  if (buttons.length === 0) throw new Error("scenes: no scene buttons");

  // name -> { indices, empty }, straight out of the payload. Never computed.
  let named = new Map();
  let slots = [];
  let armed = false;

  const isSlot = (btn) => btn.dataset.slot !== undefined;

  const slotOf = (btn) => {
    const n = Number(btn.dataset.slot);
    return Number.isFinite(n) ? slots[n - 1] : undefined;
  };

  // What a button resolves to, for preview and for the disabled decision. Both
  // answers come from data the plugin returned; neither is derived.
  function resolved(btn) {
    if (isSlot(btn)) {
      const slot = slotOf(btn);
      if (slot === undefined || slot.occupied !== true) return null;
      // A stored slot's SET is the speakers it wrote a non-zero weight to. That
      // is a read of the stored vector, not geometry arithmetic.
      const indices = [];
      slot.w.forEach((v, i) => { if (Number(v) > 0) indices.push(i + 1); });
      return { indices, empty: indices.length === 0 };
    }

    return named.get(btn.dataset.scene) ?? null;
  }

  // ── Enablement ───────────────────────────────────────────────────────────
  // `disabled` and `data-empty` only. The LABEL is never touched.
  function refreshEnablement() {
    for (const btn of buttons) {
      const r = resolved(btn);
      const empty = r === null || r.empty === true;

      // While STORE is armed a slot must stay clickable even when it is empty —
      // capturing INTO an empty slot is the entire point of the gesture.
      btn.disabled = empty && !(armed && isSlot(btn));

      btn.dataset.empty = empty ? "true" : "false";

      if (isSlot(btn)) {
        const slot = slotOf(btn);
        btn.classList.toggle("is-occupied", slot !== undefined && slot.occupied === true);
      }

      if (r !== null) btn.dataset.speakers = r.indices.join(",");
    }
  }

  function preview(btn) {
    const r = btn === null ? null : resolved(btn);
    if (typeof deps.onPreview === "function")
      deps.onPreview(r === null ? [] : r.indices);
  }

  function setArmed(next) {
    armed = next === true;
    storeToggle.setAttribute("aria-pressed", armed ? "true" : "false");
    refreshEnablement();
  }

  // ── Commit ───────────────────────────────────────────────────────────────
  async function commit(btn) {
    // THE ARMED PATH FIRST. A slot click while armed CAPTURES; it never
    // recalls. Auto-disarm happens whether the capture succeeded or not, so a
    // refused store cannot leave the surface armed for the next click.
    if (armed && isSlot(btn)) {
      try {
        await deps.nativeFn("storeScene")(Number(btn.dataset.slot));
      } catch (err) {
        console.error("storeScene failed", err);
      }
      setArmed(false);
      await refreshSlots();
      return;
    }

    if (btn.disabled) return;

    try {
      const result = await deps.nativeFn("applyScene")(btn.dataset.scene);

      // D20's guarantee arriving back. The page states the refusal rather than
      // doing nothing — "nothing happened" is indistinguishable from a dead
      // control, which is the failure mode this whole surface is written
      // against.
      if (result !== null && typeof result === "object" && result.ok !== true) {
        btn.dataset.refused = String(result.reason ?? "refused");
        return;
      }

      delete btn.dataset.refused;
    } catch (err) {
      console.error("applyScene failed", err);
    }
  }

  async function refreshSlots() {
    try {
      const payload = await deps.nativeFn("getScenes")();
      if (payload === null || typeof payload !== "object") return;
      slots = Array.isArray(payload.slots) ? payload.slots : [];
      refreshEnablement();
    } catch (err) {
      console.error("getScenes failed", err);
    }
  }

  // ── Wiring ───────────────────────────────────────────────────────────────
  for (const btn of buttons) {
    // HOVER *AND* FOCUS, and they call the same function — a preview that
    // differed between the two would satisfy FUNC-06/3 only for a mouse user.
    btn.addEventListener("pointerenter", () => preview(btn));
    btn.addEventListener("focus", () => preview(btn));
    btn.addEventListener("pointerleave", () => preview(null));
    btn.addEventListener("blur", () => preview(null));

    btn.addEventListener("click", () => { commit(btn); });
  }

  storeToggle.addEventListener("click", () => setArmed(!armed));

  return {
    // Named membership rides getVenueGeometry (Q10): it is a PURE FUNCTION OF
    // THE VENUE, so it refreshes on the generation counter that already exists
    // and cannot go stale independently of the room it describes.
    setGeometry(geometry) {
      named = new Map();
      const list = Array.isArray(geometry?.scenes) ? geometry.scenes : [];
      for (const s of list) named.set(String(s.id), { indices: s.indices ?? [], empty: s.empty === true });
      refreshEnablement();
    },

    // The four USER slots are NOT a venue function, so they have their own read
    // and their own generation (`scenesGen` on getStatus), mirroring venueGen.
    refreshSlots,

    isArmed: () => armed,
    slotCount: SLOT_COUNT,
  };
}
