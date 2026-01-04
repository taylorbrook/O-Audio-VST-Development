# Testing & Debugging Guide

## Manual Invocation

Test build-automation directly without workflow:

```
Invoke build-automation skill for [PluginName]
```

Claude will:
1. Execute build workflow
2. Present appropriate menu on success/failure
3. Await your decision

Use this to test:
- Success menu variations (different stages)
- Failure handling options
- State preservation across retries

## Simulated Failures

To test failure protocol without breaking code:

1. Temporarily rename CMakeLists.txt → CMakeLists.txt.bak
2. Invoke build-automation
3. Build will fail immediately (missing CMakeLists.txt)
4. Test failure menu options
5. Restore CMakeLists.txt

Or inject syntax error:

1. Add `#error "Test failure"` to PluginProcessor.cpp
2. Invoke build-automation
3. Build will fail with clear error
4. Test troubleshooter invocation
5. Remove error directive

## Integration Testing

Test within workflow:

1. Start `/implement [PluginName]`
2. Let workflow reach Stage 1 completion
3. Observe build-automation invocation
4. Test success menu choices
5. Continue workflow through Stage 3

Verify:
- Correct flags used per stage (--no-install for Stage 1, none for others)
- Context-appropriate menus displayed
- State preserved when pausing/resuming
