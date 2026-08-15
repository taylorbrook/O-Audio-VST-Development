# Stage 1 — Foundation: VERIFICATION

**Plugin:** O-Octagon
**Stage:** 1 of 4 — Foundation + Shell
**Phase:** verify
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `bb8086c9`

**Method:** every automated gate below was **re-run from scratch during this phase**, not read out of
SUMMARY.md. Where a claim could not be re-derived, it is marked as such.

---

## Goal-Backward Analysis

### Original goals (CONTEXT.md, PLAN.md)

1. A loadable, validating 8-channel shell — correct bus declaration and negotiation predicate
2. All 17 APVTS parameters with correct names, ranges, defaults, units and grouping
3. Session-state round-trip, written once, not revisited when Phase 2.1 adds the `VENUE` child
4. **No DBAP** — prove the transport before any geometry depends on it

### Deliverables (SUMMARY.md + code inspection)

1. `CMakeLists.txt` → target `OuariconOctagon`, three formats, no `PLUGIN_CHANNEL_CONFIGURATIONS`,
   no SAF; `isBusesLayoutSupported()` at `PluginProcessor.cpp:138-168`
2. `createParameterLayout()` at `PluginProcessor.cpp:54-107` — 17 `AudioParameterFloat` in five groups
3. `getStateInformation` / `setStateInformation` at `PluginProcessor.cpp:256-284`
4. `processBlock` at `:184-224` is a marked placeholder; `Source/` holds exactly two files

### Goal achievement

| Goal | Status | Evidence |
|---|---|---|
| 8-channel shell loads and validates | ✅ Achieved | `auval` SUCCEEDED; pluginval s10 × 3 both formats; AU config set `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` |
| 17 parameters, correct in every field | ✅ Achieved | Programmatic diff of the `auval` dump against `parameter-spec.md` — 17/17 on name, range, default **and** clump |
| State round-trip | ✅ Achieved | pluginval *Plugin state* + *Plugin state restoration* both completed at strictness 10, ×3 |
| No DBAP; non-goals honoured | ✅ Achieved | `Source/` = `PluginProcessor.{h,cpp}` only; no `ChannelMap`/`VENUE`/`SmoothedValue`/harness/WebView |
| 8 discrete feeds in a real host | ⚠️ **Not verified** | Task 13 (Logic) not run — see *Issues* |

---

## Requirements Verification

**Requirements mapped to stage-1:** 3 (FUNC-01 must, COMPAT-01 must, COMPAT-04 should)

| Requirement | Priority | Status | Acceptance criteria |
|---|---|---|---|
| FUNC-01: mono/stereo in → 8 discrete feeds via `create7point1()` carrier | must | ⚠️ **Partial** | 2 of 3 met; AC-3 is unachievable at Stage 1 by construction — see below |
| COMPAT-01: pluginval VST3 + AU, strictness 10 | must | ✅ Complete | All 3 criteria met |
| COMPAT-04: defined, non-crashing on a stereo track | should | ✅ Complete | SAFE clause accepts `stereo()`; exercised by `auval` and by Standalone launch |

### FUNC-01 — detail

| Acceptance criterion | Result |
|---|---|
| `isBusesLayoutSupported()` accepts mono-in/7.1-out and stereo-in/7.1-out | ✅ `auval` reports `[1, 8]` and `[2, 8]` |
| Rejects layouts whose output count is not 8, except the stereo fallback | ✅ The derived AU set is **exactly** six configs — the predicate admits nothing else |
| **All 8 output channels carry independent, non-duplicated signal for an off-centre source** | ❌ **Structurally impossible at Stage 1.** The D1 placeholder writes the *same* mono sum to all 8 by design. Independence requires the DBAP solve (DSP-01/DSP-05, stage-2) |

**Finding:** REQUIREMENTS.md maps FUNC-01 to `stage-1`, but its third acceptance criterion cannot be
satisfied until Stage 2 exists. This is a requirements-mapping defect, not an implementation defect —
Stage 1 delivered everything FUNC-01 can deliver. **FUNC-01 is re-mapped to `stage-2`** and its first
two criteria are recorded as already met here.

Note that Task 13's "all 8 lanes move" is evidence of *negotiation and writability*, not of
independence — with the placeholder in place all 8 lanes carry identical signal. SUMMARY.md
characterises it correctly.

### COMPAT-01 — detail

| Acceptance criterion | Result |
|---|---|
| VST3 passes pluginval strictness 10 | ✅ SUCCESS × 3 (re-run this phase) |
| AU passes pluginval strictness 10 and `auval` | ✅ SUCCESS × 3; `AU VALIDATION SUCCEEDED` |
| Run 2–3× locally before publish | ✅ 3 runs per format, per `pattern_ci_pluginval10_catches_latent_nan` |

### COMPAT-04 — detail

`PluginProcessor.cpp:162-163` admits `mono()` and `stereo()` output. `auval` exercised the `(1,1)`,
`(1,2)`, `(2,1)` and `(2,2)` configs and passed — SAFE mode is load-bearing for **AU**, not only for
Standalone. The Standalone was launched this phase on a 2-channel default output device
(MacBook Pro Speakers, 2 out) and remained running. Defined, non-crashing: confirmed.

