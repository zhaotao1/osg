#pragma once
//
// PerfHud —— 流畅度保障 (性能调优) 与屏幕帧率 HUD
// 集中应用 DatabasePager / LOD / 增量编译 等设置, 保证大数据交互流畅。
//
#include <osgViewer/Viewer>
#include "SceneLoader.h"

namespace PerfHud
{
    // 把性能配置应用到 viewer (分页线程/LOD 缩放/多线程模型/近远裁剪等)
    void apply(osgViewer::Viewer& viewer, const PerfConfig& cfg);

    // 创建一个左上角 FPS/瓦片数 HUD 相机, 返回可加入场景的节点
    osg::ref_ptr<osg::Node> createHud(osgViewer::Viewer& viewer);
}
