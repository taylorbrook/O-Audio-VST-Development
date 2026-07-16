# Stage 4: Polish — Research

**Date:** 2026-06-27
**Plugin:** O-simplePhysicalModelSynth
**Stage:** 4 of 4 (Polish) — research phase
**Scope (from CONTEXT.md):** seed 6 factory presets (FUNC-07), re-run the full gate,
CHANGELOG, install, cross-platform publish. No new DSP.

This research resolves the three open questions from discuss (preset value format,
Material load-ordering, Windows/MSVC pre-flight) and finds **one critical build seam**
(render-harness link) that must be handled in the same change.

---

## 1. Preset value format — RESOLVED: author RAW, store NORMALIZED via `convertTo0to1`

**Open Question #1 answered definitively.** The on-disk JSON stores **normalized [0,1]**
values, because the load path is:

```
loadPreset → applyPresetJson → param->setValueNotifyingHost(value)   // expects [0,1]
```
(`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:278`). And
`initializeFactoryPresets` writes `FactoryPresetDef::parameters` **verbatim** into the JSON
(`:529-532`) — no conversion. So whatever you put in the map is fed straight to
`setValueNotifyingHost`, which interprets it as normalized and **clamps anything outside
[0,1]**.

### The template to copy: O-simpleFM (NOT O-Bells)
O-simpleFM — the exact template this plugin was forked from — solves this correctly. It
authors presets in **raw/real units** and converts to normalized at build time:

- `plugins/O-simpleFM/Source/FactoryPresets.{h,cpp}` — a `build(apvts)` function.
- Helper: `normalize(apvts, raw)` → `p->convertTo0to1(value)` per ID, **skipping unknown IDs**
  (`FactoryPresets.cpp:28-36`).
- Choice params are authored as the **raw choice index** (`0.0f`/`1.0f`); `convertTo0to1`
  maps index → normalized correctly (e.g. `ratioSnap`, `modFixedMode`).
- Call site: constructor — `presetManager.initializeFactoryPresets(FactoryPresets::build(parameters));`
  (`O-simpleFM/PluginProcessor.cpp:122`). Proven to pass auval/pluginval with file I/O in
  the constructor.

### ⚠ Do NOT copy O-Bells
O-Bells (`PluginProcessor.cpp:1051+`) seeds a **mix** of normalized and raw values into the
same `setValueNotifyingHost` path — e.g. `{"damping", 0.95f}` (normalized) alongside
`{"airAbsorptionTime", 4.0f}`, `{"lpFilterCutoff", 6500.0f}`, `{"bodyTime", 3200.0f}` (raw).
The raw values **>1 silently clamp to max** on load. This is a **latent bug in O-Bells**
(those params pin to their ceiling) and is the source of the "O-Bells seeds raw" note in
CONTEXT. **The convention here is O-simpleFM's: raw author + `convertTo0to1`.**

### `convertTo0to1` covers every param type in this plugin
- `AudioParameterFloat` (the 11 `percentRange()`/dB/seconds sliders): raw→[0,1] via the range.
- `AudioParameterChoice` (`excitationType`, `resonatorType`, `stringModel`): raw index→`index/(N-1)`.
  `excitationType` (3): Pluck 0→0.0, Strike 1→0.5, Bow 2→1.0. `resonatorType` (2): String 0→0.0,
  Modal 1→1.0.
- `AudioParameterInt` (`coarseTune` −24..24): raw 0 → `convertTo0to1(0)=0.5`; round-trips to 0 st.

---

## 2. Material macro load-ordering — RESOLVED: never co-author `material` + `damping`/`decay`

**Open Question #2 answered.** `material` is a **meta** parameter with a message-thread APVTS
listener (`PluginProcessor.cpp:216-234`). When `material` changes it **overwrites** both
`damping` and `decay`:

```cpp
parameterChanged("material") → setValueNotifyingHost on damping AND decay
```

Two facts make `material` **always win** if all three are in one preset:
1. `FactoryPresetDef::parameters` is a `std::map<String,float>` → JSON keys are **alphabetical**
   (`damping` < `decay` < `material`), so material is the *last* of the three written/applied.
2. APVTS parameter listeners are **coalesced + dispatched async** on the message thread, so
   `parameterChanged` fires **after all 17 `setValueNotifyingHost` calls in `applyPresetJson`
   complete** — recomputing damping/decay from the final `material` value regardless of order.

### Authoring convention (the safe rule)
**A preset map must never contain `material` AND (`damping` or `decay`) together.**

