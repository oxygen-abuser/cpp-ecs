#pragma once

namespace ecs::world {
  class world;
}
namespace ecs::commands {
  class commands;
}

namespace ecs::system {
struct system_context {
  world::world &world;
  commands::commands &commands;
};
} // namespace ecs::system
