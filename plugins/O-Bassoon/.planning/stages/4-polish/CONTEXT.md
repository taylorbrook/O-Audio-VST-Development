# Stage 4: Polish — Context

## Discussion Summary

**Date:** 2026-05-01
**Participants:** User, Claude
**Stage:** 4 of 4 (Polish / Validation / v1.0.0 Release)
**Cycle scope:** Single pass (one Gate 6 PASS → one atomic commit → v1.0.0)

---

## Inputs Carried Forward

- **Phase 2.4 atomic commit:** ✅ landed at `dcc442c` on `main` 2026-04-29 (`feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PARTIAL (DSP-05 v1.1 candidate)`).
- **Stage 3 atomic commit:** ⏸️ **UNLANDED** at discuss-phase open. Locked subject `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`. **Process invariant: Stage 3 commit MUST land on `main` BEFORE Stage 4 execute-phase begins.** PLAN-rev-1 task #1 will be the hard gate.
- **Stage 3 verify ✅ AUTO-PASS** — 12/12 static checks + auval + pluginval-10 macOS all PASS at `~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` + `Components/O-Bassoon-dev.component`. Manual T9 (Phase 3.1 8-item Logic-AU smoke) + T17 (Gate 5 8-item Logic-AU full incl. 60s long-tone + push-channel audition) ⏸️ PENDING USER — carry forward to Stage 4 Gate 6 as items #1–#2.
- **Phase 2.4 verify ⚠️ PARTIAL rev-5** — DSP-05 morph "v1.1 candidate" (out of v1.0 scope per `should` priority + ROADMAP rev-3 ceiling burn); DSP-06 (NE end-to-end + MPE end-to-end) deferred to Stage 4 per OQ#10-rev-4 fallback. Stage 4 closes DSP-06 fully via Logic-AU MPE + Dorico Playback Template parity.
- **Outstanding requirements going into Stage 4** (REQUIREMENTS.md v1.0.5):
  - **COMPAT-01** partial (strictness-5 PASS at Stage 1, strictness-10 PASS at Stage 3 macOS only) → must reach **complete** (strictness-10 macOS AU+VST3 + Windows VST3)
  - **COMPAT-02** pending → must reach **complete** (Dorico Playback Template + microtonal score parity vs O-Lyrica)
  - **DSP-06** partial → must reach **complete** (end-to-end DAW: Dorico NE + Logic-MPE)

---

## Cycle Scope (Stage 4 Single Pass)

**4 deliverable buckets, single execute-phase:**

1. **pluginval strictness 10** — macOS AU + VST3 (already PASS at Stage 3 verify, re-confirm post-Stage-3-commit) + Windows VST3 (NEW — built from Windows machine or VM)
2. **Dorico microtonal parity** — Playback Template ingestion + microtonal test score plays at correct pitches; A/B vs O-Lyrica baseline
3. **Logic-AU full smoke** — T9 + T17 from Stage 3 carry-forward (8 + 8 items) + DSP-06 MPE close via Logic Pro 10.4+ MPE mode
4. **Release packaging** — 4 factory presets, CHANGELOG v1.0.0 entry, PLUGINS.md status update, Windows VST3 build artefact + install instructions

**Out of scope (carry to v1.0.1 / v1.1):**
- DSP-05 attack-character morph v1.1 architectural pivot (NoiseExciter onset gate ramp 0→1)
- Preset browser UI
- PKG installer (`/package`)
- GitHub release / CI publish (`/publish`)
- Bitwig MPE host verification (Logic-MPE substitutes)

---

## Approach Decisions (User-Confirmed — 2 AskUserQuestion batches × 4 questions)

### Batch 1: scope + matrix + Win + presets

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D1 | **Stage 4 cycle structure** | Single polish pass | All items independent, low-risk; family precedent for v1.0 polish stages. |
| D2 | **Test matrix (Gate 6 hosts)** | Logic-AU smoke (60s) + Dorico microtonal parity | Closes T9 + T17 carry-forward, COMPAT-02, and DSP-06 NE-half. Bitwig dropped — MPE-half re-routed via Logic-MPE. |
| D3 | **Windows VST3 build** | Required for v1.0 (cross-platform release) | Closes COMPAT-01 fully at v1.0; matches "shipped" status target. Windows machine/VM available. |
| D4 | **Preset count + distribution** | 4 ROADMAP presets + internal install only | Long Drone / Microtonal Pad / Tongued Long Tone / Bright Bassoon. `/install-plugin O-Bassoon` to system folders; no PKG, no GitHub release at v1.0. |

