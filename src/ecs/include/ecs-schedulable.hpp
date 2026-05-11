#pragma once

#include "ecs-access-set.hpp"
#include "ecs-system-traits.hpp"
#include <optional>
#include <vector>
namespace ecs::schedulable {

struct schedulable {
  std::vector<system::system_config> systems_;
  std::optional<system::condition_config> condition_;
  access::access_set set_;
  bool chained_{false};
};

struct schedulable_set {
  std::vector<schedulable> schedulables_;

  template <typename C>
    requires system::valid_condition<std::decay_t<C>>
  auto run_if(C &&cond) -> schedulable_set & {
    auto cond_cfg = system::condition_config::create(std::forward<C>(cond), "");
    for (auto &s : schedulables_) {
      if (s.condition_) {
        s.condition_ =
            system::and_condition(std::move(*s.condition_), cond_cfg);
      } else {
        s.condition_ = cond_cfg;
      }
    }
    return *this;
  }

  auto chain() -> schedulable_set &;
};

template <typename T> struct into_schedulable_set;

template <typename Fn>
  requires system::valid_system<Fn>
struct into_schedulable_set<Fn> {
  static auto convert(Fn fn) {
    schedulable s;
    auto config = system::system_config::create(fn, "");
    s.set_ = config.set_;
    s.systems_.push_back(std::move(config));
    return schedulable_set{.schedulables_ = {std::move(s)}};
  }
};

template <> struct into_schedulable_set<schedulable> {
  static auto convert(schedulable &&s) -> schedulable_set {
    return schedulable_set{.schedulables_ = {std::move(s)}};
  }
};

template <> struct into_schedulable_set<schedulable_set> {
  static auto convert(schedulable_set &&s) -> schedulable_set { return s; }
};

template <typename... Args> auto systems(Args &&...args) -> schedulable_set {
  schedulable_set result;
  schedulable current;

  auto flush_current = [&]() {
    if (!current.systems_.empty()) {
      result.schedulables_.push_back(std::move(current));
      current = schedulable{};
    }
  };

  (
      [&](auto &&arg) {
        using T = std::remove_cvref_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, schedulable_set>) {
          flush_current();
          for (auto &s : arg.schedulables_)
            result.schedulables_.push_back(std::move(s));
        } else if constexpr (std::is_same_v<T, schedulable>) {
          flush_current();
          result.schedulables_.push_back(std::move(arg));
        } else {
          // must be a plain system function
          auto config = system::system_config::create(
              std::forward<decltype(arg)>(arg), "");

          if (current.set_.conflicts(config.set_)) {
            flush_current();
          }

          current.set_.merge(config.set_);
          current.systems_.push_back(std::move(config));
        }
      }(std::forward<Args>(args)),
      ...);

  flush_current();
  return result;
}
} // namespace ecs::schedulable
