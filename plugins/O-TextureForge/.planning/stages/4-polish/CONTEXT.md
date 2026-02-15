# Stage 4: Integration & Polish - Context

## Discussion Summary

**Date:** 2026-02-14
**Participants:** User, Claude

## Requirements Confirmed

### In Scope
- **Pluginval at strictness 10** — full validation, fix any failures
- **Comprehensive edge case handling** — full roadmap scope plus additional robustness
- **Performance profiling** — fix obvious bottlenecks (no strict numeric targets)
- **Windows config audit** — verify CMakeLists.txt flags without cross-compile
- **macOS testing** — AU validation + VST3 in DAW

### Out of Scope (deferred)
- **Preset system** — skipped for v1. Users load their own corpus files.
- **User manual / documentation** — skipped for now. Write docs closer to release.
- **Windows build/test** — verified via CI/CD at publish time. Config audit only.

## Constraints Identified

- macOS-only build environment (no Windows cross-compile)
- No factory corpus files to bundle (preset system deferred)
- Plugin already passing AU validation (aumu OuTF OuDv) since Stage 1
- Windows WebView2 config already set in Stage 1 CMakeLists (NEEDS_WEBVIEW2 TRUE + static linking + user data folder)
- Corpus state persistence already implemented in Stage 3

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Presets | Skip for v1 | Bundling corpus files adds binary size; users bring own audio |
| Edge cases | Full scope | Robustness matters for a 5.0 complexity plugin — file missing on restore, memory pressure, WebView2 check, large file warning, UMAP cancel, sample rate change |
| Pluginval | Strictness 10 | Gold standard for release quality |
| Performance | Fix bottlenecks | Pragmatic — fix what causes audible glitches or UI stutter |
| Windows | Config audit only | Can't build on macOS; verify flags are correct for CI/CD |
| Docs | Skip | Not needed until closer to release |

## Edge Cases to Handle (Full Scope)

1. **No file loaded** — show "Drop audio file here" message in WebView scatter area
2. **Invalid file format** — error toast/message in WebView (unsupported format)
3. **Large corpus (>100MB)** — warning message but still load (no hard block)
4. **UMAP cancel** — button to stop UMAP computation, keep PCA layout (verify Stage 3 cancel works end-to-end)
5. **Corpus file missing on restore** — graceful fallback when saved corpus path no longer exists (clear state, show drop zone)
6. **Sample rate change** — handle prepareToPlay with new sample rate (re-segment if needed, or document limitation)
7. **Multiple instances** — verify 2+ instances don't interfere (separate corpus, separate KD-tree)
8. **WebView2 availability (Windows)** — detect missing WebView2 runtime, show fallback message (config audit only for now)
9. **Memory pressure** — detect/handle very large files that could exhaust memory

## Performance Profiling Targets

No strict numeric targets. Identify and fix:
- processBlock CPU spikes that cause audible glitches
- WebView update lag that causes visible stutter
- Memory leaks during sustained playback
- UMAP computation blocking UI thread

## Pluginval Validation

- Run at strictness level 10
- Fix all failures
- Both VST3 and AU formats
- Document any known limitations

## Open Questions

- Sample rate change handling: re-segment corpus vs. document as limitation? (Research phase should investigate JUCE best practices)
- Memory pressure: what's the practical upper bound for corpus size before degradation?

## Next Phase

Ready for: research phase
