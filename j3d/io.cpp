#include "io.h"
#include <string.h>

#include <iostream>
#include "jtk/file_utils.h"
#include "jtk/ply.h"

#include "stb_image.h"
#include "stb_image_write.h"

bool read_ply(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<jtk::vec3<float>>& normals, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles, std::vector<jtk::vec3<jtk::vec2<float>>>& uv)
  {
  return jtk::read_ply(filename, vertices, normals, clrs, triangles, uv);
  }

bool write_ply(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv)
  {
  return jtk::write_ply(filename, vertices, normals, clrs, triangles, uv);
  }

namespace
  {

  bool read_texture_filename_from_mtl(std::string& texture_file, const char* filename)
    {
    FILE* f = nullptr;
    f = fopen(filename, "r");
    if (!f)
      return false;
    char buffer[256];
    while (fgets(buffer, 256, f) != nullptr)
      {
      int first_non_whitespace_index = 0;
      while (first_non_whitespace_index < 250 && (buffer[first_non_whitespace_index] == ' ' || buffer[first_non_whitespace_index] == '\t'))
        ++first_non_whitespace_index;
      if (buffer[first_non_whitespace_index + 0] == 'm' && buffer[first_non_whitespace_index + 1] == 'a' && buffer[first_non_whitespace_index + 2] == 'p' && buffer[first_non_whitespace_index + 3] == '_' && buffer[first_non_whitespace_index + 4] == 'K' && buffer[first_non_whitespace_index + 5] == 'd')
        {
        char texture[256];
        auto scan_err = sscanf(buffer + first_non_whitespace_index, "map_Kd %s\n", texture);
        scan_err;
        texture_file = std::string(texture);
        fclose(f);
        return true;
        }
      }
    fclose(f);
    return false;
    }

  }