**Requirements summary:** ✅ Complete 2 · ⚠️ Partial 1 (re-mapped to stage-2) · ❌ Failed 0

---

## Automated Checks — all re-run this phase

| Check | Command | Result |
|---|---|---|
| Clean rebuild, 3 formats | forced recompile of both TUs, then `cmake --build build --target OuariconOctagon_{VST3,AU,Standalone}` | ✅ **0 warnings, 0 errors** in the whole log |
| AU registration | `auval -a \| grep -i octagon` | ✅ `aufx OuOc OuDv — Ouaricon Audio Development: O-Octagon-dev` |
| AU full validation | `auval -v aufx OuOc OuDv` | ✅ `AU VALIDATION SUCCEEDED` |
| AU channel config set | from the `auval` dump | ✅ `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` — **exactly RESEARCH F2's prediction** |
| Parameter count | `grep -c "parameter PASS"` | ✅ 17 |
| 17 params vs `parameter-spec.md` | programmatic diff of the `auval` dump | ✅ 17/17 on name, range, default **and** group |
| pluginval VST3 | strictness 10, `--skip-gui-tests`, ×3 | ✅ SUCCESS ×3 |
| pluginval AU | strictness 10, `--skip-gui-tests`, ×3 | ✅ SUCCESS ×3 |
| State round-trip | pluginval *Plugin state*, *Plugin state restoration* | ✅ both completed, ×3 |
| Standalone COMPAT-04 | launched on a 2-ch default device | ✅ opens, stays running |
| `PHASE-2.2-REPLACE` uniqueness | `grep -rn` across the plugin | ✅ exactly 1 occurrence |
| Forbidden CMake keywords | `grep` for `PLUGIN_CHANNEL_CONFIGURATIONS`, `PLUGIN_VERSION`, SAF | ✅ none set — only the explanatory comments |
| `setLatencySamples()` | `grep -rn Source/` | ✅ never called — only the "NEVER call" comment |
| Non-goals honoured | `ls Source/`, `grep` for the eight banned components | ✅ two files; no `PluginEditor`, no `ChannelMap`, no `VENUE` |
| Registry | `PLUGINS.md:68` | ✅ `🚧 Stage 1 \| 1.0.0-dev \| 2026-08-11` |
| Working tree | `git status` | ✅ Stage 1 fully committed at `bb8086c9` |
| Contract checksums | `shasum -a 256` × 4 vs STATUS.md frontmatter | ⚠️ 3 of 4 byte-exact; `parameter_spec` pinned the wrong file — **fixed this phase**, see *Issues 7* |

### Parameter diff — full result

```
Position  Source X   [0.0, 1.0]     def=0.5      ✓
Position  Source Y   [0.0, 1.0]     def=0.5      ✓
Position  Source Z   [-2.0, 8.0]    def=0.0      ✓
Position  Width      [0.0, 6.0]     def=0.0      ✓
Solve     Blur       [0.0, 1.0]     def=0.1      ✓
Solve     Rolloff    [3.0, 6.0]     def=4.0      ✓
Weights   Weight 1-8 [0.0, 1.0]     def=1.0      ✓ (×8)
Space     Hull Atten [0.0, 3.0]     def=1.0      ✓
Space     Air        [0.0, 1.0]     def=0.35     ✓
Output    Output     [-24.0, 12.0]  def=0.0      ✓
```

`outputGain`'s default reads back as `7e-07` rather than exactly `0.0` — that is AU's normalised
float round-trip of 0 dB in a −24…+12 range, not a spec deviation.

---

## Human Verification — outstanding

- [ ] **Task 13 — Logic 8-channel negotiation.** The one Stage 1 exit criterion still open.
  - [ ] Appears in the plugin menu and instantiates on a 7.1 / 7.1-SDDS / 5.1.2 track
  - [ ] All 8 lanes of the surround meter move
  - [ ] `outputGain` moved off default survives save → close → reopen (FUNC-05 slice)
  - [ ] Automation menu lists 17 parameters under the five groups
  - [ ] **Record which container Logic negotiated** — R2 predicts 7.1-SDDS. Observation, not a gate;
        feeds COMPAT-02 at Stage 4
- [ ] **Task 12 item 3 — audio at unity through Standalone.** JUCE's Standalone mutes input by
      default; needs a manual unmute. Subsumed by Task 13's meter check if that runs first.

---

## Issues Found

1. **FUNC-01 is mis-staged in REQUIREMENTS.md.** Its third acceptance criterion — 8 *independent*
   channels — requires the DBAP solve and cannot be met by a shell whose placeholder duplicates by
   design. **Resolution:** `verifiedAt` moved to `stage-2`; the first two criteria are recorded as
   already met at Stage 1 so Stage 2 does not re-derive them.

2. **Task 13 (Logic) not run.** Not automatable; requires Logic and a surround track. Stage 1 cannot
   be declared fully exited until it does. **Unresolved — it is the blocker below.**

