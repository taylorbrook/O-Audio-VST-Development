# O-Comp v1.10.0 — External Sidechain

**Status:** PLAN APPROVED, NOT YET IMPLEMENTED
**Created:** 2026-09-14
**Base version:** v1.9.0 (📦 Installed)
**Target version:** v1.10.0 (MINOR)
**Tier:** 2 (root-cause analysis; new feature across DSP + bus layout + UI + i18n)

---

## Goal

Give O-Comp a real external key input, plus the detector-path filtering and
monitoring that make an external key usable: a discrete aux input bus, an
Internal/External source select, a sidechain high-pass and low-pass, and a
sidechain listen toggle.

## Decisions taken (2026-09-14)

| Decision | Choice | Consequence |
|---|---|---|
| Key filtering | **HPF + LPF** | Mirrors O-MultiBandCompressor; its filter code and all three languages' strings are liftable |
| No key routed, External selected | **Auto-fall back to internal** | Compressor always works; the External selection can be a quiet lie |
| UI placement | **Grow the window** to 620×420, new bottom strip | Honest layout; needs a width-pin pass at the widest language |
| Depth | **Plan only, then stop** | This is the suite's first main+aux bus — design gets read before code is written |

---

## Why this is not a copy-paste job

**No plugin in this suite has ever negotiated a main bus alongside an aux bus.**

- `O-Texture/Source/PluginProcessor.cpp:65` declares `.withInput("Sidechain", …, false)`,
  but that is its *only* input bus — index 0, because O-Texture is a generator. Its
  `isBusesLayoutSupported` (`:382`) reads `getChannelSet(true, 0)`. Not this case.
- `O-MultiBandCompressor` says "sidechain" throughout, but it means the **internal
  detector filter path** — an HPF/LPF pair on the signal the compressor already has.
  It declares no aux bus at all.

So the *internal* half of this work is proven in-suite and the *external* half is new
ground. The risk is concentrated in host negotiation (Task 1), not in the DSP.

## What is directly reusable

| From | What | Why it matters |
|---|---|---|
| `O-MultiBandCompressor/Source/DSP/Compressor.h:337` | `updateSidechainFilters()` | RT-safe: `IIR::ArrayCoefficients` returns a stack `std::array`; `Coefficients::makeHighPass` heap-allocates a ref-counted object **on the audio thread** |
| same, `:371` | `applySidechainFilters()` | Per-detector-channel filter application |
| same, `:205` | Listen-mode branch | Each channel outputs its own filtered key |
| `O-MultiBandCompressor/Source/PluginProcessor.cpp:196-210` | SC_HPF / SC_LPF param form | Uses a **skew factor** (`0.3f`), not a lambda range — see the trap in Risks |
| `O-MultiBandCompressor/Source/ui/public/js/i18n.js` | `label.scHpf`, `label.scLpf`, `label.scListen`, `label.detector`, and the two tooltip bodies | Already carries EN + FR + zh-Hans, with the French loan-word decision already argued out ("Sidechain" kept; "Écoute SC") |

---

## Task breakdown

### Task 1 — Aux bus declaration and negotiation ⚠ HIGHEST RISK

**File:** `Source/PluginProcessor.cpp:103-107`, `:425-436`

Add a second, discrete, **disabled-by-default** input bus:

```cpp
OCompAudioProcessor::OCompAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input",     juce::AudioChannelSet::stereo(), true)
                        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                        .withOutput("Output",   juce::AudioChannelSet::stereo(), true))
```

Relax `isBusesLayoutSupported` to accept the aux bus disabled, mono, or stereo, while
keeping the existing main-bus rule intact:

```cpp
bool OCompAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();

    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != mono && mainOut != stereo)
        return false;

    // Unchanged: in-place stereo-linked processing needs input to match output.
    if (layouts.getMainInputChannelSet() != mainOut)
        return false;

    // Key bus: disabled, mono or stereo. Never wider — keyPtrs is a 2-slot array,
    // the same reason the main path caps at 2.
    const auto& key = layouts.getChannelSet(true, 1);
    return key.isDisabled() || key == mono || key == stereo;
}
```

**Verification (this is the task that can fail):**
- `auval -v aufx OuCp Ouar` passes.
- Logic Pro shows a populated **Side Chain** menu in the plugin header.
- A VST3 host (Reaper, Live) shows a second input pin pair.
- **Regression check:** an existing v1.9.0 Logic session still loads without the
  channel strip re-negotiating. Adding a disabled aux bus should be transparent,
  but a stale AU registry entry will lie — sweep both variants per CLAUDE.md before
  testing, and expect a cold `auval` rescan on first run.

