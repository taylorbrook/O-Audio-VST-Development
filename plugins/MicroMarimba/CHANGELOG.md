# MicroMarimba Changelog

All notable changes to this project will be documented in this file.

## [1.1.0] - 2026-01-10

### Added
- Circular scale indicator now flashes red when corresponding note is played
- A4 reference pitch dial resets to 440 Hz on double-click

### Fixed
- Keyboard animation bug: adjacent black key no longer depresses when white key is clicked
  - Root cause: Parent white key `transform: translateY(2px)` moved absolutely-positioned child black key
  - Solution: Added CSS counter-transform `.white-key.playing .black-key:not(.playing) { transform: translateY(-2px); }`

## [1.0.0] - 2026-01-09

### Added
- Initial release
- Physically modeled marimba synthesis with bar/mallet interaction
- Microtonality support: 12-TET, Scala file loading, MTS-ESP stub
- WebView UI with botanical paper aesthetic
- Parameters: Mallet Hardness, Bar Material, Resonance, Velocity Curve, Output Gain
- Tuning parameters: Mode selection, A4 reference pitch (400-480 Hz)
- Body resonance via convolution IR
- Playable on-screen keyboard with MIDI output
