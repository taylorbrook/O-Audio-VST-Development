# Stage 4: Polish / Validation — Verification

## Verification Date

2026-07-24

**Method:** goal-backward analysis against `CONTEXT.md` (D11–D16, C1–C10) and
`PLAN.md`, with **every automated gate independently re-run at verify** — not
read from `SUMMARY.md`. The WebView UI was re-rendered in a real browser at the
shipping 940×484 viewport, per `pattern_js_state_updater_overwrites_html_labels`
("reading the HTML is not verification — render it").

---

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

1. **Close the D11 wash-decay question** — the makeup constant is either shipped with probe-G green, or explicitly declined and recorded. Closure, not silence.
2. **Integrate OuariconPresetManager v1.0.5** with 8 factory presets authored in engineering units, audited through the real `loadPreset()`.
3. **Ship a preset bar at 940×484** plus tooltips on all 10 controls, with **zero regression** to the just-verified Stage-3 GUI.
4. **Re-confirm the full validation surface** — harness, pluginval-10 ×3 both formats, auval, fresh dual-variant install.
5. **Author the CHANGELOG at v1.0.0** and hand over the human checklist; Windows deferred to CI, `/publish` a separate step.

### Deliverables (from SUMMARY.md, confirmed by re-run + code inspection)

1. D11 **declined** by user audition at `feedback = 100`; Task 3 skipped ⇒ zero DSP diff.
2. Preset manager v1.0.5 via CMake include, 8 presets, `convertTo0to1` loop, probe N audit (8 new harness checks).
3. 44 px band (32 px height + 12 px margin), 5-control bar, tooltips on 10 controls, `ui_frontend_check.js` repaired (§3, §9) and extended (§12–14).
4. Harness 41/41; pluginval-10 ×3 both formats; auval SUCCEEDED; install swept.
5. CHANGELOG at v1.0.0; 7-item checklist batched, 1 done / 6 outstanding.

### Goal Achievement

| Goal | Status | Evidence (verify-phase re-run) |
|------|--------|--------------------------------|
| 1. D11 closed | ✅ Achieved | Recorded as **explicitly declined** in SUMMARY §4.0 + CHANGELOG. Falsifiable claim tested: `git diff bbefa10 -- Source/PluginProcessor.cpp` yields **exactly 2 hunks** — the constructor (preset table) and `get/setStateInformation`. `processBlock` and every DSP function are **untouched**. Zero-DSP-diff confirmed, not assumed. |
| 2. Preset system | ✅ Achieved | Harness re-run: 8 `preset-*` probes PASS, `worst=0.0000` on every parameter of all 8. Seeded JSON on disk proves skew was applied: `Reverse Bloom` stores `delayTime 0.60037` for 500 ms (a linear fraction would be 0.231) and `highCut 0.71706` for 8 kHz (linear 0.385). |
| 3. Bar + tooltips, no regression | ✅ Achieved | Browser render at 940×484: frame **exactly 940×484**, overflowX/Y **0**, band **32+12=44**, panels still **215 px**, 6 bar IDs present, fresh instance reads **`Default`**. UI-02 slot box **identical across modes** (`x:117 y:242 w:86 h:100`), correct control visible each way, `aria-pressed` flips, 13 divisions. Knobs unregressed: drag 35 %→62 %, dblclick reset →35 %; all 8 readouts at exact engineering defaults. |
| 4. Validation surface | ✅ Achieved | Verify-phase re-runs: harness **41/41 exit 0**; `pluginval --strictness-level 10` **VST3 SUCCESS** + **AU SUCCESS**; `auval -v aufx ORvD OuDv` **SUCCEEDED**; AU version **65536**, triple `aufx ORvD OuDv`; only `-dev` bundles installed (no orphan unsuffixed variant). |
| 5. Release artifacts | ✅ Achieved | `CHANGELOG.md` at `[1.0.0] — 2026-07-24`; `CMakeLists.txt:11` reads `VERSION 1.0.0`. Windows marked deferred-to-CI; `/publish` left to the user. |

---

## Requirements Verification

**Stage:** 4-polish
**Traceability entry:** `stage-4 | COMPAT-*, all remaining`

All 14 requirements were already marked `complete` at stages 1–3. Stage 4 ships
**no new requirement** — its job is to re-confirm the existing set survives the
preset/bar/tooltip diff. Every one was re-checked:

