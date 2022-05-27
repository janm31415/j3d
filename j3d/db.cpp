#include "db.h"
#include "mesh.h"
#include "pc.h"

#include <cassert>

using namespace jtk;

db::db()
  {

  }

db::~db()
  {
  clear();
  }

void db::swap(db& other)
  {
  std::swap(meshes, other.meshes);
  std::swap(meshes_deleted, other.meshes_deleted);
  std::swap(pcs, other.pcs);
  std::swap(pcs_deleted, other.pcs_deleted);
  }

void db::create_mesh(mesh*& new_mesh, uint32_t& id)
  {
  mesh* m = new mesh();
  id = make_db_id(MESH_KEY, (uint32_t)meshes.size());
  new_mesh = m;
  meshes.push_back(std::make_pair(id, m));
  meshes_deleted.push_back(std::make_pair(id, nullptr));
  }

mesh* db::get_mesh(uint32_t id) const
  {
  auto key = get_db_key(id);
  auto vector_index = get_db_vector_index(id);
  if (key != MESH_KEY)
    return nullptr;
  if (vector_index >= meshes.size())
    return nullptr;
  assert(meshes[vector_index].first == id);
  return meshes[vector_index].second;
  }

bool db::is_mesh(uint32_t id) const
  {
  return get_db_key(id) == MESH_KEY;
  }

void db::create_pc(pc*& new_pointcloud, uint32_t& id)
  {
  pc* m = new pc();
  id = make_db_id(PC_KEY, (uint32_t)pcs.size());
  new_pointcloud = m;
  pcs.push_back(std::make_pair(id, m));
  pcs_deleted.push_back(std::make_pair(id, nullptr));
  }

pc* db::get_pc(uint32_t id) const
  {
  auto key = get_db_key(id);
  auto vector_index = get_db_vector_index(id);
  if (key != PC_KEY)
    return nullptr;
  if (vector_index >= pcs.size())
    return nullptr;
  assert(pcs[vector_index].first == id);
  return pcs[vector_index].second;
  }

bool db::is_pc(uint32_t id) const
  {
  return get_db_key(id) == PC_KEY;
  }

void db::delete_object_hard(uint32_t id)
  {
  auto key = get_db_key(id);
  auto vector_index = get_db_vector_index(id);
  switch (key)
    {
    case MESH_KEY:
      if (meshes[vector_index].second)
        {
        delete meshes_deleted[vector_index].second;
        delete meshes[vector_index].second;
        meshes_deleted.erase(meshes_deleted.begin() + vector_index);
        meshes.erase(meshes.begin() + vector_index);
        }
      break;
    case PC_KEY:
      if (pcs[vector_index].second)
        {
        delete pcs_deleted[vector_index].second;
        delete pcs[vector_index].second;
        pcs_deleted.erase(pcs_deleted.begin() + vector_index);
        pcs.erase(pcs.begin() + vector_index);
        }
      break;
    }
  }
void db::delete_object(uint32_t id)
  {
  auto key = get_db_key(id);
  auto vector_index = get_db_vector_index(id);
  switch (key)
    {
    case MESH_KEY:
      if (meshes[vector_index].second)
        {
        meshes_deleted[vector_index].second = meshes[vector_index].second;
        meshes[vector_index].second = nullptr;
        }
      break;
    case PC_KEY:
      if (pcs[vector_index].second)
        {
        pcs_deleted[vector_index].second = pcs[vector_index].second;
        pcs[vector_index].second = nullptr;
        }
      break;
    }
  }

void db::restore_object(uint32_t id)
  {
  auto key = get_db_key(id);
  auto vector_index = get_db_vector_index(id);
  switch (key)
    {
    case MESH_KEY:
      if (!meshes[vector_index].second)
        {
        meshes[vector_index].second = meshes_deleted[vector_index].second;
        meshes_deleted[vector_index].second = nullptr;
        }
      break;
    case PC_KEY:
      if (!pcs[vector_index].second)
        {
        pcs[vector_index].second = pcs_deleted[vector_index].second;
        pcs_deleted[vector_index].second = nullptr;
        }
      break;
    }
  }

namespace
  {
  template <class T>
  void delete_objects(std::vector<std::pair<uint32_t, T*>>& vec)
    {
    for (auto& p_obj : vec)
      {
      delete p_obj.second;
      p_obj.second = nullptr;
      }
    vec.clear();
    }
  }

void db::clear()
  {
  delete_objects(meshes);
  delete_objects(meshes_deleted);
  delete_objects(pcs);
  delete_objects(pcs_deleted);
  }

std::vector<vec3<float>>* get_vertices(const db& _db, uint32_t id)
  {
  auto key = get_db_key(id);
  switch (key)
    {
    case MESH_KEY:
      return &_db.get_mesh(id)->vertices;
      break;
    case PC_KEY:
      return &_db.get_pc(id)->vertices;
      break;
    }
  return nullptr;
  }

std::vector<vec3<uint32_t>>* get_triangles(const db& _db, uint32_t id)
  {
  auto key = get_db_key(id);
  switch (key)
    {
    case MESH_KEY:
      return &_db.get_mesh(id)->triangles;
      break;
    }
  return nullptr;
  }

float4x4* get_cs(const db& _db, uint32_t id)
  {
  auto key = get_db_key(id);
  switch (key)
    {
    case MESH_KEY:
      return &_db.get_mesh(id)->cs;
      break;
    case PC_KEY:
      return &_db.get_pc(id)->cs;
      break;
    }
  return nullptr;
  }

bool is_visible(const db& _db, uint32_t id)
  {
  auto key = get_db_key(id);
  switch (key)
    {
    case MESH_KEY:
      return _db.get_mesh(id)->visible;
      break;
    case PC_KEY:
      return _db.get_pc(id)->visible;
      break;
    }
  return false;
  }

double get_load_time_in_s(const db& _db, uint32_t id)
  {
  auto key = get_db_key(id);
  switch (key)
    {
    case MESH_KEY:
      return _db.get_mesh(id)->load_time_in_s;
      break;
    case PC_KEY:
      return _db.get_pc(id)->load_time_in_s;
      break;
    }
  return -1.0;
  }