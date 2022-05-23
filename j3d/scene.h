#pragma once
#include <jtk/qbvh.h>
#include <jtk/vec.h>
#include "db.h"

#include <stdint.h>
#include <jtk/image.h>
#include <list>

struct scene_object
  {
  uint32_t db_id;
  std::vector<jtk::vec3<float>>* p_vertices;
  std::vector<jtk::vec3<uint32_t>>* p_triangles;
  std::vector<jtk::vec3<float>> triangle_normals;
  std::vector<jtk::vec3<float>>* p_vertex_colors;
  std::vector<jtk::vec3<jtk::vec2<float>>>* p_uv_coordinates;
  jtk::image<uint32_t>* p_texture;

  jtk::vec3<float> min_bb;
  jtk::vec3<float> max_bb;

  std::unique_ptr<jtk::qbvh> bvh;
  jtk::float4x4 cs;
  };

struct scene_pointcloud
  {
  uint32_t db_id;
  std::vector<jtk::vec3<float>>* p_vertices;
  std::vector<jtk::vec3<float>>* p_normals;
  std::vector<uint32_t>* p_vertex_colors;

  jtk::vec3<float> min_bb;
  jtk::vec3<float> max_bb;

  jtk::float4x4 cs;
  };

struct scene
  {
  jtk::float4x4 coordinate_system, coordinate_system_inv;

  float pivot[3];
  jtk::vec3<float> min_bb;
  jtk::vec3<float> max_bb;
  float diagonal;

  std::list<scene_object> objects;
  std::list<scene_pointcloud> pointclouds;
  };

void add_object(uint32_t id, scene& s, db& d);

void remove_object(uint32_t id, scene& s);

void prepare_scene(scene& s);

void unzoom(scene& s);