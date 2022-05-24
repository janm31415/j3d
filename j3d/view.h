#pragma once

#include <SDL.h>
#include <SDL_opengl.h>

#include "canvas.h"
#include <jtk/image.h>
#include "matcap.h"
#include "db.h"
#include "scene.h"
#include "mouse.h"
#include "settings.h"
#include "keyboard.h"

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

    bool file_has_known_extension(const char* filename);

    bool file_has_known_mesh_extension(const char* filename);

    bool file_has_known_pc_extension(const char* filename);

    int64_t load_file(const char* filename);

    void save_file(const char* filename);

    void save_mesh_to_file(int64_t id, const char* filename);

    void save_pc_to_file(int64_t id, const char* filename);

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

    void update_current_folder(const std::string& folder);

    void load_next_file_in_folder();
    void load_previous_file_in_folder();

  private:

    void imgui_ui();    

    void poll_for_events();

    void blit_screen_to_opengl_texture();

    void resize_canvas(uint32_t canvas_w, uint32_t canvas_h);

    bool mouse_in_canvas() const;

    void do_canvas_mouse();

    void process_keys();

    void delete_window();

    void render_mouse();

    void render_scene();

    void clear_scene();

    void _load_next_file_in_folder(int32_t step_size);

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
    settings _settings;
    int32_t _canvas_pos_x, _canvas_pos_y;

    jtk::image<pixel> _pixels;
    matcap _matcap;
    keyboard_handler _key;

    std::mutex _mut;

    bool _openFileDialog = false;
    bool _openMatCapFileDialog = false;
    bool _saveFileDialog = false;
  };
