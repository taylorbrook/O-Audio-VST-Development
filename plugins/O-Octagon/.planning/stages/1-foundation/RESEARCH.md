# Stage 1 — Foundation: RESEARCH

**Plugin:** O-Octagon
**Stage:** 1 of 4 — Foundation + Shell
**Phase:** research ✓
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`

**Method.** Every JUCE claim below was read out of the local JUCE 8.0.14 tree at
`/Users/taylorbrook/JUCE` and is cited `file:line`. Nothing here is recalled from
training or inferred from documentation. Repo claims were re-verified against the
working tree, not taken from STATUS.md.

**Scope discipline.** This stage builds a *shell*. Findings below that belong to
Stage 2/3 are marked **[carry]** and are recorded only because Stage 1 fixes a
decision they depend on (member order, serialisation shape, warning flags). They
are not Stage 1 work.

---

## 0. Headline findings

| # | Finding | Impact |
|---|---------|--------|
| **F1** | All **three** accepted 8-channel containers have initializer-list order identical to enum-bit order — not just 7.1 | Amplifies R1. No accepted layout can expose a hardcoded 0..7 map. **[carry to 2.1]** |
| **F2** | AU channel configs are *derived*, and SAFE mode is what creates them: `AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}` | `auval` exercises all six. The Stage 1 placeholder must be correct at 1-out and 2-out, not only 8 |
| **F3** | Standalone reaches SAFE mode through `canonicalChannelSet(n)`; **n ∈ {3,4,5,6,7} is rejected → Release-mode out-of-bounds write** | Bound every output loop by `buffer.getNumChannels()`. Never by 8, never by `getTotalNumOutputChannels()` |
| **F4** | `canonicalChannelSet(8) == create7point1()` | Standalone on an 8-out interface negotiates **real mode** with no host. Free Stage 2 test rig |
| **F5** | `create5point1point2()` has **no** VST3 layout-table entry; it resolves through the generic bit-order fallback | Works, but it is the least-exercised path of the three. Do not make it the primary |
| **F6** | JUCE hoists `Fx` to index 0 of `VST3_CATEGORIES` | The emitted string is always `Fx\|Spatial`. Declaring `"Spatial" "Fx"` does not produce Steinberg's `kSpatialFx` |
| **F7** | `AU_MAIN_TYPE kAudioUnitType_Effect` is already JUCE's default for this plugin kind | Keyword is redundant. Harmless; keep it as documentation |
| **F8** | `-Wswitch-enum`, `-Wfloat-equal`, `-Wconversion` are all in `juce_recommended_warning_flags` | Three concrete code shapes are pre-banned by the zero-warning exit criterion. §7 |

---

## 1. Correction and confirmation of inherited claims

### 1.1 STATUS.md finding #2 is correct — and understated

STATUS.md records that *"for 7.1 the enum-bit order coincidentally equals the
initializer-list order."* Verified, and it holds for all three accepted containers.

`getChannelIndexForType()` walks set bits ascending
(`juce_audio_basics/buffers/juce_AudioChannelSet.cpp:514-527`), so buffer order is
enum-bit order. The enum values (`juce_AudioChannelSet.h`, enum `ChannelType`):

```
left=1  right=2  centre=3  LFE=4  leftSurround=5  rightSurround=6
leftCentre=7  rightCentre=8  centreSurround=9
leftSurroundSide=10  rightSurroundSide=11
leftSurroundRear=20  rightSurroundRear=21
topSideLeft=28  topSideRight=29
```

Against the memberships at `juce_AudioChannelSet.cpp:567, 568, 574`:

| Set | Initializer list | Enum bits | Ascending? |
|-----|------------------|-----------|------------|
| `create7point1()` | L, R, C, LFE, Lss, Rss, Lsr, Rsr | 1, 2, 3, 4, 10, 11, 20, 21 | **yes** |
| `create7point1SDDS()` | L, R, C, LFE, Ls, Rs, Lc, Rc | 1, 2, 3, 4, 5, 6, 7, 8 | **yes** |
| `create5point1point2()` | L, R, C, LFE, Ls, Rs, Tsl, Tsr | 1, 2, 3, 4, 5, 6, 28, 29 | **yes** |

**Consequence (F1).** A hardcoded `0..7` map is byte-identical to a correct
`getChannelIndexForType()` map under *every* layout this plugin accepts. Switching
containers cannot discriminate. The only thing that permutes the buffer is a
non-identity `map1..map8` label assignment (ARCHITECTURE §6.2), so the Phase 2.1
channel-map test must drive **synthetic non-identity label maps** — a test that only
varies the container is vacuous. **[carry to 2.1]**

This is the same failure shape as `pattern_worktree_isolation_wrong_for_untracked_scope`:
a gate that passes because it had nothing to find.

### 1.2 `PLUGIN_CODE OuOc` — re-verified

39 `PLUGIN_CODE` values across `plugins/*/CMakeLists.txt`, all distinct, none `OuOc`.
`plugins/` currently holds 40 directories; O-Octagon is the one without a CMakeLists.

### 1.3 ARCHITECTURE §13 file:line references — spot-checked, all hold

`juce_AudioChannelSet.cpp:514-527`, `:567`, `:568`, `:572`;
`juce_AudioChannelSet.h:577`, `:581`, `:199`, `:209`, `:221`;
`juce_AudioProcessorValueTreeState.h:391` (`copyState`), `:404` (`replaceState`).

**Path note for the plan:** JUCE 8.0.14 splits the processors module. `AudioParameterFloat`
and `AudioParameterFloatAttributes` live at
`juce_audio_processors_headless/utilities/juce_AudioParameterFloat.h`, not under
`juce_audio_processors/utilities/`. `juce_audio_processors` declares
`dependencies: juce_gui_extra, juce_audio_processors_headless` and `#include`s it
(`juce_audio_processors.h:54, 64`), so the include line in source does not change —
only where you go to read it.

---

## 2. Bus negotiation — what each wrapper actually does

The architecture specifies `isBusesLayoutSupported()`. What follows is what each of
the three formats *does with it*, traced through the wrapper source. This is the
material that turns the three Stage 1 compatibility criteria from hope into a
prediction.

### 2.1 AU — channel configs are derived from the predicate (F2)

`juce_audio_plugin_client_AU_1.mm:175` calls
`AudioUnitHelpers::getAUChannelInfo(*juceFilter)`, implemented at
`juce_audio_processors_headless/format_types/juce_AU_Shared.h:395-527`. It:

1. builds `layoutsToTry` = `AudioChannelSet::channelSetsWithNumberOfChannels(i)` for
   i = 1..16 (`:421-432`, `maxNumChanToCheckFor = 16` at `:374`);
2. probes every (in, out) pair through `processor.checkBusesLayoutSupported()` (`:436-458`);
3. collapses the result to `AUChannelInfo` count-pairs, emitting `-1` wildcards only
   where a whole channel-count band is unrestricted (`:489-525`).

Tracing O-Octagon's predicate through this: `noRestrictions` is false (most pairs
fail), `allMatchedLayoutsExclusivelySupported` is false ((3,3) is unsupported), and
every band is restricted, so no wildcard is emitted. The result is the literal set:

```
AUChannelInfo = { (1,1), (1,2), (1,8), (2,1), (2,2), (2,8) }
```

Three consequences:

- **`PLUGIN_CHANNEL_CONFIGURATIONS` is not merely harmful, it is redundant.** JUCE
  computes this set already. The architecture's prohibition is confirmed from the
  other direction.
- **SAFE mode is load-bearing for AU, not just for Standalone.** Without the
  mono/stereo output clause the AU would publish only `(1,8)` and `(2,8)`.
- **`auval` will exercise all six configs**, including 1-in/1-out. The Stage 1
  placeholder must therefore behave at 1 and 2 output channels, not only at 8. See F3.

`channelSetsWithNumberOfChannels(8)` returns `{7.1, 7.1-SDDS, octagonal, 5.1.2}`
(`juce_AudioChannelSet.cpp:700-703`) — `octagonal()` is offered by JUCE and correctly
rejected by the predicate, consistent with the locked constraint.

### 2.2 VST3 — arrangement mapping, and why the wire order is not our problem (F5)

`juce_audio_processors_headless/format_types/juce_VST3Common.h`, `detail::layoutTable`:

| JUCE set | VST3 arrangement | Table line | VST3 channel order |
|---|---|---|---|
| `create7point1()` | `k71Music` | `:307` | L, R, C, LFE, **Lsr, Rsr, Lss, Rss** |
| `create7point1SDDS()` | `k71Cine` | `:308` | L, R, C, LFE, Ls, Rs, Lc, Rc |
| `create5point1point2()` | *(no entry)* | — | generic fallback, `:355-383` |

Two things follow.

**The VST3 wire order for `k71Music` is not the JUCE buffer order.** VST3 puts the
rear pair before the side pair; JUCE's enum-bit order puts sides (10, 11) before rears
(20, 21). JUCE reconciles this itself via `ChannelMapping` (`juce_VST3Common.h:459`),
so the plugin always sees enum-bit order — **which is exactly why addressing must go
through `getChannelIndexForType()` on the JUCE-side set and never through any
remembered wire order.** This is independent corroboration of the locked constraint
from a second format.

**5.1.2 has no hard-coded conversion.** `getSpeakerOrder()` (`:355`) falls through to
*"assume that the channels are in the same orders in both layouts"* (`:370-381`),
rebuilding the order from individual speaker bits. It works — `topSideLeft ↔ kSpeakerTsl`
is mapped at `:144` / `:254` — but it is the least-exercised of the three. Keep 7.1 as
the declared default; treat 5.1.2 strictly as a fallback.

### 2.3 Standalone — the COMPAT-04 mechanism, and a real hazard (F3, F4)

The chain: `StandalonePluginHolder` calls
`deviceManager.initialise(inputChannels, outputChannels, …)` with
`outputChannels = processor->getMainBusNumOutputChannels()` = **8**
(`juce_StandaloneFilterWindow.h:165, 357`). The device manager opens whatever the
hardware actually has; `AudioProcessorPlayer` then calls
`AudioProcessor::setPlayConfigDetails()` with the device's real counts.

`setPlayConfigDetails` (`juce_audio_processors_headless/processors/juce_AudioProcessor.cpp:350-374`)
converts a count to a set with `AudioChannelSet::canonicalChannelSet(n)`
(`juce_AudioChannelSet.cpp:636-648`):

| Device outs | `canonicalChannelSet(n)` | O-Octagon verdict |
|---|---|---|
| 1 | `mono()` | accepted — SAFE |
| **2** | **`stereo()`** | **accepted — SAFE. This is the COMPAT-04 gate** |
| 3 | `createLCR()` | **rejected** |
| 4 | `quadraphonic()` | **rejected** |
| 5 | `create5point0()` | **rejected** |
| 6 | `create5point1()` | **rejected** |
| 7 | `create7point0()` | **rejected** |
| **8** | **`create7point1()`** | **accepted — REAL mode** |

**F4 — the useful half.** Standalone on an 8-output interface negotiates genuine 7.1
with no host at all. That makes the Standalone build a legitimate Stage 2 rig for
speaker-map and DBAP listening checks, months before Logic is in the loop. Worth
knowing now; it does not change Stage 1 scope.

**F3 — the dangerous half.** On a 3–7 output device `setChannelLayoutOfBus` fails.
In Debug, JUCE's own `jassert` at `:358/:364/:371` fires — noisy but safe. In
**Release** the assertions vanish, the processor keeps its 7.1 layout
(`getTotalNumOutputChannels() == 8`) while `AudioProcessorPlayer` hands it a buffer
with only *n* channels. A loop bounded by 8 then writes past the end of the buffer.

> **Rule for Stage 1, and for every stage after it:** the output loop bound is
> `buffer.getNumChannels()`. Not the literal `8`. Not `getTotalNumOutputChannels()` —
> in exactly this failure state that accessor is the lie. **[carry to 2.2]**

This costs nothing and is the difference between a mis-negotiated Standalone that
degrades and one that corrupts memory. It is not a reason to widen SAFE mode: the
architecture's list is settled, COMPAT-04 is a 2-channel gate, and widening would put
quad/5.1 in the AU config list where they would advertise a spatialisation this
plugin does not perform.

---

## 3. The 17 parameters — concrete construction

### 3.1 API shape (verified)

`juce::AudioParameterFloat` takes `AudioParameterFloatAttributes`
(`juce_audio_processors_headless/utilities/juce_AudioParameterFloat.h:44, 76, 98`),
which inherits `withLabel` / `withStringFromValueFunction` / `withValueFromStringFunction`
from `RangedAudioParameterAttributes` (`juce_RangedAudioParameter.h:69`) and
`AudioProcessorParameterWithID.h:87`.

`juce::ParameterID { "id", 1 }` — the version hint is mandatory in JUCE 8 and the
sibling `plugins/O-Orbit/Source/PluginProcessor.cpp:66+` already uses this form.

Two-argument `NormalisableRange<float>(min, max)` gives interval 0, skew 1 — a
continuous linear range, which is what "all skews linear" (ARCHITECTURE §6.1) means.
Do **not** pass a 4-argument form with skew 1.0; it reads as if a skew were intended.

### 3.2 Ranges, defaults and units — the Stage 1 exit criterion

Ranges and defaults are ARCHITECTURE §6.1 verbatim. The `label` column is this
phase's contribution: the criterion says *"correct names, ranges, defaults **and
units**"*, and §6.1 gives units in prose only.

| # | ID | Name | Range | Default | `withLabel` | Display |
|---|----|------|-------|---------|-------------|---------|
| 1 | `srcX` | Source X | 0.0 – 1.0 | 0.5 | *(none)* | 2 dp normalised — see §3.3 |
| 2 | `srcY` | Source Y | 0.0 – 1.0 | 0.5 | *(none)* | 2 dp normalised — see §3.3 |
| 3 | `srcZ` | Source Z | −2.0 – 8.0 | 0.0 | `m` | 2 dp |
| 4 | `width` | Width | 0.0 – 6.0 | 0.0 | `m` | 2 dp |
| 5 | `rolloff` | Rolloff | 3.0 – 6.0 | 4.0 | `dB/2x` | 1 dp |
| 6 | `blur` | Blur | 0.0 – 1.0 | 0.10 | *(none)* | 2 dp |
| 7–14 | `w1`..`w8` | Weight 1..8 | 0.0 – 1.0 | 1.0 | *(none)* | 2 dp |
| 15 | `hullAtten` | Hull Atten | 0.0 – 3.0 | 1.0 | `dB/m` | 2 dp |
| 16 | `airAmount` | Air | 0.0 – 1.0 | 0.35 | *(none)* | 2 dp |
| 17 | `outputGain` | Output | −24.0 – 12.0 | 0.0 | `dB` | 1 dp |

`dB/2x` rather than `dB/doubling`: Logic truncates the unit field hard, and the
architecture's own prose form does not survive it.

The five normalised parameters take no label deliberately — a bare `0.00–1.00` reads
correctly, and inventing a unit ("norm", "%") would misrepresent `w1..w8`, which are
DBAP weights and not percentages of anything.

### 3.3 `srcX` / `srcY` display — decided here, because it constrains the header

ARCHITECTURE §6.1 requires the metres display to *"read the live venue"* and forbids
a static lambda captured at construction. A host-side value→text function is captured
at construction by definition, so the two cannot both be satisfied for the host's
automation lane.

**Decision: the host lane shows normalised 0.00–1.00; metres are a UI-side conversion
at Stage 3.1.** §6.1's mandate is about the *readout*, and it names
`pattern_webview_knob_readout_scaled_value`, which is a UI pattern — the UI asks the
processor for the venue and converts. That satisfies the requirement without
capturing `this` in a parameter lambda before the venue member exists.

**Cheap hedge, free if taken now, expensive later:** declare the venue store member
**before** `apvts` in `PluginProcessor.h`. If a future stage does want a venue-aware
host lambda, the capture is then well-defined; if not, nothing is lost. Member
declaration order is fixed at Stage 1 and is annoying to change once Stage 2 depends
on it. **[carry to 2.1]**

### 3.4 Grouping — small decision for the plan phase

**Recommendation: five `AudioProcessorParameterGroup`s** — Position (`srcX srcY srcZ
width`), Solve (`rolloff blur`), Weights (`w1..w8`), Space (`hullAtten airAmount`),
Output (`outputGain`). The headline gesture is automating eight weights; a flat
17-entry menu buries them, and O-Orbit already establishes the grouped idiom
(`PluginProcessor.cpp:62`). Group membership does not participate in parameter
identity, so this stays reversible.

### 3.5 Parameter-ID hazards — re-checked

None of the 17 IDs shadows a `juce::` free function
(cf. `critical_paramid_shadows_juce_free_function`, where `end`/`begin` collided), and
all 17 are valid C++ identifiers, so ID and member name can match. `width` is a member
name to watch under `-Wshadow-all` (§7).

---

## 4. State serialisation — written once, must survive Stage 2.1

ARCHITECTURE §4.1 gives the round-trip. Two forward-compatibility points the plan
must carry, because the code is written now and not revisited:

**Root type.** ARCHITECTURE §4.1 says the root is `OOctagon`. The sibling uses
`OOrbitParams` (`O-Orbit/PluginProcessor.cpp:32`). **Take the architecture's
`OOctagon`.** It is authoritative, no sessions exist yet, and Stage 2.1 attaches the
`VENUE` child to this exact node — changing the identifier later orphans every saved
session and every `.venue` file written in between.

**Stage 1 sessions have no `VENUE` child.** Every session saved between now and Phase
2.1 restores a tree with 17 parameters and nothing else. Phase 2.1's
`readVenueFromState()` must therefore treat a missing or partial `VENUE` node as
*"use defaults"*, not as an error, and must not assume 8 `SPEAKER` children exist.
Writing that down now is what makes the Stage 1 serialisation code genuinely
write-once. **[carry to 2.1]**

**Do not add an `AsyncUpdater` at this stage.** There is nothing to defer yet. When
Stage 2.1 adds one, `pattern_asyncupdater_guard_flag_needs_cancel` applies:
`cancelPendingUpdate()` in the restore path, or a queued apply stomps restored state.

---

## 5. CMake — deltas from the architecture's block

ARCHITECTURE §12 gives the `juce_add_plugin` call. Three corrections and one
confirmation from reading JUCE's own CMake:

**F6 — `VST3_CATEGORIES` order is not yours to choose.**
`_juce_get_vst3_category_string` (`extras/Build/CMake/JUCEUtils.cmake:1511-1538`)
removes `Fx` from wherever it appears and re-inserts it at index 0, then joins with
`|`. Declaring `"Spatial" "Fx"` emits **`Fx|Spatial`**, not Steinberg's
`kSpatialFx = "Spatial|Fx"` (`VST3_SDK/pluginterfaces/vst/ivstaudioprocessor.h:89`).
Declaring only `"Spatial"` emits the same string, because JUCE inserts `Fx` at 0 when
neither `Fx` nor `Instrument` is present (`:1514-1523`).

*Action:* write `VST3_CATEGORIES "Fx" "Spatial"` so the source matches what ships.
`Spatial` is the correct Steinberg tag for a surround panner (`:88`), so the
architecture's intent is right; only the achievable ordering differs.

**F7 — `AU_MAIN_TYPE kAudioUnitType_Effect` is the default** for a non-synth,
non-MIDI-effect plugin (`JUCEUtils.cmake:1862`). Keep it; it is self-documenting and
costs nothing.

**Both keywords are new to this repo.** No existing `plugins/*/CMakeLists.txt` sets
`VST3_CATEGORIES` or `AU_MAIN_TYPE`. If the plan copies a sibling CMakeLists as a
starting point it will silently drop both.

**Confirmed — no root edit needed.** `CMakeLists.txt:48-58` globs `plugins/*` and
adds any directory containing a `CMakeLists.txt`, honouring a `SKIP_PLUGINS` list.
Dropping `plugins/O-Octagon/CMakeLists.txt` in is sufficient.

**Confirmed — tooling resolves the target, not the folder.** `scripts/resolve-target.sh`
parses the first argument of `juce_add_plugin()` and is sourced by both
`scripts/build-and-install.sh` and the CI workflow, so `OuariconOctagon` ≠ `O-Octagon`
is handled. Invocation is `./scripts/build-and-install.sh O-Octagon` (folder name).

**Web browser flags at Stage 1.** ARCHITECTURE §12 sets `NEEDS_WEB_BROWSER TRUE`,
`JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
Set them now even though Stage 1 has no WebView: the flags are inert with a generic
editor, and having `JUCE_WEB_BROWSER` defined from the start is what lets the Phase
2.2 render harness guard `createEditor` with `#if JUCE_WEB_BROWSER` before the Stage 3
swap rather than after it (`pattern_render_harness_breaks_on_webview_editor`).

**No `juce_add_binary_data` target at Stage 1** — therefore none of
`critical_binary_data_strips_hyphens` or `critical_dual_binary_data_namespace_collision`
applies yet. Both bite at Phase 3.1.

**AGPL headers.** Every Ouaricon-authored source file carries the SPDX/AGPL notice
(see any `plugins/*/Source/PluginProcessor.h`). New files must too;
`python3 scripts/add-agpl-headers.py --apply` is idempotent and can be run after the
files exist.

---

## 6. Module reuse — none at Stage 1

`modules/registry.yaml` holds six modules. Assessed against Stage 1:

| Module | Version | Verdict |
|--------|---------|---------|
| `preset-manager` | 1.0.5 | **Stage 3.2.** Musical presets only. Note `pattern_preset_apply_needs_reset_to_defaults` — the reason the venue is a separate tree (§4.1). Must never reach the `VENUE` node |
| `vu-meter` | 1.0.0 | **Stage 3.1/3.3.** Candidate for the 8 UI-03 meters; ballistics differ from the module default (ARCHITECTURE §4.3 wants attack 0.5 / decay 0.12) so check configurability then |
| `webview-relay-manager` | 1.0.0 | **Stage 3.1.** 17 relays make destruction order a live concern |
| `resource-provider` | 1.0.0 | **Stage 3.1** |
| `webview-drop-streaming` | 1.0.0 | Not applicable — no file drop in this plugin |
| `instrument-footer-panel`, `playable-keyboard` | — | Not applicable — effect, not instrument |

**Stage 1 adds no module dependency.** Recording the Stage 3 candidates here so the
plan does not re-derive them.

---

## 7. The zero-warning criterion, concretely

`juce_recommended_warning_flags` on AppleClang
(`extras/Build/CMake/JUCEHelperTargets.cmake:52-77`) includes `-Wall -Wshadow-all
-Wshorten-64-to-32 -Wunused-parameter -Wconversion -Wsign-conversion -Wfloat-equal
-Wswitch-enum -Wmissing-prototypes -Wcast-align -Wextra-semi -Wpedantic`.
Four of these pre-ban specific code shapes:

1. **`-Wunused-parameter`** — `prepareToPlay(double, int)` and
   `isBusesLayoutSupported` will warn on any argument the shell does not use. Omit the
   name or `juce::ignoreUnused(...)`. Do not `(void)` cast.
2. **`-Wshadow-all`** — a constructor or setter parameter named `width`, `blur` or
   `sampleRate` shadows the member of the same name. Given §3.5, `width` is the live
   one.
3. **`-Wconversion` / `-Wsign-conversion`** — `double sampleRate` into a float
   context, and any `size_t`↔`int` mixing around channel indices, both warn. Channel
   loop counters are `int`, matching `buffer.getNumChannels()`.
4. **`-Wswitch-enum`** — warns on an unhandled enumerator *even with a `default:`*.
   `AudioChannelSet::ChannelType` has ~60 enumerators. **Never `switch` on it** — the
   Phase 2.1 label map must be a table or `if`-chain. **[carry to 2.1]**

`-Wfloat-equal` bans `==` / `!=` on floats: it does not bite the shell, but it bites
the Phase 2.2 control-grid dirty check the moment it is written. **[carry to 2.2]**

---

## 8. Verification plan for the six exit criteria

| Criterion | How it is actually checked |
|---|---|
| Builds clean, VST3 + AU + Standalone, zero warnings | `./scripts/build-and-install.sh O-Octagon` — resolves `OuariconOctagon`, and its Phase 4 sweeps both `-dev` and unsuffixed bundles (`critical_dev_release_variant_shadowing`) |
| `auval -a \| grep -i octagon` | AU type/subtype/manufacturer = `aufx` / `OuOc` / `OuDv` (dev branding, `CMakeLists.txt:30`). Clear `~/Library/Caches/AudioUnitCache/` and `killall -9 AudioComponentRegistrar` first |
| pluginval strictness 10, VST3 **and** AU, 2–3 runs | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --timeout-ms 60000 --validate <path>`. `scripts/verify-suite-battery.sh` is the in-repo precedent but defaults to strictness 8 and **VST3 only** — the AU run is a separate invocation against the installed `.component`. Repeat runs are not optional (`pattern_ci_pluginval10_catches_latent_nan`) |
| Standalone on a 2-channel interface (COMPAT-04) | Mechanism traced in §2.3. Passes because `canonicalChannelSet(2) == stereo()` is in the SAFE clause. On a 3–7 channel device expect Debug asserts from JUCE — that is F3, not a plugin defect, and the `buffer.getNumChannels()` bound is what keeps Release safe |
| 17 parameters, correct names/ranges/defaults/units | `GenericAudioProcessorEditor` renders all 17 with labels — this is why D2 chose it. Cross-check against §3.2 |
| Parameter change round-trips through save/reload | pluginval's state-restoration test covers this at any strictness. Confirm once by hand in Logic: move `outputGain`, save, reopen |

