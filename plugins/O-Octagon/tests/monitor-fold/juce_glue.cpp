// Symbols normally emitted into JuceLibraryCode by the CMake plugin target. This harness has no
// such target, so they are supplied here.
namespace juce
{
    extern const char* const juce_compilationDate = __DATE__;
    extern const char* const juce_compilationTime = __TIME__;
}
