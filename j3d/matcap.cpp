#include "matcap.h"

#include <jtk/vec.h>

using namespace jtk;

void make_matcap_gray(matcap& _matcap)
  {
  _matcap.cavity_clr = 0xff505050;
  uint32_t w = 512;
  uint32_t h = 512;
  _matcap.im = image<uint32_t>(w, h, true);
  vec3<float> normal_1(0, 0.8f, 1);
  normal_1 = normalize(normal_1);
  vec3<float> normal_2(0, 0.4f, 1);
  normal_2 = normalize(normal_2);
  vec3<float> normal_3(0, 0, 1);
  normal_3 = normalize(normal_3);

  float r1 = 200.f / 4.f;
  float g1 = 200.f / 4.f;
  float b1 = 200.f / 4.f;

  float r2 = 50.f;
  float g2 = 50.f;
  float b2 = 50.f;

  float r3 = 50.f;
  float g3 = 50.f;
  float b3 = 50.f;

  float r4 = 30.f;
  float g4 = 30.f;
  float b4 = 30.f;

  for (uint32_t y = 0; y < h; ++y)
    {
    uint32_t* matcap_data = _matcap.im.row(h - y - 1);
    for (uint32_t x = 0; x < w; ++x, ++matcap_data)
      {
      float u = (float)x / (float)(w - 1) * 2.f - 1.f;
      float v = (float)y / (float)(h - 1) * 2.f - 1.f;
      if (u*u + v * v <= 1.01f)
        {
        float lw = std::sqrt(1.01f - u * u - v * v);
        vec3<float> sphere_point(u, v, lw);
        float cos_angle_1 = dot(sphere_point, normal_1);
        float cos_angle_2 = pow(dot(sphere_point, normal_2), 3.f);
        float cos_angle_3 = pow(dot(sphere_point, normal_3), 5.f);
        float cos_angle_4 = pow(dot(sphere_point, normal_3), 50.f);

        float r = 32 + (r1*cos_angle_1 + r2 * cos_angle_2 + r3 * cos_angle_3 + r4 * cos_angle_4) / 1.f;
        float g = 32 + (g1*cos_angle_1 + g2 * cos_angle_2 + g3 * cos_angle_3 + g4 * cos_angle_4) / 1.f;
        float b = 32 + (b1*cos_angle_1 + b2 * cos_angle_2 + b3 * cos_angle_3 + b4 * cos_angle_4) / 1.f;

        if (r > 255.f)
          r = 255.f;
        if (r < 0.f)
          r = 0.f;

        if (g > 255.f)
          g = 255.f;
        if (g < 0.f)
          g = 0.f;

        if (b > 255.f)
          b = 255.f;
        if (b < 0.f)
          b = 0.f;

        unsigned char red = (unsigned char)r;
        unsigned char green = (unsigned char)g;
        unsigned char blue = (unsigned char)b;
        uint32_t clr = 0xff000000 | (uint32_t(blue) << 16) | (uint32_t(green) << 8) | uint32_t(red);
        *matcap_data = clr;
        }
      else
        *matcap_data = 0xff000000;
      }
    }
  }


void make_matcap_brown(matcap& _matcap)
  {
  _matcap.cavity_clr = 0xff405060;
  uint32_t w = 512;
  uint32_t h = 512;
  _matcap.im = image<uint32_t>(w, h, true);
  vec3<float> normal_1(0, 0.8f, 1);
  normal_1 = normalize(normal_1);
  vec3<float> normal_2(0, 0.4f, 1);
  normal_2 = normalize(normal_2);
  vec3<float> normal_3(0, 0, 1);
  normal_3 = normalize(normal_3);

  float r1 = 200.f / 4.f;
  float g1 = 180.f / 4.f;
  float b1 = 160.f / 4.f;

  float r2 = 50.f;
  float g2 = 40.f;
  float b2 = 30.f;

  float r3 = 50.f;
  float g3 = 50.f;
  float b3 = 50.f;

  float r4 = 30.f;
  float g4 = 30.f;
  float b4 = 30.f;

  for (uint32_t y = 0; y < h; ++y)
    {
    uint32_t* matcap_data = _matcap.im.row(h - y - 1);
    for (uint32_t x = 0; x < w; ++x, ++matcap_data)
      {
      float u = (float)x / (float)(w - 1) * 2.f - 1.f;
      float v = (float)y / (float)(h - 1) * 2.f - 1.f;
      if (u*u + v * v <= 1.01f)
        {
        float lw = std::sqrt(1.01f - u * u - v * v);
        vec3<float> sphere_point(u, v, lw);
        float cos_angle_1 = dot(sphere_point, normal_1);
        float cos_angle_2 = pow(dot(sphere_point, normal_2), 3.f);
        float cos_angle_3 = pow(dot(sphere_point, normal_3), 5.f);
        float cos_angle_4 = pow(dot(sphere_point, normal_3), 50.f);

        float r = 32 + (r1*cos_angle_1 + r2 * cos_angle_2 + r3 * cos_angle_3 + r4 * cos_angle_4) / 1.f;
        float g = 20 + (g1*cos_angle_1 + g2 * cos_angle_2 + g3 * cos_angle_3 + g4 * cos_angle_4) / 1.f;
        float b = 10 + (b1*cos_angle_1 + b2 * cos_angle_2 + b3 * cos_angle_3 + b4 * cos_angle_4) / 1.f;

        if (r > 255.f)
          r = 255.f;
        if (r < 0.f)
          r = 0.f;

        if (g > 255.f)
          g = 255.f;
        if (g < 0.f)
          g = 0.f;

        if (b > 255.f)
          b = 255.f;
        if (b < 0.f)
          b = 0.f;

        unsigned char red = (unsigned char)r;
        unsigned char green = (unsigned char)g;
        unsigned char blue = (unsigned char)b;
        uint32_t clr = 0xff000000 | (uint32_t(blue) << 16) | (uint32_t(green) << 8) | uint32_t(red);
        *matcap_data = clr;
        }
      else
        *matcap_data = 0xff000000;
      }
    }
  }

