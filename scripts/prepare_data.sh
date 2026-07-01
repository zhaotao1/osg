#!/usr/bin/env bash
# 本地数据预处理 —— 建金字塔 / 空间索引 / 影像镶嵌
# 大数据流畅度的根本: 离线跑一次即可。需已安装 GDAL。
set -euo pipefail

DATA_DIR="${1:-./data}"

echo "==> 数据目录: $DATA_DIR"

# 1) DOM 影像: 建内部金字塔 (overview)
if compgen -G "$DATA_DIR/dom/*.tif" > /dev/null; then
    for tif in "$DATA_DIR"/dom/*.tif; do
        echo "==> gdaladdo 影像: $tif"
        gdaladdo -r average "$tif" 2 4 8 16 32 64
    done
    echo "==> 影像镶嵌 mosaic.vrt"
    gdalbuildvrt "$DATA_DIR/dom/mosaic.vrt" "$DATA_DIR"/dom/*.tif
fi

# 2) DEM / DSM: 建金字塔
if compgen -G "$DATA_DIR/dem/*.tif" > /dev/null; then
    for tif in "$DATA_DIR"/dem/*.tif; do
        echo "==> gdaladdo 地形: $tif"
        gdaladdo -r average "$tif" 2 4 8 16 32 64
    done
fi

# 3) 矢量 SHP: 建空间索引 (.qix)
if compgen -G "$DATA_DIR/vector/*.shp" > /dev/null; then
    for shp in "$DATA_DIR"/vector/*.shp; do
        echo "==> 空间索引: $shp"
        ogrinfo -sql "CREATE SPATIAL INDEX ON $(basename "${shp%.shp}")" "$shp" || true
    done
fi

echo "==> 预处理完成"
