#include "DBNORules.h"
#include <cstdio>
#include <limits>

int main()
{
    int failures = 0;
    auto check = [&](bool condition, const char* name) {
        if (!condition) { std::fprintf(stderr, "FAIL: %s\n", name); ++failures; }
    };
    using DBNORules::ShouldEnter;
    check(ShouldEnter(100, 25, 100, 25), "exact threshold");
    check(ShouldEnter(100, 1, 100, 25), "surviving massive hit");
    check(!ShouldEnter(100, 26, 100, 25), "above threshold");
    check(!ShouldEnter(100, 0, 100, 25), "exactly lethal");
    check(!ShouldEnter(100, -100, 100, 25), "200 damage from 100 HP");
    check(!ShouldEnter(10, 20, 100, 25), "healing below threshold");
    check(!ShouldEnter(25, 25, 100, 25), "blocked damage");
    check(ShouldEnter(20, 19, 100, 25), "new damage after low-health revival");
    check(ShouldEnter(200, 50, 200, 25), "buffed max health");
    check(ShouldEnter(100, 40, 100, 40), "custom threshold");
    check(!ShouldEnter(100, 40, 100, 25), "lower threshold");
    check(!ShouldEnter(100, 25, 0, 25), "invalid max health");
    check(!ShouldEnter(100, std::numeric_limits<float>::quiet_NaN(), 100, 25), "invalid health");
    check(DBNORules::ClampSetting(0, 0, 60, 0) == 0, "zero protection stays zero");
    check(DBNORules::ClampSetting(-1, 0, 60, 0) == 0, "negative protection clamped");
    check(DBNORules::ClampSetting(std::numeric_limits<float>::quiet_NaN(), 1, 100, 25) == 25, "invalid threshold fallback");
    DBNORules::HealthObserver observer;
    check(!observer.Sample(100, 100, 25), "initial sample establishes baseline");
    // Engine deducted HP before the damage notification: its before/after are both 20.
    check(!ShouldEnter(20, 20, 100, 25), "callback alone misses pre-applied damage");
    check(observer.Sample(20, 100, 25), "update detects damage applied outside callback");
    check(!observer.Sample(20, 100, 25), "unchanged low HP does not repeat entry");
    observer.Reset();
    check(!observer.Sample(100, 100, 25), "reload resets baseline");
    check(!observer.Sample(0, 100, 25), "observer never rescues exact lethal damage");
    check(!observer.Sample(100, 100, 25), "death invalidates baseline");
    check(!observer.Sample(-100, 100, 25), "observer never rescues overkill");
    observer.Reset();
    check(!observer.Sample(10, 100, 25), "revival baseline below threshold");
    check(!observer.Sample(20, 100, 25), "healing is not damage");
    check(observer.Sample(19, 100, 25), "fresh damage after revival");
    observer.Reset();
    check(!observer.Sample(60, 200, 25), "buffed health baseline");
    check(observer.Sample(50, 200, 25), "observer uses buffed maximum");
    using DBNORules::ShouldSuppressStagger;
    check(ShouldSuppressStagger(true, true, true, true, "staggerStart"), "downed stagger suppressed");
    check(ShouldSuppressStagger(true, true, true, true, "StaggerStart"), "stagger event case variation");
    check(!ShouldSuppressStagger(false, true, true, true, "staggerStart"), "disabled mod passes stagger");
    check(!ShouldSuppressStagger(true, false, true, true, "staggerStart"), "disabled option passes stagger");
    check(!ShouldSuppressStagger(true, true, false, true, "staggerStart"), "standing or revived player passes stagger");
    check(!ShouldSuppressStagger(true, true, true, false, "staggerStart"), "NPC stagger unaffected");
    for (const auto event : { "staggerStop", "BleedoutStart", "BleedoutStop", "Ragdoll", "Death", "staggerStartCustom", "" }) {
        check(!ShouldSuppressStagger(true, true, true, true, event), "unrelated events pass through");
    }
    using DBNORules::InstantRagdollOnDeath;
    check(InstantRagdollOnDeath(false, true, true, true, true), "downed player death requests ragdoll");
    check(!InstantRagdollOnDeath(false, true, true, false, true), "standing death unchanged");
    check(!InstantRagdollOnDeath(false, true, false, true, true), "death toggle disabled");
    check(!InstantRagdollOnDeath(false, false, true, true, true), "disabled mod leaves death alone");
    check(!InstantRagdollOnDeath(false, true, true, true, false), "NPC death unchanged");
    check(InstantRagdollOnDeath(true, false, false, false, false), "existing engine ragdoll request preserved");
    check(!InstantRagdollOnDeath(false, true, true, true, true, true), "killmove keeps native death behavior");
    check(InstantRagdollOnDeath(true, true, true, true, true, true), "native ragdoll preserved during killmove");
    using DBNORules::CanControlAnimation;
    check(CanControlAnimation(false, false, false, 25), "living downed actor can animate");
    check(!CanControlAnimation(false, true, false, 25), "killmove blocks DBNO animation and revival");
    check(!CanControlAnimation(true, false, false, 25), "dying actor cannot play bleedout idle");
    check(!CanControlAnimation(false, false, true, 25), "death handler excludes concurrent recovery");
    check(!CanControlAnimation(false, false, false, 0), "zero HP blocks revival");
    using DBNORules::DownedHealthRegenDelay;
    check(DownedHealthRegenDelay(0, 0) == 1, "bleedout entry immediately delays regeneration");
    check(DownedHealthRegenDelay(0, 0.016f) > 0.016f, "regen stays delayed after normal update");
    check(DownedHealthRegenDelay(0, 3) == 4, "long frame cannot exhaust downed regen delay");
    check(DownedHealthRegenDelay(7, 0.016f) == 7, "longer damage-induced delay preserved");
    check(DownedHealthRegenDelay(-1, -1) == 1, "invalid negative timers normalized");
    check(DownedHealthRegenDelay(std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()) == 1, "nonfinite timers normalized");
    using DBNORules::FilterHealthRegen;
    check(FilterHealthRegen(0.7f, true, true, true, false, true) == 0, "downed natural heal rate blocked without update ticks");
    check(FilterHealthRegen(250, true, true, true, false, true) == 0, "buffed health regen multiplier blocked");
    check(FilterHealthRegen(0.7f, true, false, true, false, true) == 0.7f, "revived player immediately exposes normal rate");
    check(FilterHealthRegen(1, false, true, true, false, true) == 1, "disabled mod exposes normal rate");
    check(FilterHealthRegen(1, true, true, false, false, true) == 1, "NPC regeneration unaffected");
    check(FilterHealthRegen(1, true, true, true, true, true) == 1, "loading bypasses stale downed state");
    check(FilterHealthRegen(40, true, true, true, false, false) == 40, "health and non-regen values unaffected");
    return failures ? 1 : 0;
}
