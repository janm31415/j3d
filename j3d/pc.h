#pragma once

#include <jtk/qbvh.h>
#include <jtk/vec.h>

#include <stdint.h>

#include <memory>
#include <string>

#include "trico/trico/trico.h"

enum class pc_filetype
  {
  PC_FILETYPE_PLY,
  PC_FILETYPE_OBJ,
  PC_FILETYPE_PTS,
  PC_FILETYPE_XYZ,
  PC_FILETYPE_TRC,
  PC_FILETYPE_OFF
  };

struct pc
  {
  std::vector<jtk::vec3<float>> vertices;  
  std::vector<jtk::vec3<float>> normals;
  std::vector<uint32_t> vertex_colors;  
  jtk::float4x4 cs;
  bool visible;
  double load_time_in_s;
  };

bool read_from_file(pc& point_cloud, const std::string& filename);

bool vertices_to_csv(const pc& m, const std::string& filename);

bool write_to_file(const pc& p, const std::string& filename);

void info(const pc& p);

void cs_apply(pc& p);

std::vector<jtk::vec3<float>> estimate_normals(const pc& p, uint32_t k);

std::vector<std::pair<std::string, pc_filetype>> get_valid_pc_extensions();