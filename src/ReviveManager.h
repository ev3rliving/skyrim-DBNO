#pragma once

namespace RE
{
    struct BSAnimationGraphEvent;
    class InputEvent;
}

namespace ReviveManager
{
    enum class State
    {
        Idle,
        DownedInvuln,
        Downed,
        Reviving
    };

    struct IngestibleEntry
    {
        std::string name;
        std::string editorID;
        RE::FormID  formID;
    };

    struct MiscEntry
    {
        std::string name;
        std::string editorID;
        RE::FormID  formID;
    };

    void CacheFormPointers();
    void BuildEditorIDMap();
    void CacheIngestibleItems();
    void CacheMiscItems();
    const std::vector<IngestibleEntry>& GetIngestibleCache();
    const std::vector<MiscEntry>& GetMiscCache();
    RE::TESIdleForm* GetBleedoutStart();
    RE::TESIdleForm* GetBleedoutStop();
    void OnLoadOrNewGame();
    void OnPostLoadGame();
    void CheckStateAfterLoad();
    void EnterBleedout(RE::Actor* a_player);
    bool CanControlAnimation(RE::Actor* actor);
    void KillPlayerNow();
    void DeathReset();
    bool DoesPlayerHaveChosenConsumable();
    bool PlayerCanBleedOut();
    RE::TESForm* PickRandomRequiredItem();
    bool AreAnyAlliesStillFighting();
    void Register();
    bool IsPlayerBeastRace();
    void ApplyInvulnGodMode();
    void ClearInvulnGodMode();

    inline std::atomic<State> g_state{ State::Idle };
    inline std::atomic<int>   g_resetVersion{ 0 };
    inline std::atomic<bool>  g_postLoadPending{ false };
    inline std::atomic<bool>  g_deathTransition{ false };

    inline State GetState() { return g_state.load(); }

    class BleedoutEventHandler :
        public RE::BSTEventSink<RE::BSAnimationGraphEvent>
    {
    public:
        static BleedoutEventHandler& GetSingleton();
        RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;
    };

    class DeathEventHandler :
        public RE::BSTEventSink<RE::TESDeathEvent>
    {
    public:
        static DeathEventHandler& GetSingleton();
        RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override;
    };
}
