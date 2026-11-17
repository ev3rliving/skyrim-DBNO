#include "pch.h"
#include "ReviveManager.h"
#include "Settings.h"
#include "DamageHook.h"
#include "Strings.h"
#include "DBNORules.h"
#include "SkyPrompt/API.hpp"

namespace ReviveManager
{
    using namespace RE;

    inline std::atomic<bool> g_invulnGodModeByUs{ false };
    inline std::atomic<bool> g_deathPending{ false };
    inline std::atomic<std::chrono::steady_clock::time_point> g_deathPendingSince{};

    namespace
    {
        std::atomic<TESFaction*>  g_currentFollowerFaction{ nullptr };
        std::atomic<TESIdleForm*> g_bleedoutStart{ nullptr };
        std::atomic<TESIdleForm*> g_bleedoutStop{ nullptr };

        std::vector<IngestibleEntry> g_ingestibleCache;
        std::atomic<bool> g_ingestibleCacheBuilt{ false };
        std::vector<MiscEntry> g_miscCache;
        std::atomic<bool> g_miscCacheBuilt{ false };
        std::atomic<int>  g_postReviveVersion{ 0 };
        std::atomic<TESGlobal*> g_bloodDebtGlobal{ nullptr };
        std::atomic<bool> g_checkStatePending{ false };

        std::unordered_map<FormID, std::string> g_formIDToEditorID;

        const std::string& GetEditorID(TESForm* form)
        {
            static const std::string empty;
            if (!form) return empty;
            auto it = g_formIDToEditorID.find(form->GetFormID());
            return it != g_formIDToEditorID.end() ? it->second : empty;
        }

        SkyPromptAPI::ClientID g_skyPromptClientID{ 0 };
        std::atomic<bool> g_skyPromptShowing{ false };
        std::atomic<bool> g_skyPromptActive{ false };
        bool g_wasFirstPerson{ false };
        std::atomic<bool> g_repairAnimation{ false };
        std::atomic<bool> g_sneakControlsDisabledByUs{ false };

        void RestoreSneakControls()
        {
            if (g_sneakControlsDisabledByUs.exchange(false)) {
                if (auto* controls = ControlMap::GetSingleton())
                    controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kSneaking, true, true);
            }
        }

        bool ClearDownedSneaking(Actor* player)
        {
            // Called only on the game thread after CanControlAnimation succeeds.
            // Use the runtime actor-state accessor (AE's base offset differs).
            auto* state = player->AsActorState();
            bool graphSneaking = false;
            player->GetGraphVariableBool("IsSneaking", graphSneaking);
            if (!state || (!state->IsSneaking() && !graphSneaking)) return false;
            state->actorState1.sneaking = false;
            const bool graphSet = player->SetGraphVariableBool("IsSneaking", false);
            const bool notified = player->NotifyAnimationGraph("sneakStop");
            SKSE::log::info("[SkyrimDBNO] Cleared sneak state for bleedout; graphSet={}, sneakStop={}",
                graphSet, notified);
            return true;
        }

        SkyPromptAPI::Prompt g_revivePrompt{ "Hold to Self Revive", 8733, 0, SkyPromptAPI::PromptType::kHint };
        std::array<SkyPromptAPI::Prompt, 1> g_prompts = { g_revivePrompt };

        class RevivePromptSink : public SkyPromptAPI::PromptSink
        {
        public:
            std::span<const SkyPromptAPI::Prompt> GetPrompts() const override
            {
                return g_prompts;
            }

            void ProcessEvent(SkyPromptAPI::PromptEvent event) const override
            {
            }
        };

        static RevivePromptSink g_promptSink;

        bool IsActorValid(ActorHandle h)
        {
            auto p = h.get().get();
            if (!p || p->IsDisabled() || p->IsDead()) return false;
            auto* state = p->AsActorState();
            if (state && (state->IsBleedingOut() || state->GetLifeState() == ACTOR_LIFE_STATE::kEssentialDown || state->GetLifeState() == ACTOR_LIFE_STATE::kBleedout))
                return false;
            if (!p->GetParentCell() || p->GetParentCell()->cellState != TESObjectCELL::CellState::kAttached || !p->Is3DLoaded())
                return false;
            return true;
        }

        bool IsFriendlyActor(Actor* a)
        {
            if (!a || a->IsPlayerRef()) return false;
            if (a->IsPlayerTeammate() && a->IsInFaction(g_currentFollowerFaction.load())) return true;
            if (a->IsCommandedActor()) return true;
            auto* base = a->GetActorBase();
            if (base && base->IsSummonable()) return true;
            return false;
        }

