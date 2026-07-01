# 纯 PC 端三维数据浏览客户端（方案 A：osgEarth + OpenSceneGraph）

本地直读 **矢量 / DOM 影像 / DEM-DSM 地形 / OSGB 白模**，无服务层，双击运行。
OSGB 倾斜摄影是 OSG 原生格式，**无需转换**，这是方案 A 的核心优势。

---

## 一、目录结构

```
osgearth-pc-client/
├── CMakeLists.txt          # 构建脚本
├── README.md
├── earth/
│   └── scene.earth         # 矢量/影像/地形 配置 (改成你的本地路径)
├── config/
│   └── models.json         # OSGB 白模列表 + 性能参数
├── scripts/
│   └── prepare_data.sh      # 数据预处理 (建金字塔/空间索引)
└── src/
    ├── main.cpp            # 入口: 装配场景 + 视图 + 运行
    ├── SceneLoader.*       # 载入 earth + 解析 models.json
    ├── OsgbModelLoader.*   # OSGB 白模加载与地理定位
    └── PerfHud.*           # 流畅度保障(分页/LOD/增量编译) + HUD
```

---

## 二、环境依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| OpenSceneGraph | ≥ 3.6 | 渲染内核，含 OSGB 插件 |
| osgEarth | ≥ 3.3 | 数字地球，含 GDAL/OGR 驱动 |
| GDAL | ≥ 3.x | 影像/DEM/矢量本地直读 |
| CMake | ≥ 3.16 | 构建 |
| 编译器 | 支持 C++17 | MSVC 2019+/GCC 9+/Clang 10+ |

### 安装依赖

**macOS (Homebrew)**
```bash
brew install open-scene-graph osgearth gdal cmake
```

**Ubuntu**
```bash
sudo apt install libopenscenegraph-dev libgdal-dev cmake build-essential
# osgEarth 建议源码编译或用 vcpkg
```

**Windows (vcpkg，推荐)**
```powershell
vcpkg install osgearth[core]:x64-windows
```

> 若 CMake 找不到 osgEarth，设置环境变量 `OSGEARTH_DIR` 与 `OSG_DIR`，
> 或传 `-DCMAKE_PREFIX_PATH="C:/osg;C:/osgearth"`。

---

## 三、构建与运行

```bash
cd osgearth-pc-client
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j

# 运行 (earth/config 已自动拷到可执行目录)
cd build/bin
./pc_client                       # 用默认 earth/scene.earth + config/models.json
./pc_client ./earth/scene.earth ./config/models.json
```

---

## 四、接入你自己的数据（3 步）

1. **放数据**：把影像/DEM/矢量/OSGB 拷到 `build/bin/data/` 下（或任意路径）。
2. **改 `earth/scene.earth`**：把 `<url>` 改成矢量、DOM、DEM 的真实路径。
3. **改 `config/models.json`**：填 OSGB 目录 `path` 和数据集原点 `origin`
   （经纬度/投影坐标，取自 OSGB 的 `metadata.xml`）。

### OSGB 定位说明（关键）
OSGB 是**局部 ENU 坐标**，必须给数据集原点才能落到地球正确位置：
- 打开 OSGB 数据集的 `metadata.xml`，看 `<SRS>` 和 `<SRSOrigin>`；
- 若是 `EPSG:4326` → `origin` 填经纬度；
- 若是投影带（如 `EPSG:4547` CGCS2000 3度带）→ `srs` 填该 EPSG，`longitude/latitude` 填投影 X/Y。

---

## 五、数据预处理（流畅度的根本）

即使本地直读，大数据也要**离线建金字塔一次**，否则照样卡。见 `scripts/prepare_data.sh`：

```bash
# 影像/DEM 建内部金字塔 (直读即带 LOD)
gdaladdo -r average data/dom/DOM.tif 2 4 8 16 32 64
gdaladdo -r average data/dem/DEM.tif 2 4 8 16 32 64

# 多景影像镶嵌成一个 VRT
gdalbuildvrt data/dom/mosaic.vrt data/dom/*.tif
```

---

## 六、流畅度保障（已内置）

`PerfHud.cpp` 集中做了这些优化，`config/models.json` 的 `performance` 段可调：

