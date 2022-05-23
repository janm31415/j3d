#include "canvas.h"

#include <jtk/concurrency.h>
#include <jtk/qbvh.h>

#include "matcap.h"

extern "C"
  {
#include "trackball.h"
  }
  
#define USE_THREAD_POOL

using namespace jtk;

namespace
  {

  inline float4x4 quaternion_to_rotation(float* quaternion)
    {
    float4x4 rot;
    rot[0] = 1.f - 2.f * (quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2]);
    rot[1] = 2.f * (quaternion[0] * quaternion[1] - quaternion[2] * quaternion[3]);
    rot[2] = 2.f * (quaternion[2] * quaternion[0] + quaternion[1] * quaternion[3]);
    rot[3] = 0.f;

    rot[4] = 2.f * (quaternion[0] * quaternion[1] + quaternion[2] * quaternion[3]);
    rot[5] = 1.f - 2.f * (quaternion[2] * quaternion[2] + quaternion[0] * quaternion[0]);
    rot[6] = 2.f * (quaternion[1] * quaternion[2] - quaternion[0] * quaternion[3]);
    rot[7] = 0.f;

    rot[8] = 2.f * (quaternion[2] * quaternion[0] - quaternion[1] * quaternion[3]);
    rot[9] = 2.f * (quaternion[1] * quaternion[2] + quaternion[0] * quaternion[3]);
    rot[10] = 1.f - 2.f * (quaternion[1] * quaternion[1] + quaternion[0] * quaternion[0]);
    rot[11] = 0.f;

    rot[12] = 0.f;
    rot[13] = 0.f;
    rot[14] = 0.f;
    rot[15] = 1.f;
    return rot;
    }
  /*
  inline uint32_t make_color(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue)
    {
    return (uint32_t(alpha) << 24) | (uint32_t(blue) << 16) | (uint32_t(green) << 8) | uint32_t(red);
    }

  inline uint32_t make_color(unsigned char red, unsigned char green, unsigned char blue)
    {
    return 0xff000000 | (uint32_t(blue) << 16) | (uint32_t(green) << 8) | uint32_t(red);
    }
    */
  void fill_background(jtk::image<uint32_t>& bg)
    {
    const uint32_t h = bg.height();
    const uint32_t w = bg.width();
    for (uint32_t y = 0; y < h; ++y)
      {
      uint32_t* p_bg = bg.row(y);
      float clrf = (float)y / (float)h * 75.f;
      uint32_t clr = make_color((unsigned char)clrf, (unsigned char)clrf, (unsigned char)clrf);
      for (uint32_t x = 0; x < w; ++x, ++p_bg)
        *p_bg = clr;
      }
    }

  void copy(jtk::image<uint32_t>& dest, const jtk::image<uint32_t>& src)
    {
    const uint32_t h = src.height();
    const uint32_t w = src.width() >> 2;
    for (uint32_t y = 0; y < h; ++y)
      {
      uint32_t* p_dest = dest.row(y);
      const uint32_t* p_src = src.row(y);

      for (uint32_t x = 0; x < w; ++x, p_dest += 4, p_src += 4)
        {
        __m128 buffer = _mm_load_ps((float*)p_src);
        _mm_store_ps((float*)p_dest, buffer);
        }

      }
    }
  }

canvas::canvas()
  {
  _tp.init();
  }

canvas::canvas(uint32_t w, uint32_t h)
  {
  _tp.init();
  resize(w, h);
  }

canvas::~canvas()
  {
#if defined(USE_THREAD_POOL)
  _tp.stop();
#endif
  }

void canvas::resize(uint32_t w, uint32_t h)
  {
  im = jtk::image<uint32_t>(w,h);
  background = jtk::image<uint32_t>(w,h);
  _canvas = jtk::image<pixel>(w, h);
  _u = jtk::image<float>(w, h);
  _v = jtk::image<float>(w, h);
  buffer = jtk::image<jtk::float4>(w, h);
  for (auto& p : _canvas)
    {
    p.db_id = 0;
    p.object_id = (uint32_t)-1;
    }
  _camera = make_default_camera();
  projection_matrix = make_projection_matrix(_camera, w, h);
  projection_matrix_inv = invert_projection_matrix(projection_matrix);
  fill_background(background);
  }

void canvas::set_background_color(uint8_t r, uint8_t g, uint8_t b)
  {
  uint32_t clr = 0xff000000 | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
  for (auto& c : background)
    c = clr;
  }

void canvas::get_pixel(pixel& p, const mouse_data& data, float mouse_offset_x, float mouse_offset_y)
  {
  uint32_t X = (uint32_t)(data.mouse_x - mouse_offset_x);
  uint32_t Y = (uint32_t)(data.mouse_y - mouse_offset_y);
  p = _canvas(X, Y);
  }

void canvas::get_pixel(pixel& p, float pos_x, float pos_y, float mouse_offset_x, float mouse_offset_y)
  {
  uint32_t X = (uint32_t)(pos_x - mouse_offset_x);
  uint32_t Y = (uint32_t)(pos_y - mouse_offset_y);
  p = _canvas(X, Y);
  }

