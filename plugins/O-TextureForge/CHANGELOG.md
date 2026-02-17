# O-TextureForge Changelog

## [1.0.1] - 2026-02-15

### Fixed
- File loading was non-functional: WebView intercepted all drag-and-drop events before they reached the JUCE FileDragAndDropTarget
- Added `browseForFile` native function using juce::FileChooser for reliable file selection
- Wired click handlers on drop zone and scatter placeholder to open file browser dialog
- Large file warning (>100MB) now also works through file browser path

## [1.0.0] - 2026-02-15

### Added
- Initial release: concatenative texture synthesis engine
- 19D MFCC descriptor extraction with PCA and UMAP scatter visualization
- Real-time granular playback with KD-tree nearest-neighbor search
- Timbral macro controls (Energy, Brightness, Texture)
- Scatter position controls with variation radius
- Three MIDI modes: Pitch-Mapped, Trigger + Modulate, Generative Drone
