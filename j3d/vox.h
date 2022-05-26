#pragma once

#include <vector>
#include "jtk/vec.h"

bool read_vox(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles);

bool write_vox(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<uint32_t>>& triangles, uint32_t max_dim=100);