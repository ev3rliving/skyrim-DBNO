#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace RuntimeCompatibility
{
    // Address Library resolves these vtables for the installed runtime, including
    // format-5 databases on 1.7. Check before installing any DBNO gameplay hooks.
    inline bool ValidatePlayerInterfaces()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            SKSE::log::error("[SkyrimDBNO] Runtime validation failed: player unavailable; hooks not installed");
            return false;
        }
        const auto matches = [](const void* object, std::size_t index) {
            const REL::Relocation<std::uintptr_t> expected{ RE::PlayerCharacter::VTABLE[index] };
            return object && *static_cast<const std::uintptr_t*>(object) == expected.address();
        };
        if (!matches(player, 0) ||
            !matches(static_cast<RE::IAnimationGraphManagerHolder*>(player), 3) ||
            !matches(player->AsActorValueOwner(), 5)) {
            SKSE::log::error("[SkyrimDBNO] Runtime {} player interface mismatch; hooks not installed",
                REL::Module::get().version().string());
            return false;
        }
        SKSE::log::info("[SkyrimDBNO] Runtime {} player interfaces validated (primary, animation, actor values)",
            REL::Module::get().version().string());
        return true;
    }
}
