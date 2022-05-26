#pragma once

#include <vector>
#include "jtk/vec.h"

bool read_vox(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles);