#include "mesh.h"
#include "io.h"
#include "gltf.h"
#include "vox.h"
#include "settings.h"

#include "jtk/geometry.h"

#include <algorithm>

#include "jtk/file_utils.h"

#include <stb_image.h>

#include <iostream>
#include <list>

using namespace jtk;


jtk::image<uint32_t> make_dummy_texture(int w, int h, int block_size)
  {
  jtk::image<uint32_t> im(w, h);
  for (int y = 0; y < h; ++y)
    {
    uint32_t* p_clr = im.row(y);
    bool y_even = ((y / block_size) & 1) == 1;
    for (int x = 0; x < w; ++x, ++p_clr)
      {
      bool x_even = ((x / block_size) & 1) == 1;
      bool black = (x_even && y_even) || (!x_even && !y_even);
      *p_clr = black ? 0xff000000 : 0xffffffff;
      }
    }
  return im;
  }


void compute_bb(vec3<float>& min, vec3<float>& max, uint32_t nr_of_vertices, const vec3<float>* vertices)
  {
  if (nr_of_vertices == 0)
    return;
  min[0] = vertices[0][0];
  min[1] = vertices[0][1];
  min[2] = vertices[0][2];
  max[0] = min[0];
  max[1] = min[1];
  max[2] = min[2];
  for (uint32_t i = 1; i < nr_of_vertices; ++i)
    {
    min[0] = std::min<float>(min[0], vertices[i][0]);
    min[1] = std::min<float>(min[1], vertices[i][1]);
    min[2] = std::min<float>(min[2], vertices[i][2]);
    max[0] = std::max<float>(max[0], vertices[i][0]);
    max[1] = std::max<float>(max[1], vertices[i][1]);
    max[2] = std::max<float>(max[2], vertices[i][2]);
    }
  }

std::vector<std::pair<std::string, mesh_filetype>> get_valid_mesh_extensions()
  {
  std::vector<std::pair<std::string, mesh_filetype>> extensions;

  extensions.emplace_back(std::string("stl"), mesh_filetype::MESH_FILETYPE_STL);
  extensions.emplace_back(std::string("ply"), mesh_filetype::MESH_FILETYPE_PLY);
  extensions.emplace_back(std::string("off"), mesh_filetype::MESH_FILETYPE_OFF);
  extensions.emplace_back(std::string("obj"), mesh_filetype::MESH_FILETYPE_OBJ);
  extensions.emplace_back(std::string("trc"), mesh_filetype::MESH_FILETYPE_TRC);
  extensions.emplace_back(std::string("gltf"), mesh_filetype::MESH_FILETYPE_GLTF);
  extensions.emplace_back(std::string("glb"), mesh_filetype::MESH_FILETYPE_GLTF);
  extensions.emplace_back(std::string("vox"), mesh_filetype::MESH_FILETYPE_VOX);

  return extensions;
  }

