# j3d
3d viewer for ply, stl, obj.

Introduction
------------

j3d is a simple and straightforward application for visualizing 3d meshes and point clouds. The application uses software rendering so it can be used together with, for instance, Windows Remote Desktop without problems. Furthermore very large files can also be handled provided you have sufficient RAM. You need approximately 50 bytes of RAM per triangle (assuming no textures or vertex colors are present in the file). Thus, you need 15Gb of RAM to render a file with 300 million triangles.

![](images/j3d_screenshot_1.png)

Building
--------
First of all, j3d uses submodules, so don't forget to also call

     git submodule update --init

Next, run CMake to generate a solution file on Windows, a make file on Linux, or an XCode project on MacOs.
You can build j3d without building other external projects (as all necessary dependencies are delivered with the code). 

The default multithreading approach uses `std::thread`. There is however the option to use multithreading via [Intel's TBB library](https://software.intel.com/content/www/us/en/develop/tools/threading-building-blocks.html). This is an option in CMake: set the `JTK_THREADING` variable to `tbb`. You will have to make sure that you have the correct dll files and lib files to link with TBB. You can set the necessary variables `TBB_INCLUDE_DIR` and `TBB_LIBRARIES` via CMake.

If you build for a Mac M1 with ARM processor, then set the CMake variables JTK_TARGET to arm.

