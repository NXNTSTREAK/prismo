#!/usr/bin/env bash
# Builds dist/prismo (Linux) and dist/Prismo.exe (Windows, cross-compiled with Zig).
#   ./build.sh            both
#   ./build.sh linux      Linux only (needs g++ and SDL3)
#   ./build.sh windows    Windows only (needs .tools/venv with the ziglang package)
set -euo pipefail
cd "$(dirname "$0")"

IMGUI=third_party/imgui
COMMON=(src/prismo.cpp src/ui.cpp "$IMGUI/imgui.cpp" "$IMGUI/imgui_draw.cpp" "$IMGUI/imgui_tables.cpp"
        "$IMGUI/imgui_widgets.cpp")
FLAGS=(-std=c++17 -O2 -Ibuild -Isrc "-I$IMGUI" "-I$IMGUI/backends" -DIMGUI_DISABLE_DEMO_WINDOWS
       -DIMGUI_DISABLE_DEBUG_TOOLS)
mkdir -p build dist

# Embed the font (compressed) so the binaries are self-contained.
if [[ ! build/font.h -nt assets/AdwaitaSans-Regular.ttf ]]; then
    g++ -O2 -o build/bin2c "$IMGUI/misc/fonts/binary_to_compressed_c.cpp"
    build/bin2c -u32 assets/AdwaitaSans-Regular.ttf adwaita_sans > build/font.h
fi

target=${1:-all}

if [[ $target == all || $target == linux ]]; then
    g++ "${FLAGS[@]}" -s -static-libstdc++ -static-libgcc \
        "${COMMON[@]}" src/device_linux.cpp src/main_linux.cpp \
        "$IMGUI/backends/imgui_impl_sdl3.cpp" "$IMGUI/backends/imgui_impl_sdlrenderer3.cpp" \
        $(pkg-config --cflags --libs sdl3) -o dist/prismo
    echo "built dist/prismo"
fi

if [[ $target == all || $target == windows ]]; then
    ZIG=("$PWD/.tools/venv/bin/python" -m ziglang)
    (cd assets && "${ZIG[@]}" rc /fo ../build/app.res app.rc)
    "${ZIG[@]}" c++ -target x86_64-windows-gnu "${FLAGS[@]}" -s -DUNICODE -D_UNICODE \
        "${COMMON[@]}" src/device_windows.cpp src/main_windows.cpp \
        "$IMGUI/backends/imgui_impl_win32.cpp" "$IMGUI/backends/imgui_impl_dx11.cpp" build/app.res \
        -ld3d11 -ldxgi -ld3dcompiler_47 -ldwmapi -lhid -lsetupapi -limm32 -lgdi32 -luser32 -ladvapi32 \
        -Wl,--subsystem,windows -o "dist/Prismo.exe"
    rm -f "dist/Prismo.pdb" "dist/Prismo.lib"
    echo "built dist/Prismo.exe"
fi
