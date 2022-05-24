#include "settings.h"
#include "pref_file.h"
#include "jtk/file_utils.h"

settings::settings()
  {
  _canvas_settings.one_bit = false;
  _canvas_settings.shadow = false;
  _canvas_settings.edges = true;
  _canvas_settings.wireframe = false;
  _canvas_settings.shading = true;
  _canvas_settings.textured = true;
  _canvas_settings.vertexcolors = true;
  _canvas_w = 800;
  _canvas_h = 600;
  _executable_path = jtk::get_executable_path();
  _index_in_folder = -1;
  _matcap_type = matcap_type::MATCAP_TYPE_INTERNAL_REDWAX;
  _matcap_file = jtk::get_folder(jtk::get_executable_path())+"matcaps/basic_1.png";
  }


settings read_settings(const char* filename)
  {
  settings s;
  pref_file f(filename, pref_file::READ);
  f["one_bit"] >> s._canvas_settings.one_bit;
  f["shadow"] >> s._canvas_settings.shadow;
  f["edges"] >> s._canvas_settings.edges;
  f["wireframe"] >> s._canvas_settings.wireframe;
  f["shading"] >> s._canvas_settings.shading;
  f["textured"] >> s._canvas_settings.textured;
  f["vertexcolors"] >> s._canvas_settings.vertexcolors;
  f["current_folder"] >> s._current_folder;
  f["canvas_w"] >> s._canvas_w;
  f["canvas_h"] >> s._canvas_h;
  int32_t i;
  f["matcap_type"] >> i;
  s._matcap_type = int_to_matcap_type(i);
  f["matcap_file"] >> s._matcap_file;

  s._current_folder_files = jtk::get_files_from_directory(s._current_folder, false);

  return s;
  }

void write_settings(const settings& s, const char* filename)
  {
  pref_file f(filename, pref_file::WRITE);
  f << "one_bit" << s._canvas_settings.one_bit;
  f << "shadow" << s._canvas_settings.shadow;
  f << "edges" << s._canvas_settings.edges;
  f << "wireframe" << s._canvas_settings.wireframe;
  f << "shading" << s._canvas_settings.shading;
  f << "textured" << s._canvas_settings.textured;
  f << "vertexcolors" << s._canvas_settings.vertexcolors;
  f << "current_folder" << s._current_folder;
  f << "canvas_w" << s._canvas_w;
  f << "canvas_h" << s._canvas_h;
  f << "matcap_type" << matcap_type_to_int(s._matcap_type);
  f << "matcap_file" << s._matcap_file;
  f.release();
  }

void update_current_folder(settings& s, const char* filename)
  {
  std::string path(filename);
  std::string current_folder = jtk::get_folder(path);
  if (s._current_folder != current_folder)
    {
    s._current_folder = current_folder;
    s._current_folder_files = jtk::get_files_from_directory(current_folder, false);
    }
  s._index_in_folder = -1;
  for (int32_t i = 0; i < (int32_t)s._current_folder_files.size(); ++i)
    {
    if (s._current_folder_files[i] == path)
      {
      s._index_in_folder = i;
      break;
      }
    }
  }

std::string get_settings_path()
  {
  return jtk::get_folder(jtk::get_executable_path()) + "j3d.json";
  }