### Task 2 — Key source selection with auto-fallback

**File:** `Source/PluginProcessor.cpp` (processBlock, `:226`)

Per block, before the sample loop:

```cpp
const auto* keyBus = getBus(true, 1);
const bool keyAvailable = keyBus != nullptr
                       && keyBus->isEnabled()
                       && getChannelCountOfBus(true, 1) > 0;
const bool useExternal = (scSourceParam->load() > 0.5f) && keyAvailable;
```

Recompute **every block** — a host can enable or disable the bus between blocks.

Build a `const float* keyPtrs[2]` from either the main buffer or
`getBusBuffer(buffer, true, 1)`. Construct the bus-buffer view **once, outside the
sample loop**. If the key is mono and main is stereo, replicate slot 0 into slot 1.

### Task 3 — Detector restructure

**File:** `Source/PluginProcessor.cpp:263-303`

The current loop reads `channelPtrs[ch][sample]` (the main buffer, in place) as the
detector input and then writes gain into the same pointers. Preserve that read-then-write
ordering; introduce a separate `keyPtrs` read so the detector no longer aliases the
signal path.

New per-sample order:

1. Read key sample(s) from `keyPtrs`.
2. Apply SC HPF then SC LPF (detector-only).
3. Rectify, stereo-link (max), convert to dB.
4. Envelope, gain computer — **unchanged**.
5. If `sc_listen`: write the filtered key to the output channels instead of the
   compressed signal. Apply this **after** the detector has read, and keep the
   gain-reduction meter updating so the display still reads true while monitoring.
6. Otherwise apply gain to `channelPtrs` as today.

**Metering:** `inputLevelDB` currently reports the main input peak. With an external
key, decide explicitly whether the input meter shows the main input (recommended — it
is the signal being compressed) or the key. Recommend: leave it on the main input,
and let SC Listen be the way to hear the key.

**Latency:** unchanged. The key filters are detector-only and report nothing. No
`setLatencySamples()` call is added.

### Task 4 — Filter state

