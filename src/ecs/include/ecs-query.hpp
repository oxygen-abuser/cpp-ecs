#pragma once

#include <entt/entt.hpp>
#include <tuple>

#include "ecs-system-context.hpp"
#include "ecs-world.hpp"
#include "entt/entity/fwd.hpp"

namespace ecs::query {

template <typename... Ts> struct with {};

template <typename... Ts> struct without {};

namespace details {
template <typename... Ts> struct query_param;

template <typename T> struct query_param<T> {
  using get = std::tuple<T>;
  using exclude = std::tuple<>;
};

template <typename... Ts> struct query_param<with<Ts...>> {
  using get = std::tuple<Ts...>;
  using exclude = std::tuple<>;
};

template <typename... Ts> struct query_param<without<Ts...>> {
  using get = std::tuple<>;
  using exclude = std::tuple<Ts...>;
};

template <> struct query_param<ecs::entity::entity> {
  using get = std::tuple<>;
  using exclude = std::tuple<>;
};

template <typename... Args>
using get_list = decltype(std::tuple_cat(
    std::declval<typename query_param<Args>::get>()...));

template <typename... Args>
using exclude_list = decltype(std::tuple_cat(
    std::declval<typename query_param<Args>::exclude>()...));

template <typename... Args>
constexpr bool has_entity = (std::is_same_v<Args, ecs::entity::entity> || ...);

template <typename Tuple> struct to_get;

template <typename... Ts> struct to_get<std::tuple<Ts...>> {
  using type = entt::get_t<Ts...>;
};

template <typename Tuple> struct to_exclude;

template <typename... Ts> struct to_exclude<std::tuple<Ts...>> {
  using type = entt::exclude_t<Ts...>;
};

template <typename GetTuple, typename ExcludeTuple> struct view_builder;

template <typename... Gets, typename... Excludes>
struct view_builder<std::tuple<Gets...>, std::tuple<Excludes...>> {
  static auto build(entt::registry &reg) {
    return reg.view<Gets...>(entt::exclude<Excludes...>);
  }
};

} // namespace details

template <typename... Args> class query {
  using get_tuple = details::get_list<Args...>;
  using exclude_tuple = details::exclude_list<Args...>;
  using view_type =
      entt::view<typename details::to_get<get_tuple>::type,
                 typename details::to_exclude<exclude_tuple>::type>;

public:
  static auto fetch(ecs::system::system_context &ctx) -> query<Args...> {
    auto &reg = ctx.world.world_registry_;

    query q{};
    q.view_ = details::view_builder<get_tuple, exclude_tuple>::build(
        ctx.world.world_registry_);

    return q;
  }

  template <typename Fn> auto for_each(Fn f) -> void {
    if constexpr (details::has_entity<Args...>) {
      for (auto &&cs : view_.each()) {
        std::apply(
            [&f](entt::entity e, auto &&...args) {
              f(ecs::entity::entity{.entt = e},
                std::forward<decltype(args)>(args)...);
            },
            cs);
      }
    } else {
      for (auto &&cs : view_.each()) {
        std::apply(
            [&f](auto, auto &&...args) {
              f(std::forward<decltype(args)>(args)...);
            },
            cs);
      }
    }
  }

  auto size() -> std::size_t { return view_.size(); }

private:
  view_type view_;
};
} // namespace ecs::query
