#include "ecs-world.hpp"
#include "ecs-events.hpp"

namespace ecs::world {

auto insert_default_resources(world w) -> void {}

auto world::create() -> world { return world{}; }

auto world::init() -> void {
  insert_resource(events::event_registry{});
}

auto world::update() -> void {}

} // namespace ecs::world
