/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "TMS.h"

#ifdef TINYXML_FOUND
#include <tinyxml.h>
#endif

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace ROCKY_NAMESPACE;
using namespace ROCKY_NAMESPACE::TMS;


#define ELEM_TILEMAP "tilemap"
#define ELEM_TITLE "title"
#define ELEM_ABSTRACT "abstract"
#define ELEM_SRS "srs"
#define ELEM_VERTICAL_SRS "vsrs"
#define ELEM_VERTICAL_DATUM "vdatum"
#define ELEM_BOUNDINGBOX "boundingbox"
#define ELEM_ORIGIN "origin"
#define ELEM_TILE_FORMAT "tileformat"
#define ELEM_TILESETS "tilesets"
#define ELEM_TILESET "tileset"
#define ELEM_DATA_EXTENTS "dataextents"
#define ELEM_DATA_EXTENT "dataextent"

#define ATTR_VERSION "version"
#define ATTR_TILEMAPSERVICE "tilemapservice"

#define ATTR_MINX "minx"
#define ATTR_MINY "miny"
#define ATTR_MAXX "maxx"
#define ATTR_MAXY "maxy"
#define ATTR_X "x"
#define ATTR_Y "y"
#define ATTR_MIN_LEVEL "minlevel"
#define ATTR_MAX_LEVEL "maxlevel"

#define ATTR_WIDTH "width"
#define ATTR_HEIGHT "height"
#define ATTR_MIME_TYPE "mime-type"
#define ATTR_EXTENSION "extension"

#define ATTR_PROFILE "profile"

#define ATTR_HREF "href"
#define ATTR_ORDER "order"
#define ATTR_UNITSPERPIXEL "units-per-pixel"

#define ATTR_DESCRIPTION "description"

namespace
{
    bool intersects(const double& minXa, const double& minYa, const double& maxXa, const double& maxYa,
        const double& minXb, const double& minYb, const double& maxXb, const double& maxYb)
    {
        return std::max(minXa, minXb) <= std::min(maxXa, maxXb) &&
            std::max(minYa, minYb) <= std::min(maxYa, maxYb);
    }

    std::string getHorizSRSString(const SRS& srs)
    {
        if (srs.isHorizEquivalentTo(SRS::SPHERICAL_MERCATOR))
        {
            return "EPSG:900913";
        }
        else if (srs.isGeodetic())
        {
            return "EPSG:4326";
        }
        else
        {
            return srs.definition();
        }
    }

    Result<TileMap> parseTileMap(const json& j)
    {
        auto tileMap = j.find(ELEM_TILEMAP);
    }

    std::string getChildTextValue(const TiXmlElement* node)
    {
        auto text = dynamic_cast<const TiXmlText*>(node->FirstChild());
        return text ? text->Value() : "";
    }

