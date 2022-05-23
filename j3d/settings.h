#pragma once

#include <string>
#include <vector>

#include "canvas.h"

struct settings
  {
  settings();
  std::string _current_folder;
  std::string _executable_path;
  std::vector<std::string> _current_folder_files;
  int32_t _index_in_folder;
  canvas::canvas_settings _canvas_settings;  
  };


settings read_settings(const char* filename);

void write_settings(const settings& s, const char* filename);

void update_current_folder(settings& s, const char* filename);


