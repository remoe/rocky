# :mountain: Rocky

Rocky is a 3D geospatial map & terrain rendering engine.<img align="right" width="200" alt="Screenshot 2023-02-22 124318" src="https://user-images.githubusercontent.com/326618/220712284-8a17d87a-431f-4966-a425-0f2628b23b40.png">


Rocky will render an accurate 3D or 2D map with real geospatial imagery and elevation data. It supports thousands of map projections and many popular geodata formats including GeoTIFF, WMTS, WMS, and TMS.

This project is in its early stages. It is based on the data model in the osgEarth SDK, a 3D GIS toolkit created in 2008 and still in wide use today.

# A simple Rocky application

**Quick Start** : Head over to the [rocky-demo](https://github.com/pelicanmapping/rocky-demo) repository for a template project much like the one below! It includes a `vcpkg.json` manifest to get `vcpkg` users up and running quickly.

### main.cpp
```c++
#include <rocky_vsg/Application.h>
#include <rocky/TMSImageLayer.h>

int main(int argc, char** argv)
{
    rocky::Application app(argc, argv);

    auto imagery = rocky::TMSImageLayer::create();
    imagery->setURI("https://readymap.org/readymap/tiles/1.0.0/7/");
    app.map()->layers().add(imagery);

    if (imagery->status().failed()) 
    {
        rocky::Log::warn() << imagery->status().message << std::endl;
        return -1;
    }

    return app.run();
}
```

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.10)

project(myApp VERSION 0.1.0 LANGUAGES CXX C)

find_package(rocky CONFIG REQUIRED)

add_executable(myApp main.cpp)

target_link_libraries(myApp PRIVATE rocky::rocky rocky::rocky_vsg)

install(TARGETS myApp RUNTIME DESTINATION bin)
```

### Dependencies
Rocky requires the following additional dependencies:

* [glm](https://github.com/g-truc/glm)
* [PROJ](https://github.com/OSGeo/PROJ)
* [vsg](https://github.com/vsg-dev/VulkanSceneGraph) (VulkanSceneGraph)
* [GDAL](https://github.com/OSGeo/gdal) (optional)
* [vsgXchange](https://github.com/vsg-dev/vsgXchange) (optional)

You can install these using your favorite package manager (we recommend `vcpkg`) or you can build them yourself.

### Environment variables

Set up a couple env vars so Rocky can find its data.
```bat
set ROCKY_FILE_PATH=%rocky_install_dir%/share

set PROJ_DATA=%proj_install_dir%/share/proj
```
