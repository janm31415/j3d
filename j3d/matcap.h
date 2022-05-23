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

class matcapmap
  {
  public:
    matcapmap();

    const matcap& get_matcap(uint32_t id) const;

    void make_new_matcap(uint32_t matcap_id, const matcap& mc);
    void map_db_id_to_matcap_id(uint32_t db_id, uint32_t matcap_id);

    uint32_t get_semirandom_matcap_id_for_given_db_id(uint32_t db_id) const;

  private:
    void _optimize_maps();

  private:
    std::map<uint32_t, matcap> matcaps;
    std::map<uint32_t, uint32_t> map_db_id_to_matcap;

    std::map<uint32_t, std::map<uint32_t, matcap>::iterator> optimized_map;
  };