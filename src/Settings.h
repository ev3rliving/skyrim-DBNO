#pragma once

#include "plugin.h"

class Settings
{
public:
    static Settings& GetSingleton();

    Settings(const Settings&)            = delete;
    Settings& operator=(const Settings&) = delete;

    void Load();
    void Save();
    void ApplyLogLevel() const;

    std::atomic<bool>     disableMod{ false };
    std::atomic<bool>     debug{ false };
    std::atomic<bool>     ignoreStaggerWhileDowned{ true };
    std::atomic<bool>     ragdollOnDeathWhileDowned{ true };
    std::atomic<float>    bleedoutThresholdPercent{ 25.0f };
    std::atomic<float>    playerHealthPercentUponRevival{ 25.0f };
    std::atomic<float>    buttonTimerForRevival{ 10.0f };
    std::atomic<float>    invulnerabilityDurationSeconds{ 0.0f };
    std::atomic<uint32_t> keyboardHotkey{ 47 };
    std::atomic<uint32_t> gamepadHotkey{ 276 };
    std::atomic<bool>     consumableRequiredForRevival{ false };
    std::atomic<bool>     allowSelfReviveOutOfCombat{ true };
    std::vector<std::string> requiredConsumableEditorIDs;

    void Clamp();

private:
    Settings() = default;
};
