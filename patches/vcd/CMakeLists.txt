# Configure through the repository root with -DDBNO_BUILD_VCD=ON.
set(VCD "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
file(GLOB VCD_SOURCES CONFIGURE_DEPENDS "${VCD}/src/*.cpp")
add_library(VariadicCollisionDynamics SHARED ${VCD_SOURCES} version.cpp)
target_include_directories(VariadicCollisionDynamics PRIVATE
    "${VCD}/include" "${PROJECT_SOURCE_DIR}/vendor/CLibUtilsQTR/include"
    "${PROJECT_SOURCE_DIR}/vendor/nlohmann/include")
target_compile_definitions(VariadicCollisionDynamics PRIVATE
    UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
    PRODUCT_NAME="VariadicCollisionDynamics" BEAUTIFUL_NAME="VCD DBNO experimental patch"
    CURR_VERSION="1.3.5-dbno.1" AUTHOR_NAME="Legendman and Truman with DBNO integration contributors")
target_compile_options(VariadicCollisionDynamics PRIVATE /W3 /permissive- /Zc:preprocessor /utf-8 /EHsc)
target_precompile_headers(VariadicCollisionDynamics PRIVATE "${VCD}/include/plugin.hpp")
target_link_libraries(VariadicCollisionDynamics PRIVATE CommonLibSSE::CommonLibSSE)
install(TARGETS VariadicCollisionDynamics RUNTIME DESTINATION SKSE/Plugins COMPONENT VCD)
install(DIRECTORY assets/ DESTINATION . COMPONENT VCD)
install(FILES README.md DESTINATION . COMPONENT VCD)
install(FILES upstream/LICENSE DESTINATION . RENAME LICENSE-VCD.txt COMPONENT VCD)
install(FILES "${PROJECT_SOURCE_DIR}/vendor/CLibUtilsQTR/LICENSE" DESTINATION . RENAME LICENSE-CLibUtilsQTR.txt COMPONENT VCD)
install(FILES "${PROJECT_SOURCE_DIR}/vendor/nlohmann/LICENSE.txt" DESTINATION licenses RENAME nlohmann-json-and-Hedley-MIT.txt COMPONENT VCD)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/licenses/" DESTINATION licenses COMPONENT VCD)
add_executable(CrawlGeometryTests tests.cpp)
target_include_directories(CrawlGeometryTests PRIVATE "${VCD}/include")
add_test(NAME CrawlGeometry COMMAND CrawlGeometryTests)
