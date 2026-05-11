module;

#include <SDL3/SDL_events.h>
export module window:poll;
import ecs;
import :types;

namespace window::systems {

auto poll(ecs::events::event_writer<events::window_close> close,
          ecs::events::event_writer<events::window_resize> resize,
          ecs::events::event_writer<events::key> keys,
          ecs::resource::resource<window_resource> win) -> void {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {

    switch (e.type) {
    case SDL_EVENT_QUIT:
      close.send(events::window_close{});
      break;

    case SDL_EVENT_WINDOW_RESIZED:
      win->width = static_cast<uint32_t>(e.window.data1);
      win->height = static_cast<uint32_t>(e.window.data2);
      resize.send(
          events::window_resize{.width = win->width, .height = win->height});
      break;

    case SDL_EVENT_KEY_DOWN:
      keys.send(events::key{.key = e.key.key, .pressed = true});
      break;

    case SDL_EVENT_KEY_UP:
      keys.send(events::key{.key = e.key.key, .pressed = false});
      break;
    }
  }
}

} // namespace window::systems
