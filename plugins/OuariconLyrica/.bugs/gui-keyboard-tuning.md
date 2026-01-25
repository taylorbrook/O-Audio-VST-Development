# GUI Keyboard Does Not Reflect Custom Tuning

**Reported:** 2026-01-22
**Status:** Open
**Priority:** Medium

## Description

The on-screen GUI keyboard in OuariconLyrica does not reflect custom tuning when:
- User edits interval cents in the tuning tab
- User loads a temperament preset
- User loads a Scala file

The keyboard plays the correct tuned frequencies (the audio is correct), but the visual keyboard doesn't indicate the microtuning in any way.

## Expected Behavior

The GUI keyboard should visually indicate when notes are microtuned, possibly by:
- Showing cent deviation labels on keys
- Color-coding keys based on deviation from 12-TET
- Showing visual pitch bend indicators

## Steps to Reproduce

1. Load OuariconLyrica fresh
2. Go to Tuning tab
3. Select a non-12-TET preset (e.g., Werckmeister III) or edit interval values
4. Observe that the on-screen keyboard looks the same as 12-TET

## Technical Context

The interval list editing was fixed in v1.11.1 - the tuning engine correctly applies custom intervals. The issue is purely visual - the GUI keyboard component doesn't read from or display the current tuning state.

## Files Likely Involved

- `Resources/ui/index.html` - keyboard rendering code
- May need to expose tuning interval data to the keyboard component
