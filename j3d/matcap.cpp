#include "matcap.h"

#include <jtk/vec.h>

#include "stb_image.h"

using namespace jtk;

void make_matcap_gray(matcap& _matcap)
  {
  _matcap.type = matcap_type::MATCAP_TYPE_INTERNAL_GRAY;
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
      if (u * u + v * v <= 1.01f)
        {
        float lw = std::sqrt(1.01f - u * u - v * v);
        vec3<float> sphere_point(u, v, lw);
        float cos_angle_1 = dot(sphere_point, normal_1);
        float cos_angle_2 = pow(dot(sphere_point, normal_2), 3.f);
        float cos_angle_3 = pow(dot(sphere_point, normal_3), 5.f);
        float cos_angle_4 = pow(dot(sphere_point, normal_3), 50.f);

        float r = 32 + (r1 * cos_angle_1 + r2 * cos_angle_2 + r3 * cos_angle_3 + r4 * cos_angle_4) / 1.f;
        float g = 32 + (g1 * cos_angle_1 + g2 * cos_angle_2 + g3 * cos_angle_3 + g4 * cos_angle_4) / 1.f;
        float b = 32 + (b1 * cos_angle_1 + b2 * cos_angle_2 + b3 * cos_angle_3 + b4 * cos_angle_4) / 1.f;

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
  _matcap.type = matcap_type::MATCAP_TYPE_INTERNAL_BROWN;
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
      if (u * u + v * v <= 1.01f)
        {
        float lw = std::sqrt(1.01f - u * u - v * v);
        vec3<float> sphere_point(u, v, lw);
        float cos_angle_1 = dot(sphere_point, normal_1);
        float cos_angle_2 = pow(dot(sphere_point, normal_2), 3.f);
        float cos_angle_3 = pow(dot(sphere_point, normal_3), 5.f);
        float cos_angle_4 = pow(dot(sphere_point, normal_3), 50.f);

        float r = 32 + (r1 * cos_angle_1 + r2 * cos_angle_2 + r3 * cos_angle_3 + r4 * cos_angle_4) / 1.f;
        float g = 20 + (g1 * cos_angle_1 + g2 * cos_angle_2 + g3 * cos_angle_3 + g4 * cos_angle_4) / 1.f;
        float b = 10 + (b1 * cos_angle_1 + b2 * cos_angle_2 + b3 * cos_angle_3 + b4 * cos_angle_4) / 1.f;

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
  _matcap.type = matcap_type::MATCAP_TYPE_INTERNAL_REDWAX;
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

  float r1 = 200.f / 1.5f;
  float g1 = 200.f / 4.f;
  float b1 = 150.f / 4.f;

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
      if (u * u + v * v <= 1.01f)
        {
        float lw = std::sqrt(1.01f - u * u - v * v);
        vec3<float> sphere_point(u, v, lw);
        float cos_angle_1 = dot(sphere_point, normal_1);
        float cos_angle_2 = pow(dot(sphere_point, normal_2), 3.f);
        float cos_angle_3 = pow(dot(sphere_point, normal_3), 5.f);
        float cos_angle_4 = pow(dot(sphere_point, normal_3), 50.f);

        float r = 32 + (r1 * cos_angle_1 + r2 * cos_angle_2 + r3 * cos_angle_3 + r4 * cos_angle_4) / 1.f;
        float g = (g1 * cos_angle_1 + g2 * cos_angle_2 + g3 * cos_angle_3 + g4 * cos_angle_4) / 1.f;
        float b = (b1 * cos_angle_1 + b2 * cos_angle_2 + b3 * cos_angle_3 + b4 * cos_angle_4) / 1.f;

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
  _matcap.type = matcap_type::MATCAP_TYPE_INTERNAL_SKETCH;
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
      float val = fabs(1.f - u * u - v * v);
      if (val < thres)
        {
        float scale = val / thres;
        uint32_t s = (uint32_t)(scale * 0x000000e1);
        *matcap_data = 0xff000000 | (s << 16) | (s << 8) | s;
        }
      else
        *matcap_data = 0xffe1e1e1;
      }
    }
  }

void make_matcap_from_file(matcap& _matcap, const char* filename)
  {
  int w, h, nr_of_channels;
  unsigned char* im = stbi_load(filename, &w, &h, &nr_of_channels, 4);
  if (im)
    {
    _matcap.type = matcap_type::MATCAP_TYPE_EXTERNAL_FILE;
    _matcap.filename = std::string(filename);
    _matcap.im = jtk::span_to_image(w, h, w, (const uint32_t*)im);
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t cnt = 0;
    for (uint32_t y = 0; y < (uint32_t)h; ++y)
      {
      uint32_t* raw = _matcap.im.row(y);
      for (uint32_t x = 0; x < (uint32_t)w; ++x, ++raw)
        {
        if ((*raw) & 0x00ffffff)
          {
          r += (*raw) & 0x000000ff;
          g += ((*raw) & 0x0000ff00) >> 8;
          b += ((*raw) & 0x00ff0000) >> 16;
          ++cnt;
          }
        }
      }
    if (cnt)
      {
      r /= cnt;
      g /= cnt;
      b /= cnt;
      }
    _matcap.cavity_clr = 0xff000000 | (b << 16) | (g << 8) | r;
    stbi_image_free(im);
    }
  else
    make_matcap_red_wax(_matcap);
  }

int32_t matcap_type_to_int(matcap_type t)
  {
  switch (t)
    {
    case matcap_type::MATCAP_TYPE_INTERNAL_REDWAX: return 0;
    case matcap_type::MATCAP_TYPE_INTERNAL_GRAY: return 1;
    case matcap_type::MATCAP_TYPE_INTERNAL_BROWN: return 2;
    case matcap_type::MATCAP_TYPE_INTERNAL_SKETCH: return 3;
    case matcap_type::MATCAP_TYPE_EXTERNAL_FILE: return 4;
    default: return 0;
    }
  }

matcap_type int_to_matcap_type(int32_t i)
  {
  switch (i)
    {
    case 0: return matcap_type::MATCAP_TYPE_INTERNAL_REDWAX;
    case 1: return  matcap_type::MATCAP_TYPE_INTERNAL_GRAY;
    case 2: return  matcap_type::MATCAP_TYPE_INTERNAL_BROWN;
    case 3: return  matcap_type::MATCAP_TYPE_INTERNAL_SKETCH;
    case 4: return  matcap_type::MATCAP_TYPE_EXTERNAL_FILE;
    default: return matcap_type::MATCAP_TYPE_INTERNAL_REDWAX;
    }
  }

void make_matcap(matcap& _matcap, matcap_type t, const char* filename)
  {
  switch (t)
    {
    case matcap_type::MATCAP_TYPE_INTERNAL_REDWAX: make_matcap_red_wax(_matcap); break;
    case matcap_type::MATCAP_TYPE_INTERNAL_GRAY: make_matcap_gray(_matcap); break;
    case matcap_type::MATCAP_TYPE_INTERNAL_BROWN: make_matcap_brown(_matcap); break;
    case matcap_type::MATCAP_TYPE_INTERNAL_SKETCH: make_matcap_sketch(_matcap); break;
    case matcap_type::MATCAP_TYPE_EXTERNAL_FILE: make_matcap_from_file(_matcap, filename); break;
    default: make_matcap_red_wax(_matcap); break;
    }
  }