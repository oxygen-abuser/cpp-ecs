#pragma once

#include <functional>

namespace ecs::executor {
struct task_node {
  std::function<void()> work;
  std::vector<std::size_t> depends_on;
};

template <typename E>
concept executor_backend = requires(E e, std::vector<task_node> graph) {
  { e.run(graph) } -> std::same_as<void>;
};

class sequential_executor {
public:
  auto run(std::vector<task_node> &graph) -> void {
    for (auto& node : graph) {
      node.work();
    }
  }
};

} // namespace ecs::executor
