# O-Octagon — Implementation Roadmap

**Date:** 2026-08-11
**complexity_score: 5.0**
**Classification:** Complex (maximum score — capped)
**Strategy:** Phase-based implementation
**Build target:** `OuariconOctagon` (folder `plugins/O-Octagon`, PRODUCT_NAME `O-Octagon${OUARICON_DEV_SUFFIX}`)
**Architecture contract:** `plugins/O-Octagon/.planning/research/ARCHITECTURE.md`

---

## Complexity Factors

```
score = min(param_count / 5, 2.0) + algorithm_count + feature_count      (cap 5.0)
```

- **Parameters:** 17 musical, automatable → `min(17/5, 2.0)` = **2.0** (capped)
  - Plus 42 non-automatable venue values in a separate store. These do not enter the
    formula (they are not parameters) but they are real specification and UI surface,
    and they are the reason Stage 3 is phased as heavily as Stage 2.
- **Algorithms:** 8 DSP/algorithmic components = **8.0**
  1. `VenueModel` — bounding box, centroid, `rigScale`, sloped audience plane
  2. `ChannelMap` — speaker→buffer map via `getChannelIndexForType()` + user label layer
  3. `ConvexHull2D` — monotone chain, classification, inside test, nearest-point projection
  4. `DbapSolver` — 3D DBAP, 2011-04-14 revised equations
  5. `SourceShaper` — bbox denormalisation, stereo sub-points, `rFade` collapse, rake resolution
  6. `HullProcessor` — outside-hull gain trim + air-absorption one-pole LPF
  7. `GainStage` — 64-sample control grid, 17 per-sample smoothers, trims, output gain
  8. `VerifyPing` — band-limited pink burst, envelope, map-level injection
- **Features:** **3.0**
  - Multichannel output bus negotiation, non-standard container (+1)
  - Two-store state persistence + venue file I/O (+1)
  - Real-time visualisation — 8 live meters, DBAP level-field gradient (+1)
  - *(no FFT, no feedback loops, no multiband, no MIDI, no modulation system)*
- **Total:** 2.0 + 8.0 + 3.0 = 13.0 → **capped at 5.0**

**staged_implementation: true**

The score saturates the cap by a factor of 2.6. This is the most algorithmically dense
plugin planned in this repo to date, and the density is *breadth* (eight independent
components that must integrate correctly) rather than depth in any one component. That
shape is what the phase breakdown below is designed for: each phase closes a set of
components completely, with its own test gate, before the next depends on it.

---

## Stage Map — bound to REQUIREMENTS.md traceability

The traceability table in `REQUIREMENTS.md` is authoritative and is **not** modified here.
Every phase below is a subdivision of an existing stage, never a re-assignment.

| Stage | Requirements verified (from REQUIREMENTS.md) | Phases |
|-------|---------------------------------------------|--------|
| Stage 1 — Foundation | FUNC-01, COMPAT-01, COMPAT-04 | single pass |
| Stage 2 — DSP | FUNC-03, FUNC-07, DSP-01..08, PERF-01, PERF-02, COMPAT-03, QUAL-01..04 | 2.1, 2.2, 2.3 |
| Stage 3 — GUI | FUNC-02, FUNC-04, FUNC-05, FUNC-06, UI-01..05 | 3.1, 3.2, 3.3 |
| Stage 4 — Validation | COMPAT-02, all remaining | single pass |

---

## Stage 1 — Foundation (single pass)

**Goal:** A loadable, validating 8-channel shell with all 17 parameters and a correct bus
declaration. No DBAP yet — audio passes silent or dry.

**Components**

- `CMakeLists.txt` per ARCHITECTURE §12: target `OuariconOctagon`, `PLUGIN_CODE OuOc`,
  **`VERSION 1.0.0`** (never `PLUGIN_VERSION` — that keyword is silently ignored by JUCE
  and ships 1.0.0 regardless), `juce::juce_dsp` linked, `juce_generate_juce_header()` after
  `target_link_libraries` and before `target_compile_definitions`, and
  **no `PLUGIN_CHANNEL_CONFIGURATIONS`**.
