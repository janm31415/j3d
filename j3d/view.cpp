#include "view.h"
#include "mesh.h"
#include "pc.h"
#include "view.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"
#include "imguifilesystem.h"

#include "jtk/file_utils.h"

#include <iostream>

#include <SDL_syswm.h>

#include "jtk/geometry.h"
#include "jtk/timer.h"

#include "stb/stb_image_write.h"

#include <sstream>

using namespace jtk;

namespace
  {

  uint32_t get_next_power_of_two(uint32_t v)
    {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
    }

  void gl_check_error(const char* txt)
    {
    unsigned int err = glGetError();
    if (err)
      {
      std::stringstream str;
      str << "GL error " << err << ": " << txt;
      throw std::runtime_error(str.str());
      }
    }

  void clear_screen(image<uint32_t>& screen, const uint32_t clr)
    {
    for (auto& v : screen)
      v = clr;
    }

  }

view::view() : _w(1600), _h(900), _window(nullptr)
  {
  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);
  _w_max = DM.w;
  _h_max = DM.h;

  if (_w > _w_max)
    _w = _w_max;
  if (_h > _h_max)
    _h = _h_max;
  // Setup window
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_DisplayMode current;
  SDL_GetCurrentDisplayMode(0, &current);

  _quit = false;
  _refresh = true;

  //prepare_window();

  _screen = image<uint32_t>(_w, _h);
  clear_screen(_screen, _settings._background);

  _m.left_dragging = false;
  _m.right_dragging = false;
  _m.right_button_down = false;
  _m.left_button_down = false;
  _m.wheel_down = false;
  _m.wheel_mouse_pressed = false;
  _m.ctrl_pressed = false;
  _m.mouse_x = 0.f;
  _m.mouse_y = 0.f;
  _m.prev_mouse_x = 0.f;
  _m.prev_mouse_y = 0.f;
  _m.wheel_rotation = 0.f;

  std::string settings_path = get_settings_path();
  _settings = read_settings(settings_path.c_str());

  make_matcap(_matcap, _settings._matcap_type, _settings._matcap_file.c_str());

  _canvas.resize(_settings._canvas_w, _settings._canvas_h);
  _canvas.set_background_color(_settings._gradient_top, _settings._gradient_bottom);
  _canvas_pos_x = ((int32_t)_w - (int32_t)_canvas.width()) / 2;
  if (_canvas_pos_x & 3)
    _canvas_pos_x += 4 - (_canvas_pos_x & 3);
  _canvas_pos_y = ((int32_t)_h - (int32_t)_canvas.height()) / 2;

  _suspend = false;
  _resume = false;
  }

view::~view()
  {
  std::string settings_path = get_settings_path();
  write_settings(_settings, settings_path.c_str());
  delete_window();
  }

void view::delete_window()
  {
  if (!_window)
    return;
  glDeleteTextures(1, &_gl_texture);
  // Cleanup
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyWindow(_window);
  _window = nullptr;
  }

void view::prepare_window()
  {
  if (_window)
    return;
  _window = SDL_CreateWindow("j3d", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _w, _h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (_window == NULL)
    {
    std::cout << SDL_GetError() << "\n";
    return;
    }
  SDL_GLContext gl_context = SDL_GL_CreateContext(_window);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;
  //ImFontAtlas* p_atlas = ImGui::GetIO().Fonts;
  //p_atlas->AddFontFromFileTTF("C:/_Dev/jedi/jedi/jedi/fonts/NotoMono-Regular.ttf", 17);

  // Setup Platform/Renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(_window, gl_context);
  ImGui_ImplOpenGL2_Init();

  // Setup Style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];


  SDL_GL_MakeCurrent(_window, gl_context);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &_gl_texture);
  glBindTexture(GL_TEXTURE_2D, _gl_texture);

  _gl_texture_w = get_next_power_of_two(_w_max);
  _gl_texture_h = get_next_power_of_two(_h_max);

  GLint max_texture_size;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

  if (_gl_texture_w > (uint32_t)max_texture_size)
    _gl_texture_w = (uint32_t)max_texture_size;
  if (_gl_texture_h > (uint32_t)max_texture_size)
    _gl_texture_h = (uint32_t)max_texture_size;

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _gl_texture_w, _gl_texture_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  gl_check_error("glTexImage2D in view.cpp");
  }

void view::update_current_folder(const std::string& folder)
  {
  ::update_current_folder(_settings, folder.c_str());
  }

int64_t view::load_mesh_from_file(const char* filename)
  {
  std::scoped_lock lock(_mut);
  mesh m;
  std::string f(filename);
  jtk::timer t;
  t.start();
  bool res = read_from_file(m, f);
  if (!res)
    return -1;
  double load_time_in_s = t.time_elapsed();
  mesh* db_mesh;
  uint32_t id;
  _db.create_mesh(db_mesh, id);
  db_mesh->load_time_in_s = load_time_in_s;
  db_mesh->vertices.swap(m.vertices);
  db_mesh->triangles.swap(m.triangles);
  db_mesh->uv_coordinates.swap(m.uv_coordinates);
  db_mesh->texture.swap(m.texture);
  db_mesh->vertex_colors.swap(m.vertex_colors);
  db_mesh->cs = m.cs;
  db_mesh->visible = m.visible;
  if (db_mesh->visible)
    {
    t.start();
    add_object(id, _scene, _db);
    db_mesh->acceleration_structure_construction_time_in_s = t.time_elapsed();
    }
  prepare_scene(_scene);
  ::unzoom(_scene);
  _refresh = true;
  return (int64_t)id;
  }

