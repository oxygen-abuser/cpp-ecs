// ecs-scheduler.hpp
#pragma once
#include "ecs-schedulable.hpp"
#include "ecs-system-traits.hpp"
#include <entt/graph/adjacency_matrix.hpp>
#include <entt/graph/flow.hpp>

namespace ecs::system {

class scheduler {
public:
  struct stage {
    std::vector<schedulable::schedulable> systems;
    entt::adjacency_matrix<entt::directed_tag> graph;
    bool graph_dirty{true}; // rebuild when systems added
  };

private:
  struct scheduler_set {
    stage init;
    stage pre_update;
    stage update;
    stage post_update;
    stage pre_render;
    stage render;
    stage swap;
    stage shutdown;
  };

public:
  static auto create() -> scheduler;

  auto build_graphs() -> void;

  auto init(ecs::world::world &world) -> void;
  auto update(ecs::world::world &world) -> void;
  auto shutdown(ecs::world::world &world) -> void;

  auto add_system(system_bind_point bp, schedulable::schedulable s) -> void {
    stage_for(bp).systems.push_back(std::move(s));
    stage_for(bp).graph_dirty = true;
  }

  auto add_system(system_bind_point bp, schedulable::schedulable_set set)
      -> void {
    for (auto &s : set.schedulables_)
      add_system(bp, std::move(s));
  }

  template <typename Fn>
    requires valid_system<Fn>
  auto add_system(system_bind_point bp, Fn fn,
                  std::optional<std::string> name = {}) -> void {
    auto system_name = name ? std::move(*name) : std::string{};
    schedulable::schedulable s;
    s.systems_.push_back(
        system_config::create(std::forward<Fn>(fn), std::move(system_name)));
    s.set_ = s.systems_.back().set_;
    add_system(bp, std::move(s));
  }

private:
  auto stage_for(system_bind_point bp) -> stage & {
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
    __builtin_unreachable();
  }

  scheduler_set set_;
};

} // namespace ecs::system
