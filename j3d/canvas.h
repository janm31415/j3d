#pragma once

#include "camera.h"
#include <jtk/image.h>
#include <jtk/render.h>
#include "pixel.h"
#include "scene.h"
#include "mouse.h"
#include "matcap.h"
#include <jtk/concurrency.h>

class canvas
  {
  public:

    struct canvas_settings
      {
      bool one_bit;
      bool shadow;
      bool edges;
      bool wireframe;
      bool shading;
      bool textured;
      bool vertexcolors;
      };

    canvas();

    canvas(uint32_t w, uint32_t h);
    ~canvas();

    void resize(uint32_t w, uint32_t h);
    void get_pixel(pixel& p, const mouse_data& data, float mouse_offset_x, float mouse_offset_y);

    void get_pixel(pixel& p, float pos_x, float pos_y, float mouse_offset_x, float mouse_offset_y);

    void do_mouse(bool& refresh, mouse_data& data, scene& s, float mouse_offset_x, float mouse_offset_y);

    void update_canvas(jtk::image<pixel>& out, int x0, int y0, int x1, int y1, const scene& s);

    void update_canvas(int x0, int y0, int x1, int y1, const scene& s);

    void render_scene(jtk::image<pixel>& out, const scene* s);

    void render_scene(const scene* s);

    void render_pointclouds_on_image(const scene* s, const jtk::image<pixel>& pix);

    void set_background_color(uint8_t r, uint8_t g, uint8_t b);

    void blit_onto(jtk::image<uint32_t>& screen, int32_t pos_x, int32_t pos_y);

    const uint32_t width() const { return im.width(); }
    const uint32_t height() const { return im.height(); }

    void update_settings(const canvas_settings& s)
      {
      _settings = s;
      }

    void canvas_to_image(const jtk::image<pixel>& canvas, const matcapmap& _matcap);
    void canvas_to_image(const matcapmap& _matcap);

    const jtk::float4x4& get_projection_matrix() const { return projection_matrix; }
    const jtk::float4x4& get_inverse_projection_matrix() const { return projection_matrix_inv; }

    const camera& get_camera() const { return _camera; }

    const jtk::image<pixel>& get_pixels() const { return _canvas; }

    const jtk::image<uint32_t>& get_image() const { return im; }

    jtk::image<uint32_t>& get_image() { return im; }

  private:
    float compute_convex_cos_angle(float x1, float y1, float u1, float v1, float depth1, float x2, float y2, float u2, float v2, float depth2);
    
    void _render_wireframe(const jtk::image<pixel>& canvas, const matcapmap& _matcap);

    void _canvas_to_one_bit_image(const jtk::image<pixel>& _combined_canvas, const matcapmap& _matcap);

    uint32_t _get_color(const pixel* p, const matcapmap& _matcap) const;

    uint32_t _get_color(const pixel* p, const matcap& _matcap, float u, float v) const;


  private:
    jtk::render_data _rd;
    jtk::frame_buffer _fb;
    jtk::image<float> _zbuffer;

    jtk::image<uint32_t> im, background;
    jtk::image<jtk::float4> buffer;
    camera _camera;
    jtk::float4x4 projection_matrix, projection_matrix_inv;
    jtk::image<pixel> _canvas;
    jtk::image<float> _u, _v;
    canvas_settings _settings;

    jtk::thread_pool _tp;
    
  };
