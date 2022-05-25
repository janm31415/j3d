#include "gltf.h"

#define TINYGLTF_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#include "jtk/qbvh.h"

#include <string>

namespace
  {

  struct active_attributes
    {
    bool pos = false;
    bool normal = false;
    bool tangent = false;
    bool texcoord0 = false;
    bool texcoord1 = false;
    bool color0 = false;
    };

  struct primitive_attributes
    {
    std::vector<jtk::vec3<float>> pos;
    std::vector<jtk::vec3<float>> normal;
    std::vector<jtk::vec4<float>> tangent;
    std::vector<jtk::vec2<float>> texcoord0, texcoord1;
    std::vector<jtk::vec4<float>> color0;
    };

  void recurse_nodes(active_attributes& aa, const tinygltf::Model& model,
    int node_index)
    {
    const auto& node = model.nodes[node_index];
    if (node.mesh >= 0) {
      const auto& mesh = model.meshes[node.mesh];
      for (const auto& prim : mesh.primitives)
        {
        if (prim.attributes.count("POSITION"))
          aa.pos = true;
        if (prim.attributes.count("NORMAL"))
          aa.normal = true;
        if (prim.attributes.count("TANGENT"))
          aa.tangent = true;
        if (prim.attributes.count("TEXCOORD_0"))
          aa.texcoord0 = true;
        if (prim.attributes.count("TEXCOORD_1"))
          aa.texcoord1 = true;
        if (prim.attributes.count("COLOR_0"))
          aa.color0 = true;
        }
      }
    for (const auto child_index : node.children)
      {
      recurse_nodes(aa, model, child_index);
      }
    }

  active_attributes find_active_attributes(const tinygltf::Model& model)
    {
    active_attributes aa;
    for (const auto& scene : model.scenes)
      {
      for (auto node_index : scene.nodes)
        {
        recurse_nodes(aa, model, node_index);
        }
      }
    return aa;
    }

  bool is_float4x4(const std::vector<double>& v)
    {
    return v.size() == 16;
    }

  jtk::float4x4 to_float4x4(const std::vector<double>& v)
    {
    assert(is_float4x4(v));
    jtk::float4x4 m;
    for (uint32_t i = 0; i < 16; ++i)
      m[i] = v[i];
    return m;
    }

  bool is_float3(const std::vector<double>& v)
    {
    return v.size() == 3;
    }

  jtk::vec3<float> to_float3(const std::vector<double>& v)
    {
    assert(is_float3(v));
    jtk::vec3<float> f(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
    return f;
    }

  bool is_float4(const std::vector<double>& v)
    {
    return v.size() == 4;
    }

  jtk::float4 to_float4(const std::vector<double>& v)
    {
    assert(is_float4(v));
    jtk::float4 f;
    for (std::size_t i = 0; i < 4; ++i)
      f[i] = static_cast<float>(v[i]);
    return f;
    }

  jtk::vec3<float> transform_point(const jtk::float4x4& m, const jtk::vec3<float>& p)
    {
    auto res = jtk::matrix_vector_multiply(m, jtk::float4(p[0], p[1], p[2], 1.f));
    return jtk::vec3<float>(res[0] / res[3], res[1] / res[3], res[2] / res[3]);
    }

  //jtk::vec3<float> transform_vector(const jtk::float4x4& m, const jtk::vec3<float>& p)
  //  {
  //  auto res = jtk::matrix_vector_multiply(m, jtk::float4(p[0], p[1], p[2], 0.f));
  //  return jtk::vec3<float>(res[0], res[1], res[2]);
  //  }

  jtk::float4x4 get_local_matrix(const tinygltf::Node& node) {
    if (is_float4x4(node.matrix)) {
      return to_float4x4(node.matrix);
      }
    else {
      auto local_matrix = jtk::get_identity();
      // Translation is applied last, scaling is applied first,
      // so local matrix should be (T * R * S).
      if (is_float3(node.translation)) {
        const auto t = to_float3(node.translation);
        const auto T = jtk::make_translation(t);
        local_matrix = T;
        }
      if (is_float4(node.rotation)) {
        const auto quaternion = to_float4(node.rotation);
        const auto R = jtk::quaternion_to_rotation(quaternion);
        local_matrix = jtk::matrix_matrix_multiply(local_matrix, R);
        }
      if (is_float3(node.scale)) {
        const auto s = to_float3(node.scale);
        const auto S = jtk::make_scale3d(s[0], s[1], s[2]);
        local_matrix = jtk::matrix_matrix_multiply(local_matrix, S);
        }
      return local_matrix;
      }
    }


  int get_accessor_index(const tinygltf::Primitive& prim, const std::string& attribute)
    {
    const auto found = prim.attributes.find(attribute);
    if (found == prim.attributes.end())
      return -1;
    return found->second;
    }

  const tinygltf::Accessor* get_accessor(const tinygltf::Model& model, const tinygltf::Primitive& prim, const std::string& attribute)
    {
    const auto idx = get_accessor_index(prim, attribute);
    if (idx < 0)
      return nullptr;
    return &(model.accessors[idx]);
    }

  template <typename T>
  std::vector<T> get_buffer(const tinygltf::Model& model, const tinygltf::Accessor& acc)
    {
    std::vector<T> output_buffer;
    output_buffer.resize(acc.count);
    const auto& view = model.bufferViews[acc.bufferView];
    const auto& buffer = model.buffers[view.buffer];
    const auto* ptr = buffer.data.data() + view.byteOffset + acc.byteOffset;
    const auto stride = acc.ByteStride(view);
    for (int i = 0; i < acc.count; ++i)
      {
      std::memcpy(&output_buffer[i], ptr, sizeof(T));
      ptr += stride;
      }
    return output_buffer;
    }

  template <typename TDst, typename TSrc, typename Convertor>
  std::vector<TDst> get_buffer(const tinygltf::Model& model, const tinygltf::Accessor& acc, Convertor convertor)
    {
    std::vector<TDst> output_buffer;
    output_buffer.resize(acc.count);
    const auto& view = model.bufferViews[acc.bufferView];
    const auto& buffer = model.buffers[view.buffer];
    const auto* ptr = buffer.data.data() + view.byteOffset + acc.byteOffset;
    const auto stride = acc.ByteStride(view);
    for (int i = 0; i < acc.count; ++i)
      {
      TSrc element;
      std::memcpy(&element, ptr, sizeof(element));
      output_buffer[i] = convertor(element);
      ptr += stride;
      }
    return output_buffer;
    }

  std::vector<uint32_t> get_index_buffer(const tinygltf::Model& model, const tinygltf::Primitive& prim)
    {
    if (prim.indices < 0)
      return {};
    const auto& accessor = model.accessors[prim.indices];
    if (!accessor.normalized && accessor.type == TINYGLTF_TYPE_SCALAR)
      {
      if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
        auto convertor = [](std::uint8_t x)
          {
          return static_cast<std::uint32_t>(x);
          };
        return get_buffer<std::uint32_t, std::uint8_t>(model, accessor, convertor);
        }
      else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
        auto convertor = [](std::uint16_t x)
          {
          return static_cast<std::uint32_t>(x);
          };
        return get_buffer<std::uint32_t, std::uint16_t>(model, accessor, convertor);
        }
      else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
        return get_buffer<std::uint32_t>(model, accessor);
        }
      }
    return {};
    }

  template <typename T>
  float denormalize(T x)
    {
    return x / static_cast<float>(std::numeric_limits<T>::max());
    }

  jtk::vec2<float> denormalize_ubyte2(const jtk::vec2<uint8_t>& b)
    {
    return jtk::vec2<float>(denormalize(b.x), denormalize(b.y));
    }
  jtk::vec4<float> denormalize_ubyte4(const jtk::vec4<uint8_t>& b)
    {
    return jtk::vec4<float>(denormalize(b.x), denormalize(b.y), denormalize(b.z), denormalize(b.w));
    }
  jtk::vec2<float> denormalize_ushort2(const jtk::vec2<uint16_t>& b)
    {
    return jtk::vec2<float>(denormalize(b.x), denormalize(b.y));
    }
  jtk::vec4<float> denormalize_ushort4(const jtk::vec4<uint16_t>& b)
    {
    return jtk::vec4<float>(denormalize(b.x), denormalize(b.y), denormalize(b.z), denormalize(b.w));
    }

  std::vector<jtk::vec2<float>> get_texcoord_buffer(const tinygltf::Model& model, const tinygltf::Primitive& prim, const char* semantic)
    {
    const auto* accessor = get_accessor(model, prim, semantic);
    if (!accessor)
      return {};
    if (accessor->type != TINYGLTF_TYPE_VEC2)
      {
      return {};
      }
    const auto is_float = (accessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    const auto is_unbyte = (accessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) && accessor->normalized;
    const auto is_unshort = (accessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) && accessor->normalized;
    if (is_float)
      {
      return get_buffer<jtk::vec2<float>>(model, *accessor);
      }
    else if (is_unbyte)
      {
      return get_buffer<jtk::vec2<float>, jtk::vec2<uint8_t>>(model, *accessor, denormalize_ubyte2);
      }
    else if (is_unshort) {
      return get_buffer<jtk::vec2<float>, jtk::vec2<uint16_t>>(model, *accessor, denormalize_ushort2);
      }
    else
      {
      return {};
      }
    }

  std::vector<jtk::vec4<float>> get_color_buffer(const tinygltf::Model& model, const tinygltf::Primitive& prim, const char* semantic)
    {
    const auto* accessor = get_accessor(model, prim, semantic);
    if (!accessor)
      return {};
    const auto is_float = (accessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    const auto is_unbyte = (accessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) && accessor->normalized;
    const auto is_unshort = (accessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) && accessor->normalized;
    if (accessor->type == TINYGLTF_TYPE_VEC3)
      {
      if (is_float)
        {
        auto convertor = [](const jtk::vec3<float>& rgb) { return jtk::vec4<float>(rgb.x, rgb.y, rgb.z, 1.f); };
        return get_buffer<jtk::vec4<float>, jtk::vec3<float>>(model, *accessor, convertor);
        }
      else if (is_unbyte)
        {
        auto convertor = [](const jtk::vec3<uint8_t>& rgb)
          {
          return jtk::vec4<float>(denormalize(rgb.x), denormalize(rgb.y), denormalize(rgb.z), 1.f);
          };
        return get_buffer<jtk::vec4<float>, jtk::vec3<uint8_t>>(model, *accessor, convertor);
        }
      else if (is_unshort)
        {
        auto convertor = [](const jtk::vec3<uint16_t>& rgb)
          {
          return jtk::vec4<float>(denormalize(rgb.x), denormalize(rgb.y), denormalize(rgb.z), 1.f);
          };
        return get_buffer<jtk::vec4<float>, jtk::vec3<uint16_t>>(model, *accessor, convertor);
        }
      }
    else if (accessor->type == TINYGLTF_TYPE_VEC4)
      {
      if (is_float)
        {
        return get_buffer<jtk::vec4<float>>(model, *accessor);
        }
      else if (is_unbyte)
        {
        return get_buffer<jtk::vec4<float>, jtk::vec4<uint8_t>>(model, *accessor, denormalize_ubyte4);
        }
      else if (is_unshort)
        {
        return get_buffer<jtk::vec4<float>, jtk::vec4<uint16_t>>(model, *accessor, denormalize_ushort4);
        }
      }
    return {};
    }

  std::vector<jtk::vec3<float>> get_float3_buffer(
    const tinygltf::Model& model,
    const tinygltf::Primitive& prim,
    const char* semantic)
    {
    const auto* accessor = get_accessor(model, prim, semantic);
    if (!accessor)
      return {};
    if (accessor->type != TINYGLTF_TYPE_VEC3 || accessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
      {
      return {};
      }
    return get_buffer<jtk::vec3<float>>(model, *accessor);
    }

  std::vector<jtk::vec4<float>> get_float4_buffer(const tinygltf::Model& model,
    const tinygltf::Primitive& prim,
    const char* semantic)
    {
    const auto* accessor = get_accessor(model, prim, semantic);
    if (!accessor)
      return {};
    if (accessor->type != TINYGLTF_TYPE_VEC4 || accessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
      {
      return {};
      }
    return get_buffer<jtk::vec4<float>>(model, *accessor);
    }

  bool validate_attribute_counts(const primitive_attributes& pa)
    {
    if (pa.pos.empty()) {
      return false;
      }
    if (!pa.normal.empty() && pa.normal.size() != pa.pos.size())
      {
      return false;
      }
    if (!pa.tangent.empty() && pa.tangent.size() != pa.pos.size())
      {
      return false;
      }
    if (!pa.texcoord0.empty() && pa.texcoord0.size() != pa.pos.size())
      {
      return false;
      }
    if (!pa.texcoord1.empty() && pa.texcoord1.size() != pa.pos.size())
      {
      return false;
      }
    if (!pa.color0.empty() && pa.color0.size() != pa.pos.size())
      {
      return false;
      }
    return true;
    }

  void add_missing_attributes(primitive_attributes& pa, const active_attributes& aa)
    {
    const auto cnt = pa.pos.size();
    assert(cnt >= 3);
    if (aa.normal && pa.normal.size() < cnt)
      {
      pa.normal.resize(cnt, jtk::vec3<float>(0, 0, 1));
      }
    if (aa.tangent && pa.tangent.size() < cnt)
      {
      pa.tangent.resize(cnt, jtk::vec4<float>(1, 0, 0, 1));
      }
    if (aa.texcoord0 && pa.texcoord0.size() < cnt)
      {
      pa.texcoord0.resize(cnt, jtk::vec2<float>(0, 0));
      }
    if (aa.texcoord1 && pa.texcoord1.size() < cnt)
      {
      pa.texcoord1.resize(cnt, jtk::vec2<float>(0, 0));
      }
    if (aa.color0 && pa.color0.size() < cnt)
      {
      pa.color0.resize(cnt, jtk::vec4<float>(1, 1, 1, 1));
      }
    }

  void add_primitive(std::vector<jtk::vec3<float>>& vertices,
    std::vector<jtk::vec3<float>>& normals,
    std::vector<uint32_t>& clrs,
    std::vector<jtk::vec3<uint32_t>>& triangles,
    std::vector<jtk::vec3<jtk::vec2<float>>>& uv,
    const tinygltf::Model& model,
    jtk::float4x4 model_matrix,
    const tinygltf::Primitive& prim,
    const active_attributes& active_attrs)
    {
    if (prim.mode != TINYGLTF_MODE_TRIANGLES)
      {
      return;
      }

    primitive_attributes pa;
    pa.pos = get_float3_buffer(model, prim, "POSITION");
    pa.normal = get_float3_buffer(model, prim, "NORMAL");
    pa.tangent = get_float4_buffer(model, prim, "TANGENT");
    pa.texcoord0 = get_texcoord_buffer(model, prim, "TEXCOORD_0");
    pa.texcoord1 = get_texcoord_buffer(model, prim, "TEXCOORD_1");
    pa.color0 = get_color_buffer(model, prim, "COLOR_0");

    if (!validate_attribute_counts(pa))
      return;

    add_missing_attributes(pa, active_attrs);

    auto indices = get_index_buffer(model, prim);
    std::uint32_t min_index = 0;
    std::uint32_t max_index = static_cast<std::uint32_t>(pa.pos.size() - 1);
    if (indices.empty())
      {
      indices.reserve(pa.pos.size());
      for (std::uint32_t idx = 0; idx < indices.size(); ++idx)
        indices.push_back(idx);
      }
    else
      {
      const auto minmax = std::minmax_element(indices.begin(), indices.end());
      min_index = *minmax.first;
      max_index = *minmax.second;
      }
    if (indices.size() < 3)
      {
      return;
      }
    if (indices.size() % 3 != 0)
      {
      return;
      }
    if (max_index >= pa.pos.size())
      {
      return;
      }

    const auto index_range = max_index - min_index + 1;

    for (auto& pos : pa.pos)
      {
      pos = transform_point(model_matrix, pos);
      }
    for (auto& n : pa.normal) {
      n = jtk::transform_vector(model_matrix, n);
      }
    for (auto& t : pa.tangent)
      {
      jtk::vec3<float> xyz(t[0], t[1], t[2]);
      xyz = transform_vector(model_matrix, xyz);
      t = jtk::vec4<float>(xyz.x, xyz.y, xyz.z, t[3]);
      }

    const auto index_offset = static_cast<std::uint32_t>(vertices.size());

    assert((indices.size() % 3) == 0);
    for (uint32_t i = 0; i < indices.size() / 3; ++i)
      {
      jtk::vec3<uint32_t> tria(indices[i * 3] + index_offset - min_index, indices[i * 3 + 1] + index_offset - min_index, indices[i * 3 + 2] + index_offset - min_index);
      triangles.push_back(tria);
      }

    if (active_attrs.pos)
      {
      for (uint32_t i = 0; i < index_range; ++i)
        vertices.push_back(pa.pos[min_index + i]);
      }
    if (active_attrs.normal)
      {
      for (uint32_t i = 0; i < index_range; ++i)
        normals.push_back(pa.normal[min_index + i]);
      }
    if (active_attrs.texcoord0)
      {
      for (uint32_t i = 0; i < indices.size() / 3; ++i)
        {
        jtk::vec3<jtk::vec2<float>> tria_uv(pa.texcoord0[indices[i*3]], pa.texcoord0[indices[i*3+1]], pa.texcoord0[indices[i*3+2]]);
        uv.push_back(tria_uv);
        }
      }
    if (active_attrs.color0)
      {
      for (uint32_t i = 0; i < index_range; ++i)
        {
        uint32_t red = (uint32_t)(pa.color0[min_index + i].x * 255.f);
        uint32_t green = (uint32_t)(pa.color0[min_index + i].y * 255.f);
        uint32_t blue = (uint32_t)(pa.color0[min_index + i].z * 255.f);
        uint32_t alpha = (uint32_t)(pa.color0[min_index + i].w * 255.f);
        uint32_t clr = (alpha << 24) | (blue << 16) | (green << 8) | red;
        clrs.push_back(clr);
        }
      }
    }

  void recurse_nodes(std::vector<jtk::vec3<float>>& vertices,
    std::vector<jtk::vec3<float>>& normals,
    std::vector<uint32_t>& clrs,
    std::vector<jtk::vec3<uint32_t>>& triangles,
    std::vector<jtk::vec3<jtk::vec2<float>>>& uv,
    const tinygltf::Model& model,
    const active_attributes& active_attrs,
    jtk::float4x4 model_matrix,
    int node_index)
    {
    const auto& node = model.nodes[node_index];
    if (!node.matrix.empty() || !node.translation.empty() ||
      !node.rotation.empty() || !node.scale.empty()) {
      model_matrix = jtk::matrix_matrix_multiply(model_matrix, get_local_matrix(node));
      }
    if (node.mesh >= 0) {
      const auto& mesh = model.meshes[node.mesh];
      for (const auto& prim : mesh.primitives) {
        add_primitive(vertices, normals, clrs, triangles, uv, model, model_matrix, prim, active_attrs);
        }
      }
    for (const auto child_index : node.children) {
      recurse_nodes(vertices, normals, clrs, triangles, uv, model, active_attrs, model_matrix, child_index);
      }
    }
  }

bool read_gltf(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<jtk::vec3<float>>& normals, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles, std::vector<jtk::vec3<jtk::vec2<float>>>& uv)
  {
  vertices.clear();
  normals.clear();
  clrs.clear();
  triangles.clear();
  uv.clear();
  tinygltf::TinyGLTF ctx;
  ctx.SetStoreOriginalJSONForExtrasAndExtensions(true);
  tinygltf::Model model;
  std::string errors, warnings;
  if (!ctx.LoadBinaryFromFile(&model, &errors, &warnings, filename))
    {
    if (!ctx.LoadASCIIFromFile(&model, &errors, &warnings, filename))
      {
      return false;
      }
    }

  // Remove all but one scene.
  if (model.defaultScene > 0) {
    using std::swap;
    swap(model.scenes[0], model.scenes[model.defaultScene]);
    }
  model.scenes.resize(1);
  model.defaultScene = 0;
  const auto active_attrs = find_active_attributes(model);

  for (const auto node_index : model.scenes.front().nodes)
    {
    const jtk::float4x4& model_matrix = jtk::make_identity();
    recurse_nodes(vertices, normals, clrs, triangles, uv, model, active_attrs, model_matrix, node_index);
    }

  return true;
  }