| Requirement | Priority | Status | Re-confirmed at stage 4 by |
|-------------|----------|--------|----------------------------|
| FUNC-01 granular reverse smear | must | ✅ Complete | harness `impulse-bloom` PASS |
| FUNC-02 Sync + Free timing | must | ✅ Complete | `sync-quarter-120`, `free-150/500/1200ms` PASS (exact latency) |
| FUNC-03 damped feedback regen | must | ✅ Complete | `damping-generations`, `single-generation` PASS |
| FUNC-04 stereo width | should | ✅ Complete | `width-0-centered`, `width-100-spread` PASS |
| DSP-01 click-free grains | must | ✅ Complete | `clicks-defaults`, `clicks-density-sweep` PASS |
| DSP-02 RT-safe damping filters | must | ✅ Complete | zero DSP diff since Stage-2 code review; `sweep-lowCut/highCut` PASS |
| DSP-03 loop stability @ fb 100 | must | ✅ Complete | `stability-60s` peak 0.2404, zero NaN; **plus** `preset-Near-Infinite` 30 s render peak 0.2838 |
| DSP-04 skewed ranges, presets in engineering units | should | ✅ Complete | `convertTo0to1` loop + probe N worst delta **0.0000** |
| UI-01 all 10 params, scaled readouts | should | ✅ Complete | browser render: 8 readouts at exact defaults; dblclick reset via `getParameterDefaults`; frontend check §4/§5 PASS |
| UI-02 Sync/Free swap | nice | ✅ Complete | slot box **mode-invariant** at the new y=242 (+44 as predicted) |
| PERF-01 real-time safe | must | ✅ Complete | `processBlock` untouched; pluginval-10 ×3 (execute) + ×1 both formats (verify) |
| COMPAT-01 pluginval VST3 + AU | must | ✅ Complete | **re-verified this phase** — VST3 SUCCESS, AU SUCCESS at strictness 10 |
| COMPAT-02 no-tempo fallback | must | ✅ Complete | `no-playhead-fallback`, `null-bpm-fallback` PASS |
| QUAL-01 artifact-free | must | ✅ Complete | 10 `sweep-*` probes + `mode-switch` PASS |

**Requirements Summary:**
- ✅ Complete: **14**
- ⚠️ Partial: 0
- ⏸️ Deferred: 0
- ❌ Failed: 0

---

## Automated Checks (all re-run at verify)

| Check | Result | Notes |
|-------|--------|-------|
| Build — VST3, AU, harness | ✅ Pass | CMake reconfigure clean, **zero warnings** |
| Render harness | ✅ Pass | **41/41 PASS, exit 0** (33 Stage-2 + 8 preset audits) |
| `ui_frontend_check.js` | ✅ Pass | **76/76, 0 failures, exit 0** — see Issue 1 on the count |
| Native-fn grep-diff | ✅ Pass | **11 ≡ 11**, names match exactly (1 in `app.js` + 10 in `preset-manager.js` ≡ 11 `withNativeFunction`) |
| `pluginval --strictness-level 10` VST3 | ✅ Pass | **SUCCESS** (verify confirmation run; 4 total with execute) |
| `pluginval --strictness-level 10` AU | ✅ Pass | **SUCCESS** (verify confirmation run; 4 total with execute) |
| `auval -v aufx ORvD OuDv` | ✅ Pass | **AU VALIDATION SUCCEEDED** |
| AU component version | ✅ Pass | **65536** = 1.0.0; type/subtype/manufacturer `aufx ORvD OuDv` |
| Dual-variant install sweep | ✅ Pass | Only `O-ReverseDelay-dev.{vst3,component}` present — no orphan unsuffixed bundle (`critical_dev_release_variant_shadowing`) |
| Browser render @ 940×484 | ✅ Pass | Exact geometry, overflowX/Y = 0, band = 44, panels 215 px, name = `Default` |
| Console cleanliness | ✅ Pass | **Zero JS errors.** Only entry is the stub-server `favicon.ico` 404, absent from the WebView path |
| **C5** tooltip edge safety | ✅ Pass | All 10 tips a full **230 px**; `mix` clamped to `left:702` = exactly `940−230−8` with `--arrow-x:157px` tracking; `delayTime` clamps at `left:8`. **Zero overflow on any of the 10.** Note: this only fails at a 940 viewport — an early probe at 1200 px never exercised the clamp |
| **C6** HTML labels survive binding | ✅ Pass | Save / Load / **Delete** labels and FREE/SYNC text intact after bind and after a mode toggle |
| UI-02 mode invariance | ✅ Pass | `x:117 y:242 w:86 h:100` identical both modes; 13 divisions |
| Preset bar round-trip | ✅ Pass | ◀ ▶ cycle all 8 names in case-insensitive order, **wrap** at the 9th, prev reverses |
| Knob non-regression | ✅ Pass | drag 35 %→62 %, dblclick →35 %; all 8 readouts at exact engineering defaults |
| **C1** skew on disk | ✅ Pass | `delayTime 0.60037` / `highCut 0.71706`; probe N round-trip **0.0000** |
| **C3** single binary-data target | ✅ Pass | One `juce_add_binary_data`, `NAMESPACE UIBinaryData` + distinct `HEADER_NAME` |
| **C8** no "/" in preset names | ✅ Pass | All 8 filenames slash-free ("Rhythmic Reverse") |
| **C9/F6** MSVC SafePointer | ✅ Pass | Frontend check asserts hoisted-to-local (never nested init-capture) **and** the null path bare-returns without `complete()` |
| Factory seeding on disk | ✅ Pass | 8 JSONs + `.factory-version` = `1.0.0` |
| `User/` dir absence | ✅ Not a defect | `savePreset()` calls `getUserPresetsDirectory().createDirectory()` before writing (`OuariconPresetManager.h:348`) and `getPresetList` guards on `isDirectory()` — the dir is created lazily on first save |
| Zero-DSP-diff claim | ✅ Pass | Diff vs `bbefa10` = constructor + `get/setStateInformation` only |

