# Piece of Cake cloud backend

The native Unreal app is the frontend. Supabase Auth, authenticated HTTPS RPCs and PostgreSQL form the deployed backend. No local API server or paid host is required. The existing Web folder separately contains the optional Pixel Streaming client; Vercel cannot run the native game renderer.

## Current feature scope

- Private cloud profiles using Supabase anonymous authentication on first normal launch; the Auth user is unique and authenticated, even though no email is collected. Cloud saves can be turned off from the in-game menu.
- Mac Keychain retains the rotating refresh token. Access tokens stay in memory. The binary contains only the project's public publishable key.
- Cloud storage for best shards, best relics and completion, with monotonic merges so stale clients cannot erase a better score.
- A bounded, persistent outbox retries completed-run uploads after a network outage or restart. Repeated submissions are idempotent.
- Completed-run history and a generated-name casual leaderboard. Developer-flight runs are excluded from the board. Scores are client-reported; this is not an anti-cheat service.
- Private rows protected by RLS, read-only table grants and constrained RPC writes; bounded run retention and per-user daily submission limit.
- Eight-second asynchronous HTTP timeouts. Unavailable cloud service does not block playing. The app reports a pending save rather than falsely claiming it was uploaded.

This first version does not provide email accounts, account recovery, cross-device linking, checkpoint resume, multiplayer or purchases. A cloud profile belongs to the installation's Keychain credential. Removing that credential loses access to the profile. Local save files are a small offline cache; they are not the backend database.

## Deploy on the Free plan only

1. Create a Supabase Free organization/project; do not attach billing, upgrade compute, enable paid add-ons or start a paid trial.
2. Keep automatic table grants disabled and automatic RLS enabled.
3. Apply `migrations/001_cloud_progress.sql` once through the SQL editor or an owner-authorized database connection.
4. Enable anonymous sign-ins in Authentication → Sign In / Providers. Keep the provider's auth rate limits enabled.
5. Copy the public Project URL and publishable key into `Config/Cloud.ini` at the repository root:

```ini
[PieceOfCake.Cloud]
URL="https://PROJECT.supabase.co"
PublishableKey=sb_publishable_PUBLIC_VALUE
```

Never put the database password, secret API key or service-role JWT in this file. The packaging script merges this public configuration into the staged DefaultGame.ini before cooking. The native client reads Unreal's normal Game configuration; cooked games intentionally reject arbitrary loose INI files.

6. Run `npm ci && npm test` in Backend, then `python3 Backend/tests/live_cloud.py`. Run `Scripts/verify_native_features.py --executable /path/to/PieceOfCake.app/Contents/MacOS/PieceOfCake --cloud` and complete gameplay regressions before releasing.

## Deployed project

The Free project `brzacmgdmgvyzzztcsgx` is provisioned in Seoul. The migration and anonymous Auth are enabled. Real HTTPS tests passed ownership isolation, validation, idempotence and refreshed-session persistence on 8 October 2026. Native app validation is recorded separately in `docs/TEST_RESULTS.md`.

## Tests and operating limits

`npm test` runs the exact migration on PostgreSQL through PGlite, with two mocked Auth identities. It checks private-row isolation, unauthenticated denial, restricted writes, invalid values, idempotence, stale-save merging and leaderboard privacy. This is database integration testing, not evidence of a live cloud deployment.

The live test uses two disposable anonymous Auth profiles against the real project's API. It never prints credentials or changes existing player rows. Test submissions are assisted and excluded from public rankings. Keep live test identities private in ignored runtime files for cleanup using the owner's dashboard.

Supabase Free includes a 500 MB database and 50,000 monthly active users. It can pause inactive projects and does not include automatic backups. Export schema/data through the owner dashboard as needed. Never enable paid usage to handle a quota failure; report it and let the game remain playable.

Official documentation: [Free plan](https://supabase.com/docs/guides/platform/billing-on-supabase), [anonymous authentication](https://supabase.com/docs/guides/auth/auth-anonymous), [row-level security](https://supabase.com/docs/guides/database/postgres/row-level-security).
