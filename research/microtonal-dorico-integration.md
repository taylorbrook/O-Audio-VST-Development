---
title: "Microtonal Dorico Integration: Module Architecture, Setup, Quirks, Troubleshooting"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
dorico_version: "6.x"
summary: "Internal developer reference for the v1.5 microtonal cohort's Dorico integration via VST3 Note Expression. Covers module architecture, canonical Path B setup procedure (Library Manager Import), host-side behavior quirks, and troubleshooting signatures. Source-of-truth notes; not user-facing manual copy."
domain: dsp
type: reference
keywords:
  - dorico
  - vst3-note-expression
  - microtuning
  - doricolib
  - library-manager
  - integration-notes
stages: [3, 4]
audience: internal-dev-only
agents: [dsp]
related:
  - .planning/phases/23-extract/
  - .planning/phases/24-propagate/
  - .planning/phases/25-package-docs/
  - .claude/skills/spike-findings-VST-development/
---

# Microtonal Dorico Integration: Module Architecture, Setup, Quirks, Troubleshooting

**Audience:** internal Ouaricon developers (us, v1.6 and beyond). Not user-facing manual copy.
**Tone:** technical reference. Code-grounded; terse; no quickstart prose.
**Source-of-truth posture:** raw material for later end-user manual authoring on the sales
website (FUT-06). The canonical Path B story (Library Manager Import → per-channel assignment)
plus the v2 schema-defect lesson are documented here once, where future implementers can find
them.

## Table of Contents

