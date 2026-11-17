#include "pch.h"
#include "Settings.h"
#include "DBNORules.h"

Settings& Settings::GetSingleton()
{
    static Settings instance;
    return instance;
}

void Settings::Load()
{
    constexpr auto path = L"Data/SKSE/Plugins/SkyrimDBNO.ini";

    CSimpleIniA ini;
    ini.SetUnicode();
    ini.LoadFile(path);

    disableMod.store(ini.GetBoolValue("General", "disableMod", false));
    debug.store(ini.GetBoolValue("General", "debug", false));
    ignoreStaggerWhileDowned.store(ini.GetBoolValue("General", "ignoreStaggerWhileDowned", true));
    ragdollOnDeathWhileDowned.store(ini.GetBoolValue("General", "ragdollOnDeathWhileDowned", true));

    float hpPct = static_cast<float>(ini.GetDoubleValue("General", "bleedoutThresholdPercent", 25.0));
    float hpRevivePct = static_cast<float>(ini.GetDoubleValue("General", "playerHealthPercentUponRevival", 25.0));
    float timer = static_cast<float>(ini.GetDoubleValue("General", "buttonTimerForRevival", 10.0));
    float invulnDur = static_cast<float>(ini.GetDoubleValue("General", "invulnerabilityDurationSeconds", 0.0));
    uint32_t kb = static_cast<uint32_t>(ini.GetLongValue("General", "keyboardHotkey", 47));
    uint32_t gp = static_cast<uint32_t>(ini.GetLongValue("General", "gamepadHotkey", 276));
    bool consumableReq = ini.GetBoolValue("General", "consumableRequiredForRevival", false);
    bool selfReviveOOC = ini.GetBoolValue("General", "allowSelfReviveOutOfCombat", true);

    bleedoutThresholdPercent.store(hpPct);
    playerHealthPercentUponRevival.store(hpRevivePct);
    buttonTimerForRevival.store(timer);
    invulnerabilityDurationSeconds.store(invulnDur);
    keyboardHotkey.store(kb);
    gamepadHotkey.store(gp);
    consumableRequiredForRevival.store(consumableReq);
    allowSelfReviveOutOfCombat.store(selfReviveOOC);

    requiredConsumableEditorIDs.clear();
    const char* raw = ini.GetValue("General", "requiredConsumables", "");
    if (raw && raw[0] != '\0') {
        std::string rawStr(raw);
        std::istringstream stream(rawStr);
        std::string token;
        bool hasOldFormat = false;
        while (std::getline(stream, token, ',')) {
            auto start = token.find_first_not_of(" \t");
            auto end = token.find_last_not_of(" \t");
            if (start == std::string::npos) continue;
            std::string trimmed = token.substr(start, end - start + 1);
            if (trimmed.find('|') == std::string::npos) { hasOldFormat = true; break; }
            requiredConsumableEditorIDs.push_back(trimmed);
        }
        if (hasOldFormat) {
            requiredConsumableEditorIDs.clear();
            SKSE::log::info("[SkyrimDBNO] Old consumable format detected, cleared list");
        }
    }

    Clamp();
}

void Settings::Save()
{
    Clamp();
    constexpr auto path = L"Data/SKSE/Plugins/SkyrimDBNO.ini";

    CSimpleIniA ini;
    ini.SetUnicode();

    ini.SetBoolValue("General", "disableMod", disableMod.load());
    ini.SetBoolValue("General", "debug", debug.load());
    ini.SetBoolValue("General", "ignoreStaggerWhileDowned", ignoreStaggerWhileDowned.load());
    ini.SetBoolValue("General", "ragdollOnDeathWhileDowned", ragdollOnDeathWhileDowned.load());
    ini.SetDoubleValue("General", "bleedoutThresholdPercent", static_cast<double>(bleedoutThresholdPercent.load()));
    ini.SetDoubleValue("General", "playerHealthPercentUponRevival", static_cast<double>(playerHealthPercentUponRevival.load()));
    ini.SetDoubleValue("General", "buttonTimerForRevival", static_cast<double>(buttonTimerForRevival.load()));
    ini.SetDoubleValue("General", "invulnerabilityDurationSeconds", static_cast<double>(invulnerabilityDurationSeconds.load()));
    ini.SetLongValue("General", "keyboardHotkey", static_cast<long>(keyboardHotkey.load()));
    ini.SetLongValue("General", "gamepadHotkey", static_cast<long>(gamepadHotkey.load()));
    ini.SetBoolValue("General", "consumableRequiredForRevival", consumableRequiredForRevival.load());
    ini.SetBoolValue("General", "allowSelfReviveOutOfCombat", allowSelfReviveOutOfCombat.load());

    std::string consumableStr;
    for (size_t i = 0; i < requiredConsumableEditorIDs.size(); ++i) {
        if (i > 0) consumableStr += ",";
        consumableStr += requiredConsumableEditorIDs[i];
    }
    ini.SetValue("General", "requiredConsumables", consumableStr.c_str());

    ini.SaveFile(path);
    ApplyLogLevel();
}

void Settings::Clamp()
{
    float hp = DBNORules::ClampSetting(bleedoutThresholdPercent.load(), 1.0f, 100.0f, 25.0f);
    if (hp < 1.0f) hp = 1.0f;
    if (hp > 100.0f) hp = 100.0f;
    bleedoutThresholdPercent.store(hp);

    float hpRevive = DBNORules::ClampSetting(playerHealthPercentUponRevival.load(), 1.0f, 100.0f, 25.0f);
    if (hpRevive < 1.0f) hpRevive = 1.0f;
    if (hpRevive > 100.0f) hpRevive = 100.0f;
    playerHealthPercentUponRevival.store(hpRevive);

    float t = DBNORules::ClampSetting(buttonTimerForRevival.load(), 1.0f, 60.0f, 10.0f);
    if (t < 1.0f) t = 1.0f;
    if (t > 60.0f) t = 60.0f;
    buttonTimerForRevival.store(t);

    float invulnDur = DBNORules::ClampSetting(invulnerabilityDurationSeconds.load(), 0.0f, 60.0f, 0.0f);
    if (invulnDur < 0.0f) invulnDur = 0.0f;
    if (invulnDur > 60.0f) invulnDur = 60.0f;
    invulnerabilityDurationSeconds.store(invulnDur);
}

void Settings::ApplyLogLevel() const
{
    auto log = spdlog::default_logger();
    if (!log) return;

    if (debug.load()) {
        log->set_level(spdlog::level::debug);
        log->flush_on(spdlog::level::debug);
    } else {
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
    }
}
