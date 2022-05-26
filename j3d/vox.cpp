#include "vox.h"
#define OGT_VOX_IMPLEMENTATION
#include "ogt/ogt_vox.h"
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include "ogt/ogt_voxel_meshify.h"

#include "mesh.h"
#include "jtk/concurrency.h"
#include "jtk/qbvh.h"

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

  // a helper function to save a magica voxel scene to disk.
  void save_vox_scene(const char* pcFilename, const ogt_vox_scene* scene)
    {
    // save the scene back out. 
    uint32_t buffersize = 0;
    uint8_t* buffer = ogt_vox_write_scene(scene, &buffersize);
    if (!buffer)
      return;

    // open the file for write
#if defined(_MSC_VER) && _MSC_VER >= 1400
    FILE* fp;
    if (0 != fopen_s(&fp, pcFilename, "wb"))
      fp = 0;
#else
    FILE* fp = fopen(pcFilename, "wb");
#endif
    if (!fp) {
      ogt_vox_free(buffer);
      return;
      }

    fwrite(buffer, buffersize, 1, fp);
    fclose(fp);
    ogt_vox_free(buffer);
    }

  void set_voxel(uint8_t* data, uint8_t value, uint32_t x, uint32_t y, uint32_t z, uint32_t* dim)
    {
    uint32_t idx = x + (y + z * dim[1]) * dim[0];
    data[idx] = value;
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
      clrs.push_back(0xff000000 | (blue << 16) | (green << 8) | red);
      }
    triangles.reserve(triangles.size() + mesh->index_count / 3);
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


