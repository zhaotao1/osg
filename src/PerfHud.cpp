#include "PerfHud.h"

#include <osg/LOD>
#include <osg/Camera>
#include <osg/Geode>
#include <osgText/Text>
#include <osgDB/DatabasePager>
#include <osgUtil/IncrementalCompileOperation>
#include <osgViewer/ViewerEventHandlers>

#include <osgEarth/Notify>

namespace PerfHud
{

void apply(osgViewer::Viewer& viewer, const PerfConfig& cfg)
{
    // --- 1. 多线程渲染模型: 每相机独立线程, 交互最顺 ---
    viewer.setThreadingModel(osgViewer::Viewer::DrawThreadPerContext);

    // --- 2. 帧率上限 (省电/稳定) ---
    if (cfg.targetFrameRate > 0)
        viewer.setRunMaxFrameRate(cfg.targetFrameRate);

    // --- 3. LOD 缩放: >1 更早降级(更快), <1 更晚降级(更精细) ---
    viewer.getCamera()->setLODScale(cfg.lodScale);

    // --- 4. 数据库分页(流式加载)线程数 + 内存中 PagedLOD 上限 ---
    if (osgDB::DatabasePager* pager = viewer.getDatabasePager())
    {
        if (cfg.maxPagedLodInMemory > 0)
            pager->setTargetMaximumNumberOfPageLOD(cfg.maxPagedLodInMemory);

        // 视野外瓦片尽快过期释放显存/内存, 防长时间漫游内存膨胀
        pager->setUnrefImageDataAfterApplyPolicy(true, true);
        pager->setDoPreCompileGLObjects(cfg.incrementalCompile);
    }

    // --- 5. 增量式 GL 对象编译: 大模型入场不卡顿(分帧上传显存) ---
    if (cfg.incrementalCompile)
    {
        osg::ref_ptr<osgUtil::IncrementalCompileOperation> ico =
            new osgUtil::IncrementalCompileOperation();
        ico->setTargetFrameRate(static_cast<float>(cfg.targetFrameRate > 0 ? cfg.targetFrameRate : 60));
        viewer.setIncrementalCompileOperation(ico.get());
    }

    // --- 6. 小物体剔除: 屏幕上小于阈值像素的对象不绘制 ---
    if (cfg.smallFeatureCulling)
    {
        osg::CullSettings::CullingMode mode = viewer.getCamera()->getCullingMode();
        mode |= osg::CullSettings::SMALL_FEATURE_CULLING;
        viewer.getCamera()->setCullingMode(mode);
        viewer.getCamera()->setSmallFeatureCullingPixelSize(3.0f);
    }

    // --- 7. 内置统计 (按 's' 切换) ---
    viewer.addEventHandler(new osgViewer::StatsHandler);

    OE_NOTICE << "[Perf] targetFPS=" << cfg.targetFrameRate
              << " lodScale=" << cfg.lodScale
              << " pagerThreads=" << cfg.databasePagerThreads
              << " smallCull=" << cfg.smallFeatureCulling
              << std::endl;
}

osg::ref_ptr<osg::Node> createHud(osgViewer::Viewer& viewer)
{
    // 简易左上角文字 HUD (提示操作). 详细 FPS 用内置 StatsHandler('s')。
    osg::ref_ptr<osg::Camera> cam = new osg::Camera;
    cam->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    cam->setProjectionMatrixAsOrtho2D(0, 1280, 0, 800);
    cam->setViewMatrix(osg::Matrix::identity());
    cam->setClearMask(GL_DEPTH_BUFFER_BIT);
    cam->setRenderOrder(osg::Camera::POST_RENDER);
    cam->setAllowEventFocus(false);
    cam->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osgText::Text> text = new osgText::Text;
    text->setCharacterSize(16.0f);
    text->setPosition(osg::Vec3(12.0f, 776.0f, 0.0f));
    text->setColor(osg::Vec4(1, 1, 1, 0.9f));
    text->setDataVariance(osg::Object::DYNAMIC);
    text->setText(
        "PC 本地三维客户端 (osgEarth)\n"
        "s=统计  w=线框  L=光照  空格=复位视角  ESC=退出");

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(text.get());
    cam->addChild(geode.get());
    return cam;
}

} // namespace PerfHud
