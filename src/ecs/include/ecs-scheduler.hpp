#pragma once

#include "ecs-schedulable.hpp"
#include "ecs-system-traits.hpp"

namespace ecs::system {

class scheduler {
private:
  struct scheduler_set {
    std::vector<schedulable::schedulable> init;
    std::vector<schedulable::schedulable> pre_update;
    std::vector<schedulable::schedulable> update;
    std::vector<schedulable::schedulable> post_update;
    std::vector<schedulable::schedulable> pre_render;
    std::vector<schedulable::schedulable> render;
    std::vector<schedulable::schedulable> swap;
    std::vector<schedulable::schedulable> shutdown;
  };

public:
  static auto create() -> scheduler;

  auto init(ecs::world::world &world) -> void;
  auto update(ecs::world::world &world) -> void;
  auto shutdown(ecs::world::world &world) -> void;

  auto add_system(system_bind_point bind_point, schedulable::schedulable sched)
      -> void {
    vec_for(bind_point).push_back(std::move(sched));
  }

  auto add_system(system_bind_point bp, schedulable::schedulable_set set)
      -> void {
    for (auto &s : set.schedulables_)
      vec_for(bp).push_back(std::move(s));
  }

  template <typename Fn>
    requires valid_system<Fn>
  auto add_system(system_bind_point bind_point, Fn fn,
                  std::optional<std::string> name = {}) -> void {
    auto system_name = name ? std::move(*name) : std::string{};
    schedulable::schedulable s;
    s.systems_.push_back(
        system_config::create(std::forward<Fn>(fn), std::move(system_name)));
    add_system(bind_point, std::move(s));
  }

private:
  auto vec_for(system_bind_point bp)
      -> std::vector<schedulable::schedulable> & {
    switch (bp) {
    case system_bind_point::init:
      return set_.init;
    case system_bind_point::pre_update:
      return set_.pre_update;
    case system_bind_point::update:
      return set_.update;
    case system_bind_point::post_update:
      return set_.post_update;
    case system_bind_point::pre_render:
      return set_.pre_render;
    case system_bind_point::render:
      return set_.render;
    case system_bind_point::swap:
      return set_.swap;
    case system_bind_point::shutdown:
      return set_.shutdown;
    }
  }

  scheduler_set set_;
};

} // namespace ecs::system