bool write_vox(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<uint32_t>>& triangles, uint32_t max_dim)
  {
  jtk::vec3<float> min_bb, max_bb;
  compute_bb(min_bb, max_bb, (uint32_t)vertices.size(), vertices.data());
  int largest_dim = 0;
  if ((max_bb[1] - min_bb[1]) > (max_bb[largest_dim] - min_bb[largest_dim]))
    largest_dim = 1;
  if ((max_bb[2] - min_bb[2]) > (max_bb[largest_dim] - min_bb[largest_dim]))
    largest_dim = 2;
  
  uint32_t dim[3] = { (uint32_t)(max_dim * (max_bb[0] - min_bb[0]) / (max_bb[largest_dim] - min_bb[largest_dim])),
     (uint32_t)(max_dim * (max_bb[1] - min_bb[1]) / (max_bb[largest_dim] - min_bb[largest_dim])),
     (uint32_t)(max_dim * (max_bb[2] - min_bb[2]) / (max_bb[largest_dim] - min_bb[largest_dim])) };
  for (int j = 0; j < 3; ++j)
    if (dim[j] == 0)
      dim[j] = 1;

  uint8_t* data = new uint8_t[dim[0] * dim[1] * dim[2]];
  memset((void*)data, 0, dim[0] * dim[1] * dim[2]);

  std::unique_ptr<jtk::qbvh> bvh = std::unique_ptr<jtk::qbvh>(new jtk::qbvh(triangles, vertices.data()));

  for (int direction_dim = 0; direction_dim < 3; ++direction_dim)
    {
    jtk::float4 direction(1, 0, 0, 0);
    if (direction_dim == 1)
      direction = jtk::float4(0, 1, 0, 0);
    else if (direction_dim == 2)
      direction = jtk::float4(0, 0, 2, 0);
    int dim1 = (direction_dim + 1) % 3;
    int dim2 = (direction_dim + 2) % 3;
    for (uint32_t d1 = 0; d1 < dim[dim1]; ++d1)
      {
      for (uint32_t d2 = 0; d2 < dim[dim2]; ++d2)
        {
        jtk::vec3<float> ray_start;
        ray_start[direction_dim] = min_bb[direction_dim];
        ray_start[dim1] = ((d1 + 0.5f) / (double)dim[dim1]) * (max_bb[dim1] - min_bb[dim1]) + min_bb[dim1];
        ray_start[dim2] = ((d2 + 0.5f) / (double)dim[dim2]) * (max_bb[dim2] - min_bb[dim2]) + min_bb[dim2];
        jtk::ray r;
        r.orig = jtk::float4(ray_start[0], ray_start[1], ray_start[2], 1.f);
        r.dir = direction;
        r.t_near = 0.f;
        r.t_far = std::numeric_limits<float>::max();
        std::vector<uint32_t> triangle_ids;
        std::vector<jtk::hit> all_hits = bvh->find_all_triangles(triangle_ids, r, triangles.data(), vertices.data());
        for (size_t i = 0; i < all_hits.size(); ++i)
          {
          const auto& hit = all_hits[i];
          const uint32_t v0 = triangles[triangle_ids[i]][0];
          const uint32_t v1 = triangles[triangle_ids[i]][1];
          const uint32_t v2 = triangles[triangle_ids[i]][2];
          const auto pos = vertices[v0] * (1.f - hit.u - hit.v) + hit.u * vertices[v1] + hit.v * vertices[v2];
          float x = (pos.x - min_bb[0]) / (max_bb[0] - min_bb[0]);
          uint32_t X = (uint32_t)(x * dim[0]);
          if (X == dim[0])
            X = dim[0] - 1;
          float y = (pos.y - min_bb[1]) / (max_bb[1] - min_bb[1]);
          uint32_t Y = (uint32_t)(y * dim[1]);
          if (Y == dim[1])
            Y = dim[1] - 1;
          float z = (pos.z - min_bb[2]) / (max_bb[2] - min_bb[2]);
          uint32_t Z = (uint32_t)(z * dim[2]);
          if (Z == dim[2])
            Z = dim[2] - 1;
          set_voxel(data, 255, X, Y, Z, dim);
          }
        }
      }
    }

  ogt_vox_model* model = new ogt_vox_model;
  model->size_x = dim[0];
  model->size_y = dim[1];
  model->size_z = dim[2];
  model->voxel_data = data;

  ogt_vox_palette palette;
  for (int i = 0; i < 256; ++i)
    {
    palette.color[i].r = (uint8_t)i;
    palette.color[i].g = (uint8_t)i;
    palette.color[i].b = (uint8_t)i;
    palette.color[i].a = 255;
    }

  ogt_vox_transform identity_transform = _vox_transform_identity();

  ogt_vox_layer default_layer;
  default_layer.hidden = false;
  default_layer.name = "default";

  ogt_vox_group default_group;
  default_group.hidden = false;
  default_group.layer_index = 0;
  default_group.parent_group_index = k_invalid_group_index;
  default_group.transform = identity_transform;
  default_group.name = "default";
  default_group.transform_anim.keyframes = nullptr;
  default_group.transform_anim.num_keyframes = 0;
  default_group.transform_anim.loop = false;

  ogt_vox_instance instance;
  instance.group_index = 0;   // default_group
  instance.hidden = false;
  instance.layer_index = 0;   // default_layer
  instance.model_index = 0;
  instance.name = filename;
  instance.transform = identity_transform;
  instance.transform_anim.keyframes = nullptr;
  instance.transform_anim.num_keyframes = 0;
  instance.transform_anim.loop = false;
  instance.model_anim.keyframes = nullptr;
  instance.model_anim.num_keyframes = 0;
  instance.model_anim.loop = false;

  ogt_vox_scene output_scene;
  output_scene.groups = &default_group;
  output_scene.num_groups = 1;
  output_scene.instances = &instance;
  output_scene.num_instances = 1;
  output_scene.instances = &instance;
  output_scene.num_layers = 1;
  output_scene.layers = &default_layer;
  output_scene.num_models = 1;
  output_scene.models = &((const ogt_vox_model*)model);
  output_scene.palette = palette;
  output_scene.cameras = nullptr;
  output_scene.num_cameras = 0;

  ogt_vox_matl default_mat;
  default_mat.content_flags = 0;

  for (int i = 0; i < 256; ++i)
    output_scene.materials.matl[i] = default_mat;
  save_vox_scene(filename, &output_scene);

  delete model;
  delete[] data;
  return true;
  }