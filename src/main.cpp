//
// 纯 PC 端三维数据浏览客户端 (方案 A: osgEarth + OpenSceneGraph)
// ===============================================================
// 本地直读: 矢量(OGR) / DOM 影像(GDAL) / DEM-DSM 地形(GDAL) / OSGB 白模
// 无服务层, 双击运行, 数据放本地磁盘。
//
// 用法:
//     pc_client [scene.earth] [models.json]
// 默认:
//     ./earth/scene.earth   ./config/models.json
//
// 交互:
//     鼠标左键拖动 = 旋转 / 平移 ; 滚轮 = 缩放
//     s = 性能统计 ; w = 线框 ; L = 光照开关 ; 空格 = 复位视角 ; ESC = 退出
//
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osgDB/ReadFile>

#include <osgEarth/MapNode>
#include <osgEarth/EarthManipulator>
#include <osgEarth/Notify>

#include "SceneLoader.h"
#include "PerfHud.h"

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <cstdlib>

using namespace osgEarth;
using namespace osgEarth::Util;

// 让打包后的程序自包含: 把随包携带的 gdal-data / proj-data / 插件目录
// 通过环境变量告诉 GDAL/PROJ/OSG, 无需用户机器预装。
static void setupPortableEnv(const char* argv0)
{
    const std::string exeDir =
        osgDB::getRealPath(osgDB::getFilePath(argv0));
    if (exeDir.empty()) return;

    auto setIfExists = [&](const char* var, const std::string& sub)
    {
        const std::string p = exeDir + "/" + sub;
        if (osgDB::fileExists(p))
        {
#ifdef _WIN32
            _putenv_s(var, p.c_str());
#else
            setenv(var, p.c_str(), 1);
#endif
            OE_NOTICE << "[Env] " << var << " = " << p << std::endl;
        }
    };
    setIfExists("GDAL_DATA", "gdal-data");
    setIfExists("PROJ_LIB",  "proj-data");
    setIfExists("PROJ_DATA", "proj-data");
    // OSG 插件在 bin/osgPlugins-*, OSG 默认会搜索 exe 同级目录, 一般无需额外设置
}

int main(int argc, char** argv)
{
    osg::ArgumentParser args(&argc, argv);

    // 打包自包含: 定位随包的 gdal-data/proj-data (必须在 osgEarth::initialize 之前)
    setupPortableEnv(argv[0]);

    // osgEarth 3.x 初始化 (注册 GDAL/OGR 驱动等)
    osgEarth::initialize();

    const std::string earthFile =
        (argc > 1) ? argv[1] : "./earth/scene.earth";
    const std::string modelsFile =
        (argc > 2) ? argv[2] : "./config/models.json";

    // ---------- 1. 载入 earth (矢量/影像/地形) ----------
    osg::ref_ptr<osg::Node> root;
    MapNode* mapNode = SceneLoader::loadEarth(earthFile, root);
    if (!mapNode)
    {
        OE_WARN << "启动失败: 无法载入 " << earthFile << std::endl;
        return 1;
    }

    // ---------- 2. 载入 OSGB 白模 + 读取性能配置 ----------
    PerfConfig perf;
    SceneLoader::loadModels(modelsFile, mapNode, perf);

    // ---------- 3. 视图与操控器 ----------
    osgViewer::Viewer viewer(args);
    viewer.setSceneData(root.get());

    osg::ref_ptr<EarthManipulator> manip = new EarthManipulator(args);
    viewer.setCameraManipulator(manip.get());
    // EarthManipulator 会自动读取 earth 内的 <viewpoints> 并飞到第 0 个视点。

    // ---------- 4. 流畅度保障 (性能调优) ----------
    PerfHud::apply(viewer, perf);

    // ---------- 5. HUD 与常用事件 ----------
    if (osg::ref_ptr<osg::Node> hud = PerfHud::createHud(viewer))
    {
        // HUD 挂到根组
        if (osg::Group* g = root->asGroup())
            g->addChild(hud.get());
    }
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);         // 全屏切换 'f'
    viewer.addEventHandler(new osgViewer::ThreadingHandler);
    viewer.addEventHandler(new osgViewer::LODScaleHandler);          // '/' '*' 调 LOD
    viewer.addEventHandler(new osgGA::StateSetManipulator(
        viewer.getCamera()->getOrCreateStateSet()));                 // 'w' 线框 / 'L' 光照

    // ---------- 6. 运行 ----------
    OE_NOTICE << "===== 客户端启动完成, 开始渲染 =====" << std::endl;
    viewer.setReleaseContextAtEndOfFrameHint(false);
    return viewer.run();
}
