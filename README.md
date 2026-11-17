# Skyrim DBNO - Down But Not Out

**Version 1.0.2** - November 17, 2026

A configurable down-but-not-out system, based on Player Self Revive by zfroggyman.
Survive a hit at low health, enter bleedout, crawl, and hold a button to self-revive.

This repository contains the main mod and the separate [experimental VCD patch](patches/vcd/README.md).
See [BUILDING.md](BUILDING.md) for a self-contained Windows source build.

## Requirements and installation

- Skyrim Special Edition with matching SKSE64 and Address Library for SKSE Plugins.
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
  and [SkyPrompt](https://www.nexusmods.com/skyrimspecialedition/mods/148703), including their requirements.
- Current Menu Framework versions also require [ImGui Icons](https://www.nexusmods.com/skyrimspecialedition/mods/114790).
- Microsoft Visual C++ v14 Redistributable **x64** compatible with MSVC 14.44 or later.

Install `SkyrimDBNO-1.0.2.zip` with your mod manager. Disable the original
**Player Self Revive**; it is not a runtime requirement and must not run alongside
this fork. No ESP/ESL or animation files are included. This mod itself does not
require behavior generation.

Tested in game on Skyrim **1.6.1170** during development. Built for SE/AE.
Version 1.0.1 restored legacy loading on 1.5.97. Version 1.0.2 adds runtime
interface validation and regression coverage for the existing 1.7.99/1.7.104
support, including Address Library format 5. In-game testing on 1.5.97 and
1.7.99/1.7.104 remains pending; future runtimes are unverified. VR is not supported.
Use SKSE, Address Library, SkyPrompt and Menu Framework builds that support your
exact game runtime. Version 1.0.2 passed a user smoke test on 1.6.1170;
logs confirmed interface validation, hook/menu registration, bleedout entry,
the downed-death ragdoll request and VCD crawl/restoration without DBNO errors.
Gameplay rules are unchanged.

## Playing and configuring

Open SKSE Menu Framework and select **Skyrim DBNO**. Changes save automatically
to `Data/SKSE/Plugins/SkyrimDBNO.ini`.

| Setting | Default | Range / behavior |
| --- | --- | --- |
| Bleedout health threshold | 25% | 1–100% of current maximum HP |
| Self-revive health | 25% | 1–100% of current maximum HP |
| Hold duration | 10 seconds | 1–60 seconds |
| Revive input | V / gamepad A | Configurable |
| Invulnerability | 0 seconds | 0–60; applies on entry and after revival |
| Ignore stagger while downed | On | Filters ordinary stagger-start requests |
| Ragdoll on downed death | On | Active killmoves retain engine handling |
| Require consumables | Off | Gates self-revival, not bleedout entry |
| Out-of-combat self-revival | On | Turning it off also blocks out-of-combat entry |

A damaging hit must leave you **alive** at or below the threshold. Lethal damage
still kills: 200 damage against 100 HP does not enter bleedout. Entry preserves
remaining HP; the regular health bar is also your downed health pool. Enemies
are not deliberately made to stop attacking you.

Natural health regeneration is blocked while downed, including outside combat
and during `tfc`. Direct healing from spells, potions or scripts is unaffected.
After revival the remaining regeneration delay expires normally, usually after
about one second unless another delay is longer.

Sneaking is cleared on entry and sneak input is locked while downed. Reviving
leaves you out of sneak mode and releases DBNO's input lock. Ordinary stagger
suppression is optional. Animation recovery waits for stagger/ragdoll interruptions
to end and defers to death and active killmoves.

## Updating

Replace the old DBNO installation and keep only one active `SkyrimDBNO.dll`.
Back up your customized INI before replacing files; development-version settings
remain compatible. The optional experimental VCD patch already used with 0.1.6+
can stay installed with 1.0.2. Its interface is unchanged.

## Optional collision mods

**VCD is not required.** The separate experimental VCD patch changes the player's
movement capsule while downed. It requires VCD and VCD Resources and must win
the VCD DLL file conflict. It uses public source labeled 1.3.5 rather than being
a binary patch against the supplied 1.3.6 DLL; it is not bundled in this release.

Precision and Accurate Projectile Collision provide separate melee/projectile
body collision handling. They are optional, not requirements for DBNO. If using
Precision with VCD, turn **Disable Character Bumper OFF** in Precision's MCM.

## Compatibility and limits

- Do not run the original Player Self Revive alongside this fork. Other death
  alternative, essential-player and damage-interception mods can conflict.
- No bleedout countdown, ally revival, custom execution or new animation set.
- Loading a save clears DBNO state and exits bleedout without healing. Downed
  state and timers are not serialized.
- Beast forms are excluded through the inherited race-name check; unusual or
  localized mod-added races may need compatibility work.
- Custom behavior/animation mods can bypass ordinary stagger or killmove handling.
- The backup health observer sees net health changes between frames; simultaneous
  healing can conceal damage. Positive invulnerability timers use real time.

## Support and source

Logs: `Documents/My Games/Skyrim Special Edition/SKSE/SkyrimDBNO.log`.
For reports, include runtime, mod version, reproduction steps and the log.
For crashes, include a crash logger report. Debug logging is available in settings.

The repository is source, not a game install. Build with [BUILDING.md](BUILDING.md)
or use a packaged mod download. See [CHANGELOG.md](CHANGELOG.md),
[THIRD-PARTY.md](THIRD-PARTY.md), [COPYING.txt](COPYING.txt), [EXCEPTIONS.md](EXCEPTIONS.md)
and `licenses/` for attribution and license notices.

## Repository layout

- `src/`, `tests/`, `assets/`: main DBNO code, regression checks and install assets.
- `patches/vcd/`: separate VCD build, modified upstream source and crawl preset.
- `vendor/`: dependency source snapshots and recorded upstream revisions.
- `cmake/`, `Build.ps1`: portable build and separate mod packaging.

Generated libraries, DLLs, ZIPs and logs are excluded from Git. The optional VCD
DLL is packaged separately and never included in the main DBNO ZIP.

## Development

AI assisted with code changes, debugging, log review, tests, the VCD integration
and documentation. The maintainer directed the features and performed in-game
testing. Original authors' credits and license notices are retained.
