# O-Strata - Requirements

---
version: 2.0.0
plugin: O-Strata
created: 2026-09-07
lastUpdated: 2026-09-14 (**Stage 4 Round B verify — the Stage 4 verdict** (`stages/4-polish/VERIFICATION.md`): COMPAT-02 complete (CI run 34809565500 `dcbc609a`, `windows-vst3` green ×3, MSVC + WebView2 VST3, pluginval 10 exit 0 with 0 FAIL lines), PERF-02 Bandlimited item complete (D4 padded evaluator, H7 BL 4.24 % <= 4.13 + 2.0), UI-01 / FUNC-07 WebView2 halves complete at the CONTEXT Finding 6 ceiling with the hands-on rows a **named deferral — owner none, blocked on hardware**, COMPAT-01 held, DSP-06 / FUNC-08 / FUNC-11 / QUAL-03 / UI-04 regressions held; QUAL-04 pending (material complete, Taylor's sitting outstanding), PERF-03 pending both halves. **Every `must` requirement is now complete.** 2026-09-13 (Stage 4 Round A verify — FUNC-08 / FUNC-11 / QUAL-03 complete, UI-04 + COMPAT-01 regressions held; 2026-09-12: Stage 4 discuss — D2 COMPAT-02 evidence = `ci-tests.yml` O-Strata macOS harness + Windows VST3 / pluginval-10 jobs run by manual dispatch (the repository never builds on push); D3 local install only, no `/publish` in Stage 4; D4 vectorised Chebyshev basis taken as the PERF-02 Bandlimited item, the analytic fallback above the muting note and the pitch-tracking soft knee → v1.1; Stage 3 verify — UI-02 / UI-03 / UI-04 complete, UI-01 / FUNC-07 partial at the CONTEXT D2 ceiling (WebView2 halves → Phase 4.2), PERF-03 pending the WKWebView row; Stage 3 Round A verify — UI-04 partial (controls half), FUNC-05 UI list of 46 verified; Stage 2 Round B verify — FUNC-06 / DSP-02 / DSP-04 / QUAL-01 complete, FUNC-06 acceptance amended to the ≤ D_max·K bound on A-notes; 2026-09-11: DSP-01 acceptance amended to the Decision 2 law)
supersedes: superseded-baked-v1/REQUIREMENTS.md (v1.0.0, baked-geometry design)
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 28
**Coverage:** must: 19 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Each oscillator (A, B) is a live wave-terrain oscillator: a closed orbit at the note frequency scans a 2D terrain per sample; θ is the phase accumulator so Sync / Bend / Window warps, FM, unison and Phase apply unchanged | must | complete | stage-2 |
| FUNC-02 | Analytic terrain library ≥ 6 (Sine Product, Radial Rings, Saddle, Ridged Cosines, Mitsuhashi, Cosine Wells), each with Terrain Freq and two documented shape inputs (Mod X / Mod Y), clean-room formulas | must | complete | stage-2 |
| FUNC-03 | Orbit library ≥ 11 (Ellipse, Superellipse, Limaçon, Epitrochoid 3/5/7, Hypocycloid 3/5/7, Butterfly, Squarcle) with Size, Aspect, Rotation, Centre X/Y and a per-orbit shape input (Orbit Mod) | must | complete | stage-2 |
| FUNC-04 | Trajectory feedback: previous output displaces the next orbit point, with Feedback amount and Feedback Damp; bounded for every setting | must | complete | stage-2 |
| FUNC-05 | Audio-rate modulation: Orbit Size (= `osc?Pos`), Aspect, Rotation, Centre X, Centre Y, Orbit Mod, Terrain Freq, Terrain Mod X/Y, Feedback and Saturation are mod-matrix destinations (10 new per oscillator, appended after O-Prism's 26), smoothed per sample — UI half (46 entries in every destination dropdown) verified Stage 3 Round A, 2026-09-12 | must | complete | stage-2 |
| FUNC-06 | Per-oscillator Quality: Bandlimited (Chebyshev, 1×) / 2× / 4× — 2× / 4× half complete (Stage 2 Round A, 2026-09-11), Bandlimited half complete (Round B, 2026-09-12: H6 127 / 127 sounding rows ≤ −90 dB; 17 rows above ≈ A6 with K ≥ 6 orbits on even terrains are muted by the truncation law — documented limit) | must | complete | stage-2 |
| FUNC-07 | User greyscale PNG terrain import via file chooser and drag-and-drop, with Image Blur and Edge Mode; decode and pre-blur off the audio thread — DSP half (import API, blur, edge, off-thread job ≤ 100 ms) verified Stage 2 Round B, 2026-09-12; chooser / drag-and-drop / view are Stage 3 (macOS) — drop path + cap / signature refusals + Imported… selected from C++ verified 2026-09-12 (layout gate drop section; chooser + real-PNG Blur / Edge rows for Taylor); the WebView2 drop half is measured on the Phase 4.2 CI Windows build (Stage 3 CONTEXT D2) — **complete at the CONTEXT Finding 6 ceiling, Stage 4 Round B, 2026-09-14**: MSVC compiles the chooser / drop path and pluginval 10 opens the editor on Windows; the `dataTransfer.files` drop *by hand* on WebView2 is part of the named deferral | must | complete | stage-4 |
| FUNC-08 | Imported PNG persists in plugin state and presets (raw bytes ≤ 2 MB; path + SHA-256 above the cap); presets regenerate on load — bytes half (identical bytes ⇒ identical render SHA-256, H10) verified Stage 2 Round B, 2026-09-12; state child / cap / notice verified Stage 4 Round A, 2026-09-13 (`stages/4-polish/round-a/VERIFICATION.md`: H10 (a)–(e) — bytes-form state round trip render-SHA-identical, path form present / deleted / rewritten, preset `customState` through the file loader + the no-`customState` clear, two ≈ 1.9 MB slots in 64 ms; Logic bytes-form + live-restore rows for Taylor) | must | complete | stage-4 |
| FUNC-09 | All O-Prism v1.24.0 non-oscillator sections (sub/noise, envelopes, dual filters, LFOs, mod matrix, FX rack, global) carry over unchanged | must | complete | stage-2 |
| FUNC-10 | Full microtonal tuning engine (scala-tuning-engine v3.0.1: factory tunings, Scala/KBM import, EDO / harmonic / rank-2 generators, tuning tab) | must | complete | stage-2 |
| FUNC-11 | Factory presets ≥ 15 covering terrains, orbits, feedback and Bandlimited mode, every one passing the symmetry gate (DSP-06) — verified Stage 4 Round A, 2026-09-13: 18 presets, 6 / 6 terrains, 11 / 11 orbits, feedback 3, Bandlimited 3, the five BRIEF use cases; H2 20 / 20 rows at C4 (Coarse / Fine + osc B); on-disk bank stamped `1.0.0+18c17735821c` with the `Init` orphan swept (Squarcle Storm's figure varies run-to-run through the S&H LFO's clock-seeded RNG, −2.2 … −4.0 dB, always inside −6 dB — Round B item) | should | complete | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Pitch-tracked terrain spatial frequency ("terrain mip"): with Pitch Track = 1 the harmonic count of a patch is identical below C4 and does not grow above it (F_eff = F · min(1, C4/f_note)^Track — ARCH Decision 2, scaling downward only); Pitch Track = 0 keeps the spatial frequency fixed (amended 2026-09-11, Stage 2 Round A verify — the "constant across the keyboard" wording was unsatisfiable at F = 4 under the contracted law) | must | complete | stage-2 |
| DSP-02 | Bandlimited mode: terrain in the Chebyshev basis, total degree ≤ 16, coefficients truncated per pitch so the maximum harmonic (n+m)·K stays below Nyquist; coefficient sets computed off the audio thread and swapped atomically; PNG terrains projected onto the basis at import | must | complete | stage-2 |
| DSP-03 | Oversampling is per oscillator (halfband polyphase IIR 2×, 4× HQ), parameters held over sub-samples; the voice loop, filters, LFOs and mod matrix run at base rate | must | complete | stage-2 |
| DSP-04 | Orbit (including feedback displacement) clamped inside [−1,1]²; PNG terrains pre-blurred with Mirror / Window edge handling; every sampled value `isfinite`-guarded — clamp / guard half complete (Round A, 2026-09-11), pre-blur / edge half complete (Round B, 2026-09-12: H11 Mirror / Window ≤ −58 dB, tiling control fails) | must | complete | stage-2 |
| DSP-05 | No `pow`, `std::function` or allocation in the per-sample path; enum `switch` dispatch; fast tanh; `SmoothedValue` per modulated parameter; a Quality change never allocates on the audio thread | must | complete | stage-2 |
| DSP-06 | Symmetry rule: default orbit centre, aspect and every library / factory default produce harmonic 1 as the strongest partial or within 6 dB of it (a centred orbit over an even terrain plays an octave up) | must | complete | stage-2 |
| DSP-07 | Saturation applies tanh to the scanned value before the oscillator output; identity at 0 | should | complete | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Per-oscillator 3D terrain view (displaced 64×64 wireframe + orbit + scan point + feedback trail) with a 30 Hz playhead, hand-rolled WebGL2 — WKWebView in Stage 3; the WebView2 render is verified on the Phase 4.2 CI Windows build (Stage 3 CONTEXT D2) — macOS half verified 2026-09-12 (`stages/3-gui/VERIFICATION.md`: layout gate webgl DPR 1 / 2 + lose / restore, orbit golden 2.9e-6, WKWebView paints the pushed maps; Terrain-tab / Logic hands-on rows open) — **WebView2 half complete at the CONTEXT Finding 6 ceiling, Stage 4 Round B, 2026-09-14**: the CI Windows build compiles the view's TUs under MSVC and pluginval 10 opens the editor (a silently-blank WebView2 would surface as an Editor / Editor Automation failure or timeout; neither occurred); that the view *renders correctly* on Windows is part of the named deferral | should | complete | stage-4 |
| UI-02 | Canvas 2D fallback in the same view when WebGL2 is unavailable; localised placeholder in en/fr/zh-Hans — verified 2026-09-12 (layout gate fallback section in en / fr / zh-Hans, every layout row holding) | must | complete | stage-3 |
| UI-03 | Drag on the view edits Orbit Centre X/Y, wheel edits Orbit Size, alt-drag edits Rotation, through the slider relay with drag-start/drag-end — verified 2026-09-12 (layout gate interaction section: values move, exactly one Started / Ended pair per gesture, A ↔ B repoint; the ⌘Z count in Logic is a human row) | should | complete | stage-3 |
| UI-04 | 3D view and playhead re-push after every preset apply and session restore — controls half (34 parameters bound through relays / proxies, preset apply calls `requestTerrainRepush`) verified Stage 3 Round A, 2026-09-12; view / playhead re-push verified 2026-09-12 (`stateGeneration` bumped by `setStateInformation` + the four preset natives, editor forces all five pushes on the next 30 Hz tick; live preset / session-restore row for Taylor) | must | complete | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations, locks, file I/O or coefficient projection in processBlock) | must | complete | stage-2 |
| PERF-02 | CPU: **oscillator delta** ≤ 12 % of one Apple M-series core — 16 voices × 2 oscillators, unison 1, Quality 2×, default patch, measured as (full render) − (same render with the harness-only terrain-kernel bypass); whole-plugin total reported, not gated; unison capped at 4 for the live oscillator (amended 2026-09-10, Stage 2 discuss D2); **Bandlimited item closed Stage 4 Round B, 2026-09-14 (D4)**: the padded Chebyshev evaluator replaced `clenshaw2D` on the audio thread — `--gate clenshaw` 81.4 ns → 28.0 ns (2.9×) with accuracy contracted at ≤ 2e-5 (random, measured 9.775e-06) and ≤ 1e-6 (projected, 2.980e-07), taking the H7 Bandlimited oscillator delta from ≈ 11.9 % to **4.24 % ≤ the same run's 2× delta 4.13 % + 2.0**; the H7 Bandlimited row is a sound *local* regression guard but is load-flaky under CPU contention (VERIFICATION Issue 1 — a ratio form is a v1.1 item) | must | complete | stage-2 |
| PERF-03 | 3D view costs ≤ 2 ms per frame in WKWebView and WebView2 at DPR 2 — WKWebView measured in Stage 3; the WebView2 figure comes from the Phase 4.2 CI Windows build (Stage 3 CONTEXT D2) — instrumentation + `~/Library/Logs/O-Strata/view-perf.log` path verified 2026-09-12; the WKWebView number itself is open (Stage 3 VERIFICATION human row 1: ⌥-click the Terrain HUD ×3 in the Standalone and Logic) — **Stage 4 Round B, 2026-09-14: still pending on both halves.** `~/Library/Logs/O-Strata/` does not exist; Decision 36's scripted Standalone burst is proven unreachable (four attempts, two mechanisms, 0 AX windows, occluded-WKWebView rAF), so no `gl.finish()` flag was added and `terrain-view.js` is untouched; the WebView2 half is part of the named deferral. Neither the page nor the instrumentation is implicated (shell diff 0 px, layout 607 / 0) | should | pending | stage-4 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) at strictness 10, and auval — re-verified after the re-parameterise pass (the baked fork passed on 2026-09-07) | must | complete | stage-1 |
| COMPAT-02 | Windows VST3 build via CI (WebView2, static linking) — `ci-tests.yml` O-Strata Windows job, manual dispatch, green before the Round B verify (Stage 4 CONTEXT D2); the build + pluginval 10 is the ceiling — WebView2 render / drop / PERF-03 hands-on rows need a Windows machine (CONTEXT Finding 6) — **verified Stage 4 Round B, 2026-09-14** (`stages/4-polish/VERIFICATION.md`): run [34809565500](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/34809565500) at `headSha` `dcbc609a`, job `windows-vst3` `success` with all 13 steps green on three consecutive dispatches — MSVC compiled the plugin TUs, WebView2 linked, the VST3 was produced, and pluginval strictness 10 exited 0 under `bash -e -o pipefail` with 0 `FAIL` lines across every suite including Editor, Open editor whilst processing, Automation, Editor Automation and Fuzz parameters; the WebView2 hands-on halves are a **named deferral — owner none, blocked on hardware** (no Windows machine in the loop; CONTEXT Finding 6) | must | complete | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | Aliasing: every library terrain × orbit at defaults measures ≤ −60 dB non-harmonic energy at C6 at Quality 2× with Pitch Track 1; ≤ −90 dB in Bandlimited mode with a trig-polynomial orbit — 2× half complete (Round A, 2026-09-11: 198 / 198 rows, worst −70.7 dB), Bandlimited half complete (Round B, 2026-09-12: 127 / 127 sounding rows, worst −98.4 dB) | must | complete | stage-2 |
| QUAL-02 | No zipper noise when any mod destination of FUNC-05 is stepped by host automation or the mod matrix (sample-step detector on the rendered output) | must | complete | stage-2 |
| QUAL-03 | Loading a preset whose imported PNG is missing degrades to the library fallback with a visible notice, never silence or a crash — verified Stage 4 Round A, 2026-09-13: a missing or SHA-mismatched source plays Sine Product (H10 (b): rms 0.242, render SHA-equal to a Sine Product instance), `sourceMissing` in the status push, the sticky notice carries Locate… which refuses a hash mismatch (layout gate `locate`); the Locate… hands-on row is Taylor's | nice | complete | stage-4 |
| QUAL-04 | Listening pass on the factory presets and on a terrain × orbit grid rendered by the harness — **material complete and verified Stage 4 Round B, 2026-09-14** (`stages/4-polish/LISTENING.md`: Table A 18 presets × {Logic, Live 12} + 3 Bandlimited chord cells = 39 verdict cells, Table B 66 C4 grid rows + 19 flagged pairs = 85, Table C 15 human rows; the 198-WAV grid is `shasum -c` clean against the LF golden and the 18 post-FX preset renders are 0 silent) — **every verdict cell is still blank: the sign-off is Taylor's sitting** (PLAN Decision 41; defaults changes follow the Decision 38 CHANGE protocol) | nice | pending | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Live oscillator

- [ ] Rendering C4 with Sine Product / Ellipse at defaults gives a periodic waveform at 261.6 Hz ± 1 cent whose spectrum changes when Orbit Size changes, with no re-bake and no table
- [ ] Sync, Bend and Window warps, unison detune/width and Phase produce the same effect on the terrain oscillator as on O-Prism's wavetable oscillator (θ parity)

### FUNC-02 / FUNC-03: Libraries

- [ ] Each analytic terrain's Mod X / Mod Y are documented and audibly change the spectrum; each orbit's Orbit Mod is documented and audibly changes the spectrum (Ellipse: inert, documented)
- [ ] Orbit Size 0.05 → 1.0 over Sine Product gives a monotonically non-decreasing spectral centroid

### FUNC-04: Feedback

- [ ] Grid Feedback × Damp × 6 terrains × 11 orbits at C2/C4/C6: bounded output (|y| ≤ 1), no NaN, no DC runaway; Feedback 0 is bit-identical to the feedback path removed

### FUNC-05: Audio-rate modulation

- [ ] An LFO at 40 Hz on Orbit Centre X produces sidebands at ±40 Hz around each harmonic (proof the destination is per-sample, not per-block)
- [x] `modSlot?Dst` lists 46 entries: O-Prism's 26 followed by the 10 new per oscillator — layout gate 46 options × 16 selects, Stage 3 Round A (2026-09-12)

### FUNC-06 / DSP-02 / DSP-03: Quality

- [x] Bandlimited mode on every exact orbit (Ellipse, Limaçon, Epitrochoid 3 / 5 / 7, Hypocycloid 3 / 5 / 7) × 6 terrains at A2 / A4 / A6 (exact-cycle): the highest harmonic above −100 dB is ≤ D_max · K (equality where the terrain has content on the cut diagonal — 13 / 127) and non-harmonic energy ≤ −90 dB on every sounding row; rows the truncation law mutes (D_max = 1 on an even terrain: 17 at A6 with K ≥ 6, or K = 4 on Radial Rings) are reported as the documented limit — H6 Bandlimited, 2026-09-12 (`stages/2-dsp/VERIFICATION.md`; amended from "equals the truncation bound at C2 / C4 / C6": equality is unsatisfiable where the terrain's own spectrum ends below the bound, and the harness measures A-notes for exact cycles)
- [x] Switching Quality while notes are held does not allocate (debug allocator) and does not click beyond the crossfade — H8 Quality row 0 allocations, `crossfade` ≤ plateau, storm click ratio ≤ 1.16 (2026-09-12)
- [x] Oversampling latency reported through `setLatencySamples` from `prepareToPlay` — `latency` 1 / 4 on all 9 Quality pairs (2026-09-11 / 12)

### FUNC-07 / FUNC-08: PNG import and persistence

- [ ] PNG loads via chooser and via drag-and-drop on macOS and Windows; Blur and Edge Mode are audible and visible in the 3D view
- [x] Save/reload a preset with an embedded PNG: identical rendered output (SHA-256 of a 1 s render matches); above the cap: path + SHA; missing file → fallback + notice (QUAL-03) — H10 (a) / (b) / (c), Stage 4 Round A (2026-09-13); the chooser / drop / Blur / Edge bullet above stays Stage 3's

### FUNC-09 / FUNC-10: Inherited sections

- [ ] Parameter IDs for sub/noise, envelope, filter, LFO, mod matrix, FX, tuning and global sections match O-Prism v1.24.0
- [ ] The tuning tab loads a Scala file and a 31-EDO generator; rendered fundamental equals the TuningEngine frequency within 0.5 cent on ≥ 5 keys

### DSP-01: Pitch tracking

- [x] With Pitch Track 1, the number of partials above −40 dB at C2 and C4 differs by ≤ 2 (shared F_eff below the knee) and the count at C6 does not exceed the C4 count, at F = 1 and F = 4; the tracked C6 count is strictly below the Pitch Track 0 count; with Pitch Track 0 at F = 8, non-harmonic energy at C6 / C8 rises ≥ 20 dB (negative control) — H4, 2026-09-11 (`stages/2-dsp/round-a/VERIFICATION.md`; amended from "C2 / C4 / C6 differ by ≤ 2": measured 5 / 5 / 2 at F = 1 and 13 / 12 / 5 at F = 4, the F = 4 form is unsatisfiable under Decision 2)

### DSP-05 / PERF-01 / PERF-02: Real-time

- [x] pluginval strictness 10 passes; zero allocations in processBlock under a debug allocator across all Quality settings — pluginval ×2 SUCCESS; H8 rows (terrain / orbit, Quality, storm, image) 0 counted (2026-09-12)
- [x] CPU harness (H7): 16 voices × 2 osc, unison 1, 2×, default patch, 48 kHz, 10 s, Release, best of 3 → oscillator delta = (wall ÷ audio) − (wall ÷ audio with `terrainKernelBypass`) ≤ 12 %; total, baseline and delta printed; unison 4 / 4× / Bandlimited deltas reported — 4.19 / 4.38 % (2026-09-12); Bandlimited row ≈ 12 % reported (Stage 4 PERF item: vectorised basis evaluator)

### DSP-06: Symmetry gate

- [x] For every library terrain × orbit at defaults and every factory preset: harmonic 1 is the strongest partial or within 6 dB in ≥ 95 % of 1 s at C4; negative control: Centre (0,0), Aspect 1 over Sine Product fails the gate (octave up) — library grid 66 / 66 (Stage 2); factory presets 20 / 20 rows, negative control −109 dB (Stage 4 Round A, 2026-09-13)

### UI-01 through UI-04: 3D view

- [ ] WebGL2 view renders in the Standalone, in Logic (WKWebView) and in a Windows host (WebView2); forcing `getContext('webgl2')` to null shows the Canvas 2D fallback and the localised placeholder in all three languages; `WEBGL_lose_context` recovers — fallback ×3 languages + lose / restore: layout gate 2026-09-12; WKWebView paints the pushed maps (Standalone capture, 2026-09-12); Terrain-tab Standalone / Logic renders = human rows, WebView2 = Phase 4.2
- [ ] Playhead advances with θ at 30 Hz; the feedback trail is visible when Feedback > 0; unchanged state sends no event — change gates in `pushState` / `pushCycle` and the `fb > 0.001` trail gate verified in code 2026-09-12; the held-note row is human
- [ ] Dragging the view moves Orbit Centre X/Y through the relay with drag-start/drag-end (one host undo step); after a preset apply the view shows the new terrain within one push interval — one relay pair per gesture: layout gate interaction section 2026-09-12; preset re-push path code-verified (`stateGeneration`); the Logic ⌘Z count and the live preset row are human

### QUAL-01 / QUAL-02: Artifacts

- [x] Offline render harness sweeps every library terrain × orbit at A2, A4, A6 (exact-cycle) at 2× with Pitch Track 1: non-harmonic energy ≤ −60 dB, no NaN — 198 / 198, worst −70.7 dB (2026-09-11 / 12); Bandlimited 127 / 127 sounding rows ≤ −90 dB (2026-09-12)
- [x] Stepping each FUNC-05 destination by 10 % every 100 ms at C4: excess step ratio ≤ 1.5 vs the adjacent plateaus (the absolute 0.1 literal is gain-staging dependent, Round A) — worst 1.08 (2026-09-11 / 12)

### COMPAT-01 / COMPAT-02: Validation

- [x] pluginval strictness 10 VST3 + AU pass; auval pass — after the re-parameterise pass (2026-09-10, `stages/1-foundation/VERIFICATION.md`)
- [ ] CI Windows build green; WebView2 static linking and `withUserDataFolder()` in place

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 (re-parameterise pass: 205 parameters, 46 mod destinations) |
| stage-2 | FUNC-01..06, FUNC-09, FUNC-10, DSP-*, PERF-01, PERF-02, QUAL-01, QUAL-02 |
| stage-3 | FUNC-07, UI-*, PERF-03 |
| stage-4 | FUNC-08, FUNC-11, COMPAT-02, QUAL-03, QUAL-04, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Baked geometry sources (mesh slicing, volume orbits, terrain sweeps → wavetable) | Ships as an O-Prism factory bank; full design preserved in `superseded-baked-v1/` | v1.1 |
| Bandlimited F-lattice (13 coefficient sets over Terrain Freq, per-voice block-rate lerp) | Keeps Phase 2.4 bounded; single set per oscillator, Terrain Freq clamped ≤ 2 in this mode (Stage 2 discuss D4) | v1.1 |
| Wavetable oscillator mode | O-Prism's job | none |
| RGB-channel terrain morphing | Extension of PNG import | v1.1 |
| Dual displaced-orbit stereo | Per-voice CPU doubling | v1.x |
| Scanned synthesis | Stability + block-rate simulation | v1.2+ |
| ADAA for terrains | Unsound for composite paths | none |
| Analytic 2× fallback above the Bandlimited muting note (17 A6 rows, K ≥ 6 on even terrains) | Re-opens the taper-law contract and QUAL-01's −90 dB above the knee; documented limit in CHANGELOG + the `silent above <note>` readout (Stage 4 discuss D4) | v1.1 |
| Pitch-tracking soft knee (replace the hard min(1, C4/f)) | Re-opens ARCH Decision 2 + DSP-01; every default patch changes above C4 (Stage 4 discuss D4) | v1.1 |
| three.js / lit rendering | No bundler; ESM-only | v1.x |

---
*Regenerated from BRIEF.md on 2026-09-08 (re-plan)*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