void canvas::do_mouse(bool& refresh, mouse_data& data, scene& s, float mouse_offset_x, float mouse_offset_y)
  {
  if (data.left_button_down || data.right_button_down || data.wheel_down)
    {
    uint32_t X = (uint32_t)(data.mouse_x - mouse_offset_x);
    uint32_t Y = (uint32_t)(data.mouse_y - mouse_offset_y);
    auto f = _canvas(X, Y);
    //if (f.object_id != (uint32_t)-1)
    if (f.db_id)
      {
      const uint32_t w = im.width();
      const uint32_t h = im.height();

      float4 screen_pos((2.f * ((X + 0.5f) / w) - 1.f), (2.f * ((Y + 0.5f) / h) - 1.f), _camera.nearClippingPlane, 1.f);
      float4 dir = matrix_vector_multiply(projection_matrix_inv, screen_pos);
      dir[3] = 0.f;
      dir = matrix_vector_multiply(s.coordinate_system, dir);
      float4 origin(0.f, 0.f, 0.f, 1.f);
      origin = matrix_vector_multiply(s.coordinate_system, origin);
      float4 pt = origin + f.depth * dir;
      s.pivot[0] = pt[0];
      s.pivot[1] = pt[1];
      s.pivot[2] = pt[2];
      }
    }
  if (data.prev_mouse_x != data.mouse_x || data.prev_mouse_y != data.mouse_y)
    {
    if ((data.right_dragging || data.left_dragging) && !data.ctrl_pressed)
      {
      refresh = true;
      float spin_quat[4];
      float wf = float(im.width());
      float hf = float(im.height());
      float x = float(data.mouse_x) - mouse_offset_x;
      float y = float(data.mouse_y) - mouse_offset_y;
      trackball(spin_quat,
        -(wf - 2.0f*(data.prev_mouse_x - mouse_offset_x)) / wf,
        -(2.0f*(data.prev_mouse_y - mouse_offset_y) - hf) / hf,
        -(wf - 2.0f*x) / wf,
        -(2.0f*y - hf) / hf);
      auto rot = quaternion_to_rotation(spin_quat);

      float4 c;
      c[0] = s.pivot[0];
      c[1] = s.pivot[1];
      c[2] = s.pivot[2];
      c[3] = 1.0;
      auto center_cam = matrix_vector_multiply(s.coordinate_system_inv, c);

      auto t1 = make_translation(center_cam[0], center_cam[1], center_cam[2]);
      auto t2 = make_translation(-center_cam[0], -center_cam[1], -center_cam[2]);

      auto tmp1 = matrix_matrix_multiply(t2, s.coordinate_system_inv);
      auto tmp2 = matrix_matrix_multiply(t1, rot);
      s.coordinate_system_inv = matrix_matrix_multiply(tmp2, tmp1);
      s.coordinate_system = invert_orthonormal(s.coordinate_system_inv);

      data.prev_mouse_x = data.mouse_x;
      data.prev_mouse_y = data.mouse_y;
      }
    else if (data.wheel_mouse_pressed || ((data.right_dragging || data.left_dragging) && data.ctrl_pressed))
      {
      refresh = true;
      float x = float(data.mouse_x);
      float y = float(data.mouse_y);

      float cx = s.pivot[0];
      float cy = s.pivot[1];
      float cz = s.pivot[2];

      float4 v(cx, cy, cz, 1.f);
      v = matrix_vector_multiply(s.coordinate_system_inv, v);
      v = matrix_vector_multiply(projection_matrix, v);
      v[0] /= v[3];
      v[1] /= v[3];
      v[2] /= v[3];

      cz = v[3];

      float projection_width = projection_matrix_inv[0];
      float projection_height = projection_matrix_inv[5];


      float dist_x_to_translate = 2.f*(x - data.prev_mouse_x) / float(im.width())*projection_width*cz;
      float dist_y_to_translate = 2.f*(y - data.prev_mouse_y) / float(im.height())*projection_height*cz;

      s.coordinate_system_inv[12] += dist_x_to_translate;
      s.coordinate_system_inv[13] += dist_y_to_translate;
      s.coordinate_system_inv[14] += 0.f;

      s.coordinate_system = invert_orthonormal(s.coordinate_system_inv);

      data.prev_mouse_x = data.mouse_x;
      data.prev_mouse_y = data.mouse_y;
      }
    }
  if (data.wheel_rotation != 0.f)
    {
    refresh = true;
    float4 camera_orig(0.f, 0.f, _camera.nearClippingPlane, 1.f);
    camera_orig = matrix_vector_multiply(projection_matrix_inv, camera_orig);
    camera_orig[3] = 1.f;
    camera_orig = matrix_vector_multiply(s.coordinate_system, camera_orig);

    camera_orig[0] -= s.pivot[0];
    camera_orig[1] -= s.pivot[1];
    camera_orig[2] -= s.pivot[2];

    float length = std::sqrt(camera_orig[0] * camera_orig[0] + camera_orig[1] * camera_orig[1] + camera_orig[2] * camera_orig[2]);
    float z = length / 5.f;

    if (data.wheel_rotation > 0)
      {
      float4 v(0.f, 0.f, z, 0.f);
      v = matrix_vector_multiply(s.coordinate_system, v);
      s.coordinate_system[12] += v[0];
      s.coordinate_system[13] += v[1];
      s.coordinate_system[14] += v[2];
      }
    else if (data.wheel_rotation < 0)
      {
      float4 v(0.f, 0.f, -z, 0.f);
      v = matrix_vector_multiply(s.coordinate_system, v);
      s.coordinate_system[12] += v[0];
      s.coordinate_system[13] += v[1];
      s.coordinate_system[14] += v[2];
      }
    s.coordinate_system_inv = invert_orthonormal(s.coordinate_system);
    data.wheel_rotation = 0.0;
    }
  }