bool read_obj(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<jtk::vec3<float>>& normals, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles, std::vector<jtk::vec3<jtk::vec2<float>>>& uv, jtk::image<uint32_t>& texture)
  {
  using namespace jtk;
  std::string mtl_filename;
  std::vector<vec3<float>>().swap(vertices);
  std::vector<vec3<float>>().swap(normals);
  std::vector<uint32_t>().swap(clrs);
  std::vector<vec3<uint32_t>>().swap(triangles);
  std::vector<vec3<vec2<float>>>().swap(uv);
  texture = jtk::image<uint32_t>();

  FILE* f = nullptr;
  f = fopen(filename, "r");
  if (!f)
    return false;

  std::vector<vec2<float>> tex;
  std::vector<vec3<uint32_t>> tria_uv;

  char buffer[256];
  while (fgets(buffer, 256, f) != nullptr)
    {
    int first_non_whitespace_index = 0;
    while (first_non_whitespace_index < 250 && (buffer[first_non_whitespace_index] == ' ' || buffer[first_non_whitespace_index] == '\t'))
      ++first_non_whitespace_index;

    if (buffer[first_non_whitespace_index + 0] == 'm' && buffer[first_non_whitespace_index + 1] == 't' && buffer[first_non_whitespace_index + 2] == 'l' && buffer[first_non_whitespace_index + 3] == 'l' && buffer[first_non_whitespace_index + 4] == 'i' && buffer[first_non_whitespace_index + 5] == 'b')
      {
      char mtlfile[256];
      auto scan_err = sscanf(buffer + first_non_whitespace_index, "mtllib %s\n", mtlfile);
      if (scan_err == 1)
        {
        mtl_filename = std::string(mtlfile);
        }
      }
    if (buffer[first_non_whitespace_index + 0] == 'v' && buffer[first_non_whitespace_index + 1] == ' ')
      {
      float x, y, z;
      auto err = sscanf(buffer + first_non_whitespace_index, "v %f %f %f\n", &x, &y, &z);
      if (err != 3)
        {
        fclose(f);
        return false;
        }
      vertices.push_back(vec3<float>(x, y, z));
      }
    else if (buffer[first_non_whitespace_index + 0] == 'v' && buffer[first_non_whitespace_index + 1] == 'n' && buffer[2] == ' ')
      {
      float x, y, z;
      auto err = sscanf(buffer + first_non_whitespace_index, "vn %f %f %f\n", &x, &y, &z);
      if (err != 3)
        {
        fclose(f);
        return false;
        }
      normals.push_back(vec3<float>(x, y, z));
      }
    else if (buffer[first_non_whitespace_index + 0] == 'v' && buffer[first_non_whitespace_index + 1] == 't' && buffer[2] == ' ')
      {
      float x, y;
      auto err = sscanf(buffer + first_non_whitespace_index, "vt %f %f\n", &x, &y);
      if (err != 2)
        {
        fclose(f);
        return false;
        }
      tex.push_back(vec2<float>(x, y));
      }
    else if (buffer[first_non_whitespace_index + 0] == 'f' && buffer[first_non_whitespace_index + 1] == ' ')
      {
      uint32_t t0, t1, t2, t3, v0, v1, v2, v3;
      auto err = sscanf(buffer + first_non_whitespace_index, "f %d/%d %d/%d %d/%d %d/%d\n", &t0, &v0, &t1, &v1, &t2, &v2, &t3, &v3);
      if (err == 6)
        {
        tria_uv.push_back(vec3<uint32_t>(v0 - 1, v1 - 1, v2 - 1));
        triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
        }
      else if (err == 8)
        {
        tria_uv.push_back(vec3<uint32_t>(v0 - 1, v1 - 1, v2 - 1));
        triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
        tria_uv.push_back(vec3<uint32_t>(v0 - 1, v2 - 1, v3 - 1));
        triangles.push_back(vec3<uint32_t>(t0 - 1, t2 - 1, t3 - 1));
        }
      else
        {
        err = sscanf(buffer + first_non_whitespace_index, "f %d/%d/ %d/%d/ %d/%d/ %d/%d/\n", &t0, &v0, &t1, &v1, &t2, &v2, &t3, &v3);
        if (err == 6)
          {
          tria_uv.push_back(vec3<uint32_t>(v0 - 1, v1 - 1, v2 - 1));
          triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
          }
        else if (err == 8)
          {
          tria_uv.push_back(vec3<uint32_t>(v0 - 1, v1 - 1, v2 - 1));
          triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
          tria_uv.push_back(vec3<uint32_t>(v0 - 1, v2 - 1, v3 - 1));
          triangles.push_back(vec3<uint32_t>(t0 - 1, t2 - 1, t3 - 1));
          }
        else
          {
          err = sscanf(buffer + first_non_whitespace_index, "f %d %d %d %d\n", &t0, &t1, &t2, &t3);
          if (err == 3)
            {
            triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
            }
          else if (err == 4)
            {
            triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
            triangles.push_back(vec3<uint32_t>(t0 - 1, t2 - 1, t3 - 1));
            }
          else
            {
            uint32_t tx0, tx1, tx2, tx3;
            err = sscanf(buffer + first_non_whitespace_index, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", &t0, &v0, &tx0, &t1, &v1, &tx1, &t2, &v2, &tx2, &t3, &v3, &tx3);
            if (err == 9)
              {
              tria_uv.push_back(vec3<uint32_t>(v0 - 1, v1 - 1, v2 - 1));
              triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
              }
            else if (err == 12)
              {
              tria_uv.push_back(vec3<uint32_t>(v0 - 1, v1 - 1, v2 - 1));
              triangles.push_back(vec3<uint32_t>(t0 - 1, t1 - 1, t2 - 1));
              tria_uv.push_back(vec3<uint32_t>(v0 - 1, v2 - 1, v3 - 1));
              triangles.push_back(vec3<uint32_t>(t0 - 1, t2 - 1, t3 - 1));
              }
            else
              {
              fclose(f);
              return false;
              }

            }
          }
        }
      }
    }
  fclose(f);
  if (!tria_uv.empty() && (triangles.size() != tria_uv.size()))
    return false;
  if (!tria_uv.empty())
    {
    for (auto& t : tex)
      {
      t[1] = 1.f - t[1];
      }
    uv.reserve(triangles.size());
    for (auto t : tria_uv)
      {
      uv.emplace_back(tex[t[0]], tex[t[1]], tex[t[2]]);
      }
    }

  if (!mtl_filename.empty())
    {
    if (!file_exists(mtl_filename))
      {
      mtl_filename = get_folder(filename) + mtl_filename;
      }
    std::string texture_filename;
    if (read_texture_filename_from_mtl(texture_filename, mtl_filename.c_str()))
      {
      if (!file_exists(texture_filename))
        {
        texture_filename = get_folder(mtl_filename) + texture_filename;
        }
      int w, h, nr_of_channels;
      unsigned char* im = stbi_load(texture_filename.c_str(), &w, &h, &nr_of_channels, 4);
      if (im)
        {
        texture = jtk::span_to_image(w, h, w, (const uint32_t*)im);
        stbi_image_free(im);
        }
      }
    }

  return true;
  }

bool write_obj(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv, const jtk::image<uint32_t>& texture)
  {
  using namespace jtk;
  std::string objpath(filename);

  std::string mtlfilename;

  if (texture.width() > 0 && texture.height() > 0)
    {

    std::string fn = get_filename(objpath);
    std::string folder = get_folder(objpath);
    std::string imagefilename = fn + ".png";
    mtlfilename = fn + ".mtl";

    std::string imagepath = folder.empty() ? imagefilename : folder + imagefilename;
    std::string mtlpath = folder.empty() ? mtlfilename : folder + mtlfilename;

    std::ofstream mtlfile;

    mtlfile.open(mtlpath.c_str(), std::ios::out);
    if (mtlfile.fail())
      return false;
    if (mtlfile.bad())
      return false;
    if (mtlfile.is_open())
      {
      mtlfile << "newmtl material_0" << std::endl;
      mtlfile << "Ka 0.0000 0.0000 0.0000" << std::endl;
      mtlfile << "Kd 0.8000 0.8000 0.8000" << std::endl;
      mtlfile << "Ks 1.0000 1.0000 1.0000" << std::endl;
      mtlfile << "Tf 0.0000 0.0000 0.0000" << std::endl;
      mtlfile << "d 1.0000" << std::endl;
      mtlfile << "Ns 0" << std::endl;
      mtlfile << "map_Kd " << imagefilename << std::endl;
      mtlfile.clear();
      }
    mtlfile.close();

    if (!stbi_write_png(imagepath.c_str(), texture.width(), texture.height(), 4, (void*)texture.data(), texture.stride() * 4))
      return false;
    }

  std::ofstream outputfile;
  outputfile.open(objpath.c_str(), std::ios::out);
  if (outputfile.fail())
    return false;
  if (outputfile.bad())
    return false;
  if (outputfile.is_open())
    {
    outputfile.clear();
    if (!mtlfilename.empty())
      {
      outputfile << "mtllib " << mtlfilename << std::endl;
      outputfile << "usemtl material_0" << std::endl;
      }
    for (size_t i = 0; i < vertices.size(); ++i)
      {
      outputfile << "v " << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2] << std::endl;
      }
    for (size_t i = 0; i < normals.size(); ++i)
      {
      outputfile << "vn " << normals[i][0] << " " << normals[i][1] << " " << normals[i][2] << std::endl;
      }
    for (size_t i = 0; i < uv.size(); ++i)
      {
      outputfile << "vt " << uv[i][0][0] << " " << 1.f-uv[i][0][1] << std::endl;
      outputfile << "vt " << uv[i][1][0] << " " << 1.f-uv[i][1][1] << std::endl;
      outputfile << "vt " << uv[i][2][0] << " " << 1.f-uv[i][2][1] << std::endl;
      }
    for (size_t i = 0; i < triangles.size(); ++i)
      {
      if (uv.empty())
        {
        outputfile << "f " << triangles[i][0] + 1 << " " << triangles[i][1] + 1 << " " << triangles[i][2] + 1 << std::endl;
        }
      else
        {
        outputfile << "f " << triangles[i][0] + 1 << "/" << (i * 3) + 1 << " ";
        outputfile << triangles[i][1] + 1 << "/" << (i * 3 + 1) + 1 << " ";
        outputfile << triangles[i][2] + 1 << "/" << (i * 3 + 2) + 1 << std::endl;
        }
      }
    }
  outputfile.close();

  return true;
  }