### Batch 2: DSP-06 MPE close + iteration cap + commit + Stage 3 timing

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D5 | **DSP-06 MPE-half closure** | Logic Pro MPE mode | Logic 10.4+ supports MPE; routes per-channel pitch-bend to per-voice via existing `juce::Synthesiser::handlePitchWheel` path (Stage 0 D3 lock + Phase 2.4 OQ#7-rev-4). No new tool dependency. |
| D6 | **Inline iteration ceiling** | rev-3 (family precedent) | Matches Phase 2.1–2.4 cap. Polish stage low-risk; rev-3 absorbs preset-tuning + Win-build + Dorico-template tweaks. |
| D7 | **Atomic commit pattern** | Single Stage 4 commit on Gate 6 PASS | Subject locked: `feat(O-Bassoon): Stage 4 polish + v1.0.0 release - COMPAT-01/02 + DSP-06 PASS`. Bundles pluginval-10 + Win build + Dorico + Logic smoke + presets + CHANGELOG + PLUGINS.md. |
| D8 | **Stage 3 commit timing** | Land BEFORE Stage 4 execute-phase begins | Family invariant per Phase 2.4 precedent. PLAN-rev-1 task #1 = hard gate on Stage 3 commit landing on `main`. User trigger required (`commit it` / `land it`). |

### Derived Decisions (carried from inputs / locked at discuss)

| # | Decision | Source |
|---|----------|--------|
| D9 | **Gate 6 PASS bar = 10 items** | 1 Logic T9 smoke (8-item subset → roll-up "Phase 3.1 smoke OK") + 1 Logic T17 Gate 5 (8-item → roll-up "Stage 3 polish OK") + 1 pluginval-10 macOS AU+VST3 + 1 pluginval-10 Win VST3 + 1 Dorico Playback Template ingestion + 1 Dorico microtonal score parity (vs O-Lyrica baseline) + 1 Logic-MPE per-channel pitch-bend per-voice (DSP-06 MPE-half) + 1 4-preset recall round-trip (save/load all 4 in Logic) + 1 CHANGELOG v1.0.0 entry written + 1 Windows install verification (FL or Reaper or Ableton-Win plugin scanner). |
| D10 | **DSP-05 v1.1 candidate stays out of v1.0** | Phase 2.4 verify rev-5 closure; `should` priority. Documented in CHANGELOG "Known Limitations" + STATUS Next Steps item 7. |
| D11 | **Dorico Playback Template source** | `modules/tuning/note-expression/v1.1.0` ships the canonical `.dorico_pt`. Per project memory `critical_dorico_distribution_mechanism.md` — Dorico does NOT auto-ingest standalone `.doricoexpmap` drops; must ship Playback Template OR `.doricolib`. Use `note-expression` module v1.1.0 template (production-validated by O-Lyrica 2.3.0). |
| D12 | **Reference baseline for Dorico parity** | O-Lyrica microtonal score from `spike-findings-VST-development` (`generalize-microtones` skill). A/B same score on O-Lyrica + O-Bassoon — quarter-sharp / quarter-flat / 1/6-tone notes; pitch should track audibly identical. |
| D13 | **Windows build path** | `cmake --build build --config Release --target O-Bassoon_VST3 --parallel` per CLAUDE.md Windows protocol. Use `.\scripts\build-and-install.ps1 O-Bassoon` if available in repo. |
| D14 | **Windows pluginval invocation** | `pluginval.exe --strictness-level 10 --validate "$env:COMMONPROGRAMFILES\VST3\O-Bassoon-dev.vst3"`. Family-precedent: O-AnalogEQ + O-Wind + O-MicrotonalSampler all clear strictness-10 on Windows. |
| D15 | **Preset format** | `juce::AudioProcessor::getStateInformation` / `setStateInformation` round-trip XML (APVTS state + 1 KB tuning state). Save via `Save Preset…` in Logic (.aupreset for AU, FXP/FXB for VST3); commit binaries to `plugins/O-Bassoon/presets/` directory. |
| D16 | **CHANGELOG format** | Standard Ouaricon-family `CHANGELOG.md` (Keep-a-Changelog flavor): `## [1.0.0] - 2026-05-01` heading + `### Added` (modal voice, 4 expression systems, 10 params, NE/MPE, WebView UI, Tuning panel) + `### Known Limitations` (DSP-05 morph subtle, presets minimal, no preset browser). Reference Phase 2.1–Stage 3 commit shas. |
| D17 | **PLUGINS.md update** | Move O-Bassoon row from "🚧 In progress / Stage 3" → "✅ Working v1.0.0" or "📦 Installed". Confirm exact PLUGINS.md schema at plan-phase. |
| D18 | **REQUIREMENTS.md updates locked at verify-phase** | COMPAT-01 partial → **complete**; COMPAT-02 pending → **complete**; DSP-06 partial → **complete** (closes both NE-half via Dorico parity + MPE-half via Logic-MPE). DSP-05 stays **partial** (v1.1 candidate, documented limitation). |

---

## Open Questions (handed to research-phase)

| # | Question | Why it matters |
|---|----------|----------------|
| OQ1 | **Dorico Playback Template ingestion path** — exact .dorico_pt file location in `modules/tuning/note-expression/v1.1.0`, plus how Dorico consumes it (drag-drop into `Playback Templates` folder? Library Manager import? Per project memory the mechanism is non-obvious and standalone .doricoexpmap drops are silently rejected). |
| OQ2 | **Dorico microtonal test score** — does O-Lyrica's spike-findings ship a canonical .dorico file, or do we author one fresh? If author fresh — what microtones (quarter-sharp ¼♯ / quarter-flat ¼♭ / 1/6-tone / 31-EDO?), what range, what duration? |
| OQ3 | **Logic Pro MPE mode setup** — exact UI path to enable MPE on an instrument track in Logic 10.4+ (Track Inspector → MIDI → MPE? Smart Controls? External MPE controller required, or virtual MPE source available within Logic)? |
| OQ4 | **Windows VST3 build environment** — confirm Windows machine/VM is on hand; confirm `.\scripts\build-and-install.ps1` exists and works for O-Bassoon (vs. raw `cmake --build`); confirm WebView2 runtime + Inno Setup if needed; confirm pluginval.exe path. |
| OQ5 | **Preset binary format + commit hygiene** — .aupreset binary commit OK or do we use a JSON/XML state dump? FXP/FXB for VST3 — host-portable across DAW vendors? Family precedent for O-Wind / O-Bowed / O-Contrabass preset format. |
| OQ6 | **CHANGELOG.md location + schema** — top-level repo `CHANGELOG.md` or `plugins/O-Bassoon/CHANGELOG.md`? Family precedent (O-MicrotonalSampler ships per-plugin; O-AnalogEQ ships top-level only). Schema: Keep-a-Changelog vs custom format. |
| OQ7 | **PLUGINS.md row schema** — exact column set + status vocabulary (`✅ Working` / `📦 Installed` / `🚧 In progress` — pick one). What metadata: version, date, plugin code, format list, notes. |
| OQ8 | **Logic-AU 60s sustain success criterion** — pass/fail bar identical to Phase 2.4 (numpy.isfinite + RMS drift <0.5 dB + CPU drift <2%) or relaxed for Stage 4 (subjective "no glitches" in DAW)? Stage 4 is end-user-focused; subjective may be appropriate. |
| OQ9 | **DSP-06 MPE Logic verification protocol** — minimum test: 4 simultaneous notes on channels 2/3/4/5, send per-channel pitch-bend ±2 semis, verify per-voice pitch tracks via tuner. Or expand: MPE controller (Linnstrument/Seaboard) hardware test? |
| OQ10 | **Atomic commit scope** — does the Stage 4 commit include the Stage 3 changes (Resources/, PluginEditor.{h,cpp}, .planning/stages/3-gui/) IF Stage 3 commit is landed first per D8? Or are those already isolated from the working tree at commit time? Working tree currently shows ~11 modified + 2 untracked Stage 3 paths. |

---

## Risks (with mitigations)

| # | Risk | Severity | Likelihood | Mitigation |
|---|------|----------|------------|------------|
| R1 | **Dorico Playback Template ingestion silent-fail** | High | Medium | Per project memory: Dorico does NOT auto-ingest `.doricoexpmap` drops. Use `note-expression` v1.1.0 Playback Template (.dorico_pt). Verify in Dorico Library Manager that the template appears + applies to a fresh project. If silent-fail, fall back to manual `.doricolib` install + document in CHANGELOG. |
| R2 | **Windows VST3 build environment friction** | Medium | Medium | OQ4 resolves at research-phase. Family precedent: O-AnalogEQ + O-Wind + O-MicrotonalSampler all build clean on Windows. `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` already set per Stage 1 audit (project memory critical pattern). |
| R3 | **pluginval-10 Windows regression vs macOS** | Low | Low | macOS pluginval-10 is PASS; structural code is identical cross-platform. Only WebView2 init or font-load divergence likely. Mitigation: `withWinWebView2Options.withUserDataFolder("OBassoon_WebView")` already in PluginEditor.cpp:44 per project memory critical pattern. |
| R4 | **Logic-AU 60s sustain regression after Stage 3** | Low | Low | Stage 3 verify confirmed binaries unchanged; push-channel atomics (currentActiveVoiceCount, currentEffectiveBreath, currentVibratoEnvelope) are relaxed-memory-order + allocation-free in processBlock prologue. Pre-Phase-3 Logic-AU 60s sustain (Phase 2.4 verify) was PASS. |
| R5 | **DSP-06 Logic-MPE setup uncertainty** | Medium | Medium | OQ3 resolves at research-phase. Fallback: synthetic per-channel pitch-bend MIDI file routed into Logic-AU (no MPE hardware needed). Final fallback: trust Stage 2 inline `juce::Synthesiser::handlePitchWheel` static-check + accept partial DSP-06 MPE-half. |
| R6 | **Preset binary cross-DAW portability** | Medium | Medium | OQ5 resolves. .aupreset is Logic/AU-locked; FXP is VST3-portable but vendor-quirky. Mitigation: ship state-dump JSON alongside binary; document in `presets/README.md` how to load in each DAW. |
| R7 | **Stage 3 commit fold-in scope creep** | Low | Low | OQ10 resolves at plan-phase. Family invariant: Stage 3 commit lands FIRST (D8); Stage 4 commit only contains Stage 4 deltas. Working-tree clean check after Stage 3 commit. |
| R8 | **Bitwig MPE drop blocks family-canonical DSP-06 closure** | Low | Low | D5 chose Logic-MPE substitute; Logic 10.4+ MPE is production-quality (Apple-shipped). Bitwig MPE not strictly required — it was the OQ#10-rev-4 default, not a hard contract. |
| R9 | **CHANGELOG / PLUGINS.md schema drift** | Low | Medium | OQ6 + OQ7 resolve at research-phase. Lift O-MicrotonalSampler CHANGELOG verbatim (per-plugin precedent + most-recent format). PLUGINS.md schema audit at first plan-phase task. |
| R10 | **rev-3 iteration ceiling burn on Win build / Dorico parity** | Medium | Medium | Phase 2.4 burned rev-5; Stage 4 has more independent tasks (less coupled iteration). Mitigation: each task independent → if Win build needs 3 iterations, Dorico parity doesn't burn from same budget. Track per-task iteration count separately. |

---

## Atomic Commit (locked subject — pending Gate 6 PASS)

**Subject:** `feat(O-Bassoon): Stage 4 polish + v1.0.0 release - COMPAT-01/02 + DSP-06 PASS`

**Commit body to include at verify-phase:**
- Stage 4 single-pass execute summary
- Gate 6 10-item PASS table
- REQUIREMENTS.md v1.0.6 deltas (COMPAT-01/02/DSP-06 → complete; DSP-05 stays partial v1.1 candidate)
- Phase 2.4 → Stage 3 → Stage 4 commit shas in trail

---

## Process Invariants

1. **Stage 3 atomic commit MUST land on `main` BEFORE Stage 4 execute-phase begins.** PLAN-rev-1 task #1 hard gate. Subject locked at Stage 3 verify: `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`.
2. **Single Stage 4 atomic commit on Gate 6 PASS.** No mid-stage commits.
3. **rev-3 inline iteration ceiling.** Burned per-task (independent budget per Win-build, Dorico-parity, Logic-smoke, presets).
4. **No DSP-05 v1.1 work in Stage 4.** Out-of-scope register; deferred to v1.0.1 / v1.1.
5. **No Stage 5 — v1.0.0 ships at Stage 4 atomic commit.** STATUS.md transitions to `stage_4_complete` / `v1_0_0_shipped`.

---

## Next Phase

**Ready for:** `/plugin-research O-Bassoon 4-polish`

10 OQs with research-phase deliverables expected. Research can run in parallel with the Stage 3 commit-landing step (D8) — neither blocks the other; only execute-phase blocks on Stage 3 commit.

---
*Generated 2026-05-01 from /plugin-discuss O-Bassoon 4-polish*
*Schema: family-canonical CONTEXT.md (matches Phase 2.1–2.4 + Stage 3 precedent)*
