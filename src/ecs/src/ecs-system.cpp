// ecs-system.cpp
#include "ecs-system.hpp"
#include "ecs-events.hpp"
#include "ecs-world.hpp"
#include <entt/graph/flow.hpp>
#include <span>

namespace ecs::system {

static auto build_graph(
    std::span<const schedulable::schedulable> systems
) -> entt::adjacency_matrix<entt::directed_tag> {
    entt::flow flow;
    for (std::size_t i = 0; i < systems.size(); ++i) {
        flow.bind(static_cast<entt::id_type>(i));
        for (auto& type : systems[i].set_.reads)
            flow.ro(static_cast<entt::id_type>(type.hash_code()));
        for (auto& type : systems[i].set_.writes)
            flow.rw(static_cast<entt::id_type>(type.hash_code()));
    }
    return flow.graph();
}

static auto run_stage(
    world::world& w,
    scheduler::stage& stage
) -> void {
    if (stage.systems.empty()) return;

    if (stage.graph_dirty) {
        stage.graph = build_graph(stage.systems);
        stage.graph_dirty = false;
    }

    // DFS respecting dependency order
    std::vector<bool> visited(stage.systems.size(), false);

    std::function<void(std::size_t)> run = [&](std::size_t v) {
        if (visited[v]) return;
        // run dependencies first
        for (auto [from, to] : stage.graph.in_edges(v))
            run(from);
        visited[v] = true;
        // check condition
        if (stage.systems[v].condition_ &&
            !stage.systems[v].condition_->run(w))
            return;
        // run systems
        for (auto& cfg : stage.systems[v].systems_)
            cfg.run(w);
    };

    for (std::size_t i = 0; i < stage.systems.size(); ++i)
        run(i);

    // flush commands
    for (auto& s : stage.systems)
        for (auto& cfg : s.systems_)
            cfg.apply(w);
}

auto scheduler::build_graphs() -> void {
    auto rebuild = [](stage& s) {
        if (s.graph_dirty && !s.systems.empty()) {
            s.graph = build_graph(s.systems);
            s.graph_dirty = false;
        }
    };
    rebuild(set_.init);
    rebuild(set_.pre_update);
    rebuild(set_.update);
    rebuild(set_.post_update);
    rebuild(set_.pre_render);
    rebuild(set_.render);
    rebuild(set_.swap);
    rebuild(set_.shutdown);
}

auto scheduler::create() -> scheduler { return scheduler{}; }

auto scheduler::init(world::world& world) -> void {
    run_stage(world, set_.init);
}

auto scheduler::update(world::world& world) -> void {
    run_stage(world, set_.pre_update);
    run_stage(world, set_.update);
    run_stage(world, set_.post_update);
    run_stage(world, set_.pre_render);
    run_stage(world, set_.render);
    run_stage(world, set_.swap);
    if (auto r = world.get_resource<events::event_registry>())
        r->get().swap_all(world);
}

auto scheduler::shutdown(world::world& world) -> void {
    run_stage(world, set_.shutdown);
}

} // namespace ecs::system
