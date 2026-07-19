# Stage 4: Polish — Execution Plan

**Date:** 2026-07-15
**Plugin:** O-Contrabass
**Entry state:** Stage 3 (GUI) VERIFIED 2026-07-11. Discuss + Research complete (`CONTEXT.md`, `RESEARCH.md`).
**Ships:** v1.0.0 — final stage. Author preset banks, run subjective/perf/Dorico validation, close the
Windows cross-platform gate, ship the Dorico distribution bundle, reconcile version/docs.

---

## Goal

Bring O-Contrabass to a validated, production-ready **v1.0.0** — everything short of a public
release, which the user is holding. Concretely, close the six Stage-4 requirements:

- **FUNC-04** — ship 10 factory presets (5 Orchestral + 5 Drone), seeded skew-safe.
- **COMPAT-02** — ship a full `.doricolib` Playback Template bundle; microtonal NE plays correctly in Dorico.
- **COMPAT-01** — Windows VST3 built + pluginval strictness-10 green **via CI** (no public release published).
- **PERF-02** — measured `<5%` CPU/voice on Apple Silicon @ 44.1/48 kHz, 256-block, defaults.
- **FUNC-03 + DSP-10** — subjective sign-off (dual orchestral/drone + slow-attack character) via documented A/B audition.
- Reconcile version/docs (CHANGELOG → v1.0.0, parameter-spec, NOTES v1.1 deferrals, registry.yaml).

**Cross-cutting invariant (every task):** **DSP is FROZEN.** 19/19 render goldens stay
byte-identical. All work writes parameter STATE + docs + distribution artifacts + CI/test tooling —
never signal-path arithmetic. Any golden shift = defect, not polish. The v1.1 deferral list
(STRING_TENSION inert @0.5, `.tun` parser, DSP-07/08/09 depth, FUNC-07 MTS-ESP stub) stays deferred.

---

## Decisions resolved (from RESEARCH open questions — override at execute if desired)

| # | Decision | Choice | Why |
|---|----------|--------|-----|
| 1 | Preset banks | **Flat alphabetical list + naming convention** (no categories) | Canonical module has no `category` field; vendoring O-Prism's categorized manager is a bigger change unjustified for 10 presets |
| 2 | Preset tuning-reset | **Drone presets carry explicit `TUNING_SYSTEM`/`NOTE_EXPRESSION`**; per-string pitch via `DETUNE_*` | WR-01 reset clobbers tuning params to 12-TET/440/true on load; DETUNE_* are independent APVTS params, don't touch Scala engine |
| 3 | Dorico GUID | **Ship dev-branded bundle** (`ABCDEF019182FAEB4F7544764F436273`, name `O-Contrabass-dev`) + document release-GUID swap | Matches O-MicrotonalSampler precedent; validation runs against the dev build we can build locally |
| 4 | Dorico dynamics | **`kNoteVelocity` for v1.0** | No CC11/CC1 listener exists; a CC11 path touches param handling → risks frozen goldens → defer to v1.1 |
| 5 | Windows CI shape | **`workflow_dispatch` + `validate-only` gate + pluginval-10 step**; extend `SKIP_PLUGINS` to O-Contrabass-only for the validation run | Non-publishing path required (user holds release); trimming siblings de-risks Windows-hostile sibling configure + speeds the run |
| 6 | PERF harness | **`--sample-rate` + `--block-size` CLI knobs + RTF/CPU% field**; corroborate with a Logic CPU-meter spot-read | Minimal surface, golden gate is WAV-sha256-only so timing additions can't break the invariant; O-Bassoon Logic precedent |
| 7 | Version/docs | Collapse CHANGELOG `[1.1.0-dev]` → `[1.0.0]` (compiled version is already 1.0.0); fix parameter-spec voice count; NOTES v1.1 list; registry.yaml R5 | Doc-only reconciliation, no code-version change |