int64_t view::load_pc_from_file(const char* filename)
  {
  std::scoped_lock lock(_mut);
  pc point_cloud;
  std::string f(filename);
  jtk::timer t;
  t.start();
  bool res = read_from_file(point_cloud, f);
  if (!res)
    return -1;
  double load_time_in_s = t.time_elapsed();
  pc* db_pc;
  uint32_t id;
  _db.create_pc(db_pc, id);
  db_pc->vertices.swap(point_cloud.vertices);
  db_pc->load_time_in_s = load_time_in_s;
  db_pc->normals.swap(point_cloud.normals);
  db_pc->vertex_colors.swap(point_cloud.vertex_colors);
  db_pc->cs = point_cloud.cs;
  db_pc->visible = point_cloud.visible;
  if (db_pc->visible)
    add_object(id, _scene, _db);
  prepare_scene(_scene);
  ::unzoom(_scene);
  _refresh = true;
  return (int64_t)id;
  }

bool view::file_has_known_mesh_extension(const char* filename)
  {
  std::string ext = jtk::get_extension(std::string(filename));
  if (ext.empty())
    return false;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](char ch) {return (char)::tolower(ch); });

  static std::vector<std::pair<std::string, mesh_filetype>> valid_mesh_extensions = get_valid_mesh_extensions();
  for (const auto& valid_ext : valid_mesh_extensions)
    {
    if (valid_ext.first == ext)
      return true;
    }

  return false;
  }

bool view::file_has_known_pc_extension(const char* filename)
  {
  std::string ext = jtk::get_extension(std::string(filename));
  if (ext.empty())
    return false;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](char ch) {return (char)::tolower(ch); });

  static std::vector<std::pair<std::string, pc_filetype>> valid_pc_extensions = get_valid_pc_extensions();
  for (const auto& valid_ext : valid_pc_extensions)
    {
    if (valid_ext.first == ext)
      return true;
    }
  return false;
  }

bool view::file_has_known_extension(const char* filename)
  {
  return file_has_known_mesh_extension(filename) || file_has_known_pc_extension(filename);
  }

int64_t view::load_file(const char* filename)
  {
  int64_t id = load_mesh_from_file(filename);

  if (id < 0)
    id = load_pc_from_file(filename);
  else
    {
    if (get_triangles(_db, (uint32_t)id)->empty()) // mesh without triangles? -> point cloud
      {
          {
          std::scoped_lock lock(_mut);
          remove_object((uint32_t)id, _scene);
          _db.delete_object_hard((uint32_t)id);
          }
          id = load_pc_from_file(filename);
      }
    }
  if (id >= 0)
    {
    ::update_current_folder(_settings, filename);
    std::string window_title = "j3d - " + std::string(filename);
    SDL_SetWindowTitle(this->_window, window_title.c_str());
    }
  return id;
  }

void view::save_mesh_to_file(int64_t id, const char* filename)
  {
  mesh* m = _db.get_mesh((uint32_t)id);
  if (m && file_has_known_mesh_extension(filename))
    {
    std::string fn(filename);
    if (write_to_file(*m, fn, _settings))
      {
      ::update_current_folder(_settings, filename);
      //std::string window_title = "j3d - " + std::string(filename);
      //SDL_SetWindowTitle(this->_window, window_title.c_str());
      }
    }
  }

void view::save_pc_to_file(int64_t id, const char* filename)
  {
  pc* p = _db.get_pc((uint32_t)id);
  if (p && file_has_known_pc_extension(filename))
    {
    std::string fn(filename);
    if (write_to_file(*p, fn))
      {
      ::update_current_folder(_settings, filename);
      //std::string window_title = "j3d - " + std::string(filename);
      //SDL_SetWindowTitle(this->_window, window_title.c_str());
      }
    }
  }

void view::screenshot(const char* filename)
  {
  std::string fn(filename);
  std::string ext = jtk::get_extension(fn);
  if (ext.empty())
    return;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](char ch) {return (char)::tolower(ch); });

  const jtk::image<uint32_t>& im = _canvas.get_image();
  const uint32_t w = im.width();
  const uint32_t h = im.height();
  uint32_t* raw = new uint32_t[w * h];
  uint32_t* p_raw = raw;
  for (uint32_t y = 0; y < h; ++y)
    {
    const uint32_t* p_im = im.data() + y * im.stride();
    for (uint32_t x = 0; x < w; ++x)
      *p_raw++ = *p_im++;
    }

  if (ext == "png")
    stbi_write_png(filename, w, h, 4, (void*)raw, w * 4);
  else if (ext == "jpg" || ext == "jpeg")
    stbi_write_jpg(filename, w, h, 4, raw, 80);
  else if (ext == "bmp")
    stbi_write_bmp(filename, w, h, 4, raw);
  else if (ext == "tga")
    stbi_write_tga(filename, w, h, 4, raw);

  delete[] raw;
  }

void view::save_file(const char* filename)
  {
  if (!_db.get_meshes().empty())
    save_mesh_to_file(_db.get_meshes().front().first, filename);
  else if (!_db.get_pcs().empty())
    save_pc_to_file(_db.get_pcs().front().first, filename);
  }

