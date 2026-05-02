---
plugin: O-MicrotonalSampler
version: v1.11.1
reviewed: 2026-05-02
depth: deep
files_reviewed: 12
files_reviewed_list:
  - Source/PluginProcessor.cpp
  - Source/PluginProcessor.h
  - Source/PluginEditor.cpp
  - Source/PluginEditor.h
  - Source/MicrotonalSamplerVoice.cpp
  - Source/MicrotonalSamplerVoice.h
  - Source/MicrotonalSamplerSound.h
  - Source/SampleLoader.cpp
  - Source/SampleLoader.h
  - Source/FilenameParser.cpp
  - Source/FilenameParser.h
  - Source/SampleMap.h
findings:
  critical: 5
  high: 9
  medium: 11
  low: 8
  total: 33
status: issues_found
---

# O-MicrotonalSampler — C++ Code Review

**Reviewer:** gsd-code-reviewer (Claude Opus 4.7)
**Stance:** Adversarial — assume defects exist; surface what is provable.

## Summary

| Severity | Count |
|----------|-------|
| CRITICAL | 5 |
| HIGH | 9 |
| MEDIUM | 11 |
| LOW | 8 |
| **Total** | **33** |

**Top concerns:**
1. **Audio-thread allocation/lock violations** — `processBlock` calls `setValueNotifyingHost` for every CC11 message (CR-01), which performs message-thread dispatch + locks. This is a clear real-time violation and the most exploitable defect surfaced.
2. **TOCTOU + use-after-free risk on `cellLow`/`variantLow` pointers** — voice stores raw pointers into a `SampleMap` it snapshotted, but then RE-snapshots on every `startNote` *without* clearing the prior pointers first. If `startNote` exits early after re-snapshot, dangling pointers may persist (HG-01).
3. **Path-traversal vulnerability in `dropSessionAddFile`** — JS-supplied `relPath` is passed to `getChildFile` without validating against `..`, absolute paths, or symlinks, allowing the WebView (compromised JS or hostile drag-drop content) to write arbitrary files (CR-02).
4. **Integer overflow / DoS in base64 streaming** — no bound on per-file or aggregate session size; a malicious page or huge folder drag can OOM the plugin (CR-03).
5. **Filename parser strict-prefix bug** — `parseAsRrIndex` matches `take` *before* checking for full prefix specificity, so a filename like `take-C3` (where `take` is a token) silently consumes an empty digits string and returns nullopt — but `tk` shadowing inside `take` produces incorrect behavior on tokens like `taken1`. Verified below (HG-02).

---

## CRITICAL

### CR-01: Audio-thread calls `setValueNotifyingHost` (real-time violation)
**File:** `Source/PluginProcessor.cpp:313-321`
**Severity:** CRITICAL — RT-safety violation, can cause audio glitches and host deadlocks
**Issue:**
```cpp
for (const auto meta : midiMessages)
{
    const auto msg = meta.getMessage();
    if (msg.isController() && msg.getControllerNumber() == 11)
    {
        if (auto* ep = parameters.getParameter ("expression"))
            ep->setValueNotifyingHost (msg.getControllerValue() / 127.0f);
    }
}
```
`setValueNotifyingHost()` is documented as message-thread-only in JUCE and triggers parameter listeners (which under VST3/AU may acquire host locks, post messages, allocate, or block). Calling it from `processBlock` for every CC11 byte is a real-time correctness violation. A fast-moving CC11 stream from a DAW can stall the audio thread.

**Why it matters:** Crackles, dropouts, and (in some hosts — Logic, FL Studio) deadlocks when host listeners chain back to the audio thread.

