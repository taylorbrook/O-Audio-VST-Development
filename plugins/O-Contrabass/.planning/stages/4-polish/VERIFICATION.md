# Stage 4: Polish — Verification

## Verification Date

2026-07-15

## Verdict

**⚠️ PARTIAL** — the entire automated bar and every in-tree code / distribution /
doc artifact is **VERIFIED GREEN**. Four requirements carry human-in-the-loop
gates that cannot be auto-completed (and one — Windows CI dispatch — is
outward-facing while the user is holding release). The plugin is **code-complete
and automated-validated at v1.0.0**; final ship sign-off is blocked on those four
gates only.

---

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

Bring O-Contrabass to a validated, production-ready **v1.0.0** (everything short of
a public release, which the user is holding). Close the six Stage-4 requirements:

1. **FUNC-04** — ship 10 factory presets (5 Orchestral + 5 Drone), skew-safe.
2. **COMPAT-02** — ship a full `.doricolib` Playback Template bundle; microtonal NE plays in Dorico.
3. **COMPAT-01** — Windows VST3 built + pluginval-10 green via CI (no public release).
4. **PERF-02** — measured `<5%` CPU/voice @ 44.1/48 kHz, 256-block, defaults.
5. **FUNC-03 + DSP-10** — subjective sign-off (dual orchestral/drone + slow-attack) via documented A/B.
6. Reconcile version/docs (CHANGELOG→1.0.0, parameter-spec, NOTES v1.1 deferrals, registry.yaml).

**Cross-cutting invariant:** DSP is FROZEN — 19/19 render goldens stay byte-identical.

### Deliverables (from SUMMARY.md + code inspection)

1. 10 `FactoryPresetDef`s in `Source/PluginProcessor.cpp` (engineering units → single `convertTo0to1` loop → `initializeFactoryPresets`).
2. `Resources/dorico/` — 5 files (`.doricolib`, `endpointconfig.xml`, `playbacktemplatespec.xml`, INSTALL, SMOKE-TEST) + CMake `install(DIRECTORY …)` rule.
3. `.github/workflows/build-and-release.yml` — `workflow_dispatch` + `validate_only` gate + Windows pluginval-10 step (authored; not yet dispatched).
4. `tests/render-harness/main.cpp` — isolated `--perf` mode (`--sample-rate`/`--block-size`, RTF + CPU%/voice).
5. `AUDITION.md` probe table (human gate rig).
6. Docs: CHANGELOG `[1.0.0]`, parameter-spec `kNumVoices=4`, NOTES v1.1 deferrals, registry.yaml preset-manager 1.0.4.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| FUNC-04 (10 presets, skew-safe) | ✅ Achieved | All 10 seed to `~/Library/O-Contrabass/Presets/Factory/`; round-trip **exact** across all param types (see below). Subjective QA (×8) optional. |
| COMPAT-02 (Dorico bundle) | ⚠️ Partial | Bundle authored + structurally verified (load-bearing pair, ID chain, well-formed, safe comment). TC-4 Dorico playback = **human gate, not run**. |
| COMPAT-01 (Windows pluginval-10) | ⚠️ Partial | CI path authored (YAML valid). Dispatch is **outward-facing + release held** → left for user to trigger. macOS already green. |
| PERF-02 (`<5%` CPU/voice) | ✅ Achieved | Measured **0.615% @44.1k / 0.663% @48k** (256-block, defaults, 1 voice). Logic corroboration optional. |
| FUNC-03 + DSP-10 (subjective) | ⚠️ Pending | AUDITION.md rig authored; DSP evidence in place (onset ~1168 ms, no note-on click). Ears-on A/B = **human gate, not run**. |
| Version/docs reconciliation | ✅ Achieved | CHANGELOG 1.0.0, parameter-spec 4-voice, NOTES deferrals, registry.yaml 1.0.4 — all confirmed. |
| DSP FROZEN (19/19 goldens) | ✅ Held | `reproduce-goldens.sh` → **19/19 byte-identical** at HEAD. |

---

## Requirements Verification

**Stage:** 4-polish
**Requirements for this stage:** 6 (FUNC-03, FUNC-04, PERF-02, COMPAT-02 + COMPAT-01 remainder + DSP-10 subjective carry-forward)

| Requirement | Priority | Status | Basis |
|-------------|----------|--------|-------|
| FUNC-04: 10 factory presets (5 orch + 5 drone) | should | ✅ Complete | 10 presets seeded; skew/linear/choice/bool round-trip all exact; `convertTo0to1` loop verified. Subjective ×8 QA is an optional refinement loop, non-blocking. |
| PERF-02: `<5%` CPU/voice @ 44.1/48 kHz, 256-block | should | ✅ Complete | 0.615% / 0.663% median — far under budget. Automated harness is the metric; Logic spot-read is optional corroboration. |
| FUNC-03: dual orchestral + drone from one engine | must | ⏸️ Pending (human) | Purely subjective; requires ears-on A/B vs reference libraries. AUDITION.md P1/P3/P4 unfilled. |
| DSP-10: slow expressive attack, natural | must | ⏸️ Pending (human) | DSP evidence solid (1168 ms onset, no note-on click, Gate 8a). Final character sign-off is subjective — AUDITION.md P2 unfilled. |
| COMPAT-02: Dorico NE microtonal playback | must | ⏸️ Pending (human) | Bundle structurally correct; TC-4 24-EDO quarter-sharp smoke test needs Dorico + human ears. |
| COMPAT-01: Windows VST3 pluginval-10 | must | ⏸️ Partial (human) | CI authored; dispatch is outward-facing + user holds release. macOS VST3+AU already SUCCESS. |

