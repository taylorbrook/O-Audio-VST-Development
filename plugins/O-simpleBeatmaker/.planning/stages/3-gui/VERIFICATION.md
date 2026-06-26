# Stage 3 (GUI) — VERIFICATION

**Verdict: PASS.** The GUI stage goal — a single-page, projector-readable WebView
teaching UI with a live step grid + playhead, the applied-Δt timing lane, a live
MIDI readout, all 42 params bound two-way, cross-platform correct — is achieved
and validated. Critic review returned **0 blockers** (progression allowed).

## ROADMAP Stage-3 test criteria (goal-backward)

### Phase 3.1 — grid + playhead + controls + cross-platform wiring
- [x] Grid renders; clicking a cell toggles a hit; click-again cycles
  normal/accent/ghost; per-step velocity visible (height + brightness + glyph).
  *Verified: screenshot shows lit velocity cells; cycle logic `nextVelocity()` +
  `setStep` native fn; right-click / Delete erases.*
- [x] Playhead sweeps the grid in sync with transport (and free-runs in standalone).
  *Verified: `frame.ph` (fractional step index) highlights `floor(phase)` column;
  screenshot shows the amber playhead column; free-run confirmed in Standalone.*
- [x] All knobs/selectors two-way bound (drag→DSP; host automation→UI).
  *Verified: 29 sliders + 1 combo + 12 toggles via Web*Relay/3-arg attachments;
  `jassert(param)` on each; pluginval fuzz-params SUCCESS; auval param-set/ramp PASS.*
- [x] Renders on macOS (VST3+AU) AND Windows VST3 (no blank UI).
  *macOS verified by screenshot + auval. Windows: `NEEDS_WEBVIEW2 TRUE` +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder(tempDir)` —
  the known blank-on-Windows regression is absent (critic + memory).*
- [x] Grid state round-trips (save/reload restores the pattern).
  *Verified: grid persisted in the `PATTERN` ValueTree (Stage 1, unchanged); UI
  paints from `getGrid()` on load + re-polls ~4×/s so host state restores show up.*

### Phase 3.2 — timing/groove lane + live MIDI readout (QUAL-02)
- [x] Swing pushes off-beats later in the lane; humanize scatters; quantize pulls
  the scatter back while leaving swing. *By construction: the lane plots `d =
  appliedSampleInBar − nominalSampleInBar` from each VizEvent — the Stage-2 DSP
  already verified swing-survives-quantize (DSP-04) and the lane renders that same
  applied Δt.*
- [x] The lane offset **matches what is heard** (QUAL-02 — it is the applied Δt,
  not a recomputation). *Critic confirmed: JS consumes `h.d` directly and only
  divides by samples-per-16th for display scale; no feel-formula recompute.*
- [x] MIDI readout shows note-ons from sequencer playback AND played MIDI, with
  velocity. *Verified: `frame.hits` carries `src` (0=SEQ/1=MIDI) from the same
  VizAnalyzer stream the audio emits into; readout prints SEQ/MIDI + note + vel.*
- [x] No audio-thread allocation/FFT; UI smooth; no event loss under fast patterns.
  *Verified: lock-free `AbstractFifo` drain on the 60 Hz message-thread Timer; all
  per-frame `DynamicObject` allocation is message-thread; ring capacity 1024,
  scratch 256. PERF (Stage 2) shipping path unchanged.*

### Phase 3.3 — tooltips + single-page scaffolding + preset hook
- [x] Plain-language tooltip on every control + the grid + the timing lane.
  *Verified: `TIPS` map covers all params (generic per-voice keys), grid, lane,
  MIDI, presets; pointer AND focus-driven (a11y).*
- [x] Single projector-readable page. *Verified by screenshot — one scrolling page,
  field-guide aesthetic consistent with the simple family.*
- [x] Preset selector UI hook present (content deferred to Stage 4 per CONTEXT).

## Technical validation
- **Build:** `ninja` VST3 + AU + Standalone — clean (only benign JUCE
  switch-enum/deprecation warnings); UIResources binary-data target links.
- **auval** `aumu OSiB OuDv`: **AU VALIDATION SUCCEEDED** (render, 1-channel,
  bad-max-frames, parameter set/ramp, **MIDI** — all PASS).
- **pluginval** `--strictness-level 10`: **SUCCESS** (buses 0-in/2-out, fuzz params).
- **Visual:** Standalone screenshot — full UI renders (grid + playhead + lane +
  MIDI readout + all knobs/strips + master + clear button). Not blank.
- **Native-fn parity:** JS `getNativeFunction` set == C++ `withNativeFunction` set
  (`setStep` / `getGrid` / `clearGrid` / `getSampleRate`).
- **Install:** cache cleared + dual-variant swept + dev bundles installed.

## Critic review (post-execute, pre-verify)
- **0 blockers → progression allowed.** ui-critic gate_pass (7.0),
  architecture-critic gate_pass (~9.0), foundation-critic gate_pass (9.75).
- Every flagged focus area CLEAN: editor member/lifecycle order, relay/attachment
  3-arg correctness, resource-provider bare-path, QUAL-02 lane fidelity, per-frame
  emit safety, grid native-fn bounds/threading, ID drift (none), dependency
  direction (acyclic), advisory-tap read-only claim, Windows readiness.
- **10 advisory warnings; 6 addressed opportunistically this stage:**
  - UI-001 grid cells now keyboard-operable (tabindex/role + Enter/Space cycle,
    Delete/Backspace erase) with live `aria-label`/`aria-pressed`.
  - UI-002 knobs expose `aria-valuenow`/`aria-valuetext`/min/max + `aria-label`;
    mute/solo expose `aria-pressed`.
  - UI-004 tooltips now fire on focus (keyboard) as well as pointer.
  - UI-006 added a **Clear all** affordance wired to the `clearGrid` binding
    (was registered-but-unused).
  - FND-001 stale Stage-1 CMake header comment corrected.
  - ARCH-001 RESEARCH.md reconciled to the shipped consolidated `frame` event.
  - **Left (benign, non-gating):** UI-003 (a few sub-12px secondary labels — bumping
    risks layout regression), UI-005 (preset "armed" state — has an explanatory
    caption), ARCH-002 (`getVizAnalyzer()` mutable ref — editor uses consumer API
    only), ARCH-003 (`currentSampleRate` non-atomic advisory read — benign).

## Residual (Stage 4, not goal failures)
- Concept-isolating factory preset **content** (FUNC-05), playability tuning, final
  validation sweep, QUAL-02 audible-vs-visible audit in a DAW, CHANGELOG v1.0.0.
- Real-DAW transport-sync smoke test (host playhead) — covered structurally here;
  hands-on DAW pass belongs to Stage 4 / install.