3. **Task 12 item 3 unverified.** Unity gain through the placeholder is confirmed *by inspection*
   only (`:217-222` — mono sum read before write, written to every channel at unity). No offline
   measurement exists because the render harness is a deliberate Phase 2.2 non-goal. Machine
   confirmation arrives with that harness.

4. **F3 hazard not exercised.** No 3–7-output interface was available. The defence — bounding the
   output loop by `buffer.getNumChannels()` at `:206` — is present and correct in source, but
   reasoned rather than measured. Carry to Stage 2.

5. **Gate bypass on record.** The `0-ideation → 1-foundation` gate was run with `--force`; its build
   check is unconditional on target stage and cannot pass before `CMakeLists.txt` exists. Logged to
   `.planning/gate-bypasses.log`. Three `set -e` crash fixes to `run-gate.sh` were made and committed
   (they blocked the 0→1 gate for *every* new plugin); making the build check stage-aware is a
   semantic change to shared infrastructure and was deliberately **not** made. Open decision.

6. **Known-benign.** pluginval on the AU emits `WARNING: Current program is -1`. JUCE AU-wrapper
   program reporting, present across the repo; the run still returns SUCCESS.

7. **`contract_checksums.parameter_spec` pinned the superseded draft. FIXED this phase.**
   Three of the four recorded contract checksums verified byte-exact. The fourth did not — against a
   file `git diff bb8086c9 HEAD` shows is **unchanged**, so the recorded value was never valid for it.

   | | SHA-256 |
   |---|---|
   | Recorded as `parameter_spec` | `5c5f4f06…7022f` |
   | Actual `parameter-spec.md` | `b45f88dc…9e02f` |
   | `parameter-spec-draft.md` **at `bb8086c9^`** (pre-banner) | `5c5f4f06…7022f` ← **exact match** |

   The checksum meant to pin the promoted spec was pinning **the stale draft, as it stood before the
   superseded banner was added** — the very file PLAN P4 exists to stop anything from reading, and
   which still marks OQ3/4/5 and the 17-vs-18 count as *open*. A future contract-drift check would
   have validated against the wrong document and passed while doing it — and drift in the *promoted*
   spec, the one the code is built from, would have gone undetected indefinitely.

   Nothing downstream consumed the bad value: the shipped binary matches `parameter-spec.md` 17/17,
   which is the check that actually matters and it was made directly against the binary.

   **Resolution:** frontmatter updated to `b45f88dc…9e02f`. Verified this phase that the other three
   (`brief`, `architecture`, `roadmap`) are byte-exact.

   **Carry to Stage 2:** whatever writes `contract_checksums` hashed the file the promotion *replaced*
   rather than the file it *produced*. Re-verify all four checksums at every stage boundary — a
   checksum that silently points at the wrong file is worse than no checksum, because it reports
   green.

---

## Stage Verdict

**Status:** ⚠️ **PARTIAL**

Every automated gate passes, independently re-run, with zero failures. The 17-parameter contract,
the bus predicate, the AU config derivation and the state round-trip are all confirmed against the
shipped binary rather than against the source. The shell is real and it validates.

What holds the verdict at PARTIAL is a single **manual** gate: Task 13. It is the only Stage 1 exit
criterion not met, and it is the one that closes the loop on the whole point of the stage — that a
real host negotiates 8 channels.

**Ready for next stage:** **Yes, with one caveat.** Nothing in Stage 2 is blocked by Task 13 —
Phase 2.1 builds `ChannelMap` and the `VENUE` tree, neither of which depends on the Logic result
(the plugin accepts all three containers regardless of which one Logic picks). Running Task 13 now
is nonetheless the cheaper order: if Logic fails to negotiate 8 channels, the fault is in the bus
predicate, and finding that after a DBAP solver exists is materially more expensive to unpick.

**Blockers:**
- Task 13 — Logic 8-channel negotiation. Blocks the Stage 1 *sign-off*, not Stage 2 *start*.

---

## Carried into Stage 2

Unchanged from PLAN.md §Carried forward. The two Stage 1 makes concrete:

- **[2.1]** F1 holds for all three accepted containers — initializer order == enum-bit order — so a
  hardcoded 0..7 map is byte-identical to a correct one under every accepted layout. The channel-map
  test **must** drive non-identity `map1..map8` label maps; a container-only test is vacuous
  (`critical_audiochannelset_is_a_bitset_not_an_order`).
- **[2.1]** The `PHASE-2.2-REPLACE` block at `PluginProcessor.cpp:188-223` is the single hardcoded
  output index in the plugin. Phase 2.1's "zero hardcoded output indices outside `ChannelMap`" gate
  is expected to fail against it — **retire it, do not grandfather it.**
- **[2.2]** Output loop bound is `buffer.getNumChannels()` — never `8`, never
  `getTotalNumOutputChannels()`, which is the accessor that lies under F3.
- **[2.2]** The render harness closes issues 3 and 4 above. Give it a unity-gain-through-all-outputs
  case at 1, 2 and 8 output channels.