        void DoSelfRevive()
        {
            if (IsPlayerBeastRace()) return;
            const bool dbg = Settings::GetSingleton().debug.load();
            auto* plr = PlayerCharacter::GetSingleton();
            if (!CanControlAnimation(plr) ||
                !PlayerCanBleedOut()) return;
            auto& s = Settings::GetSingleton();
            bool consumableRequired = s.consumableRequiredForRevival.load();
            TESForm* requiredItem = nullptr;
            if (consumableRequired && !s.requiredConsumableEditorIDs.empty()) {
                requiredItem = PickRandomRequiredItem();
                if (!requiredItem) return;
            }
            g_resetVersion.fetch_add(1);
            g_state.store(State::Reviving);
            SkyPromptAPI::RemovePrompt(&g_promptSink, g_skyPromptClientID);
            g_skyPromptShowing.store(false);
            g_skyPromptActive.store(false);
            plr->RemoveAnimationGraphEventSink(&BleedoutEventHandler::GetSingleton());

            if (auto* proc = plr->GetActorRuntimeData().currentProcess) {
                proc->PlayIdle(plr, g_bleedoutStop.load(), plr);
            }

            plr->SetLifeState(ACTOR_LIFE_STATE::kAlive);
            RestoreSneakControls();
            auto* cmap = ControlMap::GetSingleton();
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kActivate, true, true);
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kJumping, true, true);
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kMainFour, true, true);
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kPOVSwitch, true, true);

            float maxHp = plr->GetActorValueMax(ActorValue::kHealth);
            if (maxHp >= 1.0f) {
                float revivePct = s.playerHealthPercentUponRevival.load() / 100.0f;
                float targetHp = maxHp * revivePct;
                if (targetHp < 1.0f) targetHp = 1.0f;
                float currentHp = plr->AsActorValueOwner()->GetActorValue(ActorValue::kHealth);
                float hpDiff = targetHp - currentHp;
                if (std::abs(hpDiff) > 0.5f) {
                    if (hpDiff > 0.0f) plr->AsActorValueOwner()->RestoreActorValue(ActorValue::kHealth, hpDiff);
                    else plr->AsActorValueOwner()->DamageActorValue(ActorValue::kHealth, -hpDiff);
                }
            } else if (dbg) {
                SKSE::log::debug("[SkyrimDBNO] Revive HP placement skipped: max health read {:.1f}", maxHp);
            }

            const int pv = g_postReviveVersion.fetch_add(1) + 1;
            const float invulnDur = s.invulnerabilityDurationSeconds.load();
            DamageHook::SetDownedInvuln(false);
            DamageHook::SetPostReviveInvuln(false);
            ClearInvulnGodMode();
            if (invulnDur > 0.0f) {
                DamageHook::SetPostReviveInvuln(true);
                ApplyInvulnGodMode();
                std::thread([pv, invulnDur]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(invulnDur * 1000.0f)));
                    if (auto* ti = SKSE::GetTaskInterface()) ti->AddTask([pv]() {
                        if (g_postReviveVersion.load() != pv) return;
                        DamageHook::SetPostReviveInvuln(false);
                        ClearInvulnGodMode();
                    });
                }).detach();
            }

            g_state.store(State::Idle);

            if (g_wasFirstPerson) {
                PlayerCamera::GetSingleton()->ForceFirstPerson();
            }

            if (consumableRequired && requiredItem) {
                if (requiredItem->GetFormType() == FormType::AlchemyItem) {
                    auto* al = static_cast<AlchemyItem*>(requiredItem);
                    if (al->IsFood()) {
                        if (auto* equipMgr = ActorEquipManager::GetSingleton()) {
                            equipMgr->EquipObject(plr, al);
                        }
                    } else {
                        plr->DrinkPotion(al, nullptr);
                    }
                    if (dbg) SKSE::log::debug("[SkyrimDBNO] Self-revive with '{}'", al->GetName());
                } else if (requiredItem->GetFormType() == FormType::Misc) {
                    auto* misc = static_cast<TESObjectMISC*>(requiredItem);
                    plr->RemoveItem(misc, 1, ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                    if (dbg) SKSE::log::debug("[SkyrimDBNO] Self-revive, removed misc '{}'", misc->GetName());
                }
            }

            bool swimming = false;
            if (auto* state = plr->AsActorState())
                swimming = state->IsSwimming();
            if (swimming) {
                if (dbg) SKSE::log::debug("[SkyrimDBNO] Swimming at revive, starting water-exit monitor");
                const int swimPv = g_postReviveVersion.load();
                auto done = std::make_shared<std::atomic<bool>>(false);
                std::thread([swimPv, done, dbg]() {
                    while (!done->load()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        if (g_postReviveVersion.load() != swimPv) return;
                        if (auto* ti = SKSE::GetTaskInterface())
                            ti->AddTask([swimPv, done, dbg]() {
                                if (g_postReviveVersion.load() != swimPv) return;
                                auto* p = PlayerCharacter::GetSingleton();
                                if (!CanControlAnimation(p)) { done->store(true); return; }
                                bool sw = false;
                                if (auto* st = p->AsActorState()) sw = st->IsSwimming();
                                if (sw) return;
                                if (auto* proc = p->GetActorRuntimeData().currentProcess)
                                    proc->PlayIdle(p, g_bleedoutStop.load(), p);
                                if (dbg) SKSE::log::debug("[SkyrimDBNO] Swimming monitor: BleedoutStop after exiting water");
                                done->store(true);
                            });
                    }
                }).detach();
            }

            if (dbg) SKSE::log::debug("[SkyrimDBNO] Player self-revived");
        }

        void ShowSkyPrompt()
        {
            if (IsPlayerBeastRace()) return;
            if (g_skyPromptShowing.load()) return;
            auto st = g_state.load();
            if (st != State::DownedInvuln && st != State::Downed) return;

            auto& s = Settings::GetSingleton();
            uint32_t kb = s.keyboardHotkey.load();
            uint32_t gp = s.gamepadHotkey.load();

            if (kb == 0 && gp == 0) {
                if (s.debug.load()) SKSE::log::debug("[SkyrimDBNO] No hotkey configured, can't show prompt");
                return;
            }

            std::array<std::pair<INPUT_DEVICE, uint32_t>, 2> keys;
            size_t keyCount = 0;

            if (kb != 0) {
                if (kb >= 256) {
                    keys[keyCount] = { INPUT_DEVICE::kMouse, kb - 256 };
                } else {
                    keys[keyCount] = { INPUT_DEVICE::kKeyboard, kb };
                }
                keyCount++;
            }
            if (gp != 0) {
                keys[keyCount] = { INPUT_DEVICE::kGamepad, gp };
                keyCount++;
            }

            static std::pair<INPUT_DEVICE, uint32_t> s_keysBuf[2];
            for (size_t i = 0; i < keyCount; ++i) s_keysBuf[i] = keys[i];
            g_prompts[0].button_key = std::span{ s_keysBuf, keyCount };
            g_prompts[0].progress = 0.0f;

            (void)SkyPromptAPI::SendPrompt(&g_promptSink, g_skyPromptClientID);
            g_skyPromptShowing.store(true);
            g_skyPromptActive.store(true);
        }

        class ReviveInputSink : public BSTEventSink<InputEvent*>
        {
        public:
            static ReviveInputSink& GetSingleton()
            {
                static ReviveInputSink instance;
                return instance;
            }

            BSEventNotifyControl ProcessEvent(InputEvent* const* a_event, BSTEventSource<InputEvent*>*) override
            {
                if (Settings::GetSingleton().disableMod.load()) return BSEventNotifyControl::kContinue;
                if (IsPlayerBeastRace()) return BSEventNotifyControl::kContinue;
                if (!a_event) return BSEventNotifyControl::kContinue;
                if (g_state.load() != State::Downed && g_state.load() != State::DownedInvuln) return BSEventNotifyControl::kContinue;
                if (!g_skyPromptActive.load()) return BSEventNotifyControl::kContinue;

                auto& s = Settings::GetSingleton();
                uint32_t kb = s.keyboardHotkey.load();
                uint32_t gp = s.gamepadHotkey.load();

                for (auto* e = *a_event; e; e = e->next) {
                    if (e->GetEventType() != INPUT_EVENT_TYPE::kButton) continue;
                    auto* btn = static_cast<ButtonEvent*>(e);

                    uint32_t key = btn->GetIDCode();
                    auto device = btn->GetDevice();

                    if (device == INPUT_DEVICE::kMouse) key += 256;
                    else if (device == INPUT_DEVICE::kGamepad)
                        key = SKSE::InputMap::GamepadMaskToKeycode(key);

                    bool match = false;
                    if (kb != 0 && key == kb) match = true;
                    if (gp != 0 && device == INPUT_DEVICE::kGamepad && key == gp) match = true;

                    if (!match) continue;

                    if (btn->IsDown()) {
                        if (!m_isHolding) {
                            m_isHolding = true;
                            m_holdStart = std::chrono::steady_clock::now();
                            m_holdCV = g_resetVersion.load();
                        }
                    } else if (btn->IsUp()) {
                        m_isHolding = false;
                    }
                }
                return BSEventNotifyControl::kContinue;
            }

            bool IsHolding() const { return m_isHolding; }
            float GetHoldDuration() const
            {
                if (!m_isHolding) return 0.0f;
                return std::chrono::duration<float>(std::chrono::steady_clock::now() - m_holdStart).count();
            }
            int GetHoldCV() const { return m_holdCV; }
            void ResetHold()
            {
                m_isHolding = false;
            }

        private:
            std::atomic<bool> m_isHolding{ false };
            std::chrono::steady_clock::time_point m_holdStart;
            std::atomic<int> m_holdCV{ 0 };
        };

        void StartHoldMonitor()
        {
            const int cv = g_resetVersion.load();
            auto pending = std::make_shared<std::atomic<bool>>(false);
            std::thread([cv, pending]() {
                while (g_resetVersion.load() == cv) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if (pending->exchange(true)) continue;
                    if (auto* ti = SKSE::GetTaskInterface()) ti->AddTask([cv, pending]() {
                        pending->store(false);
                        if (g_resetVersion.load() != cv) return;
                        auto st = g_state.load();
                        if (st != State::Downed && st != State::DownedInvuln) return;
                        auto& sink = ReviveInputSink::GetSingleton();
                        if (!CanControlAnimation(PlayerCharacter::GetSingleton())) {
                            sink.ResetHold();
                            return;
                        }
                        if (auto* ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) {
                            sink.ResetHold();
                            return;
                        }
                        const float needed = Settings::GetSingleton().buttonTimerForRevival.load();
                        const float held = sink.IsHolding() && sink.GetHoldCV() == cv ? sink.GetHoldDuration() : 0.0f;
                        g_prompts[0].progress = std::clamp(held / needed, 0.0f, 1.0f);
                        if (g_skyPromptShowing.load()) (void)SkyPromptAPI::SendPrompt(&g_promptSink, g_skyPromptClientID);
                        if (held >= needed) {
                            sink.ResetHold();
                            DoSelfRevive();
                        }
                    });
                    else pending->store(false);
                }
            }).detach();
        }
    }

    bool IsPlayerBeastRace()
    {
        auto* player = PlayerCharacter::GetSingleton();
        if (!player) return false;
        auto* race = player->GetRace();
        if (!race) return false;
        std::string raceName = race->GetFullName();
        if (raceName.empty()) return false;
        std::string lower;
        lower.reserve(raceName.size());
        for (char c : raceName)
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        bool match = lower.find("were") != std::string::npos ||
                     lower.find("lycan") != std::string::npos ||
                     lower.find("vampire lord") != std::string::npos;
        static std::string lastRace;
        static bool lastMatch = false;
        if (raceName != lastRace || match != lastMatch) {
            if (Settings::GetSingleton().debug.load())
                SKSE::log::debug("[SkyrimDBNO] IsPlayerBeastRace: race='{}' match={}", raceName, match);
            lastRace = raceName;
            lastMatch = match;
        }
        return match;
    }

    namespace
    {
        void SetPlayerGodMode(bool a_enable)
        {
            static REL::Relocation<bool*> godMode{ RELOCATION_ID(517711, 404238) };
            *godMode = a_enable;
        }
    }

    void ApplyInvulnGodMode()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;
        if (RE::PlayerCharacter::IsGodMode()) return;
        SetPlayerGodMode(true);
        g_invulnGodModeByUs.store(true);
        if (Settings::GetSingleton().debug.load())
            SKSE::log::debug("[SkyrimDBNO] god mode ON");
    }

    void ClearInvulnGodMode()
    {
        if (!g_invulnGodModeByUs.exchange(false)) return;
        SetPlayerGodMode(false);
        if (Settings::GetSingleton().debug.load())
            SKSE::log::debug("[SkyrimDBNO] god mode OFF");
    }

    void DeathReset()
    {
        RestoreSneakControls();
        const auto oldState = g_state.load();
        if (oldState == State::Downed || oldState == State::DownedInvuln || oldState == State::Reviving) {
            if (auto* controls = ControlMap::GetSingleton()) {
                controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kActivate, true, true);
                controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kJumping, true, true);
                controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kMainFour, true, true);
                controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kPOVSwitch, true, true);
            }
        }

        if (auto* plr = PlayerCharacter::GetSingleton()) {
            plr->RemoveAnimationGraphEventSink(&BleedoutEventHandler::GetSingleton());
        }
        g_skyPromptShowing.store(false);
        g_skyPromptActive.store(false);
        ReviveInputSink::GetSingleton().ResetHold();
        DamageHook::SetDownedInvuln(false);
        DamageHook::SetPostReviveInvuln(false);
        ClearInvulnGodMode();
        g_resetVersion.fetch_add(1);
        g_postReviveVersion.fetch_add(1);
        g_state.store(State::Idle);
        if (auto* ti = SKSE::GetTaskInterface())
            ti->AddTask([]() { SkyPromptAPI::RemovePrompt(&g_promptSink, g_skyPromptClientID); });
        if (Settings::GetSingleton().debug.load())
            SKSE::log::debug("[SkyrimDBNO] DeathReset: all state cleared");
    }

    void KillPlayerNow()
    {
        ClearInvulnGodMode();
        if (IsPlayerBeastRace()) return;
        auto* p = PlayerCharacter::GetSingleton();
        if (!p) return;
        g_deathPending.store(true);
        g_deathPendingSince.store(std::chrono::steady_clock::now());
        g_state.store(State::Reviving);
        p->RemoveAnimationGraphEventSink(&BleedoutEventHandler::GetSingleton());
        g_skyPromptShowing.store(false);
        g_skyPromptActive.store(false);
        if (Settings::GetSingleton().debug.load()) SKSE::log::debug("[SkyrimDBNO] KillPlayerNow");
        auto* cmap = ControlMap::GetSingleton();
        cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kActivate, true, true);
        cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kJumping, true, true);
        cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kMainFour, true, true);
        cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kPOVSwitch, true, true);
        p->SetLifeState(ACTOR_LIFE_STATE::kAlive);
        p->AsActorValueOwner()->DamageActorValue(ActorValue::kHealth, 999999.0f);
        if (g_wasFirstPerson) {
            PlayerCamera::GetSingleton()->ForceFirstPerson();
        }
        DeathReset();
    }

    bool AreAnyAlliesStillFighting()
    {
        auto* plr = PlayerCharacter::GetSingleton();
        if (!plr) return false;

        auto* lst = ProcessLists::GetSingleton();
        if (!lst) return false;

        std::unordered_set<FormID> seen;

        auto check = [&](ActorHandle h) -> bool {
            auto a = h.get().get();
            if (!a || a->IsPlayerRef()) return false;
            if (!seen.insert(a->GetFormID()).second) return false;
            if (!IsFriendlyActor(a)) return false;
            if (!IsActorValid(h)) return false;
            return true;
        };

        for (auto& h : lst->highActorHandles)
            if (check(h)) return true;
        for (auto& h : lst->middleHighActorHandles)
            if (check(h)) return true;

        return false;
    }

    void CacheFormPointers()
    {
        g_bleedoutStart.store(TESForm::LookupByEditorID<TESIdleForm>("BleedoutStart"));
        g_bleedoutStop.store(TESForm::LookupByEditorID<TESIdleForm>("BleedoutStop"));
        SKSE::log::info("[SkyrimDBNO] Bleedout idles: start={}, stop={}",
            g_bleedoutStart.load() != nullptr, g_bleedoutStop.load() != nullptr);
        auto* dh = TESDataHandler::GetSingleton();
        g_currentFollowerFaction.store(dh->LookupForm<TESFaction>(0x0005C84E, "Skyrim.esm"));
        g_bloodDebtGlobal.store(dh->LookupForm<TESGlobal>(0x800, "BloodDebt.esp"));
        if (g_bloodDebtGlobal.load())
            SKSE::log::info("[SkyrimDBNO] BloodDebt compatibility enabled");

        g_prompts[0].text = Strings::Get("$PSR_RevivePrompt$", "Hold to Self Revive");

        g_skyPromptClientID = SkyPromptAPI::RequestClientID();
        if (g_skyPromptClientID == 0)
            SKSE::log::warn("[SkyrimDBNO] SkyPrompt not available, prompts disabled");
        else {
            SKSE::log::info("[SkyrimDBNO] SkyPrompt client ID: {}", g_skyPromptClientID);
            (void)SkyPromptAPI::RequestTheme(g_skyPromptClientID, "SkyrimDBNO");
        }
    }

    void BuildEditorIDMap()
    {
        g_formIDToEditorID.clear();
        const auto& [map, lock] = TESForm::GetAllFormsByEditorID();
        BSReadLockGuard l{ lock };
        if (map) {
            for (auto& [key, form] : *map) {
                if (form) g_formIDToEditorID[form->GetFormID()] = key.c_str();
            }
        }
        SKSE::log::info("[SkyrimDBNO] EditorID map built: {} entries", g_formIDToEditorID.size());
    }

    void CacheIngestibleItems()
    {
        auto* dh = TESDataHandler::GetSingleton();
        if (!dh) return;
        auto& arr = dh->GetFormArray<AlchemyItem>();
        g_ingestibleCache.clear();
        g_ingestibleCache.reserve(arr.size());
        for (auto* form : arr) {
            if (!form) continue;
            if (form->IsPoison()) continue;
            IngestibleEntry entry;
            entry.name = form->GetName();
            if (entry.name.empty()) entry.name = GetEditorID(form);
            if (entry.name.empty()) continue;
            entry.editorID = GetEditorID(form);
            entry.formID = form->GetFormID();
            g_ingestibleCache.push_back(std::move(entry));
        }
        std::sort(g_ingestibleCache.begin(), g_ingestibleCache.end(), [](const IngestibleEntry& a, const IngestibleEntry& b) {
            return a.name < b.name;
        });
        g_ingestibleCacheBuilt.store(true);
        SKSE::log::info("[SkyrimDBNO] Ingestible cache built: {} items", g_ingestibleCache.size());
    }

    const std::vector<IngestibleEntry>& GetIngestibleCache()
    {
        return g_ingestibleCache;
    }

    void CacheMiscItems()
    {
        auto* dh = TESDataHandler::GetSingleton();
        if (!dh) return;
        auto& arr = dh->GetFormArray<TESObjectMISC>();
        g_miscCache.clear();
        g_miscCache.reserve(arr.size());
        for (auto* form : arr) {
            if (!form) continue;
            MiscEntry entry;
            entry.name = form->GetName();
            if (entry.name.empty()) continue;
            entry.editorID = GetEditorID(form);
            entry.formID = form->GetFormID();
            g_miscCache.push_back(std::move(entry));
        }
        std::sort(g_miscCache.begin(), g_miscCache.end(), [](const MiscEntry& a, const MiscEntry& b) {
            return a.name < b.name;
        });
        g_miscCacheBuilt.store(true);
        SKSE::log::info("[SkyrimDBNO] Misc cache built: {} items", g_miscCache.size());
    }

    const std::vector<MiscEntry>& GetMiscCache()
    {
        return g_miscCache;
    }

    bool MatchesRequiredEntry(TESForm* obj, const std::string& entry)
    {
        size_t pipePos = entry.find('|');
        std::string_view storedEdid(entry.data(), pipePos != std::string::npos ? pipePos : 0);
        std::string_view storedName;
        if (pipePos != std::string::npos) {
            storedName = std::string_view(entry.data() + pipePos + 1, entry.size() - pipePos - 1);
        } else {
            storedName = std::string_view(entry);
        }
        const std::string& formEdid = GetEditorID(obj);
        const char* formName = obj->GetName();
        if (!storedEdid.empty() && !formEdid.empty() && storedEdid == formEdid) return true;
        if (formName && storedName == formName) return true;
        return false;
    }

    TESIdleForm* GetBleedoutStart()
    {
        return g_bleedoutStart.load();
    }

    TESIdleForm* GetBleedoutStop()
    {
        return g_bleedoutStop.load();
    }

    bool DoesPlayerHaveChosenConsumable()
    {
        auto* plr = PlayerCharacter::GetSingleton();
        if (!plr) return false;
        auto& s = Settings::GetSingleton();
        if (!s.consumableRequiredForRevival.load()) return true;
        if (s.requiredConsumableEditorIDs.empty()) return false;
        for (auto& [obj, cnt] : plr->GetInventoryCounts()) {
            if (!obj || cnt <= 0) continue;
            auto ft = obj->GetFormType();
            if (ft != FormType::AlchemyItem && ft != FormType::Misc) continue;
            if (ft == FormType::AlchemyItem) {
                auto* al = static_cast<AlchemyItem*>(obj);
                if (al->IsPoison()) continue;
            }
            for (const auto& req : s.requiredConsumableEditorIDs) {
                if (MatchesRequiredEntry(obj, req)) return true;
            }
        }
        return false;
    }

    TESForm* PickRandomRequiredItem()
    {
        auto* plr = PlayerCharacter::GetSingleton();
        if (!plr) return nullptr;
        auto& s = Settings::GetSingleton();
        if (s.requiredConsumableEditorIDs.empty()) return nullptr;
        std::vector<TESForm*> available;
        for (auto& [obj, cnt] : plr->GetInventoryCounts()) {
            if (!obj || cnt <= 0) continue;
            auto ft = obj->GetFormType();
            if (ft != FormType::AlchemyItem && ft != FormType::Misc) continue;
            if (ft == FormType::AlchemyItem) {
                auto* al = static_cast<AlchemyItem*>(obj);
                if (al->IsPoison()) continue;
            }
            for (const auto& req : s.requiredConsumableEditorIDs) {
                if (MatchesRequiredEntry(obj, req)) {
                    available.push_back(obj);
                    break;
                }
            }
        }
        if (available.empty()) return nullptr;
        static std::mt19937 rng(std::random_device{}());
        return available[std::uniform_int_distribution<size_t>(0, available.size() - 1)(rng)];
    }

    bool PlayerCanBleedOut()
    {
        auto& s = Settings::GetSingleton();
        if (!s.consumableRequiredForRevival.load()) return true;
        if (s.requiredConsumableEditorIDs.empty()) return true;
        return DoesPlayerHaveChosenConsumable();
    }

    DeathEventHandler& DeathEventHandler::GetSingleton()
    {
        static DeathEventHandler instance;
        return instance;
    }

    BSEventNotifyControl DeathEventHandler::ProcessEvent(const TESDeathEvent* a_event, BSTEventSource<TESDeathEvent>*)
    {
        if (a_event && a_event->actorDying && a_event->actorDying->IsPlayerRef()) {
            g_deathPending.store(false);
            DeathReset();
        }
        return BSEventNotifyControl::kContinue;
    }

    void Register()
    {
        auto* inputSource = BSInputDeviceManager::GetSingleton();
        if (inputSource)
            inputSource->AddEventSink(&ReviveInputSink::GetSingleton());
        SKSE::log::info("[SkyrimDBNO] InputEventSink registered");

        auto* holder = ScriptEventSourceHolder::GetSingleton();
        if (holder)
            holder->AddEventSink(&DeathEventHandler::GetSingleton());
        SKSE::log::info("[SkyrimDBNO] DeathEventSink registered");
    }

    BleedoutEventHandler& BleedoutEventHandler::GetSingleton()
    {
        static BleedoutEventHandler instance;
        return instance;
    }

    BSEventNotifyControl BleedoutEventHandler::ProcessEvent(const BSAnimationGraphEvent* a_event, BSTEventSource<BSAnimationGraphEvent>*)
    {
        if (Settings::GetSingleton().disableMod.load()) return BSEventNotifyControl::kContinue;
        if (IsPlayerBeastRace()) return BSEventNotifyControl::kContinue;
        if (!a_event) return BSEventNotifyControl::kContinue;
        auto st = g_state.load();
        if (st != State::DownedInvuln && st != State::Downed)
            return BSEventNotifyControl::kContinue;
        if (a_event->tag == BSFixedString("MTState")) g_repairAnimation.store(true);
        return BSEventNotifyControl::kContinue;
    }

    void OnLoadOrNewGame()
    {
        RestoreSneakControls();
        g_deathTransition.store(false);
        if (auto* plr = PlayerCharacter::GetSingleton()) {
            plr->RemoveAnimationGraphEventSink(&BleedoutEventHandler::GetSingleton());
        }
        g_skyPromptShowing.store(false);
        g_skyPromptActive.store(false);
        if (auto* ti = SKSE::GetTaskInterface())
            ti->AddTask([]() { SkyPromptAPI::RemovePrompt(&g_promptSink, g_skyPromptClientID); });
        ReviveInputSink::GetSingleton().ResetHold();
        DamageHook::SetDownedInvuln(false);
        DamageHook::SetPostReviveInvuln(false);
        ClearInvulnGodMode();
        g_deathPending.store(false);
        g_resetVersion.fetch_add(1);
        g_postReviveVersion.fetch_add(1);
        g_state.store(State::Idle);
        if (Settings::GetSingleton().disableMod.load()) {
            SKSE::log::info("[SkyrimDBNO] OnLoadOrNewGame: disableMod active, state reset only");
            return;
        }
        if (Settings::GetSingleton().debug.load())
            SKSE::log::debug("[SkyrimDBNO] OnLoadOrNewGame: state reset to Idle");
    }

    void CheckStateAfterLoad()
    {
        if (g_checkStatePending.exchange(false) || IsPlayerBeastRace()) return;
        if (Settings::GetSingleton().debug.load())
            SKSE::log::debug("[SkyrimDBNO] CheckStateAfterLoad: retry started, 25x200ms");
        const int cv = g_resetVersion.load();
        std::thread([cv]() {
            for (int i = 0; i < 25; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                if (g_resetVersion.load() != cv) return;
                if (auto* ti = SKSE::GetTaskInterface())
                    ti->AddTask([cv, i]() {
                        if (g_resetVersion.load() != cv) return;
                        auto* p = PlayerCharacter::GetSingleton();
                        if (!CanControlAnimation(p) || g_state.load() != State::Idle) return;
                        auto* idle = g_bleedoutStop.load();
                        if (auto* proc = p->GetActorRuntimeData().currentProcess) {
                            proc->PlayIdle(p, idle, p);
                            if (Settings::GetSingleton().debug.load())
                                SKSE::log::debug("[SkyrimDBNO] CheckStateAfterLoad iter {}: BleedoutStop PlayIdle sent, proc={}, idle={}", i, static_cast<void*>(proc), static_cast<void*>(idle));
                        }
                        if (i == 24) {
                            g_postLoadPending.store(false);
                            if (Settings::GetSingleton().debug.load())
                                SKSE::log::debug("[SkyrimDBNO] CheckStateAfterLoad: retry complete, g_postLoadPending cleared");
                        }
                    });
            }
        }).detach();
    }

    void OnPostLoadGame()
    {
        g_state.store(State::Idle);
        g_postLoadPending.store(true);
        auto* plr = PlayerCharacter::GetSingleton();
        if (!plr) return;
        DamageHook::SetDownedInvuln(false);
        DamageHook::SetPostReviveInvuln(false);
        ClearInvulnGodMode();
        g_deathPending.store(false);

        plr->RemoveAnimationGraphEventSink(&BleedoutEventHandler::GetSingleton());

        if (plr->IsDead()) return;
        plr->SetLifeState(ACTOR_LIFE_STATE::kAlive);

        auto* cmap = ControlMap::GetSingleton();
        if (cmap) {
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kActivate, true, true);
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kJumping, true, true);
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kMainFour, true, true);
            cmap->ToggleControls(UserEvents::USER_EVENT_FLAG::kPOVSwitch, true, true);
            if (Settings::GetSingleton().debug.load())
                SKSE::log::debug("[SkyrimDBNO] OnPostLoadGame: controls re-enabled");
        }

        g_skyPromptShowing.store(false);
        g_skyPromptActive.store(false);
        if (auto* ti = SKSE::GetTaskInterface())
            ti->AddTask([]() { SkyPromptAPI::RemovePrompt(&g_promptSink, g_skyPromptClientID); });
        ReviveInputSink::GetSingleton().ResetHold();
        if (Settings::GetSingleton().debug.load())
            SKSE::log::debug("[SkyrimDBNO] OnPostLoadGame: sky prompt removed");

        if (IsPlayerBeastRace()) return;

        if (g_wasFirstPerson) {
            PlayerCamera::GetSingleton()->ForceFirstPerson();
            if (Settings::GetSingleton().debug.load())
                SKSE::log::debug("[SkyrimDBNO] OnPostLoadGame: camera restored to first person (wasFirstPerson=1)");
        }

        CheckStateAfterLoad();

        if (Settings::GetSingleton().debug.load())
            SKSE::log::debug("[SkyrimDBNO] OnPostLoadGame: player fully reset");
    }

    bool CanControlAnimation(Actor* actor)
    {
        if (!actor) return false;
        const auto life = actor->AsActorState()->GetLifeState();
        return DBNORules::CanControlAnimation(actor->IsDead() || life == ACTOR_LIFE_STATE::kDying || life == ACTOR_LIFE_STATE::kDead,
            actor->IsInKillMove(), g_deathTransition.load(), actor->AsActorValueOwner()->GetActorValue(ActorValue::kHealth));
    }

    void EnterBleedout(Actor* player)
    {
        auto& settings = Settings::GetSingleton();
        if (!CanControlAnimation(player) ||
            settings.disableMod.load() || IsPlayerBeastRace() || g_state.load() != State::Idle) return;
        if (!player->IsInCombat() && !settings.allowSelfReviveOutOfCombat.load()) {
            SKSE::log::info("[SkyrimDBNO] Threshold entry skipped: out-of-combat DBNO is disabled");
            return;
        }

        const int cv = g_resetVersion.fetch_add(1) + 1;
        ClearDownedSneaking(player);
        // Invalidate any delayed post-revive/swimming cleanup from the previous episode.
        g_postReviveVersion.fetch_add(1);
        const float duration = settings.invulnerabilityDurationSeconds.load();
        g_state.store(duration > 0.0f ? State::DownedInvuln : State::Downed);
        // Also cover entry from the damage callback, between player updates.
        player->UpdateRegenDelay(ActorValue::kHealth,
            DBNORules::DownedHealthRegenDelay(player->GetRegenDelay(ActorValue::kHealth), 0.0f));
        g_postLoadPending.store(false);
        ReviveInputSink::GetSingleton().ResetHold();
        g_repairAnimation.store(true);
        DamageHook::SetDownedInvuln(duration > 0.0f);
        if (duration > 0.0f) ApplyInvulnGodMode();
        player->AddAnimationGraphEventSink(&BleedoutEventHandler::GetSingleton());
        if (auto* camera = PlayerCamera::GetSingleton()) {
            g_wasFirstPerson = camera->IsInFirstPerson();
            camera->ForceThirdPerson();
        }
        if (auto* controls = ControlMap::GetSingleton()) {
            // Movement is intentionally left enabled, as in the base mod.
            g_sneakControlsDisabledByUs.store(controls->IsSneakingControlsEnabled());
            controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kSneaking, false, true);
            controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kActivate, false, true);
            controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kJumping, false, true);
            controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kMainFour, false, true);
            controls->ToggleControls(UserEvents::USER_EVENT_FLAG::kPOVSwitch, false, true);
        }
        ShowSkyPrompt();
        StartHoldMonitor();

        // All actor/graph access happens on the game thread. The worker only queues ticks.
        const auto started = std::chrono::steady_clock::now();
        auto lastRepair = std::make_shared<std::chrono::steady_clock::time_point>();
        auto pending = std::make_shared<std::atomic<bool>>(false);
        std::thread([cv, duration, started, lastRepair, pending]() {
            while (g_resetVersion.load() == cv) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (pending->exchange(true)) continue;
                if (auto* ti = SKSE::GetTaskInterface()) ti->AddTask([cv, duration, started, lastRepair, pending]() {
                    pending->store(false);
                    if (g_resetVersion.load() != cv) return;
                    auto* p = PlayerCharacter::GetSingleton();
                    if (!p) return;
                    if (p->IsDead()) { DeathReset(); return; }
                    // Wait for engine death without replaying bleedout or erasing its origin.
                    if (p->AsActorValueOwner()->GetActorValue(ActorValue::kHealth) <= 0.0f) return;
                    if (!CanControlAnimation(p)) {
                        g_repairAnimation.store(true);
                        ReviveInputSink::GetSingleton().ResetHold();
                        return;
                    }
                    if (Settings::GetSingleton().disableMod.load()) {
                        if (auto* proc = p->GetActorRuntimeData().currentProcess; proc && g_bleedoutStop.load())
                            proc->PlayIdle(p, g_bleedoutStop.load(), p);
                        p->SetLifeState(ACTOR_LIFE_STATE::kAlive);
                        DeathReset();
                        return;
                    }
                    if (auto* ui = RE::UI::GetSingleton(); ui && ui->GameIsPaused()) return;

                    auto st = g_state.load();
                    if (st != State::Downed && st != State::DownedInvuln) return;
                    const auto now = std::chrono::steady_clock::now();
                    if (st == State::DownedInvuln && std::chrono::duration<float>(now - started).count() >= duration) {
                        DamageHook::SetDownedInvuln(false);
                        ClearInvulnGodMode();
                        g_state.store(State::Downed);
                    }
                    auto* state = p->AsActorState();
                    if (!p->Is3DLoaded() || p->IsInRagdollState() || (state && state->IsStaggered())) {
                        g_repairAnimation.store(true);
                        return;
                    }
                    if (ClearDownedSneaking(p)) g_repairAnimation.store(true);
                    if (g_repairAnimation.load() && now - *lastRepair >= std::chrono::milliseconds(750) && CanControlAnimation(p)) {
                        if (auto* proc = p->GetActorRuntimeData().currentProcess; proc && g_bleedoutStart.load()) {
                            g_repairAnimation.store(false);
                            *lastRepair = now;
                            const bool played = proc->PlayIdle(p, g_bleedoutStart.load(), p);
                            // Retry rejected idles; ignore synchronous events from a successful one.
                            g_repairAnimation.store(!played);
                            if (Settings::GetSingleton().debug.load())
                                SKSE::log::debug("[SkyrimDBNO] BleedoutStart animation accepted={}", played);
                        }
                    }
                });
                else pending->store(false);
            }
        }).detach();
        SKSE::log::info("[SkyrimDBNO] Downed at {} health; threshold {}%; protection {}s",
            player->AsActorValueOwner()->GetActorValue(ActorValue::kHealth), settings.bleedoutThresholdPercent.load(), duration);
    }
}