float canvas::compute_convex_cos_angle(float x1, float y1, float u1, float v1, float depth1, float x2, float y2, float u2, float v2, float depth2)
  {
  float4 screen_pos1((2.f * ((x1 + 0.5f) / _canvas.width()) - 1.f), (2.f * ((y1 + 0.5f) / _canvas.height()) - 1.f), _camera.nearClippingPlane, 1.f);
  float4 dir1 = matrix_vector_multiply(projection_matrix_inv, screen_pos1);
  dir1[3] = 0.f;

  float4 pt1 = depth1 * dir1;

  float4 screen_pos2((2.f * ((x2 + 0.5f) / _canvas.width()) - 1.f), (2.f * ((y2 + 0.5f) / _canvas.height()) - 1.f), _camera.nearClippingPlane, 1.f);
  float4 dir2 = matrix_vector_multiply(projection_matrix_inv, screen_pos2);
  dir2[3] = 0.f;

  float4 pt2 = depth2 * dir2;

  float4 n1(u1, v1, std::sqrt(1.f - u1 * u1 - v1 * v1), 0.f);
  float4 n2(u2, v2, std::sqrt(1.f - u2 * u2 - v2 * v2), 0.f);

  float angle;

  if (std::abs(dot(n1, n2) - 1.f) > 0.0001)
    {
    auto pt = pt2 - pt1;
    pt = pt / std::sqrt(dot(pt, pt));
    angle = dot(pt, n1);
    }
  else
    angle = 0.f;
  return angle;
  }

namespace
  {

  inline uint32_t get_U(float u, const matcap& _matcap)
    {
    return (uint32_t)std::floor(0.5f + (u + 1.f)*(_matcap.im.width() - 1)*0.5f);
    }

  inline uint32_t get_V(float v, const matcap& _matcap)
    {
    return (uint32_t)std::floor(0.5f + (-v + 1.f)*(_matcap.im.height() - 1)*0.5f);
    }

  inline uint32_t get_color(const matcap& _matcap, uint32_t U, uint32_t V, uint32_t shadow)
    {
    uint32_t clr = _matcap.im(U, V);
    if (shadow)
      {
      uint32_t r = clr & 0x000000ff;
      uint32_t g = (clr & 0x0000ff00) >> 8;
      uint32_t b = (clr & 0x00ff0000) >> 16;
      r = (r) >> 2;
      g = (g) >> 2;
      b = (b) >> 2;
      clr = 0xff000000 | (b << 16) | (g << 8) | r;
      }
    return clr;
    }

  inline uint32_t get_angle_color(float angle, const matcap& _matcap, float u, float v, uint32_t mark)
    {
    int U = get_U(u, _matcap);
    int V = get_V(v, _matcap);
    auto clr = get_color(_matcap, U, V, mark);
    if (std::abs(angle) <= 1.f)
      {
      uint32_t r = clr & 0x000000ff;
      uint32_t g = (clr & 0x0000ff00) >> 8;
      uint32_t b = (clr & 0x00ff0000) >> 16;

      float scale = (1.57079632679489f - std::abs(acos(angle) - 1.57079632679489f)) / 1.57079632679489f;
      scale = std::sqrt(1.f - scale);
      if (angle > 0.f) // concave
        {
        auto clr2 = _matcap.cavity_clr;
        uint32_t r2 = clr2 & 0x000000ff;
        uint32_t g2 = (clr2 & 0x0000ff00) >> 8;
        uint32_t b2 = (clr2 & 0x00ff0000) >> 16;
        r = (uint32_t)(r * (1.f - scale) + r2 * scale*1.2f);
        g = (uint32_t)(g * (1.f - scale) + g2 * scale*1.2f);
        b = (uint32_t)(b * (1.f - scale) + b2 * scale*1.2f);
        }
      else
        {
        auto clr2 = _matcap.cavity_clr;
        uint32_t r2 = clr2 & 0x000000ff;
        uint32_t g2 = (clr2 & 0x0000ff00) >> 8;
        uint32_t b2 = (clr2 & 0x00ff0000) >> 16;
        r = (uint32_t)(r * (1.f - scale) + r2 * scale*0.5f);
        g = (uint32_t)(g * (1.f - scale) + g2 * scale*0.5f);
        b = (uint32_t)(b * (1.f - scale) + b2 * scale*0.5f);
        }

      if (r > 255)
        r = 255;
      if (g > 255)
        g = 255;
      if (b > 255)
        b = 255;
      clr = 0xff000000 | (b << 16) | (g << 8) | r;
      }
    return clr;
    }


  }

void canvas::canvas_to_image(const matcapmap& _matcap)
  {
  canvas_to_image(_canvas, _matcap);
  }



