# Stage 4: Polish — Execution Summary

**Date:** 2026-07-24
**Plan:** `PLAN.md` (19 tasks / 4 phases) — 18 executed, 1 conditional task correctly skipped
**Verdict:** all four phase gates green; ship-ready at v1.0.0

---

## Phase 4.0 — Entry Gate

| Task | Result |
|------|--------|
| 1. D7/D6 Standalone audition (HUMAN, BLOCKING) | ✅ **Closed — makeup constant DECLINED** |
| 2. Harness baseline | ✅ **33/33 PASS, exit 0** (`washRms[5..10s] = 0.0064433`) |
| 3. Feedback makeup constant (CONDITIONAL) | ⏭️ **Not run** — Task 1 declined it |

**D11 outcome (CONTEXT success-criterion #1 — the question is closed, not silent):**
the user auditioned at `feedback = 100` and reported the wash length is **right as
shipped**. The `k = 2.0f` pre-`tanh` makeup constant is **explicitly declined**. The
topology's inherent ≈ −7.3 dB/generation pre-damping loss (−4.3 dB Hann² duty
+ −3.0 dB pan→mono-sum round trip) is the intended character.

**Stage 4 therefore carries ZERO DSP diff.** Harness stayed 33 at entry / 41 at exit
(not the 34 / 42 the plan reserved for the conditional path).

---

## Phase 4.1 — Preset System

| Task | Result |
|------|--------|
| 4. CMake wiring | ✅ include dir + ONE binary-data target |
| 5. Processor + 8-preset factory table | ✅ engineering units → `convertTo0to1` |
| 6. Editor — 10 native fns, MSVC-safe | ✅ bridge surface **11** |
| 7. Harness probe N | ✅ 8 new checks, **33 → 41** |
| 8. Factory re-seed discipline | ✅ 8 JSONs + `.factory-version` after `rm -rf` |

- **C1 validated by measurement, not assertion.** Probe N audits all eight presets
  through the *shipping* `OuariconPresetManager::loadPreset()`. Worst round-trip
  delta: **0.0000 on every one of the ten parameters of all eight presets.** The
  seeded JSON proves the skew mattered — `Reverse Bloom` stores `delayTime 0.6004`
  for 500 ms (a linear fraction would have been 0.25 ≈ 112 ms) and `highCut 0.7171`
  for 8000 Hz (linear 0.385 ≈ 2100 Hz).
- **C2 needed zero code (F8 confirmed by reading the module):** v1.0.5
  `applyPresetJson()` already resets every `RangedAudioParameter` to default,
  meta-first, before applying.
- **F6 deviation applied.** Both `FileChooser` completions hoist
  `SafePointer<ReverseDelayEditor>` to a **local** before `launchAsync` instead of
  init-capturing it in the nested lambda as O-Contrabass does. The null path
  bare-returns; it never calls `complete()`, which would itself be a UAF.
- **Session state** routes through `getStateAsXml()` / `setStateFromXml()`, so the
  preset name survives save/reload and pre-Stage-4 APVTS sessions still load.

---

## Phase 4.2 — Preset Bar + Tooltips

| Task | Result |
|------|--------|
| 9. Geometry 940 × 484 | ✅ editor + both CSS spots |
| 10. Bar markup + Naturalist CSS | ✅ 5 controls, copy in `data-*` |
| 11. `initPresetBar()` TDZ-safe | ✅ hoisted, inside `init()`, try/catch |
| 12. Tooltips on all 10 controls | ✅ measure-then-pin |
| 13. Browser-stub repair | ✅ both F4 blockers fixed |
| 14. `ui_frontend_check.js` repair + extend | ✅ **76/76, exit 0** |
| 15. Stub render gate | ✅ all six sub-gates |

**Gate 15 evidence (driven in a real browser against the stub):**

- **940 × 484 exactly**, `overflowX = 0`, `overflowY = 0`.
- **Zero JS console errors.** The `window.__JUCE__` shim eliminated the 5 s
  `_waitForNative` error; the only entry is the known stub-server `favicon.ico`
  404, which does not exist in the WebView path.
- `.time-slot` box **identical across Free and Sync** — `x:117 y:242 w:86 h:100`,
  correct control visible each way, FREE/SYNC labels and `aria-pressed` intact,
  13 division options. The `y` moved 198 → 242, exactly the predicted +44; the
  assertion is mode-invariance, never the old absolute y.
- Panels still 215 px at unchanged x; footer untouched.
- **Bar round-trip driven end to end:** ◀ ▶ cycle all 8 factory names in the
  predicted case-insensitive order (Dark Cavern → Guitar Swell → Near-Infinite →
  Reverse Bloom → Rhythmic Reverse → Slow Wash → Tight Smear → Vocal Halo) and
  wrap; prev reverses; Save → Load → navigate back → two-click Delete → name
  returns to `Default` and the button label is restored from `data-label`.
- A fresh instance lands on **`Default`**, not a list member.
- **C5 confirmed.** The `mix` tooltip — right-most control, the one C5 predicted
  would break — renders at a **full 230 px**, clamped to `left: 702` (exactly
  `940 − 230 − 8`) with `--arrow-x: 157px` still pointing at the knob. All 10 tips
  fire with correct copy; `delayTime` correctly flips to `below`.
- **No regression:** knob drag 35 % → 62 %, dblclick reset → 35 %, and all 8
  readouts at exact engineering defaults after the bar and tooltip additions.

---

