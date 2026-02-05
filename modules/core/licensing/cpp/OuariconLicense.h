/*
  ==============================================================================

    OuariconLicense.h
    Ouaricon Module System - Plugin License Manager

    Handles license activation, offline JWT token validation, and periodic
    re-validation against the Supabase licensing backend.

    Thread-safe: network calls run on a background thread, state updates
    are delivered on the message thread via callAsync.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <functional>

class OuariconLicense : public juce::Timer
{
public:
    // =========================================================================
    // Types
    // =========================================================================

    enum class Status
    {
        Checking,       // Startup: loading stored token
        Unlicensed,     // No valid license found
        Licensed,       // Valid license (online or offline token)
        Expired,        // Offline token expired, needs online refresh
        Revoked,        // Server reports license revoked
        Error           // Network or unexpected error
    };

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void licenseStatusChanged (OuariconLicense& license, Status newStatus) = 0;
    };

    // =========================================================================
    // Construction
    // =========================================================================

    /**
     * @param productId       Product identifier matching backend (e.g., "ouaricon-tremolo")
     * @param supabaseUrl     Supabase project URL
     * @param supabaseAnonKey Supabase anonymous (public) key
     */
    OuariconLicense (const juce::String& productId,
                     const juce::String& supabaseUrl,
                     const juce::String& supabaseAnonKey);

    ~OuariconLicense() override;

    // =========================================================================
    // Activation
    // =========================================================================

    /** Activate a license key on this machine. Callback fires on message thread. */
    void activate (const juce::String& licenseKey,
                   std::function<void (bool success, juce::String message)> callback);

    /** Deactivate this machine from the current license. */
    void deactivate (std::function<void (bool success)> callback);

    // =========================================================================
    // State
    // =========================================================================

    Status getStatus() const                    { return currentStatus.load(); }
    bool isLicensed() const                     { return currentStatus.load() == Status::Licensed; }
    juce::String getLicenseKey() const           { return licenseKey; }
    juce::String getStatusMessage() const        { return statusMessage; }

    // =========================================================================
    // Listeners
    // =========================================================================

    void addListener (Listener* l)              { listeners.add (l); }
    void removeListener (Listener* l)           { listeners.remove (l); }

private:
    // =========================================================================
    // Timer (periodic re-validation)
    // =========================================================================

    void timerCallback() override;

    // =========================================================================
    // Startup
    // =========================================================================

    void checkStoredLicense();
    void refreshOnline();

    // =========================================================================
    // Token file I/O
    // =========================================================================

    juce::File getStorageDir() const;
    bool loadStoredLicense();
    void saveToken (const juce::String& token, const juce::String& key);
    void clearStoredLicense();

    // =========================================================================
    // JWT decode (payload only — no signature verification)
    // =========================================================================

    bool validateOfflineToken (const juce::String& jwt);
    int64_t getTokenExpiryTime (const juce::String& jwt) const;
    static juce::var decodeJwtPayload (const juce::String& jwt);
    static juce::String base64UrlDecode (const juce::String& input);

    // =========================================================================
    // Machine ID (platform-specific)
    // =========================================================================

    static juce::String generateMachineId();
    static juce::String getOsInfo();

    // =========================================================================
    // HTTP
    // =========================================================================

    struct HttpResponse
    {
        int statusCode = 0;
        juce::var body;
    };

    HttpResponse postJsonSync (const juce::String& endpoint, const juce::var& body);

    /** Run a function on a background thread. Cancels any in-flight request. */
    void runOnNetworkThread (const juce::String& threadName, std::function<void()> work);

    // =========================================================================
    // Internal state
    // =========================================================================

    void setStatus (Status s, const juce::String& message = {});

    juce::String productId;
    juce::String supabaseUrl;
    juce::String supabaseAnonKey;

    std::atomic<Status> currentStatus { Status::Checking };
    juce::String licenseKey;
    juce::String offlineToken;
    juce::String statusMessage;
    juce::String machineId;

    // Background thread for network requests
    class NetworkThread;
    std::unique_ptr<NetworkThread> networkThread;

    juce::ListenerList<Listener> listeners;

    // Re-validation every 24 hours
    static constexpr int kRevalidationIntervalMs = 24 * 60 * 60 * 1000;
    // Refresh token 7 days before expiry
    static constexpr int kTokenRefreshDaysBeforeExpiry = 7;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OuariconLicense)
};