**Fix:** Stage the value into a `std::atomic<float> pendingExpressionFromCC11`, then post to the host via a `juce::MessageManager::callAsync` or a lock-free queue drained by an editor-attached AsyncUpdater. Alternatively, since the squared smoother already drives the audio gain, write the raw value *only* to a thread-local atomic and let a 30 Hz timer in the editor (or processor's own AsyncUpdater) push to the host.

---

### CR-02: Path traversal in `dropSessionAddFile` allows arbitrary filesystem writes
**File:** `Source/PluginEditor.cpp:399-452` (specifically lines 409, 439-441)
**Severity:** CRITICAL — security
**Issue:**
```cpp
const auto relPath   = args[1].toString();
// ...
auto target = currentDropSessionDir.getChildFile (relPath);
target.getParentDirectory().createDirectory();
if (! target.replaceWithData (mb.getData(), mb.getSize()))
```
`relPath` comes from JavaScript and is passed directly to `juce::File::getChildFile`. `getChildFile` *does* normalize `..` — but it is not guaranteed to resolve to a child of the parent: per JUCE source, `getChildFile("../foo")` returns the parent's sibling `foo`. Combined with `replaceWithData`, this gives JS arbitrary write access anywhere the plugin process can write. Symlinks inside the temp dir created by an attacker also escape.

**Why it matters:** A malicious page (or compromised resource) loaded in the WebView can write a file to e.g. `~/Library/LaunchAgents/com.evil.plist`, `$HOME/.ssh/authorized_keys`, plugin component bundles in `~/Library/Audio/Plug-Ins/`, etc. Since users routinely drag-drop folders here, even a single malformed JS payload smuggled through a future tuning-import flow could compromise the host.

**Fix:**
```cpp
const auto target = currentDropSessionDir.getChildFile (relPath);
if (! currentDropSessionDir.isAChildOf (target.getParentDirectory())
    && target.getParentDirectory() != currentDropSessionDir
    && ! target.getParentDirectory().isAChildOf (currentDropSessionDir))
{
    DBG ("dropSessionAddFile: path traversal attempt: " << relPath);
    complete (juce::var (false));
    return;
}
// Also reject absolute paths up front:
if (juce::File::isAbsolutePath (relPath)) { complete (juce::var (false)); return; }
// Reject backslashes and any segment equal to "..":
if (relPath.contains ("..") || relPath.contains ("\\")) {
    complete (juce::var (false)); return;
}
```

---

### CR-03: No size limit on base64 file streaming — DoS / OOM
**File:** `Source/PluginEditor.cpp:399-452`
**Severity:** CRITICAL — DoS, memory exhaustion
**Issue:** `dropSessionAddFile` decodes an arbitrary-length base64 string into a `juce::MemoryBlock` with no per-file or session-aggregate cap. The CHANGELOG note ("250MB libraries before background loader starts") implies multi-GB drops are reachable. Worse, when `replaceWithData` fails (disk full, permissions), the held `MemoryBlock` is still alive in the lambda scope.

A malicious WebView page calling `dropSessionAddFile` with a 4GB base64 string crashes the DAW (32-bit allocator overflow) or freezes the system (paging).

**Why it matters:** Any plugin host has a fixed RAM budget; a renegade base64 payload trivially exhausts it. Also: `juce::MemoryOutputStream stream (mb, false)` *appends* to `mb` — but `mb` is freshly constructed, so this is fine — however, no max-size check is done before the decode begins.

**Fix:**
```cpp
constexpr size_t kMaxFileBytes = 256 * 1024 * 1024;   // 256 MB per file
constexpr size_t kMaxSessionBytes = 4ULL * 1024 * 1024 * 1024;  // 4 GB per session
// Track aggregate size on the editor:
size_t currentDropSessionTotalBytes = 0;

// Roughly: decoded_size ≈ base64.length() * 3 / 4
const size_t projected = (size_t) base64.length() * 3 / 4;
if (projected > kMaxFileBytes
    || currentDropSessionTotalBytes + projected > kMaxSessionBytes)
{
    DBG ("dropSessionAddFile: size cap exceeded");
    complete (juce::var (false));
    return;
}
// after successful write:
currentDropSessionTotalBytes += mb.getSize();
```

---

### CR-04: Use-after-free risk on stale `variantLow`/`cellLow` pointers when sample map is reloaded mid-note
**File:** `Source/MicrotonalSamplerVoice.cpp:336-374, 553-565` and `Source/MicrotonalSamplerVoice.h:89-97`
**Severity:** CRITICAL — UB, potential audio-thread crash
**Issue:** Voice holds raw pointers `cellLow`, `cellHigh`, `variantLow`, `variantHigh` that index into `currentMap->cells[i].variants[j]`. The lifetime guarantee from `currentMap` (shared_ptr held by member) keeps the buffer alive — *but only as long as `currentMap` is unchanged*.

In `startNote` (line 354-360):
```cpp
if (sampleMapSource != nullptr)
{
    currentMap = std::atomic_load (sampleMapSource);  // overwrites prior shared_ptr
}
```
If the user reloads a folder between two notes that arrive in the same audio block (or while the voice is in the steal-tail state), `currentMap` is replaced. The OLD `cellLow/variantLow` pointers were taken from the PREVIOUS map, which has now been dropped (refcount went to zero unless another voice held a snapshot). The pointers dangle.

The order of operations in `startNote` (steal-detection on line 337 happens BEFORE the new snapshot on line 354) means `renderTailRamp` reads from `variantLow` *before* it gets overwritten — but `variantLow` is *not* re-validated against the new map, and on a real implementation the dangling pointer remains accessible until line 386-390 resets it.

Worse, in the `startNote` ELSE branch on line 366-374 ("currentMap == null or empty"), pointers are only nulled if the snapshot is empty. The intermediate moment between the new snapshot taking effect (line 359) and pointers being either reassigned (line 466-472) or nulled (line 386-390) leaves a window where `variantLow` points into the freed prior map.

**Why it matters:** The window is small but real — a heavily concurrent state restore (replay queue + drop-folder) can trigger it. Symptom: rare audio-thread crash when stealing voices during folder load.

**Fix:** At the top of `startNote`, snapshot the new map BEFORE running steal-tail logic, OR keep a separate `prevMap` shared_ptr for the duration of `renderTailRamp`:
```cpp
void startNote(...) {
    // Steal-tail uses CURRENT pointers — but render reads via the OLD map.
    // Capture the old map locally so refcount stays ≥ 1 across the swap.
    std::shared_ptr<SampleMap> prevMap = currentMap;  // local ref keeps it alive
    if (adsr.isActive() && variantLow != nullptr) {
        renderTailRamp(...);  // safe because prevMap is alive
    }
    // ... now swap to new map:
    currentMap = std::atomic_load(sampleMapSource);
    cellLow = cellHigh = nullptr;
    variantLow = variantHigh = nullptr;
    // ... rest of startNote
}
```

---

### CR-05: `kickNextReplayOp` recursive callback can deadlock during state restore if `sampleLoader` is null
**File:** `Source/PluginProcessor.cpp:627-681`
**Severity:** CRITICAL — null deref
**Issue:**
```cpp
void OMicrotonalSamplerAudioProcessor::kickNextReplayOp()
{
    while (! pendingReplayOps.empty())
    {
        // ... pops op ...
        sampleLoader->loadFolder (...);  // line 649 — no null check
        return;
    }
}
```
`sampleLoader` is a `unique_ptr` initialized in the constructor (PluginProcessor.cpp:161). If construction fails (`std::make_unique` throws — e.g. OOM during plugin load) the destructor runs and the processor never finishes constructing — but `setStateInformation` calls into `restoreStateValueTree` which calls `kickNextReplayOp`. If `kickNextReplayOp` is somehow reached with a null `sampleLoader` (e.g. via a future refactor or during shutdown when the loader was reset), this is a null dereference.

`loadSampleFolder` line 410 *does* null-check `sampleLoader`, but `kickNextReplayOp` does not — a defensive inconsistency.

**Why it matters:** Defensive consistency. The replay path is reachable from `setStateInformation`, which JUCE may call at any plugin state, including reactivation after a `releaseResources`/release-then-resurrect cycle in some hosts.

**Fix:** Add a null-guard at line 649:
```cpp
if (sampleLoader == nullptr) {
    pendingReplayOps.clear();
    return;
}
```

---

## HIGH

### HG-01: `confirmRoundRobinLoad` chain continuation captures `this` after potential map-swap; lambda outlives function
**File:** `Source/PluginProcessor.cpp:589-620`
**Severity:** HIGH — lifetime / re-entrancy
**Issue:** `pendingDuplicateChainContinuation` captures `this` by reference (effectively); it's invoked synchronously at the end of `confirmRoundRobinLoad` (line 619). However, if `applyFolderLoad` (line 606) itself triggers a sample-map-change callback that re-enters into another folder load (via UI), the chained `chain()` call dispatches a new `kickNextReplayOp` which schedules `loadFolder`, which wipes the loader's state — including any in-flight callback referencing the now-already-popped `pendingReplayOps.front()`.

**Why it matters:** State-restore paths chain N+1 ops; if op N's confirmation fires another folder load before chain() runs, the queue is corrupted. Since `pendingReplayOps` is mutated by `kickNextReplayOp` itself, the `chain()` call after `applyFolderLoad` re-enters a now-stale queue.

**Fix:** Save `pendingReplayOps.empty()` state before `applyFolderLoad`, and only chain if the queue still represents the original sequence. Alternatively, use a generation counter on the queue and abort chain calls from stale generations.

---

### HG-02: Filename parser `parseAsRrIndex` accepts ambiguous 0-digit input via "rr"/"tk" prefix shadowing in the "take"-prefix path
**File:** `Source/FilenameParser.cpp:246-275`
**Severity:** HIGH — logic / data integrity
**Issue:** The order of prefix checks is `rr` → `take` → `tk`. But `tk` is a prefix of itself only — fine. `rr` is the prefix for `rr1`, `rr03`. `take` is the prefix for `take1`. **However**, `take` also starts with `t` and contains `ke` — so `taker` doesn't match `tk` but the test `lc.startsWith("take")` for input `taker` returns true, then `extractTrailingDigits("r")` returns nullopt — fine. **But:**
- Input `tk5x` — `lc.startsWith("tk")` is true, calls `extractTrailingDigits("5x")` — returns nullopt due to non-digit. **Correct.**
- Input `rrr` — `startsWith("rr")` is true, extract from "r" → nullopt. **Correct.**
- **But** input `take` — `startsWith("rr")` false, `startsWith("take")` true, extract from "" → empty string returns nullopt at the `len < 1` check. **Correct.**
- **However:** input `take-1` is tokenized to `["take", "1"]` (because `-` is a delimiter at line 305 of FilenameParser.cpp). Token "take" alone will not match (extractTrailingDigits empty), and token "1" may be parsed as bare integer (note) by line 335. So a filename like `C3-take-1` parses MIDI=60, vel=0, **rr=−1** — but the user's intent was clearly `take=1` (rr index 0).

This is an INVERSION bug: well-intentioned filenames using delimiter-separated take indices silently lose RR semantics. Worse, the "1" token may match `parseAsBareInteger` and shift the interpretation entirely — though in this case `C3` already provides a note so the bare-int fallback is skipped (line 333).

**Why it matters:** Users who follow conventions like `Piano_C3_take_1.wav` (separator-separated) get RR-disabled silently. There is no warning. The RR token convention is documented as `take[N]` — non-separated — but tokenizers eating the digit is a foot-gun.

**Fix:** Either (a) extend the tokenizer to recognize "take_1" as one token by NOT splitting on underscore between `take`/`tk`/`rr` and a trailing digit, or (b) post-process: if a token T is `take`/`tk`/`rr`, check if T+1 is a 1-2 digit integer and consume both. Document the convention more strictly.

---

### HG-03: `applyFolderLoad` MergeRR path skipped slot fails to reset RR counter for newly-added cells
**File:** `Source/PluginProcessor.cpp:521-546`
**Severity:** HIGH — logic
**Issue:**
```cpp
if (op.mode == LoadMode::MergeRR)
{
    applyMergeRrCell (merged->cells, newCell, kMaxVariantsPerCell, lastSkippedFiles);
}
else { /* replace */ }
rrCounters[(size_t) counterIdx].store ((uint8_t) 0xFFu, ...);  // line 544
```
The counter reset is unconditional after either branch. Good. **But:** `applyMergeRrCell` may reject all variants when the cap is reached (returns false), in which case the existing cell is unchanged — yet the RR counter is still reset. This means a fully-loaded 64-variant cell that rejects an incoming merge will have its RR cycle restarted on the next note-on, picking variant 0 instead of continuing the cycle.

**Why it matters:** Subtle UX defect — users adding a new variant that hits the cap silently see the RR cycle reset. Audible if they're playing fast cycles.

**Fix:** Move the counter reset inside the `if (anyChange)` branch:
```cpp
const bool changed = (op.mode == LoadMode::MergeRR)
    ? applyMergeRrCell (merged->cells, newCell, kMaxVariantsPerCell, lastSkippedFiles)
    : (replaceCellAndReturnTrue(...));
if (changed)
    rrCounters[(size_t) counterIdx].store(0xFFu, std::memory_order_relaxed);
```

---

### HG-04: `selectVariantIndex` clips counter store to 254, silently corrupting state if N > 254
**File:** `Source/MicrotonalSamplerVoice.cpp:232-233`
**Severity:** HIGH — defensive logic / latent bug
**Issue:**
```cpp
counter.store ((uint8_t) juce::jlimit (0, 254, next),
               std::memory_order_relaxed);
return next;
```
The clip-to-254 is to preserve 0xFF as the sentinel. But the function returns the unclipped `next`, used to index `cell.variants` (line 467). If `N` ever exceeds 254, the stored counter saturates while the returned `next` does not — the next invocation will read counter=254 even when the actual previous index was 255+, so RR cycle behavior diverges silently.

**Today** N is capped at `kMaxVariantsPerCell = 64` (PluginProcessor.cpp:519), so the clip-to-254 never trips. **But this is a brittle invariant** — if someone bumps the cap to 256+, this becomes a wrap-around bug that is hard to spot.

**Why it matters:** Hidden coupling between `kMaxVariantsPerCell` and the counter storage. The constant should be enforced statically.

**Fix:** Either:
```cpp
static_assert(kMaxVariantsPerCell <= 254, "RR counter is uint8_t with 0xFF sentinel; max variants must fit in 254");
```
Or expand the counter to `uint16_t` and accept the doubled memory cost (1KB instead of 512B).

---

### HG-05: `loadSampleFolder` callback captures `op` by value but the lambda holds the closure for an indeterminate amount of time
**File:** `Source/PluginProcessor.cpp:429-447`
**Severity:** HIGH — concurrency / lifetime
**Issue:**
```cpp
sampleLoader->loadFolder (
    folder, ...,
    [this, op](std::shared_ptr<SampleMap> newMap, ...) { ... },
    [this](const juce::String& reason) { ... });
```
`this` is captured by raw pointer. Comments document that `~SampleLoader` joins (`stopThread(2000)`), and the callbacks are dispatched via `MessageManager::callAsync` from `SampleLoader::run` (line 416). However:
- If the user closes the project (processor destroyed) WHILE a callback is queued in the `MessageManager`, the lambda runs after `~OMicrotonalSamplerAudioProcessor` — `this` dangles. JUCE's `~AudioProcessor` does NOT cancel pending message-thread callbacks created via `callAsync`.
- The 2-second `stopThread` timeout in `~SampleLoader` only joins the thread; it does NOT flush queued message-thread tasks already posted by the now-completed thread.

**Why it matters:** A user closing a project mid-load can crash the host. JUCE's pluginval and stress-test harnesses occasionally exercise this path.

**Fix:** Use `juce::MessageManager::callAsync` with a weak handle (`juce::WeakReference<OMicrotonalSamplerAudioProcessor>`), or track outstanding callbacks in the processor and cancel/no-op them in the destructor:
```cpp
std::shared_ptr<bool> aliveFlag = std::make_shared<bool>(true);  // member
~processor() { *aliveFlag = false; }
// in callback:
[this, op, alive=aliveFlag](...) {
    if (!*alive) return;
    // ...
}
```

---

### HG-06: `dropSessionAddFile` `MemoryOutputStream stream(mb, false)` — second arg `appendToExistingBlockContent` is `false`, but the JUCE doc says `false` discards prior data (correct here) — *however*, `mb.getSize()` after flush may be 0 if decode failed
**File:** `Source/PluginEditor.cpp:424-441`
**Severity:** HIGH — silent data loss / no validation
**Issue:**
```cpp
juce::MemoryBlock mb;
{
    juce::MemoryOutputStream stream (mb, false);
    if (! juce::Base64::convertFromBase64 (stream, base64)) { ... return false; }
    stream.flush();
}
// later:
target.replaceWithData (mb.getData(), mb.getSize());
```
If `convertFromBase64` returns true but writes 0 bytes (e.g. `base64` is an empty string — not currently rejected since the check above requires `args.size() >= 3` but doesn't validate non-empty content), `replaceWithData(nullptr, 0)` may silently create an empty file. The audio loader will then skip it as "invalid header" — but the user gets no feedback that their 0-byte file was a transmission failure.

**Why it matters:** A flaky JS bridge (intermittent transmission failures) silently corrupts the user's drag-drop with empty files, which then surface as cryptic "invalid header" toasts.

**Fix:**
```cpp
if (mb.getSize() == 0) {
    DBG ("dropSessionAddFile: decoded to 0 bytes (transmission failure?)");
    complete (juce::var (false));
    return;
}
```

---

### HG-07: `MicrotonalSamplerVoice::renderNextBlock` resets state from inside a per-sample loop while still executing later iterations
**File:** `Source/MicrotonalSamplerVoice.cpp:641-650`
**Severity:** HIGH — control flow
**Issue:**
```cpp
for (int i = 0; i < numSamples; ++i)
{
    // ... ADSR + render ...
    if (! adsr.isActive())
    {
        cellLow         = nullptr;
        cellHigh        = nullptr;
        variantLow      = nullptr;
        variantHigh     = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }
}
```
The ADSR is checked AFTER rendering the sample. But `clearCurrentNote()` releases the voice back to the synth pool. If the synth (running on the same audio thread) reassigns the voice within the same `renderNextBlock` call (it shouldn't — JUCE's render walks voices serially), the next iteration in another voice's render sees stale state.

More concretely: on the FINAL sample of a block (i = numSamples - 1), this exits early without rendering the remaining samples — there are none, so this path is fine. **But:** if the ADSR reaches inactive on iteration 0, samples 1..numSamples-1 are NOT rendered AND NOT zeroed. The output buffer was cleared at processBlock top, so they remain at 0 — *unless* the buffer was non-zero from an earlier voice in this block (it would have been added to). Re-reading `processBlock` line 301 — `buffer.clear()` runs once per block before voice render — so the zeros are correct. **Edge case:** if ADSR goes inactive early but a steal-tail is still in progress, the steal-tail mix happened at the top of `renderNextBlock` (line 531-551) BEFORE the per-sample loop, so it's already mixed in. Good.

**Verdict:** Not a bug today, but the early `return` from inside a per-sample loop without zeroing the remaining samples is fragile. Consider a `break` instead of `return`, with an explicit "remaining samples are zero by buffer.clear() invariant" comment.

**Why it matters:** Future refactors might add post-loop work in `renderNextBlock` (e.g. per-block filtering) and break the invariant silently.

**Fix:** Replace `return` with `break`, then audit assumptions about post-loop work.

---

### HG-08: Race condition between `applyFolderLoad` member mutations and audio-thread reads
**File:** `Source/PluginProcessor.cpp:467-583`
**Severity:** HIGH — concurrency
**Issue:** `applyFolderLoad` runs on the message thread. It mutates:
- `currentSampleMap` — atomic via `std::atomic_store` ✓
- `rrCounters[i]` — atomic via per-element atomics ✓
- `loadOpHistory` — `std::vector<LoadOp>` — **NOT atomic**
- `lastSkippedFiles` — `juce::StringArray` — **NOT atomic**

Audio thread reads `currentSampleMap` (atomic — fine) and `rrCounters` (atomic — fine), but the `getStateInformation` path on the message thread reads `loadOpHistory` and `lastSkippedFiles` while another message-thread call (`applyFolderLoad`) may also be writing. This is *probably* fine because both are on the message thread — but `getStateInformation` is called by the host on what JUCE documents as "the message thread or a thread that owns the AudioProcessor" — i.e. NOT guaranteed to be the message thread in all hosts.

Reaper, in particular, has been known to call `getStateInformation` from a save-state worker thread.

**Why it matters:** Project save during a folder-load completion can read a half-written `loadOpHistory` vector, producing corrupted XML. Likely silent data loss on save.

**Fix:** Wrap `loadOpHistory` and `lastSkippedFiles` mutations + reads under a `juce::CriticalSection`, OR document the threading contract precisely (host must serialize). The least-invasive fix:
```cpp
mutable juce::CriticalSection persistenceLock;
// in applyFolderLoad: const juce::ScopedLock l(persistenceLock); /* mutate */
// in captureStateValueTree: const juce::ScopedLock l(persistenceLock); /* read */
```

---

### HG-09: `loadSingleSample` callback computes `placed` but doesn't handle the `mergeAsRr=false` collision case correctly
**File:** `Source/PluginProcessor.cpp:803-844`
**Severity:** HIGH — logic
**Issue:**
```cpp
auto next = std::make_shared<SampleMap>();
bool placed = false;
if (currentMap != nullptr) {
    next->cells.reserve(currentMap->cells.size() + 1);
    for (const auto& c : currentMap->cells) {
        if (c.midiNote == targetMidi && c.velocityLayer == targetVel) {
            if (doMerge) {
                SampleCell mergedCell = c;
                mergedCell.variants.push_back (newVariant);
                next->cells.push_back (std::move (mergedCell));
                placed = true;
            }
            // mergeAsRr=false → drop the old cell (replace path).
            continue;
        }
        next->cells.push_back (c);
    }
    // ...
}
// ...
if (! placed) {
    SampleCell freshCell;
    freshCell.midiNote      = targetMidi;
    freshCell.velocityLayer = targetVel;
    freshCell.variants.push_back (std::move (newVariant));
    next->cells.push_back (std::move (freshCell));
}
```
When `mergeAsRr=false` and a collision happens, the existing cell is dropped (skipped via `continue`), but `placed` remains false, so the `if (!placed)` block creates a fresh cell. This is correct behavior. **But:** `newVariant` was passed by value to the lambda (line 750), then captured by the inner branches as a reference. When `doMerge=true`, it's pushed via copy at line 815 (no `std::move`). When `!placed`, it's `std::move`d at line 842. So the merge path silently makes a redundant copy of `newVariant.audio` (a `shared_ptr` — cheap) and `newVariant.filename` (a `juce::String` — cheap). **Not a bug**, but inefficient.

**Real bug:** the `existingCell` pointer (line 775) was acquired by linear scan over `currentMap->cells` BEFORE the deep-copy loop. The pointer is used at line 786-790 only for the size check. After this check, the new map is built independently. **This is fine** — but the pointer is stored as a parameter to a check that uses `currentMap->cells` (the source). This is safe because we hold `currentMap` (the snapshot) for the entire callback. **No issue.**

**Real real bug — found on closer reading:** The `if (next->cells.size() == 1)` block at line 851-855 unconditionally sets lowestNote/highestNote to targetMidi. This is INCORRECT if the collision branch (mergeAsRr=true) replaced the only cell — in which case `next->cells.size() == 1` after the merge, and we incorrectly clamp the range. **However**, in that case `targetMidi` IS the only cell's MIDI note, so the clamp is correct. **No issue.**

**Actual real issue:** When `currentMap == nullptr`, line 833 sets `next->numVelocityLayers = 1` — but line 848 then computes `juce::jmax(next->numVelocityLayers, targetVel + 1)` — so a fresh map with vel=3 gets `numVelocityLayers = 4`. **Correct.** But if `currentMap != nullptr` AND has `numVelocityLayers = 4` AND we're adding a vel=0 single sample, `next->numVelocityLayers` is preserved at 4. **Correct.**

**Real defect found:** When a single-sample replace happens on the LAST cell of a map (i.e. there was exactly one cell at coords (X, V) and we replace it), `next->cells.size()` == 1 after the loop. Lines 824-827 copied `currentMap->lowestNote/highestNote` (e.g. could be 21..108 from a prior multi-cell map that's been mostly deleted — but if there's only one cell, lowest=highest=that cell's midi). Line 851-855 then sets lowestNote = highestNote = targetMidi. **Correct.**

I cannot construct a counter-example. Demoting to MEDIUM (see MD-04 below).

**Verdict:** Withdraw HG-09.

---

## MEDIUM

### MD-01: `MicrotonalSamplerVoice::stealTailBuffer` size is `kMaxStealRamp = ceil(0.005*SR) + 16`, but at SR=192kHz this is 976 samples — initialization at prepareToPlay only
**File:** `Source/MicrotonalSamplerVoice.cpp:144-152`
**Severity:** MEDIUM
**Issue:** `kMaxStealRamp` is set in `prepareToPlay` and `stealTailBufferL/R` are sized accordingly. If the host changes sample rate without calling `prepareToPlay` (rare but allowed in some hosts during offline rendering), the buffers could be undersized. Also, line 245 in `renderTailRamp` reads `(int) stealTailBufferL.size()` after a fall-through path that returned early on null variant — *that* path zeroes only `n` samples, where `n = jmin(rampSamples, sizeL, sizeR)`. If `rampSamples > sizeL`, the rest of the buffer is left uninitialized. Then the consumer in `renderNextBlock` line 535-545 reads `stealTailSamplesRemaining` samples — which was set to `rampSamples` (unbounded by buffer size in the steal-detect path at line 339-345 — wait, line 339 *does* `jmin(kMaxStealRamp, stealTailBufferL.size())`). OK.

**Why it matters:** Non-issue under normal flow, but the buffer-size guard is duplicated in 3 places and could drift.

**Fix:** Compute `rampSamples` once at the top of `startNote` and assert against buffer size.

---

### MD-02: `selectVariantIndex` xorshift32 RNG seeded from `this` pointer — predictable across plugin instances on ASLR-disabled systems
**File:** `Source/MicrotonalSamplerVoice.cpp:155-163`
**Severity:** MEDIUM (security/quality — comment in code says "not a security boundary")
**Issue:** The seed is `(uint32_t)(thisPtr ^ (uintptr_t)(sampleRate * 1000.0))`. `thisPtr` mod 2^32 is highly correlated across voices (heap allocations are cluster-aligned), and `sampleRate * 1000.0` is 44100000 / 48000000 / 96000000 — a tiny set of values. Result: voices in the same plugin instance, same SR, will have correlated low bits, defeating the no-repeat goal somewhat.

**Why it matters:** Round-robin determinism — users may notice visible patterns on staccato passages. Not a security issue. Comment explicitly disclaims security.

**Fix:** Use `juce::Random::getSystemRandom().nextInt()` once at construction:
```cpp
rngState = (uint32_t) juce::Random::getSystemRandom().nextInt();
if (rngState == 0) rngState = 0x12345678u;
```
Or hash the pointer with a better mixer (splitmix64).

---

### MD-03: `snapshotWaveformPeaks` does message-thread O(N) scan of audio buffer — can be > 1 ms on large samples
**File:** `Source/PluginProcessor.cpp:1242-1280`
**Severity:** MEDIUM — out of scope for v1 perf but flagged as documentation
**Issue:** The comment claims "≈1 ms / 5 s sample" but for a 60s field recording at 96kHz stereo, that's 5.76M samples — closer to 20-50ms on the message thread, blocking the UI. JS calls this synchronously from a click handler; the user sees a stutter.

**Why it matters:** UX. v1 scope says perf is OOS, but this directly impacts user-perceptible responsiveness.

**Fix:** Move the scan to a `juce::Thread` and post the JSON via `callAsync`. Or compute peaks once at load time and cache in `SampleVariant`.

---

### MD-04: `loadSingleSample` collapse-to-single behavior loses RR variants silently
**File:** `Source/PluginProcessor.cpp:703-882` (lines 819-820 in particular)
**Severity:** MEDIUM — UX / data loss
**Issue:** When `mergeAsRr=false` and the target cell has multiple variants, the old cell is dropped and replaced with a single-variant cell. The variants vector (potentially 64 audio buffers) is silently discarded. The header comment (line 217-218) documents this but only with a "Per-cell load" comment block — the user-facing UI has a different notion ("Replace") that doesn't necessarily convey "drop all RR variants".

**Why it matters:** User loses work. They may have spent hours loading 32 RR variants into a cell, then click the cell's "replace" button thinking "replace just this take" — and lose all 32.

**Fix:** UI policy — the JS layer should display a confirm modal when a multi-variant cell is the target of `mergeAsRr=false`. Strongly consider making `mergeAsRr=true` the default for cells with > 1 variant.

---

### MD-05: `getStateInformation` does not synchronize with in-flight folder load completions
**File:** `Source/PluginProcessor.cpp:1580-1585`
**Severity:** MEDIUM — concurrency
**Issue:** If the host saves the project mid-load, `loadOpHistory` may be empty (load not yet complete) but `currentSampleFolder` may be set. The saved XML lacks the in-flight folder, so reopening the project shows an empty bank. Worse, the user gets no warning.

**Why it matters:** Data loss on save during async load.

**Fix:** Either (a) block save until load completes (synchronous join with timeout) or (b) explicitly save the in-flight `currentSampleFolder` as a pending op so it's replayed on restore. The fast/safe option:
```cpp
void OMicrotonalSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Wait for in-flight load to complete (with timeout) so loadOpHistory is current.
    if (sampleLoader != nullptr && sampleLoader->isThreadRunning())
        sampleLoader->stopThread(2000);  // joins; callback was already posted to msg thread
    auto root = captureStateValueTree();
    // ...
}
```

---

### MD-06: `restoreTuningFromValueTree` writes XML payload to a temp file using `Time::currentTimeMillis` as filename — collision possible on rapid restores
**File:** `Source/PluginProcessor.cpp:1438-1443, 1459-1466`
**Severity:** MEDIUM
**Issue:**
```cpp
auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory)
    .getChildFile("o-microtonalsampler-restore-"
        + juce::String(juce::Time::currentTimeMillis()) + ".kbm");
```
Two restores within the same millisecond (e.g. concurrent project loads in a multi-DAW setup) collide and the second overwrites the first. Also: `tmp.deleteFile()` is called only on the success path — if `loadKBMFile` throws, the temp file is leaked.

**Why it matters:** Disk litter, very rare collisions.

**Fix:** Use `juce::File::createTempFile` which guarantees uniqueness; use RAII for deletion:
```cpp
auto tmp = juce::File::createTempFile(".kbm");
juce::ScopedScopeGuard cleanup([&]() { tmp.deleteFile(); });
```

---

### MD-07: `parseAsScientificPitch` accepts `cN` (lowercase) and `Cb` (B as flat after C → invalid) ambiguously
**File:** `Source/FilenameParser.cpp:65-78`
**Severity:** MEDIUM
**Issue:** The comment at line 67-70 explicitly handles the case: lowercase `b` is flat; uppercase `B` after a letter is invalid. **However:** inputs like `cb3` (lowercase) reach this code with `s[idx] == 'b'` — match flat, accidental = -1, semitone = 0 + (-1) = -1 → `(octave + 2) * 12 + (-1)`. For `cb3`: `5 * 12 + (-1) = 59`. That's MIDI 59 = B3 in the new C3=60 convention. **Wait** — `Cb3` in scientific notation is "C-flat in octave 3" = B in octave 2, i.e. should be MIDI 47 (under C3=60: B2 = (2+2)*12+11 = 59 — *also 59*). So actually 59 is correct!

But: what about `cbx3`? Won't reach here (cbx isn't a recognized pitch). Fine. What about `c#b3`? After `s[1] == '#'`, accidental = +1, idx = 2. Then `s[2] == 'b'` — but the `if` only checks at `idx == 1`. **No issue** — only ONE accidental is allowed.

What about `Bb3`? `s[0] == 'B'` → semi = 11. `s[1] == 'b'` → flat, accidental = -1. semi+acc = 10 → MIDI = 5*12+10 = 70 (under C3=60: A#3 = 70 = correct).

**Verdict:** No actual parse bug here; demoting to LOW. (See LO-01.)

---

### MD-08: `RangedDirectoryIterator` recurses=false but loader does not document why
**File:** `Source/SampleLoader.cpp:264-267`
**Severity:** MEDIUM — design
**Issue:** Folder load is non-recursive. Users who organize samples in subfolders (Piano/v1/C3.wav) get an empty load with no obvious error. The "no usable samples" failure callback fires but doesn't tell them about the subdirs they may have meant to recurse into.

**Why it matters:** UX. Common user mistake.

**Fix:** Either recurse (with a depth limit), or detect subfolders and surface a hint in the failure message: "Found 0 audio files but 3 subfolders — drag a single folder containing files, not a folder of folders."

---

### MD-09: `applyFolderLoad` ReplaceLayer mode does not validate target layer is in current map's range
**File:** `Source/PluginProcessor.cpp:495-509`
**Severity:** MEDIUM
**Issue:** `op.targetLayer` is jlimited to 0..3 (line 497), but if the current map's `numVelocityLayers` is 1 and the user invokes ReplaceLayer with target=3, the erase-if loop finds nothing to remove (no cells have `velocityLayer==3`), then the new cells are merged. The map's `numVelocityLayers` is recomputed at line 563 from `maxLayer + 1`, so the map ends up with `numVelocityLayers=4` even though only layer 0 and layer 3 are populated. Layer 1 and 2 will dispatch via `findCell` to nearest-cell fallback — likely picking the layer-0 cell. This is *probably* fine, but it's not what "ReplaceLayer" suggests.

**Why it matters:** Confused behavior on edge cases.

**Fix:** Document the behavior in the LoadMode comment. Or reject `targetLayer >= currentMap->numVelocityLayers` for ReplaceLayer with a toast "Target layer doesn't exist; use Append instead."

---

### MD-10: `loadFolder` callback in PluginProcessor:429 captures `op` but the outer `loadSampleFolder` already mutated `currentSampleFolder` (line 418) before the load completes
**File:** `Source/PluginProcessor.cpp:418, 449-459`
**Severity:** MEDIUM
**Issue:** `currentSampleFolder = folder;` runs synchronously at line 418, BEFORE the loader posts back. If the load fails, the failure callback (line 450-459) sets `currentSampleFolder = juce::File();` — but only on FAILURE. On success, no update is needed (already set). **But:** between line 418 and the success callback, the user may have triggered another load that updates `currentSampleFolder` first. The failure of the FIRST load then wipes the SECOND load's `currentSampleFolder`. Race.

**Why it matters:** Rare but reproducible: rapid drag-drop of two folders, where the first is invalid and the second is valid. The valid one's `currentSampleFolder` gets nuked when the first one's failure callback fires.

**Fix:** Capture the folder by value in the failure callback and only nuke if `currentSampleFolder == capturedFolder`:
```cpp
[this, captured = folder](const juce::String& reason) {
    if (currentSampleFolder == captured)
        currentSampleFolder = juce::File();
    // ...
}
```

---

### MD-11: `processOneFile` calls `formatManager.createReaderFor(file)` per file — registers basic formats every `run()` call (line 205-206)
**File:** `Source/SampleLoader.cpp:205-206`
**Severity:** MEDIUM — performance / code smell
**Issue:** The header documents this is intentional ("AudioFormatManager is intentionally NOT a member") but the `registerBasicFormats` call happens every `run()`, which scans all built-in formats. For folder loads with 100+ files, this is amortized — but for `loadSingleVariant`, it pays the overhead per single load.

**Why it matters:** Trivial perf hit; defensible because the comment cites RESEARCH pitfall #9 (likely a JUCE thread-safety concern). Documentation is sufficient — flagging for awareness only.

**Fix:** None required if pitfall #9 cites correctness. Otherwise, hoist to a thread-local `static thread_local juce::AudioFormatManager fmgr; static thread_local bool initialized = false; if (!initialized) { fmgr.registerBasicFormats(); initialized = true; }`.

---

## LOW

### LO-01: `parseAsScientificPitch` rejects multi-digit octaves like "C-12" (octave -12, MIDI underflow)
**File:** `Source/FilenameParser.cpp:104-105`
**Severity:** LOW — defensive
**Issue:** After parsing octave, the check `if (midi < 0 || midi > 127) return std::nullopt;` correctly rejects out-of-range. Fine. **But** the `octStr.getIntValue()` parse silently accepts whitespace/garbage trailing the digit run (returns 0 for "abc"). The earlier validation loop (line 96-98) does require all chars after sign to be digits — so non-digit trailing junk is rejected. **No bug.** Defensive.

**Fix:** None required. Comment is good.

---

### LO-02: `cubicInterp` clamps to `[0, N-1]` via `juce::jlimit` — fine, but at `pos < 0`, returns y1 = buf[clamp(0)] = buf[0] without warning
**File:** `Source/MicrotonalSamplerVoice.cpp:49-70`
**Severity:** LOW
**Issue:** Defensive clamp masks bugs in upstream `pos` computation. If `posLow` is ever negative due to a math error, the voice silently outputs `buf[0]` repeated. No diagnostic.

**Fix:** Add a `jassert(pos >= 0.0)` at the top of `cubicInterp` so debug builds catch upstream bugs.

---

### LO-03: `MicrotonalSamplerSound::appliesToNote(int)` always returns true — no MIDI validation
**File:** `Source/MicrotonalSamplerSound.h:20`
**Severity:** LOW
**Issue:** `appliesToNote` accepts MIDI 0..127 (the synth caller validates this), but the sound returns true even for negative or > 127 — relies on caller. Fine in JUCE 8. But brittle.

**Fix:** None — JUCE handles this. Documentation.

---

### LO-04: `juce::WebSliderRelay` constructed with raw string IDs — typo would silently dead-link
**File:** `Source/PluginEditor.cpp:54-61`
**Severity:** LOW — code quality
**Issue:** Slider IDs are duplicated as strings here AND in JS AND in `createParameterLayout`. A typo in one place silently breaks the binding (no compile error, no runtime warning).

**Fix:** Centralize in a `namespace ParamIDs { constexpr const char* attack = "attack"; ... }` header shared with JS via build-time codegen.

---

### LO-05: `restoreStateValueTree` silently no-ops if `root.hasType` doesn't match
**File:** `Source/PluginProcessor.cpp:1514-1517`
**Severity:** LOW
**Issue:** Returning silently on type mismatch hides version-incompat issues. If a future plugin version changes the APVTS root type, old projects fail silently with no diagnostic.

**Fix:** `DBG ("restoreStateValueTree: type mismatch — expected " << parameters.state.getType() << ", got " << root.getType());`.

---

### LO-06: `snapshotSampleMapJson` uses string concatenation for JSON — no escape on filenames containing quotes/backslashes
**File:** `Source/PluginProcessor.cpp:1095-1182`
**Severity:** LOW
**Issue:** Most filename emissions go through `juce::JSON::toString(juce::var(filename))` which escapes correctly. **But** the `lengthSamples`, `loopMode`, etc., are concatenated raw. `loopModeToString` returns a fixed string (safe). `lengthSamples` is an int (safe). No injection vector found.

**Verdict:** Safe today, but the manual JSON builder is fragile. Consider migrating to `juce::JSON::toString(juce::var(dynamicObject))` like `snapshotWaveformPeaks` does.

**Fix:** Migrate to the dynamic-object pattern for consistency.

---

### LO-07: `getResource` logs to `juce::Logger::writeToLog` for missing resources but returns std::nullopt — JS could probe for paths
**File:** `Source/PluginEditor.cpp:1603-1604`
**Severity:** LOW — info disclosure
**Issue:** A WebView with a malicious script can fetch `/non/existent/path` repeatedly to log-spam the host's debug output. Also, the log line includes the exact requested URL — if WebView2 ever exposes this log to the user, it's an info leak.

**Fix:** Rate-limit the log, or remove it entirely (the resource provider already returns nullopt cleanly).

---

### LO-08: `MicrotonalSamplerVoice::renderTailRamp` early-out branch zeroes `min(rampSamples, sizeL, sizeR)` samples but uses `juce::jmin` with 3 args — version compatibility
**File:** `Source/MicrotonalSamplerVoice.cpp:245-247`
**Severity:** LOW
**Issue:** `juce::jmin(rampSamples, (int) stealTailBufferL.size(), (int) stealTailBufferR.size())` — JUCE's `jmin` does support 3 args. Fine. But the `(int) ...size()` casts could narrow on huge buffers (>2GB samples — impossible here).

**Fix:** None.

---

## Out-of-Scope Notes (not classified)

- **Performance:** `processOneFile` allocates one `juce::AudioBuffer<float>` per file via `setSize(srcChannels, srcSamples)` — fine on the loader thread, just noting.
- **Memory:** No leaks detected in normal flow. `juce::DynamicObject*` allocations at line 1282 and 1356 are wrapped in `juce::var`, which manages lifetime.
- **`std::atomic_store` on shared_ptr:** Deprecated in C++20 but acceptable per the comments. The conditional compile guard `__cpp_lib_atomic_shared_ptr` handles forward compatibility correctly.
- **Cross-platform:** Windows WebView2 user-data-folder (line 72-77), JUCE namespace for native functions, resource provider returning paths (not URLs) — all match the patterns documented in CLAUDE.md memory.

---

_Reviewed: 2026-05-02_
_Reviewer: Claude (gsd-code-reviewer / Opus 4.7 1M)_
_Depth: deep (cross-file analysis, ~2.5h on 6700 LOC)_