bool read_from_file(mesh& m, const std::string& filename)
  {
  std::string ext = jtk::get_extension(filename);
  if (ext.empty())
    return false;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](char ch) {return (char)::tolower(ch); });

  static std::vector<std::pair<std::string, mesh_filetype>> valid_extensions = get_valid_mesh_extensions();

  std::wstring wfilename = jtk::convert_string_to_wstring(filename);

  for (const auto& valid_ext : valid_extensions)
    {
    if (valid_ext.first == ext)
      {
      switch (valid_ext.second)
        {
        case mesh_filetype::MESH_FILETYPE_STL:
        {
          if (!read_stl(m.vertices, m.triangles, wfilename.c_str()))
            {
            if (!read_stl_ascii(m.vertices, m.triangles, wfilename.c_str()))
              return false;
            }
          break;
        }
        case mesh_filetype::MESH_FILETYPE_PLY:
        {
        std::vector<jtk::vec3<float>> vertex_normals;
        std::vector<uint32_t> vertex_colors;
        if (!read_ply(wfilename.c_str(), m.vertices, vertex_normals, vertex_colors, m.triangles, m.uv_coordinates))
          return false;
        if (!vertex_colors.empty())
          {
          m.vertex_colors = convert_vertex_colors(vertex_colors);
          }
        break;
        }
        case mesh_filetype::MESH_FILETYPE_OFF:
        {
        if (!read_off(m.vertices, m.triangles, wfilename.c_str()))
          return false;
        break;
        }
        case mesh_filetype::MESH_FILETYPE_OBJ:
        {
        std::vector<jtk::vec3<float>> vertex_normals;
        std::vector<uint32_t> vertex_colors;
        if (!read_obj(wfilename.c_str(), m.vertices, vertex_normals, vertex_colors, m.triangles, m.uv_coordinates, m.texture))
          return false;
        if (!vertex_colors.empty())
          {
          m.vertex_colors = convert_vertex_colors(vertex_colors);
          }
        break;
        }
        case mesh_filetype::MESH_FILETYPE_TRC:
        {
        std::vector<jtk::vec3<float>> vertex_normals;
        std::vector<uint32_t> vertex_colors;
        if (!read_trc(wfilename.c_str(), m.vertices, vertex_normals, vertex_colors, m.triangles, m.uv_coordinates))
          return false;
        if (!vertex_colors.empty())
          {
          m.vertex_colors = convert_vertex_colors(vertex_colors);
          }
        break;
        }
        case mesh_filetype::MESH_FILETYPE_GLTF:
        {
        std::vector<jtk::vec3<float>> vertex_normals;
        std::vector<uint32_t> vertex_colors;
        if (!read_gltf(filename.c_str(), m.vertices, vertex_normals, vertex_colors, m.triangles, m.uv_coordinates, m.texture))
          return false;
        if (!vertex_colors.empty())
          {
          m.vertex_colors = convert_vertex_colors(vertex_colors);
          }
        break;
        }
        case mesh_filetype::MESH_FILETYPE_VOX:
        {
        std::vector<uint32_t> vertex_colors;
        if (!read_vox(filename.c_str(), m.vertices, vertex_colors, m.triangles))
          return false;
        if (!vertex_colors.empty())
          {
          m.vertex_colors = convert_vertex_colors(vertex_colors);
          }
        break;
        }
        }

      if (!m.uv_coordinates.empty() && (m.texture.width() == 0 || m.texture.height() == 0))
        m.texture = make_dummy_texture(512, 512);
      m.cs = get_identity();
      m.visible = true;
      return true;

      } // if (valid_ext.first == ext)
    }
  return false;
  }

bool vertices_to_csv(const mesh& m, const std::string& filename)
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

bool triangles_to_csv(const mesh& m, const std::string& filename)
  {
  std::vector<std::vector<std::string>> data;
  for (const auto& tria : m.triangles)
    {
    std::vector<std::string> line;
    line.push_back(std::to_string(tria[0]));
    line.push_back(std::to_string(tria[1]));
    line.push_back(std::to_string(tria[2]));
    data.push_back(line);
    }
  return csv_write(data, filename.c_str(), ",");
  }

std::vector<uint32_t> convert_vertex_colors(const std::vector<jtk::vec3<float>>& vertex_colors)
  {
  std::vector<uint32_t> colors;
  colors.reserve(vertex_colors.size());
  for (auto& clr : vertex_colors)
    {
    uint32_t r = (uint32_t)(clr[0] * 255.f);
    uint32_t g = (uint32_t)(clr[1] * 255.f);
    uint32_t b = (uint32_t)(clr[2] * 255.f);
    r = r > 255 ? 255 : r;
    g = g > 255 ? 255 : g;
    b = b > 255 ? 255 : b;
    colors.push_back(0xff000000 | (b << 16) | (g << 8) | r);
    }
  return colors;
  }