| Preset family | Author | Omit | Rationale |
|---------------|--------|------|-----------|
| **String** (Bright Steel, Muted Nylon, Koto/Harp, Bowed String) | `material` (raw %) | `damping`, `decay` | The macro derives both; sliders visibly track; steel↔nylon **is** the material axis (pedagogically correct). |
| **Modal** (Struck Bar, Bell) | `damping`, `decay` (raw %) | `material` | Modal ring length needs explicit damping/decay; the steel↔nylon framing is string-specific. `material` stays at default 30 (cosmetic only — no listener fires since it's unchanged → damping/decay survive). |

### Material → damping/decay forward map (so authors know what a `material` value yields)
From the macro formula (`m = material/100`):

| `material` | fc (Damping tone) | g (Decay/ring) | Character |
|-----------:|-------------------|----------------|-----------|
| 8  | ~9.4 kHz (bright) | ~0.990 (long)  | Bright Steel |
| 15 | ~8.6 kHz          | ~0.985 (long)  | Bowed (sustained, bright) |
| 35 | ~5.6 kHz (mid)    | ~0.972 (medium)| Koto/Harp |
| 85 | ~2.3 kHz (dark)   | ~0.935 (short) | Muted Nylon |

The single material axis cleanly covers all four String presets.

### No "Default" factory preset needed
FUNC-07 wants exactly the **6 named** concept presets — no "Default" among them
(`CONTEXT.md` table). `getPresetList()` (`OuariconPresetManager.h:401-415`) populates the bar
purely from the factory/user dirs, and `currentPresetName` initializes to `"Default"` as a
*label* (not a file). The plugin's power-on sound is already the `createParameterLayout`
defaults. **Ship the 6 named presets only.** This also sidesteps the meta-param wart in
O-simpleFM's `makeDefaultPreset` (which would set `material` and stomp the damping/decay
defaults). *If a Default preset is ever added, exclude `material` from its id list.*

---

## 3. ⚠ CRITICAL build seam — render-harness must also link `FactoryPresets.cpp`

The render-harness compiles **`PluginProcessor.cpp`** (`tests/render-harness/CMakeLists.txt:25`)
but lists no other plugin `.cpp`. The moment the constructor calls `FactoryPresets::build()`,
the harness gets an **undefined-symbol link error** (`FactoryPresets::build`). This is the #1
silent footgun for this stage (same class as the WebView/editor seam from Stage 3).

**Mandatory companion edit:** add
`${CMAKE_CURRENT_SOURCE_DIR}/../../Source/FactoryPresets.cpp` to the harness
`target_sources` block, alongside the existing `PluginProcessor.cpp`. Re-run the harness
at Stage-4 START (D5) — that build will catch this immediately if missed.

*(Side effect, harmless: the harness constructor will now seed 6 JSON files under
`~/Library/O-simplePhysicalModelSynth/Presets/Factory/` when run locally. CI does not run the
harness for release builds.)*

---

## 4. Windows / MSVC pre-flight — ALL CLEAR (no fixes required)

**Open Question #3 answered.** Scanned the three known suite gotchas:

| Risk | Result | Evidence |
|------|--------|----------|
| **MSVC C3493** (non-static `constexpr` local captured in a lambda) | **CLEAN** | Every `constexpr` in `Source/` is **namespace/class scope** (`static constexpr` members, `inline constexpr` ParamIDs). Zero local `constexpr` inside lambdas. No `static` qualifier needed. |
| **Dual `juce_add_binary_data` namespace collision** | **N/A** | Single binary-data target `O-simplePhysicalModelSynth_UIResources` (`CMakeLists.txt:76`); no embedded-samples target → default `BinaryData` namespace is safe. |
| **WebView2 silent-blank on Windows** (COMPAT-02) | **SATISFIED** | `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` both present (`CMakeLists.txt:27,123`). |

### Release branding / publish path (also confirmed)
- CI is **tag-driven**: tag `O-simplePhysicalModelSynth-v1.0.0` → `build-and-release.yml`
  parses plugin+version (`:37-43`), resolves the CMake target by grepping `juce_add_plugin(`
  (`:87`), builds mac **VST3+AU** and Windows **VST3**. **No registration/matrix edit needed.**
- CI builds with `OUARICON_RELEASE=ON` → **unsuffixed** bundles + company "Ouaricon Audio" +
  manufacturer code **`OuAu`** (root `CMakeLists.txt:23-27`). Local dev builds are `-dev` / `OuDv`.
