# O-MicrotonalSampler v1.11.1 — Code Review Summary

**Reviewers:** 4 parallel agents (gsd-code-reviewer, dsp-explorer, frontend-explorer, architecture-explorer)
**Scope:** ~6.4K lines C++ + ~6.4K lines web resources
**Date:** 2026-05-02

Detailed reports:
- [REVIEW-cpp-bugs.md](REVIEW-cpp-bugs.md) — 33 findings (5 CRITICAL, 9 HIGH, 11 MEDIUM, 8 LOW)
- [REVIEW-dsp.md](REVIEW-dsp.md) — DSP correctness + audio-thread audit
- [REVIEW-frontend.md](REVIEW-frontend.md) — WebView/JS/CSS audit
- [REVIEW-architecture.md](REVIEW-architecture.md) — God-class + module extraction analysis

---

## Top-priority fixes (ship-blockers)

### Security
| # | Finding | Location | Fix |
|---|---------|----------|-----|
| 1 | **Path traversal in `dropSessionAddFile`** — JS-supplied `relPath` passed straight to `getChildFile` → `replaceWithData` with no `..` / absolute / symlink validation. WebView can write to `~/Library/LaunchAgents/`, `~/.ssh/authorized_keys`, etc. | PluginEditor.cpp:399-452 | Validate `relPath` is relative, reject `..`, resolve final path is within session temp dir, no symlinks |
| 2 | **No size cap on base64 streaming** — trivially OOMs DAW with malicious page or huge folder | PluginEditor.cpp:399-452 | Per-file 256MB cap, per-session 4GB cap |

### Audio-thread / crash bugs
| # | Finding | Location | Fix |
|---|---------|----------|-----|
| 3 | **`setValueNotifyingHost` called from processBlock per CC11 byte** — RT-thread violation; can stall audio or deadlock hosts | PluginProcessor.cpp:313-321 | Stage to atomic, drain via AsyncUpdater on message thread |
| 4 | **Use-after-free on `cellLow`/`variantLow` raw pointers** when `currentMap` is swapped during `startNote`. Steal-tail logic runs before new map snapshot — pointers from prior map can dangle | MicrotonalSamplerVoice.cpp:336-374 | Capture old map locally for `renderTailRamp` duration |
| 5 | **`renderTailRamp` early-return logic is inverted** — returns and zeros buffer in the *normal* case, not the error case. **All voice-steal tails are silently silenced — clicks/silence on every steal** | MicrotonalSamplerVoice.cpp:240-254 | Flip the guard condition |
| 6 | **APVTS `getRawParameterValue("attack")->load()` not null-checked** — typo or missing param crashes audio thread on note-on | MicrotonalSamplerVoice.cpp:486-489 | Cache atomic pointers in prepareToPlay, assert non-null |
| 7 | **Ramp coefficient division underflow** — `(float)i / (float)rampSamples` produces NaN/Inf when rampSamples is 1-2 | MicrotonalSamplerVoice.cpp:293 | Guard with `if (rampSamples < 2)` skip-ramp branch |

### Frontend silent-failure bugs
| # | Finding | Location | Fix |
|---|---------|----------|-----|
| 8 | **`readFileEntryAsBase64` not wrapped in try/catch in the streaming loop** — FileReader rejection silently corrupts partial folder uploads | sampler-app.js:1346-1356 | Per-iteration try/catch; skip file + warn on error |
| 9 | **Modal promise chains have no rejection handlers** — `showFolderLoadOptionsModal`, `streamFolderEntryToCpp`. Backend timeout hangs the UI permanently | sampler-app.js:1450-1543, 1320-1362 | Wrap native-fn awaits, resolve dialogs to null, surface toast |
| 10 | **`pendingClickTimer` race on grid rebuild** — timer fires against stale cell DOM after `renderGrid` runs while FileChooser is open | sampler-app.js:779-815 | Clear pending timer in renderGrid, or capture MIDI/layer in closure |

---

## High-priority (next sprint)

| Finding | Location | Why |
|---------|----------|-----|
| `confirmRoundRobinLoad` chain can re-enter corrupted replay queue | PluginProcessor.cpp | Cascading load failures |
| `parseAsRrIndex` silently loses RR semantics for separator-tokenized names (`Piano_C3_take_1.wav` → rr=−1) | FilenameParser.cpp | Round-robin disabled for valid filenames |
| `selectVariantIndex` clips to 254 to preserve 0xFF sentinel; brittle coupling to `kMaxVariantsPerCell=64` | MicrotonalSamplerVoice.cpp | Add `static_assert(kMaxVariantsPerCell < 255)` |
| Folder-load callbacks capture `this` raw — project close mid-load can crash via post-destructor message-thread callback | PluginProcessor.cpp | Use `juce::WeakReference<>` |
| `loadOpHistory`/`lastSkippedFiles` not synchronized — Reaper calls `getStateInformation` off message thread → corrupted XML | PluginProcessor.cpp | Add message-thread lock or atomic snapshot |

---

## Architecture wins (high ROI, ranked)

| # | Action | Effort | Impact |
|---|--------|--------|--------|
| 1 | **Data-driven native function registry** — replace 42 inline `addNativeFunction(...)` calls (~590 lines) with a config-driven loop in PluginEditor constructor | M | Shrinks editor by ~200 lines; sets pattern for O-Bells, O-Lyrica peers |
| 2 | **Extract drag-drop streaming → shared module** — same WKWebView workaround duplicates across 3+ plugins (PluginEditor.cpp dropSession* + sampler-app.js streaming helpers) | L | Unifies the v1.0.4 pattern repo-wide |
| 3 | **Remove dead `tuning-panel-readonly.css`** — embedded but unused since v1.2.0 | S | Cleanup |

---

## Verified-correct (no action — these are NOT regressions)

- ✅ Base64 decode uses `juce::Base64::convertFromBase64` (NOT the JUCE-proprietary `MemoryBlock::fromBase64Encoding`)
- ✅ `TuningPanel` constructor correctly receives the `Juce` ES module namespace (NOT `window.__JUCE__`)
- ✅ WebView2 `withUserDataFolder` set on Windows
- ✅ Resource provider receives bare paths and uses direct equality (no `fromFirstOccurrenceOf("://")` bug)
- ✅ `setLatencySamples` used in prepareToPlay (no non-virtual `getLatencySamples()` override)
- ✅ `getBackendResourceAddress()` handles cross-platform URL scheme correctly
- ✅ Cubic Hermite interpolation, equal-power panning, loop math sound
- ✅ Voice render path otherwise lock-free, atomics + shared_ptr snapshot correct

---

## Recommended next move

**Option A (security + crashes only):** Bundle ship-blockers #1-#10 into a v1.11.2 PATCH. Estimated: 1-2 days.

**Option B (full sweep):** v1.12.0 MINOR with ship-blockers + HIGH-priority fixes + architecture-win #1 (native-fn registry). Estimated: 1 week.

**Option C (refactor cycle):** Run `/improve O-MicrotonalSampler` for a focused multi-cycle improvement: (1) PATCH for security/crash bugs, (2) MINOR for HIGH-priority + architecture, (3) shared-module extraction PR (architecture #2) coordinated across O-Bells / O-Lyrica.
