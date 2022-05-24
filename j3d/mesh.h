#pragma once

#include <jtk/qbvh.h>
#include <jtk/image.h>
#include <jtk/vec.h>

#include <stdint.h>

#include <memory>
#include <string>

enum class mesh_filetype
  {
  MESH_FILETYPE_STL,
  MESH_FILETYPE_PLY,
  MESH_FILETYPE_OFF,
  MESH_FILETYPE_OBJ,
  MESH_FILETYPE_TRC
  };

struct mesh
  {
  std::vector<jtk::vec3<float>> vertices;
  std::vector<jtk::vec3<uint32_t>> triangles;
  std::vector<jtk::vec3<float>> vertex_colors;
  std::vector<jtk::vec3<jtk::vec2<float>>> uv_coordinates;
  jtk::image<uint32_t> texture;
  jtk::float4x4 cs;
  bool visible;
  };

std::vector<std::pair<std::string, mesh_filetype>> get_valid_mesh_extensions();

jtk::image<uint32_t> make_dummy_texture(int w, int h, int block_size = 32);
std::vector<uint32_t> convert_vertex_colors(const std::vector<jtk::vec3<float>>& vertex_colors);
std::vector<jtk::vec3<float>> convert_vertex_colors(const std::vector<uint32_t>& vertex_colors);

void compute_bb(jtk::vec3<float>& min, jtk::vec3<float>& max, uint32_t nr_of_vertices, const jtk::vec3<float>* vertices);
bool read_from_file(mesh& m, const std::string& filename);

bool vertices_to_csv(const mesh& m, const std::string& filename);
bool triangles_to_csv(const mesh& m, const std::string& filename);

bool write_to_file(const mesh& m, const std::string& filename);

void info(const mesh& m);

void cs_apply(mesh& m);