#pragma once

#include "plugin.hpp"
#include "logger.hpp"

// Optional, versioned C ABI. Query only on the game thread. Never load DBNO
// ourselves: absence (or an older DBNO) leaves normal VCD behavior intact.
namespace DBNO {
    inline bool IsCrawling()
    {
        using Query = bool (*)() noexcept;
        static const auto query = []() -> Query {
            const auto module = GetModuleHandleW(L"SkyrimDBNO.dll");
            auto result = module ? reinterpret_cast<Query>(GetProcAddress(module, "SkyrimDBNO_IsCrawlingV1")) : nullptr;
            logger::info("DBNO crawl integration: {}", result ? "connected" : "unavailable (requires SkyrimDBNO 0.1.6+)");
            return result;
        }();
        return query && query();
    }
}
