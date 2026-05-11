#pragma once

#include "ecs-events.hpp"
#include "ecs-system.hpp"
#include "ecs-world.hpp"
namespace ecs::app {

struct exit_resource {
  bool should_exit{false};
};

class app;

namespace details {
template <typename P>
concept plugin = requires(P p, app &a) {
  { p.build(a) } -> std::same_as<void>;
};
} // namespace details

class app {
public:
  static auto create() -> app {
    world::world w = world::world::create();
    w.insert_resource(events::event_registry{});
    w.insert_resource(exit_resource{});

    system::scheduler s = system::scheduler::create();

    app a;
    a.world_ = std::move(w);
    a.scheduler_ = std::move(s);

    return a;
  }

  auto init() -> void { scheduler_.init(world_); }
  auto tick() -> void { scheduler_.update(world_); }
  auto shutdown() -> void { scheduler_.shutdown(world_); }

  auto run() -> void {
    running_ = true;

    init();
    while (running_) {
      tick();
      running_ = !world_.resource<exit_resource>().should_exit;
    }
    shutdown();
  }

  auto quit() -> void { running_ = false; }

  template <typename Fn>
  auto add_system(system::system_bind_point bp, Fn fn,
                  std::optional<std::string> name = {}) -> app & {
    scheduler_.add_system(bp, std::forward<Fn>(fn));
    return *this;
  }

  auto add_system(system::system_bind_point bp,
                  schedulable::schedulable_set set) & -> app & {
    for (auto &s : set.schedulables_)
      scheduler_.add_system(bp, std::move(s));
    return *this;
  }

  auto add_system(system::system_bind_point bp,
                  schedulable::schedulable_set set) && -> app && {
    for (auto &s : set.schedulables_)
      scheduler_.add_system(bp, std::move(s));
    return std::move(*this);
  }

  // events
  template <typename T> auto add_event() & -> app & {
    auto &reg = world_.resource<events::event_registry>();
    reg.register_event<T>(world_);
    return *this;
  }

  // resources
  template <typename T> auto insert_resource(T &&value) & -> app & {
    world_.insert_resource(std::forward<T>(value));
    return *this;
  }

  // plugins
  template <typename P> auto add_plugin(P plugin) & -> app & {
    plugin.build(*this);
    return *this;
  }

  template <typename T> auto insert_resource(T &&value) && -> app && {
    world_.insert_resource(std::forward<T>(value));
    return std::move(*this);
  }

  template <typename Fn>
  auto add_system(system::system_bind_point bp, Fn fn,
                  std::optional<std::string> name = {}) && -> app && {
    scheduler_.add_system(bp, std::forward<Fn>(fn), std::move(name));
    return std::move(*this);
  }

  template <typename T> auto add_event() && -> app && {
    world_.resource<events::event_registry>().register_event<T>(world_);
    return std::move(*this);
  }

  template <typename P> auto add_plugin(P plugin) && -> app && {
    plugin.build(*this);
    return std::move(*this);
  }

  auto world() -> world::world & { return world_; }
  auto world() const -> world::world const & { return world_; }

  auto scheduler() -> system::scheduler & { return scheduler_; }
  auto scheduler() const -> system::scheduler const & { return scheduler_; }

private:
  system::scheduler scheduler_;
  world::world world_;

  bool running_{false};
};

} // namespace ecs::app
