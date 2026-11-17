#include "pch.h"
#include "Hooks.h"
#include "ReviveManager.h"
#include "Settings.h"
#include "DBNORules.h"
#include "DamageHook.h"

namespace Hooks
{
    using namespace RE;

    namespace
    {
        struct PlayerUpdateHook
        {
            static inline REL::Relocation<void (*)(PlayerCharacter*, float)> original;
            static inline DBNORules::HealthObserver healthObserver;
            static inline int generation = -1;

            static void Update(PlayerCharacter* player, float delta)
            {
                const auto state = ReviveManager::GetState();
                if (player && !Settings::GetSingleton().disableMod.load() &&
                    !ReviveManager::g_postLoadPending.load() &&
                    (state == ReviveManager::State::Downed || state == ReviveManager::State::DownedInvuln)) {
                    // Set this BEFORE the engine update can tick regeneration.
                    // Combat state is deliberately irrelevant. No persistent AV edits.
                    player->UpdateRegenDelay(ActorValue::kHealth,
                        DBNORules::DownedHealthRegenDelay(player->GetRegenDelay(ActorValue::kHealth), delta));
                }
                original(player, delta);
                auto& settings = Settings::GetSingleton();
                const int currentGeneration = ReviveManager::g_resetVersion.load();
                if (currentGeneration != generation) {
                    healthObserver.Reset();
                    generation = currentGeneration;
                }
                if (!ReviveManager::CanControlAnimation(player) || !player->Is3DLoaded() ||
                    settings.disableMod.load() || ReviveManager::g_postLoadPending.load() ||
                    ReviveManager::GetState() != ReviveManager::State::Idle ||
                    DamageHook::IsPostReviveInvuln()) {
                    healthObserver.Reset();
                    return;
                }
                if (auto* ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) return;
                const float hp = player->AsActorValueOwner()->GetActorValue(ActorValue::kHealth);
                const float maximum = player->GetActorValueMax(ActorValue::kHealth);
                const float threshold = settings.bleedoutThresholdPercent.load();
                if (settings.debug.load() && (!healthObserver.valid || hp < healthObserver.previous)) {
                    SKSE::log::debug("[SkyrimDBNO] Health observer: previous={}, hp={}, max={}, threshold={}%, combat={}",
                        healthObserver.valid ? healthObserver.previous : hp, hp, maximum, threshold, player->IsInCombat());
                }
                if (healthObserver.Sample(hp, maximum, threshold)) {
                    SKSE::log::info("[SkyrimDBNO] Update observed surviving threshold damage: hp={}, max={}, threshold={}%", hp, maximum, threshold);
                    ReviveManager::EnterBleedout(player);
                }
            }
        };

        bool IsPlayerBeastForm()
        {
            return ReviveManager::IsPlayerBeastRace();
        }

        bool IsPlayerDownedOrInvuln()
        {
            auto st = ReviveManager::g_state.load();
            return st == ReviveManager::State::Downed || st == ReviveManager::State::DownedInvuln;
        }

        struct HealthRegenHook
        {
            static inline REL::Relocation<float (*)(const ActorValueOwner*, ActorValue)> original;

            static float GetValue(const ActorValueOwner* owner, ActorValue value)
            {
                const float result = original(owner, value);
                if (value != ActorValue::kHealRate && value != ActorValue::kHealRateMult) return result;
                auto* player = PlayerCharacter::GetSingleton();
                // No actor-value queries in this guard: avoid recursive GetActorValue calls.
                return DBNORules::FilterHealthRegen(result, !Settings::GetSingleton().disableMod.load(),
                    IsPlayerDownedOrInvuln(), player && owner == player->AsActorValueOwner(),
                    ReviveManager::g_postLoadPending.load(), true);
            }

            static void Install()
            {
                auto* player = PlayerCharacter::GetSingleton();
                if (!player) {
                    SKSE::log::error("[SkyrimDBNO] Health regen hook unavailable: player singleton missing");
                    return;
                }
                // The ActorValueOwner base offset changes between Skyrim runtimes.
                // Resolve it through CommonLib's runtime accessor, never a static cast.
                auto* owner = player->AsActorValueOwner();
                const auto address = *reinterpret_cast<std::uintptr_t*>(owner);
                const REL::Relocation<std::uintptr_t> expected{ PlayerCharacter::VTABLE[5] };
                if (address != expected.address()) {
                    SKSE::log::error("[SkyrimDBNO] Health regen hook skipped: unexpected ActorValueOwner vtable");
                    return;
                }
                REL::Relocation<std::uintptr_t> vtable{ address };
                original = vtable.write_vfunc(0x1, GetValue);
                SKSE::log::info("[SkyrimDBNO] Player health regeneration rate filter installed");
            }
        };

        struct DeathHook
        {
            static inline REL::Relocation<void (*)(Actor*, Actor*, float, bool, bool)> original;

