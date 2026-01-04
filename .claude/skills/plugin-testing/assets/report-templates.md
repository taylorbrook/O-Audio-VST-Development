# Report Templates

## Mode 1 Test Results (Pass) {#mode1-results-pass}

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  TEST RESULTS: {PLUGIN_NAME}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✓ All tests passed ({PASS_COUNT}/{TOTAL_COUNT})

Executed tests:
{TEST_LIST}

Duration: {DURATION}
Timestamp: {TIMESTAMP}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Mode 1 Test Results (Fail) {#mode1-results-fail}

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  TEST RESULTS: {PLUGIN_NAME}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✗ {FAIL_COUNT}/{TOTAL_COUNT} tests failed

Passed tests:
{PASSED_TEST_LIST}

Failed tests:
{FAILED_TEST_LIST}

Duration: {DURATION}
Timestamp: {TIMESTAMP}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Mode 2 Pluginval Results (Pass) {#mode2-results-pass}

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  PLUGINVAL VALIDATION: {PLUGIN_NAME}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✓ All formats passed (strictness level 10)

VST3: ✅ PASSED
  Location: {VST3_PATH}
  Tests run: {VST3_TEST_COUNT}
  Duration: {VST3_DURATION}

AU: ✅ PASSED
  Location: {AU_PATH}
  Tests run: {AU_TEST_COUNT}
  Duration: {AU_DURATION}

Timestamp: {TIMESTAMP}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Mode 2 Pluginval Results (Fail) {#mode2-results-fail}

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  PLUGINVAL VALIDATION: {PLUGIN_NAME}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✗ Validation failed (strictness level 10)

VST3: {VST3_STATUS}
  Location: {VST3_PATH}
  {VST3_FAILURES}

AU: {AU_STATUS}
  Location: {AU_PATH}
  {AU_FAILURES}

Common issues detected:
{FAILURE_SUMMARY}

Timestamp: {TIMESTAMP}

Full logs: logs/{PLUGIN_NAME}/pluginval_{TIMESTAMP}.log

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Mode 3 Completion Message {#mode3-completion}

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  MANUAL DAW TESTING: {PLUGIN_NAME}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Checklist generated based on:
- Parameter spec: {PARAM_COUNT} parameters
- Plugin type: {PLUGIN_TYPE}
- Features: {FEATURE_LIST}

Testing checklist:
{CHECKLIST_ITEMS}

Instructions:
1. Load plugin in your DAW
2. Test each item in the checklist
3. Report back with results (pass/fail per item)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Test Log Format {#test-log-format}

```
TEST LOG: {PLUGIN_NAME}
═══════════════════════════════════════

Timestamp: {TIMESTAMP}
Mode: {TEST_MODE}
Plugin Status: {STATUS}

Test Configuration:
- Build type: {BUILD_TYPE}
- Formats tested: {FORMATS}
- Strictness level: {STRICTNESS}

Results:
{DETAILED_RESULTS}

Next Steps:
{NEXT_STEPS}

═══════════════════════════════════════
```
