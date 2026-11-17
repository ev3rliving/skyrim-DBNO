#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <iostream>
#include <fstream>

int main()
{
    // Exercise CommonLib's actual selection and accessors without launching Skyrim.
    // This verifies library routing, not the layout of an untested game executable.
    const REL::Version versions[]{ {1, 5, 97, 0}, {1, 6, 353, 0},
        {1, 6, 629, 0}, {1, 6, 1170, 0}, {1, 7, 99, 0}, {1, 7, 104, 0} };
    alignas(RE::PlayerCharacter) std::byte storage[sizeof(RE::PlayerCharacter) + 1024]{};
    auto* actor = reinterpret_cast<RE::PlayerCharacter*>(storage);
    const auto base = reinterpret_cast<std::uintptr_t>(actor);
    for (const auto& version : versions) {
        if (!REL::Module::mock(version)) return 1;
        const bool ae = version >= REL::Version{1, 6, 0, 0};
        const bool newer = version >= REL::Version{1, 6, 629, 0};
        if (REL::Module::IsAE() != ae || REL::Module::IsSE() == ae ||
            REL::RelocationID(11, 22).id() != (ae ? 22 : 11) ||
            reinterpret_cast<std::uintptr_t>(actor->AsActorValueOwner()) - base != (newer ? 0xB8 : 0xB0) ||
            reinterpret_cast<std::uintptr_t>(actor->AsActorState()) - base != (newer ? 0xC0 : 0xB8)) {
            std::cerr << "Runtime selection/accessor failure: " << version.string() << '\n';
            return 1;
        }
        std::cout << version.string() << " runtime routing and actor accessors passed\n";
    }
    // Generate a tiny synthetic format-5 database; do not ship game addresses.
    for (const auto version : { REL::Version{1, 7, 99, 0}, REL::Version{1, 7, 104, 0} }) {
        if (!REL::Module::mock(version)) return 1;
        const auto fixture = std::filesystem::current_path() / "dbno-format5-test.bin";
        {
            std::ofstream file(fixture, std::ios::binary);
            const auto write = [&file](std::uint32_t value) {
                file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            };
            write(5);
            for (std::size_t i = 0; i < 4; ++i) write(version[i]);
            const char name[64] = "SkyrimSE.exe";
            file.write(name, sizeof(name));
            write(8); write(0); write(3);
            write(0); write(0x1234); write(0x5678);
        }
        if (!REL::IDDB::inject(fixture.wstring(), version) ||
            REL::IDDB::get().id2offset(1) != 0x1234 ||
            REL::IDDB::get().id2offset(2) != 0x5678) return 1;
        REL::IDDB::reset();
        std::filesystem::remove(fixture);
        std::cout << version.string() << " format-5 auto-detection and lookup passed\n";
    }
    REL::Module::reset();
}
