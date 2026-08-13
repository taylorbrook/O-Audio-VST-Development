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
| Stage 4 — Validation | COMPAT-02, all remaining | 4.1, 4.2 *(amended 2026-08-12, D1 — was "single pass")* |

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

- 8 live per-speaker level indicators at their plan positions — atomic array, ~30 Hz read
  **by a fixed-interval JS pull with a deadline-released in-flight guard** (*not* a
  `juce::Timer`), `requestAnimationFrame` ballistics, attack 0.5 / decay 0.12, −60..0 dBFS
  scale, 1.5 s peak-hold releasing at 20 dB/s (critical-patterns §20)

  > **Amended 2026-08-12 at the Phase 3.3 plan boundary (P70), same re-pin as the gradient
  > bullet below.** "~30 Hz Timer read" names a mechanism this plugin does not use, and
  > honouring it literally would undo Phase 3.1's deliberate choice to keep `PluginEditor`
  > `Timer`-free — which is what lets `tests/ui-stub/` render the whole UI and makes the
  > pre-integration half of every layout gate possible. `ARCHITECTURE.md` §4.3 carried the
  > identical error and was corrected at the 3.3 **discuss** boundary (amendment 2); this
  > bullet is the same error in the second document. The **rate** was always right; only the
  > mechanism was wrong. The deadline clause is RESEARCH-3.3 N9, measured: a guard released
  > only on settlement latches permanently the first time a completion is dropped.
- Scene buttons `ALL` `FRONT` `REAR` `LEFT` `RIGHT` `SIDES` + 4 user slots — each writes all
  8 weight parameters via `setValueNotifyingHost` so scenes record as ordinary automation
- DBAP level-field gradient backdrop (paper figs 1-3): per-pixel **`1/k = √denom`, the
  un-normalised DBAP field** over a coarse grid, recomputed on the message thread on
  geometry/weight change, drawn to an offscreen canvas and blitted — **never** recomputed
  per frame

  > **Amended 2026-08-12 at the Phase 3.3 plan boundary (P70).** This bullet originally
  > specified `max_i v_i²`. That formula is **degenerate and was measured so** against the
  > shipping `DbapSolver` over the default envelope (RESEARCH-3.3 N10): DBAP normalises to
  > `Σ v_i² = 1`, so `max_i v_i²` measures only how *concentrated* the image is — it reads
  > **identically 1.0000 at every point in the room** whenever exactly one weight is
  > non-zero, and 3.2–5.4 dB otherwise. The picture goes blank precisely when the spatial
  > situation is most extreme. `1/k` is the same quantity the solver **already computes**
  > as `denom` before normalising, gives 1.3–10.4 dB with correct radial structure, and
  > never degenerates. Reached through a defaulted `float* outInvK = nullptr` out-param, so
  > no call site changes and `powCalls == 16` is untouched. REQUIREMENTS.md's four UI-04
  > criteria never named a formula and are unaffected; this bullet was the only place one
  > was named.
  >
  > **The field over a real raked audience plane is genuinely flat** — every grid point sits
  > at `z = 0` while the speakers are 4.50–5.40 m up, so the minimum 3-D distance is ≥ 4.5 m
  > in a 12 × 15 m hall. That is a property of the rig, not of either formula. The backdrop
  > therefore normalises to the **per-recompute observed min/max** and prints the actual dB
  > span in a legend; an absolute 0..1 colour map renders a uniform wash while looking as
  > though it carries information.
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

## Stage 4 — Validation (phases 4.1, 4.2)