namespace
  {
  float clamp(float x, float minval, float maxval)
    {
    return x < minval ? minval : (x > maxval ? maxval : x);
    }
  }


uint32_t canvas::_get_color(const pixel* p, const matcap& _matcap, float u, float v) const
  {
  uint32_t clr;
  if (p->mark & 2)
    {
    if (_settings.shading)
      {
      float4 nor(u, v, sqrt(1.f - u * u - v * v), 0.f);
      float4 lig(0.f, 0.f, 1.f, 0.f);
      float occ = p->mark & 1 ? 0.3f : 1.f;
      float dif = clamp(dot(nor, lig), 0.f, 1.f)*occ;

      clr = 0xff000000 | ((uint32_t)(p->b*dif) << 16) | ((uint32_t)(p->g*dif) << 8) | ((uint32_t)(p->r*dif));
      }
    else
      {
      if (p->mark & 1)
        clr = 0xff000000 | ((uint32_t)(p->b >> 2) << 16) | ((uint32_t)(p->g >> 2) << 8) | ((uint32_t)(p->r >> 2));
      else
        clr = 0xff000000 | ((uint32_t)p->b << 16) | ((uint32_t)p->g << 8) | ((uint32_t)p->r);
      }

    }
  else
    {
    int U = get_U(u, _matcap);
    int V = get_V(v, _matcap);

    clr = get_color(_matcap, U, V, p->mark);
    }
  return clr;
  }

uint32_t canvas::_get_color(const pixel* p, const matcapmap& _matcap) const
  {
  return _get_color(p, _matcap.get_matcap(p->db_id), p->u, p->v);
  }


void canvas::_render_wireframe(const jtk::image<pixel>& canvas, const matcapmap& _matcap)
  {
  const uint32_t w = im.width();
  const uint32_t h = im.height();

  const pixel* p_up_canvas = canvas.row(0);

  for (uint32_t y = 0; y < h; ++y)
    {
    uint32_t* p_im_line = im.row(y);
    const pixel* p_canvas = canvas.row(y);
    const pixel* p_right_canvas = p_canvas + 1;

    for (uint32_t x = 0; x < w - 1; ++x)
      {
      if (p_canvas->object_id != (uint32_t)(-1))
        {

        if ((p_right_canvas->object_id != (uint32_t)(-1)) && (p_right_canvas->object_id != p_canvas->object_id))
          {
          const float scale = (p_canvas->u*p_canvas->u + p_canvas->v*p_canvas->v)*0.5f;
          const auto wire_color = make_color((unsigned char)(255 * scale), (unsigned char)(255 * scale), (unsigned char)(255 * scale));
          *p_im_line = wire_color;
          }
        else if ((p_up_canvas->object_id != (uint32_t)(-1)) && (p_up_canvas->object_id != p_canvas->object_id))
          {
          const float scale = (p_canvas->u*p_canvas->u + p_canvas->v*p_canvas->v)*0.5f;
          const auto wire_color = make_color((unsigned char)(255 * scale), (unsigned char)(255 * scale), (unsigned char)(255 * scale));
          *p_im_line = wire_color;
          }
        else
          {
          *p_im_line = _get_color(p_canvas, _matcap);
          }

        }
      ++p_im_line;
      ++p_canvas;
      ++p_up_canvas;
      ++p_right_canvas;
      }
    if (p_canvas->object_id != (uint32_t)(-1))
      {
      *p_im_line = _get_color(p_canvas, _matcap);
      }
    p_up_canvas = canvas.row(y);
    }
  }

