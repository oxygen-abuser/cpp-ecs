#include "ecs-schedulable.hpp"

namespace ecs::schedulable {

auto schedulable_set::chain() -> schedulable_set & {
  for (auto &s : schedulables_) {
    s.chained_ = true;
  }
  return *this;
}

} // namespace ecs::schedulable