`auval` will additionally exercise all six AU channel configs from F2, including
1-in/1-out. The D1 placeholder must not assume 8 output channels — see F3.

---

## 9. Open items handed to the plan phase

1. **`parameter-spec.md` promotion (D4).** Write
   `plugins/O-Octagon/.planning/parameter-spec.md` from ARCHITECTURE §6.1 plus the
   label/display column in §3.2 above. The draft keeps OQ3/4/5 and the 17-vs-18 count
   marked *open* although all four were resolved at Stage 0; the foundation agent must
   read the new file, not the draft.
2. **Parameter grouping** — §3.4 recommends five groups. Confirm or take a flat list.
3. **Member declaration order** in `PluginProcessor.h` — venue store before `apvts`
   (§3.3). Costs nothing now.
4. **Placeholder marker.** D1's dry-to-all-outputs loop needs a greppable token —
   propose `// PHASE-2.2-REPLACE:` — so Phase 2.1's *"zero hardcoded output channel
   indices outside `ChannelMap`"* gate finds it and retires it rather than
   grandfathering it.

## 10. Nothing found that contradicts the locked architecture

Bus declaration, the `isBusesLayoutSupported()` predicate, the two-store state design,
the 17 parameters and the `VERSION` keyword all survived verification unchanged. The
research produced one amplification (F1), two mechanism traces that turn compatibility
criteria into predictions (F2, F3/F4), and three build-level corrections (F5, F6, F7).
None of them reopens a settled decision.

