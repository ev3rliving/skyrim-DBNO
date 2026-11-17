# Inspect the actual DLL so a missing legacy entry point fails release checks.
get_filename_component(tool_dir "${DUMPBIN}" DIRECTORY)
execute_process(COMMAND "${tool_dir}/dumpbin.exe" /exports "${PLUGIN}"
    RESULT_VARIABLE result OUTPUT_VARIABLE exports ERROR_VARIABLE errors)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Could not inspect plugin exports: ${errors}")
endif()
foreach(symbol SKSEPlugin_Query SKSEPlugin_Load SKSEPlugin_Version SkyrimDBNO_IsCrawlingV1)
    if(NOT exports MATCHES "[ \t]${symbol}[\r\n]")
        message(FATAL_ERROR "Missing required DLL export: ${symbol}")
    endif()
endforeach()