void view::clear_scene()
  {
  std::scoped_lock lock(_mut);
  for (const auto& meshes : _db.get_meshes())
    {
    remove_object(meshes.first, _scene);
    }
  for (const auto& pcs : _db.get_pcs())
    {
    remove_object(pcs.first, _scene);
    }
  _db.clear();
  }

void view::render_scene()
  {
  // assumes a lock has been set already
  _canvas.update_settings(_settings._canvas_settings);
  _canvas.render_scene(&_scene);
  _pixels = _canvas.get_pixels();
  _canvas.canvas_to_image(_pixels, _matcap);
  _canvas.render_pointclouds_on_image(&_scene, _pixels);
  _refresh = false;
  }


void view::force_redraw()
  {
  std::scoped_lock lock(_mut);
  render_scene();
  }

jtk::vec3<float> view::get_world_position(int x, int y)
  {
  std::scoped_lock lock(_mut);
  jtk::vec3<float> invalid_vertex(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
  if (x < 0 || y < 0 || x >= (int)_pixels.width() || y >= (int)_pixels.height())
    return invalid_vertex;
  const auto& p = _pixels(x, y);
  if (p.db_id == 0)
    return invalid_vertex;
  mesh* m = _db.get_mesh((uint32_t)p.db_id);
  if (m)
    {
    const uint32_t v0 = m->triangles[p.object_id][0];
    const uint32_t v1 = m->triangles[p.object_id][1];
    const uint32_t v2 = m->triangles[p.object_id][2];
    const float4 V0(m->vertices[v0][0], m->vertices[v0][1], m->vertices[v0][2], 1.f);
    const float4 V1(m->vertices[v1][0], m->vertices[v1][1], m->vertices[v1][2], 1.f);
    const float4 V2(m->vertices[v2][0], m->vertices[v2][1], m->vertices[v2][2], 1.f);
    const float4 pos = V0 * (1.f - p.barycentric_u - p.barycentric_v) + p.barycentric_u * V1 + p.barycentric_v * V2;
    auto world_pos = matrix_vector_multiply(m->cs, pos);
    return jtk::vec3<float>(world_pos[0], world_pos[1], world_pos[2]);
    }
  pc* ptcl = _db.get_pc((uint32_t)p.db_id);
  if (ptcl)
    {
    const float4 pos(ptcl->vertices[p.object_id][0], ptcl->vertices[p.object_id][1], ptcl->vertices[p.object_id][2], 1.f);
    auto world_pos = matrix_vector_multiply(m->cs, pos);
    return jtk::vec3<float>(world_pos[0], world_pos[1], world_pos[2]);
    }
  return invalid_vertex;
  }

uint32_t view::get_index(int x, int y)
  {
  std::scoped_lock lock(_mut);
  if (x < 0 || y < 0 || x >= (int)_pixels.width() || y >= (int)_pixels.height())
    return (uint32_t)(-1);
  const auto& p = _pixels(x, y);
  if (p.db_id == 0)
    return (uint32_t)(-1);
  uint32_t closest_v = get_closest_vertex(p, get_vertices(_db, p.db_id), get_triangles(_db, p.db_id));
  return closest_v;
  }

uint32_t view::get_id(int x, int y)
  {
  std::scoped_lock lock(_mut);
  if (x < 0 || y < 0 || x >= (int)_pixels.width() || y >= (int)_pixels.height())
    return (uint32_t)0;
  const auto& p = _pixels(x, y);
  if (p.db_id == 0)
    return (uint32_t)0;
  return p.db_id;
  }

void view::_load_next_file_in_folder(int32_t step_size)
  {
  if (_settings._current_folder_files.empty())
    return;
  while (step_size < 0)
    step_size += (int32_t)_settings._current_folder_files.size();
  int32_t current_index = _settings._index_in_folder;
  _settings._index_in_folder = (_settings._index_in_folder + step_size) % _settings._current_folder_files.size();
  while (current_index != _settings._index_in_folder)
    {
    const std::string& file_to_check = _settings._current_folder_files[_settings._index_in_folder];
    if (file_has_known_extension(file_to_check.c_str()))
      {
      clear_scene();
      load_file(file_to_check.c_str());
      _refresh = true;
      //if (id >= 0)
      //  {
      //  unzoom();
      //  }
      break;
      }
    _settings._index_in_folder = (_settings._index_in_folder + step_size) % _settings._current_folder_files.size();
    }
  }

void view::load_next_file_in_folder()
  {
  _load_next_file_in_folder(1);
  }

void view::load_previous_file_in_folder()
  {
  _load_next_file_in_folder(-1);
  }

void view::unzoom()
  {
  std::scoped_lock lock(_mut);
  ::unzoom(_scene);
  _refresh = true;
  }

void view::process_keys()
  {
  if (ImGui::GetIO().WantCaptureKeyboard)
    return;
  if (_key.is_pressed(SDLK_ESCAPE))
    {
    _quit = true;
    }
  else if (_key.is_pressed(SDLK_SPACE))
    {
    load_next_file_in_folder();
    }
  else if (_key.is_pressed(SDLK_BACKSPACE))
    {
    load_previous_file_in_folder();
    }
  else if (_key.is_pressed(SDLK_1))
    {
    resize_canvas(512, 512);
    }
  else if (_key.is_pressed(SDLK_2))
    {
    resize_canvas(800, 600);
    }
  else if (_key.is_pressed(SDLK_3))
    {
    resize_canvas(1024, 768);
    }
  else if (_key.is_pressed(SDLK_4))
    {
    resize_canvas(1024, 1024);
    }
  else if (_key.is_pressed(SDLK_5))
    {
    resize_canvas(1280, 800);
    }
  else if (_key.is_pressed(SDLK_6))
    {
    resize_canvas(1440, 900);
    }
  else if (_key.is_pressed(SDLK_7))
    {
    resize_canvas(1600, 900);
    }
  else if (_key.is_pressed(SDLK_F1))
    {
    make_matcap_red_wax(_matcap);
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_F2))
    {
    make_matcap_brown(_matcap);
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_F3))
    {
    make_matcap_gray(_matcap);
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_F4))
    {
    make_matcap_sketch(_matcap);
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_i))
    {
    _showInfo = !_showInfo;
    }
  else if (_key.is_pressed(SDLK_s))
    {
    if (_m.ctrl_pressed)
      {
      _saveFileDialog = true;
      }
    else
      {
      _settings._canvas_settings.shadow = !_settings._canvas_settings.shadow;
      _refresh = true;
      }
    }
  else if (_key.is_pressed(SDLK_t))
    {
    _settings._canvas_settings.textured = !_settings._canvas_settings.textured;
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_l))
    {
    _settings._canvas_settings.shading = !_settings._canvas_settings.shading;
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_e))
    {
    if (_m.ctrl_pressed)
      {
      _screenshotDialog = true;
      }
    else
      {
      _settings._canvas_settings.edges = !_settings._canvas_settings.edges;
      _refresh = true;
      }
    }
  else if (_key.is_pressed(SDLK_o))
    {
    if (_m.ctrl_pressed)
      {
      _openFileDialog = true;
      }
    }
  else if (_key.is_pressed(SDLK_m))
    {
    if (_m.ctrl_pressed)
      {
      _openMatCapFileDialog = true;
      }
    }
  else if (_key.is_pressed(SDLK_b))
    {
    _settings._canvas_settings.one_bit = !_settings._canvas_settings.one_bit;
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_u))
    {
    ::unzoom(_scene);
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_v))
    {
    _settings._canvas_settings.vertexcolors = !_settings._canvas_settings.vertexcolors;
    _refresh = true;
    }
  else if (_key.is_pressed(SDLK_w))
    {
    _settings._canvas_settings.wireframe = !_settings._canvas_settings.wireframe;
    _refresh = true;
    }
  }

