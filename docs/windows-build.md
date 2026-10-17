# Windows x64 build: current state

This branch adds a **diagnostic build pipeline**, not a working binary. The upstream Gameplay Football project is discontinued. No release is published in this fork.

## Run the build

Open **Actions → Windows x64 - Gameplay Football → Run workflow** on branch `ci/windows-x64-build`, or view checks on the PR. GitHub Actions attempts to configure and compile with MinGW-w64 on a Windows runner. When it succeeds, the artifact is named `gameplayfootball-windows-x64`.

## Dependencies verified from CMakeLists.txt

- OpenGL
- SDL **1.x** (legacy SDL CMake FindSDL), SDL_image, SDL_ttf, SDL_gfx, SDL_net
- Boost: system, thread, signals, filesystem (the legacy **signals** component has been removed from modern Boost)
- SGE (legacy SDL graphics extension)
- OpenAL
- Blunted2 (separate upstream engine: https://github.com/BazkieBumpercar/Blunted2)

The README also points to a separate gameplay data archive at http://properlydecent.com/data/gameplayfootball/ (availability and redistribution rights are unverified). Runtime models/textures/data are **not included** in this source tree.

## Expected initial failure

The workflow deliberately runs the original CMake dependency discovery without fabricating stub libraries, so it is expected to stop at missing packages. Installing only a modern compiler is insufficient. The following engineering work remains:

1. Port or obtain compatible builds of Blunted2 and SGE for MinGW-w64 x64.
2. Restore compatible SDL 1.x-family libraries or port code to maintained SDL APIs.
3. Replace Boost.Signals with a compatible supported implementation and update code/CMake.
4. Address outdated CMake policies/Windows-specific linker assumptions (for example `dl` and `m` in the current UNIX-oriented link list).
5. Obtain permitted game assets and package them alongside DLL dependencies; test startup and gameplay on Windows.

**Do not merge this PR expecting a playable Windows game.** Its purpose is repeatable CI, early diagnostic output, and an artifact once the port is complete. The original source files remain unchanged.