std::vector<jtk::vec3<float>> convert_vertex_colors(const std::vector<uint32_t>& vertex_colors)
  {
  std::vector<jtk::vec3<float>> out;
  out.reserve(vertex_colors.size());
  for (uint32_t clr : vertex_colors)
    {
    uint32_t red = clr & 255;
    uint32_t green = (clr >> 8) & 255;
    uint32_t blue = (clr >> 16) & 255;
    out.emplace_back((float)red / 255.f, (float)green / 255.f, (float)blue / 255.f);
    }
  return out;
  }

bool write_to_file(const mesh& m, const std::string& filename, const settings& sett)
  {
  std::string ext = jtk::get_extension(filename);
  if (ext.empty())
    return false;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](char ch) {return (char)::tolower(ch); });
  if (ext == "stl")
    {
    return jtk::write_stl(m.vertices.data(), (uint32_t)m.triangles.size(), m.triangles.data(), nullptr, nullptr, filename.c_str());
    }
  else if (ext == "ply")
    {
    std::vector<uint32_t> colors = convert_vertex_colors(m.vertex_colors);
    std::vector<jtk::vec3<float>> normals;
    return write_ply(filename.c_str(), m.vertices, normals, colors, m.triangles, m.uv_coordinates);
    }
  else if (ext == "off")
    {
    return jtk::write_off((uint32_t)m.vertices.size(), m.vertices.data(), (uint32_t)m.triangles.size(), m.triangles.data(), filename.c_str());
    }
  else if (ext == "obj")
    {
    std::vector<uint32_t> colors = convert_vertex_colors(m.vertex_colors);
    std::vector<jtk::vec3<float>> normals;
    return write_obj(filename.c_str(), m.vertices, normals, colors, m.triangles, m.uv_coordinates, m.texture);
    }
  else if (ext == "glb")
    {
    std::vector<uint32_t> colors = convert_vertex_colors(m.vertex_colors);
    std::vector<jtk::vec3<float>> normals;
    return write_glb(filename.c_str(), m.vertices, normals, colors, m.triangles, m.uv_coordinates, m.texture);
    }
  else if (ext == "gltf")
    {
    std::vector<uint32_t> colors = convert_vertex_colors(m.vertex_colors);
    std::vector<jtk::vec3<float>> normals;
    return write_gltf(filename.c_str(), m.vertices, normals, colors, m.triangles, m.uv_coordinates, m.texture);
    }
  else if (ext == "vox")
    {
    return write_vox(filename.c_str(), m.vertices, m.vertex_colors, m.triangles, m.uv_coordinates, m.texture, sett._vox_max_size);
    }
  return false;
  }

void info(const mesh& m)
  {
  std::cout << "---------------------------------------" << std::endl;
  std::cout << "MESH" << std::endl;
  std::cout << "Triangles: " << m.triangles.size() << std::endl;
  std::cout << "Vertices: " << m.vertices.size() << std::endl;
  std::cout << "Coordinate system: " << std::endl;
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      std::cout << m.cs[i + 4 * j] << " ";
      }
    std::cout << std::endl;
    }
  std::cout << "Vertex colors: " << (m.vertex_colors.empty() ? "No" : "Yes") << std::endl;
  std::cout << "UV coordinates: " << (m.uv_coordinates.empty() ? "No" : "Yes") << std::endl;
  std::cout << "Texture dimensions: " << m.texture.width() << " x " << m.texture.height() << std::endl;
  std::cout << "Visible: " << (m.visible ? "Yes" : "No") << std::endl;
  std::cout << "---------------------------------------" << std::endl;
  }

void cs_apply(mesh& m)
  {
  for (auto& v : m.vertices)
    {
    jtk::float4 V(v[0], v[1], v[2], 1.f);
    V = jtk::matrix_vector_multiply(m.cs, V);
    v[0] = V[0];
    v[1] = V[1];
    v[2] = V[2];
    }
  m.cs = jtk::get_identity();
  }