- `BusesProperties` in the **constructor member-initialiser list**: mono in →
  `create7point1()` out.
- `isBusesLayoutSupported()` exactly as ARCHITECTURE §4.2 — mono/stereo in; 7.1,
  7.1-SDDS, 5.1.2 out for real mode; mono/stereo out for SAFE mode.
- APVTS with all 17 `AudioParameterFloat` parameters, ranges and defaults per
  ARCHITECTURE §6.1.
- `getStateInformation` / `setStateInformation` round-tripping `apvts.copyState()`. The
  VENUE child is added in Stage 2.1; the serialisation code is written once here and does
  not change.

**Test criteria**

- [ ] Builds clean on macOS (VST3, AU, Standalone) with zero warnings from
      `juce_recommended_warning_flags`
- [ ] `auval -a | grep -i octagon` lists the AU
- [ ] **pluginval strictness 10 passes — VST3 and AU — run 2-3 times**
      (`pattern_ci_pluginval10_catches_latent_nan`)
- [ ] Standalone opens on a 2-channel interface (SAFE mode) without error — this is the
      COMPAT-04 gate and it is why SAFE mode exists
- [ ] All 17 parameters appear in the host automation list with correct names, ranges,
      defaults and units
- [ ] A parameter change round-trips through save/reload of session state
- [ ] **FUNC-01, COMPAT-01, COMPAT-04 verified**

---

## Stage 2 — DSP (3 phases)

### Phase 2.1 — Geometry Core

**Goal:** Everything that is geometry and routing, with the channel-map test suite standing
before a single gain is computed. This phase deliberately front-loads the plugin's highest
risk.

**Components**

- `Source/Data/VenueModel.{h,cpp}` — 42-value `ValueTree` store, bounding box, centroid,
  `rigScale`, `earHeight(y)` with the zero-span guard, default venue per ARCHITECTURE §OQ4
- `Source/DSP/ConvexHull2D.{h,cpp}` — monotone chain with `<= EPS_CROSS` (collinear points
  **popped**), area-scaled epsilon, dedup pre-pass, classification, inside test,
  nearest-point projection, the full §3.1.6 degeneracy matrix
- `Source/DSP/ChannelMap.{h,cpp}` — `rebuildChannelMap()` per ARCHITECTURE §3.2.3:
  single construction site, `ChannelType`-keyed, permutation-validated, `mapInvalid` atomic
- `VenueSnapshot` double-buffer + generation counter (ARCHITECTURE §3.6.6)
- VENUE child tree attached to `apvts.state`; session round-trip extended

**Test criteria**

- [ ] **Channel-map Layer 1** — runtime invariant: order reconstructed by scanning
      `getChannelIndexForType()` over bits 0..63 is strictly increasing and matches
      `getTypeOfChannel(i)` for all i
- [ ] **Channel-map Layer 2** — `tests/tools/gen_juce_channel_order.py` parses
      `juce_AudioChannelSet.h`/`.cpp` at build time, emits `JuceChannelOrderGolden.h`
      plus a SHA-256; the test compares against a committed checksum and **fails the build**
      if JUCE's enum values or 7.1 membership change. *Asserted against parsed source, never
      a mirrored constant* (`pattern_test_fixture_mirrors_drift_silently`)
- [ ] Duplicate label assignment → map rejected, `mapInvalid` set, last valid map retained
- [ ] Missing label assignment (type absent from the negotiated set) → same
- [ ] Hull of the traced layout yields exactly vertices **1, 2, 4, 5, 6, 7**, with 3 and 8
      classified `ON_EDGE` — not interior, not vertex
- [ ] A point at a physical rear corner of the room classifies as **outside**
- [ ] Hull projection matches brute-force nearest-point-on-segment to 1e-6 for a 200-point
      fixture set
- [ ] Degenerate venues — all 8 collinear, all 8 coincident, zero rake span — produce finite
      results and do not crash
- [ ] `srcZ = 0` resolves to a height varying linearly from `rakeFront` at `bbMinY` to
      `rakeRear` at `bbMaxY`; changing `rakeRear` alone changes a rear source's absolute height
- [ ] `grep -rn` confirms **zero hardcoded output channel indices** outside `ChannelMap`
- [ ] **COMPAT-03, DSP-03, DSP-04, FUNC-03 verified**

---

### Phase 2.2 — DBAP Solve and Gain Application

**Goal:** Audio actually spatialises, at constant intensity, block-size invariantly.

**Components**

- `Source/DSP/DbapSolver.{h,cpp}` — 3D revised equations, `kInvTwentyLog10Two`,
  `kMinDistance` floor, blur→`r_s` mapping against `rigScale`, the all-zero-weight silence
  guard, `t = pow(d,-a)` reuse so only 8 `pow` per sub-point
- `Source/DSP/GainStage.{h,cpp}` — the **64-sample absolute-sample-aligned control grid**,
  17 `SmoothedValue` (8 L + 8 R + output gain) at `reset(sr, 0.005)`, the per-sample inner
  loop with its exactly-once `getNextValue()` invariant
- Dirty-check: 17-float snapshot + venue generation counter; solve skipped when unchanged
- `ScopedNoDenormals`
- `tests/render-harness/` — built **now**, not at Stage 4. `createEditor` guarded with
  `#if JUCE_WEB_BROWSER` so it survives the Stage-3 WebView swap
  (`pattern_render_harness_breaks_on_webview_editor`)

**Test criteria**

- [ ] `a = R / (20·log10 2)` matches hand-computed values at R = 3, 4, 6
- [ ] `d_i` includes the `(z_i − z_s)²` term — changing **only** `srcZ` changes the gain
      vector (note: the graded default speaker heights of ARCHITECTURE §OQ4 exist so this
      test cannot pass vacuously)
- [ ] Gain vector matches an independent reference implementation of eqs 9-10 to 1e-6 over a
      fixture set of source positions
- [ ] **`Σ v_i² = 1 ± 1e-6`** across a dense sweep inside the hull, outside the hull, at
      hull vertices, and at exact speaker positions — measured **at the solver output**,
      before the hull gain trim deliberately breaks it
- [ ] Holds across the full rolloff (3-6) × blur (0-1) product
- [ ] `w_i = 0` → exactly zero gain at speaker i; a 2-speaker subset preserves `Σ v_i² = 1`
      and measured output level is unchanged
- [ ] **All-zero weights → silence, not NaN, not full-scale**
- [ ] Source at each exact speaker coordinate with `blur = 0` → finite output
- [ ] Pathological input (silence, DC, full-scale, denormals) → no sticky NaN
- [ ] **Block-size invariance:** with parameters driven programmatically at control-grid-
      aligned absolute sample offsets, renders at blockSize 512 and 4096 are **bit-identical
      (`memcmp`, not a tolerance)**. Separately, with parameters held constant, any pair of
      block sizes is bit-identical. *(ARCHITECTURE §3.6.3 — this protocol is the executable
      form of QUAL-03 and must be implemented as written.)*
- [ ] No zipper noise on a full-speed sweep of position or of all 8 weights
- [ ] RT-safety pass: no allocation, lock or file I/O in `processBlock`; `pow` count per
      block confirmed ≤ 32 by instrumentation
- [ ] **Channel-map Layer 3 (the audible test):** render with `w_j = δ_ij` and a unique tone
      per speaker; assert each output channel's dominant FFT bin is exactly its speaker's
      frequency. **Mandatory — this is the layer that catches a reintroduced hardcoded index.**
- [ ] **DSP-01, DSP-02, DSP-05, PERF-01, PERF-02, QUAL-02, QUAL-03, QUAL-04 verified**

---

### Phase 2.3 — Source Shaping and Outside-Hull Processing

**Goal:** Width, hull attenuation, air absorption, per-speaker trims. The chain is complete.

**Components**

