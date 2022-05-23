#pragma once

#include <jtk/qbvh.h>

struct camera
  {
  enum FitResolutionGate { Fill = 0, Overscan };

  float focalLength = 35.f;
  float filmApertureWidthInch = 0.825f;
  float filmApertureHeightInch = 0.446f;
  float nearClippingPlane = 5.f;
  float farClipingPlane = 1000.f;
  float zoom;
  FitResolutionGate fitFilm;
  };


camera make_default_camera();

jtk::float4x4 frustum(float left, float right, float bottom, float top, float near_plane, float far_plane);

jtk::float4x4 invert_projection_matrix(const jtk::float4x4& in);

jtk::float4x4 make_projection_matrix(const camera& c, int screen_w, int screen_h);
 