> **Amended 2026-08-12 at the Stage 4 discuss boundary.** Two changes, D1 and D2.
>
> **D1 — the pass is split, not single.** Every criterion below is either closable at the desk with
> no host and no ear, or needs a running Logic session and a human. Running them as one pass means
> planning the human session before the machine gates have frozen the binary it would be run
> against. **4.1 = machine. 4.2 = host-and-ear, against a frozen 4.1 binary.**
>
> **D2 — the goal line "in the hall" is corrected.** No hall is available this milestone; the rig is
> **Logic + an 8-channel interface at the desk**. This costs less than it reads: every criterion
> below closes at the desk. `D5`'s *only* unique coverage is QUAL-01 criterion 2's **audible**
> clause, whose open question — *does ~15 % of an 8 kHz component, as a one-sample step on a single
> hull-crossing gesture, tick audibly on HF-rich material?* — is a **one-gesture, any-monitoring**
> judgement that needs no room. What a hall would add is spatial-coherence judgement, which **no
> requirement row asks for**. Stated so it is a decision and not an omission.
>
> **D11 — amended again 2026-08-13 at the 4.2 discuss boundary. D2's rig does not exist.** No
> physical 8-out interface is attached: the devices present are BlackHole 2ch, **BlackHole 64ch**,
> MacBook Pro Speakers (2), the built-in mic, Teams and Zoom, with no aggregate configured. D2 leaned
> on the interface **twice** — for verify-ping's "8 physical outputs" and for the audible clause's
> "any-monitoring" judgement — and neither holds as written. *(Surfaced by this stage's own parting
> rule: when an amendment corrects a claim, grep the other contracts for the same claim. It fired at
> D7 one boundary ago and it fires here.)*
>
> The rig is **Logic Pro 12.3 + BlackHole 64ch**, and the two leans are discharged differently:
>
> - **Verify-ping** closes against the **CoreAudio device boundary**, evidenced by per-channel
>   capture and script analysis — Logic's surround assignment, I/O routing and the driver boundary
>   are all exercised, and capture is *stronger* evidence than eight moving meters. The residual is
>   **one specific hardware driver**, recorded with **owner: none**, because it is a property of a
>   piece of hardware and would not generalise across interfaces even if one were present.
>   **The criterion below keeps the word "physical".** It is not edited to fit what the rig can
>   prove — this project has been caught three times by a check that stopped looking at what it
>   claimed to look at, and the fix is to record the scope, never to re-word the criterion.
> - **The audible clause** runs off an **offline bounce on headphones**, which is valid because
>   QUAL-03 proved block-size invariance and 4.1 proved the binary bit-reproducible: the bounce holds
>   the same samples realtime would. Two halves, not interchangeable — a soloed difference signal is
>   a **locator** (soloing removes masking); the full bounce in context is **the requirement**.
>
> **Bounce-order and LFE-gain need no device to run** — the bounce is offline. BlackHole 64ch is
> required only to make Logic offer a 7.1 output at all, which is 4.2 research question Q1 and gates
> the phase. See `stages/4-polish/CONTEXT-4.2.md` D11–D13.

**Goal:** Logic Pro 12.3 on BlackHole 64ch, with the bounce path confirmed.
*(Goal line amended 2026-08-13, D11 — was "an 8-channel desk rig".)*

### Phase 4.1 — machine gates (no host, no ear)

- [ ] **CI wiring** — the 44 unit + 48 harness C++ probes are **built and run in a new secretless
      `.github/workflows/ci-tests.yml`** on macOS, on push and pull_request. *(Added 2026-08-12, D6.
      Carried as a residual since Phase 2.1 verify and widened by every phase since, but **never
      written into this list** — a residual nobody owned. The two JS gates stay local-only by
      decision, not by omission: they are DPR/viewport-sensitive and CI-flaky in this repo.
      O-Octagon is the **first plugin in this repo to run a test target in CI at all**.)*
      *(**Destination amended 2026-08-12 at the 4.1 plan boundary, A1/P86.** This bullet previously
      named `build-and-release.yml`, which `plugins/O-Octagon/CMakeLists.txt:172-176` — PLAN-2.1 P13
      — explicitly forbids: it is tag-triggered and secrets-bearing. Two live contracts contradicted
      each other one day after this bullet was written. A new secretless workflow honours P13
      literally **and** delivers D6's stated intent better than the original destination could: the
      failure mode is "a JUCE bump ships silently", and a JUCE bump is a **commit**, not a tag.
      P13 is not overturned; it is satisfied. The CMakeLists comment is corrected in the same
      commit — A2/P86, because it says the gap "is logged as a repo-level todo instead" and after
      this it is not.)*
- [ ] **The JUCE version is a single derived source** — `.github/juce-version.txt`, read by both
      workflows. A second literal in a second workflow would drift **silently in the one direction
      that matters** (bump the release workflow, and the probe workflow keeps proving the old JUCE
      green — a gate reporting green about a JUCE nobody ships).
      *(Added 2026-08-12 at the 4.1 plan boundary, P87 — `pattern_test_fixture_mirrors_drift_silently`)*
- [ ] Windows CI: VST3 builds under MSVC and passes pluginval 10, **in `ci-tests.yml`**, not by
      dispatching the signing workflow. *(Amended 2026-08-12 at the 4.1 plan boundary, P88. Two
      changes. **(a)** The pre-CI scan this bullet asked for is **discharged** — RESEARCH-4.1 N4
      found **zero** non-static `constexpr` in any lambda in `Source/**` and **zero** occurrences of
      `SafePointer` anywhere in `Source/`, so neither named pattern can fire and the bullet must
      stop describing the Windows risk as those two. The real risk is that **no MSVC has ever parsed
      this code**; a static scan is not a compile, and only a CI run answers it. **(b)** The
      existing `workflow_dispatch --validate_only` path would work, but `build-macos` has no
      `validate_only` guard — deliberately, it doubles as the signing-secrets gate — so dispatching
      it **signs and notarises**, which **D4 excludes**. Putting the Windows job in the secretless
      workflow keeps D4 true as written.)*
