#include "pc.h"
#include "io.h"

#include <jtk/file_utils.h>
#include <jtk/fitting.h>
#include <jtk/geometry.h>
#include <jtk/mat.h>
#include <iostream>

#include <algorithm>

#include <jtk/point_tree.h>

#include <jtk/containers.h>

using namespace jtk;

std::vector<std::pair<std::string, pc_filetype>> get_valid_pc_extensions()
  {
  std::vector<std::pair<std::string, pc_filetype>> extensions;

  extensions.emplace_back(std::string("ply"), pc_filetype::PC_FILETYPE_PLY);
  extensions.emplace_back(std::string("obj"), pc_filetype::PC_FILETYPE_OBJ);
  extensions.emplace_back(std::string("pts"), pc_filetype::PC_FILETYPE_PTS);
  extensions.emplace_back(std::string("xyz"), pc_filetype::PC_FILETYPE_XYZ);
  extensions.emplace_back(std::string("trc"), pc_filetype::PC_FILETYPE_TRC);

  return extensions;
  }

bool read_from_file(pc& point_cloud, const std::string& filename)
  {
  std::string ext = jtk::get_extension(filename);
  if (ext.empty())
    return false;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](char ch) {return (char)::tolower(ch); });

  static std::vector<std::pair<std::string, pc_filetype>> valid_extensions = get_valid_pc_extensions();

  for (const auto& valid_ext : valid_extensions)
    {
    if (valid_ext.first == ext)
      {
      switch (valid_ext.second)
        {      
        case pc_filetype::PC_FILETYPE_PLY:
        {
        std::vector<jtk::vec3<uint32_t>> triangles;
        std::vector<jtk::vec3<jtk::vec2<float>>> uv;
        if (!read_ply(filename.c_str(), point_cloud.vertices, point_cloud.normals, point_cloud.vertex_colors, triangles, uv))
          return false;
        if (point_cloud.vertices.empty())
          return false;
        break;
        }
        case pc_filetype::PC_FILETYPE_OBJ:
        {
        std::vector<jtk::vec3<uint32_t>> triangles;
        std::vector<jtk::vec3<jtk::vec2<float>>> uv;
        jtk::image<uint32_t> texture;
        if (!read_obj(filename.c_str(), point_cloud.vertices, point_cloud.normals, point_cloud.vertex_colors, triangles, uv, texture))
          return false;
        if (point_cloud.vertices.empty())
          return false;
        break;
        }
        case pc_filetype::PC_FILETYPE_PTS:
        {
        std::vector<int> intensity;
        if (!read_pts(filename.c_str(), point_cloud.vertices, intensity, point_cloud.vertex_colors))
          return false;
        if (point_cloud.vertices.empty())
          return false;
        break;
        }
        case pc_filetype::PC_FILETYPE_XYZ:
        {
        if (!read_xyz(filename.c_str(), point_cloud.vertices))
          return false;
        if (point_cloud.vertices.empty())
          return false;
        break;
        }
        case pc_filetype::PC_FILETYPE_TRC:
        {
        std::vector<jtk::vec3<uint32_t>> triangles;
        std::vector<jtk::vec3<jtk::vec2<float>>> uv;
        if (!read_trc(filename.c_str(), point_cloud.vertices, point_cloud.normals, point_cloud.vertex_colors, triangles, uv))
          return false;
        if (point_cloud.vertices.empty())
          return false;
        break;
        }
        }

      point_cloud.cs = get_identity();
      point_cloud.visible = true;
      return true;

      } // if (valid_ext.first == ext)
    }
  return false;
  }

bool vertices_to_csv(const pc& m, const std::string& filename)
  {
  std::vector<std::vector<std::string>> data;
  for (const auto& vertex : m.vertices)
    {
    std::vector<std::string> line;
    line.push_back(std::to_string(vertex[0]));
    line.push_back(std::to_string(vertex[1]));
    line.push_back(std::to_string(vertex[2]));
    data.push_back(line);
    }
  return csv_write(data, filename.c_str(), ",");
  }

bool write_to_file(const pc& p, const std::string& filename)
  {
  std::string ext = jtk::get_extension(filename);
  if (ext.empty())
    return false;
  if (ext == "ply")
    {
    std::vector<jtk::vec3<uint32_t>> triangles;
    std::vector<jtk::vec3<jtk::vec2<float>>> uv;
    return write_ply(filename.c_str(), p.vertices, p.normals, p.vertex_colors, triangles, uv);
    }
  else if (ext == "obj")
    {
    std::vector<jtk::vec3<uint32_t>> triangles;
    std::vector<jtk::vec3<jtk::vec2<float>>> uv;
    jtk::image<uint32_t> texture;
    return write_obj(filename.c_str(), p.vertices, p.normals, p.vertex_colors, triangles, uv, texture);
    }
  else if (ext == "pts")
    {
    std::vector<int> intensity;
    return write_pts(filename.c_str(), p.vertices, intensity, p.vertex_colors);
    }
  else if (ext == "xyz")
    {
    return write_xyz(filename.c_str(), p.vertices);
    }
  else if (ext == "trc")
    {
    std::vector<jtk::vec3<uint32_t>> triangles;
    std::vector<jtk::vec3<jtk::vec2<float>>> uv;
    return write_trc(filename.c_str(), p.vertices, p.normals, p.vertex_colors, triangles, uv);
    }
  return false;
  }

