# Contributing

Build and test with `./Build.ps1 -WithVCD` from an x64 VS developer shell.
Keep main-mod changes and optional VCD changes scoped to their respective folders.
Include a description of the behavior change and relevant automated/in-game checks.

For bug reports, include the Skyrim runtime, DBNO version, reproduction steps,
other death/animation mods, and SkyrimDBNO.log. For crashes include a crash logger
report. Note whether the optional VCD fork is installed. Review logs before posting
if you do not want machine-specific paths shared publicly.

Preserve dependency copyright and license notices. Runtime actor interfaces vary
between SE/AE versions; use the runtime accessors rather than guessed base offsets.
Do not commit build outputs, local INI overrides, credentials or game files.

The first public release targets the behavior tested on Skyrim 1.6.1170.
Other runtimes and animation stacks need explicit testing rather than assumptions.
