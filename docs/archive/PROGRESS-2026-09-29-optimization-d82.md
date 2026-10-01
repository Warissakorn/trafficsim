# PROGRESS archive — 2026-09-29: measured optimization pass, docs headroom, CI (D82)

Moved out of `docs/PROGRESS.md` on 2026-10-01 as the oldest live entry.

## 2026-09-29 — Measured optimization pass: docs headroom, CI (D82)

- **Docs headroom.** Four live docs sat at 494–499 of the 500-line limit, and a three-line note
  had just turned CI red. Moved whole, never deleted: M3.2.8b–M3.2.9g out of this file; the
  2026-09-15..21 follow-ups out of `VISSIM_PARITY.md`; the M1.11/M1.11.1/M1.12/M3.1 bodies out of
  `ROADMAP.md` (each keeps its heading and a status line). The archive index moved from the top
  of this file to `archive/README.md`. `NETWORK_EDITOR.md`'s two Connector sections became
  `NETWORK_EDITOR_CONNECTORS.md` (current, not archived). Lines: PROGRESS 497→214, ROADMAP
  499→424, NETWORK_EDITOR 498→406, VISSIM_PARITY 494→328. PROGRESS ≈25.2k→18.6k tokens (the
  decision log is most of what remains). No other doc referenced a moved section by anchor.
- **Stale references.** `CLAUDE.md` and `VISSIM_PARITY.md` §7 sent readers to `NETWORK_EDITOR.md`
  §"Two file kinds", which does not exist; the table is under §"Save, recovery and formats".
  `VISSIM_PARITY.md` §3 still said levels and display types do not exist and named `src/render/`
  (removed in M1.24); it is now marked dated like §1 and §6. The remaining ~99 "broken" paths the
  audit script reports are lane ids, branch names and archived history, not references.
- **CI runs once per commit.** Runs 355 and 356 were both the full five-job suite on `1a702eb`
  (`push` and `pull_request`). `push` now fires only on `main`, `workflow_dispatch` covers a
  branch without a pull request, and a newer push to a pull request cancels the older run.
- **CI uses the runner's 4 vCPUs.** `--parallel 2` became 4; the Windows jobs set `CL=/MP`,
  because the Visual Studio generator's `--parallel` only runs projects side by side and
  `trafficsim-tests` compiled its files one at a time. Baseline (run 355): build steps
  windows-desktop 6:17, windows-core 4:41, linux desktop 4:12, release 3:09, headless 2:31.
- **vcpkg applocal off on Windows.** Package run 68 failed in `z-applocal` with exit 32 (a
  sharing violation between parallel targets). nlohmann-json is header-only and Qt ships through
  `windeployqt`, so the copy step had nothing to copy; `-DVCPKG_APPLOCAL_DEPS=OFF` removes the
  race before more parallelism makes it likelier.
- **ccache on the Linux jobs.** Measured locally on a clean `release` build, 4 cores: 196 s
  cold, 8 s warm, 177/177 hits; `CCACHE_SLOPPINESS` (pch_defines, time_macros, include-file
  times) is what lets the PCH targets hit. The cache is `actions/cache` keyed per preset and
  commit, restored by prefix; ~120 MB for `release`, capped at 500 MB. Windows is not covered:
  ccache with MSVC needs the Ninja generator, which would change how those jobs build.
- **Measured in CI** (build step; baseline runs 355/356 → run 360, items 3–4 with ccache cold):
  windows-desktop 6:17/6:16 → 4:31 (−28%; the run's critical path, whole run 8:27 → 6:33);
  windows-core 4:41/3:41 → 3:55 (within noise); linux desktop 4:12/4:14 → 3:30; headless
  2:31/2:08 → 1:57; **release 3:09/3:37 → 4:01, slower** — one sample, taken while ccache was
  writing its first cache, so not yet attributed. The next run is the first warm-cache one.
  **Warm cache (run 361):** Linux build steps release 0:08, headless 0:07, desktop 0:24; the
  release slowdown is gone. Windows is now the whole critical path: windows-desktop build 5:40
  (4:31 in run 360; baseline 6:17/6:16, so −10 to −28%), windows-core 3:45 (no change); whole
  run 8:03 against 8:27. Getting Windows down means the Ninja generator plus a compiler cache
  there — a separate change, not made here.
- **Eleven files no longer lean on a PCH for `nlohmann/json.hpp`.** D27 says a file that builds
  or reads a `Json` includes the definition itself; six `src/project` sources, three tools and
  every `tests/` file (through `test.hpp`) compiled only because their target precompiles it.
  `-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON` now builds and passes 69/69; with the PCH on nothing
  changes (the header is already precompiled there), and `trafficsim-cli 42` is byte-identical.
- **Windows jobs on Ninja + ccache.** The Visual Studio generator takes no compiler launcher,
  so both Windows jobs configure with Ninja inside the vcvars64 environment (`cl` named
  explicitly, since the runner's PATH also carries MinGW), ccache from Chocolatey, PCH off
  (neither ccache nor sccache caches MSVC `/Yu`), and the same `actions/cache` scheme as Linux.
  `/MP` is gone: Ninja already runs one file per core. `package.yml` keeps the Visual Studio
  generator and its PCH; it builds release artifacts, where a cache buys nothing.
  First run (364) failed on MSVC: `history.cpp` and `split_link.cpp` throw `std::invalid_argument`
  without `<stdexcept>`, which libstdc++ and libc++ reach through `<string>` and MSVC does not.
  An include check over `src/ tools/ tests/` (each `std::` symbol against the file's own
  include closure) found the same class in twelve files; all now include `<stdexcept>`.
  **Measured:** Windows build steps, VS + PCH 4:31/5:40 (desktop) and 3:45/3:55 (core) →
  Ninja cold 7:44/7:31 and 5:04/4:57 (PCH off, empty cache) → **warm (run 367) 0:09 and 0:05**.
  Whole run 8:27 before this pass → 2:55 warm. A pull request restores `main`'s cache, so only
  a change to many sources, or a new cache key, pays the cold price.