**RESEARCH corrections folded in:** only **4** params are skewed — `BOW_SPEED`(0.5), `BOW_PRESSURE`(0.5),
`BRIGHTNESS`(0.25), `VIBRATO_ONSET`(0.5). `VIBRATO_RATE`/`SLOW_LFO_RATE`/`REFERENCE_PITCH` are LINEAR
(CONTEXT was wrong). Mitigation is identical: route **every** value through `convertTo0to1`. Compiled
version already `1.0.0`. `parameter-spec.md` wrongly says "monophonic" — actual `kNumVoices = 4`.

---

## Tasks

### Group A — Factory Presets (FUNC-04)

**1. [ ] Author the 10 `FactoryPresetDef`s in engineering units + seed them**
- Files: `Source/PluginProcessor.cpp` (constructor, after `setCustomStateCallbacks` block ~line 196+)
- Depends on: none
- Pattern: copy **O-SpectralShaper** inline `std::vector<FactoryPresetDef>` + single `convertTo0to1`
  loop + `presetManager.initializeFactoryPresets(...)` (`O-SpectralShaper/Source/PluginProcessor.cpp:254-291`).
- Skew-safe loop (mandatory — handles the 4 skewed params + int/choice/bool uniformly):
  ```cpp
  for (auto& preset : factoryPresets)
      for (auto& [id, value] : preset.parameters)
          if (auto* p = parameters.getParameter(id))
              value = p->convertTo0to1(value);
  presetManager.initializeFactoryPresets(factoryPresets);
  ```
- Author values in **engineering units**, never raw normalized fractions (skew → 4×–30× wrong).
- Int/choice/bool in natural units: `{"ACTIVE_STRINGS",4.0f}`, `{"TUNING_SYSTEM",2.0f}`, `{"NOTE_EXPRESSION",1.0f}`.
- **Do NOT author STRING_TENSION away from 0.5** (inert, v1.1 — moving it re-baselines goldens if ever wired).
- Preset set (param inventory + ranges/skews in RESEARCH Q1.6; relevance map Q1.7):
  - **Orchestral:** Cinematic Bass Sustain *(default — first alphabetically or explicit)*, Section Bass, Solo Arco Bass, Pianissimo Bass, Forte Bass.
  - **Drone:** Infinite Drone, Just-Intoned Drone *(DETUNE_A=+204, DETUNE_D=−14, DETUNE_G=+182; ACTIVE_STRINGS=4)*, Scordatura Bass, Sub Drone, Dark Pad Bass.
- Per Decision 2: drone presets set explicit `TUNING_SYSTEM`/`NOTE_EXPRESSION` so WR-01 reset doesn't clobber intent.

**2. [ ] Handle the version-sentinel re-seed hazard during the execute audition loop**
- Files: execute notes / build step (no source change)
- Depends on: Task 1
- `initializeFactoryPresets` only re-seeds when `JucePlugin_VersionString` changes; version is pinned
  1.0.0 all stage → edited preset defs will NOT re-seed once `Factory/` exists.
- Mitigation: `rm -rf ~/Library/O-Contrabass/Presets/Factory/` before each reinstall so tweaks take.
  (First-ever install seeds cleanly; hazard is only on re-tweaks within the same version.)

### Group B — Dorico `.doricolib` Bundle (COMPAT-02)  *(delegate authoring to `dorico-agent`)*

