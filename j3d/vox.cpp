#include "vox.h"
#define OGT_VOX_IMPLEMENTATION
#include "ogt/ogt_vox.h"
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include "ogt/ogt_voxel_meshify.h"

namespace
  {
  // a helper function to load a magica voxel scene given a filename.
  const ogt_vox_scene* load_vox_scene(const char* filename, uint32_t scene_read_flags = 0)
    {
    // open the file
#if defined(_MSC_VER) && _MSC_VER >= 1400
    FILE* fp;
    if (0 != fopen_s(&fp, filename, "rb"))
      fp = 0;
#else
    FILE* fp = fopen(filename, "rb");
#endif
    if (!fp)
      return NULL;

    // get the buffer size which matches the size of the file
    fseek(fp, 0, SEEK_END);
    uint32_t buffer_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // load the file into a memory buffer
    uint8_t* buffer = new uint8_t[buffer_size];
    fread(buffer, buffer_size, 1, fp);
    fclose(fp);

    // construct the scene from the buffer
    const ogt_vox_scene* scene = ogt_vox_read_scene_with_flags(buffer, buffer_size, scene_read_flags);

    // the buffer can be safely deleted once the scene is instantiated.
    delete[] buffer;

    return scene;
    }

  // computes transform * vec4(vec3.xyz, 0.0)
  ogt_mesh_vec3 transform_vector(const ogt_vox_transform& transform, const ogt_mesh_vec3& vec) {
    ogt_mesh_vec3 result;
    result.x = (vec.x * transform.m00) + (vec.y * transform.m10) + (vec.z * transform.m20) + (0.0f * transform.m30);
    result.y = (vec.x * transform.m01) + (vec.y * transform.m11) + (vec.z * transform.m21) + (0.0f * transform.m31);
    result.z = (vec.x * transform.m02) + (vec.y * transform.m12) + (vec.z * transform.m22) + (0.0f * transform.m32);
    return result;
    }

  // computes transform * vec4(vec3.xyz, 1.0)
  // assumes (m03, m13, m23, m33) == (0,0,0,1)
  ogt_mesh_vec3 transform_point(const ogt_vox_transform& transform, const ogt_mesh_vec3& vec) {
    ogt_mesh_vec3 result;
    result.x = (vec.x * transform.m00) + (vec.y * transform.m10) + (vec.z * transform.m20) + (1.0f * transform.m30);
    result.y = (vec.x * transform.m01) + (vec.y * transform.m11) + (vec.z * transform.m21) + (1.0f * transform.m31);
    result.z = (vec.x * transform.m02) + (vec.y * transform.m12) + (vec.z * transform.m22) + (1.0f * transform.m32);
    return result;
    }

  }

