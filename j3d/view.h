#pragma once

#include <SDL.h>
#include <SDL_opengl.h>

#include "canvas.h"
#include <jtk/image.h>
#include "matcap.h"
#include "db.h"
#include "scene.h"
#include "mouse.h"

#include <jtk/qbvh.h>

#include <mutex>
#include <memory>

class ear_detector;
class face_detector;

class view
  {
  public:
    view();
    ~view();

    int64_t load_mesh_from_file(const char* filename);    

    int64_t load_pc_from_file(const char* filename);  

    jtk::vec3<float> get_world_position(int x, int y);

    uint32_t get_index(int x, int y);

    uint32_t get_id(int x, int y);
    
    void force_redraw();

    void unzoom();    

    void prepare_window();

    void loop();

    void quit();

  private:

    uint32_t _get_semirandom_matcap_id(uint32_t object_id) const;

    void poll_for_events();

    void blit_screen_to_opengl_texture();

    void resize_canvas(uint32_t canvas_w, uint32_t canvas_h);

    bool mouse_in_canvas() const;

    void do_canvas_mouse();

    void delete_window();

    void render_mouse();

    void render_scene();

  private:

    SDL_Window* _window;
    uint32_t _w, _h;
    uint32_t _w_max, _h_max;
    mouse_data _m;
    bool _quit;
    bool _refresh;
    bool _suspend;
    bool _resume;
    scene _scene;
    db _db;

    GLuint _gl_texture;
    uint32_t _gl_texture_w;
    uint32_t _gl_texture_h;

    jtk::image<uint32_t> _screen;
    canvas _canvas;
    canvas::canvas_settings _settings;
    int32_t _canvas_pos_x, _canvas_pos_y;

    jtk::image<pixel> _pixels;
    matcapmap _matcap;

    std::mutex _mut;
  };
