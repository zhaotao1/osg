#!/usr/bin/env bash
# macOS 打包: 生成 .dmg (拖拽安装) 与 .tar.gz
# 依赖: brew install open-scene-graph osgearth gdal cmake
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="build-mac"
echo "==> 配置 (Release)"
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "==> 编译"
cmake --build "$BUILD_DIR" --config Release -j

echo "==> 打包 (CPack: DragNDrop + TGZ)"
cd "$BUILD_DIR"
cpack -G "DragNDrop;TGZ"

echo "==> 完成, 产物:"
ls -1 ./*.dmg ./*.tar.gz 2>/dev/null || true

echo
echo "提示: 若要在未装 osgEarth 的 Mac 上运行, 依赖已由 CMake 收集进包。"
echo "     Apple Silicon 上默认打 arm64 包; 需要 Intel 包请加:"
echo "     cmake -B build-mac -DCMAKE_OSX_ARCHITECTURES=x86_64 ..."
