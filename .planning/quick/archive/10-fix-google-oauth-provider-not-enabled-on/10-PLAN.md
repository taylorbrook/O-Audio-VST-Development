---
phase: quick-10
plan: 01
type: execute
wave: 1
depends_on: []
files_modified: []
autonomous: false
requirements: [OAUTH-01]

must_haves:
  truths:
    - "User clicks 'Continue with Google' on oaudio.io/auth/login and is redirected to Google's OAuth consent screen"
    - "After Google authentication, user is redirected back to /auth/callback.html and logged in"
    - "Google OAuth works on both login and any page that offers Google sign-in"
  artifacts: []
  key_links:
    - from: "Supabase Auth (Google provider)"
      to: "Google Cloud OAuth 2.0 Client"
      via: "Client ID + Client Secret configured in Supabase dashboard"
      pattern: "provider: google"
    - from: "Google Cloud OAuth consent screen"
      to: "oaudio.io/auth/callback.html"
      via: "Authorized redirect URI"
      pattern: "https://azccrkrgdtycfsuboimi.supabase.co/auth/v1/callback"
---

<objective>
Fix the Google OAuth "provider is not enabled" error on oaudio.io so users can sign in with Google.

Purpose: Users clicking "Continue with Google" on https://oaudio.io/auth/login.html currently get a 400 error: `{"code":400,"error_code":"validation_failed","msg":"Unsupported provider: provider is not enabled"}`. This is a Supabase dashboard configuration issue -- the Google OAuth provider has never been enabled. The client-side code (`signInWithOAuth({ provider: 'google' })`) is correct.

Output: Working Google OAuth sign-in flow on oaudio.io
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@/Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.js
@/Users/taylorbrook/Dev/Ouaricon Audio Website/js/auth.js
@/Users/taylorbrook/Dev/Ouaricon Audio Website/js/supabase-client.js
@/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/login.html
@/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/callback.html

Supabase project ref: azccrkrgdtycfsuboimi
Supabase project URL: https://azccrkrgdtycfsuboimi.supabase.co
Site URL: https://oaudio.io
Auth callback URL: https://oaudio.io/auth/callback.html
</context>

<tasks>

<task type="checkpoint:human-action" gate="blocking">
  <name>Task 1: Configure Google OAuth in Google Cloud Console and Supabase Dashboard</name>
  <action>
This is a dashboard-only configuration task. No code changes are needed -- the client-side code is already correct.

**Step A: Google Cloud Console -- Create OAuth 2.0 Credentials**

1. Go to https://console.cloud.google.com/
2. Select or create a project for Ouaricon Audio
3. Navigate to "APIs & Services" > "Credentials"
4. Click "Create Credentials" > "OAuth client ID"
5. If prompted, configure the OAuth consent screen first:
   - User Type: External
   - App name: Ouaricon Audio
   - User support email: your email
   - Developer contact: your email
   - Scopes: email, profile, openid (defaults)
   - Publishing status: Production (so any Google user can sign in, not just test users)
6. Create OAuth client ID:
   - Application type: Web application
   - Name: Ouaricon Audio Web
   - Authorized JavaScript origins: `https://oaudio.io`
   - Authorized redirect URIs: `https://azccrkrgdtycfsuboimi.supabase.co/auth/v1/callback`
     (This is the Supabase auth callback -- Supabase handles the OAuth redirect, NOT your site directly)
7. Copy the **Client ID** and **Client Secret**

**Step B: Supabase Dashboard -- Enable Google Provider**

1. Go to https://supabase.com/dashboard/project/azccrkrgdtycfsuboimi
2. Navigate to "Authentication" > "Providers"
3. Find "Google" in the list and click to expand
4. Toggle "Enable Google provider" ON
5. Paste the **Client ID** from Step A
6. Paste the **Client Secret** from Step A
7. Verify the "Redirect URL" shown matches: `https://azccrkrgdtycfsuboimi.supabase.co/auth/v1/callback`
   (This is the URL you added as an authorized redirect URI in Google Cloud Console)
8. Click "Save"

**Step C: Verify Supabase Site URL**

1. In Supabase Dashboard, go to "Authentication" > "URL Configuration"
2. Ensure "Site URL" is set to: `https://oaudio.io`
3. Ensure "Redirect URLs" includes: `https://oaudio.io/auth/callback.html`
   (Add it if missing -- this is the allowlist for where Supabase will redirect after auth)
  </action>
  <verify>
After configuration:
1. Open https://oaudio.io/auth/login.html
2. Click "Continue with Google"
3. You should be redirected to Google's consent screen (not get a 400 error)
4. Select your Google account
5. You should be redirected to https://oaudio.io/auth/callback.html
6. callback.html should process the session and redirect to /member/index.html
  </verify>
  <done>
Google OAuth sign-in works end-to-end: user clicks "Continue with Google" on login page, authenticates with Google, and is redirected back to the site logged in. The 400 "provider is not enabled" error no longer occurs.
  </done>
</task>

</tasks>

<verification>
- Visit https://oaudio.io/auth/login.html and click "Continue with Google" -- should redirect to Google OAuth
- Complete Google sign-in -- should redirect back to /auth/callback.html then to /member/index.html
- Check browser console for any errors during the flow
- Verify the user appears in Supabase Dashboard > Authentication > Users with provider "google"
</verification>

<success_criteria>
- "Continue with Google" button on login page initiates Google OAuth flow (no 400 error)
- After Google authentication, user is redirected back and logged in
- User session is created in Supabase
</success_criteria>

<output>
After completion, create `.planning/quick/10-fix-google-oauth-provider-not-enabled-on/10-SUMMARY.md`
</output>
