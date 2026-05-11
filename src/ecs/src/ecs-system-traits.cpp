#include "ecs-system-traits.hpp"

namespace ecs::system {

auto and_condition(condition_config left, condition_config right)
    -> condition_config {
  condition_config c;
  c.runner_ = [left = std::move(left),
               right = std::move(right)](system_context &ctx) mutable {
    return left.run(ctx.world) && right.run(ctx.world);
  };
  return c;
}

} // namespace ecs::system
