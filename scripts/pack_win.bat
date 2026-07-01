@echo off
REM Windows x64 打包: 生成 NSIS 安装程序 + 绿色 zip
REM 依赖(推荐 conda-forge 预编译, 免源码编译):
REM   conda create -n build -c conda-forge osgearth gdal cmake ninja
REM   conda activate build
REM (或用 vcpkg: 设 VCPKG_ROOT)  需装 NSIS 才能出 .exe 安装包
setlocal
cd /d "%~dp0\.."

set BUILD_DIR=build-win
set VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

echo ==^> 配置 (x64 Release)
if defined CONDA_PREFIX (
    echo    使用 conda 环境: %CONDA_PREFIX%
    cmake -B %BUILD_DIR% -G Ninja ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_PREFIX_PATH="%CONDA_PREFIX%\Library"
) else if defined VCPKG_ROOT (
    echo    使用 vcpkg: %VCPKG_ROOT%
    cmake -B %BUILD_DIR% -A x64 ^
        -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" ^
        -DVCPKG_TARGET_TRIPLET=x64-windows ^
        -DCMAKE_BUILD_TYPE=Release
) else (
    cmake -B %BUILD_DIR% -A x64 -DCMAKE_BUILD_TYPE=Release
)
if errorlevel 1 goto :err

echo ==^> 编译
cmake --build %BUILD_DIR% --config Release -j
if errorlevel 1 goto :err

echo ==^> 打包 (CPack: ZIP + NSIS)
cd %BUILD_DIR%
cpack -G "ZIP;NSIS"
if errorlevel 1 goto :err

echo ==^> 完成, 产物在 %BUILD_DIR%\
dir /b *.zip *.exe 2>nul
goto :eof

:err
echo 构建失败, 请检查依赖 (osgEarth/GDAL) 是否安装, 或设置 CONDA_PREFIX / VCPKG_ROOT
exit /b 1
