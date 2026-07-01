#!/usr/bin/env bash
# macOS 打包: 生成 .dmg (拖拽安装) 与 .tar.gz
# 依赖(推荐 conda-forge 预编译, 免源码编译):
#   conda create -n build -c conda-forge osgearth gdal cmake ninja
#   conda activate build
# (或用 vcpkg: 设 VCPKG_ROOT, 见 README)
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="build-mac"

echo "==> 配置 (Release)"
if [ -n "${CONDA_PREFIX:-}" ]; then
    echo "   使用 conda 环境: $CONDA_PREFIX"
    cmake -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$CONDA_PREFIX"
elif [ -n "${VCPKG_ROOT:-}" ]; then
    echo "   使用 vcpkg: $VCPKG_ROOT"
    cmake -B "$BUILD_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_TARGET_TRIPLET="${VCPKG_TRIPLET:-arm64-osx}" \
        -DCMAKE_BUILD_TYPE=Release
else
    echo "   (未检测到 conda/vcpkg, 假定 osgEarth 已在系统路径)"
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

echo "==> 编译"
cmake --build "$BUILD_DIR" --config Release -j

echo "==> 打包 (CPack: DragNDrop + TGZ)"
cd "$BUILD_DIR"
cpack -G "DragNDrop;TGZ"

echo "==> 完成, 产物:"
ls -1 ./*.dmg ./*.tar.gz 2>/dev/null || true
