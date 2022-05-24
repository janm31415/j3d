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
  _executable_path = jtk::get_executable_path();
  _index_in_folder = -1;
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
  f.release();
  }

void update_current_folder(settings& s, const char* filename)
  {
  std::string path(filename);
  s._current_folder = jtk::get_folder(path);
  s._current_folder_files = jtk::get_files_from_directory(s._current_folder, false);
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