void view::poll_for_events()
  {
  _m.right_button_down = false;
  _m.left_button_down = false;
  _m.wheel_down = false;
  _key.initialise_for_new_events();
  SDL_Event event;
  while (SDL_PollEvent(&event))
    {
    ImGui_ImplSDL2_ProcessEvent(&event);
    _key.handle_event(event);
    if (event.type == SDL_QUIT)
      {
      SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
      quit();
      }
    if (event.type == SDL_MOUSEMOTION)
      {
      _m.prev_mouse_x = _m.mouse_x;
      _m.prev_mouse_y = _m.mouse_y;
      _m.mouse_x = float(event.motion.x);
      _m.mouse_y = float(event.motion.y);
      }
    if (event.type == SDL_MOUSEBUTTONDOWN)
      {
      if (event.button.button == 2)
        {
        _m.wheel_mouse_pressed = true;
        _m.wheel_down = true;
        }
      else if (event.button.button == 1)
        {
        _m.left_dragging = true;
        _m.left_button_down = true;
        }
      else if (event.button.button == 3)
        {
        _m.right_dragging = true;
        _m.right_button_down = true;
        }
      }
    if (event.type == SDL_MOUSEBUTTONUP)
      {
      if (event.button.button == 2)
        _m.wheel_mouse_pressed = false;
      else if (event.button.button == 1)
        {
        _m.left_dragging = false;
        }
      else if (event.button.button == 3)
        {
        _m.right_dragging = false;
        }
      }
    if (event.type == SDL_MOUSEWHEEL)
      {
      _m.wheel_rotation += event.wheel.y;
      }
    if (event.type == SDL_KEYDOWN)
      {
      switch (event.key.keysym.sym)
        {
        case SDLK_LCTRL:
        case SDLK_RCTRL:
        {
        _m.ctrl_pressed = true;
        break;
        }
        }
      }
    if (event.type == SDL_KEYUP)
      {
      switch (event.key.keysym.sym)
        {
        case SDLK_LCTRL:
        case SDLK_RCTRL:
        {
        _m.ctrl_pressed = false;
        break;
        }
        }
      }
    if (event.type == SDL_WINDOWEVENT)
      {
      if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
        int new_w, new_h;
        SDL_GetWindowSize(_window, &new_w, &new_h);
        _w = (uint32_t)new_w < _w_max ? (uint32_t)new_w : _w_max;
        _h = (uint32_t)new_h < _h_max ? (uint32_t)new_h : _h_max;
        _screen = image<uint32_t>(_w, _h);
        clear_screen(_screen, _settings._background);
        _canvas_pos_x = ((int32_t)_w - (int32_t)_canvas.width()) / 2;
        if (_canvas_pos_x & 3)
          _canvas_pos_x += 4 - (_canvas_pos_x & 3);
        _canvas_pos_y = ((int32_t)_h - (int32_t)_canvas.height()) / 2;
        _refresh = true;
        }
      }
    if (event.type == SDL_DROPFILE)
      {
      auto dropped_filedir = event.drop.file;
      std::string path(dropped_filedir);
      SDL_free(dropped_filedir);    // Free dropped_filedir memory
      clear_scene();
      load_file(path.c_str());
      }
    }
  }

