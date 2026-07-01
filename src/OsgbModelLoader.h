#pragma once
//
// OsgbModelLoader —— 本地 OSGB 白模(倾斜摄影/实景三维)加载器
// 支持: 单个 .osgb 文件, 或 Smart3D 风格的瓦片目录(Data/Tile_*/Tile_*.osgb)
// 通过 osgEarth::GeoTransform 把局部 ENU 模型摆放到地球正确经纬度上。
//
#include <string>
#include <osg/Node>
#include <osg/ref_ptr>

namespace osgEarth { class MapNode; }

struct ModelDef
{
    std::string name;
    std::string path;          // OSGB 文件或数据集目录
    std::string srs = "EPSG:4326";
    double      longitude = 0.0;  // 或投影坐标 X
    double      latitude  = 0.0;  // 或投影坐标 Y
    double      height    = 0.0;
    bool        enabled   = true;
};

class OsgbModelLoader
{
public:
    // 加载并定位一个 OSGB 数据集; 失败返回 nullptr
    static osg::ref_ptr<osg::Node> load(const ModelDef& def);

private:
    // 读取原始 OSGB 几何(不含定位)
    static osg::ref_ptr<osg::Node> readRaw(const std::string& path);
    // 扫描瓦片目录, 把每个顶层瓦片作为可分页节点加入一个 Group
    static osg::ref_ptr<osg::Node> readTiledDir(const std::string& dir);
};
