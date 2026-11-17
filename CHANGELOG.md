# Changelog

## 1.0.2 — November 17, 2026

- Audit the existing 1.7 runtime support against Player Self Revive 3.7. Retain
  CommonLib's AE runtime selection, Address Library format-5 reader and v5 metadata.
  DBNO does not use the detection hook that upstream skips on 1.7.99.
- Validate the player's primary, animation-holder and actor-value interfaces
  against the runtime Address Library before registering DBNO or installing hooks.
  Log the detected runtime and validation result for compatibility reports.
- Add regression checks for SE/AE address selection and actor interface offsets
  on 1.5.97, 1.6.353, 1.6.629, 1.6.1170, 1.7.99 and 1.7.104, plus synthetic
  format-5 database decoding on both 1.7 versions.
- Preserve 1.0.1's legacy loading fix and all gameplay behavior. Tests exercise
  library routing, not actual Skyrim 1.7 executable layouts or gameplay.
- 1.7.99/1.7.104 in-game validation remains pending; future runtimes are unverified.
- User smoke-tested 1.0.2 on 1.6.1170. Logs confirmed interface validation,
  hook/menu registration, bleedout entry, downed-death ragdoll requests and VCD
  crawl/restoration, with no DBNO warnings or errors in that session.

## 1.0.1 — November 17, 2026

- Add the legacy `SKSEPlugin_Query` export required by SKSE 2.0.20 on Skyrim
  1.5.97. Version 1.0.0 omitted this entry point and could not load there.
- Retain the newer loader metadata and optional VCD interface. Gameplay code
  is unchanged; this corrects plugin loading, not a health-threshold setting.
- Check all four required DLL exports as part of the build tests.
- Skyrim 1.5.97 gameplay remains unverified pending an in-game test.

## 1.0.0 — November 16, 2026

First public release of **Skyrim DBNO - Down But Not Out**. Gameplay matches the
tested 0.1.9 development build; this release updates version metadata, documentation,
credits, license notices and distribution packaging.

- Configurable surviving-health threshold for entering bleedout, default 25%.
- Crawl while downed and hold a configurable input to self-revive.
- Preserve remaining health on entry; lethal damage still kills normally.
- Configurable revive health, hold time, consumable requirements and protection
  duration. Protection defaults to zero seconds.
- Optional suppression of ordinary stagger while downed and instant ragdoll on
  ordinary downed death; defer to active engine killmoves.
- Clear sneak state on entry to prevent overlapping sneak/crawl locomotion.
- Suppress natural health regeneration throughout bleedout, including outside
  combat and during free camera (`tfc`).
- In-game settings through SKSE Menu Framework and prompts through SkyPrompt.
- Optional state-query interface for the separate experimental VCD integration.

User testing has confirmed bleedout operation, stagger suppression, VCD collision
and restoration, and the 0.1.9 free-camera regeneration fix. Sneak-entry speed and
all possible killmove/animation combinations have not been exhaustively verified.
