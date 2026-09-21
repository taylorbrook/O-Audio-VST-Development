---
title: "Repo Modernization Audit — JUCE, Build, Claude Tooling, Gates, Hygiene, Instruments"
created: 2026-09-21
domain: tooling
type: research
keywords:
  - juce
  - build-system
  - ci
  - claude-code
  - tooling
  - repo-hygiene
  - modernization
  - audit
---

# Repo Modernization Audit — 2026-09-21

## Executive Summary

| # | Action | Why now | Effort | Risk |
|---|--------|---------|--------|------|

## 1. JUCE Version & Toolchain

All upstream facts below were retrieved **2026-09-21** from
`https://github.com/juce-framework/JUCE` via `gh api` and `raw.githubusercontent.com`.

| Finding | Evidence | Recommendation | Effort | Risk |
|---------|----------|----------------|--------|------|
| The pin is two releases behind, and one of them is a new major. Pinned JUCE is 8.0.14 (published 2026-06-22); upstream shipped 8.0.15 on 2026-07-21 and then JUCE **9.0.0** the same day, with 9.0.2 on 2026-09-07. The framing "latest 8.x" is now a dead end — 8.0.15 is the terminal 8.x release. | `.github/juce-version.txt` = `8.0.14`; local tree `project(JUCE VERSION 8.0.14 ...)` at `/Users/taylorbrook/JUCE/CMakeLists.txt:35`; `gh api repos/juce-framework/JUCE/releases` → `9.0.2 2026-09-07`, `9.0.1 2026-08-10`, `9.0.0 2026-07-21`, `8.0.15 2026-07-21`, `8.0.14 2026-06-22` (retrieved 2026-09-21) | Treat 8.0.15 and 9.x as two separate decisions. Take 8.0.15 first as a low-risk patch-level move; hold 9.x until the override hazard below is gated. | S | Low |
| **The vendored-override copy is an unguarded clobber, and 8.0.15 already proves it bites.** CI does `cp -R vendored/JUCE-overrides/modules/. JUCE/modules/` over the freshly downloaded JUCE with no version check. Upstream changed one of the two override targets between 8.0.14 and 8.0.15 — a one-line `getPosition()` fix removing a `jmax(0, projectTimeSamples)` clamp. Bumping the pin today would silently revert that upstream fix in every CI binary. | `.github/workflows/build-and-release.yml:160` and `:525`; `.github/workflows/ci-tests.yml:128` and `:251`; `gh api repos/juce-framework/JUCE/compare/8.0.14...8.0.15` → `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` `additions=1 deletions=1`; `grep -rn "8\.0\.1[0-9]" vendored/` returns nothing, so no version is tied to the override | Before any bump, add a guard step that fails when the override's base JUCE version ≠ `.github/juce-version.txt`, and re-derive the two override files from the new tag rather than copying the old ones forward. | M | High |
| Two independent patch mechanisms exist for the same fork, and nothing proves they agree. Local dev applies a real patch; CI copies whole files. They happen to be in sync **today** — byte-identical modulo CRLF — but by coincidence, not by any gate. | `scripts/apply-juce-patches.sh` applies `scripts/juce-patches/note-expression-juce-8.0.14.patch`; `grep -n "apply-juce-patches" .github/workflows/*.yml` returns **nothing**; `diff <(tr -d '\r' < vendored/JUCE-overrides/.../juce_audio_plugin_client_VST3.cpp) /Users/taylorbrook/JUCE/.../juce_audio_plugin_client_VST3.cpp` → 0 diff lines (vendored copy is CRLF, local is LF) | Make one mechanism authoritative: generate `vendored/JUCE-overrides/` from the patch in a script, and add a CI step that regenerates and diffs so drift fails loudly. | M | Med |
| The Note Expression fork is still genuinely required — upstream has **not** absorbed it. But it has shrunk by an order of magnitude across the 8.0.9 → 8.0.14 re-derivation, so re-deriving it against 9.x is a much smaller job than the older patch suggests. | Master's `modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h` (HTTP 200, retrieved 2026-09-21) contains **0** occurrences of `NoteExpression`/`onVst3RawEvent`/`Vst3RawEvent`; `grep -c '^@@'` → `note-expression-juce-8.0.9.patch` 27 hunks / 1114 lines vs `note-expression-juce-8.0.14.patch` **2 hunks / 117 lines** | Keep the fork. Budget the 9.x re-derivation at roughly the 2-hunk scale, not the 27-hunk scale — both hunks are purely additive. | S | Low |
| **JUCE 9 deprecation exposure is effectively zero.** Every removed or changed symbol named in upstream `BREAKING_CHANGES.md` for 9.0.0–9.0.2 was grepped against `plugins/` and `modules/`; all return 0 files. The WebView-first UI architecture is why — nothing here touches `juce::Drawable` or the SVG/OpenGL/MIDI-selector surfaces that 9.x changed. | `grep -rIl` across `plugins modules CMakeLists.txt` → `createFromSVG` 0, `Drawable` 0, `getStrokeType` 0, `getDashLengths` 0, `setDashLengths` 0, `getMidiInputSelectorListBox` 0, `JUCE_INCLUDE_ZLIB_CODE` 0, `MultiTouch` 0, `OpenGL` 0 source files; symbols extracted from `https://raw.githubusercontent.com/juce-framework/JUCE/master/BREAKING_CHANGES.md` (retrieved 2026-09-21) | The 9.x source-compatibility risk is not the blocker people assume. The real blocker is the override mechanism in row 2, not the API surface. | S | Low |
| The one JUCE-9 surface this repo does touch is the WebView JS package relocation (9.0.1), and even that is comment-only. `modules/juce_gui_extra/native/javascript` moved to `modules/juce_gui_extra/native/typescript/webview-interop`. | `grep -rn "juce_gui_extra/native/javascript"` → 3 hits, all comments: `plugins/O-Octagon/CMakeLists.txt:80`, `plugins/O-Octagon/Source/ui/public/js/app.js:81`, `plugins/O-Contrabass/.planning/mockups/v1-CMakeLists.txt:73`; upstream `BREAKING_CHANGES.md` "Version 9.0.1" entry (retrieved 2026-09-21) | Fix the three stale comments whenever 9.x is taken. No code change required. | S | Low |
| **34 plugins each vendor a private copy of JUCE's WebView interop `index.js`, and the copies have drifted into 3 distinct versions.** JUCE 9.0.1 published this as `@juce-framework/webview` on npm specifically to end hand-copying. | `find plugins -path '*/Source/ui/public/js/juce' -not -path '*/backups/*' \| wc -l` → **34**; `shasum` of `index.js` → `2e45ed40` ×27, `fcb126fa` ×5 (O-Bass, O-IntonationPad, O-MultiBandCompressor, O-SimpleReverb, O-Tremolo), `fea0d398` ×2 (O-Chorus, O-TextureForge); `CHANGE_LIST.md` "Version 9.0.1 — Added a new TypeScript npm package for WebView integrarion" (retrieved 2026-09-21) | Reconcile the 3 variants to one before considering npm consumption. The drift is a live correctness question today, independent of any JUCE bump. | M | Med |
| A hardcoded absolute home directory is the non-Windows JUCE fallback in a public repo. Any contributor without `JUCE_DIR` set, on macOS or Linux, gets a configure failure pointing at a stranger's home directory. | `CMakeLists.txt:41` → `add_subdirectory(/Users/taylorbrook/JUCE JUCE)`; the same literal is the default in `scripts/apply-juce-patches.sh` (`JUCE_DIR="${JUCE_DIR:-/Users/taylorbrook/JUCE}"`) | Replace the fallback with a `find_package`/`FetchContent` path or a clear `message(FATAL_ERROR ...)` naming `JUCE_DIR`. Cheap, and it is the first thing an outside contributor hits. | S | Low |
| The bundled VST3 SDK is current — this is a non-finding worth recording so it is not re-audited. | `kVstVersionString "VST 3.8.0"` at `/Users/taylorbrook/JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK/pluginterfaces/vst/vsttypes.h:27` | No action. | S | Low |
| Toolchain spread between local and CI is wide, and the macOS deployment floor predates every compiler now in play. Local is macOS 26.6.2 / Xcode 26.3 / CMake 4.2.1 / Ninja 1.13.2; CI builds on `macos-14`. The 10.13 floor is set unconditionally. | `CMakeLists.txt:5` → `CMAKE_OSX_DEPLOYMENT_TARGET "10.13"`; `.github/workflows/build-and-release.yml:141` and `.github/workflows/ci-tests.yml:94` → `runs-on: macos-14`; local `cmake --version` 4.2.1, `ninja --version` 1.13.2, `xcodebuild -version` Xcode 26.3, `sw_vers` 26.6.2 | Raise the floor to something a current Xcode actually supports and move CI to a newer macOS image — but verify against real DAW-host minimums first, since this is the one row here that changes shipped binaries. | M | Med |
| No CLAP format is built, for any of the 44 plugins. All build the same three formats. | `grep -h FORMATS plugins/*/CMakeLists.txt` → `FORMATS VST3 AU Standalone` ×44; `grep -rli clap plugins/*/CMakeLists.txt CMakeLists.txt modules/` → no matches | Optional. `clap-juce-extensions` adds CLAP as a wrapper target without touching plugin source; worth it only if a target DAW (Bitwig, Reaper) matters commercially. | M | Low |

**Upgrade risk/benefit, stated plainly.** A bump to **8.0.15** buys a handful of Windows GUI-scaling fixes, an Oboe update, and the `getPosition()` clamp fix — and costs almost nothing, because `BREAKING_CHANGES.md` lists **no** entries between 8.0.13 and 9.0.0, so 8.0.14 → 8.0.15 is breaking-change-free. A bump to **9.x** buys a new SVG parser, variable fonts, a rewritten CoreAudio backend, and the npm WebView package — none of which this repo currently needs — and costs re-deriving the 2-hunk Note Expression patch plus re-validating 44 plugins. **The binding constraint in both cases is the same:** the unguarded `vendored/JUCE-overrides/` clobber (row 2). Fix that gate first and both bumps become ordinary; leave it and even the "free" 8.0.15 bump silently reverts an upstream audio-position fix.

## 2. Build Efficiency

| Finding | Evidence | Recommendation | Effort | Risk |
|---------|----------|----------------|--------|------|

## 3. Claude Workflow & Tooling

| Finding | Evidence | Recommendation | Effort | Risk |
|---------|----------|----------------|--------|------|

## 4. Testing & Quality Gates

| Finding | Evidence | Recommendation | Effort | Risk |
|---------|----------|----------------|--------|------|

## 5. Repo Hygiene

| Finding | Evidence | Recommendation | Effort | Risk |
|---------|----------|----------------|--------|------|

## 6. Instrument-Specific Best Practices

| Finding | Evidence | Recommendation | Effort | Risk |
|---------|----------|----------------|--------|------|

## Apply Later
