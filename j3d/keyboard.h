#pragma once

#include <SDL.h>

#include <unordered_map>
#include <unordered_set>

class keyboard_handler
  {
  enum state
    {
    DOWN,
    UP
    };

  public:

    void initialise_for_new_events()
      {
      pressed.clear();
      }

    void handle_event(SDL_Event event)
      {      
      if (event.type == SDL_KEYDOWN)
        {
        keystate[event.key.keysym.sym] = DOWN;
        pressed.insert(event.key.keysym.sym);
        }
      else if (event.type == SDL_KEYUP)
        keystate[event.key.keysym.sym] = UP;
      }
    bool is_pressed(int key_code) const
      {
      auto it = pressed.find(key_code);
      return (it != pressed.end());
      }
    bool is_down(int key_code) const
      {
      auto it = keystate.find(key_code);
      if (it != keystate.end())
        return it->second == DOWN;
      else
        return false;
      }
    bool is_up(int key_code) const
      {
      auto it = keystate.find(key_code);
      if (it != keystate.end())
        return it->second == UP;
      else
        return true;
      }
    void down(int key_code)
      {
      keystate[key_code] = DOWN;
      }
    void up(int key_code)
      {
      keystate[key_code] = UP;
      }
  private:
    std::unordered_set<uint32_t> pressed;
    std::unordered_map<uint32_t, state> keystate;
  };