void view::blit_screen_to_opengl_texture()
  {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, _w, 0, _h, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glViewport(0, 0, _w, _h);
  glClear(GL_COLOR_BUFFER_BIT);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, _gl_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


  const uint32_t step_block_x = _w / _gl_texture_w + 1;
  const uint32_t step_block_y = _h / _gl_texture_h + 1;

  for (uint32_t block_y = 0; block_y < step_block_y; ++block_y)
    {
    for (uint32_t block_x = 0; block_x < step_block_x; ++block_x)
      {
      const uint32_t x_offset = block_x * _gl_texture_w;
      const uint32_t y_offset = block_y * _gl_texture_h;
      uint32_t wi = std::min<uint32_t>(_gl_texture_w, _w - x_offset);
      uint32_t he = std::min<uint32_t>(_gl_texture_h, _h - y_offset);

      uint32_t* p_im_start = (uint32_t*)(_screen.row(y_offset) + x_offset);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, _screen.stride());
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, wi, he, GL_RGBA, GL_UNSIGNED_BYTE, (void*)p_im_start);
      gl_check_error("glTexSubImage2D in view.cpp");
      const float tex_w = float(wi) / float(_gl_texture_w);
      const float tex_h = float(he) / float(_gl_texture_h);

      const float x0 = float(x_offset);
      const float y0 = float(y_offset);

      const float x1 = x0 + float(wi);
      const float y1 = y0 + float(he);
      glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(0.0, 0.0);
      glVertex2f(x0, y0);
      glTexCoord2f(tex_w, 0.0);
      glVertex2f(x1, y0);
      glTexCoord2f(0.0, tex_h);
      glVertex2f(x0, y1);
      glTexCoord2f(tex_w, tex_h);
      glVertex2f(x1, y1);
      glEnd();
      gl_check_error("GL_TRIANGLE_STRIP in view.cpp");
      }
    }
  }

