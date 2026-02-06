/*
  ==============================================================================

    OuariconLicense.cpp
    Ouaricon Module System - Plugin License Manager

    Implementation: HTTP activation, JWT decode, machine ID, token storage.

  ==============================================================================
*/

#include "OuariconLicense.h"
#include <juce_cryptography/juce_cryptography.h>

#if JUCE_MAC
 #include <IOKit/IOKitLib.h>
#endif

#if JUCE_WINDOWS
 #include <Windows.h>
#endif

// =============================================================================
// NetworkThread — simple background thread for HTTP requests
// =============================================================================

class OuariconLicense::NetworkThread : public juce::Thread
{
public:
    NetworkThread (const juce::String& name, std::function<void()> workFn)
        : juce::Thread (name), work (std::move (workFn)) {}

    void run() override { work(); }

private:
    std::function<void()> work;
};

// =============================================================================
// Construction / Destruction
// =============================================================================

OuariconLicense::OuariconLicense (const juce::String& productId_,
                                  const juce::String& supabaseUrl_,
                                  const juce::String& supabaseAnonKey_)
    : productId (productId_),
      supabaseUrl (supabaseUrl_),
      supabaseAnonKey (supabaseAnonKey_),
      machineId (generateMachineId())
{
    checkStoredLicense();
}

OuariconLicense::~OuariconLicense()
{
    stopTimer();

    if (networkThread && networkThread->isThreadRunning())
        networkThread->stopThread (3000);
}

// =============================================================================
// Startup — check for stored offline token
// =============================================================================

void OuariconLicense::checkStoredLicense()
{
    if (loadStoredLicense())
    {
        if (validateOfflineToken (offlineToken))
        {
            setStatus (Status::Licensed, "License valid (offline)");
            // Start periodic re-validation
            startTimer (kRevalidationIntervalMs);
            // Try an immediate background refresh if token is getting old
            auto expiry = getTokenExpiryTime (offlineToken);
            auto now = juce::Time::currentTimeMillis() / 1000;
            auto daysUntilExpiry = (expiry - now) / 86400;

            if (daysUntilExpiry < kTokenRefreshDaysBeforeExpiry)
                refreshOnline();
        }
        else
        {
            // Token expired or invalid — try online refresh
            setStatus (Status::Expired, "License token expired, refreshing...");
            refreshOnline();
        }
    }
    else
    {
        setStatus (Status::Unlicensed, "No license found");
    }
}

// =============================================================================
// Activate
// =============================================================================

void OuariconLicense::activate (const juce::String& key,
                                std::function<void (bool, juce::String)> callback)
{
    setStatus (Status::Checking, "Activating...");

    runOnNetworkThread ("OuariconActivate", [this, key, callback]()
    {
        juce::DynamicObject::Ptr body = new juce::DynamicObject();
        body->setProperty ("license_key",  key);
        body->setProperty ("machine_id",   machineId);
        body->setProperty ("machine_name", juce::SystemStats::getComputerName());
        body->setProperty ("os_info",      getOsInfo());

       #ifdef JucePlugin_VersionString
        body->setProperty ("app_version", juce::String (JucePlugin_VersionString));
       #endif

        auto response = postJsonSync ("activate-license", juce::var (body.get()));

        juce::MessageManager::callAsync ([this, response, key, callback]()
        {
            if (response.statusCode == 200
                && response.body.isObject()
                && (bool) response.body.getProperty ("valid", false))
            {
                auto token = response.body.getProperty ("offline_token", "").toString();
                offlineToken = token;
                licenseKey = key;
                saveToken (token, key);
                setStatus (Status::Licensed, "License activated");
                startTimer (kRevalidationIntervalMs);

                if (callback)
                    callback (true, "License activated successfully");
            }
            else if (response.statusCode == 0)
            {
                setStatus (Status::Error, "Network error — check your internet connection");

                if (callback)
                    callback (false, statusMessage);
            }
            else
            {
                auto code = response.body.getProperty ("code", "").toString();
                auto message = response.body.getProperty ("message", "Activation failed").toString();

                if (code == "activation_limit_reached")
                {
                    auto details = response.body.getProperty ("details", juce::var());
                    auto max  = (int) details.getProperty ("max_activations", 3);
                    message = "All " + juce::String (max) + " activations used. "
                              "Deactivate a machine at oaudio.io/member";
                }

                setStatus (code == "license_revoked" ? Status::Revoked : Status::Error, message);

                if (callback)
                    callback (false, message);
            }
        });
    });
}

