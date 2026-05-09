#pragma once
#include "ecs-system-context.hpp"
#include "ecs-world.hpp"

namespace ecs::resource {

template <typename T> struct resource_const {
  T *ptr;

  static auto fetch(ecs::system::system_context &ctx) -> resource_const<T> {
    return {.ptr = &ctx.world.resource<T>()};
  }

  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }
};

template <typename T> struct resource {
  T *ptr;

  static auto fetch(ecs::system::system_context &ctx) -> resource<T> {
    return {.ptr = &ctx.world.resource<T>()};
  }

  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }
};

template <typename T> struct local {
  T *value{nullptr};
  T &operator*() { return *value; }
  T *operator->() { return value; }
};

} // namespace ecs::resource
