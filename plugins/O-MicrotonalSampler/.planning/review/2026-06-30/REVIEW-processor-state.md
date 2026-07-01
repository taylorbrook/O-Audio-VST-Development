---
phase: processor-state-review
reviewed: 2026-06-30T00:00:00Z
depth: deep
files_reviewed: 2
files_reviewed_list:
  - plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp
  - plugins/O-MicrotonalSampler/Source/PluginProcessor.h
findings:
  critical: 1
  warning: 4
  info: 4
  total: 9
status: issues_found
---

# O-MicrotonalSampler PluginProcessor — Code Review Report

**Reviewed:** 2026-06-30
**Depth:** deep
**Files Reviewed:** 2 (`PluginProcessor.cpp` ~3300 lines, `PluginProcessor.h` ~856 lines)
**Status:** issues_found

## Summary

Reviewed the core AudioProcessor: `processBlock` real-time safety, CC11/dynamics
routing, KS/CC/PC technique triggers, VST3 Note Expression drain, full state
save/load + migration (APVTS + tuning + technique names + CC/PC tables + trims +
embedded audio), and the message-thread/audio-thread shared-state model.

Overall the RT-safety discipline in `processBlock` is strong: parameter pointers
are cached, MIDI is scanned without allocation (pre-sized `ksFilteredBuffer`), CC11
host notification is correctly deferred off the audio thread via `AsyncUpdater`
(the v1.12.1 CR-01 fix), denormals are guarded, and the output buffer is cleared.
The COW `shared_ptr` map-swap pattern is used consistently.

However, deep cross-file tracing of the **state serialization path** surfaced one
genuine data-loss bug (the embedded-audio round-trip silently drops the per-cell
`technique` axis) and several threading / migration inconsistencies where the
codebase's own documented off-message-thread `getStateInformation` guard
(persistenceLock / HG-08) is applied to some state but not to sibling state read
in the same capture function.

The three highest-impact findings are: **CR-01** (embedded-audio technique data
loss), **WR-01** (unguarded `techniqueNames` / `TuningEngine` read in
`captureStateValueTree`), and **WR-02** (preset-load state bleed-through).

## Critical Issues

### CR-01: Embedded-audio state round-trip silently drops `SampleCell.technique` (data loss)

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:2628-2655` (write) and `:2660-2711` (read)
**Issue:**
`buildEmbeddedAudioTree` serializes each cell's `midiNote` and `velocityLayer` but
NOT `cell.technique`:

```cpp
cellTree.setProperty ("midi",  cell.midiNote,      nullptr);
cellTree.setProperty ("layer", cell.velocityLayer, nullptr);
// cell.technique is never written
```

`decodeEmbeddedAudioTree` correspondingly reads only `midi`/`layer` (lines
2675-2676), so every decoded `SampleCell` keeps its default `technique = 0`
(`SampleMap.h:137`). Because `technique` is part of the cell key
(`(midiNote, velocityLayer, technique)` — see `findCellExact`, `applyMergeRrCell`,
and the collision logic in `applyFolderLoad`), an embedded folder that was
assigned to a non-zero technique slot (e.g. a `pizz` library on slot 5, or any
folder loaded with `overrideTechnique=true`) reloads with **all cells collapsed
onto technique 0 ("ord")**.

The replay path confirms the loss is not recovered elsewhere: `kickNextReplayOp`
Case 1 (line 1264-1272) applies `op.embeddedSlots` verbatim via `applyFolderLoad`,
which merges `newCell.technique` as-is and never re-derives technique from
`op.targetTechnique`/`op.overrideTechnique`. Result on project reopen: silent
sample-map corruption — cells land on the wrong technique, can collide with real
`ord` cells at the same `(midi, layer)`, and the technique tab the user built
disappears. This is opt-in (embed is off by default) but when triggered it is
silent data corruption on every reload.

**Fix:** Persist and restore the technique axis in the embedded tree.

```cpp
// buildEmbeddedAudioTree (write)
cellTree.setProperty ("midi",  cell.midiNote,      nullptr);
cellTree.setProperty ("layer", cell.velocityLayer, nullptr);
cellTree.setProperty ("tech",  cell.technique,     nullptr);   // ADD

// decodeEmbeddedAudioTree (read)
cell.velocityLayer = juce::jlimit (0, 3, (int) cellTree.getProperty ("layer", 0));
cell.technique     = juce::jlimit (0, kMaxTechniques - 1,
                                   (int) cellTree.getProperty ("tech", 0));  // ADD
