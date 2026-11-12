---
title: "O-GranulatorInstrument Stage 4 (Polish / Validation) — Execution Plan, Rework R1 (per-stream feedback + reverb)"
created: 2026-11-12
juce_version: "8.0.15"
phase: "4-polish"
summary: "Eight waves: (W0) pre-flight, the F1 Record-in copy committed on its own (C0), baseline B0 on the unmodified tree incl. a new per-preset render dump (C1); (W1) v2 mockup of the FX · Feedback page (Variant B, Variant A behind a mockup-only switch), scratch-tree gates, commit C2, then the D65 approval checkpoint — execute STOPS; (W2, not blocked) B1: per-stream control ramps, four audible tanks, loop-side tank / toggle / Reset hold dormant at their defaults, all on the OLD parameter IDs, compared to B0 bitwise or at −110 dB, commit C3; (W3, not blocked) `rec_arm` auto-clear from the 20 ms poll timer after the CM1 / OS1 audit, rows AC1–AC9, early auval + pluginval proof, commit C4; (W4, blocked on D65) B2: spec body rewrite + addendum C, 293 = 200 / 74 / 19, destination table 150, table version 2, goldens from the spec, harness + parity port, page landing, B3 copy of the bank to all four streams, commit C5; (W5) B3 showcase re-voices (10), B4 new rows (Pre, toggle, tanks, class, clicks), `--cpu25` / `--cpu26` / `--cpu4` verdicts, commit C6; (W6) build / install / validators / final regression; (W7) CHANGELOG, ARCHITECTURE / ROADMAP addenda, HANDS-ON-SESSION-4 re-authored, SUMMARY-R1, STATUS, commit C7, then the carried Windows CI checkpoint. VERSION stays 1.0.0. No tag. No module edit. No VERBATIM-leaf edit; one ADAPTED leaf (`forked/ControlGrid.h`) changes in W4 only (Open sign-off OS1). Render goldens never re-pinned."
stages: [4]
agents: [dsp-agent, gui-agent, polish-agent, build-gate-runner]
signoffs: "2026-11-12 Taylor: all 12 RESEARCH-R1 sign-offs per the recommendation column — 1 mockup opens on Variant B, Variant A behind a mockup-only switch, picked at the D65 gate; 2 Pre = a second loop-side tank per stream; 3 hybrid tail (audible tank rings out then idles, loop tank follows audibleTarget); 4 Reset flushes a Pre stream's loop tank (60 ms); 5 Pre ceiling-hold ships with an INFO row + a hands-on row; 6 `#write-ro` dropped; 7 English destination names shortened to 'FB Low Cut' etc.; 8 `rec_arm` also clears on an arm that cannot start; 9 host automation of `rec_arm` accepted; 10 showcase set of 10; 11 pre-R1 sessions / user presets re-map silently, table version 2; 12 `sN_fb_enable`, `sN_reverb_pre` as Choice {Post, Pre}. Plus RESEARCH-R1 'Planner decisions, no sign-off needed' (one item amended: see OS2)."
---

# O-GranulatorInstrument — Stage 4 Execution Plan, Rework R1 (per-stream feedback + reverb)

## Goal

Hands-on finding F2 lands before 1.0.0, and F1 ships with it:
- **D60 / D62 / D63:** each stream has its own feedback (Amount, Low Cut, High Cut, Saturate, Diffuse, on / off) and its own reverb (Mix, Size, Damp, Pre / Post). The 8 global parameters are gone. Freeze and Reset All stay global.
- **D61 / D65:** the FX · Feedback page carries all of it in the 1200 × 800 frame, after you approve a v2 mockup.
- **D64:** the 64 presets are copied to the new parameters; 10 are re-voiced to show per-stream contrasts.
- **D66:** Record-in Arm returns to Off when a take ends or cannot start.
- **D67 / D68:** 293 parameters (200 Float / 74 Choice / 19 Bool), 150 destinations, destination table version 2.
- **F1:** the "Select a sidechain input" copy is committed, built and installed.

**What must not move.**
- `renders-2.1 / 2.2 / 2.4.sha256` (42 lines) stay byte-identical through every step. Parity stays 168 / 0 + GV 2 / 0.
- With feedback 0 and reverb 0 the engine is bitwise what it is today.
- A preset that enables one stream (38 of 64) renders bitwise as at B0 after the copy; the other 26 null at ≤ −110 dB.
- Nothing outside the R1 scope changes (scope fence below). Every wave ends with the carried regression set green (PD4).

