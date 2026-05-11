module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <string_view>

export module sdl;

export namespace sdl {
namespace types {
using window = SDL_Window;
using event = SDL_Event;
using keycode = SDL_Keycode;
using windowFlags = SDL_WindowFlags;
using init_flags = SDL_InitFlags;
using properties = SDL_PropertiesID;
} // namespace types

inline auto init(types::init_flags flags) { return SDL_Init(flags); }
inline auto quit() { SDL_Quit(); }
inline auto poll_event(types::event *e) { return SDL_PollEvent(e); }
inline auto create_window(std::string_view title, uint32_t w, uint32_t h,
                          SDL_WindowFlags flags) {
  return SDL_CreateWindow(title.data(), static_cast<int>(w),
                          static_cast<int>(h), flags);
}
inline auto destroy_window(types::window *w) { SDL_DestroyWindow(w); }
inline auto get_window_properties(types::window *w) {
  return SDL_GetWindowProperties(w);
}
inline auto get_pointer_property(types::properties props, const char *name,
                                 void *default_value) {
  return SDL_GetPointerProperty(props, name, default_value);
}
inline auto get_number_property(types::properties props, const char *name,
                                int64_t default_value) {
  return SDL_GetNumberProperty(props, name, default_value);
}

namespace events {
constexpr auto quit = SDL_EVENT_QUIT;
constexpr auto key_down = SDL_EVENT_KEY_DOWN;
constexpr auto key_up = SDL_EVENT_KEY_UP;
constexpr auto win_resize = SDL_EVENT_WINDOW_RESIZED;
} // namespace events

namespace props {
constexpr auto x11_display = SDL_PROP_WINDOW_X11_DISPLAY_POINTER;
constexpr auto x11_window = SDL_PROP_WINDOW_X11_WINDOW_NUMBER;
constexpr auto wayland_display = SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER;
constexpr auto wayland_surface = SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER;
} // namespace props

namespace init_flags {
constexpr auto video = SDL_INIT_VIDEO;
}
namespace window_flags {
constexpr auto resizeable = SDL_WINDOW_RESIZABLE;
}
} // namespace sdl
