#include "pch.h"
#include "UI.h"
#include "Settings.h"
#include "Strings.h"
#include "ReviveManager.h"
#include "SKSEMenuFramework.h"

namespace ImGui = ImGuiMCP;
using namespace ImGuiMCP;

namespace
{
    const char* dxKbNames[] = {
        "[NONE]",
        "Escape", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "Minus", "Equals", "Backspace", "Tab",
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
        "Left Bracket", "Right Bracket", "Enter",
        "Left Control", "A", "S", "D", "F", "G", "H", "J", "K", "L",
        "Semicolon", "Apostrophe", "~ (Console)", "Left Shift", "Back Slash",
        "Z", "X", "C", "V", "B", "N", "M",
        "Comma", "Period", "Forward Slash", "Right Shift",
        "NUM*", "Left Alt", "Spacebar", "Caps Lock",
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
        "Num Lock", "Scroll Lock",
        "NUM7", "NUM8", "NUM9", "NUM-", "NUM4", "NUM5", "NUM6", "NUM+",
        "NUM1", "NUM2", "NUM3", "NUM0", "NUM.",
        "F11", "F12",
        "NUM Enter", "Right Control",
        "NUM/",
        "SysRq / PrtScr", "Right Alt",
        "Pause",
        "Home", "Up Arrow", "PgUp", "Left Arrow", "Right Arrow", "End", "Down Arrow", "PgDown",
        "Insert", "Delete",
        "Left Mouse Button", "Right Mouse Button", "Middle/Wheel Mouse Button",
        "Mouse Button 3", "Mouse Button 4", "Mouse Button 5",
        "Mouse Button 6", "Mouse Button 7", "Mouse Wheel Up", "Mouse Wheel Down"
    };

    const uint32_t dxKbValues[] = {
        0,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        39, 40, 41, 42, 43,
        44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54,
        55, 56, 57, 58,
        59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
        69, 70,
        71, 72, 73, 74, 75, 76, 77, 78,
        79, 80, 81, 82, 83,
        87, 88,
        156, 157,
        181,
        183, 184,
        197,
        199, 200, 201, 203, 205, 207, 208, 209,
        210, 211,
        256, 257, 258,
        259, 260, 261,
        262, 263, 264, 265
    };

    const char* dxGpNames[] = {
        "[NONE]",
        "DPAD UP", "DPAD DOWN", "DPAD LEFT", "DPAD RIGHT",
        "START", "BACK",
        "LEFT THUMB", "RIGHT THUMB",
        "LEFT SHOULDER", "RIGHT SHOULDER",
        "A", "B", "X", "Y",
        "LT", "RT"
    };

    const uint32_t dxGpValues[] = {
        0,
        266, 267, 268, 269,
        270, 271,
        272, 273,
        274, 275,
        276, 277, 278, 279,
        280, 281
    };

    int FindComboIndex(const uint32_t* values, size_t count, uint32_t current) {
        for (size_t i = 0; i < count; ++i) {
            if (values[i] == current) return static_cast<int>(i);
        }
        return -1;
    }

