#pragma once

#include <string>
#include <vector>

#include "canvas.h"
#include "matcap.h"

struct settings
  {
  settings();
  std::string _current_folder;
  std::string _executable_path;
  std::vector<std::string> _current_folder_files;
  int32_t _index_in_folder;
  canvas::canvas_settings _canvas_settings;  
  uint32_t _canvas_w, _canvas_h;
  matcap_type _matcap_type;
  std::string _matcap_file;
  uint32_t _gradient_top, _gradient_bottom, _background;
  };


settings read_settings(const char* filename);

void write_settings(const settings& s, const char* filename);

void update_current_folder(settings& s, const char* filename);

std::string get_settings_path();
