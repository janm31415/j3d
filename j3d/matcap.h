#pragma once

#include <jtk/image.h>

#include <stdint.h>
#include <string>
#include <vector>

enum class matcap_type
  {
  MATCAP_TYPE_INTERNAL_REDWAX,
  MATCAP_TYPE_INTERNAL_GRAY,
  MATCAP_TYPE_INTERNAL_BROWN,
  MATCAP_TYPE_INTERNAL_SKETCH,
  MATCAP_TYPE_EXTERNAL_FILE,
  };

struct matcap
  {
  jtk::image<uint32_t> im;
  uint32_t cavity_clr;
  matcap_type type;
  std::string filename;
  };

void make_matcap_gray(matcap& _matcap);
void make_matcap_brown(matcap& _matcap);
void make_matcap_red_wax(matcap& _matcap);
void make_matcap_sketch(matcap& _matcap);
void make_matcap_from_file(matcap& _matcap, const char* filename);

int32_t matcap_type_to_int(matcap_type t);
matcap_type int_to_matcap_type(int32_t i);

void make_matcap(matcap& _matcap, matcap_type t, const char* filename);
