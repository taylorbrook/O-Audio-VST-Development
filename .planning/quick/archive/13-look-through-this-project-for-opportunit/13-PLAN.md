---
phase: quick-13
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .planning/quick/13-look-through-this-project-for-opportunit/13-REPORT.md
autonomous: true
requirements: [QUICK-13]

must_haves:
  truths:
    - "User receives a prioritized report of refactoring and simplification opportunities"
    - "Report is actionable with concrete scope estimates and risk levels"
    - "Report distinguishes quick wins from larger architectural efforts"
  artifacts:
    - path: ".planning/quick/13-look-through-this-project-for-opportunit/13-REPORT.md"
      provides: "Prioritized refactoring opportunity report"
      min_lines: 80
  key_links: []
---

<objective>
Produce a comprehensive, prioritized report of refactoring and simplification opportunities across the VST plugin codebase.

Purpose: Identify concrete areas where duplicated code, dead artifacts, and inconsistent patterns create maintenance burden, so the user can decide which refactors to pursue.
Output: A detailed report at `.planning/quick/13-look-through-this-project-for-opportunit/13-REPORT.md`
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@PLUGINS.md
@modules/registry.yaml
@CMakeLists.txt
</context>

<tasks>

<task type="auto">
  <name>Task 1: Audit codebase for duplication, dead code, and inconsistencies, then produce prioritized report</name>
  <files>.planning/quick/13-look-through-this-project-for-opportunit/13-REPORT.md</files>
  <action>
Conduct a systematic audit of the codebase focusing on these specific opportunity areas already identified during planning, PLUS any additional ones discovered during deeper analysis:

**HIGH PRIORITY -- Massive Duplication (~1,700+ lines)**

1. **Preset native function boilerplate duplicated across 14 plugins.** Every plugin with presets copy-pastes ~100-120 lines of identical `withNativeFunction("savePreset"/"loadPreset"/"getPresetList"/"getCurrentPreset"/"selectNextPreset"/"selectPreviousPreset"/"deletePreset"/"isFactoryPreset"/"savePresetWithDialog"/"loadPresetFromFile")` lambda code into their PluginEditor constructor. This is the single largest refactoring opportunity. The `preset-manager` module exists at `modules/persistence/preset-manager/` but only provides C++ and JS -- it does NOT eliminate the ~100 lines of native function registration boilerplate each Editor must hand-write. A helper function or macro could reduce each plugin's preset integration to 1-3 lines.

   Affected plugins: O-AnalogEQ, O-Bass, O-Bells, O-Chorus, O-Comp, O-Detune, O-DigiDelay, O-FreqPulse, O-Lyrica, O-Marimba, O-Polystutter, O-SimpleReverb, O-SpectralShaper, O-Tremolo

2. **`getResource()` URL mapping duplicated in all 23 PluginEditors.** Every single Ouaricon plugin hand-writes a `getResource()` method with the same `makeVector` lambda and identical mapping for `/`, `/index.html`, `/js/juce/index.js`, `/js/juce/check_native_interop.js`, and `/modules/preset-manager.js`. Only the plugin-specific image resources differ. The `resource-provider` module is registered in `modules/registry.yaml` but its directory (`core/resource-provider`) does NOT exist -- it was never implemented. Creating this module would eliminate ~30-50 lines of boilerplate per plugin.

3. **WebView initialization boilerplate.** The `WebViewRelayManager` module exists at `modules/core/webview-relay-manager/` but only O-Prism uses it. All other 22 plugins manually write the relay-create / webview-create / attachment-create pattern with identical `withBackend(webview2)`, `withWinWebView2Options(...)`, `withNativeIntegrationEnabled()` boilerplate.

**MEDIUM PRIORITY -- Inconsistencies and Dead Artifacts**

4. **Ghost module in registry.** `modules/registry.yaml` registers `resource-provider` at path `core/resource-provider` but this directory does not exist. The registry should either have this entry removed or the module should be created.

5. **Module adoption gaps.** Only 5 of 14 preset-using plugins have `ouaricon_add_module ... preset-manager` in their CMakeLists. The other 9 have the preset manager code but don't formally declare the module dependency. Similarly, only 1 of 23 plugins (O-Prism) uses the `webview-relay-manager` module.

6. **CMakeLists.txt inconsistency.** Some plugins include `modules/cmake/OuariconModules.cmake` and some don't. The `target_link_libraries` blocks are copy-pasted identically (same 13 JUCE modules) across all 23 plugins. A CMake function could set default JUCE dependencies.

7. **Licensing boilerplate in Editors.** 9 plugins have `#if OUARICON_LICENSING_ENABLED` blocks with identical `licenseOverlay` setup, `licenseStatusChanged` callback, and destructor cleanup. This could potentially be folded into the WebViewRelayManager or a dedicated licensing-UI helper.

8. **Inconsistent processor variable naming.** Some editors use `audioProcessor`, some use `processorRef` to refer to the processor. Minor but adds friction when reading across plugins.

**LOWER PRIORITY -- Nice to have**

9. **`parentHierarchyChanged()` pattern.** About half the plugins use `parentHierarchyChanged()` for deferred WebView navigation, while others navigate immediately in the constructor. The deferred pattern is safer (prevents crashes during plugin scanning) but isn't universally applied.

10. **Large PluginProcessor files.** O-Polystutter (1,829 lines), O-Bells (1,413 lines), O-Lyrica (1,070 lines) have processors that could benefit from extracting more DSP logic into separate DSP/ classes, as some other plugins already do. This is lower priority because it's plugin-specific complexity, not systemic duplication.

For each opportunity, the report should include:
- Description of the problem
- Affected plugins/files
- Estimated scope (small/medium/large)
- Risk level (low/medium/high -- considering these are shipped, installed plugins)
- Suggested approach
- Lines of code that could be eliminated

Also investigate:
- `plugins/tache_plugins/` for any cleanup opportunities (archived/ideated plugins still in the build)
- Whether the `TEMPLATE-HEADLESS-EDITOR` in tache_plugins is still useful
- Any unused files, stale build artifacts, or orphaned resources
- JS-side duplication patterns in `Source/ui/` across plugins (parameter-bindings.js, common CSS patterns, etc.)

Write the report to `.planning/quick/13-look-through-this-project-for-opportunit/13-REPORT.md` organized by priority tier with an executive summary at the top.
  </action>
  <verify>
    File exists at .planning/quick/13-look-through-this-project-for-opportunit/13-REPORT.md and contains prioritized refactoring opportunities with scope estimates
  </verify>
  <done>
    Comprehensive refactoring report delivered with HIGH/MEDIUM/LOW priority tiers, each opportunity having affected files, estimated scope, risk level, and suggested approach. Report includes both the opportunities identified during planning analysis and any additional ones found during deeper investigation.
  </done>
</task>

</tasks>

<verification>
- Report file exists and is well-structured
- Each opportunity has: description, affected files, scope, risk, approach
- Opportunities are prioritized by impact
</verification>

<success_criteria>
- User receives a single actionable document listing all refactoring opportunities
- Highest-impact opportunities (preset boilerplate, resource provider, relay manager adoption) are clearly highlighted
- Report distinguishes safe quick wins from riskier architectural changes
- Enough detail that the user could create follow-up tasks from any item
</success_criteria>

<output>
After completion, create `.planning/quick/13-look-through-this-project-for-opportunit/13-SUMMARY.md`
</output>