| 手段 | 作用 |
|------|------|
| DrawThreadPerContext 多线程 | 渲染与剔除并行，交互更顺 |
| DatabasePager 流式分页 | OSGB/瓦片按需加载，不一次性吃满内存 |
| 视野外瓦片过期释放 | 长时间漫游内存/显存不膨胀 |
| IncrementalCompile 增量编译 | 大模型入场分帧上传显存，不卡顿 |
| LODScale / 小物体剔除 | 远处降精度、微小物体不绘制 |
| 帧率上限 | 稳定省电 |

**运行期快捷键**：`s` 统计面板、`w` 线框、`L` 光照、`/ *` 调 LOD、`f` 全屏、`空格` 复位视角。

---

## 七、性能建议

- 数据放 **SSD**，瓶颈从网络变为磁盘 IO；
- 影像/DEM 一律先 `gdaladdo`；
- OSGB 若卡，调大 `models.json` 里 `lodScale`（如 1.5）提前降级；
- 显存吃紧时设 `maxPagedLodInMemory`（如 2000）限制常驻瓦片数。

---

## 八、打包分发（Windows / macOS）

打包已用 **CPack** 集成，并会自动收集运行时依赖（OSG 插件 `osgdb_*`、
osgEarth/GDAL 动态库、GDAL `proj.db` 数据），生成**自包含、可直接双击运行**的包。

### macOS —— 生成 `.dmg` / `.tar.gz`
```bash
brew install open-scene-graph osgearth gdal cmake
bash scripts/pack_mac.sh
# 产物: build-mac/*.dmg  build-mac/*.tar.gz
```
- Apple Silicon 默认打 **arm64** 包；要 Intel 包加 `-DCMAKE_OSX_ARCHITECTURES=x86_64`。
- 分发给别人若提示「已损坏」是未签名：右键打开，或 `xattr -dr com.apple.quarantine <App>`。

### Windows —— 生成 NSIS 安装包 `.exe` / 绿色 `.zip`
```powershell
# 推荐用 vcpkg 装依赖
vcpkg install osgearth[core]:x64-windows
set VCPKG_ROOT=C:\vcpkg
scripts\pack_win.bat
# 产物: build-win\*.exe (安装程序)  build-win\*.zip (免安装)
```
- 出 `.exe` 安装包需装 [NSIS](https://nsis.sourceforge.io)；只要 `.zip` 可忽略。
- 应用启动会把随包的 `gdal-data`/`proj-data` 设进环境变量，**无需目标机预装 GDAL**。

### 关于「Windows x86（32 位）」
> **不建议 32 位**：osgEarth/GDAL 的 32 位预编译库难凑齐，且大数据渲染受 32 位地址空间（≤4GB）限制极易 OOM。
> 生产请用 **x64**。确有 32 位需求时：`vcpkg install osgearth:x86-windows` 全量源码编译，
> 并把脚本里 `-A x64` 改 `-A Win32`、triplet 改 `x86-windows`。

### 打包常见坑
| 现象 | 原因 / 解决 |
|------|------------|
| 双击闪退、读不了 osgb/tif | OSG 插件 `osgPlugins-*` 没进包 → 看 CMake 是否输出 `STATUS OSG 插件目录` |
| 坐标错乱 / 投影报错 | 缺 `proj.db` → 确认包内有 `proj-data/proj.db` |
| Win 提示缺 dll | 依赖收集不全 → `dumpbin /dependents pc_client.exe` 排查后补进 `bin/` |
| Mac 报「已损坏」 | 未签名/公证 → 临时用 `xattr` 去隔离；正式分发需 Apple 开发者签名+公证 |

---

## 九、CI 自动打包（GitHub Actions）

`.github/workflows/build.yml` 已配置好，**同时在 macOS 与 Windows runner 上构建打包**
（跨平台包不能交叉编译，必须各自平台产出）。

| 触发方式 | 行为 |
|----------|------|
| 推送 tag `v*`（`git tag v1.0.0 && git push --tags`） | 两平台打包 + 自动发布到 GitHub Release |
| 手动触发（Actions 页 `Run workflow`） | 两平台打包，产物在 Artifacts |
| PR 到 main/master | 构建验证（不发布） |

**产物**：`mac-package`（`.dmg`/`.tar.gz`）、`windows-package`（`.exe`/`.zip`）。

Windows 用 **vcpkg** 装 osgEarth 并**缓存**依赖（首次慢，之后走缓存）；macOS 用 **Homebrew**。
发布 Release 需仓库 Actions 有 `contents: write` 权限。