void make_matcap_red_wax(matcap& _matcap)
  {
  _matcap.cavity_clr = 0xFF7D7DFF;
  uint32_t w = 512;
  uint32_t h = 512;
  _matcap.im = image<uint32_t>(w, h, true);
  vec3<float> normal_1(0, 0.8f, 1);
  normal_1 = normalize(normal_1);
  vec3<float> normal_2(0, 0.4f, 1);
  normal_2 = normalize(normal_2);
  vec3<float> normal_3(0, 0, 1);
  normal_3 = normalize(normal_3);

  float r1 = 200.f/1.5f;
  float g1 = 200.f/4.f;
  float b1 = 150.f/4.f;

  float r2 = 30.f;
  float g2 = 25.f;
  float b2 = 20.f;

  float r3 = 50.f;
  float g3 = 50.f;
  float b3 = 50.f;

  float r4 = 30.f;
  float g4 = 30.f;
  float b4 = 30.f;

  for (uint32_t y = 0; y < h; ++y)
    {
    uint32_t* matcap_data = _matcap.im.row(h - y - 1);
    for (uint32_t x = 0; x < w; ++x, ++matcap_data)
      {
      float u = (float)x / (float)(w - 1) * 2.f - 1.f;
      float v = (float)y / (float)(h - 1) * 2.f - 1.f;
      if (u*u + v * v <= 1.01f)
        {
        float lw = std::sqrt(1.01f - u * u - v * v);
        vec3<float> sphere_point(u, v, lw);
        float cos_angle_1 = dot(sphere_point, normal_1);
        float cos_angle_2 = pow(dot(sphere_point, normal_2), 3.f);
        float cos_angle_3 = pow(dot(sphere_point, normal_3), 5.f);
        float cos_angle_4 = pow(dot(sphere_point, normal_3), 50.f);

        float r = 32+(r1*cos_angle_1 + r2 * cos_angle_2 + r3 * cos_angle_3 + r4 * cos_angle_4) / 1.f;
        float g = (g1*cos_angle_1 + g2 * cos_angle_2 + g3 * cos_angle_3 + g4 * cos_angle_4) / 1.f;
        float b = (b1*cos_angle_1 + b2 * cos_angle_2 + b3 * cos_angle_3 + b4 * cos_angle_4) / 1.f;

        if (r > 255.f)
          r = 255.f;
        if (r < 0.f)
          r = 0.f;

        if (g > 255.f)
          g = 255.f;
        if (g < 0.f)
          g = 0.f;

        if (b > 255.f)
          b = 255.f;
        if (b < 0.f)
          b = 0.f;

        unsigned char red = (unsigned char)r;
        unsigned char green = (unsigned char)g;
        unsigned char blue = (unsigned char)b;
        uint32_t clr = 0xff000000 | (uint32_t(blue) << 16) | (uint32_t(green) << 8) | uint32_t(red);
        *matcap_data = clr;
        }
      else
        *matcap_data = 0xff000000;        
      }
    }
  }

void make_matcap_sketch(matcap& _matcap)
  {
  _matcap.cavity_clr = 0xFF505050;

  const float thres = 0.4f;
  uint32_t w = 512;
  uint32_t h = 512;
  _matcap.im = image<uint32_t>(w, h, true);
  for (uint32_t y = 0; y < h; ++y)
    {
    uint32_t* matcap_data = _matcap.im.row(h - y - 1);
    for (uint32_t x = 0; x < w; ++x, ++matcap_data)
      {      
      float u = (float)x / (float)(w - 1) * 2.f - 1.f;
      float v = (float)y / (float)(h - 1) * 2.f - 1.f;      
      float val = fabs(1.f - u*u - v*v);
      if (val < thres)
        {
        float scale = val/ thres;
        uint32_t s = (uint32_t)(scale*0x000000e1);
        *matcap_data = 0xff000000 | (s << 16) | (s << 8) | s;
        }
      else
        *matcap_data = 0xffe1e1e1;
      }
    }
  }


matcapmap::matcapmap()
  {
  matcap m;
  make_matcap_red_wax(m);
  matcaps[0] = m;
  make_matcap_gray(m);
  matcaps[1] = m;
  make_matcap_brown(m);
  matcaps[2] = m;
  make_matcap_sketch(m);
  matcaps[3] = m;
  }

const matcap& matcapmap::get_matcap(uint32_t id) const
  {
  return optimized_map.find(id)->second->second;
  }

void matcapmap::make_new_matcap(uint32_t matcap_id, const matcap& mc)
  {
  matcaps[matcap_id] = mc;
  }

void matcapmap::map_db_id_to_matcap_id(uint32_t db_id, uint32_t matcap_id)
  {
  if (matcaps.find(matcap_id) == matcaps.end())
    map_db_id_to_matcap[db_id] = 0;
  else
    map_db_id_to_matcap[db_id] = matcap_id;
  _optimize_maps();
  }

void matcapmap::_optimize_maps()
  {
  optimized_map.clear();
  for (const auto& m : map_db_id_to_matcap)
    {
    auto it = matcaps.find(m.second);
    if (it == matcaps.end())
      it = matcaps.begin();
    optimized_map[m.first] = it;
    }
  }

uint32_t matcapmap::get_semirandom_matcap_id_for_given_db_id(uint32_t db_id) const
  {
  uint32_t count = db_id % matcaps.size();
  auto it = matcaps.begin();
  for (uint32_t i = 0; i < count; ++i)
    ++it;
  return it->first;
  }