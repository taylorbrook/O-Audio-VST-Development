---
phase: quick-8
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - ../../../"Ouaricon Audio Website"/member/index.html
  - ../../../"Ouaricon Audio Website"/js/member-portal.js
  - ../../../"Ouaricon Audio Website"/supabase/functions/create-checkout-session/index.ts
autonomous: false
requirements: [FREE-PURCHASE-FIX]
must_haves:
  truths:
    - "Free products purchased individually appear as standalone product cards, not inside a bundle accordion"
    - "Cart is cleared after a free purchase redirects to member portal"
    - "Confirmation email is sent for free purchases"
  artifacts:
    - path: "member/index.html"
      provides: "free_purchase query param handling, cart clearing, confirmation modal"
    - path: "js/member-portal.js"
      provides: "Corrected bundle detection logic that only groups true bundle purchases"
    - path: "supabase/functions/create-checkout-session/index.ts"
      provides: "Free order handler with reliable email sending"
  key_links:
    - from: "member/index.html inline script"
      to: "Cart.clear()"
      via: "free_purchase URL param detection"
      pattern: "urlParams\\.get\\('free_purchase'\\)"
    - from: "member-portal.js renderDashboard"
      to: "renderBundleAccordion vs renderProductCard"
      via: "bundle detection logic checking purchase metadata, not just license count"
      pattern: "purchase\\.product_ids.*bundle"
---

<objective>
Fix three bugs in the free purchase flow: (1) individually purchased free products incorrectly grouped under "Complete Collection" bundle in member portal, (2) cart not clearing after free purchase, (3) no confirmation email sent for free purchases.

Purpose: Free products (O-Bells, O-Detune) purchased together should appear as individual product cards in the member area, the cart should clear after purchase, and the user should receive a confirmation email.

Output: Fixed member portal display logic, free purchase query param handling, and email sending.
</objective>

<execution_context>
NOTE: The website codebase lives at /Users/taylorbrook/Dev/Ouaricon Audio Website/ (separate repo from VST-development).
All file edits must target that directory.
</execution_context>

<context>
@/Users/taylorbrook/Dev/Ouaricon Audio Website/js/member-portal.js
@/Users/taylorbrook/Dev/Ouaricon Audio Website/js/checkout.js
@/Users/taylorbrook/Dev/Ouaricon Audio Website/js/cart.js
@/Users/taylorbrook/Dev/Ouaricon Audio Website/member/index.html
@/Users/taylorbrook/Dev/Ouaricon Audio Website/supabase/functions/create-checkout-session/index.ts
@/Users/taylorbrook/Dev/Ouaricon Audio Website/supabase/functions/stripe-webhook/index.ts
@/Users/taylorbrook/Dev/Ouaricon Audio Website/data/products.json
</context>

<tasks>

<task type="auto">
  <name>Task 1: Fix bundle detection logic and add free_purchase handling to member portal</name>
  <files>
    /Users/taylorbrook/Dev/Ouaricon Audio Website/js/member-portal.js
    /Users/taylorbrook/Dev/Ouaricon Audio Website/member/index.html
  </files>
  <action>
**Bug 1: Products incorrectly grouped as "Complete Collection"**

In `member-portal.js`, the `renderDashboard` function (line ~222) groups licenses by `purchase_id` and treats any purchase with multiple licenses (`groupLicenses.length > 1`) as a bundle. This is wrong -- two individual free products purchased in a single cart checkout share one `purchase_id` but are NOT a bundle.

Fix the bundle detection in `renderDashboard`: Instead of assuming any multi-license purchase is a bundle, check whether the purchase's `product_ids` array (stored on the `purchases` table) contains an actual bundle ID. The approach:

1. In `loadLicenses`, also fetch the related `purchase_id` data by joining purchases to get `product_ids`. Modify the Supabase query to include the purchase's product_ids:
   ```js
   .select(`
     id, product_id, license_key, max_activations, status, created_at, purchase_id,
     purchases!inner ( product_ids ),
     activations ( id, machine_id, machine_name, os_info, status, activated_at, deactivated_at )
   `)
   ```
   Then on each returned license, extract `license.purchases.product_ids` and attach it as `license.original_cart_items`.

2. In `renderDashboard`, change the bundle detection logic: A purchase is a bundle ONLY if any item in `original_cart_items` matches a known bundle ID. Check the `productCatalog.bundles` array -- if `productCatalog.bundles.length === 0` (which it currently is), no purchases should ever render as bundles. The logic should be:
   ```js
   const bundleIds = new Set((productCatalog.bundles || []).map(b => b.id));
   // A purchase group is a bundle only if the original cart contained a bundle product
   const isBundlePurchase = groupLicenses[0].original_cart_items?.some(id => bundleIds.has(id));
   ```
   If `isBundlePurchase` is false, push ALL licenses in the group to `individualLicenses` instead of `bundlePurchases`.

**Bug 2: Cart not clearing after free purchase**

In `member/index.html` inline script (around line 1256), the code only checks for `session_id` URL param (Stripe paid flow). The free purchase flow uses `free_purchase` param instead (set by `create-checkout-session` Edge Function line 151). Add handling:

After the existing `sessionId` check (line 1258-1263), add:
```js
const freePurchaseId = urlParams.get('free_purchase');
if (freePurchaseId) {
  // Free purchase completed - clear cart and show confirmation
  setTimeout(() => handleFreePurchaseConfirmation(freePurchaseId), 500);
}
```