    Result<TileMap> parseTileMapFromXML(const std::string& xml)
    {
        TileMap tilemap;

        TiXmlDocument doc;
        doc.Parse(xml.c_str());
        if (doc.Error())
        {
            return Status(Status::GeneralError, util::make_string()
                << "XML parse error at row " << doc.ErrorRow()
                << " col " << doc.ErrorCol());
        }

        auto tilemapxml = doc.RootElement();
        std::string name = tilemapxml->Value();
        if (!tilemapxml || name != "TileMap")
            return Status(Status::ConfigurationError, "XML missing TileMap element");

        tilemapxml->QueryStringAttribute("version", &tilemap.version);
        tilemapxml->QueryStringAttribute("tilemapservice", &tilemap.tileMapService);

        for (const TiXmlNode* childnode = tilemapxml->FirstChild(); 
            childnode != nullptr; 
            childnode = childnode->NextSibling())
        {
            auto childxml = childnode->ToElement();
            if (childxml)
            {
                std::string name = childxml->Value();
                if (name == "Abstract")
                {
                    tilemap.abstract = getChildTextValue(childxml);
                }
                else if (name == "Title")
                {
                    tilemap.title = getChildTextValue(childxml);
                }
                else if (name == "SRS")
                {
                    tilemap.srsString = getChildTextValue(childxml);
                }
                else if (name == "BoundingBox")
                {
                    childxml->QueryDoubleAttribute("minx", &tilemap.minX);
                    childxml->QueryDoubleAttribute("miny", &tilemap.minY);
                    childxml->QueryDoubleAttribute("maxx", &tilemap.maxX);
                    childxml->QueryDoubleAttribute("maxy", &tilemap.maxY);
                }
                else if (name == "Origin")
                {
                    childxml->QueryDoubleAttribute("x", &tilemap.originX);
                    childxml->QueryDoubleAttribute("y", &tilemap.originY);
                }
                else if (name == "TileFormat")
                {
                    childxml->QueryUnsignedAttribute("width", &tilemap.format.width);
                    childxml->QueryUnsignedAttribute("height", &tilemap.format.height);
                    childxml->QueryStringAttribute("mime-type", &tilemap.format.mimeType);
                    childxml->QueryStringAttribute("extension", &tilemap.format.extension);
                }
                else if (name == "TileSets")
                {
                    std::string temp;
                    childxml->QueryStringAttribute("profile", &temp);
                    tilemap.profileType =
                        temp == "global-geodetic" ? ProfileType::GEODETIC :
                        temp == "global-mercator" ? ProfileType::MERCATOR :
                        temp == "local" ? ProfileType::LOCAL :
                        ProfileType::UNKNOWN;

                    for (const TiXmlNode* tilesetnode = childxml->FirstChild();
                        tilesetnode != nullptr;
                        tilesetnode = tilesetnode->NextSibling())
                    {
                        auto tilesetxml = tilesetnode->ToElement();
                        if (tilesetxml)
                        {
                            TileSet tileset;
                            tilesetxml->QueryStringAttribute("href", &tileset.href);
                            tilesetxml->QueryDoubleAttribute("units-per-pixel", &tileset.unitsPerPixel);
                            tilesetxml->QueryUnsignedAttribute("order", &tileset.order);
                            tilemap.tileSets.push_back(std::move(tileset));
                        }
                    }
                }
                else if (name == "DataExtents")
                {
                    Profile profile = tilemap.createProfile();

                    for (const TiXmlNode* denode = childxml->FirstChild();
                        denode != nullptr;
                        denode = denode->NextSibling())
                    {
                        auto dexml = denode->ToElement();
                        if (dexml)
                        {
                            double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
                            unsigned maxLevel = 0;
                            std::string description;

                            dexml->QueryDoubleAttribute("minx", &minX);
                            dexml->QueryDoubleAttribute("miny", &minY);
                            dexml->QueryDoubleAttribute("maxx", &maxX);
                            dexml->QueryDoubleAttribute("maxy", &maxY);
                            dexml->QueryUnsignedAttribute("maxlevel", &maxLevel);
                            dexml->QueryStringAttribute("description", &description);

                            if (maxLevel > 0u)
                            {
                                if (description.empty())
                                    tilemap.dataExtents.push_back(DataExtent(GeoExtent(profile.srs(), minX, minY, maxX, maxY), 0, maxLevel));
                                else
                                    tilemap.dataExtents.push_back(DataExtent(GeoExtent(profile.srs(), minX, minY, maxX, maxY), 0, maxLevel, description));
                            }
                            else
                            {
                                if (description.empty())
                                    tilemap.dataExtents.push_back(DataExtent(GeoExtent(profile.srs(), minX, minY, maxX, maxY), 0));
                                else
                                    tilemap.dataExtents.push_back(DataExtent(GeoExtent(profile.srs(), minX, minY, maxX, maxY), 0, description));
                            }
                        }
                    }
                }
            }
        }

        // Now, clean up any messes.
         
        // Try to compute the profile based on the SRS if there was no PROFILE tag given
        if (tilemap.profileType == ProfileType::UNKNOWN && !tilemap.srsString.empty())
        {
            SRS srs(tilemap.srsString);

            tilemap.profileType =
                srs.isGeodetic() ? ProfileType::GEODETIC :
                srs.isHorizEquivalentTo(SRS::SPHERICAL_MERCATOR) ? ProfileType::MERCATOR :
                srs.isProjected() ? ProfileType::LOCAL :
                ProfileType::UNKNOWN;
        }

        tilemap.computeMinMaxLevel();
        tilemap.computeNumTiles();
        tilemap.generateTileSets(20u);

        return tilemap;
    }
}