void canvas::_canvas_to_one_bit_image(const jtk::image<pixel>& _combined_canvas, const matcapmap& _matcap)
  {
  const uint32_t black = 0xff000000;
  const uint32_t white = 0xffffffff;

  const uint32_t w = im.width();
  const uint32_t h = im.height();

  const float threshold = 0.001f;
  const pixel* p_up_combined_canvas = _combined_canvas.row(0);

  for (uint32_t y = 0; y < h; ++y)
    {
    uint32_t* p_im_line = im.row(y);
    const pixel* p_combined_canvas = _combined_canvas.row(y);
    const pixel* p_right_combined_canvas = p_combined_canvas + 1;

    for (uint32_t x = 0; x < w - 1; ++x)
      {
      if (p_combined_canvas->object_id != (uint32_t)(-1))
        {
        float angle = 1.f;
        if ((p_right_combined_canvas->object_id != (uint32_t)(-1)) && ((fabs(p_combined_canvas->u - p_right_combined_canvas->u) > threshold) || (fabs(p_combined_canvas->v - p_right_combined_canvas->v) > threshold)))
          {
          float u1 = p_combined_canvas->u;
          float v1 = p_combined_canvas->v;
          float u2 = p_right_combined_canvas->u;
          float v2 = p_right_combined_canvas->v;

          float w1 = std::sqrt(1.f - u1 * u1 - v1 * v1);
          float w2 = std::sqrt(1.f - u2 * u2 - v2 * v2);

          angle = u1 * u2 + v1 * v2 + w1 * w2;
          }
        else if ((p_up_combined_canvas->object_id != (uint32_t)(-1)) && ((fabs(p_combined_canvas->u - p_up_combined_canvas->u) > threshold) || (fabs(p_combined_canvas->v - p_up_combined_canvas->v) > threshold)))
          {
          float u1 = p_combined_canvas->u;
          float v1 = p_combined_canvas->v;
          float u2 = p_up_combined_canvas->u;
          float v2 = p_up_combined_canvas->v;

          float w1 = std::sqrt(1.f - u1 * u1 - v1 * v1);
          float w2 = std::sqrt(1.f - u2 * u2 - v2 * v2);

          angle = u1 * u2 + v1 * v2 + w1 * w2;
          }
        int U = get_U(p_combined_canvas->u, _matcap.get_matcap(p_combined_canvas->db_id));
        int V = get_V(p_combined_canvas->v, _matcap.get_matcap(p_combined_canvas->db_id));
        auto clr = get_color(_matcap.get_matcap(p_combined_canvas->db_id), U, V, p_combined_canvas->mark);
        int res = (((clr & 0xff0000) >> 16) + ((clr & 0xff00) >> 8) + (clr & 0xff)) >> 7;
        ++res;
        bool clr_black = (((x % res == 0) && (y % res == 0)));
        if (fabs(angle) < 0.95f)
          {
          if (res == 1)
            clr_black = !clr_black;
          else
            clr_black = true;
          }
        *p_im_line = clr_black ? black : white;
        }
      ++p_im_line;
      ++p_combined_canvas;
      ++p_up_combined_canvas;
      ++p_right_combined_canvas;
      }
    if (p_combined_canvas->object_id != (uint32_t)(-1))
      {
      int U = get_U(p_combined_canvas->u, _matcap.get_matcap(p_combined_canvas->db_id));
      int V = get_V(p_combined_canvas->v, _matcap.get_matcap(p_combined_canvas->db_id));

      auto clr = get_color(_matcap.get_matcap(p_combined_canvas->db_id), U, V, p_combined_canvas->mark);
      int res = (((clr & 0xff0000) >> 16) + ((clr & 0xff00) >> 8) + (clr & 0xff)) >> 7;
      ++res;
      if ((((w - 1) % res == 0) && (y % res == 0)))
        *p_im_line = black;
      else
        *p_im_line = white;
      }
    p_up_combined_canvas = _combined_canvas.row(y);
    }
  }


void canvas::canvas_to_image(const jtk::image<pixel>& _combined_canvas, const matcapmap& _matcap)
  {
  if (_settings.one_bit)
    {
    _canvas_to_one_bit_image(_combined_canvas, _matcap);
    }
  else if (_settings.wireframe)
    _render_wireframe(_combined_canvas, _matcap);
  else if (_settings.edges)
    {
    const uint32_t w = im.width();
    const uint32_t h = im.height();

    const float threshold = 0.001f;
    const pixel* p_up_combined_canvas = _combined_canvas.row(0);

    for (uint32_t y = 0; y < h; ++y)
      {
      uint32_t* p_im_line = im.row(y);
      const pixel* p_combined_canvas = _combined_canvas.row(y);
      const pixel* p_right_combined_canvas = p_combined_canvas + 1;

      for (uint32_t x = 0; x < w - 1; ++x)
        {
        if (p_combined_canvas->object_id != (uint32_t)(-1))
          {

          if ((p_right_combined_canvas->object_id != (uint32_t)(-1)) && ((fabs(p_combined_canvas->u - p_right_combined_canvas->u) > threshold) || (fabs(p_combined_canvas->v - p_right_combined_canvas->v) > threshold)))
            {
            float u1 = p_combined_canvas->u;
            float v1 = p_combined_canvas->v;
            float u2 = p_right_combined_canvas->u;
            float v2 = p_right_combined_canvas->v;

            float angle = compute_convex_cos_angle((float)x, (float)y, u1, v1, p_combined_canvas->depth, (float)x + 1.f, (float)y, u2, v2, p_right_combined_canvas->depth);

            auto clr = get_angle_color(angle, _matcap.get_matcap(p_combined_canvas->db_id), u1, v1, p_combined_canvas->mark);
            *p_im_line = clr;
            }
          else if ((p_up_combined_canvas->object_id != (uint32_t)(-1)) && ((fabs(p_combined_canvas->u - p_up_combined_canvas->u) > threshold) || (fabs(p_combined_canvas->v - p_up_combined_canvas->v) > threshold)))
            {
            float u1 = p_combined_canvas->u;
            float v1 = p_combined_canvas->v;
            float u2 = p_up_combined_canvas->u;
            float v2 = p_up_combined_canvas->v;

            float angle = compute_convex_cos_angle((float)x, (float)y, u1, v1, p_combined_canvas->depth, (float)x, (float)y - 1.f, u2, v2, p_up_combined_canvas->depth);
            auto clr = get_angle_color(angle, _matcap.get_matcap(p_combined_canvas->db_id), u1, v1, p_combined_canvas->mark);
            *p_im_line = clr;
            }
          else
            {
            *p_im_line = _get_color(p_combined_canvas, _matcap);
            }

          }
        ++p_im_line;
        ++p_combined_canvas;
        ++p_up_combined_canvas;
        ++p_right_combined_canvas;
        }
      if (p_combined_canvas->object_id != (uint32_t)(-1))
        {
        *p_im_line = _get_color(p_combined_canvas, _matcap);
        }
      p_up_combined_canvas = _combined_canvas.row(y);
      }
    } // if edges
  else
    {
    const uint32_t w = im.width();
    const uint32_t h = im.height();

    for (uint32_t y = 0; y < h; ++y)
      {
      uint32_t* p_im_line = im.row(y);
      const pixel* p_combined_canvas = _combined_canvas.row(y);
      for (uint32_t x = 0; x < w; ++x)
        {
        if (p_combined_canvas->object_id != (uint32_t)(-1))
          {
          *p_im_line = _get_color(p_combined_canvas, _matcap);
          }
        ++p_im_line;
        ++p_combined_canvas;
        }
      }
    }
  }