// =============================================================================
// Deactivate
// =============================================================================

void OuariconLicense::deactivate (std::function<void (bool)> callback)
{
    if (licenseKey.isEmpty())
    {
        if (callback)
            callback (false);
        return;
    }

    runOnNetworkThread ("OuariconDeactivate", [this, callback]()
    {
        juce::DynamicObject::Ptr body = new juce::DynamicObject();
        body->setProperty ("license_key", licenseKey);
        body->setProperty ("machine_id",  machineId);

        auto response = postJsonSync ("deactivate-license", juce::var (body.get()));

        juce::MessageManager::callAsync ([this, response, callback]()
        {
            bool success = response.statusCode == 200
                           && response.body.isObject()
                           && (bool) response.body.getProperty ("success", false);

            if (success)
            {
                clearStoredLicense();
                licenseKey = {};
                offlineToken = {};
                stopTimer();
                setStatus (Status::Unlicensed, "License deactivated");
            }

            if (callback)
                callback (success);
        });
    });
}

// =============================================================================
// Periodic re-validation (timer)
// =============================================================================

void OuariconLicense::timerCallback()
{
    if (currentStatus.load() == Status::Licensed && ! licenseKey.isEmpty())
        refreshOnline();
}

void OuariconLicense::refreshOnline()
{
    runOnNetworkThread ("OuariconValidate", [this]()
    {
        juce::DynamicObject::Ptr body = new juce::DynamicObject();
        body->setProperty ("license_key", licenseKey);
        body->setProperty ("machine_id",  machineId);

        auto response = postJsonSync ("validate-license", juce::var (body.get()));

        juce::MessageManager::callAsync ([this, response]()
        {
            if (response.statusCode == 200
                && response.body.isObject()
                && (bool) response.body.getProperty ("valid", false))
            {
                // Validation succeeded — re-activate to get a fresh token
                juce::DynamicObject::Ptr activateBody = new juce::DynamicObject();
                activateBody->setProperty ("license_key",  licenseKey);
                activateBody->setProperty ("machine_id",   machineId);
                activateBody->setProperty ("machine_name", juce::SystemStats::getComputerName());
                activateBody->setProperty ("os_info",      getOsInfo());

                runOnNetworkThread ("OuariconRefresh", [this, activateBody]()
                {
                    auto refreshResp = postJsonSync ("activate-license",
                                                     juce::var (activateBody.get()));

                    juce::MessageManager::callAsync ([this, refreshResp]()
                    {
                        if (refreshResp.statusCode == 200
                            && refreshResp.body.isObject()
                            && (bool) refreshResp.body.getProperty ("valid", false))
                        {
                            auto newToken = refreshResp.body.getProperty ("offline_token", "").toString();

                            if (newToken.isNotEmpty())
                            {
                                offlineToken = newToken;
                                saveToken (newToken, licenseKey);
                            }

                            setStatus (Status::Licensed, "License valid");
                        }
                    });
                });
            }
            else if (response.statusCode == 403)
            {
                auto code = response.body.getProperty ("code", "").toString();

                if (code == "license_revoked")
                {
                    clearStoredLicense();
                    setStatus (Status::Revoked, "License has been revoked");
                }
                else if (code == "license_expired")
                {
                    clearStoredLicense();
                    setStatus (Status::Expired, "License has expired");
                }
            }
            // On network failure (statusCode == 0), silently keep using offline token
        });
    });
}

// =============================================================================
// Token file I/O
// =============================================================================

