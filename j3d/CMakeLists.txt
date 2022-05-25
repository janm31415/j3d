if (WIN32)
  set(TBB_INCLUDE_DIR "C:/Program Files/TBB/include" CACHE PATH "the include path for TBB")
  set(TBB_LIBRARIES "C:/Program Files/TBB/lib/intel64/vc14/tbb.lib" "C:/Program Files/TBB/lib/intel64/vc14/tbb_debug.lib" CACHE FILEPATH "the tbb lib files")
  set(OPENGL_LIBRARIES opengl32.lib CACHE FILEPATH "opengl lib file")
endif (WIN32)

if (UNIX)
  if (APPLE)
    set(TBB_INCLUDE_DIR "/usr/local/include" "/usr/include" CACHE PATH "the include path for TBB")
    set(TBB_LIBRARIES "/usr/local/lib/libtbb.dylib" CACHE FILEPATH "the tbb .dylib file")
    set(OPENGL_LIBRARIES "/System/Library/Frameworks/OpenGL.framework/OpenGL" CACHE FILEPATH "opengl lib file")
  else()
    set(TBB_INCLUDE_DIR "/usr/local/include" "/usr/include" CACHE PATH "the include path for TBB")
    set(TBB_LIBRARIES "/usr/lib/x86_64-linux-gnu/libtbb.so" "/usr/lib/x86_64-linux-gnu/libtbb.so.2" CACHE FILEPATH "the tbb .so files")
    set(OPENGL_LIBRARIES "/usr/lib/x86_64-linux-gnu/libOpenGL.so.0.0.0" CACHE FILEPATH "opengl lib file")
  endif(APPLE)
endif (UNIX)

set(IMGUI
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_opengl2.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_sdl.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_stdlib.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/dirent_portable.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imguifilesystem.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_draw.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_demo.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_opengl2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_sdl.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_stdlib.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_tables.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_widgets.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imguifilesystem.cpp
)


set(STB
../stb/stb_image.h
../stb/stb_image_write.h
)

set(JSON
${CMAKE_CURRENT_SOURCE_DIR}/../json/json.hpp
)

set(TINYGLTF
${CMAKE_CURRENT_SOURCE_DIR}/../tinygltf/tiny_gltf.h
)


set(HDRS
canvas.h
camera.h
db.h
gltf.h
io.h
keyboard.h
matcap.h
mesh.h
mouse.h
pc.h
pixel.h
pref_file.h
scene.h
settings.h
trackball.h
view.h
)
	
set(SRCS
j3d.rc
camera.cpp
canvas.cpp
db.cpp
gltf.cpp
io.cpp
main.cpp
matcap.cpp
mesh.cpp
pc.cpp
pixel.cpp
pref_file.cpp
scene.cpp
settings.cpp
trackball.c
view.cpp
)

set(RESOURCES
j3d.ico
)


if (UNIX)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -msse4.1 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -msse4.1 -pthread -std=c++11")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -msse4.1 -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -msse4.1 -pthread -std=c++11")
endif (UNIX)

# general build definitions
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DMEMORY_LEAK_TRACKING)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

include ("../jtk/jtk/jtk.cmake")

if (WIN32)
add_executable(j3d WIN32 ${HDRS} ${SRCS} ${RESOURCES} ${STB} ${IMGUI} ${JSON} ${TINYGLTF})
else (WIN32)
add_executable(j3d ${HDRS} ${SRCS} ${RESOURCES} ${STB} ${IMGUI} ${JSON} ${TINYGLTF})
endif (WIN32)

source_group("Header Files" FILES ${HDRS})
source_group("Source Files" FILES ${SRCS})
source_group("stb" FILES ${STB})
source_group("ImGui" FILES ${IMGUI})
source_group("json" FILES ${JSON})
source_group("tinygltf" FILES ${TINYGLTF})



target_include_directories(j3d
    PRIVATE
    .
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/../jtk/
    ${CMAKE_CURRENT_SOURCE_DIR}/../stb/
    ${CMAKE_CURRENT_BINARY_DIR}/../sdl2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/
    )	
	
target_link_libraries(j3d
    PRIVATE
    ${OPENGL_LIBRARIES}    
    SDL2-static
    SDL2main
    trico
    )
    

if (UNIX)

set(XLIBINCLUDE "")
set(XLIBLIBRARY "X11.so")

if (APPLE)
set(XLIBINCLUDE "/usr/X11/include/")
set(XLIBLIBRARY "/usr/X11/lib/libX11.dylib")
endif (APPLE)

target_include_directories(j3d
   PRIVATE
   ${XLIBINCLUDE}
   )
   
target_link_libraries(j3d
    PRIVATE
    ${XLIBLIBRARY}
    )
endif (UNIX)

if (${JTK_THREADING} STREQUAL "tbb")
  target_include_directories(j3d
      PRIVATE
      ${TBB_INCLUDE_DIR}
      )
  target_link_libraries(j3d
      PRIVATE	
      ${TBB_LIBRARIES}      
      )	
endif (${JTK_THREADING} STREQUAL "tbb")   

     
add_custom_command(TARGET j3d POST_BUILD 
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/matcaps" "${CMAKE_CURRENT_BINARY_DIR}/matcaps")
add_custom_command(TARGET j3d POST_BUILD 
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/matcaps" "$<TARGET_FILE_DIR:j3d>/matcaps")

