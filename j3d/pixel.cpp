#include "pixel.h"


using namespace jtk;

uint32_t get_closest_vertex(const pixel& p, const vec3<float>* vertices, const vec3<uint32_t>* triangles)
  {
  const uint32_t v0 = triangles[p.object_id][0];
  const uint32_t v1 = triangles[p.object_id][1];
  const uint32_t v2 = triangles[p.object_id][2];
  const vec3<float> V0 = vertices[v0];
  const vec3<float> V1 = vertices[v1];
  const vec3<float> V2 = vertices[v2];
  const vec3<float> pos = V0 * (1.f - p.barycentric_u - p.barycentric_v) + p.barycentric_u*V1 + p.barycentric_v*V2;

  const auto A = pos - V0;
  const auto B = pos - V1;
  const auto C = pos - V2;

  const float distA = dot(A, A);
  const float distB = dot(B, B);
  const float distC = dot(C, C);

  return (distA < distB) ? ((distA < distC) ? v0 : v2) : ((distB < distC) ? v1 : v2);
  }


uint32_t get_closest_vertex(const pixel& p, const std::vector<jtk::vec3<float>>* vertices, const std::vector<jtk::vec3<uint32_t>>* triangles)
  {
  if (triangles != nullptr)
    return get_closest_vertex(p, vertices->data(), triangles->data());
  return p.object_id;
  }