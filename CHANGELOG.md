# Changelog

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