- **Note for the gate:** the local auval check is `auval -v aumu OsPM OuDv` (dev code). The
  **shipped** AU is `aumu OsPM OuAu`. Same subtype `OsPM`; only the manufacturer differs by
  build flavor — expected, not a defect.

---

## 5. Implementation surface (port-ready) — what Stage 4 actually touches

Seeding is the **only** code change. All preset infra (10 native fns, state-I/O swap,
`getPresetList`, bar UI) was wired in Stage 3 — **no JS / native-fn / param changes**.

| Action | File | Note |
|--------|------|------|
| **NEW** | `Source/FactoryPresets.h` | Adapt O-simpleFM: `namespace FactoryPresets { build(apvts) }`. |
| **NEW** | `Source/FactoryPresets.cpp` | `normalize()` helper + 6 `makePreset` calls (raw units). `#include "PluginProcessor.h"` for `ParamIDs`. |
| **EDIT** | `Source/PluginProcessor.cpp` ctor (~line 151) | Replace the empty-stub comment with `presetManager.initializeFactoryPresets(FactoryPresets::build(parameters));`. |
| **EDIT** | `CMakeLists.txt` `target_sources` | Add `Source/FactoryPresets.cpp`. |
| **EDIT** | `tests/render-harness/CMakeLists.txt` | Add `../../Source/FactoryPresets.cpp` (see §3 — link gate). |

---

## 6. Draft preset values (raw units) — starting points for plan; user auditions (D3)

Authored per §1 (raw) and §2 (String→material / Modal→damping+decay; never both). These are
**ear-tune starting points** — D3 has the user audition after install and the plan/execute may
refine. Omitted IDs fall back to `createParameterLayout` defaults. Choice values are raw indices.

| Param (raw) | Bright Steel | Muted Nylon | Koto/Harp | Struck Bar | Bell | Bowed String |
|---|---|---|---|---|---|---|
| `excitationType` | 0 Pluck | 0 Pluck | 0 Pluck | 1 Strike | 1 Strike | 2 Bow |
| `resonatorType` | 0 String | 0 String | 0 String | 1 Modal | 1 Modal | 0 String |
| `excitationPosition` | 18 | 30 | 22 | 30 | 25 | 30 |
| `excitationColor` | 75 | 35 | 60 | 55 | 65 | 50 |
| `bowForce` | — | — | — | — | — | 55 |
| `inharmonicity` | — | — | — | 10 | 80 | — |
| `modeBrightness` | — | — | — | 45 | 70 | — |
| `material` | 8 | 85 | 35 | *(omit)* | *(omit)* | 15 |
| `damping` | *(macro)* | *(macro)* | *(macro)* | 55 | 70 | *(macro)* |
| `decay` | *(macro)* | *(macro)* | *(macro)* | 60 | 88 | *(macro)* |
| `ampAttack` (s) | 0.001 | 0.001 | 0.001 | 0.001 | 0.001 | 0.04 |
| `ampRelease` (s) | 0.3 | 0.15 | 0.25 | 0.3 | 0.6 | 0.25 |
| `outputLevel` (dB) | −6 | −5 | −6 | −8 | −8 | −6 |

`coarseTune`/`fineTune` omitted everywhere (default 0/0). `stringModel` omitted (KS only,
default 0). `velToBrightness` omitted (default 60) unless ear-tuning wants otherwise.

---

## 7. Gate / verification notes carried into plan

- **Re-run render-harness at Stage-4 START** (D5) — it doubles as the §3 link-seam check.
  Expect ALL PASS (no DSP touched); the new `FactoryPresets.cpp` must link.
- Factory-preset load test: each of the 6 loads without error, sets the expected
  excitation/resonator combo, round-trips state. (`getPresetList` returns 6.)
- pluginval strictness-10 (VST3+AU) + `auval -v aumu OsPM OuDv` SUCCEEDS (dev), native-fn /
  param-ID parity unchanged (no new params/fns), `node --check`.
- `build-and-install.sh` dual-variant sweep, Standalone screenshot.
- **Publish:** tag `O-simplePhysicalModelSynth-v1.0.0` after green verify; CHANGELOG v1.0.0;
  confirm mac VST3/AU + Win VST3 artifacts (first real MSVC exercise — §4 says clean).

---

## Open items for plan

1. Finalize the 6 raw-value maps (§6 is the seed) and confirm Modal `damping`/`decay`
   produce the intended ring by ear after install (D3).
2. Decide whether to also fold the §3 harness edit + §5 ctor edit into a single execute task
   gated by one harness rebuild.
3. CHANGELOG v1.0.0 content + release tag — author at execute/publish.
