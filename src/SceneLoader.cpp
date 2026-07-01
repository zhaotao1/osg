#include "SceneLoader.h"
#include "OsgbModelLoader.h"

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <osgEarth/MapNode>
#include <osgEarth/Map>
#include <osgEarth/Notify>
#include <osgEarth/JsonUtils>

#include <fstream>
#include <sstream>

using namespace osgEarth;

MapNode* SceneLoader::loadEarth(const std::string& earthFile,
                                osg::ref_ptr<osg::Node>& outRoot)
{
    OE_NOTICE << "[Scene] 载入 earth: " << earthFile << std::endl;
    osg::ref_ptr<osg::Node> node = osgDB::readRefNodeFile(earthFile);
    if (!node.valid())
    {
        OE_WARN << "[Scene] earth 载入失败: " << earthFile << std::endl;
        return nullptr;
    }
    MapNode* mapNode = MapNode::get(node.get());
    if (!mapNode)
    {
        OE_WARN << "[Scene] 文件不是有效的 MapNode: " << earthFile << std::endl;
        return nullptr;
    }
    outRoot = node;
    return mapNode;
}

static std::string readWholeFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void SceneLoader::loadModels(const std::string& jsonFile,
                             MapNode* mapNode,
                             PerfConfig& outPerf)
{
    if (!mapNode) return;

    const std::string text = readWholeFile(jsonFile);
    if (text.empty())
    {
        OE_WARN << "[Scene] 无 models.json (跳过白模): " << jsonFile << std::endl;
        return;
    }

    Json::Reader reader;
    Json::Value  root;
    if (!reader.parse(text, root))
    {
        OE_WARN << "[Scene] models.json 解析失败" << std::endl;
        return;
    }

    // ---- 性能参数 ----
    if (root.isMember("performance"))
    {
        const Json::Value& p = root["performance"];
        outPerf.targetFrameRate      = p.get("targetFrameRate", outPerf.targetFrameRate).asInt();
        outPerf.lodScale             = p.get("lodScale", outPerf.lodScale).asDouble();
        outPerf.databasePagerThreads = p.get("databasePagerThreads", outPerf.databasePagerThreads).asInt();
        outPerf.maxPagedLodInMemory  = p.get("maxPagedLodInMemory", outPerf.maxPagedLodInMemory).asInt();
        outPerf.smallFeatureCulling  = p.get("smallFeatureCulling", outPerf.smallFeatureCulling).asBool();
        outPerf.incrementalCompile   = p.get("incrementalCompile", outPerf.incrementalCompile).asBool();
    }

    // ---- 白模列表 ----
    if (!root.isMember("models"))
    {
        OE_NOTICE << "[Scene] models.json 无 models 数组" << std::endl;
        return;
    }

    const Json::Value& models = root["models"];
    for (Json::Value::const_iterator it = models.begin(); it != models.end(); ++it)
    {
        const Json::Value& m = *it;

        ModelDef def;
        def.name    = m.get("name", "OSGB").asString();
        def.path    = m.get("path", "").asString();
        def.enabled = m.get("enabled", true).asBool();

        if (m.isMember("origin"))
        {
            const Json::Value& o = m["origin"];
            def.srs       = o.get("srs", "EPSG:4326").asString();
            def.longitude = o.get("longitude", 0.0).asDouble();
            def.latitude  = o.get("latitude", 0.0).asDouble();
            def.height    = o.get("height", 0.0).asDouble();
        }

        if (!def.enabled || def.path.empty()) continue;

        osg::ref_ptr<osg::Node> node = OsgbModelLoader::load(def);
        if (node.valid())
            mapNode->addChild(node);
    }
}
