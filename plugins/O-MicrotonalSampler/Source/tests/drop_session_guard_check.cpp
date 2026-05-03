/*
  ==============================================================================

    drop_session_guard_check.cpp
    O-MicrotonalSampler — v1.11.2 drag-drop streaming security regression test.

    Manual run: build with `ninja O-MicrotonalSampler_DropSessionGuardCheck`
    and execute the produced binary. Returns exit code = number of failed
    cases (0 = all pass).

    What this exercises
    -------------------
    The pure helpers in modules/core/webview-drop-streaming/cpp/DropSessionGuard.h
    are the security boundary for the dropSessionAddFile native function
    (now lives inside the shared module's SessionManager as of v1.13.0).
    They were introduced in O-MicrotonalSampler v1.11.2 to address two
    CRITICAL findings from the v1.11.1 code review:

      CR-02 path traversal — JS-supplied relPath previously flowed straight
                            into juce::File::getChildFile, allowing
                            "../etc/passwd" or absolute paths to write
                            outside the session temp dir.
      CR-03 unbounded base64 — no per-file or per-session size cap; a
                              hostile page could OOM the DAW with a single
                              huge base64 string.

    The asserts here are the contract the editor relies on. Any change that
    weakens them must be flagged in the v1.x.y CHANGELOG entry.

  ==============================================================================
*/

#include <juce_core/juce_core.h>

// v1.13.0: DropSessionGuard.h moved into the shared
// modules/core/webview-drop-streaming module. The module's cpp/ is on the
// include path via ouaricon_add_module() in CMakeLists.txt, so a bare
// include works.
#include "DropSessionGuard.h"
#include <iostream>
#include <string>

#if JUCE_MAC || JUCE_LINUX || JUCE_BSD
 #include <unistd.h>   // ::symlink
 #include <cerrno>
#endif

namespace
{
    int failed = 0;

    void check (bool cond, const std::string& desc)
    {
        if (cond)
        {
            std::cout << "  PASS: " << desc << "\n";
        }
        else
        {
            std::cout << "  FAIL: " << desc << "\n";
            ++failed;
        }
    }

    // -------------------- Test 1: relPath traversal rejection --------------------
    void testRelPathTraversalRejection()
    {
        std::cout << "[Test 1] relPath traversal rejection (CR-02)\n";

        // The headline attack — a literal "../etc/passwd".
        check (ouaricon::dropguard::validateRelPath ("../etc/passwd").isNotEmpty(),
               "rejects ../etc/passwd (literal headline attack)");

        // Embedded ".." segment further down the path.
        check (ouaricon::dropguard::validateRelPath ("subdir/../../etc/passwd").isNotEmpty(),
               "rejects subdir/../../etc/passwd (nested traversal)");

        // Trailing ".." (would resolve to sessionDir's parent).
        check (ouaricon::dropguard::validateRelPath ("a/b/..").isNotEmpty(),
               "rejects a/b/.. (trailing traversal)");

        // Absolute POSIX path.
        check (ouaricon::dropguard::validateRelPath ("/etc/passwd").isNotEmpty(),
               "rejects /etc/passwd (absolute POSIX)");

       #if JUCE_WINDOWS
        check (ouaricon::dropguard::validateRelPath ("C:\\Windows\\System32\\drivers\\etc\\hosts").isNotEmpty(),
               "rejects absolute Windows path");
       #endif

        // Backslash separator (would bypass forward-slash segment scan on
        // POSIX while resolving to a Windows separator on Win32).
        check (ouaricon::dropguard::validateRelPath ("foo\\..\\bar").isNotEmpty(),
               "rejects backslash separator");

        // Empty string.
        check (ouaricon::dropguard::validateRelPath ("").isNotEmpty(),
               "rejects empty relPath");

        // Sanity — well-formed relative paths must pass.
        check (ouaricon::dropguard::validateRelPath ("piano/C3.wav").isEmpty(),
               "accepts piano/C3.wav (well-formed)");
        check (ouaricon::dropguard::validateRelPath ("Piano_v1/take1/C3_take_1.wav").isEmpty(),
               "accepts deeply-nested well-formed path");
        check (ouaricon::dropguard::validateRelPath ("file.wav").isEmpty(),
               "accepts bare filename");
    }

