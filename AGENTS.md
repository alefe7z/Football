# Instructions for AI coding agents

Two AI agents (Claude and ChatGPT/Codex) work on this repository at the same
time for one owner. Before changing anything, read
[docs/colaboracao/PROTOCOLO.md](docs/colaboracao/PROTOCOLO.md) (Portuguese) and
reserve your task in
[docs/colaboracao/DIVISAO-DE-TRABALHO.md](docs/colaboracao/DIVISAO-DE-TRABALHO.md).

## Ground rules

- Never commit to `master`. Work on `claude/<topic>` or `chatgpt/<topic>`.
- Stay in your area (see the division-of-work table). Changes outside it must be
  small and listed in your session report.
- Shared files (`data/locale/*.ini`, `sources.cmake`, `tests/CMakeLists.txt`,
  `.github/workflows/ci.yml`): only add lines; never reorder or reformat.
- Every session ends with a report in `docs/colaboracao/relatorios/` (template in
  `docs/colaboracao/modelos/`) and an entry at the top of `docs/colaboracao/DIARIO.md`.
- Reports say what was verified and how, and what was not verified.

## Build and test (Linux)

```bash
bash scripts/setup_linux_deps.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
cd build && ctest -j4
```

If FetchContent cannot download GoogleTest, install the `googletest` package and
add `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/usr/src/googletest`.

Full automated 3D match on a virtual display (about 90 s):

```bash
cd build && ../tests/run_menu_smoke.sh ./gameplayfootball ../data/menu_smoke_full_match.config 420 \
  "[menu-smoke] Full-match verification succeeded, quitting test run"
```

`GF_CAPTURE_DIRECTORY=<dir>` saves rendered frames as PNG. Smoke runs rewrite the
config file they are given: restore `data/menu_smoke_*.config` with `git checkout`
before committing.

## Conventions

- C++17, `.clang-format` at the root; format only the lines you touch.
- Rules without graphics dependencies go in a header-only module with unit
  tests (example: `src/onthepitch/penaltyshootout.hpp` and
  `tests/onthepitch/penalty_shootout_test.cpp`).
- On-screen text uses `TR("key")` / `TRF("key", {...})`; every key must exist in
  all five locales (en, pt, es, fr, de). `LocalizationTest` enforces this.
- Commit messages: `Area: what changed`, in English.
- Detailed known defects and feature gaps: [docs/gap-list.md](docs/gap-list.md).
