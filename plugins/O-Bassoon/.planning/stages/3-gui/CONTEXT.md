# Stage 3: GUI — Context

## Discussion Summary

**Date:** 2026-05-01
**Plugin:** O-Bassoon
**Stage:** 3 of 4 (GUI)
**Phase:** discuss
**Participants:** User, Claude
**Predecessor:** Stage 2 / Phase 2.4 verify ⚠️ PARTIAL (closed) — `STATUS.md` line 24
**Blocker:** UI mockup pass (`/ui-mockup O-Bassoon`) — Stage 3 cannot enter execute-phase until mockup HTML lands. Discuss-phase runs ahead of mockup so the mockup brief is sharper.

## Inputs Loaded

- `BRIEF.md` — 10 parameters, "long-tone-friendly", Ouaricon-family visual language inheritance, UI deferred to mockup pass
- `ROADMAP.md` — Stage 3 D8 (2-phase GUI), v1.0 tuning headless per D6
- `parameter-spec-draft.md` — 4 natural groups (Vibrato 3 / Expression 3 / Envelope 2 / Voicing+Output 2)
- `REQUIREMENTS.md` v1.0.5 — UI-01 (all v1.0 params with family visual language) + UI-02 (mockup designed and approved before implementation)
- `STATUS.md` — Stage 2 closed; Phase 2.4 atomic commit PENDING explicit user trigger
- Family precedents: O-Wind 900×600 paper-naturalist with tabs (Sound / Tuning / Effects); O-Lyrica 700×450 paper (no tabs); O-MicrotonalSampler tab pattern (Sample Map / Tuning / About)

## Requirements Confirmed

- **UI-01** — UI exposes all 10 v1.0 parameters with Ouaricon family visual language (paper + sage green naturalist, Garamond serif, botanical overlay)
- **UI-02** — UI mockup designed and approved before Stage 3 execute-phase (this discuss-phase precedes the mockup pass; discuss decisions inform the mockup brief)

## Constraints Identified