void canvas::update_canvas(int x0, int y0, int x1, int y1, const scene& s)
  {
  update_canvas(_canvas, x0, y0, x1, y1, s);
  }

void canvas::update_canvas(jtk::image<pixel>& out, int x0, int y0, int x1, int y1, const scene& s)
  {
  const uint32_t w = im.width();
  const uint32_t h = im.height();

  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 < 0)
    x1 = 0;
  if (y1 < 0)
    y1 = 0;

  if (x0 >= (int)w)
    x0 = w - 1;
  if (y0 >= (int)h)
    y0 = h - 1;
  if (x1 >= (int)w)
    x1 = w - 1;
  if (y1 >= (int)h)
    y1 = h - 1;

  float4 origin(0.f, 0.f, 0.f, 1.f);
  origin = matrix_vector_multiply(s.coordinate_system, origin);

  float4 light(s.pivot[0] + s.diagonal*3.f, s.pivot[1] + s.diagonal*3.f, s.pivot[2] + s.diagonal*3.f, 1.f);

  light = matrix_vector_multiply(s.coordinate_system, light);

  std::vector<const qbvh*> bvhs;
  aligned_vector<float4x4> object_cs;
  aligned_vector<float4x4> inverted_object_cs;
  std::vector<const vec3<uint32_t>*> triangles;
  std::vector<const vec3<float>*> vertices;
  std::vector<const vec3<float>*> triangle_normals;
  std::vector<const vec3<float>*> vertex_colors;
  std::vector<const vec3<vec2<float>>*> uv_coordinates;
  std::vector<const image<uint32_t>*> textures;
  std::vector<uint32_t> db_ids;
  const uint32_t max_submodels = (uint32_t)s.objects.size();
  bvhs.reserve(max_submodels);
  object_cs.reserve(max_submodels);
  inverted_object_cs.reserve(max_submodels);
  triangles.reserve(max_submodels);
  vertices.reserve(max_submodels);
  db_ids.reserve(max_submodels);
  triangle_normals.reserve(max_submodels);
  vertex_colors.reserve(max_submodels);
  uv_coordinates.reserve(max_submodels);
  textures.reserve(max_submodels);
  for (const auto& obj : s.objects)
    {
    if (!obj.bvh.get())
      continue;
    bvhs.push_back(obj.bvh.get());
    object_cs.push_back(obj.cs);
    inverted_object_cs.push_back(invert_orthonormal(obj.cs));
    triangles.push_back(obj.p_triangles->data());
    vertices.push_back(obj.p_vertices->data());
    triangle_normals.push_back(obj.triangle_normals.data());
    vertex_colors.push_back(obj.p_vertex_colors ? obj.p_vertex_colors->data() : nullptr);
    uv_coordinates.push_back(obj.p_uv_coordinates ? obj.p_uv_coordinates->data() : nullptr);
    textures.push_back(obj.p_texture);
    db_ids.push_back(obj.db_id);
    }

  if (bvhs.empty())
    {
    parallel_for(uint32_t(y0), uint32_t(y1 + 1), [&](uint32_t y)
      {
      pixel* p_canvas_line = out.row(y) + x0;
      for (int x = x0; x <= x1; ++x)
        {
        p_canvas_line->db_id = 0;
        p_canvas_line->object_id = (uint32_t)-1;
        p_canvas_line->u = 0.f;
        p_canvas_line->v = 0.f;
        p_canvas_line->depth = std::numeric_limits<float>::max();
        ++p_canvas_line;
        }
      });
    return;
    }

  qbvh_two_level_with_transformations bvh(bvhs.data(), object_cs.data(), (uint32_t)bvhs.size());

