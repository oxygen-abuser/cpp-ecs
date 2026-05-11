module;
#include <bits/move.h>
export module window:plugin;

import ecs;
import :types;
import :init;
import :poll;
import :swap;

namespace window {
auto kill(ecs::resource::resource<ecs::app::exit_resource> exit) -> void {
  exit->should_exit = true;
}
auto window_closed(ecs::events::event_reader<events::window_close> close)
    -> bool {
  if (!close.read().empty()) {
    return true;
  }
  return false;
}

auto window_plugin::create(window_config &&config) -> window_plugin {
  window_plugin plugin;
  plugin.config_ = std::move(config);
  return plugin;
}

auto window_plugin::build(ecs::app::app &app) -> void {
  using bp = ecs::system::system_bind_point;
  namespace sch = ecs::schedulable;
  app
      // resources
      .insert_resource(config_)
      // events
      .add_event<events::window_close>()
      .add_event<events::window_resize>()
      .add_event<events::key>()
      .add_event<events::mouse_button>()
      .add_event<events::mouse_move>()
      // systems
      .add_system(bp::init, window::systems::window_init)
      .add_system(bp::update, sch::systems(kill).run_if(window_closed))
      .add_system(bp::pre_update, window::systems::poll)
      .add_system(bp::swap, window::systems::swap);
}

} // namespace window