**Out of scope (CONTEXT-R1 § Out of scope, carried CONTEXT § Out of scope):**
- A master feedback Amount, an output reverb, a link-all-streams control.
- Streams / Mod / Voice page changes beyond the renumbered destinations.
- A version bump or a tag (D56). VERSION stays 1.0.0.
- Windows hands-on, Reaper, Dorico, MPE hardware (D54, D57).
- Running the human session (verify's job, D51).
- State migration for pre-R1 sessions or user presets (sign-off 11).
- REQUIREMENTS.md edits (verify rewrites the evidence lines, PD34).

**Scope fence (the only paths R1 may change).**
`Source/{GiParamIds.h, PluginProcessor.{h,cpp}, PluginEditor.{h,cpp}, FactoryPresets.h}`, `Source/dsp/{Engine.{h,cpp}, GiModMatrix.h, GiVoiceMod.h, GiRecorder.h}`, `Source/dsp/forked/{ControlGrid.h, FORK-MANIFEST.md}` (W4 only, OS1), `Source/ui/public/{index.html, js/i18n.js}`, `tests/**`, `.planning/**`, `CHANGELOG.md`, `../../PLUGINS.md` (date cell). Not touched: `CMakeLists.txt`, `GiUiJson.h`, `GiRecordHead.h`, every other `Source/dsp/*` file, the 12 other forked leaves, `modules/**`, `scripts/**`, `modules/registry.yaml`, the v1 mockup set. A need outside the fence = STOP and ask.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/4-polish/CONTEXT-R1.md` | D60–D69. **Binding, not reopened**, except as amended by RESEARCH-R1's "Corrections to CONTEXT-R1" table and the sign-offs in the frontmatter. |
| `stages/4-polish/RESEARCH-R1.md` | Summary 1–17 (cited **RS**x), cross-slice reconciliation, sign-offs; slices **A** (DSP), **B** (CPU / byte-identity), **C** (page / gates / i18n), **D** (contract / presets / `rec_arm`), cited **A·Qx** etc. Where a slice disagrees with the top tables, the tables win. Where this plan's "Research corrections" disagree with the research, this plan wins. |
| `stages/4-polish/research-r1/` | `bench.cpp`, `tank_probe.py`, `proto.js` (Variant A / B DOM transforms), `measure*.js`, `parse_presets.py`, `cpu25.log`, hash dumps. |
| `stages/4-polish/{PLAN.md, SUMMARY.md, VERIFICATION.md}` | Carried decisions PD1–PD20 of the first pass, deviations 1–13, the Addendum (F1 / F2). PLAN.md is executed and **not edited**. |
| `stages/4-polish/HANDS-ON-SESSION-4.md` | The sheet T30 re-authors; ❌ triage rows F1 / F2. |
| `stages/3-gui/PLAN-3.3.md` | Carried gate order (its PD26), state-file rules (PD18), row naming discipline (PD21), checkpoint procedure. |
| `.planning/{STATUS.md, parameter-spec.md, params.tsv, REQUIREMENTS.md, ROADMAP.md}`, `.planning/research/ARCHITECTURE.md` (§17, §18, §20, §22) | Contracts and checksums. |
| This plugin | The files in the scope fence, plus read-only: `Source/dsp/{GiRecordHead.h, forked/FeedbackChain.h, forked/OutputLaw.h}`, `Source/GiUiJson.h`, `CMakeLists.txt`, `.planning/mockups/v1-*`. |
| Repo (run or read, never edit) | Root `CLAUDE.md` (build / install / cache clearing, commit discipline). `scripts/{serve-ui, check-i18n, i18n-fr-lint, i18n-zh-lint, check-ui-labels, boot-all-uis}.js`, `scripts/{build-and-install.sh, resolve-target.sh}`, `scripts/ui-stub/*`, the FR / zh glossaries. `modules/effects/reverb-unit/cpp/ReverbUnit.h` (1.2.0). `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`. |

**Plan-time facts** (2026-11-12, HEAD `0a5e7dee`; every anchor below re-grepped by the planner). Nothing was built or run at plan time: run results are quoted from VERIFICATION.md and RESEARCH-R1. Last commit that changed compiled source or `CMakeLists.txt` = **`63ad0c5d`**; last under `tests/` = `f0516bca`; `1b17f41c` touched only `Source/dsp/forked/FORK-MANIFEST.md`.

| File | Facts |
|---|---|
| `Source/GiParamIds.h` (527) | header comment `:33-35`; `kModDests = 126 :67`; `kModDestTableVersion = 1 :69`; `kStateVersion = 1 :70`; counts `:75-80` (`kStreamCount = 38`, `kFeedbackCount = 5`, `kReverbCount = 3`, `static_assert == 261`); `streamDefs :290`; `rec_arm :340`; `feedbackDefs :351`; `reverbDefs :364`; `boolDefaultFor :417`; registration loops `:437-441`; `kModDestIds :472` (globals `:500-501`); `static_assert == 126 :505` |
| `Source/dsp/GiModMatrix.h` (426) | dest enum `:77-82` (`StreamStride = 28`, `Feedback = 114` … `LfoRate1 = 122`); `Field` enum `:84-89` (`NumFields` = 28); asserts `:90, :92`; `isStream :93`; `fieldOf :95`; `isVoiceClass :98`; `streamFieldFromRaw :112`; `streamFieldValue :150` |
| `Source/dsp/forked/ControlGrid.h` (662, class **ADAPTED** in FORK-MANIFEST) | `kGiModDests = 126 :203`; pointers `:224-232`; binds `:323-331`; fields `:434-442`; latch `:539-547`; dest clamp `:618` |
| `Source/dsp/Engine.h` (975) | hooks `:259-325` (`reverbHardCut :265`, `chainKnobParity :268`, `sumTap :269`, `tapProbe, retProbe :272`, `lambdaRampZero :275`, `rClampOff :310`, `injectReturn :314`, `swapKillHard :325`); `wetScrubs :350`; `getRevMixForTest :625`; `getChainForTest :628`; `fbEff() :749`; chains / returns / ramps `:864-880`; substitutes `:955-957`; `reverb` + atomics `:962-967` |
| `Source/dsp/Engine.cpp` (4173) | reverb bind `:49`; prepare: reverb `:184-186`, chain controls `:188-197`, heads `:208-221`; `audibleTarget :349`; `updateChainCutoffs :500-514`; tick block `:1065-1102`; `resetTake :1227`; voice-field loops `:1912, :2018, :3386`; `destVoiceNow :1962-1965`; `:2976-2978`; `:3417-3419`; `runChains :3722` (tap filled `:3745`, `tapProbe :3748-3754`, `chain[s].process :3765`); `updateHeads :3801` (λ `:3814-3829`, run target `:3857-3860`, `parkedNow :3873`); `writeHeads :3881` (parked skip `:3895-3900`, `writeCell :3917`); `writeReverbAtomics :3926-3941`; `mixPiece :3943` (skip rule `:3945-3951`, `runChains` call `:4038`, stream terms `:4040-4095`, R96 guard `:4097-4107`, reverb `:4109-4115`, knee `:4129-4149`) |
| `Source/dsp/GiRecorder.h` (634) | header comment `:42-47`; reasons `:73`; `prepare :175` (Armed → Interrupted `:179-180`, `prevArm` seed `:211`); `beginBlock :219` (edges `:235-241`, SidechainUnavailable `:249`, held `:261`, Cancelled `:271`); Bypassed `:389`; `takeCommit :406`; `enterArmed :470`; `stopTake :540` (NoSignal `:545`, TooShort `:553`, `ready.store :566`); `toIdle :572` |
| `Source/PluginProcessor.h` (953) | `GiRecRunHold :94`; count comment `:331`; `kDests :576`; `modRouted :634`; `pollRecorder :772`; `RecPollTimer :878-882`; `RecPollThread :884` |
| `Source/PluginProcessor.cpp` (2185) | excluded params `:120`; `OGI_NO_FACTORY_WRITE :122`; poll start `:128-136`; bank stamp `:145-167`; `pollRecorder :224-232`; layout loop `:240-275` (Bool comment `:270`); `setEngineDestRanges :284-292`; `getStateInformation :641` (table version `:654`); `setStateInformation :701` (`replaceState :724`, version comment `:738`); head-display rule `:1536`; `clearSlot :1613` |
| `Source/PluginEditor.cpp` (1230) | `kSliderIds :109` (globals `:250-257`); `kComboIds :288`; `kToggleIds :361` (`rec_arm :376`); relay loops `:653-658`; attachments `:1142-1165`; 34 `withNativeFunction`; `setResizable :1190`; `setSize (1200, 800) :1191` |
| `Source/FactoryPresets.h` (1023) | `modDest :86`; `factoryPresetSpecs :107`; `validateSpec :900` (excluded ids `:909`); `customStateOf :944`; `makeFactoryPresets :971`; `bankHash :1005`. 107 quoted global ids: `feedback` 10, `fb_high_cut` 2, `fb_saturation` 1, `fb_diffusion` 10, `reverb_mix` 59, `reverb_size` 24, `reverb_damp` 1 |
| `Source/ui/public/index.html` (5122, `cmp` == `mockups/v1-ui.html`) | `.heading :106`; `.knob-cell :222`; `.knob-value-input :228`; pointer-events rule `:359`; `.fig.wide :372-373`; `.flow-arrow :381`; `#write-ro` CSS `:695-701`; `#toggle-freeze :769`; `#reset-all :848`; global band markup `:1078-1094`; module script `:1357`; `KNOB_IDS :1381`; `TOGGLE_IDS :1597`; `SEGMENT_IDS :1616`; `FORMAT :1640`; `TOOLTIP_DELAY_MS :1835`; `DEST_COUNT = 126 :1850`; `PARSERS :2334`; `attachValueEntry :2416`; `bindToggle :2651`; `bindSegmented :2663`; `onParamChanged :2956` (stream regex `:2963`, figure arms `:2969-2971`); `applyStreamStates :2998`; `collectDestinations :3041`; `isGlobalDest :3084-3088`; lane Record handler `:3835-3839`; `figBase :4022`; `svfMag :4071`; `drawStreamFigures :4076`; `drawReverbFigure :4152`; `initFigures :4251`; `modulatedNorm(114, …) :4591`; `showTooltip :4826`; canon region `let uiLanguage :4928` … `initI18n :5015`. Counts: 137 `data-dest` attributes on 133 lines (125 distinct); 18 `data-display` figures (4 filter, 4 drive, 4 ring, 1 reverb, 4 window, 1 amp); 161 knob cells |
| `Source/ui/public/js/i18n.js` (3230, `cmp` == `mockups/v1-i18n.js`; **F1 uncommitted in both**) | `I18N :44` (135 keys), `LABELS :1128` (322), `TIP_BINDINGS :2760` (446). Tips: `fb_diffusion :245`, `fb_high_cut :253`, `fb_low_cut :261`, `fb_saturation :269`, `feedback :277`, `rec-status :709`, `rec_arm :717`, `reverb_damp :781`, `reverb_mix :789`, `reverb_size :797`, `write-amount :1117`. Bindings `#knob-feedback :3011` … `#write-ro :3016` … `#knob-reverb_damp :3019` |
| `tests/render-harness/` | **454 / 0** at Stage 4 verify. `main.cpp` (1938): D rows `:628-676`, state rows `:960-1030` (`mod16_dest` 125 `:973`, version "1" `:1021`), flags `:1692-1838`, full-run order `:1855-1872` (`pg4::runAll :1870`, `rowHM1 :1871`), home re-exec `:1887`. `engine_gates_24.h` (1380): `flParams :333`, `rowFL1s :445`, `fl2TapRms :464`, `loopParams :500`, `rowFL4 :542`, `rowFL5 :591`, `rowFL7 :673`, `rowFL8 :731`, `rowFA4 :1114`, `measure25 :1202`, `c25Extra :1280`, `cpu25 :1286`. `engine_gates_24b.h` (479): local κ `:70-98`, `ckChain :261`, row list `:8-17`. `engine_gates_25b.h`: `cpu25a :911`, C25m `:923-945`. `engine_gates_26a.h` (940): `recBase :143`, `armAt :151`, OS1 `:387`, CM1 `:749-779`, CM2 `:784-798`, CM3 `:800-833`. `engine_gates_26b.h`: **`CK6a` / `CK6b` / `CK6n` `:30-31, :901-918`**, `cpu26 :1122`. `preset_gates_4.h` (1416): FP0 readers `:460, :479, :481, :541`, FP4 label `:828`, FP4b `:854`, `dumpFactoryPresets :1252`, `cpu4 :1310`. `ui_gates_32.h`: `rowsFB1 :633`. `ui_gates_33.h` (1741): `pageRule :122-143`, dest literal 119 `:856, :866, :893-894, :912, :1522`, bound 125 `:992, :1490`. Message-loop pump: `eg21::pump` `engine_gates_21.h:142` |
| Other tests | `tests/parity/ParityRows.h` (577): `fbRevBase :190`, `fbRevIds :197`, PB `:388-395`, PR `:422`. `ui_frontend_check.js` (990): constants `:143`, [16] `:807-953` (pageRule regex `:879`, pair count `:907`, expected lists `:911`). `ui_binding_probe.js` (2123): `N_PARAMS :200`, `KIND_COUNTS :201`, tally regex `:212`, mutation literal `:333`, combo count `:401`, lane row names `:983`, dest loop `:1849`. `i18n-states.json`: 47 states (`mod-matrix-widest` #3, `write-held` #42, `write-per-lap` #43). `golden/extract_spec.py` (376): stride `:276`, counts `:289-290`, 126 `:295-296`, `STREAM28` + formula `:313-323`, anchors `:325-326`, skew-row count `:328`, Bool-On list `:338-339`, final print `:376` |
| Elsewhere | `CMakeLists.txt` (255): `VERSION 1.0.0 :47`, `reverb-unit :84`, param-dump `OGI_NO_FACTORY_WRITE :221`, harness `OGI_TEST_HOOKS :233-235`, only `factory_gen.cpp` is `-ffp-contract=off` `:251-254`. `ReverbUnit.h` (522): atomic `bind :112`, `prepare :125`, `process :176` (`wantActive :182`, cold start `:186-189`, idle return `:196`), mono sum `:260`, ±2 write clip `:283`. `CHANGELOG.md` (29): one `## [1.0.0] - 2026-11-11`; Record-in `:10`, Modulation `:12`, Feedback `:13`. `PLUGINS.md:76` = `🚧 Stage 4 \| 1.0.0 \| … \| 2026-11-11`. Spec headings: stream `:89`, Record-in `:134`, Feedback `:148`, reverb `:160`, LFOs `:168`, dest table `:251`, state `:302`, Full ID list `:359` |
| State of the tree | 7 / 7 contract hashes == STATUS frontmatter (body recipe: `sed -n '/^<!-- LOCKED BODY/,$p' …/parameter-spec.md \| tail -n +2 \| shasum -a 256`). Branch `main`, 194 commits ahead of `origin/main`, no `*GranulatorInstrument*` tag. Dirty under the plugin: the 2 F1 files; 4 modified planning docs (STATUS, CONTEXT, HANDS-ON-SESSION-4, VERIFICATION); untracked `CONTEXT-R1.md`, `RESEARCH-R1.md`, `research-r1/`, `stages/3-gui/gate-report.json` (not R1's, never added). `modules/registry.yaml`: 5 uncommitted hunks on this plugin's `used_by` rows, left alone. Real home: `~/Library/O-GranulatorInstrument/Presets/Factory` holds 64 files + 2 sentinels; there is no `Presets/User` folder |

**Line numbers drift: re-grep before editing.**

---

## Research corrections (checked at plan time)

1. **`CK6` is taken.** `CK6a` / `CK6b` / `CK6n` exist (`engine_gates_26b.h:30-31, :901-918`; hook comment `Engine.h:325`), and Stage 4's FP6 is described by "the CK6 metric". The research's new click rows CK6 / CK7 / CK8 become **CK7** (toggle), **CK8** (Pre ↔ Post, with **CK8o** INFO) and **CK9** (mute with reverb on).
2. **CK9's "residual < −90 dBFS after 70 ms" clause is dropped.** Slice A wrote it for "fade and idle". Sign-off 3 chose the hybrid: the audible tail rings out. CK9 gates the click at the mute edge; the tail's end is TK1's job.
3. **Slice D's line numbers for `GiRecorder.h` and `extract_spec.py` are stale** (off by 5 to 32 lines; the symbols are right). The anchors in the table above are the current ones. Slice B's `extract_spec.py` anchors are correct.
4. **The `rec_arm` audit is larger than "41 uses".** 41 is the count of literal `rec_arm` lines in the harness headers. Takes are started through `recBase` / `armAt` (`engine_gates_26a.h:143, :151`): 95 helper call lines across `engine_gates_26a.h` (57), `26b.h` (18), `ui_gates_32.h` (15, with no literal `rec_arm`), `ui_gates_31.h` (3), `ui_gates_33.h` (2). T11 follows the helpers, not the literal.
5. **"`rec_arm` saved and restored as 0" is not adopted** (research D·Q9, tagged assumed; listed under "Planner decisions"). pluginval's "Plugin state restoration" test compares each parameter's value when the state is captured with its value after that state is restored (repo memory: signature "<Param> not restored on setStateInformation"). A save that writes 0 while the live value is 1 fails it whenever Arm is On at that moment, and the preceding parameter fuzz makes that likely. Default taken: the state functions are **not touched**; a restored On that cannot start is cleared by the sign-off-8 path. See PD20 and **OS2**.
6. **Validators against the auto-clear were not analysed.** auval and pluginval set parameters and read them back. With sign-off 8 an arm that cannot start is written back to Off within about 20 ms when the message loop runs. T14 therefore proves auval + pluginval right after the auto-clear lands, not at the end. A `rec_arm` line in either validator's output = STOP (**OS4**).
7. **"No forked-leaf edit" cannot hold literally.** `kGiModDests` and the engine's parameter pointers / latch live in `Source/dsp/forked/ControlGrid.h`, class ADAPTED. The research prescribes the edit (A · Engine API, D pitfall 5). Default taken: that one leaf changes, in W4 only, under the manifest's ADAPTED rule; the 12 others stay byte-identical. See PD12 and **OS1**.
8. **B2 and the B3 copy cannot be two green waves.** `FactoryPresets.h` names the 8 global ids 107 times; `validateSpec` rejects them the moment the IDs go. They land as separate tasks with separate comparisons inside one wave (W4).
9. **B0 raws "in scratch" do not survive the D65 stop** (a session scratchpad is per session). They go under `build/` (gitignored, durable); small text evidence is committed (PD2).
10. **Research A extends carried rows** (FL1, FL5, FA4, CK1, RV2 "Pre arm", "mute arm"). In this plan a carried row is ported by ID only; every new arm is a new row (FL1p, FL5p, FA4p, CK7, CK9), so the carried extract stays comparable to B.
11. **No carried row checks the destination class.** D1–D5 (`main.cpp:628-676`) compare ids, host names, the version and the formula; nothing compares `GiDest::isVoiceClass` with the class column of `mod-dests.tsv`, and nothing drives a per-voice source into a class-G destination inside a stream block. New rows **DC1** and **MC1** (RS11's silent failure).
12. **The `reverb_pre` tip draft (C·D69) describes one reverb that moves.** The adopted topology (sign-off 2) keeps the audible reverb and adds a second one inside the loop. Wording follows the reconciliation table, in the page's own term ("working copy", not "tape"): PD24.
13. **Sign-off 7 names English only.** French titles ("IV · Coupe-haut de réinjection") still clip in the 98 px select, as French already does today. Default: French and Chinese titles unchanged, shown at the D65 gate (**OS3**).
14. **Mockup state fields.** `.claude/skills/ui-mockup/references/state-tracking.md:271` expects a `vN-ui.yaml` finalization marker for `finalized_version: N`. R1 ships no yaml by design (C·Q11). STATUS `latest_mockup_version` / `finalized_version` / `mockup_version_synced` stay **1**; v2 is recorded in STATUS prose and in spec addendum C.
15. Minor: slice D's "133 `data-dest` occurrences" is the line count; there are 137 attributes (slice C and the probe's DM2 are right).

Everything else checked matches RESEARCH-R1: the engine already has 4 chains / taps / returns / heads with shared control ramps only; stride 28 → 36 and the 150-entry layout; the tick, head, reverb and knee statements; the module's gating, mono sum and ±2 clip; the 12 arm endings; the bank census (10 / 59, 38 / 24 / 1 / 1 streams); the page counts; the gate constants; the F1 diff (2 files, 12 lines each); the registry hunks.

---

## Open sign-offs

Defaults are taken so execute is not blocked. T6 asks for OS1, OS2 and OS3. OS1 and OS3 govern nothing before W4. OS2 governs T12, which is not blocked: answer it when resuming from T6, otherwise the default is built.

| # | Question | Default taken | Alternative |
|---|---|---|---|
| **OS1** | The stability rule says no forked-leaf edit. R1 cannot be built without changing `forked/ControlGrid.h` (ADAPTED): it holds `kGiModDests` and the 8 global pointers / fields / binds / latches. | Edit that one leaf in T17 only: `kGiModDests` 150; the 8 globals move into `Stream` with `fbOn` and `reverbPre`; `freeze` stays. FORK-MANIFEST column 5 re-hashed, column 7 gains the R1 line. Drift check must read 1 / 13 before the manifest update and 0 / 13 after. The other 12 leaves do not move. | None that avoids the file (`kGiModDests` lives there). A smaller diff is possible (count only, per-stream fields in a new non-forked header), at the cost of a split parameter struct. |
| **OS2** | Research planner decision: save and restore `rec_arm` as 0. | **Not adopted** (correction 5). State functions untouched; a session saved while armed restores On, and the sign-off-8 path clears it at the next `prepare` (row 11). A restore while audio is running still produces an On edge, as today. | A restore-swallow in the recorder (a generation the processor bumps in `setStateInformation`; the audio thread re-seeds `prevArm` and ends the arm). Keeps state faithful and guarantees a restore never starts a take. About 15 lines + a row; say so at T6 to add it to T12. |
| **OS3** | French and Chinese destination titles for the four `fb_*` knobs. | Unchanged. The D65 material includes the MOD tab in French with a feedback destination selected. | Shorten French to the glossary's short form ("Coupe-bas réinj." etc.); fr-lint decides. |
| **OS4** | If auval or pluginval reports `rec_arm` as not retained / not restored because of the auto-clear (rows 9 and 11 of the endings table). | Not pre-decided: T14 **stops and asks**. | Candidates to choose from then: clear rows 9 / 11 only while an editor is open; delay those two clears; or accept a documented validator note. |

---

## Plan-phase decisions

### Order, baselines, gates

**PD1 Landing order (RS9) mapped to waves.**

| Step | Wave | What lands | Compared with |
|---|---|---|---|
| B0 | W0 | Harness dump / compare modes only | The unmodified tree is the reference |
| B1 | W2 | All DSP, on the old parameter IDs, new controls at their defaults | B0: hashes, full extract, parity lines, 64 preset renders |
| B2 | W4 | Parameter surface (293, dest 150, version 2), spec, goldens, harness port, page | B1: every ported row reproduces its B1 value |
| B3 copy | W4 | Bank on the new IDs (wildcards) | B0 raws: 38 bitwise, 26 at ≤ −110 dB |
| B3 showcase | W5 | 10 re-voiced presets | They leave the comparison by name; the other 54 still compare to B0 |
| B4 | W5 | New rows only | — |

`rec_arm` (W3) is independent of B1–B4 and sits between B1 and B2 because it is not blocked on D65.

**PD2 Baseline storage.**
- `$B0` = `build/plugins/O-GranulatorInstrument/r1-b0/` (gitignored, survives a session change): 64 raw `.f32` renders (about 246 MB) + their sha256 list. `$B1a` beside it (T8's dump, the reference for T9).
- Do not delete `build/` during R1. If `$B0` is lost → STOP and ask (regenerating means building commit C1's tree elsewhere).
- `$E` = `.planning/stages/4-polish/evidence-r1/` (committed, text only): `B.ids.txt`, `B.full.txt`, `b0-preset-renders.sha256`, `b0-eg24.txt`, `b0-parity.txt`, `b0-cpu.txt`, then one `B1.full.txt` / `B2.full.txt` per step and the gate logs worth keeping. These are evidence, not goldens: no carried gate reads them.

**PD3 Extract comparison.** A harness run prints `[PASS|FAIL] <row id> <label and measured detail>`.
- `ids` = `PASS|FAIL` + the first token of the label, sorted. For the 454 carried rows it must equal `B.ids.txt` at every step: same IDs, all PASS.
- `full` = the carried extract of PLAN-3.3 T1 (sorted rows without timing detail). Its diff against the previous step is allowed **only** on that step's listed rows:
  - **B1 list (measured values only):** multi-stream rows with reverb on: FL1, CL1 (the ±2 tank clip: printed peaks move, thresholds hold), FL5, FL7, FA4, DR1, the NZ rows, FP2, FP5, FP6. Before T8, read each row and record its stream count; a row that is single-stream must not move.
  - **B2 list (label / count text):** the port table of PD16.
  - Any other row moving = STOP and attribute it to the task just landed.
- Every move is recorded old → new in SUMMARY-R1.

**PD4 Carried regression set.** Run at the end of every wave that changes `Source/` or `tests/`; a wave that ends red does not commit.
- **R-core:**
  1. Harness, param-dump, VST3, AU and Standalone targets build, rc 0, 0 warnings in `Source/`.
  2. Full harness (BG): PD3 holds; new rows PASS; every `n` control FAILS its predicate; 0 NOT FIRED; HM1 green.
  3. `--dump-render-hashes` == `renders-2.4.sha256`; `--dump-render-hashes-legacy` == `renders-2.2.sha256`; its 7 `-pinned` lines == `renders-2.1.sha256`; both print `ctrl FIRED`.
  4. `tests/parity/run.sh`: 168 / 0 + GV 2 / 0; the PB / PR residual lines equal `b0-parity.txt` digit for digit.
  5. param-dump == `.planning/params.tsv` (the current pin).
  6. `tests/fork-drift-check.sh`: 0 / 13.
  7. `--dump-ui-overrides` twice, byte-identical, == the committed file.
  8. `node tests/ui_frontend_check.js` ALL PASS; `--nc` 4 / 4 fail.
  9. The listings of `~/Library/O-GranulatorInstrument` and `~/Music/Ouaricon` are unchanged by the harness and the param-dump console.
- **R-ui** (waves that change the page, i18n, parameters, editor tables, telemetry or `tests/*.js`: W3 for the probe, W4, W6): the carried order of PLAN-3.3 PD26: overrides → `ui_frontend_check.js` → `ui_binding_probe.js` (+ `--nc`) → check-i18n → fr-lint → zh-lint → check-ui-labels → boot-all-uis `--strict-tips`; then `tests/preset_name_fit.js`.
- **R-val** (W3, W6): install per CLAUDE.md, auval, pluginval 10 VST3 + AU with the sidechain on and off, shipping build restored.
- Exit 77 anywhere = not run = not a pass.

**PD5 The D65 gate.**
- Mockup file set: `.planning/mockups/v2-ui.html` (the whole page), `v2-i18n.js`, `v2-i18n-states.json`. **No** `v2-CMakeLists.txt`, `v2-PluginEditor.*` or `v2-ui.yaml`. The ui-finalization agent is not run (it emits the CMake and editor templates; a mockup CMake that names `juce_add_plugin(` in a comment breaks `resolve-target.sh` once landed).
- Forked after C0: `v2-ui.html` from `Source/ui/public/index.html`, `v2-i18n.js` from `Source/ui/public/js/i18n.js` (so F1 is in), states from `tests/i18n-states.json`.
- The page is authored natively as **Variant B**. Variant A is applied by one `<script>` block and one switch control, each line carrying the token `MOCKUP-ONLY` (ported from `research-r1/proto.js`). Landing requires `grep -c MOCKUP-ONLY` == 0.
- Served from a scratch tree `$M` = `build/ui-mockup-v2/`: `scripts/` copied, `modules/` symlinked, the plugin's `CMakeLists.txt`, `Source/PluginEditor.cpp` and a `cp -R` of `Source/ui/public/` (not `git archive`: the configure-copied `modules/*.js` are gitignored) with `index.html` := v2 and `js/i18n.js` := v2, the v2 states file as `tests/i18n-states.json`, plus **draft** `.planning/params.tsv` (293 rows) and **draft** `tests/ui-stub/generic-overrides.json` generated by a scratch script from the committed files. Drafts never enter the repo.
- **Blocked on approval:** every edit of `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js` and `tests/i18n-states.json`; and, because `ui_frontend_check.js` [8] is a four-way closure (param-dump == `params.tsv` == editor tables == page binders) that a wave may not leave red, the whole parameter surface: T15–T22, then W5–W7.
- **Not blocked:** W2 (B1) and W3 (`rec_arm`). They touch no page file and no parameter ID.

### DSP (B1)

**PD6 Control ramps ×4 (RS1).** Per stream: `fbRamp`, `fbMod` / `fbNow` / `useFbMod`, `diffRamp` + substitute, `satDRamp` + substitute, `fbLowLogRamp`, `fbHighLogRamp`, `chainLowHz` / `chainHighHz`, `parityKnob`; `fbEff (s)`.
- In B1 all four copies read the global values through the same statements (`setTarget (·, rampCells20)`, `tick`), so every sample is bit-equal.
- `updateChainCutoffs` gets four dirty checks with four cached pairs. One shared pair would leave three chains without coefficients.
- No chain skip. Chains, `fbRet`, heads, `run` ramps and `returnPk` are already per stream and are not touched.

**PD7 Audible tanks ("tank A", RS2, RS7, sign-off 3).**
- `ReverbUnit reverbA[4]` on each stream's term, before the stream sum. Per-stream `enabled`, `size`, `damp`, `mix` atomics; `predelay`, `mod`, `shimmer` shared constants.
- Each stream's term is stored (`ys[s][2][kCell]`), guarded for non-finite samples **only while that tank's gate is on**, processed in place, then summed in today's order with today's `have` logic. The "no stream audible → stream 0's term is the output" fallback goes through tank A of stream 0.
- The sum-level R96 guard (`:4097-4107`) and `wetKnee` → `output_gain` (`:4129-4149`) stay as they are, once, after the sum.
- Gate A, written at every grid tick with no dirty check in front of it: `streamLive or tailPk > 1e-5`.
  - `streamLive` = `enable && audibleTarget (s, anySolo)`, or the stream's audibility ramp not settled at 0.
  - `tailPk` = held peak of the tank's own output from the previous cell with the existing 50 ms release (`pkRel`).
- `process` is called for all four tanks on every piece. A tank that is not called cannot finish its 50 ms fade and resumes a stale tail.
- A stream's term joins the sum iff `audible[s]` or its tank is live. With Mix 0 a tank is never live, so the 2.1 skip statements are untouched.
- No input-energy idle on an audible stream (it would restart the tank LFOs and break single-stream bit-identity and the PR parity rows).

**PD8 Loop tanks ("tank B", RS3, RS4, sign-offs 2 – 5).**
- `ReverbUnit reverbB[4]` on `tapBuf` in `runChains`, after the tap is filled (`:3745`) and before `chain[s].process` (`:3765`). Same Size / Damp / Mix atomics as tank A, its own `enabled` atomic. The `tapProbe` block moves to after tank B.
- A finite guard on `tapBuf` before tank B, only while gate B is on, with a counter. A NaN inside a tank is permanent.
- Gate B, derived at every tick: `reverbPre && fbOn && audibleTarget && ! fbEff (s).settledAt (0) && preHold[s] == 0`.
- Pre ↔ Post = gate B. The module's 50 ms active-gain ramp is the crossfade; no engine crossfade, no flush call.
- Reset: a take of slot s with `resetTake` true (`:1227`) sets `preHold[s]` to `rampCellsFor (fs, 60 ms)` cells; the tank fades, idles and restarts cold. The audible tail is not touched.
- Boundedness in Pre rests on the writer (`GiRecordHead.h:181-198`: clamp to ±0.7, then into `[min (wc, rw), max (wc, rw)]`), not on the AGC. `GiRecordHead.h` is not edited.
- Shipped as is (sign-off 5): in Pre the loop can sit at the 0.7 ceiling, and the head keeps writing for the tail's length after note-off. INFO row + hands-on row. The −60 dBFS run threshold for Pre streams is a documented fallback only, not built.

**PD9 Feedback toggle (RS5).** `fbOn` joins the head's run target (`:3857-3860`): Off is a per-stream Freeze: 20 ms `run` ramp, park, no writes; Amount is kept and a routed Amount cannot lift it. Off from load starts parked. Amount 0 stays a bitwise no-op through the parked-head skip (`:3873`, `:3895`), now with `fbEff (s)`.

**PD10 Bit-identity rules for the edit itself.**
- Keep the per-stream term statements and the `yL = tL` / `yL += tL` accumulation exactly as written; never fold a tank's output into the same expression as the sum (the engine is not built `-ffp-contract=off`).
- Never add an inaudible stream's `+0.0f` term (`−0 + 0 = +0` breaks the 2.1 hashes).
- Gates A and B, `revTail`-style liveness and `preHold` are derived unconditionally at the tick, never behind the cutoff dirty check.
- In B1 the new per-stream controls are engine-private (`fbOn` true, Pre false, fixed); they bind to parameters in T17. **B1 moves no byte under `Source/dsp/forked/`.**

**PD11 Hooks and accessors** (all under `OGI_TEST_HOOKS`; X2 must stay 0).
- Ported: `reverbHardCut` per stream, `chainKnobParity`, `lambdaRampZero`, `getRevMixForTest (s)`.
- New: `fbOnHardStop`, `preHardCut`, `revTailOff`, `preHoldOff`, `tankGuardOff`, `gateBIgnoreAudible`, `tankGateSticky`, `tankOnSum`, `addZeroTerm`, `dropTankTerm`, `destClassLegacy`; counters `tankRunCells[4]`, `tankBRunCells[4]`, `tankProcessCalls`, `tankGuardScrubs`.

**PD12 Modules and forked leaves.**
- `reverb-unit` stays 1.2.0, unedited: 8 instances, no mutable statics, about 393 KB each at 48 kHz, allocated in `prepare` on the host thread.
- The 11 VERBATIM leaves (`FeedbackChain.h` among them) and `EngineConstants.h` (ADAPTED) are not edited.
- `forked/ControlGrid.h` (ADAPTED) per OS1.

### Parameter contract (B2)

**PD13 IDs, types, order (D68, sign-off 12, RS10).**
- Ten rows appended to `streamDefs()` (38 → 48), in this order: `fb_enable` (Bool, default **On**, host name "Feedback On"), `feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion`, `reverb_pre` (Choice {Post, Pre}, default 0, host name "Reverb Position"), `reverb_mix`, `reverb_size`, `reverb_damp`. Float ranges, defaults and skew centres are copied verbatim from today's globals (`GiParamIds.h:354-369`).
- `feedbackDefs()` and `reverbDefs()` are deleted. 293 = 9 + 5 + 8 + 192 + 7 + 20 + 4 + 48 = **200 Float / 74 Choice / 19 Bool**.
- New indices: s1 22–69, s2 70–117, s3 118–165, s4 166–213, rec 214–220, LFO 221–240, macros 241–244, mod 245–292. Every parameter stays `ParameterID { id, 1 }`.
- `boolDefaultFor` already handles a default-On Bool through `def->def`; the "every other Bool Off" comments are corrected.
- `rec_arm`, `freeze`, `kStateVersion` unchanged.

**PD14 Destination table (D67, RS11).**
- Base `b = 2 + 36·(N − 1)`. Fields 0–27 unchanged; 28 `feedback`, 29 `fb_low_cut`, 30 `fb_high_cut`, 31 `fb_saturation`, 32 `fb_diffusion`, 33 `reverb_mix`, 34 `reverb_size`, 35 `reverb_damp`.
- s1 2–37 (new 30–37), s2 38–73 (66–73), s3 74–109 (102–109), s4 110–145 (138–145), LFO rates 146–149. Total 150.
- Class: the 32 new entries are **G** (per stream, not per voice). `isVoiceClass (d) = isStream (d) && fieldOf (d) < 28`. The low-diff spelling keeps `Field::NumFields` = 28 as the voice-field count (the loops at `Engine.cpp:1912, 2018, 3386` then need no edit) and adds the 8 stream-global fields and `StreamStride = 36` beside it. Also bounded: `Engine.cpp:2976`, `:3417`, `GiVoiceMod.h:322`.
- Global set for the mono badge: `d == 1`, or `2 ≤ d ≤ 145 && (d − 2) % 36 ≥ 28`, or an LFO rate 146–149 while that LFO is Global (base 145).
- `kModDestTableVersion` = **2**. No remap code. `setStateInformation` keeps ignoring the version (sign-off 11).

**PD15 Spec, extractor, goldens.**
- `parameter-spec.md`: body rewrite + header **addendum C** (R1, D60–D68): the one-time break of the append-only rule, table version 2, the auto-clear, and that the spec's own "new mockup version and re-finalization" rule is met by the approved v2 mockup (no yaml, correction 14).
- Body edits (re-grep; headings in the facts table): mockup references for the FX page; total and group table; the `rec_arm` convention sentence (auto-clear from the processor's message-thread timer, never from a listener); the stream section (38 → 48, `28 × (N − 1)` → 36, the `fb_enable` default exception, 10 new rows with Mod dest 30–37); the `rec_arm` row and the Record-arming paragraph; the Feedback and Output reverb sections removed (their Design Sync S3 text moves to the stream rows); LFO dest 145 + N; the global set; 126 → 150; the grid; the state section (count 293, `modDestTableVersion` 2); the Full ID list; Design Sync notes S3 / S7.
- `tests/golden/extract_spec.py` is edited in the same task: stride 36, `(200, 74, 19)` and 293, 150, the stream formula with the 8 new suffixes, anchors `dest_ids[30] == 's1_feedback'` and `dest_ids[146] == 'lfo1_rate'`, `setSkewForCentre` rows 19 → **25**, Bools defaulting On = `s1_enable` + the four `sN_fb_enable`, the final print.
- `tests/golden/{params.tsv, mod-dests.tsv}` are regenerated **from the spec, never from the code**. `mod-sources.tsv` must come out byte-identical.
- STATUS `parameter_spec` and `parameter_spec_body` are re-hashed in the same commit, with the reason.

**PD16 Harness and parity port.** 71 lines in 10 render-harness files + 12 in `ParityRows.h` (B·Q6 table), plus the counts (D · Hard-coded counts).
- **Rule:** a row that set a global `x` sets all four `sN_x` (helper `all4`). Four identical streams are the old global, so a ported row reproduces its B1 value.
- **Routes:** a route to an old global destination becomes a route to stream 1's (`GiDest::stream (0, field)`). Single-stream rows reproduce B1. A multi-stream row whose value moves for that reason is listed with the reason.
- `ParityRows.h` gets an ours-side ID map only. `tests/parity/parent_render.cpp` and the parent `.f32` set are not touched.
- **Amended rows (same ID, PASS, text moves):**

| Rows | Change |
|---|---|
| G1–G3 | 261 → 293; 176 / 70 / 15 → 200 / 74 / 19 |
| D1–D5 | 126 → 150; D4 version 2; D5 formula `2 + 36 (N − 1)` with the 36 suffixes |
| S1a, S2 | 293 values; `mod16_dest` 149; `modDestTableVersion` "2"; 293 PARAM children |
| N2, N2s, FN3, FL7 read sets | `bound == 293 && touched == 7`; FL7's randomised bound count |
| AU1 | over 293 parameters; the drift set must still be the 4 LFO rates |
| FP0 | coverage readers become per stream: a range rule holds when **at least one enabled stream** meets it (identical to B for the 54 copied presets) |
| FP4, FP4b | 259 → 291 random parameters; FP4b sets all four `sN_feedback` |
| UPM rows | 259 → 291 keys; 261 → 293 values |
| DR1 | 125 / 125 → 149 / 149 |
| UMN1, UMB1 and the `ui_gates_33.h` literals | destination 119 → 35 (`s1_reverb_mix`); key set `{1, 2, 3, 35}`; bounds 125 → 149; `pageRule` = the PD14 modulo rule |
| UFB1, UTL32, USV1, UPM6c, TL rows | `feedback` → all four |
| C25w, C25r, C25w96, C25m, C26rec, C25mpe | `c25Extra` sets all four streams' feedback, diffusion, saturation, Mix and Size; C25m's route table ported |

**PD17 Param-dump and overrides re-pin.**
- `.planning/params.tsv` is regenerated from the param-dump console. Check by script, joined on id: 253 surviving rows are identical in every cell except the index; 8 rows are gone; the 32 new Float rows equal their old global row in every cell except index, id and name; 4 Bool + 4 Choice rows are new. Anything else = STOP.
- `tests/ui-stub/generic-overrides.json` is regenerated twice, byte-identical; its diff is limited to the 8 global entries out and the 40 new ids in.

### Record-in Arm (D66)

**PD18 Mechanism (RS13).**
- `GiRecorder`: two relaxed atomics. `armGen` is incremented on every accepted On edge (including the unavailable and held cases). `endGen.store (armGen)` at every ending in PD19.
- Message side, in `RecPollTimer::timerCallback` after `pollRecorder()`: when `endGen` moved since the last check, `endGen == armGen` and the parameter reads On → `beginChangeGesture`, `setValueNotifyingHost (0)`, `endChangeGesture`.
- Timer path only. Never from `parameterChanged`, never an `AsyncUpdater` from the audio thread, never the editor poll (it must work with the editor closed), never on `RecPollThread`.
- The one-shot rule (D34) in the audio path is unchanged; D66 only adds the write-back.
- The `endGen == armGen` guard keeps an arm made during Committing (held, then Armed) from being cancelled by the previous take's clear.
- No page change: the relay carries the host-visible write, `onParamChanged ('rec_arm')` re-renders, and the lane Record handler (`index.html:3835-3839`) works as written once Arm is Off.

**PD19 Endings that clear (sign-off 8).** Rows of the D·Q9 table: 1 commit, 2 NoSignal, 3 TooShort (when still On), 6 `prepare` while Armed, 7 `prepare` while Recording, 8 bypass while Armed or Recording, 9 arm with no sidechain bus, 10 capacity overrun, 11 a stale On seeded at `prepare` (increment `armGen`, then end it). **Not** on Cancelled (the user already wrote Off). An arm that is waiting (Threshold, Next Note) never clears.

**PD20 Saved state (correction 5, OS2).** `getStateInformation` and `setStateInformation` are not edited. Presets still never carry `rec_arm` (excluded, unchanged; do not "fix" a stale On by removing the exclusion).

**PD21 Audit before the rows (D pitfall 6).** T11 lists every row that turns Arm On (literal, `recBase` / `armAt`, or the `engine_gates_21.h:341-353` randomiser) **and** pumps the message loop before it reads Arm, a take count or recorder state. Known pump sites: `engine_gates_21.h:1796, 1880, 1898, 1996, 2045, 2052, 2221`; `engine_gates_24.h:956-1081`; `engine_gates_26a.h:767-833` (CM1–CM3); `ui_gates_32.h:1348`. OS1 does not pump: it stays true at engine level and its label says so. CM1's Timer arm will find Arm Off afterwards. Wording fixes land before the implementation, so a later failure is not mistaken for a regression.

### Page

**PD22 Layout (sign-offs 1, 6; C·Q7).** Variant B at the measured numbers of C·7.4:
- One section per row. Rows 1–3 and 5 are three knobs plus a title cell (70 × 28 box, two lines at most) in the slot the figure leaves; rows 1–3 at 88 px pitch.
- Row 4: the five feedback knobs in chain order at 57 px pitch (`.knob-cell.n5`: label and readout boxes 56 px) under a heading line with the on / off switch (`.tog.sw` 32 × 20). The switch and the segments are siblings placed over the heading line, not children (headings are `pointer-events: none`).
- Row 5: Reverb, with a vertical Pre / Post segment.
- Row 1's title is one word (`h.filter`). The filter-type segment is vertical.
- The five feedback cells take `.off` while `sN_fb_enable` is off (`applyStreamStates` + the `onParamChanged` regex).
- All 13 figures on this page go (`drawStreamFigures`, `drawReverbFigure`, `svfMag`, the three figure arms, `.fig.wide`, `.flow-arrow`); the 4 window figures and the amp figure stay. `#write-ro` goes with its CSS and `modulatedNorm(114, …)`. No C++ change; 34 native functions.
- Freeze and Reset All do not move (header and lane strip).
- Variant A (D61's literal 4 + 4 with an 80 px cell) exists only behind the mockup switch. If you pick A at T6, T15 re-authors the inline layout as A.

**PD23 Page tables and the mono rule.**
- `KNOB_IDS` 144 → 168; `FORMAT` 176 → 200 keys; `TOGGLE_IDS` 15 → 19; `SEGMENT_IDS` 15 → 19 (`sN_reverb_pre` binds through the existing `bindSegmented`); `DEST_COUNT` 150.
- `data-dest`: 137 → 161 attributes; 98 change value (90 on s2–s4, 8 on LFO rates), 8 go, 32 arrive with `data-dkind="stream"` and a `data-dkey`. No new destination strings: names come from the tip titles.
- The mono rule changes in **one task, three files plus the engine**: `isGlobalDest` (`index.html:3084`), `pageRule` (`ui_gates_33.h:122`), the regex + pair count + expected lists in `ui_frontend_check.js` [16], and `isVoiceClass` (PD14). The frontend check matches the C++ `if` text by regex: reword both together.
- Nothing between `let uiLanguage` and the end of `initI18n` changes (check-i18n [6] byte-compares it).

**PD24 Strings (D69, sign-off 7).**
- **LABELS** 322 → 310: 19 die (`fig.bandpass`, `fig.bypass`, `fig.clean`, `fig.decay`, `fig.dry`, `fig.highpass`, `fig.lowpass`, `h.feedbackNetwork`, `h.outputReverb`, `label.fbNoteWc`, the 7 `label.write*`, `h.grainFilter`, `h.crush`); 7 are born (`h.filter`, `h.driveCrush`, `h.feedback`, `h.reverb`, `opt.reverb.pre`, `opt.reverb.post`, `aria.fbOn`).
- **I18N** 135 → 136: `write-amount` dies; `fb_enable` and `reverb_pre` are born.
- **TIP_BINDINGS** 446 → 477: 9 rows out, 32 knobs + 4 switches + 4 segments in.
- **Bodies changed** (en / fr / zh-Hans): `feedback`, the four `fb_*` ("this stream's working copy"), the three `reverb_*` ("this stream's reverb"), and `rec_arm` (one sentence: Arm switches off when the take ends).
- **English titles shortened:** "FB Low Cut", "FB High Cut", "FB Saturation", "FB Diffusion". "Feedback" stays.
- **`reverb_pre` body (English draft):** "Post: the reverb tail is not written into this stream's working copy. Pre: a second reverb sits inside the feedback loop, so the tail is written back." French and Chinese follow the glossaries (réinjection / 反馈, réverb / 混响).
- New French is `reviewed: false`, new Chinese `reviewed: 'mt'`. Texts are the C·D69 table unless a lint gate objects.
- Dead keys and dead binding rows are removed in the same edit as the markup (check-i18n [15], boot-all-uis `--strict-tips`).

**PD25 States.** 47 → **48**: `write-held` and `write-per-lap` are removed; `mod-matrix-widest` is re-picked by measurement after the renumber and the shorter English names; `fb-off`, `reverb-pre` and `entry-open-fb-row` are appended at the end (the 3.3 rule: new states go last; `__giState` exists only after `rec-armed`). `entry-open-fb-row` re-opens its field after each `__setLanguage`, as `entry-open-hdr-54px` does.

**PD26 Pair rule.** Until T21 the pairs are `index.html` ↔ `v1-ui.html` and `js/i18n.js` ↔ `v1-i18n.js`. From T21 they are `index.html` ↔ `v2-ui.html`, `js/i18n.js` ↔ `v2-i18n.js` and `tests/i18n-states.json` ↔ `v2-i18n-states.json`. The v1 files freeze at the fork point; `cmp index.html v1-ui.html` fails by design after landing and is no longer a gate.

### Bank

**PD27 Copy (D64, RS12).** The 107 quoted global ids become wildcard keys (`"s*_feedback"` …). One helper, `expandSpecValues()`, turns an `s*_x` pair into four `sN_x` pairs in place; it runs at the top of **both** `validateSpec` and `makeFactoryPresets`. A later literal `sN_x` pair overrides the wildcard. Toggles On and reverb Post are the defaults and are not written. The copy covers disabled streams too. Names, seeds and routes are untouched (routes are by id).

**PD28 Showcase set (sign-off 10).** Names and seeds stay; values and descriptions change. Level is re-trimmed with `sN_level`, never `output_gain`. No Keys preset.

| Preset | Contrast |
|---|---|
| Drones · Deep Well | S1 feeds back dark and diffused (70 %, high cut 4 k); S2 noise bed feedback Off, light reverb |
| Drones · Organ Loft | S1 reverb Post; S2 reverb Pre with feedback 30 % |
| Drones · Night Engine | S1 feedback 55 % dry; S2 feedback Off, reverb 70 % |
| Expressive · Bend Choir | S1 vowel close (Mix 15 %); S2 halo (Mix 65 %, Size 85) |
| Live · Four Lanes | S1 dry, S2 small room, S3 feedback 40 % + diffusion, S4 reverb Pre wash |
| Motion · Phase Garden | a 7th route LFO → `s2_reverb_mix`; S1 at a fixed Mix |
| Pads · Fifth Bloom | S1 25 %, S2 50 %, S3 80 % with reverb Pre |
| Pads · Slow Tide | S2 reverb Pre + feedback 30 %; S1 Post |
| Rhythmic · Triplet Rain | S1 tight and dry (5 %), feedback 0; S2 washed (45 %) |
| Textures · Broken Radio | S1 feeds back saturated and band-limited (50 %, sat 60, high cut 3 k); S2 dry |

Alternates if one cannot meet FP2 or `--cpu4`: Drones · Low Orbit, Textures · Rust Cloud. A swap is recorded.

**PD29 Preset comparison rows.** Two harness modes in `preset_gates_4.h`, outside the full set (they need `$B0`; a missing directory exits 77):
- `--dump-preset-renders <dir>`: per preset, a fixed 10 s render on FP2's note script and lane setup; the raw `.f32` and one sha256 line. LF line ends.
- `--cmp-preset-renders <dir>` prints, per preset, the count of differing samples, the null in dB and the gain. It judges in one of two ways:
  - with `--all-bitwise`: **PT0**, 64 / 64 bitwise. Used where nothing may move at all (T3 on its own dump, T7, and T9 against `$B1a`).
  - **PT1:** every single-stream preset is bitwise equal to its B0 raw (`differ == 0`).
  - **PT2:** every multi-stream preset nulls at ≤ −110 dB (residual RMS re the B0 RMS) with least-squares gain `|g − 1| ≤ 1e-5`, **and** `differ > 0` (a build that silently kept one output tank would be bitwise on all 64).
  - Controls: **PT2n** `dropTankTerm` on stream 2 → fails; from T20 on, **PT2n2** `s2_reverb_size` + 1 on a two-stream preset → worse than −60 dB.
  - Expected counts: 38 + 26 through T20; 38 + 16 from T23, when `--exclude-showcase` drops the 10 names of PD28 (one constant beside the rows; the mode prints them).

### Proof rows and CPU

**PD30 New harness rows.** New header `tests/render-harness/engine_gates_r1.h`, namespace `egr1`, flag `--egr1`, in the full run after `pg4::runAll()` and before `rowHM1()`; the carried order is unchanged. Every row asserts its stimulus fired before its result. Every `n` control runs the same predicate on a deliberately broken input and must FAIL it. Every per-stream row has an arm on a stream ≥ 2.

| Row | Stimulus | Pass | Control |
|---|---|---|---|
| **FL1p** | FL1 parameters, Pre ×4, 60 s, full-scale noise tapes | 0 bound violations vs max (pristine peak, 0.7); 0 non-finite / subnormal in output, tapes (all mip levels), chains. Validity: chain-input probe peak > 0.7 on ≥ 1 slot, else INVALID | FL1's scanners (denorm_min, NaN, bound + 1 ulp) |
| **FL1sp** | Pre, Mix 100, Size 100, saturation 0, 0.5-peak tapes, 60 s | written ≤ 0.7f exactly on 4 slots; validity `rClamps > 0` | `rClampOff`, 10 s, **no** `injectReturn` → written > 0.7. If it does not fire, raise the stimulus |
| **FL2p** | FL2 geometry, Pre, Mix 100, Size 50, tape noise 0.05 | chain-input RMS at 16 voices ≤ 1 voice + 1 dB | `sumTap` → ≥ +6 dB |
| **FL3p** | FL3 + Pre, Mix 60, Size 100; Freeze while the tail rings | hashes after ramp + settle == 10 s later; writes frozen | the hash at the Freeze edge differs |
| **FL4p** | FL1 parameters, Pre, Mix 100, feedback 0, 60 s | 4 / 4 hashes == pristine, 0 writes | feedback 0.1 % → differ |
| **FL4t** | feedback 100, `sN_fb_enable` Off ×4 from load; Post and Pre arms | 4 / 4 == pristine, 0 writes, 0 parks-with-settle | toggle On → differ |
| **FL4m** | toggle Off + a Macro route at depth 100 % onto `sN_feedback` (s1 and s3) | 0 writes | the same route with the toggle On → writes > 0 |
| **FL5p** | Pre on s2 and s4, feedback 100, Mix 60; Pre ↔ Post and toggle switches mid-render | FL1 bound holds, 0 non-finite; tapes 1 and 3 bitwise equal to a no-switch render | switch s1 instead → tape 1 differs |
| **TG1** | Pre; NaN injected on one voice bus with `nanKillOff` | tape, chain and later output finite; guard counter > 0 | `tankGuardOff` → non-finite persists |
| **RS2p** | Pre, Mix 60, Size 100; notes off; Reset s1 while the head still runs on the tail | s1 == pristine 200 ms after the take and 2 s later; s2–s4 unchanged | `preHoldOff` → s1 != pristine |
| **MU1** | Pre, feedback 100, Mix 60; mute s1 mid-phrase (arm on s3) | that slot's `writes` frozen from t + 0.5 s; the others keep writing | `gateBIgnoreAudible` → writes continue |
| **IN1** | 4 streams with different feedback + reverb values; change each of s1's 10 new parameters (arm on s3) | the other three tapes and a solo render of another stream are bitwise unchanged | change that other stream → differs |
| **SI1** | 2 streams, `s1_reverb_mix` 50, the rest 0 | == stream 2's dry render + stream 1's wet render, null ≤ −120 dB | `tankOnSum` → fails |
| **TK1** | A stream sounds, stops, sounds again | `tankRunCells[s]` > 0 while it sounds and 0 from 2048 samples after `tailPk` falls; the re-entry tail is bit-equal to a never-ran control; all four tanks called every piece | `tankGateSticky` → the counter never returns to 0 |
| **MZ1** | Render job N1 with the per-stream path live | hash == `renders-2.4` line | `addZeroTerm` → differs |
| **FA4p** | Pre On / Off ×4, toggle switches, four tanks re-enabled in one cell | 0 allocations on the audio thread | FA4's control |
| **CK7** | toggle Off, then On, on `ckChain()`; position 50 %, spray 100 %; at a tick | output κ ≤ 2.0× static (local 5 ms-frame κ) | `fbOnHardStop` > 2.0× |
| **CK8** | Pre ↔ Post: sine tape, feedback 100, Mix 60, Size 50; measured on the return probe | κ (return) ≤ 2.0× a static-Pre render with the same tank history | `preHardCut` > 2.0× |
| **CK9** | s1 Mix 50, Size 50, sine tape; mute at a tick | output κ ≤ 2.0× static | `revTailOff` > 2.0× |
| **DC1** | all 150 destinations | `isVoiceClass (d)` == (class column of `mod-dests.tsv` is V) | `destClassLegacy` → 32 mismatches |
| **MC1** | Velocity → `s2_reverb_mix`; Mod Env 1 → `s3_feedback` | the stream's engine value moves and follows the newest voice (D4) | `destClassLegacy` → no movement |
| **UHD1** | Freeze on; `s2_feedback` > 0; `s1_feedback` 0; `s3_fb_enable` Off | head shown dashed on lane 2 only | a rule that reads stream 1 only → fails |
| **AC1–AC8** | PD31 | | |

INFO lines (not rows): **CK8o** (the Pre ↔ Post switch on the output), **FL8p** (FL8 in Pre: chain-input and tape peak; FL8 itself stays Post-only), **PreTail** (Pre, Size 100, Mix 60: seconds the head writes after note-off; return peak 10 s later).

Names grepped at plan time, 0 hits each: FL1p, FL1sp, FL2p, FL3p, FL4p, FL4t, FL4m, FL5p, FL8p, TG1, RS2p, MU1, IN1, SI1, TK1, MZ1, FA4p, CK7, CK8, CK8o, CK9, DC1, MC1, UHD1, AC1–AC9, PT0, PT1, PT2, FD1, VE15, `egr1`. T1 greps them again.

**PD31 `rec_arm` rows** (in `engine_gates_r1.h`; the harness has a MessageManager and `eg21::pump`).

| Row | Stimulus | Pass | Control |
|---|---|---|---|
| **AC1** | Manual take with signal, run to commit, pump 100 ms; an `AudioProcessorListener` records what a host sees | Arm == 0; exactly one gesture begin / change (0) / end; tape committed | hook `testNoArmAutoClear` → Arm stays 1 |
| **AC2** | Five discards: silent take; bypass while Armed; `prepareToPlay` while Armed; arm with the sidechain bus disabled; Arm set On with audio stopped, then `prepareToPlay` | Arm == 0 after the pump in each; slot unchanged | the `endGen` write removed from one branch → that arm stays 1 |
| **AC3** | Threshold below threshold, and Next Note with no notes: 2 s of blocks with pumps between | Arm == 1, state Armed, zero host notifications; then the trigger fires and AC1 holds | "clear whenever state != Recording" → clears while waiting |
| **AC4** | After AC1, one `setValueNotifyingHost (1)` | a second take lands (`recCommits` == 2) | covered by AC1n |
| **AC5** | An On edge during Committing, before the pump | the new take arms (`heldArms` == 1, then `recArms` == 2); Arm still 1 while it records | the `endGen == armGen` guard off → the new arm is cancelled |
| **AC6** | In AC1's listener | the write arrives on the message thread at APVTS-listener depth 0 | the write issued from `parameterChanged` → depth 1 |
| **AC7** | (a) A state blob with `rec_arm` = 1 restored, then `prepareToPlay`, pump. (b) Save, restore, save again with nothing changed | (a) Arm == 0, `recArms` unchanged; (b) the two blobs are byte-identical | (a) the row-11 branch removed → Arm stays 1 |
| **AC8** | Apply a factory preset while waiting Armed (Threshold) | Arm == 1, state Armed, latched trigger unchanged | existing UPM3n |
| **AC9** (probe) | Click `lane-arm-2` after a host push of `rec_arm` = 0 | `rec_target` == 1, the toggle pressed | the handler without its `!a.getValue()` branch |

INFO in AC7: a restore while blocks are running (today's edge behaviour, OS2).

**PD32 Page proof rows.**
- `ui_frontend_check.js`: constants 200 / 74 / 19 / 293; [8] closure; [16] with the modulo rule, 16 × 17 × 149 pairs and the expected lists `[1, 30…37, 66…73, 102…109, 138…145, 146…149]` (Global) and the same without 146–149 (Per-voice).
- `ui_binding_probe.js`: `N_PARAMS` 293; `KIND_COUNTS` knob 168, segment 19, toggle 19; tally `292 / 293`; 74 combos; mutation literal `data-dest="63"`; dest loop `[1, 146, 27, 30]`; DM2 expects 161 == 161 and 149 / 149; ST1 on 48 states.
- New probe rows: **AC9**; **FD1** / **FD1n** (the five feedback cells of a stream take `.off` when its switch goes off and lose it when it comes back; control: the `onParamChanged` arm removed); **VE15** (the value field open on a row-4 cell does not overlap either neighbour's readout text, en and fr).
- `--nc` is re-run, not only the main probe (a stale mutation literal makes a control REFUSE with exit 78 instead of failing its row).

**PD33 CPU (RS6).** Run alone, in the background, compared by the session factor.
- `--cpu25`: C25w ≤ 40, C25t ≤ 12.
- `--cpu26`: C25m ≤ 40, C25tm ≤ 12; new INFO **C25mPre** (C25m with Pre ×4: eight tanks; projected ≈ 31.2 %). C25mPre above 40 = STOP and ask.
- `--cpu4`: every preset T ≤ 12 %, W ≤ 40 %; the carried borderline / re-run rule. W is "the preset as authored" (VERIFICATION issue 1), unchanged.
- Projection: C25m ≈ 29.9 %; typical max ≈ 9.6 % (Hollow Wind) with no tank idle at all.
- Overrun: retune that preset's content first, then the D21 levers; a `grain_limit` default change = stop and ask (the first plan's PD16).

### Release and docs

**PD34 Docs.**
- `CHANGELOG.md`: still exactly one `## [1.0.0] - <date>` heading, date = the day T31 runs. Body: `:10` Record-in gains "Arm switches off when a take ends" and the sidechain prompt (F1); `:13` "Feedback and space" becomes per-stream feedback with on / off and per-stream reverb, before or after the feedback loop, Freeze and Reset All global; `:12` may name the new destinations. No "Fixed" or "Changed" section: nothing was released.
- `.planning/research/ARCHITECTURE.md` and `.planning/ROADMAP.md`: one R1 addendum paragraph each; both checksums re-pinned.
- `.planning/stages/2-dsp/CONTEXT-2.6.md` D34: one "amended by D66" line.
- `GiRecorder.h:42-47` header comment.
- `PLUGINS.md:76`: the date cell only. Status stays `🚧 Stage 4 | 1.0.0`.
- **REQUIREMENTS.md is not edited in execute** (checksum unchanged). SUMMARY-R1 lists what verify rewrites: DSP-04 (evidence rows per stream), DSP-05 (wording: per-stream reverb; parity evidence unchanged), DSP-06 (DR1 149 / 149), FUNC-07 (auto-clear), FUNC-08, PERF-02 (four / eight tanks).
- STATUS: mockup fields stay 1 (correction 14); a one-line progress note at every commit (last task, commit, next task), so a new session resumes from the file.
- `modules/registry.yaml`: untouched.

**PD35 HANDS-ON-SESSION-4, re-authored by hand in place (D·Sheet).**
- **Rewritten:** the defaults paragraph; 2.4-1 – 2.4-7; 2.5a-7, 2.5a-11; 3.1-1; 3.1-5; 3.2-10 – 3.2-12; 3.2-16; 3.2-24; 3.3-5; 3.3-14; LI6; the whole Pass 2 audition (64 rows; the 10 showcase rows get per-stream "expect" text); 2.4-8 – 2.4-10; 2.6-2 (it still says "No sidechain signal"); 3.2-13 – 3.2-15; S3.3-L2, S3.3-L3; LV1, LV2.
- **New rows:** per-stream toggle On / Off (no click, Amount kept); Pre ↔ Post while sounding; a dry stream beside a washed one; a per-voice route onto a stream's Reverb Mix with the mono badge; Pre: the loop holding at the ceiling and the tail still writing after note-off (sign-off 5); feedback and reverb at 100 % on four streams (the wet is cleaner than before above the tank's internal clip); Arm Off after a commit and after a discard; the lane Record button twice in a row; three takes with one click each; the host's parameter view after a take; a held-On automation region in Logic and in Live (sign-off 9); Live undo after a take; FX page density by eye in French.
- **Setup row (sign-off 11):** delete pre-R1 user presets; do not reopen pre-R1 projects. Rows 0-5 / 0-6 stay (the bank stamp rewrites the factory folder).
- **F1 retest rows:** S3.3-L1, S3.3-L2, S3.3-L3, 3.3-15, LV2.
- The "What automation already covers" paragraph is corrected (FP0–FP6, not PR0–PR6; the R1 rows added). The Results table is re-tallied. F1 and F2 in the triage table are marked resolved-pending-retest.

**PD36 Release rules (carried).** VERSION 1.0.0. No tag. No push except through T32, which is the first plan's T19 unchanged: the standing answer is hold.

---

## Tasks

**Roles.**
- **[dsp]** = dsp-agent writes engine / processor / harness code.
- **[gui]** = gui-agent writes the mockup, page, i18n and JS gates.
- **[agent]** = polish-agent writes specs, goldens, presets, docs.
- **[run]** = builds, gates, validators, commits.

**Conventions.**
- **BG** = `run_in_background`, logging to `$S/<name>.log` (`$S` = the executing session's scratchpad). Anything over about 5 min runs BG (600 s watchdog): the full harness, `--cpu*`, check-ui-labels, pluginval.
- `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`. `$B0`, `$B1a`, `$E`, `$M` as in PD2 / PD5.
- Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.
- **Commits.** Trunk-based on `main`. `git branch --show-current` and `git status --short` immediately before each. New files are staged by explicit path; never `git add -A`, never `git commit -a`. Form: `git commit -- plugins/O-GranulatorInstrument PLUGINS.md` (C0 names its two files instead). `modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, `plugins/O-GrainScatter/*` and `stages/3-gui/gate-report.json` are never staged.
- **Sessions.** Execute spans sessions. Session 1 ends at T6. A later session reads STATUS's progress note, re-runs T1's cheap checks (branch, hashes, `cmp`, `$B0` present) and continues.

### Wave 0 — pre-flight, F1, baseline B0

1. **T1 — pre-flight** [run]. Output: `$S/preflight.txt`.
   - Contracts re-hashed against the STATUS frontmatter: 7 / 7.
   - Branch `main`; `git log 63ad0c5d..HEAD -- plugins/O-GranulatorInstrument/{Source,tests,CMakeLists.txt}` shows only `1b17f41c` (FORK-MANIFEST line) and tests commit `f0516bca`.
   - Both pairs `cmp` clean. `git diff --stat` on the two F1 files: 12 lines each, the `rec-status` tip and `rec.noSignal` only.
   - Every Plan-time fact symbol re-grepped; drift recorded.
   - PD30 / PD31 / PD32 row names grepped across `tests/` and `Source/` (excluding `ui/public/modules`): 0 hits each.
   - Dirty set recorded; `modules/registry.yaml` hunks recorded (5) and left.
   - Real-home listing snapshotted: `~/Library/O-GranulatorInstrument` (expect Factory 64 + 2, no User) and `~/Music/Ouaricon`.
   - Logic and Live are not running (asked once, recorded).
   - Any mismatch → STOP and ask.
   - Depends on: none
2. **T2 — F1 commit C0** [run].
   - check-i18n, fr-lint, zh-lint on the tree: 0 · 0 · 0.
   - `git commit -- plugins/O-GranulatorInstrument/Source/ui/public/js/i18n.js plugins/O-GranulatorInstrument/.planning/mockups/v1-i18n.js`. Nothing else in this commit.
   - Planning docs still uncommitted from discuss / research / plan (CONTEXT-R1, RESEARCH-R1, `research-r1/`, PLAN-R1, STATUS, CONTEXT, HANDS-ON-SESSION-4, VERIFICATION) ride with C1.
   - F1's build and install happen at T14; its by-eye rows at verify.
   - Depends on: T1
3. **T3 — baseline B0 + commit C1** [agent + run].
   - Build the harness on the tree as it is. Full run (BG) → `B.ids.txt`, `B.full.txt` (expect **454 / 0**, 0 NOT FIRED).
   - Add `--dump-preset-renders` and `--cmp-preset-renders` (PD29). Tests only: `git diff --stat -- Source CMakeLists.txt` is empty.
   - Rebuild; full run again: `ids` and `full` identical to the first run.
   - `$H --dump-preset-renders $B0` twice into two directories: the 64 sha256 lines are identical → `b0-preset-renders.sha256`. Record each preset's enabled-stream count (expect 38 / 24 / 1 / 1).
   - `--cmp-preset-renders $B0 --all-bitwise` on this same build: PT0 64 / 64. Without the flag, PT2 must **fail** its `differ > 0` clause on all 26 multi-stream presets: that is the "one output tank" signature the clause exists to catch, seen firing on the tree that has one tank.
   - Record: render hashes + legacy (== goldens, `ctrl FIRED`); parity 168 / 0 + GV 2 / 0 with the PB / PR residual lines → `b0-parity.txt`; `--eg24` output → `b0-eg24.txt`; param-dump == `.planning/params.tsv`; drift 0 / 13; overrides diff-clean.
   - R-ui baseline: frontend ALL PASS (`--nc` 4 / 4), probe 261 × 2 with `--nc`, check-i18n · fr · zh 0 · 0 · 0, check-ui-labels `default + 47`, boot-all-uis 0 DEAD, PN1.
   - auval: PASS with exactly the 4 LFO-rate warnings (on the installed Stage 4 build).
   - CPU baseline, BG, alone: `--cpu25`, then `--cpu4` (it runs the `--cpu26` rows first) → `b0-cpu.txt` with the session factor.
   - Commit C1: the two harness files, `$E`, the planning docs listed in T2, STATUS progress note.
   - Files: `tests/render-harness/{preset_gates_4.h, main.cpp}`, `.planning/stages/4-polish/evidence-r1/*`
   - Depends on: T2

### Wave 1 — v2 mockup and the D65 gate

4. **T4 — v2 mockup files** (PD5, PD22–PD25) [gui].
   - Fork the three files after C0. Author Variant B natively at the C·7.4 numbers; add the `MOCKUP-ONLY` Variant A transform and switch.
   - Page tables, `data-dest` renumber, `DEST_COUNT`, the modulo `isGlobalDest`, figure removal, `#write-ro` removal, the `.off` arm for the feedback cells, row-4 `.n5` cells.
   - Strings per PD24, bindings 477, states per PD25.
   - No file outside `.planning/mockups/v2-*` is written.
   - Files: `.planning/mockups/{v2-ui.html, v2-i18n.js, v2-i18n-states.json}`
   - Depends on: T3
5. **T5 — scratch tree, gates, screenshots, commit C2** [run].
   - Build `$M` per PD5. Drafts from `$S/mk_drafts.py` (byte-preserving I/O); the stub banner must read `param-dump (293 rows) + overrides`.
   - On `$M`, Variant B: check-i18n `--root`, fr-lint, zh-lint, check-ui-labels `--root` (`default + 48`), boot-all-uis `--strict-tips`. All exit 0; a failure is fixed in the v2 files and re-run.
   - Measurements by script, en / fr / zh-Hans: row-4 label gaps (expect fr ≥ 8 px), the open value field against both neighbours on a row-4 cell, title cells ≤ 2 lines, the stream IV tooltip clamp at 1200 × 800 after the 350 ms dwell, bottom edge ≤ 786, the widest destination option per language against the 98 px select.
   - Screenshots into `build/ui-mockup-v2/shots/`: FX tab ×3 languages for B and for A; MOD tab with a feedback destination selected, en and fr; `fb-off`; `reverb-pre`; a row-4 value field open.
   - `cmp` of both v1 pairs still clean; `git diff --stat C1..HEAD -- Source tests` empty.
   - Commit C2: the three v2 files, STATUS (`next_action`: D65 approval pending, with the serve command).
   - Depends on: T4
6. **T6 — CHECKPOINT: D65 approval** [run; **Taylor decides; execute STOPS here**].
   - Present: `node build/ui-mockup-v2/scripts/serve-ui.js --plugin O-GranulatorInstrument --hold` (open the printed URL at 1200 × 800; the switch in the corner flips A / B), the screenshot folder, the T5 measurements, `diff` size of v1 → v2.
   - Ask for: (1) Variant B or A; (2) row density (a readout sits 3 px above the next row's label); (3) the destination names as shown; (4) OS1, OS2, OS3.
   - Then the two-step handoff of CLAUDE.md: `/clear`, then `/plugin-execute O-GranulatorInstrument 4-polish`. Stop.
   - **On resume:** W2 and W3 run in any case. T15 onward runs only with an explicit approval recorded in STATUS (variant, date, OS answers). A revision request loops T4 → T5 → T6 with a new commit; W2 / W3 work is unaffected.
   - Approval to plan is not approval of the layout.
   - Depends on: T5

### Wave 2 — B1: DSP on the old parameter IDs (not blocked)

7. **T7 — control ramps ×4** (PD6, PD10) [dsp + run].
   - Checkpoint: build the harness (BG); hashes 14 / 14 / 14; full extract **identical to B**; parity lines identical; `--cmp-preset-renders $B0 --all-bitwise`: **PT0 64 / 64** (tanks are untouched at this point).
   - Files: `Source/dsp/Engine.{h,cpp}`, harness accessor uses if a signature changes
   - Depends on: T3 (runs after T6 in session order)
8. **T8 — tank A ×4 and gate A** (PD7, PD10, PD11) [dsp + run].
   - Before the edit: record the stream count of each PD3 B1-list row.
   - Checkpoint: hashes 14 / 14 / 14; `ids` == B; `full` diff only on the B1 list; RV1–RV3 and every PB / PR line digit for digit; PT1 38 / 38 bitwise, PT2 26 / 26 (≤ −110 dB, `|g − 1|` ≤ 1e-5, `differ > 0`); PT2n fails.
   - `$H --dump-preset-renders $B1a`.
   - Files: `Source/dsp/Engine.{h,cpp}`, the harness lines that use `reverbHardCut` / `getRevMixForTest`
   - Depends on: T7
9. **T9 — tank B, gate B, guard, Reset hold, toggle fold** (PD8, PD9, PD11) [dsp + run].
   - All dormant: `fbOn` true and Pre false are fixed engine-private values until T17.
   - Checkpoint: hashes 14 / 14 / 14; `full` **identical to T8's**; `--cmp-preset-renders $B1a --all-bitwise`: PT0 64 / 64.
   - Files: `Source/dsp/Engine.{h,cpp}`
   - Depends on: T8
10. **T10 — B1 proof + commit C3** [run].
    - R-core (PD4). `git diff --stat C2..HEAD -- Source/dsp/forked` is empty; drift 0 / 13 with no manifest edit.
    - `--cpu26` (BG, alone): C25m ≤ 40, C25tm ≤ 12; delta vs `b0-cpu.txt` recorded (expect about +1.0 on C25m).
    - `B1.full.txt` saved. Commit C3.
    - Depends on: T9

### Wave 3 — Record-in Arm auto-clear (not blocked)

11. **T11 — audit CM1 / OS1 and the take helpers** (PD21) [run + agent]. Output: `$S/ac-audit.txt` and `$E/ac-audit.txt`.
    - The list of rows that arm and pump; for each, what it reads afterwards and whether D66 changes it.
    - Label fixes only (OS1 "engine level, no pump"; CM1 "Arm Off after the pump"). Harness full run: `ids` == B; `full` moves only on the relabelled rows.
    - Files: `tests/render-harness/engine_gates_26a.h` (+ any audited header, labels only)
    - Depends on: T10
12. **T12 — implementation** (PD18–PD20) [dsp].
    - `armGen` / `endGen`, the nine endings, the Timer-path write, the hooks; the header comment.
    - If OS2's alternative was chosen at T6: the restore-swallow generation.
    - Files: `Source/dsp/GiRecorder.h`, `Source/PluginProcessor.{h,cpp}`
    - Depends on: T11
13. **T13 — rows AC1–AC9** (PD31) [agent + run].
    - Create `engine_gates_r1.h` with the AC section, the `--egr1` flag and the full-run call (after `pg4::runAll()`, before `rowHM1()`).
    - AC9 in the probe.
    - Files: `tests/render-harness/{engine_gates_r1.h, main.cpp}`, `tests/ui_binding_probe.js`
    - Depends on: T12
14. **T14 — wave proof, early validators, commit C4** [run].
    - R-core. The audited rows behave as T11 predicted. Renders unchanged.
    - Probe + `--nc` (AC9 PASS, AC9n fails); the other UI gates are unaffected (no page or i18n byte moved: assert).
    - **R-val now** (corrections 5, 6): `./scripts/build-and-install.sh O-GranulatorInstrument`; auval PASS with **exactly** the 4 LFO-rate warnings; pluginval 10 VST3 + AU, sidechain on, three runs each with different `--seed` values; the same with `OGI_GATE_SC_DEFAULT_OFF=ON`; shipping build restored and reinstalled; cache variable back to `OFF`.
    - A `rec_arm` line in auval or pluginval output → **STOP and ask** (OS4). Do not weaken a validator setting.
    - Commit C4.
    - Depends on: T13

### Wave 4 — B2 parameter surface, page landing, B3 copy (blocked on T6)

15. **T15 — finalize v2 to the approved variant** [gui + run].
    - Strip the switch and the rejected variant; `grep -c MOCKUP-ONLY` == 0 in all three files. If A was approved, re-author the inline layout as A first.
    - Apply the T6 answers (OS3 wording, any requested change).
    - Re-run T5's scratch gates and measurements on `$M`.
    - Files: `.planning/mockups/v2-*`
    - Depends on: T6 (approval recorded), T14
16. **T16 — spec, extractor, goldens** (PD15) [agent + run].
    - The extractor's own cross-checks pass and print `OK: 293 params (200 Float / 74 Choice / 19 Bool), 150 dests, 18 sources`.
    - `mod-sources.tsv` byte-identical.
    - Controls on scratch copies of the spec: one stream row removed; `sN_fb_enable` default Off on one stream; one new destination out of order → the extractor exits 1 each time and writes nothing.
    - Files: `.planning/parameter-spec.md`, `tests/golden/{extract_spec.py, params.tsv, mod-dests.tsv}`
    - Depends on: T15
17. **T17 — C++ parameter surface** (PD13, PD14, OS1) [dsp].
    - `GiParamIds.h`: defs, counts, asserts, destination table, version 2, comments.
    - `GiModMatrix.h`: stride 36, the 8 stream-global fields, `isStream` bound to `LfoRate1`, bounded `isVoiceClass`, layout asserts (`stream (3, KeyTrack) == 137`, `stream (0, 28) == 30`, `LfoRate1 == 146`).
    - `forked/ControlGrid.h` + FORK-MANIFEST: run the drift check **before** the manifest update (must read 1 / 13: the check sees the edit), then re-hash column 5 and add the column-7 line (0 / 13).
    - `Engine.{h,cpp}`: the per-stream ramps, atomics and gates now read `gp.s[s]`; the per-stream destinations in `updateChainCutoffs`, the tick block, `updateHeads`, `writeReverbAtomics`; the class bounds at `:2976`, `:3417`; `GiVoiceMod.h:322`.
    - `PluginProcessor.{h,cpp}`: the head-display rule per slot (`freeze && sN_feedback > 0 && sN_fb_enable`); comments and `kDests`.
    - `PluginEditor.{h,cpp}`: the three relay tables (200 / 74 / 19) and comments. No other editor code.
    - Files: as listed
    - Depends on: T16
18. **T18 — harness and parity port** (PD16) [agent].
    - Files: the 10 render-harness headers / `main.cpp` of B·Q6, `tests/parity/ParityRows.h`
    - Depends on: T17
19. **T19 — bank copy** (PD27) [agent].
    - Wildcards + `expandSpecValues()`; FP0's per-stream readers; FP4 / FP4b / UPM counts.
    - Files: `Source/FactoryPresets.h`, `tests/render-harness/{preset_gates_4.h, ui_gates_31.h}`
    - Depends on: T18
20. **T20 — B2 / B3-copy checkpoint** [run].
    - Build the harness and the param-dump console (BG).
    - Hashes 14 / 14 / 14. `ids` == B. `full` vs `B1.full.txt`: text moves only on the PD16 table; every ported row's measured value equals B1's. Parity 168 / 0 + GV 2 / 0, PB / PR lines digit for digit.
    - `--cmp-preset-renders $B0`: PT1 38 / 38 bitwise, PT2 26 / 26; PT2n and PT2n2 fail.
    - FP0–FP6 PASS with their controls; FP2's spread recorded (expect 6.32 dB).
    - AU1: the drift set is still the 4 LFO rates. A 5th member = STOP.
    - HM1 green; the real factory folder untouched by the harness and the param-dump console.
    - `B2.full.txt` saved. A failure is attributed by row family: engine rows → T17 / T18; FP / PT rows → T19.
    - Depends on: T19
21. **T21 — page landing, JS gates, re-pins** (PD17, PD23–PD26, PD32) [gui + run].
    - Assert `cmp index.html v1-ui.html` and `cmp js/i18n.js v1-i18n.js` are still clean (no drift since the fork), then `cp` the three v2 files over `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `tests/i18n-states.json`; `cmp` the three v2 pairs.
    - `ui_frontend_check.js` and `ui_binding_probe.js` per PD32; FD1, VE15.
    - `.planning/params.tsv` regenerated and checked by the PD17 script; overrides regenerated twice.
    - STATUS: `parameter_spec` and `parameter_spec_body` re-hashed with the reason.
    - Files: `Source/ui/public/{index.html, js/i18n.js}`, `tests/{i18n-states.json, ui_frontend_check.js, ui_binding_probe.js}`, `tests/ui-stub/generic-overrides.json`, `.planning/{params.tsv, STATUS.md}`
    - Depends on: T20
22. **T22 — UI gates, wave proof, commit C5** [run].
    - R-core + R-ui. Expected: frontend ALL PASS (`--nc` 4 / 4); probe 293 × 2, DM2 161 / 149 / 149, FD1, VE15, AC9, ST1 on 48 states, `--nc` live; check-i18n · fr · zh 0 · 0 · 0; check-ui-labels `param-dump (293 rows) + overrides`, `default + 48`; boot-all-uis 0 DEAD / 0 late; PN1.
    - A new collision or fit failure is fixed in `index.html` **and** `v2-ui.html` (or the i18n pair) and the sequence re-run.
    - Shipping targets build with 0 warnings in `Source/`. No install here.
    - Commit C5.
    - Depends on: T21

### Wave 5 — B3 showcase, B4 new rows, CPU

23. **T23 — showcase re-voices** (PD28) [agent + run].
    - One preset at a time: values, description, `sN_level` trim.
    - After each: FP0, FP1, FP2 (spread ≤ 9 dB), FP4 and FP6 for that preset. Fixes go into the table, never into a gate.
    - `--cmp-preset-renders $B0 --exclude-showcase`: PT1 38 / 38, PT2 16 / 16; the 10 names printed as excluded.
    - Files: `Source/FactoryPresets.h`
    - Depends on: T22
24. **T24 — new rows** (PD30) [agent + run].
    - `--egr1` first, then the full set. Each row PASSES, each control FAILS its predicate, each validity clause holds (an INVALID is not a PASS). INFO lines recorded.
    - A row that fails for a DSP reason: fix the engine, then repeat T20's comparisons (hashes, B extract, PT rows) before going on. A fix that changes Pre or toggle behaviour re-runs T23's FP rows for the showcase presets that use them (Organ Loft, Four Lanes, Fifth Bloom, Slow Tide; Deep Well, Night Engine).
    - Files: `tests/render-harness/{engine_gates_r1.h, main.cpp}`, `Source/dsp/Engine.{h,cpp}` (hooks; fixes only if a row demands one)
    - Depends on: T23
25. **T25 — CPU verdicts** (PD33) [run, **BG, alone**].
    - `--cpu25`, then `--cpu4` (which runs the `--cpu26` rows and prints C25mPre).
    - Record: session factor, C25w, C25m, C25mPre, C25tm, bank T median / p90 / max, W max, top 5, each showcase preset's T against its B0 value.
    - Retune loop per PD33; each retune re-runs that preset's FP rows.
    - Depends on: T24
26. **T26 — wave proof + commit C6** [run].
    - R-core, PT rows, PN1 (names unchanged). Shipping targets build with 0 warnings.
    - Commit C6.
    - Depends on: T25

### Wave 6 — build, install, validators, final regression

27. **T27 — build / install / validate** [run].
    - Logic and Live quit. Build VST3 + AU + Standalone + consoles, **0 warnings in `Source/`**.
    - `./scripts/build-and-install.sh O-GranulatorInstrument` (cache clear, dual-variant sweep). Installed binaries `cmp`-identical to the build; `auval -a | grep -i granulatorinstrument` shows one `-dev` line.
    - auval `aumu OuGI OuDv`: PASS with exactly the 4 LFO-rate warnings.
    - pluginval 10 VST3 + AU, sidechain on, then off (`OGI_GATE_SC_DEFAULT_OFF=ON`); shipping build restored and reinstalled.
    - X2: 0 hook symbols and 0 `stub_` strings in the shipping VST3 / AU / Standalone; harness control non-zero.
    - First run: the first instantiation rewrote `~/Library/O-GranulatorInstrument/Presets/Factory` (new `.bank-hash`; 64 `.json` + 2 sentinels; the files carry `"s1_reverb_mix"`; none carries the bare keys `"reverb_mix"` or `"feedback"`, and none carries `"output_gain"`).
    - `tests/windows-webview2-gate.sh` exit 0; its 6 controls exit 1 (the editor file changed).
    - Standalone smoke (window capture): the FX tab boots with the per-stream rows; readouts at their defaults.
    - Depends on: T26
28. **T28 — final regression** [run].
    - If no file under `Source/`, `tests/` or `CMakeLists.txt` changed since T26's build, T22 / T24 / T25 / T26 / T27 are the final-tree regression: say so and list the commit.
    - Otherwise re-run R-core, R-ui, the PT rows, `--cpu26` (BG) and R-val on the final tree.
    - `cmp` the three v2 pairs. `git diff --stat C0..HEAD` lists nothing outside the scope fence.
    - Depends on: T27

### Wave 7 — docs, sheet, summary

29. **T29 — docs and metadata** (PD34) [agent].
    - CHANGELOG (`grep -c '^## \['` == 1; the CI awk extract prints a non-empty body); ARCHITECTURE and ROADMAP addenda; CONTEXT-2.6 note; PLUGINS.md date.
    - Files: `CHANGELOG.md`, `.planning/research/ARCHITECTURE.md`, `.planning/ROADMAP.md`, `.planning/stages/2-dsp/CONTEXT-2.6.md`, `../../PLUGINS.md`
    - Depends on: T28
30. **T30 — HANDS-ON-SESSION-4 re-authored** (PD35) [agent].
    - Every carried row ID still present exactly once (count by ID before and after; the removed and added IDs listed).
    - Files: `.planning/stages/4-polish/HANDS-ON-SESSION-4.md`
    - Depends on: T29
31. **T31 — SUMMARY-R1, STATUS, commit C7** [agent + run].
    - `SUMMARY-R1.md` (complex template): what was built; gate table on the final tree; new rows with measured values and control results; PD3 moves old → new; the PD16 amended rows; the param-dump and overrides re-pin; CPU table; deviations; residual risks; what verify must re-run and rewrite (REQUIREMENTS list of PD34); open items (D51 session, D54).
    - STATUS → `stage_4_rework_r1_execute_complete`; `architecture` and `roadmap` checksums re-hashed; `requirements` unchanged (assert); next: `/plugin-verify O-GranulatorInstrument 4-polish`.
    - Commit C7. Then the two-step handoff.
    - Files: `.planning/stages/4-polish/SUMMARY-R1.md`, `.planning/STATUS.md`
    - Depends on: T30
32. **T32 — Windows CI checkpoint** (carried: first plan T19 / PD18, sign-off 2 of that plan) [run, **Taylor confirms first**].
    - Present the commit count ahead of origin, the sessions it includes and the dispatch command. The standing answer is **hold**.
    - Only on an explicit go in that moment: `git push origin main`, then `gh workflow run build-and-release.yml -f plugin_name=O-GranulatorInstrument -f validate_only=true`; record the run. Never tag.
    - Without a go: SUMMARY-R1 and STATUS keep "D54 CI arm pending `/publish`".
    - Depends on: T31

---

## Dependency graph

```
T1 ── T2 ── T3 ── T4 ── T5 ── T6 (STOP: D65)
                                │
                                ├─ not blocked ─ T7 ── T8 ── T9 ── T10 ── T11 ── T12 ── T13 ── T14 ─┐
                                │                                                                   │
                                └─ approval ────────────────────────────────────────────────────────┴─ T15 ── T16 ── T17 ── T18 ── T19 ── T20 ── T21 ── T22
                                                                                                                                                    │
      T32 ── T31 ── T30 ── T29 ── T28 ── T27 ── T26 ── T25 ── T24 ── T23 ───────────────────────────────────────────────────────────────────────────┘
```

Checkpoint order inside a wave is one task at a time (shared build directory; a render move is attributed to one task): T7 → T8 → T9, then T20 → T21.

## Commit split

| Commit | Content | Tasks |
|---|---|---|
| C0 | F1 Record-in copy: `js/i18n.js` + `v1-i18n.js`, nothing else | T2 |
| C1 | Harness dump / compare modes, `evidence-r1/` baseline, planning docs from discuss / research / plan | T3 |
| C2 | `v2-ui.html`, `v2-i18n.js`, `v2-i18n-states.json`; STATUS paused at D65 | T4–T5 |
| C3 | B1 DSP: per-stream ramps, tanks A / B, gates, toggle fold, hooks | T7–T10 |
| C4 | `rec_arm` auto-clear, audit relabels, AC1–AC9 | T11–T14 |
| C5 | Spec + addendum C, extractor, goldens, 293-parameter surface, ControlGrid + manifest, harness / parity port, bank copy, page landing, JS gates, `params.tsv`, overrides, STATUS checksums | T15–T22 |
| C6 | Showcase re-voices, `engine_gates_r1.h` DSP rows, CPU evidence | T23–T26 |
| C7 | CHANGELOG, ARCHITECTURE / ROADMAP addenda, CONTEXT-2.6 note, PLUGINS.md date, HANDS-ON-SESSION-4, SUMMARY-R1, STATUS | T29–T31 |

---

## Gate criteria (R1 execute)

| Gate | Pass |
|---|---|
| Renders | `renders-2.4`, legacy `renders-2.2`, `renders-2.1` pinned lines byte-identical at T7, T8, T9, T10, T14, T20, T22, T26 and on the final tree. **Zero re-pins.** |
| Harness | The 454 carried rows: same IDs, all PASS; text moves only on the PD3 B1 list and the PD16 table. New rows PASS (PD30, PD31); every `n` control FAILS its predicate; no INVALID; 0 NOT FIRED; HM1 green |
| Parity | 168 / 0 + GV 2 / 0; PB / PR residual lines equal to B0 digit for digit |
| Preset renders | PT0 64 / 64 at T7 and at T9 (vs `$B1a`); from T8 on PT1 38 bitwise and PT2 26 (16 after the showcase) at ≤ −110 dB, `\|g − 1\|` ≤ 1e-5, `differ > 0`; PT2n, PT2n2 fail; the 10 excluded names printed |
| Contract | 293 = 200 / 74 / 19; destinations 150; table version 2; extractor OK and its 3 controls exit 1; goldens generated from the spec; `mod-sources.tsv` unchanged |
| param-dump | == the T21 pin; the diff vs the Stage 4 pin is exactly PD17 |
| Fork drift | 1 / 13 before the manifest update at T17, 0 / 13 after and at every later gate; only `ControlGrid.h` and `FORK-MANIFEST.md` moved under `forked/` |
| Presets | 64; FP0–FP6 PASS with controls; FP2 spread ≤ 9 dB; no preset sets `output_gain`; bank stamp rewrites the folder on first run |
| CPU | C25w ≤ 40, C25t ≤ 12, C25m ≤ 40, C25tm ≤ 12; `--cpu4` every preset T ≤ 12 %, W ≤ 40 %; INFO C25mPre recorded (≤ 40) |
| `rec_arm` | AC1–AC9 PASS with controls; audited rows as predicted; state functions unchanged (`git diff` shows no hunk in `getStateInformation` / `setStateInformation` unless OS2's alternative was chosen) |
| Home hygiene | HM1; real `~/Library/O-GranulatorInstrument` and `~/Music/Ouaricon` untouched by the harness and the param-dump console |
| Build | VST3 + AU + Standalone + consoles; 0 warnings in `Source/`; VERSION 1.0.0 |
| auval | PASS; warnings == exactly the 4 LFO rates (AU1 agrees) |
| pluginval 10 | VST3 + AU SUCCESS, sidechain on and off; at T14 three seeds each |
| X2 | 0 hook symbols, 0 `stub_` in shipping binaries |
| Windows gate | script exit 0; 6 controls exit 1 |
| UI | frontend ALL PASS (`--nc` 4 / 4); probe 293 × 2, DM2 161 / 149, FD1, VE15, AC9, ST1 on 48 states, controls live; check-i18n · fr · zh 0 · 0 · 0; check-ui-labels 0 with `default + 48`; boot-all-uis 0 DEAD; PN1 |
| `cmp` | `index.html` == `v2-ui.html`; `js/i18n.js` == `v2-i18n.js`; `tests/i18n-states.json` == `v2-i18n-states.json`; `MOCKUP-ONLY` count 0 |
| Release | one `## [1.0.0]` heading; PLUGINS.md `🚧 Stage 4 \| 1.0.0`; no tag; no push without T32's go |
| Scope | `git diff --stat C0..HEAD` lists nothing outside the scope fence; `modules/`, `scripts/`, `CMakeLists.txt` untouched |

Exit 77 anywhere = not run = not a pass.

## Risks / watch items

- **Validators and the auto-clear** (corrections 5, 6). The riskiest unknown. It is proven at T14, before anything is built on it.
- **Statement shape.** One reordered multiply in the stream term, or a tank output folded into the sum expression, changes FMA contraction and breaks the 42 hash lines. T7's "64 / 64 bitwise" and T8's PT1 catch it at the task that caused it.
- **A gate that sits behind a dirty check latches** (repo memory: run flag past a dirty check). Gates A and B, `tailPk` and `preHold` are recomputed at every tick. TK1 and MU1 gate it.
- **A NaN in a tank is permanent**, and `SmoothedValue::reset` does not clear one. Guards sit before both tanks; TG1 gates the loop side.
- **Class by position.** A per-voice source on a new destination silently does nothing if `isVoiceClass` stays unbounded. DC1 and MC1.
- **The mono rule lives in four places.** A reworded C++ `if` reads as "pageRule not found" in the frontend check, not as a mismatch.
- **Vacuous Pre rows.** FL1p and FL1sp assert liveness on the stimulus; FL1sp's control removes the clamp itself.
- **Click rows in a reverb.** The tank's level under a sine depends on LFO phase: static references share the tank history; position 50 % with spray 100 %; CK8 is measured on the return probe because a hard control may be unable to fire at the output.
- **Showcase before the new rows.** PD1 keeps the research order. A DSP fix at T24 re-runs the affected showcase presets; the bank's CPU and loudness verdicts (T25, T26) are always taken on the final DSP.
- **FP0's reader change** is a predicate change on a carried row. It is identical to B for the 54 copied presets; it is recorded as a planned amendment.
- **`$B0` lives under `build/`.** A clean rebuild that deletes `build/` loses the reference.
- **Destination select width.** Four English names per stream are shortened; French still clips and no gate sees it (OS3).
- **Pre-R1 sessions and user presets re-map silently** (sign-off 11). The machine has no `Presets/User` folder today; the sheet's Setup row covers a later one.
- **`reverb-unit` is shared.** Eight instances allocate about 3.1 MB at 48 kHz in `prepare`; `clearTank` runs on the audio thread at each cold start (no allocation; FA4p).
- **T32 is outward-facing and repo-wide.** No push without an explicit go in that moment.

## Deviations protocol

Record every departure in SUMMARY-R1 (what, why, evidence, which gate). The PD16 amended rows, the PD17 re-pins, the ControlGrid edit (OS1) and correction 5 are planned and recorded as such.

Stop and ask only for:
- a render-hash move, or a carried row moving outside its list;
- a path outside the scope fence, a module or `scripts/` change, a VERBATIM-leaf edit;
- a LOCKED-contract edit beyond PD15;
- `rec_arm` appearing in a validator's output (OS4);
- a 5th auval drift member;
- C25mPre above 40 %, a `grain_limit` default change or a D21 engine lever;
- a page design change beyond the approved variant;
- a lost `$B0`;
- any push or tag.

## Carried (not R1)

- COMPAT-02 DAW pass (MPE, Dorico) → 1.0.x. Reaper. Windows hands-on.
- `~SlotLoader` 10 s join on a hung disk; the detached-writer bundle-unload residual (CHANGELOG known issues).
- The `customStateOf` dangling-pointer pattern in O-Granulator (SUMMARY deviation 8; an `/improve` there).
- `--cpu4` W does not force 16 voices (VERIFICATION issue 1).
- PN1 has zero margin on the widest preset name (VERIFICATION issue 2).
- `modules/registry.yaml` own-row version hunks, uncommitted, not R1's.
- BRIEF.md's prose counts (historical; checksum unchanged).
- README row (left to `/publish`).

## What verify must re-run

Verify re-runs, on the final tree and without copying from SUMMARY-R1:
- R-core, R-ui and R-val of PD4, in full, with the probe's `--nc`.
- `--egr1` and the full harness; the count of new rows by name.
- `--cmp-preset-renders $B0` (PT1 38, PT2 16, the 10 excluded) while `$B0` exists; the sha256 of each `$B0` raw against `b0-preset-renders.sha256`.
- The extractor from the spec into a scratch directory: `cmp` with the three goldens.
- The PD17 join script on `.planning/params.tsv`.
- `--cpu25`, `--cpu4` alone in the background.
- The fork-drift check, and a read of the `ControlGrid.h` diff against the manifest's column-7 line.
- The 7 contract hashes against STATUS (4 moved in execute: `parameter_spec`, `parameter_spec_body`, `architecture`, `roadmap`; `requirements` moves at verify itself).
- A code read: the Timer-path write (no listener, no `AsyncUpdater`, not on `RecPollThread`); gates A / B derived at the tick; `process` called for all eight tanks; the stream-term statements unchanged in shape.

Verify then updates REQUIREMENTS.md (PD34 list) and runs the human session on the re-authored sheet, including the F1 retest rows and the new R1 rows.

## Success criteria (gate for verify)

- [ ] F1 committed on its own before any R1 i18n edit, then built and installed
- [ ] v2 mockup approved at the D65 checkpoint before `index.html` was edited; the landed page is the approved variant with no mockup-only code
- [ ] Per-stream feedback and reverb work with the 8 globals removed: 293 parameters (200 / 74 / 19), 150 destinations, table version 2, spec + extractor + goldens + checksums consistent
- [ ] Feedback 0 and the toggle Off write nothing; Pre is bounded by the writer; a muted stream's audible tail rings out and its loop tank stops writing; Reset flushes the loop tank (FL1p–FL5p, TG1, RS2p, MU1, IN1, SI1, TK1)
- [ ] Per-voice sources reach the new destinations through the newest voice, and the mono badge agrees in the engine, the page and the three gates (DC1, MC1, UMB1, frontend [16])
- [ ] No click on the toggle, the Pre / Post switch or a mute with reverb on (CK7, CK8, CK9)
- [ ] Render goldens byte-identical throughout; parity 168 / 0; the 454 carried rows pass with only the listed text moves
- [ ] 38 single-stream presets bitwise and 16 multi-stream presets within −110 dB of B0; 10 showcase presets re-voiced inside FP2 and `--cpu4`
- [ ] C25m ≤ 40 %, every preset T ≤ 12 % / W ≤ 40 %, C25mPre recorded
- [ ] Arm returns to Off after a commit, a discard or an arm that cannot start, from the message thread, visible to the host; waiting arms are left alone (AC1–AC9); auval and pluginval unaffected
- [ ] Five UI gates green with 48 states in three languages; bindings and closure at 293; the three v2 pairs `cmp` clean
- [ ] Build, install, auval (4 LFO-rate warnings exactly), pluginval 10 sidechain on / off, X2, Windows grep gate
- [ ] Only `forked/ControlGrid.h` moved among the forked leaves, recorded in FORK-MANIFEST; no module, `scripts/` or `CMakeLists.txt` change
- [ ] CHANGELOG `## [1.0.0]` amended, spec addendum C, ARCHITECTURE / ROADMAP addenda, STATUS checksums, PLUGINS.md date, HANDS-ON-SESSION-4 re-authored, SUMMARY-R1 written; VERSION 1.0.0, no tag, no push