bool
TileMap::valid() const
{
    return profileType != ProfileType::UNKNOWN;
}

void TileMap::computeMinMaxLevel()
{
    minLevel = INT_MAX;
    maxLevel = 0;
    for (auto& tileSet : tileSets)
    {
        if (tileSet.order < minLevel) minLevel = tileSet.order;
        if (tileSet.order > maxLevel) maxLevel = tileSet.order;
    }
}

void TileMap::computeNumTiles()
{
    numTilesWide = -1;
    numTilesHigh = -1;

    if (tileSets.size() > 0)
    {
        unsigned int level = tileSets[0].order;
        double res = tileSets[0].unitsPerPixel;

        numTilesWide = (int)((maxX - minX) / (res * format.width));
        numTilesHigh = (int)((maxY - minY) / (res * format.width));

        //In case the first level specified isn't level 0, compute the number of tiles at level 0
        for (unsigned int i = 0; i < level; i++)
        {
            numTilesWide /= 2;
            numTilesHigh /= 2;
        }
    }
}

Profile
TileMap::createProfile() const
{
    Profile profile;

    std::string def = srsString;
    if (!vsrsString.empty())
    {
        if (vsrsString == "egm96")
            def += "+5773";
    }
    SRS new_srs(def);

    if (profileType == ProfileType::GEODETIC)
    {
        profile = Profile::GLOBAL_GEODETIC;
    }
    else if (profileType == ProfileType::MERCATOR)
    {
        profile = Profile::SPHERICAL_MERCATOR;
    }
    else if (new_srs.isHorizEquivalentTo(SRS::SPHERICAL_MERCATOR))
    {
        //HACK:  Some TMS sources, most notably TileCache, use a global mercator extent that is very slightly different than
        //       the automatically computed mercator bounds which can cause rendering issues due to the some texture coordinates
        //       crossing the dateline.  If the incoming bounds are nearly the same as our definion of global mercator, just use our definition.
        double eps = 1.0;
        Profile merc(Profile::SPHERICAL_MERCATOR);
        if (numTilesWide == 1 && numTilesHigh == 1 &&
            equiv(merc.extent().xMin(), minX, eps) &&
            equiv(merc.extent().yMin(), minY, eps) &&
            equiv(merc.extent().xMax(), maxX, eps) &&
            equiv(merc.extent().yMax(), maxY, eps))
        {
            profile = merc;
        }
    }

    else if (
        new_srs.isGeodetic() &&
        equiv(minX, -180.) &&
        equiv(maxX, 180.) &&
        equiv(minY, -90.) &&
        equiv(maxY, 90.))
    {
        profile = Profile::GLOBAL_GEODETIC;
    }

#if 0
    if (!profile.valid())
    {
        // everything else is a "LOCAL" profile.
        profile = Profile(
            new_srs,
            Box(minX, minY, maxX, maxY),
            std::max(numTilesWide, 1u),
            std::max(numTilesHigh, 1u));
    }
#endif

    return std::move(profile);
}