```

Absence of the `tech` attr (older embedded saves) defaults to 0, preserving
back-compat. Consider also recomputing/auto-growing `technique_count` after an
embedded replay (as `applyFolderLoad` already does) so the restored tab strip is
visible.

## Warnings

### WR-01: `techniqueNames` and `TuningEngine` read without `persistenceLock` in `captureStateValueTree` — data race with off-message-thread `getStateInformation`

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:2924-2940` (and `:2718-2745`)
**Issue:**
The codebase explicitly documents that some hosts (Reaper) call
`getStateInformation` off the message thread — that is the entire rationale for
the HG-08 `persistenceLock` guarding `loadOpHistory` and `lastSkippedFiles`
(see header `PluginProcessor.h:719-731` and `captureStateValueTree` line
2878-2882). But the same function, in the same off-thread call, reads two other
pieces of mutable non-atomic state **without any lock**:

- `techniqueNames` (a `juce::StringArray`) at lines 2932-2936, while
  `setTechniqueName` / `resetTechniqueNames` (lines 2250-2280) mutate it on the
  message thread with no synchronization.
- `tuningEngine` via `captureTuningValueTree` (lines 2924-2925 → 2718-2745),
  which calls `getMasterTune`, `getIntervals`, `generateScalaFileContent`, etc.,
  while the UI can be editing the tuning on the message thread.

`juce::StringArray` reallocates its backing array on `set`/`add`; a concurrent
read during reallocation is a torn read / use-after-free → crash. This is the
exact scenario the persistenceLock was introduced to prevent, applied
inconsistently. The mitigation is undermined for every user who both runs Reaper
(off-thread save) and edits technique names or tuning.

**Fix:** Snapshot `techniqueNames` under `persistenceLock` the same way
`loadOpHistory` is snapshotted, and take the lock in the setters. For the tuning
engine, either capture a snapshot under the lock or document that tuning writes
must also hold it. Minimum:

```cpp
juce::StringArray techNamesSnapshot;
{
    const juce::ScopedLock persistLock (persistenceLock);
    techNamesSnapshot = techniqueNames;   // copy under lock
}
// ...build <TechniqueNames> from techNamesSnapshot...
// and guard setTechniqueName/resetTechniqueNames with the same lock.
```

### WR-02: Preset load leaves stale technique names and CC/PC tables (state bleed-through)

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:3021-3045` (2b) and `:3059-3109` (2d)
**Issue:**
`restoreStateValueTree` handles "state the preset does not carry" inconsistently
across its sections:

- **Trims (2e, lines 3116-3122):** correctly reset the entire `cellTrims` table to
  0 dB *before* applying the sparse saved entries — so a preset with no
  `<CellTrims>` restores to unity, no bleed.
- **Technique names (2b, lines 3026-3045):** only override slots present in the
  saved tree; never reset to the default vocabulary first. Loading a preset that
  lacks `<TechniqueNames>` (or carries fewer slots) leaves the *previous* preset's
  renamed techniques in place.
- **CC/PC tables (2d, lines 3060-3109):** only rebuilt when the corresponding
  child is `isValid()`. A preset with no `<CcMapping>`/`<PcMapping>` leaves the
  previous session's custom trigger tables active.

For project reopen this is usually masked (a same-version save always emits all
children), but `restorePresetXml` explicitly exists to load presets across
sessions/versions — so loading an older or partial `.omspreset` over a customized
session produces a silently mixed state that the user did not author.

**Fix:** Mirror the trims pattern: reset `techniqueNames` to `resetTechniqueNames()`
defaults and reset the CC/PC shared_ptrs to `defaultCcMapping(count)` /
`defaultPcMapping()` before applying whatever the loaded tree carries. This makes
every preset load deterministic regardless of prior state.

### WR-03: Fresh-instance default absorbs MIDI notes 0-9 as keyswitches; stale back-compat comment

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:257-281` (params) and `:245-256` (comment)
**Issue:**
The block comment at 245-256 states the back-compat default is
`technique_count=1, ks_enabled=false` ("reproduces v1.13.0 behaviour exactly").
The actual parameter defaults contradict it:

```cpp
technique_count : default 8   (line 260)
ks_enabled      : default true (line 270)
ks_low_note     : default 0    (line 275)
ks_high_note    : default 9    (line 280)
```