bool read_vox(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles)
  {
  vertices.clear();
  clrs.clear();
  triangles.clear();
  const ogt_vox_scene* p_scene = load_vox_scene(filename);
  if (!p_scene)
    return false;

  const char* mesh_algorithm = "polygon";
  const uint32_t frame_index = 0; // only first frame  

  // put the color index into the alpha component of every color in the palette
  ogt_vox_palette palette = p_scene->palette;
  for (uint32_t i = 0; i < 256; ++i)
    {
    palette.color[i].a = (uint8_t)i;
    }

  // meshify all models.
  ogt_voxel_meshify_context meshify_context = {};
  std::vector<ogt_mesh*> meshes;
  meshes.resize(p_scene->num_models, nullptr);
  uint32_t base_vertex_index = 0;

  for (uint32_t instance_index = 0; instance_index < p_scene->num_instances; instance_index++)
    {
    const ogt_vox_instance* instance = &p_scene->instances[instance_index];
    // skip this instance if it's hidden in the .vox file
    if (instance->hidden)
      continue;
    // skip this instance if it's part of a hidden layer in the .vox file
    if (p_scene->layers[instance->layer_index].hidden)
      continue;
    // skip this instance if it's part of a hidden group
    if (p_scene->num_groups > 0 && instance->group_index != k_invalid_group_index && p_scene->groups[instance->group_index].hidden)
      continue;

    ogt_vox_transform transform = p_scene->groups ? ogt_vox_sample_instance_transform(instance, frame_index, p_scene) : _vox_transform_identity();
    uint32_t          model_index = ogt_vox_sample_instance_model(instance, frame_index);

    // just in time generate the mesh for this model if we haven't already done so.
    const ogt_vox_model* model = p_scene->models[model_index];
    if (model && !meshes[model_index])
      {
      ogt_mesh* mesh =
        (strcmp(mesh_algorithm, "polygon") == 0) ? ogt_mesh_from_paletted_voxels_polygon(&meshify_context, model->voxel_data, model->size_x, model->size_y, model->size_z, (const ogt_mesh_rgba*)&palette.color[0]) :
        (strcmp(mesh_algorithm, "greedy") == 0) ? ogt_mesh_from_paletted_voxels_greedy(&meshify_context, model->voxel_data, model->size_x, model->size_y, model->size_z, (const ogt_mesh_rgba*)&palette.color[0]) :
        (strcmp(mesh_algorithm, "simple") == 0) ? ogt_mesh_from_paletted_voxels_simple(&meshify_context, model->voxel_data, model->size_x, model->size_y, model->size_z, (const ogt_mesh_rgba*)&palette.color[0]) :
        nullptr;
      for (uint32_t i = 0; i < mesh->vertex_count; ++i)
        {
        // pre-bias the mesh vertices by the model dimensions - resets the center/pivot so it is at (0,0,0)
        mesh->vertices[i].pos.x -= (float)(model->size_x / 2);
        mesh->vertices[i].pos.y -= (float)(model->size_y / 2);
        mesh->vertices[i].pos.z -= (float)(model->size_z / 2);
        // the normal is always a unit vector aligned on one of the 6 cardinal directions, here we just
        // precompute which index it was (same order as the 'vn' tags we wrote out when opening the file)
        // and write it as a uint32_t into the x field. This allows us to avoid do this index conversion
        // for every vert in a mesh, and not for every vert multiplied by the number of instances.
        ogt_mesh_vec3& normal = mesh->vertices[i].normal;
        uint32_t normal_index = normal.x != 0 ? (normal.x > 0.0f ? 0 : 1) :
          normal.y != 0 ? (normal.y > 0.0f ? 2 : 3) :
          (normal.z > 0.0f ? 4 : 5);
        *(uint32_t*)&normal.x = normal_index;
        }
      // cache this mesh so we don't need to do it multiple times.
      meshes[model_index] = mesh;
      }
    // some instances can have no geometry, so we just skip em
    const ogt_mesh* mesh = meshes[model_index];
    if (!mesh)
      continue;
    vertices.reserve(vertices.size() + mesh->vertex_count);
    clrs.reserve(clrs.size() + mesh->vertex_count);
    for (size_t i = 0; i < mesh->vertex_count; ++i)
      {
      ogt_mesh_vec3 pos = transform_point(transform, mesh->vertices[i].pos);
      vertices.emplace_back(pos.x, pos.y, pos.z);
      uint32_t red = mesh->vertices[i].color.r;
      uint32_t green = mesh->vertices[i].color.g;
      uint32_t blue = mesh->vertices[i].color.b;
      clrs.push_back(0xff000000|(blue<<16)|(green<<8)|red);
      }
    triangles.reserve(triangles.size() + mesh->index_count/3);
    for (size_t i = 0; i < mesh->index_count; i += 3)
      {
      uint32_t v_i0 = base_vertex_index + mesh->indices[i + 0];
      uint32_t v_i1 = base_vertex_index + mesh->indices[i + 1];
      uint32_t v_i2 = base_vertex_index + mesh->indices[i + 2];
      triangles.emplace_back(v_i0, v_i1, v_i2);
      }
    base_vertex_index += mesh->vertex_count;

    }

  ogt_vox_destroy_scene(p_scene);
  return true;
  }