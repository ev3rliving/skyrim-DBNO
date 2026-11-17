# Credits and source provenance

- Player Self Revive by zfroggyman: user-provided `Player Self Revive Source` archive, mod-manager version 3.7.0.0 (source plugin.h identifies 1.0.0). https://www.nexusmods.com/skyrimspecialedition/mods/182847 . Original COPYING.txt and EXCEPTIONS.md retained. The fork modifies damage handling, state recovery, settings/UI, naming, and build configuration.
- SkyPrompt API by A. Ege Yilmaz: https://github.com/QTR-Modding/SkyPromptAPI , revision `1fe9d10d31fb0cb8c5074ddcfeba3f89518823e7`, MIT. Source and license in vendor/SkyPromptAPI.
- SKSE Menu Framework API: https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API , revision `1dcb70179076aae4ab626f43c5baab2735ca5877`, LGPL 2.1. Unmodified source and license in vendor/MenuFrameworkAPI.
- CommonLibSSE/CommonLibVR by alandtse and contributors: https://github.com/alandtse/CommonLibVR , v7.2.0 revision `7a60f4de794095d7b0f8928d1b930a52e9a7da83`, GPL 3 or later. Matching source is locally available in vendor/CommonLibSSE.
- spdlog 1.15.3 by Gabi Melman and contributors, including bundled fmt notices; MIT. Source: https://github.com/gabime/spdlog . Matching source is included under vendor/spdlog in the release source archive.
- SimpleIni by Brodie Thiesfield, MIT: https://github.com/brofield/simpleini . Source included under vendor/simpleini.
- DirectXTK SimpleMath by Microsoft and contributors, MIT: https://github.com/microsoft/DirectXTK . Source included under vendor/DirectXTK.
- MinHook/hde64 by Tsuda Kageyu and contributors, with original component notices: https://github.com/TsudaKageyu/minhook . Source included under vendor/minhook; used by CommonLib's patch safety checks.
- rapidcsv by Daniel Lörch and contributors, BSD-3-Clause: https://github.com/d99kris/rapidcsv . Source included under vendor/rapidcsv.
- The prompt-position JSON is derived from Player Self Revive's supplied theme, retaining its author credit.

The public 1.0.0 main archive includes dependency license notices in licenses/.
The matching source archive contains modified source, API dependencies, static-library
dependency sources and build instructions. Original Player Self Revive license and
exception texts are retained unchanged. Keep the source download available alongside
the binary download. No original runtime Player Self Revive, SkyPrompt, Menu Framework,
VCD, Precision or APC DLL is redistributed in the main archive.

## Optional VCD fork

- Variadic Collision Dynamics by Legendman and Truman: https://github.com/legendman89/VariadicCollisionDynamics , baseline `790cc9573a45262a44f9471642ebbe9fdff6c205`, GPL-3.0. Modified source and original license are in `patches/vcd/upstream`. Source version 1.3.5; this is not a binary patch for arbitrary newer VCD releases.
- CLibUtilsQTR: https://github.com/QTR-Modding/CLibUtilsQTR , revision `69b25fea84dd23cef287d3ca20ccd3d348c09cb1`, MIT. Source and license in `vendor/CLibUtilsQTR`.
- nlohmann JSON 3.11.3 by Niels Lohmann, with Hedley by Evan Nemerson, MIT: https://github.com/nlohmann/json . Source and component notices in `vendor/nlohmann`.

The main mod does not link VCD. The optional patch builds a separate VCD DLL.
`vendor/REVISIONS.json` records dependency revisions; generated libraries are not
tracked. All applicable original licenses remain in their source directories.
