# Stage 4 (Polish) — PLAN

**Goal:** Ship v1.0.0 — the 8-preset concept tour (FUNC-06) fully wired and audibly correct, playability confirmed (FUNC-07), and a clean VST3+AU validation sweep. Zero DSP/parameter changes; Stages 2–3 stay frozen.

## Tasks

### T1 — Complete the preset roster in the UI (additive)
- `index.html`: reorder + extend the `.tour-buttons` to the full 8 in pedagogical order:
  Saw Sweep · Pluck · Brass Stab · Sweep Pad · Acid Bass · Square Bass · Noise Wind · Self-Oscillation.
  Add `data-tip` keys `lessonSawSweep`, `lessonSquareBass`, `lessonNoiseWind`.
- `app.js`: add the 3 new entries to `LESSONS` (caption strings) and to the `TIP` map (hover tooltips), matching the existing tone.
- **Files:** `Source/ui/public/index.html`, `Source/ui/public/js/app.js`
- **Invariant:** `data-preset` key == LESSONS key == C++ match string (all 8).

### T2 — Author the 8 C++ snapshots in `applyFactoryPreset`
- Replace the stub body with: reset-to-default loop + `setReal`/`setChoice` lambdas + an 8-branch `if/else` using the values from RESEARCH.md §3.
- Use `parameters.getParameter(id)` + `convertTo0to1(real)`; choices via index.
- **File:** `Source/PluginProcessor.cpp` (method already declared in `.h`; signature unchanged).
- **Invariant:** touches NO DSP — only APVTS writes through the public host API.

### T3 — Build VST3 + AU, clear caches, install
- `ninja O-simpleSubtractive_VST3 O-simpleSubtractive_AU` from `build/`.
- Cache-clear + dual-variant sweep + install per CLAUDE.md (or `build-and-install.sh`).

### T4 — Validation sweep
- `auval -v aumu <subtype> <manuf>` → must print **AU VALIDATION SUCCEEDED**.
- `pluginval --strictness-level 10` on the VST3 → **ALL TESTS PASSED**.
- Smoke-load check: state round-trips (already green Stage 1, re-confirm after preset writes).

### T5 — CHANGELOG v1.0.0 + STATUS + REQUIREMENTS
- New `CHANGELOG.md` with the v1.0.0 entry (feature summary across Stages 1–4).
- Mark FUNC-06 / FUNC-07 ✅ in `REQUIREMENTS.md`; update `STATUS.md` to Stage 4 complete / 100%.

### T6 — SUMMARY.md + VERIFICATION.md
- `SUMMARY.md`: what shipped, files touched, preset table.
- `VERIFICATION.md`: goal-backward pass/fail against FUNC-06, FUNC-07, validation sweep.

## Success criteria
- [ ] 8 tour buttons present; each loads a distinct, audibly-on-concept patch; UI controls + visuals update with no DOM poking.
- [ ] `data-preset` / `LESSONS` / C++ match strings agree for all 8 (no silent no-op).
- [ ] Self-Oscillation plays a clean in-tune sine across the keyboard; Noise Wind is audibly noise-dominant (osc fundamental rejected).
- [ ] No DSP or parameter-default change; Stage 2/3 behaviour intact (regression).
- [ ] auval SUCCEEDED; pluginval s10 ALL TESTS PASSED; VST3+AU build clean.
- [ ] CHANGELOG v1.0.0 written; STATUS/REQUIREMENTS updated.

## Risk / regression watch
- **Silent bridge gap** if a key mismatches — grep-diff the three lists after T1/T2.
- **Skew bug** if normalised values are passed to `convertTo0to1` — pass reals only.
- Do not alter the 5 existing presets' behaviour beyond filling their (previously empty) snapshots.
