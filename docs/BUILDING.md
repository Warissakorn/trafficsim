# Building TrafficSim

## Requirements

- C++20 compiler: GCC 12+, recent Clang, or Visual Studio 2022 MSVC.
- CMake 3.24+ and Ninja for the checked-in presets.
- nlohmann/json 3.11+ (header-only; provided by your package manager).
- Qt 6.4+ Widgets for the desktop; Qt Test for the desktop smoke test.

The headless engine/CLI does not need Qt. The application, CLI, tests and repository
checks are C++. CMake, JSON, Markdown and workflow YAML remain their respective data,
build and documentation formats. There is no npm or Python step in the normal build.

## Windows — Visual Studio 2022 and Qt

1. Install Visual Studio 2022 with **Desktop development with C++**, including the
   Windows SDK, CMake and Ninja.
2. Install a Qt 6 **MSVC 2022 64-bit** desktop kit using the Qt installer.
3. Install nlohmann/json using vcpkg: `vcpkg install nlohmann-json:x64-windows`.
4. Open **x64 Native Tools Command Prompt for VS 2022** in the repository.

Example paths below must be replaced by your Qt and vcpkg installations:

```bat
set PATH=C:/Qt/6.8.3/msvc2022_64/bin;%PATH%
cmake --preset desktop -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build --preset desktop
ctest --preset desktop
build\desktop\bin\trafficsim-desktop.exe
build\desktop\bin\trafficsim-cli.exe 42
```

Put the Qt kit's `bin` on PATH before testing as well as launching, or deploy the DLLs
beside the executables. Use the same compiler/architecture for Qt and TrafficSim.
MinGW Qt binaries are not compatible with an MSVC application.

For a local runnable folder, build Release and use the matching Qt kit's `windeployqt`:

```bat
cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build --preset release
C:/Qt/6.8.3/msvc2022_64/bin/windeployqt.exe --release build/release/bin/trafficsim-desktop.exe
```

Keep `bin/data/` beside the executable. This is a developer deployment folder;
the signed installer, file association and clean-machine acceptance remain M7.

## Linux — Ubuntu 24.04

```bash
sudo apt-get update
sudo apt-get install g++ cmake ninja-build nlohmann-json3-dev qt6-base-dev
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
./build/desktop/bin/trafficsim-desktop --language th
```

In displayless CI the desktop test uses `QT_QPA_PLATFORM=offscreen`. Normal launches
use your desktop session's platform plugin to display a visible window.

## Headless, custom paths and installation

```bash
cmake --preset headless
cmake --build --preset headless
ctest --preset headless
./build/headless/bin/trafficsim-cli --seed 42
cmake --install build/headless --prefix ./out/install
./out/install/bin/trafficsim-cli 42
```

If nlohmann/json has no package config, set
`-DTRAFFICSIM_JSON_INCLUDE_DIR=/path/to/include` (containing `nlohmann/`).
No dependency is downloaded silently during configuration. For a custom Qt install
use `-DCMAKE_PREFIX_PATH=/path/to/Qt/kit`.

Presets use Ninja: executables are under `build/<preset>/bin/`. Visual Studio or Ninja
Multi-Config generators add `Debug/` or `Release/` after `bin/`; use `--config Release`
when building and `ctest -C Release` when testing. Runtime data follows that directory.

## Checks and sanitizers

```bash
cmake --build build/headless --target check
cmake -S . -B build/asan -G Ninja -DTRAFFICSIM_BUILD_DESKTOP=OFF -DTRAFFICSIM_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/asan
ctest --test-dir build/asan --output-on-failure
```

The sanitizer option applies to GCC/Clang. Tests use explicit failure checks, so
Release builds do not disable them through `NDEBUG`. Strict floating-point flags
disable fast-math and contraction; they do not promise bitwise identity across different
math libraries. Preserve toolchain/version metadata for future report runs.

Qt licensing depends on the modules and distribution arrangement; consult the
[Qt module documentation](https://doc.qt.io/qt-6/qtwidgets-index.html#licenses).
No commercial license or redistribution arrangement is assumed by this repository.

## Network editor

After building the desktop, run `trafficsim-desktop --editor` (add `--language th` for
Thai). `--scenario path/to/network.traffic.json` opens an editor document when used with
`--editor`; without it the application still opens the M0 simulation harness. See
[NETWORK_EDITOR.md](NETWORK_EDITOR.md) for drawing, images and saving.
The headless build includes document/history tests but does not build Qt editor tests.
