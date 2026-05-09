#pragma once
#include "ecs-resource.hpp"
#include "ecs-system-context.hpp"
#include "ecs-world.hpp"
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace ecs::events {

template <typename T> class event {
public:
  auto swap_and_clear() -> void {
    _old_buffer = std::move(_current_buffer);
    _current_buffer.clear();
  }

  auto old_buffer() const -> std::span<const T> { return {_old_buffer}; }

  auto send(T &&e) -> void { _current_buffer.push_back(std::forward<T>(e)); }

private:
  std::vector<T> _current_buffer;
  std::vector<T> _old_buffer;
};

class event_registry {
public:
  auto swap_all(world::world &world) -> void {
    for (auto &f : _swap_functions)
      f(world);
  }

  template <typename T> auto register_event(ecs::world::world &world) -> void {
    world.insert_resource(event<T>{});
    _swap_functions.push_back(
        [](world::world &w) { w.resource<event<T>>().swap_and_clear(); });
  }

private:
  using swap_fn_t = std::function<void(world::world &)>;
  std::vector<swap_fn_t> _swap_functions;
};

template <typename T> class event_reader {
public:
  auto read() const -> std::span<const T> { return _events->old_buffer(); }

  auto begin() const { return _events->old_buffer().begin(); }
  auto end() const { return _events->old_buffer().end(); }

  static auto fetch(ecs::system::system_context &ctx) -> event_reader<T> {
    event_reader<T> reader;
    reader._events = &ctx.world.resource_const<event<T>>();
    return reader;
  }

private:
  const event<T> *_events;
};

template <typename T> class event_writer {
public:
  auto send(T &&e) -> void { _events->send(std::forward<T>(e)); }

  static auto fetch(ecs::system::system_context &ctx) -> event_writer<T> {
    event_writer<T> writer;
    writer._events = &ctx.world.resource<event<T>>();
    return writer;
  }

private:
  event<T> *_events;
};

} // namespace ecs::events