std::string
TileMap::getURI(const TileKey& tilekey, bool invertY) const
{
    if (!intersectsKey(tilekey))
    {
        //OE_NOTICE << LC << "No key intersection for tile key " << tilekey.str() << std::endl;
        return "";
    }

    unsigned int zoom = tilekey.levelOfDetail();
    unsigned int x = tilekey.tileX(), y = tilekey.tileY();

    //Some TMS like services swap the Y coordinate so 0,0 is the upper left rather than the lower left.  The normal TMS
    //specification has 0,0 at the bottom left, so inverting Y will make 0,0 in the upper left.
    //http://code.google.com/apis/maps/documentation/overlays.html#Google_Maps_Coordinates
    if (!invertY)
    {
        auto [numCols, numRows] = tilekey.profile().numTiles(tilekey.levelOfDetail());
        y = numRows - y - 1;
    }

    bool sub = filename.find('$') != filename.npos;

    //OE_NOTICE << LC << "KEY: " << tilekey.str() << " level " << zoom << " ( " << x << ", " << y << ")" << std::endl;

    //Select the correct TileSet
    if (tileSets.size() > 0)
    {
        for (auto& tileSet : tileSets)
        {
            if (tileSet.order == zoom)
            {
                std::stringstream ss;
                std::string basePath = std::filesystem::path(filename).remove_filename().string();
                if (sub)
                {
                    auto temp = filename;
                    util::replace_in_place(temp, "${x}", std::to_string(x));
                    util::replace_in_place(temp, "${y}", std::to_string(y));
                    util::replace_in_place(temp, "${z}", std::to_string(zoom));
                    return temp;
                }
                else
                {
                    if (!basePath.empty())
                    {
                        ss << basePath << "/";
                    }
                    ss << zoom << "/" << x << "/" << y << "." << format.extension;
                    std::string ssStr;
                    ssStr = ss.str();
                    return ssStr;
                }
            }
        }
    }
    else if (sub)
    {
        auto temp = filename;
        util::replace_in_place(temp, "${x}", std::to_string(x));
        util::replace_in_place(temp, "${y}", std::to_string(y));
        util::replace_in_place(temp, "${z}", std::to_string(zoom));
        return temp;
    }

    else // Just go with it. No way of knowing the max level.
    {
        std::stringstream ss;
        std::string basePath = std::filesystem::path(filename).remove_filename().string();
        if (!basePath.empty())
        {
            ss << basePath << "/";
        }
        ss << zoom << "/" << x << "/" << y << "." << format.extension;
        std::string ssStr;
        ssStr = ss.str();
        return ssStr;
    }

    return "";
}

bool
TileMap::intersectsKey(const TileKey& tilekey) const
{
    Box b = tilekey.extent().bounds();

    bool inter = intersects(
        minX, minY, maxX, maxY,
        b.xmin, b.ymin, b.xmax, b.ymax);

    if (!inter && tilekey.profile().srs().isHorizEquivalentTo(SRS::SPHERICAL_MERCATOR))
    {
        glm::dvec3 keyMin(b.xmin, b.ymin, b.zmin);
        glm::dvec3 keyMax(b.xmax, b.ymax, b.zmax);

        auto xform = tilekey.profile().srs().to(tilekey.profile().srs().geoSRS());
        xform(keyMin, keyMin);
        xform(keyMax, keyMax);

        inter = intersects(
            minX, minY, maxX, maxY,
            b.xmin, b.ymin, b.xmax, b.ymax);
    }

    return inter;
}

void
TileMap::generateTileSets(unsigned numLevels)
{
    Profile p = createProfile();
    tileSets.clear();

    double width = (maxX - minX);

    for (unsigned int i = 0; i < numLevels; ++i)
    {
        auto [numCols, numRows] = p.numTiles(i);
        double res = (width / (double)numCols) / (double)format.width;

        TileSet ts{
            "",
            res,
            i
        };
        tileSets.emplace_back(TileSet{ "", res, i });
    }
}