**File:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` (prepareToPlay `:204`)

`juce::dsp::IIR::Filter<float> scHPF[2], scLPF[2];` plus cached `currentSCHPFFreq` /
`currentSCLPFFreq` and `scHPFEnabled` / `scLPFEnabled` flags.

Prepare and `reset()` in `prepareToPlay`; reset in `releaseResources`.

**Copy O-MBC's guard structure exactly.** The enabled flag is assigned *outside* the
"frequency changed" branch:

```cpp
if (wantHPF && hpfFreq != currentSCHPFFreq) { …update coefficients…; currentSCHPFFreq = hpfFreq; }
scHPFEnabled = wantHPF;          // ← outside, deliberately
```

Folding that assignment into the `if` reintroduces the exact bug O-MBC documents at
`Compressor.h:330`: set HPF to 100 Hz → Off → 100 Hz and the filter silently stays
off, because `currentSCHPFFreq` still reads 100.

**New for O-Comp:** reset the filter state when the key source flips Internal↔External.
The filters would otherwise carry state from the other source into the first samples of
the new one. Use an atomic flag set on source change and checked at block start.

### Task 5 — Four new parameters

**File:** `Source/PluginProcessor.cpp:37` (`createParameterLayout`)

| ID | Type | Range / choices | Default |
|---|---|---|---|
| `sc_source` | Choice | `{ "Internal", "External" }` | Internal |
| `sc_hpf` | Float | `NormalisableRange(0.0f, 2000.0f, 0.1f, 0.3f)`, 0 = Off | 0.0 |
| `sc_lpf` | Float | `NormalisableRange(0.0f, 20000.0f, 0.1f, 0.3f)`, 0 = Off | 0.0 |
| `sc_listen` | Bool | — | false |

All four are **new IDs**. No existing ID is renamed, no existing range moves, nothing
is removed. Cache the raw pointers in the constructor alongside the existing seven.

### Task 6 — UI

**Files:** `Source/PluginEditor.cpp:227` (setSize), `Source/PluginEditor.h:65-83`,
`Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`

- `setSize(620, 360)` → `setSize(620, 420)`.
- New "Detector / Sidechain" strip along the bottom (y ≈ 355): Int/Ext source control,
  SC HPF knob, SC LPF knob, SC Listen toggle. The existing frame is absolutely
  positioned and full — meters already run to y=345 — so the height has to grow.
- **No new art needed.** `paper-bg.jpg` is `background-size: cover` and `shell.png` is
  `contain` (`index.html:47`, `:403`), so both rescale to the taller frame.
- Relays and attachments: two `WebSliderRelay` (`sc_hpf`, `sc_lpf`), one
  `WebToggleButtonRelay` (`sc_listen`), one `WebComboBoxRelay` (`sc_source`), each
  registered with `.withOptionsFrom(...)` and given a matching attachment. Keep the
  documented Relays → WebView → Attachments construction order (`PluginEditor.h:58`).
- i18n: port O-MBC's `label.scHpf` / `label.scLpf` / `label.scListen` / `label.detector`
  and the two tooltip bodies, in all three languages. O-Comp ships EN + FR + zh-Hans,
  so a missing arm fails the gate.
- Frequency readouts show `Off` at 0 Hz and are editable; port O-MBC's `parseFreq()`
  behaviour (typed "off" → 0) **and** its i18n lint exemption for the literal `Off`
  scoped to `.knob-value`, or the label gate will flag the readouts.

### Task 7 — Presets

Existing user and factory presets carry none of the four new IDs, so they load with
Internal / Off / Off / listen-off — **bit-identical to v1.9.0 behaviour.** That is the
backward-compatibility argument for MINOR and should be stated in the CHANGELOG.

If any factory preset is updated to set `sc_hpf` / `sc_lpf`, the value **must** be
written through the owning parameter's own `NormalisableRange`. Hand-writing the
normalised 0–1 fraction ignores the 0.3 skew and recalls 10–30× wrong — O-MBC documents
exactly this at `PluginProcessor.cpp:229`.

### Task 8 — Verification

- `cmake -DOUARICON_BUILD_TESTS=ON` → `param-dump` target confirms 11 parameters with
  the expected IDs, ranges and defaults (a regex over `createParameterLayout()` is not
  authoritative; only a walk of a constructed processor is).
- `tests/ui_tip_render_check.js` extended to cover the four new tooltips.
- `pluginval --strictness-level 10` on VST3 and AU.
- Manual: Logic side-chain routing, key HPF/LPF audibly moving the trigger point,
  SC Listen monitoring the filtered key, and External-with-nothing-routed falling back
  silently to internal.

---

## Version: MINOR → v1.10.0

Not breaking. Four added parameter IDs; no rename, no range change, no removal, no
state-format change. The aux bus ships disabled by default, so an existing session's
negotiated layout is unchanged.

The one thing to watch is not a code-compatibility issue but a **registry** one: adding
a bus changes the AU's published configuration, so the first load after install may
force a rescan. Sweep both `-dev` and unsuffixed bundles per CLAUDE.md before testing.

---

## Risks

| # | Risk | Mitigation |
|---|---|---|
| 1 | **Logic does not offer the Side Chain menu.** First main+aux bus in the suite; no in-house precedent to copy. | Task 1 is verified standalone, before any DSP work. If AU negotiation fails, that is the moment to stop and research — not after the UI is rebuilt. |
| 2 | **Lambda `NormalisableRange` is invisible to the WebView slider frontend.** A log-ish frequency knob is the exact shape that invites one. | Use the skew-factor form (`0.3f`) as O-MBC does. Never the lambda ctor. |
| 3 | **Cached-coefficient guard leaks the enabled flag** (100 Hz → Off → 100 Hz stays off). | Assign `scHPFEnabled` / `scLPFEnabled` outside the frequency-changed branch, per O-MBC's documented fix. |
| 4 | **zh-Hans width pinning.** The new strip adds content-sized boxes; each must be pinned at its widest language or the layout moves when the language changes. | Pin every new box at the widest language. Note `check-ui-labels` is blind to width-pinned `data-i18n`, so this needs an eye, not just the gate. |
| 5 | **Filter state carries across a source switch**, producing a transient. | Reset filter state on Internal↔External change (Task 4). |
| 6 | **Auto-fallback hides a routing mistake.** Chosen deliberately, but it means External can silently mean internal. | Accepted. Revisit as "fall back + flag it in the UI" if it proves confusing in use; that needs a native→JS state push. |

---

## Gate before implementation

`backups/O-Comp/` currently holds **v1.2.0 and v1.4.3 only — there is no v1.9.0
backup.** Phase 0.9 requires one before any source file is touched:

```bash
./scripts/verify-backup.sh O-Comp 1.9.0
```

Create it first if absent. This is a hard gate, not a formality — it is the rollback
path for an eight-task change to a shipped, installed plugin.

---

## Sequencing

1. Backup v1.9.0 and verify (**gate**)
2. Task 1 — bus declaration → **verify in Logic and a VST3 host before continuing**
3. Tasks 5, 4, 2, 3 — parameters, filter state, source select, detector restructure
4. Task 6 — UI, relays, i18n
5. Task 7 — preset note
6. Task 8 — verification, CHANGELOG, build, install
