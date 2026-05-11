module;
#include "bgfx/defines.h"
#include <SDL3/SDL_log.h>
#include <bgfx/bgfx.h>

export module window:init;

import ecs;
import sdl;

import :types;

namespace window::systems {

auto window_init(ecs::commands::commands &commands,
                 ecs::resource::resource<window::window_config> config)
    -> void {
  sdl::init(sdl::init_flags::video);
  auto window =
      sdl::create_window(config->title.c_str(), config->width, config->height,
                         sdl::window_flags::resizeable);

  auto props = sdl::get_window_properties(window);

  auto wayland_display =
      sdl::get_pointer_property(props, sdl::props::wayland_display, nullptr);
  auto wayland_surface =
      sdl::get_pointer_property(props, sdl::props::wayland_surface, nullptr);
  auto x11_display =
      sdl::get_pointer_property(props, sdl::props::x11_display, nullptr);
  auto x11_window = sdl::get_number_property(props, sdl::props::x11_window, 0);

  SDL_Log("wayland_display: %p", wayland_display);
  SDL_Log("wayland_surface: %p", wayland_surface);
  SDL_Log("x11_display: %p", x11_display);
  SDL_Log("x11_window: %lld", (long long)x11_window);
  SDL_Log("win: %p", (void *)window);

  // bgfx::Init init{};
  // init.type = bgfx::RendererType::Vulkan;
  // init.platformData.ndt = x11_display;
  // init.platformData.nwh = (void *)(uintptr_t)x11_window;
  // init.platformData.type = bgfx::NativeWindowHandleType::Default;
  // init.resolution.width = config->width;
  // init.resolution.height = config->height;
  // init.resolution.reset = BGFX_RESET_VSYNC;
  // bgfx::init(init);

  commands.insert_resource(window::window_resource{.window = window,
                                                   .width = config->width,
                                                   .height = config->height,
                                                   .should_close = false});
}

} // namespace window::systems
