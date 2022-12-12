/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "TerrainNode.h"
#include "TerrainTileNode.h"
#include "TerrainContext.h"
#include <rocky/TileKey.h>
#include <rocky/Threading.h>
#include <rocky/ImageLayer.h>

#include <vsg/all.h>

using namespace rocky;
using namespace rocky::util;

#define ARENA_LOAD_TILE "terrain.load_tile"

TerrainNode::TerrainNode(RuntimeContext& new_runtime, const Config& conf) :
    vsg::Inherit<vsg::Group, TerrainNode>(),
    TerrainSettings(conf),
    _runtime(new_runtime)
{
    construct(conf);
}

void
TerrainNode::construct(const Config& conf)
{
    //nop
}

Config
TerrainNode::getConfig() const
{
    Config conf("terrain");
    TerrainSettings::saveToConfig(conf);
    return conf;
}

void
TerrainNode::setMap(shared_ptr<Map> new_map, IOControl* ioc)
{
    ROCKY_SOFT_ASSERT_AND_RETURN(new_map, void());

    // create a new context for this map
    _context = std::make_shared<TerrainContext>(
        new_map,
        _runtime,
        *this, // settings
        this); // host

    // remove everything and start over
    this->children.clear();

    _tilesRoot = vsg::Group::create();

    std::vector<TileKey> keys;
    Profile::getAllKeysAtLOD(this->firstLOD, new_map->getProfile(), keys);

    for (unsigned i = 0; i < keys.size(); ++i)
    {
        auto tile = _context->tiles->createTile(
            keys[i],
            nullptr, // parent
            _context);

        tile->doNotExpire = true;

        // Add it to the scene graph
        _tilesRoot->addChild(tile);

#if 0 // temp
        // Post-add initialization:
        tileNode->initializeData(*this);
#endif 
    }

    // create the graphics pipeline to render this map
    auto stateGroup = _context->stateFactory->createTerrainStateGroup();
    stateGroup->addChild(_tilesRoot);
    this->addChild(stateGroup);
}

void
TerrainNode::traverse(vsg::RecordTraversal& nv) const
{
    // Pass the context along in the traversal
    nv.setObject("TerrainContext",
        static_cast<vsg::Object*>(
            const_cast<TerrainNode*>(this)));

    vsg::Group::traverse(nv);
}

void
TerrainNode::update(const vsg::FrameStamp* fs)
{
    _context->tiles->update(fs, _context);
}

void
TerrainNode::ping(
    TerrainTileNode* tile0,
    TerrainTileNode* tile1, 
    TerrainTileNode* tile2, 
    TerrainTileNode* tile3,
    vsg::RecordTraversal& nv)
{
    _context->tiles->ping(tile0, tile1, tile2, tile3, nv);
}