#if defined(USE_THREAD_POOL)
  pooled_parallel_for(uint32_t(y0), uint32_t(y1 + 1), [&](uint32_t y)
#else
  parallel_for(uint32_t(y0), uint32_t(y1 + 1), [&](uint32_t y)
#endif
    {
    pixel* p_canvas_line = out.row(y) + x0;
    for (int x = x0; x <= x1; ++x)
      {
      float4 screen_pos((2.f * ((x + 0.5f) / w) - 1.f), (2.f * ((y + 0.5f) / h) - 1.f), _camera.nearClippingPlane, 1.f);
      float4 dir = matrix_vector_multiply(projection_matrix_inv, screen_pos);
      dir[3] = 0.f;
      dir = matrix_vector_multiply(s.coordinate_system, dir);

      ray r;
      r.orig = origin;
      r.dir = dir;
      r.t_near = s.diagonal / 100.f;
      r.t_far = std::numeric_limits<float>::max();


      uint32_t object_id, two_level_index;
      auto hit = bvh.find_closest_triangle(object_id, two_level_index, r, bvhs.data(), inverted_object_cs.data(), triangles.data(), vertices.data());

      if (hit.found)
        {
        float4 n = float4(triangle_normals[two_level_index][object_id][0], triangle_normals[two_level_index][object_id][1], triangle_normals[two_level_index][object_id][2], 0.f);
        n = matrix_vector_multiply(s.coordinate_system_inv, n);
        n = matrix_vector_multiply(object_cs[two_level_index], n);
        r.t_far = hit.distance;
        p_canvas_line->u = n[0];
        p_canvas_line->v = n[1];
        p_canvas_line->depth = hit.distance;
        p_canvas_line->object_id = object_id;
        p_canvas_line->barycentric_u = hit.u;
        p_canvas_line->barycentric_v = hit.v;
        p_canvas_line->db_id = db_ids[two_level_index];
        p_canvas_line->mark = 0;

        if (_settings.textured && uv_coordinates[two_level_index] != nullptr)
          {
          const auto& uvcoords = uv_coordinates[two_level_index][object_id];
          auto coord = (1.f - hit.u - hit.v)*uvcoords[0] + hit.u*uvcoords[1] + hit.v*uvcoords[2];
          coord[0] = std::max(std::min(coord[0], 1.f), 0.f);
          coord[1] = std::max(std::min(coord[1], 1.f), 0.f);
          int x = (int)(coord[0] * (textures[two_level_index]->width() - 1));
          int y = (int)(coord[1] * (textures[two_level_index]->height() - 1));
          uint32_t color = (*textures[two_level_index])(x, y);
          p_canvas_line->r = color & 0x000000ff;
          p_canvas_line->g = (color & 0x0000ff00) >> 8;
          p_canvas_line->b = (color & 0x00ff0000) >> 16;
          p_canvas_line->mark |= 2;
          }
        else if (_settings.vertexcolors && vertex_colors[two_level_index] != nullptr)
          {
          const uint32_t v0 = triangles[two_level_index][object_id][0];
          const uint32_t v1 = triangles[two_level_index][object_id][1];
          const uint32_t v2 = triangles[two_level_index][object_id][2];
          const auto& c0 = vertex_colors[two_level_index][v0];
          const auto& c1 = vertex_colors[two_level_index][v1];
          const auto& c2 = vertex_colors[two_level_index][v2];
          const auto c = c0 * (1.f - hit.u - hit.v) + hit.u*c1 + hit.v*c2;
          p_canvas_line->r = (uint8_t)(c[0] * 255.f);
          p_canvas_line->g = (uint8_t)(c[1] * 255.f);
          p_canvas_line->b = (uint8_t)(c[2] * 255.f);
          p_canvas_line->mark |= 2;
          }       

        if (_settings.shadow)
          {
          const uint32_t v0 = triangles[two_level_index][object_id][0];
          const uint32_t v1 = triangles[two_level_index][object_id][1];
          const uint32_t v2 = triangles[two_level_index][object_id][2];
          float4 V0(vertices[two_level_index][v0][0], vertices[two_level_index][v0][1], vertices[two_level_index][v0][2], 1.f);
          float4 V1(vertices[two_level_index][v1][0], vertices[two_level_index][v1][1], vertices[two_level_index][v1][2], 1.f);
          float4 V2(vertices[two_level_index][v2][0], vertices[two_level_index][v2][1], vertices[two_level_index][v2][2], 1.f);
          V0 = jtk::transform(object_cs[two_level_index], V0);
          V1 = jtk::transform(object_cs[two_level_index], V1);
          V2 = jtk::transform(object_cs[two_level_index], V2);
          const float4 pos = V0 * (1.f - hit.u - hit.v) + hit.u*V1 + hit.v*V2;
          const float4 light_dir = light - pos;

          r.orig = pos;
          r.dir = light_dir;
          r.t_near = 1e-3f;
          r.t_far = std::numeric_limits<float>::max();
          auto hit2 = bvh.find_closest_triangle(object_id, two_level_index, r, bvhs.data(), inverted_object_cs.data(), triangles.data(), vertices.data());
          if (hit2.found)
            p_canvas_line->mark |= 1;
          }
        }
      else
        {
        p_canvas_line->db_id = 0;
        p_canvas_line->object_id = (uint32_t)-1;
        p_canvas_line->u = 0.f;
        p_canvas_line->v = 0.f;
        p_canvas_line->depth = std::numeric_limits<float>::max();
        }
      ++p_canvas_line;
      }
#if defined(USE_THREAD_POOL)
    }, _tp);
#else
    });
#endif
  }

void canvas::render_scene(jtk::image<pixel>& out, const scene* s)
  {
  if (out.width() != _canvas.width() || out.height() != _canvas.height())
    out = jtk::image<pixel>(_canvas.width(), _canvas.height());

  if (s)
    update_canvas(out, 0, 0, width() - 1, height() - 1, *s);
  else
    {
    for (auto& p : out)
      {
      p.db_id = 0;
      p.object_id = (uint32_t)-1;
      }
    }
  }

