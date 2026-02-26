---
phase: quick-9
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - /Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.js
  - /Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.example.js
  - /Users/taylorbrook/Dev/Ouaricon Audio Website/auth/login.html
autonomous: true
requirements: [QUICK-9]

must_haves:
  truths:
    - "After clicking email confirmation link, user lands on callback.html which processes the token and redirects to login"
    - "Login page shows a 'Thank you for confirming your email' success message when arrived via email verification"
    - "The verify-email.html page remains unchanged as the 'check your email' pre-verification page"
  artifacts:
    - path: "/Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.js"
      provides: "EMAIL_VERIFY_URL pointing to callback.html"
      contains: "callback.html"
    - path: "/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/login.html"
      provides: "Thank you message for verified users"
      contains: "verified"
  key_links:
    - from: "js/config.js EMAIL_VERIFY_URL"
      to: "auth/callback.html"
      via: "Supabase email confirmation redirect"
      pattern: "callback\\.html"
    - from: "auth/callback.html"
      to: "auth/login.html?verified=true"
      via: "email_confirmation type redirect"
      pattern: "verified=true"
    - from: "auth/login.html"
      to: "showFormSuccess"
      via: "URL param check on DOMContentLoaded"
      pattern: "verified.*true"
---

<objective>
Fix the email confirmation flow so users see a "thank you" message after clicking the verification link, instead of being sent back to the "check your email" page.

Purpose: Currently, Supabase redirects the email confirmation link to verify-email.html (the pre-verification "check your email" page), creating a confusing loop. The fix routes the confirmation link through callback.html, which already handles email_confirmation tokens and redirects to login.html with a verified=true param.

Output: Updated config pointing EMAIL_VERIFY_URL to callback.html, and an improved success message on the login page.
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@/Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.js
@/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/callback.html
@/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/login.html
@/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/verify-email.html
</context>

<tasks>

<task type="auto">
  <name>Task 1: Route email confirmation to callback.html and improve login success message</name>
  <files>
    /Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.js
    /Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.example.js
    /Users/taylorbrook/Dev/Ouaricon Audio Website/auth/login.html
  </files>
  <action>
1. In `/Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.js` (line 47), change:
   ```
   EMAIL_VERIFY_URL: window.location.origin + '/auth/verify-email.html',
   ```
   to:
   ```
   EMAIL_VERIFY_URL: window.location.origin + '/auth/callback.html',
   ```
   This makes Supabase redirect the email confirmation link to callback.html, which already has logic (lines 258-260) to detect `type === 'email_confirmation'` and redirect to `/auth/login.html?verified=true`.

2. In `/Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.example.js` (line 53), make the same change so the example stays in sync.

3. In `/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/login.html`, update the success message shown when `?verified=true` is in the URL. Find the block around lines 285-288:
   ```javascript
   if (urlParams.get('verified') === 'true') {
     showFormSuccess(form, 'Email verified! You can now sign in.');
   }
   ```
   Change the message to:
   ```javascript
   if (urlParams.get('verified') === 'true') {
     showFormSuccess(form, 'Thank you for confirming your email! You can now sign in.');
   }
   ```

Do NOT modify verify-email.html -- it correctly serves as the "check your email" page shown immediately after registration. Do NOT modify callback.html -- it already handles the email_confirmation flow correctly. Do NOT modify auth.js -- it correctly passes EMAIL_VERIFY_URL via `emailRedirectTo` option to Supabase signUp.
  </action>
  <verify>
1. Confirm config.js line 47 now contains `callback.html` instead of `verify-email.html`:
   `grep "EMAIL_VERIFY_URL" "/Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.js"`
2. Confirm config.example.js matches:
   `grep "EMAIL_VERIFY_URL" "/Users/taylorbrook/Dev/Ouaricon Audio Website/js/config.example.js"`
3. Confirm login.html has the updated thank you message:
   `grep "Thank you for confirming" "/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/login.html"`
4. Confirm verify-email.html is unchanged (still shows "Check Your Email"):
   `grep "Check Your Email" "/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/verify-email.html"`
5. Confirm callback.html still handles email_confirmation:
   `grep "email_confirmation" "/Users/taylorbrook/Dev/Ouaricon Audio Website/auth/callback.html"`
  </verify>
  <done>
- EMAIL_VERIFY_URL in config.js and config.example.js points to /auth/callback.html
- Login page shows "Thank you for confirming your email! You can now sign in." when ?verified=true
- The full flow works: signup -> verify-email.html (check your email) -> click link -> callback.html (processes token) -> login.html?verified=true (thank you message)
  </done>
</task>

</tasks>

<verification>
Trace the complete email confirmation flow:
1. User signs up -> auth.js calls supabase.auth.signUp with emailRedirectTo = origin + '/auth/callback.html'
2. User sees verify-email.html ("Check Your Email" page)
3. User clicks email link -> Supabase redirects to /auth/callback.html with token params
4. callback.html detects type === 'email_confirmation' -> redirects to /auth/login.html?verified=true
5. login.html detects ?verified=true -> shows "Thank you for confirming your email! You can now sign in."
</verification>

<success_criteria>
- Email confirmation link redirects through callback.html (not verify-email.html)
- Login page displays a clear "thank you" confirmation message after email verification
- verify-email.html remains the pre-verification "check your email" page (unchanged)
- callback.html email_confirmation handling remains intact (unchanged)
</success_criteria>

<output>
After completion, create `.planning/quick/9-fix-email-confirmation-landing-page-to-s/9-SUMMARY.md`
</output>