## Phase 4.3 — Validation + Release

| Gate | Result |
|------|--------|
| `build-and-install.sh` | ✅ dual-variant sweep, AU cache cleared, only `-dev` bundles present |
| Render harness (exit re-run) | ✅ **41/41 PASS, exit 0** |
| `pluginval --strictness-level 10` VST3 ×3 | ✅ **3/3 SUCCESS**, zero failures |
| `pluginval --strictness-level 10` AU ×3 | ✅ **3/3 SUCCESS**, zero failures |
| `auval -v aufx ORvD OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| AU component version | ✅ **65536** (= 1.0.0); `CMakeLists.txt:11` reads `VERSION 1.0.0` |
| `CHANGELOG.md` | ✅ authored at v1.0.0 with the actual numbers above |

pluginval-10 coverage includes Editor, Open editor whilst processing, Automation,
Editor Automation, **Plugin state**, **Plugin state restoration**, Parameter
thread safety and Fuzz parameters — so the new preset-manager state routing was
exercised, not just compiled.

---

## Deviations from PLAN.md

1. **Task 3 skipped** — conditional on Task 1, which declined it. Harness is
   33 entry / 41 exit, not 34 / 42.
2. **Binary-data symbol is `presetmanager_js`, not `preset_manager_js`.**
   `juce_add_binary_data` **strips** the hyphen rather than converting it to an
   underscore. Caught at first compile; the plan's predicted name was wrong.
3. **`highCut` audit tolerance set from measurement (open item #2).** The plan
   pencilled `< 2.0 Hz` and I initially coded `5.0`; the first probe-N run printed
   **0.0000** for every parameter, so the shipped tolerance is **0.5 Hz** — the
   same as `lowCut`, ~4 orders of magnitude above the observed error and ~4 below
   what a real skew bug produces.
4. **Probe N's wash window is [2 s, 4 s], not the plan's [6 s, 8 s].** At the
   plan's window a low-feedback preset like *Vocal Halo* (fb = 30, delay 380 ms
   ⇒ ≈ −17.8 dB/generation) has legitimately decayed far below the 1e-7 floor, so
   the assertion would have failed correct audio. The window now sits immediately
   after the 2 s excitation burst, where every preset must sound. Measured
   `washRms` ranges 0.0131 (Vocal Halo) → 0.0421 (Slow Wash); threshold 1e-5.
5. **`ui_frontend_check.js` needed a THIRD repair beyond the plan's §3 and §9.**
   The `juce_add_binary_data` block regex is non-greedy to the first `)`, so a
   single parenthesis inside an explanatory comment silently truncated the SOURCES
   list and turned correct code into a FAIL. The block is now matched against a
   comment-stripped copy of the CMake, mirroring what §11 already does for the
   harness CMake. (This bit during execution: my own C3 comment contained "(C3 —
   …)".)
6. **Bar ships 5 controls including Delete** — user-confirmed against D15's
   4-control sketch, so human-checklist item 6 stands as locked.
7. **Bar geometry decomposition:** 32 px height + 12 px `margin-bottom` = the
   44 px band. `ui_frontend_check` §12 now asserts that sum, since it is what
   keeps the panels and footer where Stage 3 left them.

---

## Success Criteria

| # | Criterion | Status |
|---|-----------|--------|
| 1 | D11 makeup constant closed | ✅ explicitly **declined**, recorded here + in CHANGELOG |
| 2 | Harness green at entry and exit | ✅ 33/33 entry, **41/41** exit, exit 0 |
| 3 | Preset manager v1.0.5, 8 presets, engineering units, audited, re-seed verified | ✅ round-trip delta 0.0000 |
| 4 | Bar at 940×484; check repaired + extended; grep-diff 11 ≡ 11 | ✅ 76/76 exit 0 |
| 5 | Tooltips on 10 controls, `mix` full-width | ✅ 230 px, clamped, arrow tracks |
| 6 | Stub render exact, zero console errors, bar driven | ✅ |
| 7 | pluginval-10 ×3 both formats; auval | ✅ 3/3 + 3/3 + SUCCEEDED |
| 8 | Fresh install with dual-variant sweep | ✅ |
| 9 | CHANGELOG at v1.0.0; VERSION + AU 65536 | ✅ |
| 10 | Human checklist handed over; Windows deferred; `/publish` separate | ✅ below |

---

## Human checklist — outstanding (`human_needed` at verify)

1. ✅ **D7 audition** — done at Phase 4.0; no makeup constant shipped, so no
   re-audition is required.
2. ⬜ Load in **Logic and Ableton**: WebView renders at 940 × 484; automation
   round-trips in-host.
3. ⬜ Mono→stereo listen (Stage-1 carryover; probe measured Δ0.0000 dB, never heard).
4. ⬜ Session save/reload round-trip (Stage-1 carryover — now also exercises the
   new preset-name state routing).
5. ⬜ All 8 factory presets audibly distinct, none runaway/NaN — especially
   **Near-Infinite**.
6. ⬜ Save a user preset → reload → delete, through the new bar **in the real
   WebView**. The stub drove this leg end to end, but the two dialog functions
   open a native `FileChooser` that only exists in the plugin.
7. ⬜ All 10 tooltips appear on hover; the right-most (`mix`) tooltip is **not**
   shrink-wrapped. Verified in the browser; confirm in the real WKWebView.

**Windows:** explicitly deferred to CI (D13/D14). `/publish` remains a separate,
user-triggered step.
