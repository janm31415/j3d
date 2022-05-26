#pragma once

#include <vector>
#include "jtk/vec.h"
#include "jtk/image.h"

bool read_vox(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles);

bool write_vox(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv, const jtk::image<uint32_t>& texture, uint32_t max_dim=100);