TileMap::TileMap(
    const std::string& url,
    const Profile& profile,
    const DataExtentList& in_dataExtents,
    const std::string& formatString,
    int tile_width,
    int tile_height)
{
    const GeoExtent& ex = profile.extent();

    if (profile.valid())
    {
        profileType =
            profile.srs().isGeodetic() ? ProfileType::GEODETIC :
            profile.srs().isHorizEquivalentTo(SRS::SPHERICAL_MERCATOR) ? ProfileType::MERCATOR :
            ProfileType::LOCAL;
    }

    minX = ex.xmin();
    minY = ex.ymin();
    maxX = ex.xmax();
    maxY = ex.ymax();

    originX = ex.xmin();
    originY = ex.ymin();

    filename = url;

    srsString = getHorizSRSString(profile.srs());
    //vsrsString = profile.srs().vertical();

    format.width = tile_width;
    format.height = tile_height;

    auto [x, y] = profile.numTiles(0);
    numTilesWide = x;
    numTilesHigh = y;

    // format can be a mime-type or an extension:
    std::string::size_type p = formatString.find('/');
    if (p == std::string::npos)
    {
        format.extension = formatString;
        format.mimeType = ""; // TODO
        //tileMap->_format.setMimeType( Registry::instance()->getMimeTypeForExtension(format) );
    }
    else
    {
        format.mimeType = formatString;
        format.extension = ""; // TODO;
        //tileMap->_format.setExtension( Registry::instance()->getExtensionForMimeType(format) );
    }

    //Add the data extents
    for (auto& temp : in_dataExtents)
        dataExtents.push_back(temp);

    // If we have some data extents specified then make a nicer bounds than the
    if (!dataExtents.empty())
    {
        // Get the union of all the extents
        GeoExtent e(dataExtents.front());
        for (unsigned int i = 1; i < dataExtents.size(); i++)
        {
            e.expandToInclude(dataExtents[i]);
        }

        // Convert the bounds to the output profile
        GeoExtent bounds = profile.clampAndTransformExtent(e);
        //GeoExtent bounds = e.transform(profile.srs());
        minX = bounds.xmin();
        minY = bounds.ymin();
        maxX = bounds.xmax();
        maxY = bounds.ymax();
    }

    generateTileSets(20u);
    computeMinMaxLevel();
}


//----------------------------------------------------------------------------

Result<TileMap>
ROCKY_NAMESPACE::TMS::readTileMap(const URI& location, const IOOptions& io)
{
    auto r = location.read(io);

    if (r.status.failed())
        return r.status;

    auto tilemap = parseTileMapFromXML(r->data);

#if 0
    // Read tile map into a Config:
    Config conf;
    std::stringstream buf(r->data.to_string());
    conf.from_xml(buf);

    // parse that into a tile map:
    auto tilemap = parseTileMapFromConfig(conf);
#endif

    if (tilemap.status.ok())
    {
        tilemap.value.filename = location.full();

        // record the timestamp (if there is one) in the tilemap. It's not a persistent field
        // but will help with things like per-session caching.
        //tileMap->setTimeStamp( r.lastModifiedTime() );
        //TODO
    }

    return tilemap;
}



void
TMS::Driver::close()
{
    tileMap = TileMap();
    //_writer = nullptr;
    //_forceRGBWrites = false;
}

Status
TMS::Driver::open(
    const URI& uri,
    Profile& profile,
    const std::string& format,
    bool isCoverage,
    DataExtentList& dataExtents,
    const IOOptions& io)
{
    _isCoverage = isCoverage;

    // URI is mandatory.
    if (uri.empty())
    {
        return Status(Status::ConfigurationError, "TMS driver requires a valid \"uri\" property");
    }

    // A repo is writable only if it's local.
    if (uri.isRemote())
    {
        //OE_DEBUG << LC << "Repo is remote; opening in read-only mode" << std::endl;
    }

    // Is this a new repo? (You can only create a new repo at a local non-archive URI.)
    bool isNewRepo = false;

#if 0
    if (!uri.isRemote() &&
        !osgEarth::isPathToArchivedFile(uri.full()) &&
        !osgDB::fileExists(uri.full()))
    {
        isNewRepo = true;

        // new repo REQUIRES a profile:
        if (!profile.valid())
        {
            return Status(Status::ConfigurationError, "Fail: profile required to create new TMS repo");
        }
    }
#endif

    // Take the override profile if one is given
    if (profile.valid())
    {
        //Log::info() << "TMS: " << "Using express profile \"" << profile.to_json() << "\" for URI \"" << uri.base() << "\""
        //    << std::endl;

        DataExtentList dataExtents_dummy; // empty

        tileMap = TileMap(
            uri.full(),
            profile,
            dataExtents_dummy,
            format,
            256,
            256);

#if 0
        // If this is a new repo, write the tilemap file to disk now.
        if (isNewRepo)
        {
            if (format.empty())
            {
                return Status(Status::ConfigurationError, "Missing required \"format\" property: e.g. png, jpg");
            }

            TMS::TileMapReaderWriter::write(_tileMap.get(), uri.full());
            OE_INFO << LC << "Created new TMS repo at " << uri.full() << std::endl;
        }
#endif
    }

    else
    {
        // Attempt to read the tile map parameters from a TMS TileMap XML tile on the server:
        auto tileMapRead = readTileMap(uri, io);
        //_tileMap = TMS::TileMapReaderWriter::read( uri, readOptions );

        if (tileMapRead.status.failed())
            return tileMapRead.status;

        tileMap = tileMapRead.value;

        Profile profileFromTileMap = tileMap.createProfile();
        if (profileFromTileMap.valid())
        {
            profile = profileFromTileMap;
        }
    }

    // Make sure we've established a profile by this point:
    if (!profile.valid())
    {
        return Status::Error("Failed to establish a profile for " + uri.full());
    }

#if 0
    // resolve the writer
    if (!uri.isRemote() && !resolveWriter(format))
    {
        io.services().log().warn << "Cannot create writer; writing disabled" << std::endl;
    }
#endif

    // TileMap and profile are valid at this point. Build the tile sets.
    // Automatically set the min and max level of the TileMap
    if (!tileMap.tileSets.empty())
    {
        if (!tileMap.dataExtents.empty())
        {
            for (auto& de : tileMap.dataExtents)
            {
                dataExtents.push_back(de);
            }
        }
    }

    if (dataExtents.empty() && profile.valid())
    {
        dataExtents.push_back(DataExtent(profile.extent(), 0, tileMap.maxLevel));
    }

    return StatusOK;
}