bool read_pts(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<int>& intensity, std::vector<uint32_t>& clrs)
  {
  vertices.clear();
  intensity.clear();
  clrs.clear();

  FILE* f = nullptr;
  f = fopen(filename, "r");
  if (!f)
    return false;

  char buffer[1024];
  uint64_t nr_of_points = 0;

  while (fgets(buffer, 1024, f) != nullptr)
    {
    int first_non_whitespace_index = 0;
    while (first_non_whitespace_index < 250 && (buffer[first_non_whitespace_index] == ' ' || buffer[first_non_whitespace_index] == '\t'))
      ++first_non_whitespace_index;

    float x, y, z;
    int in;
    uint32_t r, g, b;    

    auto err = sscanf(buffer + first_non_whitespace_index, "%f %f %f %d %d %d %d\n", &x, &y, &z, &in, &r, &g, &b);
    if (err == 7)
      {
      vertices.emplace_back(x, y, z);
      intensity.push_back(in);
      uint32_t clr = 0xff000000 | (b << 16) | (g << 8) | r;
      clrs.push_back(clr);
      }
    else
      {
      err = sscanf(buffer + first_non_whitespace_index, "%f %f %f\n", &x, &y, &z);
      if (err == 3)
        {
        vertices.emplace_back(x, y, z);
        }
      else
        {
        err = sscanf(buffer + first_non_whitespace_index, "%d\n", &nr_of_points);
        if (err != 1)
          {
          std::cout << "Invalid pts file\n";
          return false;
          }
        }
      }
    }

  if (nr_of_points && vertices.size() != nr_of_points)
    {
    std::cout << "Invalid pts file\n";
    return false;
    }

  return true;
  }