- `Source/DSP/SourceShaper.{h,cpp}` — bbox denormalisation with the zero-span guard;
  bearing from the **centroid**; `rFade = 0.15·rigScale` spread collapse; the `(0,−1)`
  fallback bearing at the exact centre; per-sub-point rake resolution; the always-two-
  sub-points construction with the `0.5` feed (no `width == 0` branch)
- `Source/DSP/HullProcessor.{h,cpp}` — dB/m trim floored at −24 dB; air LPF
  `fc = clamp(20000·2^(−airAmount·d_hull/3), 500, 20000)`; **`airAmount = 0` skips the
  filter entirely and resets its state**; per-block `std::isfinite` state check with
  `reset()` on failure
- Venue trims folded into the smoothed targets after the solve

**Test criteria**

- [ ] `width = 0` produces gain vectors bit-identical to a single mono-summed source point
- [ ] `width > 0` produces two sub-points perpendicular to the bearing from the centroid,
      with L on the audience's left when the puck is downstage
- [ ] **Sweeping the puck through the centroid at `width = 6` produces no discontinuity** —
      sample-level check for a step in any output channel exceeding the smoother's maximum
      per-sample delta
- [ ] Sub-points straddling the hull boundary (one in, one out) behave correctly and
      continuously
- [ ] `hullAtten = 0` → bit-identical output to hull processing removed
- [ ] `airAmount = 0` → **bit-identical** to the filter absent (this is what the skip branch
      buys; a 20 kHz cutoff would fail this)
- [ ] Air LPF cutoff matches the specified curve at `airAmount` ∈ {0.35, 1.0} × `d_hull` ∈
      {5, 15} m
- [ ] Injecting a non-finite sample recovers within one block (no sticky NaN)
- [ ] Per-speaker trim of −12/+6 dB produces exactly that level change at that speaker and
      nowhere else
- [ ] Full-range sweeps of every parameter produce no clicks, discontinuities or level jumps
- [ ] Block-size invariance still holds with width, hull and air all active
- [ ] **DSP-06, DSP-07, DSP-08, FUNC-07, QUAL-01 verified**

---

## Stage 3 — GUI (3 phases)

WebView throughout, per juce8-critical-patterns §3, §9, §11, §12, §13, §21.
Two screens: **Room** (performance) and **Venue** (measurement).

### Phase 3.1 — Two-screen shell, Room plan, musical parameters

**Goal:** The performance surface works.

**Components**

- WebView editor: `std::unique_ptr` members in relay → webview → attachment order;
  `NEEDS_WEB_BROWSER TRUE`; `check_native_interop.js` in `juce_add_binary_data` and served
  by the explicit-URL resource provider; `type="module"` on every script tag
- Screen switcher (Room / Venue)
- Room plan canvas: proportioned to the derived room envelope (speaker bbox + 15% margin),
  **explicit convex hull overlay**, 8 numbered speaker glyphs, draggable source puck
- 8 weight controls sited *at* their speakers on the plan
- Rolloff, blur, width, hull attenuation, air, output controls
- Height control (bare slider in this phase; the elevation strip is 3.3)
- All 17 `WebSliderParameterAttachment` bindings — **three arguments**, the third `nullptr`
- `srcX`/`srcY` readouts in **metres**, resolved against the live venue via a
  `getNativeFunction` call, not a JS-side min/max map
  (`pattern_webview_knob_readout_scaled_value`)
- SAFE-mode banner driven by an atomic from the processor

**Test criteria**

- [ ] Render the UI against `tests/ui-stub/juce-stub.js` **before** integrating, so a
      top-level TDZ throw cannot take out every later initializer
      (`pattern_module_toplevel_init_tdz`)
- [ ] `grep`-diff every `getNativeFunction` in JS against every `withNativeFunction` in C++
      — zero gaps (`pattern_webview_native_fn_bridge_gap`)
- [ ] Canvas sized with explicit `width`/`height` in `calc()` plus a DPR backing store —
      never left+right stretch (`o-textureforge-cursor-bug`)