void canvas::render_scene(const scene* s)
  {
  copy(im, background);

  render_scene(_canvas, s);
  }


void canvas::blit_onto(jtk::image<uint32_t>& screen, int32_t pos_x, int32_t pos_y)
  {
  if (pos_x & 3)
    throw std::runtime_error("blit_onto: can't use SIMD.");

  uint32_t im_x = 0;
  uint32_t im_y = 0;

  if (pos_x < 0)
    {
    im_x = -pos_x;
    pos_x = 0;
    }
  if (pos_y < 0)
    {
    im_y = -pos_y;
    pos_y = 0;
    }

  int32_t im_szx = (int32_t)im.width() - (int32_t)im_x;
  int32_t im_szy = (int32_t)im.height() - (int32_t)im_y;

  if (pos_x + im_szx > (int)screen.width())
    im_szx = (int32_t)screen.width() - pos_x;
  if (pos_y + im_szy > (int)screen.height())
    im_szy = (int32_t)screen.height() - pos_y;

  if (im_szx < 0)
    return;
  if (im_szy < 0)
    return;

  if (im_szx & 3)
    im_szx += 4 - (im_szx & 3);

  const uint32_t sse_sz = im_szx >> 2;

  for (int y = 0; y < im_szy; ++y)
    {
    uint32_t* p_screen = screen.row(screen.height() - 1 - (y + pos_y)) + pos_x;
    uint32_t* p_canvas = im.row(im_y + y) + im_x;

    for (uint32_t x = 0; x < sse_sz; ++x, p_screen += 4, p_canvas += 4)
      {
      __m128 buf = _mm_load_ps((float*)p_canvas);
      _mm_store_ps((float*)p_screen, buf);
      }

    }
  }

void canvas::render_pointclouds_on_image(const scene* s, const jtk::image<pixel>& pix)
  {
  using namespace jtk;

  if (!s->pointclouds.empty())
    {
    if (_zbuffer.width() != pix.width() || _zbuffer.height() != pix.height())
      _zbuffer = jtk::image<float>(pix.width(), pix.height());
    int w = (int)pix.width();
    int h = (int)pix.height();
    for (int y = 0; y < h; ++y)
      {
      float* p_z = _zbuffer.data() + y * _zbuffer.stride();
      const pixel* p_pix = pix.data() + y * pix.stride();
      for (int x = 0; x < w; ++x)
        {
        *p_z = (p_pix->db_id != 0) ? 1.f / p_pix->depth : 0.f;
        ++p_z;
        ++p_pix;
        }
      }
    _fb.h = h;
    _fb.w = w;
    _fb.pixels = im.data(); // todo: check stride an alignment
    _fb.zbuffer = _zbuffer.data();
    bind(_rd, _fb);
    float camera_position[16], object_system[16], projection_mat[16];
    for (int i = 0; i < 16; ++i)
      {
      projection_mat[i] = projection_matrix[i];
      camera_position[i] = s->coordinate_system_inv[i];
      }
    for (const auto& pc : s->pointclouds)
      {
      for (int i = 0; i < 16; ++i)
        {
        object_system[i] = pc.cs[i];
        }
      bind(_rd, camera_position, object_system, projection_mat);
      object_buffer ob;
      ob.number_of_vertices = (uint32_t)pc.p_vertices->size();
      ob.vertices = (const float*)pc.p_vertices->data();
      ob.normals = _settings.shading ? (const float*)pc.p_normals->data() : nullptr;
      ob.colors = _settings.one_bit ? nullptr : (const uint32_t*)pc.p_vertex_colors->data();
      bind(_rd, ob);
      present(_rd, 0xffffffff, [&](uint32_t vertex_id, const __m128i& index, const __m128i& mask)
        {
        if (_mm_extract_epi32(mask, 0) != 0)
          {
          const uint32_t idx = _mm_extract_epi32(index, 0);   
          _canvas[idx].object_id = vertex_id;
          _canvas[idx].depth = 1.f / _fb.zbuffer[idx];
          _canvas[idx].db_id = pc.db_id;
          }
        if (_mm_extract_epi32(mask, 1) != 0)
          {
          const uint32_t idx = _mm_extract_epi32(index, 1);
          _canvas[idx].object_id = vertex_id+1;
          _canvas[idx].depth = 1.f / _fb.zbuffer[idx];
          _canvas[idx].db_id = pc.db_id;
          }
        if (_mm_extract_epi32(mask, 2) != 0)
          {
          const uint32_t idx = _mm_extract_epi32(index, 2);
          _canvas[idx].object_id = vertex_id+2;
          _canvas[idx].depth = 1.f / _fb.zbuffer[idx];
          _canvas[idx].db_id = pc.db_id;
          }
        if (_mm_extract_epi32(mask, 3) != 0)
          {
          const uint32_t idx = _mm_extract_epi32(index, 3);
          _canvas[idx].object_id = vertex_id+3;
          _canvas[idx].depth = 1.f / _fb.zbuffer[idx];
          _canvas[idx].db_id = pc.db_id;
          }
        });
      }
    }
  }