juce::File OuariconLicense::getStorageDir() const
{
    auto appData = juce::File::getSpecialLocation (
        juce::File::userApplicationDataDirectory);

   #if JUCE_MAC
    return appData.getChildFile ("Ouaricon").getChildFile (productId);
   #elif JUCE_WINDOWS
    return appData.getChildFile ("Ouaricon").getChildFile (productId);
   #else
    // Linux: ~/.config/Ouaricon/
    return appData.getChildFile ("Ouaricon").getChildFile (productId);
   #endif
}

bool OuariconLicense::loadStoredLicense()
{
    auto dir = getStorageDir();
    auto tokenFile = dir.getChildFile ("license.json");

    if (! tokenFile.existsAsFile())
        return false;

    auto content = tokenFile.loadFileAsString();
    auto parsed = juce::JSON::parse (content);

    if (! parsed.isObject())
        return false;

    licenseKey   = parsed.getProperty ("key", "").toString();
    offlineToken = parsed.getProperty ("token", "").toString();

    return licenseKey.isNotEmpty() && offlineToken.isNotEmpty();
}

void OuariconLicense::saveToken (const juce::String& token, const juce::String& key)
{
    auto dir = getStorageDir();
    dir.createDirectory();

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("key",       key);
    obj->setProperty ("token",     token);
    obj->setProperty ("product",   productId);
    obj->setProperty ("machine",   machineId);
    obj->setProperty ("saved_at",  juce::Time::getCurrentTime().toISO8601 (true));

    auto tokenFile = dir.getChildFile ("license.json");
    tokenFile.replaceWithText (juce::JSON::toString (juce::var (obj.get())));
}

void OuariconLicense::clearStoredLicense()
{
    auto dir = getStorageDir();
    auto tokenFile = dir.getChildFile ("license.json");

    if (tokenFile.existsAsFile())
        tokenFile.deleteFile();

    licenseKey = {};
    offlineToken = {};
}

// =============================================================================
// JWT decode (payload only)
// =============================================================================

bool OuariconLicense::validateOfflineToken (const juce::String& jwt)
{
    auto payload = decodeJwtPayload (jwt);

    if (! payload.isObject())
        return false;

    // Check expiry
    auto exp = static_cast<juce::int64> (payload.getProperty ("exp", 0));
    auto now = juce::Time::currentTimeMillis() / 1000;

    if (now >= exp)
        return false;

    // Check issuer
    if (payload.getProperty ("iss", "").toString() != "ouaricon.com")
        return false;

    // Check claims match this machine and product
    if (payload.getProperty ("lic", "").toString() != licenseKey)
        return false;

    if (payload.getProperty ("mid", "").toString() != machineId)
        return false;

    if (payload.getProperty ("pid", "").toString() != productId)
        return false;

    return true;
}

int64_t OuariconLicense::getTokenExpiryTime (const juce::String& jwt) const
{
    auto payload = decodeJwtPayload (jwt);

    if (payload.isObject())
        return static_cast<juce::int64> (payload.getProperty ("exp", 0));

    return 0;
}

juce::var OuariconLicense::decodeJwtPayload (const juce::String& jwt)
{
    // JWT format: header.payload.signature
    auto parts = juce::StringArray::fromTokens (jwt, ".", "");

    if (parts.size() != 3)
        return {};

    auto payloadJson = base64UrlDecode (parts[1]);
    return juce::JSON::parse (payloadJson);
}

juce::String OuariconLicense::base64UrlDecode (const juce::String& input)
{
    // Convert base64url to standard base64
    auto standardBase64 = input;
    standardBase64 = standardBase64.replace ("-", "+").replace ("_", "/");

    // Add padding
    int padding = (4 - (standardBase64.length() % 4)) % 4;

    for (int i = 0; i < padding; ++i)
        standardBase64 += "=";

    juce::MemoryBlock decoded;
    juce::MemoryOutputStream mos (decoded, false);

    if (juce::Base64::convertFromBase64 (mos, standardBase64))
        return juce::String::fromUTF8 ((const char*) decoded.getData(),
                                       (int) decoded.getSize());

    return {};
}

// =============================================================================
// Machine ID — platform-specific
// =============================================================================

