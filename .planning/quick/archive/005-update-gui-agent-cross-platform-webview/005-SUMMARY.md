---
phase: quick-005
plan: 01
subsystem: agents
tags: [gui-agent, webview, cross-platform, windows, webview2]
dependency-graph:
  requires: []
  provides:
    - Cross-platform WebView implementation guidance in gui-agent
    - Windows WebView2 CMake flags in Stage 3 template
    - unique_ptr member pattern for all WebView components
  affects:
    - All future Stage 3 GUI implementations
    - Existing plugins needing WebView2 remediation
tech-stack:
  added: []
  patterns:
    - std::unique_ptr for all WebView members (relays, webView, attachments)
    - Constructor-body initialization for cross-platform #if blocks
    - withKeepPageLoadedWhenBrowserIsHidden for FL Studio compatibility
    - withUserDataFolder for Windows plugin host compatibility
key-files:
  created: []
  modified:
    - .claude/agents/gui-agent.md
decisions:
  - id: Q005-01
    decision: All WebView members use std::unique_ptr with constructor-body initialization
    rationale: Enables #if JUCE_WINDOWS blocks for platform-specific options that cannot be expressed in initializer lists
  - id: Q005-02
    decision: 4 new critical patterns added (#4-7) for cross-platform awareness
    rationale: Codifies silent IE fallback, URL scheme differences, evaluateJavascript asymmetry, and Options builder gotcha
metrics:
  duration: 3m 4s
  completed: 2026-02-06
---

# Quick Task 005: Update gui-agent with Cross-Platform WebView Best Practices

**One-liner:** gui-agent updated with Windows WebView2 CMake flags, unique_ptr member pattern, and 4 new cross-platform critical patterns from research doc

## What Changed

The gui-agent (`.claude/agents/gui-agent.md`) was updated to incorporate cross-platform WebView best practices from `research/cross-platform-webview-best-practices.md`. Previously the agent only targeted macOS WebView configuration, causing Windows builds to silently show blank white rectangles (34 of 35 plugins affected).

## Task Commits

| Task | Name | Commit | Key Changes |
|------|------|--------|-------------|
| 1 | Update CMake, WebView construction, and member patterns | 8aa1636 | gui-agent.md (+163/-59 lines) |

## Changes Applied

### 1. CMake Template (Section 9)
- Added `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin()` call
- Added `NEEDS_WEBVIEW2 TRUE` in `juce_add_plugin()` call
- Added `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` to compile definitions
- Updated key points with explanations for each flag

### 2. WebView Constructor (Section 7)
- Changed from initializer-list construction to constructor-body initialization
- Added `withKeepPageLoadedWhenBrowserIsHidden()` to prevent FL Studio blank page
- Added `#if JUCE_WINDOWS` block with `withUserDataFolder()`, `withStatusBarDisabled()`, `withBuiltInErrorPageDisabled()`
- WebView created via `std::make_unique<juce::WebBrowserComponent>(options)`

### 3. Member Declarations (Section 6)
- All relay members changed to `std::unique_ptr<juce::WebSliderRelay>` etc.
- WebView member changed to `std::unique_ptr<juce::WebBrowserComponent>`
- All attachment members changed to `std::unique_ptr<juce::WebSliderParameterAttachment>` etc.
- Pseudo-code generators updated to output unique_ptr patterns

### 4. Critical Patterns (4 new)
- Pattern #4: Silent IE fallback on Windows
- Pattern #5: URL scheme differences (do NOT hard-code)
- Pattern #6: evaluateJavascript() error handling asymmetry
- Pattern #7: Options builder returns new objects

### 5. Required Reading
- Added reference to `research/cross-platform-webview-best-practices.md`

### 6. Troubleshooting (3 new issues)
- Issue 6: Blank WebView on Windows
- Issue 7: WebView2 permission denied in plugin host (Windows)
- Issue 8: Page goes blank when plugin window hidden (FL Studio)

### 7. Self-Validation Checklist (6 new checks)
- NEEDS_WEBVIEW2 TRUE in juce_add_plugin()
- JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
- withKeepPageLoadedWhenBrowserIsHidden()
- Windows-specific withUserDataFolder() block
- All WebView members use std::unique_ptr
- No hard-coded URL schemes

### 8. Thread Safety Section
- Updated example to use `std::unique_ptr` for all WebView members

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Updated resized() and thread_safety_patterns sections for unique_ptr consistency**
- **Found during:** Task 1
- **Issue:** The `resized()` code used `webView.setBounds()` (direct member access) and the thread safety example used direct member declarations, both inconsistent with the new unique_ptr pattern
- **Fix:** Changed to `webView->setBounds()` and updated thread safety example to use `std::unique_ptr` declarations
- **Files modified:** .claude/agents/gui-agent.md
- **Commit:** 8aa1636

## Verification Results

All 10 verification greps returned >= 1 match:
- NEEDS_WEBVIEW2 TRUE: 6
- JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING: 6
- withUserDataFolder: 5
- withKeepPageLoadedWhenBrowserIsHidden: 4
- cross-platform-webview-best-practices: 1
- Silent IE fallback: 1
- std::unique_ptr<juce::WebSliderRelay>: 5
- evaluateJavascript: 4
- withStatusBarDisabled: 2
- withBuiltInErrorPageDisabled: 2

Structural integrity checks all pass (JSON report format, preconditions, contracts, state management, workflow routing, thread safety patterns).

## Self-Check: PASSED