    bool ContainsIgnoreCase(std::string_view haystack, std::string_view needle)
    {
        if (needle.empty()) return true;
        auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
            [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
        return it != haystack.end();
    }
}

void UI::HelpMarker(const char* a_desc)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.55f, 0.55f, 0.55f, 1.0f });
    ImGui::TextUnformatted("(?)");
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
        ImGui::TextUnformatted(a_desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void UI::Register()
{
    if (!SKSEMenuFramework::IsInstalled()) {
        SKSE::log::warn("[SkyrimDBNO] SKSE Menu Framework not installed, skipping UI registration");
        return;
    }
    SKSEMenuFramework::SetSection(Strings::Get("$ModName$", "Skyrim DBNO"));
    SKSEMenuFramework::AddSectionItem(Strings::Get("$SettingsLabel$", "Settings"), RenderMenu);
    SKSE::log::info("[SkyrimDBNO] UI registered with SKSE Menu Framework");
}

void __stdcall UI::RenderMenu()
{
    auto& settings = Settings::GetSingleton();
    bool changed = false;

    float winWidth = ImGui::GetWindowWidth();

    bool disableModVal = settings.disableMod.load();
    if (ImGui::Checkbox(Strings::Get("$DisableModCheckbox$", "Disable Mod"), &disableModVal)) {
        settings.disableMod.store(disableModVal);
        changed = true;
    }
    ImGui::SameLine();
    HelpMarker(Strings::Get("$DisableModHelp$", "Completely disables all SkyrimDBNO functionality. Use this to temporarily turn off the mod without uninstalling."));

    bool selfReviveOOCVal = settings.allowSelfReviveOutOfCombat.load();
    if (ImGui::Checkbox(Strings::Get("$SelfReviveOOCCheckbox$", "Allow Self Revive Out of Combat"), &selfReviveOOCVal)) {
        settings.allowSelfReviveOutOfCombat.store(selfReviveOOCVal);
        changed = true;
    }
    ImGui::SameLine();
    HelpMarker(Strings::Get("$SelfReviveOOCHelp$", "When enabled, self-revive works outside combat. When disabled, self-revive only triggers during combat."));

    ImGui::Separator();
    bool ignoreStagger = settings.ignoreStaggerWhileDowned.load();
    if (ImGui::Checkbox(Strings::Get("$IgnoreStaggerDowned$", "Ignore stagger while downed"), &ignoreStagger)) {
        settings.ignoreStaggerWhileDowned.store(ignoreStagger);
        changed = true;
    }
    ImGui::SameLine();
    HelpMarker(Strings::Get("$IgnoreStaggerDownedHelp$", "Suppress ordinary stagger-start animation requests while downed. Damage and death still work normally. Does not block full knockdowns or ragdolls. Takes effect immediately for new stagger requests."));

    ImGui::Separator();
    bool ragdollDeath = settings.ragdollOnDeathWhileDowned.load();
    if (ImGui::Checkbox(Strings::Get("$RagdollDownedDeath$", "Ragdoll on death while downed"), &ragdollDeath)) {
        settings.ragdollOnDeathWhileDowned.store(ragdollDeath);
        changed = true;
    }
    ImGui::SameLine();
    HelpMarker(Strings::Get("$RagdollDownedDeathHelp$", "Ask the engine to ragdoll immediately instead of playing a death animation when you die during bleedout. Deaths while standing use their normal behavior. Does not change damage or prevent death."));

    ImGui::Separator();
    ImGui::TextUnformatted(Strings::Get("$PSR_SectionDebug$", "-- Debug --"));
    ImGui::Separator();

    bool debugVal = settings.debug.load();
    if (ImGui::Checkbox(Strings::Get("$DebugCheckbox$", "Debug"), &debugVal)) {
        settings.debug.store(debugVal);
        changed = true;
    }
    ImGui::SameLine();
    HelpMarker(Strings::Get("$DebugHelp$", "Enables detailed log output for troubleshooting."));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted(Strings::Get("$PSR_SectionHealth$", "-- Health --"));
    ImGui::Separator();

    ImGui::TextUnformatted(Strings::Get("$HealthPercentSlider$", "Bleedout Health Threshold"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$HealthPercentHelp$", "Enter bleedout after surviving damage at or below this percentage of maximum health. Lethal hits still kill. Remaining health is preserved."));
    ImGui::SetNextItemWidth(winWidth * 0.45f);
    float hpPct = settings.bleedoutThresholdPercent.load();
    if (ImGui::SliderFloat("##HealthPercentSlider", &hpPct, 1.0f, 100.0f, "%.0f%%")) {
        settings.bleedoutThresholdPercent.store(hpPct);
        changed = true;
    }

    ImGui::Spacing();

    ImGui::TextUnformatted(Strings::Get("$HealthRevivePercentSlider$", "Player Health % upon Revival"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$HealthRevivePercentHelp$", "Percentage of max health the player is restored to after reviving."));
    ImGui::SetNextItemWidth(winWidth * 0.45f);
    float hpRevivePct = settings.playerHealthPercentUponRevival.load();
    if (ImGui::SliderFloat("##HealthRevivePercentSlider", &hpRevivePct, 1.0f, 100.0f, "%.0f%%")) {
        settings.playerHealthPercentUponRevival.store(hpRevivePct);
        changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted(Strings::Get("$PSR_SectionTimer$", "-- Timer --"));
    ImGui::Separator();

    ImGui::TextUnformatted(Strings::Get("$ButtonTimerSlider$", "Button Timer for Revival"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$ButtonTimerHelp$", "How many seconds you must hold the hotkey to revive."));
    ImGui::SetNextItemWidth(winWidth * 0.45f);
    float timer = settings.buttonTimerForRevival.load();
    if (ImGui::SliderFloat("##ButtonTimerSlider", &timer, 1.0f, 60.0f, "%.0f s")) {
        settings.buttonTimerForRevival.store(timer);
        changed = true;
    }

    ImGui::TextUnformatted(Strings::Get("$InvulnDurationSlider$", "Invulnerability Duration"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$InvulnDurationHelp$", "How long you stay invulnerable after getting downed, and again after self-reviving. Set to 0 to disable protection completely."));
    ImGui::SetNextItemWidth(winWidth * 0.45f);
    float invulnDur = settings.invulnerabilityDurationSeconds.load();
    if (ImGui::SliderFloat("##InvulnDurationSlider", &invulnDur, 0.0f, 60.0f, "%.0f s")) {
        settings.invulnerabilityDurationSeconds.store(invulnDur);
        changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted(Strings::Get("$PSR_SectionHotkeys$", "-- Hotkeys --"));
    ImGui::Separator();

    constexpr auto kbComboCount = std::size(dxKbValues);
    ImGui::TextUnformatted(Strings::Get("$KBHotkeyLabel$", "Keyboard Hotkey"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$KBHotkeyHelp$", "Press this key to attempt self-revive when downed."));
    ImGui::SetNextItemWidth(winWidth * 0.4f);
    int kbIdx = FindComboIndex(dxKbValues, kbComboCount, settings.keyboardHotkey.load());
    if (kbIdx < 0) kbIdx = 0;
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Combo("##kbhotkey", &kbIdx, dxKbNames, static_cast<int>(kbComboCount))) {
        settings.keyboardHotkey.store(dxKbValues[kbIdx]);
        changed = true;
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();

    constexpr auto gpComboCount = std::size(dxGpValues);
    ImGui::TextUnformatted(Strings::Get("$GPHotkeyLabel$", "Gamepad Hotkey"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$GPHotkeyHelp$", "Press this button to attempt self-revive when downed."));
    ImGui::SetNextItemWidth(winWidth * 0.4f);
    int gpIdx = FindComboIndex(dxGpValues, gpComboCount, settings.gamepadHotkey.load());
    if (gpIdx < 0) gpIdx = 0;
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Combo("##gphotkey", &gpIdx, dxGpNames, static_cast<int>(gpComboCount))) {
        settings.gamepadHotkey.store(dxGpValues[gpIdx]);
        changed = true;
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted(Strings::Get("$PSR_SectionConsumables$", "-- Consumables --"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$ConsumableSectionHelp$", "Whatever items are chosen here will actually be consumed from your inventory upon revival."));
    ImGui::Separator();

    bool consumableReq = settings.consumableRequiredForRevival.load();
    if (ImGui::Checkbox(Strings::Get("$ConsumableRequiredCheckbox$", "Consumable Required for Revival"), &consumableReq)) {
        settings.consumableRequiredForRevival.store(consumableReq);
        changed = true;
    }
    ImGui::SameLine();
    HelpMarker(Strings::Get("$ConsumableRequiredHelp$", "When enabled, the player must have at least one of the listed items to self-revive. Bleedout entry does not require an item."));

    ImGui::BeginDisabled(!consumableReq);

    ImGui::SetNextItemWidth(winWidth * 0.4f);
    ImGui::InputTextWithHint("##consumableFilter", Strings::Get("$ConsumableFilterHint$", "Filter items..."), _consumableFilterBuf, sizeof(_consumableFilterBuf));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$ConsumableFilterHelp$", "Type a partial item name to filter the dropdown list. Case insensitive."));

    const auto& cache = ReviveManager::GetIngestibleCache();
    std::string filterStr(_consumableFilterBuf);

    struct FilteredEntry {
        int cacheIndex;
        std::string label;
    };
    std::vector<FilteredEntry> filtered;
    for (size_t i = 0; i < cache.size(); ++i) {
        if (ContainsIgnoreCase(cache[i].name, filterStr) ||
            (!cache[i].editorID.empty() && ContainsIgnoreCase(cache[i].editorID, filterStr))) {
            char buf[384];
            std::snprintf(buf, sizeof(buf), "%s [0x%08X]", cache[i].name.c_str(), cache[i].formID);
            filtered.push_back({ static_cast<int>(i), buf });
        }
    }

    static int selectedConsumableIdx = -1;
    static std::string prevFilter;
    if (filterStr != prevFilter) {
        prevFilter = filterStr;
        selectedConsumableIdx = -1;
    }

    const char* preview = Strings::Get("$ConsumableDropdownHint$", "Select item...");
    if (selectedConsumableIdx >= 0 && static_cast<size_t>(selectedConsumableIdx) < filtered.size()) {
        preview = filtered[selectedConsumableIdx].label.c_str();
    }

    ImGui::SetNextItemWidth(winWidth * 0.4f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::BeginCombo("##consumableDropdown", preview)) {
        for (size_t i = 0; i < filtered.size(); ++i) {
            const bool isSelected = (static_cast<size_t>(selectedConsumableIdx) == i);
            if (ImGui::Selectable(filtered[i].label.c_str(), isSelected)) {
                selectedConsumableIdx = static_cast<int>(i);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::Button(Strings::Get("$ConsumableAddButton$", "Add"))) {
        if (selectedConsumableIdx >= 0 && static_cast<size_t>(selectedConsumableIdx) < filtered.size()) {
            const auto& entry = cache[filtered[selectedConsumableIdx].cacheIndex];
            std::string idStr = entry.editorID + "|" + entry.name;
            bool found = false;
            for (const auto& id : settings.requiredConsumableEditorIDs) {
                if (id == idStr) { found = true; break; }
            }
            if (!found) {
                settings.requiredConsumableEditorIDs.push_back(idStr);
                changed = true;
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::TextUnformatted(Strings::Get("$PSR_SectionMisc$", "Misc Items"));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$MiscSectionHelp$", "Misc items are NOT consumed like food/potions. Instead, one item is simply deleted from your inventory upon revival."));

    ImGui::SetNextItemWidth(winWidth * 0.4f);
    ImGui::InputTextWithHint("##miscFilter", Strings::Get("$MiscFilterHint$", "Filter items..."), _miscFilterBuf, sizeof(_miscFilterBuf));
    ImGui::SameLine();
    HelpMarker(Strings::Get("$MiscFilterHelp$", "Type a partial item name to filter the dropdown list. Case insensitive."));

    const auto& miscCache = ReviveManager::GetMiscCache();
    std::string miscFilterStr(_miscFilterBuf);

    struct FilteredMiscEntry {
        int cacheIndex;
        std::string label;
    };
    std::vector<FilteredMiscEntry> filteredMisc;
    for (size_t i = 0; i < miscCache.size(); ++i) {
        if (ContainsIgnoreCase(miscCache[i].name, miscFilterStr)) {
            char buf[384];
            std::snprintf(buf, sizeof(buf), "%s [0x%08X]", miscCache[i].name.c_str(), miscCache[i].formID);
            filteredMisc.push_back({ static_cast<int>(i), buf });
        }
    }

    static int selectedMiscIdx = -1;
    static std::string prevMiscFilter;
    if (miscFilterStr != prevMiscFilter) {
        prevMiscFilter = miscFilterStr;
        selectedMiscIdx = -1;
    }

    const char* miscPreview = Strings::Get("$MiscDropdownHint$", "Select item...");
    if (selectedMiscIdx >= 0 && static_cast<size_t>(selectedMiscIdx) < filteredMisc.size()) {
        miscPreview = filteredMisc[selectedMiscIdx].label.c_str();
    }

    ImGui::SetNextItemWidth(winWidth * 0.4f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::BeginCombo("##miscDropdown", miscPreview)) {
        for (size_t i = 0; i < filteredMisc.size(); ++i) {
            const bool isSelected = (static_cast<size_t>(selectedMiscIdx) == i);
            if (ImGui::Selectable(filteredMisc[i].label.c_str(), isSelected)) {
                selectedMiscIdx = static_cast<int>(i);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::Button(Strings::Get("$MiscAddButton$", "Add Misc"))) {
        if (selectedMiscIdx >= 0 && static_cast<size_t>(selectedMiscIdx) < filteredMisc.size()) {
            const auto& entry = miscCache[filteredMisc[selectedMiscIdx].cacheIndex];
            std::string idStr = entry.editorID + "|" + entry.name;
            bool found = false;
            for (const auto& id : settings.requiredConsumableEditorIDs) {
                if (id == idStr) { found = true; break; }
            }
            if (!found) {
                settings.requiredConsumableEditorIDs.push_back(idStr);
                changed = true;
            }
        }
    }

    ImGui::EndDisabled();

    if (!settings.requiredConsumableEditorIDs.empty()) {
        ImGui::TextUnformatted(Strings::Get("$RequiredConsumablesList$", "Required Consumables:"));
        for (size_t i = 0; i < settings.requiredConsumableEditorIDs.size(); ++i) {
            ImGui::TextUnformatted(settings.requiredConsumableEditorIDs[i].c_str());
            ImGui::SameLine();
            ImGui::PushID(static_cast<int>(i));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            if (ImGui::SmallButton("[X]")) {
                settings.requiredConsumableEditorIDs.erase(settings.requiredConsumableEditorIDs.begin() + static_cast<std::ptrdiff_t>(i));
                changed = true;
                ImGui::PopStyleColor();
                ImGui::PopID();
                break;
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }

    if (changed) {
        settings.Save();
    }
}
