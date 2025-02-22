/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include <rocky_vsg/Application.h>
#include <rocky_vsg/LineString.h>

#if !defined(ROCKY_SUPPORTS_TMS)
#error This example requires TMS support. Check CMake.
#endif

#include <rocky/TMSImageLayer.h>
#include <rocky/TMSElevationLayer.h>
#include <rocky/contrib/EarthFileImporter.h>

template<class T>
int error(T layer)
{
    rocky::Log::warn() << "Problem with layer \"" <<
        layer->name() << "\" : " << layer->status().message << std::endl;
    return -1;
}

int main(int argc, char** argv)
{
    // instantiate the application engine.
    rocky::Application app(argc, argv);

    if (app.map()->layers().empty())
    {
        // add an imagery layer to the map
        auto layer = rocky::TMSImageLayer::create();
        layer->setURI("https://readymap.org/readymap/tiles/1.0.0/7/");
        app.map()->layers().add(layer);

        // check for error
        if (layer->status().failed())
            return error(layer);

        // add an elevation layer to the map
        auto elevation = rocky::TMSElevationLayer::create();
        elevation->setURI("https://readymap.org/readymap/tiles/1.0.0/116/");
        app.map()->layers().add(elevation);

        // check for error
        if (elevation->status().failed())
            return error(elevation);
    }

    // Make a line string.
    auto line = rocky::LineString::create();

    auto xform = rocky::SRS::WGS84.to(rocky::SRS::ECEF);
    for (double lon = -180.0; lon <= 180.0; lon += 2.5)
    {
        glm::dvec3 ecef;
        if (xform(glm::dvec3(lon, 0.0, 100000), ecef))
            line->pushVertex(ecef);
    }
    
    line->setStyle(rocky::LineStyle{
        { 1,0.7,0.3,1 }, // color
        5.0f,            // width
        0xfff0, 1        // stipple pattern & factor
        });

    // make a map object with our line as an attachment:
    auto obj = rocky::MapObject::create(line);

    // add it to the application.
    app.add(obj);

    // run until the user quits.
    return app.run();
}