- [ ] Every control moves its parameter; host automation moves every control
- [ ] Puck drag is relative-delta, not absolute cursor tracking (critical-patterns §16)
- [ ] Hull overlay matches the computed hull, including the on-edge speakers 3 and 8
- [ ] Room plan proportions match the venue's aspect ratio
- [ ] SAFE-mode banner appears on a stereo track and only there
- [ ] **UI-02 verified**

---

### Phase 3.2 — Venue screen, venue store, verify-ping

**Goal:** The measurement surface works, and the patch is confirmable in the hall.

**Components**

- Venue screen: 8-row x/y/z entry in metres with numeric validation; rake front/rear;
  8-row speaker→channel-label mapping table showing the **negotiated set name**; 8
  calibration trims; hull classification readout per speaker (VERTEX / ON_EDGE / INTERIOR)
- Venue save/load via `FileChooser::launchAsync` — **`SafePointer` capture, and on a dead
  pointer `return` bare, never `complete(false)`**
  (`pattern_webview_launchasync_safepointer_no_complete`)
- Musical preset store, separate — `applyPresetJson` resets the 17 musical parameters to
  defaults first and **cannot reach the VENUE node**
  (`pattern_preset_apply_needs_reset_to_defaults` + FUNC-05, reconciled structurally)
- `Source/DSP/VerifyPing.{h,cpp}` — member-owned `juce::Random`; pink noise band-limited
  200 Hz – 8 kHz; −20 dBFS RMS / −6 dBFS peak ceiling **independent of `outputGain` and all
  trims**; 20 ms raised-cosine envelope; latched manual step with a 120 s safety timeout;
  auto-cycle 1.2 s on / 0.4 s gap, order 1→8; injected **at the channel map** with all other
  channels hard-zeroed
- `mapInvalid` warning surfaced

**Test criteria**

- [ ] All 24 coordinate fields and both rake heights accept typed metre values and reject
      non-numeric input
- [ ] Saving then reloading a venue reproduces **all 42 values exactly** (bit-compare)
- [ ] Editing a coordinate changes the gain vector
- [ ] **Loading a musical preset leaves all 42 venue values bit-identical**
- [ ] A musical preset saved under venue A recalls correctly under venue B, with position
      resolved against venue B's bounding box
- [ ] Session state round-trips both stores together
- [ ] Ping plays from **exactly one** speaker at a time, all others silent
- [ ] Manual step advances 1→8; auto-cycle completes all 8 unattended in 12.8 s
- [ ] Ping level is bounded by the fixed ceiling at `outputGain = +12 dB` and at
      `trim = +6 dB` — measured, not assumed
- [ ] Latched ping self-stops at 120 s
- [ ] Changing a label-map row moves audio to the corresponding physical output, confirmed
      by ping
- [ ] Duplicate/missing label assignment surfaces the warning and does **not** reroute
- [ ] **FUNC-02, FUNC-04, FUNC-05, UI-01 verified**

---

### Phase 3.3 — Visualisation and scenes

**Goal:** The plugin becomes readable at a distance in a dark hall.

**Components**

- 8 live per-speaker level indicators at their plan positions — atomic array, ~30 Hz Timer
  read, `requestAnimationFrame` ballistics, attack 0.5 / decay 0.12, −60..0 dBFS scale,
  1.5 s peak-hold releasing at 20 dB/s (critical-patterns §20)
- Scene buttons `ALL` `FRONT` `REAR` `LEFT` `RIGHT` `SIDES` + 4 user slots — each writes all
  8 weight parameters via `setValueNotifyingHost` so scenes record as ordinary automation
- DBAP level-field gradient backdrop (paper figs 1-3): per-pixel `max_i v_i²` over a coarse
  grid, recomputed on the message thread on geometry/weight change, drawn to an offscreen
  canvas and blitted — **never** recomputed per frame
- Side-elevation strip: raked audience line, speaker heights, source height above the plane

**Test criteria**

- [ ] Meters respond to audio, ballistics are smooth, and **the speaker that lights matches
      the speaker the sound comes from** (this is a second human check on the channel map)
- [ ] Each scene writes all 8 weights atomically and records as automation; two scenes can
      be faded between
