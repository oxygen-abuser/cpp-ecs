#pragma once
#include "ecs-world.hpp"
#include "ecs-system-context.hpp"
#include <functional>
#include <vector>

namespace ecs::commands {
class commands {
public:
  template<typename ...Cs>
  auto spawn(Cs... cs) -> void {
    queue_.push_back([... cs = std::forward<Cs>(cs)](world::world &w) {
      auto e = w.create_entity();
      (w.add_component(e, cs), ...);
    });
  }

  auto despawn(entity::entity e) ->void {
    queue_.push_back([e](world::world &w) { w.destroy(e); });
  }

  template<typename R>
  auto insert_resource(R&& res) -> void {
    queue_.push_back([r = std::forward<R>(res)](world::world &w) {
      w.insert_resource(std::move(r));
    });
  }

  auto apply(world::world &w) -> void {
    for (auto &cmd : queue_) {
      cmd(w);
    }
    queue_.clear();
  }

  static auto fetch(system::system_context &ctx) -> commands & {
    return ctx.commands;
  }

private:
  using command_fn_t = std::function<void(world::world &)>;
  std::vector<command_fn_t> queue_;
};
} // namespace ecs::commands
