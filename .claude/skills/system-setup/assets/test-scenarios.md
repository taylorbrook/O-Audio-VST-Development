# System Setup Test Scenarios

Available test scenarios for `/setup --test=SCENARIO`:

## fresh-system
- Simulates brand new system with no dependencies installed
- Tests installation flow for all components
- Validates dependency detection logic

## missing-juce
- Has Python, CMake, build tools installed
- Missing only JUCE framework
- Tests JUCE installation path selection

## old-versions
- Has all dependencies but outdated versions
- Tests version checking logic
- Validates upgrade recommendations

## custom-paths
- Dependencies installed in non-standard locations
- Tests path detection algorithms
- Validates custom path configuration

## partial-python
- Has Python but missing pip or venv
- Tests Python component installation
- Validates Python environment configuration

## Usage Examples

```bash
/setup --test=fresh-system
/setup --test=missing-juce
/setup --test=old-versions
```

## Test Mode Behavior

In test mode, the skill:
- Uses mock data for all dependency checks
- Makes **no actual system changes**
- Useful for validating setup flow without modifying environment
- Returns simulated results matching the chosen scenario
