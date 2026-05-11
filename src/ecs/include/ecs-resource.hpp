#pragma once
#include "ecs-system-context.hpp"
#include "ecs-world.hpp"
#include <functional>
#include <optional>
#include <type_traits>

namespace ecs::resource {

template <typename T> struct resource_const {
  using inner_value = std::remove_cvref_t<T>;
  T *ptr;

  static auto fetch(ecs::system::system_context &ctx) -> resource_const<T> {
    return {.ptr = &ctx.world.resource<T>()};
  }

  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }
};

template <typename T> struct resource {
  using inner_value = std::remove_cvref_t<T>;
  T *ptr;

  static auto fetch(ecs::system::system_context &ctx) -> resource<T> {
    return {.ptr = &ctx.world.resource<T>()};
  }

  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }
};

template <typename T> struct local {
  using inner_value = std::remove_cvref_t<T>;
  T *value{nullptr};
  T &operator*() { return *value; }
  T *operator->() { return value; }
};

template <typename T>
concept valid_resource = requires { typename T::inner_value; };

template <typename T>
  requires valid_resource<T>
struct if_res {

  auto should_run() const { return exists_; }

  auto operator->() -> typename T::inner_value * { return value_.ptr; }
  auto operator*() -> T & { return *value_; }

  static auto fetch(system::system_context &ctx) -> if_res {
    auto opt = ctx.world.get_resource<typename T::inner_value>();
    if (!opt)
      return {.exists_ = false};
    return {.value_ = T::fetch(ctx), .exists_ = true};
  }

  T value_;
  bool exists_;
};

} // namespace ecs::resource
