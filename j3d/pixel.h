#pragma once

#include <stdint.h>

#include <vector>

#include <jtk/vec.h>
#include <stdint.h>


struct pixel
  {
  pixel() : object_id((uint32_t)-1), u(0.f), v(0.f), barycentric_u(0.f), barycentric_v(0.f), depth(0.f), mark(0), r(0), g(0), b(0), db_id(0) {}
  /*
  mark:
  1th bit: shadow
  2nd bit: texture data is available
  */
  uint8_t mark, r, g, b;
  float u;
  float v;
  float depth;
  uint32_t object_id;
  float barycentric_u;
  float barycentric_v;
  uint32_t db_id;
  };

uint32_t get_closest_vertex(const pixel& p, const std::vector<jtk::vec3<float>>* vertices, const std::vector<jtk::vec3<uint32_t>>* triangles);
