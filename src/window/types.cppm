module;
#include <SDL3/SDL.h>
#include <string>

export module window:types;
import ecs;

export namespace window {
struct window_resource {
  SDL_Window *window;
  uint32_t width;
  uint32_t height;
  bool should_close{false};
};

struct window_config {
  std::string title = "engine";
  uint32_t width = 1280;
  uint32_t height = 720;
};

namespace events {
struct window_close {
  bool close {false};
};

struct window_resize {
  uint32_t height{};
  uint32_t width{};
};

struct key {
  SDL_Keycode key;
  bool pressed{false};
};

struct mouse_button {
  uint8_t button;
  bool pressed;
};

struct mouse_move {
  float x;
  float y;
  float dx;
  float dy;
};

} // namespace events

class window_plugin {
public:
  static auto create(window_config &&config) -> window_plugin;
  auto build(ecs::app::app &app) -> void;

private:
  window_config config_;
};
} // namespace window
