# Y.Disquette

A desktop GUI for Yandex Disk (Qt6, C++17): folder tree, sync selection, bidirectional sync with change tracking. **Linux only.**

## Running the AppImage

Download `Y.Disquette-<version>-x86_64.AppImage` from the repository Releases. Make it executable and run:

```bash
chmod +x Y.Disquette-*-x86_64.AppImage
./Y.Disquette-*-x86_64.AppImage
```

## Building from source

**Requirements:** CMake 3.16+, Qt6 (Core, Widgets, Network, Sql, WebEngineCore, WebEngineWidgets), C++17 compiler.

**Ubuntu / Debian:** `sudo apt install cmake qt6-base-dev qt6-tools-dev qt6-webengine-dev`

**Build:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

**Run:** `./Y.Disquette` (from `build`) or `./build/Y.Disquette` (from repo root).

**Tests:** `ctest --test-dir build --output-on-failure`

## Versioning

Version is set in `CMakeLists.txt` (`project(Y.Disquette VERSION 0.1.0)`) and in Git tags (`v1.0.0`). Pushing a `v*` tag triggers GitHub Actions to build, run tests, and publish the AppImage to Releases.

## Plan

See the [plan/](plan/) directory.
