#pragma once

#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <any>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>

#include <entt/entt.hpp>

namespace ecs::entity {
struct entity {
  entt::entity entt;

  auto operator<=>(const entity &) const = default;
  bool operator==(const entity &) const = default;
};

constexpr entity null = entity{entt::null};
}; // namespace ecs::entity

namespace ecs::query {
template <typename... Args> class query; // forward declare
}

namespace ecs::world {

class world {
public:
  static auto create() -> world;
  auto init() -> void;
  auto update() -> void;

  template <typename T> auto insert_resource(T &&value) -> void {
    assert(resources_.find(typeid(T)) == resources_.end() &&
           "resource already exists");
    resources_[typeid(T)] = std::make_unique<std::any>(std::forward<T>(value));
  }
  template <typename T> auto remove_resource() -> bool {
    auto it = resources_.find(typeid(T));
    if (it == resources_.end())
      return false;
    resources_.erase(it);
    return true;
  }

  template <typename T>
  auto get_resource() -> std::optional<std::reference_wrapper<T>> {
    auto it = resources_.find(typeid(T));
    if (it == resources_.end())
      return std::nullopt;
    return std::ref(std::any_cast<T &>(*it->second));
  }

  template <typename T>
  auto get_resource_const() const
      -> std::optional<std::reference_wrapper<const T>> {
    auto it = resources_.find(typeid(T));
    if (it == resources_.end())
      return std::nullopt;
    return std::cref(std::any_cast<const T &>(*it->second));
  }

  template <typename T> auto resource() -> T & {
    auto it = resources_.find(typeid(T));
    assert(it != resources_.end());
    return std::any_cast<T &>(*it->second);
  }

  template <typename T> auto resource_const() const -> const T & {
    auto it = resources_.find(typeid(T));
    assert(it != resources_.end());
    return std::any_cast<const T &>(*it->second);
  }

  template <typename C> auto get_component(ecs::entity::entity e) -> C & {
    auto res = world_registry_.try_get<C>(e.entt);
    assert(res != nullptr && "entity does not have component");
    return *res;
  }

  template <typename C>
  auto get_component_const(ecs::entity::entity e) -> C const & {
    auto res = world_registry_.try_get<C>(e.entt);
    assert(res != nullptr && "entity does not have component");
    return *res;
  }

  template <typename C>
  auto add_component(ecs::entity::entity e, C &&component) {
    world_registry_.emplace<std::remove_cvref_t<C>>(e.entt,
                                                    std::forward<C>(component));
  };

  template <typename C, typename... Args>
  auto emplace_component(ecs::entity::entity e, Args &&...args)
      -> decltype(auto) {
    return world_registry_.emplace<C>(e.entt, std::forward<Args>(args)...);
  }

  template <typename C> auto has_component(ecs::entity::entity e) -> bool {
    return world_registry_.all_of<C>(e.entt);
  }

  auto create_entity() -> ecs::entity::entity {
    auto raw = world_registry_.create();
    return {
        .entt = raw,
    };
  };

  auto is_valid(entity::entity e) -> bool {
    return world_registry_.valid(e.entt);
  }

  auto destroy(entity::entity e) -> void { world_registry_.destroy(e.entt); }

  template <typename... Cs> auto count() const -> std::size_t {
    return world_registry_.view<Cs...>().size();
  }

  auto each(auto fn) -> void {
    for (auto e : world_registry_.view<entt::entity>()) {
      fn(entity::entity{.entt = e});
    }
  }

private:
  std::unordered_map<std::type_index, std::unique_ptr<std::any>> resources_;
  entt::registry world_registry_{};

private:
  template <typename... Args> friend class ecs::query::query;
};

} // namespace ecs::world
