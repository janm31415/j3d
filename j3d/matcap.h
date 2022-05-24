#pragma once

#include <jtk/image.h>

#include <stdint.h>
#include <map>
#include <vector>

struct matcap
  {
  jtk::image<uint32_t> im;
  uint32_t cavity_clr;
  };

void make_matcap_gray(matcap& _matcap);
void make_matcap_brown(matcap& _matcap);
void make_matcap_red_wax(matcap& _matcap);
void make_matcap_sketch(matcap& _matcap);