juce::String OuariconLicense::generateMachineId()
{
    juce::String rawId;

   #if JUCE_MAC
    // macOS: IOKit hardware serial number
    mach_port_t mainPort = 0;
   #if defined (kIOMainPortDefault)
    if (__builtin_available (macOS 12.0, *))
        mainPort = kIOMainPortDefault;
   #else
    mainPort = kIOMasterPortDefault;
   #endif

    io_service_t platformExpert = IOServiceGetMatchingService (
        mainPort,
        IOServiceMatching ("IOPlatformExpertDevice"));

    if (platformExpert != 0)
    {
        CFTypeRef serialRef = IORegistryEntryCreateCFProperty (
            platformExpert,
            CFSTR ("IOPlatformSerialNumber"),
            kCFAllocatorDefault, 0);

        IOObjectRelease (platformExpert);

        if (serialRef != nullptr)
        {
            char buffer[256];
            if (CFStringGetCString ((CFStringRef) serialRef, buffer, sizeof (buffer),
                                    kCFStringEncodingUTF8))
            {
                rawId = juce::String::fromUTF8 (buffer);
            }

            CFRelease (serialRef);
        }
    }

   #elif JUCE_WINDOWS
    // Windows: MachineGuid from registry
    HKEY hKey;

    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                       L"SOFTWARE\\Microsoft\\Cryptography",
                       0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        WCHAR buffer[256];
        DWORD bufferSize = sizeof (buffer);

        if (RegQueryValueExW (hKey, L"MachineGuid", nullptr, nullptr,
                              (LPBYTE) buffer, &bufferSize) == ERROR_SUCCESS)
        {
            rawId = juce::String (buffer);
        }

        RegCloseKey (hKey);
    }

   #elif JUCE_LINUX
    // Linux: /etc/machine-id
    juce::File machineIdFile ("/etc/machine-id");

    if (machineIdFile.existsAsFile())
        rawId = machineIdFile.loadFileAsString().trim();

    if (rawId.isEmpty())
    {
        juce::File dbusId ("/var/lib/dbus/machine-id");

        if (dbusId.existsAsFile())
            rawId = dbusId.loadFileAsString().trim();
    }
   #endif

    // Fallback: computer name + user name
    if (rawId.isEmpty())
        rawId = juce::SystemStats::getComputerName()
                + "-" + juce::SystemStats::getFullUserName();

    // Hash for consistent length and privacy
    return juce::SHA256 (rawId.toUTF8(), (size_t) rawId.getNumBytesAsUTF8())
               .toHexString();
}

juce::String OuariconLicense::getOsInfo()
{
    return juce::SystemStats::getOperatingSystemName();
}

// =============================================================================
// HTTP
// =============================================================================

OuariconLicense::HttpResponse OuariconLicense::postJsonSync (
    const juce::String& endpoint, const juce::var& body)
{
    HttpResponse result;

    auto jsonString = juce::JSON::toString (body);
    auto url = juce::URL (supabaseUrl + "/functions/v1/" + endpoint)
                   .withPOSTData (jsonString);

    auto extraHeaders = "Content-Type: application/json\r\n"
                        "apikey: " + supabaseAnonKey;

    int httpStatusCode = 0;
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                       .withExtraHeaders (extraHeaders)
                       .withConnectionTimeoutMs (15000)
                       .withStatusCode (&httpStatusCode);

    auto stream = url.createInputStream (options);

    if (stream == nullptr)
    {
        result.statusCode = 0;  // Network error
        return result;
    }

    result.statusCode = httpStatusCode;
    auto responseText = stream->readEntireStreamAsString();
    result.body = juce::JSON::parse (responseText);

    return result;
}

void OuariconLicense::runOnNetworkThread (const juce::String& threadName,
                                          std::function<void()> work)
{
    // Cancel any in-flight request
    if (networkThread && networkThread->isThreadRunning())
        networkThread->stopThread (2000);

    networkThread = std::make_unique<NetworkThread> (threadName, std::move (work));
    networkThread->startThread();
}

// =============================================================================
// State management
// =============================================================================

void OuariconLicense::setStatus (Status s, const juce::String& message)
{
    currentStatus.store (s);
    statusMessage = message;
    listeners.call ([this, s] (Listener& l) { l.licenseStatusChanged (*this, s); });
}
