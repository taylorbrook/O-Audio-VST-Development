J=/Users/taylorbrook/JUCE/modules
DEFS="-DNDEBUG=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_audio_formats=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1 -DJUCE_DISPLAY_SPLASH_SCREEN=0"
CXXF="-std=gnu++17 -O3 -mmacosx-version-min=11.0 $DEFS -I$J"
FW="-framework Cocoa -framework Foundation -framework IOKit -framework Security -framework Accelerate -framework CoreAudio -framework CoreMIDI -framework QuartzCore -framework AudioToolbox"
