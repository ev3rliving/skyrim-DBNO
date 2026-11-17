#include "pch.h"
#include "DamageHook.h"
#include "DBNORules.h"
#include "ReviveManager.h"
#include "Settings.h"

namespace DamageHook
{
    namespace
    {
        thread_local bool inDamage = false;
        std::atomic<bool> postReviveInvuln{ false };
        std::atomic<bool> downedInvuln{ false };
        using DamageFn = void (*)(RE::Actor*, RE::Actor*, float);
        DamageFn original;

        void Damage(RE::Actor* target, RE::Actor* attacker, float amount)
        {
            auto& settings = Settings::GetSingleton();
            if (inDamage || !target || !target->IsPlayerRef() || settings.disableMod.load() ||
                ReviveManager::IsPlayerBeastRace()) {
                original(target, attacker, amount);
                return;
            }
            struct Guard {
                Guard() { inDamage = true; }
                ~Guard() { inDamage = false; }
            } guard;
            if (postReviveInvuln.load() || downedInvuln.load()) return;

            auto* values = target->AsActorValueOwner();
            const float before = values->GetActorValue(RE::ActorValue::kHealth);
            // Let the engine apply mitigation and death before evaluating DBNO.
            original(target, attacker, amount);
            const float after = values->GetActorValue(RE::ActorValue::kHealth);
            if (settings.debug.load()) {
                SKSE::log::debug("[SkyrimDBNO] Damage callback: amount={}, before={}, after={}, state={}",
                    amount, before, after, static_cast<int>(ReviveManager::GetState()));
            }
            if (target->IsDead()) {
                ReviveManager::DeathReset();
                return;
            }
            // Zero HP can precede KillImpl. Keep the downed state available for the
            // death hook; TESDeathEvent (or confirmed death above) owns cleanup.
            if (after <= 0.0f) return;
            if (ReviveManager::GetState() != ReviveManager::State::Idle) return;
            if (DBNORules::ShouldEnter(before, after, target->GetActorValueMax(RE::ActorValue::kHealth),
                    settings.bleedoutThresholdPercent.load())) {
                ReviveManager::EnterBleedout(target);
            }
        }
    }

    void SetPostReviveInvuln(bool value) { postReviveInvuln.store(value); }
    bool IsPostReviveInvuln() { return postReviveInvuln.load(); }
    void SetDownedInvuln(bool value) { downedInvuln.store(value); }

    void Install()
    {
        REL::Relocation<std::uintptr_t> vtable{ RE::PlayerCharacter::VTABLE[0] };
        original = reinterpret_cast<DamageFn>(vtable.write_vfunc(0x104, Damage));
        SKSE::log::info("[SkyrimDBNO] Player health-damage hook installed");
    }
}
