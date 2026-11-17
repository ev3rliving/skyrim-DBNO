cmake_minimum_required(VERSION 3.26)
cmake_policy(SET CMP0141 NEW)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
project(SkyrimDBNO VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
option(DBNO_BUILD_VCD "Build the separate experimental VCD fork" OFF)
include(cmake/Dependencies.cmake)

add_library(SkyrimDBNO SHARED src/main.cpp src/DamageHook.cpp src/Hooks.cpp
    src/ReviveManager.cpp src/Settings.cpp src/Strings.cpp src/UI.cpp)
target_include_directories(SkyrimDBNO PRIVATE src vendor/MenuFrameworkAPI
    vendor/SkyPromptAPI/include vendor/simpleini)
target_compile_definitions(SkyrimDBNO PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(SkyrimDBNO PRIVATE /W3 /permissive- /Zc:preprocessor /utf-8 /EHsc)
target_precompile_headers(SkyrimDBNO PRIVATE src/pch.h)
target_link_libraries(SkyrimDBNO PRIVATE CommonLibSSE::CommonLibSSE)

install(TARGETS SkyrimDBNO RUNTIME DESTINATION SKSE/Plugins COMPONENT DBNO)
install(DIRECTORY assets/ DESTINATION . COMPONENT DBNO)
install(FILES README.md CHANGELOG.md COPYING.txt EXCEPTIONS.md THIRD-PARTY.md
    DESTINATION . COMPONENT DBNO)
install(DIRECTORY licenses/ DESTINATION licenses COMPONENT DBNO)
enable_testing()
add_executable(DBNORulesTests tests/DBNORulesTests.cpp)
target_include_directories(DBNORulesTests PRIVATE src)
add_test(NAME DBNODamageRules COMMAND DBNORulesTests)
add_test(NAME RuntimeActorAccess COMMAND "${CMAKE_COMMAND}"
    "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/RuntimeActorAccess.cmake")
if(DBNO_BUILD_VCD)
    add_subdirectory(patches/vcd)
endif()