Result<shared_ptr<Image>>
TMS::Driver::read(
    const URI& uri,
    const TileKey& key,
    bool invertY,
    bool isMapboxRGB,
    const IOOptions& io) const
{
    shared_ptr<Image> image;
    URI imageURI;

    // create the URI from the tile map?
    if (tileMap.valid() && key.levelOfDetail() <= tileMap.maxLevel)
    {
        imageURI = URI(tileMap.getURI(key, invertY), uri.context());
        if (!imageURI.empty() && isMapboxRGB)
        {
            if (imageURI.full().find('?') == std::string::npos)
                imageURI = URI(imageURI.full() + "?mapbox=true", uri.context());
            else
                imageURI = URI(imageURI.full() + "&mapbox=true", uri.context());
        }

        auto fetch = imageURI.read(io);
        if (fetch.status.failed())
        {
            return fetch.status;
        }

        std::istringstream buf(fetch->data);
        auto image_rr = io.services().readImageFromStream(buf, fetch->contentType, io);

        if (image_rr.status.failed())
        {
            return image_rr.status;
        }

        image = image_rr.value;

        if (!image)
        {
            if (imageURI.empty() || !tileMap.intersectsKey(key))
            {
                // We couldn't read the image from the URL or the cache, so check to see
                // whether the given key is less than the max level of the tilemap
                // and create a transparent image.
                if (key.levelOfDetail() <= tileMap.maxLevel)
                {
                    ROCKY_TODO("");
                    return Image::create(Image::R8G8B8A8_UNORM, 1, 1);

                    //if (_isCoverage)
                    //    return LandCover::createEmptyImage();
                    //else
                    //    return ImageUtils::createEmptyImage();
                }
            }
        }
    }

    if (image)
        return image;
    else
        return Status(Status::ResourceUnavailable);
}


URI
TMS::Driver::createSubstitutionURI(const TileKey& key, const URI& uri, bool invert_y) const
{
    auto href = uri.full();
    auto y = key.tileY();
    if (!invert_y) {
        //http://code.google.com/apis/maps/documentation/overlays.html#Google_Maps_Coordinates
        auto [cols, rows] = key.profile().numTiles(key.levelOfDetail());
        y = rows - y - 1;
    }
    util::replace_in_place(href, "${x}", std::to_string(key.tileX()));
    util::replace_in_place(href, "${y}", std::to_string(y));
    util::replace_in_place(href, "${z}", std::to_string(key.levelOfDetail()));
    return URI(href, uri.context());
}
