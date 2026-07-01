#pragma once
//
// SceneLoader —— 场景装配器
// 1) 载入 .earth (矢量/影像/地形, GDAL/OGR 本地直读)
// 2) 解析 config/models.json, 逐个加载 OSGB 白模并挂入地图
// 3) 读出性能参数供 main 应用
//
#include <string>
#include <vector>
#include <osg/ref_ptr>
#include <osg/Group>

namespace osgEarth { class MapNode; }

struct PerfConfig
{
    int    targetFrameRate       = 60;
    double lodScale              = 1.0;
    int    databasePagerThreads  = 4;
    int    maxPagedLodInMemory   = 0;     // 0 = 不限制
    bool   smallFeatureCulling   = true;
    bool   incrementalCompile    = true;
};

class SceneLoader
{
public:
    // 载入 earth 文件, 返回 MapNode (失败返回 nullptr)
    static osgEarth::MapNode* loadEarth(const std::string& earthFile,
                                        osg::ref_ptr<osg::Node>& outRoot);

    // 解析 models.json, 把启用的 OSGB 白模加入 mapNode; 同时输出性能配置
    static void loadModels(const std::string& jsonFile,
                           osgEarth::MapNode* mapNode,
                           PerfConfig& outPerf);
};
