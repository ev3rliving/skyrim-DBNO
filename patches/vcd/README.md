# VCD DBNO experimental patch 1

Created October 31, 2026. This is an unofficial experimental source build of
Variadic Collision Dynamics with automatic Skyrim DBNO crawling collision.
The user confirmed crawl collision and restoration after revival/loading on
November 1, 2026. This optional release retains that tested DLL unchanged;
documentation and dependency notices were updated for the DBNO 1.0.0 release.
Other animation/body combinations may need collision-dimension tuning.

## Install

1. Install **Skyrim DBNO 1.0.0**. The patch also supports the original 0.1.6+ state-query interface.
2. Keep **Variadic Collision Dynamics** and **Variadic Collision Dynamics - Resources** installed.
3. Install `VCD-DBNO-experimental-1.zip` as a separate mod after both. Allow its
   `SKSE/Plugins/VariadicCollisionDynamics.dll` to win the file conflict.
   There must be only one effective VCD DLL; do not rename the original DLL.
4. In Precision's MCM, turn **Disable Character Bumper OFF**. The supplied
   Precision defaults in this workspace had it ON. Confirm the active profile's
   value; the patch deliberately does not replace your Precision settings file.
5. For body-shaped melee hits, keep Precision enabled. For arrows/bolts/spell
   projectiles, install [Accurate Projectile Collision](https://www.nexusmods.com/skyrimspecialedition/mods/188551)
   and its requirements, including a compatible Precision version. These mods
   are external dependencies, not bundled or installed by this patch.

**Version caveat:** your local VCD distribution is labeled 1.3.6. The public source
used here identifies itself as 1.3.5 (commit
`790cc9573a45262a44f9471642ebbe9fdff6c205`). This replaces the entire VCD DLL with
that source build; it is not a binary patch against 1.3.6. Any differences from
the supplied 1.3.6 DLL are unverified. Use a separate test profile.
This build targets SE/AE, including 1.6.1170, and does not support VR.

## What changes

- VCD asks DBNO's versioned C export whether its player is currently downed.
- A living, non-transformed player in a DBNO downed/reviving state receives the
  `DBNOCrawl` preset in preference to combat/indoor/outdoor/swimming presets.
- Loading, disabling DBNO, recovery, death and killmoves release that selection.
  VCD then selects its ordinary preset from the player's current situation.
- The crawl capsule keeps its original bottom contact plane. Sneak/slide/sitting
  scaling does not stack with it. Restoration retains the original vanilla and
  convex geometry caches rather than treating the crawl shape as standing height.
- Existing NPC presets and the Resources pack are retained. No new ESP or
  animation behavior generation is required by this patch itself.

The new preset is a short **vertical movement capsule**, not a precise horizontal
body outline. Radius and height are starting values for the normal humanoid
bleedout animation, not a measured fit for every body or animation replacer.
VCD's preset editor can edit it, or edit
`SKSE/Plugins/VariadicCollisionDynamics/presets/DBNOCrawl.json` while the game is closed.
The initial capsule is 0.88 Havok units high (0.32 endpoint separation + twice
the 0.28 radius). Keep X/Y endpoint components zero and position Z zero; this
VCD implementation maps the vertical endpoints and uses position X/Y for offset.
Remove this JSON to disable automatic crawl selection; ordinary VCD behavior resumes.

## Attack collision is separate

This patch does not replace weapon or projectile damage handling. Precision's
animated body shapes supply melee collision; Accurate Projectile Collision uses
those shapes for projectiles. The APC author's description explicitly distinguishes
this from VCD's controller/environment collision. That combination is the intended
solution for both requested collision types, but bleedout compatibility must be
verified in the user's game. Installing this archive alone does not supply APC.

Sources: [VCD](https://github.com/legendman89/VariadicCollisionDynamics),
[Precision](https://www.nexusmods.com/skyrimspecialedition/mods/72347),
[APC](https://www.nexusmods.com/skyrimspecialedition/mods/188551).

## In-game checks

1. Enable VCD's player collision drawing. Standing should retain your normal
   selected preset; damage down to the threshold and confirm a shorter capsule
   with the bottom still at the same level. Walk/crawl against walls and NPCs.
2. Revive in an open area. Confirm the normal capsule returns. Repeat while
   sneaking, after changing cells, and after loading a save made while downed.
3. Check a normal downed death and a killmove: collision must not remain assigned
   to DBNO. Check stagger still leaves the player downed.
4. Test enemy melee strikes across the visible torso and above it. Separately
   test arrows through the torso/limbs and through empty space above the back.
   Compare with Precision's collision debug display. Don't infer attack accuracy
   merely from VCD's bumper drawing.
5. Do not start by reviving beneath an obstruction: this patch does not add a
   standing-clearance check to self-revival.

`VariadicCollisionDynamics.log` should contain `DBNO crawl integration: connected`
and transitions to `DBNO bleedout` / `DBNO Crawl`. Missing preset/API leaves normal
VCD selection in place. Review that log alongside `SkyrimDBNO.log` after testing.

## Roll back / source

Disable this separate patch mod to restore your original VCD DLL. Skyrim DBNO
1.0.0 continues to work without this patch or VCD.

`VCD-DBNO-experimental-1-source.zip` contains the modified VCD source, build wrapper,
and build support sources. VCD is GPL-3.0; its original LICENSE is included.
CLibUtilsQTR is MIT; its license is included. Other dependency notices are in
the source bundle. No upstream authors have endorsed this build.

For a repository build, run `./Build.ps1 -WithVCD` at the repository root in an
x64 VS developer PowerShell with CMake/Ninja. See [BUILDING.md](../../BUILDING.md).
The build compiles both plugins and emits separate ZIPs in `dist/`.
`upstream/` contains the VCD fork; the root `vendor/` folder supplies CLibUtilsQTR,
nlohmann JSON 3.11.3 (Niels Lohmann), Hedley (Evan Nemerson), and shared build dependencies.
Original header attributions are retained. Tests check the capsule bottom-plane invariant.
The repository build recompiles the same gameplay source using its portable build
setup; the previously distributed experimental DLL remains the in-game-tested binary.
