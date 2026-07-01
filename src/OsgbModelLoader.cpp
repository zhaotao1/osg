#include "OsgbModelLoader.h"

#include <osg/Group>
#include <osg/ProxyNode>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

#include <osgEarth/GeoTransform>
#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>
#include <osgEarth/Registry>
#include <osgEarth/Notify>

#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

osg::ref_ptr<osg::Node> OsgbModelLoader::readRaw(const std::string& path)
{
    osg::ref_ptr<osg::Node> node = osgDB::readRefNodeFile(path);
    if (!node.valid())
        OE_WARN << "[OSGB] 读取失败: " << path << std::endl;
    return node;
}

osg::ref_ptr<osg::Node> OsgbModelLoader::readTiledDir(const std::string& dir)
{
    // Smart3D / ContextCapture 结构:  <dir>/Tile_+xxx_+yyy/Tile_+xxx_+yyy.osgb
    // 每个瓦片根 .osgb 本身是 PagedLOD, 读根节点很轻, 深层 LOD 由 DatabasePager 按需分页。
    osg::ref_ptr<osg::Group> group = new osg::Group;
    group->setName("OSGB-Tiles");

    if (!fs::exists(dir) || !fs::is_directory(dir))
    {
        OE_WARN << "[OSGB] 目录不存在: " << dir << std::endl;
        return nullptr;
    }

    size_t count = 0;
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_directory()) continue;

        const std::string tileName = entry.path().filename().string();
        // 优先匹配同名 .osgb; 否则取目录内任意一个 .osgb 作为入口
        std::string rootOsgb = (entry.path() / (tileName + ".osgb")).string();
        if (!fs::exists(rootOsgb))
        {
            rootOsgb.clear();
            for (const auto& f : fs::directory_iterator(entry.path()))
            {
                if (osgDB::getLowerCaseFileExtension(f.path().string()) == "osgb")
                {
                    rootOsgb = f.path().string();
                    break;
                }
            }
        }
        if (rootOsgb.empty()) continue;

        // 用 ProxyNode 延迟加载, 交给分页系统流式调度, 避免一次性吃满内存
        osg::ref_ptr<osg::ProxyNode> proxy = new osg::ProxyNode;
        proxy->setLoadingExternalReferenceMode(
            osg::ProxyNode::DEFER_LOADING_TO_DATABASE_PAGER);
        proxy->setFileName(0, rootOsgb);
        group->addChild(proxy);
        ++count;
    }

    if (count == 0)
    {
        OE_WARN << "[OSGB] 目录内未找到瓦片: " << dir << std::endl;
        return nullptr;
    }

    OE_NOTICE << "[OSGB] 载入瓦片数量: " << count << " @ " << dir << std::endl;
    return group;
}

osg::ref_ptr<osg::Node> OsgbModelLoader::load(const ModelDef& def)
{
    if (!def.enabled) return nullptr;

    // 1) 读取几何 (文件 or 目录)
    osg::ref_ptr<osg::Node> raw;
    if (fs::is_directory(def.path))
        raw = readTiledDir(def.path);
    else
        raw = readRaw(def.path);

    if (!raw.valid()) return nullptr;

    // 2) 用 GeoTransform 把局部 ENU 模型定位到地球经纬度
    const osgEarth::SpatialReference* srs =
        osgEarth::SpatialReference::get(def.srs);
    if (!srs)
    {
        OE_WARN << "[OSGB] 无效 SRS: " << def.srs << std::endl;
        return raw; // 退而求其次: 不定位直接返回
    }

    osgEarth::GeoPoint origin(
        srs, def.longitude, def.latitude, def.height,
        osgEarth::ALTMODE_ABSOLUTE);

    osg::ref_ptr<osgEarth::GeoTransform> xform = new osgEarth::GeoTransform;
    xform->setName(def.name);
    xform->setPosition(origin);
    xform->addChild(raw);

    OE_NOTICE << "[OSGB] 定位 '" << def.name << "' -> "
              << def.longitude << ", " << def.latitude
              << " (" << def.srs << ")" << std::endl;
    return xform;
}
