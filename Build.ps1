param(
    [switch]$WithVCD,
    [string]$CMake = 'cmake',
    [int]$Jobs = 2,
    [string]$PortableToolsRoot = ''
)
$ErrorActionPreference = 'Stop'
if ($PortableToolsRoot) {
    $taskVC = Join-Path $PortableToolsRoot 'msvc/VC/Tools/MSVC/14.44.35207'
    $taskSDK = Join-Path $PortableToolsRoot 'sdk-extracted/Windows Kits/10'
    $taskCMakeBin = Join-Path $PortableToolsRoot 'tools/cmake-3.31.8-windows-x86_64/bin'
    $taskNinja = Join-Path $PortableToolsRoot 'tools/ninja'
    $env:PATH = "$taskVC/bin/Hostx64/x64;$taskSDK/bin/10.0.26100.0/x64;$taskCMakeBin;$taskNinja;$env:PATH"
    $env:INCLUDE = "$taskVC/include;$taskSDK/Include/10.0.26100.0/ucrt;$taskSDK/Include/10.0.26100.0/shared;$taskSDK/Include/10.0.26100.0/um;$taskSDK/Include/10.0.26100.0/winrt"
    $env:LIB = "$taskVC/lib/x64;$taskSDK/Lib/10.0.26100.0/ucrt/x64;$taskSDK/Lib/10.0.26100.0/um/x64"
}
$taskVCD = if ($WithVCD) { 'ON' } else { 'OFF' }
& $CMake -S $PSScriptRoot -B "$PSScriptRoot/build" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DDBNO_BUILD_VCD=$taskVCD"
if ($LASTEXITCODE -ne 0) { throw 'Configuration failed.' }
& $CMake --build "$PSScriptRoot/build" --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
$taskCTest = Join-Path (Split-Path (Get-Command $CMake).Source -Parent) 'ctest.exe'
& $taskCTest --test-dir "$PSScriptRoot/build" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Regression checks failed.' }
& $CMake --install "$PSScriptRoot/build" --component DBNO --prefix "$PSScriptRoot/dist/main"
if ($LASTEXITCODE -ne 0) { throw 'DBNO staging failed.' }
Compress-Archive -Path "$PSScriptRoot/dist/main/*" -DestinationPath "$PSScriptRoot/dist/SkyrimDBNO-1.0.2.zip" -Force
if ($WithVCD) {
    & $CMake --install "$PSScriptRoot/build" --component VCD --prefix "$PSScriptRoot/dist/vcd"
    if ($LASTEXITCODE -ne 0) { throw 'VCD staging failed.' }
    Compress-Archive -Path "$PSScriptRoot/dist/vcd/*" -DestinationPath "$PSScriptRoot/dist/VCD-DBNO-experimental-1.zip" -Force
}