void info(const pc& p)
  {
  std::cout << "---------------------------------------" << std::endl;
  std::cout << "POINTCLOUD" << std::endl;
  std::cout << "Vertices: " << p.vertices.size() << std::endl;
  std::cout << "Coordinate system: " << std::endl;
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      std::cout << p.cs[i + 4 * j] << " ";
      }
    std::cout << std::endl;
    }
  std::cout << "Vertex normals: " << (p.normals.empty() ? "No" : "Yes") << std::endl;
  std::cout << "Vertex colors: " << (p.vertex_colors.empty() ? "No" : "Yes") << std::endl;
  std::cout << "Visible: " << (p.visible ? "Yes" : "No") << std::endl;
  std::cout << "---------------------------------------" << std::endl;
  }

void cs_apply(pc& p)
  {
  for (auto& v : p.vertices)
    {
    jtk::float4 V(v[0], v[1], v[2], 1.f);
    V = jtk::matrix_vector_multiply(p.cs, V);
    v[0] = V[0];
    v[1] = V[1];
    v[2] = V[2];
    }
  p.cs = jtk::get_identity();
  }

namespace
  {
  struct tree_point
    {
    jtk::vec3<float> pt;
    uint32_t idx;
    float operator [](int i) const
      {
      return pt[i];
      }
    float& operator [](int i)
      {
      return pt[i];
      }
    };

  struct point_tree_traits
    {
    typedef float value_type;
    enum { dimension = 3 };
    typedef tree_point point_type;
    };

  uint64_t make_edge(uint32_t v0, uint32_t v1)
    {
    uint64_t e = v0;
    e <<= 32;
    e |= (uint64_t)v1;
    return e;
    }

  uint32_t edge_to_v0(uint64_t e)
    {
    e >>= 32;
    return (uint32_t)(e & 0xffffffff);
    }

  uint32_t edge_to_v1(uint64_t e)
    {
    return (uint32_t)(e & 0xffffffff);
    }
  }

std::vector<jtk::vec3<float>> estimate_normals(const pc& p, uint32_t k)
  {
  std::vector<jtk::vec3<float>> normals;
  normals.reserve(p.vertices.size());

  point_tree<point_tree_traits> tree;
  std::vector<tree_point> vert;
  vert.reserve(p.vertices.size());
  for (uint32_t v = 0; v < p.vertices.size(); ++v)
    {
    tree_point pt;
    pt.pt = p.vertices[v];
    pt.idx = v;
    vert.push_back(pt);
    }
  tree.efficient_build_tree(vert.begin(), vert.end());

  for (uint32_t v = 0; v < p.vertices.size(); ++v)
    {
    tree_point tp;
    tp.pt = p.vertices[v];;
    std::vector < tree_point > pts = tree.find_k_nearest((int)k, tp);
    jtk::vec3<float> origin, normal;
    float eig;
    std::vector<jtk::vec3<float>> raw_pts;
    raw_pts.reserve(pts.size());
    for (auto p : pts)
      raw_pts.push_back(p.pt);
    jtk::fit_plane(origin, normal, eig, raw_pts);
    normals.push_back(normal);
    }

  std::vector<bool> treated(p.vertices.size(), false);

  jtk::hashed_heap<uint64_t, float> heap;

  uint32_t v = 0;
  while (true)
    {
    while (v < p.vertices.size() && treated[v])
      ++v;
    if (v == p.vertices.size())
      break;

    treated[v] = true;

    tree_point tp;
    tp.pt = p.vertices[v];
    std::vector < tree_point > pts = tree.find_k_nearest((int)k, tp);
    for (const auto& pt : pts)
      {
      if (pt.idx != v && !treated[pt.idx])
        {
        float score = fabs(dot(normals[v], normals[pt.idx]));
        heap.push(std::pair<uint64_t, float>(make_edge(v, pt.idx), score));
        }
      }

    while (!heap.empty())
      {
      auto top_element = heap.top();
      heap.pop();
      uint32_t v0 = edge_to_v0(top_element.first);
      uint32_t v1 = edge_to_v1(top_element.first);
      if (!treated[v1])
        {
        treated[v1] = true;
        if (dot(normals[v0], normals[v1]) < 0.f)
          normals[v1] = -normals[v1];

        tp.pt = p.vertices[v1];
        std::vector < tree_point > pts = tree.find_k_nearest((int)k, tp);
        for (const auto& pt : pts)
          {
          if (pt.idx != v1 && !treated[pt.idx])
            {
            float score = fabs(dot(normals[v1], normals[pt.idx]));
            heap.push(std::pair<uint64_t, float>(make_edge(v1, pt.idx), score));
            }
          }

        }
      }
    }

  return normals;
  }