---

## References

**JUCE 8.0.14** (`/Users/taylorbrook/JUCE`)

| Fact | File:line |
|---|---|
| Buffer order is enum-bit order | `juce_audio_basics/buffers/juce_AudioChannelSet.cpp:514-527` |
| `create7point1()` / SDDS / `5point1point2()` membership | `juce_AudioChannelSet.cpp:567, 568, 574` |
| `octagonal()` membership | `juce_AudioChannelSet.cpp:572` |
| `canonicalChannelSet()` | `juce_AudioChannelSet.cpp:636-648` |
| 8-channel candidate sets | `juce_AudioChannelSet.cpp:700-703` |
| `create7point1()` / SDDS / `5point1point2()` decls | `juce_AudioChannelSet.h:199, 209, 221` |
| `getTypeOfChannel()` / `getChannelIndexForType()` | `juce_AudioChannelSet.h:577, 581` |
| AU channel-config derivation | `juce_audio_processors_headless/format_types/juce_AU_Shared.h:395-527` |
| `maxNumChanToCheckFor = 16` | `juce_AU_Shared.h:374` |
| AU wrapper consumes it | `juce_audio_plugin_client/juce_audio_plugin_client_AU_1.mm:175, 398` |
| VST3 layout table (`k71Music`, `k71Cine`) | `juce_audio_processors_headless/format_types/juce_VST3Common.h:307, 308` |
| VST3 generic order fallback | `juce_VST3Common.h:355-383` |
| VST3 `ChannelMapping` | `juce_VST3Common.h:459` |
| `topSideLeft ↔ kSpeakerTsl` | `juce_VST3Common.h:144, 254` |
| `kSpatial` / `kSpatialFx` | `VST3_SDK/pluginterfaces/vst/ivstaudioprocessor.h:88, 89` |
| `setPlayConfigDetails()` | `juce_audio_processors_headless/processors/juce_AudioProcessor.cpp:350-374` |
| Standalone device init | `juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h:165, 357` |
| `AudioParameterFloatAttributes` | `juce_audio_processors_headless/utilities/juce_AudioParameterFloat.h:44, 76, 98` |
| `withLabel` | `juce_RangedAudioParameter.h:69`, `juce_AudioProcessorParameterWithID.h:87` |
| `APVTS::copyState/replaceState` | `juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.h:391, 404` |
| `juce_audio_processors` → headless dependency | `juce_audio_processors/juce_audio_processors.h:54, 64` |
| `VST3_CATEGORIES` reordering | `extras/Build/CMake/JUCEUtils.cmake:1511-1538` |
| `AU_MAIN_TYPE` / `VST3_CATEGORIES` defaults | `JUCEUtils.cmake:1841, 1862` |
| Warning flags | `extras/Build/CMake/JUCEHelperTargets.cmake:52-77` |

**Repo**

- `CMakeLists.txt:24-31` (branding vars), `:48-58` (plugin glob)
- `scripts/resolve-target.sh`, `scripts/build-and-install.sh`, `scripts/verify-suite-battery.sh:170-174`
- `scripts/add-agpl-headers.py`
- `modules/registry.yaml`
- `plugins/O-Orbit/CMakeLists.txt`, `plugins/O-Orbit/Source/PluginProcessor.cpp:28-120, 290-306`
- `troubleshooting/patterns/juce8-critical-patterns.md` §1, §4

**Planning**

- `.planning/research/ARCHITECTURE.md` §4.1, §4.2, §6.1, §6.2, §12, §13, §14
- `.planning/ROADMAP.md` — Stage 1
- `.planning/stages/1-foundation/CONTEXT.md` — D1–D4
- `.planning/REQUIREMENTS.md` — FUNC-01, COMPAT-01, COMPAT-04
