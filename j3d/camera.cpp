#include "camera.h"

using namespace jtk;

camera make_default_camera()
  {
  camera c;
  c.focalLength = 35.f;
  c.filmApertureWidthInch = 1.024f;//0.825f;
  c.filmApertureHeightInch = 0.768f;//0.446f;  
  c.nearClippingPlane = 0.1f;
  c.farClipingPlane = std::numeric_limits<float>::max();
  c.fitFilm = camera::Overscan;
  c.zoom = 1.f;
  return c;
  }

float4x4 frustum(float left, float right, float bottom, float top, float near_plane, float far_plane)
  { 
  float4x4 out (float4(2.f*near_plane / (right - left), 0.f, 0.f, 0.f), float4(0.f, -2.f*near_plane / (top - bottom), 0.f, 0.f),
    float4((right + left) / (right - left), -(top + bottom) / (top - bottom), -(far_plane + near_plane) / (far_plane - near_plane), -1.f),
    float4(0.f, 0.f, -(2.f * far_plane * near_plane) / (far_plane - near_plane), 0.f));
  return out;
  }

float4x4 invert_projection_matrix(const float4x4& in)
  {  
  float4x4 out(float4(1.f / in[0], 0.f, 0.f, 0.f), float4(0.f, 1.f / in[5], 0.f, 0.f), float4(0.f, 0.f, 0.f, 1.f / in[14]), float4(in[8] / in[0], in[9] / in[5], -1.f, in[10] / in[14]));
  return out;
  }

float4x4 make_projection_matrix(const camera& c, int screen_w, int screen_h)
  {
  static const float inchToMm = 25.4f;

  float top = ((c.filmApertureHeightInch * inchToMm / 2.f) / c.focalLength) * c.nearClippingPlane;
  float right = ((c.filmApertureWidthInch * inchToMm / 2.f) / c.focalLength) * c.nearClippingPlane;

  float xscale = c.zoom;
  float yscale = c.zoom;

  float deviceAspectRatio = screen_w / (float)screen_h;
  float filmAspectRatio = c.filmApertureWidthInch / c.filmApertureHeightInch;

  switch (c.fitFilm) {
    default:
    case camera::Fill:
      if (filmAspectRatio > deviceAspectRatio) {
        xscale *= deviceAspectRatio / filmAspectRatio;
        }
      else {
        yscale *= filmAspectRatio / deviceAspectRatio;
        }
      break;
    case camera::Overscan:
      if (filmAspectRatio > deviceAspectRatio) {
        yscale *= filmAspectRatio / deviceAspectRatio;
        }
      else {
        xscale *= deviceAspectRatio / filmAspectRatio;
        }
      break;
    }

  right *= xscale;
  top *= yscale;

  float bottom = -top;
  float left = -right;

  auto projection_matrix = frustum(left, right, bottom, top, c.nearClippingPlane, c.farClipingPlane);
  return projection_matrix;
  }