- **Cross-platform WebView (per project memory):** must include `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in CMakeLists; resource provider receives bare paths not full URLs (`if (url == "/" || url == "/index.html")` style); macOS/iOS/Linux use `juce://juce.backend/`, Windows uses `https://juce.backend/` — never hard-code schemes
- **`Juce` namespace vs `window.__JUCE__` (per project memory):** shared `tuning-panel.js` takes a `juceApi` constructor arg and calls `juceApi.getNativeFunction(name)` — must be passed `Juce` (ES-module namespace), NOT `window.__JUCE__` (low-level postMessage handler). Failure mode: panel renders but every backend call silently throws TypeError.
- **WebView2 user-data folder on Windows (per project memory):** must `withUserDataFolder(File::getSpecialLocation(File::tempDirectory).getChildFile("OBassoon_WebView"))` — DAW hosts deny default location and JUCE silently falls back to IE backend (no resource provider, blank page).
- **JUCE 8 critical patterns:** WebSliderParameterAttachment 3-arg constructor with `nullptr` undoManager (#12); relative-drag knobs frame-delta pattern (#16); `valueChangedEvent.addListener` callback receives no params, read via `getNormalisedValue()` (#15); ES6 module loading `type="module"` (#21); `IS_SYNTH TRUE` already set at Stage 1 (#22)
- **DSP-07** — UI sources must contain zero references to O-Reed (already enforced at Stage 1; carry forward)
- **PERF-01** — UI must not introduce allocations on the audio thread; feedback channels must be lock-free (atomic load on audio thread, JS poll/push on message thread)

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D1 | Aesthetic | **Ouaricon botanical** (paper + sage green, Garamond serif, botanical overlay) | User authority — "Ouaricon botanical as always". Inherits from O-Wind palette (CSS variables `--bg-paper #F5E6D3`, `--green-mid #6B8E4E`, `--brown-frame #5C4033`, `--brown-text #3C2F2F`). Maximum family cohesion; O-Bassoon is the bassoon counterpart to O-Wind's flute. |
| D2 | Editor size | **900×600** (O-Wind size) | Sectioned panels need breathing room; 4 groups × ~225 px column width comfortably fits 3 knobs/section with header + footer. Matches family's sustained-instrument-editor footprint. |
| D3 | Parameter grouping | **4 sections: Vibrato (3) \| Expression (3) \| Envelope (2) \| Voice+Output (2)** | Mirrors `parameter-spec-draft.md` natural groups. Recognizable mental model: vibrato cluster, dynamics cluster, time-shape cluster, voicing+master cluster. |
| D4 | `attack_character` control | **Continuous knob 0..1** with **Soft / Tongued** end-labels | Honest to DSP morph reality (Phase 2.4 implements `juce::jmap` interpolation between `softShape` and `tonguedShape`). Mid-position = audible morph. Toggle would discard the entire mid-morph palette the DSP supports. |
| D5 | Visual feedback elements | **All three:** active-voice dots (1..N), live breath/CC2 meter, vibrato-active pulsing dot | User selected all three. Justifies the 900×600 footprint. Requires C++→JS push channel. Engineering scope acknowledged in Phase 3.2. |
| D6 | Tab structure | **Tabs: "Sound" (default) / "Tuning" / "About"** — family-canonical pattern | User authority — "have it as a tab like on all other instruments in this repo". Matches O-Wind (Sound/Tuning/Effects), O-MicrotonalSampler (Sample Map/Tuning/About) precedent. Tuning tab embeds shared `tuning-panel.{css,js}` module (Circle/Polar/TrueKeys, .scl loader). |
| D7 | **DEVIATION from BRIEF D6 / ROADMAP** — tuning UI exposure | **Tuning panel exposed as a tab at v1.0** | BRIEF D6 originally said "12-TET headless at v1.0; UI exposure deferred to v1.1". Superseded by user authority at this discuss-phase. Implication: ROADMAP Stage 3 scope expands; BRIEF + REQUIREMENTS need a Stage 3 amendment row. Engine itself is already wired headless from Stage 1 — only the panel embed + bindings are new. |
| D8 | Phase split | **2 phases (per ROADMAP D8):** Phase 3.1 layout + parameter binding + tuning-tab embed; Phase 3.2 polish + 3 feedback elements + final Logic-AU verification | Honors original ROADMAP D8 commitment. Phase 3.1 scope grows by tuning-tab embed (vs. ROADMAP's 10-param-only scope) but stays within "layout + binding" theme. Phase 3.2 takes on the 3 feedback elements (voice dots, breath meter, vibrato dot) which need bidirectional channels but no new attachments — fits "polish" theme. |
| D9 | Knob interaction | **Relative-drag** (frame-delta), per `juce8-critical-patterns.md` #16 | Family-canonical. No jump-to-cursor. Engineering precedent: O-Wind, O-Lyrica, O-MicrotonalSampler. |
| D10 | Resource provider URL handling | **Bare-path equality checks** (`if (url == "/" \|\| url == "/index.html")`) | Per project memory and O-Bells precedent. Never use `fromFirstOccurrenceOf("://")` on what is already a bare path — the well-known regression mode is "Frame load interrupted" + blank page. |

## Cycle Scope (Stage 3)

**In scope:**
- Convert finalized UI mockup → `Source/ui/public/index.html` (Phase 3.1)
- Ouaricon-botanical aesthetic CSS (paper + sage green + Garamond + botanical overlay) (Phase 3.1)
- 4-section knob layout for 10 APVTS parameters (Phase 3.1)
- Tab structure: Sound (default) / Tuning / About (Phase 3.1)
- Tuning tab: embed shared `tuning-panel.{css,js}` module + bind via `Juce` namespace `getNativeFunction` (Phase 3.1)
- 10× `WebSliderRelay` + 10× `WebSliderParameterAttachment` (3-arg constructor, `nullptr` undoManager) (Phase 3.1)
- `voice_count` is `AudioParameterInt` → still uses `getSliderState` per `juce8-critical-patterns.md` #19 (Phase 3.1)
- CMakeLists updates: `juce_add_binary_data` for UI resources, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, Windows `withUserDataFolder` (Phase 3.1)
- 3 visual feedback elements with C++→JS push channels (Phase 3.2):
  - **Active-voice dots** — push voice-count snapshot (or active-count) at block boundaries via `emitEventIfBrowserIsVisible`; JS render 0..N filled dots
  - **Breath meter** — push `effective_breath = ui_breath × cc2_normalised` at ~30 Hz throttle; JS render thin meter
  - **Vibrato-active dot** — push `vibrato_onset_progress + sin(phase)` envelope at ~30 Hz throttle; JS pulse dot at vibrato_rate when active
- Verify UI-01 + UI-02 (Phase 3.2)
- Polish pass + final Logic-AU verification + atomic commit (Phase 3.2)

**Out of scope (defer to v1.1+):**
- Preset browser (BRIEF non-goal)
- Aftertouch → vibrato modulation UI (BRIEF stretch)
- Custom knob skins beyond family default
- Spectrum/scope visualization
- MIDI learn UI
- Dorico playback-template UI exposure (Stage 4 owns Playback Template generation)

## Open Questions (handed to research-phase)

1. **Tuning-panel module wiring pattern** — does O-Wind/O-MicrotonalSampler import `tuning-panel.js` from a shared module path or copy it per-plugin? Confirm canonical pattern; check `modules/tuning/scala-tuning-engine/` for the panel source of truth.
2. **`Juce` namespace import** — confirm exact ES-module import path the family uses (`./juce/index.js` vs `/juce/index.js` vs other) and how it's bundled with `juce_add_binary_data`.
3. **C++→JS push-channel pattern** — confirm `emitEventIfBrowserIsVisible` (`juce_WebBrowserComponent.h`) is the family-canonical channel for the 3 feedback elements vs. JS-side `window.setInterval` polling a getter via `getNativeFunction`. Compare to O-MicrotonalSampler if it pushes any live state.
4. **Audio→message-thread bridge** — what's the family-standard for ferrying live values (voice count, breath, vibrato envelope) from `processBlock` to the message thread without `std::atomic` per-block overhead inflation? Check O-Wind / O-MicrotonalSampler patterns; expected answer is `std::atomic<float>` snapshot read by a `Timer::callback` on message thread.
5. **Active-voice count** — for the dots indicator, do we expose `voice_count` (the cap) or `current-active-voice-count` (the live count from BassoonSynthesiser's findFreeVoice activeVoices loop)? The latter is the more interesting feedback element. Confirm at research-phase.
6. **Botanical overlay asset** — is the bassoon family asset library expected to ship a bassoon-specific botanical (the bassoon plant doesn't exist; bassoonists pun on cane/reed) or do we reuse O-Wind's overlay? Confirm at research / mockup phase.
7. **About-tab content scope** — version, credits, license attribution, link to project? Match family precedent at research-phase (O-MicrotonalSampler about-tab pattern).
8. **Windows WebView2 redistributable** — confirm static-link path is sufficient for this plugin's deployment targets; cross-check against current plugin-audit memo (34/35 plugins missing the flag — O-Bassoon must NOT be one of them).
9. **Mockup blocker resolution** — when does `/ui-mockup O-Bassoon` run? Before Phase 3.1 plan-phase (recommended; mockup is a hard gate for execute) or in parallel? Confirm at research/plan-phase.
10. **ROADMAP / BRIEF amendment** — per D7 deviation, the docs need a row update. When is the right phase to backfill — at this discuss-phase, at plan-phase as a planning task, or at Stage 3 close? Recommend plan-phase (low cost, high traceability).

## Risks

| # | Risk | Severity | Mitigation |
|---|---|---|---|
| 1 | Tuning-tab embed underestimated — adding a tab is structurally easy but the shared `tuning-panel.{css,js}` brings a non-trivial JS surface (Circle/Polar/TrueKeys widgets, .scl parser, native bindings) and a known footgun (`Juce` namespace vs `window.__JUCE__`, per memory) | **Medium** | Treat tuning-tab embed as Phase 3.1 anchor task (not afterthought). Lift integration pattern verbatim from O-Wind. Run a smoke check at execute-phase: load tuning tab, confirm intervals table renders + Generate button works (memory-known regression sentinel). |
| 2 | C++→JS push channels for 3 feedback elements break PERF-01 if implemented naively (audio-thread allocation or lock) | **Medium** | All 3 channels read from `std::atomic<float>` snapshots written on the audio thread; message-thread `Timer::callback` polls and emits. Zero allocation in `processBlock`. Pattern lifted from family precedent. Validate with the existing 16-item static-check grep battery + pluginval-10 fuzz at Stage 4. |
| 3 | Botanical overlay licensing — if O-Wind's asset is bundled-only (not redistributable), copy may need a fresh asset for O-Bassoon | **Low** | Audit at research-phase. Worst case: produce a public-domain botanical illustration (USDA Plants? Wikimedia botanical archive?). Defer asset finalization to mockup pass. |
| 4 | Resource-provider regression — O-Bells/O-Lyrica use bare-path equality but a recent regression caused "Frame load interrupted" via `fromFirstOccurrenceOf("://")` use on a bare path (per memory) | **High** (if regressed) / **Low** (if pattern lifted verbatim) | Code review + pre-commit grep gate: `grep -rn "fromFirstOccurrenceOf" plugins/O-Bassoon/Source/` MUST return zero matches. Lift `withResourceProvider` lambda verbatim from O-Bells / O-Wind. |
| 5 | Windows WebView2 falls back to IE silently → blank page in DAW host | **High** (if missing) | CMakeLists must include both flags simultaneously: `NEEDS_WEBVIEW2 TRUE` (links `WebView2LoaderStatic.lib`) AND compile-define `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. Plus runtime `withUserDataFolder` to a temp dir. Triple-check at plan-phase. |
| 6 | 3-arg `WebSliderParameterAttachment` constructor footgun — older 2-arg signature compiles but throws a deprecation warning that some builds elevate to error | **Low** | Use 3-arg form with `nullptr` undoManager per `juce8-critical-patterns.md` #12. Already family-canonical. |
| 7 | Mockup-deferred discuss-phase produces stale CONTEXT.md if mockup design diverges from these decisions | **Medium** | Discuss-phase locks design *intent*; mockup pass ratifies *form*. If mockup pass surfaces a divergence (e.g. user prefers 3 sections instead of 4), CONTEXT.md is amended at plan-phase via a `(rev-2)` addendum, not rewritten. Consistent with Stage 2 / Phase 2.x rev-N pattern. |
| 8 | Live feedback elements (voice dots, breath, vibrato) introduce DAW-side CPU overhead at message-thread polling rate | **Low** | Throttle to 30 Hz (33 ms `Timer::callback` interval). Skip emit when value within epsilon of last-emitted value. Pattern: family precedent (O-MicrotonalSampler emits sample-map state at low rate). |
| 9 | Tab switch state persistence — does opening Tuning tab persist across plugin reload? Default of "always opens to Sound tab" is fine, but ROADMAP-aligned only if explicit | **Low** | Default to "Sound tab on open". No persistence at v1.0. Defer to v1.1 if user requests. Document as Stage 3 known limitation. |
| 10 | Stage 3 execute-phase blocks on mockup landing — if mockup pass surfaces blocking design questions that take >1 cycle to resolve, Stage 3 stalls | **Medium** | Mockup pass is a separate orchestrator (`/ui-mockup`) with its own iteration loop. Stage 3 plan-phase MUST NOT begin until mockup is finalised AND user has approved. STATUS.md `next_action` flips to `stage_3_research_phase` only after mockup approval lands. |

## Phase Plan Preview (informational — locked at plan-phase)

**Phase 3.1 — Layout + Binding + Tuning Tab Embed:**
- Convert mockup HTML → `Source/ui/public/index.html`
- Ouaricon-botanical CSS (lift O-Wind palette + Garamond + paper + botanical overlay)
- Tab markup: Sound / Tuning / About + tab-switch JS (lift O-Wind/O-MicrotonalSampler pattern)
- 4-section knob layout (Vibrato 3 / Expression 3 / Envelope 2 / Voicing 2)
- 10× WebSliderRelay + 10× WebSliderParameterAttachment (3-arg)
- Tuning-tab `tuning-panel.{css,js}` embed + `Juce` namespace `getNativeFunction` wiring
- About tab static content
- CMakeLists: `juce_add_binary_data`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, Windows `withUserDataFolder`
- Resource-provider lambda (bare-path equality)
- Build + install + pluginval-5 + auval

**Phase 3.2 — Polish + Feedback Elements:**
- C++→JS push channels (atomic snapshots + 30 Hz Timer + `emitEventIfBrowserIsVisible`):
  - Active-voice count (live, not cap)
  - Effective breath (ui × CC2)
  - Vibrato envelope (onset progress × LFO)
- JS render: voice dots row, breath meter, pulsing vibrato dot
- Layout polish (spacing, hover states, knob value tooltips)
- Final Logic-AU manual checklist
- pluginval-10 + auval re-run
- Atomic commit `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`

## Next Phase

**Ready for:** research-phase
**Blocker:** UI mockup pass (`/ui-mockup O-Bassoon`) before plan-phase
**Atomic commit subject (locked):** `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`
**Inline iteration ceiling:** rev-3 (family precedent: Phase 2.x discuss-phases)

---

## Rev-2 Addendum — UI Mockup Pass Skipped (2026-05-01)

**Process deviation per user authority** ("I would like to skip the UI mockup"). The family-canonical mockup-first workflow (`/ui-mockup` orchestrator → browser preview → user feedback → revise → approval gate → execute) is replaced by direct gui-agent execute. Iteration loop moves post-build to Logic-AU audition.

### What changes

| Concern | Before | After (rev-2) |
|---|---|---|
| Mockup orchestrator | `/ui-mockup O-Bassoon` runs before plan-phase; produces HTML mockup at `<plugin>/.planning/mockup/index.html` | Skipped entirely. No mockup artefact produced. |
| HTML production | Phase 3.1 task T1 converts mockup HTML → `Resources/ui/index.html` | gui-agent generates `Resources/ui/index.html` directly from CONTEXT.md decisions D1–D10 (no intermediate file) |
| Iteration loop | Browser preview ↔ user feedback before any C++ touched | In-DAW audition after build/install; revisions land as in-cycle iteration up to rev-3 ceiling |
| Approval gate | "Mockup finalised AND user-approved" before execute begins | Removed. Execute-phase begins after Phase 2.4 atomic commit lands. |
| `next_action` | `stage_3_ui_mockup_pass_then_execute_phase` | `stage_3_execute_phase` (gated only on Phase 2.4 commit) |
| UI-02 requirement | "UI mockup designed and approved before Stage 3 implementation" | **AMENDED:** "UI design approved at execute-phase via in-DAW audition." See REQUIREMENTS.md UI-02 row. |
| Risk #10 (mockup blocker) | High-severity stall risk if mockup surfaces blocking design questions | **Retired.** Replaced by Risk #11 below. |
| PLAN.md T1 ("BLOCKED on mockup") | Hard block: T1 must wait for mockup approval | **Repurposed:** T1 becomes "gui-agent generates `Resources/ui/index.html` from CONTEXT D1–D10" — no external blocker. |

### Why the locked decisions still hold

CONTEXT.md D1–D10 already specify design *intent* at sufficient granularity for direct implementation:
- D1 aesthetic (Ouaricon-botanical, palette pinned to O-Wind CSS variables)
- D2 size (900×600)
- D3 layout (4 sections, parameter grouping pinned)
- D4 control type per parameter (continuous knob with end-labels)
- D5 feedback elements (3 enumerated)
- D6 tab structure (Sound default / Tuning / About)
- D7 deviation locked (tuning-tab at v1.0)
- D9 interaction (relative-drag, frame-delta)
- D10 resource provider (bare-path equality)

The mockup pass would have ratified *form* (specific knob diameter, hover micro-interactions, exact spacing); these are now decided by the gui-agent from family precedent (lift O-Wind verbatim) and refined post-build via audition.

### New risk introduced

| # | Risk | Severity | Mitigation |
|---|---|---|---|
| 11 | Skipping mockup pass means design surprises only surface after compile/install — gross structural mistakes (e.g. tab bar covers a knob, knob row overflows at 900 px) cost a full build cycle to discover instead of a browser refresh | **Medium** | Lift O-Wind layout structure verbatim (proven 900×600 with tabs + sections). Limit gui-agent freedom to known-good family precedent. Use rev-1 → rev-3 iteration ceiling at execute-phase to absorb any audition-surfaced revisions. |

### Carry-forward decisions (still locked)

D1–D10 unchanged. PLAN.md tasks T0 (doc backfill), T2–T17 unchanged. Atomic commit subject unchanged. 2-phase split (3.1 layout+binding+tuning-tab; 3.2 polish+feedback) unchanged.

### Phase 2.4 atomic commit gate (unchanged)

Stage 3 execute-phase still cannot begin until `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PARTIAL (DSP-05 v1.1 candidate)` lands on `main`. This is an independent commit-protocol gate, not a UI-design gate.