1. [Module Architecture](#module-architecture)            — DOCS-01
2. [Canonical Dorico Setup Procedure](#canonical-dorico-setup-procedure)  — DOCS-02 (Path B)
3. [Host-Side Behavior Quirks](#host-side-behavior-quirks)  — DOCS-03
4. [Troubleshooting Signatures](#troubleshooting-signatures)  — DOCS-04

---

## Module Architecture

The `note-expression` module (v1.1.0; `modules/tuning/note-expression/`) provides VST3 Note
Expression (`kTuningTypeID`) plumbing for any JUCE-based plugin that needs Dorico microtonal
playback. Public API lives under the `Ouaricon::NoteExpression` nested namespace.

### Public surface

Header: [`modules/tuning/note-expression/cpp/NoteExpression.h`](../modules/tuning/note-expression/cpp/NoteExpression.h).
The header is **Steinberg-free** — non-VST3 builds (AU / Standalone / VST2 / AAX / LV2 / Unity)
include it without pulling any VST3-SDK symbol into their link line.

Three consumer-facing types:

- `Ouaricon::NoteExpression::PendingTuningTable` — a 128-slot
  `std::array<std::atomic<double>, 128>`. Stores semitone deltas indexed by MIDI pitch.
- `Ouaricon::NoteExpression::VST3Extensions` — subclass of `juce::VST3ClientExtensions`. Owns
  the `PendingTuningTable`, drains the patched JUCE wrapper's raw-event queue, advertises the
  Note Expression Controller (NEC) to hosts.
- `Ouaricon::NoteExpression::applyPendingTuning(table, midiNote, currentFrequency)` — header-only
  voice helper. Multiplicatively composes the pending NE delta with any base frequency.

Consumer wiring is three lines: `ouaricon_add_module(<Plugin> note-expression)` in the
plugin's `CMakeLists.txt`, a `VST3Extensions` member on the processor, and an
`applyPendingTuning` call in each voice's `startNote` after base-frequency computation.

### NEC advertisement flow

`Controller` (full body in `NoteExpression_VST3.cpp`; forward-declared in the header) implements
`Steinberg::Vst::INoteExpressionController` and advertises exactly one NE type:
`kTuningTypeID`, bipolar, absolute, normalized `[0,1]` mapping to plain semitones `[-120, +120]`.

`VST3Extensions::queryIEditController` is the dispatch point — when a host queries
`INoteExpressionController::iid` via `VST3ClientExtensions::queryIEditController`, the
extensions object lazy-creates the `Controller` (swap-deleter pimpl idiom — see TU split below)
and returns the interface pointer.

```cpp
// modules/tuning/note-expression/cpp/NoteExpression.h (excerpt)
class VST3Extensions : public juce::VST3ClientExtensions
{
public:
    int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override;
    void onVst3RawEvent (const Vst3RawEvent& e) override;
    void drainBlockEvents (std::vector<Vst3RawEvent>& out);
    void drainAndUpdate();
    PendingTuningTable& getPendingTable() noexcept;
    // ...
};
```

Dorico does **not** consume this advertisement (see DOCS-03 — quirk #2). The NEC is kept
because Cubase / Nuendo and other VST3 hosts use it as a capability signal.

### Raw-event queue semantics

JUCE 8.0.4 silently drops `kNoteExpressionValueEvent` and `noteId`-tagged NoteOn/NoteOff
events inside `MidiEventList::toMidiBuffer` because they cannot round-trip through
`juce::MidiMessage`. The Ouaricon JUCE patch
(`scripts/juce-patches/note-expression-juce-8.0.4.patch`; idempotent applier at
`scripts/apply-juce-patches.sh`) adds `VST3ClientExtensions::onVst3RawEvent` so the plugin
sees raw events **before** the lossy conversion. The patch carries a verbatim
`JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)` marker that both the apply-script
idempotency check and the configure-time CMake gate rely on.

`VST3Extensions::onVst3RawEvent` (called on the audio thread immediately before
`processBlock`) pushes events onto an internal `blockEvents` `std::vector<Vst3RawEvent>`
reserved to 64 slots (T-23-02). The processor calls `drainAndUpdate()` at the top of
`processBlock` before `renderNextBlock` — this swaps the buffer out to a scratch vector and
runs the dispatch slot's correlation function (see two-TU split, below).

### Voice-routing logic

`applyPendingTuning` is header-only and Steinberg-free:

```cpp
// modules/tuning/note-expression/cpp/NoteExpression.h
inline double applyPendingTuning (PendingTuningTable& table,
                                  int                 midiNoteNumber,
                                  double              currentFrequency)
{
    if (midiNoteNumber < 0 || midiNoteNumber >= 128)
        return currentFrequency;

    const double semis = table[(size_t) midiNoteNumber]
                             .exchange (0.0, std::memory_order_acq_rel);
    if (semis == 0.0)
        return currentFrequency;

    return currentFrequency * std::pow (2.0, semis / 12.0);
}
```

Two load-bearing details:

1. **`exchange(0.0)`** (not `load + store`) ensures a retriggered note at the same pitch in a
   later block does not inherit a stale offset.
2. **Composition order is load-bearing.** `applyPendingTuning` must run AFTER the voice's
   base-frequency computation (so tuning stacks multiplicatively over any existing
   `TuningEngine` lookup, MIDI-standard, or humanize value) and BEFORE the DSP `trigger(...)`
   call (so the first output sample sizes to the tuned frequency — no attack zipper). See
   landmine 4 in
   [`.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md`](../.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md).

The composition pattern in O-Lyrica's `HarpSynthVoice::startNote` is the canonical exemplar:

```cpp
// plugins/O-Lyrica/Source/HarpSynthVoice.cpp (composition pattern)
double currentFrequency = computeBaseFrequencyViaTuningEngine (midiNoteNumber);
if (pendingTuningSource != nullptr)
    currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                           *pendingTuningSource, midiNoteNumber, currentFrequency);
stringModel.trigger (currentFrequency, velocity);
```

### TuningEngine composition

The module composes **multiplicatively** with each plugin's existing `TuningEngine` (or any
other base-frequency source). NE deltas stack on top of alternate tunings without bypassing
tuning math: `final = base * pow(2, semis/12)`. Plugins that use the optional
`scala-tuning-engine` module gain microtonal NE on top of their selected non-12-TET tuning
without code changes — the helper sees the `TuningEngine`-resolved frequency as `currentFrequency`
and applies the Dorico delta on top.

O-Lyrica is the Phase 23 canary; the seven Phase 24 propagation consumers (O-Bells, O-Wind,
O-Reed, O-Bowed, O-Formant, O-IntonationPad, O-Prism) follow the same composition shape with
five propagation-pattern variants catalogued in the Phase 24 final-sweep summary
(classic-Synthesiser-multi-osc, classic-Synthesiser-physical-period,
classic-Synthesiser-multi-sub-voice, MPE-helper-based, MPE-per-call-site).

### Two-TU split (Phase 23 D-23-04-A architectural fix)

The module is split across two translation units to keep VST3-SDK symbols out of non-VST3
link lines:

| TU | Path | Contents | Linked into |
|----|------|----------|-------------|
| SharedCode-bound | `cpp/NoteExpression.cpp` | `VST3Extensions` ctor/dtor, `drainAndUpdate` body, `registerNEUpdate`, `registerNEQuery`, `noopControllerDelete` | every format's link line |
| VST3-only       | `cpp/vst3/NoteExpression_VST3.cpp` | `Controller` body, `realControllerDelete`, `updatePendingFromEvents`, `vst3QueryIEditController`, static-init `DispatchRegistrar` | only `${TARGET}_VST3` |

Two function-pointer dispatch slots bridge the layers without leaking Steinberg symbols
through the SharedCode TU's vtable references:

```cpp
// modules/tuning/note-expression/cpp/NoteExpression.h (excerpt)
using NEUpdateFn = void (*) (
    const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>&,
    PendingTuningTable&);
void registerNEUpdate (NEUpdateFn fn) noexcept;

using NEQueryFn = int32_t (*) (
    VST3Extensions&,
    const Steinberg::TUID,
    void**);
void registerNEQuery (NEQueryFn fn) noexcept;
```

The SharedCode TU owns two `std::atomic<...>` slots (`g_neUpdate`, `g_neQuery`); the VST3 TU
registers its bodies into the slots via static-init at TU load:

```cpp
// modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (excerpt)
struct DispatchRegistrar
{
    DispatchRegistrar() noexcept
    {
        registerNEUpdate (&updatePendingFromEvents);
        registerNEQuery  (&vst3QueryIEditController);
    }
};
[[maybe_unused]] static const DispatchRegistrar g_dispatchRegistrar;
```

When the VST3 TU is not linked (AU / Standalone / VST2 / etc.), the static-init never runs,
the dispatch slots stay `nullptr`, and SharedCode's `drainAndUpdate` becomes a fast
pass-through. Correct: non-VST3 hosts cannot deliver `kTuningTypeID` events.

The `VST3Extensions::nec` member is a custom-deleter pimpl
(`std::unique_ptr<Controller, void(*)(Controller*)>`). SharedCode's ctor initializes it with
`noopControllerDelete`; the VST3 TU's `vst3QueryIEditController` lazy-creates the `Controller`
and atomically swaps in `realControllerDelete` via move-assignment of a fresh `unique_ptr`.
The custom function-pointer deleter is what allows the SharedCode-bound dtor (`= default`) to
compile without seeing `Controller`'s body — `unique_ptr`'s destructor invokes the stored
function pointer, never `delete Controller*` directly. Plan 23-05 records the full rationale.

### Module ownership of the Dorico expression-map asset (Phase 25 D-04)

Per Plan 25-01 v3, the `note-expression` module owns the canonical Dorico expression-map
library bundle as a module resource:

```
modules/tuning/note-expression/resources/
├── library/
│   └── Ouaricon-VST3-NoteExpression.doricolib    # 6,431 B; Dorico-valid; Path B
└── README-microtonal-suite.txt                   # user-facing fallback README
```

`module.cmake` adds a per-consumer `install(SCRIPT)` rule keyed off
`ouaricon_add_module(<Plugin> note-expression)`. The script runs at install time and copies
both files to the platform-specific Ouaricon shared path (see DOCS-02). Each of the 8 cohort
plugins' PKG (macOS) / EXE (Windows) installers bundles the same canonical bytes via the
shared PKG postinstall block in `.claude/skills/plugin-packaging/references/pkg-creation.md`
and the shared Inno Setup `[Files]` template variables in
`.claude/skills/plugin-packaging/assets/inno-template.iss`.

---

## Canonical Dorico Setup Procedure

Path B distribution model: ship one `.doricolib` to one platform-specific Ouaricon shared
path. User performs a one-time `Library → Library Manager → Import…` per machine, then assigns
the expression map per project via `Play → Endpoints → Expression Map`. No Playback Template
archive, no auto-discovery scan, no write into Dorico's user-config additions directory, no
write into Dorico's `Expression Maps/User/` directory.

### Per-machine one-time setup

1. Install any Ouaricon plugin (PKG on macOS, EXE on Windows). The installer drops
   `Ouaricon-VST3-NoteExpression.doricolib` (6,431 B) plus `README-microtonal-suite.txt` to
   the canonical Ouaricon shared path:
   - **macOS:** `~/Library/Application Support/Ouaricon/Microtonal Suite/`
   - **Windows:** `%APPDATA%\Ouaricon\Microtonal Suite\`
2. Open Dorico (any project; the import is global, not project-scoped).
3. `Library → Library Manager → Import…`.
4. Navigate to the install path above and select `Ouaricon-VST3-NoteExpression.doricolib`.
5. Click Import. Dorico merges "Ouaricon VST3 Note Expression" into the project library.
   Verify under `Library → Expression Maps`.

The import persists across Dorico restarts and version upgrades. Re-importing after a plugin
reinstall is harmless but unnecessary — the `.doricolib` content is stable across all 8 cohort
plugins (byte-identical bundling per Plan 25-02 D-06).

### Per-project per-channel assignment

1. Load any Ouaricon plugin in the project: `Play → Endpoints → Add Plug-in` (or via existing
   routing).
2. With the plugin's channel selected, locate the `Expression Map` dropdown in the Endpoints
   panel.
3. Pick "Ouaricon VST3 Note Expression" from the dropdown.
4. Microtonal accidentals on that channel now route as VST3 Note Expression to the plugin.

### Verification

Place a note with a quarter-sharp accidental at C4. Press play. Pitch should land at
**C4 + 50¢ ≈ 269 Hz** (between standard C4 = 261.63 Hz and C♯ = 277.18 Hz). If the pitch is
exactly at 261.63 Hz (12-TET C4) or 277.18 Hz (12-TET C♯), the expression map is not assigned
correctly — see [Troubleshooting Signatures](#troubleshooting-signatures).

This verification exactly mirrors the Phase 24 3-point Dorico smoke gate: (1) quarter-sharp
plays at the target microtonal frequency; (2) no attack zipper at note onset; (3) polyphonic
isolation (a chord with C4 + E4 plays C4 detuned and E4 at 12-TET, confirming the noteId
correlation in DOCS-03 quirk #1).

### Out-of-scope distribution patterns (deferred)

- **Auto-discovery via Dorico's user-extension expression-maps directory** — Plan 25-01
  Wave 0 v3 probe FAILed (informational, non-blocking; logged in
  `25-01-WAVE-0-VERIFICATION.md` for v1.6 deferred ideas). v1.5 ships explicit-import.
- **Dorico's user-config library-additions directory write** — rejected. Path B does not
  write into Dorico's user-config space.
- **Playback Template archive distribution** — superseded by Path B (see
  `25-FINDING-path-b-validation.md`). Architecture is over-engineered for routing-only use
  cases. Candidate for resurrection if a future "demo project setup" feature wants curated
  `.pluginstate` snapshots.

---

## Host-Side Behavior Quirks

Each subsection is a single host-side behavior we observed during Phase 23 spikes (1-3) and
the Phase 25 Path B validation. Each is load-bearing for at least one piece of the shipped
plumbing.

### Dorico's neighbor-semitone + NE-delta representation (Pattern 1)

Quarter-sharp C4 in Dorico arrives as:

- `noteOn(pitch=61 (C#4), noteId=N)` — the **next semitone up**, not the named pitch
- `NoteExpression(typeId=kTuningTypeID, noteId=N, value=0.497917)` (representing −50¢) — in
  the same `processBlock`

The plugin must correlate the NE delta to the `noteId` at trigger time, not bias the noteOn
or derive tuning direction from the NE sign. The two-pass correlation in
`updatePendingFromEvents` builds a `std::map<int32_t, int> noteIdToPitch` from NoteOns first,
then resolves each `kTuningTypeID` NE event back to its pitch via the map. **Always correlate
by `noteId`, never by MIDI pitch** — same-pitch retriggers in the same block would otherwise
collide.

Final frequency is correct either way (the math `pow(2, -50¢/1200)` against C♯ = 277.18 Hz
yields ~269.29 Hz — the same number as `pow(2, +50¢/1200)` against C = 261.63 Hz), but the
implementation MUST use the noteId path. Quarter-flat C4 is similarly represented as
`pitch=59 (B3), NE=+50¢`.

### NEC handshake is ignored by Dorico (Pattern 5; Landmine 1)

Across 2103 log lines in the validated Spike 002 trace, Dorico never queried
`INoteExpressionController::iid` via `queryInterface`. Dorico routes NE events based **solely
on the expression map's `microtonalPlaybackMethod` setting**, not on the plugin's NEC
advertisement.

We keep the `Controller` advertisement anyway — Cubase, Nuendo, Bitwig, and other VST3 hosts
do use it as a capability signal. Dropping it would silently break parity with those hosts.

Practical consequence for Dorico: the Path B `.doricolib` is the load-bearing host-binding
artifact. Without the imported expression map (with `kVST3NoteExpression` set), Dorico falls
back to VST2 detune or pitch bend (see Landmine 3) regardless of how comprehensively the
plugin advertises NEC support.

### Sample-offset timing requirements

NE events carry sample offsets within the block. The voice-trigger logic must resolve the
queue entry at trigger time — which is **after** block-rate ingest, when `processBlock` calls
`renderNextBlock` — not on `noteOn` event arrival. The order in `VST3Extensions::drainAndUpdate`:

1. `drainBlockEvents` swaps the wrapper's pushed-events buffer into a scratch vector.
2. The dispatch slot's `updatePendingFromEvents` runs the two-pass correlation, populating
   `PendingTuningTable[pitch]` for every correlated `kTuningTypeID` NE.
3. The synthesizer's `renderNextBlock` fires; each voice's `startNote` reads its slot via
   `applyPendingTuning(table, midiNote, base)` — `exchange(0.0, acq_rel)`.

`acq_rel` ordering on the `exchange` pairs with the `release` ordering on the
`updatePendingFromEvents` `store`. Visibility of the value across the audio thread's two
phases (drain / render) is well-defined under the C++ memory model.

### kScoreLibrary 48-container schema requirement (NEW — v2 defect lesson)

A Dorico-valid `.doricolib` requires the full **48 top-level `<kScoreLibrary>` containers as
siblings**, even when most are empty. The factory shape (verified against
`/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml`)
is:

```xml
<kScoreLibrary>
  <temperaments> <entities array="true"/> </temperaments>
  <accidentalSystems> <entities array="true"/> </accidentalSystems>
  <accidentalDefinitions> <entities array="true"/> </accidentalDefinitions>
  <tonalitySystemDefinitions> <entities array="true"/> </tonalitySystemDefinitions>
  <ensembles> <entities array="true"/> </ensembles>
  <instruments> <entities array="true"/> </instruments>
  <instrumentNames>
    <language>kEnglish</language>
    <entities array="true"/>
  </instrumentNames>
  <!-- ... 40 more containers ... -->
  <expressionMapDefinitions>
    <entities array="true">
      <ExpressionMapDefinition>...</ExpressionMapDefinition>
    </entities>
  </expressionMapDefinitions>
  <!-- ... -->
  <lineStyleCollectionDefinition> <entities array="true"/> </lineStyleCollectionDefinition>
</kScoreLibrary>
```

Dorico's Library Manager Import rejects partial-skeleton files — including
`<kScoreLibrary><expressionMapDefinitions>...</expressionMapDefinitions></kScoreLibrary>`
fragments — with **"Error opening file: invalid file format"**.

**The v2 implementation (commit `819b2b4`) inherited this defect.** Plan 25-01 v2 trusted the
recovered cd2c2c6 XML body verbatim, but that body was an `<ExpressionMapDefinition>`
*fragment* wrapped in only `<kScoreLibrary><expressionMapDefinitions>`, not a complete library
bundle with all 48 containers. Dorico Library Manager Import failed with the invalid-file-format
error.

**v3 protocol (D-03):** bootstrap from the HALion Sonic factory `expressionMapsDefinitions.xml`
skeleton, empty all containers except `<expressionMapDefinitions>` (set
`<entities array="true"/>` on each), inject the recovered `<ExpressionMapDefinition>` body
(`entityID=xmap.ouaricon.vst3_note_expression`,
`microtonalPlaybackMethod=kVST3NoteExpression`, the technique combination block) into
`<expressionMapDefinitions>/<entities>`, write to
`modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`.

The shipped reference asset (6,431 B) is `diff -q` clean against
`/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (the verified Path B reference built during
the 2026-04-27 finding). Two-line schema sanity check before any future edit lands:

```bash
xmllint --noout modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib  # syntactic
xmllint --xpath 'count(/kScoreLibrary/*)' modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib  # must be 48
```

Special case: `<instrumentNames>` retains its child `<language>kEnglish</language>` element
alongside the empty `<entities array="true"/>` — this is a factory-skeleton invariant, not a
free choice. Stripping it produces a different-but-still-rejected file.

### Explicit-import vs auto-discovery (D-01 rationale)

v1.5 ships **explicit Library Manager Import** to one Ouaricon-controlled path, not
auto-discovery via Dorico's `Expression Maps/User/` directory. Three rationales:

1. **Determinism.** No Dorico-side scan-behavior dependency. Behavior is identical across
   Dorico versions, plugin installation orders, and user environments.
2. **Scope cleanliness.** The Ouaricon shared path
   (`~/Library/Application Support/Ouaricon/...` on macOS;
   `%APPDATA%\Ouaricon\...` on Windows) is Ouaricon-controlled. Writing into Dorico's
   user-config directories would leak plugin-installer scope into Dorico's runtime data.
3. **Empirical validation.** The Plan 25-01 Wave 0 v3 probe tested whether dropping the
   reference asset into `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/`
   would auto-populate the Endpoints dropdown without explicit import. **Result: FAIL** —
   the expression map did not appear without an explicit `Library → Library Manager → Import`
   action. Logged in `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` as
   informational evidence for v1.6 deferred-ideas.

The cost is one manual import per machine. Documented in the shipped README, in DOCS-02, and
in DOCS-04 troubleshooting. Auto-discovery is a v1.6 polish candidate if user feedback shows
the manual step is friction; the install script preserves a comment block pointer to
`git show 819b2b4:modules/tuning/note-expression/install-microtonal-suite.cmake.in` for the
Dorico-version probe pattern that auto-discovery work would revive.

### `<pluginNames>` skipped this milestone (D-02 rationale)

The shipped `.doricolib` does **NOT** populate the `<pluginNames>` element on the
`<ExpressionMapDefinition>`. The element ships self-closing-empty:

```xml
<pluginNames />
```

Practical consequence: when the user loads an Ouaricon plugin in Dorico's Endpoints panel,
Dorico does NOT auto-suggest "Ouaricon VST3 Note Expression" in the `Expression Map` dropdown.
The user picks it manually after the one-time Library Manager Import.

Why skipped: schema verification cost (verifying the exact element name and entry format
Dorico expects, validating against a factory `.doricolib` that ships with `<pluginNames>`
populated, confirming auto-suggest behavior on dev/prod CIDs) was not paid this milestone.
The deferred work is bounded — ~30 minutes of effort once a sample factory file is in hand —
and is captured as a v1.6 revival candidate.

**Carry-forward note for v1.6:** if revisited, ship both prod and dev plugin names (16 entries
total — 8 cohort plugins × {prod, dev}) so dev installs auto-suggest too. Dev users are the
maintainer cohort and the absence of auto-suggest under dev builds is friction we'll feel
ourselves. Captured here so future-Claude doesn't re-ask the user.

**Asset is CID-free.** Without `<pluginNames>` and without Path A's `<endpointconfig>`, the
shipped XML carries zero plugin GUID references. Dev/prod build flavors ship byte-identical
`.doricolib` content. The dev/prod CID divergence problem that v2 attempted to solve via
`ouaricon_extract_vst3_cids` is moot under Path B — that helper is dead code, surgically
deleted under D-10 in commit `db20a04`.

### Microtonality method invariant (Landmine 3)

The shipped `<ExpressionMapDefinition>` pins:

```xml
<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>
```

This is **the** load-bearing setting. Two failure modes if it regresses:

- `kAuto` — Dorico falls back to VST2 detune or pitch bend depending on plugin format. Neither
  reaches a JUCE-based VST3 plugin. Quarter-sharp plays at the neighbor-semitone (12-TET) on
  any non-Steinberg VST3.
- `kPitchBend` — Dorico sends pitch bend, which does not propagate per-note in the way Dorico
  needs (single-channel pitch bend bends the whole channel; MPE-style per-note pitch bend on
  separate channels is a different routing entirely).

Default expression maps (NotePerformer, HSSE, HALion, etc.) ship `kAuto`. End users who try
to "plug an Ouaricon plugin into Dorico and play a quarter-sharp" with a default expression
map hear 12-TET and assume the plugin is broken. The shipped `.doricolib` is the fix; users
who skip the import inherit Dorico's default routing.

A grep on the shipped asset confirms the invariant:

```bash
grep -c 'kVST3NoteExpression' modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
# Expected: 1
```

---

## Troubleshooting Signatures

Symptom-cause-fix table for the failure modes Path B exposes. Each row is anchored on a
specific symptom string the user is most likely to report or observe.

| Symptom | Cause | Fix |
|---------|-------|-----|
| Quarter-sharp plays at exactly 277.18 Hz (next semitone up) instead of ~269 Hz | User did not assign "Ouaricon VST3 Note Expression" to the plugin channel; map exists in library after Import but isn't bound to the plugin slot | `Play → Endpoints → <plugin channel> → Expression Map` dropdown → select "Ouaricon VST3 Note Expression". Verify under `Library → Expression Maps` that the map is present in the project library. |
| "Ouaricon VST3 Note Expression" not in `Play → Endpoints → Expression Map` dropdown | User skipped the one-time Library Manager Import | `Library → Library Manager → Import…` and select `Ouaricon-VST3-NoteExpression.doricolib` from `~/Library/Application Support/Ouaricon/Microtonal Suite/` (macOS) or `%APPDATA%\Ouaricon\Microtonal Suite\` (Windows). The import is global and persists across Dorico restarts. |
| Library Manager Import fails with "Error opening file: invalid file format" | The `.doricolib` is a `<kScoreLibrary>` fragment, not a full 48-container library bundle (the v2 defect; should not occur with the v3 shipped asset) | Verify `xmllint --xpath 'count(/kScoreLibrary/*)' file` returns 48. If not, re-author from the HALion Sonic factory skeleton per D-03 (bootstrap from `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml`, empty containers, inject `<ExpressionMapDefinition>`). The shipped 6,431 B canonical asset is the reference. |
| Expression map appears in dropdown but quarter-sharp still plays at semitone | (a) `microtonalPlaybackMethod` regressed to `kAuto`/`kPitchBend` in the imported `.doricolib`; OR (b) plugin's NEC handshake broken; OR (c) plugin advertises a CID that Dorico's name+CID match rejects (though Path B's CID-free asset makes this unlikely) | Verify `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` in the `.doricolib` (`grep -c 'kVST3NoteExpression' file` must return 1). Rebuild plugin and verify `moduleinfo.json` Audio Module Class CID matches build expectations. For dev/prod CID variance on dev installs, verify `OUARICON_DEV_SUFFIX` is consistent between the build and install (dev VST3s advertise `<Plugin>-dev`). |
| All 8 plugins missing the suite asset on a fresh install | Per-plugin packaging script did not consume the updated shared `pkg-creation.md` / `inno-template.iss` templates; the plugin was packaged with an old template before Plan 25-02 v3 landed | Rebuild PKG/EXE for the affected plugin. Verify the shared template files contain the Microtonal Suite block: `pkg-creation.md` Sections 4a + 4b should include `mkdir -p "$TEMP_DIR/payload/${PLUGIN_NAME}/microtonal-suite"` and `SUITE_DIR="$USER_HOME/Library/Application Support/Ouaricon/Microtonal Suite"`; `inno-template.iss` `[Files]` should include `{{MICROTONAL_SUITE_DORICOLIB_PATH}}` and `{{MICROTONAL_SUITE_README_PATH}}` entries. |
| Suite asset landed but file is owned by `root` on macOS | Postinstall script's `chown -R "$ACTUAL_USER:staff"` step missing or failed | Verify `pkg-creation.md` Section 4b heredoc contains the chown for `$USER_HOME/Library/Application Support/Ouaricon`. Rerun installer. Inspect Console.app for postinstall errors during the install pass. The shared template carries the correct chown line per Plan 25-02 v3 commit `b8c9b00`. |
| Plugin loads in Dorico but `processBlock` never sees NE events (silent failure mode) | JUCE patch missing or stale — `kNoteExpressionValueEvent` and `noteId`-tagged NoteOn/NoteOff are dropped in `MidiEventList::toMidiBuffer` before `onVst3RawEvent` fires | Run `./scripts/apply-juce-patches.sh`. Verify `JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)` marker present: `grep -rn 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/`. The CMake configure-time gate in `module.cmake` should fatal-error if the marker is missing, but a stale build directory may bypass the gate — clean reconfigure resolves it. |
| Dorico smoke test passes for VST3 but AU build fails to link with undefined Steinberg symbols | Pre-Phase 23 D-23-04-A regression — `JucePlugin_Build_VST3` guards in `NoteExpression.h` evaluated at TU-compile site; SharedCode (compiled with VST3=1) carried Steinberg symbol references the AU link line couldn't resolve | Should not occur with current module v1.1.0+ — the two-TU split (cpp/NoteExpression.cpp + cpp/vst3/NoteExpression_VST3.cpp) plus the per-format source routing in `modules/cmake/OuariconModules.cmake` prevent this defect class. Verify `scripts/verify-au-link.sh <Plugin>` passes. If a new consumer fails, confirm the consumer's `CMakeLists.txt` uses the per-format module include convention, not a flat `target_sources`. |
| Auto-discovery probe (Wave 0) was PASS but v1.5 still ships explicit-import | This is by design (D-01); the probe is informational only. v1.5 ships explicit-import for cross-version determinism. Auto-discovery is a v1.6 candidate | No fix; expected behavior. Log the probe result in `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` with date and Dorico version for v1.6 reference. The current Plan 25-01 Wave 0 v3 probe FAILed; FAIL is the unblocking outcome (v1.5 ships explicit-import regardless). |

When in doubt, the canonical reference asset for verifying schema correctness is the v3
shipped file at
`modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`.
`xmllint --noout file` confirms syntactic validity; `xmllint --xpath 'count(/kScoreLibrary/*)' file`
returning `48` confirms the kScoreLibrary structural invariant; `grep -c 'kVST3NoteExpression' file`
returning `1` confirms the load-bearing microtonality method. Those three checks together
catch the entire DOCS-04 failure surface that originates from the shipped asset itself.

For the host-binding side, the validated reference experiment is the Path B end-to-end test:
install any Ouaricon plugin → Dorico Library Manager Import → load plugin → assign expression
map → place quarter-sharp accidental on C4 → press play → verify ~269 Hz. The Phase 24
3-point gate (microtonal pitch + no zipper + polyphonic isolation) and the matrix-PASS
recorded in `.planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md` are the ground truth
for "plumbing works"; the troubleshooting table above maps observed-symptom back into the
plumbing layer that broke.

---

*Audience: internal-dev-only. Source-of-truth notes for v1.6+ implementers; not user-facing
manual copy. The end-user manual at the sales website (FUT-06) is out of scope for v1.5 and
will consume this document as raw material.*

*Phase: 25-package-docs · Plan: 25-03 · Path B locked.*
