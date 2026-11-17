# Regression for the 1.6.1170 crash at SkyrimDBNO.dll+0x244C1 in 0.1.1.
# Direct inherited ActorValueOwner calls use the compiler's Actor base offset
# (0x98 in this build), not the runtime offset (0xB8 on 1.6.1170).
# Actor pointers must explicitly use AsActorValueOwner().
file(GLOB sources "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.h")
foreach(source IN LISTS sources)
    file(READ "${source}" contents)
    string(REGEX MATCH "(player|plr|p|target|a_player|a_target)[ \t\r\n]*->[ \t\r\n]*(GetActorValue|GetPermanentActorValue|GetBaseActorValue|SetActorValue|ModActorValue|RestoreActorValue|DamageActorValue)[ \t\r\n]*\\(" unsafe_call "${contents}")
    if(unsafe_call)
        message(FATAL_ERROR "Unsafe actor-value access in ${source}: ${unsafe_call}; use AsActorValueOwner()")
    endif()
endforeach()
message(STATUS "Actor-value access regression guard passed")