- [ ] A shared JS state updater does not erase HTML-authored labels — render them, use
      `data-label` + `aria-pressed` (`pattern_js_state_updater_overwrites_html_labels`)
- [ ] Gradient backdrop matches the solver: sample 20 grid points and compare against a
      direct solve to 1e-3
- [ ] Gradient redraw does not spike CPU during puck drag (offscreen + blit confirmed)
- [ ] Elevation strip shows `srcZ = 0` riding the rake front-to-back
- [ ] **FUNC-06, UI-03, UI-04, UI-05 verified**

> **Descope path if Stage 3 runs long:** UI-04 (gradient) and UI-05 (elevation strip) are
> `nice` priority in REQUIREMENTS.md and neither gates a concert. Ship them in v1.1 before
> letting them delay the hall test. UI-03 (meters) is `should` and must **not** be dropped —
> it is a defence on R1.

---

## Stage 4 — Validation (single pass)

**Goal:** Logic Pro, in the hall, with the bounce path confirmed.

**Components and test criteria**

- [ ] Instantiates in Logic Pro on a surround track with 7.1 output
- [ ] **Record which 8-channel set Logic actually negotiated** (surfaced on the Venue
      screen). ARCHITECTURE §3.2.2 predicts this may be 7.1-SDDS, not plain 7.1 — this is
      risk R2 and Stage 4 is where it is settled. Feed the answer back into the research doc.
- [ ] Verify-ping confirms all 8 outputs reach distinct physical channels
- [ ] Automation of `srcX`/`srcY`/`srcZ` and `w1..w8` is visible and writable in Logic's
      automation lanes
- [ ] **Bounce-order test** (`research/logic-pro-multichannel-octaphonic-dbap.md` §6a test 1):
      bounce a 7.1 project with a distinct tone in each of the 8 slots, interleaved; read off
      the order; confirm the identity label map gives *channel N = speaker N*
- [ ] **LFE-gain test** (§6a test 2): bounce identical −20 dBFS tone into the LFE slot and one
      other slot; compare levels. Any delta means Logic is touching the LFE path and speaker 4
      needs a compensating default trim
- [ ] pluginval strictness 10, VST3 and AU, **run 2-3 times** before publishing
- [ ] `auval -v aufx OuOc <manufacturer>` passes
- [ ] Windows CI: VST3 builds and passes pluginval 10. Scan for MSVC C3493 (non-static
      `constexpr` in a lambda) and `SafePointer(this)` init-capture in nested lambdas
- [ ] Factory musical presets authored in **engineering units** with `convertTo0to1`
- [ ] CHANGELOG.md, NOTES.md, PLUGINS.md updated
- [ ] **COMPAT-02 verified; all remaining requirements closed**

---

## Implementation Notes

### Thread safety

- Audio thread reads the venue through a **double-buffered POD `VenueSnapshot`** with a
  release/acquire generation counter — never a `ValueTree`, never a lock, and deliberately
  not `std::atomic<shared_ptr>` (a refcount decrement landing on the audio thread is the
  `pattern_retired_map_reaper_rt_free` failure).
- All 17 parameters read as `std::atomic<float>` via `getRawParameterValue()->load()`,
  snapshotted **once per control block**, never per sample.
- Meters: `std::atomic<float>[8]`, relaxed ordering, audio-thread max-store / UI-thread
  read-and-zero. A benign race, acceptable for a meter.
- `mapInvalid`: `std::atomic<bool>`.
- Hull build, channel-map build, `ValueTree` mutation and all file I/O are **message thread
  only**.
- If any state restore is deferred through an `AsyncUpdater`, `cancelPendingUpdate()` must
  be called in the restore path (`pattern_asyncupdater_guard_flag_needs_cancel`).

### Performance

| Component | Cost |
|---|---|
| Per-sample inner loop | 8 × (2 mul + 1 add + 1 mul) + 17 `getNextValue()` ≈ 50 flops |
| DBAP solve | ≤ 16 `std::pow` per 64-sample control block |
| Hull inside test | 8 cross products per control block |
| Hull projection | O(≤ 8), only when the source is outside |
| Air LPF | 2 one-pole filters, per sample, skipped entirely at `airAmount = 0` |
| **Total estimate** | **< 0.5% of one core at 48 kHz** |

