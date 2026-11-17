# Build from a fresh clone

The repository includes the dependency source snapshots. There are no required
submodules, sibling mod projects, prebuilt `.lib` files, or vcpkg bootstrap steps.

## Tools

- Windows x64.
- Visual Studio 2022 / Build Tools with Desktop development with C++, MSVC v143
  supporting C++23, and a Windows SDK. Local validation uses MSVC 14.44 and SDK 10.0.26100.0.
- CMake 3.26+ and Ninja on PATH.
- Git for cloning.

Open an **x64 developer PowerShell** and run:

```powershell
git clone https://github.com/ev3rliving/skyrim-DBNO.git
cd skyrim-DBNO
./Build.ps1
```

For both DBNO and the optional VCD fork:

```powershell
./Build.ps1 -WithVCD
```

The first build compiles CommonLibSSE and can take several minutes and substantial
disk space. `-Jobs 2` is the default; adjust to match available memory.
The optional `-PortableToolsRoot` parameter is for the development toolchain layout;
normal contributors should use the VS developer shell.

## Output

- `dist/SkyrimDBNO-1.0.0.zip`: main mod only.
- `dist/VCD-DBNO-experimental-1.zip`: optional VCD replacement, when requested.
- `dist/main/` and `dist/vcd/`: staged install trees.

Build.ps1 runs CTest before staging. Main checks cover damage/death rules,
stagger filtering, regeneration filtering and runtime actor-value access.
The optional build adds crawl geometry checks. They do not replace in-game testing.
The VCD fork is based on source labeled 1.3.5; see its README before installing.

## CMake commands

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDBNO_BUILD_VCD=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
cmake --install build --component DBNO --prefix dist/main
cmake --install build --component VCD --prefix dist/vcd
```

Use the named install components. An unfiltered CMake install may install
upstream development headers/libraries as well as mod content.

## Dependency policy

See `vendor/REVISIONS.json` and `THIRD-PARTY.md`. VCD's recorded revision is the
baseline before the integration changes in `patches/vcd/upstream/`.
Source snapshots preserve licenses; runtime framework DLLs and generated build
products are not tracked. Dependency updates should be deliberate and reviewed
for Skyrim runtime layout changes. Actor value access must use
`AsActorValueOwner()` on the runtime actor, not a compile-time base cast.

Unused upstream demo GIFs, VCD font payloads and CommonLib binary test fixtures
are omitted from Git. CommonLib's own test suite is disabled; DBNO's tests and
the crawl geometry tests are included. Install VCD and its Resources separately
when using the optional patch in game.
