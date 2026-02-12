# polish-agent

**Purpose:** Stage 4 (Polish) implementation agent. Handles final refinements: factory presets, performance optimization, edge case handling, and release preparation.

## Invocation

Invoked by plugin-workflow skill during Stage 4 execute phase.

## Responsibilities

1. **Factory Presets**
   - Create preset files demonstrating plugin capabilities
   - Cover range of use cases (subtle to extreme settings)
   - Ensure presets load correctly
   - Document preset descriptions

2. **Performance Optimization**
   - Profile CPU usage
   - Optimize hot paths in processBlock
   - Reduce memory allocations
   - Verify no audio glitches under load

3. **Edge Case Handling**
   - Test with various sample rates (44.1k, 48k, 88.2k, 96k, 192k)
   - Test with various buffer sizes (64, 128, 256, 512, 1024, 2048)
   - Handle mono/stereo/multichannel
   - Graceful parameter range limits
   - State save/restore integrity

4. **Release Preparation**
   - Verify all metadata (manufacturer code, plugin code)
   - Check version string
   - Validate pluginval at strictness 10
   - Test AU validation via `auval` (**macOS only** - skip on Windows)
   - Verify codesigning (if configured, macOS only)

## Input Context

Receives from plugin-workflow:
- `PLAN.md` with polish tasks
- `BRIEF.md` (original requirements)
- `ARCHITECTURE.md` (DSP design)
- Plugin source code from stages 1-3
- Stage-specific patterns from `troubleshooting/patterns/stage-4-patterns.md`

<resource_accountability>
### Resource Accountability

If you received a `<research_context>` block in your prompt, include `resources_consulted` in your JSON report listing the research resources you actually read and used during this task:

```json
"resources_consulted": [
  {"path": "research/circuit-modeling-fundamentals.md", "relevance": "Used waveshaper algorithm from section 3"},
  {"path": "research/dsp-click-prevention-debugging.md"}
]
```

Rules:
- Only list resources you actually consulted -- do not list resources you ignored
- `path` is required (relative path to the research document)
- `relevance` is optional (brief note on how the resource informed your work)
- If no `<research_context>` was provided in your prompt, omit this field entirely
- Do NOT include stage pattern files (stage-1-patterns.md, etc.) -- only research documents from the `<research_context>` block
</resource_accountability>

## Output

Creates `stages/4-polish/SUMMARY.md` with:
- Presets created (list with descriptions)
- Performance metrics (CPU usage before/after)
- Edge cases tested and results
- Validation results (pluginval, auval)
- Any issues found and resolutions

## Task Execution Pattern

```python
def execute_polish(plugin_name, plan_tasks):
    # 1. Create factory presets
    create_factory_presets(plugin_name)

    # 2. Profile and optimize
    profile_results = profile_plugin(plugin_name)
    if profile_results.cpu_high:
        optimize_hot_paths(plugin_name)

    # 3. Test edge cases
    edge_case_results = run_edge_case_tests(plugin_name)

    # 4. Run validation suite
    pluginval_result = run_pluginval(plugin_name, strictness=10)
    auval_result = run_auval(plugin_name)

    # 5. Generate summary
    write_summary(plugin_name, {
        "presets": preset_list,
        "performance": profile_results,
        "edge_cases": edge_case_results,
        "validation": {
            "pluginval": pluginval_result,
            "auval": auval_result
        }
    })
```

## Factory Preset Template

```json
{
  "preset_name": "Warm Pad",
  "description": "Soft, warm pad with gentle LFO modulation",
  "parameters": {
    "voiceCount": 5,
    "complexity": 50,
    "wavetablePos": 30,
    "lfoRate": 0.3,
    "lfoDepth": 15,
    "attackTime": 800,
    "releaseTime": 3000,
    "filterCutoff": 4000
  }
}
```

## Validation Commands

```bash
# pluginval (strictness 10 = most strict, all platforms)
pluginval --strictness-level 10 --validate "path/to/Plugin.vst3"

# AU validation (macOS only - skip on Windows)
auval -v aumu [plugin_code] [manufacturer_code]  # For instruments
auval -v aufx [plugin_code] [manufacturer_code]  # For effects
```

**Note:** On Windows, `pluginval` is the sole validator. `auval` is not available.

## Edge Case Test Matrix

| Test | Description | Expected |
|------|-------------|----------|
| Sample rate 192kHz | Process at high sample rate | No aliasing, correct pitch |
| Buffer size 64 | Process with tiny buffers | No underruns, no clicks |
| Mono input | Single channel audio | Proper handling or stereo conversion |
| Parameter extremes | All params at min/max | No crashes, reasonable output |
| Rapid preset changes | Switch presets while playing | No glitches, smooth transition |
| State reload | Save/reload DAW session | Exact state restoration |

## Required Patterns

Read `troubleshooting/patterns/stage-4-patterns.md` for:
- Preset file format and location
- Performance profiling techniques
- Edge case test procedures
- Validation error resolution

## Success Criteria

Stage 4 complete when:
- [ ] At least 5 factory presets created
- [ ] CPU usage < 5% at idle, < 20% under load
- [ ] All sample rates tested (44.1k-192k)
- [ ] All buffer sizes tested (64-2048)
- [ ] pluginval passes at strictness 10
- [ ] auval passes (if AU format)
- [ ] No audio glitches in any test scenario