This plugin is not CPU-bound. **Do not trade correctness for speed anywhere in it** — every
"optimisation" available here (skipping the second sub-point solve, branching out a
`getNextValue()`, moving the solve to a per-block schedule) trades a real correctness
property for an unmeasurable gain.

### Latency

**Zero.** No lookahead, no oversampling, no FFT. Do **not** call `setLatencySamples()` —
`getLatencySamples()` is non-virtual in JUCE 8 and the repo has been caught by this before.

### Denormal protection

`juce::ScopedNoDenormals` at the top of `processBlock`. The air LPF is the only recursive
element; its 500 Hz cutoff floor keeps it clear of denormal territory on normal input.

### Known challenges

1. **The channel map is the project's risk budget.** Silent failure, passes every automated
   gate, audible only in the hall. Three-layer test suite in Phase 2.1 + 2.2. For
   `create7point1()` the enum-bit order *coincidentally* equals the initializer-list order —
   which means a hardcoded map looks correct today. Do not be reassured by that.
2. **Logic may negotiate 7.1-SDDS, not 7.1** (`kAudioChannelLayoutTag_Emagic_Default_7_1`).
   Mitigated by accepting all three 8-channel containers and keying the label map on
   `ChannelType`. Settled at Stage 4.
3. **PERF-02 and QUAL-03 are incompatible under a per-block solve.** The 64-sample
   absolute-sample-aligned control grid is the only construction that satisfies both. This
   is not an implementation detail — it is the reason offline bounces will match what was
   heard while composing.
4. **The centre-crossing L/R flip** was found at design time, not by ear. The `rFade` spread
   collapse fixes it. Do not remove it as "an unnecessary special case" — it is the opposite.
5. **`airAmount = 0` must be bit-transparent**, which requires a skip-and-reset branch, not
   a 20 kHz cutoff.
6. **Two-screen WebView UI is the largest in the repo.** Stub-render before integrating;
   grep-diff the native-function bridge; explicit canvas sizing with a DPR backing store.
7. **Venue measurement is a project dependency, not a code dependency.** The default venue
   (ARCHITECTURE §OQ4) makes everything testable beforehand, and the graded default speaker
   heights specifically exist so a dropped `z` term cannot pass DSP-01's test vacuously.

### Reference plugins in this repo

| Plugin | What to take |
|---|---|
| `O-Orbit` | `Source/{Data,DSP}/` layout; permissive mono/stereo-in `isBusesLayoutSupported()`; queue-geometry-for-the-audio-thread pattern. **Do not** take the motion engine, VBAP, or the SAF dependency — O-Octagon must not link SAF. |
| `O-ReverseDelay` | `tests/render-harness/` and `tests/ui-stub/` structure — both are needed here, and the harness is built at Stage 2, not Stage 4. |
| `O-Prism` | Dev/release variant-shadowing hazard in the AU registry; `build-and-install.sh` Phase 4 sweeps both. |

---

## References

- Creative brief: `plugins/O-Octagon/.planning/BRIEF.md`
- Requirements: `plugins/O-Octagon/.planning/REQUIREMENTS.md`
- Parameter draft: `plugins/O-Octagon/.planning/parameter-spec-draft.md` *(count discrepancy
  resolved: **17**, see ARCHITECTURE §11)*
- DSP architecture: `plugins/O-Octagon/.planning/research/ARCHITECTURE.md`
- Stage 0 context: `plugins/O-Octagon/.planning/stages/0-ideation/CONTEXT.md`
- Locked architecture: `research/logic-pro-multichannel-octaphonic-dbap.md`
- JUCE multichannel reference: `research/juce8-multichannel-spatial-audio.md`
- Critical patterns: `troubleshooting/patterns/juce8-critical-patterns.md`
- UI mockup: **not yet created** — Room + Venue screens, due before Stage 3.1
