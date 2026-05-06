---
phase: quick-005
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .claude/agents/gui-agent.md
autonomous: true

must_haves:
  truths:
    - "gui-agent CMake section instructs NEEDS_WEBVIEW2 TRUE and JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1"
    - "gui-agent WebView construction includes Windows withUserDataFolder and withKeepPageLoadedWhenBrowserIsHidden"
    - "gui-agent critical patterns section warns about silent IE fallback, URL scheme differences, and evaluateJavascript error asymmetry"
    - "gui-agent standardizes on unique_ptr for all WebView members (relays, webView, attachments)"
    - "gui-agent references research/cross-platform-webview-best-practices.md"
  artifacts:
    - path: ".claude/agents/gui-agent.md"
      provides: "Cross-platform WebView implementation guidance for Stage 3"
      contains: "NEEDS_WEBVIEW2 TRUE"
  key_links:
    - from: ".claude/agents/gui-agent.md"
      to: "research/cross-platform-webview-best-practices.md"
      via: "reference in required reading section"
      pattern: "cross-platform-webview-best-practices"
---

<objective>
Update the gui-agent (.claude/agents/gui-agent.md) to incorporate cross-platform WebView best practices from the research document so that Stage 3 implementations produce plugins that build and run correctly on both macOS and Windows.

Purpose: The gui-agent currently only targets macOS WebView configuration. Without Windows-specific CMake flags (NEEDS_WEBVIEW2, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING) and runtime options (withUserDataFolder, withKeepPageLoadedWhenBrowserIsHidden), plugins show a blank white rectangle on Windows. 34 of 35 plugins are affected.

Output: Updated gui-agent.md with complete cross-platform WebView guidance.
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.claude/agents/gui-agent.md
@research/cross-platform-webview-best-practices.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Update CMake, WebView construction, and member patterns for cross-platform support</name>
  <files>.claude/agents/gui-agent.md</files>
  <action>
Update the gui-agent with the following targeted changes. Read the full file first, then apply all changes in a single write.

**1. Section 9 (Update CMakeLists.txt for WebView, ~line 670-704):**

Replace the CMake template to include the complete cross-platform configuration:
- Add `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin()` call (explicit, for Linux webkit2gtk)
- Add `NEEDS_WEBVIEW2 TRUE` in `juce_add_plugin()` call (for Windows WebView2LoaderStatic.lib)
- Add `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` to `target_compile_definitions()`
- Keep existing `JUCE_WEB_BROWSER=1` and `JUCE_USE_CURL=0`

Update the "Key points" bullet list to explain what each flag does:
- `NEEDS_WEB_BROWSER TRUE` -- links webkit2gtk on Linux
- `NEEDS_WEBVIEW2 TRUE` -- links WebView2LoaderStatic.lib on Windows
- `JUCE_WEB_BROWSER=1` -- enables WebBrowserComponent class compilation
- `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` -- static linking for Windows (auto-defines JUCE_USE_WIN_WEBVIEW2=1), prevents silent blank WebView from missing DLL
- `JUCE_USE_CURL=0` -- not needed for local HTML serving

**2. Section 7 (PluginEditor.cpp WebView construction, ~line 529-603):**

Update the WebView Options initialization to include cross-platform options:
- Add `.withKeepPageLoadedWhenBrowserIsHidden()` to the options chain (prevents about:blank in FL Studio)
- Add a `#if JUCE_WINDOWS` block after the options chain that applies Windows-specific options:
  ```cpp
  #if JUCE_WINDOWS
  options = options.withWinWebView2Options(
      juce::WebBrowserComponent::Options::WinWebView2{}
          .withUserDataFolder(
              juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("[PluginName]_WebView"))
          .withStatusBarDisabled()
          .withBuiltInErrorPageDisabled()
  );
  #endif
  ```
- Note: This requires changing the options from inline construction to a variable pattern. Update the constructor accordingly to build options as a local variable, then pass to webView construction.

**3. Section 6 (PluginEditor.h, ~line 453-497) and Section 7 constructor:**

Standardize on `std::unique_ptr` for ALL WebView members (relays, webView, attachments). The research document recommends this pattern to prevent initialization order issues.

In PluginEditor.h, change member declarations from direct members:
```cpp
juce::WebSliderRelay gainRelay;
```
to unique_ptr:
```cpp
std::unique_ptr<juce::WebSliderRelay> gainRelay;
```

Do this for ALL relay members, the WebBrowserComponent member, and ALL attachment members.

In PluginEditor.cpp constructor, change from initializer-list construction to body construction:
```cpp
// 1. Create relays FIRST
gainRelay = std::make_unique<juce::WebSliderRelay>("gain");

// 2. Build options and create WebView
auto options = juce::WebBrowserComponent::Options{}
    .withNativeIntegrationEnabled()
    .withResourceProvider([this](const auto& url) { return getResource(url); })
    .withKeepPageLoadedWhenBrowserIsHidden()
    .withOptionsFrom(*gainRelay)
    ...;

#if JUCE_WINDOWS
options = options.withWinWebView2Options(...);
#endif

webView = std::make_unique<juce::WebBrowserComponent>(options);

// 3. Create attachments LAST
gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("gain"), *gainRelay, nullptr);
```

Update the pseudo-code generators (relay/attachment generation) to output unique_ptr patterns.

**4. Critical Patterns section (~line 509-527):**

Add new patterns after the existing Pattern #3:

