#include "scene.h"
#include "mesh.h"
#include "pc.h"
#include <jtk/geometry.h>

using namespace jtk;

void add_object(uint32_t id, scene& s, db& d)
  {
  if (d.is_mesh(id))
    {
    mesh* p_mesh = d.get_mesh(id);
    scene_object obj;
    obj.db_id = id;
    obj.p_triangles = &p_mesh->triangles;
    obj.p_vertices = &p_mesh->vertices;
    obj.p_vertex_colors = &p_mesh->vertex_colors;
    obj.p_uv_coordinates = &p_mesh->uv_coordinates;
    obj.p_texture = &p_mesh->texture;
    compute_triangle_normals(obj.triangle_normals, obj.p_vertices->data(), obj.p_triangles->data(), (uint32_t)obj.p_triangles->size());
    obj.cs = p_mesh->cs;
    compute_bb(obj.min_bb, obj.max_bb, (uint32_t)obj.p_vertices->size(), obj.p_vertices->data());    
    obj.bvh = std::unique_ptr<qbvh>(new qbvh(*obj.p_triangles, obj.p_vertices->data()));
    s.objects.emplace_back(std::move(obj));
    } 
  if (d.is_pc(id))
    {
    pc* p_pc = d.get_pc(id);
    scene_pointcloud obj;
    obj.db_id = id;
    obj.p_vertices = &p_pc->vertices;
    obj.p_vertex_colors = &p_pc->vertex_colors;
    obj.p_normals = &p_pc->normals;   
    obj.cs = p_pc->cs;
    compute_bb(obj.min_bb, obj.max_bb, (uint32_t)obj.p_vertices->size(), obj.p_vertices->data());   
    s.pointclouds.emplace_back(std::move(obj));
    } 
  }

void remove_object(uint32_t id, scene& s)
  {
  auto it = std::find_if(s.objects.begin(), s.objects.end(), [&](const scene_object& so) { return so.db_id == id; });
  if (it != s.objects.end())
    s.objects.erase(it);
  auto it2 = std::find_if(s.pointclouds.begin(), s.pointclouds.end(), [&](const scene_pointcloud& so) { return so.db_id == id; });
  if (it2 != s.pointclouds.end())
    s.pointclouds.erase(it2);
  }

void prepare_scene(scene& s)
  {
  if (!s.objects.empty())
    {
    auto min_bb = transform(s.objects.front().cs, s.objects.front().min_bb);
    auto max_bb = transform(s.objects.front().cs, s.objects.front().max_bb);
    s.min_bb = min(min_bb, max_bb);
    s.max_bb = max(min_bb, max_bb);
    }
  else if (!s.pointclouds.empty())
    {
    auto min_bb = transform(s.pointclouds.front().cs, s.pointclouds.front().min_bb);
    auto max_bb = transform(s.pointclouds.front().cs, s.pointclouds.front().max_bb);
    s.min_bb = min(min_bb, max_bb);
    s.max_bb = max(min_bb, max_bb);
    }
  else
    {
    s.min_bb = vec3<float>(0, 0, 0);
    s.max_bb = vec3<float>(0, 0, 0);
    }
  for (const auto& obj : s.objects)
    {
    auto local_min_bb = transform(obj.cs, obj.min_bb);
    auto local_max_bb = transform(obj.cs, obj.max_bb);
    s.min_bb = min(s.min_bb, min(local_min_bb, local_max_bb));
    s.max_bb = max(s.max_bb, max(local_min_bb, local_max_bb));
    }
  for (const auto& obj : s.pointclouds)
    {
    auto local_min_bb = transform(obj.cs, obj.min_bb);
    auto local_max_bb = transform(obj.cs, obj.max_bb);
    s.min_bb = min(s.min_bb, min(local_min_bb, local_max_bb));
    s.max_bb = max(s.max_bb, max(local_min_bb, local_max_bb));
    }

  s.diagonal = s.max_bb[0] - s.min_bb[0];
  s.diagonal = std::max<float>(s.diagonal, s.max_bb[1] - s.min_bb[1]);
  s.diagonal = std::max<float>(s.diagonal, s.max_bb[2] - s.min_bb[2]);
  }

void unzoom(scene& s)
  {
  s.coordinate_system = get_identity();
  for (int j = 0; j < 3; ++j)
    s.pivot[j] = (s.min_bb[j] + s.max_bb[j])*0.5f;
  s.coordinate_system[12] = s.pivot[0];
  s.coordinate_system[13] = s.pivot[1];
  s.coordinate_system[14] = s.pivot[2] + s.diagonal * 2.f;

  s.coordinate_system_inv = invert_orthonormal(s.coordinate_system);
  }