cmake_minimum_required(VERSION 3.16)
project(OsgEarthPCClient VERSION 1.0.0 LANGUAGES CXX)

# ------------------------------------------------------------------
# 纯 PC 端三维数据浏览客户端 (方案 A: osgEarth + OpenSceneGraph)
# 本地直读: 矢量 / DOM 影像 / DEM-DSM 地形 / OSGB 白模
# ------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 输出目录统一到 bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ------------------------------------------------------------------
# 依赖查找: OpenSceneGraph + osgEarth
# 如果 CMake 找不到, 请设置环境变量:
#   OSG_DIR      -> OpenSceneGraph 安装根目录
#   OSGEARTH_DIR -> osgEarth 安装根目录
# 或给 cmake 传 -DCMAKE_PREFIX_PATH="/path/osg;/path/osgearth"
# ------------------------------------------------------------------
find_package(OpenSceneGraph 3.6 REQUIRED
    osg osgDB osgGA osgViewer osgUtil osgText osgShadow)

# osgEarth 3.x 提供 config 包; 若失败退回手动查找
find_package(osgEarth CONFIG QUIET)
if(NOT osgEarth_FOUND)
    message(STATUS "osgEarth config package 未找到, 尝试手动定位头文件与库")
    find_path(OSGEARTH_INCLUDE_DIR osgEarth/MapNode
        HINTS $ENV{OSGEARTH_DIR}/include /usr/local/include /opt/homebrew/include)
    find_library(OSGEARTH_LIBRARY NAMES osgEarth
        HINTS $ENV{OSGEARTH_DIR}/lib /usr/local/lib /opt/homebrew/lib)
    if(NOT OSGEARTH_INCLUDE_DIR OR NOT OSGEARTH_LIBRARY)
        message(FATAL_ERROR "找不到 osgEarth, 请设置 OSGEARTH_DIR 环境变量")
    endif()
endif()

# ------------------------------------------------------------------
# 可执行文件
# ------------------------------------------------------------------
add_executable(pc_client
    src/main.cpp
    src/SceneLoader.cpp
    src/OsgbModelLoader.cpp
    src/PerfHud.cpp
)

