#include "gltf.h"

#define TINYGLTF_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#include "jtk/qbvh.h"
#include "jtk/file_utils.h"

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

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
      m[i] = static_cast<float>(v[i]);
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

  std::vector<jtk::vec2<float>> normalize_texcoord_buffer(std::vector<jtk::vec2<float>> buffer, const tinygltf::Accessor& accessor)    
    {
    if (accessor.minValues.size() < 2 || accessor.maxValues.size() < 2)
      return buffer;
    for (auto& pt : buffer)
      {
      for (int j = 0; j < 2; ++j)
        {
        pt[j] = pt[j] - accessor.minValues[j];
        pt[j] /= accessor.maxValues[j] - accessor.minValues[j];
        }
      }
    return buffer;
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
      return normalize_texcoord_buffer(get_buffer<jtk::vec2<float>>(model, *accessor), *accessor);
      }
    else if (is_unbyte)
      {
      return normalize_texcoord_buffer(get_buffer<jtk::vec2<float>, jtk::vec2<uint8_t>>(model, *accessor, denormalize_ubyte2), *accessor);
      }
    else if (is_unshort) {
      return normalize_texcoord_buffer(get_buffer<jtk::vec2<float>, jtk::vec2<uint16_t>>(model, *accessor, denormalize_ushort2), *accessor);
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
    triangles.reserve(triangles.size() + indices.size() / 3);
    for (uint32_t i = 0; i < indices.size() / 3; ++i)
      {
      jtk::vec3<uint32_t> tria(indices[i * 3] + index_offset - min_index, indices[i * 3 + 1] + index_offset - min_index, indices[i * 3 + 2] + index_offset - min_index);
      triangles.push_back(tria);
      }

    if (active_attrs.pos)
      {
      vertices.reserve(vertices.size() + index_range);
      for (uint32_t i = 0; i < index_range; ++i)
        vertices.push_back(pa.pos[min_index + i]);
      }
    if (active_attrs.normal)
      {
      normals.reserve(normals.size() + index_range);
      for (uint32_t i = 0; i < index_range; ++i)
        normals.push_back(pa.normal[min_index + i]);
      }
    if (active_attrs.texcoord0)
      {
      uv.reserve(uv.size() + indices.size() / 3);
      for (uint32_t i = 0; i < indices.size() / 3; ++i)
        {
        jtk::vec3<jtk::vec2<float>> tria_uv(pa.texcoord0[indices[i * 3]], pa.texcoord0[indices[i * 3 + 1]], pa.texcoord0[indices[i * 3 + 2]]);
        uv.push_back(tria_uv);
        }
      }
    if (active_attrs.color0)
      {
      clrs.reserve(clrs.size() + index_range);
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

  template <typename T>
  int add_buffer(tinygltf::Model& model, const std::vector<T>& data)
    {
    if (data.empty())
      return -1;
    model.buffers.emplace_back();
    auto& bytes = model.buffers.back().data;
    bytes.resize(data.size() * sizeof(T));
    std::memcpy(bytes.data(), data.data(), bytes.size());
    return static_cast<int>(model.buffers.size() - 1);
    }

  int add_buffer_view(tinygltf::Model& model, int buffer, int target = TINYGLTF_TARGET_ARRAY_BUFFER)
    {
    if (buffer < 0)
      return -1;
    model.bufferViews.emplace_back();
    auto& view = model.bufferViews.back();
    view.buffer = buffer;
    view.byteLength = model.buffers[buffer].data.size();
    view.byteOffset = 0;
    view.byteStride = 0;
    view.dracoDecoded = false;
    view.target = target;
    return static_cast<int>(model.bufferViews.size() - 1);
    }

  template <typename T>
  int add_accessor(tinygltf::Model& model, int buffer_view, int type,
    int componentType) {
    if (buffer_view < 0)
      return -1;
    model.accessors.emplace_back();
    auto& acc = model.accessors.back();
    acc.bufferView = buffer_view;
    acc.byteOffset = 0;
    acc.type = type;
    acc.componentType = componentType;
    assert(model.bufferViews[buffer_view].byteLength % sizeof(T) ==
      0); // ...or something went wrong...
    acc.count =
      static_cast<int>(model.bufferViews[buffer_view].byteLength / sizeof(T));
    acc.normalized = false;
    return static_cast<int>(model.accessors.size() - 1);
    }

  int add_accessor_float2(tinygltf::Model& model, int buffer_view)
    {
    return add_accessor<jtk::vec2<float>>(model, buffer_view, TINYGLTF_TYPE_VEC2,
      TINYGLTF_COMPONENT_TYPE_FLOAT);
    }

  int add_accessor_float3(tinygltf::Model& model, int buffer_view)
    {
    return add_accessor<jtk::vec3<float>>(model, buffer_view, TINYGLTF_TYPE_VEC3,
      TINYGLTF_COMPONENT_TYPE_FLOAT);
    }

  int add_accessor_float4(tinygltf::Model& model, int buffer_view)
    {
    return add_accessor<jtk::vec4<float>>(model, buffer_view, TINYGLTF_TYPE_VEC4,
      TINYGLTF_COMPONENT_TYPE_FLOAT);
    }

  int add_index_buffer_accessor(tinygltf::Model& model, int index_buffer, std::size_t begin_index, std::size_t end_index)
    {
    if (index_buffer < 0)
      return -1;
    const int index_count = static_cast<int>(end_index - begin_index);
    model.bufferViews.emplace_back();
    auto& view = model.bufferViews.back();
    view.buffer = index_buffer;
    view.byteLength = index_count * sizeof(std::uint32_t);
    view.byteOffset = begin_index * sizeof(std::uint32_t);
    view.byteStride = 0;
    view.dracoDecoded = false;
    view.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    model.accessors.emplace_back();
    auto& accessor = model.accessors.back();
    accessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
    accessor.byteOffset = 0;
    accessor.type = TINYGLTF_TYPE_SCALAR;
    accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    accessor.count = index_count;
    accessor.normalized = false;
    return static_cast<int>(model.accessors.size() - 1);
    }


  struct gltf_bbox
    {
    jtk::vec3<float> min{ FLT_MAX, FLT_MAX, FLT_MAX };
    jtk::vec3<float> max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
    };

  gltf_bbox calculate_bounding_box(const std::vector<jtk::vec3<float>>& v)
    {
    gltf_bbox bb;
    for (const auto& p : v)
      {
      bb.min = min(bb.min, p);
      bb.max = max(bb.max, p);
      }
    return bb;
    }

  bool _write_gltf(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv, const jtk::image<uint32_t>& texture, bool binary)
    {
    tinygltf::Model model;
    std::vector<jtk::vec4<float>> tangents;
    std::vector<jtk::vec2<float>> texcoord0, texcoord1;
    std::vector<jtk::vec4<float>> color0;
    color0.reserve(clrs.size());
    for (auto clr : clrs)
      {
      float r = (clr & 255) / 255.f;
      float g = ((clr >> 8) & 255) / 255.f;
      float b = ((clr >> 16) & 255) / 255.f;
      float a = ((clr >> 24) & 255) / 255.f;
      color0.emplace_back(r, g, b, a);
      }

    if (!uv.empty())
      {
      texcoord0.resize(vertices.size());
      for (size_t i = 0; i < uv.size(); ++i)
        {
        for (int j = 0; j < 3; ++j)
          {
          uint32_t vertex_id = triangles[i][j];
          jtk::vec2<float> vertex_uv = uv[i][j];
          texcoord0[vertex_id] = vertex_uv;
          }
        }
      }

    const int index_buffer = add_buffer(model, triangles);
    const int pos_buffer = add_buffer(model, vertices);
    const int normal_buffer = add_buffer(model, normals);
    const int tangent_buffer = add_buffer(model, tangents);
    const int texcoord0_buffer = add_buffer(model, texcoord0);
    const int texcoord1_buffer = add_buffer(model, texcoord1);
    const int color0_buffer = add_buffer(model, color0);

    const int pos_buffer_view = add_buffer_view(model, pos_buffer);
    const int normal_buffer_view = add_buffer_view(model, normal_buffer);
    const int tangent_buffer_view = add_buffer_view(model, tangent_buffer);
    const int texcoord0_buffer_view = add_buffer_view(model, texcoord0_buffer);
    const int texcoord1_buffer_view = add_buffer_view(model, texcoord1_buffer);
    const int color0_buffer_view = add_buffer_view(model, color0_buffer);

    const int pos_acc = add_accessor_float3(model, pos_buffer_view);
    const int normal_acc = add_accessor_float3(model, normal_buffer_view);
    const int tangent_acc = add_accessor_float4(model, tangent_buffer_view);
    const int texcoord0_acc = add_accessor_float2(model, texcoord0_buffer_view);
    const int texcoord1_acc = add_accessor_float2(model, texcoord1_buffer_view);
    const int color0_acc = add_accessor_float4(model, color0_buffer_view);

    const auto bb = calculate_bounding_box(vertices);
    model.accessors[pos_acc].minValues = { bb.min[0], bb.min[1], bb.min[2] };
    model.accessors[pos_acc].maxValues = { bb.max[0], bb.max[1], bb.max[2] };

    model.scenes.emplace_back();
    model.scenes.back().nodes.push_back(0);
    model.defaultScene = 0;

    model.nodes.emplace_back();
    model.nodes.back().mesh = 0;

    model.materials.emplace_back();
    model.materials.back().pbrMetallicRoughness.baseColorTexture.index = texture.width() > 0 ? 0 : -1;
    model.materials.back().values["baseColorTexture"].json_double_value["index"] = 0;

    tinygltf::Primitive base_gltf_prim;
    base_gltf_prim.mode = TINYGLTF_MODE_TRIANGLES;
    if (pos_acc >= 0)
      base_gltf_prim.attributes["POSITION"] = pos_acc;
    if (normal_acc >= 0)
      base_gltf_prim.attributes["NORMAL"] = normal_acc;
    if (tangent_acc >= 0)
      base_gltf_prim.attributes["TANGENT"] = tangent_acc;
    if (texcoord0_acc >= 0)
      base_gltf_prim.attributes["TEXCOORD_0"] = texcoord0_acc;
    if (texcoord1_acc >= 0)
      base_gltf_prim.attributes["TEXCOORD_1"] = texcoord1_acc;
    if (color0_acc >= 0)
      base_gltf_prim.attributes["COLOR_0"] = color0_acc;
    model.meshes.emplace_back();

    model.meshes.back().primitives.push_back(base_gltf_prim);
    tinygltf::Primitive& gltf_prim = model.meshes.back().primitives.back();
    gltf_prim.material = 0;
    gltf_prim.indices = add_index_buffer_accessor(model, index_buffer, 0, triangles.size() * 3);

    if (texture.width() > 0 && texture.height() > 0)
      {
      std::string fn(filename);
      model.textures.emplace_back();
      model.textures.back().source = 0;
      model.images.emplace_back();
      model.images.back().uri = jtk::get_filename(fn) + ".png";
      model.images.back().width = texture.width();
      model.images.back().height = texture.height();
      model.images.back().bits = 8;
      model.images.back().component = 4;
      model.images.back().pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
      std::vector<unsigned char> raw(texture.width() * texture.height() * 4);
      uint32_t* p_raw = (uint32_t*)raw.data();
      for (uint32_t y = 0; y < texture.height(); ++y)
        {
        const uint32_t* p_im = texture.data() + y * texture.stride();
        for (uint32_t x = 0; x < texture.width(); ++x)
          {
          *p_raw++ = *p_im++;
          }
        }
      model.images.back().image.swap(raw);
      }

    tinygltf::TinyGLTF ctx;
    ctx.SetStoreOriginalJSONForExtrasAndExtensions(true);
    return ctx.WriteGltfSceneToFile(&model, filename, true, true, true, binary);
    }

  }


bool read_gltf(const char* filename, std::vector<jtk::vec3<float>>& vertices, std::vector<jtk::vec3<float>>& normals, std::vector<uint32_t>& clrs, std::vector<jtk::vec3<uint32_t>>& triangles, std::vector<jtk::vec3<jtk::vec2<float>>>& uv, jtk::image<uint32_t>& texture)
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

  if (!model.images.empty())
    {
    std::string fn(filename);
    std::string uri = model.images.front().uri;
    if (!uri.empty())
      {
      std::string image_path = jtk::get_folder(fn) + uri;
      int w, h, nr_of_channels;
      unsigned char* im = stbi_load(image_path.c_str(), &w, &h, &nr_of_channels, 4);
      if (im)
        {
        texture = jtk::span_to_image(w, h, w, (const uint32_t*)im);
        stbi_image_free(im);
        }
      }
    else if (!model.images.front().image.empty())
      {
      const auto& im = model.images.front();
      if (im.bits == 8 && im.component == 4)
        {
        texture = jtk::image<uint32_t>(im.width, im.height);
        const uint32_t* p_im = (const uint32_t*)im.image.data();
        for (uint32_t y = 0; y < (uint32_t)im.height; ++y)
          {
          uint32_t* p_tex = texture.data() + y*texture.stride();
          for (uint32_t x = 0; x < (uint32_t)im.width; ++x)
            {
            *p_tex++ = *p_im++;
            }
          }
        }
      }
    }

  return true;
  }

bool write_glb(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv, const jtk::image<uint32_t>& texture)
  {
  return _write_gltf(filename, vertices, normals, clrs, triangles, uv, texture, true);
  }

bool write_gltf(const char* filename, const std::vector<jtk::vec3<float>>& vertices, const std::vector<jtk::vec3<float>>& normals, const std::vector<uint32_t>& clrs, const std::vector<jtk::vec3<uint32_t>>& triangles, const std::vector<jtk::vec3<jtk::vec2<float>>>& uv, const jtk::image<uint32_t>& texture)
  {
  return _write_gltf(filename, vertices, normals, clrs, triangles, uv, texture, false);
  }