Add a new `handleFreePurchaseConfirmation` function that:
1. Queries `purchases` table by `id` (not `stripe_session_id`) to get the purchase record
2. Loads product catalog, builds items list HTML using `buildItemsList`
3. Shows the purchase confirmation modal
4. Calls `Cart.clear()` to clear the cart
5. Has the same error handling as `handlePurchaseConfirmation`

This function is very similar to `handlePurchaseConfirmation` but queries by purchase `id` instead of `stripe_session_id`, and the amount is always $0.
  </action>
  <verify>
    1. Open browser dev tools, manually test: add O-Bells and O-Detune to cart, complete free checkout
    2. Verify member portal shows both products as individual cards (not under "Complete Collection" accordion)
    3. Verify cart badge shows 0 after redirect to member portal
    4. Verify confirmation modal appears showing the purchased products
  </verify>
  <done>
    - Free products purchased together render as individual product cards, not under a bundle accordion
    - Cart is cleared when member portal loads with `free_purchase` query parameter
    - Confirmation modal shows for free purchases just like paid purchases
  </done>
</task>

<task type="auto">
  <name>Task 2: Debug and fix confirmation email for free purchases</name>
  <files>
    /Users/taylorbrook/Dev/Ouaricon Audio Website/supabase/functions/create-checkout-session/index.ts
  </files>
  <action>
The `handleFreeOrder` function in `create-checkout-session/index.ts` has email sending logic (lines 110-148) that should work, but may be silently failing. Add better error handling and logging to diagnose the issue:

1. Add explicit logging before the email attempt:
   ```ts
   console.log(`Free order: attempting email to ${userEmail}, products: ${JSON.stringify(products)}`)
   ```

2. The `PORTAL_URL` env var (line 116) defaults to `https://oaudio.io/member-portal.html` which is wrong -- the member portal is at `/member/` not `/member-portal.html`. Fix the default:
   ```ts
   const portalUrl = Deno.env.get('PORTAL_URL') || 'https://oaudio.io/member/'
   ```
   Also fix the same wrong default in `stripe-webhook/index.ts` (line 206) for consistency.

3. Wrap the email section in explicit try/catch with detailed error logging so failures are visible in Supabase Edge Function logs:
   ```ts
   try {
     const emailResult = await sendEmail({ ... })
     if (emailResult.success) {
       console.log(`Free order: confirmation sent to ${userEmail}, id: ${emailResult.id}`)
     } else {
       console.error('Free order: email failed:', emailResult.error)
     }
   } catch (emailError) {
     console.error('Free order: email exception:', emailError)
   }
   ```

4. After making changes, deploy the updated Edge Function:
   ```bash
   cd "/Users/taylorbrook/Dev/Ouaricon Audio Website"
   npx supabase functions deploy create-checkout-session
   ```
   If deployment requires auth, create a checkpoint for the user.

5. Also deploy the stripe-webhook function if the PORTAL_URL default was fixed there:
   ```bash
   npx supabase functions deploy stripe-webhook
   ```
  </action>
  <verify>
    1. Check Supabase Edge Function logs after a test free purchase to see email sending logs
    2. Verify confirmation email arrives in inbox after free purchase
    3. If email still fails, the improved logging will show the exact error (missing RESEND_API_KEY, domain verification issue, etc.)
  </verify>
  <done>
    - Free purchase confirmation email is sent successfully OR the root cause is identified in logs
    - PORTAL_URL default is corrected in both Edge Functions
    - Edge Functions are deployed with the fixes
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <what-built>Fixed three bugs in the free purchase flow: bundle display logic, cart clearing, and email confirmation</what-built>
  <how-to-verify>
    1. Go to the product pages and add O-Bells and O-Detune to your cart (both are $0)
    2. Click Checkout -- should redirect to member portal with `?free_purchase=...` in URL
    3. Verify: confirmation modal appears showing both products at $0
    4. Verify: cart badge shows 0 (cart was cleared)
    5. Verify: member portal "Your Products" section shows O-Bells and O-Detune as individual product cards, NOT under a "Complete Collection" accordion
    6. Check your email for a purchase confirmation from Ouaricon Audio
    7. If email did not arrive, check Supabase Edge Function logs for the create-checkout-session function to see the error
  </how-to-verify>
  <resume-signal>Type "approved" or describe any issues</resume-signal>
</task>

</tasks>

<verification>
- Member portal displays free products as standalone cards when purchased together
- Cart clears on free purchase redirect (free_purchase query param handled)
- Confirmation modal appears for free purchases
- Email is sent (or root cause identified if Resend API is misconfigured)
- Existing paid purchase flow (session_id) continues to work unchanged
- Actual bundle purchases (complete-collection) still render as accordion when bundles exist in catalog
</verification>

<success_criteria>
- O-Bells and O-Detune purchased together appear as 2 individual product cards in member portal
- Cart count reads 0 after free purchase redirect
- Purchase confirmation modal displays on free purchase arrival at member portal
- Confirmation email sent (requires RESEND_API_KEY to be configured in Supabase)
</success_criteria>

<output>
After completion, create `.planning/quick/8-fix-free-purchase-flow-products-land-in-/8-SUMMARY.md`
</output>