**Requirements Summary:**
- ✅ Complete: 2 (FUNC-04, PERF-02)
- ⏸️ Pending / Partial (human gate): 4 (FUNC-03, DSP-10, COMPAT-02, COMPAT-01)
- ❌ Failed: 0

---

## Automated Checks (all re-run fresh at this verify)

| Check | Result | Notes |
|-------|--------|-------|
| 19/19 render goldens byte-identical | ✅ PASS | `reproduce-goldens.sh` → "OK: all 19 goldens reproduce byte-identical". Frozen-DSP invariant held through the entire stage. |
| auval `aumu OCbs OuDv` | ✅ SUCCEEDED | AU validation, dev triple. |
| pluginval strictness-10 (macOS VST3, warm) | ✅ SUCCESS | Exit 0, 0 failures, full SR × block matrix incl. sub-block fuzz. |
| ui_frontend_check.js | ✅ 14/14 | Includes bridge gate + scaled-value knob readout + resource-provider coverage. |
| Bridge gate (JS ↔ C++ native fns) | ✅ 32 = 32 | 2 mockup + 10 preset + 20 tuning, no dead registrations. |
| PERF-02 `--perf` @44.1k / 48k, 256-block | ✅ PASS | 0.615% / 0.663% median CPU/voice (budgets 5805µs / 5333µs; medians ~35µs). |
| Factory presets seed + round-trip | ✅ PASS | 10/10 on disk; Forte BOW_PRESSURE 0.629465 → 3.2; DETUNE_A 0.585 → +204¢; TUNING_SYSTEM 1.0 = idx 2 (12-TET). |
| Dorico bundle structure | ✅ PASS | 5 files; 3 XML well-formed; load-bearing `<pitchBendRange>2` + `<microtonalPlaybackMethod>kVST3NoteExpression`; pluginID `ABCDEF…436273`; comment inside `<kScoreLibrary>` (crash-safe). |
| CMake Dorico install rule | ✅ PASS | `install(DIRECTORY Resources/dorico DESTINATION share/O-Contrabass COMPONENT Dorico)` present. |
| CI validate-only + Windows pluginval-10 | ✅ Authored | `workflow_dispatch` (plugin_name/version/validate_only) + parse-tag + Windows pluginval v1.0.3 `--strictness-level 10 --timeout-ms 600000`. YAML valid. Not yet dispatched. |
| Docs / version | ✅ PASS | CHANGELOG `[1.0.0]`; compiled version 1.0.0 (Info.plist + moduleinfo.json); parameter-spec `kNumVoices=4`; NOTES v1.1 deferrals; registry.yaml preset-manager 1.0.4. |

---

## Human Verification (remaining gates — cannot be auto-completed)

- [ ] **COMPAT-01 — Windows pluginval-10 via CI.** Commit + push the workflow changes, then
      `gh workflow run build-and-release.yml -f plugin_name=O-Contrabass -f validate_only=true`.
      Confirm Windows build + pluginval-10 pass **and the WebView UI is not blank** (first-ever
      real-Windows visual test). No public Release is published on this path. *(Outward-facing;
      left un-pushed because the user is holding release.)*
- [ ] **COMPAT-02 — Dorico TC-4 (P0).** Run `Resources/dorico/SMOKE-TEST.md`; 24-EDO
      quarter-sharp must play at correct microtonal pitch (the only check that catches a dropped
      top-level microtonal field).
- [ ] **FUNC-03 / DSP-10 — subjective sign-off.** Run `AUDITION.md` in Logic (P1 orchestral,
      P2 slow attack, P3 drone, P4 A/B); record CONFIRM/REVISE.
- [ ] **5 Logic manual checks** carried from Stage 3 (editor open/close ×10, 31-param interaction,
      picker UAF, Logic smoke, visual QA @1000×650) + a Logic CPU-meter spot-read (PERF-02 corroboration).

---

## Issues Found

- **None blocking.** No golden drift, no pluginval/auval failure, no bridge/UI regression, no
  skew mis-seed. All in-scope code, docs, and distribution artifacts verified present and correct.
- **Flag (non-blocking):** two stale differently-named AU variants remain installed
  (`O-Contrabass-pre-2-5-dev.component`, `O-Contrabass-pre-port.component`) — different AU
  subtypes, so **non-shadowing**. Optional cleanup.
- **Nothing committed/pushed.** The v1.1 deferral list (STRING_TENSION inert, `.tun` parser,
  DSP-07/08/09 depth, FUNC-07 MTS-ESP stub, Dorico CC11 dynamics) remains deferred per the
  Do-NOT-touch list — verified untouched (goldens byte-identical).

---

## Stage Verdict

**Status:** ⚠️ PARTIAL — automated verification COMPLETE (all green); 4 human-in-the-loop gates PENDING.

**Ready to ship v1.0.0:** Not yet — blocked only on the 4 human gates above. Everything
auto-verifiable is green; the plugin is code-complete and automated-validated.

**Blockers to final sign-off:**
1. Windows pluginval-10 via CI (COMPAT-01) — push + `workflow_dispatch` (user decision; release held).
2. Dorico TC-4 smoke test (COMPAT-02, P0).
3. FUNC-03 / DSP-10 subjective audition (AUDITION.md).
4. 5 Logic manual checks + PERF-02 Logic corroboration.
