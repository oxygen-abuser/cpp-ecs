#include "ecs-world.hpp"
#include "ecs.hpp"

// components
struct position {
  float x, y, z;
};
struct velocity {
  float dx, dy, dz;
};
struct camera {};
struct collider {
  float radius;
};

// resources
struct time_res {
  float delta;
};
struct config {
  int max_entities;
};

// ── systems covering every param combination ──────────────────────────────

// 1. no params
auto empty_system() -> void {}

// 2. single local
auto local_only(ecs::resource::local<int> counter) -> void { ++(*counter); }

// 3. single resource (const)
auto res_const_only(ecs::resource::resource_const<time_res> time) -> void {
  auto _ = time->delta;
}

// 4. single resource (mutable)
auto res_mut_only(ecs::resource::resource<config> cfg) -> void {

  cfg->max_entities = 100;
}

// 5. multiple locals
auto multi_local(ecs::resource::local<int> frame,
                 ecs::resource::local<float> accum) -> void {
  ++(*frame);
  *accum += 0.016f;
}

// 6. local + res
auto local_and_res(ecs::resource::local<int> counter,
                   ecs::resource::resource_const<time_res> time) -> void {
  *counter += static_cast<int>(time->delta);
}

// 7. res + res_mut
auto res_and_res_mut(ecs::resource::resource_const<time_res> time,
                     ecs::resource::resource<config> cfg) -> void {
  cfg->max_entities += static_cast<int>(time->delta);
}

// 8. query only — bare components
auto query_bare(ecs::query::query<position, const velocity> q) -> void {
  q.for_each([](position &p, const velocity &v) {
    p.x += v.dx;
    p.y += v.dy;
    p.z += v.dz;
  });
}

// 9. query with entity
auto query_with_entity(ecs::query::query<ecs::entity::entity, position> q)
    -> void {
  q.for_each([](ecs::entity::entity e, position &p) {
    p.x = static_cast<float>(entt::to_integral(e.entt));
  });
}

// 10. query with With filter
auto query_with_filter(ecs::query::query<position, ecs::query::with<camera>> q)
    -> void {
  q.for_each([](position &p) { p.x = 0.f; });
}

// 11. query with Without filter
auto query_without_filter(
    ecs::query::query<position, ecs::query::without<collider>> q) -> void {
  q.for_each([](position &p) { p.y = 0.f; });
}

// 12. query with With + Without
auto query_with_and_without(
    ecs::query::query<position, ecs::query::with<camera>,
                      ecs::query::without<collider>>
        q) -> void {
  q.for_each([](position &p) { p.z = 0.f; });
}

// 13. query + entity + With + Without + const
auto query_all_filters(
    ecs::query::query<ecs::entity::entity, position, const velocity,
                      ecs::query::with<camera>, ecs::query::without<collider>>
        q) -> void {
  q.for_each([](ecs::entity::entity e, position &p, const velocity &v) {
    p.x += v.dx * static_cast<float>(entt::to_integral(e.entt));
  });
}

// 14. query + res
auto query_and_res(ecs::query::query<position, const velocity> q,
                   ecs::resource::resource_const<time_res> time) -> void {
  q.for_each(
      [&time](position &p, const velocity &v) { p.x += v.dx * time->delta; });
}

// 15. query + local
auto query_and_local(ecs::query::query<position> q,
                     ecs::resource::local<int> frame) -> void {
  ++(*frame);
  q.for_each([](position &p) { p.x = 0.f; });
}

// 16. local + query + local (locals interleaved)
auto interleaved_locals(ecs::resource::local<int> pre_count,
                        ecs::query::query<position, const velocity> q,
                        ecs::resource::local<float> accum,
                        ecs::resource::resource_const<time_res> time) -> void {
  ++(*pre_count);
  q.for_each([&](position &p, const velocity &v) {
    p.x += v.dx * time->delta;
    *accum += v.dx;
  });
}

// 17. every bind point covered
auto init_system() -> void {}
auto pre_update_system() -> void {}
auto update_system() -> void {}
auto post_update_system() -> void {}
auto pre_render_system() -> void {}
auto render_system() -> void {}
auto swap_system() -> void {}
auto shutdown_system() -> void {}

// auto main() -> int {
//   auto world = ecs::world::world::create();
//   world.insert_resource(time_res{.delta = 0.016f});
//   world.insert_resource(config{.max_entities = 50});

//   // spawn some entities
//   auto e1 = world.create_entity();
//   auto e2 = world.create_entity();
//   world.add_component(e1, position{0.f, 0.f, 0.f});
//   world.add_component(e1, velocity{1.f, 0.f, 0.f});
//   world.add_component(e1, camera{});
//   world.add_component(e2, position{5.f, 5.f, 5.f});
//   world.add_component(e2, velocity{0.f, 1.f, 0.f});
//   world.add_component(e2, collider{1.f});

//   auto sched = ecs::system::scheduler::create();

//   using bp = ecs::system::system_bind_point;

//   // every bind point
//   sched.add_system(bp::init, init_system, "init");
//   sched.add_system(bp::pre_update, pre_update_system, "pre_update");
//   sched.add_system(bp::update, update_system, "update");
//   sched.add_system(bp::post_update, post_update_system, "post_update");
//   sched.add_system(bp::pre_render, pre_render_system, "pre_render");
//   sched.add_system(bp::render, render_system, "render");
//   sched.add_system(bp::swap, swap_system, "swap");
//   sched.add_system(bp::shutdown, shutdown_system, "shutdown");

//   // every param combination
//   sched.add_system(bp::update, empty_system, "empty");
//   sched.add_system(bp::update, local_only, "local_only");
//   sched.add_system(bp::update, res_const_only, "res_const_only");
//   sched.add_system(bp::update, res_mut_only, "res_mut_only");
//   sched.add_system(bp::update, multi_local, "multi_local");
//   sched.add_system(bp::update, local_and_res, "local_and_res");
//   sched.add_system(bp::update, res_and_res_mut, "res_and_res_mut");
//   sched.add_system(bp::update, query_bare, "query_bare");
//   sched.add_system(bp::update, query_with_entity, "query_with_entity");
//   sched.add_system(bp::update, query_with_filter, "query_with_filter");
//   sched.add_system(bp::update, query_without_filter, "query_without_filter");
//   sched.add_system(bp::update, query_with_and_without,
//                    "query_with_and_without");
//   sched.add_system(bp::update, query_all_filters, "query_all_filters");
//   sched.add_system(bp::update, query_and_res, "query_and_res");
//   sched.add_system(bp::update, query_and_local, "query_and_local");
//   sched.add_system(bp::update, interleaved_locals, "interleaved_locals");

//   sched.init(world);
//   sched.update(world);
//   sched.shutdown(world);

//   return 0;
// }