---

## Human Verification

Item 1 was completed during Phase 4.0 (it was the entry gate). **Six remain
outstanding** — none is a blocker for stage closure, all are pre-`/publish` listens:

- [x] 1. **D7 audition** — done at Phase 4.0; makeup constant declined, so no re-audition needed.
- [ ] 2. Load in **Logic and Ableton**: WebView renders at 940×484; automation round-trips in-host.
- [ ] 3. Mono→stereo listen (Stage-1 carryover; probe measured Δ0.0000 dB, never heard).
- [ ] 4. Session save/reload round-trip — now also exercises the new preset-name state routing.
- [ ] 5. All 8 factory presets audibly distinct, none runaway/NaN — especially **Near-Infinite**.
- [ ] 6. Save → reload → delete a user preset through the bar **in the real WebView**. The stub drove this leg end to end, but the two dialog fns open a native `FileChooser` that exists only in the plugin.
- [ ] 7. All 10 tooltips on hover in the real WKWebView; `mix` not shrink-wrapped. Measured 230 px in the browser at the true viewport.

---

## Issues Found

1. **`ui_frontend_check.js` assertion count was reported as 77/77; it is 76/76.**
   The suite has **76** `check()` call sites — the 77th `grep` match is the
   `function check(cond, desc)` definition itself. Zero functional impact (0
   failures, exit 0 either way), but it was stated in the user-facing CHANGELOG.
   **Resolution:** corrected to 76/76 in `CHANGELOG.md` and `SUMMARY.md`.
   All 76 call sites execute — none is skipped behind a conditional.

2. **`CMakeLists.txt:77` misdocumented the binary-data symbol.** The comment read
   `Symbol: UIBinaryData::preset_manager_js`, but `juce_add_binary_data` **strips**
   the hyphen — the real symbol is `presetmanager_js`, which is what
   `PluginEditor.cpp:70` correctly uses. Comment-only, so the build was never
   affected; but it misdocumented a **recurring** trap
   (`critical_binary_data_strips_hyphens`) at the exact place a future reader
   would look, and it contradicted SUMMARY deviation #2.
   **Resolution:** comment corrected and the memory-pattern name cited inline.
   The fix deliberately introduces parentheses inside a CMake comment — the very
   thing that broke §9 during execute — so it doubles as a regression test of
   deviation #5's comment-stripping repair. Re-ran after the edit: frontend check
   **76/76 exit 0**, CMake reconfigure + build clean, zero warnings.

**No defects found in shipped behaviour.** Both issues are documentation-level.

### Observation for future stages (not a defect)

The C5 tooltip gate is **viewport-sensitive**: at a 1200 px browser viewport the
`mix` tip renders 230 px wide but its right edge lands at 974, and the clamp is
never exercised because the code clamps against `window.innerWidth`. Only at the
shipping 940 px viewport does `left` clamp to 702. Any future re-run of this gate
must resize the browser to the real window size first, or it silently proves
nothing.

---

## Stage Verdict

**Status:** ✅ **VERIFIED**

**Ready for next stage:** Yes — this is the final stage. **Plugin is ship-ready at v1.0.0.**

**Blockers:** none.

**Carried forward to `/publish`:**
- 6 outstanding human-checklist listens (above) — recommended before publishing.
- **Windows** is deferred to CI by D13/D14. Static prerequisites are in place:
  both WebView2 CMake flags set, and the two `FileChooser` completions hoist
  their `SafePointer` to a local (the pattern MSVC rejects when init-captured in
  a nested lambda). First Windows CI build is the real test.
- `/publish` remains a separate, user-triggered step.
