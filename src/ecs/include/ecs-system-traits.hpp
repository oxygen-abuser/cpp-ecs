#pragma once

#include <concepts>
#include <expected>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ecs-access-set.hpp"
#include "ecs-commands.hpp"
#include "ecs-resource.hpp"
#include "ecs-system-context.hpp"
#include "ecs-world.hpp"

namespace ecs::system {
template <typename T> struct is_local : std::false_type {};

template <typename T> struct is_local<resource::local<T>> : std::true_type {};

template <typename T> constexpr bool is_local_v = is_local<T>::value;

template <typename T>
concept world_fetchable = requires(system_context &ctx) {
  { T::fetch(ctx) } -> std::convertible_to<T>;
};

template <typename T>
concept guard_param = requires(const T &t) {
  { t.should_run() } -> std::same_as<bool>;
};

template <typename T>
concept system_param = is_local_v<std::remove_cvref_t<T>> ||
                       world_fetchable<std::remove_cvref_t<T>>;

template <typename Fn> struct function_traits;

template <typename Fn>
struct function_traits : function_traits<decltype(&Fn::operator())> {};

// function pointer
template <typename Ret, typename... Args>
  requires(system_param<Args> && ...)
struct function_traits<Ret (*)(Args...)> {
  using args = std::tuple<Args...>;
  using return_type = Ret;
};

// const operator() — regular lambdas
template <typename Class, typename Ret, typename... Args>
  requires(system_param<Args> && ...)
struct function_traits<Ret (Class::*)(Args...) const> {
  using args = std::tuple<Args...>;
  using return_type = Ret;
};

// mutable operator() — mutable lambdas
template <typename Class, typename Ret, typename... Args>
  requires(system_param<Args> && ...)
struct function_traits<Ret (Class::*)(Args...)> {
  using args = std::tuple<Args...>;
  using return_type = Ret;
};

template <typename T> struct inner_local;

template <typename T> struct inner_local<resource::local<T>> {
  using type = T;
};
template <typename T, typename Tuple>
struct tuple_contains : std::false_type {};

template <typename T, typename... Ts>
struct tuple_contains<T, std::tuple<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <typename T, typename Tuple>
constexpr bool tuple_contains_v = tuple_contains<T, Tuple>::value;

template <typename T, typename Ret>
concept valid_system_base = requires {
  typename function_traits<std::decay_t<T>>::args;
  typename function_traits<std::decay_t<T>>::return_type;
} && std::is_same_v<typename function_traits<T>::return_type, Ret>;

template <typename T>
concept valid_system = valid_system_base<T, void>;

template <typename T>
concept valid_condition =
    valid_system_base<T, bool> &&
    !tuple_contains_v<ecs::commands::commands &,
                      typename function_traits<std::decay_t<T>>::args>;

template <typename T, typename E>
concept valid_fallible_system = valid_system_base<T, std::expected<void, E>>;

template <typename T> using inner_local_t = typename inner_local<T>::type;
template <typename T> struct local_tuple {
  using type = std::tuple<>;
};

template <typename T> struct local_tuple<ecs::resource::local<T>> {
  using type = std::tuple<T>;
};

template <typename... Ts> struct extract_locals {
  using type = decltype(std::tuple_cat(
      std::declval<typename local_tuple<Ts>::type>()...));
};

template <typename T> struct unpack_locals {
  using type = std::tuple<>;
};

template <typename... Ts> struct unpack_locals<std::tuple<Ts...>> {
  using type = typename extract_locals<Ts...>::type;
};

template <std::size_t N, typename... Args>
consteval auto local_index_before() -> std::size_t {
  std::size_t count = 0;
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    ((count +=
      (I < N && is_local_v<std::tuple_element_t<I, std::tuple<Args...>>>)),
     ...);
  }(std::make_index_sequence<N>{});
  return count;
}

template <typename Tuple>
using unpack_locals_t = typename unpack_locals<Tuple>::type;


template <typename Args> auto make_access_set() -> ecs::access::access_set {
  return ecs::access::extract_access<Args>::get();
}


enum class system_bind_point {
  // initialization
  init,
  // main loop
  pre_update,
  update,
  post_update,
  pre_render,
  render,
  swap,
  // shutodown
  shutdown,

};

template <typename Ret> struct system_config_base {
  std::string name_;
  std::function<Ret(system_context &)> runner_;
  ecs::access::access_set set_;

  auto run(ecs::world::world &world) -> Ret {
    auto context = system_context{world, commands_};
    return runner_(context);
  }

  template <typename Fn>
    requires valid_system_base<Fn, Ret>
  static auto create(Fn fn, std::string &&name) -> system_config_base {
    using args_t = typename function_traits<Fn>::args;
    using locals_t = unpack_locals_t<args_t>;

    auto runner = [locals = locals_t{},
                   system = std::move(fn)](system_context &ctx) mutable {
      return dispatch<Fn, args_t, decltype(locals)>(system, ctx, locals);
    };

    system_config_base config;
    config.name_ = std::forward<std::string>(name);
    config.runner_ = std::move(runner);
    config.commands_ = commands::commands{};
    config.set_ = make_access_set<args_t>();

    return config;
  }

  auto apply(world::world &w) -> void { commands_.apply(w); }

private:
  template <typename Fn, typename Tuple, typename LocalsTuple>
  static auto dispatch(Fn &fn, system_context &ctx, LocalsTuple &locals)
      -> Ret {
    return []<typename... Args>(Fn &f, system_context &ctx, LocalsTuple &l,
                                std::type_identity<std::tuple<Args...>>) {
      return dispatch_impl<Fn, Args...>(
          f, ctx, l, std::make_index_sequence<sizeof...(Args)>{});
    }(fn, ctx, locals, std::type_identity<Tuple>{});
  }

  template <typename Fn, typename... Args, typename LocalsTuple,
            std::size_t... I>
  static auto dispatch_impl(Fn &fn, system_context &ctx, LocalsTuple &locals,
                            std::index_sequence<I...>) -> Ret {
    auto params =
        std::forward_as_tuple(resolve_param<I, Args...>(ctx, locals)...);
    bool should_run = true;
    (
        [&should_run, &params]<std::size_t Idx>() {
          using T = std::remove_cvref_t<
              std::tuple_element_t<Idx, std::tuple<Args...>>>;
          if constexpr (guard_param<T>) {
            should_run = should_run && std::get<Idx>(params).should_run();
          }
        }.template operator()<I>(),
        ...);
    if (!should_run) {
      if constexpr (std::is_void_v<Ret>) {
        return;
      } else {
        return Ret{};
      }
    }

    return std::apply(fn, std::move(params));
  }

  template <std::size_t I, typename... Args, typename LocalsTuple>
  static auto resolve_param(system_context &ctx, LocalsTuple &locals)
      -> decltype(auto) {
    using T = std::remove_cvref_t<std::tuple_element_t<I, std::tuple<Args...>>>;
    if constexpr (is_local_v<T>) {
      constexpr auto local_idx = local_index_before<I, Args...>();
      return ecs::resource::local<inner_local_t<T>>{
          &std::get<local_idx>(locals)};
    } else {
      return T::fetch(ctx);
    }
  }

private:
  ecs::commands::commands commands_;
};

using system_config = system_config_base<void>;
using condition_config = system_config_base<bool>;
template <typename E>
using fallible_config = system_config_base<std::expected<void, E>>;

auto and_condition(condition_config left, condition_config right)
    -> condition_config;

} // namespace ecs::system