With these defaults, a **fresh plugin instance silently absorbs every note-on in
MIDI 0-9 (C-1..A-1) as a keyswitch** (`processBlock` lines 655-672 `continue`s
past — never forwarded to the synth). Any sample library mapped down into that
low range loses those notes with no user-visible cause. The processBlock KS
absorption is unconditional on `ks_enabled`, so this is on by default.

**Fix:** Either (a) restore the documented safe default (`ks_enabled=false`) so
keyswitches are opt-in, or (b) if KS-on-by-default is the intended v1.14+ product
decision, update the misleading comment AND consider defaulting the KS range to a
region outside the normal playable sample range (or gating absorption on a loaded
map that actually populates technique slots). At minimum the comment must be
corrected so future maintainers don't "fix" the defaults back.

### WR-04: `capturePresetXml`/`getStateInformation` can serialize a half-mutated map during a live load

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:2916-2917`
**Issue:**
`buildEmbeddedAudioTree(*op.embeddedSlots)` walks the embedded slots' audio
buffers and base64-encodes them during capture. The capture takes a snapshot of
`loadOpHistory` under `persistenceLock` (good), but `applyFolderLoad` releases the
lock *before* it finishes wiring `sampleMapChangedCallback` and the embed snapshot
is attached inside the locked region (line 1117-1121), so the history snapshot is
consistent. The residual risk is that WAV encoding of large embedded libraries
runs while holding references but is CPU-heavy on whatever thread the host used
for `getStateInformation`; if that is the audio-adjacent thread in some hosts it
can stall. This is lower severity than WR-01 (correctness is preserved because
the `embeddedSlots` shared_ptr pins the buffers), but the per-variant 24-bit WAV
encode of a 250 MB library on a save call is a latency hazard worth bounding.

**Fix:** Document the expected calling thread, and consider moving embedded-audio
WAV encoding to a background step or caching the encoded blobs at load time rather
than re-encoding on every save.

## Info

### IN-01: `std::atomic_load` on `shared_ptr` in the audio path is not lock-free

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:33-42, 648-649` (and per-voice `startNote`)
**Issue:** `processBlock` calls `atomicLoad(currentCcMapping)` / `atomicLoad(currentPcMapping)`
every block and voices `atomicLoad(currentSampleMap)` at note-on. The C++17
free-function `std::atomic_load(&shared_ptr)` is implemented on libc++/libstdc++
with a spinlock pool, not lock-free — the audio thread can briefly spin on a lock
the message thread holds during `atomic_store`. The priority-inversion window is
tiny (pointer swap + refcount) and this is a known/accepted constraint (C++17 has
no `std::atomic<shared_ptr>`; see project MEMORY.md). Recorded for awareness only;
no action required unless targeting a lock-free upgrade under C++20.

### IN-02: `processBlock` walks the MIDI buffer twice

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:582-587` and `:655-725`
**Issue:** The CC11 scan (lines 582-587) and the KS/CC/PC scan (lines 655-725)
are two independent passes over `midiMessages`. Correctness is fine; the CC11
harvest could be folded into the KS/CC/PC loop (or vice-versa) to walk the buffer
once. Minor maintainability/clarity note (perf is out of v1 scope).

### IN-03: Default technique vocabulary disagrees across code and comments

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:2266-2280` vs `PluginProcessor.h:506, 648`
**Issue:** `resetTechniqueNames()` seeds `{"ord","sp","st","stacc","cs","pizz","harm","trem"}`,
but the header docstrings (`.h:506` and `.h:648`) and the state comment
(`.cpp:2244`) advertise `{"ord","sp","st","sv","cs","pizz","harm","mart"}`. Slots
3 and 7 differ. Cosmetic, but the persisted default names drive the UI and any
default expression-map alignment; reconcile the comments with the code.

### IN-04: KS candidate clamp maps two notes to the same technique at default range

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:667-669`
**Issue:** With `ks_high_note=9` and `technique_count` clamped to 8, notes 8 and 9
both `jlimit` to technique 7 (`jmin(7, techCount-1)`). Harmless (both select the
last technique) but means the default KS range advertises one more slot than exists.
Consider defaulting `ks_high_note` to `ks_low_note + technique_count - 1` or
documenting the saturation.

---

_Reviewed: 2026-06-30_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
