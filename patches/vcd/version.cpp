#include "plugin.hpp"
SKSEPluginVersion = []() noexcept {
    SKSE::PluginVersionData v{};
    v.PluginVersion(REL::Version{1, 3, 5, 1});
    v.PluginName("VariadicCollisionDynamics");
    v.AuthorName("Legendman & Truman; DBNO integration contributors");
    v.UsesAddressLibrary();
    v.UsesNoStructs();
    v.versionIndependenceEx |= SKSE::PluginVersionData::kVersionIndependentEx_AddressLibraryV5;
    return v;
}();
