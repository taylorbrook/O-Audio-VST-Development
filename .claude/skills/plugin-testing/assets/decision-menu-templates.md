# Decision Menu Templates

## Mode Selection Menu {#mode-selection}

```
How would you like to test {PLUGIN_NAME}?

1. Automated stability tests
   → Quick C++ unit tests (requires Tests/ directory)
   → Validates core functionality and edge cases
   → Best for: plugins with test infrastructure

2. Build + pluginval ⭐ RECOMMENDED
   → Industry-standard validation at strictness level 10
   → Tests VST3 and AU formats
   → Catches 99% of issues before DAW testing
   → Required before distribution

3. Manual DAW testing
   → Hands-on testing in your DAW
   → Custom checklist based on plugin features
   → Best for: final validation and sound quality checks

4. Skip testing (NOT RECOMMENDED)
   → Proceed to installation without validation
   → Use only if you've already tested externally

Choose (1-4): _
```

## Missing Tests Directory Menu {#missing-tests}

```
Tests/ directory not found for {PLUGIN_NAME}.

Mode 1 requires C++ test infrastructure (GTest/Catch2).

Recommended: Use Mode 2 (pluginval) instead - it's faster and more comprehensive.

Would you like to:
1. Switch to Mode 2 (Build + Pluginval) ⭐ RECOMMENDED
2. Set up test infrastructure first
3. Cancel testing

Choose (1-3): _
```

## Pluginval Installation Menu {#pluginval-install}

```
Pluginval not found on system.

Pluginval is required for Mode 2 testing. Install via:
- Download: https://github.com/Tracktion/pluginval/releases
- Homebrew: brew install pluginval

Would you like to:
1. Install via Homebrew (recommended)
2. Download manually
3. Switch to Mode 1 (Automated Tests)
4. Switch to Mode 3 (Manual DAW Testing)
5. Cancel testing

Choose (1-5): _
```

## Post-Test Menu (Mode 1 Success) {#post-test-mode1}

```
✓ All automated tests passed ({PASS_COUNT}/{TOTAL_COUNT})

What would you like to do?

1. Investigate results (view detailed logs)
2. Run Mode 2 (pluginval) for comprehensive validation ⭐ RECOMMENDED
3. Continue to installation
4. Return to main workflow

Choose (1-4): _
```

## Post-Test Menu (Mode 1 Failure) {#post-test-mode1-fail}

```
✗ {FAIL_COUNT}/{TOTAL_COUNT} tests failed

What would you like to do?

1. Investigate failures (detailed analysis) ⭐ RECOMMENDED
2. View failed test logs
3. Re-run tests after manual fixes
4. Skip failures and continue (NOT RECOMMENDED)

Choose (1-4): _
```

## Post-Test Menu (Mode 2 Success) {#post-test-mode2}

```
✓ Pluginval validation passed (strictness level 10)

VST3: {VST3_RESULT}
AU: {AU_RESULT}

What's next?

1. Continue to installation ⭐ RECOMMENDED
2. Run Mode 1 (Automated Tests) for additional coverage
3. Run Mode 3 (Manual DAW Testing)
4. View detailed pluginval logs
5. Return to main workflow

Choose (1-5): _
```

## Post-Test Menu (Mode 2 Failure) {#post-test-mode2-fail}

```
✗ Pluginval validation failed

VST3: {VST3_RESULT}
AU: {AU_RESULT}

Common failures: {FAILURE_SUMMARY}

What would you like to do?

1. Investigate failures (deep analysis) ⭐ RECOMMENDED
2. View full pluginval output
3. Re-run after manual fixes
4. Skip failures and continue (NOT RECOMMENDED)

Choose (1-4): _
```

## Post-Test Menu (Mode 3 Complete) {#post-test-mode3}

```
✓ Manual DAW testing complete

Results: {USER_REPORTED_RESULTS}

What's next?

1. Continue to installation ⭐ RECOMMENDED
2. Run Mode 2 (pluginval) for validation
3. Document issues found
4. Return to main workflow

Choose (1-4): _
```

## Failure Investigation Menu {#failure-investigation}

```
Investigating {TEST_NAME} failure...

Analysis: {BRIEF_SUMMARY}

What would you like to do?

1. See detailed explanation and fix recommendations
2. View relevant code sections
3. Delegate to deep-research for root cause analysis
4. Apply suggested fix
5. Skip this failure

Choose (1-5): _
```