**3. [ ] Author the 3-artifact Dorico bundle (single-family reduction of O-MicrotonalSampler's)**
- Files (new dir):
  - `Resources/dorico/EndpointConfigs/O-Contrabass/playbacktemplatedeps.doricolib`
  - `Resources/dorico/EndpointConfigs/O-Contrabass/endpointconfig.xml`
  - `Resources/dorico/PlaybackTemplateSpecs/O-Contrabass/playbacktemplatespec.xml`
- Depends on: none (parallel with A/C/D)
- Template: `plugins/O-MicrotonalSampler/Resources/dorico/` (Strings def at `.doricolib:33-320`).
- **P0 LOAD-BEARING microtonal fields** (recurring silent 12-TET regression) — inside `<ExpressionMapDefinition>`, between `<applyStageTemplateSettings>` and `<initSwitchData>`:
  ```xml
  <pitchBendRange>2</pitchBendRange>
  <microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>
  ```
  Value is `2` (not 48); never `kAuto`. Per-combination copies are additive, NOT a substitute.
- Reduce exp-map to a **single `pt.natural` combo, empty `<switchOnActions array="true"/>`** — delete the 7 non-natural technique combos (O-Contrabass is sustained-arco only, no keyswitches).
- **ID chain must match byte-for-byte** (RESEARCH Q2.1): spec `configID` → endpoint `configID` + `expressionMapID` → doricolib `entityID`; endpoint `<pluginID>` = VST3 ClassID GUID.
- Identifiers (Decision 3, dev build): `<pluginID> = ABCDEF019182FAEB4F7544764F436273`, `<pluginName> = O-Contrabass-dev`.
- Routing: endpoint `<instruments>` entityIDs (`instrument.strings.contrabass`, `.contrabass.d`, `.uprightbass`), NOT `<instrumentFamilies>`. One endpoint, one channel, `kSoloPlayer`.
- Dynamics: `volumeType kNoteVelocity` (Decision 4 — NOT the CC11 default O-MicrotonalSampler uses).
- Parser strictness: any XML comment lives INSIDE `<kScoreLibrary>`, never before the root element (leading comment → launch crash).
- On any edit: bump the definition `<version>` (cache key = entityID+version).

**4. [ ] Author `INSTALL-DORICO.md` + `SMOKE-TEST.md`**
- Files: `Resources/dorico/INSTALL-DORICO.md`, `Resources/dorico/SMOKE-TEST.md`
- Depends on: Task 3
- INSTALL: 3-folder install map (macOS `~/Library/Application Support/Steinberg/Dorico 6/…`), quit→copy→relaunch→*Play → Playback Template… → tick → Apply and Close* ("Loaded ≠ Applied"), + documented **release-GUID swap** (dev `OuDv`/`-dev` → release `OuAu`/unsuffixed has a DIFFERENT GUID).
- SMOKE-TEST: **P0 acceptance = TC-4 (24-EDO quarter-sharp)** end-to-end microtonal pitch test — the only check that catches a dropped top-level microtonal field.

**5. [ ] Add a CMake `install(DIRECTORY …)` rule for the Dorico bundle**
- Files: `CMakeLists.txt`
- Depends on: Task 3
- None exists → the bundle won't ship without it. Install `Resources/dorico/…` into the package.

### Group C — Windows CI + pluginval-10 (COMPAT-01)  *(⚠️ BLOCKER — must author new CI paths)*

**6. [ ] Add a non-publishing `workflow_dispatch` validate-only path**
- Files: `.github/workflows/build-and-release.yml` (trigger block :9-12; create-release :522-586; macOS sign/notarize)
- Depends on: none
- Add `workflow_dispatch` with a `plugin_name` input + a `validate_only` flag. When `validate_only`, **skip/gate `create-release`** (and sign/notarize) — the current only-trigger (`*-v*` tag) auto-publishes a public Release, which conflicts with the held-release decision.
- Per Decision 5: extend `SKIP_PLUGINS` to build **O-Contrabass only** for the validation run — de-risks Windows-hostile sibling configure (path only ever ran per-tagged-plugin; ~30 siblings unverified on `windows-latest`) and speeds the run.

**7. [ ] Add a pluginval strictness-10 step to the `build-windows` job**
- Files: `.github/workflows/build-and-release.yml` (windows job :397-517)
- Depends on: Task 6
- CI runs pluginval **nowhere** today (every repo hit is docs). Add: pin a pluginval Windows version, download it, run `pluginval.exe --strictness-level 10 --validate <…>/O-Contrabass*.vst3` (VST3 only, no AU on Windows), upload the log as an artifact.
- Raise `--timeout-ms` / add a warm-up run — cold WebView2 on a fresh runner may hit the first-run Editor-Automation timeout macOS saw (passed warm).
- WebView2 static-linking is config-correct (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, NuGet SDK provisioned) but **never exercised on a real runner** — this run is the actual blank-UI test.

### Group D — PERF-02 Benchmark (<5% CPU/voice)

**8. [ ] Add `--sample-rate`/`--block-size` knobs + RTF/CPU% to render-harness; run the benchmark**
- Files: `tests/render-harness/main.cpp` (hardcoded `sampleRate=44100/blockSize=512` at :653-654; timer plumbing at :4085-4087)
- Depends on: none
- Harness already times every `processBlock` (median/max micros). Add CLI overrides for sample rate + block size (or a `--perf` mode) and emit an RTF/CPU% field on non-golden-locked modes.
- **Safe under DSP-frozen:** golden gate is WAV-sha256-only (`reproduce-goldens.sh:90-91`) → timing/RTF additions cannot shift a golden.
- Method (Decision 6): `cpuPct = 100 × medianMicros / (blockSize/sampleRate × 1e6)`. Budgets @256-block: **5805µs @44.1k, 5333µs @48k**. Single sustained note = exactly 1 active voice → existing single-note render IS the per-voice measurement.
- Run @ {44100, 48000} × 256-block on **Release** build, defaults (= Cinematic Bass Sustain). Record the number. Corroborate with a Logic CPU-meter spot-read (O-Bassoon precedent). This is the house's first-ever RTF/CPU% number → the method becomes reusable.

### Group E — Subjective Audition (FUNC-03 + DSP-10 + FUNC-04 QA)  *(human-in-the-loop)*

**9. [ ] Author `AUDITION.md` probe-table; user auditions all 10 presets in Logic; capture sign-off**
- Files: `stages/4-polish/AUDITION.md`; sign-off → `STATUS.md` carry_forward + REQUIREMENTS verify comments on FUNC-03/DSP-10/FUNC-04
- Depends on: Tasks 1, 2 (presets installed), and a Release build installed
- Reuse the **R38 probe-table format** (`stages/2-dsp/RESEARCH.md:7139-7197`): `| Probe | Preset | Material | Reference | PASS |`. No git-worktree side-by-side (DSP frozen → no "before"). A/B = preset-switch within one installed Release build + compare vs user references.
- Probe rows (RESEARCH Q5.2): Orchestral arco (Cinematic vs Spitfire/CSS/VSL) · Slow attack DSP-10 (no note-on click, natural ~1168ms swell) · Drone (Infinite Drone vs O'Malley/Conrad) · A/B switch (Cinematic ↔ Infinite Drone, one instance, no host retune) · **Preset QA ×8** (remaining presets — accept or request tweak).
- Doubles as FUNC-04 QA — user requests tweaks, loop back to Task 1/2 (remember the re-seed `rm -rf`).
- Optionally archive harness `--out` renders as a repeatable objective anchor (O-Bassoon reference-archive precedent).

### Group F — Version / Docs Housekeeping

**10. [ ] Reconcile version + stale docs**
- Files: `CHANGELOG.md`, `.planning/parameter-spec.md`, `NOTES.md`, `.planning/../registry.yaml` (R5)
- Depends on: none (do near end so it reflects final state)
- CHANGELOG: collapse `[1.1.0-dev]` header → `[1.0.0]` (compiled version already 1.0.0 — doc-only).
- parameter-spec.md: fix stale "monophonic/1 voice" → `kNumVoices = 4`; note it omits skew factors (read skew only from `.cpp`).
- NOTES.md: record the v1.1 deferral list (STRING_TENSION, `.tun`, DSP-07/08/09, FUNC-07 MTS-ESP, Dorico CC11 dynamics).
- registry.yaml R5: preset-manager 1.0.2→1.0.4 (cosmetic, opportunistic, non-blocking — module.yaml is authoritative).

### Group G — Final Validation Battery  *(the verify gate — runs after A–F)*

**11. [ ] Run the full validation battery + 5 Logic manual checks**
- Files: none (verification)
- Depends on: Tasks 1–10
- **Automated (must all stay green):** 19/19 render goldens byte-identical · auval `aumu OCbs OuDv` SUCCEEDED · pluginval strictness-10 macOS (warm) SUCCESS · ui_frontend_check 14/14 · bridge gate 32 JS = 32 C++ · **Windows pluginval-10 (Task 7)** · **PERF-02 <5% (Task 8)** · **Dorico TC-4 P0 (Task 4)**.
- **Manual, Logic Pro** (5 human-in-the-loop checks carried from Stage 3): (1) Release editor open/close ×10 (destruction-order gate) · (2) 31-param DAW interaction incl. 4 skewed-param generic-view spot check + detune detents + ACTIVE_STRINGS stepper + TUNING_SYSTEM gating + NOTE_EXPRESSION toggle · (3) picker UAF scenario (open picker → close window → choose/cancel) · (4) Logic smoke (E1 drone / automate BOW_SPEED / project reload) · (5) visual QA @1000×650 (7 sections + tab bar, console clean, preset bar vs `~/Library/O-Contrabass/Presets/`, tuning survives preset round-trip, Schelleng dot / spectrum / VU tracking).
- **Dorico, manual (COMPAT-02):** run SMOKE-TEST.md, P0 = TC-4 24-EDO quarter-sharp plays correctly.

---

## Task Dependency Summary

```
A1 preset defs ─┬─> A2 re-seed hazard ─┐
                │                       ├─> E9 audition (needs installed Release + presets)
B3 doricolib ───┼─> B4 install/smoke ──┼─> G11 final battery (Dorico TC-4)
                └─> B5 CMake install    │
C6 dispatch ────────> C7 pluginval ─────┤
D8 perf harness ────────────────────────┤
F10 docs ───────────────────────────────┘
```
A1, B3, C6, D8, F10 have no upstream deps → can start in parallel. G11 is the terminal gate.

---

## Success Criteria

- [ ] 10 factory presets seed to `~/Library/O-Contrabass/Presets/Factory/`, recall correctly (skewed params NOT 4×–30× off), appear in the preset bar.
- [ ] Just-Intoned Drone recalls DETUNE_A=+204 / DETUNE_D=−14 / DETUNE_G=+182; drone presets survive the WR-01 tuning-reset (explicit TUNING_SYSTEM/NOTE_EXPRESSION).
- [ ] Dorico bundle installs; template appears in *Play → Playback Template…*; **TC-4 (24-EDO quarter-sharp) plays at correct microtonal pitch** (not nearest-12-TET).
- [ ] Windows VST3 builds on CI and passes pluginval strictness-10; WebView UI renders (not blank); **no public GitHub Release published**.
- [ ] Measured CPU/voice `< 5%` @ 44.1 kHz AND 48 kHz, 256-block, defaults (RTF harness number + Logic corroboration).
- [ ] User CONFIRM on FUNC-03 (orchestral + drone credible from one engine) and DSP-10 (slow attack, no note-on click) recorded in STATUS carry_forward + REQUIREMENTS.
- [ ] CHANGELOG at `[1.0.0]`; parameter-spec voice count fixed; NOTES v1.1 deferrals listed.
- [ ] **19/19 render goldens byte-identical** — unchanged through the entire stage (the frozen-DSP invariant).
- [ ] Full automated bar green at HEAD; 5 Logic manual checks pass.

---

## Do-NOT-touch (frozen / v1.1 deferrals)

STRING_TENSION inert (keep default 0.5, do not wire) · `.tun` parser absent (picker `.scl`-only, choice
index 0=Scala/1=MTS-ESP/2=12-TET frozen) · DSP-07 sub-harmonic depth · DSP-08 slow-LFO breathing ·
DSP-09 vibrato depth · FUNC-07 MTS-ESP stub (returns 12-TET) · Dorico CC11 sustained-dynamics listener.
Any change that shifts a golden is out of scope and signals a defect.