    // -------------------- Test 2: size caps --------------------
    void testSizeCaps()
    {
        std::cout << "[Test 2] base64 streaming size caps (CR-03)\n";

        // Per-file cap: 256 MB → base64 length ≈ 256 MB * 4 / 3.
        // Project a >256 MB decoded payload by passing a base64 length
        // larger than (256 MB * 4 / 3).
        const juce::int64 oneMb = 1024LL * 1024LL;

        // Decoded ≈ 200 MB → under cap.
        {
            juce::uint64 projected = 0;
            const juce::int64 base64Len = (200LL * oneMb) * 4LL / 3LL;
            const auto reason = ouaricon::dropguard::checkSizeCaps (
                base64Len, /*sessionTotal*/ 0ULL, projected);
            check (reason.isEmpty(), "200 MB decoded payload accepted (under 256 MB cap)");
        }

        // Decoded ≈ 300 MB → over per-file cap.
        {
            juce::uint64 projected = 0;
            const juce::int64 base64Len = (300LL * oneMb) * 4LL / 3LL;
            const auto reason = ouaricon::dropguard::checkSizeCaps (
                base64Len, /*sessionTotal*/ 0ULL, projected);
            check (reason.isNotEmpty(),
                   "300 MB decoded payload rejected (over 256 MB per-file cap)");
            check (reason.contains ("per-file"),
                   "rejection reason mentions per-file cap");
            check (projected > ouaricon::dropguard::kMaxFileBytes,
                   "projected size exceeds kMaxFileBytes");
        }

        // Decoded ≈ 1 GB single payload → over per-file cap.
        {
            juce::uint64 projected = 0;
            const juce::int64 base64Len = (1024LL * oneMb) * 4LL / 3LL;
            const auto reason = ouaricon::dropguard::checkSizeCaps (
                base64Len, /*sessionTotal*/ 0ULL, projected);
            check (reason.isNotEmpty(),
                   "1 GB single-file payload rejected (over 256 MB cap)");
        }

        // Per-session cap: simulate a session that has already accepted
        // ~3.9 GB and is now offered another 200 MB → over 4 GB session cap.
        {
            const juce::uint64 nearCap =
                ouaricon::dropguard::kMaxSessionBytes - (100ULL * 1024ULL * 1024ULL); // 4 GB - 100 MB
            juce::uint64 projected = 0;
            const juce::int64 base64Len = (200LL * oneMb) * 4LL / 3LL; // ≈ 200 MB decoded
            const auto reason = ouaricon::dropguard::checkSizeCaps (
                base64Len, nearCap, projected);
            check (reason.isNotEmpty(),
                   "200 MB on top of 3.9 GB rejected (over 4 GB session cap)");
            check (reason.contains ("per-session"),
                   "rejection reason mentions per-session cap");
        }

        // Empty payload → rejected (mirrors REVIEW HG-06: zero-byte writes
        // produce a cryptic "invalid header" downstream).
        {
            juce::uint64 projected = 0;
            const auto reason = ouaricon::dropguard::checkSizeCaps (
                /*base64Length*/ 0, /*sessionTotal*/ 0ULL, projected);
            check (reason.isNotEmpty(), "empty payload rejected");
        }

        // Constant sanity — caps are exactly the documented values.
        check (ouaricon::dropguard::kMaxFileBytes == 256ULL * 1024ULL * 1024ULL,
               "per-file cap is 256 MB");
        check (ouaricon::dropguard::kMaxSessionBytes == 4ULL * 1024ULL * 1024ULL * 1024ULL,
               "per-session cap is 4 GB");
    }

    // -------------------- Test 3: parent-chain symlink guard --------------------
    void testParentChainSymlinkGuard()
    {
        std::cout << "[Test 3] parent-chain symlink guard\n";

        // Build a real session dir under the OS temp directory.
        auto tempRoot = juce::File::getSpecialLocation (juce::File::tempDirectory);
        auto sessionDir = tempRoot.getChildFile (
            "o-microtonalsampler-dropguard-test-"
            + juce::String (juce::Time::currentTimeMillis()));
        const auto created = sessionDir.createDirectory();
        check (created.wasOk(), "session dir created");

        // Well-formed target inside the session.
        {
            auto target = sessionDir.getChildFile ("subdir/file.wav");
            const auto reason = ouaricon::dropguard::validateParentChain (sessionDir, target);
            check (reason.isEmpty(),
                   "accepts well-formed target inside session (subdir does not yet exist)");
        }

        // Sibling escape — even with a clean relPath, a target whose parent
        // chain leaves the session must be rejected.
        {
            auto outside = tempRoot.getChildFile ("outside-dir");
            outside.createDirectory();
            auto target = outside.getChildFile ("file.wav");
            const auto reason = ouaricon::dropguard::validateParentChain (sessionDir, target);
            check (reason.isNotEmpty(),
                   "rejects target whose parent is outside the session dir");
            outside.deleteRecursively();
        }

        // Symlink escape — create a symlink inside sessionDir that points
        // outside, then try to write through it.
       #if JUCE_MAC || JUCE_LINUX || JUCE_BSD
        {
            auto evilTarget = tempRoot.getChildFile (
                "o-microtonalsampler-symlink-target-"
                + juce::String (juce::Time::currentTimeMillis()));
            evilTarget.createDirectory();

            auto linkPath = sessionDir.getChildFile ("escape");
            // Create a directory symlink: link → evilTarget.
            const auto sym = ::symlink (evilTarget.getFullPathName().toRawUTF8(),
                                        linkPath.getFullPathName().toRawUTF8());
            if (sym == 0)
            {
                auto target = linkPath.getChildFile ("file.wav");
                const auto reason = ouaricon::dropguard::validateParentChain (
                    sessionDir, target);
                check (reason.isNotEmpty(),
                       "rejects target whose parent chain traverses a symlink");
                check (reason.contains ("symlink"),
                       "rejection reason mentions symlink");
            }
            else
            {
                std::cout << "  SKIP: could not create test symlink (errno="
                          << errno << ") — running without root or on a "
                          << "filesystem that disallows symlinks\n";
            }
            linkPath.deleteFile();
            evilTarget.deleteRecursively();
        }
       #else
        std::cout << "  SKIP: symlink subtest is POSIX-only\n";
       #endif

        sessionDir.deleteRecursively();
    }
}

int main (int /*argc*/, char** /*argv*/)
{
    std::cout << "O-MicrotonalSampler v1.11.2 — DropSessionGuard regression tests\n";
    std::cout << "================================================================\n\n";

    testRelPathTraversalRejection();
    std::cout << "\n";
    testSizeCaps();
    std::cout << "\n";
    testParentChainSymlinkGuard();

    std::cout << "\n================================================================\n";
    if (failed == 0)
        std::cout << "All checks passed.\n";
    else
        std::cout << failed << " check(s) FAILED.\n";

    return failed;
}