bool write_pts(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<int>& intensity, const std::vector<uint32_t>& clrs)
  {
  std::ofstream out;

  out.open(filename, std::ios::out);
  if (out.fail())
    return false;
  if (out.bad())
    return false;
  if (out.is_open())
    {
    out << vertices.size() << std::endl;
    for (size_t i = 0; i < vertices.size(); ++i)
      {
      float x = vertices[i][0];
      float y = vertices[i][1];
      float z = vertices[i][2];
      int in = 0;
      if (intensity.size() == vertices.size())
        in = intensity[i];
      uint32_t clr = 0;
      if (clrs.size() == vertices.size())
        clr = clrs[i];
      uint32_t r = clr & 255;
      uint32_t g = (clr >> 8) & 255;
      uint32_t b = (clr >> 16) & 255;
      out << x << " " << y << " " << z << " " << in << " " << r << " " << g << " " << b << std::endl;
      }
    out.close();
    }
  return true;
  }

bool read_xyz(const char* filename, std::vector<jtk::vec3<float>>& vertices)
  {
  vertices.clear();
  std::ifstream in(filename, std::ios::in);

  if (in.fail())
    return false;
  if (in.bad())
    return false;
  if (in.is_open())
    {
    while (!in.eof())
      {
      std::string line;
      std::getline(in, line);
      if (!line.empty())
        {
        std::stringstream str;
        str << line;
        float x, y, z;
        str >> x >> y >> z;
        vertices.emplace_back(x, y, z);
        }
      }
    in.close();
    }
  return true;
  }

bool write_xyz(const char* filename, const std::vector<jtk::vec3<float>>& vertices)
  {
  std::ofstream out;

  out.open(filename, std::ios::out);
  if (out.fail())
    return false;
  if (out.bad())
    return false;
  if (out.is_open())
    {
    out << vertices.size() << std::endl;
    for (size_t i = 0; i < vertices.size(); ++i)
      {
      float x = vertices[i][0];
      float y = vertices[i][1];
      float z = vertices[i][2];     
      out << x << " " << y << " " << z << std::endl;
      }
    out.close();
    }
  return true;
  }