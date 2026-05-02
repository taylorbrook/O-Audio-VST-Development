/*
  ==============================================================================

    DropSessionGuard.h
    O-MicrotonalSampler — v1.11.2 drag-drop streaming security guards.

    Header-only validation helpers used by the dropSessionAddFile native
    function (PluginEditor.cpp). Split out so the regression test can
    exercise the path-traversal and size-cap checks without spinning up a
    WebView/editor.

    Threats addressed
    -----------------
    The drag-drop streaming surface accepts a JS-supplied relative path and
    a base64-encoded file payload. Two CRITICAL findings from the v1.11.1
    review:

      CR-02 path traversal — relPath was forwarded to juce::File::getChildFile
                            with no validation; "../etc/passwd" would write
                            outside the session temp dir.
      CR-03 unbounded base64 — no per-file or per-session size cap; a hostile
                              page could OOM the DAW.

    These helpers reject both classes before any allocation or filesystem
    write occurs. Each check returns a reason string (empty == accepted) so
    the caller can DBG-log a single line and reply false to JS.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace ouaricon::dropguard
{
    // Per-file payload cap for a single dropSessionAddFile invocation.
    // 256 MB is generous for any realistic sample (24-bit / 96 kHz / 5 min
    // stereo ≈ 165 MB) while preventing a single malicious payload from
    // exhausting host memory.
    inline constexpr juce::uint64 kMaxFileBytes    = 256ULL * 1024ULL * 1024ULL;

    // Aggregate cap across all dropSessionAddFile calls in a single drop
    // session. 4 GB matches the v1.0.4 CHANGELOG note ("250 MB libraries
    // before background loader starts") with ~16× headroom for orchestral
    // multi-mic libraries, while bounding the worst-case temp-dir blow-up.
    inline constexpr juce::uint64 kMaxSessionBytes = 4ULL  * 1024ULL * 1024ULL * 1024ULL;

    // Validates that `relPath` is a safe, sandboxed-relative path:
    //   - non-empty
    //   - not absolute (no leading slash, no Windows drive letter)
    //   - contains no ".." segment
    //   - contains no NUL byte or backslash (JS layer is documented to use
    //     forward slashes; backslashes would give Windows separator semantics
    //     and bypass the segment scan)
    //
    // Returns an empty String on success, or a short human-readable reason
    // for rejection.
    inline juce::String validateRelPath (const juce::String& relPath)
    {
        if (relPath.isEmpty())
            return "empty relPath";

        if (juce::File::isAbsolutePath (relPath))
            return "absolute path rejected";

        if (relPath.containsChar ('\\'))
            return "backslash separator rejected";

        if (relPath.containsChar ((juce::juce_wchar) 0))
            return "NUL byte rejected";

        // Tokenise on '/' and reject any ".." segment. We deliberately do
        // NOT collapse "a/../b" to "b" — even a balanced traversal indicates
        // a malicious or buggy producer.
        for (auto&& seg : juce::StringArray::fromTokens (relPath, "/", {}))
        {
            if (seg == "..")
                return "parent-directory traversal rejected";
        }

        return {};
    }

    // Walks the parent chain of `target` upward toward `sessionDir`. Rejects
    // if any existing ancestor is a symbolic link (which would escape the
    // sandbox even with a clean relPath) or if the chain exits sessionDir's
    // tree before reaching it.
    //
    // The target file itself is allowed to not yet exist (replaceWithData
    // creates it). Non-existent ancestors are tolerated — they will be
    // created by getParentDirectory().createDirectory() in the caller.
    //
    // Returns an empty String on success or a reason on rejection.
    inline juce::String validateParentChain (const juce::File& sessionDir,
                                             const juce::File& target)
    {
        if (! sessionDir.isDirectory())
            return "session dir missing";

        // sessionDir itself must not be a symlink. Defence-in-depth — the
        // dropSessionStart code creates a real directory, but a malicious
        // local user could theoretically replace it between calls.
        if (sessionDir.isSymbolicLink())
            return "session dir is a symlink";

        juce::File current = target.getParentDirectory();

        for (int hops = 0; hops < 256; ++hops)
        {
            if (current == sessionDir)
                return {};

            // Once we exit sessionDir's subtree we have escaped.
            if (! current.isAChildOf (sessionDir))
                return "parent chain escapes session dir";

            if (current.exists() && current.isSymbolicLink())
                return "symlink in parent chain rejected";

            current = current.getParentDirectory();
        }

        return "parent chain too deep";
    }

    // Conservatively projects the decoded size of a base64 payload. Standard
    // base64 with padding decodes to at most ⌈length * 3 / 4⌉ bytes; we use
    // this upper bound for cap checks so we can reject *before* allocating
    // the decode buffer.
    inline juce::uint64 projectedDecodedSize (juce::int64 base64Length)
    {
        if (base64Length <= 0)
            return 0;
        return (juce::uint64) base64Length * 3ULL / 4ULL;
    }

    // Checks per-file and per-session caps. `currentSessionBytes` is the
    // running total of successfully-accepted files in this session. The
    // caller updates that total only on a successful write.
    //
    // On success, writes the projected decoded size to `projectedOut` and
    // returns an empty reason. On rejection, returns a short reason and
    // leaves `projectedOut` set to the projected size (for diagnostics).
    inline juce::String checkSizeCaps (juce::int64 base64Length,
                                       juce::uint64 currentSessionBytes,
                                       juce::uint64& projectedOut)
    {
        const auto projected = projectedDecodedSize (base64Length);
        projectedOut = projected;

        if (projected == 0)
            return "empty payload rejected";

        if (projected > kMaxFileBytes)
            return "per-file cap exceeded (256 MB)";

        if (currentSessionBytes + projected < currentSessionBytes)
            return "session-cap arithmetic overflow";

        if (currentSessionBytes + projected > kMaxSessionBytes)
            return "per-session cap exceeded (4 GB)";

        return {};
    }
}