            static void Kill(Actor* victim, Actor* attacker, float damage, bool sendEvent, bool instantRagdoll)
            {
                auto& settings = Settings::GetSingleton();
                // Capture DBNO state before the original dispatches TESDeathEvent and clears it.
                const bool isPlayer = victim && victim->IsPlayerRef();
                const bool killmove = victim && victim->IsInKillMove();
                const bool wasTransition = isPlayer ? ReviveManager::g_deathTransition.exchange(true) : false;
                const bool selected = DBNORules::InstantRagdollOnDeath(instantRagdoll,
                    !settings.disableMod.load(), settings.ragdollOnDeathWhileDowned.load(),
                    IsPlayerDownedOrInvuln(), isPlayer, killmove);
                if (isPlayer && killmove && IsPlayerDownedOrInvuln())
                    SKSE::log::info("[SkyrimDBNO] Downed death during killmove: preserving engine execution behavior");
                if (selected && !instantRagdoll)
                    SKSE::log::info("[SkyrimDBNO] Downed death: requesting instant ragdoll");
                // Preserve the killer, damage, event flag, and the engine's existing ragdoll requests.
                original(victim, attacker, damage, sendEvent, selected);
                if (isPlayer) ReviveManager::g_deathTransition.store(wasTransition);
            }
        };

        struct StaggerHook
        {
            using Holder = IAnimationGraphManagerHolder;
            static inline REL::Relocation<bool (*)(Holder*, const BSFixedString&)> original;

            static bool Notify(Holder* holder, const BSFixedString& event)
            {
                auto* player = PlayerCharacter::GetSingleton();
                auto& settings = Settings::GetSingleton();
                // This interface is a TESObjectREFR base, not the versioned ActorValueOwner base.
                const bool isPlayer = player && holder == static_cast<Holder*>(player);
                const char* name = event.c_str();
                if (DBNORules::ShouldSuppressStagger(!settings.disableMod.load(),
                        settings.ignoreStaggerWhileDowned.load(), IsPlayerDownedOrInvuln() && ReviveManager::CanControlAnimation(player),
                        isPlayer, name ? std::string_view(name) : std::string_view{})) {
                    if (settings.debug.load())
                        SKSE::log::debug("[SkyrimDBNO] Suppressed stagger-start request while downed: {}", name);
                    // Report the start request as rejected. Damage processing is untouched.
                    return false;
                }
                return original(holder, event);
            }

            static void Install()
            {
                auto* player = PlayerCharacter::GetSingleton();
                if (!player) {
                    SKSE::log::error("[SkyrimDBNO] Stagger hook unavailable: player singleton missing");
                    return;
                }
                auto* holder = static_cast<Holder*>(player);
                // Resolve the actual interface vtable rather than guessing an Actor offset.
                const auto address = *reinterpret_cast<std::uintptr_t*>(holder);
                const REL::Relocation<std::uintptr_t> expected{ PlayerCharacter::VTABLE[3] };
                if (address != expected.address()) {
                    SKSE::log::error("[SkyrimDBNO] Stagger hook skipped: unexpected animation-holder vtable");
                    return;
                }
                REL::Relocation<std::uintptr_t> vtable{ address };
                // IAnimationGraphManagerHolder::NotifyAnimationGraph is virtual slot 1.
                original = vtable.write_vfunc(0x1, Notify);
                SKSE::log::info("[SkyrimDBNO] Player stagger-start filter installed; enabled={}",
                    Settings::GetSingleton().ignoreStaggerWhileDowned.load());
            }
        };

        NiAVObject* Load3D(Character& a_this, bool a_arg1)
        {
            if (Settings::GetSingleton().disableMod.load() || IsPlayerBeastForm())
                return _Load3D(a_this, a_arg1);
            auto* ret = _Load3D(a_this, a_arg1);
            if (!a_this.IsPlayerRef() || !a_this.Is3DLoaded() || !ReviveManager::CanControlAnimation(&a_this)) return ret;
            if (ReviveManager::g_postLoadPending.load()) {
                auto* idle = ReviveManager::GetBleedoutStop();
                auto* proc = a_this.GetActorRuntimeData().currentProcess;
                if (idle && proc) {
                    proc->PlayIdle(&a_this, idle, &a_this);
                    if (Settings::GetSingleton().debug.load())
                        SKSE::log::debug("[SkyrimDBNO] Load3D: post-load BleedoutStop");
                }
            } else if (IsPlayerDownedOrInvuln()) {
                auto* idle = ReviveManager::GetBleedoutStart();
                auto* proc = a_this.GetActorRuntimeData().currentProcess;
                if (idle && proc) {
                    proc->PlayIdle(&a_this, idle, &a_this);
                }
            }
            return ret;
        }

    }

    void Install()
    {
        REL::Relocation<std::uintptr_t> charVtbl{ PlayerCharacter::VTABLE[0] };
        _Load3D = charVtbl.write_vfunc(0x6A, Load3D);
        PlayerUpdateHook::original = charVtbl.write_vfunc(0xAD, PlayerUpdateHook::Update);
        // Actor::KillImpl on SE/AE; use the player's own original implementation.
        DeathHook::original = charVtbl.write_vfunc(0x10E, DeathHook::Kill);
        SKSE::log::info("[SkyrimDBNO] Downed-death ragdoll hook installed; enabled={}",
            Settings::GetSingleton().ragdollOnDeathWhileDowned.load());
        StaggerHook::Install();
        HealthRegenHook::Install();


        SKSE::log::info("[SkyrimDBNO] 3D hook and post-update health observer installed");
    }
}
