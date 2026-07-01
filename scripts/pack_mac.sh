#!/usr/bin/env bash
# macOS 打包: 生成 .dmg (拖拽安装) 与 .tar.gz
# 依赖: osgEarth 不在 Homebrew, 用 vcpkg 安装; Homebrew 只装构建工具。
#   brew install cmake ninja pkg-config autoconf automake libtool
#   git clone https://github.com/microsoft/vcpkg ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh
#   ~/vcpkg/vcpkg install osgearth[core]:arm64-osx   # Intel Mac 用 x64-osx
#   export VCPKG_ROOT=~/vcpkg
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="build-mac"
# Apple Silicon = arm64-osx; Intel = x64-osx
TRIPLET="${VCPKG_TRIPLET:-arm64-osx}"

echo "==> 配置 (Release, triplet=$TRIPLET)"
if [ -n "${VCPKG_ROOT:-}" ]; then
    cmake -B "$BUILD_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
        -DCMAKE_BUILD_TYPE=Release
else
    echo "   (未设置 VCPKG_ROOT, 假定 osgEarth 已在系统路径)"
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

echo "==> 编译"
cmake --build "$BUILD_DIR" --config Release -j

echo "==> 打包 (CPack: DragNDrop + TGZ)"
cd "$BUILD_DIR"
cpack -G "DragNDrop;TGZ"

echo "==> 完成, 产物:"
ls -1 ./*.dmg ./*.tar.gz 2>/dev/null || true

echo
echo "提示: Intel Mac 打包请设 VCPKG_TRIPLET=x64-osx 并加 -DCMAKE_OSX_ARCHITECTURES=x86_64"
