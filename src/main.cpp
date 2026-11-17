#include "pch.h"

#include "DamageHook.h"
#include "Hooks.h"
#include "ReviveManager.h"
#include "Settings.h"
#include "Strings.h"
#include "UI.h"
#include "RuntimeCompatibility.h"

#include "plugin.h"

// Optional VCD integration; game-thread query, no ownership or C++ ABI crossing.
extern "C" __declspec(dllexport) bool SkyrimDBNO_IsCrawlingV1() noexcept
{
    using State = ReviveManager::State;
    const auto state = ReviveManager::GetState();
    if (Settings::GetSingleton().disableMod.load() ||
        ReviveManager::g_postLoadPending.load() ||
        (state != State::Downed && state != State::DownedInvuln && state != State::Reviving)) return false;
    auto* player = RE::PlayerCharacter::GetSingleton();
    return player && !ReviveManager::IsPlayerBeastRace() &&
        ReviveManager::CanControlAnimation(player);
}

namespace
{
    void InitLogger()
    {
        auto path = SKSE::log::log_directory();
        if (!path) return;

        *path /= std::format("{}.log", Plugin::NAME);

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log  = std::make_shared<spdlog::logger>(std::string(Plugin::NAME), std::move(sink));
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(log));
    }

    void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
    {
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
            Settings::GetSingleton().Load();
            Settings::GetSingleton().ApplyLogLevel();
            if (!RuntimeCompatibility::ValidatePlayerInterfaces()) return;
            Strings::Load();
            ReviveManager::CacheFormPointers();
            ReviveManager::BuildEditorIDMap();
            ReviveManager::CacheIngestibleItems();
            ReviveManager::CacheMiscItems();
            ReviveManager::Register();
            DamageHook::Install();
            Hooks::Install();
            UI::Register();

            SKSE::log::info("[SkyrimDBNO] v{} loaded. Debug={}, HealthPct={:.0f}%, HoldTimer={:.1f}s",
                Plugin::VERSION,
                Settings::GetSingleton().debug.load(),
                Settings::GetSingleton().bleedoutThresholdPercent.load(),
                Settings::GetSingleton().buttonTimerForRevival.load());
        }

        if (a_msg->type == SKSE::MessagingInterface::kPreLoadGame) {
            ReviveManager::OnLoadOrNewGame();
        }

        if (a_msg->type == SKSE::MessagingInterface::kPostLoadGame) {
            ReviveManager::OnPostLoadGame();
        }
        if (a_msg->type == SKSE::MessagingInterface::kNewGame) {
            ReviveManager::OnLoadOrNewGame();
        }
    }
}

SKSEPluginVersion = []() noexcept {
    SKSE::PluginVersionData v{};
    v.PluginVersion(REL::Version{ 1, 0, 2, 0 });
    v.PluginName("SkyrimDBNO");
    v.AuthorName("Skyrim DBNO contributors; based on Player Self Revive");
    v.UsesAddressLibrary();
    v.UsesNoStructs();
    v.versionIndependenceEx |= SKSE::PluginVersionData::kVersionIndependentEx_AddressLibraryV5;
    return v;
}();

// SKSE 2.0.20 requires Query before Load; newer loaders use the version export.
// Keep this free of runtime relocations: SKSE::Init has not run yet.
SKSE_EXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = Plugin::NAME.data();
    a_info->version = REL::Version{ 1, 0, 2, 0 }.pack();
    return !a_skse->IsEditor() && a_skse->RuntimeVersion() >= REL::Version{ 1, 5, 97, 0 };
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    InitLogger();

    SKSE::log::info("[SkyrimDBNO] {} v{}", Plugin::NAME, Plugin::VERSION);

    SKSE::Init(a_skse);

    SKSE::log::info("[SkyrimDBNO] Runtime {}; family {}; Address Library v5 support enabled",
        a_skse->RuntimeVersion().string(), REL::Module::IsAE() ? "AE (1.6/1.7+)" : "SE");

    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

    return true;
}