target_include_directories(pc_client PRIVATE
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${OSGEARTH_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(osgEarth_FOUND)
    target_link_libraries(pc_client PRIVATE
        ${OPENSCENEGRAPH_LIBRARIES}
        osgEarth::osgEarth)
else()
    target_link_libraries(pc_client PRIVATE
        ${OPENSCENEGRAPH_LIBRARIES}
        ${OSGEARTH_LIBRARY})
endif()

# 把 earth 配置与示例资源拷贝到运行目录
add_custom_command(TARGET pc_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/earth  $<TARGET_FILE_DIR:pc_client>/earth
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:pc_client>/config
    COMMENT "拷贝 earth / config 到运行目录")

install(TARGETS pc_client RUNTIME DESTINATION bin)
install(DIRECTORY earth  DESTINATION bin)
install(DIRECTORY config DESTINATION bin)

# ==================================================================
# 运行时依赖收集 + 打包 (Windows x64 zip/NSIS, macOS drag-n-drop dmg)
# ------------------------------------------------------------------
# 难点: 必须把 OSG 插件(osgdb_*)、osgEarth/GDAL 动态库、GDAL proj.db
# 一起打进包, 否则用户机器上没装库就无法读 osgb/tif/shp。
# ==================================================================

# ---- 定位 OSG 插件目录 (osgdb_osgb / osgdb_gdal / osgdb_ogr ...) ----
# OSG 插件通常在 <osg>/lib/osgPlugins-<ver> 或 bin 下
get_filename_component(_osg_lib_dir "${OSG_LIBRARY}" DIRECTORY)
if(NOT _osg_lib_dir)
    # 从 osgDB 库路径推断
    get_filename_component(_osg_lib_dir "${OSGDB_LIBRARY}" DIRECTORY)
endif()

# 额外搜索 conda-forge / vcpkg 布局 (预编译二进制)
# vcpkg manifest 模式安装树位于 <build>/vcpkg_installed/<triplet>/
file(GLOB _osg_plugin_dirs
    "${_osg_lib_dir}/osgPlugins-*"
    "${_osg_lib_dir}/../lib/osgPlugins-*"
    "${_osg_lib_dir}/../bin/osgPlugins-*"
    "$ENV{CONDA_PREFIX}/lib/osgPlugins-*"
    "$ENV{CONDA_PREFIX}/Library/bin/osgPlugins-*"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/*/plugins/osgPlugins-*"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/*/bin/osgPlugins-*"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/*/tools/osg/osgPlugins-*")

if(_osg_plugin_dirs)
    list(GET _osg_plugin_dirs 0 OSG_PLUGIN_DIR)
    message(STATUS "OSG 插件目录: ${OSG_PLUGIN_DIR}")
    # 插件放到与可执行文件相对的位置, OSG 会自动搜索
    if(APPLE)
        install(DIRECTORY "${OSG_PLUGIN_DIR}" DESTINATION bin)
    elseif(WIN32)
        install(DIRECTORY "${OSG_PLUGIN_DIR}" DESTINATION bin)
    else()
        install(DIRECTORY "${OSG_PLUGIN_DIR}" DESTINATION bin)
    endif()
else()
    message(WARNING "未找到 OSG 插件目录, 打包后可能无法读取 osgb/gdal 数据. "
                    "请手动把 osgPlugins-<ver> 拷入包的 bin/ 下")
endif()

# ---- 用 CMake 的 file(GET_RUNTIME_DEPENDENCIES) 收集动态库 ----
# 额外搜索目录: 预编译库(conda/vcpkg)所在的 lib/bin, CI 可通过 -DRUNTIME_SEARCH_DIRS 追加
set(RUNTIME_SEARCH_DIRS "" CACHE STRING "额外的动态库搜索目录(分号分隔), 用于依赖收集")
list(APPEND RUNTIME_SEARCH_DIRS
    "$ENV{CONDA_PREFIX}/lib"
    "$ENV{CONDA_PREFIX}/Library/bin")
# vcpkg manifest 安装树的 bin/lib (含 osgEarth/GDAL 的 dll/dylib)
file(GLOB _vcpkg_bin_dirs
    "${CMAKE_BINARY_DIR}/vcpkg_installed/*/bin"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/*/lib")
list(APPEND RUNTIME_SEARCH_DIRS ${_vcpkg_bin_dirs})

# 在 install 阶段扫描 exe 依赖的 dll/dylib 并拷入包
install(CODE "
    message(STATUS \"收集运行时依赖 ...\")
    file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES \"$<TARGET_FILE:pc_client>\"
        RESOLVED_DEPENDENCIES_VAR _res
        UNRESOLVED_DEPENDENCIES_VAR _unres
        DIRECTORIES \"\${CMAKE_INSTALL_PREFIX}/bin\" ${RUNTIME_SEARCH_DIRS}
        PRE_EXCLUDE_REGEXES \"api-ms-.*\" \"ext-ms-.*\"
        POST_EXCLUDE_REGEXES \".*system32.*\" \".*/usr/lib/.*\" \".*/System/.*\")
    foreach(_dep \${_res})
        file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\"
             TYPE SHARED_LIBRARY FOLLOW_SYMLINK_CHAIN FILES \"\${_dep}\")
    endforeach()
    if(_unres)
        message(STATUS \"未解析(通常是系统库, 可忽略): \${_unres}\")
    endif()
")

# ---- GDAL 数据 (proj.db 等), 缺了会导致投影/坐标转换失败 ----
# 兼容 conda-forge 与 vcpkg manifest 安装树的布局
file(GLOB _vcpkg_gdal_data "${CMAKE_BINARY_DIR}/vcpkg_installed/*/share/gdal")
file(GLOB _vcpkg_proj_data "${CMAKE_BINARY_DIR}/vcpkg_installed/*/share/proj")
find_path(GDAL_DATA_DIR gdalvrt.xsd
    HINTS $ENV{GDAL_DATA} ${_vcpkg_gdal_data}
          $ENV{CONDA_PREFIX}/share/gdal $ENV{CONDA_PREFIX}/Library/share/gdal
          /usr/local/share/gdal /opt/homebrew/share/gdal /usr/share/gdal)
find_path(PROJ_DATA_DIR proj.db
    HINTS $ENV{PROJ_LIB} $ENV{PROJ_DATA} ${_vcpkg_proj_data}
          $ENV{CONDA_PREFIX}/share/proj $ENV{CONDA_PREFIX}/Library/share/proj
          /usr/local/share/proj /opt/homebrew/share/proj /usr/share/proj)
if(GDAL_DATA_DIR)
    install(DIRECTORY "${GDAL_DATA_DIR}/" DESTINATION bin/gdal-data)
endif()
if(PROJ_DATA_DIR)
    install(DIRECTORY "${PROJ_DATA_DIR}/" DESTINATION bin/proj-data)
endif()

# ==================================================================
# CPack: 生成安装包 / 压缩包
# ==================================================================
set(CPACK_PACKAGE_NAME "PC三维数据浏览客户端")
set(CPACK_PACKAGE_VENDOR "YourOrg")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "纯PC端本地三维数据浏览客户端 (osgEarth)")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OsgEarthPCClient")
set(CPACK_PACKAGE_EXECUTABLES "pc_client" "PC三维客户端")

if(WIN32)
    # 同时生成 NSIS 安装程序和绿色 zip
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
elseif(APPLE)
    # 生成 .dmg 拖拽安装 (以及 tar.gz 备用)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_VOLUME_NAME "PC三维客户端")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

