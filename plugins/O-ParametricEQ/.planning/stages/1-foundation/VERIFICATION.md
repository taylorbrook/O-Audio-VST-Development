# Stage 1: Foundation - Verification

## Verification Date

2026-11-12 (verify session; independent re-run against `455f8af5`, fresh compile of this plugin's translation units, reinstalled 0.1.0 `-dev` bundles)

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md)

1. The plugin builds as VST3 + AU + Standalone.
2. The frozen 196-parameter layout is registered in `parameter-spec.md` order (D1 zero-padded IDs, D2 analyzer mode as UI state, D3 no DSP answer encoded).
3. Audio passes through unchanged; latency 0; `bypass` is the host bypass parameter; only mono→mono and stereo→stereo are accepted.
4. The v1 state format round-trips exactly and loads the three foreign states (unknown parameter, missing parameter, `stateVersion="2"`). Gate G11 passes (DSP-07).
5. pluginval strictness 10 on VST3 and AU, and `auval`, pass (COMPAT-01).
6. Shell editor only (D4): generic editor, WebView plumbing wired, nothing tracked under `Source/ui/`.
7. Suite traps avoided: no parameter written from a listener, no first-order `IIR::Filter` constructor, no lambda range, target resolves, reserved IDs unregistered and listed beside the layout.

### Deliverables (from SUMMARY.md, confirmed in the tree)

1. `CMakeLists.txt`: `aufx OuPQ OuDv`, 0.1.0, VST3 / AU / Standalone, plus `O-ParametricEQ-render-harness` and `O-ParametricEQ-param-dump`.
2. `Source/PluginProcessor.{h,cpp}`: table-driven layout, 25 groups, every parameter `ParameterID { id, 1 }`, interval 0, `setSkewForCentre` on `freq` and `q`.
3. `passThrough()` shared by `processBlock` and `processBlockBypassed`; reads no parameter, touches no sample on the two accepted layouts.
4. v1 state codec: root `stateVersion` / `pluginVersion` / `uiLanguage`, one `UI` child, validated UI values, default fill, unknown children kept.
5. Installed `-dev` VST3 and AU; validators green.
6. `createEditor()` returns `GenericAudioProcessorEditor`; `Source/ui/` holds only the two configure-copied, gitignored module scripts.
7. `tests/render-harness/main.cpp` (36 rows), `tests/golden/params.tsv`, `tests/golden/check_spec.py`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Builds | ✅ Achieved | The five plugin-owned objects were deleted and rebuilt: rc 0, no warning in any `O-ParametricEQ/` file. Standalone alive after 6 s. |
| 2 Layout | ✅ Achieved | Harness L1–L3 pass; `--dump-params` diff against the golden is empty; `check_spec.py` → `OK: 196 params (50 Bool / 72 Choice / 74 Float), 25 groups`. A second parser written in this session (not `check_spec.py`) matched all 196 rows on index, ID, name, type, group and choice count, and recomputed both skews (0.2032145, 0.1866524). |
| 3 Pass-through, buses, bypass | ✅ Achieved | P1 48 / 48 `memcmp`-equal, P1n, L4, B1. |
| 4 State / G11 | ✅ Achieved | S1–S8 pass: 196 / 196 `getValue()` bit-equal, byte-equal re-save (8140 bytes), unknown child kept, missing parameter at default, `stateVersion="2"` loads. |
| 5 COMPAT-01 | ✅ Achieved | Re-run on the reinstalled bundles (installed binaries hash-equal to the build): pluginval 10 VST3 ×2 `SUCCESS`, AU `SUCCESS`; `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`, capabilities `[1, 1] [2, 2]`; `ZZZZ` control rc 2. |
| 6 Shell editor | ✅ Achieved | No `PluginEditor*`; `git status` clean under the plugin; check-i18n `ALL CHECKS PASS — 48 localized plugin(s)` (unchanged). |
| 7 Traps | ✅ Achieved | Token grep clean (`--exclude-dir=modules`), no lambda range, `setLatencySamples` ×1, `_dyn_` ×3 in the comment, declaration count 1, target resolves to `OuariconParametricEQ`, `OuPQ` unique in the repo. |

## Requirements Verification

**Stage:** stage-1
**Requirements for this stage:** 2 total (1 must, 1 should, 0 nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: pluginval (VST3 + AU) and auval | must | ✅ Complete | Both criteria met on the installed build |
| DSP-07: band engine and state format leave room for dynamic EQ | should | ⚠️ Partial | State-format and layout half met (G11: hint 1 everywhere, 72 IDs reserved and unregistered, unknown `PARAM` kept across a v1 save, missing parameter defaults, later `stateVersion` loads). The band-engine half (`Band::dynOffsetDb`, ARCHITECTURE "Engine room") has no code until Phase 2.2 and is checked there. |

**Requirements Summary:**
- ✅ Complete: 1
- ⚠️ Partial: 1 (remaining half is Stage 2 work by construction)
- ⏸️ Deferred (later stage): 25
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Spec hash | ✅ Pass | `60e7cf7f…c6229f`, unchanged |
| Build (forced recompile) | ✅ Pass | rc 0. The only warnings are five libc++ "platform no longer supported" lines from the SDK header (deployment target 10.13, suite-wide) |
| Render harness | ✅ Pass | `36 PASS, 0 FAIL`; rows L1–L5, B1, P1, P1n, S1–S8 all present |
| Golden diff / shape | ✅ Pass | Diff empty; 197 non-comment lines; no CR |
| `check_spec.py` | ✅ Pass | exit 0 |
| Independent spec ↔ golden parse | ✅ Pass | 0 mismatches, 196 unique IDs |
| Static gates | ✅ Pass | See goal 7 |
| Install | ✅ Pass | rc 0; exactly the two `-dev` bundles; installed = built (sha1) |
| pluginval 10 | ✅ Pass | VST3 ×2, AU ×1 |
| auval | ✅ Pass | `[1, 1] [2, 2]`; negative control fails as expected |
| Standalone smoke | ✅ Pass | alive after 6 s |
| check-i18n | ✅ Pass | 48, unchanged |
| CHANGELOG / PLUGINS.md / registry | ✅ Pass | `## [0.1.0] - 2026-11-12`; one row, no duplicate; `used_by` committed under `preset-manager` and `vu-meter` |
| Commit hygiene | ✅ Pass | `455f8af5` lists only this plugin's paths plus `PLUGINS.md` and `modules/registry.yaml`; no tag on HEAD |

### Mutation run (new in verify)

Eight copies of `PluginProcessor.cpp` were mutated in the session scratchpad and linked against the unmodified harness objects. Nothing in the repo or in `build/` was changed.

| Mutation | Harness | Caught by |
|---|---|---|
| M0 unmodified control | 36 / 0 | (none, as expected) |
| M3 `isBusesLayoutSupported` accepts everything | 35 / 1 | B1 |
| M4 `analyzerMode` range check removed | 35 / 1 | S8 |
| M5 invalid blob resets the state | 35 / 1 | S7 |
| M6 one parameter with version hint 2 | 34 / 2 | L1, L2 |
| M7 gain 0.999999 when not bypassed | 32 / 4 | P1 |
| M8 a reserved `_dyn_` ID registered | 26 / 10 | L1, L2, L3, S1–S4, S6 |
| M1 default fill removed (restore step 5) | 36 / 0 | not caught: see Issues 1 |
| M2 `UI` strip on restore removed (step 4) | 36 / 0 | not caught: see Issues 2 |

## Human Verification

Not blocking: neither Stage 1 requirement has a hands-on criterion. Worth a minute before Stage 2 changes the audio path.

- [ ] Logic (AU): O-ParametricEQ-dev instantiates on a mono and a stereo track, the generic editor opens, audio passes unchanged.
- [ ] One VST3 host: same; the automation list shows the four globals first, then `B1 On` … `B24 Placement`.
- [ ] Save, close and reopen a session with a few parameters moved: values return.

## Issues Found

1. **The default fill in `setStateInformation` (step 5) is redundant under JUCE 8.0.15, and S5 does not gate it.** With the fill removed, S5 still passes: `updateParameterConnectionsToChildTrees` appends the missing `PARAM` child, `valueTreeChildAdded` fires, and `setNewState` applies the denormalised default (`juce_AudioProcessorValueTreeState.cpp:406-452`). RESEARCH pitfall 2 ("a missing parameter keeps its current value", marked VERIFIED) is wrong for this JUCE version. Behaviour is correct with or without the fill; S5 itself is sound (pre-set guard, both parameters confirmed non-default before the restore). No change made: the fill is harmless and protects against a JUCE change. Do not cite RESEARCH pitfall 2 as a JUCE fact in later stages.
2. **The `UI` strip on restore (step 4) is redundant with the strip on save.** Equivalent mutant: the save path removes every `UI` child from its copy before appending the fresh one, so S2 / S3 hold either way. No change made.
3. **DSP-07 cannot be fully closed at Stage 1.** Its wording includes the band engine, which does not exist yet. Marked partial; the remaining half belongs to Phase 2.2. ROADMAP's coverage table lists DSP-07 under Stage 1 only, so Stage 2 plan should add a row for the `dynOffsetDb` hook.
4. **Carried from SUMMARY, unchanged:** P3 means a session saved by a later version, opened and re-saved in v1, keeps its dynamic-EQ `PARAM` children but is stamped `stateVersion="1"`. A later version's migration must therefore not overwrite dynamic-EQ values on the strength of the version number alone. Non-numeric UI strings convert to 0 (Stage 3 call).
5. **Installed VST3 differed from the rebuilt one before reinstall** (the forced relink produced a different binary). All validators above ran after `build-and-install.sh`, on hash-equal bundles.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Blockers (if any):** none. Open, non-blocking: the three hands-on items above; DSP-07 engine half (Phase 2.2).
