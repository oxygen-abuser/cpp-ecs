#include "ecs-system.hpp"
#include "ecs-events.hpp"
#include "ecs-world.hpp"
#include <algorithm>
#include <functional>

namespace ecs::system {

static auto update_set(world::world &w, std::span<system_config> set) {
  auto run = [&w](system_config &config) { config.run(w); };
  auto apply_commands = [&w](system_config &config) { config.apply(w); };
  std::ranges::for_each(set, run);
  std::ranges::for_each(set, apply_commands);
}

auto scheduler::create() -> scheduler { return scheduler{}; }

auto scheduler::init(ecs::world::world &world) -> void {
  update_set(world, set_.init);
}

auto scheduler::update(ecs::world::world &world) -> void {
  update_set(world, set_.pre_update);
  update_set(world, set_.update);
  update_set(world, set_.post_update);
  update_set(world, set_.pre_render);
  update_set(world, set_.render);
  update_set(world, set_.swap);

  if (auto r = world.get_resource<events::event_registry>()) {
    r->get().swap_all(world);
  }
}

auto scheduler::shutdown(ecs::world::world &world) -> void {
  update_set(world, set_.shutdown);
}

} // namespace ecs::system
