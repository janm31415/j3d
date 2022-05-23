#pragma once

struct mouse_data
  {
  bool left_dragging;
  bool right_dragging;
  bool wheel_mouse_pressed;
  float mouse_x;
  float mouse_y;
  float prev_mouse_x;
  float prev_mouse_y;
  double wheel_rotation;
  bool left_button_down;
  bool right_button_down;
  bool wheel_down;
  bool ctrl_pressed;
  };