**Pattern #4: Silent IE fallback on Windows:**
- Without `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, Windows silently falls back to IE backend
- IE backend does NOT support resource providers -- plugin shows blank white rectangle
- No error or warning is shown

**Pattern #5: URL scheme differences (do NOT hard-code):**
- macOS/iOS/Linux: `juce://juce.backend/` (custom URL scheme)
- Windows/Android: `https://juce.backend/` (intercepted by WebView2)
- Always use `getResourceProviderRoot()` in C++ and `getBackendResourceAddress()` in JavaScript
- Never hard-code `juce://` or `https://juce.backend/`

**Pattern #6: evaluateJavascript() error handling asymmetry:**
- macOS/Linux: returns detailed errors (type, message, source URL, line/column)
- Windows/Android: errors are indistinguishable from success returning null
- Mitigation: use defensive JavaScript with try/catch and console logging

**Pattern #7: Options builder returns new objects:**
- `Options` builder methods return NEW objects, do NOT modify in-place
- Must chain calls or capture return value
- Wrong: `options.withBackend(...)` (discarded silently)
- Correct: `options = options.withBackend(...)` or chain in constructor

**5. Required Reading section (~line 128-143):**

Add a new bullet referencing the research document:
```
8. **Cross-platform WebView best practices:** `research/cross-platform-webview-best-practices.md` - CRITICAL for Windows compatibility
```

**6. Troubleshooting section (~line 935-989):**

Add new issues:

**Issue 6: Blank WebView on Windows:**
- Symptom: Plugin builds and loads but shows blank white rectangle on Windows
- Cause: Missing `NEEDS_WEBVIEW2 TRUE` and/or `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- Resolution: Add both flags to CMakeLists.txt
- Note: Silent -- no errors or warnings

**Issue 7: WebView2 permission denied in plugin host (Windows):**
- Symptom: WebView fails to create in certain DAW hosts on Windows
- Cause: Default user data folder location not writable in plugin context
- Resolution: Use `withUserDataFolder()` with temp directory path

**Issue 8: Page goes blank when plugin window hidden (FL Studio):**
- Symptom: WebView navigates to about:blank when editor is hidden/shown
- Cause: Missing `withKeepPageLoadedWhenBrowserIsHidden()` option
- Resolution: Add the option to WebView construction

**7. Self-Validation checklist (Section 11, ~line 727-777):**

Add new automated check items:
- [ ] CMakeLists.txt includes `NEEDS_WEBVIEW2 TRUE` in juce_add_plugin()
- [ ] CMakeLists.txt defines `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- [ ] WebView constructor includes `withKeepPageLoadedWhenBrowserIsHidden()`
- [ ] WebView constructor includes Windows-specific `withUserDataFolder()` block
- [ ] All WebView members use `std::unique_ptr` (not direct members)
- [ ] No hard-coded URL schemes (`juce://` or `https://juce.backend/`)
  </action>
  <verify>
Verify all changes were applied by checking for key strings in the updated file:

```bash
# All of these should return matches:
grep -c "NEEDS_WEBVIEW2 TRUE" .claude/agents/gui-agent.md
grep -c "JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING" .claude/agents/gui-agent.md
grep -c "withUserDataFolder" .claude/agents/gui-agent.md
grep -c "withKeepPageLoadedWhenBrowserIsHidden" .claude/agents/gui-agent.md
grep -c "cross-platform-webview-best-practices" .claude/agents/gui-agent.md
grep -c "Silent IE fallback" .claude/agents/gui-agent.md
grep -c "std::unique_ptr<juce::WebSliderRelay>" .claude/agents/gui-agent.md
grep -c "evaluateJavascript" .claude/agents/gui-agent.md
grep -c "withStatusBarDisabled" .claude/agents/gui-agent.md
grep -c "withBuiltInErrorPageDisabled" .claude/agents/gui-agent.md
```

Each grep should return >= 1 match. Zero on any means that change was missed.
  </verify>
  <done>
The gui-agent at .claude/agents/gui-agent.md contains:
1. Complete cross-platform CMake template with NEEDS_WEBVIEW2 TRUE and JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
2. WebView constructor pattern with withKeepPageLoadedWhenBrowserIsHidden and Windows-specific withUserDataFolder/withStatusBarDisabled/withBuiltInErrorPageDisabled
3. All WebView members declared as std::unique_ptr with constructor-body initialization
4. Critical patterns covering silent IE fallback, URL scheme differences, evaluateJavascript asymmetry, and Options builder gotcha
5. Reference to research/cross-platform-webview-best-practices.md in required reading
6. New troubleshooting entries for blank Windows WebView, permission denied, and FL Studio blank page
7. Updated self-validation checklist with cross-platform checks
  </done>
</task>

</tasks>

<verification>
After the update, confirm the gui-agent contains all cross-platform guidance:

1. CMake section has complete 4-flag template (NEEDS_WEB_BROWSER, NEEDS_WEBVIEW2, JUCE_WEB_BROWSER=1, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
2. WebView constructor uses unique_ptr pattern with body initialization
3. Windows-specific options block present with #if JUCE_WINDOWS guard
4. Critical patterns section has 7+ patterns (original 3 + 4 new cross-platform patterns)
5. Required reading references the research document
6. Troubleshooting has entries for Windows-specific failures
7. Validation checklist includes cross-platform checks
</verification>

<success_criteria>
- gui-agent.md updated with all cross-platform WebView best practices
- Any future Stage 3 execution using the gui-agent will produce CMakeLists.txt with NEEDS_WEBVIEW2 TRUE and JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
- Any future Stage 3 execution will produce PluginEditor code with Windows-specific WebView2 options
- All WebView member patterns use std::unique_ptr throughout
- No breaking changes to existing workflow structure (section numbering, JSON report format, preconditions, etc.)
</success_criteria>

<output>
After completion, create `.planning/quick/005-update-gui-agent-cross-platform-webview/005-SUMMARY.md`
</output>
