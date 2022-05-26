#ifndef _SDL_main_h
#define _SDL_main_h
#endif
#include <SDL.h>

#include <stdio.h>
#include <string.h>

#include "view.h"

#include <iostream>

#include "jtk/concurrency.h"

#define JTK_FILE_UTILS_IMPLEMENTATION
#include "jtk/file_utils.h"

#include "jtk/fitting.h"

#define JTK_GEOMETRY_IMPLEMENTATION
#include "jtk/geometry.h"

#define JTK_PLY_IMPLEMENTATION
#include "jtk/ply.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define JTK_QBVH_IMPLEMENTATION
#include "jtk/qbvh.h"

#define JTK_IMAGE_IMPLEMENTATION
#include "jtk/image.h"

using namespace jtk;

constexpr double pi = 3.141592653589793238462643383279;

int main(int argc, char** argv)
  {
  if (SDL_Init(SDL_INIT_VIDEO))
    return -1;
  {
  view v;
  v.prepare_window();
  for (int i = 1; i < argc; ++i)
    {
    v.load_file(argv[i]);
    //if (id >= 0)
    //  {
    //  v.update_current_folder(std::string(argv[i]));
    //  }
    }
  v.loop();
  }
  SDL_Quit();
  return 0;
  }
