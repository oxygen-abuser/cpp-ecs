
#include "ecs-system.hpp"
#include <optional>
#include <vector>
namespace ecs::schedulable {

struct schedulable {
  std::vector<system::system_config> systems_;
  std::optional<system::condition_config> condition_;
  bool chained_{false};
};

struct schedulable_set {
  std::vector<schedulable> schedulables_;

  template <typename C>
    requires system::valid_condition<C>
  auto run_if(C &&cond) -> schedulable_set & {
    auto cond_cfg = system::condition_config::create(std::forward<C>(cond), "");
    for (auto &s : schedulables_) {
      if (s.condition_) {
        s.condition_ =
            system::and_condition(std::move(*s.condition_), cond_cfg);
      } else {
        s.condition_ = std::move(cond_cfg);
      }
    }
    return *this;
  }

  auto chain() -> schedulable_set & {
    for (auto &s : schedulables_) {
      s.chained_ = true;
    }
    return *this;
  }
};

template <typename T> struct into_schedulable_set;

template <typename Fn>
  requires system::valid_system<Fn>
struct into_schedulable_set<Fn> {
  static auto convert(Fn fn) {
    schedulable s;
    s.systems_.push_back(system::system_config::create(fn, ""));
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
  (
      [&result](auto &&arg) {
        using T = std::remove_cvref_t<decltype(arg)>;
        auto set =
            into_schedulable_set<T>::convert(std::forward<decltype(arg)>(arg));
        for (auto &s : set.schedulables_)
          result.schedulables_.push_back(std::move(s));
      }(std::forward<Args>(args)),
      ...);
  return result;
}
} // namespace ecs::schedulable
