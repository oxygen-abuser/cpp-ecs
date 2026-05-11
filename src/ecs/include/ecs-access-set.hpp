#pragma once

#include "ecs-query.hpp"
#include "ecs-resource.hpp"

#include "ecs-world.hpp"

#include <typeindex>
#include <unordered_set>
namespace ecs::access {

struct access_set {
  std::unordered_set<std::type_index> reads;
  std::unordered_set<std::type_index> writes;

  auto conflicts(const access_set &other) -> bool {
    for (auto &w : writes) {
      if (other.writes.contains(w) || other.reads.contains(w)) {
        return true;
      }
    }

    for (auto &w : other.writes) {
      if (reads.contains(w)) {
        return true;
      }
    }

    return false;
  }

  auto merge(const access_set &other) -> void {
    reads.insert(other.reads.begin(), other.reads.end());
    writes.insert(other.writes.begin(), other.writes.end());
  }
};

template <typename T> struct param_access {
  static auto get(access_set &) -> void {}
};

template <typename T> struct param_access<ecs::resource::resource<T>> {
  static auto get(access_set &set) -> void { set.writes.insert(typeid(T)); }
};

template <typename T> struct param_access<ecs::resource::resource_const<T>> {
  static auto get(access_set &set) -> void { set.reads.insert(typeid(T)); }
};

// if_res<resource<T>> — write access (same as resource<T>)
template <typename T>
struct param_access<resource::if_res<resource::resource<T>>> {
  static auto get(access_set &s) -> void { s.writes.insert(typeid(T)); }
};

// if_res<resource_const<T>> — read access
template <typename T>
struct param_access<resource::if_res<resource::resource_const<T>>> {
  static auto get(access_set &s) -> void { s.reads.insert(typeid(T)); }
};

// local<T> — no shared access, per-system
template <typename T> struct param_access<resource::local<T>> {
  static auto get(access_set &) -> void {}
};

// commands — no shared resource access
template <> struct param_access<commands::commands> {
  static auto get(access_set &) -> void {}
};

template <typename C> struct arg_access {
  static auto get(access_set &set) -> void { set.writes.insert(typeid(C)); }
};

template <typename C> struct arg_access<const C> {
  static auto get(access_set &set) -> void { set.reads.insert(typeid(C)); }
};

template <typename... Cs> struct arg_access<query::with<Cs...>> {
  static auto get(access_set &set) -> void {
    (set.reads.insert(typeid(Cs)), ...);
  }
};

template <typename... Cs> struct arg_access<query::without<Cs...>> {
  static auto get(access_set &set) -> void {
    (set.reads.insert(typeid(Cs)), ...);
  }
};

template <> struct arg_access<ecs::entity::entity> {
  static auto get(access_set &) -> void {}
};

template <typename... Args> struct param_access<ecs::query::query<Args...>> {
  static auto get(access_set &set) -> void {
    (arg_access<Args>::get(set), ...);
  }
};

template <typename Tuple> struct extract_access;

template <typename... Args> struct extract_access<std::tuple<Args...>> {
  static auto get() -> access_set {
    access_set s;
    (param_access<std::remove_cvref_t<Args>>::get(s), ...);
    return s;
  }
};

} // namespace ecs::access