void view::info()
  {
  mesh* m = nullptr;
  if (!_db.get_meshes().empty())
    m = _db.get_meshes().front().second;
  pc* p = nullptr;
  if (!_db.get_pcs().empty())
    p = _db.get_pcs().front().second;

  if (!m && !p)
    return;

  ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(14, 50), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Info", &_showInfo))
    {
    ImGui::End();
    return;
    }
  uint32_t nr_of_vertices = (uint32_t)(m ? m->vertices.size() : p->vertices.size());
  ImGui::InputScalar("#vertices", ImGuiDataType_U32, &nr_of_vertices, 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
  uint32_t nr_of_triangles = (uint32_t)(m ? m->triangles.size() : 0);
  ImGui::InputScalar("#triangles", ImGuiDataType_U32, &nr_of_triangles, 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
  double fps = 1.0 / _last_render_time_in_seconds;
  ImGui::InputScalar("fps", ImGuiDataType_Double, &fps, 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
  float minbb[3] = { _scene.min_bb[0], _scene.min_bb[1], _scene.min_bb[2] };
  ImGui::InputFloat3("min", minbb, "%.3f", ImGuiInputTextFlags_ReadOnly);
  float maxbb[3] = { _scene.max_bb[0], _scene.max_bb[1], _scene.max_bb[2] };
  ImGui::InputFloat3("max", maxbb, "%.3f", ImGuiInputTextFlags_ReadOnly);
  float sizebb[3] = { maxbb[0] - minbb[0], maxbb[1] - minbb[1], maxbb[2] - minbb[2] };
  ImGui::InputFloat3("size", sizebb, "%.3f", ImGuiInputTextFlags_ReadOnly);
  float lt = (float)((m ? m->load_time_in_s : p->load_time_in_s));
  ImGui::InputFloat("file load time (s)", &lt, 0.f, 0.f, "%.6f", ImGuiInputTextFlags_ReadOnly);
  if (m)
    {
    float accelt = m->acceleration_structure_construction_time_in_s;
    ImGui::InputFloat("bvh construction (s)", &accelt, 0.f, 0.f, "%.6f", ImGuiInputTextFlags_ReadOnly);
    }
  ImGui::End();
  }

void view::resize_canvas(uint32_t canvas_w, uint32_t canvas_h)
  {
  _settings._canvas_w = canvas_w;
  _settings._canvas_h = canvas_h;
  _refresh = true;
  //if (canvas_w > _w)
  //  canvas_w = _w;
  //if (canvas_h > _h)
  //  canvas_h = _h;
  _canvas.resize(canvas_w, canvas_h);
  _canvas.set_background_color(_settings._gradient_top, _settings._gradient_bottom);
  _canvas_pos_x = ((int32_t)_w - (int32_t)_canvas.width()) / 2;
  if (_canvas_pos_x & 3)
    _canvas_pos_x += 4 - (_canvas_pos_x & 3);
  _canvas_pos_y = ((int32_t)_h - (int32_t)_canvas.height()) / 2;
  }

bool view::mouse_in_canvas() const
  {
  return ((_m.mouse_x >= (float)_canvas_pos_x) && (_m.mouse_y >= (float)_canvas_pos_y) &&
    (_m.mouse_x < (float)(_canvas_pos_x + _canvas.width())) && (_m.mouse_y < (float)(_canvas_pos_y + _canvas.height())));
  }

void view::do_canvas_mouse()
  {
  if (ImGui::GetIO().WantCaptureMouse)
    return;
  if (!mouse_in_canvas())
    {
    if (_m.left_button_down || _m.right_button_down || _m.wheel_down)
      return;
    }
  _canvas.do_mouse(_refresh, _m, _scene, (float)_canvas_pos_x, (float)_canvas_pos_y);
  }

namespace
  {
  void _draw_square(int x, int y, int size, jtk::image<uint32_t>& im, uint32_t clr)
    {
    int hs = (size - 1) / 2;
    if (x >= hs && y >= hs && x < ((int)im.width() - hs) && y < ((int)im.height() - hs))
      {
      for (int a = x - hs; a <= x + hs; ++a)
        {
        im(a, y - hs) = clr;
        im(a, y + hs) = clr;
        }
      for (int a = y - hs; a <= y + hs; ++a)
        {
        im(x - hs, a) = clr;
        im(x + hs, a) = clr;
        }
      }
    }
  }

void view::render_mouse()
  {
  if (!mouse_in_canvas())
    return;
  pixel p_actual;
  const float pos_x = _m.mouse_x;
  const float pos_y = _m.mouse_y;
  _canvas.get_pixel(p_actual, _m, (float)_canvas_pos_x, (float)_canvas_pos_y);
  const uint32_t clr = 0xff0000d0;
  //if (p_actual.object_id != (uint32_t)(-1))
  if (p_actual.db_id)
    {
    uint32_t closest_v = get_closest_vertex(p_actual, get_vertices(_db, p_actual.db_id), get_triangles(_db, p_actual.db_id));
    auto V = (*get_vertices(_db, p_actual.db_id))[closest_v];
    jtk::float4 V4(V[0], V[1], V[2], 1.f);
    V4 = jtk::matrix_vector_multiply(*get_cs(_db, p_actual.db_id), V4);
    V4 = jtk::matrix_vector_multiply(_scene.coordinate_system_inv, V4);
    V4 = jtk::matrix_vector_multiply(_canvas.get_projection_matrix(), V4);
    int x = (int)(((V4[0] / V4[3] + 1.f) / 2.f) * _canvas.width() - 0.5f);
    int y = (int)(((V4[1] / V4[3] + 1.f) / 2.f) * _canvas.height() - 0.5f);
    _draw_square(x + _canvas_pos_x, _screen.height() - 1 - (y + _canvas_pos_y), 3, _screen, clr);
    _draw_square(x + _canvas_pos_x, _screen.height() - 1 - (y + _canvas_pos_y), 5, _screen, 0xff000000);
    }
  }

void view::imgui_ui()
  {
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplSDL2_NewFrame(_window);
  ImGui::NewFrame();

  ImGuiWindowFlags window_flags = 0;
  window_flags |= ImGuiWindowFlags_NoTitleBar;
  window_flags |= ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoCollapse;
  window_flags |= ImGuiWindowFlags_MenuBar;
  window_flags |= ImGuiWindowFlags_NoBackground;
  window_flags |= ImGuiWindowFlags_NoResize;
  window_flags |= ImGuiWindowFlags_NoScrollbar;
  bool open = true;
  bool canvas_size_512x512 = (_settings._canvas_w == 512 && _settings._canvas_h == 512);
  bool canvas_size_800x600 = (_settings._canvas_w == 800 && _settings._canvas_h == 600);
  bool canvas_size_1024x768 = (_settings._canvas_w == 1024 && _settings._canvas_h == 768);
  bool canvas_size_1024x1024 = (_settings._canvas_w == 1024 && _settings._canvas_h == 1024);
  bool canvas_size_1280x800 = (_settings._canvas_w == 1280 && _settings._canvas_h == 800);
  bool canvas_size_1440x900 = (_settings._canvas_w == 1440 && _settings._canvas_h == 900);
  bool canvas_size_1600x900 = (_settings._canvas_w == 1600 && _settings._canvas_h == 900);

  bool redwax = _settings._matcap_type == matcap_type::MATCAP_TYPE_INTERNAL_REDWAX;
  bool gray = _settings._matcap_type == matcap_type::MATCAP_TYPE_INTERNAL_GRAY;
  bool brown = _settings._matcap_type == matcap_type::MATCAP_TYPE_INTERNAL_BROWN;
  bool sketch = _settings._matcap_type == matcap_type::MATCAP_TYPE_INTERNAL_SKETCH;
  bool matcap_external = _settings._matcap_type == matcap_type::MATCAP_TYPE_EXTERNAL_FILE;

  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2((float)_w, 10), ImGuiCond_Always);

  if (ImGui::Begin("j3d", &open, window_flags))
    {
    if (!open)
      _quit = true;
    if (ImGui::BeginMenuBar())
      {
      if (ImGui::BeginMenu("File", ""))
        {
        if (ImGui::MenuItem("Open", "ctrl+o"))
          {
          _openFileDialog = true;
          }
        if (ImGui::MenuItem("Next file in folder", "space"))
          {
          load_next_file_in_folder();
          }
        if (ImGui::MenuItem("Previous file in folder", "backspace"))
          {
          load_previous_file_in_folder();
          }
        ImGui::Separator();
        if (ImGui::MenuItem("Info", "i"))
          {
          _showInfo = true;
          }
        ImGui::Separator();
        if (ImGui::MenuItem("Save as", "ctrl+s"))
          {
          _saveFileDialog = true;
          }
        ImGui::Separator();
        if (ImGui::MenuItem("Screenshot", "ctrl+e"))
          {
          _screenshotDialog = true;
          }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Esc"))
          {
          _quit = true;
          }
        ImGui::EndMenu();
        }
      if (ImGui::BeginMenu("Canvas"))
        {
        if (ImGui::MenuItem("512x512", "1", &canvas_size_512x512))
          resize_canvas(512, 512);
        if (ImGui::MenuItem("800x600", "2", &canvas_size_800x600))
          resize_canvas(800, 600);
        if (ImGui::MenuItem("1024x768", "3", &canvas_size_1024x768))
          resize_canvas(1024, 768);
        if (ImGui::MenuItem("1024x1024", "4", &canvas_size_1024x1024))
          resize_canvas(1024, 1024);
        if (ImGui::MenuItem("1280x800", "5", &canvas_size_1280x800))
          resize_canvas(1280, 800);
        if (ImGui::MenuItem("1440x900", "6", &canvas_size_1440x900))
          resize_canvas(1440, 900);
        if (ImGui::MenuItem("1600x900", "7", &canvas_size_1600x900))
          resize_canvas(1600, 900);
        ImGui::Separator();
        if (ImGui::MenuItem("One bit", "b", &_settings._canvas_settings.one_bit))
          _refresh = true;
        if (ImGui::MenuItem("Light shading", "l", &_settings._canvas_settings.shading))
          _refresh = true;
        if (ImGui::MenuItem("Texture", "t", &_settings._canvas_settings.textured))
          _refresh = true;
        if (ImGui::MenuItem("Wireframe", "w", &_settings._canvas_settings.wireframe))
          _refresh = true;
        if (ImGui::MenuItem("Shadow", "s", &_settings._canvas_settings.shadow))
          _refresh = true;
        if (ImGui::MenuItem("Edges", "e", &_settings._canvas_settings.edges))
          _refresh = true;
        if (ImGui::MenuItem("Vertex colors", "v", &_settings._canvas_settings.vertexcolors))
          _refresh = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Unzoom", "u"))
          {
          unzoom();
          }
        ImGui::EndMenu();
        }
      if (ImGui::BeginMenu("MatCap"))
        {
        if (ImGui::MenuItem("Red wax", "F1", &redwax))
          {
          make_matcap(_matcap, matcap_type::MATCAP_TYPE_INTERNAL_REDWAX, nullptr);
          _settings._matcap_type = _matcap.type;
          _refresh = true;
          }
        if (ImGui::MenuItem("Brown", "F2", &brown))
          {
          make_matcap(_matcap, matcap_type::MATCAP_TYPE_INTERNAL_BROWN, nullptr);
          _settings._matcap_type = _matcap.type;
          _refresh = true;
          }
        if (ImGui::MenuItem("Gray", "F3", &gray))
          {
          make_matcap(_matcap, matcap_type::MATCAP_TYPE_INTERNAL_GRAY, nullptr);
          _settings._matcap_type = _matcap.type;
          _refresh = true;
          }
        if (ImGui::MenuItem("Sketch", "F4", &sketch))
          {
          make_matcap(_matcap, matcap_type::MATCAP_TYPE_INTERNAL_SKETCH, nullptr);
          _settings._matcap_type = _matcap.type;
          _refresh = true;
          }
        if (ImGui::MenuItem("From file", "ctrl+m", &matcap_external))
          {
          _openMatCapFileDialog = true;
          }
        ImGui::EndMenu();
        }
      if (ImGui::BeginMenu("Colors"))
        {
        float gradient_top_red = (_settings._gradient_top & 255) / 255.f;
        float gradient_top_green = ((_settings._gradient_top >> 8) & 255) / 255.f;
        float gradient_top_blue = ((_settings._gradient_top >> 16) & 255) / 255.f;
        float gradient_bottom_red = (_settings._gradient_bottom & 255) / 255.f;
        float gradient_bottom_green = ((_settings._gradient_bottom >> 8) & 255) / 255.f;
        float gradient_bottom_blue = ((_settings._gradient_bottom >> 16) & 255) / 255.f;
        float bg_red = (_settings._background & 255) / 255.f;
        float bg_green = ((_settings._background >> 8) & 255) / 255.f;
        float bg_blue = ((_settings._background >> 16) & 255) / 255.f;
        float coltop[3] = { gradient_top_red, gradient_top_green, gradient_top_blue };
        float colbottom[3] = { gradient_bottom_red, gradient_bottom_green, gradient_bottom_blue };
        float colback[3] = { bg_red, bg_green, bg_blue };
        if (ImGui::ColorEdit3("Gradient top", coltop))
          {
          uint32_t r = (uint32_t)(coltop[0] * 255.f);
          uint32_t g = (uint32_t)(coltop[1] * 255.f);
          uint32_t b = (uint32_t)(coltop[2] * 255.f);
          uint32_t clr = 0xff000000 | (b << 16) | (g << 8) | r;
          _settings._gradient_top = clr;
          _canvas.set_background_color(_settings._gradient_top, _settings._gradient_bottom);
          _refresh = true;
          }
        if (ImGui::ColorEdit3("Gradient bottom", colbottom))
          {
          uint32_t r = (uint32_t)(colbottom[0] * 255.f);
          uint32_t g = (uint32_t)(colbottom[1] * 255.f);
          uint32_t b = (uint32_t)(colbottom[2] * 255.f);
          uint32_t clr = 0xff000000 | (b << 16) | (g << 8) | r;
          _settings._gradient_bottom = clr;
          _canvas.set_background_color(_settings._gradient_top, _settings._gradient_bottom);
          _refresh = true;
          }
        if (ImGui::ColorEdit3("Background", colback))
          {
          uint32_t r = (uint32_t)(colback[0] * 255.f);
          uint32_t g = (uint32_t)(colback[1] * 255.f);
          uint32_t b = (uint32_t)(colback[2] * 255.f);
          uint32_t clr = 0xff000000 | (b << 16) | (g << 8) | r;
          _settings._background = clr;
          _refresh = true;
          }
        if (ImGui::Button("Restore colors"))
          {
          _settings._gradient_top = 0xff000000;
          _settings._gradient_bottom = 0xff404040;
          _settings._background = 0xff000000 | (uint32_t(49) << 16) | (uint32_t(49) << 8) | uint32_t(49);
          _canvas.set_background_color(_settings._gradient_top, _settings._gradient_bottom);
          _refresh = true;
          }
        ImGui::EndMenu();
        }
      if (ImGui::BeginMenu("Vox"))
        {
        int sz = (int)_settings._vox_max_size;
        if (ImGui::SliderInt("max dimension size", &sz, 1, 1000))
          {
          _settings._vox_max_size = (uint32_t)sz;
          }
        ImGui::EndMenu();
        }
      ImGui::EndMenuBar();
      }
    ImGui::End();
    }

  static ImGuiFs::Dialog open_file_dlg(false, true, false);
  const char* openFileChosenPath = open_file_dlg.chooseFileDialog(_openFileDialog, _settings._current_folder.c_str(), ".ply;.stl;.obj;.trc;.xyz;.pts;.gltf;.glb;.vox;.off", "Open file", ImVec2(-1, -1), ImVec2(50, 50));
  _openFileDialog = false;
  if (strlen(openFileChosenPath) > 0)
    {
    clear_scene();
    load_file(open_file_dlg.getChosenPath());
    }

  static ImGuiFs::Dialog open_matcap_dlg(false, true, false);
  const char* openMatCapChosenPath = open_matcap_dlg.chooseFileDialog(_openMatCapFileDialog, jtk::get_folder(_settings._matcap_file).c_str(), ".png;.jpg;.jpeg;.bmp;.tga;.ppm", "Open MatCap", ImVec2(-1, -1), ImVec2(50, 50));
  _openMatCapFileDialog = false;
  if (strlen(openMatCapChosenPath) > 0)
    {
    make_matcap_from_file(_matcap, openMatCapChosenPath);
    _settings._matcap_type = _matcap.type;
    _settings._matcap_file = std::string(openMatCapChosenPath);
    _refresh = true;
    }

  static ImGuiFs::Dialog save_file_dlg(false, false, false);
  const char* saveFileChosenPath = save_file_dlg.saveFileDialog(_saveFileDialog, _settings._current_folder.c_str(), nullptr, ".ply;.stl;.obj;.trc;.xyz;.pts;.glb;.gltf;.vox;.off", "Save file as", ImVec2(-1, -1), ImVec2(50, 50));
  _saveFileDialog = false;
  if (strlen(saveFileChosenPath) > 0)
    {
    save_file(saveFileChosenPath);
    }

  static ImGuiFs::Dialog screenshot_file_dlg(false, false, false);
  const char* screenshotFileChosenPath = screenshot_file_dlg.saveFileDialog(_screenshotDialog, _settings._current_folder.c_str(), nullptr, ".png;.bmp;.jpg;.tga", "Screenshot file", ImVec2(-1, -1), ImVec2(50, 50));
  _screenshotDialog = false;
  if (strlen(screenshotFileChosenPath) > 0)
    {
    screenshot(screenshotFileChosenPath);
    }

  if (_showInfo)
    info();

  //ImGui::ShowDemoWindow();
  ImGui::Render();
  }

void view::quit()
  {
  _quit = true;
  }

void view::loop()
  {
  while (!_quit)
    {

    if (_window)
      {
      poll_for_events();
      imgui_ui();
      do_canvas_mouse();
      process_keys();
      }


    if (_refresh)
      {
          {
          std::scoped_lock lock(_mut);
          auto tic = std::chrono::high_resolution_clock::now();
          render_scene();
          auto toc = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double> diff = toc - tic;
          _last_render_time_in_seconds = diff.count();
          }
      }

    clear_screen(_screen, _settings._background);
    _canvas.blit_onto(_screen, _canvas_pos_x, _canvas_pos_y);

    if (_window)
      {
      render_mouse();
      blit_screen_to_opengl_texture();
      ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
      SDL_GL_SwapWindow(_window);
      }
    else
      std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(16.0));

    if (_suspend)
      {
      std::scoped_lock lock(_mut);
      _suspend = false;
      delete_window();
      }

    if (_resume)
      {
      std::scoped_lock lock(_mut);
      _resume = false;
      prepare_window();
      }
    }
  }
