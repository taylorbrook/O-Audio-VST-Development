---
phase: quick-10
status: complete (human-action)
date: 2026-02-26
---

# Quick Task 10: Fix Google OAuth Provider Not Enabled on oaudio.io

## Problem
Users clicking "Continue with Google" on https://oaudio.io/auth/login received:
```json
{"code":400,"error_code":"validation_failed","msg":"Unsupported provider: provider is not enabled"}
```

## Diagnosis
- The client-side code (`signInWithOAuth({ provider: 'google' })`) is correct
- The Google OAuth provider was never enabled in the Supabase dashboard
- No code changes needed — this is a dashboard configuration task

## Resolution
Manual configuration required in two dashboards:

1. **Google Cloud Console** — Create OAuth 2.0 credentials with redirect URI `https://azccrkrgdtycfsuboimi.supabase.co/auth/v1/callback`
2. **Supabase Dashboard** — Enable Google provider, paste Client ID + Secret
3. **Supabase URL Config** — Ensure redirect URLs include `https://oaudio.io/auth/callback.html`

## Files Changed
None — dashboard configuration only.
