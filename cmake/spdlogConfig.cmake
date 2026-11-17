if(NOT TARGET spdlog::spdlog)
    message(FATAL_ERROR "Configure from the skyrim-DBNO repository root")
endif()
set(spdlog_FOUND TRUE)
