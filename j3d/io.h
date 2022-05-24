#pragma once

#include <vector>
#include <stdint.h>
#include <jtk/vec.h>
#include <jtk/image.h>

bool read_ply(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<jtk::vec3<float>>& normals, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles, std::vector<jtk::vec3<jtk::vec2<float>>>& uv);

bool write_ply(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv);

bool read_trc(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<jtk::vec3<float>>& normals, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles, std::vector<jtk::vec3<jtk::vec2<float>>>& uv);

bool write_trc(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv);

bool read_obj(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<jtk::vec3<float>>& normals, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles, std::vector<jtk::vec3<jtk::vec2<float>>>& uv, jtk::image<uint32_t>& texture);

bool write_obj(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv, const jtk::image<uint32_t>& texture);

bool read_pts(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<int>& intensity, std::vector<uint32_t>& clrs);

bool write_pts(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<int>& intensity, const std::vector<uint32_t>& clrs);

bool read_xyz(const char* filename, std::vector<jtk::vec3<float>>& vertices);

bool write_xyz(const char* filename, const std::vector<jtk::vec3<float>>& vertices);