- [ ] pluginval strictness 10, VST3 and AU, **run 2-3 times** before publishing
- [ ] `auval -v aufx OuOc <manufacturer>` passes
- [ ] Factory musical presets authored in **engineering units** with `convertTo0to1` — **5–6, on the
      room-character axis only** (`width`, `rolloff`, `blur`, `hullAtten`, `airAmount`,
      `outputGain`). *(Scoped 2026-08-12, D5: position is per-cue automation and the 8 weights are
      already FUNC-06's scenes; a preset that wrote either would put two mechanisms on the same
      parameters.)*
      *(**Mechanism amended 2026-08-12 at the 4.1 plan boundary, A4/P92.** D5's intent survives; its
      premise does not. `applyPresetJson` **resets all 17 parameters to their defaults before
      applying** (module WR-01, `OuariconPresetManager.h:315-331`), so **omitting a key does not
      leave that parameter alone — it resets it**: `srcX`/`srcY` → 0.5, `srcZ` → 0 m, `w1..w8` → 1.0.
      Loading any factory preset would therefore un-do whatever FUNC-06 scene is applied and
      re-centre the source — precisely the two-mechanism collision D5 exists to prevent, arriving
      through the module's defensive behaviour instead of the preset's content. **The scope is
      achieved by a snapshot-and-restore at O-Octagon's own call site**, inside the 17-parameter
      gesture bracket already open there — never by editing the shared module, which nine plugins
      include. Its probe must assert the **eleven are bit-unchanged**, not merely that the six
      moved: asserting only the six passes with the bug present.)*
- [ ] **`COMPAT-04` criterion 3** — the SAFE banner asserted through the **complement predicate**,
      with a negative control on a fourth 8-channel set confirming the banner **is raised** — which
      is what discriminates the complement spelling from `== mono || == stereo`, under which it
      would stay down. *(**Wording corrected 2026-08-12 at the 4.1 plan boundary, A3/P90.** This
      read "confirms the banner **stays down**", which is the outcome of the spelling D8 rejects:
      under the shipped complement form a fourth container is **not** one of the three real rigs, so
      `safeMode` is **true** and the banner goes **up**. Verified as written, the criterion would
      have passed **only if the code had the spelling D8 rejects** — an acceptance criterion that
      fails against correct code and passes against the defect. Same shape as D7, one boundary
      later, in a criterion authored specifically to be un-vacuous. The identical sentence in
      `REQUIREMENTS.md` is corrected in the same edit, and a **third** site — the shipped comment at
      `PluginProcessor.cpp:229-232`, which states the same direction backwards — is corrected in
      code at Task 3.)*
      *(Added 2026-08-12, D8. Criteria were derived at the 4.1 discuss boundary — the row had none,
      owed since Phase 2.2 verify. Criteria 1 and 2 carry real stage-1 evidence and are ticked;
      **criterion 3 covers `safeMode`, which landed at Phase 3.1 — two stages after the row was
      closed.** Criterion 2 also gains a render clause stage-1 had no DSP to exercise.)*
- [ ] CHANGELOG.md *(does not yet exist)*, NOTES.md, PLUGINS.md updated
- [ ] Installed to system plugin folders via `build-and-install.sh`, **dual-variant sweep confirmed**

### Phase 4.2 — host-and-ear gates (frozen 4.1 binary)

- [ ] Instantiates in Logic Pro on a surround track with 7.1 output
- [ ] **Read the negotiated 8-channel set off the Venue screen and record it.** *(Amended
      2026-08-12 at the Stage 4 discuss boundary, D7. This bullet previously read "ARCHITECTURE
      §3.2.2 predicts this may be 7.1-SDDS … risk R2 and Stage 4 is where it is settled." **R2 was
      already settled at Phase 2.1 by observation — Logic negotiated plain `create7point1()`** — and
      the retirement was recorded only in `REQUIREMENTS.md`. Verifying this bullet as written would
      have re-derived a settled fact against a stale premise.)* Stage 4 **confirms on the shipping
      binary**, whose `isBusesLayoutSupported()` is byte-identical to the commit the observation was
      made at, and additionally checks **stability across session recall**, which 2.1 could not.
- [ ] Verify-ping confirms all 8 outputs reach distinct physical channels *(**scope stated
      2026-08-13, D11** — closes against the CoreAudio device boundary by per-channel capture;
      residual "one specific hardware driver", **owner: none**. The wording is deliberately
      unchanged; see the D11 block above for why a criterion is never re-worded to fit its rig.)*
- [ ] Automation of `srcX`/`srcY`/`srcZ` and `w1..w8` is visible and writable in Logic's
      automation lanes
- [ ] **Bounce-order test** (`research/logic-pro-multichannel-octaphonic-dbap.md` §6a test 1):
      bounce a 7.1 project with a distinct tone in each of the 8 slots, interleaved; read off
      the order; confirm the identity label map gives *channel N = speaker N*
      *(**Split into a PAIR 2026-08-13 at the 4.2 discuss boundary, D20 — as written it passes
      vacuously.** The plugin says so in its own source: `VenueModel.cpp:87-89` — "because this
      default is the identity under all three accepted 8-channel containers, a channel-map test
      driven by it alone is **VACUOUS** — a hardcoded 0..7 map would pass it (RESEARCH-2.1 C1/G5).
      Every map probe must drive a **NON-IDENTITY** assignment." This bullet drives exactly that
      default, so run alone it confirms Logic's canonical bounce order and says **nothing** about
      the label map, while reading as though it confirmed both. **CR-a** = identity map → proves
      the §6 MEDIUM-confidence canonical-order claim. **CR-b** = a non-identity permutation →
      proves the label map is what determines bounce order. Source material must be **eight
      distinct tones**; eight copies of one tone makes CR-b unreadable.)*
- [ ] **LFE-gain test** (§6a test 2): bounce identical −20 dBFS tone into the LFE slot and one
      other slot; compare levels. Any delta means Logic is touching the LFE path and speaker 4
      needs a compensating default trim
      *(**Widened 2026-08-13, D15 — the test as written closes half of the claim it checks.**
      `Source/Data/VenueModel.cpp:84` asserts **as fact** that "Logic applies no automatic bass
      management or LFE low-pass", while §6 of the locked research doc rates that **MEDIUM-LOW** and
      says plainly it is "absence of evidence, not proof". A single −20 dBFS tone catches a **gain
      offset** and is **blind to a low-pass** unless it happens to sit above the crossover. Widened
      to a multi-tone or log sweep into the LFE slot and a reference slot, compared **per band**:
      a broadband delta catches the +10 dB offset, HF-band attenuation catches bass management,
      and both ≈ 0 lets the `:84` comment cite a measurement instead of an assumption. **If it
      fails: fix, re-freeze, re-run 4.1's 18 gates** — D16. The identity map is not the lever;
      moving speaker 4 off the LFE slot breaks *channel N = speaker N*, which §6a calls the entire
      reason for the default.)*
- [ ] **Gate 13's interactive half** — ~15 min Standalone launch-and-drive. The static half is fully
      discharged across 3.1 / 3.2 / 3.3 in real WKWebView; every remaining item needs synthetic
      clicks this environment cannot deliver (`-25208`) *(added 2026-08-12, D9 — carried from 3.3
      verify)*
- [ ] **Q5 — a 30 Hz meter poll against a HIDDEN WKWebView.** Unrun by **four consecutive phases**.
      The JS half is measured (3.2's N9), the JUCE drop is read from source, the two together have
      never been executed. Needs a human with **signal running**, which the 8-channel desk rig now
      supplies; `js/meters.js` exposes a `dropped` counter for exactly this *(added 2026-08-12, D9)*
- [ ] **D5 / QUAL-01 criterion 2's audible clause** — one hull-crossing gesture on **HF-rich**
      material, listening for the bounded ~15 %-of-8 kHz one-sample step. The lever if it ticks is
      RESEARCH-2.3 H3 (raising `fc(d_hull = 0)` toward Nyquist), which re-tunes the whole musical
      curve and is therefore a **discuss-boundary change, not a fix** *(added 2026-08-12, D2)*
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
2. ~~**Logic may negotiate 7.1-SDDS, not 7.1**~~ (`kAudioChannelLayoutTag_Emagic_Default_7_1`).
   **RETIRED by observation at Phase 2.1** — Logic negotiated plain `create7point1()`, all 8
   surround-meter lanes moved *(amended 2026-08-12 at the Stage 4 discuss boundary, D7; see
   ARCHITECTURE §3.2.2)*. The mitigation — accepting all three 8-channel containers and keying the
   label map on `ChannelType` — **stays shipped**, because it is what makes one observation safe to
   rely on. Stage 4 confirms rather than settles.
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
